/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    MtConnect.cpp

Abstract:
    Message Transport class - receive respond implementation

Author:
    Uri Habusha (urih) 11-Aug-99

Environment:
    Platform-independent,

--*/

#include <libpch.h>
#include <singelton.h>
#include <utf8.h>
#include <cm.h>
#include "Mt.h"
#include "Mtp.h"
#include "MtObj.h"


#include "mtresponse.tmh"

const char xHttpScheme[] = "http://";
const char xHttpHeaderTerminater[] = "\r\n\r\n";

//
// Class that holds mapping from http error code to
// handler function
//
class CHttpStatusCodeMapper
{
public:
	CHttpStatusCodeMapper()
	{
		m_StatusMapping[100] = &CMessageTransport::OnHttpDeliveryContinute;


		m_StatusMapping[200] = &CMessageTransport::OnHttpDeliverySuccess;
		m_StatusMapping[201] = &CMessageTransport::OnHttpDeliverySuccess;
		m_StatusMapping[202] = &CMessageTransport::OnHttpDeliverySuccess;
		m_StatusMapping[203] = &CMessageTransport::OnHttpDeliverySuccess;
		m_StatusMapping[204] = &CMessageTransport::OnHttpDeliverySuccess;
		m_StatusMapping[205] = &CMessageTransport::OnHttpDeliverySuccess;

		m_StatusMapping[300] = &CMessageTransport::OnRedirection;
		m_StatusMapping[301] = &CMessageTransport::OnRedirection;
		m_StatusMapping[302] = &CMessageTransport::OnRedirection;
		m_StatusMapping[303] = &CMessageTransport::OnRedirection;
		m_StatusMapping[305] = &CMessageTransport::OnRedirection;
		m_StatusMapping[307] = &CMessageTransport::OnRedirection;

		m_StatusMapping[408] = &CMessageTransport::OnRetryableHttpError;
		
		m_StatusMapping[500] = &CMessageTransport::OnRetryableHttpError;
		m_StatusMapping[501] = &CMessageTransport::OnRetryableHttpError;
		m_StatusMapping[502] = &CMessageTransport::OnRetryableHttpError;
		m_StatusMapping[503] = &CMessageTransport::OnRetryableHttpError;
		m_StatusMapping[504] = &CMessageTransport::OnRetryableHttpError;


		//
		// error code 403 has special setting in regsitry for configure it behaviour
		//
		DWORD HttpRetryOnError403 = 0;
		CmQueryValue(
				RegEntry(NULL, L"HttpRetryOnError403"),
				&HttpRetryOnError403
				);


		if(HttpRetryOnError403 != 0)
		{
			m_StatusMapping[403] = &CMessageTransport::OnRetryableHttpError;									
		}


	}

public:
	typedef void (CMessageTransport::* StatusCodeHandler) (USHORT StatusCode);
	StatusCodeHandler operator[](USHORT StatusCode)  const
	{
		std::map<int, StatusCodeHandler>::const_iterator it =  m_StatusMapping.find(StatusCode);
		if(it ==  m_StatusMapping.end()	 )
			return &CMessageTransport::OnAbortiveHttpError;

		return it->second;
	}
	
private:
	std::map<int, StatusCodeHandler> m_StatusMapping;
};



//
// Class that holds the timeout(miliseconds) to wait
//  before next trial to  requeue packet if previous requeue failed.
//
class CRequeueTimeOut
{
public:
	DWORD GetTimeout() const
	{
		return m_timeout;
	}

private:
	CRequeueTimeOut()
	{
		CmQueryValue(
					RegEntry(NULL, L"RequeueTimeOut", 30000),
					&m_timeout
					);	
	}
private:
	friend CSingelton<CRequeueTimeOut>;
	DWORD m_timeout;
};



void CMessageTransport::HandleExtraResponse(void)
{
    ASSERT(m_responseOv.IsMoreResponsesExistInBuffer());
    //
    // Initialize the Overlapped with receive response call back routines
    //
    m_responseOv =  EXOVERLAPPED(
                                ReceiveResponseHeaderSucceeded,
                                ReceiveResponseFailed
                                );


    //
    // In previous phase we read more than we needed. Copy the spare data to the head of
    // the buffer, update the counter and behave like next read is completed
    //
    memcpy(
        m_responseOv.Header(),
        m_responseOv.Header() + m_responseOv.m_ProcessedSize,
        m_responseOv.m_HeaderValidBytes - m_responseOv.m_ProcessedSize
        );


    m_responseOv.InternalHigh = m_responseOv.m_HeaderValidBytes - m_responseOv.m_ProcessedSize;
    m_responseOv.m_HeaderValidBytes = 0;

    R<CMessageTransport> ar = SafeAddRef(this);

    m_responseOv.SetStatus(STATUS_SUCCESS);
    ExPostRequest(&m_responseOv);

    ar.detach();
}


DWORD CMessageTransport::FindEndOfResponseHeader(LPCSTR buf, DWORD length)
{
    if (4 > length)
        return 0;

    for(DWORD i = 0; i < length - 3; ++i)
    {
        if (buf[i] != '\r')
            continue;

        if ((buf[i+1] == '\n') && (buf[i+2] == '\r') && (buf[i+3] == '\n'))
        {
            return (i + 4);
        }
    }

    return 0;
}



xstr_t CMessageTransport::GetNewLocation(LPCSTR p, DWORD length)
/*++

Routine Description:
    Return the redirected location from the http response. Called when the http response
	30x accepted , which means that the target url was redirected to other url.

Arguments:
    p - Point to http response header start.
	length - The length of the http respose header.

Returned Value:
    The redirected url or empty xstr_t if not found.

Note:
	The new url is specified in the Location header

--*/
{
	const char xLocation[] = "\r\nLocation:";
	const char xHeaderTerminator[] = "\r\n";

	const LPCSTR pEnd = p + length;
	p = std::search(
					p,
					pEnd,
					xLocation,
					xLocation + STRLEN(xLocation)
					);
	if(p == pEnd)
		return xstr_t();

	const LPCSTR pStartLocation = p + STRLEN(xLocation);
	p = std::search(
		pStartLocation,
		pEnd,
		xHeaderTerminator,
		xHeaderTerminator + STRLEN(xHeaderTerminator)
		);

	if(p == pEnd)
		return xstr_t();

	return xstr_t(pStartLocation, p - pStartLocation);			
}



DWORD CMessageTransport::GetContentLength(LPCSTR p, DWORD length, USHORT HttpStatus)
{
	const char xContentlength[] = "Content-Length:";

    const LPCSTR pEnd = p + length - 4;

    //
    // HTTP header must terminate with '\r\n\r\n'. We already parse
    // the header and find it as a legal HTTP header.
    //
    ASSERT(length >= 4);
    ASSERT(strncmp(pEnd, xHttpHeaderTerminater, 4) == 0);


    while (p < pEnd)
    {
        if (_strnicmp(p, xContentlength , STRLEN(xContentlength)) == 0)
            return atoi(p + STRLEN(xContentlength));

        for(;;)
        {
            if ((p[0] == '\r') && (p[1] == '\n'))
            {
                p += 2;
                break;
            }

            ++p;
        }
    }

	//
	// BUGBUG:workaround, since IIS send 100 response without Content-Length field
	//												Uri Habusha, 16-May-200
	//
    if (HttpStatus == 100)
        return 0;

    //
    // Response header doesn't contain 'Content-Length' field.
    //
    throw exception();
}


void WINAPI CMessageTransport::TimeToResponseTimeout(CTimer* pTimer)
{
    R<CMessageTransport> pmt = CONTAINING_RECORD(pTimer, CMessageTransport, m_responseTimer);

    TrERROR(NETWORKING, "Failed to receive response in time, shutting down.pmt=0x%p", pmt.get());
    pmt->Shutdown();
}


void CMessageTransport::StartResponseTimeout(void)
{
    CS lock(m_csResponse);

    //
    // Check that the waiting list isn't empty and there is a pending message that
    // waiting for response.
    // The list can be empty although only now the sending is completed and handling.
    // The scenario is: UMS adds the message to waiting for response list before the
    // sending is started; However the timer is armed only after the sending is completed.
    // In the meantime, the response has been received, and the message removed from the list.
    //
    if (m_response.empty() || m_fResponseTimeoutScheduled)
        return;

    m_fResponseTimeoutScheduled = true;

    AddRef();
    ExSetTimer(&m_responseTimer, m_responseTimeout);
}



void CMessageTransport::CancelResponseTimer(void)
{
    //
    // Canceling the timer and setting of the flag should be atomic operation. Otherwise,
    // there is a scenario in which the timer is not setting although there is a pending
    // message that waiting for response.
    // This can be occurred if before setting the flag to false, there is a thread
    // switch and StartResponseTimeout routine is activated. Although the timer isn't
    // setting the flag is true, as a result the routine doesn't set the timer and UMS doen't
    // identifies connection problem
    //
    CS lock(m_csResponse);

    if (!ExCancelTimer(&m_responseTimer))
        return;

    ASSERT(m_fResponseTimeoutScheduled);
    m_fResponseTimeoutScheduled = false;

    //
    // Decrement refernce count taken for the timer
    //
    Release();
}



bool CMessageTransport::IsPipeLineMode() const
{
	bool fHttp10Delivery = CSingelton<CHttpDeliveryVesrion>::get().IsHttp10Delivery();
	return !fHttp10Delivery && m_SocketTransport->IsPipelineSupported();
}



void CMessageTransport::CompleteProcessResponse(USHORT mqclass)
{
    //
    // Protect waiting for response list
    //
    CS lock (m_csResponse);

	CancelResponseTimer();

    if (m_response.empty())
    {
        ASSERT((State() == csShutdownCompleted));
        return;
    }


    P<CQmPacket> pPkt = &m_response.front();
    m_response.pop_front();


	//
	// Test if gave ownership to the qm (case of order packet)
	//
	if(AppPostSend(pPkt, mqclass))
	{
		pPkt.detach();
	}
	else
	{
		m_pMessageSource->EndProcessing(pPkt, mqclass);
	}



	//
	// If we are in pause state and got response on all messages
	// - we can shutdown
	//
	if(m_response.empty() && m_fPause)
	{
		Shutdown(PAUSE);
		return;
	}

	
	//
	// If http 1.0 delivery we close the connection
	//
	if(CSingelton<CHttpDeliveryVesrion>::get().IsHttp10Delivery())
	{
		TrTRACE(SRMP,"Http 1.0 delivery - MSMQ close the connection");
		Shutdown(OK);
		return;		
	}	

	//
	// If we are not in pipeline mode (https) we should ask the driver
	// to bring us next packet for delivery, because only now we finished
	// reading response for the current request.
	//
	if(!IsPipeLineMode())
	{
		ASSERT(m_response.empty());
		GetNextEntry();
	}
}


void CMessageTransport::ReceiveResponseHeaderChunk(void)
{
    //
    // Increment refernce count for asynchronous context
    //
    R<CMessageTransport> ar = SafeAddRef(this);


    //
    // Receive next response header chunk
    //
    m_pConnection->ReceivePartialBuffer(
        m_responseOv.Header() + m_responseOv.m_HeaderValidBytes,
        m_responseOv.HeaderAllocatedSize() - m_responseOv.m_HeaderValidBytes,
        &m_responseOv
        );


    ar.detach();
}



void CMessageTransport::ReceiveResponse(void)
{
    if (m_responseOv.IsMoreResponsesExistInBuffer())
    {
        HandleExtraResponse();
        return;
    }

    //
    // Initialize the Overlapped with receive response call back routines
    //
    m_responseOv = EXOVERLAPPED(ReceiveResponseHeaderSucceeded, ReceiveResponseFailed);
    m_responseOv.m_HeaderValidBytes = 0;


    //
    // Receive first chunk of response header
    //
    ReceiveResponseHeaderChunk();
}


void CMessageTransport::ProcessResponse(LPCSTR buf, DWORD length)
{
	if (length <= STRLEN("HTTP/1.1"))
	{
		//
		// buffer is not valid response, close socket
		//
		throw exception();
	}
    m_responseOv.m_ResponseStatus = static_cast<USHORT>(atoi(buf + STRLEN("HTTP/1.1")));

    TrTRACE(NETWORKING, "Received HTTP response, Http Status=%d. pmt=0x%p", m_responseOv.m_ResponseStatus, this);


    // Response was received. Cancel the response timer
    //
    CancelResponseTimer();

	CHttpStatusCodeMapper::StatusCodeHandler Handler = CSingelton<CHttpStatusCodeMapper>::get()[m_responseOv.m_ResponseStatus];
	(this->*Handler)(m_responseOv.m_ResponseStatus);
}


void CMessageTransport::ReceiveResponseHeaderSucceeded(void)
/*++

Routine Description:
    The routine is called when receive response completes succesfully.

Arguments:
    None

Returned Value:
    None.

--*/
{
    //
    // For byte streams, zero bytes having been read indicates graceful closure
    // and that no more bytes will ever be read.
    //
	DWORD bytesTransfered = DataTransferLength(m_responseOv);
    if (bytesTransfered == 0)
    {
        TrERROR(NETWORKING, "Failed to receive response, connection was closed. pmt=0x%p", this);
        throw exception();
    }



    TrTRACE(NETWORKING, "Received response header. chunk bytes=%d, total bytes=%d", bytesTransfered, m_responseOv.m_HeaderValidBytes);


    //
    // Signal idle timer that this transport is active
    //
    MarkTransportAsUsed();

    //
    // Find out if the entire header was received
    //
    m_responseOv.m_HeaderValidBytes += bytesTransfered;

    m_responseOv.m_ProcessedSize = FindEndOfResponseHeader(
                                        m_responseOv.Header(),
                                        m_responseOv.m_HeaderValidBytes
                                        );
    if (m_responseOv.m_ProcessedSize != 0)
    {
       //
        // The enire header was received. Process the response.
        // Go and read the attached message (if one exist).
        //
        ProcessResponse(m_responseOv.Header(), m_responseOv.m_ProcessedSize);
        ReceiveResponseBody();
        return;
    }


    if(m_responseOv.HeaderAllocatedSize() == m_responseOv.m_HeaderValidBytes)
    {
        //
        // Header buffer is too small. Reallocate header buffer
        //
        if( m_responseOv.HeaderAllocatedSize() + CResponseOv::xHeaderChunkSize > CResponseOv::xMaxHeaderSize)
        {
            TrERROR(NETWORKING, "Response is too large, connection was closed. pmt=0x%p", this);
            throw exception();
        }

        m_responseOv.ReallocateHeaderBuffer(m_responseOv.HeaderAllocatedSize() + CResponseOv::xHeaderChunkSize);
    }

    //
    // Validate that we didn't read past the buffer
    //
    ASSERT(m_responseOv.HeaderAllocatedSize() > m_responseOv.m_HeaderValidBytes);

    //
    // Receive next chunk of response header
    //
    ReceiveResponseHeaderChunk();
}


void WINAPI CMessageTransport::ReceiveResponseHeaderSucceeded(EXOVERLAPPED* pov)
{
    ASSERT(SUCCEEDED(pov->GetStatus()));

    CResponseOv* pResponse = static_cast<CResponseOv*>(pov);
    R<CMessageTransport> pmt = CONTAINING_RECORD(pResponse, CMessageTransport, m_responseOv);

    try
    {
        pmt->ReceiveResponseHeaderSucceeded();
    }
    catch(const exception&)
    {
        TrERROR(NETWORKING, "Failed to process received response header, shutting down. pmt=0x%p", pmt.get());
        pmt->Shutdown();
        throw;
    }
}


void WINAPI CMessageTransport::ReceiveResponseFailed(EXOVERLAPPED* pov)
/*++

Routine Description:
    Callback routine. The routine is called when receive of respond failed

Arguments:
    pov - Pointer to EXOVERLAPPED

Returned Value:
    None.

--*/
{
    ASSERT(FAILED(pov->GetStatus()));

    //
    // get the message transport object
    //
    CResponseOv* pResponse = static_cast<CResponseOv*>(pov);
    R<CMessageTransport> pmt = CONTAINING_RECORD(pResponse, CMessageTransport, m_responseOv);

    TrERROR(NETWORKING, "Failed to receive respond, shutting down. pmt=0x%p Status=0x%x", pmt.get(), pmt->m_responseOv.GetStatus());

    pmt->Shutdown();
}


void CMessageTransport::ReceiveResponseBodyChunk()
{
    //
    // Increment refernce count for asynchronous context
    //
    R<CMessageTransport> ar = SafeAddRef(this);

    //
    // Receive the entity body chunck to the same buffer, as it is ignored
    //
    m_pConnection->ReceivePartialBuffer(
        m_responseOv.m_Body,
        m_responseOv.BodyChunkSize(),
        &m_responseOv
        );

    ar.detach();
}


void CMessageTransport::ReceiveResponseBody()
{
    try
    {
        m_responseOv.m_ProcessedSize += GetContentLength(
                                            m_responseOv.Header(),
                                            m_responseOv.m_ProcessedSize,
                                            m_responseOv.m_ResponseStatus
                                            );
    }
    catch(const exception&)
    {
        //
        // Response doesn't contains 'Content-Length' Header. Close the connection.
        //
        throw;
    }

    if (m_responseOv.m_HeaderValidBytes >= m_responseOv.m_ProcessedSize)
    {
	    ReceiveResponse();
		StartResponseTimeout();
        return;
    }


    m_responseOv =  EXOVERLAPPED(ReceiveResponseBodySucceeded, ReceiveResponseFailed);
    m_responseOv.m_BodyToRead = m_responseOv.m_ProcessedSize - m_responseOv.m_HeaderValidBytes;


    //
    // Receive first chunk of entity body
    //
    ReceiveResponseBodyChunk();
}


void CMessageTransport::ReceiveResponseBodySucceeded(void)
{
    //
    // For byte streams, zero bytes having been read indicates graceful closure
    // and that no more bytes will ever be read.
    //
	DWORD bytesTransfered = DataTransferLength(m_responseOv);
    if (bytesTransfered == 0)
    {
        TrERROR(NETWORKING, "Failed to receive response body, connection was closed. pmt=0x%p", this);
        throw exception();
    }

    ASSERT(bytesTransfered <= m_responseOv.m_BodyToRead);

    TrTRACE(NETWORKING, "Received response body. chunk bytes=%d, bytes remaining=%d", bytesTransfered, m_responseOv.m_BodyToRead);

    //
    // Mark the transport as used
    //
    MarkTransportAsUsed();

    m_responseOv.m_BodyToRead -= bytesTransfered;

    if (m_responseOv.m_BodyToRead == 0)
    {
        //
        // The entire body was read successfully.
        //
	    ReceiveResponse();
		StartResponseTimeout();
	    return;
    }

    //
    // Receive next chunk of entity body
    //
    ReceiveResponseBodyChunk();
}


void WINAPI CMessageTransport::ReceiveResponseBodySucceeded(EXOVERLAPPED* pov)
{
    ASSERT(SUCCEEDED(pov->GetStatus()));

    CResponseOv* pResponse = static_cast<CResponseOv*>(pov);
    R<CMessageTransport> pmt = CONTAINING_RECORD(pResponse, CMessageTransport, m_responseOv);

    try
    {
        pmt->ReceiveResponseBodySucceeded();
    }
    catch(const exception&)
    {
        TrERROR(NETWORKING, "Failed to process received response body, shutting down. pmt=0x%p", pmt.get());
        pmt->Shutdown();
        throw;
    }
}



void CMessageTransport::RequeueUnresponsedPackets(void)
{
		//
		// return all unresponse packets to the queue
		//
		while (!m_response.empty())
		{
			CQmPacket*  pPacket = &m_response.front();
			m_response.pop_front();
			m_pMessageSource->Requeue(pPacket);
		}
}






void CMessageTransport::OnRedirection(USHORT  HttpStatusCode )
/*++

Routine Description:
   Called when http errors 30x returned from the server which means redirection
   of the request to a new url. The new url is set on the message source and
   the transport is shutdown. Next time the transport will be loaded - the new target will
   be used.
	
Arguments:
    HttpStatusCode - http error code.

Returned Value:
    None.

--*/
{
	TrERROR(NETWORKING, "Received HTTP redirection response '%d'. pmt=0x%p", HttpStatusCode, this);

	xstr_t NewLocation = GetNewLocation(m_responseOv.Header(), m_responseOv.m_ProcessedSize);
	if(NewLocation.Length() == 0)
	{
		TrERROR(NETWORKING, "could not find new location from response header of error %d ", HttpStatusCode);
		OnAbortiveHttpError(HttpStatusCode);		
		return;
	}

	P<WCHAR> NewLocationstr = UtlUtf8ToWcs(
						(utf8_char*)NewLocation.Buffer(),
						NewLocation.Length(),
						NULL
						);

	m_pMessageSource->OnRedirected(NewLocationstr.get());
	Shutdown(OK);
}


void CMessageTransport::OnAbortiveHttpError(USHORT  HttpStatusCode )
/*++

Routine Description:
    The routine is called when retryable abortive error returned from the server.
	The message is deleted with the correct class code and the
	transport will be shutdown. The queue will be remaind in the Non active group
	ready to deliver the next message.
	

Arguments:
    HttpStatusCode - http error code.

Returned Value:
    None.

--*/
{
	CompleteProcessResponse(CREATE_MQHTTP_CODE(HttpStatusCode));
	TrERROR(NETWORKING, "Received HTTP abortive  error response '%d'. pmt=0x%p", HttpStatusCode, this);
	Shutdown(RETRYABLE_DELIVERY_ERROR);
}



void CMessageTransport::OnHttpDeliverySuccess(USHORT   /* HttpStatusCode */)
/*++

Routine Description:
    The routine is called when http status OK returned from the server.
	The message is deleted and delivery continute.


Arguments:
    HttpStatusCode - http error code.

Returned Value:
    None.

--*/
{
	CompleteProcessResponse(MQMSG_CLASS_NORMAL);
}



void CMessageTransport::OnRetryableHttpError(USHORT HttpStatusCode )
/*++

Routine Description:
    The routine is called when retryable http  error returned from the server.
	Execption is thrown , the transport will be shutdown and
	the queue will be moved to the waiting list to delay the deivery for a while.


Arguments:
    HttpStatusCode - http error code.

Returned Value:
    None.

--*/
{
    TrERROR(NETWORKING, "Received HTTP retryable error response '%d'. pmt=0x%p", HttpStatusCode, this);
    throw exception();
}



void CMessageTransport::OnHttpDeliveryContinute(USHORT /* HttpStatusCode */)
{

}


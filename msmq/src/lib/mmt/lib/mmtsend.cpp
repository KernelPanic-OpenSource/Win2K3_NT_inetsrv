/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:

    MmtSend.cpp

Abstract:

    Multicast Message Transport class - Send implementation

Author:

    Shai Kariv  (shaik)  27-Aug-00

Environment:

    Platform-independent

--*/

#include <libpch.h>
#include <mqsymbls.h>
#include <mqwin64a.h>
#include <mqformat.h>
#include "Mmt.h"
#include "Mmtp.h"
#include "MmtObj.h"

#include "mmtsend.tmh"


using namespace std;

VOID 
WINAPI 
CMessageMulticastTransport::GetPacketForSendingSucceeded(
    EXOVERLAPPED* pov
    )
{
    ASSERT(SUCCEEDED(pov->GetStatus()));

	CRequestOv* pRequest = static_cast<CRequestOv*>(pov);
    R<CMessageMulticastTransport> pMmt = CONTAINING_RECORD(pRequest, CMessageMulticastTransport, m_RequestEntry);
      
    TrTRACE(NETWORKING, "Got a message to multicast. pMmt = 0x%p", pMmt.get());

	CQmPacket* pPkt = pMmt->KeepProceesingPacket();

    pMmt->DeliverPacket(pPkt);

} // CMessageMulticastTransport::GetPacketForSendingSucceeded


VOID 
WINAPI 
CMessageMulticastTransport::GetPacketForSendingFailed(
    EXOVERLAPPED* pov
    )
/*++

Routine Description:

    The routine is called when getting entry request from a queue failed.
  
Arguments:

    pov - Pointer to overlapped.
  
Returned Value:

    None.

--*/
{
    ASSERT(FAILED(pov->GetStatus()));

	CRequestOv* pRequest = static_cast<CRequestOv*>(pov);
    R<CMessageMulticastTransport> pMmt = CONTAINING_RECORD(pRequest, CMessageMulticastTransport, m_RequestEntry);

    WCHAR buffer[MAX_PATH];
    MQpMulticastIdToString(pMmt->MulticastId(), buffer, TABLE_SIZE(buffer));
    TrERROR(NETWORKING, "Failed to get a new entry from queue %ls. Shutting down. pMmt = 0x%p", buffer, pMmt.get());

    pMmt->Shutdown();

} // CMessageMulticastTransport::GetPacketForSendingFailed


VOID 
CMessageMulticastTransport::SendFailed(
    DWORD /*cbSendSize*/,
    CQmPacket * pEntry
    )
/*++

Routine Description:

    The routine is called when send a message failed
  
Arguments:

    cbSendSize - Size in bytes of the sent packet
     
    pEntry     - Pointer to the sent packet
  
Returned Value:

    None.

--*/
{
	m_SrmpRequestBuffers.free();
   
    //
    // Packet is already deleted from disk. Free it from memory.
    //
    CACPacketPtrs& acPtrs = m_RequestEntry.GetAcPacketPtrs();

    ASSERT(acPtrs.pDriverPacket != NULL);
    ASSERT(acPtrs.pPacket != NULL);
    ASSERT(pEntry->GetPointerToPacket() == acPtrs.pPacket);
    ASSERT(pEntry->GetPointerToDriverPacket() == acPtrs.pDriverPacket);

    m_pMessageSource->EndProcessing(pEntry, MQMSG_CLASS_NORMAL);
    delete pEntry;

    acPtrs.pPacket = NULL;
    acPtrs.pDriverPacket = NULL;

} // CMessageMulticastTransport::SendFailed

    
VOID 
WINAPI 
CMessageMulticastTransport::SendFailed(
    EXOVERLAPPED* pov
    )
/*++

Routine Description:

    The routine is called when send a message failed
  
Arguments:

    pov - Pointer to EXOVERLAPPED
    
Returned Value:

    None.

--*/
{
    HRESULT status = pov->GetStatus();
    ASSERT(FAILED(status));

	CRequestOv* pRequest = static_cast<CRequestOv*>(pov);
    R<CMessageMulticastTransport> pMmt = CONTAINING_RECORD(pRequest, CMessageMulticastTransport, m_ov);

    WCHAR buffer[MAX_PATH];
    MQpMulticastIdToString(pMmt->MulticastId(), buffer, TABLE_SIZE(buffer));
    TrERROR(NETWORKING, "Failed to send message to '%ls', status=0x%x, shutting down. pMmt=0x%p", buffer, status, pMmt.get());
    pMmt->SendFailed(pMmt->m_ov.m_userData1, static_cast<CQmPacket*>(pMmt->m_ov.m_userData2));
    pMmt->Shutdown();

} // CMessageMulticastTransport::SendFailed


VOID 
CMessageMulticastTransport::SendSucceeded(
    DWORD cbSendSize,
    CQmPacket * pEntry
    )
/*++

Routine Description:

    The routine is called when send a message succeeded
  
Arguments:

    cbSendSize - Size in bytes of the sent packet
     
    pEntry     - Pointer to the sent packet
  
Returned Value:

    None.

--*/
{
	
    WCHAR buffer[MAX_PATH];
    MQpMulticastIdToString(MulticastId(), buffer, TABLE_SIZE(buffer));
    TrTRACE(
		NETWORKING, 
		"Send message to '%ls' succeeded. pMmt=0x%p", 
		buffer,
		this
		);

	m_SrmpRequestBuffers.free();

   
    //
    // Packet is already deleted from disk. Free it from memory.
    //
    CACPacketPtrs& acPtrs = m_RequestEntry.GetAcPacketPtrs();

    ASSERT(acPtrs.pDriverPacket != NULL);
    ASSERT(acPtrs.pPacket != NULL);
    ASSERT(pEntry->GetPointerToPacket() == acPtrs.pPacket);
    ASSERT(pEntry->GetPointerToDriverPacket() == acPtrs.pDriverPacket);

    m_pMessageSource->EndProcessing(pEntry, MQMSG_CLASS_NORMAL);
    delete pEntry;

    acPtrs.pPacket = NULL;
    acPtrs.pDriverPacket = NULL;

    MarkTransportAsUsed();

	//
	// Update performance counters
	//
	m_pPerfmon->UpdateBytesSent(cbSendSize);
	m_pPerfmon->UpdateMessagesSent();

    GetNextEntry();

} // CMessageMulticastTransport::SendSucceeded


VOID 
WINAPI 
CMessageMulticastTransport::SendSucceeded(
    EXOVERLAPPED* pov
    )
{
    ASSERT(SUCCEEDED(pov->GetStatus()));

    R<CMessageMulticastTransport> pMmt = CONTAINING_RECORD(pov, CMessageMulticastTransport, m_ov);
    
    //
    // Send has completed successfully, go and request the next message for delivery.
    // If the request failes, the cleanup timer will eventually shutdown this
    // transport, so no explict shutdown is nesscessary.
    //
    pMmt->SendSucceeded(pMmt->m_ov.m_userData1, static_cast<CQmPacket*>(pMmt->m_ov.m_userData2));

} // CMessageMulticastTransport::SendSucceeded


CQmPacket* 
CMessageMulticastTransport::KeepProceesingPacket(
    VOID
    )
{
    //
    // Get the entry from the send overlapped. 
    //
    CACPacketPtrs& acPkts = m_RequestEntry.GetAcPacketPtrs();
    CQmPacket* pEntry;

	try
	{
	    pEntry = new CQmPacket(acPkts.pPacket, acPkts.pDriverPacket);
	}
	catch (const exception&)
	{
		TrERROR(NETWORKING, "Failed to allocate multicast message, shutting down.");
        RequeuePacketMustSucceed();
        Shutdown();
        throw;
	}

    return pEntry;

} 


DWORD CMessageMulticastTransport::SendSize(const HttpRequestBuffers&  sendBufs)
{
	DWORD cbSize = 0;

	for(HttpRequestBuffers::const_iterator it = sendBufs.begin(); it != sendBufs.end(); ++it)
	{
		cbSize += it->len;
	}

	return cbSize;
}


VOID 
CMessageMulticastTransport::DeliverPacket(
    CQmPacket* pEntry
    )
/*++

Routine Description:

    The routine delivers an entry to the destination. The delivery is asynchornous.
    On completion a call back routine is called.
  
Arguments:

    pEntry - Points to a queue entry (i.e. msmq packet).
  
Returned Value:

    None.

--*/
{
    //
    // Mark the transport as used
    //
    MarkTransportAsUsed();

	WCHAR buf[MAX_PATH];
    MQpMulticastIdToString(MulticastId(), buf, TABLE_SIZE(buf));

	bool fOnWire = false;
    try
    {
	    m_SrmpRequestBuffers = MpSerialize(*pEntry, buf, buf);
	    ASSERT(m_SrmpRequestBuffers->GetNumberOfBuffers() != 0);

	    //
	    // Increment object refernce count, to insure that the object will
	    // not be destroyed before asynchronous send operation is completed
	    //
	    R<CMessageMulticastTransport> ar = SafeAddRef(this);

	    //
	    // Protect m_socket from shutdown
	    //
	    CSR readLock(m_pendingShutdown);

	    //
	    // Delete the packet from disk. Keep it in memory until send is complete.
	    //                  
	    m_pMessageSource->LockMemoryAndDeleteStorage(pEntry);
		ASSERT(m_SrmpRequestBuffers->GetNumberOfBuffers() != 0);

		m_ov.m_userData1 = numeric_cast<DWORD>(m_SrmpRequestBuffers->GetSendDataLength());
	    m_ov.m_userData2 = pEntry;

		//
		// Log to tracing that a message was sent.
		// Do this only if we are in the proper tracing level
		//
		if (WPP_LEVEL_COMPID_ENABLED(rsTrace, PROFILING))
		{
			OBJECTID TraceMessageId;
			pEntry->GetMessageId(&TraceMessageId);

			WCHAR wszName[100];
			MQpMulticastIdToString(MulticastId(),wszName, TABLE_SIZE(wszName));

			TrTRACE(PROFILING, "MESSAGE TRACE - State:%ls   Queue:%ls  ID:%!guid!\\%u   Delivery:0x%x   Class:0x%x   Label:%.*ls", 
				L"Multicast Message being sent on wire", 
				wszName, 
				&TraceMessageId.Lineage,
				TraceMessageId.Uniquifier, 
				pEntry->GetDeliveryMode(),
				pEntry->GetClass(),
				xwcs_t(pEntry->GetTitlePtr(), pEntry->GetTitleLength()));
		}

		//
		// after message is on the wire and we somehow get exception
		// we dont requeue the packet so we will not send it twice
		//
		fOnWire = true;
	    m_pConnection->Send(
							m_SrmpRequestBuffers->GetSendBuffers(), 
							numeric_cast<DWORD>(m_SrmpRequestBuffers->GetNumberOfBuffers()), 
							&m_ov
							);
	    ar.detach();
	}
    catch (const exception&)
    {
		if (!fOnWire)
		{
	        RequeuePacketMustSucceed();
		}
		else
	    {
			TrERROR(NETWORKING, "Failed to deliver multicast message, shutting down. pMmt=0x%p", this);
			SendFailed(m_ov.m_userData1, static_cast<CQmPacket*>(m_ov.m_userData2));
	    }
        Shutdown();
        throw;
    }
	
    TrTRACE(NETWORKING, "Send message to '%ls'. pMmt=0x%p", buf, this);
}

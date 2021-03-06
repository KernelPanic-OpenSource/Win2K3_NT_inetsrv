
/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    MsmTest.cpp

Abstract:
    Multicast Session Manager library test

Author:
    Shai Kariv (shaik) 05-Sep-00

Environment:
    Platform-independent

--*/

#include <libpch.h>
#include <msi.h>
#include <Ex.h>
#include <No.h>
#include <Cm.h>
#include <Msm.h>
#include <spi.h>

#include "MsmTest.tmh"

static DWORD s_FailedRate = 0;
static DWORD s_NoOfMessages = 1000;
static HANDLE s_hEvent;

//
// The guid of a public queue
//
const GUID xQueueId1 = { /* c4f259a1-9c90-42a8-8e83-10e65d084719 */
    0xc4f259a1,
    0x9c90,
    0x42a8,
    {0x8e, 0x83, 0x10, 0xe6, 0x5d, 0x08, 0x47, 0x19}
};

const GUID xQueueId2 = { /* c4f259a1-9c90-42a8-8e83-10e65d084720 */
    0xc4f259a1,
    0x9c90,
    0x42a8,
    {0x8e, 0x83, 0x10, 0xe6, 0x5d, 0x08, 0x47, 0x20}
};

const GUID xQueueId3 = { /* c4f259a1-9c90-42a8-8e83-10e65d084721 */
    0xc4f259a1,
    0x9c90,
    0x42a8,
    {0x8e, 0x83, 0x10, 0xe6, 0x5d, 0x08, 0x47, 0x21}
};

const GUID xQueueId4 = { /* c4f259a1-9c90-42a8-8e83-10e65d084722 */
    0xc4f259a1,
    0x9c90,
    0x42a8,
    {0x8e, 0x83, 0x10, 0xe6, 0x5d, 0x08, 0x47, 0x22}
};

QUEUE_FORMAT q1(xQueueId1);
QUEUE_FORMAT q2(xQueueId2);
QUEUE_FORMAT q3(xQueueId3);
QUEUE_FORMAT q4(xQueueId4);

const MULTICAST_ID xMulticastId1 = { 1234, 5678 };
const MULTICAST_ID xMulticastId2 = { 4567, 1234 };
const MULTICAST_ID xMulticastId3 = { 3893, 2323 };

extern "C"
BOOL
APIENTRY
AcceptEx (
    IN SOCKET,
    IN SOCKET,
    IN PVOID,
    IN DWORD,
    IN DWORD,
    IN DWORD,
    OUT LPDWORD,
    IN LPOVERLAPPED lpOverlapped
    )
{
    ASSERT(lpOverlapped != NULL);

    EXOVERLAPPED* pov = static_cast<EXOVERLAPPED*>(lpOverlapped);

    pov->SetStatus(STATUS_SUCCESS);
    ExPostRequest(pov);

    return TRUE;
}


extern "C"
void
APIENTRY
GetAcceptExSockaddrs( 
  PVOID ,       
  DWORD ,  
  DWORD ,  
  DWORD ,  
  LPSOCKADDR *,  
  LPINT ,  
  LPSOCKADDR *,  
  LPINT   
)
{
}


void CmQueryValue(const RegEntry&, CTimeDuration* pValue)
{
    *pValue = CTimeDuration(rand());
}


bool IsFailed(void)
{
    if (s_FailedRate == 0)
        return FALSE;

    return ((DWORD)(rand() % 100) < s_FailedRate);
}


void
AppAcceptMulticastPacket(
    const char* ,
    DWORD ,
    const BYTE* ,
    const QUEUE_FORMAT& 
    )
{
}


void AppConnectMulticast(void)
{
}


class CSessionPerfmon : public ISessionPerfmon
{
public:
    CSessionPerfmon() : m_fInstanceCreated(false) 
    {
    }

    ~CSessionPerfmon()
    {
    }
    
	void CreateInstance(LPCWSTR)
	{
		m_fInstanceCreated = true;
	}

	void UpdateBytesSent(DWORD )
	{
		ASSERT(m_fInstanceCreated);
	}

	void UpdateMessagesSent(void)
	{
		ASSERT(m_fInstanceCreated);
	}

	void UpdateBytesReceived(DWORD )
	{
		ASSERT(m_fInstanceCreated);
	}

	void UpdateMessagesReceived(void)
    {
		ASSERT(m_fInstanceCreated);
    }

private:
	bool m_fInstanceCreated;
};


R<ISessionPerfmon>
AppGetIncomingPgmSessionPerfmonCounters(
	LPCWSTR strMulticastId,
	LPCWSTR 
	)
{
	R<CSessionPerfmon> pPerfmon = new CSessionPerfmon;
	pPerfmon->CreateInstance(strMulticastId);

	return pPerfmon;
}


void Usage(void)
{
	printf("Usage: msmtest -n <no of messages> [-f xxx] [-h]\n");
    printf("\tn - Number of messages\n");
	printf("\tf - Fail rate\n");
	printf("\th - Print this message\n");
	exit(-1);
}


extern "C" int __cdecl _tmain(int argc, LPCTSTR argv[])
/*++

Routine Description:
    Test Message Transport library

Arguments:
    Parameters.

Returned Value:
    None.

--*/
{
    WPP_INIT_TRACING(L"Microsoft\\MSMQ");

	TrInitialize();

    --argc;
    ++argv;

    s_FailedRate = 0;
    
    while (argc > 0)
	{
		if (argv[0][0] != L'-') 
			Usage();

		switch(argv[0][1])
		{
            case L'n':
            case L'N':
                s_NoOfMessages = _wtoi(argv[1]);
				argc -= 2;
				argv += 2;
                break;

            case L'f':
			case L'F':
		        s_FailedRate = _wtoi(argv[1]);
				argc -= 2;
				argv += 2;
				break;


            default:
				Usage();
		}
    }

    ExInitialize(5);
    NoInitialize();
    MsmInitialize();


	//
	// Check connect/disconnect
	//
	MsmBind(q1, xMulticastId1);
	MsmBind(q2, xMulticastId2);
	MsmBind(q3, xMulticastId3);
	MsmBind(q4, xMulticastId2);

	MsmDisconnect();
	MsmUnbind(q2);
	MsmBind(q2, xMulticastId2);
	MsmConnect();

    s_hEvent = CreateEvent(NULL, FALSE, FALSE, L"");

    bool fFailed = false;

    do
    {
        try
        {
            MsmBind(q1, xMulticastId1);
        }
        catch(const exception&)
        {
            fFailed = true;
        }
    } while(fFailed);
    
    WaitForSingleObject(s_hEvent, INFINITE);


    WPP_CLEANUP();
    return 0;
}

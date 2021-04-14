/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:
    InitMcListener.cpp

Abstract:
    Initialize Multicast listener

Author:
    Uri Habusha (urih) 26-Sep-2000

Environment:
    Platform-independent

--*/

#include "stdh.h"
#include "Mc.h"
#include "Msm.h"
#include "Fn.h"
#include "mqexception.h"
#include "lqs.h"
#include "cqmgr.h"

#include "InitMcListener.tmh"

static WCHAR *s_FN=L"Initmclistener";

static LPWSTR GetMulticastAddress(HLQS hLqs)
{
    PROPID aProp[2];
    PROPVARIANT aVar[2];
    ULONG cProps = 0;

    aProp[0] = PROPID_Q_MULTICAST_ADDRESS;
    aVar[0].vt = VT_NULL;
    ++cProps;

    //
    // Transactional queues ignore the multicast property. We allow setting the multicast
    // property for transactional queues since it's difficult to block it but do not bind.
    //
    aProp[1] = PROPID_Q_TRANSACTION;
    aVar[1].vt = VT_UI1;
    ++cProps;

    HRESULT hr = LQSGetProperties(hLqs, cProps, aProp, aVar);
    LogHR(hr, s_FN, 70);
    if (FAILED(hr))
        throw bad_hresult(hr);

    if (aVar[0].vt == VT_EMPTY)
        return NULL;

    if (aVar[1].bVal)
    {
        TrTRACE(NETWORKING, "Do not bind transactional queue to multicast address");
        return NULL;
    }

    ASSERT((aVar[0].pwszVal != NULL) && (aVar[0].vt == VT_LPWSTR));
    return aVar[0].pwszVal;
}


static void BindMulticast(const QUEUE_FORMAT& qf, LPCWSTR address)
{    
    MULTICAST_ID multicastId;

    try
    {
        FnParseMulticastString(address, &multicastId);
    }
    catch(const bad_format_name&)
    {
        //
        // If the multicast address isn't valid ignore the current address and
        // continue to procees the rest of the queues
        //
        EvReportWithError(MULTICAST_BIND_ERROR, MQ_ERROR_ILLEGAL_FORMATNAME, 1, address);
        LogIllegalPoint(s_FN, 50);
        return;
    }

    try
    {
        MsmBind(qf, multicastId);
    }
    catch (const bad_win32_error& e)
    {
        EvReportWithError(MULTICAST_BIND_ERROR, e.error(), 1, address);
        LogIllegalPoint(s_FN, 55);
    }
}


static bool InitMulticastPublicQueues(void)
    //
    // Enumerate local public queues in LQS.
    //
{
    GUID guid;
    HLQS hLQS;

    HRESULT hr = LQSGetFirst(&hLQS, &guid);

    for(;;)
    {
        //
        // No more queues
        //
        if (hr == MQ_ERROR_QUEUE_NOT_FOUND)
            return true;

        //
        // Open a public queue store according to the queue GUID.
        //
        WCHAR szFilePath[MAX_PATH_PLUS_MARGIN];
        CHLQS hLqsQueue;
        hr = LQSOpen(&guid, &hLqsQueue, szFilePath);
        if (FAILED(hr))
        {
            EvReportWithError(EVENT_ERROR_INIT_MULTICAST, hr, 1, szFilePath);
            LogHR(hr, s_FN, 60);
            return false;
        }

        AP<WCHAR> multicastAddress = GetMulticastAddress(hLqsQueue);

        if (multicastAddress != NULL)
        {
            BindMulticast(QUEUE_FORMAT(guid), multicastAddress);
        }

        hr = LQSGetNext(hLQS, &guid);
    }

    //
    // No need to close the enumeration handle in case LQSGetNext fails
    //
}


static bool InitMulticastPrivateQueues(void)
{
    //
    // Enumerate local public queues in LQS.
    //
    DWORD queueId;
    HLQS hLQS;

    HRESULT hr = LQSGetFirst(&hLQS, &queueId);

    for(;;)
    {
        //
        // No more queues
        //
        if (hr == MQ_ERROR_QUEUE_NOT_FOUND)
            return true;

        //
        // Open a private queue store according to the queue id.
        //
        WCHAR szFilePath[MAX_PATH_PLUS_MARGIN];
        CHLQS hLqsQueue;
        hr = LQSOpen(queueId, &hLqsQueue, szFilePath);
        if (FAILED(hr))
        {
            EvReportWithError(EVENT_ERROR_INIT_MULTICAST, hr, 1, szFilePath);
            return false;
        }

        AP<WCHAR> multicastAddress = GetMulticastAddress(hLqsQueue);

        if (multicastAddress != NULL)
        {
            BindMulticast(QUEUE_FORMAT(McGetMachineID(), queueId), multicastAddress);
        }

        hr = LQSGetNext(hLQS, &queueId);
    }

    //
    // No need to close the enumeration handle in case LQSGetNext fails
    //
}


bool QmpInitMulticastListen(void)
{
	//
	// If the QM is offline. Report to Multicast that it should be offline and return.
	// binding of queues to multicast address is done when the QM becomes active.
	//
	if (!QueueMgr.IsConnected())
	{
		MsmDisconnect();
		return true;
	}

    if (!InitMulticastPublicQueues())
    {
    	LogIllegalPoint(s_FN, 30);
        return false;
    }

    if (!InitMulticastPrivateQueues())
    {
    	LogIllegalPoint(s_FN, 40);
        return false;
    }

    return true;
}



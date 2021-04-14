/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    qmnotify.cpp

Abstract:

    QM notify mechanism.
	
	in MSMQ 1.0 and 2.0, when an msmq client changes the state of an msmq 
	object on the DS, the DS sends a notification to the object's owner QM.

	Now in MSMQ the DS server has been elimintated in favor of using the AD. 
	therefore, when an msmq client changes the state of an msmq object on 
	the DS, it notifies the local QM about it. the local QM will decide
	whether to sych with the DS or send an msmq notification message to the
	object's owner QM.
	Also, since QMs don't trust each other, the recieving QM accesses the DS 
	at a maximum rate of once per 15 minutes, to avoid being voulnerable to 
	denial of service atacks.

Author:
	Extended by Nir Aides (niraides) 13-Jun-2000

--*/

#include "stdh.h"
#include "cqmgr.h"
#include "qmnotify.h"
#include "pnotify.h"
#include "bupdate.h"
#include "regqueue.h"
#include <mqsec.h>
#include "ad.h"
#include <adnotify.h>
#include <privque.h>
#include <mqprops.h>
#include <mqstl.h>
#include "lqs.h"
#include "cqpriv.h"
#include "fn.h"
#include <_propvar.h>

#include "qmnotify.tmh"

static WCHAR *s_FN=L"qmnotify";

using namespace std;



extern BOOL g_fWorkGroupInstallation;

VOID WINAPI ReceiveNotifications(const CMessageProperty*, const QUEUE_FORMAT*);

#define NOTIFICATION_MSG_TIMEOUT (5 * 60) //5 minutes
#define NOTIFICATION_MSG_PRIORITY DEFAULT_M_PRIORITY

//
// Update from DS will be done in intervals of 15 minutes
// to defend against denial of service attacks.
//
static const CTimeDuration xNotificationUpdateDelay = CTimeDuration::OneSecond().Ticks() * 60 * 15; 

//
// If we recieve more then 'xLQSTresholdMagicNumber' notifications in the time interval, 
// we invoke the general LQS cache update.
//
static const DWORD xLQSTresholdMagicNumber = 100;


//
// The following two strings are used to for marshaling the notification body,
// in a readable xml-like form.
//
// Guid Format expected to be "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
//

#define QM_NOTIFICATION_INPUT_MARSHALLING_FORMAT \
		L"<Notification>" \
			L"<Event>%d</Event>" \
			L"<ObjectGuid>%40[^<]</ObjectGuid>" \
			L"<DomainController>%260[^<]</DomainController>" \
		L"</Notification>"

#define QM_NOTIFICATION_OUTPUT_MARSHALLING_FORMAT \
		L"<Notification>" \
			L"<Event>%d</Event>" \
			L"<ObjectGuid>" GUID_FORMAT L"</ObjectGuid>" \
			L"<DomainController>%s</DomainController>" \
		L"</Notification>"



//
// This class is used to marshal the notification information inside 
// an msmq message
//
class CNotificationBody
{
public:
	CNotificationBody() : 
		m_Event(neNoEvent)
	{
	}

	CNotificationBody(
		ENotificationEvent Event, 
		const GUID& ObjectGuid,
		LPCWSTR DomainController 
		);

	void MarhshalIn(const WCHAR* pBuffer, long Size);

	//
	// returns the estimated buffer size required for MarshalOut()
	//
	long RequiredWideBufferSize();

	WCHAR* MarshalOut(WCHAR* pBuffer, long Size);

	ENotificationEvent Event() const
	{
		return m_Event;
	}

	const GUID& Guid() const
	{
		return m_ObjectGuid;
	}

	const wstring& DomainController() const
	{
		return m_DomainController;
	}

	VOID DomainController(const wstring& str)
	{
		m_DomainController = str;
	}

private:
	ENotificationEvent m_Event;
	GUID m_ObjectGuid;
	wstring m_DomainController;
};



CNotificationBody::CNotificationBody(
	ENotificationEvent Event, 
	const GUID& ObjectGuid,
	LPCWSTR DomainController 
	)
{
	m_Event = Event;
	m_ObjectGuid = ObjectGuid;
	if(DomainController != NULL)
	{
		m_DomainController = wstring(DomainController);
	}
}


void CNotificationBody::MarhshalIn(const WCHAR* pBuffer, long Size)
{
	ASSERT(pBuffer != NULL && Size != 0);
	//
	// Asserts the notification body is alligned for a unicode buffer.
	//
	ASSERT(((INT_PTR)pBuffer & 1) == 0);

	int dwEvent;
	WCHAR szGuid[MAX_PATH];
	WCHAR szDomainController[MAX_PATH];

	int Count = _snwscanf(
					pBuffer, 
					Size,
					QM_NOTIFICATION_INPUT_MARSHALLING_FORMAT, 
					&dwEvent,
					szGuid,
					szDomainController
					);
	
	//
	// Number of fields read, may be 2 if no domain controller was specified in 
	// the message body.
	// 
	if(Count < 2)
	{
		TrERROR(GENERAL, "Failed in CNotificationBody::MarhshalIn().");
		LogIllegalPoint(s_FN, 120);
		throw exception();
	}


	FnParseGuidString(szGuid, &m_ObjectGuid);
	m_DomainController.erase();
	if(Count == 3)
	{
		wstring DomainController = szDomainController;
		m_DomainController = DomainController;
	}
	m_Event = (ENotificationEvent) dwEvent;

}



WCHAR* CNotificationBody::MarshalOut(WCHAR* pBuffer, long Size)
{
	ASSERT(Size >= RequiredWideBufferSize());

	int Count = _snwprintf(
					pBuffer,
					Size,
					QM_NOTIFICATION_OUTPUT_MARSHALLING_FORMAT,
					(int)m_Event,
					GUID_ELEMENTS((&m_ObjectGuid)),
					m_DomainController.c_str()
					);
	if(Count < 0)
	{
		TrERROR(GENERAL, "Buffer too small, in CNotificationBody::MarshalOut().");
		LogIllegalPoint(s_FN, 130);
		throw exception();
	}

	return pBuffer + Count;
}



long CNotificationBody::RequiredWideBufferSize()
{
	return 
		STRLEN(QM_NOTIFICATION_OUTPUT_MARSHALLING_FORMAT) - 
		STRLEN(GUID_FORMAT) + 
		numeric_cast<long>(16 + 36 + m_DomainController.length());
}



RPC_STATUS RPC_ENTRY QmNotifySecurityCallback(
	RPC_IF_HANDLE, 
	void* hBind
	)
{
	TrTRACE(RPC, "QmNotifySecurityCallback starting");
	
	if(!mqrpcIsLocalCall(hBind))
	{
		TrERROR(RPC, "Failed to verify Local RPC");
		ASSERT_BENIGN(("Failed to verify Local RPC", 0));
		return ERROR_ACCESS_DENIED;
	}
	
	TrTRACE(RPC, "QmNotifySecurityCallback passed successfully");
	return RPC_S_OK;
}

HRESULT 
GetRelevantProperties(
	AD_OBJECT Object,
    const GUID& ObjectGuid,
	LPCWSTR DomainController,
	DWORD cProps,
	PROPID* pProps,
	PROPVARIANT* pVars	
	)
{
	ASSERT(pProps[cProps - 1] == PROPID_Q_SECURITY || pProps[cProps - 1] == PROPID_QM_SECURITY);
	
	CAutoCleanPropvarArray AutoCleanVariantArray;
	AutoCleanVariantArray.attachStaticClean(cProps, pVars);

	//
	// ISSUE-2001/05/27-ilanh - notification message don't include fServerName flag
	// so we are passing false for fServerName.
	//
	HRESULT hr = ADGetObjectPropertiesGuid(
					Object,
					DomainController,
					false,	// fServerName
					&ObjectGuid,
					cProps - 1,
					pProps,
					pVars
					);

	if(FAILED(hr))
	{
		TrERROR(GENERAL, "ADGetObjectPropertiesGuid() failed, hr = 0x%x", hr);
		return LogHR(hr, s_FN, 200);
	}

	SECURITY_INFORMATION RequestedInformation = 
							OWNER_SECURITY_INFORMATION |
							GROUP_SECURITY_INFORMATION |
							DACL_SECURITY_INFORMATION  | 
							SACL_SECURITY_INFORMATION;

	//
	// Try to get security information including SACL information.
	// If we fail try again without SACL information.
	//

    MQSec_SetPrivilegeInThread(SE_SECURITY_NAME, TRUE);

	hr = ADGetObjectSecurityGuid(
			Object,
			DomainController, 
			false,	// fServerName
			&ObjectGuid,
			RequestedInformation,
			PROPID_Q_SECURITY,
			pVars + cProps - 1
			);

	MQSec_SetPrivilegeInThread(SE_SECURITY_NAME, FALSE);

	if(SUCCEEDED(hr))
	{
		AutoCleanVariantArray.detach();
		return hr;
	}

	RequestedInformation &= ~SACL_SECURITY_INFORMATION;

	hr = ADGetObjectSecurityGuid(
			Object,
			DomainController, 
			false,	// fServerName
			&ObjectGuid,
			RequestedInformation,
			PROPID_Q_SECURITY,
			pVars + cProps - 1
			);
	if(FAILED(hr))
	{
		TrERROR(GENERAL, "ADGetObjectPropertiesGuid() failed to get security property, hr = 0x%x", hr);
		return LogHR(hr, s_FN, 230);
	}

	AutoCleanVariantArray.detach();
	return LogHR(hr, s_FN, 240);
}



void 
SyncQueueState(
    const GUID& ObjectGuid,
	LPCWSTR DomainController
	)
{
	//
	// BUGBUG: need to create mechanism to keep list syncronized with 
	// LQSSetProperties(). niraides 06-Jun-00
	//
	PROPID pProps[] = {
	        PROPID_Q_TYPE, 
			PROPID_Q_INSTANCE,			 
			PROPID_Q_BASEPRIORITY,		
			PROPID_Q_JOURNAL,			 
			PROPID_Q_QUOTA,				
			PROPID_Q_JOURNAL_QUOTA,		
			PROPID_Q_CREATE_TIME,		
			PROPID_Q_MODIFY_TIME,		
//			PPROPID_Q_TIMESTAMP,		//does not exist. triggers assertion (ads.cpp 2208)
			PROPID_Q_PATHNAME,				
			PROPID_Q_LABEL, 
			PROPID_Q_AUTHENTICATE,		
			PROPID_Q_PRIV_LEVEL,		
			PROPID_Q_TRANSACTION,		
			PROPID_Q_MULTICAST_ADDRESS,

			//
			// Must be last property
			//
			PROPID_Q_SECURITY			
	};

    const DWORD cProps = TABLE_SIZE(pProps);
	PROPVARIANT pVars[cProps] = {0};
	
	CAutoCleanPropvarArray AutoCleanVariantArray;
	AutoCleanVariantArray.attachStatic(cProps, pVars);

    HRESULT hr = GetRelevantProperties(
					eQUEUE,
					ObjectGuid,
					DomainController,
					cProps,
					pProps,
					pVars	
					);

	if((hr == MQ_ERROR_NO_DS) && (DomainController != NULL))
	{
		//
		// The specified DC was unreachable. try to sync with local DC.
		//
		TrWARNING(GENERAL, "failed to sync with DC = %ls, trying local DC", DomainController);
		hr = GetRelevantProperties(
				eQUEUE,
				ObjectGuid,
				NULL,
				cProps,
				pProps,
				pVars	
				);
	}
	
	if(hr == MQ_ERROR_NO_DS)
	{
		TrERROR(GENERAL, "GetRelevantProperties() Failed, MQ_ERROR_NO_DS");
		throw exception();
	}

	if(hr == MQ_ERROR_QUEUE_NOT_FOUND)
	{
		TrTRACE(GENERAL, "GetRelevantProperties() Failed, MQ_ERROR_QUEUE_NOT_FOUND");
		DeleteCachedQueue(&ObjectGuid);
		return;
	}

	if(FAILED(hr))
	{
		//
		// Don't throw exception to allow processing of more notifications
		//
		TrERROR(GENERAL, "GetRelevantProperties() Failed, hr = 0x%x", hr);
		return;
	}
	
	UpdateCachedQueueProp(&ObjectGuid, cProps, pProps, pVars, time(NULL));
}



void 
SyncMachineState(
    const GUID& ObjectGuid,
	LPCWSTR DomainController
	)
{
	if(ObjectGuid != *CQueueMgr::GetQMGuid())
	{
		TrERROR(GENERAL, "Object guid is not this QM.");
		LogIllegalPoint(s_FN, 260);
		throw exception();
	}

	//
	// BUGBUG: need to create mechanism to keep list syncronized with 
	// CQueueMgr::UpdateMachineProperties(). niraides 06-Jun-00
	//
	PROPID pProps[] = {
            PROPID_QM_QUOTA,
            PROPID_QM_JOURNAL_QUOTA,

			//
			// Must be last property
			//
            PROPID_QM_SECURITY         
 	};

	const DWORD cProps = TABLE_SIZE(pProps);
	PROPVARIANT pVars[cProps] = {0};

	CAutoCleanPropvarArray AutoCleanVariantArray;
	AutoCleanVariantArray.attachStatic(cProps, pVars);

    HRESULT hr = GetRelevantProperties(
					eMACHINE,
					ObjectGuid,
					DomainController,
					cProps,
					pProps,
					pVars	
					);

	if((hr == MQ_ERROR_NO_DS) && (DomainController != NULL))
	{
		//
		// The specified DC was unreachable. try to sync with local DC.
		//
		TrWARNING(GENERAL, "failed to sync with DC = %ls, trying local DC", DomainController);
		hr = GetRelevantProperties(
					eMACHINE,
					ObjectGuid,
					NULL,
					cProps,
					pProps,
					pVars	
					);
	}

	if(hr == MQ_ERROR_NO_DS)
	{
		TrERROR(GENERAL, "GetRelevantProperties() failed, MQ_ERROR_NO_DS");
		LogHR(hr, s_FN, 275);
		throw exception();
	}

	if(FAILED(hr))
	{
		//
		// Don't throw exception to allow processing of more notifications
		//
		TrERROR(GENERAL, "GetRelevantProperties() failed, hr = 0x%x", hr);
		LogHR(hr, s_FN, 290);
		return;
	}
	
	QueueMgr.UpdateMachineProperties(cProps, pProps, pVars);
}



void 
SyncObjectState(
	ENotificationEvent Event,
    const GUID& ObjectGuid,
	LPCWSTR DomainController
	)
{
	if(DomainController != NULL && DomainController[0] == L'\0')
	{
		DomainController = NULL;
	}

	TrTRACE(GENERAL, "sync object, event = %d, guid = %!guid!, DomainController = %ls", Event, &ObjectGuid, DomainController);

	switch(Event)
	{
	case neChangeQueue:
	case neCreateQueue:
	case neDeleteQueue:
        SyncQueueState(ObjectGuid, DomainController);
        break;

	case neChangeMachine:
		SyncMachineState(ObjectGuid, DomainController);
		break;

	default:
		TrERROR(GENERAL, "Unsupported notification event in SyncObjectState().");
		LogIllegalPoint(s_FN, 300);
		throw exception();
	}
}



void 
BuildNotificationMsg(
	ENotificationEvent Event,
	LPCWSTR DomainController,
    const GUID& ObjectGuid,
	AP<BYTE>& Buffer,
	long* pSize
	)
/*++
Routine Description:
	Builds the body of an msmq QM-notification message.

Arguments:
	[out] Buffer - Newly allocated buffer containing the msmq message body
	[out] pSize - Buffer's size

Return Value:

--*/
{
	CNotificationBody Body(Event, ObjectGuid, DomainController);

	long BufferSize = sizeof(CNotificationHeader) + Body.RequiredWideBufferSize() * 2;
    AP<BYTE> TempBuffer = new BYTE[BufferSize];
 
	CNotificationHeader* pHeader = (CNotificationHeader*)TempBuffer.get();

    pHeader->SetVersion(QM_NOTIFICATION_MSG_VERSION);
    pHeader->SetNoOfNotifications(1);

	//
	// Asserts the notification body is alligned for a unicode buffer.
	//
	ASSERT(((INT_PTR)pHeader->GetPtrToData() & 1) == 0);

	WCHAR* pBodyData = (WCHAR*)pHeader->GetPtrToData();
	
	BYTE* pEnd = (BYTE*) Body.MarshalOut(
							pBodyData, 
							Body.RequiredWideBufferSize()
							);

	*pSize = numeric_cast<long>(pEnd - TempBuffer.get());
	Buffer = TempBuffer.detach();
}



void 
SendNotificationMsg(
    const GUID* pDestQMGuid,
	BYTE* pBuffer,
	long Size
	)
{
    CMessageProperty MsgProp;

    MsgProp.wClass=0;
    MsgProp.dwTimeToQueue = NOTIFICATION_MSG_TIMEOUT;
    MsgProp.dwTimeToLive = INFINITE ;
    MsgProp.pMessageID = NULL;
    MsgProp.pCorrelationID = NULL;
    MsgProp.bPriority= NOTIFICATION_MSG_PRIORITY;
    MsgProp.bDelivery= MQMSG_DELIVERY_EXPRESS;
    MsgProp.bAcknowledge= MQMSG_ACKNOWLEDGMENT_NONE;
    MsgProp.bAuditing=MQ_JOURNAL_NONE;
    MsgProp.dwApplicationTag= DEFAULT_M_APPSPECIFIC;
    MsgProp.dwTitleSize=0;
    MsgProp.pTitle=NULL;
    MsgProp.dwBodySize=Size;
    MsgProp.dwAllocBodySize = Size;
    MsgProp.pBody= pBuffer;

	QUEUE_FORMAT QueueFormat;
    QueueFormat.PrivateID(*pDestQMGuid, NOTIFICATION_QUEUE_ID);
    
    HRESULT hr = QmpSendPacket(
					&MsgProp, 
					&QueueFormat, 
					NULL,			  //pqdAdminQueue 
					NULL,			 //pqdResponseQueue 
					FALSE			//fSign
					);
    if (FAILED(hr))
    {
		TrERROR(GENERAL, "Failed call to QmpSendPacket() in SendNotificationMsg().");
		LogHR(hr, s_FN, 310);
		throw exception();
    }
}



void 
R_NotifyQM( 
    /* [in] */ handle_t,
    /* [in] */ enum ENotificationEvent Event,
    /* [unique][in] */ LPCWSTR DomainController,
    /* [in] */ const GUID __RPC_FAR *pDestQMGuid,
    /* [in] */ const GUID __RPC_FAR *pObjectGuid
	)
/*++
Routine Description:
	this RPC routine is invoked by the msmq client who changed an object's 
	state in the DS, to trigger synchronisation of the object's new state 
	by the owner QM.

Arguments:
	Event - The nature of the state change (created, deleted, changed...)
	DomainController - The domain controller on which the state was changed
	pDestQMGuid - id of the QM which is the owner of the object
	pObjectGuid - id of the object whose state should be sync-ed

Return Value:

--*/
{
	ASSERT(!g_fWorkGroupInstallation);

	if(Event > neChangeMachine || Event < neCreateQueue)
	{
		TrERROR(RPC, "Incorrect event type, %d", Event);
		ASSERT_BENIGN(("Incorrect event type", 0));
		RpcRaiseException(MQ_ERROR_INVALID_PARAMETER);
	}

	if((pObjectGuid == NULL) || (pDestQMGuid == NULL))
	{
		TrERROR(RPC, "NULL guid pointer input");
		ASSERT_BENIGN((pObjectGuid != NULL) && (pDestQMGuid != NULL));
		RpcRaiseException(MQ_ERROR_INVALID_PARAMETER);
	}

	if((*pObjectGuid == GUID_NULL) || (*pDestQMGuid == GUID_NULL))
	{
		TrERROR(RPC, "Invalid guid values (GUID_NULL)");
		ASSERT_BENIGN((*pObjectGuid != GUID_NULL) && (*pDestQMGuid != GUID_NULL));
		RpcRaiseException(MQ_ERROR_INVALID_PARAMETER);
	}
	
	try
	{
		//
		// If the subject of the notification is local, sync immediately
		//
		if(*pDestQMGuid == *CQueueMgr::GetQMGuid())
		{
			SyncObjectState(Event, *pObjectGuid, DomainController);
			return;
		}

		//
		// Otherwise send Notification message to owner QM
		//
		
		AP<BYTE> Buffer;
		long size;

		BuildNotificationMsg(
			Event,
			DomainController, 
			*pObjectGuid, 
			Buffer,
			&size
			);

		SendNotificationMsg(pDestQMGuid, Buffer.get(), size);
	}
	catch(const exception&)
	{
		TrERROR(GENERAL, "Failed NotifyQM().");
		LogIllegalPoint(s_FN, 330);
	}
}



void IntializeQMNotifyRpc(void)
{
	//
	// Limit R_NotifyQM input max size.
	// This will impose a limit on DomainController string.
	//
	const DWORD xNofifyMaxRpcSize = 10 * 1024;	// 10k
    RPC_STATUS status = RpcServerRegisterIf2(
				            qmnotify_v1_0_s_ifspec,
                            NULL,   
                            NULL,
				            RPC_IF_ALLOW_CALLBACKS_WITH_NO_AUTH,
				            RPC_C_PROTSEQ_MAX_REQS_DEFAULT ,
				            xNofifyMaxRpcSize,	
				            QmNotifySecurityCallback
				            );
 
    if(status != RPC_S_OK) 
    {
        TrERROR(GENERAL, "Failed to initialize HTTP RPC. Error %x", status);
		LogRPCStatus(status, s_FN, 340);
        throw exception();
    }
}



//
// A "function object" which is used to compare GUID objects
//
struct CFunc_CompareGuids: binary_function<GUID, GUID, bool> 
{
	bool operator()(const GUID& obj1, const GUID& obj2) const
	{
		C_ASSERT(sizeof(obj1) == 16);
		return (memcmp(&obj1, &obj2, sizeof(obj1)) < 0);
	}
};



//
// This class is used to store notifications until a set time interval has 
// passed, and then sync all of them together from the DS.
//
class CNotificationScheduler 
{
public:
	CNotificationScheduler() :
		m_fDoGeneralUpdate(false),
		m_fTimerArmed(false),
		m_LastTimeFired(CTimeInstant::MinValue()),
		m_Timer(TimeToHandleNotifications)
	{
	}

	VOID ScheduleNotification(const CNotificationBody& Body);
	VOID HandleNotifications();

private:
	VOID GeneralUpdate();

public:
	static void WINAPI TimeToHandleNotifications(CTimer* pTimer);

private:
	typedef map<GUID, CNotificationBody, CFunc_CompareGuids> NotificationMap;

private:
	CCriticalSection m_cs;

	bool m_fDoGeneralUpdate;
	bool m_fTimerArmed;
	CTimeInstant m_LastTimeFired;

	CTimer m_Timer;
	NotificationMap m_NotificationMap;
};



void WINAPI CNotificationScheduler::TimeToHandleNotifications(CTimer* pTimer)
{
	CNotificationScheduler* pNotificationScheduler = CONTAINING_RECORD(pTimer, CNotificationScheduler, m_Timer);

	CS Lock(pNotificationScheduler->m_cs);

	pNotificationScheduler->m_fTimerArmed = false;
	pNotificationScheduler->m_LastTimeFired = ExGetCurrentTime();
	pNotificationScheduler->HandleNotifications();
}

	
	
VOID CNotificationScheduler::HandleNotifications()
{
	CS Lock(m_cs);

	try
	{
		if(m_fDoGeneralUpdate)
		{
			GeneralUpdate();
			m_fDoGeneralUpdate = false;
			return;
		}

		NotificationMap::iterator Itr = m_NotificationMap.begin();

		for(;Itr != m_NotificationMap.end();)
		{
			CNotificationBody& Body = Itr->second;

			SyncObjectState(Body.Event(), Body.Guid(), Body.DomainController().c_str());

			Itr = m_NotificationMap.erase(Itr);
		}
	}
	catch(const exception&)
	{
		m_fTimerArmed = true;
	    ExSetTimer(&m_Timer, xNotificationUpdateDelay);
        LogIllegalPoint(s_FN, 360);
	    return;
	}
}



VOID CNotificationScheduler::GeneralUpdate()
{
	SyncMachineState(*CQueueMgr::GetQMGuid(), NULL);

	HRESULT hr = UpdateAllPublicQueuesInCache();

	if(FAILED(hr))
	{
        TrTRACE(GENERAL, "Failed UpdateAllPublicQueuesInCache() with error %d. Will schedule a retry.",hr);
		LogHR(hr, s_FN, 315);
		throw exception();
	}
}



VOID CNotificationScheduler::ScheduleNotification(const CNotificationBody& Body)
{
	CS Lock(m_cs);

	if(m_fDoGeneralUpdate)
	{
		//
		// An Update of all public queues is scheduled. No need to schedule
		// specific updates
		//
		return;
	}

	if(m_NotificationMap.size() > xLQSTresholdMagicNumber)
	{
		//
		// Too many updates schedulled. Issue a general update.
		//
		m_fDoGeneralUpdate = true;
		m_NotificationMap.clear();
		return;
	}

	CNotificationBody& MappedBody = m_NotificationMap[Body.Guid()];

	if((MappedBody.Event() != neNoEvent) && !(MappedBody.DomainController() == Body.DomainController()))
	{
		//
		// Domain controllers conflict, so synch with local domain controller.
		//
		MappedBody = Body;
		MappedBody.DomainController(L"");
	}
	else
	{
		MappedBody = Body;
	}
		
	if(!m_fTimerArmed)
	{
		m_fTimerArmed = true;

		//
		// The following timing calculations ensure timer will go off at a 
		// maximum rate of 15 minutes, and no sooner then 1 minute from 
		// its setting (assuming more notifications are likely to arrive in
		// that minute).
		//
		CTimeInstant GoOffTime = m_LastTimeFired + xNotificationUpdateDelay;
		CTimeInstant NextMinute = ExGetCurrentTime() + CTimeDuration::OneSecond().Ticks() * 60;

		if(GoOffTime < NextMinute)
		{
			GoOffTime = NextMinute; 
		}

		ExSetTimer(&m_Timer, GoOffTime);
	}
}



static CNotificationScheduler g_NotificationScheduler;



void VerifyBody(const CNotificationBody& Body)
{
	CHLQS hLQS;

	switch(Body.Event())
	{
	case neChangeQueue:
	case neCreateQueue:
		break;

	case neDeleteQueue:	
		//
		// Verify that queue is known to QM. No need to delete otherwise.
		//
		if(SUCCEEDED(LQSOpen(&Body.Guid(), &hLQS, NULL)))
			return;

		TrERROR(GENERAL, "Failed VerifyBody(). Queue is unknown to QM.");
		throw exception();

	case neChangeMachine:
		if(Body.Guid() == *CQueueMgr::GetQMGuid())
			return;

		TrERROR(GENERAL, "Failed VerifyBody(). designated QM is not us.");
		throw exception();

	default:
		TrERROR(GENERAL, "Failed VerifyBody(). Bad body.");
		throw exception();
	}
}



//
// -----------------------------------------------------------------------
//

void
HandleQueueNotification(
    LPWSTR pwcsPathName,
    GUID* pguidIdentifier,
    unsigned char  ucOperation,
    DWORD dwNoOfProps,
    PROPID * pProps,
    PROPVARIANT * pVars
    )
{
    GUID gQueue;


    if ((pwcsPathName != NULL) && (ucOperation != DS_UPDATE_CREATE))
    {
         PROPID aProp[1];
         PROPVARIANT aVar[1];
         HRESULT rc;

         aProp[0] = PROPID_Q_INSTANCE;
         aVar[0].vt = VT_CLSID;
         aVar[0].puuid = &gQueue;

         rc = GetCachedQueueProperties(1, aProp, aVar, NULL, pwcsPathName) ;
         pguidIdentifier = aVar[0].puuid;

         if (FAILED(rc))
             return;
    }

    switch (ucOperation)
    {
        case DS_UPDATE_CREATE:
        {
            ASSERT((pguidIdentifier == NULL) && (pwcsPathName != NULL));
            for (DWORD i=0; i< dwNoOfProps; i++)
            {
                if (pProps[i] == PROPID_Q_INSTANCE)
                {
                    pguidIdentifier = pVars[i].puuid;
                    break;
                }
            }
            ASSERT(pguidIdentifier != NULL);
            UpdateCachedQueueProp(pguidIdentifier, dwNoOfProps, pProps, pVars, time(NULL));
            break;
        }

        case DS_UPDATE_SET:
        {
            ASSERT(pguidIdentifier != NULL);
            UpdateCachedQueueProp(pguidIdentifier, dwNoOfProps, pProps, pVars, time(NULL));
            break;
        }

        case DS_UPDATE_DELETE:
            ASSERT(pguidIdentifier != NULL);
			DeleteCachedQueue(pguidIdentifier);
            break;

        default:
            ASSERT(0);
    }
}

void HandleMachineNotification(unsigned char ucOperation,
                               DWORD dwNoOfProps,
                               PROPID * pProps,
                               PROPVARIANT * pVars)
{
    QUEUE_FORMAT QueueFormat;
   

    if (ucOperation == DS_UPDATE_SET)
    {
        QueueMgr.UpdateMachineProperties(dwNoOfProps, pProps, pVars);
    }
}



//
// Constructor
//
CNotify::CNotify()
{
}


/*====================================================

RoutineName
    CNotify::Init()

Arguments:

Return Value:

=====================================================*/
HRESULT CNotify::Init()
{
    TrTRACE(GENERAL, "Entering CNotify::Init");

	try
	{
		if(!g_fWorkGroupInstallation)
		{
			IntializeQMNotifyRpc();
		}
	}
	catch(const exception&)
	{
        TrERROR(GENERAL, "ERROR : CNotify::Init -> Failed call to IntializeQMNotifyRpc()");
    	LogIllegalPoint(s_FN, 370);
        return LogHR(MQ_ERROR, s_FN, 5);
	}

    QUEUE_FORMAT QueueFormat;
    HRESULT hr = GetNotifyQueueFormat(&QueueFormat);
    if (FAILED(hr))
    {
        TrERROR(GENERAL, "ERROR : CNotify::Init -> couldn't get Admin-Queue from registry!!!");
        return LogHR(hr, s_FN, 10);
    }

    hr= QmpOpenAppsReceiveQueue(&QueueFormat, ReceiveNotifications);
    return LogHR(hr, s_FN, 20);
}


/*====================================================

RoutineName
    CNotify::GetNotifyQueueFormat()

Arguments:

Return Value:

=====================================================*/

HRESULT CNotify::GetNotifyQueueFormat( QUEUE_FORMAT * pQueueFormat)
{
    extern LPTSTR  g_szMachineName;

    HRESULT rc;

	std::wstring FormatName = g_szMachineName;
	FormatName += L'\\';
	FormatName += NOTIFY_QUEUE_NAME;

    rc = g_QPrivate.QMPrivateQueuePathToQueueFormat(FormatName.c_str(), pQueueFormat);

    if (FAILED(rc))
    {
        //
        // The NOTIFY_QUEUE doesn't exist
        //
        LogHR(rc, s_FN, 30);
        return MQ_ERROR;
    }

    ASSERT((pQueueFormat->GetType() == QUEUE_FORMAT_TYPE_PRIVATE) ||
           (pQueueFormat->GetType() == QUEUE_FORMAT_TYPE_DIRECT));

    return MQ_OK;
}

/*====================================================

ValidateServerPacket

Arguments:

Return Value:

=====================================================*/

static BOOL
ValidateServerPacket(
    const CMessageProperty* pmp,
    const GUID         *pGuidQM
    )
{
    //
    // The sender ID must be marked as QM.
    //
    if ((pmp->pSenderID == NULL) ||
        pmp->ulSenderIDType != MQMSG_SENDERID_TYPE_QM)
    {
        TrERROR(GENERAL, "Sender ID type is not QM (%d)", pmp->ulSenderIDType);
        return FALSE;
    }

    //
    // Server packets must be authenticated.
    //
    if (!pmp->bAuthenticated)
    {
        //
        // BUGBUG -
        // Non-authenticated server packets can be received only if we're
        // running on the server it self. This is because we do not go
        // through the QM in this case.
        // So see if the QM guid that is in the packet is identical to the
        // QM guid of our - the local QM.
        //

        if ((pmp->pSignature == NULL) ||
            memcmp(pmp->pSenderID, pGuidQM, sizeof(GUID)) != 0)
        {
            //
            // This still might happen when receiving first replication
            // messages from a new site. This special case is treated by
            // the MQIS. A special debug message is generated to indicate
            // that the message was accepted after all.
            //
            TrERROR(GENERAL, "Received a non-authenticated server message.");
            return FALSE;
        }

        //
        // Yup, so compute the hash value and then validate the signature.
        //
        CHCryptHash hHash;
        CHCryptKey hPbKey;

        //
        // Packet are signed with the base provider. Always.
        //
        HCRYPTPROV hProvQM = NULL;
        HRESULT hr = MQSec_AcquireCryptoProvider(eBaseProvider, &hProvQM);
        if (FAILED(hr))
        {
            TrERROR(GENERAL, "Failed to Acquire crypto provider, %!hresult!", hr);
            return FALSE;
        }

        ASSERT(hProvQM) ;
        if (!CryptCreateHash(hProvQM, pmp->ulHashAlg, 0, 0, &hHash))
        {
			DWORD gle = GetLastError();
            TrERROR(GENERAL,"Failed to create hash, gle = %!winerr!", gle);
            return FALSE;
        }

        hr = HashMessageProperties(
                hHash,
                pmp,
                NULL,
                NULL
                );
        if (FAILED(hr))
        {
            TrERROR(GENERAL, "Failed to HashMessageProperties, %!hresult!", hr);
            return FALSE;
        }

        if (!CryptGetUserKey(hProvQM, AT_SIGNATURE, &hPbKey))
        {
			DWORD gle = GetLastError();
            TrERROR(GENERAL, "Failed to get user key, gle = %!winerr!", gle);
            return FALSE;
        }

        if (!CryptVerifySignature(
                    hHash,
                    pmp->pSignature,
                    pmp->ulSignatureSize,
                    hPbKey,
                    NULL,
                    0
                    ))
        {
			DWORD gle = GetLastError();
            TrERROR(GENERAL, "Failed to verify signature, gle = %!winerr!", gle);
            return FALSE;
        }

    }

    //
    // See that we indeed got the message from a server.
    //
    HRESULT hr;
    PROPID PropId = PROPID_QM_SERVICE;   //[adsrv] Keeping old - query will process.
    PROPVARIANT PropVar;

    PropVar.vt = VT_UI4;
    // This search request will be recognized and specially simulated by DS
    hr = ADGetObjectPropertiesGuid(
            eMACHINE,
            NULL,   // pwcsDomainController
			false,	// fServerName
            (GUID *)pmp->pSenderID,
            1,
            &PropId,
            &PropVar
            );
    if (FAILED(hr))
    {
        TrERROR(GENERAL, "Failed to get the service type, %!hresult!", hr);
        return FALSE;
    }

    // [adsrv] Not changing it, because query will do its work.
    // It is a special case for the query.

    if ((PropVar.ulVal != SERVICE_PEC) &&
        (PropVar.ulVal != SERVICE_PSC) &&
        (PropVar.ulVal != SERVICE_BSC))
    {
        TrERROR(GENERAL, "Received notification not from a server (%d)", PropVar.ulVal);
        return FALSE;
    }

    return TRUE;
}


VOID
WINAPI
ReceiveNotifications(
    const CMessageProperty* pmp,
    const QUEUE_FORMAT* /*pqf*/
    )
{
	try
    {
		if(g_fWorkGroupInstallation)
		{
			TrERROR(GENERAL, "Notifications message received in workgroup mode.");
			return;
		}

		if ( pmp->wClass != MQMSG_CLASS_NORMAL )
		{
			TrERROR(GENERAL, "ReceiveNotifications: wrong message class");
			return;
		}
	
		DWORD dwTotalSize = pmp->dwBodySize;

		if (dwTotalSize < sizeof(CNotificationHeader))
		{
			TrERROR(GENERAL, "Bad notification message size.");
			return;
		}
		
		CNotificationHeader * pNotificationHeader = (CNotificationHeader *)pmp->pBody;

		if (pNotificationHeader->GetVersion() == QM_NOTIFICATION_MSG_VERSION)
		{
			//
			// New format notification, sent from another QM.
			//

			CNotificationBody Body;
			long BodySize = (dwTotalSize - pNotificationHeader->GetBasicSize()) / 2;
			const WCHAR* pBodyData = (const WCHAR*) pNotificationHeader->GetPtrToData();

			Body.MarhshalIn(pBodyData, BodySize);
			
			VerifyBody(Body);
			g_NotificationScheduler.ScheduleNotification(Body);

			return;
		}

		//
		// Should be an old format signed notification, sent from a DS server.
		//

		if ( pNotificationHeader->GetVersion() != DS_NOTIFICATION_MSG_VERSION)
		{
			TrERROR(GENERAL, "Wrong version number of notification message");
			LogIllegalPoint(s_FN, 140);
			return;
		}

		BOOL fSigned = ValidateServerPacket(pmp, QueueMgr.GetQMGuid());

		if(!fSigned)
		{
			TrERROR(GENERAL, "Failed DS notification validation.");
			LogIllegalPoint(s_FN, 145);
			return;
		}

		DWORD sum = pNotificationHeader->GetBasicSize();
		const unsigned char* ptr = pNotificationHeader->GetPtrToData();
  
		for (unsigned char i = 0; i < pNotificationHeader->GetNoOfNotifications(); i++)
		{
			ASSERT (sum < dwTotalSize);
			P<CDSBaseUpdate> pUpdate = new CDSBaseUpdate;

			DWORD size;

			HRESULT hr = pUpdate->Init(ptr,&size);
			if (FAILED(hr))
			{
				//
				// We don't want to read junked values
				// The notification will be ignored
				//
				TrERROR(GENERAL, "Error -  in parsing a received notification");
				LogHR(hr, s_FN, 150);
				break;
			}
			sum+=size;
			ptr+=size;

			switch ( pUpdate->GetObjectType())
			{
				case MQDS_QUEUE:
					HandleQueueNotification( pUpdate->GetPathName(),
											 pUpdate->GetGuidIdentifier(),
											 pUpdate->GetCommand(),
											 pUpdate->getNumOfProps(),
											 pUpdate->GetProps(),
											 pUpdate->GetVars());
					break;
				case MQDS_MACHINE:
					HandleMachineNotification( pUpdate->GetCommand(),
											   pUpdate->getNumOfProps(),
											   pUpdate->GetProps(),
											   pUpdate->GetVars());

					break;
				default:
					TrERROR(GENERAL, "Notification about unexpected object type");
					break;
			}

		}
    }
	catch(const exception&)
	{
        TrERROR(GENERAL, "Exception thrown inside ReceiveNotifications().");
		LogIllegalPoint(s_FN, 350);
	}
}




/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    rtmgmt.cpp

Abstract:

    Management.

Author:

    RaphiR

Revision History:

--*/

#include "stdh.h"
#include "fntoken.h"
#include "mgmtrpc.h"
#include "qmmgmt.h"
#include "rtprpc.h"
#include <mqutil.h>
#include <rtdep.h>

#include "rtmgmt.tmh"

static WCHAR *s_FN=L"rt/rtmgmt";

//---------------------------------------------------------
//
//  Function:
//      RTpMgmtObjectNameToMgmtObject
//
//  Description:
//      Convert a format name string to a Management Object union.
//
//    This function allocates a MGMT_OBJECT, which must
//    be free with RTpMgmtFreeMgmtObject function
//
//---------------------------------------------------------
static
BOOL
RTpMgmtObjectNameToMgmtObject(
    LPCWSTR lpwstrObjectName,
    LPWSTR* ppStringToFree,
    MGMT_OBJECT* pObj,
    QUEUE_FORMAT* pqf
    )
{

    //
    // Handle MACHINE= case
    //
    if(_wcsnicmp(lpwstrObjectName, MO_MACHINE_TOKEN, STRLEN(MO_MACHINE_TOKEN)) == 0)
    {
        pObj->type = MGMT_MACHINE;
        pObj->dwMachineInfo = 0;
        return TRUE;
    }

    //
    // Handle QUEUE= case
    // 
    if(_wcsnicmp(lpwstrObjectName, MO_QUEUE_TOKEN, STRLEN(MO_QUEUE_TOKEN)) == 0)
    {
        pObj->type = MGMT_QUEUE;
        pObj->pQueueFormat = pqf;
        return FnFormatNameToQueueFormat(
                    &lpwstrObjectName[STRLEN(MO_QUEUE_TOKEN) + 1],
                    pqf,
                    ppStringToFree
                    );
    }

    return FALSE;
}


//---------------------------------------------------------
//
//  Function:
//      GetRpcClientHandle
//
//  Description:
//      
//---------------------------------------------------------
static 
HRESULT 
GetRpcClientHandle(   
    handle_t* phBind
    )
{
	WCHAR *wcsStringBinding = NULL;

    RPC_STATUS status = RpcStringBindingCompose( NULL,
                                                 QMMGMT_PROTOCOL,
                                                 NULL,
                                                 g_pwzQmmgmtEndpoint,
                                                 QMMGMT_OPTIONS,
                                                 &wcsStringBinding);
    if (status != RPC_S_OK)
    {
		TrERROR(RPC, "RpcStringBindingCompose failed. Error: %!winerr!", status);
    	return HRESULT_FROM_WIN32(status);
    }

    status = RpcBindingFromStringBinding(wcsStringBinding, phBind);
    
	RpcStringFree(&wcsStringBinding);
	
    if (status != RPC_S_OK)
    {
	    TrERROR(RPC, "RpcBindingFromStringBinding failed. Error: %!winerr!", status);
        return HRESULT_FROM_WIN32(status);
    }
    return MQ_OK;
}


static
HRESULT
RTpMgmtAction(
    HANDLE hBind,
    const MGMT_OBJECT* pMgmtObj,
    LPCWSTR pAction
    )
{
    RpcTryExcept
    {
    	HRESULT hr = R_QMMgmtAction(
                hBind,
                pMgmtObj, 
                pAction
                );
        return LogHR(hr, s_FN, 40);
    }
	RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        HRESULT rc;
        rc = GetExceptionCode();
        LogHR(HRESULT_FROM_WIN32(rc), s_FN, 50); 
        PRODUCE_RPC_ERROR_TRACING;

        if(SUCCEEDED(rc))
        {
            return MQ_ERROR_SERVICE_NOT_AVAILABLE;
        }

        return rc;
    }
	RpcEndExcept
}


HRESULT
LocalMgmtAction(
    const MGMT_OBJECT* pMgmtObj, 
    LPCWSTR pAction
    )
{
    //
    // Get local  RPC binding Handle
    //
    HRESULT hr ;
    handle_t hBind = NULL;
    hr = GetRpcClientHandle(&hBind) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 60);
    }
    ASSERT(hBind != NULL);

    hr =  RTpMgmtAction(
                hBind,
                pMgmtObj,
                pAction
                );

    RpcBindingFree(&hBind);

    return LogHR(hr, s_FN, 70);
}


HRESULT
RemoteMgmtAction(
    LPCWSTR pMachineName,
    const MGMT_OBJECT* pMgmtObj, 
    LPCWSTR pAction
    )
{
    //
    // Call remote administrator
    //
    HRESULT hr;
    hr = MQ_ERROR_SERVICE_NOT_AVAILABLE ;

    CALL_REMOTE_QM(
        const_cast<LPWSTR>(pMachineName), 
        hr, 
        RTpMgmtAction(hBind, pMgmtObj, pAction)
        );

    return LogHR(hr, s_FN, 80);
}


EXTERN_C
HRESULT
APIENTRY
MQMgmtAction(
    IN LPCWSTR pMachineName,
    IN LPCWSTR pObjectName,
    IN LPCWSTR pAction
    )
{
	if(g_fDependentClient)
		return DepMgmtAction(pMachineName, pObjectName, pAction);

	HRESULT hr = RtpOneTimeThreadInit();
	if(FAILED(hr))
		return hr;



    ASSERT(pObjectName);
    ASSERT(pAction);

    QUEUE_FORMAT qf;
    MGMT_OBJECT MgmtObj;

    //
    // Parse the object name
    //
    AP<WCHAR> pStringToFree = NULL;
    try
    {
    	if(!RTpMgmtObjectNameToMgmtObject(pObjectName, &pStringToFree, &MgmtObj, &qf))
	    {
	        return LogHR(MQ_ERROR_ILLEGAL_FORMATNAME, s_FN, 90);
	    }
    }
    catch(const exception&)
    {
    	TrERROR(GENERAL, "RTpMgmtObjectNameToMgmtObject threw an exception");
    	return MQ_ERROR_INSUFFICIENT_RESOURCES;
    }

    CMQHResult rc;
    if (pMachineName == NULL)
    {
        rc = LocalMgmtAction(&MgmtObj, pAction);
        return LogHR(rc, s_FN, 100);
    }
    else
    {
        rc = RemoteMgmtAction(pMachineName, &MgmtObj, pAction);
        return LogHR(rc, s_FN, 110);
    }
}


static
HRESULT
RTpMgmtGetInfo(
    HANDLE hBind,
    const MGMT_OBJECT* pMgmtObj,
    MQMGMTPROPS* pMgmtProps
    )
{
    RpcTryExcept
    {
        HRESULT hr = R_QMMgmtGetInfo(
                hBind,
                pMgmtObj, 
                pMgmtProps->cProp,
                pMgmtProps->aPropID,
                pMgmtProps->aPropVar
                );
        return LogHR(hr, s_FN, 120);

    }
	RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        HRESULT rc;
        rc = GetExceptionCode();
        LogHR(HRESULT_FROM_WIN32(rc), s_FN, 130); 
        PRODUCE_RPC_ERROR_TRACING;

        if(SUCCEEDED(rc))
        {
            return LogHR(MQ_ERROR_SERVICE_NOT_AVAILABLE, s_FN, 140);
        }

        return rc;
    }
	RpcEndExcept
}


static
HRESULT
LocalMgmtGetInfo(
    const MGMT_OBJECT* pMgmtObj, 
    MQMGMTPROPS* pMgmtProps
    )
{
    HRESULT hr ;

    //
    // Get local  RPC binding Handle
    //
    handle_t hBind = NULL;
    hr = GetRpcClientHandle(&hBind) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 150);
    }
    ASSERT(hBind != NULL);

    hr =  RTpMgmtGetInfo(
                hBind,
                pMgmtObj, 
                pMgmtProps
                );

    RpcBindingFree(&hBind);

    return LogHR(hr, s_FN, 160);
}


static
HRESULT
RemoteMgmtGetInfo(
    LPCWSTR pMachineName,
    const MGMT_OBJECT* pMgmtObj, 
    MQMGMTPROPS* pMgmtProps
    )
{
    HRESULT hr;

    //
    // Call remote administrator
    //
    hr = MQ_ERROR_SERVICE_NOT_AVAILABLE ;

    CALL_REMOTE_QM(
        const_cast<LPWSTR>(pMachineName), 
        hr, 
        RTpMgmtGetInfo(hBind, pMgmtObj, pMgmtProps)
        );

    return LogHR(hr, s_FN, 170);
}


EXTERN_C
HRESULT
APIENTRY
MQMgmtGetInfo(
    IN LPCWSTR pMachineName,
    IN LPCWSTR pObjectName,
    IN OUT MQMGMTPROPS* pMgmtProps
    )
{
	if(g_fDependentClient)
		return DepMgmtGetInfo(pMachineName, pObjectName, pMgmtProps);

	HRESULT hr = RtpOneTimeThreadInit();
	if(FAILED(hr))
		return hr;


    ASSERT(pObjectName);
    ASSERT(pMgmtProps);

    QUEUE_FORMAT qf;
    MGMT_OBJECT MgmtObj;

    //
    // Parse the object name
    //
    AP<WCHAR> pStringToFree = NULL;
    try
    {
    	if(!RTpMgmtObjectNameToMgmtObject(pObjectName, &pStringToFree, &MgmtObj, &qf))
	    {
	        return LogHR(MQ_ERROR_ILLEGAL_FORMATNAME, s_FN, 180);
	    }
    }
    catch(const exception&)
    {
	    TrERROR(GENERAL, "RTpMgmtObjectNameToMgmtObject threw an exception");
		return MQ_ERROR_INSUFFICIENT_RESOURCES;
    }

    //
    // Make sure the propvar is set to VT_NULL 
    // (we dont support anything else)
    //
    memset(pMgmtProps->aPropVar, 0, pMgmtProps->cProp * sizeof(PROPVARIANT));
    for (DWORD i = 0; i < pMgmtProps->cProp; ++i)
    {
        pMgmtProps->aPropVar[i].vt = VT_NULL;
    }

	CMQHResult rc;
    if (pMachineName == NULL)
    {
        rc = LocalMgmtGetInfo(&MgmtObj, pMgmtProps);
        return LogHR(rc, s_FN, 190);
    }
    else
    {
        rc = RemoteMgmtGetInfo(pMachineName, &MgmtObj, pMgmtProps);
        return LogHR(rc, s_FN, 200);
    }
}

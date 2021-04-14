/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    rtrpc.cpp

Abstract:

    Rpc related stuff.

Author:

    Doron Juster (DoronJ)  04-Jun-1997

Revision History:

--*/

#include "stdh.h"
#include "mqutil.h"
#include "_mqrpc.h"
#include "mqsocket.h"
#include "ad.h"
#include "rtfrebnd.h"
#include "rtprpc.h"
#include "mgmtrpc.h"
#include "acrt.h"
#include <mqsec.h>

#include "rtrpc.tmh"

CFreeRPCHandles  g_cFreeRpcHandles ;

//
// Fix rpc ports (debug mode), read from registry.
//
#define  MAX_RPC_PORT_LEN  12
static TCHAR   s_wszRpcIpPort[ MAX_RPC_PORT_LEN ] ;
static TCHAR   s_wszRpcIpxPort[ MAX_RPC_PORT_LEN ] ;

static TCHAR   s_wszRpcIpPort2[ MAX_RPC_PORT_LEN ] ;
static TCHAR   s_wszRpcIpxPort2[ MAX_RPC_PORT_LEN ] ;

//
// The binding string MUST be global and kept valid all time.
// If we create it on stack and free it after each use then we can't
// create more then one binding handle.
// Don't ask me (DoronJ) why, but this is the case.
//
TBYTE* g_pszStringBinding = NULL ;
TBYTE* g_pszStringBinding2= NULL ;

//
//  Critical Section to make RPC thread safe.
//
CCriticalSection CRpcCS ;

//
//  License related data.
//
GUID  g_LicGuid ;
BOOL  g_fLicGuidInit = FALSE ;

//
//  Tls index for canceling RPC calls
//
#define UNINIT_TLSINDEX_VALUE   0xffffffff
DWORD  g_hThreadIndex = UNINIT_TLSINDEX_VALUE ;


//
// Local endpoints to QM
//
AP<WCHAR> g_pwzQmsvcEndpoint = 0;
AP<WCHAR> g_pwzQmsvcEndpoint2 = 0;
AP<WCHAR> g_pwzQmmgmtEndpoint = 0;


//---------------------------------------------------------
//
//  RTpUnbindQMService(...)
//
//  Description:
//
//      Set RPC binding handle to the QM service.
//
//  Return Value:
//
//      None
//
//---------------------------------------------------------

void RTpUnbindQMService()
{
   handle_t hBind = tls_hBindRpc;
   g_cFreeRpcHandles.Add(hBind);
   BOOL fSet = TlsSetValue( g_hBindIndex, NULL);
   ASSERT(fSet);
   DBG_USED(fSet);
}

//---------------------------------------------------------
//
//  RTpGetLocalQMBind(...)
//
//  Description:
//
//      Create RPC binding handle to a local QM service.
//
//  Return Value:
//
//      None
//
//---------------------------------------------------------

handle_t RTpGetLocalQMBind( TBYTE** ppStringBinding,
                            LPTSTR  pEndpoint)
{
      RPC_STATUS rc;
      if(!*ppStringBinding)
      {
          rc = RpcStringBindingCompose(
              0,
              RPC_LOCAL_PROTOCOL,
              0,
              pEndpoint,
              RPC_LOCAL_OPTION,
              ppStringBinding
              );

          ASSERT(rc == RPC_S_OK);
      }

      handle_t hBind = 0;
      rc = RpcBindingFromStringBinding(*ppStringBinding, &hBind);
      ASSERT(rc == RPC_S_OK);

      return hBind;
}

//---------------------------------------------------------
//
//  RTpGetQMServiceBind(...)
//
//  Description:
//
//      Create RPC binding handle to the QM service.
//
//  Return Value:
//
//      None
//
//---------------------------------------------------------

handle_t RTpGetQMServiceBind(BOOL fAlternate /*= FALSE*/)
{
    handle_t hBind = 0;
    CS Lock(CRpcCS);

    g_cFreeRpcHandles.FreeAll();

    if (g_fDependentClient)
    {
        HRESULT hr ;
        LPWSTR lpServer = &g_wszRemoteQMName[0];
        ULONG _eAuthnLevel = MQSec_RpcAuthnLevel();

        hr =  RTpBindRemoteQMService(
                       lpServer,
                       &hBind,
                       &_eAuthnLevel,
                       fAlternate
                       );

        if (FAILED(hr))
        {
           ASSERT(hBind == 0);
           hBind = 0;
        }
    }
    else
    {
        if(fAlternate)
        {
            //
            //  Alternate End point is used for receives on win95, so we will
            //  get the correct rundown when the application crashes.
            //  Work only with DCOM95.
            //
            hBind = RTpGetLocalQMBind(&g_pszStringBinding2, g_pwzQmsvcEndpoint2.get());
        }
        else
        {
            hBind = RTpGetLocalQMBind(&g_pszStringBinding, g_pwzQmsvcEndpoint.get());
        }
    }

    return hBind;
}

//---------------------------------------------------------
//
//  RTpBindQMService(...)
//
//  Description:
//
//      Set RPC binding handle to the QM service.
//
//  Return Value:
//
//      None
//
//---------------------------------------------------------

void RTpBindQMService()
{
    handle_t hBind = RTpGetQMServiceBind();

    BOOL fSet = TlsSetValue(g_hBindIndex, hBind);
    ASSERT(fSet) ;
	DBG_USED(fSet);
}

enum DirectQueueType
{
    dqtNONE = 0,
    dqtTCP  = 1,
    dqtSPX  = 2,
    dqtANY  = 3
};

DirectQueueType
GetDirectQueueType (
    LPWSTR* lpwsDirectQueuePath
    )
{

    if (!_wcsnicmp(*lpwsDirectQueuePath, FN_DIRECT_TCP_TOKEN, FN_DIRECT_TCP_TOKEN_LEN))
    {
        *lpwsDirectQueuePath += FN_DIRECT_TCP_TOKEN_LEN;
        return dqtTCP;
    }
    if (!_wcsnicmp(*lpwsDirectQueuePath, FN_DIRECT_OS_TOKEN, FN_DIRECT_OS_TOKEN_LEN))
    {
        *lpwsDirectQueuePath += FN_DIRECT_OS_TOKEN_LEN;
        return dqtANY;
    }

    return (dqtNONE);

}

static
HRESULT ExtractMachineName(
    LPWSTR pQueue,
    AP<WCHAR> &pMachine
    )
{
    //
    // If the remote queue is a direct queue, the routine removes
    // the direct queue type from the queue name. (it keeps with the
    // direct type in the QM to distinguish between different queues).
    // The routine below extract the machine name from the queue name
    // and create the RPC call. If the direct queue type is "TCP" or "SPX"
    // the routine also return the protocol type;
    //
    switch(GetDirectQueueType(&pQueue))
    {
        case dqtTCP:
        case dqtANY:
        case dqtNONE:
            {
                LPWSTR lpwcsSlash = wcschr(pQueue, L'\\') ;
                size_t MachineNameLen = 0 ;
                if (lpwcsSlash)
                {
                    MachineNameLen = lpwcsSlash - pQueue;
                }
                else
                {
                    MachineNameLen = wcslen(pQueue);
                }
                pMachine = new WCHAR[MachineNameLen + 1];
                wcsncpy(pMachine, pQueue, MachineNameLen) ;
                pMachine[MachineNameLen] = '\0';

                return MQ_OK;
            }

        case dqtSPX:
            {

                //
                // For SPX address, remove the ':' from the name
                // need for direct read from SPX direct format name
                //                   Uri Habusha (urih), 15-Sep-98
                //
                LPWSTR pSeparator  = wcschr(pQueue, L':');
				if(pSeparator == NULL)
				{
					TrERROR(GENERAL, "Bad queue name. Missing ':' in SPX address, %ls", pQueue);
					ASSERT(("Bad queue name. Missing ':' in SPX address", 0));
					return MQ_ERROR_INVALID_PARAMETER;
				}

                size_t size = pSeparator - pQueue;

                LPWSTR pQueue2 = pSeparator + 1;
                LPWSTR pSeparator2 = wcschr(pQueue2, L'\\') ;
				if(pSeparator2 == NULL)
				{
					TrERROR(GENERAL, "Bad queue name. Missing '\\' in SPX address, %ls", pQueue);
					ASSERT(("Bad queue name. Missing '\\' in SPX address", 0));
					return MQ_ERROR_INVALID_PARAMETER;
				}
                
				size_t size2 = pSeparator2 - pQueue2;

                pMachine = new WCHAR[wcslen(pQueue) + 2];

                LPWSTR pTempMachine = pMachine;

                wcscpy(pTempMachine, L"~");
                pTempMachine += 1;

                wcsncpy(pTempMachine, pQueue, size);
                pTempMachine += size;

                wcsncpy(pTempMachine, pQueue2, size2);
                pTempMachine += size2;

                *pTempMachine = L'\0';

                return MQ_OK;
            }

		default:
			ASSERT(0);
			return MQ_ERROR;
    }
}

//---------------------------------------------------------
//
//  RTpBindRemoteQMService(...)
//
//  Description:
//
//      Create RPC binding handle to a remote QM service.
//      First try IP then IPx.
//
//  Return Value:
//
//      None
//
//---------------------------------------------------------

HRESULT
RTpBindRemoteQMService(
    IN  LPWSTR     lpwNodeName,
    OUT handle_t*  lphBind,
    IN  OUT ULONG *peAuthnLevel,
    IN  BOOL       fAlternate /*= FALSE*/
    )
{
    AP<WCHAR> wszServer = NULL ;

    HRESULT hr = ExtractMachineName(lpwNodeName, wszServer) ;
	if(FAILED(hr))
		return hr;

    PORTTYPE PortType = IP_HANDSHAKE;

    if (fAlternate)
    {
       PortType = IP_READ;
    }
    else
    {
       PortType = IP_HANDSHAKE;
    }

	GetPort_ROUTINE pfnGetPort = R_QMGetRTQMServerPort;
	
    //
    // Choose authentication service. For LocalSystem services, chose
    // "negotiate" and let mqutil select between Kerberos or ntlm.
    // For all other cases, use ntlm.
    // LocalSystem service go out to network without any credentials
    // if using ntlm, so only for it we're interested in Kerberos.
    // All other are fine with ntlm. For remote read we do not need
    // delegation, so we'll stick to ntlm.
    // The major issue here is a bug in rpc/security, whereas a nt4
    // user on a win2k machine can successfully call
    //  status = RpcBindingSetAuthInfoEx( ,, RPC_C_AUTHN_GSS_KERBEROS,,)
    // although it's clear he can't obtain any Kerberos ticket (he's
    // nt4 user, defined only in nt4 PDC).
    //
    ULONG   ulAuthnSvc = RPC_C_AUTHN_WINNT ;
    BOOL fLocalUser =  FALSE ;
    BOOL fLocalSystem = FALSE ;

    hr = MQSec_GetUserType( NULL,
                           &fLocalUser,
                           &fLocalSystem );
    if (SUCCEEDED(hr) && fLocalSystem)
    {
        ulAuthnSvc = MSMQ_AUTHN_NEGOTIATE;
    }

	hr = mqrpcBindQMService(wszServer,
                            NULL,
                            peAuthnLevel,
                            lphBind,
                            PortType,
                            pfnGetPort,
                            ulAuthnSvc ) ;

    return hr ;
}



//---------------------------------------------------------
//
//  InitRpcGlobals(...)
//
//  Description:
//
//      Initialize RPC related names and other constant data
//
//  Return Value:
//
//---------------------------------------------------------

BOOL InitRpcGlobals()
{
    //
    //  Allocate TLS for  RPC connection with local QM service
    //
    g_hBindIndex = TlsAlloc() ;
    ASSERT(g_hBindIndex != UNINIT_TLSINDEX_VALUE) ;
    if (g_hBindIndex == UNINIT_TLSINDEX_VALUE)
    {
       return FALSE ;
    }

	BOOL fSet = TlsSetValue( g_hBindIndex, NULL ) ;
	if(!fSet)
	{
		DWORD gle = GetLastError();
		TrERROR(GENERAL, "Failed to store a value in TLS. %!winerr!", gle);
		return FALSE;
	}
	
    if (g_fDependentClient)
    {
		HRESULT hr1 = ADInit(
			NULL,
			RTpGetSupportServerInfo,
			false,
			false,
			false,
            false   // fDisableDownlevelNotifications
            );

		if FAILED(hr1)
		{
			TrERROR(GENERAL, "Failed to init AD. %!hresult!", hr1);
			return FALSE;
		}
    }


    //
    // Initialize local endpoints to QM
    //

    ComposeRPCEndPointName(QMMGMT_ENDPOINT, NULL, &g_pwzQmmgmtEndpoint);

    READ_REG_STRING(wzEndpoint, RPC_LOCAL_EP_REGNAME, RPC_LOCAL_EP);
    ComposeRPCEndPointName(wzEndpoint, NULL, &g_pwzQmsvcEndpoint);

    if ((g_dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) ||
        (g_fDependentClient))
    {
        READ_REG_STRING(wzEndpoint, RPC_LOCAL_EP_REGNAME2, RPC_LOCAL_EP2);
        ComposeRPCEndPointName(wzEndpoint, NULL, &g_pwzQmsvcEndpoint2);
    }


    //
    // Read QMID. Needed for licensing.
    //
    DWORD dwValueType = REG_BINARY ;
    DWORD dwValueSize = sizeof(GUID);

    LONG rc = GetFalconKeyValue( MSMQ_QMID_REGNAME,
                            &dwValueType,
                            &g_LicGuid,
                            &dwValueSize);

    if (rc == ERROR_SUCCESS)
    {
        g_fLicGuidInit = TRUE ;
        ASSERT((dwValueType == REG_BINARY) &&
               (dwValueSize == sizeof(GUID)));
    }

    //
    //  Allocate TLS for  cancel remote-read RPC calls
    //
    g_hThreadIndex = TlsAlloc() ;
    ASSERT(g_hThreadIndex != UNINIT_TLSINDEX_VALUE) ;
    if (g_hThreadIndex == UNINIT_TLSINDEX_VALUE)
    {
       return FALSE ;
    }

    fSet = TlsSetValue( g_hThreadIndex, NULL ) ;
    if(!fSet)
	{
		DWORD gle = GetLastError();
		TrERROR(GENERAL, "Failed to store a value in TLS. %!winerr!", gle);
		return FALSE;
	}

    return TRUE ;
}


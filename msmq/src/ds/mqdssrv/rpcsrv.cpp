/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    rpcsrv.cpp

Abstract:

    Implementation of DS CLIENT-SERVER API, server side.

Author:

    ronit hartmann (ronith)
    Ilan Herbst    (IlanH)   9-July-2000 

--*/

#include "stdh.h"
#include "mqutil.h"
#include "_mqdef.h"
#include "_mqrpc.h"
#include "dscomm.h"
#include <uniansi.h>
#include <mqsocket.h>
#include <strsafe.h>

#include "rpcsrv.tmh"

#define  RPCSRV_START_MQIS_IP_EP   2101

static RPC_BINDING_VECTOR *g_pBindings ;  // used for rpc dynamic endpoints.

static WCHAR *s_FN=L"mqdssrv/rpcsrv";


static
RPC_STATUS 
DSSpStartRpcServer(
	void
	)
/*++

Routine Description:
	Rpc Server Start listening.

Arguments:
	None.

Returned Value:
	RPC_S_OK if success, else failure code. 

--*/
{
	//
	// Issuing RPC Listen ourselves.
	//
	// Note for WinNT: all our interfaces are registed as "AUTOLISTEN".
	// The only reason we need this call here is to enable Win95 (and w2k)
	// clients to call us. Otherwise, when Win95 call RpcBindingSetAuthInfo()
	// it will get a Busy (0x6bb) error.
	//  BUGBUG, DoronJ, 15-Jan-97. This may be fixed if SetAuthInfo() specify
	//  a principal name.
	//
	// Note for Win95: On Win95 we must issue the listen.
	// RpcServerRegisterIfEx() was not implemented in the first release of
	// Win95. It was added only in the DCOM95 service pack.
	//
	// This initialization is needed also for w2k.
	// Otherwise RpcMgmt*() will get a Busy (0x6bb) error.
	// This is by design of the RPC but might be changed
	// ilanh 9-July-2000
	//
	
	//
	// fDontWait = TRUE, Functions return immediatly after completing 
	//
	RPC_STATUS status = RpcServerListen( 
							1,
							RPC_C_LISTEN_MAX_CALLS_DEFAULT,
							TRUE  // fDontWait
							);

    TrTRACE(RPC, "MQDSSRV: RpcServerListen() returned 0x%x", status);

	//
	// On WinNT, a listen may be issued by DTC, until they fix their
	// code to use RegisterIfEx() instead of RegisterIf().
	//
	if((status == RPC_S_OK) || (status == RPC_S_ALREADY_LISTENING))
		return (RPC_S_OK);

	ASSERT(("RpcServerListen returned status != RPC_S_OK", 0));

    return LogRPCStatus(status, s_FN, 5);
}


static
RPC_STATUS
DSSpRpcServerUseProtseqEp(
    unsigned short __RPC_FAR * Protseq,
    unsigned int MaxCalls,
    unsigned short __RPC_FAR * Endpoint
    )
/*++

Routine Description:
	Register Dynamic EndPoint and fix EndPoint for the protocol 

Arguments:
	Protseq	- string identifier of the protocol sequence 
	MaxCalls - max rpc calls
	Endpoint - endpoint	(could be null for dynamic endpoint)

Returned Value:
	RPC_S_OK if success, else failure code. 

--*/
{
    //
    // Listen only on IP addresses we get from winsock.
    // This enables multiple QMs to listen each to its
    // own addresses, in a cluster environment. (ShaiK)
    //

    PHOSTENT pHostEntry = gethostbyname(NULL);
    if ((pHostEntry == NULL) || (pHostEntry->h_addr_list == NULL))
    {
        ASSERT(("IP not configured", 0));
        return LogRPCStatus(RPC_S_ACCESS_DENIED, s_FN, 20);
    }

    for ( DWORD ix = 0; pHostEntry->h_addr_list[ix] != NULL; ++ix)
    {
        WCHAR wzAddress[50];
        int retval = ConvertToWideCharString(
		            inet_ntoa(*(struct in_addr *)(pHostEntry->h_addr_list[ix])),
		            wzAddress,
		            TABLE_SIZE(wzAddress)
		            );
        ASSERT(retval != 0);
        DBG_USED(retval);

        RPC_POLICY policy;
        policy.Length = sizeof(policy);
        policy.EndpointFlags = 0;
        policy.NICFlags = 0;

        RPC_STATUS status = RPC_S_OK;
        if (NULL != Endpoint)
        {
            status = I_RpcServerUseProtseqEp2(
                         wzAddress,
                         Protseq,
                         MaxCalls,
                         Endpoint,
                         NULL,
                         &policy
                         );

            TrTRACE(RPC, "MQDSSRV: I_RpcServerUseProtseqEp2 (%ls, %ls, %ls) returned 0x%x", wzAddress, Protseq, Endpoint, status);
        }
        else
        {
            status = I_RpcServerUseProtseq2(
                         wzAddress,
                         Protseq,
                         MaxCalls,
                         NULL,
                         &policy
                         );

            TrTRACE(RPC, "MQDSSRV: I_RpcServerUseProtseq2 (%ls, %ls) returned 0x%x",wzAddress, Protseq, status);
        }

        if (RPC_S_OK != status)
        {
            return LogRPCStatus(status, s_FN, 30);
        }
    }

    return RPC_S_OK;

}


DWORD  DSSpRegisterDynamicEnpoint(
    IN unsigned int  cMaxCalls,
    IN DWORD         dwFirstEP
    )
/*++

Routine Description:
	Register Dynamic EndPoint and fix EndPoint for the protocol 

Arguments:
	cMaxCalls - max rpc calls
	dwFirstEP - first static EndPoint to try

Returned Value:
	The fix endpoint which will be used for the real interface communnication. 

--*/
{
    LPWSTR lpProtocol = RPC_TCPIP_NAME;

    //
    // Register this protocol for dynamic endpoint
    //
	RPC_STATUS status = DSSpRpcServerUseProtseqEp(
							lpProtocol,
							cMaxCalls,
							NULL 	/* Endpoint */
							);
    
	if (status == RPC_S_OK)
	{
		//
		// Now register a fix endpoint which will be used for the real
		// interface communnication.
		//
		WCHAR wszEndpoint[24];
		for (DWORD j = dwFirstEP; j < dwFirstEP + 1000; j = j + 11)
		{
			HRESULT hr = StringCchPrintf(wszEndpoint, TABLE_SIZE(wszEndpoint), L"%lu", j);
			ASSERT(SUCCEEDED(hr));
			DBG_USED(hr);

			status = DSSpRpcServerUseProtseqEp(
						lpProtocol,
						cMaxCalls,
						wszEndpoint
						);

			if (status == RPC_S_OK)
			{
			   return j;
			}

			LogRPCStatus(status, s_FN, 50);
		}
	}

	TrWARNING(RPC, "MQDSSRV: DSSpRV_RegisterDynamicEnpoint: failed to register %ls",lpProtocol);

	return 0;
}

RPC_STATUS RPC_ENTRY dscommSecurityCallback(
	RPC_IF_HANDLE, 
	void* hBind
	)
{	
	TrTRACE(RPC, "dscommSecurityCallback starting");
	
	if (!mqrpcIsTcpipTransport(hBind))
    {
        TrERROR(RPC, "Failed to verify Remote RPC");
		ASSERT_BENIGN(("Failed to verify Remote RPC", 0));
		return ERROR_ACCESS_DENIED;
    }

	TrTRACE(RPC, "dscommSecurityCallback passed successfully");
	return RPC_S_OK;
}




//
// These are the fix ports used for real MQIS interface communication.
//
static DWORD s_dwIPPort  = 0 ;

/*====================================================

Function: RpcServerInit

Arguments:

Return Value:

=====================================================*/

RPC_STATUS RpcServerInit(void)
{
    TrTRACE(DS, "MQDSSRV: RpcServerInit");

    RPC_STATUS  status;
    unsigned int cMaxCalls = RPC_C_LISTEN_MAX_CALLS_DEFAULT;

    //
    // See if we use dynamic or predefined endpoints. By default we use
    // dynamic endpoints.
    //
    BOOL  fUsePredefinedEP =  RPC_DEFAULT_PREDEFINE_DS_EP;
    DWORD ulDefault =  RPC_DEFAULT_PREDEFINE_DS_EP;

    READ_REG_DWORD( 
		fUsePredefinedEP,
		RPC_PREDEFINE_DS_EP_REGNAME,
		&ulDefault 
		);

    BOOL fIPRergistered = FALSE;
    if (fUsePredefinedEP)
    {
		//
		// Read the IP port for RPC.
		//
		WCHAR  wzDsIPEp[MAX_REG_DEFAULT_LEN];
		DWORD  dwSize = sizeof(wzDsIPEp);

		HRESULT hr = GetThisServerIpPort(wzDsIPEp, dwSize);
		if (FAILED(hr))
		{
		   return LogHR(hr, s_FN, 55);
		}

		RPC_STATUS  statusIP =  RpcServerUseProtseqEp( 
									RPC_TCPIP_NAME,
									cMaxCalls,
									wzDsIPEp,
									NULL
									);

		TrTRACE(RPC, "MQDSSRV: RpcServerUseProtseqEp (tcp/ip) returned 0x%x", statusIP);

		fIPRergistered = (statusIP == RPC_S_OK);
    }
    else
    {
		s_dwIPPort = DSSpRegisterDynamicEnpoint( 
						cMaxCalls,
						RPCSRV_START_MQIS_IP_EP 
						);

		fIPRergistered =  (s_dwIPPort != 0);
    }

    if (!fUsePredefinedEP)
    {
		status = RpcServerInqBindings(&g_pBindings);
		if (status == RPC_S_OK)
		{
			status = RpcEpRegister( 
						dscomm_v1_0_s_ifspec,
						g_pBindings,
						NULL,
						L"Message Queuing Downlevel Client Support - V1" 
						);
		}

		TrERROR(DS, "MQDSSRV: Registering Endpoints, status- %lxh", status);

		if (status != RPC_S_OK)
		{
			//
			// can't register endpoints, can't be rpc server
			//
			return(RPC_S_PROTSEQ_NOT_SUPPORTED) ;
		}
		status = RpcEpRegister( 
					dscomm2_v1_0_s_ifspec,
					g_pBindings,
					NULL,
					L"Message Queuing Downlevel Client Support - V2" 
					);

		LogRPCStatus(status, s_FN, 60);

		TrERROR(DS, "MQDSSRV: Registering dscomm2 Endpoints, status- %lxh", status);

		if (status != RPC_S_OK)
		{
		  //
		  // can't register endpoints, can't be rpc server
		  //
		  return LogRPCStatus(RPC_S_PROTSEQ_NOT_SUPPORTED, s_FN, 70);
		}
	}

    if (!fIPRergistered)
    {
		//
		// can't register IP. Quit. We're quite useless.
		//
		TrERROR(DS, "MQDSSRV: RpcServerInit can't use IP");


		//
		// If use some local RPC need to check it here 
		//

		//
		// Can't use any RPC protocol. We're completely useless!
		//
		return LogRPCStatus(RPC_S_PROTSEQ_NOT_SUPPORTED, s_FN, 80);
    }

    //
    // We register this interface as AUTO_LISTEN, so we start listengin now,
    // and do not depend on a call to RpcListen elsewhere.
	// This is TRUE but RpcMgmt* will return error till we explicitly
	// call RpcServerListen()   ilanh 10-July-2000
    //
    status = RpcServerRegisterIfEx( 
				dscomm_v1_0_s_ifspec,
				NULL,
				NULL,
				RPC_IF_AUTOLISTEN | RPC_IF_ALLOW_CALLBACKS_WITH_NO_AUTH,
				cMaxCalls,
				dscommSecurityCallback 
				);

    TrTRACE(RPC, "MQDSSRV: RpcServerRegisterIf returned 0x%x",status);

    if (status != RPC_S_OK)
    {
       return LogRPCStatus(status, s_FN, 90);
    }

    status = RpcServerRegisterIfEx( 
				dscomm2_v1_0_s_ifspec,
				NULL,
				NULL,
				RPC_IF_AUTOLISTEN | RPC_IF_ALLOW_CALLBACKS_WITH_NO_AUTH,
				cMaxCalls,
				dscommSecurityCallback
				);

    TrTRACE(RPC, "MQDSSRV: RpcServerRegisterIf of dscomm2 returned 0x%x", status);

    if (status != RPC_S_OK)
    {
       return LogRPCStatus(status, s_FN, 100);
    }

    status = RpcServerRegisterAuthInfo(
				NULL, 
				RPC_C_AUTHN_WINNT, 
				NULL, 
				NULL
				);

    TrTRACE(DS, "MQDSSRV: RpcServerRegisterAuthInfo(ntlm) returned 0x%x",status);

    if (status != RPC_S_OK)
    {
       return LogRPCStatus(status, s_FN, 110);
    }

    //
    // #3117, for NT5 Beta2
    // Jul/16/1998 RaananH, added kerberos support
    //
    // register kerberos authenticaton
    //
    // kerberos needs principal name
    //
    LPWSTR pwszPrincipalName = NULL;
    status = RpcServerInqDefaultPrincName(
				RPC_C_AUTHN_GSS_KERBEROS,
				&pwszPrincipalName
				);

    TrTRACE(DS, "MQDSSRV: RpcServerInqDefaultPrincName(kerberos) returned 0x%x", status);

    if (status != RPC_S_OK)
    {
       return LogRPCStatus(status, s_FN, 120);
    }
    status = RpcServerRegisterAuthInfo( 
				pwszPrincipalName,
				RPC_C_AUTHN_GSS_KERBEROS,
				NULL,
				NULL 
				);

    RpcStringFree(&pwszPrincipalName);
    TrTRACE(DS, "MQDSSRV: RpcServerRegisterAuthInfo(kerberos) returned 0x%x",status);

    if (status != RPC_S_OK)
    {
       return LogRPCStatus(status, s_FN, 130);
    }

	//
	// Call for RpcServerListen() ourself
	// Without this call although our interfaces are register as AUTO_LISTEN
	// RpcMgmt*() functions will return error
	//
	status = DSSpStartRpcServer();

    return (status);
}

/*====================================================

RoutineName: S_DSGetServerPort()

Arguments: None

Return Value:

=====================================================*/

DWORD
S_DSGetServerPort( 
	/*[in]*/  handle_t /* hBind */,
	/*[in]*/  DWORD fIP
	)
{

	//
	// Error return null port
	//
	if(fIP == (DWORD) -1)
		return 0;

	//
	// Support only IP port
	//
	if(fIP)
	  return s_dwIPPort;

	//
	// Error return null port - dont support IPX
	//
	return 0;
}


/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:
    acrpc.cpp

Abstract:
    Simulate AC interface using RPC to QM

Author:
    Erez Haba (erezh) 1-Oct-96

Revision History:
	Nir Aides (niraides) 23-Aug-2000 - Adaptation for mqrtdep.dll

--*/

#include "stdh.h"
#include "acrt.h"
#include "rtp.h"
#include "_mqrpc.h"
#include "rtprpc.h"
#include <acdef.h>

#include "acrpc.tmh"

static WCHAR *s_FN=L"rtdep/acrpc";


#define ONE_KB 1024


inline
HRESULT
DeppExceptionFilter(
    HRESULT rc
    )
{
    if(FAILED(rc))
    {
        return rc;
    }

    if(rc == ERROR_INVALID_HANDLE)
    {
        return STATUS_INVALID_HANDLE;
    }

    return  MQ_ERROR_SERVICE_NOT_AVAILABLE;
}

HRESULT
ACDepCloseHandle(
    HANDLE hQueue
    )
{
    HRESULT hr ;
    HANDLE hThread ;
    RegisterRpcCallForCancel( &hThread, 0) ;

    RpcTryExcept
    {
        //
        //  Use address of hQueue rather than hQueue, since it is an
        //
        LPMQWIN95_QHANDLE ph95 = (LPMQWIN95_QHANDLE) hQueue ;
        HANDLE hContext = ph95->hContext ;

        hr = rpc_ACCloseHandle(&hContext);

        //
        // Free the binding handle
        //
        mqrpcUnbindQMService( &ph95->hBind,
                              NULL ) ;
        delete ph95 ;
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        hr = DeppExceptionFilter(GetExceptionCode());
        PRODUCE_RPC_ERROR_TRACING;
    }
    RpcEndExcept

    UnregisterRpcCallForCancel( hThread ) ;
    return hr ;
}


HRESULT
ACDepCreateCursor(
    HANDLE hQueue,
    CACCreateRemoteCursor& cc
    )
{
    HRESULT hr ;
    HANDLE hThread ;
    RegisterRpcCallForCancel( &hThread, 0) ;

    RpcTryExcept
    {
        hr = rpc_ACCreateCursorEx(
                    HRTQUEUE(hQueue),
                    &cc
                    );
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        hr = DeppExceptionFilter(GetExceptionCode());
        PRODUCE_RPC_ERROR_TRACING;
    }
	RpcEndExcept

    UnregisterRpcCallForCancel( hThread ) ;
    return hr ;
}

HRESULT
ACDepCloseCursor(
    HANDLE hQueue,
    ULONG hCursor
    )
{
    HRESULT hr ;
    HANDLE hThread ;
    RegisterRpcCallForCancel( &hThread, 0) ;

    RpcTryExcept
    {
        hr = rpc_ACCloseCursor(
                    HRTQUEUE(hQueue),
                    hCursor
                    );
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        hr = DeppExceptionFilter(GetExceptionCode());
        PRODUCE_RPC_ERROR_TRACING;
    }
	RpcEndExcept

    UnregisterRpcCallForCancel( hThread ) ;
    return hr ;
}

HRESULT
ACDepSetCursorProperties(
    HANDLE hProxy,
    ULONG hCursor,
    ULONG  hRemoteCursor
    )
{
    HRESULT hr ;
    HANDLE hThread ;
    RegisterRpcCallForCancel( &hThread, 0) ;

    RpcTryExcept
    {
        hr = rpc_ACSetCursorProperties(
                    HRTQUEUE(hProxy),
                    hCursor,
                    hRemoteCursor
                    );
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        hr = DeppExceptionFilter(GetExceptionCode());
        PRODUCE_RPC_ERROR_TRACING;
    }
	RpcEndExcept

    UnregisterRpcCallForCancel( hThread ) ;
    return hr ;
}

HRESULT
ACDepSendMessage(
    HANDLE hQueue,
    CACTransferBufferV2& tb,
    LPOVERLAPPED /*lpOverlapped*/
    )
{
    HRESULT hr ;
    HANDLE hThread ;
    RegisterRpcCallForCancel( &hThread, 0) ;

    RpcTryExcept
    {
        OBJECTID* pMessageID = 0;
        if(tb.old.ppMessageID)
        {
            pMessageID = *tb.old.ppMessageID;
        }

        hr = rpc_ACSendMessageEx(HRTQUEUE(hQueue), &tb, pMessageID);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
    	DWORD gle = GetExceptionCode();
    	PRODUCE_RPC_ERROR_TRACING;
    	TrERROR(SECURITY, "Failed to send message from Dependent client. %!winerr!", gle);
        hr = DeppExceptionFilter(gle);
    }
	RpcEndExcept

    UnregisterRpcCallForCancel( hThread ) ;
    return hr ;
}

struct AR_CONTEXT {
    HANDLE hEvent;
    HANDLE hQueue;
    CACTransferBufferV2* ptb;
    LPOVERLAPPED lpOverlapped;
};

DWORD
APIENTRY
DeppAsynchronousReceiverThread(
    PVOID pContext
    )
{
    AR_CONTEXT* par = static_cast<AR_CONTEXT*>(pContext);

    HANDLE hQueue = par->hQueue;
    CACTransferBufferV2 tb = *par->ptb;
    LPOVERLAPPED lpOverlapped = par->lpOverlapped;

    //
    //  initialization completed. Release the dispatcher thread
    //
    SetEvent(par->hEvent);

    HANDLE hThread ;
    //
    // note that the cancel routine add five more minutes to this timeout.
    // The five minutes will be applied if server side die and client side
    // has to cancel the rpc call.
    //
    RegisterRpcCallForCancel( &hThread, tb.old.Receive.RequestTimeout) ;

    HRESULT rc = MQ_ERROR;
    RpcTryExcept
    {
        LPMQWIN95_QHANDLE ph95 = (LPMQWIN95_QHANDLE) hQueue ;
        rc = rpc_ACReceiveMessageEx(ph95->hBind, ph95->hQMContext, &tb);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        rc = DeppExceptionFilter(GetExceptionCode());
        PRODUCE_RPC_ERROR_TRACING;
    }
	RpcEndExcept

    UnregisterRpcCallForCancel( hThread ) ;

    //
    //  The next code can cause exception, if the user release the overlapped
    //  structure; or due to incorrect release order of DLLs in Win95. it may
    //  happen that msvcrt is released before we do freeing the heap, thus
    //  causing the exception on process shut down.
    //
    __try
    {
        lpOverlapped->Internal = rc;
        if(lpOverlapped->hEvent)
        {
            SetEvent(lpOverlapped->hEvent);
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        rc = GetExceptionCode();
    }

    return rc;
}

HRESULT
ACDepReceiveMessage(
    HANDLE hQueue,
    CACTransferBufferV2& tb,
    LPOVERLAPPED lpOverlapped
    )
{
    if (tb.old.Receive.Asynchronous == FALSE)
    {
        //
        //  Synchronous recieve, no need to create thread
        //
        HRESULT hr ;
        HANDLE hThread ;
        //
        // note that the cancel routine add five more minutes to this wake
        // time.
        //
        RegisterRpcCallForCancel( &hThread, tb.old.Receive.RequestTimeout) ;

        RpcTryExcept
        {
            LPMQWIN95_QHANDLE ph95 = (LPMQWIN95_QHANDLE) hQueue ;
            hr = rpc_ACReceiveMessageEx(ph95->hBind, ph95->hQMContext, &tb);
        }
	    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
        	DWORD gle = GetExceptionCode();
        	PRODUCE_RPC_ERROR_TRACING;
        	TrERROR(SECURITY, "Failed to receive message on dependent client. %!winerr!.", gle);
            hr = DeppExceptionFilter(gle);
        }
		RpcEndExcept

        UnregisterRpcCallForCancel( hThread ) ;
        return hr ;
    }

    __try
    {
        //
        //  Asynchronous recieve, init context and create receving thread
        //
        AR_CONTEXT ar = {GetThreadEvent(), hQueue, &tb, lpOverlapped};
        ResetEvent(ar.hEvent);

        if(lpOverlapped->hEvent)
        {
            ResetEvent(lpOverlapped->hEvent);
        }

        HANDLE hThread;
        DWORD dwThreadID;
        hThread = CreateThread(
                    0,
                    0,
                    DeppAsynchronousReceiverThread,
                    &ar,
                    0,
                    &dwThreadID
                    );

        if(hThread == 0)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        //  Wait for thread initalization
        //
        DWORD dwResult;
        dwResult = WaitForSingleObject(
                        ar.hEvent,
                        INFINITE
                        );

        ASSERT(dwResult == WAIT_OBJECT_0);
        CloseHandle(hThread);

        return STATUS_PENDING;

    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return DeppExceptionFilter(GetExceptionCode());
    }
}

HRESULT
ACDepHandleToFormatName(
    HANDLE hQueue,
    LPWSTR lpwcsFormatName,
    LPDWORD lpdwFormatNameLength
    )
{
    HRESULT hr ;
    HANDLE hThread ;
    RegisterRpcCallForCancel( &hThread, 0) ;

    RpcTryExcept
    {
        hr = rpc_ACHandleToFormatName(
                    HRTQUEUE(hQueue),
                    min( *lpdwFormatNameLength, ONE_KB),
                    lpwcsFormatName,
                    lpdwFormatNameLength
                    );
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        hr = DeppExceptionFilter(GetExceptionCode());
        PRODUCE_RPC_ERROR_TRACING;
    }
	RpcEndExcept

    UnregisterRpcCallForCancel( hThread ) ;
    return hr ;
}

HRESULT
ACDepPurgeQueue(
    HANDLE hQueue,
    BOOL /*fDelete*/
    )
{
    HRESULT hr ;
    HANDLE hThread ;
    RegisterRpcCallForCancel( &hThread, 0) ;

    RpcTryExcept
    {
        hr = rpc_ACPurgeQueue(HRTQUEUE(hQueue));
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        hr = DeppExceptionFilter(GetExceptionCode());
        PRODUCE_RPC_ERROR_TRACING;
    }
	RpcEndExcept

    UnregisterRpcCallForCancel( hThread ) ;
    return hr ;
}


/*++
    Copyright (c) 1996  Microsoft Corporation

Module Name:
    XactMq.cpp

Abstract:
    This module implements CMQTransaction object

Author:
    Alexander Dadiomov (AlexDad)

--*/
#include "stdh.h"
#include "txdtc.h"
#include "rtprpc.h"
#include "XactMq.h"

#include "xactmq.tmh"

static WCHAR *s_FN=L"rtdep/xactmq";

extern    BOOL g_fClient;

//---------------------------------------------------------------------
// CMQTransaction::CMQTransaction
//---------------------------------------------------------------------
CMQTransaction::CMQTransaction()
{
    m_cRefs = 1;
    m_fCommitedOrAborted = FALSE;
    m_hXact = NULL;
    UuidCreate((UUID *)&m_Uow);
}

//---------------------------------------------------------------------
// CMQTransaction::~CMQTransaction
//---------------------------------------------------------------------
CMQTransaction::~CMQTransaction(void)
{
    if (!m_fCommitedOrAborted)
    {
        Abort(NULL, FALSE, FALSE);
    }
}

//---------------------------------------------------------------------
// CMQTransaction::QueryInterface
//---------------------------------------------------------------------
STDMETHODIMP CMQTransaction::QueryInterface(REFIID i_iid,LPVOID *ppv)
{
    *ppv = 0;                       // Initialize interface pointer.

    if (IID_IUnknown == i_iid)
    {
        *ppv = (IUnknown *)((ITransaction *)this);
    }
    else if (IID_ITransaction == i_iid)
    {
        *ppv = (ITransaction *)this;
    }
    else if (IID_IMSMQTransaction == i_iid)
    {
        *ppv = (IMSMQTransaction *)this;
    }


    if (0 == *ppv)                  // Check for null interface pointer.
    {
        return E_NOINTERFACE;       // from winerror.h
                                    // Neither IUnknown nor IResourceManagerSink
    }
    ((LPUNKNOWN) *ppv)->AddRef();   // Interface is supported. Increment
                                    // its usage count.

    return S_OK;
}


//---------------------------------------------------------------------
// CMQTransaction::AddRef
//---------------------------------------------------------------------
STDMETHODIMP_ (ULONG) CMQTransaction::AddRef(void)
{
    return InterlockedIncrement(&m_cRefs);               // Increment interface usage count.
}


//---------------------------------------------------------------------
// CMQTransaction::Release
//---------------------------------------------------------------------
STDMETHODIMP_ (ULONG) CMQTransaction::Release(void)
{
    // Is anyone using the interface?
    if (InterlockedDecrement(&m_cRefs))
    {                               // The interface is in use.
        return m_cRefs;             // Return the number of references.
    }

    delete this;                    // Interface not in use -- delete!

    return 0;                       // Zero references returned.
}


//---------------------------------------------------------------------
// CMQTransaction::Commit
//---------------------------------------------------------------------
HRESULT CMQTransaction::Commit(BOOL fRetaining, DWORD grfTC, DWORD grfRM)
{
   //------------------------------------------------------------
   // RPC call to QM for prepare/commit
   //------------------------------------------------------------
	HRESULT rc;

	if ((grfTC != 0 && grfTC != XACTTC_SYNC) ||
		grfRM != 0 || fRetaining != FALSE)
	{
		return XACT_E_NOTSUPPORTED;
	}

	RpcTryExcept
	{
		rc = R_QMCommitTransaction(&m_hXact);
		m_fCommitedOrAborted = TRUE;
	}
	RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
	{
		PRODUCE_RPC_ERROR_TRACING;
		rc = (RpcExceptionCode() == RPC_S_CALL_FAILED)? XACT_E_INDOUBT : E_FAIL;
	}
	RpcEndExcept

	return rc;
}

//---------------------------------------------------------------------
// CMQTransaction::Abort
//---------------------------------------------------------------------
HRESULT CMQTransaction::Abort(BOID* /*pboidReason*/, BOOL fRetaining, BOOL fAsync)
{
	if (fAsync || fRetaining)
	{
		return XACT_E_NOTSUPPORTED;
	}

	//------------------------------------------------------------
	// RPC call to QM for prepare/commit
	//------------------------------------------------------------
	HRESULT rc;

	RpcTryExcept
	{
		rc = R_QMAbortTransaction(&m_hXact);
		m_fCommitedOrAborted = TRUE;
	}
	RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
	{
		PRODUCE_RPC_ERROR_TRACING;
		rc = E_FAIL;
	}
	RpcEndExcept

	return rc;
}

//---------------------------------------------------------------------
// CMQTransaction::GetTransactionInfo
//---------------------------------------------------------------------
HRESULT CMQTransaction::GetTransactionInfo(XACTTRANSINFO *pinfo)
{
    ZeroMemory((PVOID)pinfo, sizeof(XACTTRANSINFO));
    CopyMemory((PVOID)&pinfo->uow, (PVOID)&m_Uow, sizeof(XACTUOW));
    return MQ_OK;
}

//---------------------------------------------------------------------
// CMQTransaction::EnlistTransaction
//---------------------------------------------------------------------
HRESULT CMQTransaction::EnlistTransaction(XACTUOW *pUow)
{

    HRESULT hr;

    // No need for several enlistments
    if (m_hXact)
    {
        return MQ_OK;
    }

    // RPC call to QM for enlistment
    RpcTryExcept
    {
        INIT_RPC_HANDLE ;

		if(tls_hBindRpc == 0)
			return MQ_ERROR_SERVICE_NOT_AVAILABLE;

        hr = R_QMEnlistInternalTransaction(tls_hBindRpc, pUow, &m_hXact);
    }
	RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
       DWORD rc = GetExceptionCode();
       TrERROR(XACT_GENERAL, "CMQTransaction::EnlistTransaction failed: RPC code=%x ", rc);
	   DBG_USED(rc);

	   PRODUCE_RPC_ERROR_TRACING;

       hr = MQ_ERROR_SERVICE_NOT_AVAILABLE;
       m_hXact = NULL;
    }
	RpcEndExcept

    if(FAILED(hr))
    {
       TrERROR(XACT_GENERAL, "QMEnlistInternalTransaction failed: %x ", hr);
    }
    return hr;
}


//---------------------------------------------------------------------
//    DepBeginTransaction() - Generates new MSMQ internal transaction
//---------------------------------------------------------------------
EXTERN_C
HRESULT
APIENTRY
DepBeginTransaction(OUT ITransaction **ppTransaction)
{
	ASSERT(g_fDependentClient);

	HRESULT hri = DeppOneTimeInit();
	if(FAILED(hri))
		return hri;

    HRESULT hr = MQ_OK;
    ITransaction* pTransaction = NULL;
    try
    {
        pTransaction = new CMQTransaction;
    }
    catch(const bad_alloc&)
    {
        hr = MQ_ERROR_INSUFFICIENT_RESOURCES;
    }


    if (SUCCEEDED(hr))
    {
        // We are not obliged to enlist immediately, but otherwise Commit for empty xact will fail
        XACTUOW Uow;

        hr = RTpProvideTransactionEnlist(pTransaction, &Uow);
        if(FAILED(hr))
        {
           delete pTransaction;	
           TrERROR(XACT_GENERAL, "RTpProvideTransactionEnlist failed: %x ", hr);
           hr = MQ_ERROR_TRANSACTION_ENLIST;
        }
        else
        {
			*ppTransaction = pTransaction;
        }
    }

    return hr;
}

//---------------------------------------------------------------------
//    DepGetTmWhereabouts() - brings controlling DTC whereabouts
//    This is a private non-published function which we need outside of the DLL
//---------------------------------------------------------------------
HRESULT
DepGetTmWhereabouts(IN  DWORD  cbBufSize,
                   OUT UCHAR *pbWhereabouts,
                   OUT DWORD *pcbWhereabouts)
{
	ASSERT(g_fDependentClient);

    HRESULT hr = MQ_OK;

    // RPC call to QM for getting whereabouts
    RpcTryExcept
    {
        INIT_RPC_HANDLE ;

		if(tls_hBindRpc == 0)
			return MQ_ERROR_SERVICE_NOT_AVAILABLE;

        hr = R_QMGetTmWhereabouts(tls_hBindRpc,
                                cbBufSize,
                                pbWhereabouts,
                                pcbWhereabouts);
    }
	RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
       DWORD rc = GetExceptionCode();
       TrERROR(XACT_GENERAL, "MQGetTmWhereabouts failed: RPC code=%x ", rc);
	   DBG_USED(rc);

	   PRODUCE_RPC_ERROR_TRACING;

       hr = MQ_ERROR_SERVICE_NOT_AVAILABLE;
    }
	RpcEndExcept

    if (FAILED(hr))
    {
       TrERROR(XACT_GENERAL, "QMGetTmWhereabouts failed: %x ", hr);
    }
    return hr;
}


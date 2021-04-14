#ifndef _WORKERREQUEST_HXX_
#define _WORKERREQUEST_HXX_

#include "asynccontext.hxx"
#include <reftrace.h>
#include <acache.hxx>
//
// We are either reading a new request, or are processing the request
//
        
enum NREQ_STATE
{
    NREQ_STATE_START = 0,
    NREQ_STATE_READ,
    NREQ_STATE_PROCESS,
    NREQ_STATE_ERROR,
    NREQ_STATE_CLIENT_CERT
};

enum NREQ_STATUS
{
    NSTATUS_NEXT = 0,
    NSTATUS_PENDING
};

#define INLINE_REQUEST_BUFFER_LEN           (sizeof(HTTP_REQUEST)+2048)

#ifdef _WIN64
#define INLINE_ALLOCATION_BUFFER_LEN        (8192)
#else
#define INLINE_ALLOCATION_BUFFER_LEN        (5120)
#endif

#define UL_NATIVE_REQUEST_CS_SPINS          (200)
#define DESIRED_PENDING_REQUESTS            (20)
#define DEFAULT_MAX_FREE_REQUESTS           (250)

//
// When we're getting a client cert, the initialize buffer size we'll use
//

#define INITIAL_CERT_INFO_SIZE              (1500)

//
// UL_NATIVE_REQUEST.  Each object represents the context of handling a 
// single HTTP_REQUEST
//

#define UL_NATIVE_REQUEST_SIGNATURE       CREATE_SIGNATURE( 'NREQ')
#define UL_NATIVE_REQUEST_SIGNATURE_FREE  CREATE_SIGNATURE( 'nreq')

class UL_NATIVE_REQUEST : public ASYNC_CONTEXT
{
public:

    UL_NATIVE_REQUEST(
        VOID
    );

    ~UL_NATIVE_REQUEST(
        VOID
    );

    VOID * 
    operator new( 
        size_t            size
    )
    {
        if ( size != sizeof( UL_NATIVE_REQUEST ) )
        {
            DBG_ASSERT( size == sizeof( UL_NATIVE_REQUEST ) );
            return NULL;        
        }
        
        DBG_ASSERT( sm_pachNativeRequests != NULL );
        return sm_pachNativeRequests->Alloc();
    }
    
    VOID
    operator delete(
        VOID *              pNativeRequest
    )
    {
        DBG_ASSERT( pNativeRequest != NULL );
        DBG_ASSERT( sm_pachNativeRequests != NULL );
        
        DBG_REQUIRE( sm_pachNativeRequests->Free( pNativeRequest ) );
    }

    //
    // The state machine advancer
    //

    VOID
    DoWork(
        DWORD           cbData,
        DWORD           dwError,
        LPOVERLAPPED    lpo
    );
    
    //
    // Global list of worker requests
    //

    VOID
    RemoveFromRequestList(
        VOID
    );

    VOID
    AddToRequestList(
        VOID
    );

    //
    // Reference counting
    //
    
    VOID
    ReferenceWorkerRequest(
        VOID
    );
    
    VOID
    DereferenceWorkerRequest(
        VOID
    );
    
    //
    // Configure # of requests to serve
    //
    
    static
    VOID
    SetRestartCount(
        ULONG           cRequests
    )
    {
        sm_cRestart = cRequests;
    }
    
    HTTP_REQUEST *
    QueryHttpRequest(
        VOID
    ) const
    {
        return (HTTP_REQUEST *) _pbBuffer;
    }

    VOID
    ResetContext(
        VOID
    );

    VOID
    SetContext(
        PVOID           pvContext
    )
    {
        _pvContext = pvContext;
    }

    HRESULT
    SendResponse(
        BOOL                  fAsync,
        DWORD                 dwFlags,
        HTTP_RESPONSE        *pResponse,
        HTTP_CACHE_POLICY    *pCachePolicy,
        DWORD                *pcbSent,
        HTTP_LOG_FIELDS_DATA *pUlLogData
    );

    HRESULT
    SendEntity(
        BOOL                  fAsync,
        DWORD                 dwFlags,
        USHORT                cChunks,
        HTTP_DATA_CHUNK *     pChunks,
        DWORD                *pcbSent,
        HTTP_LOG_FIELDS_DATA *pUlLogData
    );

    HRESULT
    ReceiveEntity(
        BOOL                fAsync,
        DWORD               dwFlags,
        VOID *              pBuffer,
        DWORD               cbBuffer,
        DWORD *             pBytesReceived
    );

    HRESULT
    ReceiveClientCertificate(
        BOOL                        fAsync,
        BOOL                        fDoCertMap,
        HTTP_SSL_CLIENT_CERT_INFO **ppClientCertInfo
    );

    BOOL
    CheckSignature(
        VOID
    ) const
    {
        return _dwSignature == UL_NATIVE_REQUEST_SIGNATURE;
    }

    //
    // Allocate some per-request memory
    //

    VOID *
    AllocateMemory(
        DWORD                   cbSize
    );

    //
    // Global UL_NATIVE_REQUEST methods
    //

    static
    HRESULT
    Initialize(
        VOID
    );

    static
    VOID
    StopListening(
        VOID
    );

    static
    VOID
    Terminate(
        VOID
    );

    static
    HRESULT
    AddPendingRequests(
        DWORD               cItems
    );

    static
    HRESULT
    ReleaseAllWorkerRequests(
        VOID
    );

    static
    DWORD
    QueryCurrentRequests(
        VOID
    )
    {
        return sm_cRequests - sm_cRequestsPending;
    }
    
    static 
    VOID
    FreeWorkerRequest(
        UL_NATIVE_REQUEST *         pWorkerRequest
    );
    
    static
    UL_NATIVE_REQUEST *
    AllocateWorkerRequest(
        VOID
    );
    
    static
    VOID
    PushFreeList(
        UL_NATIVE_REQUEST *         pWorkerRequest
    )
    {
#ifdef _WIN64
        sm_FreeListLock.WriteLock();
       
        PushEntryList( &sm_FreeList, &pWorkerRequest->_FreeListEntry );
        
        sm_FreeListLock.WriteUnlock();
#else
        RtlInterlockedPushEntrySList( &sm_FreeList, &pWorkerRequest->_FreeListEntry );
#endif    
    }
    
    static
    UL_NATIVE_REQUEST *
    PopFreeList(
        VOID
    )
    {
        SINGLE_LIST_ENTRY *         pListEntry;
        UL_NATIVE_REQUEST *         pRequest = NULL;
        
#ifdef _WIN64
        sm_FreeListLock.WriteLock();
        
        pListEntry = PopEntryList( &sm_FreeList );
        
        sm_FreeListLock.WriteUnlock();
#else
        pListEntry = RtlInterlockedPopEntrySList( &sm_FreeList );
#endif

        if ( pListEntry != NULL )
        {
            pRequest = CONTAINING_RECORD( pListEntry,
                                          UL_NATIVE_REQUEST,
                                          _FreeListEntry );
        }
        
        return pRequest;
    }
    
private:

    HTTP_CONNECTION_ID
    QueryConnectionId(
        VOID
    ) const
    {
        return QueryHttpRequest()->ConnectionId;
    }

    HTTP_REQUEST_ID
    QueryRequestId(
        VOID
    ) const
    {
        return QueryHttpRequest()->RequestId;
    }

    //
    // private helper for destructor and Reset()
    //
    
    VOID
    Cleanup(
        VOID
    );
    
    //
    // Reset the worker process to its pristine state
    //

    VOID
    Reset(
        VOID
    );
    
    //
    // State implementors
    //
    
    NREQ_STATUS
    DoStateStart(
        VOID
    );

    NREQ_STATUS
    DoStateRead(
        VOID
    );
    
    NREQ_STATUS
    DoStateProcess(
        VOID
    );
    
    NREQ_STATUS
    DoStateClientCertificate(
        VOID
    );

    //
    // Determine if we need to send a "restart count reached" message to
    // the admin process. Note that we want to send this once, thus the
    // use of InterlockedExchange() on the static member. Note also that
    // we first test the flag with non-interlocked access to avoid bus
    // thrash.
    //

    static
    BOOL
    NeedToSendRestartMsg(
        VOID
    )
    {
        if( sm_RestartMsgSent == 1 ) 
        {
            return FALSE;
        }

        return InterlockedExchange( &sm_RestartMsgSent, 1 ) == 0;
    }

    DWORD               _dwSignature;
    LONG                _cRefs;
    SINGLE_LIST_ENTRY   _FreeListEntry;
    NREQ_STATE          _ExecState;
    LIST_ENTRY          _ListEntry;
    PVOID               _pvContext;
    DWORD               _cbAsyncIOData;            // Data transferred in the last Async IO
    DWORD               _dwAsyncIOError;           // Error code from the last Async IO
    UCHAR               _achBuffer[ INLINE_REQUEST_BUFFER_LEN ];
    UCHAR *             _pbBuffer;
    DWORD               _cbBuffer;
    BUFFER              _buffClientCertInfo;
    DWORD               _dwClientCertFlags;
    HTTP_SSL_CLIENT_CERT_INFO * _pClientCertInfo;
    
    BYTE                _abAllocateMemory[ INLINE_ALLOCATION_BUFFER_LEN ];
    DWORD               _cbAllocateMemoryOffset;

    //
    // static members
    //
    
    static ULONG            sm_cRequestsServed;
    static ULONG            sm_cRestart;
    static LONG             sm_RestartMsgSent;
    static LIST_ENTRY       sm_RequestListHead;
    static CRITICAL_SECTION sm_csRequestList;
    static DWORD            sm_cRequests;
    static PTRACE_LOG       sm_pTraceLog;

#ifdef _WIN64
    static SINGLE_LIST_ENTRY sm_FreeList;
    static CSpinLock        sm_FreeListLock;
#else
    static SLIST_HEADER     sm_FreeList;
#endif
    static DWORD            sm_cFreeRequests;
    static DWORD            sm_cMaxFreeRequests;
    
    static DWORD            sm_cRequestsPending;
    static DWORD            sm_cDesiredPendingRequests;
    static BOOL             sm_fAddingRequests;
    
    static ALLOC_CACHE_HANDLER *    sm_pachNativeRequests;
};

#endif



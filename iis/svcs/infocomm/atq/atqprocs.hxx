/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :
      atqprocs.hxx

   Abstract:
      ATQ function prototypes and externs

   Author:

       Murali R. Krishnan    ( MuraliK )    1-June-1995

   Environment:
       User Mode -- Win32

   Project:

       Internet Services Common DLL

   Revision History:

--*/

#ifndef _ATQPROCS_H_
#define _ATQPROCS_H_



/************************************************************
 *   Data
 *     for detailed doc on global data, please see atqmain.cxx
 ************************************************************/


// ------------------------------
// Configuration for ATQ package
// ------------------------------

extern DWORD   g_cConcurrency;
extern BOOL    g_fUseAcceptEx; // Use AcceptEx if available
extern BOOL    g_fUseDriver; // Use NTS kernel driver
extern DWORD   g_cbMinKbSec;
extern DWORD   g_msThreadTimeout;
extern LONG    g_cMaxThreads;
extern LONG    g_cMaxThreadLimit;
extern DWORD   g_dwNumContextLists;
extern ATQ_THREAD_EXIT_CALLBACK g_pfnExitThreadCallback;

extern BOOL    g_fEnableDebugThreads;
extern BOOL    g_fCreateDebugThread;
extern DWORD   g_cForceTimeout;
extern BOOL    g_fDisableBacklogMonitor;

// ------------------------------
// Current State Information
// ------------------------------

extern DWORD   g_cListenBacklog;
extern LONG    g_cThreads;
extern LONG    g_cAvailableThreads;
extern HANDLE  g_hCompPort;
extern HANDLE  g_hShutdownEvent;
extern BOOL    g_fShutdown;

// ------------------------------
// Various State/Object Lists
// ------------------------------

extern DWORD AtqGlobalContextCount;
extern DWORD g_AtqCurrentTick;

extern ATQ_CONTEXT_LISTHEAD AtqActiveContextList[];

extern LIST_ENTRY AtqEndpointList;
extern CRITICAL_SECTION AtqEndpointLock;

extern PALLOC_CACHE_HANDLER  g_pachAtqContexts;

//
// DLL handles
//

extern HINSTANCE g_hMSWsock;

//
// Ensure that initialization/termination don't happen at the same time
//

extern CRITICAL_SECTION g_csInitTermLock;



/************************************************************
 *   Functions
 ************************************************************/

VOID  AtqValidateProductType( VOID );

//
// ATQ Context alloc/free functions
//

BOOL
I_AtqCheckThreadStatus(
                IN PVOID Context = NULL
                );

// for adding initial listen socket to the port
// for adding non-AcceptEx() AtqContext()
BOOL
I_AtqAddAsyncHandle(
    IN OUT PATQ_CONT  *    ppatqContext,
    IN PATQ_ENDPOINT       pEndpoint,
    PVOID                  ClientContext,
    ATQ_COMPLETION         pfnCompletion,
    DWORD                  TimeOut,
    HANDLE                 hAsyncIO
    );

// for adding an AcceptEx atq context to the atq processing
BOOL
I_AtqAddAsyncHandleEx(
    PATQ_CONT    *         ppatqContext,
    PATQ_ENDPOINT          pEndpoint,
    PATQ_CONT              pReuseableAtq
    );


BOOL
I_AtqPrepareAcceptExSockets(
    IN PATQ_ENDPOINT pEndpoint,
    IN DWORD         nAcceptExSockets
    );

BOOL
I_AtqAddAcceptExSocket(
    IN PATQ_ENDPOINT          pEndpoint,
    IN PATQ_CONT              patqContext
    );

BOOL
I_AtqAddListenEndpointToPort(
    IN OUT PATQ_CONT    * ppatqContext,
    IN PATQ_ENDPOINT    pEndpoint
    );

VOID
I_AtqProcessPendingListens(
    IN PATQ_CONTEXT_LISTHEAD pContextList,
    IN PATQ_ENDPOINT    pEndpoint,
    OUT PDWORD          pcForced
    );

BOOL
I_AtqStartTimeoutProcessing(
    IN PVOID Context
    );

BOOL
I_AtqStopTimeoutProcessing(
    VOID
    );

BOOL
I_AtqInitializeNtEntryPoints(
    VOID
    );

BOOL
I_AtqTransmitFileAndRecv(
    IN PATQ_CONTEXT             patqContext,            // pointer to ATQ context
    IN HANDLE                   hFile,                  // handle of file to read
    IN DWORD                    dwBytesInFile,          // Bytes to transmit
    IN LPTRANSMIT_FILE_BUFFERS  lpTransmitBuffers,      // transmit buffer structure
    IN DWORD                    dwTFFlags,              // TF Flags
    IN LPWSABUF                 pwsaBuffers,            // Buffers for recv
    IN DWORD                    dwBufferCount
    );

BOOL
I_AtqSendAndRecv(
    IN PATQ_CONTEXT             patqContext,            // pointer to ATQ context
    IN LPWSABUF                 pwsaSendBuffers,        // buffers for send
    IN DWORD                    dwSendBufferCount,      // count of buffers for send
    IN LPWSABUF                 pwsaRecvBuffers,        // Buffers for recv
    IN DWORD                    dwRecvBufferCount       // count of buffers for recv
    );

VOID
AtqpReuseOrFreeContext(
    IN PATQ_CONT    pContext,
    IN BOOL      fReuseContext
    );

VOID
AtqpProcessContext( IN PATQ_CONT  pAtqContext,
                    IN DWORD      cbWritten,
                    IN LPOVERLAPPED lpo,
                    IN BOOL       fRet);

typedef
HANDLE
(*PFN_CREATE_COMPLETION_PORT) (
            IN HANDLE hFile,
            IN HANDLE hPort,
            IN ULONG_PTR dwKey,
            IN DWORD  nThreads
            );

typedef
BOOL
(*PFN_CLOSE_COMPLETION_PORT) (
            IN HANDLE hFile
            );

typedef
BOOL
(WINAPI *PFN_GET_QUEUED_COMPLETION_STATUS)(
    HANDLE CompletionPort,
    LPDWORD lpNumberOfBytesTransferred,
    PULONG_PTR lpCompletionKey,
    LPOVERLAPPED *lpOverlapped,
    DWORD dwMilliseconds
    );

typedef
BOOL
(WINAPI *PFN_POST_COMPLETION_STATUS)(
    HANDLE CompletionPort,
    DWORD dwNumberOfBytesTransferred,
    ULONG_PTR dwCompletionKey,
    LPOVERLAPPED lpOverlapped
    );

typedef
BOOL
(*PFN_ACCEPTEX) (
    IN SOCKET sListenSocket,
    IN SOCKET sAcceptSocket,
    IN PVOID lpOutputBuffer,
    IN DWORD dwReceiveDataLength,
    IN DWORD dwLocalAddressLength,
    IN DWORD dwRemoteAddressLength,
    OUT LPDWORD lpdwBytesReceived,
    IN LPOVERLAPPED lpOverlapped
    );

typedef
VOID
(*PFN_GETACCEPTEXSOCKADDRS) (
    IN PVOID lpOutputBuffer,
    IN DWORD dwReceiveDataLength,
    IN DWORD dwLocalAddressLength,
    IN DWORD dwRemoteAddressLength,
    OUT struct sockaddr **LocalSockaddr,
    OUT LPINT LocalSockaddrLength,
    OUT struct sockaddr **RemoteSockaddr,
    OUT LPINT RemoteSockaddrLength
    );

typedef
BOOL
(*PFN_TRANSMITFILE) (
    IN SOCKET hSocket,
    IN HANDLE hFile,
    IN DWORD nBytesWrite,
    IN DWORD nBytesPerSend,
    IN LPOVERLAPPED lpo,
    IN LPTRANSMIT_FILE_BUFFERS lpTransmitBuffer,
    IN DWORD dwReserved
    );

typedef
BOOL
(WINAPI *PFN_READ_DIR_CHANGES_W)(
    HANDLE hDirectory,
    LPVOID lpBuffer,
    DWORD nBufferLength,
    BOOL bWatchSubtree,
    DWORD dwNotifyFilter,
    LPDWORD lpBytesReturned,
    LPOVERLAPPED lpOverlapped,
    LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
    );

extern PFN_ACCEPTEX g_pfnAcceptEx;
extern PFN_GETACCEPTEXSOCKADDRS g_pfnGetAcceptExSockaddrs;
extern PFN_TRANSMITFILE g_pfnTransmitFile;
extern PFN_READ_DIR_CHANGES_W g_pfnReadDirChangesW;
extern PFN_GET_QUEUED_COMPLETION_STATUS g_pfnGetQueuedCompletionStatus;
extern PFN_CREATE_COMPLETION_PORT g_pfnCreateCompletionPort;
extern PFN_CLOSE_COMPLETION_PORT  g_pfnCloseCompletionPort;
extern PFN_POST_COMPLETION_STATUS g_pfnPostCompletionStatus;

//
// inlined functions
//

inline BOOL
I_AddAtqContextToPort(IN PATQ_CONT  pAtqContext)
{
    ATQ_ASSERT( g_hCompPort );
    return  (g_pfnCreateCompletionPort(
                                    pAtqContext->hAsyncIO,
                                    g_hCompPort,
                                    (ULONG_PTR)pAtqContext,
                                    g_cConcurrency
                                    ) != NULL
             );
} // I_AddContextToPort()

inline DWORD CanonTimeout( DWORD Timeout)
    // Convert the timeout into normalized ATQ timeout value
{
    return ((Timeout == INFINITE) ? ATQ_INFINITE :
            ((Timeout + ATQ_TIMEOUT_INTERVAL - 1)/ATQ_TIMEOUT_INTERVAL + 1)
            );
} // CanonTimeout()

inline DWORD UndoCanonTimeout( DWORD Timeout)
    // Convert the timeout from normalized ATQ timeout into values in seconds
{
    return ( ((Timeout & ATQ_INFINITE) != 0) ? INFINITE :
             ((Timeout - 1) * ATQ_TIMEOUT_INTERVAL)
             );
} // UndoCanonTimeout()


#endif // _ATQPROCS_H_


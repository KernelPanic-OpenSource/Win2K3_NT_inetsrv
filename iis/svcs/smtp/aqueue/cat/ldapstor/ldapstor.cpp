//
// ldapstor.cpp -- This file contains implementations for
//      GetEmailIDStore
//      ReleaseEmailIDStore
//
// Created:
//      December 18, 1996   -- Milan Shah (milans)
//
// Changes:
//

#include "precomp.h"
#include "ldapstor.h"
#include "propstr.h"
#include "ccataddr.h"
#include "icatparam.h"
#include "cnfgmgr.h"

const DWORD CEmailIDLdapStore<CCatAddr>::Signature = (DWORD) 'IMAB';

DWORD CEmailIDLdapStore<CCatAddr>::m_dwDynamicDlPageSize = CAT_DEFAULT_DYNAMICDL_PAGE_SIZE;

//+----------------------------------------------------------------------------
//
//  Function:   GetEmailIDStore
//
//  Synopsis:   Instantiates an object of class
//              CEmailIDStore
//
//  Arguments:  [ppStore] -- On successful return, contains pointer to
//                  newly allocated object. Free this object using
//                  ReleaseEmailIDStore.
//
//  Returns:    TRUE if successful, FALSE otherwise
//
//-----------------------------------------------------------------------------

template <>
HRESULT
GetEmailIDStore(
    CEmailIDStore<CCatAddr> **ppStore)
{
    HRESULT hr;
    CEmailIDLdapStore<CCatAddr> *pLdapStore;

    pLdapStore = new CEmailIDLdapStore<CCatAddr>;
    if (pLdapStore != NULL) {
        hr = S_OK;
    } else {
        hr = E_OUTOFMEMORY;
    }

    *ppStore = (CEmailIDStore<CCatAddr> *) pLdapStore;

    return( hr );
}

//+----------------------------------------------------------------------------
//
//  Function:   ReleaseEmailIDStore
//
//  Synopsis:   Frees up instance of CEmailIDStore allocated by
//              GetEmailIDStore
//
//  Arguments:  [pStore] -- Pointer to CEmailIDStore to free.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

template <>
VOID
ReleaseEmailIDStore(
    CEmailIDStore<CCatAddr> *pStore)
{
    delete (CEmailIDLdapStore<CCatAddr> *)pStore;
}

//+----------------------------------------------------------------------------
//
//  Function:   CEmailIDLdapStore::Initialize
//
//  Synopsis:   Initializes a CEmailIDLdapStore object.
//
//  Arguments:  [pICatParams] -- ICategorizerParams to set default
//                  parameters (based on szLdapInfo), and to save and
//                  query for config info
//              [pISMTPServer] -- Interface to hold onto for
//                  triggering server events
//
//  Returns:    TRUE if successfully initialized, FALSE otherwise
//
//-----------------------------------------------------------------------------

template <class T> HRESULT CEmailIDLdapStore<T>::Initialize(
    ICategorizerParametersEx *pICatParams,
    ISMTPServer *pISMTPServer)
{
    CatFunctEnterEx((LPARAM)this, "CEmailIDLdapStore<T>::Initialize");
    HRESULT hr;

    _ASSERT(pICatParams);

    LPSTR pszUser;
    LPSTR pszDomain;
    ULARGE_INTEGER ulCurrentTime;

    //
    // Do not try to reinitialize more than once/5 minutes
    //
    GetSystemTimeAsFileTime((LPFILETIME)&ulCurrentTime);

    if( (ulCurrentTime.QuadPart - m_ulLastInitTime.QuadPart) <
        ((LONGLONG)CAT_LDAPSTORE_MIN_INIT_INTERVAL * 10000000)) {

        DebugTrace((LPARAM)this, "Not reinitializing for 5 minutes");
        hr = CAT_E_INIT_FAILED;
        ERROR_LOG("--Insufficient time--");
        goto CLEANUP;
    }


    //
    // Save and addref the interface pointers
    //
    if(m_pICatParams)
        m_pICatParams->Release();
    if(m_pISMTPServer)
        m_pISMTPServer->Release();
    if(m_pISMTPServerEx)
        m_pISMTPServerEx->Release();

    m_pICatParams = pICatParams;
    m_pISMTPServer = pISMTPServer;

    // QI for ISMTPServerEx for event logging
    if (m_pISMTPServer)
    {
        hr = m_pISMTPServer->QueryInterface(
                IID_ISMTPServerEx,
                (LPVOID *)&m_pISMTPServerEx);
        if (FAILED(hr))
        {
            ErrorTrace((LPARAM) m_pISMTPServer,
                "Unable to QI for ISMTPServerEx 0x%08X",hr);
    
            m_pISMTPServerEx = NULL;
            hr = S_OK; //Don't treat as a fatal error
        }
    }

    m_pICatParams->AddRef();

    if(m_pISMTPServer)
        m_pISMTPServer->AddRef();

    //
    // Retrieve Host, NamingContext, Account, and Password from ICategorizerParameters
    // Initialize our domain cache.
    //

    hr = RetrieveICatParamsInfo(
        &m_pszHost,
        &m_dwPort,
        &m_pszNamingContext,
        &pszUser,
        &pszDomain,
        &m_pszPassword,
        &m_bt);

    if(FAILED(hr)) {

        ERROR_LOG("RetrieveICatParamsInfo");
        hr = CAT_E_INVALID_ARG;
        goto CLEANUP;
    }

    //
    // Note that NamingContext is an optional configuration.
    //
    if (((pszUser) && (pszUser[0] != 0) ||
         m_bt == BIND_TYPE_NONE ||
         m_bt == BIND_TYPE_CURRENTUSER)) {

        hr = AccountFromUserDomain(
            m_szAccount,
            sizeof(m_szAccount),
            pszUser,
            pszDomain);
        ERROR_CLEANUP_LOG("AccountFromUserDomain");
    }
    //
    // We are a new emailIdStore initializing with a possibly
    // different config, so reset the event log stuff
    //
    ResetPeriodicEventLogs();
    //
    // Create/Initialize the connection configuration manager
    //
    // Initialize the ldap configuration manager
    // Use the automatic init if we have no specified host/port
    //
    if( ((m_pszHost == NULL) || (*m_pszHost == '\0')) &&
        (m_dwPort == 0)) {

        if(m_pCLdapCfgMgr == NULL)
            m_pCLdapCfgMgr = new CLdapCfgMgr(
                m_pISMTPServerEx,
                TRUE,           // fAutomaticConfigUpdate
                m_pICatParams,
                m_bt,
                m_szAccount,
                m_pszPassword,
                m_pszNamingContext);

        if(m_pCLdapCfgMgr == NULL) {
            hr = E_OUTOFMEMORY;
            ERROR_LOG("new CLdapCfgMgr");
            goto CLEANUP;
        }
        hr = m_pCLdapCfgMgr->HrInit();
        if(FAILED(hr)) {
            ERROR_LOG("m_pCLdapCfgMgr->HrInit");
        }

    } else {
        //
        // Initialize using the one configuration specified
        //
        LDAPSERVERCONFIG ServerConfig;
        ServerConfig.dwPort = m_dwPort;
        ServerConfig.pri = 0;
        ServerConfig.bt = m_bt;
        if(m_pszHost)
            lstrcpyn(ServerConfig.szHost, m_pszHost, sizeof(ServerConfig.szHost));
        else
            ServerConfig.szHost[0] = '\0';

        if(m_pszNamingContext)
            lstrcpyn(ServerConfig.szNamingContext, m_pszNamingContext, sizeof(ServerConfig.szNamingContext));
        else
            ServerConfig.szNamingContext[0] = '\0';

        lstrcpyn(ServerConfig.szAccount, m_szAccount, sizeof(ServerConfig.szAccount));

        if(m_pszPassword)
            lstrcpyn(ServerConfig.szPassword, m_pszPassword, sizeof(ServerConfig.szPassword));
        else
            ServerConfig.szPassword[0] = '\0';

        //
        // Create CLdapCfgMgr without the automatic config update
        // option (since one host is specified)
        //
        if(m_pCLdapCfgMgr == NULL)
            m_pCLdapCfgMgr = new CLdapCfgMgr(
                m_pISMTPServerEx,
                FALSE,          // fAutomaticConfigUpdate
                m_pICatParams);

        if(m_pCLdapCfgMgr == NULL) {
            hr = E_OUTOFMEMORY;
            ERROR_LOG("new CLdapCfgMgr");
            goto CLEANUP;
        }

        hr = m_pCLdapCfgMgr->HrInit(
            1,
            &ServerConfig);
        if(FAILED(hr)) {
            ERROR_LOG("m_pCLdapCfgMgr->HrInit");
        }
    }

    if(FAILED(hr)) {

        FatalTrace((LPARAM)this, "CLdapCfgMgr->HrInit failed hr %08lx", hr);
        m_pCLdapCfgMgr->Release();
        m_pCLdapCfgMgr = NULL;
        goto CLEANUP;
    }
    
    InitializeFromRegistry();

 CLEANUP:
    if(FAILED(hr) &&
       (hr != CAT_E_INIT_FAILED)) {
        //
        // Update the last init attempt time
        //
        GetSystemTimeAsFileTime((LPFILETIME)&m_ulLastInitTime);
    }

    DebugTrace((LPARAM)this, "returning hr %08lx", hr);
    CatFunctLeaveEx((LPARAM)this);
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Function:   CEmailIDLdapStore::InitializeFromRegistry
//
//  Synopsis:   Initializes registry configurable parameters.
//
//  Arguments:  None.
//
//  Returns:    Nothing.
//
//-----------------------------------------------------------------------------

template <>
VOID CEmailIDLdapStore<CCatAddr>::InitializeFromRegistry()
{
    HKEY hkey;
    DWORD dwErr, dwType, dwValue, cbValue;

    dwErr = RegOpenKey(HKEY_LOCAL_MACHINE, DYNAMICDL_PAGE_SIZE_KEY, &hkey);

    if (dwErr == ERROR_SUCCESS) {

        cbValue = sizeof(dwValue);
        dwErr = RegQueryValueEx(
                    hkey,
                    DYNAMICDL_PAGE_SIZE_VALUE,
                    NULL,
                    &dwType,
                    (LPBYTE) &dwValue,
                    &cbValue);

        if (dwErr == ERROR_SUCCESS && dwType == REG_DWORD && dwValue > 0) {

            InterlockedExchange(
                (PLONG) &m_dwDynamicDlPageSize,
                (LONG)dwValue);
                
        } else {
        
            InterlockedExchange(
                (PLONG) &m_dwDynamicDlPageSize,
                CAT_DEFAULT_DYNAMICDL_PAGE_SIZE);
        }

        RegCloseKey( hkey );
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   CEmailIDLdapStore::InitializeResolveListContext
//
//  Synopsis:   Creates a new async context for resolving a list of email ids.
//
//  Arguments:  [puserContext] -- As each name is completed, a completion
//                      routine is called with this context parameter.
//              [pResolveListContext] -- The LPRESOLVE_LIST_CONTEXT
//                      to initialize.
//
//  Returns:    S_OK if successfully allocated context
//              E_OUTOFMEMORY if out of memory.
//
//-----------------------------------------------------------------------------

template <class T> HRESULT CEmailIDLdapStore<T>::InitializeResolveListContext(
    VOID  *pUserContext,
    LPRESOLVE_LIST_CONTEXT pResolveListContext)
{
    CStoreListResolveContext *pCStoreContext = NULL;
    HRESULT hr;

    CatFunctEnterEx((LPARAM)this, "CEmailIDLdapStore::InitializeResolveListContext");

    pResolveListContext->pUserContext = pUserContext;

    pCStoreContext = new CStoreListResolveContext(this);
    if(pCStoreContext == NULL) {
        hr = E_OUTOFMEMORY;
        ERROR_LOG("new CStoreListResolveContext");
        goto CLEANUP;
    }

    hr = pCStoreContext->HrInitialize(
        m_pISMTPServer,
        m_pICatParams);

    if (FAILED(hr)) {

        ERROR_LOG("pCStoreContext->HrInitialize");

        if(hr == HRESULT_FROM_WIN32(ERROR_LOGON_FAILURE)) {

            LPCSTR pszAccount = m_szAccount;

            CatLogEvent(
                m_pISMTPServerEx,
                CAT_EVENT_LOGON_FAILURE,
                1,
                &pszAccount,
                hr,
                m_szAccount,
                LOGEVENT_FLAG_PERIODIC,
                LOGEVENT_LEVEL_MINIMUM);

        } else {

            // Use new logging interface so that
            // we can log system generic format messages
            // rgszString[1] here will be set inside LogEvent(..)
            // as it use FormatMessageA to generate it
            const char *rgszStrings[1] = { NULL };

            CatLogEvent(
                m_pISMTPServerEx,
                CAT_EVENT_LDAP_CONNECTION_FAILURE,
                1,
                rgszStrings,
                hr,
                NULL,
                LOGEVENT_FLAG_PERIODIC,
                LOGEVENT_LEVEL_MEDIUM);
        }

        goto CLEANUP;

    } else {

        ResetPeriodicEventLogs();
    }

    pResolveListContext->pStoreContext = (LPVOID) pCStoreContext;

 CLEANUP:
    if(FAILED(hr))
        if(pCStoreContext)
            pCStoreContext->Release();

    DebugTrace((LPARAM)this, "returning hr %08lx", hr);
    CatFunctLeaveEx((LPARAM)this);
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Function:   CEmailIDLdapStore::FreeResolveListContext
//
//  Synopsis:   Frees async context used for resolving list of email ids.
//
//  Arguments:  [pResolveListContext] -- The context to free.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

template <class T> VOID CEmailIDLdapStore<T>::FreeResolveListContext(
    LPRESOLVE_LIST_CONTEXT pResolveListContext)
{
    CStoreListResolveContext *pCStoreContext;

    pCStoreContext = (CStoreListResolveContext *) pResolveListContext->pStoreContext;

    pCStoreContext->Release();
}

//+----------------------------------------------------------------------------
//
//  Function:   CEmailIDLdapStore::LookupEntryAsync
//
//  Synopsis:   Issues a lookup request asynchronously. The callback function
//              is called once the result is available. This function is
//              used when a group of lookups are to be issued successively,
//              for example when looking up all recipients of a mail message.
//              By doing an asynchronous lookup, an opportunity to perform
//              group-wide optimizations (like batching a sequence of lookups
//              together) is created.
//
//  Arguments:  [pCCatAddr] -- Contains email ID to lookup and
//              HrCompletion routine to be called when lookup is complete.
//              [pListContext] -- Context associated with the group of lookups
//                  of which this lookup is a part.
//
//  Returns:    S_OK if lookup was successfully queued.
//              The callback function gets passed the result of the actual
//              lookup
//
//-----------------------------------------------------------------------------
template <class T> HRESULT CEmailIDLdapStore<T>::LookupEntryAsync(
    T *pCCatAddr,
    LPRESOLVE_LIST_CONTEXT pListContext)
{
    HRESULT hr = S_OK;
    CStoreListResolveContext *pCStoreContext = NULL;

    CatFunctEnterEx((LPARAM)this, "CEmailIDLdapStore::LookupEntryAsync");
    //
    // Pick up CStoreListResolveContext object from pListContext.
    // Pass it through.
    //
    pCStoreContext = (CStoreListResolveContext *) pListContext->pStoreContext;

    hr = pCStoreContext->HrLookupEntryAsync(
        pCCatAddr);
    if(FAILED(hr))
    {
        ERROR_LOG("pCStoreContext->HrLookupEntryAsync");
    }

    DebugTrace((LPARAM)this, "returning %08lx", hr);
    CatFunctLeaveEx((LPARAM)this);
    return hr;
}


//+------------------------------------------------------------
//
// Function: CEmailIDLdapStore<T>::InsertInsertionRequest
//
// Synopsis:
//
// Arguments:
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1999/03/25 15:13:55: Created.
//
//-------------------------------------------------------------
template <class T> HRESULT CEmailIDLdapStore<T>::InsertInsertionRequest(
        LPRESOLVE_LIST_CONTEXT pListContext,
        CInsertionRequest *pCRequest)
{
    HRESULT hr = S_OK;
    CStoreListResolveContext *pCStoreContext = NULL;

    CatFunctEnterEx((LPARAM)this, "CEmailIDLdapStore::InsertInsertionRequest");
    //
    // Pick up CStoreListResolveContext object from pListContext.
    // Pass it through.
    //
    pCStoreContext = (CStoreListResolveContext *) pListContext->pStoreContext;

    hr = pCStoreContext->HrInsertInsertionRequest(
        pCRequest);
    if(FAILED(hr))
    {
        ERROR_LOG("pCStoreContext->HrInsertInsertionRequest");
    }

    DebugTrace((LPARAM)this, "returning %08lx", hr);
    CatFunctLeaveEx((LPARAM)this);
    return hr;
} // CEmailIDLdapStore<T>::InsertInsertionRequest



//+------------------------------------------------------------
//
// Function: CEmailIDLdapStore::CancelResolveList
//
// Synopsis: Cancel outstanding lookups in a list resolve
//
// Arguments:
//  pResolveListContext: list context
//  hr: Optional hresult reason to pass to completion routines
//
// Returns:
//  return value of CAsyncLookupContext::CancelPendingRequests
//
// History:
// jstamerj 1998/09/29 14:51:30: Created.
//
//-------------------------------------------------------------
template <class T> HRESULT CEmailIDLdapStore<T>::CancelResolveList(
    LPRESOLVE_LIST_CONTEXT pResolveListContext,
    HRESULT hr)
{
    CStoreListResolveContext *pCStoreContext;
    //
    // Cancel lookups on this resolve list context (will call their
    // lookup's completion routine with error)
    //
    pCStoreContext = (CStoreListResolveContext *) pResolveListContext->pStoreContext;
    pCStoreContext->Cancel();

    return S_OK;
}


//+----------------------------------------------------------------------------
//
//  Function:   CEmailIDLdapStore::CancelAllLookups
//
//  Synopsis:   Cancels all async lookups that are pending
//
//  Arguments:  NONE
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

template <class T> VOID CEmailIDLdapStore<T>::CancelAllLookups()
{
    if(m_pCLdapCfgMgr)
        m_pCLdapCfgMgr->CancelAllConnectionSearches(
            m_pISMTPServer);
}


//+----------------------------------------------------------------------------
//
//  Function:   CEmailIDLdapStore::AsyncLookupCompletion
//
//  Synopsis:   Completion routine for
//
//  Arguments:  pCCatAddr: the address lookup being completed
//              lpContext: context passed to LdapConn
//
//  Returns:    NOTHING
//
//
//-----------------------------------------------------------------------------
template <class T> VOID CEmailIDLdapStore<T>::AsyncLookupCompletion(
    CCatAddr *pCCatAddr,
    LPVOID lpContext)
{
    CatFunctEnter("CEmailIDLdapStore::AsyncLookupCompletion");

    _ASSERT(pCCatAddr);
    pCCatAddr->LookupCompletion();

    pCCatAddr->Release(); // Release reference count addref'd in LookupEntryAsync

    CatFunctLeave();
}

//+------------------------------------------------------------
//
// Function: CEmailIDLdapStore::HrExpandPagedDlMembers
//
// Synopsis: Start issueing async queries to retrieve all the DL members
//
// Arguments:
//  pCCatAddr: The DL item to be expanded
//  pListContext: List context initialized in
//                InitializeResolveListContext
//  CAType: The type of address of the DL members
//  pfnCompletion: Completion that will be called after returning
//                 MAILTRANSPORT_S_PENDING
//  pContext: Paramter passed to the completion routine
//
// Returns:
//  S_OK: Success, this is not a paged DL
//  MAILTRANSPORT_S_PENDING: Will call pfnCompletion with context when
//                           finished expanding the DL
//  E_OUTOFMEMORY
//  CAT_E_DBCONNECTION: palc->GetConnection returned NULL (meaning it
//                      is having problems obtaining/maintaing a connection)
//  error from HrExpandDlPage
//
// History:
// jstamerj 1998/09/23 15:57:37: Created.
//
//-------------------------------------------------------------
template <class T> HRESULT CEmailIDLdapStore<T>::HrExpandPagedDlMembers(
    CCatAddr *pCCatAddr,
    LPRESOLVE_LIST_CONTEXT pListContext,
    CAT_ADDRESS_TYPE CAType,
    PFN_DLEXPANSIONCOMPLETION pfnCompletion,
    PVOID pContext)
{
    HRESULT hr;
    ICategorizerItemAttributes *pICatItemAttr = NULL;
    PMEMBERRESOLVECONTEXT pMemCtx = NULL;
    CStoreListResolveContext *pCStoreContext = NULL;
    CBatchLdapConnection *pConn = NULL;

    CatFunctEnterEx((LPARAM)this,
                      "CEmailIDLdapStore::HrExpandPagedDlMembers");

    //
    // Use the same CLdapConnection that the rest of the list is using
    // -- this way the same thread will be servicing all the list
    // resolve requests and we don't have to worry about thread unsafe
    // problems in CAsyncLookupContext
    //
    pCStoreContext = (CStoreListResolveContext *) pListContext->pStoreContext;
    pConn = pCStoreContext->GetConnection();
    if(pConn == NULL) {
        ErrorTrace((LPARAM)this, "Failed to get a connection to resolve paged DL");
        hr = CAT_E_DBCONNECTION;
        ERROR_LOG_ADDR(pCCatAddr, "pCStoreContext->GetConnection");
        goto CLEANUP;
    }

    //
    // Get the attributes interface
    //
    hr = pCCatAddr->GetICategorizerItemAttributes(
        ICATEGORIZERITEM_ICATEGORIZERITEMATTRIBUTES,
        &pICatItemAttr);

    if(FAILED(hr)) {
        pICatItemAttr = NULL;
        ErrorTrace((LPARAM)this, "Failed to get ICatItemAttr in HrExpandPagedDlMembers");
        ERROR_LOG_ADDR(pCCatAddr, "pCCatAddr->GetICategorizerItemAttributes");
        goto CLEANUP;
    }

    //
    // Allocate/initialize a member resolution context and
    // kick things off
    //
    pMemCtx = new MEMBERRESOLVECONTEXT;
    if(pMemCtx == NULL) {
        hr = E_OUTOFMEMORY;
        ERROR_LOG_ADDR(pCCatAddr, "new MEMBERRESOLVECONTEXT");
        goto CLEANUP;
    }

    //
    // First, get an ldapconn on which to issue searches
    //
    pMemCtx->pConn = pConn;

    //
    // AddRef here, release when we're done
    //
    pCCatAddr->AddRef();

    pMemCtx->pStore = this;
    pMemCtx->pCCatAddr = pCCatAddr;
    pMemCtx->CAType = CAType;
    pMemCtx->dwNextBlockIndex = 0;
    pMemCtx->pICatItemAttr = NULL;
    pMemCtx->hrResolveStatus = S_OK;
    pMemCtx->pfnCompletion = pfnCompletion;
    pMemCtx->pCompletionContext = pContext;

    hr = HrExpandDlPage(pMemCtx, pICatItemAttr);
    if(FAILED(hr)) {
        ERROR_LOG_ADDR(pCCatAddr, "HrExpandDlPage");
    }

 CLEANUP:
    if(hr != MAILTRANSPORT_S_PENDING) {

        if(pMemCtx) {
            if(pConn)
                pConn->Release();
            if(pMemCtx->pCCatAddr)
                pMemCtx->pCCatAddr->Release();
            delete pMemCtx;
        }
    }

    if(pICatItemAttr)
        pICatItemAttr->Release();

    CatFunctLeaveEx((LPARAM)this);
    return hr;
}


//+------------------------------------------------------------
//
// Function: CEmailIdLdapStore::HrExpandDlPage
//
// Synopsis: Expand one block of DL members
//
// Arguments:
//  pMemCtx: one (initialized) member resolve context
//  pICatItemAttr: The ICatItemAttributes to get the members from
//
// Returns:
//  S_OK: Success
//  MAILTRANSPORT_S_PENDING: Issued another search
//
// History:
// jstamerj 1998/09/23 17:02:05: Created.
//
//-------------------------------------------------------------
template <class T> HRESULT CEmailIDLdapStore<T>::HrExpandDlPage(
    PMEMBERRESOLVECONTEXT pMemCtx,
    ICategorizerItemAttributes *pICatItemAttr)
{
    HRESULT hr;
    LPSTR pszMembersAttribute;
    LPSTR pszAttributeName;
    DWORD dwMembersAttributeLength;
    BOOL fEnumerating = FALSE;
    ATTRIBUTE_ENUMERATOR enumerator;

    CatFunctEnterEx((LPARAM)this,
                      "CEmailIDLdapStore::HrExpandDlPage");

    hr = m_pICatParams->GetDSParameterA(
        DSPARAMETER_ATTRIBUTE_DL_MEMBERS,
        &pszMembersAttribute);
    ERROR_CLEANUP_LOG("m_pICatParams->GetDSParameterA");

    dwMembersAttributeLength = lstrlen(pszMembersAttribute);

    //
    // Is the members attribute being paged at all?
    //
    hr = pICatItemAttr->BeginAttributeNameEnumeration(&enumerator);
    ERROR_CLEANUP_LOG_ADDR(pMemCtx->pCCatAddr, "pICatItemAttr->BeginAttributeNameEnumeration");

    fEnumerating = TRUE;

    hr = pICatItemAttr->GetNextAttributeName(
        &enumerator,
        &pszAttributeName);

    while(SUCCEEDED(hr)) {
        //
        // We'll know it's paged DL when we see an attribue named
        // "member;range=0-high"
        //
        if( (_strnicmp(pszAttributeName,
                       pszMembersAttribute,
                       dwMembersAttributeLength) == 0) &&
            (_strnicmp(pszAttributeName + dwMembersAttributeLength,
                       SZ_PAGEDMEMBERS_INDICATOR,
                       sizeof(SZ_PAGEDMEMBERS_INDICATOR) -1 ) == 0)) {
            //
            // Parse out the range numbers
            //
            CHAR  szTempBuffer[MAX_PAGEDMEMBERS_DIGITS+1];
            LPSTR pszSrc, pszDest;
            DWORD dwLow, dwHigh;

            pszSrc = pszAttributeName +
                     dwMembersAttributeLength +
                     sizeof(SZ_PAGEDMEMBERS_INDICATOR) - 1;

            pszDest = szTempBuffer;

            while((*pszSrc != '-') && (*pszSrc != '\0') &&
                  (pszDest - szTempBuffer) < (sizeof(szTempBuffer) - 1)) {
                //
                // Copy the digits into the temporary buffer
                //
                *pszDest = *pszSrc;
                pszSrc++;
                pszDest++;
            }

            if(*pszSrc != '-') {
                //
                // Error parsing this thing (no hyphen?)
                //
                ErrorTrace((LPARAM)this, "Error parsing LDAP attribute \"%s\"",
                           pszAttributeName);
                hr = E_INVALIDARG;
                ERROR_LOG_ADDR(pMemCtx->pCCatAddr, "--no hyphen--");
                goto CLEANUP;
            }
            //
            // Null terminate the temporary buffer
            //
            *pszDest = '\0';
            //
            // Convert to a dword
            //
            dwLow = atol(szTempBuffer);

            //
            // Is this the range we're looking for?
            //
            if(dwLow == pMemCtx->dwNextBlockIndex) {
                //
                // Copy the high number into the buffer
                //
                pszDest = szTempBuffer;
                pszSrc++; // Past -

                while((*pszSrc != '\0') &&
                      (pszDest - szTempBuffer) < (sizeof(szTempBuffer) - 1)) {

                    *pszDest = *pszSrc;
                    pszSrc++;
                    pszDest++;
                }
                *pszDest = '\0';

                if(szTempBuffer[0] == '*') {

                    dwHigh = 0; // we're done expanding

                } else {

                    dwHigh = atol(szTempBuffer);
                }

                hr = pMemCtx->pCCatAddr->HrExpandAttribute(
                    pICatItemAttr,
                    pMemCtx->CAType,
                    pszAttributeName,
                    NULL);

                if(SUCCEEDED(hr) && dwHigh > 0) {

                    pMemCtx->dwNextBlockIndex = dwHigh + 1;

                    hr = HrExpandNextDlPage( pMemCtx );
                } else if(FAILED(hr)) {
                    ERROR_LOG_ADDR(pMemCtx->pCCatAddr, "pMemCtx->pCCatAddr->HrExpandAttribute");
                }
                //
                // The job of this function is done
                //
                goto CLEANUP;
            }
        }
        hr = pICatItemAttr->GetNextAttributeName(
            &enumerator,
            &pszAttributeName);
    }
    //
    // If we did not find any members;range= attribute, assume there
    // are no more members
    //
    hr = S_OK;

 CLEANUP:
    if(fEnumerating)
        pICatItemAttr->EndAttributeNameEnumeration(&enumerator);

    return hr;
}

//+------------------------------------------------------------
//
// Function: CEmailIdLdapStore::HrExpandNextDlPage
//
// Synopsis: Issue an LDAP search to fetch the next block of members
//
// Arguments:
//  pMemCtx: The initialized MEMBERRESOLVECONTEXT
//
// Returns:
//  MAILTRANSPORT_S_PENDING: Issued the search
//  E_INVALIDARG: One of the parameters was too large to fit in the
//                  fixed size attribute buffer
//  or error from LdapConn
//
// History:
// jstamerj 1998/09/23 18:01:51: Created.
//
//-------------------------------------------------------------
template <class T> HRESULT CEmailIDLdapStore<T>::HrExpandNextDlPage(
    PMEMBERRESOLVECONTEXT pMemCtx)
{
    HRESULT hr;
    CMembershipPageInsertionRequest *pCInsertion = NULL;

    CatFunctEnterEx((LPARAM)this,
                      "CEmailIDLdapStore::HrExpandNextDlPage");

    _ASSERT(pMemCtx);
    _ASSERT(pMemCtx->pCCatAddr);

    pCInsertion = new CMembershipPageInsertionRequest(
        pMemCtx);

    if(pCInsertion == NULL) {
        hr = E_OUTOFMEMORY;
        ERROR_LOG_ADDR(pMemCtx->pCCatAddr, "new CMembershipPageInsertionRequest");
        goto CLEANUP;
    }

    hr = pMemCtx->pConn->HrInsertInsertionRequest(
        pCInsertion);
    ERROR_CLEANUP_LOG_ADDR(pMemCtx->pCCatAddr, "pMemCtx->pConn->HrInsertInsertionRequest");
        
    //
    // If AsyncSearch succeeded, it is always pending
    //
    hr = MAILTRANSPORT_S_PENDING;

 CLEANUP:
    if(pCInsertion)
        pCInsertion->Release();

    DebugTrace((LPARAM)this, "HrExpandDlPage returning hr %08lx", hr);
    CatFunctLeaveEx((LPARAM)this);
    return hr;
}


//+------------------------------------------------------------
//
// Function: CEmailIDLdapStore::AsyncExpandDlCompletion
//
// Synopsis: Handle completion of an async lookup for DL members
//
// Arguments:
//  ctx: pMemCtx passed to AsyncSearch
//  dwNumResults: Number of objects matching search filter
//  rgpICatItemAttrs: Array of ICatItemAttributes
//  hr: Resolution status
//  fFinalCompletion: Indicates wether this is a partial completion or
//                    the last completion call
//
// Returns: NOTHING
//
// History:
// jstamerj 1998/09/24 09:28:18: Created.
//
//-------------------------------------------------------------
template <class T> VOID CEmailIDLdapStore<T>::AsyncExpandDlCompletion(
    LPVOID ctx,
    DWORD  dwNumResults,
    ICategorizerItemAttributes **rgpICatItemAttrs,
    HRESULT hrResolveStatus,
    BOOL fFinalCompletion)
{
    HRESULT hr = S_OK;
    PMEMBERRESOLVECONTEXT pMemCtx = NULL;
    CBatchLdapConnection *pConn = NULL;

    CatFunctEnter("CEmailIDLdapStore::AsyncExpandDlCompleton");

    pMemCtx = (PMEMBERRESOLVECONTEXT) ctx;
    _ASSERT(pMemCtx);

    pConn = pMemCtx->pConn;
    pConn->AddRef();

    //
    // Get/Release insertion context so that inserted queries will be batched
    //
    pConn->GetInsertionContext();

    //
    // If we had a previous failure for this resolution, do nothing
    //
    if(FAILED(pMemCtx->hrResolveStatus)) {

        hr = pMemCtx->hrResolveStatus;
        goto CLEANUP;
    }

    if(FAILED(hrResolveStatus)) {
        //
        // Handle failures in the cleanup code
        //
        hr = hrResolveStatus;
        ERROR_LOG_ADDR_STATIC(
            pMemCtx->pCCatAddr, 
            "async",
            pMemCtx,
            pMemCtx->pStore->GetISMTPServerEx());
        goto CLEANUP;
    }

    //
    // If we haven't yet found our search result, look for it
    //
    if(pMemCtx->pICatItemAttr == NULL) {
        //
        // Which result is ours?
        //  We need to find the result that matches the
        //  distinguishingattribute/value.  We do not need to worry
        //  about multiple matches (the first search and match in
        //  asyncctx would have caught that)
        //
        LPSTR pszDistinguishingAttribute;
        LPSTR pszDistinguishingAttributeValue;
        DWORD dwCount;
        BOOL  fFound;
        ICategorizerItemAttributes *pICatItemAttr;

        hr = pMemCtx->pCCatAddr->GetStringAPtr(
            ICATEGORIZERITEM_DISTINGUISHINGATTRIBUTE,
            &pszDistinguishingAttribute);
        ERROR_CLEANUP_LOG_ADDR_STATIC(
            pMemCtx->pCCatAddr, 
            "pMemCtx->pCCatAddr->GetSTringAPtr(distinguishingattribute)",
            pMemCtx,
            pMemCtx->pStore->GetISMTPServerEx());

        hr = pMemCtx->pCCatAddr->GetStringAPtr(
            ICATEGORIZERITEM_DISTINGUISHINGATTRIBUTEVALUE,
            &pszDistinguishingAttributeValue);
        ERROR_CLEANUP_LOG_ADDR_STATIC(
            pMemCtx->pCCatAddr, 
            "pMemCtx->pCCatAddr->GetStringAPtr(distinguishingattributevalue",
            pMemCtx,
            pMemCtx->pStore->GetISMTPServerEx());

        //
        // Find the result matching the search request
        //
        for(fFound = FALSE, dwCount = 0;
            (fFound == FALSE) && (dwCount < dwNumResults);
            dwCount++) {

            ATTRIBUTE_ENUMERATOR enumerator;
            LPSTR pszObjectAttributeValue;
            pICatItemAttr = rgpICatItemAttrs[dwCount];

            hr = pICatItemAttr->BeginAttributeEnumeration(
                pszDistinguishingAttribute,
                &enumerator);

            if(SUCCEEDED(hr)) {

                hr = pICatItemAttr->GetNextAttributeValue(
                    &enumerator,
                    &pszObjectAttributeValue);

                while(SUCCEEDED(hr) && (fFound == FALSE)) {
                    if(lstrcmpi(
                        pszObjectAttributeValue,
                        pszDistinguishingAttributeValue) == 0) {

                        fFound = TRUE;
                    }

                    hr = pICatItemAttr->GetNextAttributeValue(
                        &enumerator,
                        &pszObjectAttributeValue);
                }
                hr = pICatItemAttr->EndAttributeEnumeration(
                    &enumerator);
            }
        }

        if(fFound) {
            //
            // Save the found result
            //
            pMemCtx->pICatItemAttr = pICatItemAttr;
            pMemCtx->pICatItemAttr->AddRef();
        }
    }
    //
    // Only process the members when this search is done
    //
    if(fFinalCompletion) {

        ICategorizerItemAttributes *pICatItemAttr;

        pICatItemAttr = pMemCtx->pICatItemAttr;

        if(pICatItemAttr == NULL) {

            hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
            ERROR_LOG_ADDR_STATIC(
                pMemCtx->pCCatAddr, 
                "--no result--",
                pMemCtx,
                pMemCtx->pStore->GetISMTPServerEx());
            goto CLEANUP;
        }
        //
        // Null the ICatItemAttr pointer in the context and reset
        // hrResolutionStatus before starting a new search
        //
        pMemCtx->pICatItemAttr = NULL;
        pMemCtx->hrResolveStatus = S_OK;

        //
        // Process the new members
        //
        hr = pMemCtx->pStore->HrExpandDlPage(
            pMemCtx,
            pICatItemAttr);
        //
        // If this returns MAILTRANSPORT_S_PENDING, then this
        // completion routine will be called to free pMemCtx.
        // Therefore, we can NOT use pMemCtx below here when hr ==
        // MAILTRANSPORT_S_PENDING!
        //

        //
        // Release the Attributes interface (it was Addref'd when put into pMemCtx)
        //
        pICatItemAttr->Release();

        ERROR_CLEANUP_LOG_ADDR_STATIC(
            pMemCtx->pCCatAddr, 
            "pMemCtx->pStore->HrExpandDlPage",
            pMemCtx,
            pMemCtx->pStore->GetISMTPServerEx());
    }

 CLEANUP:
    //
    // Decrement the pending lookup added from
    // CMembershipPageInsertionRequest::HrInsertSearches
    //
    if(fFinalCompletion)
        pConn->DecrementPendingSearches(1);

    if(FAILED(hr)) {
        //
        // Save the error
        //
        pMemCtx->hrResolveStatus = hr;

    }
    if((fFinalCompletion) && (hr != MAILTRANSPORT_S_PENDING)) {
        //
        // THe final completion routine of the final search, so clean up
        //

        //
        // Call the sink completion routine
        //
        pMemCtx->pfnCompletion(
            pMemCtx->hrResolveStatus,
            pMemCtx->pCompletionContext);

        //
        // Get/Release insertion context so that inserted queries will be batched
        //
        pMemCtx->pConn->ReleaseInsertionContext();

        //
        // First release the connection
        //
        pMemCtx->pConn->Release();

        //
        // Release the CCatAddr object
        //
        pMemCtx->pCCatAddr->Release();

        //
        // Free the context that has served us thus far
        //
        if(pMemCtx->pICatItemAttr)
            pMemCtx->pICatItemAttr->Release();
        delete pMemCtx;

    } else {
        //
        // Get/Release insertion context so that inserted queries will be batched
        //
        pConn->ReleaseInsertionContext();
    }
    if(pConn)
        pConn->Release();
}

//+------------------------------------------------------------
//
// Function: CEmailIDLdapStore::HrExpandDynamicDlMembers
//
// Synopsis: Handle the expansion of a dynamicDL
//
// Arguments:
//  pCCatAddr: The item to expand
//  pListContext: List context initialized in
//                InitializeResolveListContext
//  pfnCompletion: A function to call upon async completion
//  pContext: Context to pass to the completion function
//
// Returns:
//  S_OK: Success, completed synchronously
//  MAILTRANSPORT_S_PENDING: Will complete async calling pfnCompletion
//  E_OUTOFMEMORY
//  CAT_E_DBCONNECTION: palc->GetConnection returned NULL (meaning it
//                      is having problems obtaining/maintaing a connection)
//
// History:
// jstamerj 1998/09/24 14:19:43: Created.
//
//-------------------------------------------------------------
template <class T> HRESULT CEmailIDLdapStore<T>::HrExpandDynamicDlMembers(
    CCatAddr *pCCatAddr,
    LPRESOLVE_LIST_CONTEXT pListContext,
    PFN_DLEXPANSIONCOMPLETION pfnCompletion,
    PVOID pContext)
{
    HRESULT hr;
    LPSTR pszFilterAttribute;
    CStoreListResolveContext *pCStoreContext;
    CDynamicDLSearchInsertionRequest *pCInsertionRequest = NULL;

    CatFunctEnterEx((LPARAM)this,
                      "CEmailIDLdapStore::HrExpandDynamicDlMembers");

    pCStoreContext = (CStoreListResolveContext *) pListContext->pStoreContext;

    hr = m_pICatParams->GetDSParameterA(
        DSPARAMETER_ATTRIBUTE_DL_DYNAMICFILTER,
        &pszFilterAttribute);

    if(FAILED(hr)) {
        //
        // Dynamic DLs simply aren't supported in this case
        //
        hr = S_OK;
        goto CLEANUP;
    }

    //
    // Allocate an insertion request
    //
    pCInsertionRequest = new CDynamicDLSearchInsertionRequest(
        AsyncDynamicDlCompletion,
        pCCatAddr,
        pCStoreContext,
        m_pICatParams,
        pfnCompletion,
        pContext);
    
    if(pCInsertionRequest == NULL) {
    
        hr = E_OUTOFMEMORY;
        ERROR_CLEANUP_LOG_ADDR(pCCatAddr, "new CDynamicDLSearchInsertionRequest");
        goto CLEANUP;
    }

    //
    // Now issue the search for the dynamic DL members
    //
    hr = pCStoreContext->HrInsertInsertionRequest(
        pCInsertionRequest);
        
    if(SUCCEEDED(hr)) {
        //
        // The search is async, so return pending
        //
        hr = MAILTRANSPORT_S_PENDING;
        //
        // We don't release pCInsertionRequest here because it
        // will be released on a future call to AsyncDynamicDlCompletion.
        //
    } else {
        //
        // We failed to insert the insertion request, so the completion will never
        // be called, so we need to call Release here.
        //
        ERROR_LOG_ADDR(pCCatAddr, "pCStoreContext->HrInsertInsertionRequest");
        pCInsertionRequest->Release();
    }
    
 CLEANUP:
    
    DebugTrace((LPARAM)this, "HrExpandDynamicDl returning hr %08lx", hr);
    CatFunctLeaveEx((LPARAM)this);

    return hr;
}


//+------------------------------------------------------------
//
// Function: CEmailIDLdapStore::AsyncDynamicDlCompletion
//
// Synopsis: Handle completion of an async lookup for DL members
//
// Arguments:
//  ctx: pMemCtx passed to AsyncSearch
//  dwNumResults: Number of objects matching search filter
//  rgpICatItemAttrs: Array of ICatItemAttributes
//  hr: Resolution status
//  fFinalCompletion: Indicates wether this is a partial result or not
//
// Returns: NOTHING
//
// History:
// jstamerj 1998/09/24 09:28:18: Created.
//
//-------------------------------------------------------------
template <class T> VOID CEmailIDLdapStore<T>::AsyncDynamicDlCompletion(
    LPVOID ctx,
    DWORD  dwNumResults,
    ICategorizerItemAttributes **rgpICatItemAttrs,
    HRESULT hrResolveStatus,
    BOOL fFinalCompletion)
{
    HRESULT hr = S_OK;
    DWORD dwCount;
    CDynamicDLSearchInsertionRequest *pCtx = NULL;
    BOOL fReissued = FALSE;

    CatFunctEnter("CEmailIDLdapStore::AsyncDynamicDlCompleton");

    pCtx = (CDynamicDLSearchInsertionRequest *) ctx;
    _ASSERT(pCtx);

    pCtx->m_ResolveCtx.pSLRC->GetInsertionContext();
    
    if( ((hrResolveStatus == CAT_E_DBCONNECTION) ||
        (hrResolveStatus == HRESULT_FROM_WIN32(ERROR_CANCELLED))) &&
        pCtx->m_ResolveCtx.fFirstPage &&
        !(pCtx->m_ResolveCtx.pSLRC->Canceled()) ) {
        //
        // fFinalCompletion should always be true if there was
        // an error
        //
        _ASSERT(fFinalCompletion);
        
        CDynamicDLSearchInsertionRequest *pCInsertionRequest;
        
        if(pCtx->m_ResolveCtx.pConn) {
            hr = pCtx->m_ResolveCtx.pSLRC->HrInvalidateConnectionAndRetrieveNewConnection(
                pCtx->m_ResolveCtx.pConn);
                
            ERROR_CLEANUP_LOG_ADDR_STATIC(
                pCtx->m_ResolveCtx.pCCatAddr, 
                "pCtx->m_ResolveCtx.pSLRC->HrInvalidateConnectionAndRetrieveNewConnection",
                pCtx,
                pCtx->GetISMTPServerEx());
        }

        pCInsertionRequest = new CDynamicDLSearchInsertionRequest(
            AsyncDynamicDlCompletion,
            pCtx->m_ResolveCtx.pCCatAddr,
            pCtx->m_ResolveCtx.pSLRC,
            pCtx->m_ResolveCtx.pICatParams,
            pCtx->m_ResolveCtx.pfnCompletion,
            pCtx->m_ResolveCtx.pCompletionContext);
            
        if(!pCInsertionRequest) {
            hr = E_OUTOFMEMORY;
            ERROR_LOG_ADDR_STATIC(
                pCtx->m_ResolveCtx.pCCatAddr, 
                "new CDynamicDLSearchInsertionRequest",
                pCtx,
                pCtx->GetISMTPServerEx());
            goto CLEANUP;
        }

        //
        // clean up pConn in the resolve context before reinsertion
        //
        hr = pCtx->m_ResolveCtx.pSLRC->HrInsertInsertionRequest(
            pCInsertionRequest);
        
        if (SUCCEEDED(hr)) {
            fReissued = TRUE;
            //
            // We don't release pCInsertionRequest here because it
            // will be released on a subsequent call to
            // AsyncDynamicDlCompletion.
            //
        } else {
            ERROR_LOG_ADDR_STATIC(
                pCtx->m_ResolveCtx.pCCatAddr, 
                "pCtx->m_ResolveCtx.pSLRC->HrInsertInsertionRequest",
                pCtx,
                pCtx->GetISMTPServerEx());
            pCInsertionRequest->Release();
        }
        
    } else {
    
        pCtx->m_ResolveCtx.fFirstPage = FALSE;
    
        if(FAILED(hrResolveStatus)) {
            //
            // Handle failures in the cleanup code
            //
            hr = hrResolveStatus;
            ERROR_LOG_ADDR_STATIC(
                pCtx->m_ResolveCtx.pCCatAddr, 
                "--async--",
                pCtx,
                pCtx->GetISMTPServerEx());

            goto CLEANUP;
        }

        for(dwCount = 0; dwCount < dwNumResults; dwCount++) {
            //
            // Loop through each ICatItemAttr; each one is a DL member.
            // Add it as a dynamic DL member
            //
            hr = pCtx->m_ResolveCtx.pCCatAddr->AddDynamicDLMember(
                rgpICatItemAttrs[dwCount]);

            _ASSERT(hr != MAILTRANSPORT_S_PENDING);

            ERROR_CLEANUP_LOG_ADDR_STATIC(
                pCtx->m_ResolveCtx.pCCatAddr,
                "pCtx->m_ResolveCtx.pCCatAddr->AddDynamicDLMember",
                pCtx,
                pCtx->GetISMTPServerEx());
        }
        
    }

 CLEANUP:
 
    if(FAILED(hr))
        pCtx->m_ResolveCtx.hrResolveStatus = hr;

    if(fFinalCompletion) {
    
        if (!fReissued) {
            //
            // Call the sink completion routine
            //
            pCtx->m_ResolveCtx.pfnCompletion(
                pCtx->m_ResolveCtx.hrResolveStatus,
                pCtx->m_ResolveCtx.pCompletionContext);
        }

        //
        // Get/Release insertion context so that inserted queries will be batched
        //
        pCtx->m_ResolveCtx.pSLRC->ReleaseInsertionContext();
        
        pCtx->Release();

    } else {

        //
        // There are no more async completions pending, BUT the
        // emailidldapstore has more members to tell us about
        //

        //
        // Get/Release insertion context so that inserted queries will be batched
        //
        pCtx->m_ResolveCtx.pSLRC->ReleaseInsertionContext();
    }
}


//+----------------------------------------------------------------------------
//
//  Function:   CEmailIDLdapStore::RetrieveICatParamsInfo
//
//  Synopsis:   Helper routine to retrieve the info we need from
//              ICategorizerParams.  Pointers to strings in ICatParams
//              are retrieved; the strings themselves are not copied.
//              Since ICatParams is read only at this point, the
//              strings will be good as long as we have a reference to
//              ICatParams.
//
//  Arguments:  [pszHost] -- The Host parameter is returned here
//              [pdwPort] -- The remote tcp Port# is returned here
//                        (*pdwPort is set to zero if the DSPARAMTER wasn't set)
//
//              [pszNamingContext] -- The NamingContext parameter is returned
//                  here.
//              [pszAccount] -- The LDAP account parameter is returned here.
//              [pszPassword] -- The LDAP password parameter is returned here.
//              [pbt] -- The bind type to use to connect to ldap hosts.
//
//  Returns:    S_OK always -- parameters that couldn't be retrieved
//              will be set to NULL (or simple bind for bind type)
//
//-----------------------------------------------------------------------------
template <class T> HRESULT CEmailIDLdapStore<T>::RetrieveICatParamsInfo(
    LPSTR *ppszHost,
    DWORD *pdwPort,
    LPSTR *ppszNamingContext,
    LPSTR *ppszAccount,
    LPSTR *ppszDomain,
    LPSTR *ppszPassword,
    LDAP_BIND_TYPE *pbt)
{
    CatFunctEnter("CEmailIDLdapStore::RetrieveICatParamsInfo");

    LPSTR pszBindType = NULL;
    LPSTR pszPort = NULL;

    *ppszHost = NULL;
    *pdwPort = 0;
    *ppszNamingContext = NULL;
    *ppszAccount = NULL;
    *ppszDomain = NULL;
    *ppszPassword = NULL;
    *pbt = BIND_TYPE_SIMPLE;

    m_pICatParams->GetDSParameterA(
        DSPARAMETER_LDAPHOST,
        ppszHost);
    m_pICatParams->GetDSParameterA(
        DSPARAMETER_LDAPNAMINGCONTEXT,
        ppszNamingContext);
    m_pICatParams->GetDSParameterA(
        DSPARAMETER_LDAPACCOUNT,
        ppszAccount);
    m_pICatParams->GetDSParameterA(
        DSPARAMETER_LDAPDOMAIN,
        ppszDomain);
    m_pICatParams->GetDSParameterA(
        DSPARAMETER_LDAPPASSWORD,
        ppszPassword);
    m_pICatParams->GetDSParameterA(
        DSPARAMETER_LDAPPORT,
        &pszPort);

    if(pszPort) {
        //
        // Convert from a string to a dword
        //
        *pdwPort = atol(pszPort);
    }


    m_pICatParams->GetDSParameterA(
        DSPARAMETER_LDAPBINDTYPE,
        &pszBindType);

    if(pszBindType) {
        if (lstrcmpi(pszBindType, "None") == 0) {
            *pbt = BIND_TYPE_NONE;
        }
        else if(lstrcmpi(pszBindType, "CurrentUser") == 0) {
            *pbt = BIND_TYPE_CURRENTUSER;
        }
        else if (lstrcmpi(pszBindType, "Simple") == 0) {
            *pbt = BIND_TYPE_SIMPLE;
        }
        else if (lstrcmpi(pszBindType, "Generic") == 0) {
            *pbt = BIND_TYPE_GENERIC;
        }
    }

    return S_OK;
}


//+------------------------------------------------------------
//
// Function: AccountFromUserDomain
//
// Synopsis: Helper function.  Given a username and netbios domain
//           name, form the account name to use.
//
// Arguments:
//  pszAccount: Buffer to fill in
//  dwccAccount: Size of that buffer
//  pszUser: Username.  If NULL, pszAccount will be set to ""
//  pszDomain: Domainname.  If NULL, pszUser will be copied to pszAccount
//
// Returns:
//  S_OK: Success
//  HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER): dwccAccount is not
//  sufficiently large
//
// History:
// jstamerj 1998/06/25 12:06:02: Created.
//
//-------------------------------------------------------------
template <class T> HRESULT CEmailIDLdapStore<T>::AccountFromUserDomain(
    LPTSTR pszAccount,
    DWORD  dwccAccount,
    LPTSTR pszUser,
    LPTSTR pszDomain)
{
    CatFunctEnterEx((LPARAM)this,"CEmailIDLdapStore::AccountFromUserDomainSchema");
    _ASSERT(pszAccount != NULL);
    _ASSERT(dwccAccount >= 1);

    pszAccount[0] = '\0';

    if(pszUser) {
        if((pszDomain == NULL) || (pszDomain[0] == '\0')) {
            //
            // If Domain is NULL, just copy user to account
            //
            if((DWORD)lstrlen(pszUser) >= dwccAccount) {
                return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            }
            lstrcpy(pszAccount, pszUser);

        } else {
            if((DWORD)lstrlen(pszUser) + (DWORD)lstrlen(pszAccount) + 1 >=
               dwccAccount) {
                return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            }
            lstrcpy(pszAccount, pszDomain);
            lstrcat(pszAccount, "\\");
            lstrcat(pszAccount, pszUser);
        }
    }
    DebugTrace(NULL, "Returning pszAccount = \"%s\"", pszAccount);
    CatFunctLeaveEx((LPARAM)this);
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CMembershipPageInsertionRequest::HrInsertSearches
//
// Synopsis: Insert the search for the next page of members
//
// Arguments:
//  dwcSearches: Number of searches we may insert
//
// Returns:
//  S_OK: Success
//  HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS)
//
// History:
// jstamerj 1999/03/26 11:58:26: Created.
//
//-------------------------------------------------------------
HRESULT CMembershipPageInsertionRequest::HrInsertSearches(
    DWORD dwcSearches)
{
    HRESULT hr = S_OK;
    LPSTR pszSearchFilter;
    LPWSTR pszMemberAttr;
    LPSTR pszDistinguishingAttribute;
    LPWSTR pwszDistinguishingAttribute;
    LPCWSTR rgpszAttributes[3];
    int i;

    WCHAR szMemberAttribute[MAX_MEMBER_ATTRIBUTE_SIZE +
                            (sizeof(WSZ_PAGEDMEMBERS_INDICATOR)/sizeof(WCHAR)) +
                            MAX_PAGEDMEMBERS_DIGITS +
                            sizeof("-*")];

    CatFunctEnterEx((LPARAM)this, "CMembershipPageInsertionRequest::HrInsertSearches");

    _ASSERT(m_pMemCtx);
    _ASSERT(m_pMemCtx->pCCatAddr);

    if((dwcSearches == 0) ||
       (m_fInsertedRequest == TRUE))
        goto CLEANUP;

    //
    // Now we will either insert the request or call our completion
    // with a failure
    //
    m_fInsertedRequest = TRUE;
    //
    // Use the original search filter that found this object in the
    // first place
    //
    hr = m_pMemCtx->pCCatAddr->GetStringAPtr(
        ICATEGORIZERITEM_LDAPQUERYSTRING,
        &pszSearchFilter);

    if(FAILED(hr)) {
        //
        // It is possible that we have an item where BuildQuery was
        // never triggered (in the case of a 1000+ member DL that is a
        // member of a dynamic DL).  For this case, TriggerBuildQuery
        // and try to retrieve a query string again
        //
        DebugTrace((LPARAM)this, "No query string found on a paged DL; triggering buildquery");
        hr = m_pMemCtx->pCCatAddr->HrTriggerBuildQuery();
        ERROR_CLEANUP_LOG_ADDR(m_pMemCtx->pCCatAddr, "m_pMemCtx->pCCatAddr->HrTriggerBuildQuery");

        //
        // Try to get the query string again
        //
        hr = m_pMemCtx->pCCatAddr->GetStringAPtr(
            ICATEGORIZERITEM_LDAPQUERYSTRING,
            &pszSearchFilter);
        ERROR_CLEANUP_LOG_ADDR(m_pMemCtx->pCCatAddr, "m_pMemCtx->pCCatAddr->GetSTringAPtr(ldapquerystring)");
    }

    hr = m_pMemCtx->pCCatAddr->GetStringAPtr(
        ICATEGORIZERITEM_DISTINGUISHINGATTRIBUTE,
        &pszDistinguishingAttribute);
    ERROR_CLEANUP_LOG_ADDR(m_pMemCtx->pCCatAddr, "m_pMemCtx->pCCatAddr->GetStringAPtr");
    //
    // Convert distinguishing attribute to unicode
    //
    i = MultiByteToWideChar(
        CP_UTF8,
        0,
        pszDistinguishingAttribute,
        -1,
        NULL,
        0);
    if(i == 0) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        ERROR_LOG_ADDR(m_pMemCtx->pCCatAddr, "MultiByteToWideChar - 0");
        goto CLEANUP;
    }
    pwszDistinguishingAttribute = (LPWSTR) alloca(i * sizeof(WCHAR));
    i = MultiByteToWideChar(
        CP_UTF8,
        0,
        pszDistinguishingAttribute,
        -1,
        pwszDistinguishingAttribute,
        i);
    if(i == 0) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        ERROR_LOG_ADDR(m_pMemCtx->pCCatAddr, "MultiByteToWideChar - 1");
        goto CLEANUP;
    }

    hr = m_pMemCtx->pStore->m_pICatParams->GetDSParameterW(
        DSPARAMETER_ATTRIBUTE_DL_MEMBERS,
        &pszMemberAttr);
    ERROR_CLEANUP_LOG_ADDR(m_pMemCtx->pCCatAddr, "m_pMemCtx->m_pStore->m_pICatParams->GetDSParamterW(dlmembers)");

    //
    // Form the member attribute name we want
    //
    if( _snwprintf(szMemberAttribute,
                   sizeof(szMemberAttribute)/sizeof(WCHAR),
                   L"%s" WSZ_PAGEDMEMBERS_INDICATOR L"%d-*",
                   pszMemberAttr,
                   m_pMemCtx->dwNextBlockIndex) < 0) {
        //
        // There was insufficient space in the buffer
        //
        ErrorTrace((LPARAM)this, "Insufficient space to form paged member attribute name");
        hr = E_INVALIDARG;
        ERROR_LOG_ADDR(m_pMemCtx->pCCatAddr, "_snwprintf -- insufficient buffer");
        goto CLEANUP;
    }

    //
    // Form the attribute array
    //
    rgpszAttributes[0] = szMemberAttribute;
    rgpszAttributes[1] = pwszDistinguishingAttribute;
    rgpszAttributes[2] = NULL;

    //
    // Increment here, decrement in AsyncExpandDlCompletion
    //
    m_pMemCtx->pConn->IncrementPendingSearches();

    hr = m_pMemCtx->pConn->AsyncSearch(
        m_pMemCtx->pConn->GetNamingContextW(),
        LDAP_SCOPE_SUBTREE,
        pszSearchFilter,
        rgpszAttributes,
        0, // Not a paged search (as in dynamic DLs)
        CEmailIDLdapStore<CCatAddr>::AsyncExpandDlCompletion,
        m_pMemCtx);

    if(FAILED(hr)) {
        m_pMemCtx->pConn->DecrementPendingSearches(1);
        ERROR_LOG_ADDR(m_pMemCtx->pCCatAddr, "m_pMemCtx->pConn->AsyncSearch");
        goto CLEANUP;
    }

 CLEANUP:
    if(FAILED(hr)) {
        //
        // Call completion now instead of later when we are dequeued
        //
        _ASSERT(m_fInsertedRequest);
        //
        // AsyncExpandDlCompletion will always decrement the pending searches
        //
        m_pMemCtx->pConn->IncrementPendingSearches();

        CEmailIDLdapStore<CCatAddr>::AsyncExpandDlCompletion(
            m_pMemCtx,      // ctx
            0,              // dwNumResults
            NULL,           // rgpICatItemAttrs
            hr,             // hrResolveStatus
            TRUE);          // fFinalCompletion
    }
    
    if(SUCCEEDED(hr))
        hr = (m_fInsertedRequest ? HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) : S_OK);

    DebugTrace((LPARAM)this, "returning %08lx", hr);
    CatFunctLeaveEx((LPARAM)this);
        
    return hr;
} // CMembershipPageInsertionRequest::HrInsertSearches


//+------------------------------------------------------------
//
// Function: CMembershipPageInsertionRequest::NotifyDeQueue
//
// Synopsis: Notification that this insertion request is being dequeued
//
// Arguments:
//  hr: Reason why we are dequeueing
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/04/03 17:15:17: Created.
//
//-------------------------------------------------------------
VOID CMembershipPageInsertionRequest::NotifyDeQueue(
    HRESULT hr)
{
    CatFunctEnterEx((LPARAM)this, "CMembershipPageInsertionRequest::NotifyDeQueue");
    //
    // If our request is being dequeue'd and we have not inserted our
    // request to ldapconn, then we are being cancelled
    // Notify our master of this
    //
    if(!m_fInsertedRequest) {
        //
        // AsyncExpandDlCompletion will always decrement the pending searches
        //
        m_pMemCtx->pConn->IncrementPendingSearches();

        CEmailIDLdapStore<CCatAddr>::AsyncExpandDlCompletion(
            m_pMemCtx,      // ctx
            0,              // dwNumResults
            NULL,           // rgpICatItemAttrs
            (hr == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) ? S_OK : hr),
            TRUE);          // fFinalCompletion
    }

    CatFunctLeaveEx((LPARAM)this);
} // CMembershipPageInsertionRequest::NotifyDeQueue

//+------------------------------------------------------------
//
// Function: CDynamicDLSearchInsertionRequest::CDynamicDLSearchInsertionRequest
//
// Synopsis: Constructor
//
// Arguments:
//
// Returns: NOTHING
//
// History:
// dlongley 2001/12/07: Created.
//
//-------------------------------------------------------------
CDynamicDLSearchInsertionRequest::CDynamicDLSearchInsertionRequest(
    LPLDAPCOMPLETION pfnLdapCompletion,
    CCatAddr *pCCatAddr,
    CStoreListResolveContext *pSLRC,
    ICategorizerParametersEx *pICatParams,
    PFN_DLEXPANSIONCOMPLETION pfnExpansionCompletion,
    PVOID pCompletionContext)
{
    _ASSERT(pfnLdapCompletion);
    
    m_pfnCompletion = pfnLdapCompletion;
    
    pCCatAddr->AddRef();
    m_ResolveCtx.pCCatAddr = pCCatAddr;
    
    pSLRC->AddRef();
    m_ResolveCtx.pSLRC = pSLRC;
    
    pICatParams->AddRef();
    m_ResolveCtx.pICatParams = pICatParams;
    
    m_ResolveCtx.pfnCompletion = pfnExpansionCompletion;
    m_ResolveCtx.pCompletionContext = pCompletionContext;
    m_ResolveCtx.fFirstPage = TRUE;
    m_ResolveCtx.pConn = NULL;
    m_ResolveCtx.hrResolveStatus = S_OK;
    
    m_fInsertedRequest = FALSE;
    
    m_dwPageSize = CEmailIDLdapStore<CCatAddr>::m_dwDynamicDlPageSize;
    
    m_dwSignature = SIGNATURE_CDynamicDLSearchInsertionRequest;
} // CDynamicDLSearchInsertionRequest::CDynamicDLSearchInsertionRequest

//+------------------------------------------------------------
//
// Function: CDynamicDLSearchInsertionRequest::~CDynamicDLSearchInsertionRequest
//
// Synopsis: Destructor
//
// Arguments: None
//
// Returns: NOTHING
//
// History:
// dlongley 2001/12/07: Created.
//
//-------------------------------------------------------------
CDynamicDLSearchInsertionRequest::~CDynamicDLSearchInsertionRequest()
{
    _ASSERT(m_dwSignature == SIGNATURE_CDynamicDLSearchInsertionRequest);
    
    m_ResolveCtx.pCCatAddr->Release();
    m_ResolveCtx.pSLRC->Release();
    m_ResolveCtx.pICatParams->Release();
    
    if(m_ResolveCtx.pConn) {
        m_ResolveCtx.pConn->DecrementPendingSearches(m_dwPageSize);
        m_ResolveCtx.pConn->Release();
    }
    
    m_dwSignature = SIGNATURE_CDynamicDLSearchInsertionRequest_INVALID;
} // CDynamicDLSearchInsertionRequest::~CDynamicDLSearchInsertionRequest

//+------------------------------------------------------------
//
// Function: CDynamicDLSearchInsertionRequest::HrInsertSearches
//
// Synopsis: Insert the search for the dynamic DL's members
//
// Arguments:
//  dwcSearches: Number of searches we may insert
//
// Returns:
//  S_OK: Success
//  HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS)
//
// History:
//  dlongley 2001/12/07: Created.
//
//-------------------------------------------------------------
HRESULT CDynamicDLSearchInsertionRequest::HrInsertSearches(
    DWORD dwcSearches)
{
    HRESULT hr = S_OK;
    LPSTR pszFilterAttribute, pszFilter;
    LPSTR pszBaseDNAttribute, pszBaseDN;
    BOOL fEnumeratingFilter = FALSE;
    BOOL fEnumeratingBaseDN = FALSE;
    ATTRIBUTE_ENUMERATOR enumerator_filter;
    ATTRIBUTE_ENUMERATOR enumerator_basedn;
    LPWSTR *rgpszAllAttributes;
    ICategorizerItemAttributes *pICatItemAttr = NULL;
    ICategorizerUTF8Attributes *pICatItemUTF8 = NULL;
    ICategorizerRequestedAttributes *pIRequestedAttributes = NULL;
    CBatchLdapConnection *pConn = NULL;

    CatFunctEnterEx((LPARAM)this,
                      "CDynamicDLSearchInsertionRequest::HrInsertSearches");

    if ( (dwcSearches == 0) || m_fInsertedRequest )
        goto CLEANUP;

    //
    // Get the attributes interface
    //
    hr = m_ResolveCtx.pCCatAddr->GetICategorizerItemAttributes(
        ICATEGORIZERITEM_ICATEGORIZERITEMATTRIBUTES,
        &pICatItemAttr);

    if(FAILED(hr)) {
        pICatItemAttr = NULL;
        ErrorTrace((LPARAM)this, "failed to get ICatItemAttr");
        ERROR_LOG_ADDR(m_ResolveCtx.pCCatAddr, "pCCatAddr->GetICategorizerItemAttributes");
        goto CLEANUP;
    }

    //
    // Get the UTF8 version of the attributes interface
    //
    hr = pICatItemAttr->QueryInterface(
        IID_ICategorizerUTF8Attributes,
        (LPVOID *)&pICatItemUTF8);

    if(FAILED(hr)) {
        pICatItemUTF8 = NULL;
        ErrorTrace((LPARAM)this, "Failed to get UTF8 attribute enumeration interface");
        ERROR_LOG_ADDR(m_ResolveCtx.pCCatAddr, "pICatItemAttr->QueryInterface(utf8attributes)");
        goto CLEANUP;
    }
    
    hr = m_ResolveCtx.pICatParams->GetDSParameterA(
        DSPARAMETER_ATTRIBUTE_DL_DYNAMICFILTER,
        &pszFilterAttribute);

    //
    // We checked this case in CEmailIDLdapStore<T>::HrExpandDynamicDlMembers
    // and it must have succeeded there in order for this insertion request
    // ever to have been queued.
    //
    _ASSERT( SUCCEEDED(hr) );

    hr = m_ResolveCtx.pICatParams->GetDSParameterA(
        DSPARAMETER_ATTRIBUTE_DL_DYNAMICBASEDN,
        &pszBaseDNAttribute);

    if(FAILED(hr)) {
        //
        // Use the default baseDN
        //
        ERROR_LOG_ADDR(m_ResolveCtx.pCCatAddr, "m_pICatParams->GetDSParameterA");
        pszBaseDNAttribute = NULL;
        pszBaseDN = NULL;
    }

    //
    // Find the query filter string
    //
    hr = pICatItemUTF8->BeginUTF8AttributeEnumeration(
        pszFilterAttribute,
        &enumerator_filter);

    if(SUCCEEDED(hr)) {

        fEnumeratingFilter = TRUE;

        hr = pICatItemUTF8->GetNextUTF8AttributeValue(
            &enumerator_filter,
            &pszFilter);
    }

    if(FAILED(hr)) {
        //
        // No such attribute?  No members.
        //
        ERROR_LOG_ADDR(
            m_ResolveCtx.pCCatAddr,
            "pICatItemUTF8->GetNextUTF8AttributeValue(filter) or "
            "pICatItemUTF8->BeginUTF8AttributeEnumeration(pszFilterAttribute)");
        hr = S_OK;
        
        m_fInsertedRequest = TRUE;
        
        m_pfnCompletion(
            this,
            0,
            NULL,
            hr,
            TRUE);
        
        goto CLEANUP;
    }
    //
    // Find the base DN
    //
    if(pszBaseDNAttribute) {

        hr = pICatItemUTF8->BeginUTF8AttributeEnumeration(
            pszBaseDNAttribute,
            &enumerator_basedn);

        if(SUCCEEDED(hr)) {

            fEnumeratingBaseDN = TRUE;

            hr = pICatItemUTF8->GetNextUTF8AttributeValue(
                &enumerator_basedn,
                &pszBaseDN);

        }
        if(FAILED(hr)) {
            //
            // Use the default base DN
            //
            pszBaseDN = NULL;
        }
    }

    //
    // Fetch all the requested attributes
    //
    hr = m_ResolveCtx.pICatParams->GetRequestedAttributes(
        &pIRequestedAttributes);
    ERROR_CLEANUP_LOG_ADDR(m_ResolveCtx.pCCatAddr, "pICatParams->GetRequestedAttributes");

    hr = pIRequestedAttributes->GetAllAttributesW(
        &rgpszAllAttributes);
    ERROR_CLEANUP_LOG_ADDR(m_ResolveCtx.pCCatAddr, "pIRequestedAttributes->GetAllAttributesW");
    
    //
    // Get the LDAP connection from the CStoreListResolveContext
    //
    pConn = m_ResolveCtx.pSLRC->GetConnection();
    
    DebugTrace((LPARAM)this, "GetConnection returned %08lx", hr);
    
    if(pConn == NULL) {
    
        hr = CAT_E_DBCONNECTION;
        ERROR_LOG_ADDR(m_ResolveCtx.pCCatAddr,"m_ResolveCtx.pSLRC->GetConnection");
        
    } else {
        //
        // Now issue the search for the dynamic DL members
        //
        _ASSERT(m_ResolveCtx.pConn == NULL);
        
        m_ResolveCtx.pConn = pConn;
        pConn->AddRef();
        
        pConn->IncrementPendingSearches(m_dwPageSize);
        
        hr = pConn->AsyncSearch(
            (LPCSTR) (pszBaseDN ? pszBaseDN : pConn->GetNamingContext()),
            LDAP_SCOPE_SUBTREE,
            pszFilter,
            (LPCWSTR *)rgpszAllAttributes,
            m_dwPageSize,
            m_pfnCompletion,
            this);
            
        DebugTrace((LPARAM)this, "AsyncSearch returned %08lx", hr);
            
        if(SUCCEEDED(hr)) {
            //
            // It is safe to set a member after calling AsyncSearch
            // SUCCEEDED because:
            // 1) Our caller
            // (CBatchLdapConnection::DecrementPendingSearches) has a
            // reference to us, so we will not be deleted.
            // 2) Out caller ensures that NotifyDequeue will not be
            // called while we are inside HrInsertSearches
            //
            m_fInsertedRequest = TRUE;
        } else {
            ERROR_LOG_ADDR(m_ResolveCtx.pCCatAddr,"pConn->AsyncSearch");
        }
    }
    
    if(pConn)
        pConn->Release();
  
 CLEANUP:
 
    if(fEnumeratingFilter) {

        pICatItemUTF8->EndUTF8AttributeEnumeration(
            &enumerator_filter);
    }
    if(fEnumeratingBaseDN) {

        pICatItemUTF8->EndUTF8AttributeEnumeration(
            &enumerator_basedn);
    }
    
    if(pICatItemAttr)
        pICatItemAttr->Release();

    if(pICatItemUTF8)
        pICatItemUTF8->Release();

    if(pIRequestedAttributes)
        pIRequestedAttributes->Release();
        
    if(SUCCEEDED(hr))
        hr = (m_fInsertedRequest ? HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) : S_OK);
    
    DebugTrace((LPARAM)this, "HrInsertSearches returning hr %08lx", hr);
    CatFunctLeaveEx((LPARAM)this);
    
    return hr;
} // CDynamicDLSearchInsertionRequest::HrInsertSearches

//+------------------------------------------------------------
//
// Function: CDynamicDLSearchInsertionRequest::NotifyDeQueue
//
// Synopsis: Notification that this insertion request is being dequeued
//
// Arguments:
//  hr: Reason why we are dequeueing
//
// Returns: NOTHING
//
// History:
//  dlongley 2001/12/07: Created.
//
//-------------------------------------------------------------
VOID CDynamicDLSearchInsertionRequest::NotifyDeQueue(
    HRESULT hrReason)
{
    TraceFunctEnterEx((LPARAM)this, "CDynamicDLSearchInsertionRequest::NotifyDeQueue");
    
    if(!m_fInsertedRequest) {
        //
        // AsyncSearch was not called so we need to call the completion directly
        //
        m_pfnCompletion(
            this,
            0,
            NULL,
            (hrReason == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) ? S_OK : hrReason),
            TRUE);
    }

    TraceFunctLeaveEx((LPARAM)this);
} // CDynamicDLSearchInsertionRequest::NotifyDeQueue

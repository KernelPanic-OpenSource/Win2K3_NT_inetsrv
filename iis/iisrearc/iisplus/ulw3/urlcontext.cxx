/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module Name :
     urlinfo.cxx

   Abstract:
     Gets metadata for URL

   Author:
     Bilal Alam (balam)             8-Jan-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"
#include <stringau.hxx>

//
// Utility to guard against ~ inconsistency
//

DWORD
CheckIfShortFileName(
    IN WCHAR *          pszPath,
    IN HANDLE           hImpersonation,
    OUT BOOL *          pfShort
);

W3_STATE_URLINFO::W3_STATE_URLINFO()
{
    _hr = URL_CONTEXT::Initialize();
}

W3_STATE_URLINFO::~W3_STATE_URLINFO()
{
    URL_CONTEXT::Terminate();
}

CONTEXT_STATUS
W3_STATE_URLINFO::OnCompletion(
    W3_MAIN_CONTEXT *       pMainContext,
    DWORD                   cbCompletion,
    DWORD                   dwCompletionStatus
)
/*++

Routine Description:

    Handle URLINFO completions.  CheckAccess() is called in DoWork() and this
    call is asynchronous.

Arguments:

    pMainContext - W3_MAIN_CONTEXT representing execution of state machine
    cbCompletion - Number of bytes in an async completion
    dwCompletionStatus - Error status of a completion

Return Value:

    CONTEXT_STATUS_CONTINUE - if we should continue in state machine
    else stop executing the machine and free up the current thread

--*/
{
    CONTEXT_STATUS              contextStatus;
    BOOL                        fAccessAllowed;

    contextStatus = pMainContext->CheckAccess( TRUE,   // this is a completion
                                               cbCompletion,
                                               dwCompletionStatus,
                                               &fAccessAllowed );

    if ( contextStatus == CONTEXT_STATUS_PENDING )
    {
        return CONTEXT_STATUS_PENDING;
    }

    //
    // If access is not allowed, then just finish state machine (
    // response has already been sent)
    //

    if ( !fAccessAllowed )
    {
        pMainContext->SetFinishedResponse();
    }

    return CONTEXT_STATUS_CONTINUE;
}

CONTEXT_STATUS
W3_STATE_URLINFO::DoWork(
    W3_MAIN_CONTEXT *       pMainContext,
    DWORD                   cbCompletion,
    DWORD                   dwCompletionStatus
)
/*++

Routine Description:

    Handle retrieving the metadata for this request

Arguments:

    pMainContext - W3_MAIN_CONTEXT representing execution of state machine
    cbCompletion - Number of bytes in an async completion
    dwCompletionStatus - Error status of a completion

Return Value:

    CONTEXT_STATUS_CONTINUE - if we should continue in state machine
    else stop executing the machine and free up the current thread

--*/
{
    URL_CONTEXT *           pUrlContext = NULL;
    BOOL                    fFinished = FALSE;
    HRESULT                 hr = NO_ERROR;
    W3_METADATA *           pMetaData = NULL;
    CONTEXT_STATUS          contextStatus = CONTEXT_STATUS_CONTINUE;
    W3_REQUEST *            pHttpRequest = pMainContext->QueryRequest();
    W3_RESPONSE *           pResponse = pMainContext->QueryResponse();
    BOOL                    fAccessAllowed = FALSE;

    DBG_ASSERT( pHttpRequest != NULL );
    DBG_ASSERT( pResponse != NULL );

    //
    // Set the context state.  Note that passing TRUE as the final
    // argument pushes the URL_CONTEXT into the main context.  As
    // a result, errors in this function should not attempt to
    // clean it up.
    //

    hr = URL_CONTEXT::RetrieveUrlContext( pMainContext,
                                          pMainContext->QueryRequest(),
                                          &pUrlContext,
                                          &fFinished,
                                          TRUE );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }

    DBG_ASSERT( fFinished || ( pUrlContext != NULL ) );

    //
    // From now on, errors in this function should not cleanup the URL
    // context since it is owned by the main context
    //

    pUrlContext = NULL;

    //
    // If filter wants out, leave
    //

    if ( fFinished )
    {
        pMainContext->SetDone();
        return CONTEXT_STATUS_CONTINUE;
    }

    //
    // Check access now.  That means checking for IP/SSL/Certs.  We will
    // avoid the authentication type check since the others (IP/SSL/Certs)
    // take priority.
    //

    contextStatus = pMainContext->CheckAccess( FALSE,     // not a completion
                                               0,         // cbCompletion,
                                               NO_ERROR,
                                               &fAccessAllowed );
    if ( contextStatus == CONTEXT_STATUS_PENDING )
    {
        return CONTEXT_STATUS_PENDING;
    }

    //
    // If we don't have access, then the appropriate error response was
    // already sent.  Just finish the state machine
    //

    if ( !fAccessAllowed )
    {
        pMainContext->SetFinishedResponse();
    }

    return CONTEXT_STATUS_CONTINUE;

Failure:

    if ( pUrlContext != NULL )
    {
        delete pUrlContext;
    }

    if ( !pMainContext->QueryResponseSent() )
    {
        if ( hr == HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) )
        {
            // For the non-8dot3 case
            pMainContext->QueryResponse()->SetStatus( HttpStatusNotFound );
        }
        else if ( hr == HRESULT_FROM_WIN32( ERROR_INVALID_DATA ) )
        {
            pMainContext->QueryResponse()->SetStatus( HttpStatusServerError,
                                                      Http500BadMetadata );
        }
        else if ( hr == HRESULT_FROM_WIN32( ERROR_LOGON_FAILURE ) )
        {
            pMainContext->QueryResponse()->SetStatus( HttpStatusServerError,
                                                      Http500UNCAccess );
        }
        else
        {
            pMainContext->QueryResponse()->SetStatus( HttpStatusServerError );
        }
    }

    pMainContext->SetFinishedResponse();
    pMainContext->SetErrorStatus( hr );

    return CONTEXT_STATUS_CONTINUE;
}

//static
HRESULT
URL_CONTEXT::RetrieveUrlContext(
    W3_CONTEXT *            pW3Context,
    W3_REQUEST *            pRequest,
    OUT URL_CONTEXT **      ppUrlContext,
    BOOL *                  pfFinished,
    BOOL                    fSetInW3Context
)
/*++

Routine Description:

    For a given request, get a URL_CONTEXT which represents the
    metadata and URI-specific info for that request

Arguments:

    pW3Context - W3_CONTEXT for the request
    pRequest - New request to lookup
    ppUrlContext - Set to point to new URL_CONTEXT
    pfFinished - Set to true if isapi filter said we're finished
    fSetInW3Context - OPTIONAL default FALSE

Return Value:

    HRESULT

--*/
{
    STACK_STRU(         strUrl, MAX_PATH );
    W3_URL_INFO *       pUrlInfo = NULL;
    W3_METADATA *       pMetaData = NULL;
    TOKEN_CACHE_ENTRY * pTokenEntry = NULL;
    URL_CONTEXT *       pUrlContext = NULL;
    HRESULT             hr = NO_ERROR;
    HANDLE              hToken = NULL;
    W3_TRACE_LOG *      pTraceLog;

    if ( pW3Context == NULL ||
         pRequest == NULL ||
         ppUrlContext == NULL ||
         pfFinished == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    *ppUrlContext = NULL;

    hr = pRequest->GetUrl( &strUrl );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }

    //
    // Lookup the URI info for this request
    //

    DBG_ASSERT( g_pW3Server->QueryUrlInfoCache() != NULL );

    hr = g_pW3Server->QueryUrlInfoCache()->GetUrlInfo(
                                        pW3Context,
                                        strUrl,
                                        &pUrlInfo );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }

    //
    // Now, create a URL_CONTEXT object which contains the W3_URL_INFO and
    // W3_METADATA pointers as well as state information for use on cleanup
    //

    DBG_ASSERT( pUrlInfo != NULL );

    pMetaData = (W3_METADATA*) pUrlInfo->QueryMetaData();
    DBG_ASSERT( pMetaData != NULL );

    pUrlContext = new (pW3Context) URL_CONTEXT( pMetaData, pUrlInfo );
    if ( pUrlContext == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        goto Failure;
    }
    else if( fSetInW3Context )
    {
        //
        // If specified, set the UrlContext in pW3Context.  This is necessary
        // since the below filter notification depends on having this context
        // for APPL_MD_PATH and APPL_PHYSICAL_PATH. Once set the context owns
        // cleaning it up.
        //
        ((W3_MAIN_CONTEXT*)pW3Context)->SetUrlContext( pUrlContext );
    }

    //
    // Now notify URL_MAP filters
    //

    if ( pW3Context->IsNotificationNeeded( SF_NOTIFY_URL_MAP ) )
    {
        STACK_STRA(            straPhys, MAX_PATH + 1 );
        STACK_STRA(            straSavePhys, MAX_PATH + 1 );
        STACK_STRA(            straUrl, MAX_PATH + 1 );
        STACK_STRA(            straScriptMap, MAX_PATH + 1 );
        BOOL                   fRet;
        HTTP_FILTER_URL_MAP_EX filterMap;
        STACK_STRU(            strPhysicalPath, MAX_PATH );

        hr = straPhys.CopyW( pUrlInfo->QueryPhysicalPath()->QueryStr() );
        if ( FAILED( hr ) )
        {
            goto Failure;
        }

        //
        // Save a copy for comparison after filter notification
        //
        hr = straSavePhys.Copy( straPhys );
        if ( FAILED( hr ) )
        {
            goto Failure;
        }

        hr = straUrl.CopyW( strUrl.QueryStr() );
        if ( FAILED( hr ) )
        {
            goto Failure;
        }

        filterMap.pszURL = straUrl.QueryStr();
        filterMap.pszPhysicalPath = straPhys.QueryStr();
        filterMap.cbPathBuff = straPhys.QuerySize();
        filterMap.dwFlags = pMetaData->QueryAccessPerms();
        filterMap.cchMatchingPath = pMetaData->QueryCBMatchingPathA();
        filterMap.cchMatchingURL = pMetaData->QueryCBMatchingUrlA();
        filterMap.pszScriptMapEntry = NULL;

        if ( pUrlInfo->QueryScriptMapEntry() )
        {
            hr = straScriptMap.CopyW( pUrlInfo->QueryScriptMapEntry()->QueryExecutable()->QueryStr() );

            if ( FAILED( hr ) )
            {
                goto Failure;
            }

            filterMap.pszScriptMapEntry = straScriptMap.QueryStr();
        }

        fRet = pW3Context->NotifyFilters( SF_NOTIFY_URL_MAP,
                                          &filterMap,
                                          pfFinished );

        if ( !fRet )
        {
            W3_MAIN_CONTEXT * pMainContext = pW3Context->QueryMainContext();
            DWORD             dwError = GetLastError();

            hr = HRESULT_FROM_WIN32( dwError );

            pMainContext->SetErrorStatus( hr );

            if ( dwError == ERROR_ACCESS_DENIED )
            {
                pMainContext->QueryResponse()->SetStatus( HttpStatusUnauthorized,
                                                          Http401Filter );
            }
            else if ( dwError == ERROR_FILE_NOT_FOUND ||
                      dwError == ERROR_PATH_NOT_FOUND )
            {
                pMainContext->QueryResponse()->SetStatus( HttpStatusNotFound );
            }
            else
            {
                pMainContext->QueryResponse()->SetStatus( HttpStatusServerError );
            }

            goto Failure;
        }

        //
        // If the filter is done, then we're done
        //

        if ( *pfFinished )
        {
            hr = NO_ERROR;
            goto Failure;
        }

        //
        // If the physical path was changed, remember it here
        //

        if ( strcmp( straSavePhys.QueryStr(),
                     filterMap.pszPhysicalPath ) != 0 )
        {
            hr = strPhysicalPath.CopyA( (CHAR*) filterMap.pszPhysicalPath );
            if ( FAILED( hr ) )
            {
                goto Failure;
            }

            hr = pUrlContext->SetPhysicalPath( strPhysicalPath );
            if ( FAILED( hr ) )
            {
                goto Failure;
            }
        }
    }

    //
    // We don't accept short filename since they can break metabase
    // equivalency
    //

    if ( wcschr( pUrlContext->QueryPhysicalPath()->QueryStr(),
                 L'~' ) )
    {
        BOOL fShort = FALSE;

        hr = pMetaData->GetAndRefVrAccessToken( &pTokenEntry );
        if( FAILED( hr ) )
        {
            goto Failure;
        }
        
        if ( pTokenEntry != NULL )
        {
            hToken = pTokenEntry->QueryImpersonationToken();
        }
        else
        {
            hToken = NULL;
        }


        DWORD dwError = CheckIfShortFileName(
                                pUrlContext->QueryPhysicalPath()->QueryStr(),
                                hToken,
                                &fShort );
        if ( dwError != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( dwError );
            goto Failure;
        }

        if ( fShort )
        {
            hr = HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
            goto Failure;
        }
    }
    
    //
    // Check whether the UNC user was valid.  If not, fail
    //
    
    if ( pMetaData->QueryUNCUserInvalid() )
    {
        hr = HRESULT_FROM_WIN32( ERROR_LOGON_FAILURE );
        goto Failure;
    }

    pTraceLog = pW3Context->QueryMainContext()->QueryTraceLog();
    if ( pTraceLog != NULL )
    {
        pTraceLog->Trace( L"%I64x: Successfully metadata for URL '%ws'\n",
                          pRequest->QueryRequestId(),
                          strUrl.QueryStr() );
    }

    *ppUrlContext = pUrlContext;

    if( pTokenEntry != NULL )
    {
        pTokenEntry->DereferenceCacheEntry();
        pTokenEntry = NULL;
    }

    return S_OK;

Failure:

    pTraceLog = pW3Context->QueryMainContext()->QueryTraceLog();
    if ( pTraceLog != NULL )
    {
        pTraceLog->Trace( L"%I64x: Failed to retrieve metadata for URL '%ws'.  hr = %08X\n",
                          pRequest->QueryRequestId(),
                          strUrl.QueryStr(),
                          hr );
    }

    if ( pUrlContext != NULL  )
    {
        if( !fSetInW3Context )
        {
            delete pUrlContext;
        }
    }
    else
    {
        if ( pUrlInfo != NULL )
        {
            pUrlInfo->DereferenceCacheEntry();
        }
    }

    if( pTokenEntry != NULL )
    {
        pTokenEntry->DereferenceCacheEntry();
        pTokenEntry = NULL;
    }

    return hr;
}

//static
HRESULT
W3_STATE_URLINFO::MapPath(
    W3_CONTEXT *            pW3Context,
    STRU &                  strUrl,
    STRU *                  pstrPhysicalPath,
    BOOL                    fDoFiltering,
    DWORD *                 pcchDirRoot,
    DWORD *                 pcchVRoot,
    DWORD *                 pcbAnsiDirRoot,
    DWORD *                 pcbAnsiVRoot,
    DWORD *                 pdwMask
)
/*++

Routine Description:

    Send a URL/Physical-Path pair to a filter for processing

Arguments:

    pW3Context - W3_CONTEXT for the request
    strUrl - The URL to be mapped
    pstrPhysicalPath - Filled with the mapped path upon return.  Set with
                       metadata physical path on entry
    fDoFiltering - Enable filters
    pcchDirRoot - Set to point to number of characters in found physical path
    pcchVRoot - Set to point to number of characters in found virtual path
    pcbAnsiDirRoot - Set to point to number of bytes in found ANSI physical path
    pcbAnsiVRoot - Set to point to number of bytes in found ANSI virtual path
    pdwMask - Set to point to the access perms mask of virtual path

Return Value:

    SUCCEEDED()/FAILED()

--*/
{
    HRESULT         hr = S_OK;
    W3_URL_INFO *   pUrlInfo = NULL;
    W3_METADATA *   pMetaData = NULL;

    DBG_ASSERT( pstrPhysicalPath );

    //
    // Get and keep the metadata and urlinfo for this path
    //

    DBG_ASSERT( g_pW3Server->QueryUrlInfoCache() != NULL );

    hr = g_pW3Server->QueryUrlInfoCache()->GetUrlInfo(
                                        pW3Context,
                                        strUrl,
                                        &pUrlInfo );
    if ( FAILED( hr ) )
    {
        goto Exit;
    }

    DBG_ASSERT( pUrlInfo != NULL );

    //
    // Call the filters if we should do so
    //

    if ( fDoFiltering )
    {
        hr = FilterMapPath( pW3Context,
                            pUrlInfo,
                            pstrPhysicalPath );
    }
    else
    {
        hr = pstrPhysicalPath->Copy( *( pUrlInfo->QueryUrlTranslated() ) );
    }

    if ( FAILED( hr ) )
    {
        goto Exit;
    }

    pMetaData = pUrlInfo->QueryMetaData();
    DBG_ASSERT( pMetaData != NULL );

    //
    // Return the other goodies
    //

    if ( pcchDirRoot != NULL )
    {
        *pcchDirRoot = pMetaData->QueryVrPath()->QueryCCH();
    }

    if ( pcchVRoot != NULL )
    {
        if (strUrl.QueryCCH())
        {
            *pcchVRoot = pMetaData->QueryVrLen();
        }
        else
        {
            *pcchVRoot = 0;
        }
    }

    if ( pcbAnsiDirRoot != NULL )
    {
        *pcbAnsiDirRoot = pMetaData->QueryCBMatchingPathA();
    }

    if ( pcbAnsiVRoot != NULL )
    {
        if ( strUrl.QueryCCH() )
        {
            *pcbAnsiVRoot = pMetaData->QueryCBMatchingUrlA();
        }
        else
        {
            *pcbAnsiVRoot = 0;
        }
    }

    if ( pdwMask != NULL )
    {
        *pdwMask = pMetaData->QueryAccessPerms();
    }

Exit:

    if ( pUrlInfo != NULL )
    {
        pUrlInfo->DereferenceCacheEntry();
        pUrlInfo = NULL;
    }

    return hr;
}

// static
HRESULT
W3_STATE_URLINFO::FilterMapPath(
    W3_CONTEXT *            pW3Context,
    W3_URL_INFO *           pUrlInfo,
    STRU *                  pstrPhysicalPath
    )
/*++

Routine Description:

    Have URL_MAP filters do their thing

Arguments:

    pW3Context - Context
    pUrlInfo - Contains virtual/physical path
    pstrPhysicalPath - Filled with physical path

Return Value:

    SUCCEEDED()/FAILED()

--*/
{
    HRESULT         hr = S_OK;
    BOOL            fFinished = FALSE;
    W3_METADATA *   pMetaData = NULL;
    STACK_STRU(     strFilterPath, MAX_PATH );
    STRU *          pstrFinalPhysical = NULL;

    if ( pW3Context == NULL ||
         pUrlInfo == NULL ||
         pstrPhysicalPath == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    pMetaData = pUrlInfo->QueryMetaData();
    DBG_ASSERT( pMetaData != NULL );

    //
    // We now have the metadata physical path.  Let filters change it here
    //

    if ( pW3Context->IsNotificationNeeded( SF_NOTIFY_URL_MAP ) )
    {
        STACK_STRA(            straPhys, MAX_PATH + 1 );
        STACK_STRA(            straSavePhys, MAX_PATH + 1 );
        STACK_STRA(            straUrl, MAX_PATH + 1 );
        STACK_STRA(            straScriptMap, MAX_PATH + 1 );
        BOOL                   fRet;
        HTTP_FILTER_URL_MAP_EX filterMap;

        hr = straPhys.CopyW( pUrlInfo->QueryUrlTranslated()->QueryStr() );
        if ( FAILED( hr ) )
        {
            goto Exit;
        }

        //
        // Save a copy for comparison after filter notification
        //
        hr = straSavePhys.Copy( straPhys );
        if ( FAILED( hr ) )
        {
            goto Exit;
        }

        hr = straUrl.CopyW( pUrlInfo->QueryUrl() );
        if ( FAILED( hr ) )
        {
            goto Exit;
        }

        filterMap.pszURL = straUrl.QueryStr();
        filterMap.pszPhysicalPath = straPhys.QueryStr();
        filterMap.cbPathBuff = straPhys.QuerySize();
        filterMap.dwFlags = pMetaData->QueryAccessPerms();
        filterMap.cchMatchingPath = pMetaData->QueryCBMatchingPathA();
        filterMap.cchMatchingURL = pMetaData->QueryCBMatchingUrlA();
        filterMap.pszScriptMapEntry = NULL;

        if ( pUrlInfo->QueryScriptMapEntry() )
        {
            hr = straScriptMap.CopyW( pUrlInfo->QueryScriptMapEntry()->QueryExecutable()->QueryStr() );

            if ( FAILED( hr ) )
            {
                goto Exit;
            }

            filterMap.pszScriptMapEntry = straScriptMap.QueryStr();
        }

        fRet = pW3Context->NotifyFilters( SF_NOTIFY_URL_MAP,
                                          &filterMap,
                                          &fFinished );

        //
        // Ignore finished flag in this case since we really can't do much
        // to advance to finish (since an ISAPI is calling this)
        //

        if ( !fRet )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto Exit;
        }

        //
        // Remember the mapped path
        //

        if ( strcmp( straSavePhys.QueryStr(),
                     filterMap.pszPhysicalPath ) != 0 )
        {
            hr = strFilterPath.CopyA( (CHAR*) filterMap.pszPhysicalPath );
            if ( FAILED( hr ) )
            {
                goto Exit;
            }

            pstrFinalPhysical = &strFilterPath;
        }
        else
        {
            pstrFinalPhysical = pUrlInfo->QueryUrlTranslated();
        }
    }
    else
    {
        //
        // No filter is mapping, therefore just take the URL_INFO's physical
        // path
        //

        pstrFinalPhysical = pUrlInfo->QueryUrlTranslated();

        DBG_ASSERT( pstrFinalPhysical != NULL );
    }

    //
    // We don't accept short filename since they can break metabase
    // equivalency
    //

    if ( wcschr( pstrFinalPhysical->QueryStr(),
                 L'~' ) )
    {
        BOOL fShort = FALSE;
        DWORD dwError = CheckIfShortFileName(
                                pstrFinalPhysical->QueryStr(),
                                pW3Context->QueryImpersonationToken(),
                                &fShort );
        if ( dwError != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( dwError );
            goto Exit;
        }

        if ( fShort )
        {
            hr = HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
            goto Exit;
        }
    }

    //
    // Copy the physical path is requested
    //

    hr = pstrPhysicalPath->Copy( *pstrFinalPhysical );
    if ( FAILED( hr ) )
    {
        goto Exit;
    }

Exit:

    return hr;
}

DWORD
CheckIfShortFileName(
    IN WCHAR *          pszPath,
    IN HANDLE           hImpersonation,
    OUT BOOL *          pfShort
)
/*++
    Description:

        This function takes a suspected NT/Win95 short filename and checks if there's
        an equivalent long filename.  For example, c:\foobar\ABCDEF~1.ABC is the same
        as c:\foobar\abcdefghijklmnop.abc.

        NOTE: This function should be called unimpersonated - the FindFirstFile() must
        be called in the system context since most systems have traverse checking turned
        off - except for the UNC case where we must be impersonated to get network access.

    Arguments:

        pszPath - Path to check
        hImpersonation - Impersonation handle if this is a UNC path - can be NULL if not UNC
        pfShort - Set to TRUE if an equivalent long filename is found

    Returns:

        Win32 error on failure
--*/
{
    DWORD              err = NO_ERROR;
    WIN32_FIND_DATA    FindData;
    WCHAR *            psz;
    BOOL               fUNC;

    psz = wcschr( pszPath, L'~' );
    *pfShort = FALSE;
    fUNC = (*pszPath == L'\\');

    //
    //  Loop for multiple tildas - watch for a # after the tilda
    //

    while ( psz++ )
    {
        if ( *psz >= L'0' && *psz <= L'9' )
        {
            WCHAR   achTmp[MAX_PATH];
            WCHAR * pchEndSeg;
            WCHAR * pchBeginSeg;
            HANDLE  hFind;

            //
            //  Isolate the path up to the segment with the
            //  '~' and do the FindFirst with that path
            //

            pchEndSeg = wcschr( psz, L'\\' );
            if ( !pchEndSeg )
            {
                pchEndSeg = psz + wcslen( psz );
            }

            //
            //  If the string is beyond MAX_PATH then we allow it through
            //

            if ( ((INT) (pchEndSeg - pszPath)) >= MAX_PATH )
            {
                return NO_ERROR;
            }

            memcpy( achTmp,
                    pszPath,
                    (INT) (pchEndSeg - pszPath) * sizeof( WCHAR ) );
            achTmp[pchEndSeg - pszPath] = L'\0';

            if ( fUNC && hImpersonation )
            {
                if ( !SetThreadToken( NULL, hImpersonation ))
                {
                    return GetLastError();
                }
            }

            //
            // IVANPASH: Although it looks simpler to use GetLongPathName
            // instead of manually traversing the folders, we must use
            // FindFirstFileW, because GetLongPathName requires ACLs for
            // IIS_WPG from the root of the drive to work.
            //

            hFind = FindFirstFileW( achTmp, &FindData );

            if ( fUNC && hImpersonation )
            {
                RevertToSelf();
            }

            if ( hFind == INVALID_HANDLE_VALUE )
            {
                err = GetLastError();

                DBGPRINTF(( DBG_CONTEXT,
                            "FindFirst failed!! - \"%s\", error %d\n",
                            achTmp,
                            GetLastError() ));

                //
                //  If the FindFirstFile() fails to find the file then return
                //  success - the path doesn't appear to be a valid path which
                //  is ok.
                //

                if ( err == ERROR_FILE_NOT_FOUND ||
                     err == ERROR_PATH_NOT_FOUND ||
                     err == ERROR_INVALID_NAME )
                {
                    return NO_ERROR;
                }

                return err;
            }

            DBG_REQUIRE( FindClose( hFind ));

            //
            //  Isolate the last segment of the string which should be
            //  the potential short name equivalency
            //

            pchBeginSeg = wcsrchr( achTmp, L'\\' );
            DBG_ASSERT( pchBeginSeg );
            pchBeginSeg++;

            //
            //  If the last segment doesn't match the long name then this is
            //  the short name version of the path
            //

            if ( _wcsicmp( FindData.cFileName, pchBeginSeg ))
            {
                *pfShort = TRUE;
                return NO_ERROR;
            }
        }

        psz = wcschr( psz, L'~' );
    }

    return err;
}

HRESULT
URL_CONTEXT::OpenFile(
    CACHE_USER *            pFileUser,
    W3_FILE_INFO **         ppOpenFile,
    FILE_CACHE_ASYNC_CONTEXT * pAsyncContext,
    BOOL *                  pfHandledSync,
    BOOL                    fAllowNoBuffering,
    BOOL                    fCheckForExistenceOnly
)
/*++

Routine Description:

    Open the physical path for this request.  If a map path filter did some
    redirecting, we will use that path.  Otherwise we will just use the
    path determined by metadata and cached in the W3_URL_INFO

Arguments:

    pFileUser - User to open file as
    ppOpenFile - Set to file cache entry on success
    pAsyncContext - In case an async read is desired, context with callback information
    pfHandledSync - Did the open complete synchronously
    fAllowNoBuffering - Allow the file to be opened with FILE_FLAG_NO_BUFFERING
    fCheckForExistenceOnly - Only interested in existence of the file

Return Value:

    HRESULT

--*/
{
    HRESULT             hr;
    BOOL                fDoCache;

    DBG_ASSERT( QueryMetaData() != NULL );

    fDoCache = !QueryMetaData()->QueryNoCache();

    //
    // If an ISAPI filter changed the physical path, then we need to go
    // directly to the file cache.  Otherwise, we can go thru the
    // W3_URL_INFO which may already have the cached file associated
    //

    if ( _strPhysicalPath.IsEmpty() )
    {
        //
        // No filter.  Fast path :-)
        //

        DBG_ASSERT( _pUrlInfo != NULL );

        hr = _pUrlInfo->GetFileInfo( pFileUser,
                                     fDoCache,
                                     ppOpenFile,
                                     pAsyncContext,
                                     pfHandledSync,
                                     fAllowNoBuffering,
                                     fCheckForExistenceOnly );
    }
    else
    {
        //
        // Filter case.  Must lookup in file cache :-(
        //

        DBG_ASSERT( g_pW3Server->QueryFileCache() != NULL );

        hr = g_pW3Server->QueryFileCache()->GetFileInfo(
                                        _strPhysicalPath,
                                        QueryMetaData()->QueryDirmonConfig(),
                                        pFileUser,
                                        fDoCache,
                                        ppOpenFile,
                                        pAsyncContext,
                                        pfHandledSync,
                                        fAllowNoBuffering,
                                        fCheckForExistenceOnly );
    }

    return hr;
}

//static
HRESULT
URL_CONTEXT::Initialize(
    VOID
)
/*++

Routine Description:

    Initialize URL_CONTEXT lookaside

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    return NO_ERROR;
}

//static
VOID
URL_CONTEXT::Terminate(
    VOID
)
/*++

Routine Description:

    Clean up URL_CONTEXT lookaside

Arguments:

    None

Return Value:

    HRESULT

--*/
{
}


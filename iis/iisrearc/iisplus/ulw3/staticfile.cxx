/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module Name :
     staticfile.cxx

   Abstract:
     Handle static file request

   Author:
     Bilal Alam (balam)             7-Jan-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"
#include "staticfile.hxx"

ALLOC_CACHE_HANDLER *       W3_STATIC_FILE_HANDLER::sm_pachStaticFileHandlers;

HRESULT
W3_STATIC_FILE_HANDLER::HandleDefaultLoad(
    W3_CONTEXT *            pW3Context,
    BOOL *                  pfHandled,
    BOOL *                  pfAsyncPending
)
/*++

Routine Description:

    Attempts to find a default load file applicable for this request.  If it
    does, it will switch the URL of the request and back track.

Arguments:

    pW3Context - Context
    pfHandled - Set to TRUE if this function has set a response or switched URL
                (in other words, no more processing is required)
    pfAsyncPending - Set to TRUE if async is pending so bail

Return Value:

    HRESULT - If not NO_ERROR, then *pfHandled is irrelevent

--*/
{
    URL_CONTEXT *           pUrlContext;
    W3_METADATA *           pMetaData;
    STACK_STRU(             strDefaultFiles, MAX_PATH );
    HRESULT                 hr = NO_ERROR;
    W3_REQUEST *            pRequest = pW3Context->QueryRequest();
    STRU *                  pstrPhysical;
    STACK_STRU(             strNextFile, MAX_PATH );
    WCHAR *                 pszNextFile;
    WCHAR *                 pszEndFile;
    W3_FILE_INFO *          pOpenFile = NULL;
    BOOL                    fFound = FALSE;
    STACK_STRU(             strNewUrl, MAX_PATH );
    WCHAR *                 pszQuery;
    CACHE_USER              FileUser;

    DBG_ASSERT( pW3Context != NULL );
    DBG_ASSERT( pRequest != NULL );
    DBG_ASSERT( pfHandled != NULL );
    DBG_ASSERT( pfAsyncPending != NULL );

    *pfHandled = FALSE;
    *pfAsyncPending = FALSE;

    //
    // Get the configuration info
    //

    pUrlContext = pW3Context->QueryUrlContext();
    DBG_ASSERT( pUrlContext != NULL );

    pMetaData = pUrlContext->QueryMetaData();
    DBG_ASSERT( pMetaData != NULL );

    pstrPhysical = pUrlContext->QueryPhysicalPath();
    DBG_ASSERT( pstrPhysical != NULL );

    hr = pRequest->GetUrl( &strNewUrl );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    DBG_ASSERT( strNewUrl.QueryStr() != NULL );

    //
    // First ensure the path is / suffixed.  Otherwise, redirect to such
    //
    if (strNewUrl.QueryCCH() && strNewUrl.QueryStr()[strNewUrl.QueryCCH() - 1] != L'/')
    {
        //
        // Before redirecting, first make sure it is a GET or a HEAD
        //
        HTTP_VERB VerbType = pRequest->QueryVerbType();
        if ( VerbType != HttpVerbGET &&
             VerbType != HttpVerbHEAD )
        {
            pW3Context->QueryResponse()->SetStatus( HttpStatusMethodNotAllowed );
            pW3Context->SetErrorStatus( HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION) );
            hr = pW3Context->SetupAllowHeader();
            if ( FAILED( hr ) )
            {
                return hr;
            }

            return S_OK;
        }

        //
        // Append the suffix '/'
        //
        if (FAILED(hr = strNewUrl.Escape()) ||
            FAILED(hr = strNewUrl.Append(L"/")))
        {
            return hr;
        }

        //
        // Do the HTTP redirect
        //
        if (FAILED(hr = pW3Context->SetupHttpRedirect(strNewUrl,
                                                      TRUE,
                                                      HttpStatusMovedPermanently)))
        {
            return hr;
        }

        //
        // Tell callers we are finished
        //
        *pfHandled = TRUE;
        return S_OK;
    }

    //
    // Look for default load files
    //

    hr = strDefaultFiles.Copy( *pMetaData->QueryDefaultLoadFiles() );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    pszNextFile = strDefaultFiles.QueryStr();
    while ( pszNextFile != NULL &&
            *pszNextFile != L'\0' )
    {
        pszEndFile = wcschr( pszNextFile, L',' );
        if ( pszEndFile != NULL )
        {
            *pszEndFile = L'\0';
        }

        while (iswspace(*pszNextFile))
        {
            pszNextFile++;
        }

        //
        // Append portion to directory to create a filename to check for
        //

        hr = strNextFile.Copy( *pstrPhysical );
        if ( FAILED( hr ) )
        {
            return hr;
        }

        //
        // Remove any query string
        //

        pszQuery = wcschr( pszNextFile, L'?' );
        if ( pszQuery != NULL )
        {
            hr = strNextFile.Append( pszNextFile,
                                     (DWORD)DIFF( pszQuery - pszNextFile ) );
        }
        else
        {
            hr = strNextFile.Append( pszNextFile );
        }
        if ( FAILED( hr ) )
        {
            return hr;
        }

        //
        // Make a FS path
        //

        FlipSlashes( strNextFile.QueryStr() );

        //
        // Open the file
        //

        pW3Context->QueryFileCacheUser( &FileUser );

        DBG_ASSERT( g_pW3Server->QueryFileCache() != NULL );

        hr = g_pW3Server->QueryFileCache()->GetFileInfo(
                                            strNextFile,
                                            pMetaData->QueryDirmonConfig(),
                                            &FileUser,
                                            !( pMetaData->QueryNoCache() ),
                                            &pOpenFile );
        if ( FAILED( hr ) )
        {
            DWORD           dwError = WIN32_FROM_HRESULT( hr );

            DBG_ASSERT( pOpenFile == NULL );

            //
            // If not found, or name invalid, that's ok -> proceed to next file
            //

            if ( dwError != ERROR_FILE_NOT_FOUND &&
                 dwError != ERROR_PATH_NOT_FOUND &&
                 dwError != ERROR_INVALID_NAME )
            {
                return hr;
            }

            hr = NO_ERROR;
        }
        else
        {
            DWORD           dwAttributes;

            //
            // Great, we can open the file.  We only need it for attributes.
            //

            DBG_ASSERT( pOpenFile != NULL );

            dwAttributes = pOpenFile->QueryAttributes();

            pOpenFile->DereferenceCacheEntry();
            pOpenFile = NULL;

            if ( dwAttributes & FILE_ATTRIBUTE_DIRECTORY )
            {
                //
                // If we see a directory, we ignore it and continue to
                // the next one
                //
            }
            else
            {
                fFound = TRUE;
                break;
            }
        }

        //
        // Goto next file
        //

        pszNextFile = pszEndFile ? pszEndFile + 1 : NULL;
    }

    //
    // Change the url and retrack
    //

    if ( fFound )
    {
        //
        // Ok.  We can change the URL and retrack.  Do so.
        //

        hr = strNewUrl.Append( pszNextFile );
        if ( FAILED( hr ) )
        {
            return hr;
        }

        //
        // Change the URL
        //

        hr = pRequest->SetUrl( strNewUrl, FALSE );
        if ( FAILED( hr ) )
        {
            return hr;
        }

        hr = pW3Context->ExecuteChildRequest( pRequest,
                                              FALSE,
                                              W3_FLAG_ASYNC );
        if ( FAILED( hr ) )
        {
            return hr;
        }
        else
        {
            *pfHandled = TRUE;
            *pfAsyncPending = TRUE;
            return NO_ERROR;
        }
    }
    else
    {
        //
        // If not found, the caller will continue since *pfHandled == FALSE
        // if we're here
        //
    }

    return NO_ERROR;
}

HRESULT
W3_STATIC_FILE_HANDLER::DirectoryDoWork(
    W3_CONTEXT *            pW3Context,
    BOOL *                  pfAsyncPending
)
/*++

Routine Description:

    Handle directories.  This means default loads and directory listings

Arguments:

    pW3Context - Context
    pfAsyncPending - Set to TRUE if async pending

Return Value:

    HRESULT

--*/
{
    DWORD               dwDirBrowseFlags;
    URL_CONTEXT *       pUrlContext;
    HRESULT             hr;
    BOOL                fHandled = FALSE;
    CACHE_USER          fileUser;
    BOOL                fImpersonated = FALSE;

    DBG_ASSERT( pW3Context != NULL );
    DBG_ASSERT( pfAsyncPending != NULL );

    *pfAsyncPending = FALSE;

    W3_REQUEST *pRequest = pW3Context->QueryRequest();
    DBG_ASSERT(pRequest != NULL);

    pUrlContext = pW3Context->QueryUrlContext();
    DBG_ASSERT( pUrlContext != NULL );

    //
    // Get the directory browsing flags for this directory
    //
    dwDirBrowseFlags = pUrlContext->QueryMetaData()->QueryDirBrowseFlags();

    //
    // First check for a default load (by first checking whether we are
    // allowed to serve default load)
    //
    if ( dwDirBrowseFlags & MD_DIRBROW_LOADDEFAULT )
    {
        //
        // OK.  Look for a default load
        //

        hr = HandleDefaultLoad( pW3Context,
                                &fHandled,
                                pfAsyncPending );

        if ( FAILED( hr ) || fHandled || *pfAsyncPending )
        {
            return hr;
        }
    }

    //
    // If doing directory listing, first make sure it is a GET or a HEAD
    //
    HTTP_VERB VerbType = pRequest->QueryVerbType();
    if ( VerbType != HttpVerbGET &&
         VerbType != HttpVerbHEAD )
    {
        pW3Context->QueryResponse()->SetStatus( HttpStatusMethodNotAllowed );
        pW3Context->SetErrorStatus( HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION) );
        hr = pW3Context->SetupAllowHeader();
        if ( FAILED( hr ) )
        {
            return hr;
        }

        return S_OK;
    }

    //
    // OK.  Check for whether directory listings are enabled
    //
    if ( dwDirBrowseFlags & MD_DIRBROW_ENABLED )
    {
        //
        // We may need to impersonate some other user to open the file
        //

        pW3Context->QueryFileCacheUser( &fileUser );

        if ( fileUser._hToken != NULL )
        {
            if ( !SetThreadToken( NULL, fileUser._hToken ) )
            {
                return HRESULT_FROM_WIN32( GetLastError() );
            }
            fImpersonated = TRUE;
        }

        hr = HandleDirectoryListing( pW3Context,
                                     &fHandled );

        if( fImpersonated )
        {
            RevertToSelf();
            fImpersonated = FALSE;
        }

        if ( FAILED( hr ) || fHandled )
        {
            return hr;
        }
    }

    //
    // If we are here, then neither browsing nor default loads are enabled.
    // There is nothing we can do but return a 403.
    //

    pW3Context->QueryResponse()->SetStatus( HttpStatusForbidden,
                                            Http403DirBrowsingDenied );
    pW3Context->SetErrorStatus( HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) );
    return NO_ERROR;
}


HRESULT
GetTypeAndSubType(
    LPCSTR                  pszType,
    STRA *                  pstrMainType,
    STRA *                  pstrSubType,
    BOOL *                  pfTypeOk
)
/*++

Routine Description:

    Given a mimetype of "foobar/barfoo", return "foobar" as the main type
    and "barfoo" as the subtype.

Arguments:

    pszType - Whole mime type
    pstrMainType - Filled with main type
    pstrSubType - Filled with sub type
    pfTypeOk - Is this mime type ok?

Return Value:

    HRESULT

--*/
{
    HRESULT hr;

    LPCSTR pszSlash = strchr( pszType, '/' );
    if (pszSlash == NULL)
    {
        *pfTypeOk = FALSE;
        return S_OK;
    }

    hr = pstrMainType->Copy( pszType,
                             (DWORD)DIFF( pszSlash - pszType ) );
    hr = pstrSubType->Copy( pszSlash + 1 );

    *pfTypeOk = TRUE;
    return hr;
}


HRESULT
IsAcceptable(
    LPCSTR              pszContentType,
    LPCSTR              pszAcceptHeader,
    USHORT              cchAcceptHeader,
    BOOL *              pfIsAcceptAble
)
/*++

Routine Description:

    Return whether given content type is acceptable for the given
    Accept: header

Arguments:

    pszContentType - Content type
    pszAcceptHeader - Accept header to check
    pfIsAcceptAble - Filled with bool indicating whether type is acceptable

Return Value:

    HRESULT

--*/
{
    HRESULT         hr;
    BOOL            fTypeOk;

    //
    // Quickly handle the */* case
    //

    if (cchAcceptHeader >= 3 &&
        pszAcceptHeader[cchAcceptHeader - 1] == '*' &&
        pszAcceptHeader[cchAcceptHeader - 2] == '/' &&
        pszAcceptHeader[cchAcceptHeader - 3] == '*' )
    {
        *pfIsAcceptAble = TRUE;
        return S_OK;
    }

    if ( strstr(pszAcceptHeader, "*/*") != NULL )
    {
        *pfIsAcceptAble = TRUE;
        return S_OK;
    }

    //
    // Break the Content-Type into the main- and sub-content-type
    //
    STACK_STRA ( strMainContentType, 32);
    STACK_STRA ( strSubContentType, 32);
    if ( FAILED( hr = GetTypeAndSubType( pszContentType,
                                         &strMainContentType,
                                         &strSubContentType,
                                         &fTypeOk ) ) )
    {
        return hr;
    }
    if ( !fTypeOk )
    {
        *pfIsAcceptAble = FALSE;
        return S_OK;
    }

    //
    // Skip over any spaces
    //
    while ( *pszAcceptHeader == ' ' )
    {
        pszAcceptHeader++;
    }

    STACK_STRA (strAcceptType, 64);
    STACK_STRA (strMainAcceptType, 32);
    STACK_STRA (strSubAcceptType, 32);

    for (;;)
    {
        //
        // Multiple Acceptable Types are ',' separated, get the next one
        //
        CHAR * pszComma = strchr( pszAcceptHeader, L',' );
        if ( pszComma == NULL )
        {
            if ( FAILED( hr = strAcceptType.Copy( pszAcceptHeader ) ) )
            {
                return hr;
            }
        }
        else
        {
            if ( FAILED( hr = strAcceptType.Copy( pszAcceptHeader,
                                  (DWORD)DIFF( pszComma - pszAcceptHeader ) ) ) )
            {
                return hr;
            }
        }

        //
        // Trim out any quality specifier specified after a ';'
        //
        CHAR * pszQuality = strchr( strAcceptType.QueryStr(), ';' );
        if ( pszQuality != NULL )
        {
            strAcceptType.SetLen((DWORD)DIFF(pszQuality - strAcceptType.QueryStr()));
        }

        //
        // Trim any spaces at the end
        //
        INT iSpace = strAcceptType.QueryCCH() - 1;
        while ( iSpace >= 0 &&
                strAcceptType.QueryStr()[iSpace] == ' ' )
        {
            iSpace--;
        }
        strAcceptType.SetLen( iSpace + 1 );

        //
        // Get the main- and sub-Accept types for this type
        //
        if ( FAILED(hr = GetTypeAndSubType( strAcceptType.QueryStr(),
                                            &strMainAcceptType,
                                            &strSubAcceptType,
                                            &fTypeOk ) ) )
        {
            return hr;
        }

        if ( fTypeOk )
        {
            //
            // Now actually find out if this type is acceptable
            //
            if ( !_stricmp( strMainAcceptType.QueryStr(),
                            strMainContentType.QueryStr() ) )
            {
                if ( !strcmp( strSubAcceptType.QueryStr(), "*" ) ||
                     !_stricmp( strSubAcceptType.QueryStr(),
                                strSubContentType.QueryStr() ) )
                {
                    *pfIsAcceptAble = TRUE;
                    return S_OK;
                }
            }
        }

        //
        // Set AcceptHeader to the start of the next type
        //
        if (pszComma == NULL)
        {
            *pfIsAcceptAble = FALSE;
            return S_OK;
        }
        pszAcceptHeader = pszComma + 1;
        while ( *pszAcceptHeader == ' ' )
        {
            pszAcceptHeader++;
        }
    }
}


HRESULT
W3_STATIC_FILE_HANDLER::FileDoWork(
    W3_CONTEXT *                pW3Context,
    W3_FILE_INFO *              pOpenFile
)
/*++

Routine Description:

    Handle files (non-directories).

Arguments:

    pW3Context - Context
    pOpenFile - W3_FILE_INFO with the file to send

Return Value:

    HRESULT

--*/
{
    ULARGE_INTEGER      liFileSize;
    W3_RESPONSE *       pResponse;
    W3_REQUEST  *       pRequest;
    W3_URL_INFO *       pUrlInfo;
    W3_METADATA *       pMetaData;
    HRESULT             hr;
    STACK_STRU        ( strUrl, MAX_PATH );
    LPCSTR              pszRange;
    BOOL                fHandled = FALSE;
    CACHE_USER          fileUser;

    DBG_ASSERT( pW3Context != NULL );
    DBG_ASSERT( pOpenFile != NULL );
    pResponse = pW3Context->QueryResponse();
    DBG_ASSERT( pResponse != NULL );
    pRequest = pW3Context->QueryRequest();
    DBG_ASSERT( pRequest != NULL );
    pUrlInfo = pW3Context->QueryUrlContext()->QueryUrlInfo();
    DBG_ASSERT( pUrlInfo != NULL );
    pMetaData = pW3Context->QueryUrlContext()->QueryMetaData();
    DBG_ASSERT( pMetaData != NULL );


    //
    // First make sure it a GET or a HEAD
    //
    HTTP_VERB VerbType = pRequest->QueryVerbType();
    if ( VerbType != HttpVerbGET &&
         VerbType != HttpVerbHEAD )
    {
        pW3Context->QueryResponse()->SetStatus( HttpStatusMethodNotAllowed );
        pW3Context->SetErrorStatus( HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION) );
        hr = pW3Context->SetupAllowHeader();
        if ( FAILED( hr ) )
        {
            goto Failure;
        }

        return S_OK;
    }

    //
    // Any Expect headers are not acceptable for static file requests
    //
    LPCSTR pszExpect = pRequest->GetHeader( HttpHeaderExpect );
    if (pszExpect != NULL)
    {
        pW3Context->QueryResponse()->SetStatus( HttpStatusExpectationFailed );
        return S_OK;
    }

    //
    // If this an image-map file, do the image-map stuff
    //
    if (pUrlInfo->QueryGateway() == GATEWAY_MAP)
    {
        fHandled = FALSE;
        hr = MapFileDoWork(pW3Context, pOpenFile, &fHandled);
        if (FAILED(hr))
        {
            goto Failure;
        }

        if (fHandled)
        {
            return hr;
        }

        //
        // fHandled was false, so this is a .map file which wasn't really
        // an image-map file, handle it as any other static file
        //
    }

    //
    // Add Cache-Control and Expires header if so configured
    //
    STRA *pstrCacheControlHeader = pMetaData->QueryCacheControlHeader();
    if (!pstrCacheControlHeader->IsEmpty())
    {
        if (FAILED(hr = pResponse->SetHeaderByReference(
                            HttpHeaderCacheControl,
                            pstrCacheControlHeader->QueryStr(),
                            (USHORT)pstrCacheControlHeader->QueryCCH())))
        {
            goto Failure;
        }
    }

    if (pMetaData->QueryExpireMode() == EXPIRE_MODE_STATIC)
    {
        STRA *pstrExpireHeader = pMetaData->QueryExpireHeader();

        if (FAILED(hr = pResponse->SetHeaderByReference(
                            HttpHeaderExpires,
                            pstrExpireHeader->QueryStr(),
                            (USHORT)pstrExpireHeader->QueryCCH())))
        {
            goto Failure;
        }
    }

    //
    // Do compression, if so configured
    //
    if (pMetaData->QueryDoStaticCompression() &&
        !pW3Context->QueryDoneWithCompression())
    {
        BOOL fDoCache = FALSE;

        if (FAILED(hr = HTTP_COMPRESSION::DoStaticFileCompression(
                            pW3Context, &pOpenFile, &fDoCache)))
        {
            goto Failure;
        }
        m_pOpenFile = pOpenFile;

        if (!fDoCache)
        {
            //
            // If this file is compressible but we didn't compress it,
            // don't let http.sys store the uncompressed version in its
            // cache
            //

            pW3Context->DisableUlCache();
        }
    }
    
    //
    // If we had to use a default mimemapping, then don't serve out the file
    //
    
    if ( pUrlInfo->QueryDefaultMimeMap() )
    {
        pW3Context->SetErrorStatus( HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED ) );
        pResponse->SetStatus( HttpStatusNotFound, Http404DeniedByMimeMap );
        return NO_ERROR;
    }

    //
    // Check if the Content-Type is acceptable to the client
    //

    STRA *pstrContentType = pUrlInfo->QueryContentType();
    USHORT cchAccept;
    LPCSTR pszAccept      = pRequest->GetHeader( HttpHeaderAccept, &cchAccept );
    if ( pszAccept != NULL && *pszAccept != L'\0' )
    {
        BOOL fIsAcceptAble;

        if ( FAILED( hr = IsAcceptable( pstrContentType->QueryStr(),
                                        pszAccept,
                                        cchAccept,
                                        &fIsAcceptAble ) ) )
        {
            goto Failure;
        }

        if ( !fIsAcceptAble )
        {
            pResponse->ClearHeaders();
            pResponse->SetStatus( HttpStatusNotAcceptable );
            return S_OK;
        }
    }

    //
    // Setup the response headers.  First ETag
    //

    hr = pResponse->SetHeaderByReference( HttpHeaderEtag,
                                          pOpenFile->QueryETag(),
                                          pOpenFile->QueryETagSize() );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }

    //
    // Next is Last-Modified
    //

    hr = pResponse->SetHeaderByReference( HttpHeaderLastModified,
                                          pOpenFile->QueryLastModifiedString(),
                                          GMT_STRING_SIZE - 1 );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }

    //
    // Next is Content-Location.  We only need to send this header if
    // we have internally changed the URL of the request.  In other words,
    // if this is a child execute
    //

    if ( pW3Context->QuerySendLocation() )
    {
        STACK_STRA (strContentLocation, MAX_PATH);
        STACK_STRA (strRawUrl, MAX_PATH);

        if (FAILED(hr = pRequest->GetRawUrl(&strRawUrl)) ||
            FAILED(hr = pRequest->BuildFullUrl(strRawUrl,
                                               &strContentLocation,
                                               FALSE)) ||
            FAILED(hr = pResponse->SetHeader(HttpHeaderContentLocation,
                                             strContentLocation.QueryStr(),
                                             (USHORT)strContentLocation.QueryCCH())))
        {
            goto Failure;
        }
    }

    //
    // Next is Accept-Ranges
    //
    if ( FAILED( hr = pResponse->SetHeaderByReference( HttpHeaderAcceptRanges,
                                                       "bytes", 5 ) ) )
    {
        goto Failure;
    }

    //
    // Handle the If-* (except If-Range) headers if present
    //
    fHandled = FALSE;
    if ( FAILED( hr = CacheValidationDoWork( pW3Context,
                                             pOpenFile,
                                             &fHandled ) ) )
    {
        goto Failure;
    }

    if ( fHandled )
    {
        return hr;
    }

    //
    // Now handle If-Range and Range headers
    //
    pszRange = pRequest->GetHeader( HttpHeaderRange );
    if ( ( pszRange != NULL ) &&
         ( !_strnicmp ( pszRange, "bytes", 5 ) ) )
    {
        //
        // Handle range request
        //
        fHandled = FALSE;
        if ( FAILED( hr = RangeDoWork( pW3Context, pOpenFile, &fHandled ) ) )
        {
            goto Failure;
        }

        if ( fHandled )
        {
            return hr;
        }
    }

    //
    // If we fell thru, then we are sending out the entire file
    //

    //
    // Setup Content-Type
    //
    if ( FAILED( hr = pResponse->SetHeaderByReference(
                          HttpHeaderContentType,
                          pstrContentType->QueryStr(),
                          (USHORT)pstrContentType->QueryCCH() ) ) )
    {
        goto Failure;
    }

    //
    // Setup the response chunks
    //
    pOpenFile->QuerySize( &liFileSize );

    if (liFileSize.QuadPart > 0)
    {
        if ( pOpenFile->QueryFileBuffer() != NULL &&
             liFileSize.HighPart == 0 )
        {
            hr = pResponse->AddMemoryChunkByReference(
                                pOpenFile->QueryFileBuffer(),
                                liFileSize.LowPart );
        }
        else
        {
            hr = pResponse->AddFileHandleChunk( pOpenFile->QueryFileHandle(),
                                                0,
                                                liFileSize.QuadPart );
        }

        if ( FAILED( hr ) )
        {
            goto Failure;
        }
    }

    // perf ctr
    pW3Context->QuerySite()->IncFilesSent();

    // Setup the document footer
    if (pMetaData->QueryIsFooterEnabled())
    {
        if (!pMetaData->QueryFooterString()->IsEmpty() )
        {
            STRA *pFooterString = pMetaData->QueryFooterString();
            if (pFooterString->QueryCCH())
            {
                if (FAILED(hr = pResponse->AddMemoryChunkByReference(
                                               pFooterString->QueryStr(),
                                               pFooterString->QueryCCH())))
                {
                    goto Failure;
                }
            }
        }
        else if (!pMetaData->QueryFooterDocument()->IsEmpty() )
        {
            //
            // When a footer document changes, we don't know which URL to flush,
            // so if footer is enabled, don't allow UL to cache this response
            //
            pW3Context->DisableUlCache();

            pW3Context->QueryFileCacheUser( &fileUser );

            DBG_ASSERT( m_pFooterDocument == NULL );

            DBG_ASSERT( g_pW3Server->QueryFileCache() );

            hr = g_pW3Server->QueryFileCache()->GetFileInfo(
                                        *(pMetaData->QueryFooterDocument()),
                                        NULL,
                                        &fileUser,
                                        TRUE,
                                        &m_pFooterDocument );
            if ( SUCCEEDED( hr ) )
            {
                DBG_ASSERT( m_pFooterDocument != NULL );

                m_pFooterDocument->QuerySize( &liFileSize );

                if (liFileSize.QuadPart > 0)
                {
                    if ( m_pFooterDocument->QueryFileBuffer() != NULL &&
                         liFileSize.HighPart == 0 )
                    {
                        hr = pResponse->AddMemoryChunkByReference(
                                        m_pFooterDocument->QueryFileBuffer(),
                                        liFileSize.LowPart );
                    }
                    else
                    {
                        hr = pResponse->AddFileHandleChunk(
                                        m_pFooterDocument->QueryFileHandle(),
                                        0,
                                        liFileSize.QuadPart );
                    }

                    if ( FAILED( hr ) )
                    {
                        goto Failure;
                    }
                }
            }
            else
            {
                //
                // Could not open the footer document.  Sub in a error string
                //

                CHAR achErrorString[ 512 ];
                DWORD cbErrorString = sizeof( achErrorString );

                hr = g_pW3Server->LoadString( IDS_ERROR_FOOTER,
                                              achErrorString,
                                              &cbErrorString );
                if ( FAILED( hr ) )
                {
                    goto Failure;
                }

                hr = m_strFooterString.Copy( achErrorString, cbErrorString );
                if ( FAILED( hr ) )
                {
                    goto Failure;
                }

                hr = pResponse->AddMemoryChunkByReference(
                                        m_strFooterString.QueryStr(),
                                        m_strFooterString.QueryCCH() );
                if ( FAILED( hr ) )
                {
                    goto Failure;
                }
            }
        }
    }

    return S_OK;

Failure:
    //
    // It is our responsibility to ensure that there is no incomplete response
    //

    pResponse->Clear();
    return hr;
}

CONTEXT_STATUS
W3_STATIC_FILE_HANDLER::DoWork(
    VOID
)
/*++

Routine Description:

    Execute the static file handler

Return Value:

    CONTEXT_STATUS_PENDING or CONTEXT_STATUS_CONTINUE

--*/
{
    W3_CONTEXT *pW3Context = QueryW3Context();
    DBG_ASSERT( pW3Context != NULL );

    HRESULT             hr = NO_ERROR;
    W3_RESPONSE *       pResponse = pW3Context->QueryResponse();
    W3_REQUEST *        pRequest = pW3Context->QueryRequest();
    W3_METADATA *       pMetaData;
    URL_CONTEXT *       pUrlContext;
    W3_URL_INFO *       pUrlInfo;
    W3_FILE_INFO *      pOpenFile = NULL;
    DWORD               dwFilePerms;
    CACHE_USER          fileUser;
    BOOL                fHandledSync;
    BOOL                fAllowNoBuffering = TRUE;

    //
    // Get the metadata, in particular the cached W3_URL_INFO off which we
    // we attempt to open the file
    //

    pUrlContext = pW3Context->QueryUrlContext();
    DBG_ASSERT( pUrlContext != NULL );

    pUrlInfo = pUrlContext->QueryUrlInfo();
    DBG_ASSERT( pUrlInfo != NULL );

    pMetaData = pUrlContext->QueryMetaData();
    DBG_ASSERT( pMetaData != NULL );

    if ( ETW_IS_TRACE_ON(ETW_LEVEL_CP) )
    {
        HTTP_REQUEST_ID RequestId = pRequest->QueryRequestId();

        g_pEtwTracer->EtwTraceEvent( &IISEventGuid,
                                     ETW_TYPE_IIS_STATIC_FILE,
                                     &RequestId,
                                     sizeof(HTTP_REQUEST_ID),
                                     pUrlContext->QueryPhysicalPath()->QueryStr(),
                                     pUrlContext->QueryPhysicalPath()->QueryCB(),
                                     NULL,
                                     0 );
    }

    //
    // Check web permissions.
    // Will fail, if no VROOT_MASK_READ, or if we forbid remote access and
    // the request is remote
    //

    dwFilePerms = pMetaData->QueryAccessPerms();

    if ( !IS_ACCESS_ALLOWED(pRequest, dwFilePerms, READ) )
    {
        pResponse->SetStatus( HttpStatusForbidden,
                              Http403ReadAccessDenied );
        pW3Context->SetErrorStatus( HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) );
        goto Failure;
    }

    //
    // Figure out if it is ok to open the file with buffering off, i.e.
    // are we going to just hand over the handle to http.sys or read it
    // ourselves
    //
    if ( !pW3Context->QueryDoneWithCompression() &&
         (pMetaData->QueryDoDynamicCompression() ||
          pMetaData->QueryDoStaticCompression()) )
    {
        fAllowNoBuffering = FALSE;
    }
    if ( pW3Context->IsNotificationNeeded( SF_NOTIFY_SEND_RAW_DATA ) )
    {
        fAllowNoBuffering = FALSE;
    }
    if ( pUrlInfo->QueryGateway() == GATEWAY_MAP )
    {
        fAllowNoBuffering = FALSE;
    }

    //
    // Now try to open the file
    //

    pW3Context->QueryFileCacheUser( &fileUser );

    m_AsyncContext.pfnCallback = FileOpenCallback;

    hr = pUrlContext->OpenFile( &fileUser,
                                &pOpenFile,
                                &m_AsyncContext,
                                &fHandledSync,
                                fAllowNoBuffering );

    if (FAILED(hr) || fHandledSync)
    {
        return RealDoWork(pOpenFile, hr);
    }

    return CONTEXT_STATUS_PENDING;

Failure:

    hr = pW3Context->SendResponse( W3_FLAG_ASYNC );
    if ( FAILED( hr ) )
    {
        pW3Context->SetErrorStatus( hr );
        pResponse->SetStatus( HttpStatusServerError );
        return CONTEXT_STATUS_CONTINUE;
    }

    return CONTEXT_STATUS_PENDING;
}

// static
VOID
W3_STATIC_FILE_HANDLER::FileOpenCallback(PVOID   pContext,
                                         HRESULT hr)
{
    W3_STATIC_FILE_HANDLER *pHandler = CONTAINING_RECORD(pContext,
                                                W3_STATIC_FILE_HANDLER,
                                                m_AsyncContext);

    W3_FILE_INFO *pFileInfo = pHandler->m_AsyncContext.pFileInfo;
    pHandler->m_AsyncContext.pFileInfo = NULL;

    if (pHandler->RealDoWork(pFileInfo, hr) == CONTEXT_STATUS_CONTINUE)
    {
        POST_MAIN_COMPLETION( pHandler->QueryW3Context()->QueryMainContext() );
    }
}

CONTEXT_STATUS
W3_STATIC_FILE_HANDLER::RealDoWork(
    W3_FILE_INFO *pOpenFile,
    HRESULT hr
)
/*++

Routine Description:

    Execute the static file handler

Return Value:

    CONTEXT_STATUS_PENDING or CONTEXT_STATUS_CONTINUE

--*/
{
    W3_CONTEXT *pW3Context = QueryW3Context();
    DBG_ASSERT( pW3Context != NULL );

    W3_RESPONSE *       pResponse = pW3Context->QueryResponse();
    W3_METADATA *       pMetaData;
    URL_CONTEXT *       pUrlContext;
    BOOL                fAsyncPending = FALSE;

    //
    // Get the metadata, in particular the cached W3_URL_INFO off which we
    // we attempt to open the file
    //

    pUrlContext = pW3Context->QueryUrlContext();
    DBG_ASSERT( pUrlContext != NULL );

    pMetaData = pUrlContext->QueryMetaData();
    DBG_ASSERT( pMetaData != NULL );

    if (FAILED(hr))
    {
        DWORD           dwError;

        IF_DEBUG( STATICFILE )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Error opening file %ws.  hr = %x\n",
                        pUrlContext->QueryPhysicalPath()->QueryStr(),
                        hr ));
        }

        pW3Context->SetErrorStatus( hr );
        dwError = WIN32_FROM_HRESULT( hr );
        switch( dwError )
        {
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
        case ERROR_INVALID_NAME:
            hr = NO_ERROR;
            pResponse->SetStatus( HttpStatusNotFound );
            break;

        case ERROR_LOGON_FAILURE:
        case ERROR_ACCOUNT_DISABLED:
        case ERROR_ACCESS_DENIED:
            hr = NO_ERROR;
            pResponse->SetStatus( HttpStatusUnauthorized,
                                  Http401Resource );
            break;

        case ERROR_INSUFFICIENT_BUFFER:
            hr = NO_ERROR;
            pResponse->SetStatus( HttpStatusUrlTooLong );
            break;
        }

        goto Failure;
    }

    DBG_ASSERT( pOpenFile != NULL );

    //
    // Is the file hidden?  If so, don't serve it out for legacy reasons
    //

    if ( pOpenFile->QueryAttributes() & FILE_ATTRIBUTE_HIDDEN )
    {
        pOpenFile->DereferenceCacheEntry();
        pResponse->SetStatus( HttpStatusNotFound );
        goto Failure;
    }

    //
    // Is this a file or directory?
    //

    if ( pOpenFile->QueryAttributes() & FILE_ATTRIBUTE_DIRECTORY )
    {
        //
        // At this point, we will do one of the following:
        // a) Send a directory listing
        // b) Send a default load file
        // c) Send a 302 (to redirect to a slash suffixed URL)
        // d) Send a 403 (forbidden)
        //

        pOpenFile->DereferenceCacheEntry();
        pOpenFile = NULL;

        hr = DirectoryDoWork( pW3Context,
                              &fAsyncPending );
        if ( fAsyncPending )
        {
            return CONTEXT_STATUS_PENDING;
        }

        //
        // If access denied, then send the response now
        //

        if ( WIN32_FROM_HRESULT( hr ) == ERROR_ACCESS_DENIED )
        {
            pW3Context->SetErrorStatus( hr );
            pW3Context->QueryResponse()->SetStatus( HttpStatusUnauthorized,
                                                    Http401Resource );

            hr = NO_ERROR;
        }
    }
    else
    {
        //
        // This is just a regular file.  Serve it out
        //

        //
        // Save away the file now.  We will clean it up at the end of the
        // request when this current context is cleaned up
        //

        m_pOpenFile = pOpenFile;

        hr = FileDoWork( pW3Context,
                         pOpenFile );
    }

    //
    // If there was an error here, then generate a 500.  If successful, it
    // is assumed that the response status is already set
    //

Failure:

    if ( FAILED( hr ) )
    {
        pResponse->Clear();
        pW3Context->SetErrorStatus( hr );
        pResponse->SetStatus( HttpStatusServerError );
    }

    hr = pW3Context->SendResponse( W3_FLAG_ASYNC );
    if ( FAILED( hr ) )
    {
        pW3Context->SetErrorStatus( hr );
        pResponse->SetStatus( HttpStatusServerError );
        return CONTEXT_STATUS_CONTINUE;
    }

    return CONTEXT_STATUS_PENDING;
}

HRESULT
W3_STATIC_FILE_HANDLER::SetupUlCachedResponse(
    W3_CONTEXT *                pW3Context,
    HTTP_CACHE_POLICY          *pCachePolicy
)
/*++

Routine Description:

    Setup a response to be cached by UL.  In this case we will muck with
    the cached file object to
    a) Remove its TTL
    b) Associate the current request's URL with the file object so that when
       the file object goes away, we will be called with enough info to
       flush the appropriate UL cache entry

Arguments:

    pW3Context - Context
    pCachePolicy - Cache-policy to fill in if caching desired

Return Value:

    HRESULT

--*/
{
    STACK_STRU(             strFlushUrl, MAX_PATH );
    STACK_STRU(             strQueryString, MAX_PATH );
    HRESULT                 hr;

    if ( pW3Context == NULL )
    {
        DBG_ASSERT( FALSE );
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        goto Exit;
    }

    if ( m_pOpenFile == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
        goto Exit;
    }

    //
    // If the file wasn't cached, then don't use UL cache
    //

    if ( m_pOpenFile->QueryUlCacheAllowed() == FALSE )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
        goto Exit;
    }
    
    //
    // If we are not doing dirmon for UNC, we cannot let UL cache UNC
    // responses
    //
    if ( !g_pW3Server->QueryFileCache()->QueryDoDirmonForUnc() &&
         ISUNC( m_pOpenFile->QueryPhysicalPath() ))
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
        goto Exit;
    }

    //
    // If this file was not accessed anonymously, then we need to do access
    // check anonymously before putting into cache
    //

    if ( pW3Context->QueryUserContext()->QueryAuthType() != MD_AUTH_ANONYMOUS )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
        goto Exit;
    }

    //
    // If there is a query string in the request, don't cache the file
    // lest we have multiple identical entries in the response cache
    //
 
    hr = pW3Context->QueryRequest()->GetQueryString( &strQueryString );
    if ( FAILED( hr ) )
    {
        goto Exit;
    }
    
    if ( !strQueryString.IsEmpty() )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
        goto Exit;
    }

    //
    // Get the exact URL used to flush UL cache
    //

    hr = pW3Context->QueryMainContext()->QueryRequest()->GetOriginalFullUrl(
                                                            &strFlushUrl );
    if ( FAILED( hr ) )
    {
        goto Exit;
    }

    //
    // Setup UL cache response token
    //

    DBG_ASSERT( g_pW3Server->QueryUlCache() != NULL );

    hr = g_pW3Server->QueryUlCache()->SetupUlCachedResponse(
                                        pW3Context,
                                        strFlushUrl,
                                        TRUE,
                                        pW3Context->QueryUrlContext()->QueryPhysicalPath());
    if ( SUCCEEDED( hr ) )
    {
        pCachePolicy->Policy = HttpCachePolicyUserInvalidates;
    }

Exit:

    return hr;
}

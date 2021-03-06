/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module Name :
     w3metadata.cxx

   Abstract:
     Code to read metadata and generate W3_METADATA objects
 
   Author:
     Bilal Alam (balam)             23-Jan-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"
#include "redirect.hxx"

ALLOC_CACHE_HANDLER * W3_METADATA::sm_pachW3MetaData;

W3_METADATA::W3_METADATA( OBJECT_CACHE * pObjectCache )
    : CACHE_ENTRY            ( pObjectCache ),
      _dwAccessPerm          ( MD_ACCESS_READ ),
      _dwSslAccessPerm       ( 0 ),
      _cbIpAccessCheck       ( 0 ),
      _fUseAnonSubAuth       ( FALSE ),
      _dwLogonMethod         ( LOGON32_LOGON_NETWORK_CLEARTEXT ),
      _dwVrLevel             ( 0 ),
      _dwVrLen               ( 0 ),
      _pRedirectBlob         ( NULL ),
      _dwDirBrowseFlags      ( MD_DIRBROW_LOADDEFAULT ),
      _dwAuthentication      ( 0 ),
      _dwAuthPersistence     ( 0 ),
      _strVrUserName         ( _rgVrUserName, sizeof( _rgVrUserName ) ),
      _strVrPasswd           ( _rgVrPasswd, sizeof( _rgVrPasswd ) ),
      _strUserName           ( _rgUserName, sizeof( _rgUserName ) ),
      _strPasswd             ( _rgPasswd, sizeof( _rgPasswd ) ),
      _strDomainName         ( _rgDomainName, sizeof( _rgDomainName ) ),
      _strRealm              ( _rgRealm, sizeof( _rgRealm ) ),
      _mstrAuthProviders     (),
      _pctVrToken            ( NULL ),
      _pctAnonymousToken     ( NULL ),
      _fCreateProcessAsUser  ( TRUE ),
      _fCreateProcessNewConsole ( FALSE ),
      _fDoStaticCompression  ( HTTP_COMPRESSION::QueryDoStaticCompression() ),
      _fDoDynamicCompression ( HTTP_COMPRESSION::QueryDoDynamicCompression() ),
      _dwCGIScriptTimeout    ( DEFAULT_SCRIPT_TIMEOUT ),
      _ScriptMap             (),
      _pMimeMap              ( NULL ),
      _fSSIExecDisabled      ( FALSE ),
      _fDontLog              ( FALSE ),
      _fFooterEnabled        ( FALSE ),
      _dwExpireMode          ( EXPIRE_MODE_NONE ),
      _fHaveNoCache          ( FALSE ),
      _fHaveMaxAge           ( FALSE ),
      _fDoReverseDNS         ( FALSE ),
      _cbEntityReadAhead     ( DEFAULT_ENTITY_READ_AHEAD ),
      _dwMaxRequestEntityAllowed    ( DEFAULT_MAX_REQUEST_ENTITY_ALLOWED ),
      _fNoCache              ( FALSE ),
      _dwAppIsolated         ( 0 ),
      _dwAppOopRecoverLimit  ( 0 ),
      _fKeepAliveEnabled     ( TRUE ),
      _cGetAllRecords        ( NULL ),
      _pGetAllBuffer         ( NULL ),
      _fAppPoolMatches       ( FALSE ),
      _dwRequireMapping      ( MD_PASSPORT_TRY_MAPPING ),
      _fUNCUserInvalid       ( FALSE ),
      _cbMatchingUrlA        ( 0 ),
      _cbMatchingPathA       ( 0 )
{
    //
    // Hmmm, since most of these values aren't getting initialized, if
    // somebody went and deleted all the metadata items from the tree, 
    // then bad things could happen.  We should initialize with defaults 
    // things that might screw us
    //
    
    _dirmonConfig.hToken = NULL;
    _dirmonConfig.pszDirPath = NULL;
}

W3_METADATA::~W3_METADATA()
{
    if ( _pctVrToken != NULL )
    {
        _pctVrToken->DereferenceCacheEntry();
        _pctVrToken = NULL;
    }   
    
    if ( _pctAnonymousToken != NULL )
    {
        _pctAnonymousToken->DereferenceCacheEntry();
        _pctAnonymousToken = NULL;
    }

    if ( _pMimeMap != NULL )
    {
        delete _pMimeMap;
        _pMimeMap = NULL;
    }

    if ( _pRedirectBlob != NULL )
    {
        delete _pRedirectBlob;
        _pRedirectBlob = NULL;
    }
    
    if ( _pGetAllBuffer != NULL )
    {
        delete _pGetAllBuffer;
        _pGetAllBuffer = NULL;
    }

    DBG_ASSERT(CheckSignature());
}

//static
HRESULT
W3_METADATA::Initialize(
    VOID
)
/*++

Routine Description:

    Initialize metadata lookaside

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    ALLOC_CACHE_CONFIGURATION   acConfig;
    HRESULT                     hr;    

    //
    // Initialize allocation lookaside
    //    
    
    acConfig.nConcurrency = 1;
    acConfig.nThreshold   = 100;
    acConfig.cbSize       = sizeof( W3_METADATA );

    DBG_ASSERT( sm_pachW3MetaData == NULL );
    
    sm_pachW3MetaData = new ALLOC_CACHE_HANDLER( "W3_METADATA",  
                                                  &acConfig );

    if ( sm_pachW3MetaData == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );

        DBGPRINTF(( DBG_CONTEXT,
                   "Error initializing sm_pachW3MetaData. hr = 0x%x\n",
                   hr ));

        return hr;
    }
    
    DATA_SET_CACHE::Initialize();
    
    return NO_ERROR;
}

//static
VOID
W3_METADATA::Terminate(
    VOID
)
{
    DATA_SET_CACHE::Terminate();
    
    if ( sm_pachW3MetaData != NULL )
    {
        delete sm_pachW3MetaData;
        sm_pachW3MetaData = NULL;
    }
}

//
//  Private constants.
//

#define DEFAULT_MD_RECORDS          40
#define DEFAULT_RECORD_SIZE         50

# define DEF_MD_REC_SIZE   ((1 + DEFAULT_MD_RECORDS) * \
                            (sizeof(METADATA_GETALL_RECORD) + DEFAULT_RECORD_SIZE))

HRESULT
W3_METADATA::ReadMetaData(
    const STRU & strMetabasePath,
    const STRU & strURL
)
/*++

Routine Description:

    Reads the metabase (directly) to get the metadata for the given URL

Arguments:

    strMetabasePath - The preceding service/instance goo (like "LM/W3SVC/1/ROOT" )
    strURL - URL in question
    
Return Value:

    HRESULT

--*/
{
    PMETADATA_GETALL_RECORD    pMDRecord;
    DWORD               dwNumMDRecords;
    DWORD               dwNumWamRecords;
    BYTE                tmpBuffer[ DEF_MD_REC_SIZE];
    BYTE                tmpWamBuffer[ DEF_MD_REC_SIZE];
    BUFFER              TempBuff( tmpBuffer, DEF_MD_REC_SIZE);
    BUFFER              WamBuff( tmpWamBuffer, DEF_MD_REC_SIZE);
    DWORD               i;
    DWORD               dwDataSetNumber;
    DWORD               dwWamDataSetNumber;
    WCHAR               ch;
    LPWSTR              pszInVr;
    LPWSTR              pszMinInVr;
    LPWSTR              pszURL;
    DWORD               dwNeed;
    DWORD               dwL;
    DWORD               dwVRLen;
    BYTE                tmpPrivateBuffer[ 20 ];
    BUFFER              PrivateBuffer( tmpPrivateBuffer, 20 );
    DWORD               dwPrivateBufferUsed;
    MB                  mb( g_pW3Server->QueryMDObject() );
    MB *                pmb = &mb;
    HRESULT             hr = NO_ERROR;
    WCHAR *             pszStart = NULL;
    DWORD               cchLength = 0;
    STACK_STRU(         strAppPoolId, 256 );
    STACK_STRU(         strMatchingUrl, 64 );
    WCHAR *             pszProcessAppPoolId;
    BOOL                fRet;

    //
    // We lie about modifying the input path
    //
    pszURL = (LPWSTR)strURL.QueryStr();
    DBG_ASSERT( pszURL != NULL );

    DBG_ASSERT( TempBuff.QuerySize() >=
                (DEFAULT_MD_RECORDS *
                 (sizeof(METADATA_GETALL_RECORD) + DEFAULT_RECORD_SIZE))
                );

    DBG_ASSERT( WamBuff.QuerySize() >=
                (DEFAULT_MD_RECORDS *
                 (sizeof(METADATA_GETALL_RECORD) + DEFAULT_RECORD_SIZE))
                );

    //
    // Read the metabase
    //

    if ( !pmb->Open( strMetabasePath.QueryStr() ))
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Failure;
    }

    //
    // Get the UT_FILE info
    //

    if ( !pmb->GetAll( pszURL,
                       METADATA_INHERIT | METADATA_PARTIAL_PATH,
                       IIS_MD_UT_FILE,
                       &TempBuff,
                       &dwNumMDRecords,
                       &dwDataSetNumber ))
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Failure;
    }

    //
    // Get the UT_WAM info
    //

    if ( !pmb->GetAll( pszURL,
                       METADATA_INHERIT | METADATA_PARTIAL_PATH,
                       IIS_MD_UT_WAM,
                       &WamBuff,
                       &dwNumWamRecords,
                       &dwWamDataSetNumber ))
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Failure;
    }
    
    //
    // Aarrggh.  MD_APP_APPPOOL_ID is UT_SERVER.  We want to get this property
    // so that we can validate that we don't execute AppPool A's stuff in a
    // process running as AppPool B (A != B)
    //
    // Note that it is OK if this call fails.  WAS by default uses 
    // DefaultAppPool as apppool
    //
    
    pmb->GetStr( pszURL,
                 MD_APP_APPPOOL_ID,
                 IIS_MD_UT_SERVER,
                 &strAppPoolId,
                 METADATA_INHERIT | METADATA_PARTIAL_PATH );

    pszProcessAppPoolId = (WCHAR*) UlAtqGetContextProperty( NULL,
                                                            ULATQ_PROPERTY_APP_POOL_ID ); 
    DBG_ASSERT( pszProcessAppPoolId != NULL );
   
    //
    // For efficiency sake, we'll store whether the AppPools match.  That way, we don't have
    // to repeat the determination per request.
    //
    
    if ( strAppPoolId.IsEmpty() )
    {
        _fAppPoolMatches = _wcsicmp( pszProcessAppPoolId, L"DefaultAppPool" ) == 0;
    }
    else
    {
        _fAppPoolMatches = _wcsicmp( pszProcessAppPoolId,
                                     strAppPoolId.QueryStr() ) == 0;
    }            

    //
    // Both sets of data better have the same data set number
    //

    DBG_ASSERT( dwDataSetNumber == dwWamDataSetNumber );

    //
    // Set the data set number, so that this object is metadata cachable
    //
    
    _cacheKey.SetDataSetNumber( dwDataSetNumber );
   
    //
    // Grok the buffer containing all the records
    //

    pMDRecord = (PMETADATA_GETALL_RECORD)TempBuff.QueryPtr();

    i = 0;

    //
    // Check from where we got VR_PATH
    //

    pszMinInVr = pszURL ;
    if ( *pszURL )
    {
        for ( pszInVr = pszMinInVr + wcslen(pszMinInVr) ;; )
        {
            ch = *pszInVr;
            *pszInVr = L'\0';
            dwNeed = 0;
            if ( !pmb->GetString( pszURL, 
                                  MD_VR_PATH, 
                                  IIS_MD_UT_FILE, 
                                  NULL, 
                                  &dwNeed, 
                                  0 ) &&
                 GetLastError() == ERROR_INSUFFICIENT_BUFFER )
            {
                *pszInVr = ch;
                // VR_PATH was defined at this level !

                break;
            }
            *pszInVr = ch;

            if ( ch )
            {
                if ( pszInVr > pszMinInVr )
                {
                    pszInVr--;
                }
                else
                {
                    //
                    // VR_PATH was defined above Instance vroot
                    // or not at all. If defined above, then the reference
                    // path is empty, so we can claim we found it.
                    // if not defined, then this will be catch later.
                    //

                    break;
                }
            }

            // scan for previous delimiter

            while ( *pszInVr != L'/' && *pszInVr != L'\\' )
            {
                if ( pszInVr > pszMinInVr )
                {
                    pszInVr--;
                }
                else
                {
                    //
                    // VR_PATH was defined above Instance vroot
                    // or not at all. If defined above, then the reference
                    // path is empty, so we can claim we found it.
                    // if not defined, then this will be catch later.
                    //

                    break;
                }
            }
        }

        dwVRLen = DIFF(pszInVr - pszMinInVr);
    }
    else
    {
        dwVRLen = 0;
        pszInVr = pszMinInVr;
    }

    // Close this now to minimize lock contention.
    DBG_REQUIRE(pmb->Close());

    for ( dwL = 0 ; pszMinInVr < pszInVr - 1 ; pszMinInVr++ )
    {
        if ( *pszMinInVr == L'/' || *pszMinInVr == L'\\' )
        {
            ++dwL;
        }
    }
    
    //
    // Store away an ANSI copy of the matching URL
    //

    hr = strMatchingUrl.Copy( pszURL );

    if ( FAILED( hr ) )
    {
        goto Failure;
    }

    DBG_ASSERT( strMatchingUrl.QueryCCH() >= dwVRLen );

    strMatchingUrl.SetLen( dwVRLen );

    hr = _strMatchingUrlA.CopyW( strMatchingUrl.QueryStr() );

    if ( FAILED( hr ) )
    {
        goto Failure;
    }

    _cbMatchingUrlA = strlen( _strMatchingUrlA.QueryStr() );

    //
    // Now walk through the array of returned metadata objects and format
    // each one into our predigested form.
    //
    
    _dwVrLevel = dwL;
    _dwVrLen = dwVRLen;
    
    dwPrivateBufferUsed = 0;

    for ( ; i < dwNumMDRecords; i++, pMDRecord++ ) {

        PVOID       pDataPointer;

        pDataPointer = (PVOID) ((PCHAR)TempBuff.QueryPtr() +
                                    pMDRecord->dwMDDataOffset);

        DBG_ASSERT(pMDRecord->dwMDDataTag == 0);

        switch ( pMDRecord->dwMDIdentifier )
        {

        case MD_ALLOW_KEEPALIVES:
            if (pMDRecord->dwMDDataType != DWORD_METADATA)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                goto Failure;
            }

            _fKeepAliveEnabled = *(DWORD *)pDataPointer;
            break;

        case MD_FOOTER_DOCUMENT:
            if (pMDRecord->dwMDDataType != STRING_METADATA)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                goto Failure;
            }

            if (*(WCHAR *)pDataPointer != L'\0')
            {
                if (FAILED(hr = ReadCustomFooter((WCHAR *)pDataPointer)))
                {
                    goto Failure;
                }
            }

            break;

        case MD_FOOTER_ENABLED:
            if (pMDRecord->dwMDDataType != DWORD_METADATA)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                goto Failure;
            }

            _fFooterEnabled = !!*((DWORD *) pDataPointer );

            if ( _fFooterEnabled )
            {
                //
                // If we have footers for a static file, we cannot do static
                // compression on it
                //
                
                _fDoStaticCompression = FALSE;
            }
            break;

        case MD_HTTP_EXPIRES:
            if (pMDRecord->dwMDDataType != STRING_METADATA)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                goto Failure;
            }

            if (FAILED(hr = SetExpire((WCHAR *)pDataPointer)))
            {
                goto Failure;
            }
            break;

        case MD_CC_NO_CACHE:
            if (pMDRecord->dwMDDataType != DWORD_METADATA)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                goto Failure;
            }

            if (*(BOOL *)pDataPointer)
            {
                _fHaveNoCache = TRUE;
            }
            break;

        case MD_CC_MAX_AGE:
            if (pMDRecord->dwMDDataType != DWORD_METADATA)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                goto Failure;
            }

            _dwMaxAge = *(DWORD *)pDataPointer;
            _fHaveMaxAge = TRUE;
            break;

        case MD_CC_OTHER:
            if (pMDRecord->dwMDDataType != STRING_METADATA)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                goto Failure;
            }

            if (FAILED(hr = _strCacheControlHeader.CopyWTruncate((WCHAR *)pDataPointer)))
            {
                goto Failure;
            }
            break;

        case MD_HTTP_REDIRECT:
        {
            if (pMDRecord->dwMDDataType != STRING_METADATA)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                goto Failure;
            }

            STACK_STRU( strRealSource, MAX_PATH );
            STACK_STRU( strDestination, MAX_PATH );

            if (FAILED(hr = strDestination.Copy((WCHAR *)pDataPointer)) ||
                FAILED(hr = GetTrueRedirectionSource(
                                pszURL,
                                strMetabasePath.QueryStr(),
                                &strRealSource)) ||
                FAILED(hr = SetRedirectionBlob(strRealSource,
                                               strDestination)))
            {
                goto Failure;
            }

            break;
        }

        case MD_DONT_LOG:
            if (pMDRecord->dwMDDataType != DWORD_METADATA) 
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                goto Failure;
            }

            _fDontLog = *(BOOL *)pDataPointer;
            break;

        case MD_CREATE_PROCESS_AS_USER:
            if (pMDRecord->dwMDDataType != DWORD_METADATA) 
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                goto Failure;
            }

            _fCreateProcessAsUser = *(BOOL *)pDataPointer;
            break;

        case MD_CREATE_PROC_NEW_CONSOLE:
            if (pMDRecord->dwMDDataType != DWORD_METADATA) 
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                goto Failure;
            }

            _fCreateProcessNewConsole = *(BOOL *)pDataPointer;
            break;

        case MD_SCRIPT_TIMEOUT:
            if (pMDRecord->dwMDDataType != DWORD_METADATA) 
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                goto Failure;
            }

            _dwCGIScriptTimeout = *(DWORD *)pDataPointer;
            break;

        case MD_MIME_MAP:
            if (pMDRecord->dwMDDataType != MULTISZ_METADATA)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                goto Failure;
            }

            if (*(WCHAR *)pDataPointer)
            {
                _pMimeMap = new MIME_MAP((WCHAR *)pDataPointer);
                if (_pMimeMap == NULL)
                {
                    hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
                    goto Failure;
                }
            }
            break;

        case MD_HC_DO_NAMESPACE_DYNAMIC_COMPRESSION:
            if (pMDRecord->dwMDDataType != DWORD_METADATA)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                goto Failure;
            }

            _fDoDynamicCompression = *(BOOL *)pDataPointer;
            break;

        case MD_HC_DO_NAMESPACE_STATIC_COMPRESSION:
            if (pMDRecord->dwMDDataType != DWORD_METADATA)
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                goto Failure;
            }

            _fDoStaticCompression = *(BOOL *)pDataPointer;
            break;

        case MD_ANONYMOUS_USER_NAME:
            if ( pMDRecord->dwMDDataType != STRING_METADATA ) 
            {
                hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
                goto Failure;
            }
            
            if ( FAILED( hr = _strUserName.Copy(
                                   (WCHAR *)pDataPointer) ) )
            {
                goto Failure;
            }

            break;

        case MD_ANONYMOUS_PWD:
            if ( pMDRecord->dwMDDataType != STRING_METADATA ) 
            {
                hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
                goto Failure;
            }
            
            if ( FAILED( hr = _strPasswd.Copy(
                                   (WCHAR *)pDataPointer) ) )
            {
                goto Failure;
            }
            break;

        case MD_ANONYMOUS_USE_SUBAUTH:
            if (pMDRecord->dwMDDataType != DWORD_METADATA) 
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                goto Failure;
            }

            _fUseAnonSubAuth = *(BOOL *)pDataPointer;
            break;
        
        case MD_DEFAULT_LOGON_DOMAIN:
            if ( pMDRecord->dwMDDataType != STRING_METADATA ) 
            {
                hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
                goto Failure;
            }
            
            if ( FAILED( hr = _strDomainName.Copy(
                                   (WCHAR *)pDataPointer) ) )
            {
                goto Failure;
            }
            break;

        case MD_HTTP_PICS:
        case MD_HTTP_CUSTOM:

            if ( pMDRecord->dwMDDataType != MULTISZ_METADATA ) 
            {
                hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
                goto Failure;
            }

            // 
            // Copy all the specified headers into our header buffer
            //
            
            pszStart = (WCHAR*) pDataPointer;
            while ( *pszStart != L'\0' )
            {
                cchLength = wcslen( pszStart );
                
                hr = _strCustomHeaders.AppendW( pszStart );
                if ( FAILED( hr ) )
                {
                    goto Failure;
                }
                
                hr = _strCustomHeaders.Append( "\r\n", 2 );
                if ( FAILED( hr ) )
                {
                    goto Failure;
                }
                
                pszStart += ( cchLength + 1 );
            }
            break;

        case MD_LOGON_METHOD:
            if ( pMDRecord->dwMDDataType != DWORD_METADATA ) 
            {
                hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
                goto Failure;
            }
            
            //
            // The MD_LOGON_METHOD values in the metabase don't match 
            // the NT logon values, so we'll convert them
            //
            switch ( *((DWORD *) pDataPointer ) )
            {
            case MD_LOGON_BATCH:
                _dwLogonMethod = LOGON32_LOGON_BATCH;
                break;

            case MD_LOGON_INTERACTIVE:
                _dwLogonMethod = LOGON32_LOGON_INTERACTIVE;
                break;
            
            case MD_LOGON_NETWORK:
                _dwLogonMethod = LOGON32_LOGON_NETWORK;
                break;
                
            case MD_LOGON_NETWORK_CLEARTEXT:
                _dwLogonMethod = LOGON32_LOGON_NETWORK_CLEARTEXT;
                break;

            default:
                break;
            }

            break;

        case MD_AUTHORIZATION:
            if( pMDRecord->dwMDDataType != DWORD_METADATA ) 
            {
                hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
                goto Failure;
            }
            
            _dwAuthentication = *((DWORD *) pDataPointer );            
            break;

        case MD_REALM:
            if( pMDRecord->dwMDDataType != STRING_METADATA )
            {
                hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
                goto Failure;
            }

            if( FAILED( hr = _strRealm.Copy( ( WCHAR* )pDataPointer ) ) )
            {
                goto Failure;
            }

            break;

        case MD_NTAUTHENTICATION_PROVIDERS:
            if( pMDRecord->dwMDDataType != STRING_METADATA )
            {
                hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
                goto Failure;
            }

            hr = BuildProviderList( ( WCHAR* )pDataPointer );
            if ( FAILED( hr ) )
            {
                goto Failure;
            }

            break;

        case MD_AUTHORIZATION_PERSISTENCE:
            if( pMDRecord->dwMDDataType != DWORD_METADATA )
            {
                hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
                goto Failure;
            }
            
            _dwAuthPersistence = *((DWORD *) pDataPointer );
            break;

        case MD_IP_SEC:
            if( pMDRecord->dwMDDataType != BINARY_METADATA )
            {
                hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
                goto Failure;
            }

            if ( pMDRecord->dwMDDataLen )
            { 
                hr = SetIpAccessCheck( pDataPointer,
                                       pMDRecord->dwMDDataLen );
                if ( FAILED( hr ) )
                {
                    goto Failure;
                }
            }

            break;

        case MD_ACCESS_PERM:
            if ( pMDRecord->dwMDDataType != DWORD_METADATA ) 
            {
                hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
                goto Failure;
            }
            
            _dwAccessPerm = *((DWORD*) pDataPointer);
            break;

        case MD_SSL_ACCESS_PERM:
            if ( pMDRecord->dwMDDataType != DWORD_METADATA ) 
            {
                hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
                goto Failure;
            }
            
            _dwSslAccessPerm = *((DWORD*)pDataPointer) & MD_SSL_ACCESS_MASK;
            break;

        case MD_VR_PATH:
            if ( pMDRecord->dwMDDataType != STRING_METADATA ) 
            {
                hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
                goto Failure;
            }
            
            if ( FAILED( hr = _strVrPath.Copy((WCHAR *)pDataPointer) ) )
            {
                goto Failure;
            }

            if ( FAILED( hr = _strMatchingPathA.CopyW((WCHAR *)pDataPointer) ) )
            {
                goto Failure;
            }

            _cbMatchingPathA = strlen( _strMatchingPathA.QueryStr() );
            break;

        case MD_APP_ROOT:
            
            if ( pMDRecord->dwMDDataType != STRING_METADATA ) 
            {
                hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
                goto Failure;
            }
            
            if ( FAILED( hr = _strAppMetaPath.Copy((WCHAR *)pDataPointer) ) )
            {
                goto Failure;
            }

            break;

        case MD_VR_USERNAME:
            if ( pMDRecord->dwMDDataType != STRING_METADATA ) 
            {
                hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
                goto Failure;
            }
            
            if ( FAILED( hr = _strVrUserName.Copy(
                                   (WCHAR *)pDataPointer) ) )
            {
                goto Failure;
            }

            break;

        case MD_VR_PASSWORD:
            if ( pMDRecord->dwMDDataType != STRING_METADATA ) 
            {
                hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
                goto Failure;
            }
            
            if ( FAILED( hr = _strVrPasswd.Copy(
                                   (WCHAR *)pDataPointer) ) )
            {
                goto Failure;
            }

            break;

        case MD_REDIRECT_HEADERS:
            if ( pMDRecord->dwMDDataType != MULTISZ_METADATA ) 
            {
                hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
                goto Failure;
            }
           
            // 
            // Copy all the specified headers into our header buffer
            //
            
            pszStart = (WCHAR*) pDataPointer;
            while ( *pszStart != L'\0' )
            {
                cchLength = wcslen( pszStart );
                
                hr = _strRedirectHeaders.AppendW( pszStart );
                if ( FAILED( hr ) )
                {
                    goto Failure;
                }
                
                hr = _strRedirectHeaders.Append( "\r\n", 2 );
                if ( FAILED( hr ) )
                {
                    goto Failure;
                }
                
                pszStart += ( cchLength + 1 );
            }
            break;
            
        case MD_DIRECTORY_BROWSING:
            if ( pMDRecord->dwMDDataType != DWORD_METADATA )
            {
                hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
                goto Failure;
            }
            _dwDirBrowseFlags = *((DWORD *) pDataPointer );
            break;
            
        case MD_DEFAULT_LOAD_FILE:
            if ( pMDRecord->dwMDDataType != STRING_METADATA )
            {
                hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
                goto Failure;
            }
            
            hr = _strDefaultLoad.Copy( (WCHAR*) pDataPointer );
            if ( FAILED( hr ) )
            {
                goto Failure;
            }
            break;
        
        case MD_SCRIPT_MAPS:
            if ( pMDRecord->dwMDDataType != MULTISZ_METADATA )
            {
                hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
                goto Failure;
            }

            hr = _ScriptMap.Initialize( (WCHAR *)pDataPointer );

            if( FAILED(hr) )
            {
                goto Failure;
            }
            break;
        
        case MD_CUSTOM_ERROR:
            if ( pMDRecord->dwMDDataType != MULTISZ_METADATA )
            {
                hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
                goto Failure;
            }
            
            //
            // An empty string means use hard-coded errors
            //
            
            if ( *((WCHAR*)pDataPointer) == L'\0' )
            {
                break; 
            }
            
            hr = _customErrorTable.BuildTable( (WCHAR*) pDataPointer );
            if ( FAILED( hr ) )
            {
                goto Failure;
            }
            break;

        case MD_SSI_EXEC_DISABLED:
            if ( pMDRecord->dwMDDataType != DWORD_METADATA )
            {
                hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
                goto Failure;
            }
            
            _fSSIExecDisabled = !!*( ( DWORD * ) pDataPointer );
            break;

        case MD_DO_REVERSE_DNS:
            if ( pMDRecord->dwMDDataType != DWORD_METADATA )
            {
                hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
                goto Failure;
            }
            
            _fDoReverseDNS = !!*((DWORD*)pDataPointer);
            break;
           
        case MD_UPLOAD_READAHEAD_SIZE:
            if ( pMDRecord->dwMDDataType != DWORD_METADATA )
            {
                hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
                goto Failure;
            }
            
            _cbEntityReadAhead = *(DWORD*)pDataPointer;
            break;

        case MD_MAX_REQUEST_ENTITY_ALLOWED:
            if ( pMDRecord->dwMDDataType != DWORD_METADATA )
            {
                hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
                goto Failure;
            }

            _dwMaxRequestEntityAllowed = *(DWORD*)pDataPointer;
            break;

        case MD_VR_NO_CACHE:
        
            if ( pMDRecord->dwMDDataType != DWORD_METADATA )
            {
                hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
                goto Failure;
            }
            
            _fNoCache = !!*((DWORD*)pDataPointer);
            break;
            
        case MD_PASSPORT_REQUIRE_AD_MAPPING:
        
            if ( pMDRecord->dwMDDataType != DWORD_METADATA )
            {
                hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
                goto Failure;
            }
            
            _dwRequireMapping = *((DWORD*)pDataPointer);
            break;
            
        default:
            // BUGBUGBUG
            DBG_ASSERT(CheckSignature());
            break;
        }
    }

    //
    // Walk through the WAM data
    //

    pMDRecord = (PMETADATA_GETALL_RECORD)WamBuff.QueryPtr();

    i = 0;

    for ( ; i < dwNumWamRecords; i++, pMDRecord++ ) {

        PVOID       pDataPointer;

        pDataPointer = (PVOID) ((PCHAR)WamBuff.QueryPtr() +
                                    pMDRecord->dwMDDataOffset);

        DBG_ASSERT(pMDRecord->dwMDDataTag == 0);

        switch ( pMDRecord->dwMDIdentifier )
        {
        case MD_APP_ISOLATED:

            if ( pMDRecord->dwMDDataType != DWORD_METADATA )
            {
                hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
                goto Failure;
            }

            _dwAppIsolated = *(DWORD*)pDataPointer;

            break;

        case MD_APP_WAM_CLSID:

            if ( pMDRecord->dwMDDataType != STRING_METADATA )
            {
                hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
                goto Failure;
            }

            hr = _strWamClsId.Copy( (WCHAR*)pDataPointer );

            break;

        case MD_APP_OOP_RECOVER_LIMIT:

            if ( pMDRecord->dwMDDataType != DWORD_METADATA )
            {
                hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
                goto Failure;
            }

            _dwAppOopRecoverLimit = *(DWORD*)pDataPointer;

            break;

        default:
            break;
        }
    }

    //
    // If no-cache, max-age or a dynamic expires directive is present, add
    // it to Cache-Control header
    //
    if (FAILED(hr = SetCacheControlHeader()))
    {
        goto Failure;
    }

    if ( _strVrPath.IsEmpty() )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "[ReadMetaData] Virtual Dir Path mapping not found\n" ));
                    
        hr = HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );    
        goto Failure;
    }

    //
    // If this is an UNC share, logon using associated credentials
    // keep a reference to this access token in the cache
    //
    
    if ( _strVrPath.QueryStr()[0] == L'\\' &&
         _strVrPath.QueryStr()[1] == L'\\' )
    {
        if ( _strVrUserName.QueryStr() != NULL && 
             _strVrPasswd.QueryStr()   != NULL &&
             _strVrUserName.QueryStr()[0] )
        {
            hr = CreateUNCVrToken( _strVrUserName.QueryStr(), 
                                   _strVrPasswd.QueryStr() );
            if( SUCCEEDED( hr ) )
            {
                hr = EncryptMemoryPassword( &_strVrPasswd );
                if( FAILED( hr ) )
                {
                    goto Failure;
                }
            }
            else
            {   
                //
                // Continue metadata creation, but remember the failure
                //
                
                hr = NO_ERROR;
                
                _fUNCUserInvalid = TRUE;
            }
        }
    }
    
    //
    // Setup the dirmon configuration
    //

    if ( _pctVrToken != NULL )
    {
        _dirmonConfig.hToken = _pctVrToken->QueryImpersonationToken();
    }
    else
    {
        _dirmonConfig.hToken = NULL;
    }   
    _dirmonConfig.pszDirPath = _strVrPath.QueryStr();
    
    //
    // Build a default provider list for SSPI auth if the 
    // NTAuthenticationProviders is not set in the metabase.
    //
    if( _mstrAuthProviders.IsEmpty() )
    {
        hr = BuildDefaultProviderList();
        if( FAILED( hr ) )
        {
            goto Failure;
        }
    }
    
    //
    // Get anonymous user token
    //
    
    hr = CreateAnonymousToken( _strUserName.QueryStr(),
                               _strPasswd.QueryStr() );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }

    hr = EncryptMemoryPassword( &_strPasswd );
    if( FAILED( hr ) )
    {
        goto Failure;
    }
    
    DBG_ASSERT( CheckSignature() );

    return S_OK;

Failure:
    return hr;
}

HRESULT
W3_METADATA::BuildPhysicalPath(
    STRU &          strUrl,
    STRU *          pstrPhysicalPath
)
/*++

Routine Description:

    From the current metadata, convert a URL to a physical path 
    (using the MD_VR_ROOT property and inheritance level calculated on read)

Arguments:

    strUrl - Virtual path
    pstrPhysicalPath - String filled with physical path of strURL
    
Return Value:

    BOOL

--*/
{
    LPWSTR              pszInVr;
    DWORD               dwL;
    WCHAR               ch;
    HRESULT             hr = S_OK;

    DBG_ASSERT(CheckSignature());

    //
    // Build physical path from VR_PATH & portion of URI not used to define VR_PATH
    //


    //
    // skip the URI components used to locate the virtual root
    //

    pszInVr = strUrl.QueryStr();
    dwL = _dwVrLevel;
    while ( dwL-- )
    {
        if ( *pszInVr )
        {
            DBG_ASSERT( *pszInVr == L'/' || *pszInVr == L'\\' );

            ++pszInVr;

            while ( (ch = *pszInVr) && ch != L'/' && ch !=L'\\' )
            {
                pszInVr++;
            }
        }
    }

    DBG_ASSERT( dwL == (DWORD)-1 );

    if ( FAILED(hr = pstrPhysicalPath->Copy( _strVrPath ) ) )
    {
        return hr;
    }

    //
    // Add a path delimiter char between virtual root mount point & significant part of URI
    //

    if ( pstrPhysicalPath->QueryCCH() )
    {
        ch = pstrPhysicalPath->QueryStr()[ pstrPhysicalPath->QueryCCH() - 1 ];
        if ( (ch == L'/' || ch == L'\\') && *pszInVr )
        {
            ++pszInVr;
        }
    }

    if ( FAILED(hr = pstrPhysicalPath->Append( pszInVr ) ) )
    {
        return hr;
    }

    //
    // insure physical path last char uses standard directory delimiter
    //

    FlipSlashes( pstrPhysicalPath->QueryStr() );

    return NO_ERROR;
}

HRESULT
W3_METADATA::BuildProviderList(
    IN WCHAR *                  pszProviders
    )
/*++

  Description:

    Builds a name array of SSPI Authentication providers

  Arguments:

    pszProviders - Comma separated list of providers

  Returns:

    HRESULT

--*/
{
    WCHAR *             pszCursor; 
    WCHAR *             pszEnd;
    BOOL                fFinished = FALSE;
    
    DBG_ASSERT(CheckSignature());

    if ( pszProviders == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    //
    // Parse comma delimited list of providers, removing white space
    //
    
    pszCursor = SkipWhite( pszProviders );
    for( ; ; )
    {
        pszEnd = wcschr( pszCursor, L',' );
        if ( pszEnd == NULL )
        {
            fFinished = TRUE;
            pszEnd = pszCursor + wcslen( pszCursor );
        }
        
        while ( pszEnd > pszCursor )
        {
            if ( !ISWHITEW( *pszEnd ) )
            {
                break;
            }
            pszEnd--;
        }
        
        *pszEnd = L'\0';
        
        if ( !_mstrAuthProviders.AppendW( pszCursor ) )
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }
        
        if ( fFinished )
        {
            break;
        }
        
        //
        // Advance to next provider
        //
        
        pszCursor = SkipWhite( pszEnd + 1 );
    }

    return NO_ERROR;
}    
     
HRESULT
W3_METADATA::BuildDefaultProviderList(
    VOID
    )
/*++

  Description:

    Builds a default name array of SSPI Authentication providers
    if NTAuthenticationProviders is not set in metabase

  Arguments:

    NONE

  Returns:

    HRESULT

--*/
{
    DBG_ASSERT( CheckSignature() );
    DBG_ASSERT( _mstrAuthProviders.IsEmpty() );

    if ( !_mstrAuthProviders.Append( "Negotiate" ) )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    if ( !_mstrAuthProviders.Append( "NTLM" ) )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    return NO_ERROR;
}    
     
BOOL 
W3_METADATA::CheckAuthProvider(
    IN const CHAR *        pszPkgName
    ) 
/*++

  Description:

    Check if we support the SSP package of name pszPkgName

  Arguments:

    pszPkgName - Name of the package we check against 

  Returns:

    TRUE if we support the package, FALSE if we don't.

--*/
{
    const CHAR *          pszProvider;
    
    DBG_ASSERT( CheckSignature() );
    
    if ( pszPkgName == NULL )
    {
        DBG_ASSERT( FALSE );
        return FALSE;
    }

    pszProvider = _mstrAuthProviders.First();
    while ( pszProvider != NULL )
    {
        if ( _stricmp( pszPkgName, pszProvider ) == 0 )
        {
            return TRUE;
        }
        pszProvider = _mstrAuthProviders.Next( pszProvider );
    }
    
    return FALSE;
} 
    
HRESULT
W3_METADATA::CreateUNCVrToken( 
    IN LPWSTR       pszUserName,
    IN LPWSTR       pszPasswd
    )
/*++

  Description:

    Logon the user account for the UNC virtual path

  Arguments:

    pszUserName - User name of the account in format domain\username
    pszPasswd   - Passwd of the account

  Returns:

    HRESULT

--*/
{
    STACK_STRU(             strUserName, UNLEN + 1 );
    STACK_STRU(             strDomainName, IIS_DNLEN + 1 );
    // add 1 to strUserDomain for separator "\"
    STACK_STRU(             strUserDomain, UNLEN + IIS_DNLEN + 1 + 1);
    HRESULT                 hr;
    DWORD                   dwError;
    BOOL                    fPossibleUPNLogon = FALSE;
    
    hr = strUserDomain.Copy( pszUserName );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    hr = W3_STATE_AUTHENTICATION::SplitUserDomain( strUserDomain,
                                                   &strUserName,
                                                   &strDomainName,
                                                   NULL,
                                                   &fPossibleUPNLogon );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    DBG_ASSERT( g_pW3Server->QueryTokenCache() != NULL );
    
    hr = g_pW3Server->QueryTokenCache()->GetCachedToken( 
                                                    strUserName.QueryStr(),
                                                    strDomainName.QueryStr(),
                                                    pszPasswd,
                                                    QueryLogonMethod(),
                                                    FALSE,
                                                    fPossibleUPNLogon,
                                                    NULL,
                                                    &_pctVrToken,
                                                    &dwError );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    //
    // If we didn't get a token, this is a fatal error.  It means bad config
    // so we will prop back an error which means Error 500
    //
    
    if ( _pctVrToken == NULL )
    {
        DBG_ASSERT( dwError != ERROR_SUCCESS );
       
        //
        // Make the error ERROR_LOGON_FAILURE so that we can generate the appropriate
        // custom error response
        //
        
        hr = HRESULT_FROM_WIN32( ERROR_LOGON_FAILURE );
        
        return hr;
    }
    
    return NO_ERROR;
}

HRESULT
W3_METADATA::CreateAnonymousToken( 
    IN LPWSTR       pszUserName,
    IN LPWSTR       pszPasswd
    )
/*++

  Description:

    Logon the user account for the UNC virtual path

  Arguments:

    pszUserName - User name of the account in format domain\username
    pszPasswd   - Passwd of the account

  Returns:

    HRESULT

--*/
{
    STACK_STRU(             strUserName, UNLEN );
    STACK_STRU(             strDomainName, IIS_DNLEN );
    // add 1 to strUserDomain for separator "\"
    STACK_STRU(             strUserDomain, UNLEN + IIS_DNLEN + 1 );
    HRESULT                 hr;
    DWORD                   dwLogonError;
    BOOL                    fPossibleUPNLogon = FALSE;
    BOOL                    fUseAnonSubAuth = FALSE;
    
    hr = strUserDomain.Copy( pszUserName );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    hr = W3_STATE_AUTHENTICATION::SplitUserDomain( strUserDomain,
                                                   &strUserName,
                                                   &strDomainName,
                                                   NULL,
                                                   &fPossibleUPNLogon );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    DBG_ASSERT( g_pW3Server->QueryTokenCache() != NULL );
    
    if( _fUseAnonSubAuth ) 
    {
        if( !W3_STATE_AUTHENTICATION::sm_fSubAuthConfigured )
        {
            if( W3_STATE_AUTHENTICATION::sm_lSubAuthAnonEvent == 0 )
            {
                if( !InterlockedExchange( &W3_STATE_AUTHENTICATION::sm_lSubAuthAnonEvent, 1 ) )
                {
                    //
                    // The registry key for iissuba is not configured correctly on local machine
                    //
                    g_pW3Server->LogEvent( W3_EVENT_SUBAUTH_REGISTRY_CONFIGURATION_LOCAL,
                                           0,
                                           NULL );
                }
            }
        }
        else if( !W3_STATE_AUTHENTICATION::sm_fLocalSystem )
        {
            if( W3_STATE_AUTHENTICATION::sm_lLocalSystemEvent == 0 )
            {
                if( !InterlockedExchange( &W3_STATE_AUTHENTICATION::sm_lLocalSystemEvent, 1 ) )
                {
                    //
                    // The process token does not have SeTcbPrivilege
                    //
                    g_pW3Server->LogEvent( W3_EVENT_SUBAUTH_LOCAL_SYSTEM,
                                           0,
                                           NULL );
                }
            }
        }
        else
        {
            fUseAnonSubAuth = TRUE;
        }
    }
    
    hr = g_pW3Server->QueryTokenCache()->GetCachedToken( 
                                                    strUserName.QueryStr(),
                                                    strDomainName.QueryStr(),
                                                    pszPasswd,
                                                    QueryLogonMethod(),
                                                    fUseAnonSubAuth,
                                                    fPossibleUPNLogon,
                                                    NULL,
                                                    &_pctAnonymousToken,
                                                    &dwLogonError );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    return NO_ERROR;
}

HRESULT
W3_METADATA::GetAndRefAnonymousToken(
    TOKEN_CACHE_ENTRY ** ppCachedToken
)
/*++

  Description:

    Get the anonymous token for the current request.
    Everybody that calls this function needs to call
    deref when it's done.
          

  Arguments:

    NONE

  Returns:

    HRESULT

--*/
{
    HRESULT         hr = S_OK;
    BOOL            fReadLocked = TRUE;

    //
    // If you want to modify the stack buffer length, make sure 
    // it's a multiple of 16 bytes( CRYPTPROTECTMEMORY_BLOCK_SIZE )
    //

    STACK_BUFFER( bufDecryptedPassword, PWLEN * sizeof( WCHAR ) );

    if( ppCachedToken == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    *ppCachedToken = NULL;
    
    //
    // Do a quick check for an existing token
    //
    
    if ( _pctAnonymousToken == NULL )
    {
        return NO_ERROR;
    }
    
    //
    // First see if we even have a token to return?
    //

    _TokenLock.ReadLock();
        
    if ( _pctAnonymousToken != NULL )
    {
        //
        // Checkout the token.  This insures we don't use a 
        // flushed token
        //
        
        if ( !_pctAnonymousToken->Checkout( NULL ) )
        {
            //
            // OK.  Get rid of associated token and build a new one
            //
            
            _TokenLock.ConvertSharedToExclusive();
        
            fReadLocked = FALSE;
            
            if ( _pctAnonymousToken != NULL )
            {
                _pctAnonymousToken->DereferenceCacheEntry();
                _pctAnonymousToken = NULL;
            }
            
            //
            // Do the logon
            //
        
            hr = DecryptMemoryPassword( &_strPasswd, 
                                        &bufDecryptedPassword );
            if ( SUCCEEDED( hr ) )
            {
                hr = CreateAnonymousToken( 
                           _strUserName.QueryStr(),
                           ( WCHAR * )bufDecryptedPassword.QueryPtr() );
                           
                if ( SUCCEEDED( hr ) )
                {
                    if ( _pctAnonymousToken != NULL )
                    {
                        _pctAnonymousToken->ReferenceCacheEntry();
                    }
                    
                    *ppCachedToken = _pctAnonymousToken;
                }

                SecureZeroMemory( bufDecryptedPassword.QueryPtr(),
                                  bufDecryptedPassword.QuerySize() );
            }
        }
        else
        {
            *ppCachedToken = _pctAnonymousToken;
        }
    }
    
    if ( fReadLocked )
    {
        _TokenLock.ReadUnlock();
    }
    else
    {
        _TokenLock.WriteUnlock();
    }

    return hr;
}

HRESULT
W3_METADATA::GetAndRefVrAccessToken(
    TOKEN_CACHE_ENTRY ** ppCachedToken
)
/*++

  Description:

    Get the UNC token for the current request.
    Everybody that calls this function needs to call
    deref when it's done.

  Arguments:

    NONE

  Returns:

    HRESULT

--*/
{
    HRESULT         hr = S_OK;
    BOOL            fReadLocked = TRUE;
    
    STACK_BUFFER( bufDecryptedPassword, PWLEN * sizeof( WCHAR )  );

    if( ppCachedToken == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    *ppCachedToken = NULL;

    //
    // Do a quick check for an existing token
    //
    
    if ( _pctVrToken == NULL )
    {
        return NO_ERROR;
    }
    
    //
    // First see if we even have a token to return?
    //

    _TokenLock.ReadLock();
        
    if ( _pctVrToken != NULL )
    {
        //
        // Checkout the token.  This insures we don't use a 
        // flushed token
        //
        
        if ( !_pctVrToken->Checkout( NULL ) )
        {
            //
            // OK.  Get rid of associated token and build a new one
            //
            
            _TokenLock.ConvertSharedToExclusive();
        
            fReadLocked = FALSE;
            
            if ( _pctVrToken != NULL )
            {
                _pctVrToken->DereferenceCacheEntry();
                _pctVrToken = NULL;
            }
            
            //
            // Do the logon
            //

            if ( _strVrUserName.QueryStr() != NULL && 
                 _strVrPasswd.QueryStr()   != NULL &&
                 _strVrUserName.QueryStr()[0] )
            {
                hr = DecryptMemoryPassword( &_strVrPasswd,
                                            &bufDecryptedPassword );
                if( SUCCEEDED( hr ) )
                {                
                    hr = CreateUNCVrToken( _strVrUserName.QueryStr(), 
                                           (WCHAR*) bufDecryptedPassword.QueryPtr() );
                    if( SUCCEEDED( hr ) )
                    {
                        _dirmonConfig.hToken = _pctVrToken->QueryImpersonationToken();        
                        _pctVrToken->ReferenceCacheEntry();
                        
                        *ppCachedToken = _pctVrToken;
                    }
                    else
                    {
                        _fUNCUserInvalid = TRUE;
                        hr = NO_ERROR;
                    }

                    SecureZeroMemory( bufDecryptedPassword.QueryPtr(),
                                      bufDecryptedPassword.QuerySize() );
                }
            }
        }
        else
        {
            *ppCachedToken = _pctVrToken;
        }
    }

    if ( fReadLocked )
    {
        _TokenLock.ReadUnlock();
    }
    else
    {
        _TokenLock.WriteUnlock();
    }

    return hr;
}

HRESULT
W3_METADATA::SetIpAccessCheck( 
    LPVOID   pMDData, 
    DWORD    dwDataLen
)
/*++

  Description:

    Store away the IP DNS list

  Arguments:

    pMDData - Beginning of binary blob to store
    dwDataLen - Length of binary data

  Returns:

    HRESULT

--*/
{
    if ( !_buffIpAccessCheck.Resize( dwDataLen ) )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    memcpy( _buffIpAccessCheck.QueryPtr(),
            pMDData,
            dwDataLen );
   
    _cbIpAccessCheck = dwDataLen;
    
    return NO_ERROR;
}

HRESULT
W3_METADATA::GetMetadataProperty(
    W3_CONTEXT *            pW3Context,
    DWORD                   dwPropertyId,
    PBYTE                   pbBuffer,
    DWORD                   cbBuffer,
    DWORD *                 pcbBufferRequired
)
/*++

Routine Description:

    Retrieve and serialize the requested metabase property.  

Arguments:

    pW3Context - W3 Context
    dwPropertyId - Property ID
    pbBuffer - Buffer (OPTIONAL)
    cbBuffer - Size of given buffer
    pcbBufferRequired - Set to size of buffer required

Returns:

    HRESULT

--*/
{
    HRESULT                 hr;
    DWORD                   dwDataSetNumber;
    DWORD                   dwNumMDRecords;
    METADATA_GETALL_RECORD* pRecord;
    VOID *                  pDataPointer;
    DWORD                   cbNeeded = 0;
    METADATA_RECORD *       pOutputRecord;
    
    if ( pcbBufferRequired == NULL ||
         pW3Context == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    //
    // First check whether we have already read the raw buffer.  If not, 
    // read it in
    //
    
    if ( _pGetAllBuffer == NULL )
    {
        BUFFER *                pBuffer;
        MB                      mb( g_pW3Server->QueryMDObject() );
        BOOL                    fRet;
        STACK_STRU(             strUrl, 256 );
        HANDLE                  hToken = NULL;
        
        pBuffer = new BUFFER;
        if ( pBuffer == NULL )
        {
            return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        }
        
        hr = pW3Context->QueryRequest()->GetUrl( &strUrl );
        if ( FAILED( hr ) )
        {
            return hr;
        }

        fRet = OpenThreadToken( GetCurrentThread(),
                                TOKEN_IMPERSONATE,
                                TRUE,
                                &hToken );
        if ( fRet )
        {
            DBG_ASSERT( hToken != NULL );
            RevertToSelf();
        }
        
        //
        // Open the metabase at the site level
        //
        
        fRet = mb.Open( pW3Context->QuerySite()->QueryMBRoot()->QueryStr() );
        if ( fRet )
        {
            fRet = mb.GetAll( strUrl.QueryStr(),
                              METADATA_INHERIT | METADATA_PARTIAL_PATH,
                              IIS_MD_UT_FILE,
                              pBuffer,
                              &dwNumMDRecords,
                              &dwDataSetNumber );
            
            mb.Close();
        }
        
        if ( hToken != NULL )
        {
            SetThreadToken( NULL, hToken );
            CloseHandle( hToken );
            hToken = NULL;
        }
        
        if ( !fRet )
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }
        
        //
        // Try to stuff our buffer into this cache entry
        //
        
        LockCacheEntry();
        
        if ( _pGetAllBuffer == NULL )
        {
            _pGetAllBuffer = pBuffer;
            _cGetAllRecords = dwNumMDRecords;
        }
        else
        {
            //
            // We already have one in here.  Kill ours.
            //
            
            delete pBuffer;
            pBuffer = NULL;
        }
        
        UnlockCacheEntry();
    }
    
    DBG_ASSERT( _pGetAllBuffer != NULL );
    
    //
    // Lookup the property in question
    // 
    
    for ( DWORD i = 0; i < _cGetAllRecords; i++ )
    {
        pRecord = &(((METADATA_GETALL_RECORD*) _pGetAllBuffer->QueryPtr())[ i ]);
        pDataPointer = (PVOID) ((PCHAR) _pGetAllBuffer->QueryPtr() + pRecord->dwMDDataOffset );
        
        DBG_ASSERT( pRecord != NULL );
        DBG_ASSERT( pDataPointer != NULL );

        if ( pRecord->dwMDIdentifier == dwPropertyId )
        {
            //
            // Found it!
            // 
            
            switch( pRecord->dwMDDataType )
            {
            case DWORD_METADATA:
                cbNeeded = sizeof( DWORD );
                break;
            
            case STRING_METADATA:
                cbNeeded = ( wcslen( (WCHAR*) pDataPointer ) + 1 ) * sizeof( WCHAR );
                break;

            default:
                // CODEWORK: Handle the other data types
                return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
            }
            
            //
            // Account for the METADATA_RECORD
            //
            
            cbNeeded += sizeof( METADATA_RECORD );
            
            //
            // Do the copy and other return values
            //
            
            DBG_ASSERT( cbNeeded != 0 );
            
            *pcbBufferRequired = cbNeeded;
            
            if ( pbBuffer == NULL ||
                 cbBuffer < *pcbBufferRequired )
            {
                return HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER );
            }
            
            pOutputRecord = (METADATA_RECORD*) pbBuffer;
            
            pOutputRecord->dwMDIdentifier = dwPropertyId;
            pOutputRecord->dwMDAttributes = pRecord->dwMDAttributes;
            pOutputRecord->dwMDUserType = pRecord->dwMDUserType;
            pOutputRecord->dwMDDataLen = cbNeeded - sizeof( METADATA_RECORD );
            pOutputRecord->dwMDDataType = pRecord->dwMDDataType;
            pOutputRecord->pbMDData = pbBuffer + sizeof( METADATA_RECORD );
            
            memcpy( pbBuffer + sizeof( METADATA_RECORD ),
                    pDataPointer,
                    pOutputRecord->dwMDDataLen );

            return NO_ERROR;
        }
    } 
    
    return HRESULT_FROM_WIN32( ERROR_INVALID_INDEX );
}

HRESULT 
W3_METADATA::ReadCustomFooter(
    WCHAR *                 pszFooter
)
/*++

Routine Description:

    Process a footer string, either reading the file or copying the string
    to the buffer.

Arguments:

    pszFooter - The footer string, which may be a string or a file name.
    It looks like "STRING : some-string" or "FILE : file-name"

Returns:

    HRESULT

--*/
{
    HRESULT             hr;
    STACK_STRU(         strFooter, MAX_PATH );
    BOOL                fFooterIsString = FALSE;

    // First thing to do is to determine if this is a string or a file name.
    // Skip preceding whitespace and then strcmp.

    while (iswspace(*pszFooter))
    {
        pszFooter++;
    }

    if (!_wcsnicmp(pszFooter, L"STRING", 6))
    {
        fFooterIsString = TRUE;
        pszFooter += 6;
    }
    else if (!_wcsnicmp(pszFooter, L"FILE", 4))
    {
        fFooterIsString = FALSE;
        pszFooter += 4;
    }
    else
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }

    // Now we look for 0 or more white space, followed by a colon, followed by
    // more white space.

    while (iswspace(*pszFooter))
    {
        pszFooter++;
    }

    if (*pszFooter != L':')
    {
        // No colon seperator, error.
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }

    pszFooter++;

    //
    // OK, now if this is a string we take everything after the colon to the
    // end for the string. If this is a file name then we'll open and read the
    // file.
    //
    if (fFooterIsString)
    {
        return _strFooterString.CopyW(pszFooter);
    }
    else
    {
        //
        // For files, we'll skip any more white space before the name.
        //
        while (iswspace(*pszFooter))
        {
            pszFooter++;
        }
        
        hr = _strFooterDocument.Copy( pszFooter );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }

    return S_OK;
}

HRESULT 
W3_METADATA::EncryptMemoryPassword(
    IN OUT STRU * strPassword
)
/*++

Routine Description:

    Encrypt the password within the current process

Arguments:

    strPassword - The password to be encrypted. The encrypted password
                  is in the buffer in the STRU, and is not null 
                  terminated. So QueryCB and QueryCCH don't mean 
                  anything here.

Returns:

    HRESULT

--*/
{
    HRESULT hr;

    hr = strPassword->Resize( 
                    ( ( strPassword->QueryCCH() + 1 ) * sizeof( WCHAR )                 
                      / CRYPTPROTECTMEMORY_BLOCK_SIZE + 1 ) 
                    * CRYPTPROTECTMEMORY_BLOCK_SIZE );
    if( SUCCEEDED( hr ) )
    {
        if( !CryptProtectMemory( strPassword->QueryBuffer()->QueryPtr(),
                                 strPassword->QueryBuffer()->QuerySize(),
                                 CRYPTPROTECTMEMORY_SAME_PROCESS ) )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
        }
    }
    
    return hr;
}

HRESULT 
W3_METADATA::DecryptMemoryPassword(
    IN  STRU   * strProtectedPassword,
    OUT BUFFER * bufDecryptedPassword
)
/*++

Routine Description:

    Decrypt the password with the current process

Arguments:

    strProtectedPassword - The protected password to be decrypted.
    bufDecryptedPassword - Holds the decrypted password.

Returns:

    HRESULT

--*/
{
    HRESULT  hr = S_OK;

    if( !bufDecryptedPassword->Resize( 
            strProtectedPassword->QueryBuffer()->QuerySize() ) )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }

    memcpy( bufDecryptedPassword->QueryPtr(),
            strProtectedPassword->QueryStr(),
            bufDecryptedPassword->QuerySize() );

    if( !CryptUnprotectMemory( bufDecryptedPassword->QueryPtr(),
                               bufDecryptedPassword->QuerySize(),
                               CRYPTPROTECTMEMORY_SAME_PROCESS ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
    }

    return hr;
}

HRESULT W3_METADATA::SetExpire(WCHAR *pszExpire)
/*++

Routine Description:

    Set the expire header to be used on all responses

Arguments:

    pszExpire: the string containing the description.  It could have the form
    empty: no expire
    "s, some-date" : expire on this date"
    "d, some-number" : expire after this many seconds

Returns:

    HRESULT

--*/
{
    while (iswspace(*pszExpire))
    {
        pszExpire++;
    }

    LPWSTR pszParam;
    if ((pszParam = wcschr(pszExpire, L',')) == NULL)
    {
        if (*pszExpire == L'\0')
        {
            _dwExpireMode = EXPIRE_MODE_OFF;
            return S_OK;
        }

        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }

    pszParam++;
    while (iswspace(*pszParam))
    {
        pszParam++;
    }

    HRESULT hr;
    switch (*pszExpire)
    {
    case L's':
    case L'S':
        if (FAILED(hr = _strExpireHeader.CopyWTruncate(pszParam)))
        {
            return hr;
        }
        _dwExpireMode = EXPIRE_MODE_STATIC;
        break;

    case L'd':
    case L'D':
        LPWSTR endPtr;
        DWORD dwExpire;

        if (pszParam[0] == L'0' && pszParam[1] == L'x')
        {
            dwExpire = wcstoul(pszParam + 2, &endPtr, 16);
        }
        else
        {
            dwExpire = wcstoul(pszParam, &endPtr, 10);
        }

        if (!iswspace(*endPtr) &&
            *endPtr != L'\0')
        {
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        }

        if (dwExpire != ULONG_MAX)
        {
            if (dwExpire > MAX_GLOBAL_EXPIRE)
            {
                dwExpire = MAX_GLOBAL_EXPIRE;
            }

            _dwExpireMode = EXPIRE_MODE_DYNAMIC;
            _dwExpireDelta = dwExpire;
        }
        break;

    default:
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }

    return S_OK;
}

HRESULT W3_METADATA::SetCacheControlHeader()
/*++

Routine Description:

    If no-cache, max-age or a dynamic expires directive is present, add
    it to Cache-Control header

Arguments:

    None

Returns:

    HRESULT

--*/
{
    switch (QueryExpireMode())
    {
    case EXPIRE_MODE_NONE:
        _fHaveNoCache = FALSE;
        _fHaveMaxAge = FALSE;
        break;

    case EXPIRE_MODE_DYNAMIC:
        // If we have a dynamic Expires header, create a max-age directive
        if (_dwExpireDelta != 0)
        {
            _fHaveNoCache = FALSE;
            if (!_fHaveMaxAge)
            {
                _fHaveMaxAge = TRUE;
                _dwMaxAge = _dwExpireDelta;
            }
        }
        else
        {
            _fHaveNoCache = TRUE;
            _fHaveMaxAge = FALSE;
        }
        break;

    default:
        break;
    }

    BOOL fHaveCCHeader = !_strCacheControlHeader.IsEmpty();
    HRESULT hr;
    if (_fHaveNoCache)
    {
        if (FAILED(hr = _strCacheControlHeader.Append(
                             fHaveCCHeader ? ",no-cache" : "no-cache")))
        {
            return hr;
        }
    }
    else if (_fHaveMaxAge)
    {
        CHAR pszMaxAgeBuffer[16];
        _itoa(_dwMaxAge, pszMaxAgeBuffer, 10);

        if (FAILED(hr = _strCacheControlHeader.Append(
                             fHaveCCHeader ? ",max-age=" : "max-age=")) ||
            FAILED(hr = _strCacheControlHeader.Append(pszMaxAgeBuffer)))
        {
            return hr;
        }
    }

    return S_OK;
}

HRESULT
META_SCRIPT_MAP::Initialize( 
    IN WCHAR * szScriptMapData
    )
/*++

Routine Description:

    Initialize the collection of META_SCRIPT_MAP_ENTRIES from the 
    metadata.

    This routine will modify the multisz it works with (by replacing 
    some ',' with '\0' ).
    
    Currently it modifies the in parameter, which is kindof icky. 
    We could avoid this by copying the buffer.

Arguments:
    
    szScriptMapData - A multi-sz of script map entries. 

    
Return Value:

Notes:

    Script map is a multi-sz with each string being a comma separated list
    <extension>,<executable>,<flags>,<verb list>

    <extension>:
      .xyz  - Maximum of 128 characters
      *     - Star script map - routes all requests though the executable

    <executable>
        - Extension to invoke

    <flags>:
      1     - Allow run in script access directory ( MD_SCRIPTMAPFLAG_SCRIPT )
      4 - Check for pathinfo file ( MD_SCRIPTMAPFLAG_CHECK_PATH_INFO )

    <verb list>:
      <verb>,<verb>,<verb>
        - Allowed verbs
        - If no verbs are listed, a value of "all verbs" is assumed. 

--*/
{
    DBG_ASSERT( szScriptMapData );

    HRESULT     hr = NOERROR;

    // Iterate over multisz
    WCHAR *     pszEntryIterator;
    
    // Current mapping
    WCHAR *     pszExtension;
    WCHAR *     pszExecutable;
    WCHAR *     pszFlags;
    DWORD       Flags;
    WCHAR *     pszVerbs;
    DWORD       cchVerbs;
    WCHAR *     pszDest;

    //
    // Iterate over each mapping
    //

    pszEntryIterator = szScriptMapData;
    while( *pszEntryIterator != L'\0' )
    {
        //
        // Get the extension
        //

        pszExtension = pszEntryIterator;

        //
        // Get the executable
        //
        
        pszEntryIterator = wcschr( pszEntryIterator, L',' );
        if( pszEntryIterator == NULL )
        {
            hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
            return hr;
        }

        *pszEntryIterator++ = L'\0';

        pszExecutable = pszEntryIterator;

        if( pszExecutable == L'\0' )
        {
            hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
            return hr;
        }

        //
        // Get the flags
        //
        
        pszEntryIterator = wcschr( pszEntryIterator, L',' );
        if( pszEntryIterator == NULL )
        {
            hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
            return hr;
        }

        *pszEntryIterator++ = L'\0';

        //
        // We don't need pszFlags here, but we will need it if
        // there is an empty verb list, to reset our iterator.
        //
        pszFlags = pszEntryIterator;

        Flags = wcstoul( pszFlags, NULL, 10 );

        //
        // Get the verbs
        //

        pszEntryIterator = wcschr( pszEntryIterator, L',' );
        if( pszEntryIterator != NULL )
        {
            //
            // There is a list of verbs
            //

            *pszEntryIterator++ = L'\0';

            pszDest = pszVerbs = pszEntryIterator;
        
            //
            // Format verb list as a multi-sz for each entry
            //

            cchVerbs = 1;

            while( *pszEntryIterator != L'\0')
            {
                if( *pszEntryIterator == L',' || *pszEntryIterator == L' ' )
                {
                    while( *pszEntryIterator == L',' || *pszEntryIterator == L' ' )
                    {
                        pszEntryIterator++;
                    }

                    *pszDest++ = L'\0';
                }
                else
                {
                    *pszDest++ = *pszEntryIterator++;
                }

                cchVerbs++;
            }
        }
        else
        {
            //
            // Empty Verb List
            //

            //
            // We've lost our iterator so we need to get it back.
            // Point to the terminator.
            //
            pszEntryIterator = pszFlags + wcslen( pszFlags );
            
            pszVerbs = pszEntryIterator;
            cchVerbs = 1;
        }


        //
        // Create and add the entry object to our list
        //
    
        META_SCRIPT_MAP_ENTRY * 
            pNewEntry = new META_SCRIPT_MAP_ENTRY();

        if( pNewEntry == NULL )
        {
            hr = E_OUTOFMEMORY;
            return hr;
        }

        hr = pNewEntry->Create( pszExtension,
                                pszExecutable,
                                Flags,
                                pszVerbs,
                                cchVerbs
                                );
        if( FAILED(hr) )
        {
            delete pNewEntry;
            return hr;
        }

        if (pNewEntry->QueryIsStarScriptMap())
        {
            InsertTailList( &m_StarScriptMapListHead, &pNewEntry->m_ListEntry );
        }
        else
        {
            InsertTailList( &m_ListHead, &pNewEntry->m_ListEntry );
        }
        
        //
        // Move to the next entry.
        //
        pszEntryIterator++;      
    }

    return hr;
}

BOOL
META_SCRIPT_MAP::FindEntry(
    IN  const STRU &              strExtension,
    OUT META_SCRIPT_MAP_ENTRY * * ppScriptMapEntry
    )
/*++

Routine Description:

Arguments:
    
Return Value:

--*/
{
    *ppScriptMapEntry = NULL;

    PLIST_ENTRY             pEntry;
    META_SCRIPT_MAP_ENTRY * pScriptMapEntry = NULL;

    for( pEntry  = m_ListHead.Flink;
         pEntry != &m_ListHead;
         pEntry  = pEntry->Flink )
    {
        pScriptMapEntry = CONTAINING_RECORD( pEntry, 
                                             META_SCRIPT_MAP_ENTRY, 
                                             m_ListEntry 
                                             );

        if( strExtension.Equals( pScriptMapEntry->m_strExtension ) )
        {
            *ppScriptMapEntry = pScriptMapEntry;
            return TRUE;
        }
    }

    return FALSE;
}


VOID
META_SCRIPT_MAP::Terminate( VOID )
/*++

Routine Description:

Arguments:
    
Return Value:

--*/
{
    META_SCRIPT_MAP_ENTRY * pScriptMapEntry;

    while( !IsListEmpty( &m_StarScriptMapListHead ) )
    {
        pScriptMapEntry = CONTAINING_RECORD(  m_StarScriptMapListHead.Flink,
                                              META_SCRIPT_MAP_ENTRY,
                                              m_ListEntry );

        RemoveEntryList( &pScriptMapEntry->m_ListEntry );

        delete pScriptMapEntry;
    }

    while( !IsListEmpty( &m_ListHead ) )
    {
        pScriptMapEntry = CONTAINING_RECORD(  m_ListHead.Flink,
                                              META_SCRIPT_MAP_ENTRY,
                                              m_ListEntry );

        RemoveEntryList( &pScriptMapEntry->m_ListEntry );

        delete pScriptMapEntry;
    }
}

HRESULT
META_SCRIPT_MAP_ENTRY::Create(
    IN const WCHAR * szExtension,
    IN const WCHAR * szExecutable,
    IN DWORD         Flags,
    IN const WCHAR * szVerbs,
    IN DWORD         cchVerbs
    )
/*++

Routine Description:

Arguments:
    
Return Value:

--*/
{
    HRESULT hr = NOERROR;
    DWORD cchExecutable;

    //
    // Capture initialization parameters
    //

    m_Flags = Flags;

    hr = m_strExtension.Copy( szExtension );
    if( FAILED(hr) )
    {
        return hr;
    }

    //
    // Lower-case to allow for case insensitive comparisons
    //
    _wcslwr( m_strExtension.QueryStr() );
    
    if (szExtension[0] == L'*' && szExtension[1] == L'\0')
    {
        m_fIsStarScriptMapEntry = TRUE;
    }

    //
    // We treat the executable name as an ExpandSz, so expand it
    //
    WCHAR szExpand[MAX_PATH + 1];
    if (!ExpandEnvironmentStrings(szExecutable,
                                  szExpand,
                                  sizeof szExpand/sizeof WCHAR))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (FAILED(hr = m_strExecutable.Copy( szExpand )))
    {
        return hr;
    }
    
    //
    // If the executable is quoted, remove the quotes now
    //
    // Note that we can't just remove the first and last
    // quote, as it's possible that the script map entry
    // could look like this:
    //
    //   "c:\program files\test\test.exe" "%s" "%s"
    //
    if ( m_strExecutable.QueryStr()[ 0 ] == L'\"' &&
         wcschr( m_strExecutable.QueryStr() + 1, L'\"' ) )
    {
        LPWSTR  pCursor;

        cchExecutable = m_strExecutable.QueryCCH();

        pCursor = m_strExecutable.QueryStr();

        while ( *(pCursor+1) != L'\"' )
        {
            *pCursor = *(pCursor+1);

            pCursor++;
        }

        while ( *(pCursor+2) != L'\0' )
        {
            *pCursor = *(pCursor+2);

            pCursor++;
        }

        *pCursor = L'\0';

        m_strExecutable.SetLen( cchExecutable - 2 );
    }

    if (m_strExecutable.QueryCCH() > 4)
    {
        if (!_wcsicmp(
                 m_strExecutable.QueryStr() + m_strExecutable.QueryCCH() - 4,
                 L".dll"))
        {
            m_Gateway = GATEWAY_ISAPI;
        }
        else
        {
            m_Gateway = GATEWAY_CGI;
        }
    }
    else
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }

    if (!m_Verbs.AppendW( szVerbs, cchVerbs ))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        return hr;
    }

    return S_OK;
}

HRESULT
W3_METADATA_CACHE::Initialize(
    VOID
)
/*++

  Description:

    Initialize metadata cache

  Arguments:

    None

  Return:

    HRESULT

--*/
{
    HRESULT             hr;
    DWORD               dwData;
    DWORD               dwType;
    DWORD               cbData = sizeof( DWORD );
    DWORD               csecTTL = DEFAULT_W3_METADATA_CACHE_TTL;
    HKEY                hKey;

    //
    // What is the TTL for the URI cache
    //
    
    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       L"System\\CurrentControlSet\\Services\\w3svc\\Parameters",
                       0,
                       KEY_READ,
                       &hKey ) == ERROR_SUCCESS )
    {
        DBG_ASSERT( hKey != NULL );
        
        if ( RegQueryValueEx( hKey,
                              L"MetadataCacheTTL",
                              NULL,
                              &dwType,
                              (LPBYTE) &dwData,
                              &cbData ) == ERROR_SUCCESS &&
             dwType == REG_DWORD )
        {
            csecTTL = dwData;
        }

        RegCloseKey( hKey );
    }                      
    
    //
    // We'll use TTL for scavenge period, and expect two inactive periods to
    // flush
    //
    
    hr = SetCacheConfiguration( csecTTL * 1000, 
                                csecTTL * 1000,
                                CACHE_INVALIDATION_METADATA,
                                NULL );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    return W3_METADATA::Initialize();
}

HRESULT
W3_METADATA_CACHE::GetMetaData(
    W3_CONTEXT *                pW3Context,
    STRU &                      strUrl,
    W3_METADATA **              ppMetaData
)
/*++

Routine Description:

    Retrieve a W3_METADATA, creating if necessary

Arguments:

    pW3Context - W3 context
    strUrl - Url
    ppMetaData - Set to point to metadata on success
    
Return Value:

    HRESULT

--*/
{
    W3_METADATA_KEY           metaKey;
    W3_METADATA *             pMetaData;
    DWORD                     dwDataSetNumber;
    HRESULT                   hr;
    STACK_STRU(               strFullPath, 128 );
    DATA_SET_CACHE *          pDataSetCache;
    MB                        mb( g_pW3Server->QueryMDObject() );
    HANDLE                    hToken = NULL;
    
    if ( pW3Context == NULL ||
         ppMetaData == NULL )
    {
        DBG_ASSERT( FALSE );
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        goto Failed;
    }
    
    *ppMetaData = NULL;
    
    //
    // Setup a key to lookup by determining data set number
    // 
    
    hr = GetFullMetadataPath( pW3Context,
                              strUrl,
                              &strFullPath );
    if ( FAILED( hr ) )
    {
        goto Failed;
    }
    
    //
    // Get a data set cache
    // 
    
    hr = pW3Context->QuerySite()->GetDataSetCache( &pDataSetCache );
    if ( FAILED( hr ) )
    {
        goto Failed;
    }
    
    DBG_ASSERT( pDataSetCache != NULL );
    
    hr = pDataSetCache->GetDataSetNumber( strFullPath,
                                          &dwDataSetNumber );

    pDataSetCache->DereferenceDataSetCache();
    pDataSetCache = NULL;

    if ( FAILED( hr ) )
    {
        goto Failed;
    }
    
    metaKey.SetDataSetNumber( dwDataSetNumber );
    
    //
    // Look it up
    //
    
    hr = FindCacheEntry( &metaKey, (CACHE_ENTRY**) &pMetaData );
    if ( SUCCEEDED( hr ) )
    {
        DBG_ASSERT( pMetaData != NULL );
       
        *ppMetaData = pMetaData;
        
        goto Succeeded;
    }
    
    //
    // We need to create a metadata entry and add it
    //
    
    if ( OpenThreadToken( GetCurrentThread(),
                          TOKEN_IMPERSONATE,
                          TRUE,
                          &hToken ) )
    {
        DBG_ASSERT( hToken != NULL );
        DBG_REQUIRE( RevertToSelf() );
    }

    hr = CreateNewMetaData( pW3Context,
                            strUrl,
                            strFullPath,
                            &pMetaData );
    if ( FAILED( hr ) )
    {
        goto Failed;
    }
    
    if ( hToken != NULL )
    {
        DBG_REQUIRE( SetThreadToken( NULL, hToken ) );
        DBG_REQUIRE( CloseHandle( hToken ) );
        hToken = NULL;
    }

    DBG_ASSERT( pMetaData != NULL );
    
    //
    // Add to the cache
    //
    
    AddCacheEntry( pMetaData );
    
    *ppMetaData = pMetaData;
    
Succeeded:

    DBG_ASSERT( SUCCEEDED( hr ) );

    return NO_ERROR;

Failed:

    if ( hToken != NULL )
    {
        DBG_REQUIRE( SetThreadToken( NULL, hToken ) );
        DBG_REQUIRE( CloseHandle( hToken ) );
        hToken = NULL;
    }

    DBG_ASSERT( FAILED( hr ) );

    return hr;

}

HRESULT
W3_METADATA_CACHE::CreateNewMetaData(
    W3_CONTEXT *            pW3Context,
    STRU &                  strUrl,
    STRU &                  strFullMetadataPath,
    W3_METADATA **          ppMetaData
)
/*++

Routine Description:

    Create a new W3_METADATA and initializes it

Arguments:

    pW3Context - context
    strUrl - URL
    strFullMetadataPath - Full metabase path to open
    ppMetaData - Set to new metadata entry on success

Return Value:

    HRESULT

--*/
{
    HRESULT                     hr;
    W3_METADATA *               pMetaData = NULL;
    
    if ( pW3Context == NULL ||
         ppMetaData == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    *ppMetaData = NULL;
    
    //
    // Create the metadata object
    //
    
    pMetaData = new W3_METADATA( this );
    if ( pMetaData == NULL )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    //
    // Set full URL for flushing purposes
    //
    
    hr = pMetaData->QueryMetadataPath()->Copy( strFullMetadataPath );
    if ( FAILED( hr ) )
    {
        pMetaData->DereferenceCacheEntry();
        return hr;
    }
    
    //
    // Initialize it
    //

    hr = pMetaData->ReadMetaData( *(pW3Context->QuerySite()->QueryMBRoot()),
                                  strUrl );
    if( FAILED(hr) )
    {
        pMetaData->DereferenceCacheEntry();
        return hr;
    }

    *ppMetaData = pMetaData;

    return NO_ERROR;
}

HRESULT
W3_METADATA_CACHE::GetFullMetadataPath(
    W3_CONTEXT *                pW3Context,
    STRU &                      strUrl,
    STRU *                      pstrFullPath
)
/*++

Routine Description:

    Get the full metadata given the URL and site

Arguments:

    pW3Context - Used to get the site
    strUrl - Url
    pstrFullPath - Filled with full path on success
    
Return Value:

    HRESULT

--*/
{
    HRESULT             hr;
    WCHAR *             pszSource;
    DWORD               cchSource;

    if ( pW3Context == NULL ||
         pstrFullPath == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    //
    // Build up full metabase path (including instance)
    //
    
    hr = pstrFullPath->Copy( *(pW3Context->QuerySite()->QueryMBRoot()) );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    //
    // Don't copy two slashes
    //
    
    if ( strUrl.QueryStr()[ 0 ] == L'/' ) 
    {
        pszSource = strUrl.QueryStr() + 1;
        cchSource = strUrl.QueryCCH() - 1;
    }
    else
    {
        pszSource = strUrl.QueryStr();
        cchSource = strUrl.QueryCCH();
    }
    
    hr = pstrFullPath->Append( pszSource, cchSource );
    if ( FAILED( hr ) )
    {
        return hr;
    }
   
    return NO_ERROR;
}    

#ifndef _SERVERVAR_HXX_
#define _SERVERVAR_HXX_

//
// Function used to retrieve a server variable
//

typedef HRESULT 
(SERVER_VARIABLE_ROUTINE)
(
    W3_CONTEXT *pW3Context,
    STRA       *pstrValue
);

typedef HRESULT 
(SERVER_VARIABLE_ROUTINE_W)
(
    W3_CONTEXT *pW3Context,
    STRU       *pstrValue
);

typedef SERVER_VARIABLE_ROUTINE *PFN_SERVER_VARIABLE_ROUTINE;
typedef SERVER_VARIABLE_ROUTINE_W *PFN_SERVER_VARIABLE_ROUTINE_W;

//
// Forward declaration for each server variable function
//

extern SERVER_VARIABLE_ROUTINE GetServerVariableQueryString;
extern SERVER_VARIABLE_ROUTINE GetServerVariableAllHttp;
extern SERVER_VARIABLE_ROUTINE GetServerVariableAllRaw;
extern SERVER_VARIABLE_ROUTINE GetServerVariableContentLength;
extern SERVER_VARIABLE_ROUTINE GetServerVariableContentType;
extern SERVER_VARIABLE_ROUTINE GetServerVariablePathInfo;
extern SERVER_VARIABLE_ROUTINE GetServerVariablePathTranslated;
extern SERVER_VARIABLE_ROUTINE GetServerVariableRequestMethod;
extern SERVER_VARIABLE_ROUTINE GetServerVariableInstanceId;
extern SERVER_VARIABLE_ROUTINE GetServerVariableRemoteAddr;
extern SERVER_VARIABLE_ROUTINE GetServerVariableRemotePort;
extern SERVER_VARIABLE_ROUTINE GetServerVariableRemoteHost;
extern SERVER_VARIABLE_ROUTINE GetServerVariableServerName;
extern SERVER_VARIABLE_ROUTINE GetServerVariableServerPort;
extern SERVER_VARIABLE_ROUTINE GetServerVariableServerPortSecure;
extern SERVER_VARIABLE_ROUTINE GetServerVariableServerSoftware;
extern SERVER_VARIABLE_ROUTINE GetServerVariableUrl;
extern SERVER_VARIABLE_ROUTINE GetServerVariableInstanceMetaPath;
extern SERVER_VARIABLE_ROUTINE GetServerVariableRemoteUser;
extern SERVER_VARIABLE_ROUTINE GetServerVariableLogonUser;
extern SERVER_VARIABLE_ROUTINE GetServerVariableAuthType;
extern SERVER_VARIABLE_ROUTINE GetServerVariableAuthPassword;
extern SERVER_VARIABLE_ROUTINE GetServerVariableApplMdPath;
extern SERVER_VARIABLE_ROUTINE GetServerVariableApplPhysicalPath;
extern SERVER_VARIABLE_ROUTINE GetServerVariableGatewayInterface;
extern SERVER_VARIABLE_ROUTINE GetServerVariableHttps;
extern SERVER_VARIABLE_ROUTINE GetServerVariableLocalAddr;
extern SERVER_VARIABLE_ROUTINE GetServerVariableHttpsServerIssuer;
extern SERVER_VARIABLE_ROUTINE GetServerVariableHttpsServerSubject;
extern SERVER_VARIABLE_ROUTINE GetServerVariableHttpsSecretKeySize;
extern SERVER_VARIABLE_ROUTINE GetServerVariableHttpsKeySize;
extern SERVER_VARIABLE_ROUTINE GetServerVariableClientCertIssuer;
extern SERVER_VARIABLE_ROUTINE GetServerVariableClientCertSubject;
extern SERVER_VARIABLE_ROUTINE GetServerVariableClientCertCookie;
extern SERVER_VARIABLE_ROUTINE GetServerVariableClientCertSerialNumber;
extern SERVER_VARIABLE_ROUTINE GetServerVariableClientCertFlags;
extern SERVER_VARIABLE_ROUTINE GetServerVariableHttpUrl;
extern SERVER_VARIABLE_ROUTINE GetServerVariableHttpVersion;
extern SERVER_VARIABLE_ROUTINE GetServerVariableAppPoolId;
extern SERVER_VARIABLE_ROUTINE GetServerVariableScriptTranslated;
extern SERVER_VARIABLE_ROUTINE GetServerVariableUnencodedUrl;
extern SERVER_VARIABLE_ROUTINE GetServerVariableSsiExecDisabled;
extern SERVER_VARIABLE_ROUTINE GetServerVariableOriginalUrl;

extern SERVER_VARIABLE_ROUTINE_W GetServerVariablePathInfoW;
extern SERVER_VARIABLE_ROUTINE_W GetServerVariablePathTranslatedW;
extern SERVER_VARIABLE_ROUTINE_W GetServerVariableServerNameW;
extern SERVER_VARIABLE_ROUTINE_W GetServerVariableUrlW;
extern SERVER_VARIABLE_ROUTINE_W GetServerVariableRemoteUserW;
extern SERVER_VARIABLE_ROUTINE_W GetServerVariableLogonUserW;
extern SERVER_VARIABLE_ROUTINE_W GetServerVariableApplMdPathW;
extern SERVER_VARIABLE_ROUTINE_W GetServerVariableApplPhysicalPathW;
extern SERVER_VARIABLE_ROUTINE_W GetServerVariableAppPoolIdW;
extern SERVER_VARIABLE_ROUTINE_W GetServerVariableScriptTranslatedW;
extern SERVER_VARIABLE_ROUTINE_W GetServerVariableOriginalUrlW;

//
// Server variable hash table
//
    
struct SERVER_VARIABLE_RECORD
{
    CHAR *                          _pszName;
    PFN_SERVER_VARIABLE_ROUTINE     _pfnRoutine;
    PFN_SERVER_VARIABLE_ROUTINE_W   _pfnRoutineW;
    SERVER_VARIABLE_RECORD *        _pNext;
};

#define SERVER_VARIABLE_BUCKETS         131

//
// SERVER_VARIABLE_HASH maps server variable string to routines to eval them
//

class SERVER_VARIABLE_HASH
{
public:
    SERVER_VARIABLE_HASH() 
    {
        ZeroMemory( &_rgBuckets, sizeof( _rgBuckets ) );
    }
    
    static 
    CHAR *
    ExtractKey(
        const SERVER_VARIABLE_RECORD * pRecord
    )
    {
        DBG_ASSERT( pRecord != NULL );
        return pRecord->_pszName;
    }
    
    VOID
    InsertRecord(
        SERVER_VARIABLE_RECORD *        pRecord
    )
    {
        DWORD                   dwIndex;
        SERVER_VARIABLE_RECORD* pCursor;
        
        dwIndex = HashString( pRecord->_pszName ) % SERVER_VARIABLE_BUCKETS;
        
        pCursor = _rgBuckets[ dwIndex ];
        
        pRecord->_pNext = pCursor;
        _rgBuckets[ dwIndex ] = pRecord;
    }
    
    SERVER_VARIABLE_RECORD *
    FindKey(    
        CHAR *                          pszName
    )
    {
        DWORD                   dwIndex;
        SERVER_VARIABLE_RECORD* pRecord;
        
        DBG_ASSERT( pszName != NULL );
        
        dwIndex = HashString( pszName ) % SERVER_VARIABLE_BUCKETS;
        
        pRecord = _rgBuckets[ dwIndex ];
        while ( pRecord != NULL )
        {
            if ( memcmp( pszName, pRecord->_pszName, strlen( pszName ) + 1 ) == 0 )
            {
                break;
            } 
            
            pRecord = pRecord->_pNext;
        }
        
        return pRecord;
    }
    
    static
    HRESULT
    Initialize(
        VOID
    );
    
    static
    VOID
    Terminate(
        VOID
    );
    
    static
    HRESULT
    GetServerVariable(
        W3_CONTEXT *            pContext,
        CHAR *                  pszVariableName,
        CHAR *                  pszBuffer,
        DWORD *                 pcbBuffer
    );
    
    static
    HRESULT
    GetServerVariable(
        W3_CONTEXT *            pContext,
        CHAR *                  pszVariableName,
        STRA *                  pstrVal
    );
    
    static
    HRESULT
    GetServerVariableW(
        W3_CONTEXT *            pContext,
        CHAR *                  pszVariableName,
        STRU *                  pstrVal
    );
    
private:
    
    static SERVER_VARIABLE_HASH *       sm_pRequestHash;
    static SERVER_VARIABLE_RECORD       sm_rgServerRoutines[];

    static
    HRESULT
    GetServerVariableRoutine(
        CHAR *                          pszName,
        PFN_SERVER_VARIABLE_ROUTINE   * ppfnRoutine,
        PFN_SERVER_VARIABLE_ROUTINE_W * ppfnRoutineW
    );
    
    SERVER_VARIABLE_RECORD *             _rgBuckets[ SERVER_VARIABLE_BUCKETS ]; 
};

HRESULT
TranslateIpAddressToStr  (
    DWORD dwAddress,
    STRA * pstraBuffer
);

#endif

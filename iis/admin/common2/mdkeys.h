/*++

   Copyright    (c)    1994-2000    Microsoft Corporation

   Module Name :

        mdkeys.h

   Abstract:

        Metabase key wrapper classes

   Author:

        Ronald Meijer (ronaldm)
        Sergei Antonov (sergeia)

   Project:

        Internet Services Manager

   Revision History:
        2/17/2000    sergeia     removed dependency on MFC

--*/

#ifndef _MDKEYS_H_
#define _MDKEYS_H_

//
// Include Files
//
#include <iadmw.h>
#include <iwamreg.h>
#include <iiscnfgp.h>
#include <winsvc.h>
#include <iisrsta.h>
#include "strpass.h"

//
// Forward definitions
//
class CBlob;

//
// Use this instance number to denote the master
//
#define MASTER_INSTANCE       (0)
#define IS_MASTER_INSTANCE(i) (i == MASTER_INSTANCE)

//
// Metabase node constants, used for static initialization of const
// strings further down.  Defined here with #defines for easier 
// concatenation later.
//
#define SZ_MBN_MACHINE      _T("LM")
#define SZ_MBN_FILTERS      _T("Filters")
#define SZ_MBN_MIMEMAP      _T("MimeMap")
#define SZ_MBN_TEMPLATES    _T("Templates")
#define SZ_MBN_INFO         _T("Info")
#define SZ_MBN_ROOT         _T("Root")
#define SZ_MBN_COMPRESSION  _T("Compression")
#define SZ_MBN_PARAMETERS   _T("Parameters")
#define SZ_MBN_SEP_CHAR     _T('/')
#define SZ_MBN_SEP_STR      _T("/")
#define SZ_MBN_ANYSEP_STR   _T("/\\")
#define SZ_MBN_WEB          _T("W3SVC")
#define SZ_MBN_FTP          _T("MSFTPSVC")
#define SZ_MBN_APP_POOLS    _T("AppPools")


class CIISInterface;

class _EXPORT CComAuthInfo
/*++

Class Description:

    Server/authentication information.  Contains optional 
    impersonation parameters. Typically used in the construction in 
    CIISInterface.

Public Interface:

    CComAuthInfo            : Constructor.  Impersonation optional
    operator=               : Assignment operators
    CreateServerInfoStruct  : Helper function for use in COM
    FreeServerInfoStruct    : As above.

Notes:

    Because there's an operator for a pointer to itself and because
    CIISInterface copies the information at construction time, a 
    CComAuthInfo can safely be constructed on the stack as a parameter
    to CIISInterface derived classes.

--*/
{
//
// Constructor/Destructor
//
public:
    //
    // Standard Constructor.  NULL for servername indicates
    // local computer.
    //
    CComAuthInfo(
        IN LPCOLESTR lpszServerName  = NULL,    
        IN LPCOLESTR lpszUserName    = NULL,
        IN LPCOLESTR lpszPassword    = NULL
        );

    //
    // Copy Constructors
    //
    CComAuthInfo(
        IN CComAuthInfo & auth
        );

    CComAuthInfo(
        IN CComAuthInfo * pAuthInfo OPTIONAL
        );

//
// Assignment operators
//
public:
    CComAuthInfo & operator =(CComAuthInfo & auth);
    CComAuthInfo & operator =(CComAuthInfo * pAuthInfo);
    CComAuthInfo & operator =(LPCTSTR lpszServerName);

//
// Access
//
public:
    COSERVERINFO * CreateServerInfoStruct() const;
    COSERVERINFO * CreateServerInfoStruct(DWORD dwAuthnLevel) const;
    void FreeServerInfoStruct(COSERVERINFO * pServerInfo) const;

    LPOLESTR QueryServerName() const { return m_bstrServerName; }
    LPOLESTR QueryUserName() const { return m_bstrUserName; }
    LPOLESTR QueryPassword() const { return m_bstrPassword; }
    BOOL     IsLocal() const { return m_fLocal; }
    BOOL     UsesImpersonation() const { return m_bstrUserName.Length() > 0; }
    void     SetImpersonation(LPCOLESTR lpszUser, LPCOLESTR lpszPassword);
    void     RemoveImpersonation();
    void     StorePassword(LPCOLESTR lpszPassword);

public:
    HRESULT  ApplyProxyBlanket(IUnknown * pInterface);
	HRESULT  ApplyProxyBlanket(IUnknown * pInterface,DWORD dwAuthnLevel);

    
//
// Conversion Operators
//
public:
    operator LPOLESTR() { return QueryServerName(); }
    operator CComAuthInfo *() { return this; }

//
// Static Helpers
//
public:
    //
    // Given domain\username, split into user name and domain
    //
    static BOOL SplitUserNameAndDomain(
        IN OUT CString & strUserName,
        IN CString & strDomainName
        );

    //
    // Verify username and password are correct
    //
    static DWORD VerifyUserPassword(
        IN LPCTSTR lpstrUserName,
        IN LPCTSTR lpstrPassword
        );

protected:
    //
    // Store the computer name (NULL for local computer)
    //
    void SetComputerName(
        IN LPCOLESTR lpszServerName   OPTIONAL
        );

private:
    CComBSTR    m_bstrServerName;
    CComBSTR    m_bstrUserName;
    CComBSTR    m_bstrPassword;
    BOOL        m_fLocal;
};

class _EXPORT CMetabasePath
/*++

Class Description:

    Metabase path class.  This is a helper class to build complete
    metabase paths out of various components.

    Example: CMetaKey(CComAuthInfo("ronaldm3"), CMetabasePath(SZ_WEBSVC, dwInstance, _T("root")));

--*/
{
    //
    // Metabase components in order
    //
    enum
    {
        iBlank,                    // Sep 0
        iMachine,                  // LM
        iService,                  // e.g. lm/w3svc
        iInstance,                 // e.g. lm/w3svc/1
        iRootDirectory,            // e.g. lm/w3svc/1/root
        iSubDirectory,             // e.g. lm/w3vsc/1/root/foobar
    };

//
// Metabase helper functions.
//
public:
    //
    // Clean metabase path
    //
    static LPCTSTR CleanMetaPath(
        IN OUT CString & strMetaRoot
        );

    static LPCTSTR CleanMetaPath(
        CMetabasePath & path
        );

    //
    // Find the instance number from the given metabase path
    //
    static DWORD GetInstanceNumber(LPCTSTR lpszMDPath);

    //
    // Get the last nodename in the given metabase path
    //
    static LPCTSTR GetLastNodeName(
        IN  LPCTSTR lpszMDPath,
        OUT CString & strNodeName
        );

    //
    // Truncate the path at a given sub path
    //
    static LPCTSTR TruncatePath(
        IN int     nLevel,          
        IN LPCTSTR lpszMDPath,
        OUT CString & strNewPath,
        OUT CString * pstrRemainder = NULL
        );

    static LPCTSTR GetMachinePath(
        IN  LPCTSTR lpszMDPath,
        OUT CString & strNewPath,
        OUT CString * pstrRemainder = NULL
        );

    static LPCTSTR GetServicePath(
        IN  LPCTSTR lpszMDPath,
        OUT CString & strNewPath,
        OUT CString * pstrRemainder = NULL
        );

    static LPCTSTR GetInstancePath(
        IN  LPCTSTR lpszMDPath,
        OUT CString & strNewPath,
        OUT CString * pstrRemainder = NULL
        );

    static LPCTSTR GetRootPath(
        IN  LPCTSTR lpszMDPath,
        OUT CString & strNewPath,
        OUT CString * pstrRemainder = NULL
        );

    //
    // Determine the path to the info node that's relevant
    // to this metabase path.
    //
    static LPCTSTR GetServiceInfoPath(
        IN  LPCTSTR lpszMDPath,
        OUT CString & strInfoPath,   
        IN  LPCTSTR lpszDefService  = SZ_MBN_WEB
        );

    //
    // Change path to parent node
    //
    static LPCTSTR ConvertToParentPath(
        OUT IN CString & strMetaPath
        );

    static LPCTSTR ConvertToParentPath(
        CMetabasePath& path
        );

    //
    // Determine if the path describes a home directory path
    //
    static BOOL IsHomeDirectoryPath(
        IN LPCTSTR lpszMDPath
        );

    //
    // Determine if the path describes the 'master' instance (site)
    //
    static BOOL IsMasterInstance(
        IN LPCTSTR lpszMDPath
        );

    //
    // Split the metapath at the instance border
    //
    static void SplitMetaPathAtInstance(
        IN  LPCTSTR lpszMDPath,
        OUT CString & strParent,
        OUT CString & strAlias
        );

    static BOOL IsSeparator(TCHAR c);
 

//
// Constructor/Destructor
//
public:
    CMetabasePath(
        IN BOOL    fAddBasePath,
        IN LPCTSTR lpszMDPath,
        IN LPCTSTR lpszMDPath2 = NULL,
        IN LPCTSTR lpszMDPath3 = NULL,
        IN LPCTSTR lpszMDPath4 = NULL
        );

    //
    // Construct with path components
    //
    CMetabasePath(
        IN  LPCTSTR lpszSvc        = NULL,    
        IN  DWORD   dwInstance     = MASTER_INSTANCE,
        IN  LPCTSTR lpszParentPath = NULL,        
        IN  LPCTSTR lpszAlias      = NULL    
        );

//
// Access
//
public:
    BOOL    IsHomeDirectoryPath() const { return IsHomeDirectoryPath(m_strMetaPath); }
    LPCTSTR QueryMetaPath() const { return m_strMetaPath; }

//
// Conversion Operators
//
public:
    operator LPCTSTR() const { return QueryMetaPath(); }

//
// Helpers
//
protected:
    void BuildMetaPath(  
        IN  LPCTSTR lpszSvc,
        IN  LPCTSTR szInstance,
        IN  LPCTSTR lpszParentPath,
        IN  LPCTSTR lpszAlias           
        );

    void BuildMetaPath(  
        IN  LPCTSTR lpszSvc,
        IN  DWORD   dwInstance,
        IN  LPCTSTR lpszParentPath,
        IN  LPCTSTR lpszAlias           
        );

    void AppendPath(LPCTSTR lpszPath);
    void AppendPath(DWORD dwInstance);

protected:
    //
    // Metabase path components
    //
    static const LPCTSTR _cszMachine;     
    static const LPCTSTR _cszRoot;        
    static const LPCTSTR _cszSep;         
    static const TCHAR   _chSep;          
    static const CString _anySep;

private:
    CString m_strMetaPath;
};

class _EXPORT CIISInterface
/*++

Class Description:

    Base interface class for IIS interfaces.  Most client COM-wrappers
    should derive from this class so that they can easily pick up
    share authentication and proxy blanket information methods.

Virtual Interface:

    Succeeded           : Return TRUE if item constructed successfully
    QueryResult         : Return construction error code

Public Interface:

    operator BOOL       : Cast to TRUE/FALSE depending on success
    operator HRESULT    : Cast to the HRESULT status

    QueryServerName     : Get the server name
    IsLocal             : Determine if the interface is on the local machine

--*/
{
//
// Constructor/Destructor
//
public:
    CIISInterface(
        IN CComAuthInfo * pAuthInfo,
        IN HRESULT hrInterface    = S_OK
        );

//
// Interface:
//
public:
    CComAuthInfo * QueryAuthInfo() { return &m_auth; }
    LPCOLESTR QueryServerName() const { return m_auth.QueryServerName(); }
    BOOL IsLocal() const { return m_auth.IsLocal(); }

//
// Virtual Interface:
//
public:
    virtual BOOL Succeeded() const { return SUCCEEDED(m_hrInterface); }
    virtual HRESULT QueryResult() const { return m_hrInterface; }
    virtual HRESULT ChangeProxyBlanket(
        IN LPCOLESTR lpszUserName, 
        IN LPCOLESTR lpszPassword
        );

//
// Conversion Operators
//
public:
    operator BOOL() const { return Succeeded(); }
    operator HRESULT() const { return m_hrInterface; }

protected:
    virtual HRESULT ApplyProxyBlanket() = 0;
    HRESULT Create(
        IN  int   cInterfaces,       
        IN  const IID rgIID[],      
        IN  const GUID rgCLSID[],    
        OUT int * pnInterface,          OPTIONAL
        OUT IUnknown ** ppInterface 
        );

protected:
    CComAuthInfo m_auth;
    HRESULT    m_hrInterface;
};


class _EXPORT CMetaInterface : public CIISInterface
/*++

Class description:

    Metabase interface class.

Virtual Interface:

    Succeeded           : Return TRUE if item constructed successfully
    QueryResult         : Return construction error code

Public Interface:

    Regenerate          : Recreate the interface

--*/
{
public:
    //
    // Destructor destroys the interface
    //
    virtual ~CMetaInterface();

//
// Constructor/Destructor
//
protected:
    //
    // Fully defined constructor that creates the interface.
    // Use NULL to indicate the local computer name
    //
    CMetaInterface(
        IN CComAuthInfo * pServer
        );

    //
    // Construct from existing interface
    //
    CMetaInterface(
        IN CMetaInterface * pInterface
        );

public:
    //
    // Rebuild the interface
    //
    HRESULT Regenerate();
    // 
    // Flush matabase to disk
    //
    HRESULT SaveData();
    //
    IMSAdminBase * GetInterface() { return m_pInterface; }

    HRESULT GetAdminInterface2(IMSAdminBase2 ** pp);

protected:
    virtual HRESULT ApplyProxyBlanket();

    //
    // Create a metadata object in this server. This function initializes the
    // metadata object with DCOM.
    //
    HRESULT Create();

    //
    // Make sure the interface has been created
    //
    BOOL HasInterface() const { return m_pInterface != NULL; }

//
// IADMW Interface -- all methods defines as inline at the end of this file.
//
protected:
    HRESULT OpenKey(
        IN  METADATA_HANDLE hkBase,
        IN  LPCTSTR lpszMDPath,
        IN  DWORD dwFlags,
        OUT METADATA_HANDLE * phMDNewHandle
        );

    HRESULT CloseKey(
        IN METADATA_HANDLE hKey
        );

    HRESULT SetLastChangeTime( 
        IN METADATA_HANDLE hMDHandle,
        IN LPCTSTR pszMDPath,
        IN FILETIME * pftMDLastChangeTime,
        IN BOOL bLocalTime
        );
        
    HRESULT GetLastChangeTime( 
        IN  METADATA_HANDLE hMDHandle,
        IN  LPCTSTR lpszMDPath,
        OUT FILETIME * pftMDLastChangeTime,
        IN  BOOL bLocalTime
        );

    HRESULT AddKey( 
        IN METADATA_HANDLE hKey,
        IN LPCTSTR lpszMDPath
        );

    HRESULT DeleteKey(
        IN METADATA_HANDLE hKey,
        IN LPCTSTR lpszMDPath
        );

    HRESULT DeleteChildKeys(
        IN METADATA_HANDLE hKey,
        IN LPCTSTR lpszMDPath
        );

    HRESULT EnumKeys(
        IN  METADATA_HANDLE hKey,
        IN  LPCTSTR lpszMDPath,
        OUT LPTSTR lpszMDName,
        IN  DWORD dwIndex
        );

    HRESULT CopyKey(
        IN METADATA_HANDLE hSourceKey,
        IN LPCTSTR lpszMDSourcePath,
        IN METADATA_HANDLE hDestKey,
        IN LPCTSTR lpszMDDestPath,
        IN BOOL fOverwrite,
        IN BOOL fCopy
        );

    HRESULT RenameKey(
        IN METADATA_HANDLE hKey,
        IN LPCTSTR lpszMDPath,
        IN LPCTSTR lpszNewName
        );

    HRESULT GetData(
        IN  METADATA_HANDLE hKey,
        IN  LPCTSTR lpszMDPath,
        OUT METADATA_RECORD * pmdRecord,
        OUT DWORD * pdwRequiredDataLen
        );

    HRESULT SetData(
        IN METADATA_HANDLE hKey,
        IN LPCTSTR lpszMDPath,
        IN METADATA_RECORD * pmdRecord
        );

    HRESULT DeleteData(
        IN METADATA_HANDLE hKey,
        IN LPCTSTR lpszMDPath,
        IN DWORD dwMDIdentifier,
        IN DWORD dwMDDataType
        );

    HRESULT EnumData(
        IN METADATA_HANDLE hKey,
        IN LPCTSTR lpszMDPath,
        OUT METADATA_RECORD * pmdRecord,
        IN DWORD dwIndex,
        OUT DWORD * pdwRequiredDataLen
        );

    HRESULT GetAllData(
        IN  METADATA_HANDLE hKey,
        IN  LPCTSTR lpszMDPath,
        IN  DWORD dwMDAttributes,
        IN  DWORD dwMDUserType,
        IN  DWORD dwMDDataType,
        OUT DWORD * pdwMDNumDataEntries,
        OUT DWORD * pdwMDDataSetNumber,
        IN  DWORD dwMDBufferSize,
        OUT LPBYTE pbMDBuffer,
        OUT DWORD * pdwRequiredBufferSize
        );

    HRESULT DeleteAllData( 
        IN METADATA_HANDLE hKey,
        IN LPCTSTR lpszMDPath,
        IN DWORD dwMDUserType,
        IN DWORD dwMDDataType
        );

    HRESULT CopyData( 
        IN METADATA_HANDLE hMDSourceKey,
        IN LPCTSTR lpszMDSourcePath,
        IN METADATA_HANDLE hMDDestKey,
        IN LPCTSTR lpszMDDestPath,
        IN DWORD dwMDAttributes,
        IN DWORD dwMDUserType,
        IN DWORD dwMDDataType,
        IN BOOL fCopy
        );

    HRESULT GetDataPaths( 
        IN  METADATA_HANDLE hKey,
        IN  LPCTSTR lpszMDPath,
        IN  DWORD dwMDIdentifier,
        IN  DWORD dwMDDataType,
        IN  DWORD dwMDBufferSize,
        OUT LPTSTR lpszBuffer,
        OUT DWORD * pdwMDRequiredBufferSize
        );

    HRESULT Backup( 
        IN LPCTSTR lpszBackupLocation,
        IN DWORD dwMDVersion,
        IN DWORD dwMDFlags
        );

    HRESULT Restore(    
        IN LPCTSTR lpszBackupLocation,
        IN DWORD dwMDVersion,
        IN DWORD dwMDFlags
        );

    HRESULT EnumBackups(
        OUT LPTSTR lpszBackupLocation,
        OUT DWORD * pdwMDVersion,
        OUT FILETIME * pftMDBackupTime,
        IN  DWORD dwIndex
        );

    HRESULT DeleteBackup(
        IN LPCTSTR lpszBackupLocation,
        IN DWORD dwMDVersion
        );

protected:
    IMSAdminBase * m_pInterface; 

private:
    int  m_iTimeOutValue;         
};



class _EXPORT CMetaKey : public CMetaInterface
/*++

Class Description:

    Metabase key wrapper class

Public Interface:

    CMetaKey                    : Constructor
    ~CMetaKey                   : Destructor

    Succeeded                   : TRUE if key opened successfully.
    QueryResult                 : Get the HRESULT status

    QueryValue                  : Various overloaded methods to get values
    SetValue                    : Various overloaded methods to set values
    DeleteValue                 : Delete a value
    Open                        : Open key
    ReOpen                      : Re key that was opened before
    Close                       : Close key
    ConvertToParentPath         : Change path to parent path

    operator METADATA_HANDLE    : Cast to a metadata handle
    operator LPCTSTR            : Cast to the metabase path
    operator BOOL               : Cast to TRUE if the key is open, FALSE if not

    GetHandle                   : Obtain metadata handle
    IsOpen                      : TRUE if a key is open
    QueryMetaPath               : Get the relative metabase path
    QueryFlags                  : Get the open permissions

--*/
{
//
// Constructor/Destructor
//
public:
    //
    // Null constructor that only creates the interface.
    // A key constructed this way may read from META_ROOT_HANDLE.
    // This is not true of other constructors.
    //
    CMetaKey(
        IN CComAuthInfo * pServer
        );

    //
    // As above, using an existing interface
    //
    CMetaKey(
        IN CMetaInterface * pInterface
        );

    //
    // Fully defined constructor that opens a key
    //
    CMetaKey(
        IN CComAuthInfo * pServer,
        IN LPCTSTR lpszMDPath,
        IN DWORD   dwFlags        = METADATA_PERMISSION_READ,
        IN METADATA_HANDLE hkBase = METADATA_MASTER_ROOT_HANDLE
        );

    //
    // As above, using an existing interface
    //
    CMetaKey(
        IN CMetaInterface * pInterface,
        IN LPCTSTR lpszMDPath,
        IN DWORD   dwFlags        = METADATA_PERMISSION_READ,
        IN METADATA_HANDLE hkBase = METADATA_MASTER_ROOT_HANDLE
        );

    //
    // Copy constructor, might or might not own the key
    //
    CMetaKey(
        IN BOOL fOwnKey,
        IN CMetaKey * pKey
        );

    //
    // Destructor -- closes key.
    //
    virtual ~CMetaKey();

//
// Interface
//
public:
    //
    // Fetch a DWORD
    //
    HRESULT QueryValue(
        IN  DWORD dwID, 
        OUT DWORD & dwValue,
        IN  BOOL * pfInheritanceOverride = NULL,
        IN  LPCTSTR lpszMDPath           = NULL,
        OUT DWORD * pdwAttributes        = NULL
        );

    //
    // Fetch a boolean
    //
    HRESULT QueryValue(
        IN  DWORD dwID, 
        OUT BOOL & fValue,
        IN  BOOL * pfInheritanceOverride = NULL,
        IN  LPCTSTR lpszMDPath           = NULL,
        OUT DWORD * pdwAttributes        = NULL
        );

    //
    // Fetch a string
    //
    HRESULT QueryValue(
        IN  DWORD dwID, 
        OUT CString & strValue, 
        IN  BOOL * pfInheritanceOverride = NULL,
        IN  LPCTSTR lpszMDPath           = NULL,
        OUT DWORD * pdwAttributes        = NULL
        );

    //
    // Fetch a CStrPassword
    //
    HRESULT QueryValue(
        IN  DWORD dwID, 
        OUT CStrPassword & strValue, 
        IN  BOOL * pfInheritanceOverride = NULL,
        IN  LPCTSTR lpszMDPath           = NULL,
        OUT DWORD * pdwAttributes        = NULL
        );

    //
    // Fetch a BSTR
    //
    HRESULT QueryValue(
        IN  DWORD dwID, 
        OUT CComBSTR & strValue, 
        IN  BOOL * pfInheritanceOverride = NULL,
        IN  LPCTSTR lpszMDPath           = NULL,
        OUT DWORD * pdwAttributes        = NULL
        );

    //
    // Fetch a string list
    //
    HRESULT QueryValue(
        IN  DWORD dwID, 
        OUT CStringListEx & strlValue, 
        IN  BOOL * pfInheritanceOverride = NULL,
        IN  LPCTSTR lpszMDPath           = NULL,
        OUT DWORD * pdwAttributes        = NULL
        );

    //
    // Fetch binary blob
    //
    HRESULT QueryValue(
        IN  DWORD dwID, 
        OUT CBlob & blValue, 
        IN  BOOL * pfInheritanceOverride = NULL,
        IN  LPCTSTR lpszMDPath           = NULL,
        OUT DWORD * pdwAttributes        = NULL
        );

    //
    // Store a DWORD
    //
    HRESULT SetValue(
        IN DWORD dwID,
        IN DWORD dwValue,
        IN BOOL * pfInheritanceOverride = NULL,
        IN LPCTSTR lpszMDPath           = NULL
        );

    //
    // Store a BOOL
    //
    HRESULT SetValue(
        IN DWORD dwID,
        IN BOOL fValue,
        IN BOOL * pfInheritanceOverride = NULL,
        IN LPCTSTR lpszMDPath           = NULL
        );

    //
    // Store a string
    //
    HRESULT SetValue(
        IN DWORD dwID,
        IN CString & strValue,
        IN BOOL * pfInheritanceOverride = NULL,
        IN LPCTSTR lpszMDPath           = NULL
        );

    //
    // Store a CStrPassword
    //
    HRESULT SetValue(
        IN DWORD dwID,
        IN CStrPassword & strValue,
        IN BOOL * pfInheritanceOverride = NULL,
        IN LPCTSTR lpszMDPath           = NULL
        );

    //
    // Store a BSTR
    //
    HRESULT SetValue(
        IN DWORD dwID,
        IN CComBSTR & strValue,
        IN BOOL * pfInheritanceOverride = NULL,
        IN LPCTSTR lpszMDPath           = NULL
        );


    //
    // Store a stringlist
    //
    HRESULT SetValue(
        IN DWORD dwID,
        IN CStringListEx & strlValue,
        IN BOOL * pfInheritanceOverride  = NULL,
        IN LPCTSTR lpszMDPath            = NULL
        );

    //
    // Store a binary blob
    //
    HRESULT SetValue(
        IN DWORD dwID,
        IN CBlob & blValue,
        IN BOOL * pfInheritanceOverride  = NULL,
        IN LPCTSTR lpszMDPath            = NULL
        );

    //
    // Delete Value:
    //
    HRESULT DeleteValue(
        IN DWORD   dwID,
        IN LPCTSTR lpszMDPath = NULL
        );

    //
    // Check for path existance
    //
    HRESULT DoesPathExist(
        IN LPCTSTR lpszMDPath
        );

    //
    // Create current path (which we attempted to open, and got
    // a path not found error on).
    //
    HRESULT CreatePathFromFailedOpen();

    //
    // Check for descendant overrides
    //
    HRESULT CheckDescendants(
        IN DWORD   dwID,
        IN CComAuthInfo * pServer,
        IN LPCTSTR lpszMDPath       = NULL
        );

    //
    // Open key
    //
    HRESULT Open(
        IN DWORD   dwFlags,
        IN LPCTSTR lpszMDPath       = NULL,
        IN METADATA_HANDLE hkBase   = METADATA_MASTER_ROOT_HANDLE 
        );

    //
    // Re-open previously opened key
    //
    HRESULT ReOpen(
        IN DWORD   dwFlags
        );

    //
    // As above using the same permissions as before
    //
    HRESULT ReOpen();

    //
    // Open the parent object
    // 
    HRESULT ConvertToParentPath(
        IN  BOOL fImmediate
        );

    //
    // Close key, set it to NULL, but doesn't destroy the interface
    //
    HRESULT Close();

    //
    // Add key
    //
    HRESULT AddKey(
        IN LPCTSTR lpszMDPath
        );

    //
    // Delete key off currently open key
    //
    HRESULT DeleteKey(
        IN LPCTSTR lpszMDPath
        );

    //
    // Rename key off currently open key
    //
    HRESULT RenameKey(
        IN LPCTSTR lpszMDPath,
        IN LPCTSTR lpszNewName
        );

    //
    // Get list of descendant nodes that override
    // a specific value
    //
    HRESULT GetDataPaths( 
        OUT CStringListEx & strlNodes,
        IN  DWORD dwMDIdentifier,
        IN  DWORD dwMDDataType,
        IN  LPCTSTR lpszMDPath = NULL
        );


//
// Access
//
public:
    METADATA_HANDLE GetHandle() const { return m_hKey; }
    METADATA_HANDLE GetBase() const   { return m_hBase; }
    LPCTSTR QueryMetaPath() const     { return m_strMetaPath; }
    DWORD QueryFlags() const          { return m_dwFlags; }
    BOOL IsOpen() const               { return m_hKey != NULL; }
    BOOL IsHomeDirectoryPath() const ;

//
// Conversion operators
//
public:
    operator METADATA_HANDLE() const  { return GetHandle(); }
    operator LPCTSTR() const          { return QueryMetaPath(); }
    operator BOOL() const             { return IsOpen(); }

//
// Virtual Interface:
//
public:
    virtual BOOL Succeeded() const;
    virtual HRESULT QueryResult() const;

//
// Protected members
//
protected:
    //
    // Get data
    //
    HRESULT GetPropertyValue(
        IN  DWORD dwID,
        OUT IN DWORD & dwSize,
        OUT IN void *& pvData,
        OUT IN DWORD * pdwDataType           = NULL,
        IN  BOOL * pfInheritanceOverride     = NULL,
        IN  LPCTSTR lpszMDPath               = NULL,
        OUT DWORD * pdwAttributes            = NULL
        );

    //
    // Store data
    //
    HRESULT SetPropertyValue(
        IN DWORD dwID,
        IN DWORD dwSize,
        IN void * pvData,
        IN BOOL * pfInheritanceOverride = NULL,
        IN LPCTSTR lpszMDPath           = NULL
        );

    //
    // Get All Data off the open key
    //
    HRESULT GetAllData(
        IN  DWORD dwMDAttributes,
        IN  DWORD dwMDUserType,
        IN  DWORD dwMDDataType,
        OUT DWORD * pdwMDNumEntries,
        OUT DWORD * pdwMDDataLen,
        OUT PBYTE * ppbMDData,
        IN  LPCTSTR lpszMDPath  = NULL
        );

//
// Property Table Methods
//
protected:
    //
    // Metabase table entry definition
    //
    typedef struct tagMDFIELDDEF
    {
        DWORD dwMDIdentifier;
        DWORD dwMDAttributes;
        DWORD dwMDUserType;
        DWORD dwMDDataType;
        UINT  uStringID;
    } MDFIELDDEF;

    static const MDFIELDDEF s_rgMetaTable[];

//
// CODEWORK: Ideally, these should be protected, but are used
//           by idlg.
//
public:
    static BOOL GetMDFieldDef(
        DWORD dwID,
        DWORD & dwMDIdentifier,
        DWORD & dwMDAttributes,
        DWORD & dwMDUserType,
        DWORD & dwMDDataType
        );

    //
    // Map metabase ID value to table index
    //
    static int MapMDIDToTableIndex(DWORD dwID);

//
// Allow limited access to the table
//
public:
    static BOOL IsPropertyInheritable(DWORD dwID);
    static BOOL GetPropertyDescription(DWORD dwID, CString & strName);

protected:
    BOOL    m_fAllowRootOperations;
    BOOL    m_fOwnKey;
    DWORD   m_cbInitialBufferSize;    
    DWORD   m_dwFlags;
    HRESULT m_hrKey;
    CString m_strMetaPath;
    METADATA_HANDLE m_hKey;
    METADATA_HANDLE m_hBase;
};



class _EXPORT CWamInterface : public CIISInterface
/*++

Class description:

    WAM interface class

Virtual Interface:

    Succeeded           : Return TRUE if item constructed successfully
    QueryResult         : Return construction error code

Public Interface:

    SupportsPooledProc  : Check to see if pooled out of proc is supported.

--*/
{
//
// App Protection States:
//
public:
    enum
    {
        //
        // Note: order must match MD_APP_ISOLATED values
        //
        APP_INPROC,
        APP_OUTOFPROC,
        APP_POOLEDPROC,
    };

//
// Constructor/Destructor
//
public:
    //
    // Destructor destroys the interface
    //
    virtual ~CWamInterface();

protected:
    //
    // Fully defined constructor that creates the interface.
    // Use NULL to create the interface on the local computer
    //
    CWamInterface(
        IN CComAuthInfo * pServer
        );

    //
    // Construct from existing interface.  
    //
    CWamInterface(
        IN CWamInterface * pInterface
        );

//
// Access
//
public:
    BOOL SupportsPooledProc() const { return m_fSupportsPooledProc; }

protected:
    virtual HRESULT ApplyProxyBlanket();

    //
    // Create a wam object in this server. This function initializes the
    // object with DCOM.
    //
    HRESULT Create();

    //
    // Make sure the interface has been created
    //
    BOOL HasInterface() const { return m_pInterface != NULL; }

//
// IWAM Interface
//
protected:
    HRESULT AppCreate( 
        IN LPCTSTR szMDPath,
        IN DWORD dwAppProtection
        );
    
    HRESULT AppDelete( 
        IN LPCTSTR szMDPath,
        IN BOOL fRecursive
        );
    
    HRESULT AppUnLoad( 
        IN LPCTSTR szMDPath,
        IN BOOL fRecursive
        );
    
    HRESULT AppGetStatus( 
        IN LPCTSTR szMDPath,
        OUT DWORD * pdwAppStatus
        );
    
    HRESULT AppDeleteRecoverable( 
        IN LPCTSTR szMDPath,
        IN BOOL fRecursive
        );
    
    HRESULT AppRecover( 
        IN LPCTSTR szMDPath,
        IN BOOL fRecursive
        );

protected:
    IWamAdmin * m_pInterface; 

private:
    BOOL m_fSupportsPooledProc;
};



class _EXPORT CMetaBack : public CMetaInterface, public CWamInterface
/*++

Class Description:

    Metabase backup/restore class

Virtual Interface:

    Succeeded           : Return TRUE if item constructed successfully
    QueryResult         : Return construction error code

Public Interface:

    Reset               : Enum first existing backup
    Next                : Enum next existing backup
    Backup              : Create new backup
    Delete              : Delete existing backup
    Restore             : Restore from existing backup

--*/
{
public:
    //
    // Construct and create the interfaces.  Use NULL to create
    // on the local computer.
    //
    CMetaBack(
        IN CComAuthInfo * pServer
        );

//
// Virtual Interface
//
public:
    virtual BOOL Succeeded() const;
    virtual HRESULT QueryResult() const;

//
// Interface
//
public:
    //
    // Reset counter
    //
    void Reset() { m_dwIndex = 0L; }

    HRESULT Next(
        OUT DWORD * pdwVersion,
        OUT LPTSTR lpszLocation,
        OUT FILETIME * pftBackupTime
        );

    HRESULT Backup(
        IN LPCTSTR lpszLocation
        );

    HRESULT Delete(
        IN LPCTSTR lpszLocation,
        IN DWORD dwVersion
        );

    HRESULT Restore(
        IN LPCTSTR lpszLocation,
        IN DWORD dwVersion
        );

protected:
    virtual HRESULT ApplyProxyBlanket();

protected:
    static const LPCTSTR s_szMasterAppRoot;

private:
    DWORD m_dwIndex;
};



class _EXPORT CMetaEnumerator : public CMetaKey
/*++

Class Description:

    Metabase key enumerator

Public Interface:

    CMetaEnumerator     : Constructor
    
    Reset               : Reset the enumerator
    Next                : Get next key

--*/
{
public:
    //
    // Constructor creates a new interface and opens a key
    //
    CMetaEnumerator(
        IN CComAuthInfo * pServer,
        IN LPCTSTR lpszMDPath     = NULL,
        IN METADATA_HANDLE hkBase = METADATA_MASTER_ROOT_HANDLE
        );

    //
    // Constructor which uses an existing interface and opens
    // a new key
    //
    CMetaEnumerator(
        IN CMetaInterface * pInterface,
        IN LPCTSTR lpszMDPath     = NULL,
        IN METADATA_HANDLE hkBase = METADATA_MASTER_ROOT_HANDLE
        );

    //
    // Constructor which uses an open key
    //
    CMetaEnumerator(
        IN BOOL fOwnKey,
        IN CMetaKey * pKey
        );

//
// Interface:
//
public:
    //
    // Reset counter
    //
    void Reset(DWORD counter = 0L) { m_dwIndex = counter; }
    DWORD GetIndex() { return m_dwIndex; }
    // Index stack operators, used for recursive enums
    void Push()
    {
       m_stack.push(m_dwIndex);
    }
    void Pop()
    {
       ASSERT(!m_stack.empty());
       m_dwIndex = m_stack.top();
       m_stack.pop();
    }

    //
    // Get next key as string.
    //
    HRESULT Next(
        OUT CString & strKey,
        IN  LPCTSTR lpszMDPath = NULL
        );

    //
    // Get next key as a DWORD (numeric keys only)
    //
    HRESULT Next(
        OUT DWORD & dwKey,
        OUT CString & strKey,
        IN  LPCTSTR lpszMDPath = NULL
        );

private:
    DWORD m_dwIndex;
    std::stack<DWORD> m_stack;
};



class _EXPORT CIISApplication : public CWamInterface, public CMetaKey
/*++

Class Description:

    IIS Application class

Virtual Interface:

    Succeeded           : Return TRUE if item constructed successfully
    QueryResult         : Return construction error code

Public Interface:

    RefreshState        : Refresh application state
    QueryAppState       : Return current application state
    IsEnabledApplication: Return TRUE if appplication is enabled. 
    Create              : Create app
    Delete              : Delete app
    Unload              : Unload app
    DeleteRecoverable   : Delete w. recovery allowed
    Recover             : Recover
    WriteFriendlyName   : Write friendly name to metabase

--*/
{
//
// Constructor/Destructor
//
public:
    CIISApplication(
        IN CComAuthInfo * pServer,
        IN LPCTSTR lpszMetapath
        );

//
// Virtual Interface:
//
public:
    virtual BOOL Succeeded() const;
    virtual HRESULT QueryResult() const;

//
// Interface
//
public:
    DWORD   QueryAppState() const { return m_dwAppState; }
    LPCTSTR QueryWamPath() const { return m_strWamPath; }
    BOOL    IsEnabledApplication() const;
    HRESULT RefreshAppState();
    HRESULT Create(LPCTSTR lpszName, DWORD dwAppProtection);
    HRESULT Delete(BOOL fRecursive = FALSE);
    HRESULT Unload(BOOL fRecursive = FALSE);
    HRESULT DeleteRecoverable(BOOL fRecursive = FALSE);
    HRESULT Recover(BOOL fRecursive = FALSE);
    HRESULT WriteFriendlyName(LPCTSTR lpszName);

public:
    BOOL IsInproc() const { return m_dwProcessProtection == APP_INPROC; }
    BOOL IsOutOfProc() const { return m_dwProcessProtection == APP_OUTOFPROC; }
    BOOL IsPooledProc() const { return m_dwProcessProtection == APP_POOLEDPROC; }

public:
    DWORD   m_dwProcessProtection;
    CString m_strFriendlyName;
    CString m_strAppRoot;

protected:
    void CommonConstruct();

private:
    DWORD   m_dwAppState;
    CString m_strWamPath;
    HRESULT m_hrApp;
};



class _EXPORT CIISSvcControl : public CIISInterface
/*++

Class description:

    IIS Service control

Virtual Interface:

    Succeeded           : Return TRUE if item constructed successfully
    QueryResult         : Return construction error code

--*/
{
//
// Constructor/Destructor
//
public:
    //
    // Fully defined constructor that creates the interface.
    // Use NULL to create the interface on the local computer
    //
    CIISSvcControl(
        IN CComAuthInfo * pServer
        );

    //
    // Construct from existing interface.  
    //
    CIISSvcControl(
        IN CIISSvcControl * pInterface
        );

    //
    // Destructor destroys the interface
    //
    virtual ~CIISSvcControl();

protected:
    //
    // Create an object in this server. This function initializes the
    // object with DCOM.
    //
    HRESULT Create();

    //
    // Make sure the interface has been created
    //
    BOOL HasInterface() const { return m_pInterface != NULL; }

//
// Interface
//
public:
    //
    // Stop services
    //
    HRESULT Stop(
        IN DWORD dwTimeoutMsecs,
        IN BOOL fForce
        );

    //
    // Start services
    //
    HRESULT Start(
        IN DWORD dwTimeoutMsecs
        );

    //
    // Reboot
    //
    HRESULT Reboot(
        IN DWORD dwTimeouMsecs,
        IN BOOL fForceAppsClosed
        );

    //
    // Status
    //
    HRESULT Status(
        IN  DWORD dwBufferSize,
        OUT LPBYTE pbBuffer,
        OUT DWORD * MDRequiredBufferSize,
        OUT DWORD * pdwNumServices
        );

    //
    // Kill
    //
    HRESULT Kill();

protected:
    virtual HRESULT ApplyProxyBlanket();

protected:
    IIisServiceControl * m_pInterface; 
};


#ifdef KEVLAR
class _EXPORT CWebCluster : public CIISInterface
/*++

Class description:

    IWebCluster warpper

Virtual Interface:

    Succeeded           : Return TRUE if item constructed successfully
    QueryResult         : Return construction error code

--*/
{
//
// Constructor/Destructor
//
public:
    //
    // Fully defined constructor that creates the interface.
    // Use NULL to create the interface on the local computer
    //
    CWebCluster(
        CComAuthInfo * pServer
        );

    //
    // Destructor destroys the interface
    //
    virtual ~CWebCluster();

protected:
    //
    // Create an object in this server. This function initializes the
    // object with DCOM.
    //
    HRESULT Create();

    //
    // Make sure the interface has been created
    //
    BOOL HasInterface() const { return m_pInterface != NULL; }

//
// Interface
//
public:
#if (0) // dantra: 8/17/99 legacy code, not supported by new IWebCluster interfaces
    HRESULT GetParameter( 
        LONG lParamId,
        BSTR bstrParamInfo,
        VARIANT * lpvarParam
        );
    
    HRESULT SetParameter( 
        LONG lParam,
        BSTR bstrParamInfo,
        VARIANT * lpvarResults
        );
#endif
protected:
    virtual HRESULT ApplyProxyBlanket();

protected:
    IWebCluster * m_pInterface; 
};
#endif


//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline void CComAuthInfo::StorePassword(LPCOLESTR lpszPassword)
{
    m_bstrPassword = lpszPassword;
}

inline /* virtual */ HRESULT CIISInterface::ChangeProxyBlanket(
    IN LPCOLESTR lpszUserName, 
    IN LPCOLESTR lpszPassword
    )
{
    m_auth.SetImpersonation(lpszUserName, lpszPassword);
    return ApplyProxyBlanket();
}

inline /*static */ LPCTSTR CMetabasePath::GetMachinePath(
    IN  LPCTSTR lpszMDPath,
    OUT CString & strNewPath,
    OUT CString * pstrRemainder
    )
{
    return TruncatePath(iMachine, lpszMDPath, strNewPath, pstrRemainder);
}

inline /* static */ LPCTSTR CMetabasePath::GetServicePath(
    IN  LPCTSTR lpszMDPath,
    OUT CString & strNewPath,
    OUT CString * pstrRemainder
    )
{
    return TruncatePath(iService, lpszMDPath, strNewPath, pstrRemainder);
}

inline /* static */ LPCTSTR CMetabasePath::GetInstancePath(
    IN  LPCTSTR lpszMDPath,
    OUT CString & strNewPath,
    OUT CString * pstrRemainder
    )
{
    return TruncatePath(iInstance, lpszMDPath, strNewPath, pstrRemainder);
}

inline /* static */ LPCTSTR CMetabasePath::GetRootPath(
    IN  LPCTSTR lpszMDPath,
    OUT CString & strNewPath,
    OUT CString * pstrRemainder
    )
{
    return TruncatePath(iRootDirectory, lpszMDPath, strNewPath, pstrRemainder);
}

inline /* virtual */ HRESULT CMetaInterface::ApplyProxyBlanket()
{
    return m_auth.ApplyProxyBlanket(m_pInterface);
}

inline HRESULT CMetaInterface::OpenKey(
    IN  METADATA_HANDLE hkBase,
    IN  LPCTSTR lpszMDPath,
    IN  DWORD dwFlags,
    OUT METADATA_HANDLE * phMDNewHandle
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->OpenKey(
        hkBase,
        lpszMDPath,
        dwFlags,
        m_iTimeOutValue,
        phMDNewHandle
        );
}

inline HRESULT CMetaInterface::CloseKey(
    IN METADATA_HANDLE hKey
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->CloseKey(hKey);
}

inline HRESULT CMetaInterface::SetLastChangeTime( 
    IN METADATA_HANDLE hMDHandle,
    IN LPCTSTR pszMDPath,
    IN FILETIME * pftMDLastChangeTime,
    IN BOOL bLocalTime
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->SetLastChangeTime(
        hMDHandle,
        pszMDPath,
        pftMDLastChangeTime,
        bLocalTime
        );
}
        
inline HRESULT CMetaInterface::GetLastChangeTime( 
    IN  METADATA_HANDLE hMDHandle,
    IN  LPCTSTR lpszMDPath,
    OUT FILETIME * pftMDLastChangeTime,
    IN  BOOL bLocalTime
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->GetLastChangeTime(
        hMDHandle,
        lpszMDPath,
        pftMDLastChangeTime,
        bLocalTime
        );
}

inline HRESULT CMetaInterface::AddKey( 
    IN METADATA_HANDLE hKey,
    IN LPCTSTR lpszMDPath
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->AddKey(hKey, lpszMDPath);
}

inline HRESULT CMetaInterface::DeleteKey(
    IN METADATA_HANDLE hKey,
    IN LPCTSTR lpszMDPath
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->DeleteKey(hKey, lpszMDPath);
}

inline HRESULT CMetaInterface::DeleteChildKeys(
    IN METADATA_HANDLE hKey,
    IN LPCTSTR lpszMDPath
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->DeleteChildKeys(hKey, lpszMDPath);
}

inline HRESULT CMetaInterface::EnumKeys(
    IN  METADATA_HANDLE hKey,
    IN  LPCTSTR lpszMDPath,
    OUT LPTSTR lpszMDName,
    IN  DWORD dwIndex
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->EnumKeys(hKey, lpszMDPath, lpszMDName, dwIndex);
}        

inline HRESULT CMetaInterface::CopyKey(
    IN METADATA_HANDLE hSourceKey,
    IN LPCTSTR lpszMDSourcePath,
    IN METADATA_HANDLE hDestKey,
    IN LPCTSTR lpszMDDestPath,
    IN BOOL fOverwrite,
    IN BOOL fCopy
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->CopyKey(
        hSourceKey,
        lpszMDSourcePath,
        hDestKey,
        lpszMDDestPath,
        fOverwrite,
        fCopy
        );        
}

inline HRESULT CMetaInterface::RenameKey(
    IN METADATA_HANDLE hKey,
    IN LPCTSTR lpszMDPath,
    IN LPCTSTR lpszNewName
    )
{   
    ASSERT_PTR(m_pInterface);     
    return m_pInterface->RenameKey(hKey, lpszMDPath, lpszNewName);
}

inline HRESULT CMetaInterface::GetData(
    IN  METADATA_HANDLE hKey,
    IN  LPCTSTR lpszMDPath,
    OUT METADATA_RECORD * pmdRecord,
    OUT DWORD * pdwRequiredDataLen
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->GetData(
        hKey,
        lpszMDPath,
        pmdRecord,
        pdwRequiredDataLen
        );
}

inline HRESULT CMetaInterface::SetData(
    IN METADATA_HANDLE hKey,
    IN LPCTSTR lpszMDPath,
    IN METADATA_RECORD * pmdRecord
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->SetData(
        hKey,
        lpszMDPath,
        pmdRecord
        );
}

inline HRESULT CMetaInterface::DeleteData(
    IN METADATA_HANDLE hKey,
    IN LPCTSTR lpszMDPath,
    IN DWORD dwMDIdentifier,
    IN DWORD dwMDDataType
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->DeleteData(
        hKey,
        lpszMDPath,
        dwMDIdentifier,
        dwMDDataType
        );
}

inline HRESULT CMetaInterface::EnumData(
    IN  METADATA_HANDLE hKey,
    IN  LPCTSTR lpszMDPath,
    OUT METADATA_RECORD * pmdRecord,
    IN  DWORD dwIndex,
    OUT DWORD * pdwRequiredDataLen
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->EnumData(
        hKey,
        lpszMDPath,
        pmdRecord,
        dwIndex,
        pdwRequiredDataLen
        );
}

inline HRESULT CMetaInterface::GetAllData(
    IN  METADATA_HANDLE hKey,
    IN  LPCTSTR lpszMDPath,
    IN  DWORD dwMDAttributes,
    IN  DWORD dwMDUserType,
    IN  DWORD dwMDDataType,
    OUT DWORD * pdwMDNumDataEntries,
    OUT DWORD * pdwMDDataSetNumber,
    IN  DWORD dwMDBufferSize,
    OUT LPBYTE pbMDBuffer,
    OUT DWORD * pdwRequiredBufferSize
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->GetAllData(
        hKey,
        lpszMDPath,
        dwMDAttributes,
        dwMDUserType,
        dwMDDataType,
        pdwMDNumDataEntries,
        pdwMDDataSetNumber,
        dwMDBufferSize,
        pbMDBuffer,
        pdwRequiredBufferSize
        );
}    

inline HRESULT CMetaInterface::DeleteAllData( 
    IN METADATA_HANDLE hKey,
    IN LPCTSTR lpszMDPath,
    IN DWORD dwMDUserType,
    IN DWORD dwMDDataType
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->DeleteAllData(
        hKey, 
        lpszMDPath, 
        dwMDUserType, 
        dwMDDataType
        );
}

inline HRESULT CMetaInterface::CopyData( 
    IN METADATA_HANDLE hMDSourceKey,
    IN LPCTSTR lpszMDSourcePath,
    IN METADATA_HANDLE hMDDestKey,
    IN LPCTSTR lpszMDDestPath,
    IN DWORD dwMDAttributes,
    IN DWORD dwMDUserType,
    IN DWORD dwMDDataType,
    IN BOOL fCopy
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->CopyData(
        hMDSourceKey,
        lpszMDSourcePath,
        hMDDestKey,
        lpszMDDestPath,
        dwMDAttributes,
        dwMDUserType,
        dwMDDataType,
        fCopy
        );
}

inline HRESULT CMetaInterface::GetDataPaths( 
    IN  METADATA_HANDLE hKey,
    IN  LPCTSTR lpszMDPath,
    IN  DWORD dwMDIdentifier,
    IN  DWORD dwMDDataType,
    IN  DWORD dwMDBufferSize,
    OUT LPTSTR lpszBuffer,
    OUT DWORD * pdwMDRequiredBufferSize
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->GetDataPaths(
        hKey,
        lpszMDPath,
        dwMDIdentifier,
        dwMDDataType,
        dwMDBufferSize,
        lpszBuffer,
        pdwMDRequiredBufferSize
        );
}

inline HRESULT CMetaInterface::Backup( 
    IN LPCTSTR lpszBackupLocation,
    IN DWORD dwMDVersion,
    IN DWORD dwMDFlags
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->Backup(lpszBackupLocation, dwMDVersion, dwMDFlags);
}

inline HRESULT CMetaInterface::Restore(    
    IN LPCTSTR lpszBackupLocation,
    IN DWORD dwMDVersion,
    IN DWORD dwMDFlags
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->Restore(lpszBackupLocation, dwMDVersion, dwMDFlags);
}

inline HRESULT CMetaInterface::EnumBackups(
    OUT LPTSTR lpszBackupLocation,
    OUT DWORD * pdwMDVersion,
    OUT FILETIME * pftMDBackupTime,
    IN  DWORD dwIndex
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->EnumBackups(
        lpszBackupLocation,
        pdwMDVersion,
        pftMDBackupTime,
        dwIndex
        );    
}

inline HRESULT CMetaInterface::DeleteBackup(
    IN LPCTSTR lpszBackupLocation,
    IN DWORD dwMDVersion
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->DeleteBackup(lpszBackupLocation, dwMDVersion);
}        

inline HRESULT CMetaInterface::SaveData()
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->SaveData();
}

inline HRESULT CMetaKey::AddKey(
    IN LPCTSTR lpszMDPath
    )
{
    return CMetaInterface::AddKey(m_hKey, lpszMDPath);    
}

inline HRESULT CMetaKey::DeleteKey(
    IN LPCTSTR lpszMDPath
    )
{
    return CMetaInterface::DeleteKey(m_hKey, lpszMDPath);    
}

inline HRESULT CMetaKey::RenameKey(
    IN LPCTSTR lpszMDPath,
    IN LPCTSTR lpszNewName
    )
{
    return CMetaInterface::RenameKey(m_hKey, lpszMDPath, lpszNewName);    
}

inline HRESULT CMetaKey::ReOpen()
{
    return Open(m_dwFlags, m_strMetaPath, m_hBase);
}

inline HRESULT CMetaKey::ReOpen(DWORD dwFlags)
{
    return Open(dwFlags, m_strMetaPath, m_hBase);
}

inline BOOL CMetaKey::IsHomeDirectoryPath() const
{ 
    return CMetabasePath::IsHomeDirectoryPath(m_strMetaPath); 
}

inline HRESULT CMetaKey::QueryValue(
    IN  DWORD dwID, 
    OUT BOOL & fValue,
    IN  BOOL * pfInheritanceOverride,
    IN  LPCTSTR lpszMDPath,
    OUT DWORD * pdwAttributes        
    )
{
    ASSERT(sizeof(DWORD) == sizeof(BOOL));
    return CMetaKey::QueryValue(
        dwID, 
        (DWORD &)fValue, 
        pfInheritanceOverride, 
        lpszMDPath,
        pdwAttributes
        );
}

inline HRESULT CMetaKey::SetValue(
    IN DWORD dwID,
    IN DWORD dwValue,
    IN BOOL * pfInheritanceOverride,    OPTIONAL
    IN LPCTSTR lpszMDPath               OPTIONAL
    )
{
    return SetPropertyValue(
        dwID, 
        sizeof(dwValue), 
        &dwValue, 
        pfInheritanceOverride,
        lpszMDPath
        );
}

inline HRESULT CMetaKey::SetValue(
    IN DWORD dwID,
    IN BOOL fValue,
    IN BOOL * pfInheritanceOverride,
    IN LPCTSTR lpszMDPath
    )
{
    ASSERT(sizeof(DWORD) == sizeof(BOOL));
    return CMetaKey::SetValue(
        dwID,
        (DWORD)fValue,
        pfInheritanceOverride,
        lpszMDPath
        );
}

inline HRESULT CMetaKey::SetValue(
    IN DWORD dwID,
    IN CString & strValue,
    IN BOOL * pfInheritanceOverride,    OPTIONAL
    IN LPCTSTR lpszMDPath               OPTIONAL
    )
{
    return SetPropertyValue(
        dwID,
        (strValue.GetLength() + 1) * sizeof(TCHAR),
        (void *)(LPCTSTR)strValue,
        pfInheritanceOverride,
        lpszMDPath
        );
}

inline HRESULT CWamInterface::AppDelete( 
    IN LPCTSTR szMDPath,
    IN BOOL fRecursive
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->AppDelete(szMDPath, fRecursive);
}

inline HRESULT CWamInterface::AppUnLoad( 
    IN LPCTSTR szMDPath,
    IN BOOL fRecursive
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->AppUnLoad(szMDPath, fRecursive);
}

inline HRESULT CWamInterface::AppGetStatus( 
    IN  LPCTSTR szMDPath,
    OUT DWORD * pdwAppStatus
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->AppGetStatus(szMDPath, pdwAppStatus);
}

inline HRESULT CWamInterface::AppDeleteRecoverable( 
    IN LPCTSTR szMDPath,
    IN BOOL fRecursive
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->AppDeleteRecoverable(szMDPath, fRecursive);
}

inline HRESULT CWamInterface::AppRecover( 
    IN LPCTSTR szMDPath,
    IN BOOL fRecursive
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->AppRecover(szMDPath, fRecursive);
}

inline /* virtual */ HRESULT CWamInterface::ApplyProxyBlanket()
{
    return m_auth.ApplyProxyBlanket(m_pInterface);
}

inline /* virtual */ HRESULT CMetaBack::ApplyProxyBlanket()
{
    HRESULT hr = CMetaInterface::ApplyProxyBlanket();
    return SUCCEEDED(hr) ? CWamInterface::ApplyProxyBlanket() : hr;
}

inline HRESULT CMetaBack::Next(
    OUT DWORD * pdwVersion,
    OUT LPTSTR lpszLocation,
    OUT FILETIME * pftBackupTime
    )
{
    return EnumBackups(
        lpszLocation,
        pdwVersion,
        pftBackupTime,
        m_dwIndex++
        );
}

inline HRESULT CMetaBack::Backup(
    IN LPCTSTR lpszLocation
    )
{
    return CMetaInterface::Backup(
        lpszLocation, 
        MD_BACKUP_NEXT_VERSION, 
        MD_BACKUP_SAVE_FIRST
        );
}

inline HRESULT CMetaBack::Delete(
    IN LPCTSTR lpszLocation,
    IN DWORD dwVersion
    )
{
    return DeleteBackup(lpszLocation, dwVersion);
}

inline BOOL 
CIISApplication::IsEnabledApplication() const
{
    return m_dwAppState == APPSTATUS_STOPPED 
        || m_dwAppState == APPSTATUS_RUNNING;
}

inline HRESULT CIISApplication::Delete(
    IN BOOL fRecursive
    )
{
    ASSERT(!m_strWamPath.IsEmpty());
    return AppDelete(m_strWamPath, fRecursive);
}

inline HRESULT CIISApplication::Unload(
    IN BOOL fRecursive
    )
{
    ASSERT(!m_strWamPath.IsEmpty());
    return AppUnLoad(m_strWamPath, fRecursive);
}

inline HRESULT CIISApplication::DeleteRecoverable(
    IN BOOL fRecursive
    )
{
    ASSERT(!m_strWamPath.IsEmpty());
    return AppDeleteRecoverable(m_strWamPath, fRecursive);
}

inline HRESULT CIISApplication::Recover(
    IN BOOL fRecursive
    )
{
    ASSERT(!m_strWamPath.IsEmpty());
    return AppRecover(m_strWamPath, fRecursive);
}

inline HRESULT CIISSvcControl::Stop(
    IN DWORD dwTimeoutMsecs,
    IN BOOL fForce
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->Stop(dwTimeoutMsecs, (DWORD)fForce);
}

inline HRESULT CIISSvcControl::Start(
    IN DWORD dwTimeoutMsecs
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->Start(dwTimeoutMsecs);
}

inline HRESULT CIISSvcControl::Reboot(
    IN DWORD dwTimeouMsecs,
    IN BOOL fForceAppsClosed
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->Reboot(dwTimeouMsecs, (DWORD)fForceAppsClosed);
}

inline HRESULT CIISSvcControl::Status(
    IN  DWORD dwBufferSize,
    OUT LPBYTE pbBuffer,
    OUT DWORD * MDRequiredBufferSize,
    OUT DWORD * pdwNumServices
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->Status(
        dwBufferSize, 
        pbBuffer,
        MDRequiredBufferSize,
        pdwNumServices
        );
}

inline HRESULT CIISSvcControl::Kill()
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->Kill();
}

inline /* virtual */ HRESULT CIISSvcControl::ApplyProxyBlanket()
{
    return m_auth.ApplyProxyBlanket(m_pInterface);
}

#ifdef KEVLAR
inline HRESULT CWebCluster::Create()
{
    return CIISInterface::Create(
        1,
        &IID_IWebCluster, 
        &CLSID_WebCluster, 
        NULL, 
        (IUnknown **)&m_pInterface
        );
}

inline /* virtual */ HRESULT CWebCluster::ApplyProxyBlanket()
{
    return m_auth.ApplyProxyBlanket(m_pInterface);
}
#if (0)  // dantra: 8/17/99 - legacy
inline HRESULT CWebCluster::GetParameter( 
    LONG lParamId,
    BSTR bstrParamInfo,
    VARIANT * lpvarParam
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->GetParameter(lParamId, bstrParamInfo, lpvarParam);
}
    
inline HRESULT CWebCluster::SetParameter( 
    LONG lParam,
    BSTR bstrParamInfo,
    VARIANT * lpvarResults
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->SetParameter(lParam, bstrParamInfo, lpvarResults);
}

#endif // 0
#endif // KEVLAR

#endif // _MDKEYS_H_

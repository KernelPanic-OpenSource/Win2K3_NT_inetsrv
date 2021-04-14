/*
    IisRsta.cxx

    Command line utility for access to IIisServiceControl

    FILE HISTORY:
        Phillich    06-Oct-1998     Created

*/

#define INITGUID

#include    <windows.h>
#include    <winnlsp.h>
#include    <tchar.h>
#include    <stdio.h>
#include    <ole2.h>
#include    <locale.h>
#include    "iisrsta.h"
#include    "iisrstam.h"

// Including so we can use the WIN32_FROM_HRESULT macro that is defined there
#include    "iisdef.h"

typedef BOOL (*PFNCHANGESERVICECONFIG2)(SC_HANDLE,DWORD,LPVOID);

#define MAX_STRINGIZED_ULONG_CHAR_COUNT 11      // "4294967295", including the terminating null


//
// Helper functions
//


VOID
DisplayErrorMessage(
    DWORD   dwErr
    )
/*++

    DisplayErrorMessage

        Display error message associated with error code

    Arguments:

        dwErr  - Win32 error code

    Returns:

        Nothing

--*/
{
    LPTSTR  pErr;

    if ( FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            dwErr,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&pErr,
            0,
            NULL ) )
    {
        LPTSTR   p;

        p = _tcschr( pErr, TEXT('\r') );
        if ( p != NULL )
        {
            *p = TEXT('\0');
        }
        _fputts( pErr, stdout );

        LocalFree( pErr );
    }
}


VOID
PrintMessage(
    DWORD   dwId,
    DWORD   dwParams,
    LPVOID* pParams
    )
/*++

    PrintMessage

        Print message ( from message file ) with optional parameters

    Arguments:

        dwId - message ID
        dwParams - # of params in pParams
        pParams - ptr to array of parameters

    Returns:

        Nothing

--*/
{
    LPTSTR  pBuf;

    if ( FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_HMODULE|FORMAT_MESSAGE_ARGUMENT_ARRAY,
                        (LPCVOID)NULL,  //GetModuleHandle(NULL),
                        dwId,
                        0,
                        (LPTSTR)&pBuf,
                        dwParams,
                        (va_list *)pParams ) )
    {
        _fputts( pBuf, stdout );

        LocalFree( pBuf );
    }
}


VOID
CmdError( 
    DWORD   dwId,
    HRESULT hRes 
    )
/*++

    CmdError

        Display message followed by error description ( error message + numerical code )

    Arguments:

        dwId - message ID
        hRes - error code

    Returns:

        Nothing

--*/
{
    TCHAR   achBuf[128];

    PrintMessage( dwId, 0, NULL );

    if ( hRes == HRESULT_FROM_WIN32( ERROR_RESOURCE_DISABLED ) )
    {
        PrintMessage( IRSTASTR_REMOTE_DISABLED, 0, NULL );
    }
    else if ( hRes == HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED ) )
    {
        PrintMessage( IRSTASTR_ACCESS_DENIED, 0, NULL );
    }
    else if ( hRes == HRESULT_FROM_WIN32( ERROR_SERVICE_NOT_ACTIVE ) )
    {
        PrintMessage( IRSTASTR_SERVICE_NOT_ACTIVE, 0, NULL );
    }
    else
    {
        DisplayErrorMessage( WIN32_FROM_HRESULT( hRes ) );
        wsprintf( achBuf, _T(" (%u, %x)\n"), (DWORD)hRes, (DWORD)hRes );
        _fputts( achBuf, stdout );
    }
}


LPTSTR
GetString(
    DWORD   dwId
    )
/*++

    GetString

        Retrieve message content

    Arguments:

        dwId - message ID

    Returns:

        Ptr to message. Must be freed using LocalFree()

--*/
{
    LPTSTR  pBuf;

    if ( FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_HMODULE,
                        (LPCVOID)NULL,
                        dwId,
                        0,
                        (LPTSTR)&pBuf,
                        0,
                        NULL ) )
    {
        LPTSTR   p;

        p = _tcschr( pBuf, TEXT('\r') );
        if ( p != NULL )
        {
            *p = TEXT('\0');
        }
        return pBuf;
    }

    return NULL;
}



HRESULT
DeserializeEnumServiceBuffer( 
    LPBYTE                          pbInBuffer,
    DWORD                           dwNumServices,
    LPBYTE                          pbBuffer,
    DWORD                           dwBufferSize,
    LPDWORD                         pdwMDRequiredBufferSize 
    )
/*++

    DeserializeEnumServiceBuffer

        Deserialize array of SERIALIZED_ENUM_SERVICE_STATUS to buffer,
        replacing offset in buffer by ptr

    Arguments:
        
        pbInBuffer - buffer containing serialized status as array of SERIALIZED_ENUM_SERVICE_STATUS
        dwNumServices - # of entries in pbInBuffer
        pbBuffer - buffer filled with deserialized status as array of ENUM_SERVICE_STATUS
        dwBufferSize - maximum size of pbBuffer
        pdwMDRequiredBufferSize - updated with required size if dwBufferSize too small

    Returns:
        ERROR_INSUFFICIENT_BUFFER if dwBufferSize too small
        otherwise COM status

--*/
{
    HRESULT                         hresReturn = S_OK;
    DWORD                           dwMinSize = 0;
    UINT                            i;
    SERIALIZED_ENUM_SERVICE_STATUS* pessDependentServices = (SERIALIZED_ENUM_SERVICE_STATUS*)pbInBuffer;

    if ( !pbBuffer )
    {
        dwBufferSize = 0;
    }

    dwMinSize = sizeof(ENUM_SERVICE_STATUS) * dwNumServices;

    for ( i = 0 ;
          i < dwNumServices ; 
          ++i )
    {
        UINT    cServiceName = (UINT) _tcslen( (TCHAR*)(pbInBuffer + pessDependentServices[i].iServiceName) ) + 1;
        UINT    cDisplayName = (UINT) _tcslen( (TCHAR*)(pbInBuffer + pessDependentServices[i].iDisplayName) ) + 1;

        if ( dwBufferSize >= dwMinSize + ( cServiceName + cDisplayName ) * sizeof(TCHAR) )
        {
            ((LPENUM_SERVICE_STATUS)pbBuffer)[i].ServiceStatus =
                    pessDependentServices[i].ServiceStatus;

            memcpy( pbBuffer + dwMinSize, pbInBuffer + pessDependentServices[i].iServiceName, cServiceName * sizeof(TCHAR) ); 
            ((LPENUM_SERVICE_STATUS)pbBuffer)[i].lpServiceName = (TCHAR*)(pbBuffer + dwMinSize);

            memcpy( pbBuffer + dwMinSize + cServiceName * sizeof(TCHAR), pbInBuffer + pessDependentServices[i].iDisplayName, cDisplayName * sizeof(TCHAR) );
            ((LPENUM_SERVICE_STATUS)pbBuffer)[i].lpDisplayName = (TCHAR*)(pbBuffer + dwMinSize) + cServiceName;
        }

        dwMinSize += ( cServiceName + cDisplayName ) * sizeof(TCHAR);
    }

    if ( dwBufferSize < dwMinSize )
    {
        *pdwMDRequiredBufferSize = dwMinSize;

        hresReturn = HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER );
    }

    return hresReturn;
}


HRESULT
SetEnableRemote(
    DWORD   dwValue
    )
/*++

    SetEnableRemote

        set restart I/F enabled flag in registry
        ( HKLM\SOFTWARE\Microsoft\INetStp::EnableRestart::REG_DWORD )

    Arguments:
        
        dwValue - 0 to disable I/F, !0 to enable

    Returns:
        status

--*/
{
    DWORD   dwSt = NO_ERROR;
    HKEY    hKey;

    
    //
    // Check admin privilege by accessing IISADMIN key for write
    //

    if ( ( dwSt = RegOpenKeyEx( HKEY_LOCAL_MACHINE, 
                                TEXT("SYSTEM\\CurrentControlSet\\Services\\IISADMIN"), 
                                0, 
                                KEY_WRITE, 
                                &hKey ) ) == ERROR_SUCCESS )
    {
        RegCloseKey( hKey );

        //
        // Set IISCTL interface access flag in registry
        //

        if ( ( dwSt = RegOpenKeyEx( HKEY_LOCAL_MACHINE, 
                                    TEXT("SOFTWARE\\Microsoft\\INetStp"), 
                                    0, 
                                    KEY_WRITE, 
                                    &hKey ) ) == ERROR_SUCCESS )
        {
            if ( ( dwSt = RegSetValueEx( hKey, 
                                         TEXT("EnableRestart"),
                                         0, 
                                         REG_DWORD, 
                                         (const BYTE*)&dwValue, 
                                         sizeof(DWORD) ) ) == ERROR_SUCCESS )
            {
            }

            RegCloseKey( hKey );
        }
    }

    return HRESULT_FROM_WIN32( dwSt );
}


BOOL
GetNumeric( 
    LPSTR   pszNumArg,
    LPDWORD pdwVal
    )
{
    // protect against being called wrong.
    if ( pszNumArg == NULL ||
         pdwVal == NULL )
    {
        return FALSE;
    }

    if ( !isdigit( (UCHAR)(*pszNumArg) ) )
    {
        return FALSE;
    }

    *pdwVal = atoi( pszNumArg );

    return TRUE;
}

BOOL 
IsIIS6orGreater(
    )
/*++

    IsIIS6orGreater

        According to Aaron we are guaranteed to have
        the appropriate keys in the registry by the
        time iisreset /scm is called.

    Arguments:
        
        None

    Returns:
        TRUE = IIS 6 or greater;
        FALSE = anything below IIS 6;

--*/
{
    // Assume it is not IIS 6.
    BOOL IsIIS6 = FALSE;

    HKEY        hKey;
    DWORD       dwValue;
    DWORD       dwType;
    DWORD       dwSize;

    // Check the registry of the major version setup number.
    // If it is not there we assume it is not a 6.0 machine,
    // since it's better to setup a 6.0 machine in 5.0 state
    // instead of a 5.0 machine in 6.0 state.
    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE, 
                       TEXT("System\\CurrentControlSet\\Services\\W3SVC\\Parameters"), 
                       0, 
                       KEY_READ, 
                       &hKey ) == ERROR_SUCCESS )
    {
        if ( RegQueryValueEx( hKey, 
                              TEXT("MajorVersion"),
                              0, 
                              &dwType, 
                              (LPBYTE)&dwValue, 
                              &dwSize ) == ERROR_SUCCESS )
        {
            
            if ( dwType == REG_DWORD )
            {
                if ( dwValue >= 6 ) 
                {
                    // were are running on a system that does not
                    // have atleast IIS 6 on it

                    IsIIS6 = TRUE;
                }

            }

        }

        RegCloseKey( hKey );

    }


    return IsIIS6;

}

HRESULT
SetSCM(
	BOOL	fEnable
	)
/*++

    SetSCM

        set SCM Recovery configuration for IISADMIN service.
        Does nothing on pre Windows 2000 systems.

    Arguments:
        
        fEnable - TRUE to enable IISRESET invocation on service failure, FALSE to disable

    Returns:
        status

--*/
{
    PFNCHANGESERVICECONFIG2 pfnChangeServiceConfig2 = NULL;
    SERVICE_FAILURE_ACTIONS sfaAction;
    SC_ACTION               saCmdline[3];
    SC_HANDLE               schSCM;
    SC_HANDLE               schSrv;
    HRESULT                 hres = S_OK;
    TCHAR                   achModuleName[MAX_PATH];
    TCHAR                   achFailureCommand[MAX_PATH+32];
    HINSTANCE               hAdvapi;

    hAdvapi = LoadLibrary(_T("ADVAPI32.DLL"));
    if ( hAdvapi != NULL )
    {
        pfnChangeServiceConfig2 = (PFNCHANGESERVICECONFIG2)GetProcAddress( hAdvapi, "ChangeServiceConfig2W" );
    }

    if ( pfnChangeServiceConfig2 )
    {
        if ( !GetModuleFileName( NULL, 
                                 achModuleName, 
                                 sizeof(achModuleName)/sizeof(TCHAR) ) )
        {
            hres = HRESULT_FROM_WIN32( GetLastError() );
        }

        if ( SUCCEEDED( hres ) )
        {
            schSCM = OpenSCManager(NULL,
                                   NULL,
                                   SC_MANAGER_CONNECT);
            if ( schSCM == NULL )
	        {
                hres = HRESULT_FROM_WIN32( GetLastError() );
            }
            else 
	        {

                // We need to determine what IIS version we are running on
                // in order to know the correct way to setup the server.

                BOOL fIIS6orGreater = IsIIS6orGreater();

                //
                // In IIS 5.1 we want to use the default /restart 
                // setting of iisreset.  In IIS 6 or greater we
                // want to use the /start option.  A customer
                // can configure it to use the /restart option, but
                // that is not the default for the scm.
                //
                if ( fIIS6orGreater )
                {
                    //achModuleName is MAX_PATH size, the rest of the string is less than 32 characters.
                    wsprintf( achFailureCommand, _T("\"%s\" /start /fail=%%1%%"), achModuleName );
                }
                else
                {
                    //achModuleName is MAX_PATH size, the rest of the string is less than 32 characters.
                    wsprintf( achFailureCommand, _T("\"%s\" /fail=%%1%%"), achModuleName );
                }

                sfaAction.lpCommand = achFailureCommand;
                sfaAction.lpRebootMsg = _T("");
                sfaAction.dwResetPeriod = 24 * 60 * 60;

                if ( fEnable )
                {
                    sfaAction.cActions = 3;
                    sfaAction.lpsaActions = saCmdline;
                    saCmdline[0].Type = SC_ACTION_RUN_COMMAND;
                    saCmdline[0].Delay = 1;
                    saCmdline[1].Type = SC_ACTION_RUN_COMMAND;
                    saCmdline[1].Delay = 1;
                    saCmdline[2].Type = SC_ACTION_RUN_COMMAND;
                    saCmdline[2].Delay = 1;
                }
                else
                {
                    sfaAction.cActions = 3;
                    sfaAction.lpsaActions = saCmdline;
                    saCmdline[0].Type = SC_ACTION_NONE;
                    saCmdline[0].Delay = 0;
                    saCmdline[1].Type = SC_ACTION_NONE;
                    saCmdline[1].Delay = 0;
                    saCmdline[2].Type = SC_ACTION_NONE;
                    saCmdline[2].Delay = 0;
                }

                schSrv = OpenService( schSCM,
                                      _T("IISADMIN"),
                                      SERVICE_CHANGE_CONFIG);

                if ( schSrv )
                {
                    if ( !pfnChangeServiceConfig2( schSrv, 
                                                   SERVICE_CONFIG_FAILURE_ACTIONS, 
                                                   &sfaAction ) )
                    {
                        hres = HRESULT_FROM_WIN32( GetLastError() );
                    }

                    CloseServiceHandle( schSrv );
                }
                else
                {
                    hres = HRESULT_FROM_WIN32( GetLastError() );
                }

                //
                // Now that we have configured IISAdmin correctly, we also need to 
                // configure W3SVC.
                //

                if ( fIIS6orGreater )
                {

                    sfaAction.lpCommand = NULL;
                    sfaAction.lpRebootMsg = _T("");
                    sfaAction.dwResetPeriod = 24 * 60 * 60;

                    if ( fEnable )
                    {
                        sfaAction.cActions = 3;
                        sfaAction.lpsaActions = saCmdline;
                        saCmdline[0].Type = SC_ACTION_RESTART;
                        saCmdline[0].Delay = 1;
                        saCmdline[1].Type = SC_ACTION_RESTART;
                        saCmdline[1].Delay = 1;
                        saCmdline[2].Type = SC_ACTION_RESTART;
                        saCmdline[2].Delay = 1;
                    }
                    else
                    {
                        sfaAction.cActions = 3;
                        sfaAction.lpsaActions = saCmdline;
                        saCmdline[0].Type = SC_ACTION_NONE;
                        saCmdline[0].Delay = 0;
                        saCmdline[1].Type = SC_ACTION_NONE;
                        saCmdline[1].Delay = 0;
                        saCmdline[2].Type = SC_ACTION_NONE;
                        saCmdline[2].Delay = 0;
                    }

                    schSrv = OpenService( schSCM,
                                          _T("W3SVC"),
                                          SERVICE_CHANGE_CONFIG |
                                          SERVICE_START);

                    if ( schSrv )
                    {
                        if ( !pfnChangeServiceConfig2( schSrv, 
                                                       SERVICE_CONFIG_FAILURE_ACTIONS, 
                                                       &sfaAction ) )
                        {
                            hres = HRESULT_FROM_WIN32( GetLastError() );
                        }

                        CloseServiceHandle( schSrv );
                    }
                    else
                    {
                        hres = HRESULT_FROM_WIN32( GetLastError() );
                    }
                } // end of setting up restart on w3svc.
                

                CloseServiceHandle( schSCM );
            }
        }

    }

    if ( hAdvapi )
    {
        FreeLibrary( hAdvapi );
    }

    return hres;
}


enum CMDS { CMD_NONE, CMD_START, CMD_STOP, CMD_REBOOT, CMD_RESTART, CMD_KILL };

// We use lstrcmpiA to compare arguments typed in with our cmd options
// on foreign code pages case insensitivity can be missed, but since we
// are comparing against the english letters, and don't use foreign letters
// I believe this will be ok.
#pragma prefast(push)
#pragma prefast(disable:400, "Don't complain about using lstrcmpiA") 

int __cdecl 
main( 
    int argc, 
    char*argv[] 
    )
/*++

    main

        main function

    Arguments:
        
        argc
        argv

    Returns:

        0 if no error, otherwise error code

--*/
{
    IIisServiceControl* pIf;
    int                 iA;
    int                 Status = 0;
    HRESULT             hRes;
    CMDS                iCmd = CMD_NONE;
    DWORD               dwStopTimeout = 60 * 1000;
    DWORD               dwRestartTimeout = 20 * 1000;
    DWORD               dwStartTimeout = 60 * 1000;
    LPBYTE              pbBuffer = NULL;
    BYTE                abBuffer[4096];
    LPBYTE              pbOutBuffer = NULL;
    BYTE                abOutBuffer[4096];
    DWORD               dwRequired;
    DWORD               dwNumServices;
    LPVOID              apvParams[8];
    UINT                i;
    BOOL                fNoCmd = FALSE;
    BOOL                fRebootOnError = FALSE;
    BOOL                fKillOnError = TRUE;
    BOOL                fForce = TRUE;
    BOOL                fRebootRestart = FALSE;
    BOOL                fStatus = FALSE;
    COSERVERINFO        csiMachineName;
    MULTI_QI            rgmq;
    WCHAR               awchComputer[64];
    LPSTR               pszMachineName = NULL;
    BOOL                fErrDisplayed = FALSE;
    DWORD               dwFailCount;
    ULONG               CodePage = 0;
    TCHAR               CodePageString[MAX_STRINGIZED_ULONG_CHAR_COUNT + 1];

    //
    // Make sure international versions display text ok - RonaldM
    //
    _tsetlocale( LC_ALL, _T(".OCP") );

    // in order to get output working when the user code page
    // does not match the system code page or the system installed language
    // we have added the following lines, thru the second _tsetlocale.
    // this was recommended by Rostislav Shabalin.  It was also recommended
    // to keep the original _tsetlocale because it may be setting configuration
    // that will not get changed in the second _tsetlocal call.  
    // See RAID Windows Bugs #712030 for more info.
    CodePage = GetConsoleOutputCP();

    CodePageString[0] = _T('.');
    _ultot(CodePage, &(CodePageString[1]), 10 );

    _tsetlocale( LC_ALL, CodePageString );


    // Per issue: 439690 we need to make this call to make sure
    // that localized builds work correctly in all cases.
    SetThreadUILanguage(0);

    _fputts( _T("\n"), stdout );

    //
    // scan command line
    //

    for ( iA = 1 ; 
          iA < argc ; 
          ++iA )
    {
        if ( argv[iA][0] == '-' || argv[iA][0] == '/' )
        {
            if ( !lstrcmpiA( argv[iA]+1, "ENABLE" ) )
            {
                hRes = SetEnableRemote( 1 );

                if ( FAILED( hRes ) )
                {
                    if ( hRes == HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED ) )
                    {
                        PrintMessage( IRSTASTR_ACCESS_DENIED, 0, NULL );
                    }
                    else
                    {
                        DisplayErrorMessage( WIN32_FROM_HRESULT( hRes ) );
                    }

                    Status = WIN32_FROM_HRESULT( hRes );
                }
                else
                {
                    PrintMessage( IRSTASTR_ENABLED, 0, NULL );
                }

                fNoCmd = TRUE;
            }
            else if ( !lstrcmpiA( argv[iA]+1, "DISABLE" ) )
            {
                hRes = SetEnableRemote( 0 );

                if ( FAILED( hRes ) )
                {
                    if ( hRes == HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED ) )
                    {
                        PrintMessage( IRSTASTR_ACCESS_DENIED, 0, NULL );
                    }
                    else
                    {
                        DisplayErrorMessage( WIN32_FROM_HRESULT( hRes ) );
                    }

                    Status = WIN32_FROM_HRESULT( hRes );
                }
                else
                {
                    PrintMessage( IRSTASTR_DISABLED, 0, NULL );
                }

                fNoCmd = TRUE;
            }
            else if ( !_strnicmp( argv[iA]+1, "SCM", sizeof("SCM")-1 ) )
            {
                if ( FAILED( hRes = SetSCM( TRUE ) ) )
                {
                    DisplayErrorMessage( WIN32_FROM_HRESULT( hRes ) );

                    Status = WIN32_FROM_HRESULT( hRes );
                }
                goto Exit;
            }
            else if ( !_strnicmp( argv[iA]+1, "NOSCM", sizeof("NOSCM")-1 ) )
            {
                if ( FAILED( hRes = SetSCM( FALSE ) ) )
                {
                    DisplayErrorMessage( WIN32_FROM_HRESULT( hRes ) );

                    Status = WIN32_FROM_HRESULT( hRes );
                }
                goto Exit;
            }
            else if ( !_strnicmp( argv[iA]+1, "STOPTIMEOUT:", sizeof("STOPTIMEOUT:")-1 ) )
            {
                if ( !GetNumeric( argv[iA]+ 1 + sizeof("STOPTIMEOUT:") - 1, &dwStopTimeout ) )
                {
                    goto invalid_param;
                }
                dwStopTimeout *= 1000;
            }
            else if ( !_strnicmp( argv[iA]+1, "TIMEOUT:", sizeof("TIMEOUT:")-1 ) )
            {
                if ( !GetNumeric( argv[iA]+ 1 + sizeof("TIMEOUT:") - 1, &dwStopTimeout ) )
                {
                    goto invalid_param;
                }
                dwStopTimeout *= 1000;
                dwRestartTimeout = dwStopTimeout;
            }
            else if ( !_strnicmp( argv[iA]+1, "STARTTIMEOUT:", sizeof("STARTTIMEOUT:")-1 ) )
            {
                if ( !GetNumeric( argv[iA]+ 1 + sizeof("STARTTIMEOUT:") - 1, &dwStartTimeout ) )
                {
                    goto invalid_param;
                }
                dwStartTimeout *= 1000;
            }
            else if ( !_strnicmp( argv[iA]+1, "RESTARTTIMEOUT:", sizeof("RESTARTTIMEOUT:")-1 ) )
            {
                if ( !GetNumeric( argv[iA]+ 1 + sizeof("RESTARTTIMEOUT:") - 1, &dwRestartTimeout ) )
                {
                    goto invalid_param;
                }
                dwRestartTimeout *= 1000;
            }
            else if ( !_strnicmp( argv[iA]+1, "fail=", sizeof("fail=")-1 ) )
            {
                //
                // SCM flag to control restart threshold. We restart only 
                // 50 times per SCM restart period.
                //
                // ...  The SCM UI already has a way to address this: they add
                //  a "/fail=N" parameter to the command line 
                // of the restarter app for each time they fail within the time 
                // period and leave it up to the restarter app to interpret 
                // this parameter and throttle restarts approprately.  
                // So...(here's the change request) we need to capture this 
                // value, and if "N" is greater than our limit for 1 day of
                // restarting (hardcoded to 50), we should just exit the 
                // command line app with a success code without doing anything,
                // i.e. "IISRESET.EXE /fail=50" should restart IIS,
                // but "IISRESET.EXE /fail=51" should be a no-op.
                // For simplicity, we should hardcode this value of 50 in 
                // our app-- if a user wants a different value, he can write
                // a batch file wrapper to do it!  You can see 
                // the SCM recovery tab for more info.
                //

                dwFailCount = atoi( argv[iA]+ 1 + sizeof("fail=") - 1 );

                if ( dwFailCount > 50 )
                {
                    return 0;
                }
            }
            else if ( !lstrcmpiA( argv[iA]+1, "START" ) )
            {
                if ( fRebootRestart )
                {
                }
                else if ( iCmd == CMD_STOP )
                {
                    iCmd = CMD_RESTART;
                }
                else
                {
                    iCmd = CMD_START;
                }
            }
            else if ( !lstrcmpiA( argv[iA]+1, "STOP" ) )
            {
                if ( fRebootRestart )
                {
                }
                if ( iCmd == CMD_START )
                {
                    iCmd = CMD_RESTART;
                }
                else
                {
                    iCmd = CMD_STOP;
                }
            }
            else if ( !lstrcmpiA( argv[iA]+1, "REBOOT" ) )
            {
                iCmd = CMD_REBOOT;
                fRebootRestart = TRUE;
            }
            else if ( !lstrcmpiA( argv[iA]+1, "KILL" ) )
            {
                iCmd = CMD_KILL;
                fRebootRestart = TRUE;
            }
            else if ( !lstrcmpiA( argv[iA]+1, "RESTART" ) )
            {
                iCmd = CMD_RESTART;
                fRebootRestart = TRUE;
            }
            else if ( !lstrcmpiA( argv[iA]+1, "STATUS" ) )
            {
                fStatus = TRUE;
            }
            else if ( !lstrcmpiA( argv[iA]+1, "REBOOTONERROR" ) )
            {
                fRebootOnError = TRUE;
                fKillOnError = FALSE;
            }
            else if ( !lstrcmpiA( argv[iA]+1, "NOFORCE" ) )
            {
                fKillOnError = FALSE;
                fForce = FALSE;
            }
            else if ( !lstrcmpiA( argv[iA]+1, "HELP" ) )
            {
                PrintMessage( IRSTASTR_USAGE, 0, NULL );

                return 0;
            }
            else
            {
invalid_param:
                PrintMessage( IRSTASTR_USAGE, 0, NULL );

                return ERROR_INVALID_PARAMETER;
            }
        }
        else
        {
            pszMachineName = argv[iA];
        }
    }

    if ( iCmd == CMD_NONE && !fNoCmd && !fStatus )
    {
        iCmd = CMD_RESTART;
    }

    //
    //fill the structure for CoCreateInstanceEx
    //

    ZeroMemory( &csiMachineName, sizeof(csiMachineName) );

    if ( pszMachineName )
    {
        if ( !MultiByteToWideChar( CP_ACP,
                                   MB_PRECOMPOSED,
                                   pszMachineName,
                                   -1, // process whole string including null terminator
                                   awchComputer,
                                   sizeof(awchComputer) / sizeof(WCHAR)) )
        {
            return GetLastError();
        }

        csiMachineName.pwszName =  awchComputer;
    }
    else
    {
        csiMachineName.pwszName =  NULL;
    }

    if ( fNoCmd )
    {
        iCmd = CMD_NONE;
        fStatus = FALSE;
    }

    if ( iCmd != CMD_NONE || fStatus )
    {

        BOOL  fCoInitialized = true;
        //
        // call method
        //

        rgmq.pIID = &IID_IIisServiceControl;
        rgmq.pItf = NULL;
        rgmq.hr = 0;

        if (FAILED(hRes = CoInitializeEx( NULL, COINIT_MULTITHREADED ))) {

            fCoInitialized = false;
        }
        else if ( SUCCEEDED( hRes = CoCreateInstanceEx( CLSID_IisServiceControl,
                                                   NULL,
                                                   CLSCTX_SERVER,
                                                   &csiMachineName,
                                                   1,
                                                   &rgmq ) ) &&
             SUCCEEDED( hRes = rgmq.hr ) )
        {
            pIf = (IIisServiceControl*)rgmq.pItf;

            switch ( iCmd )
            {
                case CMD_START:
                    PrintMessage( IRSTASTR_START_ATTEMPT, 0, NULL );
                    hRes = pIf->Start( dwStartTimeout );
                    if ( SUCCEEDED( hRes ) )
                    {
                        PrintMessage( IRSTASTR_START_SUCCESS, 0, NULL );
                    }
                    else
                    {
                        CmdError( IRSTASTR_START_FAILED, hRes );
                        fErrDisplayed = TRUE;
                    }
                    break;

                case CMD_STOP:
                    PrintMessage( IRSTASTR_STOP_ATTEMPT, 0, NULL );
                    hRes = pIf->Stop( dwStopTimeout, fKillOnError );
                    if ( SUCCEEDED( hRes ) )
                    {
                        PrintMessage( IRSTASTR_STOP_SUCCESS, 0, NULL );
                    }
                    else
                    {
                        CmdError( IRSTASTR_STOP_FAILED, hRes );
                        fErrDisplayed = TRUE;
                    }
                    break;

                case CMD_REBOOT:
                    PrintMessage( IRSTASTR_REBOOT_ATTEMPT, 0, NULL );
                    hRes = pIf->Reboot( dwRestartTimeout, fForce );
                    if ( SUCCEEDED( hRes ) )
                    {
                        PrintMessage( IRSTASTR_REBOOT_SUCCESS, 0, NULL );
                    }
                    else
                    {
                        CmdError( IRSTASTR_REBOOT_FAILED, hRes );
                        fErrDisplayed = TRUE;
                    }
                    break;

                case CMD_KILL:
                    PrintMessage( IRSTASTR_KILL_ON_ERROR, 0, NULL );
                    hRes = pIf->Kill();
                    if ( SUCCEEDED( hRes ) )
                    {
                        PrintMessage( IRSTASTR_KILL_SUCCESS, 0, NULL );
                    }
                    else
                    {
                        CmdError( IRSTASTR_KILL_FAILED, hRes );
                        fErrDisplayed = TRUE;
                    }
                    break;

                case CMD_RESTART:

                    PrintMessage( IRSTASTR_STOP_ATTEMPT, 0, NULL );
                    hRes = pIf->Stop( dwRestartTimeout, fKillOnError );

                    if ( SUCCEEDED( hRes ) )
                    {
                        PrintMessage( IRSTASTR_STOP_SUCCESS, 0, NULL );

                        PrintMessage( IRSTASTR_START_ATTEMPT, 0, NULL );
                        hRes = pIf->Start( dwStartTimeout );

                        if ( SUCCEEDED( hRes ) )
                        {
                            PrintMessage( IRSTASTR_RESTART_SUCCESS, 0, NULL );
                        }
                    }
                    if ( FAILED( hRes ) )
                    {
                        CmdError( IRSTASTR_RESTART_FAILED, hRes );
                        fErrDisplayed = TRUE;
                    }
                    break;
            }

            if ( fStatus )
            {
                pbBuffer = NULL;
                fErrDisplayed = FALSE;

                if ( FAILED( hRes = pIf->Status( sizeof(abBuffer), 
                                                 abBuffer, 
                                                 &dwRequired, 
                                                 &dwNumServices ) ) )
                {
                    if ( HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER ) == hRes )
                    {
                        if ( (pbBuffer = (LPBYTE)LocalAlloc( LMEM_FIXED, dwRequired )) == NULL )
                        {
                            hRes = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
                        }
                        else
                        {
                            hRes = pIf->Status( dwRequired, 
                                                pbBuffer, 
                                                &dwRequired, 
                                                &dwNumServices );
                        }
                    }
                }
                else
                {
                    pbBuffer = abBuffer;
                }

                if ( SUCCEEDED( hRes ) )
                {
                    pbOutBuffer = NULL;

                    if ( FAILED( hRes = DeserializeEnumServiceBuffer( pbBuffer, 
                                                                      dwNumServices, 
                                                                      abOutBuffer, 
                                                                      sizeof(abOutBuffer), 
                                                                      &dwRequired ) ) )
                    {
                        if ( HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER ) == hRes )
                        {
                            if ( (pbOutBuffer = (LPBYTE)LocalAlloc( LMEM_FIXED, dwRequired )) == NULL )
                            {
                                hRes = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
                            }
                            else
                            {
                                hRes = DeserializeEnumServiceBuffer( pbBuffer, 
                                                                     dwNumServices, 
                                                                     pbOutBuffer, 
                                                                     dwRequired, 
                                                                     &dwRequired );
                            }
                        }
                    }
                    else
                    {
                        pbOutBuffer = abOutBuffer;
                    }
                }

                if ( SUCCEEDED( hRes ) )
                {
                    for ( i = 0 ; i < dwNumServices ; ++i )
                    {
                        apvParams[0] = ((LPENUM_SERVICE_STATUS)pbOutBuffer)[i].lpDisplayName;
                        apvParams[1] = ((LPENUM_SERVICE_STATUS)pbOutBuffer)[i].lpServiceName;

                        switch ( ((LPENUM_SERVICE_STATUS)pbOutBuffer)[i].ServiceStatus.dwCurrentState )
                        {
                            case SERVICE_STOPPED:
                                apvParams[2] = GetString(IRSTASTR_SERVICE_STOPPED); break;

                            case SERVICE_START_PENDING:
                                apvParams[2] = GetString(IRSTASTR_SERVICE_START_PENDING); break;

                            case SERVICE_STOP_PENDING:
                                apvParams[2] = GetString(IRSTASTR_SERVICE_STOP_PENDING); break;

                            case SERVICE_RUNNING:
                                apvParams[2] = GetString(IRSTASTR_SERVICE_RUNNING); break;

                            case SERVICE_CONTINUE_PENDING:
                                apvParams[2] = GetString(IRSTASTR_SERVICE_CONTINUE_PENDING); break;

                            case SERVICE_PAUSE_PENDING:
                                apvParams[2] = GetString(IRSTASTR_SERVICE_PAUSE_PENDING); break;

                            case SERVICE_PAUSED:
                                apvParams[2] = GetString(IRSTASTR_SERVICE_PAUSED); break;

                            default:
                                apvParams[2] = GetString(IRSTASTR_SERVICE_DEFAULT); break;
                        }

                        PrintMessage( IRSTASTR_STATUS_ITEM, 3, apvParams );
                    }
                }

                if ( pbBuffer != NULL && pbBuffer != abBuffer )
                {
                    LocalFree( pbBuffer );
                }

                if ( pbOutBuffer != NULL && pbOutBuffer != abOutBuffer )
                {
                    LocalFree( pbOutBuffer );
                }

            }

            if ( FAILED( hRes ) && fRebootOnError
                 && ( iCmd == CMD_STOP || iCmd == CMD_RESTART ) )
            {
                fErrDisplayed = FALSE;
                PrintMessage( IRSTASTR_REBOOT_ON_ERROR, 0, NULL );

                hRes = pIf->Reboot( 0, fForce );

                if ( SUCCEEDED( hRes ) )
                {
                    PrintMessage( IRSTASTR_REBOOT_SUCCESS, 0, NULL );
                }
            }

            pIf->Release();

            if ( FAILED( hRes ) )
            {
                if ( !fErrDisplayed )
                {
                    if ( hRes == HRESULT_FROM_WIN32( ERROR_RESOURCE_DISABLED ) )
                    {
                        PrintMessage( IRSTASTR_REMOTE_DISABLED, 0, NULL );
                    }
                    else if ( hRes == HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED ) )
                    {
                        PrintMessage( IRSTASTR_ACCESS_DENIED, 0, NULL );
                    }
                    else
                    {
                        DisplayErrorMessage( WIN32_FROM_HRESULT( hRes ) );
                    }
                }

                Status = WIN32_FROM_HRESULT( hRes );
            }
        }
        else
        {
            if ( hRes == HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED ) )
            {
                PrintMessage( IRSTASTR_ACCESS_DENIED, 0, NULL );
            }
            else
            {
                DisplayErrorMessage( WIN32_FROM_HRESULT( hRes ) );
            }

            Status = WIN32_FROM_HRESULT( hRes );
        }
        if (fCoInitialized) {
            CoUninitialize();
        }
    }

Exit:

    return Status;
}
#pragma prefast(pop)

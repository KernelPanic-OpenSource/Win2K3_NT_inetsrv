/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: privilge.cpp

Abstract:
    Handle process and thread privileges.

Author:
    Doron Juster (DoronJ)  08-Jun-1998

Revision History:

--*/

#include <stdh_sec.h>

#include "privilge.tmh"

static WCHAR *s_FN=L"acssctrl/privilge";

//+-------------------------------------------------------------------
//
// Function:   SetSpecificPrivilegeInAccessToken()
//
// Description:
//      Enable/Disable a security privilege in the access token.
//
// Parameters:
//      hAccessToken - the access token on which the function should operate.
//          The token should be opened with the TOKEN_ADJUST_PRIVILEGES flag.
//      lpwcsPrivType - the privilege type.
//      bEnabled - Indicates whether the privilige should be enabled or
//          disabled.
//
//+-------------------------------------------------------------------

HRESULT 
SetSpecificPrivilegeInAccessToken( 
	HANDLE  hAccessToken,
    LPCTSTR lpwcsPrivType,
    BOOL    bEnabled 
	)
{
    DWORD             dwErr = 0 ;
    HRESULT           hr = MQSec_OK ;
    LUID              luidPrivilegeLUID;
    TOKEN_PRIVILEGES  tpTokenPrivilege;

    if (!LookupPrivilegeValue( 
			NULL,
            lpwcsPrivType,
            &luidPrivilegeLUID
			))
    {
        TrERROR(SECURITY, "Failed to lookup privilege value. %!winerr!", GetLastError());
        return MQSec_E_LOOKUP_PRIV;
    }

    tpTokenPrivilege.PrivilegeCount = 1;
    tpTokenPrivilege.Privileges[0].Luid = luidPrivilegeLUID;
    tpTokenPrivilege.Privileges[0].Attributes =
                                      bEnabled ? SE_PRIVILEGE_ENABLED : 0 ;

    if (!AdjustTokenPrivileges( 
			hAccessToken,
            FALSE,         // Do not disable all
            &tpTokenPrivilege,
            0,
            NULL,           // Ignore previous info
            NULL            // Ignore previous info
			))
    {
        TrERROR(SECURITY, "Failed to adjust token privileges. %!winerr!", GetLastError());
        return MQSec_E_ADJUST_TOKEN;
    }
    else
    {
        dwErr = GetLastError();
        ASSERT((dwErr == ERROR_SUCCESS) ||
               (dwErr == ERROR_NOT_ALL_ASSIGNED));
    }

    return LogHR(hr, s_FN, 30);
}


//+-------------------------------------------------------------------
//
// Function:  MQSec_SetPrivilegeInThread()
//
// Description:
//      Enable/Disable a security privilege in the access token of the
//      current thread.
//
// Parameters:
//      lpwcsPrivType - the privilege type.
//      bEnabled - Indicates whether the privilige should be enabled or
//                 disabled.
//
//+-------------------------------------------------------------------

HRESULT
APIENTRY  
MQSec_SetPrivilegeInThread( 
			LPCTSTR lpwcsPrivType,
            BOOL    bEnabled 
			)
{
    HANDLE  hAccessToken = NULL;

    if (!OpenThreadToken( 
			GetCurrentThread(),
            TOKEN_ADJUST_PRIVILEGES,
            TRUE,
            &hAccessToken 
			))
	{
        DWORD gle = GetLastError();
        if (gle == ERROR_NO_TOKEN)
        {
            if (!OpenProcessToken( 
					GetCurrentProcess(),
                    TOKEN_ADJUST_PRIVILEGES,
                    &hAccessToken 
					))
			{
                TrERROR(SECURITY, "Failed to open current process token. %!winerr!", GetLastError());
				return MQSec_E_OPEN_TOKEN;
            }
        }
        else
        {
            TrERROR(SECURITY, "Failed to open thread token. %!winerr!", gle);
			return MQSec_E_OPEN_TOKEN;
        }
    }

    HRESULT hr = SetSpecificPrivilegeInAccessToken( 
						hAccessToken,
                        lpwcsPrivType,
                        bEnabled 
						);

	CloseHandle(hAccessToken);
    hAccessToken = NULL;
    return LogHR(hr, s_FN, 40);
}


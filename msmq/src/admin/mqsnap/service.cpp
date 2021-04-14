#include <stdafx.h>

//#include "cpldef.h"
#include <winsvc.h>
//#include "ctlpnl.h"
#include <mqtypes.h>
#include <_mqdef.h>
/*
#ifndef DLL_IMPORT
#define DLL_IMPORT __declspec(dllimport)
#endif
*/
#include <_registr.h>
#include <tlhelp32.h>
#include "localutl.h"
#include "globals.h"
#include "autorel.h"
#include "autorel2.h"
#include "mqtg.h"
#include "acioctl.h"
#include "acdef.h"
#include "acapi.h"

#include "service.tmh"

#define MQQM_SERVICE_FILE_NAME  TEXT("mqsvc.exe")
#define MQDS_SERVICE_NAME       TEXT("MQDS")

#define WAIT_INTERVAL	50
#define MAX_WAIT_FOR_SERVICE_TO_STOP	5*60*1000  // 5 minutes


static
BOOL
GetServiceAndScmHandles(
    SC_HANDLE *phServiceCtrlMgr,
    SC_HANDLE *phService,
    DWORD dwAccessType)
{
    *phServiceCtrlMgr = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (*phServiceCtrlMgr == NULL)
    {
        MessageDSError(GetLastError(), IDS_SERVICE_MANAGER_PRIVILEGE_ERROR);

        return FALSE;
    }

    *phService = OpenService(*phServiceCtrlMgr, MQQM_SERVICE_NAME, dwAccessType);
    if (*phService == NULL)
    {
        MessageDSError(GetLastError(), IDS_SERVICE_PRIVILEGE_ERROR);

        CloseServiceHandle(*phServiceCtrlMgr);
        return FALSE;
    }
    return TRUE;
}


static
BOOL
GetMSMQProcessHandle(
    SC_HANDLE hService,
    HANDLE *phProcess
	)
{
	*phProcess = 0;

	//
	// Get service process ID
	//
    SERVICE_STATUS_PROCESS ServiceStatusProcess;
	DWORD dwBytesNeeded;
    BOOL fSucc = QueryServiceStatusEx(
								hService,
								SC_STATUS_PROCESS_INFO,
								reinterpret_cast<LPBYTE>(&ServiceStatusProcess),
								sizeof(ServiceStatusProcess),
								&dwBytesNeeded
								);
    
    if(!fSucc)
    {
        MessageDSError(GetLastError(), IDS_QUERY_SERVICE_ERROR);
		return FALSE;
	}

	//
	// Get hanlde to the service process
	//
	HANDLE hProcess = OpenProcess(SYNCHRONIZE, FALSE, ServiceStatusProcess.dwProcessId);
	
	if (hProcess == NULL)
	{
		//
		// The service is stopped. Either we got a 0
		// process ID in ServiceStatusProcess, or the ID
		// that we got was of a process that already stopped
		//
		if (GetLastError() == ERROR_INVALID_PARAMETER)
		{
			return TRUE;
		}

        MessageDSError(GetLastError(), IDS_OPEN_PROCESS_ERROR);
		return FALSE;
	}

	*phProcess = hProcess;
	return TRUE;
}


static
BOOL
AskUserIfStopServices(
	LPENUM_SERVICE_STATUS lpServiceStruct,
	DWORD nServices
	)
{
	CString strServicesList;
	UINT numOfDepServices = 0;
	//
	// Build a list of all active dependent services
	// Write each one at new line
	//
	for ( DWORD i = 0; i < nServices; i ++ )
	{
		if ( (_wcsicmp(lpServiceStruct[i].lpServiceName, xDefaultTriggersServiceName) == 0) ||
			 (_wcsicmp(lpServiceStruct[i].lpServiceName, MQDS_SERVICE_NAME) == 0) )
		{
			continue;
		}

		strServicesList += "\n";
		strServicesList += "\"";
		strServicesList += lpServiceStruct[i].lpDisplayName;
		strServicesList += "\"";
		
		numOfDepServices++;
	}

	if ( numOfDepServices == 0 )
	{
		return TRUE;
	}

	CString strMessage;
	strMessage.FormatMessage(IDS_DEP_SERVICES_LIST, strServicesList);
	return ( AfxMessageBox(strMessage, MB_OKCANCEL) == IDOK );
}


static
BOOL
WaitForServiceToStop(
	SC_HANDLE hService
	)
{
	DWORD dwWait = 0;

	for (;;)
	{
		SERVICE_STATUS ServiceStatus;
		if (!QueryServiceStatus(hService, &ServiceStatus))
		{
			//
			//  indication here is not helpful for the user.
			//
			return FALSE;
		}

		if (ServiceStatus.dwCurrentState == SERVICE_STOPPED)
		{
			return TRUE;
		}
		
		if ( dwWait > MAX_WAIT_FOR_SERVICE_TO_STOP )
		{
			//
			// If this routine fails, and error message will be displayed.
			// The routine that displays the message does GetLastError()
			// In this case we need to specify what happened.
			//
			SetLastError(ERROR_SERVICE_REQUEST_TIMEOUT);
			return FALSE;
		}

		Sleep(WAIT_INTERVAL);
		dwWait += WAIT_INTERVAL;
	}
}


BOOL
WaitForMSMQServiceToTerminate(
	HANDLE hProcess
	)
{
	if (hProcess == 0)
	{
		return TRUE;
	}

	//
	// Wait on MSMQ service process handle
	//
	DWORD dwRes = WaitForSingleObject(hProcess, MAX_WAIT_FOR_SERVICE_TO_STOP);

	if (dwRes == WAIT_OBJECT_0)
	{
		return TRUE;
	}

	if (dwRes == WAIT_FAILED )
	{
		return FALSE;
	}

	//
	// We had timeout.
	// If this routine fails, and error message will be displayed.
	// The routine that displays the message does GetLastError()
	// In this case we need to specify what happened.
	//
	ASSERT(dwRes == WAIT_TIMEOUT);
	SetLastError(ERROR_SERVICE_REQUEST_TIMEOUT);
	return FALSE;
}


static
BOOL
StopSingleDependentService(
	SC_HANDLE hServiceMgr,
	LPCWSTR pszServiceName
	)	
{
	CServiceHandle hService( OpenService(
								hServiceMgr, 
								pszServiceName, 
								SERVICE_STOP | SERVICE_QUERY_STATUS | SERVICE_ENUMERATE_DEPENDENTS
								) );	
	if (hService == 0)
	{
		return FALSE;
	}

	SERVICE_STATUS ServiceStatus;
	BOOL fRet = ControlService(hService, SERVICE_CONTROL_STOP, &ServiceStatus);

	if ( !fRet && GetLastError() != ERROR_SERVICE_NOT_ACTIVE)
	{
		return FALSE;
	}

	//
	// Wait untill state = SERVICE_STOPPED
	//
	fRet = WaitForServiceToStop(hService);

	return fRet;
}


//
// StopDependentServices
//
// This function stops all services dependent on MSMQ. 
// Enumeration of dependent services gives a list in descending
// degree of depedency. Stopping the services in order that the 
// enumeration gives will not cause dependency clashes.
//
static
BOOL
StopDependentServices(
	SC_HANDLE hServiceMgr,
	SC_HANDLE hService,
	CWaitCursor& wc
	)
{
	DWORD dwBytesNeeded, nServices;

	//
	// Try to find out how much memory is needed for data
	//
	BOOL fRet = EnumDependentServices(
					hService,
					SERVICE_ACTIVE,
					NULL,
					0,
					&dwBytesNeeded,
					&nServices
					);

	//
	// Zero dependent services
	//
	if ( fRet )
	{
		return TRUE;
	}

	if ( !fRet && GetLastError() != ERROR_MORE_DATA )
	{
		MessageDSError(GetLastError(), IDS_ENUM_MSMQ_DEPEND);
		return FALSE;
	}

	AP<ENUM_SERVICE_STATUS> lpServiceStruct = reinterpret_cast<LPENUM_SERVICE_STATUS>(new BYTE[dwBytesNeeded]);
	DWORD dwBuffSize = dwBytesNeeded;

	//
	// Get all the data
	//
	if ( !EnumDependentServices(
					hService,
					SERVICE_ACTIVE,
					lpServiceStruct,
					dwBuffSize,
					&dwBytesNeeded,
					&nServices
					) )
	{
		MessageDSError(GetLastError(), IDS_ENUM_MSMQ_DEPEND);
		return FALSE;
	}

	//
	// Ask user if it is OK to stop all dependent services
	//
	if ( !AskUserIfStopServices(lpServiceStruct, nServices))
	{
		return FALSE;
	}

	wc.Restore();

	for ( DWORD i = 0; i < nServices; i ++ )
	{
		for(;;)
		{
			fRet = StopSingleDependentService(
							hServiceMgr, 
							lpServiceStruct[i].lpServiceName
							);
			if ( !fRet )
			{
				BOOL fRetry = MessageDSError(
									GetLastError(),
									IDS_STOP_SERVICE_ERR, 
									lpServiceStruct[i].lpDisplayName,
									MB_RETRYCANCEL | MB_ICONEXCLAMATION
									);
				
				//
				// User asked for retry
				//
				if (fRetry == IDRETRY)
				{
					wc.Restore();
					continue;
				}

				return FALSE;
			}

			break;
		}
	}

	return TRUE;	
}


//
// See whether the service is running.
//
BOOL
GetServiceRunningState(
    BOOL *pfServiceIsRunning)
{
    SC_HANDLE hServiceCtrlMgr;
    SC_HANDLE hService;

    //
    // Get a handle to the service.
    //
    if (!GetServiceAndScmHandles(&hServiceCtrlMgr,
                             &hService,
                             SERVICE_QUERY_STATUS))
    {
        return FALSE;
    }

	//
	// Automatic wrappers
	//
	CServiceHandle hSCm(hServiceCtrlMgr);
	CServiceHandle hSvc(hService);

    //
    // Query the service status.
    //
    SERVICE_STATUS SrviceStatus;
    if (!QueryServiceStatus(hService, &SrviceStatus))
    {
        MessageDSError(GetLastError(), IDS_QUERY_SERVICE_ERROR);
		return FALSE;
    }
    else
    {
        *pfServiceIsRunning = SrviceStatus.dwCurrentState == SERVICE_RUNNING;
    }

    return TRUE;
}


//
// Stop the MQQM service.
//
BOOL
StopService()
{
	CWaitCursor wc;

    //
    // Get a handle to the service.
    //
    SC_HANDLE hServiceCtrlMgr;
    SC_HANDLE hService;

    if (!GetServiceAndScmHandles(&hServiceCtrlMgr,
                             &hService,
                             SERVICE_STOP | SERVICE_QUERY_STATUS | SERVICE_ENUMERATE_DEPENDENTS))
    {
        return FALSE;
    }

 	//
	// Automatic wrappers
	//
	CServiceHandle hSCm(hServiceCtrlMgr);
	CServiceHandle hSvc(hService);

	CHandle hProcess;
	if (!GetMSMQProcessHandle(hService, &hProcess))
	{
		return FALSE;
	}

	//
    // Stop the service.
    //
	SERVICE_STATUS SrviceStatus;
	DWORD dwErr;
	BOOL fRet;

	for(;;)
	{
		fRet = ControlService(hService,
							  SERVICE_CONTROL_STOP,
							  &SrviceStatus);

		dwErr = GetLastError();

		//
		// If service is already stopped, or there are dependent services active
		// it is normal situtation. Other cases are errors.
		//
		if (!fRet && 
			dwErr != ERROR_SERVICE_NOT_ACTIVE && 
			dwErr != ERROR_DEPENDENT_SERVICES_RUNNING)
		{
			if ( MessageDSError(
							dwErr,
							IDS_STOP_SERVICE_ERROR, 
							NULL,
							MB_RETRYCANCEL | MB_ICONEXCLAMATION) == IDRETRY)
			{
				wc.Restore();
				continue;
			}

			return FALSE;
		}

		break;
	}

	//
	// If there are dependent serivices running, attempt to stop them
	//
	if ( !fRet && dwErr == ERROR_DEPENDENT_SERVICES_RUNNING)
	{
		fRet = StopDependentServices(hServiceCtrlMgr, hService, wc);
		if ( !fRet )
		{
			return FALSE;
		}

		for(;;)
		{
			//
			// Send Stop control to QM again - it should not fail this time
			//
			fRet = ControlService(hService,
					  SERVICE_CONTROL_STOP,
					  &SrviceStatus);

			
			if ( !fRet && 
			   (GetLastError() != ERROR_SERVICE_NOT_ACTIVE) )
			{
				if ( MessageDSError(
							GetLastError(),
							IDS_STOP_SERVICE_ERROR, 
							NULL,
							MB_RETRYCANCEL | MB_ICONEXCLAMATION) == IDRETRY)
				{
					wc.Restore();
					continue;
				}

				return FALSE;
			}

			break;
		}

	}

	for(;;)
	{
		if (!WaitForMSMQServiceToTerminate(hProcess) )
		{
			if ( MessageDSError(
						GetLastError(), 
						IDS_STOP_SERVICE_ERROR, 
						NULL, 
						MB_RETRYCANCEL | MB_ICONEXCLAMATION) == IDRETRY)
			{
				wc.Restore();
				continue;
			}

			return FALSE;
		}

		break;
	}

    return TRUE;
}

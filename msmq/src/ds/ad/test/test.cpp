#include "..\..\h\ds_stdh.h"
#include "ad.h"
#include "cm.h"
#include "tr.h"

#include "test.tmh"

extern "C" 
int 
__cdecl 
_tmain(
    int /* argc */,
    LPCTSTR* /* argv */
    )
{
    WPP_INIT_TRACING(L"Microsoft\\MSMQ");

	CmInitialize(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\MSMQ\\Parameters", KEY_ALL_ACCESS);
	TrInitialize();

	//
	// Test Setup raw detection
	//
    DWORD dwDsEnv = ADRawDetection();
	TrTRACE(AdTest, "DsEnv = %d", dwDsEnv);

	//
	// Test ADInit
	//
	ADInit(NULL, NULL, false, false, false, NULL, true);

    //
    //  Retrieve local computer name
    //
    DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;
    AP<WCHAR> pwcsComputerName = new WCHAR[dwSize];

    if (GetComputerName(pwcsComputerName, &dwSize))
    {
        CharLower(pwcsComputerName);
    }
    else
    {
        printf("failed to retreive local computer name \n");
    }

    //
    //  Get local computer sites
    //
    GUID* pguidSites;
    DWORD numSites;
    HRESULT hr;

    hr = ADGetComputerSites(
                        pwcsComputerName,
                        &numSites,
                        &pguidSites
                        );
    if (FAILED(hr))
    {
        printf("FAILURE: to getComputerSites, computer = %S, hr =%lx\n", pwcsComputerName, hr);
    }
    if (numSites != 1)
    {
        printf("FAILURE: wrong number of sites \n");
    }

    WPP_CLEANUP();

    return(0);
} 


void LogMsgHR(HRESULT hr, LPWSTR wszFileName, USHORT usPoint)
{
    printf("MQAD Test Error: %s(%u). HR: 0x%x", wszFileName, usPoint, hr);
}


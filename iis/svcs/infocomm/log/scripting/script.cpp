// script.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//		To merge the proxy/stub code into the object DLL, add the file
//		dlldatax.c to the project.  Make sure precompiled headers
//		are turned off for this file, and add _MERGE_PROXYSTUB to the
//		defines for the project.
//
//		If you are not running WinNT4.0 or Win95 with DCOM, then you
//		need to remove the following define from dlldatax.c
//		#define _WIN32_WINNT 0x0400
//
//		Further, if you are running MIDL without /Oicf switch, you also
//		need to remove the following define from dlldatax.c.
//		#define USE_STUBLESS_PROXY
//
//		Modify the custom build rule for script.idl by adding the following
//		files to the Outputs.
//			script_p.c
//			dlldata.c
//		To build a separate proxy/stub DLL,
//		run nmake -f scriptps.mk in the project directory.

#include "stdafx.h"

#include "resource.h"
#include "script.h"
#include "dlldatax.h"

#include "script_i.c"
#include <iadmw.h>
#include "LogScripting.h"

#ifdef _MERGE_PROXYSTUB
extern "C" HINSTANCE hProxyDll;
#endif

CHAR    g_szModuleName[] = "LogScript";

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_LogScripting, CLogScripting)
END_OBJECT_MAP()

DECLARE_DEBUG_PRINTS_OBJECT( );

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	lpReserved;
#ifdef _MERGE_PROXYSTUB
	if (!PrxDllMain(hInstance, dwReason, lpReserved))
		return FALSE;
#endif
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		_Module.Init(ObjectMap, hInstance);
		DisableThreadLibraryCalls(hInstance);

        CREATE_DEBUG_PRINT_OBJECT(g_szModuleName);
        LOAD_DEBUG_FLAGS_FROM_REG_STR("System\\CurrentControlSet\\Services\\InetInfo\\Parameters", 0);

	}
	else if (dwReason == DLL_PROCESS_DETACH)
    {
        DELETE_DEBUG_PRINT_OBJECT();
		_Module.Term();
    }
	return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
#ifdef _MERGE_PROXYSTUB
	if (PrxDllCanUnloadNow() != S_OK)
		return S_FALSE;
#endif
	return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
#ifdef _MERGE_PROXYSTUB
	if (PrxDllGetClassObject(rclsid, riid, ppv) == S_OK)
		return S_OK;
#endif
	return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
#ifdef _MERGE_PROXYSTUB
	HRESULT hRes = PrxDllRegisterServer();
	if (FAILED(hRes))
		return hRes;
#endif
	// registers object, typelib and all interfaces in typelib
	return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
#ifdef _MERGE_PROXYSTUB
	PrxDllUnregisterServer();
#endif
	_Module.UnregisterServer();
	return S_OK;
}



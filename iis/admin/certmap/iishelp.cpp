#include "stdafx.h"

INT g_iDebugOutputLevel = 0;

void GetOutputDebugFlag(void)
{
    DWORD rc, err, size, type;
    HKEY  hkey;
    err = RegOpenKey(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\InetMgr"), &hkey);
    if (err != ERROR_SUCCESS) {return;}
    size = sizeof(DWORD);
    err = RegQueryValueEx(hkey,_T("OutputDebugFlag"),0,&type,(LPBYTE)&rc,&size);
    if (err != ERROR_SUCCESS || type != REG_DWORD) {rc = 0;}
    RegCloseKey(hkey);

	if (rc < 0xffffffff)
	{
		g_iDebugOutputLevel = rc;
	}
    return;
}

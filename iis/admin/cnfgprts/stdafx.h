// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxctl.h>         // MFC support for OLE Controls
#include <afxdtctl.h>
#include <atlbase.h>

// Delete the two includes below if you do not wish to use the MFC
//  database classes
#ifndef _UNICODE
//#include <afxdb.h>			// MFC database classes
//#include <afxdao.h>			// MFC DAO database classes
#endif //_UNICODE

#include <afxcmn.h>			// MFC support for Windows Common Controls
#include <afxtempl.h>

#include <assert.h>
#include <aclapi.h>

#define _COMIMPORT
#include "common.h"
#include "iishelp.h"
/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: stdh.h

Abstract: Generic header file for utility code

Author: Doron Juster  (DoronJ)  24-May-1998

--*/

#ifndef __SEC_STDH_UT_H
#define __SEC_STDH_UT_H

#include <_stdh.h>
#include <mqutil.h>
extern HINSTANCE g_hInstance;
extern void XactFreeDTC(void);
extern LPCWSTR g_wszMachineName;

#endif // __SEC_STDH_UT_H


/*++

   Copyright (c) 1997-1999 Microsoft Corporation

   Module  Name :

       iisext_guid.c

   Abstract:

        Contains ADSI IISExtensions CLSIDs, LIBIDs, and IIDs

   Environment:

      Win32 User Mode

--*/

#define INITGUID


#include <ole2.h>
#include "iwamreg.h"
#include "iadmw.h" 


//--------------------------------------------------------------------------
//
//  ADSI IISExtensions CLSIDs
//
//--------------------------------------------------------------------------

const IID LIBID_IISExt = {0x2a56ea30,0xafeb,0x11d1,{0x98,0x68,0x00,0xa0,0xc9,0x22,0xe7,0x03}};

const CLSID CLSID_IISExtDsCrMap = {0xbc36cde8,0xafeb,0x11d1,{0x98,0x68,0x00,0xa0,0xc9,0x22,0xe7,0x03}};

const CLSID CLSID_IISExtComputer = {0x91ef9258,0xafec,0x11d1,{0x98,0x68,0x00,0xa0,0xc9,0x22,0xe7,0x03}};

const CLSID CLSID_IISExtApp = {0xb4f34438,0xafec,0x11d1,{0x98,0x68,0x00,0xa0,0xc9,0x22,0xe7,0x03}};

const CLSID CLSID_IISExtServer = {0xc3b32488,0xafec,0x11d1,{0x98,0x68,0x00,0xa0,0xc9,0x22,0xe7,0x03}};

const CLSID CLSID_IISExtApplicationPool = { 0xe99f9d0c, 0xfb39, 0x402b, { 0x9e, 0xeb, 0xaa, 0x18, 0x52, 0x37, 0xbd, 0x34 } };

const CLSID CLSID_IISExtApplicationPools = { 0x95863074, 0xa389, 0x406a, { 0xa2, 0xd7, 0xd9, 0x8b, 0xfc, 0x95, 0xb9, 0x5 } };

const CLSID CLSID_IISExtWebService = { 0x40b8f873, 0xb30e, 0x475d, { 0xbe, 0xc5, 0x4d, 0xe, 0xbb, 0xd, 0xba, 0xf3 } };

DEFINE_GUID(IID_IISDsCrMap, 0xedcd6a60, 0xb053, 0x11d0, 0xa6, 0x2f, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x52);

DEFINE_GUID(IID_IISApp, 0x46fbbb80, 0x192, 0x11d1, 0x9c, 0x39, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x3);

DEFINE_GUID(IID_IISApp2, 0x603DCBEA, 0x7350, 0x11d2, 0xA7, 0xBE, 0x0, 0x0, 0xF8, 0x8, 0x5B, 0x95);

DEFINE_GUID(IID_IISApp3, 0x2812b639, 0x8fac, 0x4510, 0x96, 0xc5, 0x71, 0xdd, 0xbd, 0x1f, 0x54, 0xfc);

DEFINE_GUID(IID_IISComputer, 0xcf87a2e0, 0x78b, 0x11d1, 0x9c, 0x3d, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x3);

DEFINE_GUID(IID_IISComputer2, 0x63d89839, 0x5762, 0x4a68, 0xb1, 0xb9, 0x35, 0xa07, 0xea, 0x76, 0xcb, 0xbf);

DEFINE_GUID(IID_IISApplicationPool, 0xb3cb1e1, 0x829a, 0x4c06, 0x8b, 0x9, 0xf5, 0x6d, 0xa1, 0x89, 0x4c, 0x88);

DEFINE_GUID(IID_IISApplicationPools, 0x587f123f, 0x49b4, 0x49dd, 0x93, 0x9e, 0xf4, 0x54, 0x7a, 0xa3, 0xfa, 0x75);

DEFINE_GUID(IID_IISWebService, 0xee46d40c, 0x1b38, 0x4a02, 0x89, 0x8d, 0x35, 0x8e, 0x74, 0xdf, 0xc9, 0xd2);

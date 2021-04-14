//-----------------------------------------------------------------------------
//
//
//  File: qwiktime.h
//
//  Description: Header for CAQQuickTime class... a class to handle filling
//      and comparing FILETIME by using the GetTickCount instead of using
//      GetSystemTime
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      7/9/98 - MikeSwa Created
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __QWIKTIME_H__
#define __QWIKTIME_H__

#define QUICK_TIME_SIG 'miTQ'

class CAQQuickTime
{
  protected:
    DWORD       m_dwSignature;
    DWORD       m_dwLastInternalTime;
    FILETIME    m_ftSystemStart;

    DWORD dwGetInternalTime();
  public:
    CAQQuickTime();

    // Get expire time using context or by getting the current time
    void GetExpireTime(
                IN     DWORD cMinutesExpireTime,
                IN OUT FILETIME *pftExpireTime,
                IN OUT DWORD *pdwExpireContext); //if non-zero, will use last time

    // Overloaded version takes start time instead of using context
    void GetExpireTime(
                IN     FILETIME ftStartTime,
                IN     DWORD cMinutesExpireTime,
                IN OUT FILETIME *pftExpireTime);

    BOOL fInPast(IN FILETIME *pftExpireTime, IN OUT DWORD *pdwExpireContext);
};

#endif //__QWIKTIME_H__


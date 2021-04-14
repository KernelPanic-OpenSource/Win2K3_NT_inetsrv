/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Main

File: perfdata.h

Owner: DmitryR

PERFMON related data in asp.dll -- header file
===================================================================*/

#ifndef _ASP_PERFDATA_H
#define _ASP_PERFDATA_H

#ifndef PERF_DISABLE

#include "asppdef.h"
#ifndef _ASP_DEBUG_EXT
#include "denali.h"
#endif


/*===================================================================
CPerfData -- PERFMON data for ASP
             CPerfProcBlock 
                + macros to update counters
                + clsid to remember
                + place to update counters before perfmon inited
===================================================================*/

class CPerfData : public CPerfProcBlock
    {
private:
    // Initial counter values (gathered when uninit)
    DWORD m_rgdwInitCounters[C_PERF_PROC_COUNTERS];
    // CLSID
    DWORD m_fValid : 1;
    DWORD m_dwProcId;

public:
    inline CPerfData() 
        : m_fValid(FALSE)
        {
        memset(m_rgdwInitCounters, 0, CB_COUNTERS);
        }

    inline ~CPerfData()
        {
        }

    inline HRESULT Init(DWORD   procId)
        {
        HRESULT hr = InitForThisProcess(procId, m_rgdwInitCounters);
        if (SUCCEEDED(hr))
            {
            m_dwProcId = procId;
            m_fValid = TRUE;
            }
        return hr;
        }

    inline HRESULT UnInit()
        {
        m_fValid = FALSE;
        return CPerfProcBlock::UnInit();
        }

    inline BOOL FValid()
        {
        return m_fValid;
        }

    inline const DWORD ProcId()
        {
        return m_dwProcId;
        }

    // helper inline to get counter address as DWORD *
    inline DWORD *PDWCounter(int i)
        {
        return m_fInited ? &(m_pData->m_rgdwCounters[i])
                         : &(m_rgdwInitCounters[i]);
        }
    
    // helper inline to get counter address as LPLONG
    inline LPLONG PLCounter(int i)
        {
        return (LPLONG)PDWCounter(i);
        }

    // Inlines to change individual counters --------------

    inline void Incr_DEBUGDOCREQ()
        {
        InterlockedIncrement(PLCounter(ID_DEBUGDOCREQ));
        }
    inline void Incr_REQERRRUNTIME()
        {
        InterlockedIncrement(PLCounter(ID_REQERRRUNTIME));
        }
    inline void Incr_REQERRPREPROC()
        {
        InterlockedIncrement(PLCounter(ID_REQERRPREPROC));
        }
    inline void Incr_REQERRCOMPILE()
        {
        InterlockedIncrement(PLCounter(ID_REQERRCOMPILE));
        }
    inline void Incr_REQERRORPERSEC()
        {
        InterlockedIncrement(PLCounter(ID_REQERRORPERSEC));
        }
    inline void Add_REQTOTALBYTEIN(DWORD dw)
        {
        EnterCriticalSection(&m_csReqLock);
        *PDWCounter(ID_REQTOTALBYTEIN) += dw;
    	LeaveCriticalSection(&m_csReqLock);
        }
    inline void Add_REQTOTALBYTEOUT(DWORD dw)
        {
        EnterCriticalSection(&m_csReqLock);
        *PDWCounter(ID_REQTOTALBYTEOUT) += dw;
    	LeaveCriticalSection(&m_csReqLock);
        }
    inline void Set_REQEXECTIME(DWORD dw)
        {
        InterlockedExchange(PLCounter(ID_REQEXECTIME), (LONG)dw);
        }
    inline void Set_REQWAITTIME(DWORD dw)
        {
        InterlockedExchange(PLCounter(ID_REQWAITTIME), (LONG)dw);
        }
    inline void Incr_REQCOMFAILED()
        {
        InterlockedIncrement(PLCounter(ID_REQCOMFAILED));
        }
    inline void Incr_REQBROWSEREXEC()
        {
        InterlockedIncrement(PLCounter(ID_REQBROWSEREXEC));
        }
    inline void Decr_REQBROWSEREXEC()
        {
        InterlockedDecrement(PLCounter(ID_REQBROWSEREXEC));
        }
    inline void Incr_REQFAILED()
        {
        InterlockedIncrement(PLCounter(ID_REQFAILED));
        }
    inline void Incr_REQNOTAUTH()
        {
        InterlockedIncrement(PLCounter(ID_REQNOTAUTH));
        }
    inline void Incr_REQNOTFOUND()
        {
        InterlockedIncrement(PLCounter(ID_REQNOTFOUND));
        }
    inline DWORD Incr_REQCURRENT()
        {
        return InterlockedIncrement(PLCounter(ID_REQCURRENT));
        }
    inline void Decr_REQCURRENT()
        {
        InterlockedDecrement(PLCounter(ID_REQCURRENT));
        }
    inline void Incr_REQREJECTED()
        {
        InterlockedIncrement(PLCounter(ID_REQREJECTED));
        }
    inline void Incr_REQSUCCEEDED()
        {
        InterlockedIncrement(PLCounter(ID_REQSUCCEEDED));
        }
    inline void Incr_REQTIMEOUT()
        {
        InterlockedIncrement(PLCounter(ID_REQTIMEOUT));
        }
    inline DWORD Incr_REQTOTAL()
        {
        return((DWORD)InterlockedIncrement(PLCounter(ID_REQTOTAL)));
        }
    inline void Incr_REQPERSEC()
        {
        InterlockedIncrement(PLCounter(ID_REQPERSEC));
        }
    inline void Incr_SCRIPTFREEENG()
        {
        InterlockedIncrement(PLCounter(ID_SCRIPTFREEENG));
        }
    inline void Decr_SCRIPTFREEENG()
        {
        InterlockedDecrement(PLCounter(ID_SCRIPTFREEENG));
        }
    inline void Set_SESSIONLIFETIME(DWORD dw)
        {
        InterlockedExchange(PLCounter(ID_SESSIONLIFETIME), (LONG)dw);
        }
    inline void Incr_SESSIONCURRENT()
        {
        InterlockedIncrement(PLCounter(ID_SESSIONCURRENT));
        }
    inline void Decr_SESSIONCURRENT()
        {
        InterlockedDecrement(PLCounter(ID_SESSIONCURRENT));
        }
    inline void Incr_SESSIONTIMEOUT()
        {
        InterlockedIncrement(PLCounter(ID_SESSIONTIMEOUT));
        }
    inline void Incr_SESSIONSTOTAL()
        {
        InterlockedIncrement(PLCounter(ID_SESSIONSTOTAL));
        }
    inline void Incr_TEMPLCACHE()
        {
        InterlockedIncrement(PLCounter(ID_TEMPLCACHE));
        }
    inline void Decr_TEMPLCACHE()
        {
        InterlockedDecrement(PLCounter(ID_TEMPLCACHE));
        }
    inline void Zero_TEMPLCACHE()
        {
        InterlockedExchange(PLCounter(ID_TEMPLCACHE), 0);
        }
    inline void Incr_TEMPLCACHEHITS()
        {
        InterlockedIncrement(PLCounter(ID_TEMPLCACHEHITS));
        }
    inline void Incr_TEMPLCACHETRYS()
        {
        InterlockedIncrement(PLCounter(ID_TEMPLCACHETRYS));
        }
    inline void Incr_MEMORYTEMPLCACHE()
        {
        InterlockedIncrement(PLCounter(ID_MEMORYTEMPLCACHE));
        }
    inline void Decr_MEMORYTEMPLCACHE()
        {
        InterlockedDecrement(PLCounter(ID_MEMORYTEMPLCACHE));
        }
    inline void Zero_MEMORYTEMPLCACHE()
        {
        InterlockedExchange(PLCounter(ID_MEMORYTEMPLCACHE), 0);
        }
    inline void Incr_MEMORYTEMPLCACHEHITS()
        {
        InterlockedIncrement(PLCounter(ID_MEMORYTEMPLCACHEHITS));
        }
    inline void Incr_MEMORYTEMPLCACHETRYS()
        {
        InterlockedIncrement(PLCounter(ID_MEMORYTEMPLCACHETRYS));
        }
    inline void Incr_TEMPLFLUSHES()
        {
        InterlockedIncrement(PLCounter(ID_TEMPLFLUSHES));
        }
    inline void Incr_TRANSABORTED()
        {
        InterlockedIncrement(PLCounter(ID_TRANSABORTED));
        }
    inline void Incr_TRANSCOMMIT()
        {
        InterlockedIncrement(PLCounter(ID_TRANSCOMMIT));
        }
    inline void Incr_TRANSPENDING()
        {
        InterlockedIncrement(PLCounter(ID_TRANSPENDING));
        }
    inline void Decr_TRANSPENDING()
        {
        InterlockedDecrement(PLCounter(ID_TRANSPENDING));
        }
    inline void Incr_TRANSTOTAL()
        {
        InterlockedIncrement(PLCounter(ID_TRANSTOTAL));
        }
    inline void Incr_TRANSPERSEC()
        {
        InterlockedIncrement(PLCounter(ID_TRANSPERSEC));
        }
    inline void Incr_ENGINECACHEHITS()
        {
        InterlockedIncrement(PLCounter(ID_ENGINECACHEHITS));
        }
    inline void Incr_ENGINECACHETRYS()
        {
        InterlockedIncrement(PLCounter(ID_ENGINECACHETRYS));
        }
    inline void Incr_ENGINEFLUSHES()
        {
        InterlockedIncrement(PLCounter(ID_ENGINEFLUSHES));
        }

    };

#ifndef _ASP_DEBUG_EXT

// We init PERFMON data on first request
extern BOOL g_fPerfInited;

// Object to access main shared PERFMON memory
extern CPerfMainBlock g_PerfMain;

// Object to access shared memory (incl. counters) for this process
extern CPerfData g_PerfData;

HRESULT PreInitPerfData();

HRESULT InitPerfDataOnFirstRequest(CIsapiReqInfo    *pIReq);

HRESULT UnInitPerfData();
HRESULT UnPreInitPerfData();

#endif  // _ASP_DEBUG_EXT

#else

#ifndef _ASP_DEBUG_EXT

inline HRESULT PreInitPerfData()
    {
    return S_OK;
    }

inline HRESULT InitPerfDataOnFirstRequest(CIsapiReqInfo    *pIReq) 
    {
    return S_OK; 
    }

inline HRESULT UnInitPerfData()
    {
    return S_OK; 
    }

inline HRESULT UnPreInitPerfData()
    {
    return S_OK; 
    }

#endif  // _ASP_DEBUG_EXT

#endif  // PERF_DISABLE

#endif // _ASP_PERFDATA_H

/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    app_pool_table.h

Abstract:

    The IIS web admin service app pool table class definition.

Author:

    Seth Pollack (sethp)        01-Oct-1998

Revision History:

--*/


#ifndef _APP_POOL_TABLE_H_
#define _APP_POOL_TABLE_H_



//
// prototypes
//

class APP_POOL_TABLE
    : public CTypedHashTable< APP_POOL_TABLE, APP_POOL, const WCHAR * >
{

public:

    APP_POOL_TABLE(
        )
        : CTypedHashTable< APP_POOL_TABLE, APP_POOL, const WCHAR * >
                ( "APP_POOL_TABLE" )
    { /* do nothing*/ }

    ~APP_POOL_TABLE(
        )
    { DBG_ASSERT( Size() == 0 ); }

    static
    const WCHAR *
    ExtractKey(
        IN const APP_POOL * pAppPool
        )  
    { return pAppPool->GetAppPoolId(); }
    
    static
    DWORD
    CalcKeyHash(
        IN const WCHAR * Key
        ) 
    { return HashStringNoCase( Key ); }
    
    static
    bool
    EqualKeys(
        IN const WCHAR * Key1,
        IN const WCHAR * Key2
        )
    { 

        return ( CompareStringNoCase( Key1, Key2 ) == 0 );
    }
    
    static
    void
    AddRefRecord(
        IN APP_POOL * pAppPool,
        IN int IncrementAmount
        ) 
    { /* do nothing*/ 
        UNREFERENCED_PARAMETER( pAppPool );
        UNREFERENCED_PARAMETER( IncrementAmount );    
    }

    VOID
    Shutdown(
        );

    DWORD
    RequestCounters(
        );

    VOID
    ResetAllWorkerProcessPerfCounterState(
        );

    VOID
    Terminate(
        );

    static
    LK_ACTION
    DeleteAppPoolAction(
        IN APP_POOL * pAppPool, 
        IN VOID * pDeleteListHead
        );

    static
    LK_ACTION
    RecordPoolStatesAction(
        IN APP_POOL * pAppPool, 
        IN VOID * pfRecycleAsWell
        );


#if DBG
    VOID
    DebugDump(
        );

    static
    LK_ACTION
    DebugDumpAppPoolAction(
        IN APP_POOL * pAppPool, 
        IN VOID * pIgnored
        );
#endif  // DBG

private:

    APP_POOL_TABLE( const APP_POOL_TABLE & );
    void operator=( const APP_POOL_TABLE & );


};  // APP_POOL_TABLE



#endif  // _APP_POOL_TABLE_H_


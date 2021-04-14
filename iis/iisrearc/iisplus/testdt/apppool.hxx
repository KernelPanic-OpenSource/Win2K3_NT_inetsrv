/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
      DataChannel.hxx

   Abstract:
      Wrapper object for dealing with the Data Channel

   Author:

       Murali R. Krishnan    ( MuraliK )    20-Oct-1998

   Project:

       IIS Worker Process

--*/

# ifndef _APPPOOL_HXX_
# define _APPPOOL_HXX_

#include <locks.h>

//
// class UL_APP_POOL
//
// Encapsulates the data channel for UL
// The data channel is used for all types of request processing
//

class UL_APP_POOL
{
public:
    UL_APP_POOL(
        VOID
    );

    ~UL_APP_POOL(
        VOID
    );

    HRESULT
    Initialize(
        LPCWSTR pszAppPoolName
    );

    HANDLE
    QueryAndLockHandle(
        VOID
    );

    HRESULT
    UnlockHandle(
        VOID
    );

    HRESULT
    Cleanup(
        VOID
    );

private:

    HANDLE              _hAppPool;
    CReaderWriterLock3  _Lock;

}; // class UL_APP_POOL



#endif

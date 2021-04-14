/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    CryDebug.cpp

Abstract:
    Cryptograph debugging

Author:
    Ilan Herbst (ilanh) 06-Mar-00

Environment:
    Platform-independent, _DEBUG only

--*/

#include <libpch.h>
#include "Cry.h"
#include "Cryp.h"

#include "crydebug.tmh"

#ifdef _DEBUG

//---------------------------------------------------------
//
// Validate Cryptograph state
//
void CrypAssertValid(void)
{
    //
    // CryInitalize() has *not* been called. You should initialize the
    // Cryptograph library before using any of its funcionality.
    //
    ASSERT(CrypIsInitialized());

    //
    // TODO:Add more Cryptograph validation code.
    //
}


//---------------------------------------------------------
//
// Initialization Control
//
static LONG s_fInitialized = FALSE;

void CrypSetInitialized(void)
{
    LONG fCryAlreadyInitialized = InterlockedExchange(&s_fInitialized, TRUE);

    //
    // The Cryptograph library has *already* been initialized. You should
    // not initialize it more than once. This assertion would be violated
    // if two or more threads initalize it concurently.
    //
    ASSERT(!fCryAlreadyInitialized);
}


BOOL CrypIsInitialized(void)
{
    return s_fInitialized;
}


#endif // _DEBUG
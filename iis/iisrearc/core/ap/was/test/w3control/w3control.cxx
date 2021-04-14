/*++

Copyright (c) 2000-2000 Microsoft Corporation

Module Name:

    w3control.cxx

Abstract:

    IW3Control interface test app.

Author:

    Seth Pollack (sethp)        21-Feb-2000

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop


//
// Private constants.
//


//
// Private types.
//


//
// Private prototypes.
//


//
// Private globals.
//

// debug support
DECLARE_DEBUG_PRINTS_OBJECT();
DECLARE_DEBUG_VARIABLE();


// usage information
const WCHAR g_Usage[] = 
L"Usage: w3control apppoolId\n";

#define W3_CONTROL_COMMAND_GETMODE W3_CONTROL_COMMAND_CONTINUE + 1
#define W3_CONTROL_COMMAND_RECYCLE W3_CONTROL_COMMAND_CONTINUE + 2

//
// Public functions.
//

INT
__cdecl
wmain(
    INT argc,
    PWSTR argv[]
    )
{

    HRESULT hr = S_OK;
    LPWSTR AppPoolId = NULL;
    IW3Control * pIW3Control = NULL;


    CREATE_DEBUG_PRINT_OBJECT( "w3control" );

    //
    // Validate and parse parameters.
    //

    if ( ( argc < 2 ) || ( argc > 2 ) )
    {
        wprintf( g_Usage );
        goto exit;
    }

    AppPoolId = argv[2];

    //
    // Prepare to make the call.
    //

    hr = CoInitializeEx(
                NULL,                   // reserved
                COINIT_MULTITHREADED    // threading model
                );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Initializing COM failed\n"
            ));

        goto exit;
    }


/*
    DBGPRINTF((
        DBG_CONTEXT, 
        "About to create instance\n"
        ));
*/

    hr = CoCreateInstance( 
                CLSID_W3Control,                    // CLSID
                NULL,                               // controlling unknown
                CLSCTX_SERVER,                      // desired context
                IID_IW3Control,                     // IID
                ( VOID * * ) ( &pIW3Control )       // returned interface
                );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Creating instance of IW3Control failed\n"
            ));

        goto exit;
    }


    //
    // Call the appropriate method.
    //

/*
    DBGPRINTF((
        DBG_CONTEXT, 
        "About to call method\n"
        ));
*/

    hr = pIW3Control->RecycleAppPool( AppPoolId );
    if ( FAILED( hr ) )
    {
        wprintf( L"call failed, hr=%x\n", hr );

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Calling method on IW3Control failed\n"
            ));

        goto exit;
    }

    pIW3Control->Release();


    CoUninitialize();


    DELETE_DEBUG_PRINT_OBJECT();


exit:

    return ( INT ) hr;

}   // wmain


//
// Private functions.
//


/*++

   Copyright    (c)    2000    Microsoft Corporation

   Module Name :
     isapi_request.cxx

   Abstract:
     IIS+ IIsapiCore implementation.

   Author:
     Wade Hilmo (wadeh)             29-Aug-2000

   Project:
     w3core.dll

--*/

#include "precomp.hxx"
#include "isapi_request.hxx"
#include "iisapicore_i.c"
#include "isapi_handler.h"
#include "stringau.hxx"

#define ISAPI_REQUEST_CACHE_THRESHOLD   (400) // Value lifted from IIS 5

#define SZ_FAILED_OOP_REQUEST_LOG_MESSAGE   \
    "Out-of-process+ISAPI+extension+request+failed."

ALLOC_CACHE_HANDLER *   ISAPI_REQUEST::sm_pachIsapiRequest;
PTRACE_LOG              ISAPI_REQUEST::sm_pTraceLog;

HRESULT
SendEntityBodyAndLogDataHelper(
    W3_CONTEXT                 *pW3Context,
    ULATQ_CONTEXT               pContext,
    BOOL                        fAsync,
    DWORD                       dwFlags,
    USHORT                      cChunks,
    HTTP_DATA_CHUNK *           pChunks,
    DWORD                      *pcbSent
);

BOOL
ISAPI_REQUEST::InitClass( VOID )
/*++

Routine Description:

    Acache initialization function

Arguments:

    None

Return Value:

    TRUE on success, FALSE on error

--*/
{
    ALLOC_CACHE_CONFIGURATION  acConfig = { 1, ISAPI_REQUEST_CACHE_THRESHOLD,
                                            sizeof( ISAPI_REQUEST ) };

    if ( NULL != sm_pachIsapiRequest) {

        // already initialized
        return ( TRUE);
    }

    sm_pachIsapiRequest = new ALLOC_CACHE_HANDLER( "IsapiRequest",
                                                    &acConfig);

#if DBG
    sm_pTraceLog = CreateRefTraceLog( 2000, 0 );
#else
    sm_pTraceLog = NULL;
#endif

    return ( NULL != sm_pachIsapiRequest);
}

VOID
ISAPI_REQUEST::CleanupClass( VOID )
/*++

Routine Description:

    Acache cleanup function

Arguments:

    None

Return Value:

    None

--*/
{
    if ( sm_pTraceLog != NULL )
    {
        DestroyRefTraceLog( sm_pTraceLog );
        sm_pTraceLog = NULL;
    }

    if ( NULL != sm_pachIsapiRequest)
    {
        delete sm_pachIsapiRequest;
        sm_pachIsapiRequest = NULL;
    }

    return;
}

HRESULT
ISAPI_REQUEST::Create(
    VOID
    )
/*++

Routine Description:

    Creates a newly allocated ISAPI_REQUEST object.  This
    function does initialization tasks which wouldn't be
    appropriate in the constructor due to potential failure
    during initialization.

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    HRESULT hr = NOERROR;

    DBG_ASSERT( CheckSignature() );
    DBG_ASSERT( _pW3Context );

    IF_DEBUG( ISAPI )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Creating ISAPI_REQUEST %p, W3Context=%p, Handler=%p.\r\n",
            this,
            _pW3Context,
            _pW3Context->QueryHandler()
            ));
    }

    if ( _fIsOop )
    {
        hr = CoCreateFreeThreadedMarshaler(this, &_pUnkFTM);
    }

    if ( FAILED( hr ) )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "ISAPI_REQUEST %p.  Failed to CoCreate free threaded marshaler.\r\n",
            this
            ));

        goto ErrorExit;
    }

    IF_DEBUG( ISAPI )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "ISAPI_REQUEST %p created successfully.\r\n",
            this
            ));
    }

    return hr;

ErrorExit:

    DBG_ASSERT( FAILED( hr ) );

    DBGPRINTF((
        DBG_CONTEXT,
        "Failed to create ISAPI_REQUEST %p.  HRESULT=%08x.\r\n",
        this,
        hr
        ));

    return hr;
}

HRESULT STDMETHODCALLTYPE
ISAPI_REQUEST::QueryInterface(
    REFIID riid,
    void __RPC_FAR *__RPC_FAR *ppvObject
    )
/*++

Routine Description:

    COM Goo

Arguments:

    riid      - Id of the interface requested
    ppvObject - Upon return, points to requested interface

Return Value:

    HRESULT

--*/
{
    DBG_ASSERT( CheckSignature() );
    DBG_ASSERT( ppvObject );

    *ppvObject = NULL;
/*
    DBGPRINTF((
        DBG_CONTEXT,
        "WAM_REQUEST::QueryInterface looking for ... ( " GUID_FORMAT " )\n",
        GUID_EXPAND( &riid)
        ));
*/
    if( riid == IID_IIsapiCore )
    {
        *ppvObject = static_cast<IIsapiCore *>( this );
    }
    else if ( riid == IID_IMarshal )
    {
        if ( _pUnkFTM == NULL )
        {
            DBG_ASSERT(FALSE);
            return E_NOINTERFACE;
        }
        else
        {
            return _pUnkFTM->QueryInterface(riid, ppvObject);
        }
    }
    else if( riid == IID_IUnknown )
    {
        *ppvObject = static_cast<IIsapiCore *>( this );
    }
    else if ( _pUnkFTM != NULL )
    {
        return _pUnkFTM->QueryInterface(riid, ppvObject);
    }
    else
    {
        return E_NOINTERFACE;
    }

    DBG_ASSERT( *ppvObject );
    ((IUnknown *)*ppvObject)->AddRef();
/*
    DBGPRINTF((
        DBG_CONTEXT,
        "WAM_REQUEST::QueryInterface found ( " GUID_FORMAT ", %p )\n",
        GUID_EXPAND( &riid),
        *ppvObject
        ));
*/
    return NOERROR;
}

ULONG STDMETHODCALLTYPE
ISAPI_REQUEST::AddRef(
    void
    )
/*++

Routine Description:

    COM Goo - adds a reference to the object

Arguments:

    None

Return Value:

    The number of references remaining at completion of this call

--*/
{
    LONG cRefs;

    DBG_ASSERT( CheckSignature() );

    cRefs = InterlockedIncrement( &_cRefs );

    //
    // Log the reference ( sm_pTraceLog!=NULL if DBG=1)
    //

    if ( sm_pTraceLog != NULL )
    {
        WriteRefTraceLog( sm_pTraceLog,
                          cRefs,
                          this );
    }

    return cRefs;
}

ULONG STDMETHODCALLTYPE
ISAPI_REQUEST::Release(
    void
    )
/*++

Routine Description:

    COM Goo - deletes a referece to the object, and deletes
              the object upon zero references

Arguments:

    None

Return Value:

    The number of references remaining at completion of this call

--*/
{
    LONG cRefs;
    BOOL fIsOop = _fIsOop;
    W3_ISAPI_HANDLER * pW3IsapiHandler = NULL;

    DBG_ASSERT( CheckSignature() );

    //
    // WARNING - This object is always created by W3_ISAPI_HANDLER,
    //           and that code uses the return value from Release
    //           to determine if it's safe to advance the core
    //           state machine.  It is essential that this function
    //           only return 0 in the case where it's called delete.
    //

    cRefs = InterlockedDecrement( &_cRefs );

    if ( sm_pTraceLog != NULL )
    {
        WriteRefTraceLog( sm_pTraceLog,
                          cRefs,
                          this );
    }

    if ( ( cRefs == 1 ) && fIsOop )
    {
        _pWamProcess->RemoveIsapiRequestFromList( this );
    }

    if ( cRefs == 0 )
    {
        DBG_ASSERT( _pW3Context );
        DBG_REQUIRE( ( pW3IsapiHandler = (W3_ISAPI_HANDLER*)_pW3Context->QueryHandler() ) != NULL );
        
        delete this;

        //
        // Notify the W3_ISAPI_HANDLER that we are done with it
        //
        
        pW3IsapiHandler->IsapiRequestFinished();

        return 0;
    }

    return cRefs;
}

HRESULT STDMETHODCALLTYPE
ISAPI_REQUEST::GetServerVariable(
    LPSTR           szVariableName,
    BYTE *          szBuffer,
    DWORD           cbBuffer,
    DWORD *         pcbBufferRequired
    )
/*++

Routine Description:

    Returns the value of a server variable

Arguments:

    szVariableName    - The name of the server variable
    szBuffer          - Upon return, contains the value of the server variable
    cbBuffer          - The size of szBuffer.
    pcbBufferRequired - On successful return, the number of bytes copied
                        to the buffer.  On failure, the number of bytes
                        required to hold szBuffer.

Return Value:

    HRESULT

--*/
{
    HRESULT         hr = NOERROR;

    //
    // The only current caller for this function is w3isapi.dll, which does
    // parameter validation from any outside code.  So, we can get away with
    // just asserting here.
    //

    DBG_ASSERT( CheckSignature() );
    DBG_ASSERT( _pW3Context );
    DBG_ASSERT( szVariableName );
    DBG_ASSERT( pcbBufferRequired );
    DBG_ASSERT( szBuffer || ( cbBuffer == 0 ) );

    hr = SERVER_VARIABLE_HASH::GetServerVariable(
        _pW3Context,
        szVariableName,
        (LPSTR)szBuffer,
        &cbBuffer
        );

    *pcbBufferRequired = cbBuffer;

    return hr;
}

HRESULT STDMETHODCALLTYPE
ISAPI_REQUEST::ReadClient(
    DWORD64 IsaContext,
    BYTE *pBuffer,
    DWORD cbBuffer,
    DWORD dwBytesToRead,
    DWORD *pdwSyncBytesRead,
    DWORD dwFlags
    )
 /*++

Routine Description:

    Reads data from the client

Arguments:

    IsaContext       - The ISAPI_CONTEXT for this request (opaque)
    pBuffer          - Contains read data upon return for sync reads
    cbBuffer         - The size of pSyncReadBuffer
    dwBytesToRead    - The number of bytes to read
    pdwSyncBytesRead - The number of bytes copied to pBuffer in sync case
    dwFlags          - HSE_IO_* flags from caller

Return Value:

    HRESULT

--*/
{
    DWORD   dwBytesRead;
    HRESULT hr = NOERROR;
    BOOL    fAsync = !!( dwFlags & HSE_IO_ASYNC );

    //
    // The only current caller for this function is w3isapi.dll, which does
    // parameter validation from any outside code.  So, we can get away with
    // just asserting here.
    //

    DBG_ASSERT( CheckSignature() );
    DBG_ASSERT( _pW3Context );
    DBG_ASSERT( fAsync == !!IsaContext );
    DBG_ASSERT( dwBytesToRead );
    DBG_ASSERT( pdwSyncBytesRead || fAsync );

    //
    // Make /W4 happy...
    //

    if ( pBuffer == NULL && cbBuffer != 0 )
    {
        DBG_ASSERT( pBuffer || ( cbBuffer == 0 ) );
    }

    if ( fAsync )
    {
        DBG_ASSERT( _IsapiContext == 0 );
        _IsapiContext = IsaContext;
    }

    //
    // If this is an OOP async read, then we need to create a
    // local buffer to read into.
    //

    if ( _fIsOop && fAsync )
    {
        DBG_ASSERT( _pAsyncReadBuffer == NULL );

        _pAsyncReadBuffer = (LPBYTE)LocalAlloc( LPTR, dwBytesToRead );

        if ( !_pAsyncReadBuffer )
        {
            goto ErrorExit;
        }

        //
        // This reference insures that if the OOP host crashes and
        // COM releases all the OOP-held references, that this
        // object will survive until the I/O completion occurs.
        //

        AddRef();

        hr = _pW3Context->ReceiveEntity(
            fAsync ? W3_FLAG_ASYNC : W3_FLAG_SYNC,
            _pAsyncReadBuffer,
            dwBytesToRead,
            &dwBytesRead
            );
    }
    else
    {
        hr = _pW3Context->ReceiveEntity(
            fAsync ? W3_FLAG_ASYNC : W3_FLAG_SYNC,
            pBuffer,
            dwBytesToRead,
            fAsync ? &dwBytesRead : pdwSyncBytesRead
            );
    }

    //
    // If the request is chunked, look for ERROR_HANDLE_EOF.  This
    // is how http.sys signals the end of a chunked request.
    //
    // Since an ISAPI extension is looking for a successful, zero
    // byte read, we'll need to change the result of the above call.
    //
    // Note that on an asynchronous call, we'll need to trigger a
    // "fake" completion with zero bytes.
    //

    if ( FAILED( hr ) && _pW3Context->QueryRequest()->IsChunkedRequest() )
    {
        if ( hr == HRESULT_FROM_WIN32( ERROR_HANDLE_EOF ) )
        {
            hr = NOERROR;

            if ( fAsync )
            {
                POST_MAIN_COMPLETION( _pW3Context->QueryMainContext() );
            }
            else
            {
                *pdwSyncBytesRead = 0;
            }
        }
    }

    //
    // We now return you to your regular error handling program.
    //

    if ( FAILED( hr ) )
    {
        //
        // If ReceiveEntity fails for some reason other than
        // EOF, then consider this a non recoverable
        // error and set the error to be WSAECONNRESET for
        // compatibility with legacy ISAPI code.
        //

        hr = HRESULT_FROM_WIN32( WSAECONNRESET );

        if ( _fIsOop && fAsync )
        {
            //
            // Release the above reference, since no completion
            // will be coming.
            //

            Release();
        }

        goto ErrorExit;
    }

    return hr;

ErrorExit:

    DBG_ASSERT( FAILED( hr ) );

    if ( fAsync &&
         _IsapiContext != 0 )
    {
        _IsapiContext = 0;
    }

    if ( _pAsyncReadBuffer )
    {
        LocalFree( _pAsyncReadBuffer );
        _pAsyncReadBuffer = NULL;
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE
ISAPI_REQUEST::WriteClient(
    DWORD64         IsaContext,
    BYTE *          pBuffer,
    DWORD           cbBuffer,
    DWORD           dwFlags
    )
/*++

Routine Description:

    Writes data to the client

Arguments:

    IsaContext - The ISAPI_CONTEXT for this request (opaque)
    pBuffer    - Contains the data to write
    cbBuffer   - The amount of data to be written
    dwFlags    - HSE_IO_* flags from caller

Return Value:

    HRESULT

--*/
{
    W3_RESPONSE *   pResponse;
    HRESULT         hr = NOERROR;
    BOOL            fAsync = !!( dwFlags & HSE_IO_ASYNC );

    //
    // The only current caller for this function is w3isapi.dll, which does
    // parameter validation from any outside code.  So, we can get away with
    // just asserting here.
    //

    DBG_ASSERT( CheckSignature() );
    DBG_ASSERT( _pW3Context );
    DBG_ASSERT( fAsync == !!IsaContext );
    DBG_ASSERT( pBuffer || ( cbBuffer == 0 ) );

    //
    // For a zero byte send, just return
    //
    // If the call was async, we'll need to fire off a completion.
    //
    // Note that this case shouldn't ever happen, as the w3isapi.dll
    // code that calls this currently does a check for a zero byte
    // write attempt.  That code is somewhat broken in that a completion
    // will never occur for a zero byte completion.  That's the way all
    // previous versions of ISAPI have worked, though.  This code is
    // here for the sole purpose that this interface could work properly
    // with an API that expects a completion on a zero byte async write.
    //

    if ( cbBuffer == 0 )
    {
        if ( fAsync )
        {
            POST_MAIN_COMPLETION( _pW3Context->QueryMainContext() );
        }

        return hr;
    }

    if ( fAsync )
    {
        DBG_ASSERT( _IsapiContext == 0 );
        _IsapiContext = IsaContext;
    }

    DBG_REQUIRE( ( pResponse = _pW3Context->QueryResponse() ) != NULL );

    //
    // If this as an OOP async write then we will work from a local copy
    // of pBuffer.
    //

    if ( _fIsOop && fAsync )
    {
        DBG_ASSERT( _pAsyncWriteBuffer == NULL );

        _pAsyncWriteBuffer = (LPBYTE)LocalAlloc( LPTR, cbBuffer );

        if ( !_pAsyncWriteBuffer )
        {
            goto ErrorExit;
        }

        memcpy( _pAsyncWriteBuffer, pBuffer, cbBuffer );

        pBuffer = _pAsyncWriteBuffer;
    }

    //
    // Before sending the current data, we need to clear out
    // any outstanding chunks from the response object.  This can't
    // ever cause a problem for a purely synchronous ISAPI.  And,
    // since w3isapi.dll protects against multiple outstanding
    // asynchronous I/O, we shouldn't see a problem with a
    // purely asynchronous I/O.
    //
    // If an ISAPI sends data asynchronously and then follows up
    // with a second, synchronous send, then it's possible that
    // the second send could clear the chunks from the first before
    // they've been fully processed.  This is a really, really
    // dumb thing for an ISAPI to do, since the response would
    // likely be scrambled at the client.  So, we'll live with
    // problems in that scenario.
    //

    pResponse->Clear();

    if (!_pW3Context->QueryResponseSent())
    {
        _pW3Context->SetDisconnect( TRUE );
    }

    //
    // Now setup the buffer we want to send
    //

    hr = pResponse->AddMemoryChunkByReference(
        pBuffer,
        cbBuffer
        );

    if ( FAILED( hr ) )
    {
        goto ErrorExit;
    }

    //
    // Ok, now send what we've got.
    //

    if ( _fIsOop && fAsync )
    {
        //
        // This reference insures that if the OOP host crashes and
        // COM releases all the OOP-held references, that this
        // object will survive until the I/O completion occurs.
        //

        AddRef();
    }

    hr = _pW3Context->SendEntity(
        ( fAsync ? W3_FLAG_ASYNC : W3_FLAG_SYNC ) | W3_FLAG_MORE_DATA
        );

    if ( FAILED( hr ) )
    {
        //
        // If SendEntity fails, consider this a non recoverable
        // error and set the error to be WSAECONNRESET for
        // compatibility with legacy ISAPI code.
        //

        hr = HRESULT_FROM_WIN32( WSAECONNRESET );

        if ( _fIsOop && fAsync )
        {
            //
            // Release the above reference, since no I/O completion
            // will ever happen.
            //

            Release();
        }

        goto ErrorExit;
    }

    return hr;

ErrorExit:

    DBG_ASSERT( FAILED( hr ) );

    //
    // Note that checking for async here is not an optimization.
    //
    // ISAPI guards against two concurrent async operations, but
    // it's valid to have a synchronous and asynchronous operation
    // happening currently (ie. async write, exec url, etc.,
    // concurrent with a synchronous read).
    //
    // If we don't check for async, it's possible for a failed
    // synchronous read to reset the _IsapiContext that belongs
    // to some other action that's running async.
    //

    if ( fAsync &&
         _IsapiContext != 0 )
    {
        _IsapiContext = 0;
    }

    if ( _pAsyncWriteBuffer )
    {
        LocalFree( _pAsyncWriteBuffer );
        _pAsyncWriteBuffer = NULL;
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE
ISAPI_REQUEST::SendResponseHeaders(
    BOOL            fDisconnect,
    LPSTR           szStatus,
    LPSTR           szHeaders,
    DWORD
    )
/*++

Routine Description:

    Sends response headers to the client

Arguments:

    fDisconnect - If FALSE, then we need to avoid closing the connection
    szStatus    - The status to send (ie. "200 OK")
    szHeaders   - The headers to send (ie. "foo: value1\r\nBar: value2\r\n")
    dwFlags     - HSE_IO_* flags from caller

Return Value:

    HRESULT

--*/
{
    W3_RESPONSE   * pResponse;
    STACK_STRA(     strStatus,64);
    STACK_STRA(     strHeaders,128);
    LPSTR           szCookedStatus = szStatus;
    LPSTR           szCookedHeaders = szHeaders;
    LPSTR           pCursor = NULL;
    HRESULT         hr;

    //
    // The only current caller for this function is w3isapi.dll, which does
    // parameter validation from any outside code.  So, we can get away with
    // just asserting here.
    //

    DBG_ASSERT( CheckSignature() );
    DBG_ASSERT( _pW3Context );
    DBG_REQUIRE( ( pResponse = _pW3Context->QueryResponse() ) != NULL );

    //
    // Legacy ISAPI Gymnastics:
    //
    // Some ISAPI extensions will try and piggy-back response
    // headers onto the status string that they pass to this
    // function (ie. "200 OK\r\nFoo: bar".)  While this would
    // work in IIS 5 - since IIS 5 didn't try and validate the
    // stream - it doesn't fly with http.sys.  Ultimately, this
    // will cause http.sys to fail our send response call.
    //
    // So, to accommodate these ISAPIs, we'll look for this and
    // move any such piggy-backed headers into the right argument.
    //

    if ( szStatus )
    {
        pCursor = strpbrk( szStatus, "\r\n" );
    }

    if ( pCursor )
    {
        //
        // Create a local copy of the status and
        // strip off the extra header data.
        //

        hr = strStatus.Copy( szStatus );

        if ( FAILED( hr ) )
        {
            return ( hr );
        }

        strStatus.SetLen( pCursor - szStatus );

        //
        // Eat the \r\n preceding the header
        //

        while ( *pCursor == '\r' ||
                *pCursor == '\n' )
        {
            pCursor++;
        }

        //
        // Create a local copy of the headers
        //

        hr = strHeaders.Copy( pCursor );

        if ( FAILED( hr ) )
        {
            return hr;
        }

        //
        // We'll want to make sure that strHeaders ends
        // properly with a "\r\n" at this point.  Probably the
        // easiest way to do this is to just eat any trailing
        // '\r' or '\n' characters and then append it (some ISAPI
        // developers omit the '\r' and just go with '\n'...)
        //

        pCursor = strHeaders.QueryStr() + strHeaders.QueryCCH() - 1;

        while ( pCursor > strHeaders.QueryStr() &&
                ( *pCursor == '\r' ||
                  *pCursor == '\n' ) )
        {
            pCursor--;
        }

        strHeaders.SetLen( pCursor - strHeaders.QueryStr() + 1 );

        hr = strHeaders.Append( "\r\n" );

        if ( FAILED( hr ) )
        {
            return hr;
        }

        //
        // If we were passed a non-NULL pointer for szHeaders, we'll
        // need to append that data to strHeaders.  We'll assume that
        // szHeaders was correctly formed by the caller and skip the
        // "\r\n" games.
        //
        // Else, if we were passed a NULL pointer for szHeaders, we
        // still need to append the final "\r\n" before the entity
        // body.
        //

        if ( szHeaders )
        {
            hr = strHeaders.Append( szHeaders );
        }
        else
        {
            hr = strHeaders.Append( "\r\n" );
        }

        if ( FAILED( hr ) )
        {
            return hr;
        }

        //
        // Finally, set szCookedStatus and szCookedHeaders to point
        // at our new data.
        //

        szCookedStatus = strStatus.QueryStr();
        szCookedHeaders = strHeaders.QueryStr();
    }

    //
    // Set the disconnect disposition
    //

    _pW3Context->SetDisconnect( fDisconnect );

    //
    // Need to clear any existing response
    //

    pResponse->Clear();

    //
    // Setup response from ISAPI
    //

    hr = pResponse->BuildResponseFromIsapi(
        _pW3Context,
        szCookedStatus,
        szCookedHeaders,
        szCookedHeaders ? (DWORD)strlen( szCookedHeaders ) : 0
        );

    if ( FAILED( hr ) )
    {
        return hr;
    }

    //
    // Is the status is access denied, then set the sub status to
    // "Denied by Application"
    //

    if ( pResponse->QueryStatusCode() == HttpStatusUnauthorized.statusCode )
    {
        pResponse->SetStatus( HttpStatusUnauthorized,
                              Http401Application );
    }

    hr =  _pW3Context->SendResponse(
          W3_FLAG_SYNC
          | W3_FLAG_MORE_DATA
          | W3_FLAG_NO_ERROR_BODY
          | W3_FLAG_NO_CONTENT_LENGTH );

    if ( FAILED( hr ) )
    {
        //
        // If SendResponse fails, consider this a non recoverable
        // error and set the error to be WSAECONNRESET for
        // compatibility with legacy ISAPI code.
        //

        hr = HRESULT_FROM_WIN32( WSAECONNRESET );
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE
ISAPI_REQUEST::MapPath(
    BYTE *          szPath,
    DWORD           cbPath,
    DWORD *         pcbBufferRequired,
    BOOL            fUnicode
    )
/*++

Routine Description:

    Maps a URL to a physical path

Arguments:

    szPath            - On entry, the URL to map
                      - On return, the mapped physical path
    cbPath            - The size of szPath
    pcbBufferRequired - On successful return, the number of szPath
                        On error return, the number of bytes needed in szPath
    fUnicode          - If TRUE, szPath should be a UNICODE string on entry
                        and return

Return Value:

    HRESULT

--*/
{
    STACK_STRU(     struUrl,MAX_PATH );
    STACK_STRU(     struPath,MAX_PATH );
    HRESULT         hr;

    //
    // The only current caller for this function is w3isapi.dll, which does
    // parameter validation from any outside code.  So, we can get away with
    // just asserting here.
    //

    DBG_ASSERT( CheckSignature() );
    DBG_ASSERT( _pW3Context );
    DBG_ASSERT( szPath || ( cbPath == 0 ) );
    DBG_ASSERT( pcbBufferRequired );

    //
    // This is kind of a weird function - the return string
    // gets copied over the top of the source string.
    //

    if ( fUnicode )
    {
        hr = struUrl.Copy( (LPWSTR)szPath );
    }
    else
    {
        hr = struUrl.CopyA( (LPSTR)szPath );
    }

    if ( FAILED( hr ) )
    {
        return hr;
    }

    hr = W3_STATE_URLINFO::MapPath(
        _pW3Context,
        struUrl,
        &struPath,
        TRUE,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
        );

    if ( FAILED( hr ) )
    {
        return hr;
    }

    *pcbBufferRequired = cbPath;

    if ( fUnicode )
    {
        return struPath.CopyToBuffer( (LPWSTR)szPath, pcbBufferRequired );
    }
    else
    {
        STACK_STRA (straPath, MAX_PATH );

        if (FAILED(hr = straPath.CopyW(struPath.QueryStr(),
                                       struPath.QueryCCH())))
        {
            return hr;
        }

        return straPath.CopyToBuffer( (LPSTR)szPath, pcbBufferRequired );
    }
}

HRESULT STDMETHODCALLTYPE
ISAPI_REQUEST::MapPathEx(
    BYTE *          szUrl,
    DWORD           ,
    BYTE *          szPath,
    DWORD           cbPath,
    DWORD *         pcbBufferRequired,
    DWORD *         pcchMatchingPath,
    DWORD *         pcchMatchingUrl,
    DWORD *         pdwFlags,
    BOOL            fUnicode
    )
/*++

Routine Description:

    Does path mapping, plus a bit more

Arguments:

    szUrl             - The URL to map
    szPath            - Upon return, the physical path for the URL
    cbPath            - The size of szPath
    pcbBufferRequired - Upon failed return, the size needed for szPath
    pcchMatchingPath  - Upon return, the number of characters in szPath
                        that correspond to the vroot in the URL
    pcchMatchingUrl   - Upon return, the number of characters in szUrl
                        that correspond to the vroot in the URL
    pdwFlags          - Upon return, the metadata AccessPerm flags for the URL
    fUnicode          - If TRUE, the caller wants to talk UNICODE

Return Value:

    HRESULT

--*/
{
    STACK_STRU( struUrl,MAX_PATH );
    STACK_STRU( struPath,MAX_PATH );
    DWORD       cbPathCopied;
    DWORD       cchMatchingPathA;
    DWORD       cchMatchingUrlA;
    HRESULT     hr;

    DBG_ASSERT( CheckSignature() );
    DBG_ASSERT( _pW3Context );

    if ( fUnicode )
    {
        hr = struUrl.Copy( (LPWSTR)szUrl );
    }
    else
    {
        hr = struUrl.CopyA( (LPSTR)szUrl );
    }

    if ( FAILED( hr ) )
    {
        return hr;
    }

    //
    // Call W3_STATE_URLINFO::MapPath to do the work
    //

    hr = W3_STATE_URLINFO::MapPath(
        _pW3Context,
        struUrl,
        &struPath,
        TRUE,
        pcchMatchingPath,
        pcchMatchingUrl,
        &cchMatchingPathA,
        &cchMatchingUrlA,
        pdwFlags
        );

    if ( FAILED( hr ) )
    {
        return hr;
    }

    //
    // W3_STATE_URLINFO::MapPath works by looking for a cache entry
    // for the requested szUrl.  If it exists, the returned information
    // will come from the cache data.  This causes a potential problem
    // if the cache contains a URL as "/foo/" and the caller to this
    // function passes "/foo" as the URL.  In this case, *pcchMatchingUrl
    // will be 5, as it's derived from the cache data.  It would be bad,
    // though, to pass a *pcchMatchingUrl greater than the length of the
    // szUrl we were given...
    //

    if ( fUnicode )
    {
        if ( *pcchMatchingUrl &&
            ( ( ((LPWSTR)szUrl)[*pcchMatchingUrl - 1] == L'\0' ) ||
              ( ((LPWSTR)szUrl)[*pcchMatchingUrl - 1] == L'/' ) ) )
        {
            (*pcchMatchingUrl)--;
        }
    }
    else
    {
        //
        // Use the ANSI matching value
        //

        *pcchMatchingUrl = cchMatchingUrlA;

        if ( *pcchMatchingUrl &&
            ( ( szUrl[*pcchMatchingUrl - 1] == '\0' ) ||
              ( szUrl[*pcchMatchingUrl - 1] == '/' ) ) )
        {
            (*pcchMatchingUrl)--;
        }
    }

    if ( pcbBufferRequired )
    {
        if ( fUnicode )
        {
            *pcbBufferRequired = (struPath.QueryCCH() + 1) * sizeof(WCHAR);
        }
        else
        {
            *pcbBufferRequired = struPath.QueryCCH() + 1;
        }
    }

    if ( szPath )
    {
        if ( fUnicode )
        {
            cbPathCopied = cbPath;

            hr = struPath.CopyToBuffer( (LPWSTR)szPath, &cbPathCopied );

            if ( hr == HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER ) &&
                 cbPath >= sizeof(WCHAR) )
            {
                //
                // Copy what we can.
                //
                // Note that there are ISAPI extensions that depend on
                // being able to get a truncated path in the case where
                // MAX_PATH is not sufficient to store the complete data.
                //
                // These extensions depend on using ERROR_INSUFFICIENT_BUFFER
                // as a warning that the data is truncated.
                //

                memset( szPath, 0, cbPath );
                memcpy( szPath, struPath.QueryStr(), cbPath - sizeof(WCHAR) );
            }

        }
        else
        {
            //
            // Convert the path to ANSI
            //

            STACK_STRA(    strAnsiPath, MAX_PATH+1 );
            DWORD        cbAnsiPath;
            DWORD        dwError;

            //
            // Get the ANSI version of the path.
            //

            cbAnsiPath = WideCharToMultiByte( CP_ACP,
                                              0,
                                              struPath.QueryStr(),
                                              -1,
                                              strAnsiPath.QueryStr(),
                                              MAX_PATH,
                                              NULL,
                                              NULL );

            if ( cbAnsiPath == 0 )
            {
                dwError = GetLastError();

                if ( dwError == ERROR_INSUFFICIENT_BUFFER )
                {
                    //
                    // Get the size of the buffer needed and
                    // call it again.
                    //

                    cbAnsiPath = WideCharToMultiByte( CP_ACP,
                                                      0,
                                                      struPath.QueryStr(),
                                                      -1,
                                                      NULL,
                                                      0,
                                                      NULL,
                                                      NULL );

                    if ( cbAnsiPath == 0 )
                    {
                        return HRESULT_FROM_WIN32( GetLastError() );
                    }

                    hr = strAnsiPath.Resize( cbAnsiPath );

                    if ( FAILED( hr ) )
                    {
                        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
                    }

                    cbAnsiPath = WideCharToMultiByte( CP_ACP,
                                                      0,
                                                      struPath.QueryStr(),
                                                      -1,
                                                      strAnsiPath.QueryStr(),
                                                      cbAnsiPath,
                                                      NULL,
                                                      NULL );

                    if ( cbAnsiPath == 0 )
                    {
                        return HRESULT_FROM_WIN32( GetLastError() );
                    }
                }
                else
                {
                    return HRESULT_FROM_WIN32( dwError );
                }
            }

            cbPathCopied = cbPath;
            strAnsiPath.SetLen( cbAnsiPath - 1 );

            hr = strAnsiPath.CopyToBuffer(
                (LPSTR)szPath,
                &cbPathCopied
                );

            if ( hr == HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER ) &&
                 cbPath >= sizeof(CHAR) )
            {
                //
                // Copy what we can.
                //
                // Note that there are ISAPI extensions that depend on
                // being able to get a truncated path in the case where
                // MAX_PATH is not sufficient to store the complete data.
                //
                // These extensions depend on using ERROR_INSUFFICIENT_BUFFER
                // as a warning that the data is truncated.
                //

                memset( szPath, 0, cbPath );
                memcpy( szPath, strAnsiPath.QueryStr(), cbPath - sizeof(CHAR) );
            }
        }
    }

    if ( pcchMatchingPath &&
         !fUnicode )
    {
        //
        // Use the ANSI matching value
        //

        *pcchMatchingPath = cchMatchingPathA;
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE
ISAPI_REQUEST::TransmitFile(
    DWORD64         IsaContext,
    DWORD_PTR       hFile,
    DWORD64         cbOffset,
    DWORD64         cbWrite,
    LPSTR           szStatusCode,
    BYTE *          pHead,
    DWORD           cbHead,
    BYTE *          pTail,
    DWORD           cbTail,
    DWORD           dwFlags
    )
/*++

Routine Description:

    Transmits a file to the client

Arguments:

    IsaContext   - The ISAPI_CONTEXT for this request (opaque)
    hFile        - Handle to file (requires FILE_FLAG_SEQUENTIAL_SCAN)
    cbOffset     - Offset in file to begin transmitting
    cbWrite      - The number of bytes to transmit
    szStatusCode - HTTP status to return (ie "200 OK")
    pHead        - Bytes to send before file data
    cbHead       - The size of pHead
    pTail        - Bytes to send after file data
    cbTail       - The size of pTail
    dwFlags      - HSE_IO_* flags from the caller

Return Value:

    HRESULT

--*/
{
    W3_ISAPI_HANDLER *  pW3IsapiHandler;
    W3_RESPONSE *       pResponse;
    DWORD               dwW3Flags;
    BOOL                fSendAsResponse;
    HRESULT             hr;

    //
    // The only current caller for this function is w3isapi.dll, which does
    // parameter validation from any outside code.  So, we can get away with
    // just asserting here.
    //

    DBG_ASSERT( CheckSignature() );
    DBG_ASSERT( _pW3Context );
    DBG_ASSERT( IsaContext );
    DBG_ASSERT( cbHead == 0 || pHead != NULL );
    DBG_ASSERT( cbTail == 0 || pTail != NULL );
    DBG_ASSERT( (HANDLE)hFile != INVALID_HANDLE_VALUE );
    DBG_ASSERT( dwFlags & HSE_IO_ASYNC );
    DBG_REQUIRE( ( pResponse = _pW3Context->QueryResponse() ) != NULL );
    DBG_REQUIRE( ( pW3IsapiHandler = (W3_ISAPI_HANDLER*)_pW3Context->QueryHandler() ) != NULL );

    DBG_ASSERT( _IsapiContext == 0 );
    _IsapiContext = IsaContext;

    //
    // If the caller is OOP, then make copies
    // of the file handle, head data and tail data.  We
    // don't need to make a copy of the status because
    // the BuildResponseFromIsapi function does that itself.
    //

    if ( _fIsOop )
    {
        if ( hFile != NULL )
        {
            hr = pW3IsapiHandler->DuplicateWamProcessHandleForLocalUse(
                (HANDLE)hFile, &_hTfFile
                );

            if ( FAILED( hr ) )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );
                goto ErrorExit;
            }

            hFile = (DWORD_PTR)_hTfFile;
        }

        if ( pHead )
        {
            _pTfHead = (LPBYTE)LocalAlloc( LPTR, cbHead );

            if ( !_pTfHead )
            {
                hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
                goto ErrorExit;
            }

            memcpy( _pTfHead, pHead, cbHead );

            pHead = _pTfHead;
        }

        if ( pTail )
        {
            _pTfTail = (LPBYTE)LocalAlloc( LPTR, cbTail );

            if ( !_pTfTail )
            {
                hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
                goto ErrorExit;
            }

            memcpy( _pTfTail, pTail, cbTail );

            pTail = _pTfTail;
        }
    }

    //
    // Convert the HSE_IO_* flags to W3_FLAG_* flags
    //

    // Init the flags for sending
    dwW3Flags = W3_FLAG_ASYNC | W3_FLAG_MORE_DATA;

    // If disconnect flag is not set, then we'll keep the connection open
    if ( !( dwFlags & HSE_IO_DISCONNECT_AFTER_SEND ) )
    {
        _pW3Context->SetDisconnect( FALSE );
    }
    else
    {
        _pW3Context->SetDisconnect( TRUE );
    }

    //
    // Clear any previous chunks from the response
    //

    pResponse->Clear();

    //
    // If HSE_IO_SEND_HEADERS is specified, then we're sending
    // the initial part of the response (and pHead will be an
    // LPSTR containing the headers that the caller wants to
    // send), else we'll be sending this data completely as
    // entity data.
    //

    fSendAsResponse = !!( dwFlags & HSE_IO_SEND_HEADERS );

    if ( fSendAsResponse )
    {
        //
        // Set the status using data from the caller
        //

        hr = pResponse->BuildResponseFromIsapi( _pW3Context,
                                                szStatusCode,
                                                (LPSTR)pHead,
                                                cbHead );
        if ( FAILED( hr ) )
        {
            goto ErrorExit;
        }

        //
        // Is the status is access denied, then set the sub status to
        // "Denied by Application"
        //

        if ( pResponse->QueryStatusCode() == HttpStatusUnauthorized.statusCode )
        {
            pResponse->SetStatus( HttpStatusUnauthorized,
                                  Http401Application );
        }
    }
    else
    {
        //
        // Do something with pHead if provided
        //

        if ( cbHead )
        {
            hr = pResponse->AddMemoryChunkByReference(
                    pHead,
                    cbHead
                );

            if ( FAILED( hr ) )
            {
                goto ErrorExit;
            }
        }
    }

    //
    // Now add the file handle to the response.  Note that it's
    // allowed for the caller to pass a NULL handle.  In that case,
    // we won't add it, and any present pHead and pTail will still
    // get sent to the client.
    //

    if ( hFile )
    {
        hr = pResponse->AddFileHandleChunk(
                (HANDLE)hFile,
                cbOffset,
                cbWrite
                );

        if ( FAILED( hr ) )
        {
            goto ErrorExit;
        }
    }

    //
    // Add the tail if provided
    //

    if ( cbTail )
    {
        hr = pResponse->AddMemoryChunkByReference(
            pTail,
            cbTail
            );

        if ( FAILED( hr ) )
        {
            goto ErrorExit;
        }
    }

    //
    // Ok, now that the stuff is all set up, send it, either
    // as a response or as entity
    //

    if ( _fIsOop )
    {
        //
        // This reference insures that if the OOP host crashes and
        // COM releases all the OOP-held references, that this
        // object will survive until the I/O completion occurs.
        //

        AddRef();
    }

    if ( fSendAsResponse )
    {
        hr = _pW3Context->SendResponse( dwW3Flags
                                        | W3_FLAG_NO_ERROR_BODY
                                        | W3_FLAG_NO_CONTENT_LENGTH 
                                        | W3_FLAG_MORE_DATA );
    }
    else
    {
        hr = _pW3Context->SendEntity( dwW3Flags 
                                      | W3_FLAG_MORE_DATA );
    }

    if ( FAILED( hr ) )
    {

        //
        // If SendEntity or SendResponse fail, consider this
        // a non recoverable error and set the error to be
        // WSAECONNRESET for compatibility with legacy ISAPI code.
        //

        hr = HRESULT_FROM_WIN32( WSAECONNRESET );

        if ( _fIsOop )
        {
            //
            // Release the above reference, since no I/O completion will
            // occur.
            //

            Release();
        }

        goto ErrorExit;
    }

    return hr;

ErrorExit:

    DBG_ASSERT( FAILED( hr ) );

    if ( _IsapiContext != 0 )
    {
        _IsapiContext = 0;
    }

    if ( _fIsOop )
    {
        if ( _hTfFile != NULL && _hTfFile != INVALID_HANDLE_VALUE )
        {
            CloseHandle( _hTfFile );
            _hTfFile = INVALID_HANDLE_VALUE;
        }

        if ( _pTfHead )
        {
            LocalFree( _pTfHead );
            _pTfHead = NULL;
        }

        if ( _pTfTail )
        {
            LocalFree( _pTfTail );
            _pTfTail = NULL;
        }
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE
ISAPI_REQUEST::SetConnectionClose(
    BOOL    fClose
    )
/*++

Routine Description:

    Sets the W3_CONTEXT to close (or not) connection
    upon completion of the response.

Arguments:

    fClose - BOOL to pass to SetDisconnect

Return Value:

    HRESULT

--*/
{
    DBG_ASSERT( CheckSignature() );

    _pW3Context->SetDisconnect( fClose );

    return NOERROR;
}

HRESULT STDMETHODCALLTYPE
ISAPI_REQUEST::SendRedirect(
    LPCSTR          szLocation,
    BOOL            fDisconnect
    )
/*++

Routine Description:

    Sends a 302 redirect message to the client

Arguments:

    szLocation - The URL to redirect to.
    fDisconnect - If TRUE, the close connection

Return Value:

    HRESULT

--*/
{
    STACK_STRA(     strLocation, MAX_PATH );
    HTTP_STATUS httpStatus = { 302, REASON("Object Moved") };
    HRESULT         hr;

    DBG_ASSERT( CheckSignature() );
    DBG_ASSERT( _pW3Context );
    DBG_ASSERT( szLocation );

    //
    // Use W3_CONTEXT::SetupHttpRedirect to build the redirect
    // response.
    //

    hr = strLocation.Copy( szLocation );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    _pW3Context->SetDisconnect( fDisconnect );

    hr = _pW3Context->SetupHttpRedirect(
        strLocation,
        FALSE,   // Don't include the original query string
        httpStatus
        );

    if ( FAILED( hr ) )
    {
        return hr;
    }

    //
    // Now send the response synchronously (the calling ISAPI might
    // do something silly like send more data after this function
    // returns, so we can't do it asynchronously.)
    //

    hr =  _pW3Context->SendResponse(
          W3_FLAG_SYNC |
          W3_FLAG_MORE_DATA
          );

    if ( FAILED( hr ) )
    {
        //
        // If SendResponse fails, consider this a non recoverable
        // error and set the error to be WSAECONNRESET for
        // compatibility with legacy ISAPI code.
        //

        hr = HRESULT_FROM_WIN32( WSAECONNRESET );
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE
ISAPI_REQUEST::GetCertificateInfoEx(
    DWORD           cbAllocated,
    DWORD *         pdwCertEncodingType,
    BYTE *          pbCertEncoded,
    DWORD *         pcbCertEncoded,
    DWORD *         pdwCertificateFlags
    )
/*++

Routine Description:

    Gets certificate info

Arguments:

    cbAllocated         - The size of the pbCertEncoded buffer
    pdwCertEncodingType - Upon return, the cert encoding type
    pbCertEncoded       - Upon return, contains the cert info
    pcbCertEncoded      - Upon successful return, the number of bytes
                          in pbCertEncoded.  On failed return, the number
                          of bytes required to contain pbCertEncoded
    pdwCertificateFlags - Upon return, the certificate flags

Return Value:

    HRESULT

--*/
{
    DBG_ASSERT( CheckSignature() );
    DBG_ASSERT( _pW3Context );

    return _pW3Context->GetCertificateInfoEx( cbAllocated,
                                              pdwCertEncodingType,
                                              pbCertEncoded,
                                              pcbCertEncoded,
                                              pdwCertificateFlags );
}

HRESULT STDMETHODCALLTYPE
ISAPI_REQUEST::AppendLog(
    LPSTR           szExtraParam,
    USHORT          StatusCode
    )
/*++

Routine Description:

    Append the string to the querystring logged

Arguments:

    szExtraParam - the string to be appended

Return Value:

    HRESULT

--*/
{
    HRESULT hr;

    //
    // The only current caller for this function is w3isapi.dll,
    // which validates the parameters.
    //

    DBG_ASSERT( CheckSignature() );
    DBG_ASSERT( _pW3Context );

    if (StatusCode != 0)
    {
        _pW3Context->QueryResponse()->SetStatusCode(StatusCode);
    }

    if (szExtraParam[0] != '\0')
    {
        STRA &strLogParam = _pW3Context->QueryMainContext()
                                ->QueryLogContext()->m_strLogParam;
        if (strLogParam.IsEmpty())
        {
            STACK_STRU (strQueryString, 128);

            if (FAILED(hr = _pW3Context->QueryRequest()->GetQueryStringA(&strLogParam)))
            {
                return hr;
            }
        }

        return strLogParam.Append(szExtraParam);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
ISAPI_REQUEST::ExecuteUrl(
    DWORD64         IsaContext,
    EXEC_URL_INFO * pExecUrlInfo
    )
/*++

Routine Description:

    Execute a child request

Arguments:

    ISaContext   - The ISAPI_CONTEXT for this request (opaque)
    pExecUrlInfo - Description of request to execute

Return Value:

    HRESULT

--*/
{
    W3_ISAPI_HANDLER *      pIsapiHandler = NULL;
    HRESULT                 hr = NO_ERROR;
    BOOL                    fAsync;
    BYTE *                  pbOriginalEntity = NULL;

    //
    // The parameters (i.e. HSE_EXEC_URL_INFO structure) was validated on
    // the W3ISAPI.DLL side, so we can make assumptions about validity
    //

    DBG_ASSERT( CheckSignature() );
    DBG_ASSERT( pExecUrlInfo != NULL );
    DBG_ASSERT( _pW3Context );

    DBG_REQUIRE( ( pIsapiHandler = (W3_ISAPI_HANDLER*)_pW3Context->QueryHandler() ) != NULL );

    fAsync = TRUE;

    DBG_ASSERT( fAsync == !!IsaContext );

    if ( fAsync )
    {
        DBG_ASSERT( _IsapiContext == 0 );
        _IsapiContext = IsaContext;
    }

    //
    // If we're OOP, we need to dup the user token (if there) and the
    // entity body (if there and this is async request)
    //

    if ( _fIsOop )
    {
        //
        // Duplicate the user token if there
        //

        if ( pExecUrlInfo->pUserInfo != NULL &&
             pExecUrlInfo->pUserInfo->hImpersonationToken != NULL )
        {
            DBG_ASSERT( _hExecUrlToken == NULL );

            hr = pIsapiHandler->DuplicateWamProcessHandleForLocalUse(
                            (HANDLE) pExecUrlInfo->pUserInfo->hImpersonationToken,
                            &_hExecUrlToken );
            if ( FAILED( hr ) )
            {
                goto Finished;
            }

            pExecUrlInfo->pUserInfo->hImpersonationToken = reinterpret_cast<DWORD_PTR> (_hExecUrlToken);
        }

        //
        // Duplicate the entity buffer if there and this is async request
        //

        if ( fAsync &&
             pExecUrlInfo->pEntity != NULL &&
             pExecUrlInfo->pEntity->lpbData != NULL &&
             pExecUrlInfo->pEntity->cbAvailable > 0 )
        {
            DBG_ASSERT( _pbExecUrlEntity == NULL );

            _pbExecUrlEntity = LocalAlloc( LMEM_FIXED,
                                           pExecUrlInfo->pEntity->cbAvailable );
            if ( _pbExecUrlEntity == NULL )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );
                goto Finished;
            }

            memcpy( _pbExecUrlEntity,
                    pExecUrlInfo->pEntity->lpbData,
                    pExecUrlInfo->pEntity->cbAvailable );

            //
            // Remember the original pointer to entity body.  We must
            // maintain it on exit so that RPC unmarshalls the correct
            // buffer :-(
            //

            pbOriginalEntity = pExecUrlInfo->pEntity->lpbData;

            pExecUrlInfo->pEntity->lpbData = (LPBYTE) _pbExecUrlEntity;
        }
    }

    //
    // Execute the darn thing
    //

    if ( _fIsOop && fAsync )
    {
        //
        // This reference insures that if the OOP host crashes and
        // COM releases all the OOP-held references, that this
        // object will survive until the I/O completion occurs.
        //

        AddRef();
    }

    hr = _pW3Context->CleanIsapiExecuteUrl( pExecUrlInfo );

    if ( FAILED( hr ) )
    {
        if ( _fIsOop && fAsync )
        {
            //
            // Release the above reference, since no I/O completion will
            // occur.
            //

            Release();
        }
    }

    //
    // If nothing is pending, then we can clean up any dup'd stuff now
    //

Finished:

    if ( FAILED( hr ) &&
         fAsync &&
         _IsapiContext != 0 )
    {
        _IsapiContext = 0;
    }

    if ( FAILED( hr ) || !fAsync )
    {
        if ( _pbExecUrlEntity != NULL )
        {
            LocalFree( _pbExecUrlEntity );
            _pbExecUrlEntity = NULL;
        }

        if ( _hExecUrlToken != NULL )
        {
            CloseHandle( _hExecUrlToken );
            _hExecUrlToken = NULL;
        }
    }

    //
    // Regardless of return status, we need to restore the entity pointer
    // if needed so RPC unmarshalls the right thing
    //

    if ( pbOriginalEntity != NULL )
    {
        DBG_ASSERT( pExecUrlInfo != NULL );
        DBG_ASSERT( pExecUrlInfo->pEntity != NULL );

        pExecUrlInfo->pEntity->lpbData = pbOriginalEntity;
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE
ISAPI_REQUEST::SendCustomError(
    DWORD64         IsaContext,
    CHAR *          pszStatus,
    USHORT          uHttpSubError
)
/*++

Routine Description:

    Send a custom error (if available, otherwise error out with
    ERROR_FILE_NOT_FOUND)

Arguments:

    IsaContext      - The ISAPI_CONTEXT for this request (opaque)
    pszStatus       - Status line
    uHttpSubError   - Sub error

Return Value:

    HRESULT

--*/
{
    HRESULT                 hr;
    BOOL                    fAsync;
    HSE_CUSTOM_ERROR_INFO   customErrorInfo;

    DBG_ASSERT( CheckSignature() );

    //
    // If we have a non-NULL IsaContext, then this is an async request
    //

    fAsync = !!IsaContext;

    if ( fAsync )
    {
        DBG_ASSERT( _IsapiContext == 0 );
        _IsapiContext = IsaContext;
    }

    if ( _fIsOop && fAsync )
    {
        //
        // This reference insures that if the OOP host crashes and
        // COM releases all the OOP-held references, that this
        // object will survive until the I/O completion occurs.
        //

        AddRef();
    }

    customErrorInfo.pszStatus = pszStatus;
    customErrorInfo.uHttpSubError = uHttpSubError;
    customErrorInfo.fAsync = fAsync;

    hr = _pW3Context->CleanIsapiSendCustomError( &customErrorInfo );
    if ( FAILED( hr ) )
    {
        if ( fAsync && _IsapiContext != 0 )
        {
            _IsapiContext = 0;
        }

        if ( _fIsOop && fAsync )
        {
            //
            // Release the above reference, since no I/O completion
            // will ever happen.
            //

            Release();
        }
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE
ISAPI_REQUEST::GetExecuteUrlStatus(
    USHORT *        pChildStatusCode,
    USHORT *        pChildSubErrorCode,
    DWORD *         pChildWin32Error
    )
/*++

Routine Description:

    Get the status of the last child execute

Arguments:

    pChildStatusCode   - Filled with status code of child execute
    pChildSubErrorCode - Filled sub error if applicable
    pChildWin32Error   - Filled last Win32 saved for child request

Return Value:

    HRESULT

--*/
{
    DBG_ASSERT( CheckSignature() );
    DBG_ASSERT( _pW3Context != NULL );

    _pW3Context->QueryChildStatusAndError( pChildStatusCode,
                                           pChildSubErrorCode,
                                           pChildWin32Error );

    return NO_ERROR;
}

HRESULT STDMETHODCALLTYPE
ISAPI_REQUEST::VectorSend(
    DWORD64         IsaContext,
    BOOL            fDisconnect,
    LPSTR           pszStatus,
    LPSTR           pszHeaders,
    VECTOR_ELEMENT *pElements,
    DWORD           nElementCount,
    BOOL            fFinalSend,
    BOOL            fCacheResponse
    )
/*++
  Routine description

    Do a vector send of multiple file handle/memory chunks

  Parameters

    IsaContext      - The ISAPI_CONTEXT for this request (opaque)
    fDisconnect     - Do we disconnect after send
    pszStatus       - The status to be sent if any
    pszHeaders      - The headers to be sent if any
    pElements       - The file handle/memory chunks to be sent
    nElementCount   - The number of these chunks
    fFinalSend      - Should we tell http.sys on behalf of the ISAPI that this
                      is the final send for this response
    fCacheResponse  - Should we ask http.sys to cache this response

  Return value

    HRESULT
--*/
{
    W3_ISAPI_HANDLER *  pW3IsapiHandler;
    HRESULT             hr = S_OK;
    DWORD               cchHeaders = 0;
    W3_RESPONSE *       pResponse;
    BOOL                fAsync;
    BOOL                fFragmentCacheUsed = FALSE;
    WCHAR *             pszFullFragmentName;
    DWORD               i;
    BOOL                fSwitchToParsed = FALSE;

    DBG_ASSERT( CheckSignature() );
    DBG_ASSERT( _pW3Context );
    DBG_REQUIRE( ( pResponse = _pW3Context->QueryResponse() ) != NULL );
    DBG_REQUIRE( ( pW3IsapiHandler = (W3_ISAPI_HANDLER*)_pW3Context->QueryHandler() ) != NULL );

    //
    // A non-NULL context indicates async request
    //

    fAsync = !!IsaContext;

    if ( fAsync )
    {
        DBG_ASSERT( _IsapiContext == 0 );
        _IsapiContext = IsaContext;
    }

    if (pszHeaders != NULL)
    {
        cchHeaders = (DWORD)strlen(pszHeaders);
    }

    if ( _fIsOop )
    {
        //
        // Need to make copies of the file handles and maybe even memory buffer
        //
        if ( fAsync && pszHeaders )
        {
            _pTfHead = (LPBYTE)LocalAlloc(LMEM_FIXED, cchHeaders + 1 );

            if ( _pTfHead == NULL )
            {
                hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
                goto Exit;
            }

            memcpy( _pTfHead, pszHeaders, cchHeaders + 1 );

            pszHeaders = (LPSTR)_pTfHead;
        }

        if (!_bufVectorElements.Resize(nElementCount * sizeof(VECTOR_ELEMENT)))
        {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
            goto Exit;
        }
        ZeroMemory(_bufVectorElements.QueryPtr(),
                   nElementCount * sizeof(VECTOR_ELEMENT));

        VECTOR_ELEMENT *pNewElements = (VECTOR_ELEMENT *)_bufVectorElements.QueryPtr();

        for (i=0; i<nElementCount; i++)
        {
            if (pElements[i].hFile)
            {
                //
                // File handle chunk
                //
                hr = pW3IsapiHandler->DuplicateWamProcessHandleForLocalUse(
                                        (HANDLE)pElements[i].hFile,
                                        (HANDLE *)&pNewElements[i].hFile);
                if (FAILED(hr))
                {
                    goto Exit;
                }
                pNewElements[i].cbOffset = pElements[i].cbOffset;
                pNewElements[i].cbFileSize = pElements[i].cbFileSize;
            }
            else if (pElements[i].pBuffer)
            {
                //
                // Memory buffer chunk
                //
                if (fAsync)
                {
                    //
                    // Need to copy the buffer too
                    //
                    pNewElements[i].pBuffer = (BYTE *)LocalAlloc(LMEM_FIXED, pElements[i].cbBufSize);
                    if (pNewElements[i].pBuffer == NULL)
                    {
                        hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
                        goto Exit;
                    }

                    memcpy(pNewElements[i].pBuffer,
                           pElements[i].pBuffer,
                           pElements[i].cbBufSize);
                }
                else
                {
                    pNewElements[i].pBuffer = pElements[i].pBuffer;
                }

                pNewElements[i].cbBufSize = pElements[i].cbBufSize;
            }

            _nElementCount++;
        }

        pElements = pNewElements;
    }

    //
    // First Clear any goo left from a previous Send
    //
    pResponse->Clear();

    //
    // We must always obey the fDisconnect flag
    // assuming the SSFVectorSend always correctly sets it.
    //

    _pW3Context->SetDisconnect(fDisconnect);

    //
    // Now do the real work
    //
    if (pszStatus != NULL)
    {
        hr = pResponse->BuildResponseFromIsapi( _pW3Context,
                                                pszStatus,
                                                pszHeaders,
                                                cchHeaders );
        if (FAILED(hr))
        {
            goto Exit;
        }

        //
        // Is the status is access denied, then set the sub status to
        // "Denied by Application"
        //

        if ( pResponse->QueryStatusCode() == HttpStatusUnauthorized.statusCode )
        {
            pResponse->SetStatus( HttpStatusUnauthorized,
                                  Http401Application );
        }
    }

    for (i=0; i<nElementCount; i++)
    {
        if (pElements[i].hFile)
        {
            hr = pResponse->AddFileHandleChunk((HANDLE)pElements[i].hFile,
                                               pElements[i].cbOffset,
                                               pElements[i].cbFileSize);
        }
        else if (pElements[i].pBuffer)
        {
            hr = pResponse->AddMemoryChunkByReference(pElements[i].pBuffer,
                                                      pElements[i].cbBufSize);
        }
        else
        {
            if (FAILED(hr = _pW3Context->QueryHeaderBuffer()->AllocateSpace(
                                pElements[i].pszFragmentName,
                                (DWORD)wcslen(pElements[i].pszFragmentName),
                                &pszFullFragmentName)))
            {
                goto Exit;
            }

            hr = pResponse->AddFragmentChunk(pszFullFragmentName,
                                             (USHORT)wcslen(pszFullFragmentName) * sizeof(WCHAR));

            fFragmentCacheUsed = TRUE;
        }

        if (FAILED(hr))
        {
            goto Exit;
        }
    }

    if (fFragmentCacheUsed)
    {
        //
        // Cannot used fragment-cache if using either compression or send-raw data filters
        //
        W3_METADATA *pMetaData = _pW3Context->QueryUrlContext()->QueryMetaData();
        if ( !_pW3Context->QueryDoneWithCompression() &&
             pMetaData->QueryDoDynamicCompression() )
        {
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
            goto Exit;
        }

        if ( _pW3Context->IsNotificationNeeded( SF_NOTIFY_SEND_RAW_DATA ) )
        {
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
            goto Exit;
        }

        //
        // We also cannot generate content-length in this case
        //
        _pW3Context->QueryMainContext()->SetShouldGenerateContentLength( FALSE );
    }

    if ( _fIsOop && fAsync )
    {
        //
        // This reference insures that if the OOP host crashes and
        // COM releases all the OOP-held references, that this
        // object will survive until the I/O completion occurs.
        //

        AddRef();
    }

    if (pszStatus != NULL)
    {
        //
        // If the response is to be cached, then we'll need to switch to
        // parsed mode. 
        //
        
        if (fCacheResponse)
        {
            _fCacheResponse = TRUE;
            fSwitchToParsed = TRUE;
        }
        
        //
        // If the ISAPI is doing a keep-alive single-send, is 
        // referencing a fragment (1+), and did not specify a 
        // content-length header themselves, then switch to
        // parsed so that HTTP.SYS can do the work for us
        //
        
        if (fFragmentCacheUsed &&
            !fSwitchToParsed &&
            fFinalSend &&
            !fDisconnect &&
            pszHeaders != NULL &&
            strstr( pszHeaders, "Content-Length: " ) == NULL &&
            strstr( pszHeaders, "Transfer-Encoding: chunked" ) == NULL)
        {
            fSwitchToParsed = TRUE;   
        }
        
        //
        // Do the switch
        //
          
        if (fSwitchToParsed)
        {            
            hr = _pW3Context->QueryResponse()->SwitchToParsedMode();
            if (FAILED(hr))
            {
                Release();
                goto Exit;   
            }
        }

        hr = _pW3Context->SendResponse( (fAsync ? W3_FLAG_ASYNC : W3_FLAG_SYNC)
                                        | (fFinalSend ? 0 : W3_FLAG_MORE_DATA)
                                        | W3_FLAG_NO_ERROR_BODY
                                        | W3_FLAG_NO_CONTENT_LENGTH );
    }
    else
    {
        hr = _pW3Context->SendEntity( (fAsync ? W3_FLAG_ASYNC : W3_FLAG_SYNC)
                                     | (fFinalSend ? 0 : W3_FLAG_MORE_DATA) );
    }

    if (FAILED(hr))
    {
        //
        // If SendEntity or SendResponse fail, consider this
        // a non recoverable error and set the error to be
        // WSAECONNRESET for compatibility with legacy ISAPI code.
        //

        hr = HRESULT_FROM_WIN32( WSAECONNRESET );

        if ( _fIsOop && fAsync )
        {
            //
            // Release the above reference, since no I/O completion will
            // occur.
            //

            Release();
        }
    }

 Exit:

    if ( FAILED( hr ) && fAsync && _IsapiContext != 0 )
    {
        _IsapiContext = 0;
    }

    if ( _fIsOop &&
        (FAILED(hr) || !fAsync) )
    {
        //
        // Need to destroy handles/memory buffers we copied
        //
        if ( _pTfHead )
        {
            LocalFree( _pTfHead );
            _pTfHead = NULL;
        }

        for (i=0; i<_nElementCount; i++)
        {
            if (pElements[i].hFile)
            {
                CloseHandle((HANDLE)pElements[i].hFile);
            }
            else if (pElements[i].pBuffer && fAsync)
            {
                LocalFree(pElements[i].pBuffer);
            }
        }

        _nElementCount = 0;
    }

    return hr;
}

HRESULT
ISAPI_REQUEST::GetCustomError(
        DWORD dwError,
        DWORD dwSubError,
        DWORD dwBufferSize,
        BYTE  *pvBuffer,
        DWORD *pdwRequiredBufferSize,
        BOOL  *pfIsFileError,
        BOOL  *pfSendErrorBody)

/*++

Routine Description:

    Finds the CustomError for this error and subError.  The results are returned
    in pvBuffer provided there is enough buffer space.  The amount of buffer space
    required is returned in pdwRequestBufferSize regardless.

Arguments:


    dwError                 - major error (e.g. 500)
    dwSubError              - sub error (e.g. 13)
    dwBufferSize            - size, in bytes, of buffer at pvBuffer
    pvBuffer                - pointer to buffer for result
    pdwRequiredBufferSize   - amount of buffer used/need
    pfIsFileError           - return boolean if custom error is a filename
    pfSendErrorBody         - upon return, TRUE if error body should be sent

Return Value:

    HRESULT

--*/
{
    HRESULT         hr = NOERROR;
    W3_METADATA     *pMetadata = NULL;
    LPSTR           pMimeStr = "text/html";

    STACK_STRA(mimeStr, 64);

    STACK_STRU(             strError, 64 );

    DBG_ASSERT( CheckSignature() );

    // first dig out the W3 Metadata pointer

    pMetadata = _pW3Context->QueryUrlContext()->QueryMetaData();
    DBG_ASSERT( pMetadata != NULL );

    // get the custom error for this error code

    hr = pMetadata->FindCustomError( (USHORT)dwError,
                                     (USHORT)dwSubError,
                                     pfIsFileError,
                                     &strError );

    // if successful, and the custom error is a file, we need to
    // get the file's mime type

    if (SUCCEEDED(hr) && *pfIsFileError) {

        // lookup the MIME_ENTRY for this file.

        if (SUCCEEDED(SelectMimeMappingForFileExt(strError.QueryStr(),pMetadata->QueryMimeMap(), &mimeStr))) {
            pMimeStr = mimeStr.QueryStr();
        }
    }

    // if found, convert the UNICODE string to ANSI

    if (SUCCEEDED(hr)) {

        int ret;

        ret = WideCharToMultiByte(CP_ACP,
                                  0,
                                  strError.QueryStr(),
                                  -1,
                                  (LPSTR)pvBuffer,
                                  dwBufferSize,
                                  NULL,
                                  NULL);

        *pdwRequiredBufferSize = ret;

        // check return.  If zero, then the conversion failed.
        // GetLastError() contains the error.

        if (ret == 0) {

            DWORD   winError = GetLastError();

            // if InsufBuff, then call again to get the required size

            if (winError == ERROR_INSUFFICIENT_BUFFER) {

                *pdwRequiredBufferSize = WideCharToMultiByte(CP_ACP,
                                                             0,
                                                             strError.QueryStr(),
                                                             -1,
                                                             NULL,
                                                             0,
                                                             NULL,
                                                             NULL);

                // if the error is a filename, then include in the required
                // buffer size the length of the mime string

                if (*pfIsFileError) {

                    *pdwRequiredBufferSize += (DWORD)strlen(pMimeStr) + 1;
                }
            }
            // in any case, make a HRESULT from the win32 error and return that

            hr = HRESULT_FROM_WIN32(winError);
        }

        // if we continue to be successful, the next step is to put
        // the mime string after the null byte of the file name

        if (SUCCEEDED(hr) && *pfIsFileError) {

            int fileLen = (int)strlen((char *)pvBuffer);

            // make sure we have enough buffer

            if ((fileLen + strlen(pMimeStr) + 2) > dwBufferSize) {

                hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            }
            else {

                // looks like we do.  Copy the mime string in

                strcpy(&((char *)pvBuffer)[fileLen+1],pMimeStr);

                pdwRequiredBufferSize += strlen(pMimeStr) + 1;
            }
        }
    }

    if ( pfSendErrorBody != NULL )
    {
        *pfSendErrorBody = _pW3Context->QuerySendErrorBody();
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE
ISAPI_REQUEST::TestConnection(
    BOOL    *pfIsConnected
    )
/*++

Routine Description:

    returns state of the connection (TRUE = opened, FALSE = closed)

Arguments:

    pfIsConnected - sets to TRUE if connection is still open,
                    FALSE if it was closed already

Return Value:

    HRESULT

--*/
{
    HRESULT             hr = NOERROR;
    W3_CONNECTION *     pConnection = NULL;
    W3_MAIN_CONTEXT *   pMainContext = NULL;

    DBG_ASSERT( CheckSignature() );

    DBG_ASSERT( _pW3Context != NULL );
    pMainContext = _pW3Context->QueryMainContext();

    DBG_ASSERT( pMainContext != NULL );
    pConnection = pMainContext->QueryConnection( TRUE );

    if ( pConnection == NULL )
    {
        //
        // Issue 02/08/2001 jaroslad:
        // QueryConnection currently doesn't have a way to return
        // error that occured. For now assume that out of memory
        // occured
        //

        hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);

        //
        // we will not touch pfIsConnected in the case of error
        // it is caller's responsibility to check if this call
        // succeeded before using pfIsConnected
        //
    }
    else
    {
        *pfIsConnected =  pConnection->QueryConnected();
    }
    return hr;
}

HRESULT STDMETHODCALLTYPE
ISAPI_REQUEST::GetSspiInfo(
    BYTE *pCredHandle,
    DWORD cbCredHandle,
    BYTE *pCtxtHandle,
    DWORD cbCtxtHandle)
/*++

Routine Description:

    Returns SSPI info about the request

Arguments:

    pCredHandle  - Upon return, contains the credential handle
    cbCredHandle - The size of pCredHandle
    pCtxtHandle  - Upon return, contains the context handle
    cbCtxtHandle - The size of pCtxtHandle

Return Value:

    HRESULT

--*/
{
    W3_USER_CONTEXT *       pUserContext;

    DBG_ASSERT( CheckSignature() );
    DBG_ASSERT( _pW3Context != NULL );

    if ( _fIsOop )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
    }

    pUserContext = _pW3Context->QueryUserContext();
    DBG_ASSERT( pUserContext != NULL );

    return pUserContext->GetSspiInfo( pCredHandle,
                                      cbCredHandle,
                                      pCtxtHandle,
                                      cbCtxtHandle );
}

HRESULT STDMETHODCALLTYPE
ISAPI_REQUEST::QueryToken(
    BYTE *szUrl,
    DWORD,
    DWORD dwTokenType,
    DWORD64 *pToken,
    BOOL fUnicode)
/*++

Routine Description:

    Returns the VR token for the request

Arguments:

    szUrl       - The URL for which we need to get the token
    cbUrl       - The size of szUrl
    dwTokenType - Either TOKEN_VR_TOKEN or TOKEN_ANONYMOUS_TOKEN
    pToken      - Upon return, points to the resulting token
    fUnicode    - If TRUE, szUrl is UNICODE, else it's ANSI

Return Value:

    HRESULT

--*/
{
    STACK_STRU(         struUrl,MAX_PATH );
    W3_URL_INFO *       pUrlInfo = NULL;
    W3_METADATA *       pMetaData = NULL;
    TOKEN_CACHE_ENTRY * pTokenEntry = NULL;
    HANDLE              hToken = NULL;
    HANDLE              hTokenLocalDuplicate = NULL;
    HRESULT             hr;
    BOOL                fSuccess;

    DBG_ASSERT( _pW3Context );
    DBG_ASSERT( szUrl );
    DBG_ASSERT( pToken );
    DBG_ASSERT( dwTokenType == TOKEN_VR_TOKEN ||
                dwTokenType == TOKEN_ANONYMOUS_TOKEN );

    //
    // Get the metadata for the specified URL
    //

    if ( fUnicode )
    {
        hr = struUrl.Copy( (LPWSTR)szUrl );
    }
    else
    {
        hr = struUrl.CopyA( (LPSTR)szUrl );
    }

    if ( FAILED( hr ) )
    {
        return hr;
    }

    DBG_ASSERT( g_pW3Server->QueryUrlInfoCache() != NULL );

    hr = g_pW3Server->QueryUrlInfoCache()->GetUrlInfo(
                                        _pW3Context,
                                        struUrl,
                                        &pUrlInfo );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    DBG_ASSERT( pUrlInfo != NULL );

    pMetaData = pUrlInfo->QueryMetaData();
    DBG_ASSERT( pMetaData != NULL );

    //
    // Assume that the VR token is NULL, unless we can determine otherwise
    //

    *pToken = NULL;

    //
    // Ok, so now let's get the token
    //

    if ( dwTokenType == TOKEN_VR_TOKEN )
    {
        hr = pMetaData->GetAndRefVrAccessToken( &pTokenEntry );
        if( FAILED( hr ) )
        {
           return hr;
        }
    }
    else if ( dwTokenType == TOKEN_ANONYMOUS_TOKEN )
    {
        hr = pMetaData->GetAndRefAnonymousToken( &pTokenEntry );
        if( FAILED( hr ) )
        {
           return hr;
        }
    }
    else
    {
        DBG_ASSERT( FALSE );
    }

    if ( pTokenEntry != NULL )
    {
        hToken = pTokenEntry->QueryImpersonationToken();
    }

    if ( hToken )
    {
        //
        // Need to duplicate the handle.  This is really only necessary for
        // OOP requests, but we need to do it for both, so that the extension
        // doesn't need to worry about knowing if it's inproc or not before
        // deciding if it needs to close the handle.
        //

        fSuccess = DupTokenWithSameImpersonationLevel(
            hToken,
            MAXIMUM_ALLOWED,
            TokenPrimary,
            &hTokenLocalDuplicate
            );

        if( fSuccess )
        {
            if( _fIsOop )
            {
                HANDLE  hTokenRemote = NULL;

                fSuccess = DuplicateHandle(
                                GetCurrentProcess(),
                                hTokenLocalDuplicate,
                                _pWamProcess->QueryProcess(),
                                &hTokenRemote,
                                0,
                                FALSE,
                                DUPLICATE_SAME_ACCESS
                                );

                CloseHandle(hTokenLocalDuplicate);
                hTokenLocalDuplicate = NULL;

                *pToken = (DWORD64)hTokenRemote;
            }
            else
            {
                *pToken = reinterpret_cast<DWORD64>(hTokenLocalDuplicate);
            }
        }
    }

    pUrlInfo->DereferenceCacheEntry();

    if ( pTokenEntry != NULL )
    {
        pTokenEntry->DereferenceCacheEntry();
        pTokenEntry = NULL;
    }
    
    return hr;
}

HRESULT STDMETHODCALLTYPE
ISAPI_REQUEST::ReportAsUnhealthy(
    BYTE *szImage,
    DWORD,
    BYTE *szReason,
    DWORD cbReason)
/*++

Routine Description:

    Handles an ISAPI reporting itself as unhealthy

Arguments:

    szImage  - A UNICODE string with the ISAPI image name
    cbImage  - The size of szImage
    szReason - A UNICODE string from the ISAPI indicating the problem
    cbReason - The size of szReason

Return Value:

    HRESULT

--*/
{
    W3_ISAPI_HANDLER *  pW3IsapiHandler;
    const WCHAR  *      pszEventLog[2];

    DBG_ASSERT( _pW3Context );
    DBG_ASSERT( szImage );
    DBG_ASSERT( cbReason == 0 || szReason != NULL );
    DBG_REQUIRE( ( pW3IsapiHandler = (W3_ISAPI_HANDLER*)_pW3Context->QueryHandler() ) != NULL );


    //
    // First, log the event.  After we set the unhealthy flag,
    // this process could get terminated at any time.
    //
    // There are two possible messages to log, depending on
    // whether the extension gave us a reason for being
    // unhealthy.
    //

    pszEventLog[0] = reinterpret_cast<WCHAR*>( szImage );

    if ( cbReason > 0 )
    {
        pszEventLog[1] = reinterpret_cast<WCHAR*>( szReason );

        g_pW3Server->LogEvent(
            W3_EVENT_UNHEALTHY_ISAPI,
            2,
            pszEventLog,
            0
            );
    }
    else
    {
        g_pW3Server->LogEvent(
            W3_EVENT_UNHEALTHY_ISAPI_NO_REASON,
            1,
            pszEventLog,
            0
            );
    }

    //
    // Now tell someone that we feel sick
    //

    UlAtqSetUnhealthy();

    return NO_ERROR;
}


HRESULT STDMETHODCALLTYPE
ISAPI_REQUEST::AddFragmentToCache(
    VECTOR_ELEMENT * pVectorElement,
    WCHAR          * pszFragmentName
)
/*++

Routine Description:

    Add the fragment to cache

Arguments:

    pVectorElement - The fragment to be added
    pszFragmentName - name of the fragment

Return Value:

    HRESULT

--*/
{
    HTTP_DATA_CHUNK DataChunk;

    ZeroMemory(&DataChunk, sizeof DataChunk);
    if (pVectorElement->hFile)
    {
        DataChunk.DataChunkType = HttpDataChunkFromFileHandle;
        DataChunk.FromFileHandle.FileHandle = (HANDLE)pVectorElement->hFile;
        DataChunk.FromFileHandle.ByteRange.StartingOffset.QuadPart = pVectorElement->cbOffset;
        DataChunk.FromFileHandle.ByteRange.Length.QuadPart = pVectorElement->cbFileSize;
    }
    else if (pVectorElement->pBuffer)
    {
        DataChunk.DataChunkType = HttpDataChunkFromMemory;
        DataChunk.FromMemory.pBuffer = pVectorElement->pBuffer;
        DataChunk.FromMemory.BufferLength = pVectorElement->cbBufSize;
    }

    return UlAtqAddFragmentToCache(&DataChunk,
                                   pszFragmentName);
}


HRESULT STDMETHODCALLTYPE
ISAPI_REQUEST::ReadFragmentFromCache(
    WCHAR          * pszFragmentName,
    BYTE           * pvBuffer,
    DWORD            cbSize,
    DWORD          * pcbCopied
)
/*++

Routine Description:

    Read the fragment from cache

Arguments:

    pszFragmentName - name of the fragment
    pvBuffer - the buffer to read in
    cbSize - the size of the buffer
    pcbCopied - the amount copied in on return

Return Value:

    HRESULT

--*/
{
    return UlAtqReadFragmentFromCache(pszFragmentName,
                                      pvBuffer,
                                      cbSize,
                                      pcbCopied);
}


HRESULT STDMETHODCALLTYPE
ISAPI_REQUEST::RemoveFragmentFromCache(
    WCHAR          * pszFragmentName
)
/*++

Routine Description:

    Remove the fragment from cache

Arguments:

    pszFragmentName - name of the fragment

Return Value:

    HRESULT

--*/
{
    return UlAtqRemoveFragmentFromCache(pszFragmentName);
}


HRESULT
ISAPI_REQUEST::PreprocessIoCompletion(
    DWORD   cbIo
    )
/*++

Routine Description:

    Handles cleanup for any functions that use fCopiedData=TRUE.

    In the case of a TransmitFile or WriteClient, this just
    involves closing handles and freeing buffers.  In the case
    of ReadClient, we need to push the read buffer to the OOP
    process.

Arguments:

    cbIo - The number of bytes in a read buffer, if present

Return Value:

    HRESULT

--*/
{
    W3_ISAPI_HANDLER *  pW3IsapiHandler;
    HRESULT             hr = NOERROR;
    DWORD               i;

    DBG_ASSERT( CheckSignature() );
    DBG_ASSERT( _pW3Context );
    DBG_REQUIRE( ( pW3IsapiHandler = (W3_ISAPI_HANDLER*)_pW3Context->QueryHandler() ) != NULL );

    //
    // Cleanup any existing TF info, since we're done with it.
    //

    if ( _hTfFile != NULL && _hTfFile != INVALID_HANDLE_VALUE )
    {
        CloseHandle( _hTfFile );
        _hTfFile = INVALID_HANDLE_VALUE;
    }

    if ( _pTfHead )
    {
        LocalFree( _pTfHead );
        _pTfHead = NULL;
    }

    if ( _pTfTail )
    {
        LocalFree( _pTfTail );
        _pTfTail = NULL;
    }

    //
    // Cleanup any existing async write buffer
    //

    if ( _pAsyncWriteBuffer )
    {
        LocalFree( _pAsyncWriteBuffer );
        _pAsyncWriteBuffer = NULL;
    }

    //
    // If we have an async read buffer, then push the data
    // to the WAM process and free it
    //

    if ( _pAsyncReadBuffer )
    {
        hr = pW3IsapiHandler->MarshalAsyncReadBuffer(
            _IsapiContext,
            _pAsyncReadBuffer,
            cbIo
            );

        //
        // Note that the above function could fail if, for
        // example, the dllhost has crashed.  There's not
        // anything we can do about it here, though.  We'll
        // ignore it.
        //

        LocalFree( _pAsyncReadBuffer );
        _pAsyncReadBuffer = NULL;
    }

    //
    // Clean up HSE_EXEC_URL stuff
    //

    if ( _pbExecUrlEntity != NULL )
    {
        LocalFree( _pbExecUrlEntity );
        _pbExecUrlEntity = NULL;
    }

    if ( _hExecUrlToken != NULL )
    {
        CloseHandle( _hExecUrlToken );
        _hExecUrlToken = NULL;
    }

    //
    // Need to destroy handles/memory buffers we copied for vector send
    //
    VECTOR_ELEMENT *pElements = (VECTOR_ELEMENT *)_bufVectorElements.QueryPtr();
    for (i=0; i<_nElementCount; i++)
    {
        if (pElements[i].hFile)
        {
            CloseHandle((HANDLE)pElements[i].hFile);
        }
        else if (pElements[i].pBuffer)
        {
            LocalFree(pElements[i].pBuffer);
        }
    }

    _nElementCount = 0;

    return hr;
}

HRESULT STDMETHODCALLTYPE
ISAPI_REQUEST::GetMetadataProperty( 
    DWORD               dwPropertyId,
    BYTE *              pbBuffer,
    DWORD               cbBuffer,
    DWORD *             pcbBufferRequired
    )
/*++

Routine Description:

    Retrieves a metabase property

Arguments:

    dwPropertyId - MD_ property ID (must be of type UT_FILE)
    pbBuffer - Buffer to receive serialized data
    cbBuffer - Size of buffer
    pcbBufferRequired - Filled with size of buffer required
    
Return Value:

    HRESULT

--*/
{
    URL_CONTEXT *           pUrlContext;
    W3_METADATA *           pMetadata;
    
    DBG_ASSERT( _pW3Context );
    
    pUrlContext = _pW3Context->QueryUrlContext();
    DBG_ASSERT( pUrlContext != NULL );
    
    pMetadata = pUrlContext->QueryMetaData();
    DBG_ASSERT( pMetadata != NULL );

    return pMetadata->GetMetadataProperty( _pW3Context,
                                           dwPropertyId,
                                           pbBuffer,
                                           cbBuffer,
                                           pcbBufferRequired );
}


HRESULT STDMETHODCALLTYPE
ISAPI_REQUEST::GetCacheInvalidationCallback(
    DWORD64         *pfnCallback)
{
    //
    // BUGBUG: CODEWORK: will need another mechanism if we want to implement
    // this for OOP
    //
    if (_fIsOop)
    {
        return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
    }

    *(PFN_HSE_CACHE_INVALIDATION_CALLBACK *)pfnCallback = UlAtqFlushUlCache;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE
ISAPI_REQUEST::CloseConnection(
    VOID
    )
{
    W3_RESPONSE *   pResponse;
    DWORD           cbSent = NULL;
    HRESULT         hr;

    DBG_ASSERT( CheckSignature() );
    DBG_ASSERT( _pW3Context );
    DBG_REQUIRE( ( pResponse = _pW3Context->QueryResponse() ) != NULL );

    //
    // Tell http.sys to close the connection.  This send should not
    // cause http.sys to clean up the request - and logging data
    // sent later (when the ISAPI is done) should be correctly
    // handled.
    //

    hr = SendEntityBodyAndLogDataHelper( _pW3Context,
                                         _pW3Context->QueryUlatqContext(),
                                         FALSE,
                                         HTTP_SEND_RESPONSE_FLAG_DISCONNECT,
                                         0,
                                         NULL,
                                         &cbSent );

    if ( FAILED( hr ) )
    {
        //
        // If SendEntity fails, consider this a non recoverable
        // error and set the error to be WSAECONNRESET for
        // compatibility with legacy ISAPI code.
        //

        hr = HRESULT_FROM_WIN32( WSAECONNRESET );
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE
ISAPI_REQUEST::AllocateMemory(
    DWORD            cbSize,
    DWORD64        * ppvBuffer
    )
/*++

Routine Description:

    Allocate some per-request memory

Arguments:

    cbSize - Size to allocate
    ppvBuffer - Filled with buffer
    
Return Value:

    HRESULT

--*/
{
    DWORD64         dwBuffer;
    VOID *          pvBuffer;
    
    DBG_ASSERT( CheckSignature() );
    DBG_ASSERT( _pW3Context );

    pvBuffer = _pW3Context->ContextAlloc( cbSize );    
    if ( pvBuffer == NULL )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    *ppvBuffer = (DWORD64) pvBuffer;
    return S_OK;
}

ISAPI_REQUEST::~ISAPI_REQUEST()
/*++

Routine Description:

    Destructor

Arguments:

    None

Return Value:

    None

--*/
{
    DBG_ASSERT( CheckSignature() );

    //
    // Release the free threaded marshaler
    //

    if ( _pUnkFTM )
    {
        _pUnkFTM->Release();
        _pUnkFTM = NULL;
    }

    //
    // Dissociate ourselves from the WAM_PROCESS, if present
    //

    if ( _pWamProcess )
    {
        if (_pWamProcess->QueryCrashed())
        {
            AppendLog(SZ_FAILED_OOP_REQUEST_LOG_MESSAGE,
                      500);
        }

        _pWamProcess->DecrementRequestCount();

        _pWamProcess->Release();
        _pWamProcess = NULL;
    }

    _dwSignature = ISAPI_REQUEST_SIGNATURE_FREE;

    IF_DEBUG( ISAPI )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "ISAPI_REQUEST %p has been destroyed.\r\n",
            this
            ));
    }
}

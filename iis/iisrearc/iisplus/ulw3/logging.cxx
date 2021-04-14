/*++

   Copyright    (c)   2000    Microsoft Corporation

   Module Name :
     logging.cxx

   Abstract:
     Handle Logging

   Author:
     Anil Ruia (AnilR)              1-Jul-2000

   Environment:
     Win32 - User Mode

   Project:
     w3core.dll
--*/

#include "precomp.hxx"

CONTEXT_STATUS W3_STATE_LOG::DoWork(
    W3_MAIN_CONTEXT *pMainContext,
    DWORD,
    DWORD)
/*++

Routine Description:

    Log the request (and call the END_OF_REQUEST notification)

Arguments:

    pMainContext - W3_MAIN_CONTEXT representing an execution of the
                   state machine

Return Value:

    CONTEXT_STATUS_CONTINUE - if we should continue in state machine
    else stop executing the machine and free up the current thread

--*/
{
    BOOL            fUnused = FALSE;

    if (pMainContext->QueryNeedFinalDone())
    {
        //
        // Clear remaining chunks from handle/response state
        //

        pMainContext->QueryResponse()->Clear();

        //
        // End of request notifications happen BEFORE logging.
        //

        if ( pMainContext->IsNotificationNeeded( SF_NOTIFY_END_OF_REQUEST ) )
        {
            pMainContext->NotifyFilters( SF_NOTIFY_END_OF_REQUEST,
                                         NULL,
                                         &fUnused );
        }
        
        //
        // Clear any crud from the last write from END_OF_REQUEST filter
        //

        pMainContext->QueryResponse()->Clear();

        //
        // Do the final send
        //

        pMainContext->SendEntity(W3_FLAG_PAST_END_OF_REQ);
    }

    if (pMainContext->QueryDoCustomLogging())
    {
        if (FAILED(pMainContext->CollectLoggingData(FALSE)))
        {
            goto Exit;
        }

        pMainContext->QuerySite()->LogInformation(pMainContext->QueryLogContext());
    }

 Exit:
    return CONTEXT_STATUS_CONTINUE;
}


CONTEXT_STATUS W3_STATE_LOG::OnCompletion(
    W3_MAIN_CONTEXT *,
    DWORD,
    DWORD)
/*++

Routine Description:

    Completion for the W3_STATE_LOG state

Arguments:

    pMainContext - W3_MAIN_CONTEXT representing an execution of the
                   state machine

Return Value:

    CONTEXT_STATUS_CONTINUE - if we should continue in state machine
    else stop executing the machine and free up the current thread

--*/
{
    //
    // This code should never get hit
    //
    DBG_ASSERT(FALSE);

    return CONTEXT_STATUS_CONTINUE;
}


HRESULT W3_MAIN_CONTEXT::CollectLoggingData(BOOL fCollectForULLogging)
/*++

Routine Description:

    Routine which collects all the logging data

Arguments:

    fCollectForULLogging - whether the data is being collected for http.sys/custom logging

Return Value:

    HRESULT

--*/
{
    STACK_STRA(strVal, 64);
    STACK_STRU(strValW, 64);
    HRESULT hr = S_OK;
    DWORD dwLength;
    CHUNK_BUFFER * pHeaderBuffer;

    HTTP_LOG_FIELDS_DATA *pUlLogData = QueryUlLogData();
    W3_REQUEST *pRequest = QueryRequest();
    W3_RESPONSE *pResponse = QueryResponse();

    pHeaderBuffer = QueryHeaderBuffer();
    DBG_ASSERT( pHeaderBuffer != NULL );

    if (IsNotificationNeeded(SF_NOTIFY_LOG))
    {
        //
        // There is a SF_NOTIFY_LOG filter, collect the data for the
        // notification
        //
        HTTP_FILTER_LOG filtLog;
        ZeroMemory(&filtLog, sizeof filtLog);

        if (SUCCEEDED(hr = GetServerVariableRemoteHost(this, &strVal)))
        {
            dwLength = strVal.QueryCCH() + 1;
            if (FAILED(hr = pHeaderBuffer->AllocateSpace(
                                dwLength,
                                (PCHAR *)&filtLog.pszClientHostName)))
            {
                goto Exit;
            }
            strVal.CopyToBuffer((LPSTR)filtLog.pszClientHostName,
                                 &dwLength);
        }
        else
        {
            goto Exit;
        }

        if (SUCCEEDED(hr = GetServerVariableLogonUser(this, &strVal)))
        {
            dwLength = strVal.QueryCCH() + 1;
            if (FAILED(hr = pHeaderBuffer->AllocateSpace(
                                dwLength,
                                (PCHAR *)&filtLog.pszClientUserName)))
            {
                goto Exit;
            }
            strVal.CopyToBuffer((LPSTR)filtLog.pszClientUserName,
                                 &dwLength);
        }
        else
        {
            goto Exit;
        }

        if (SUCCEEDED(hr = GetServerVariableLocalAddr(this, &strVal)))
        {
            dwLength = strVal.QueryCCH() + 1;
            if (FAILED(hr = pHeaderBuffer->AllocateSpace(
                                dwLength,
                                (PCHAR *)&filtLog.pszServerName)))
            {
                goto Exit;
            }
            strVal.CopyToBuffer((LPSTR)filtLog.pszServerName,
                                 &dwLength);
        }
        else
        {
            goto Exit;
        }

        pRequest->QueryVerb((CHAR **)&filtLog.pszOperation, (USHORT *)&dwLength);

        if (FAILED(hr = pRequest->GetUrl(&strValW)) ||
            FAILED(hr = strVal.CopyWToUTF8(strValW)) ||
            FAILED(hr = pHeaderBuffer->AllocateSpace(
                                strVal.QueryStr(),
                                strVal.QueryCCH(),
                                (CHAR **)&filtLog.pszTarget)))
        {
            goto Exit;
        }

        //
        // If an ISAPI used HSE_APPEND_LOG_PARAMETER use it
        //
        if (_LogContext.m_strLogParam.IsEmpty())
        {
            if (FAILED(hr = pRequest->GetQueryStringA(&_LogContext.m_strLogParam)))
            {
                goto Exit;
            }
        }
        filtLog.pszParameters = _LogContext.m_strLogParam.QueryStr();

        filtLog.dwHttpStatus = pResponse->QueryStatusCode();
        
        if ( HRESULT_FACILITY( QueryErrorStatus() ) == FACILITY_WIN32 )
        {
            filtLog.dwWin32Status = WIN32_FROM_HRESULT( QueryErrorStatus() );
        }
        else
        {
            filtLog.dwWin32Status = QueryErrorStatus();
        }

        filtLog.dwBytesSent = _LogContext.m_dwBytesSent;
        filtLog.dwBytesRecvd = _LogContext.m_dwBytesRecvd;
        filtLog.msTimeForProcessing = GetTickCount() - _LogContext.m_msStartTickCount;

        NotifyFilters(SF_NOTIFY_LOG, &filtLog, NULL);

        //
        // The filter may have changed some the data, copy it back to our
        // logging structure
        //
        pUlLogData->ClientIp = (CHAR *)filtLog.pszClientHostName;
        pUlLogData->ClientIpLength = (USHORT) strlen(filtLog.pszClientHostName);

        if (FAILED(hr = strValW.CopyA(filtLog.pszClientUserName)) ||
            FAILED(hr = pHeaderBuffer->AllocateSpace(
                            strValW.QueryStr(),
                            strValW.QueryCCH(),
                            (PWSTR*) &pUlLogData->UserName)))
        {
            goto Exit;
        }
        pUlLogData->UserNameLength =
            (USHORT)(strValW.QueryCCH() * sizeof(WCHAR));

        pUlLogData->ServerIp = (CHAR *)filtLog.pszServerName;
        pUlLogData->ServerIpLength = (USHORT) strlen(filtLog.pszServerName);

        pUlLogData->Method = (CHAR *)filtLog.pszOperation;
        pUlLogData->MethodLength = (USHORT) strlen(filtLog.pszOperation);
        pUlLogData->MethodNum = (HTTP_VERB)METHOD_HASH::GetIndex((CHAR *)filtLog.pszOperation);        

        if (FAILED(hr = strValW.CopyA(filtLog.pszTarget)) ||
            FAILED(hr = pHeaderBuffer->AllocateSpace(
                            strValW.QueryStr(),
                            strValW.QueryCCH(),
                            (PWSTR*) &pUlLogData->UriStem)))
        {
            goto Exit;
        }
        pUlLogData->UriStemLength =
            (USHORT)(strValW.QueryCCH() * sizeof(WCHAR));

        pUlLogData->UriQuery = (CHAR *)filtLog.pszParameters;
        pUlLogData->UriQueryLength = (USHORT) strlen(filtLog.pszParameters);

        pUlLogData->ProtocolStatus = (USHORT) filtLog.dwHttpStatus;
        pUlLogData->Win32Status = filtLog.dwWin32Status;
        _LogContext.m_dwBytesSent = filtLog.dwBytesSent;
        _LogContext.m_dwBytesRecvd = filtLog.dwBytesRecvd;
        _LogContext.m_msProcessingTime = filtLog.msTimeForProcessing;
    }
    else
    {
        //
        // No filter, just get the logging data
        //
        if (SUCCEEDED(hr = GetServerVariableRemoteHost(this, &strVal)))
        {
            if (FAILED(hr = pHeaderBuffer->AllocateSpace(
                                strVal.QueryStr(),
                                strVal.QueryCCH(),
                                (PSTR*) &pUlLogData->ClientIp)))
            {
                goto Exit;
            }
            pUlLogData->ClientIpLength = (USHORT)strVal.QueryCCH();
        }
        else
        {
            goto Exit;
        }

        if ( QueryUserContext() == NULL )
        {
            if (FAILED(hr = pRequest->GetRequestUserName(&strVal)))
            {
                goto Exit;
            }

            if (strVal.IsEmpty())
            {
                pUlLogData->UserName = L"";
                pUlLogData->UserNameLength = 0;
            }
            else
            {
                if (FAILED(hr = strValW.CopyA(strVal.QueryStr())) ||
                    FAILED(hr = pHeaderBuffer->AllocateSpace(
                                    strValW.QueryStr(),
                                    strValW.QueryCCH(),
                                    (PWSTR*) &pUlLogData->UserName)))
                {
                    goto Exit;
                }
                pUlLogData->UserNameLength = (USHORT)strValW.QueryCB();
            }
        }
        else
        {
            pUlLogData->UserName = QueryUserContext()->QueryUserName();
            if (pUlLogData->UserName)
            {
                pUlLogData->UserNameLength = (USHORT)wcslen(pUlLogData->UserName) * sizeof(WCHAR);
            }
        }

        if (SUCCEEDED(hr = GetServerVariableLocalAddr(this, &strVal)))
        {
            if (FAILED(hr = pHeaderBuffer->AllocateSpace(
                                strVal.QueryStr(),
                                strVal.QueryCCH(),
                                (PSTR*) &pUlLogData->ServerIp)))
            {
                goto Exit;
            }
            pUlLogData->ServerIpLength =
                (USHORT)strVal.QueryCCH();
        }
        else
        {
            goto Exit;
        }

        pRequest->QueryVerb((CHAR**) &pUlLogData->Method,
                            &pUlLogData->MethodLength);
        pUlLogData->MethodNum = pRequest->QueryVerbType();

        pRequest->QueryUrl((WCHAR**) &pUlLogData->UriStem,
                           &pUlLogData->UriStemLength);

        //
        // If an ISAPI used HSE_APPEND_LOG_PARAMETER use it
        //
        if (_LogContext.m_strLogParam.IsEmpty())
        {
            if (FAILED(hr = pRequest->GetQueryStringA(&_LogContext.m_strLogParam)))
            {
                goto Exit;
            }
        }
        pUlLogData->UriQuery = _LogContext.m_strLogParam.QueryStr();
        pUlLogData->UriQueryLength = (USHORT)_LogContext.m_strLogParam.QueryCCH();

        pUlLogData->ProtocolStatus = pResponse->QueryStatusCode();
        if ( HRESULT_FACILITY( QueryErrorStatus() ) == FACILITY_WIN32 )
        {
            pUlLogData->Win32Status = WIN32_FROM_HRESULT( QueryErrorStatus() );
        }
        else
        {
            pUlLogData->Win32Status = QueryErrorStatus();
        }

        if (!fCollectForULLogging)
        {
            _LogContext.m_msProcessingTime =
                GetTickCount() - _LogContext.m_msStartTickCount;
        }
    }

    //
    // Now get data unaffected by any SF_NOTIFY_LOG filter
    //
    
    HTTP_SUB_ERROR subError;
    pResponse->QuerySubError(&subError);
    pUlLogData->SubStatus = subError.mdSubError;

    pUlLogData->ServerPort = ntohs( pRequest->QueryLocalPort() );

    pUlLogData->ServiceName = QuerySite()->QueryName()->QueryStr();
    pUlLogData->ServiceNameLength = (USHORT)QuerySite()->QueryName()->QueryCCH();

    pUlLogData->ServerName = g_pW3Server->QueryComputerName();
    pUlLogData->ServerNameLength = g_pW3Server->QueryComputerNameLength();

    if (fCollectForULLogging)
    {
        pUlLogData->Host = pRequest->GetHeader(HttpHeaderHost,
                                               &pUlLogData->HostLength);

        pUlLogData->UserAgent = pRequest->GetHeader(HttpHeaderUserAgent,
                                                    &pUlLogData->UserAgentLength);
        pUlLogData->Cookie = pRequest->GetHeader(HttpHeaderCookie,
                                                 &pUlLogData->CookieLength);

        pUlLogData->Referrer = pRequest->GetHeader(HttpHeaderReferer,
                                                   &pUlLogData->ReferrerLength);
    }
    else
    {
        if (FAILED(hr = GetServerVariableHttpVersion(this,
                                        &_LogContext.m_strVersion)))
        {
            goto Exit;
        }

        if (QuerySite()->IsRequiredExtraLoggingFields())
        {
            //
            // If the custom logging module needs extra logging fields, get them
            //

            const MULTISZA *pmszExtraLoggingFields =
                QuerySite()->QueryExtraLoggingFields();
            LPSTR pszHeaderName = (LPSTR)pmszExtraLoggingFields->First();
            STACK_STRA( strHeaderName, 128);

            while (pszHeaderName != NULL)
            {
                hr = strHeaderName.Copy( pszHeaderName,
                                         (DWORD)strlen(pszHeaderName) - 1 );
                if (FAILED(hr))
                {
                    goto Exit;
                }

                hr = pRequest->GetHeader( strHeaderName,
                                          &strVal );
                if (FAILED(hr))
                {
                    if (hr == HRESULT_FROM_WIN32(ERROR_INVALID_INDEX))
                    {
                        strVal.Reset();
                        hr = S_OK;
                    }
                    else
                    {
                        goto Exit;
                    }
                }

                if (!_LogContext.m_mszHTTPHeaders.Append(strVal.QueryStr()))
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    goto Exit;
                }

                pszHeaderName = (LPSTR)pmszExtraLoggingFields->Next(pszHeaderName);
            }
        }
    }

 Exit:
    return hr;
}

//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:  parse.cxx
//
//  Contents:  Parser Object
//
//  History:
//----------------------------------------------------------------------------
#define MAX_TOKEN_LENGTH       MAX_PATH+1
#define MAX_KEYWORDS           4

typedef struct _kwdlist {
    DWORD   dwTokenId;
    LPWSTR  Keyword;
} KWDLIST, *PKWDLIST;

class FAR CLexer
{
public:
    CLexer(LPWSTR szBuffer);
    ~CLexer();

    BOOL
    CLexer::IsKeyword(LPWSTR szToken, LPDWORD pdwToken);

    WCHAR
    CLexer::NextChar();

    void
    CLexer::PushbackChar();

    HRESULT
    CLexer::GetNextToken(LPWSTR szToken, LPDWORD pdwToken);

    HRESULT
    CLexer::PushBackToken();

    void
    CLexer::SetAtDisabler(BOOL bFlag);

    BOOL
    CLexer::GetAtDisabler();

private:

    LPWSTR _ptr;
    LPWSTR _Buffer;
    DWORD  _dwLastTokenLength;
    DWORD  _dwLastToken;
    DWORD  _dwEndofString;
    BOOL   _bAtDisabled;

};

HRESULT
ADsObject(CLexer * pTokenizer, POBJECTINFO pObjectInfo);

HRESULT
IISObject(CLexer * pTokenizer, POBJECTINFO pObjectInfo);

HRESULT
PathName(CLexer * pTokenizer, POBJECTINFO pObjectInfo);

HRESULT
Component(CLexer * pTokenizer, POBJECTINFO pObjectInfo);

HRESULT
Type(CLexer * pTokenizer, POBJECTINFO pObjectInfo);

HRESULT
ProviderName(CLexer * pTokenizer, POBJECTINFO pObjectInfo);

HRESULT
AddTreeName(POBJECTINFO pObjectInfo, LPWSTR szToken);

HRESULT
AddClassName(POBJECTINFO pObjectInfo, LPWSTR szToken);

HRESULT
SetType(POBJECTINFO pObjectInfo, DWORD dwToken);

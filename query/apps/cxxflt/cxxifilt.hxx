//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994
//
//  File:       txtifilt.hxx
//
//  Contents:   Text filter 'class factory'.
//
//  History:    23-Feb-1994     KyleP   Created
//
//  Notes:      Machine generated.  Hand modified.
//
//--------------------------------------------------------------------------

#pragma once

//
// Standard Ole exports
//

extern "C" SCODE STDMETHODCALLTYPE DllGetClassObject( REFCLSID   cid,
                                                      REFIID     iid,
                                                      void **    ppvObj );

extern "C" SCODE STDMETHODCALLTYPE DllCanUnloadNow( void );

//+-------------------------------------------------------------------------
//
//  Class:      CxxIFilterBase
//
//  Purpose:    Manage aggregation, refcounting for CTextIFilter
//
//  History:    23-Feb-94 KyleP     Created
//
//--------------------------------------------------------------------------

extern "C" GUID CLSID_CTextIFilter;

class CxxIFilterBase : public IFilter, public IPersistFile, public IPersistStream
{
public:

    //
    // From IUnknown
    //

    SCODE STDMETHODCALLTYPE  QueryInterface(REFIID riid, void  * * ppvObject);

    ULONG STDMETHODCALLTYPE  AddRef();

    ULONG STDMETHODCALLTYPE  Release();

    //
    // From IFilter
    //

    SCODE STDMETHODCALLTYPE  Init( ULONG grfFlags,
                                   ULONG cAttributes,
                                   FULLPROPSPEC const * aAttributes,
                                   ULONG * pFlags ) = 0;

    SCODE STDMETHODCALLTYPE  GetChunk( STAT_CHUNK * pStat) = 0;

    SCODE STDMETHODCALLTYPE  GetText( ULONG * pcwcBuffer,
                                      WCHAR * awcBuffer ) = 0;

    SCODE STDMETHODCALLTYPE  GetValue( PROPVARIANT * * ppPropValue ) = 0;

    SCODE STDMETHODCALLTYPE  BindRegion( FILTERREGION origPos,
                                         REFIID riid,
                                         void ** ppunk) = 0;

    //
    // From IPersistFile
    //

    SCODE STDMETHODCALLTYPE  GetClassID( CLSID * pClassID ) = 0;

    SCODE STDMETHODCALLTYPE  IsDirty() = 0;

    SCODE STDMETHODCALLTYPE  Load( LPCWSTR pszFileName,
                                   DWORD dwMode) = 0;

    SCODE STDMETHODCALLTYPE  Save( LPCWSTR pszFileName,
                                   BOOL fRemember ) = 0;

    SCODE STDMETHODCALLTYPE  SaveCompleted( LPCWSTR pszFileName ) = 0;

    SCODE STDMETHODCALLTYPE  GetCurFile( LPWSTR  * ppszFileName ) = 0;

    //
    // From IPersistStream
    //

    SCODE STDMETHODCALLTYPE Load( IStream * pStm ) = 0;

    SCODE STDMETHODCALLTYPE Save( IStream * pStm, BOOL fClearDirty ) = 0;

    SCODE STDMETHODCALLTYPE GetSizeMax( ULARGE_INTEGER * pcbSize ) = 0;

protected:
    friend class CTextIFilterCF;

            CxxIFilterBase();
    virtual ~CxxIFilterBase();

    long _uRefs;
};

//+-------------------------------------------------------------------------
//
//  Class:      CTextIFilterCF
//
//  Purpose:    Class factory for text filter class
//
//  History:    23-Feb-94 KyleP     Created
//
//--------------------------------------------------------------------------

class CxxIFilterCF : public IClassFactory
{
public:

    CxxIFilterCF();

    virtual  SCODE STDMETHODCALLTYPE  QueryInterface( REFIID riid,
                                                      void  ** ppvObject);

    virtual  ULONG STDMETHODCALLTYPE  AddRef();

    virtual  ULONG STDMETHODCALLTYPE  Release();

    virtual  SCODE STDMETHODCALLTYPE  CreateInstance( IUnknown * pUnkOuter,
                                                      REFIID riid, void  * * ppvObject );

    virtual  SCODE STDMETHODCALLTYPE  LockServer( BOOL fLock );

protected:

    friend SCODE STDMETHODCALLTYPE DllGetClassObject( REFCLSID cid,
                                                      REFIID iid, void** ppvObj );
    virtual ~CxxIFilterCF();

    long _uRefs;
};



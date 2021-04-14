/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    xlatobj.cpp

Abstract:

    Implementation of routines that deal with translation of generic MSMQ objects:
    CObjXlateInfo

Author:

    Raanan Harari (raananh)

--*/

#include "ds_stdh.h"
#include <activeds.h>
#include "mqad.h"
#include "_propvar.h"
#include "dsutils.h"
#include "utils.h"
#include "xlatqm.h"
#include <adsiutl.h>

#include "xlatobj.tmh"


static WCHAR *s_FN=L"mqad/xlatobj";

//--------------------------------------------------------------------
// static functions fwd declaration
//--------------------------------------------------------------------

static 
HRESULT 
GetPropvarByIADs(
		IN IADs * pIADs,
		IN LPCWSTR pwszPropName,
		IN ADSTYPE adstype,
		IN VARTYPE vartype,
		IN BOOL fMultiValued,
		OUT PROPVARIANT * ppropvarResult
		);


static
HRESULT 
GetPropvarByDN(
		IN LPCWSTR pwszObjectDN,
		IN LPCWSTR pwcsDomainController,
        IN bool	   fServerName,
		IN LPCWSTR pwszPropName,
		IN ADSTYPE adstype,
		IN VARTYPE vartype,
		IN BOOL fMultiValued,
		OUT PROPVARIANT * ppropvarResult,
		OUT IADs ** ppIADs
		);


static 
HRESULT 
GetPropvarBySearchObj(
		IN IDirectorySearch *pSearchObj,
		IN ADS_SEARCH_HANDLE hSearch,
		IN LPCWSTR pwszPropName,
		IN ADSTYPE adstype,
		IN VARTYPE vartype,
		OUT PROPVARIANT * ppropvarResult
		);

//--------------------------------------------------------------------
// CObjXlateInfo implementation
//--------------------------------------------------------------------

CObjXlateInfo::CObjXlateInfo(
                    LPCWSTR             pwszObjectDN,
                    const GUID*         pguidObjectGuid)
/*++
    Constructor for the generic xlate info for an MSMQ objects
--*/
{
    //
    // record the DN of the object if any
    //
    if (pwszObjectDN)
    {
        m_pwszObjectDN = new WCHAR[wcslen(pwszObjectDN) + 1];
        wcscpy(m_pwszObjectDN, pwszObjectDN);
    }

    //
    // record the guid of the object if any
    //
    if (pguidObjectGuid)
    {
        m_pguidObjectGuid = new GUID;
        *m_pguidObjectGuid = *pguidObjectGuid;
    }
//
//    no need for following initialization since these are auto-release and inited
//    to NULL already
//
//    m_pIADs = NULL;
//    m_pSearchObj = NULL;
//
}


CObjXlateInfo::~CObjXlateInfo()
/*++
    Destructor for the generic xlate info for an MSMQ objects.
--*/
{
    //
    // members are auto release
    //
}


void CObjXlateInfo::InitGetDsProps(IN IADs * pIADs)
/*++
    Abstract:
        Initialization for GetDsProp call.
        GetDsProp will use the given IADs object when trying to
        get props for the object.

    Parameters:
        pIADs           - IADs interface for the object

    Returns:
      None
--*/
{
    pIADs->AddRef();  // keep it alive
    m_pIADs = pIADs;  // will auto release on destruction
}


void CObjXlateInfo::InitGetDsProps(IN IDirectorySearch * pSearchObj,
                                       IN ADS_SEARCH_HANDLE hSearch)
/*++
    Abstract:
        Initialization for GetDsProp call.
        GetDsProp will use the given search object first when trying to
        get props for the object, before binding to it using IADs.

    Parameters:
        pSearchObj      - search object
        hSearch         - search handle

    Returns:
      None
--*/
{
    pSearchObj->AddRef();      // keep it alive
    m_pSearchObj = pSearchObj; // will auto release on destruction
    m_hSearch = hSearch;
}


HRESULT 
CObjXlateInfo::GetDsProp(
	IN LPCWSTR pwszPropName,
	LPCWSTR    pwcsDomainController,
	IN bool	   fServerName,
	IN ADSTYPE adstype,
	IN VARTYPE vartype,
	IN BOOL fMultiValued,
	OUT PROPVARIANT * ppropvarResult
	)
/*++
    Abstract:
        Get a DS property value of the object as a propvariant, w/o going
        to translation routine or default value

    Parameters:
        pwszPropName    - property name
        adstype         - requested ADSTYPE
        vartype         - requested VARTYPE in result propvariant
        fMultiValued    - whether the property is multi-valued in the DS
        ppropvarResult  - propvariant to fill, should be empty already

    Returns:
      MQ_OK - success, ppropvarResult is filled
      E_ADS_PROPERTY_NOT_FOUND - property was not found
      other HRESULT errors
--*/
{
    HRESULT hr;
    CMQVariant propvarResult;
    BOOL fGotPropFromSearchObj = FALSE;

    //
    // start with getting the property from search object
    //
    if (m_pSearchObj.get() != NULL)
    {
        hr = GetPropvarBySearchObj(m_pSearchObj.get(),
                                   m_hSearch,
                                   pwszPropName,
                                   adstype,
                                   vartype,
                                   propvarResult.CastToStruct());
        if (FAILED(hr))
        {
            TrERROR(DS, "Failed to get propery %ls from search object. %!hresult!",pwszPropName, hr);
            return hr;
        }

        //
        // hr could be S_OK (if property found) or S_FALSE (if property was not requested in search)
        //
        if (hr == S_OK) //e.g. (hr != S_FALSE)
        {
            //
            // we don't need to check further
            //
            fGotPropFromSearchObj = TRUE;
        }
    }

    //
    // if search object was not helpfull, use IADs
    //
    if (!fGotPropFromSearchObj)
    {
        //
        // property was not found, use IADs
        //
        if (m_pIADs.get() != NULL)
        {
            //
            // there is already an open IADs for the object, use it
            //
            hr = GetPropvarByIADs(m_pIADs.get(),
                                  pwszPropName,
                                  adstype,
                                  vartype,
                                  fMultiValued,
                                  propvarResult.CastToStruct());
            if (FAILED(hr))
            {
                TrERROR(DS, "Failed to get propvar by IADs for %ls. %!hresult!", pwszPropName, hr);
                return hr;
            }
        }
        else
        {
            //
            // IADs is not set, bind to the object, and save the IADs
            //
            R<IADs> pIADs;
            hr = GetPropvarByDN(
						ObjectDN(),
						pwcsDomainController,
						fServerName,
						pwszPropName,
						adstype,
						vartype,
						fMultiValued,
						propvarResult.CastToStruct(),
						&pIADs.ref()
						);
            if (FAILED(hr))
            {
                TrERROR(DS, "Failed to get propvar by DN for %ls. %!hresult!", pwszPropName, hr);
                return hr;
            }

            //
            // save the IADs
            // we must not AddRef it since we created it and we need to totally release
            // it on destruction, it is not a passed parameter we need to keep alive
            //
            m_pIADs = pIADs;
        }
    }

    //
    // return values
    //
    *ppropvarResult = *(propvarResult.CastToStruct());
    (propvarResult.CastToStruct())->vt = VT_EMPTY;
    return MQ_OK;
}



//--------------------------------------------------------------------
// external functions
//--------------------------------------------------------------------


//--------------------------------------------------------------------
// static functions
//--------------------------------------------------------------------

static 
HRESULT 
GetPropvarByIADs(
		IN IADs * pIADs,
		IN LPCWSTR pwszPropName,
		IN ADSTYPE adstype,
		IN VARTYPE vartype,
		IN BOOL fMultiValued,
		OUT PROPVARIANT * ppropvarResult
		)
/*++
    Abstract:
        Get a DS property as a propvariant, w/o going to translation routine,
        using IADs

    Parameters:
        pIADs           - IADs interface for the object
        pwszPropName    - property name
        adstype         - requested ADSTYPE
        vartype         - requested VARTYPE in result propvariant
        fMultiValued    - whether the property is multi-valued in the DS
        ppropvarResult  - propvariant to fill, should be empty already

    Returns:
      S_OK - success, ppropvarResult is filled
      other HRESULT errors
--*/
{
    HRESULT hr;
    //
    // get prop
    //
    CAutoVariant varResult;
    BS bsProp = pwszPropName;
    if (fMultiValued)
    {
        hr = pIADs->GetEx(bsProp, &varResult);
    }
    else
    {
        hr = pIADs->Get(bsProp, &varResult);
    }    
    if (FAILED(hr))
    {
        TrERROR(DS, "Failed to get property %ls from object. %!hresult!", pwszPropName, hr);
        return hr;
    }

    //
    // translate to propvariant
    //
    CMQVariant propvarResult;
    hr = Variant2MqVal(propvarResult.CastToStruct(), &varResult, adstype, vartype);
    if (FAILED(hr))
    {
        TrERROR(DS, "Failed to ranslates OLE Variant into MQPropVal value. %!hresult!", hr);
        return hr;
    }

    //
    // return values
    //
    *ppropvarResult = *(propvarResult.CastToStruct());
    (propvarResult.CastToStruct())->vt = VT_EMPTY;
    return S_OK;
}


static 
HRESULT 
GetPropvarByDN(
	IN LPCWSTR pwszObjectDN,
	IN LPCWSTR pwcsDomainController,
    IN bool	   fServerName,
	IN LPCWSTR pwszPropName,
	IN ADSTYPE adstype,
	IN VARTYPE vartype,
	IN BOOL fMultiValued,
	OUT PROPVARIANT * ppropvarResult,
	OUT IADs ** ppIADs
	)
/*++
    Abstract:
        Get a DS property as a propvariant, w/o going to translation routine,
        using its DN. It also returns the IADs for the object.

    Parameters:
        pwszObjectDN    - distinguished name of the object
        pwszPropName    - property name
        adstype         - requested ADSTYPE
        vartype         - requested VARTYPE in result propvariant
        fMultiValued    - whether the property is multi-valued in the DS
        ppropvarResult  - propvariant to fill, should be empty already
        ppIADs          - returned IADs interface for the object

    Returns:
      S_OK - success, ppropvarResult is filled
      other HRESULT errors
--*/
{
    HRESULT hr;

    //
    // Create ADSI path
    //
    DWORD lenDC = (pwcsDomainController != NULL) ? wcslen(pwcsDomainController) : 0;
    AP<WCHAR> pwszPath = new WCHAR[1 + x_LdapProviderLen + lenDC + 1 + wcslen(pwszObjectDN)];

    if (pwcsDomainController == NULL)
    {
        swprintf( pwszPath,
                  L"%s%s",
                  x_LdapProvider,
                  pwszObjectDN);
    }
    else
    {
        swprintf( pwszPath,
                  L"%s%s/%s",
                  x_LdapProvider,
                  pwcsDomainController,
                  pwszObjectDN);
    }
    //
    // bind to the obj
    //
    R<IADs> pIADs;

	DWORD Flags = ADS_SECURE_AUTHENTICATION;
	if(fServerName && (pwcsDomainController != NULL))
	{
		Flags |= ADS_SERVER_BIND;
	}

	AP<WCHAR> pEscapeAdsPathNameToFree;

	hr = ADsOpenObject(
			UtlEscapeAdsPathName(pwszPath, pEscapeAdsPathNameToFree),
			NULL,
			NULL,
			Flags, 
			IID_IADs,
			(void**)&pIADs
			);
	

    LogTraceQuery(pwszPath, s_FN, 59);
    if (FAILED(hr))
    {
        TrERROR(DS, "Failed to open %ls. %!hresult!", pwszPath, hr);
        return hr;
    }

    //
    // get the prop
    //
    CMQVariant propvarResult;
    hr = GetPropvarByIADs(pIADs.get(), pwszPropName, adstype, vartype, fMultiValued, propvarResult.CastToStruct());
    if (FAILED(hr))
    {
        TrERROR(DS, "Failed to get propvar %ls. %!hresult!",pwszPropName, hr);
        return hr;
    }

    //
    // return values
    //
    *ppropvarResult = *(propvarResult.CastToStruct());
    (propvarResult.CastToStruct())->vt = VT_EMPTY;
    *ppIADs = pIADs.detach();
    return S_OK;
}


static 
HRESULT 
GetPropvarBySearchObj(
	IN IDirectorySearch *pSearchObj,
	IN ADS_SEARCH_HANDLE hSearch,
	IN LPCWSTR pwszPropName,
	IN ADSTYPE adstype,
	IN VARTYPE vartype,
	OUT PROPVARIANT * ppropvarResult
	)
/*++
    Abstract:
        Get a DS property as a propvariant, w/o going to translation routine,
        using a search object.
        Note it might not find the property if it was not requested by the
        search originator.

    Parameters:
        pSearchObj      - search object
        hSearch         - search handle
        pwszPropName    - property name
        adstype         - requested ADSTYPE
        vartype         - requested VARTYPE in result propvariant
        ppropvarResult  - propvariant to fill, should be empty already

    Returns:
      S_OK - success, ppropvarResult is filled
      S_FALSE - property not requested by search originator, ppropvarResult is not filled
      other HRESULT errors
--*/
{
    //
    // check if prop is requested
    //
    ADS_SEARCH_COLUMN columnProp;
    HRESULT hr = pSearchObj->GetColumn(hSearch, const_cast<LPWSTR>(pwszPropName), &columnProp);
    if (FAILED(hr) && (hr != E_ADS_COLUMN_NOT_SET))
    {
        TrERROR(DS, "Failed to get column for %ls. %!hresult!", pwszPropName, hr);
        return hr;
    }
    
    if (hr == E_ADS_COLUMN_NOT_SET)
    {
        //
        // property was not requested
        //
        return S_FALSE;
    }

    //
    // property was found, make sure the column is freed eventually
    //
    CAutoReleaseColumn cAutoReleaseColumnProp(pSearchObj, &columnProp);

    //
    // convert it to propvariant
    //
    CMQVariant propvarResult;
    hr = AdsiVal2MqVal(propvarResult.CastToStruct(),
                       vartype,
                       adstype,
                       columnProp.dwNumValues,
                       columnProp.pADsValues);
    if (FAILED(hr))
    {
        TrERROR(DS, "Failed to convert val to mqval %!hresult!", hr);
        return hr;
    }

    //
    // return values
    //
    *ppropvarResult = *(propvarResult.CastToStruct());
    (propvarResult.CastToStruct())->vt = VT_EMPTY;
    return S_OK;
}

/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    mqadp.cpp

Abstract:

    MQAD DLL private internal functions for
    DS queries, etc.

Author:

    ronit hartmann ( ronith)

--*/

#include "ds_stdh.h"
#include <mqaddef.h>
#include "baseobj.h"
#include "mqadglbo.h"
#include "siteinfo.h"
#include "mqattrib.h"
#include "utils.h"
#include "adtempl.h"
#include "dsutils.h"
#include "mqadp.h"
#include "queryh.h"
#include "delqn.h"

#include "mqadp.tmh"

static WCHAR *s_FN=L"mqad/mqadp";



void MQADpAllocateObject(
                AD_OBJECT            eObject,
                LPCWSTR              pwcsDomainController,
				bool				 fServerName,
                LPCWSTR              pwcsObjectName,
                const GUID *         pguidObject,
                const SID *          pSid,
                CBasicObjectType**   ppObject
                )
{

    switch( eObject)
    {
    case eQUEUE:
        *ppObject = new CQueueObject(
                                    pwcsObjectName,
                                    pguidObject,
                                    pwcsDomainController,
									fServerName
                                    );
        break;

    case eMACHINE:
        *ppObject = new CMqConfigurationObject(
                                    pwcsObjectName,
                                    pguidObject,
                                    pwcsDomainController,
									fServerName
                                    );
        break;

    case eSITE:
        *ppObject = new CSiteObject(
                                    pwcsObjectName,
                                    pguidObject,
                                    pwcsDomainController,
									fServerName
                                    );
        break;

    case eFOREIGNSITE:
        *ppObject = new CSiteObject(
                                    pwcsObjectName,
                                    pguidObject,
                                    pwcsDomainController,
									fServerName,
									true	// fForeignSite
                                    );
        break;

    case eENTERPRISE:
        *ppObject = new CEnterpriseObject(
                                    pwcsObjectName,
                                    pguidObject,
                                    pwcsDomainController,
									fServerName
                                    );
        break;

    case eUSER:
        *ppObject = new CUserObject(
                                    pwcsObjectName,
                                    pguidObject,
                                    pSid,
                                    pwcsDomainController,
									fServerName
                                    );
        break;

    case eROUTINGLINK:
        *ppObject = new CRoutingLinkObject(
                                    pwcsObjectName,
                                    pguidObject,
                                    pwcsDomainController,
									fServerName
                                    );
        break;

    case eSERVER:
        *ppObject = new CServerObject(
                                    pwcsObjectName,
                                    pguidObject,
                                    pwcsDomainController,
									fServerName
                                    );
        break;

    case eSETTING:
        *ppObject = new CSettingObject(
                                    pwcsObjectName,
                                    pguidObject,
                                    pwcsDomainController,
									fServerName
                                    );
        break;

    case eCOMPUTER:
        *ppObject = new CComputerObject(
                                    pwcsObjectName,
                                    pguidObject,
                                    pwcsDomainController,
									fServerName
                                    );
        break;

    case eMQUSER:
        *ppObject = new CMqUserObject(
                                    pwcsObjectName,
                                    pguidObject,
                                    pSid,
                                    pwcsDomainController,
									fServerName
                                    );
        break;


    default:
        ASSERT(0);
        *ppObject = NULL;
        break;
    }
}


const WCHAR x_limitedChars[] = {L'\n',L'/',L'#',L'>',L'<', L'=', 0x0a, 0};
const DWORD x_numLimitedChars = sizeof(x_limitedChars)/sizeof(WCHAR) - 1;

/*====================================================
    FilterSpecialCharaters()
    Pares the object (queue) name and add escape character before limited chars

    If pwcsOutBuffer is NULL, the function allocates a new buffer and return it as
    return value. Otherwise, it uses pwcsOutBuffer, and return it. If pwcsOutBuffer is not
    NULL, it should point to a buffer of lenght dwNameLength*2 +1, at least.

  NOTE: dwNameLength does not contain existing escape characters, if any
=====================================================*/
WCHAR * FilterSpecialCharacters(
            IN     LPCWSTR          pwcsObjectName,
            IN     const DWORD      dwNameLength,
            IN OUT LPWSTR pwcsOutBuffer /* = 0 */,
            OUT    DWORD_PTR* pdwCharactersProcessed /* = 0 */)

{
    AP<WCHAR> pBufferToRelease;
    LPWSTR pname;

    if (pwcsOutBuffer != 0)
    {
        pname = pwcsOutBuffer;
    }
    else
    {
        pBufferToRelease = new WCHAR[ (dwNameLength *2) + 1];
        pname = pBufferToRelease;
    }

    const WCHAR * pInChar = pwcsObjectName;
    WCHAR * pOutChar = pname;
    for ( DWORD i = 0; i < dwNameLength; i++, pInChar++, pOutChar++)
    {
        //
        // Ignore current escape characters
        //
        if (*pInChar == L'\\')
        {
            *pOutChar = *pInChar;
            pOutChar++;
            pInChar++;
        }
        else
        {
            //
            // Add backslash before special characters, unless it was there
            // already.
            //
            if ( 0 != wcschr(x_limitedChars, *pInChar))
            {
                *pOutChar = L'\\';
                pOutChar++;
            }
        }

        *pOutChar = *pInChar;
    }
    *pOutChar = L'\0';

    pBufferToRelease.detach();

    if (pdwCharactersProcessed != 0)
    {
        *pdwCharactersProcessed = pInChar - pwcsObjectName;
    }
    return( pname);
}

CCriticalSection s_csInitialization;

HRESULT
MQADInitialize(
    IN  bool    fIncludingVerify
    )
/*++

Routine Description:
    Initialize MQAD

Arguments:
    fIncludingVerify - indication if initialization of update-allowed is
                       required for the performed operation

Return Value
	HRESULT

--*/
{

    if (fIncludingVerify)
    {
        HRESULT hr = g_VerifyUpdate.Initialize();
        if (FAILED(hr))
        {
			TrERROR(DS, "g_VerifyUpdate.Initialize failed, create/delete/update operation will not be allowed, hr = 0x%x", hr);
            return LogHR(hr, s_FN, 20);
        }
    }

    if (g_fInitialized)
    {
        return MQ_OK;
    }

    //
    //  Postponding initialization to the point where Active
    //  Directory access is really needed
    //

    //
    //  Access AD without holding critical section
    //
    AP<WCHAR> pwcsDsRoot;
    AP<WCHAR> pwcsLocalDsRoot;
    AP<WCHAR> pwcsSchemaContainer;
    AP<WCHAR> pwcsConfigurationContainer;

    HRESULT hr = g_AD.GetRootDsName(
                    &pwcsDsRoot,
                    &pwcsLocalDsRoot,
                    &pwcsSchemaContainer,
                    &pwcsConfigurationContainer
                    );

    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 10);
    }

    {
        CS lock(s_csInitialization);
        if (g_fInitialized)
        {
            return MQ_OK;
        }

		//
		// Initialize global constant strings, done only once
		//
		
		ASSERT(g_pwcsDsRoot.get() == NULL);

        MQADpInitPropertyTranslationMap();

        //
        //  build services, sites and msmq-service path names
        //
        DWORD ConfigLen = wcslen(pwcsConfigurationContainer);

        AP<WCHAR> pwcsServicesContainer = new WCHAR[ConfigLen +  x_ServicePrefixLen + 2];
        swprintf(
            pwcsServicesContainer,
             L"%s"
             TEXT(",")
             L"%s",
            x_ServicesPrefix,
            pwcsConfigurationContainer.get()
            );

        AP<WCHAR> pwcsMsmqServiceContainer = new WCHAR[ConfigLen + x_MsmqServicePrefixLen + 2];
        swprintf(
            pwcsMsmqServiceContainer,
             L"%s"
             TEXT(",")
             L"%s",
            x_MsmqServicePrefix,
            pwcsConfigurationContainer.get()
            );

        AP<WCHAR> pwcsSitesContainer = new WCHAR[ConfigLen +  x_SitesPrefixLen + 2];

        swprintf(
            pwcsSitesContainer,
             L"%s"
             TEXT(",")
             L"%s",
            x_SitesPrefix,
            pwcsConfigurationContainer.get()
            );

        g_pwcsDsRoot = pwcsDsRoot.detach();
		TrTRACE(DS, "DsRoot = %ls", g_pwcsDsRoot.get());

        g_pwcsLocalDsRoot = pwcsLocalDsRoot.detach();
		TrTRACE(DS, "LocalDsRoot = %ls", g_pwcsLocalDsRoot.get());

        g_pwcsSchemaContainer = pwcsSchemaContainer.detach();
		TrTRACE(DS, "SchemaContainer = %ls", g_pwcsSchemaContainer.get());

		g_pwcsConfigurationContainer = pwcsConfigurationContainer.detach();
		TrTRACE(DS, "ConfigurationContainer = %ls", g_pwcsConfigurationContainer.get());

		g_pwcsServicesContainer = pwcsServicesContainer.detach();
		TrTRACE(DS, "ServicesContainer = %ls", g_pwcsServicesContainer.get());

		g_pwcsMsmqServiceContainer = pwcsMsmqServiceContainer.detach();
		TrTRACE(DS, "MsmqServiceContainer = %ls", g_pwcsMsmqServiceContainer.get());

		g_pwcsSitesContainer = pwcsSitesContainer.detach();
		TrTRACE(DS, "SitesContainer = %ls", g_pwcsSitesContainer.get());

        g_fInitialized = true;
    }
    return MQ_OK;
}

//+------------------------------------------
//
//  HRESULT MQADpQueryNeighborLinks()
//
//+------------------------------------------

HRESULT MQADpQueryNeighborLinks(
                        IN  LPCWSTR            pwcsDomainController,
						IN  bool			   fServerName,
                        IN  eLinkNeighbor      LinkNeighbor,
                        IN  LPCWSTR            pwcsNeighborDN,
                        IN OUT CSiteGateList * pSiteGateList
                        )

/*++

Routine Description:

Arguments:
        eLinkNeighbor :  specify according to which neighbor property, to perform
                         the locate ( PROPID_L_NEIGHBOR1 or PROPID_L_NEIGHBOR2)
        pwcsNeighborDN : the DN name of the site

        CSiteGateList : list of site-gates

Return Value:
--*/
{
    //
    //  Query the gates on all the links of a specific site ( pwcsNeighborDN).
    //  But only on links where the site is specified as
    //  neighbor-i ( 1 or 2)
    //
    MQPROPERTYRESTRICTION propRestriction;
    propRestriction.rel = PREQ;

    const WCHAR * pwcsAttribute = NULL;

    if ( LinkNeighbor == eLinkNeighbor1)
    {
        pwcsAttribute = MQ_L_NEIGHBOR1_ATTRIBUTE;
    }
    else
    {
        pwcsAttribute = MQ_L_NEIGHBOR2_ATTRIBUTE;
    }
    ASSERT( pwcsAttribute != NULL);


    AP<WCHAR> pwcsFilteredNeighborDN;
    StringToSearchFilter( pwcsNeighborDN,
                          &pwcsFilteredNeighborDN
                          );
    R<CRoutingLinkObject> pObject = new CRoutingLinkObject(NULL,NULL, pwcsDomainController, fServerName);

    DWORD dwFilterLen = x_ObjectCategoryPrefixLen +
                        pObject->GetObjectCategoryLength() +
                        x_ObjectCategorySuffixLen +
                        wcslen(pwcsAttribute) +
                        wcslen(pwcsFilteredNeighborDN) +
                        13;

    AP<WCHAR> pwcsSearchFilter = new WCHAR[ dwFilterLen];

    DWORD dw = swprintf(
        pwcsSearchFilter,
        L"(&%s%s%s(%s=%s))",
        x_ObjectCategoryPrefix,
        pObject->GetObjectCategory(),
        x_ObjectCategorySuffix,
        pwcsAttribute,
        pwcsFilteredNeighborDN.get()
        );
    DBG_USED( dw);
	ASSERT( dw < dwFilterLen);


    PROPID prop = PROPID_L_GATES_DN;

    CAdQueryHandle hQuery;
    HRESULT hr;

    hr = g_AD.LocateBegin(
            searchOneLevel,	
            adpDomainController,
            e_MsmqServiceContainer,
            pObject.get(),
            NULL,   //pguidSearchBase
            pwcsSearchFilter,
            NULL,   // pDsSortkey
            1,
            &prop,
            hQuery.GetPtr());

    if ( hr == HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT))
    {
        TrWARNING(DS, "MsmqServices not found = 0x%x",  hr);
        return(MQ_OK);
    }
    if (FAILED(hr))
    {
        TrWARNING(DS, "Locate begin failed = 0x%x",  hr);
        return LogHR(hr, s_FN, 190);
    }
    //
    //  Read the results one by one
    //

    DWORD cp = 1;

    while (SUCCEEDED(hr))
    {
        cp = 1;
        CMQVariant var;
        var.SetNULL();

        hr = g_AD.LocateNext(
                    hQuery.GetHandle(),
                    &cp,
                    var.CastToStruct()
                    );
        if (FAILED(hr))
        {
            TrWARNING(DS, "Locate next failed, hr = 0x%x", hr);
            return LogHR(hr, s_FN, 200);
        }
        if ( cp == 0)
        {
            //
            //  no more results
            //
            break;
        }
        //
        //  Add to list
        //

        if ( var.GetCALPWSTR()->cElems > 0)
        {
            AP<GUID> pguidGates;
            DWORD    dwNumGates;
            HRESULT hr1 = MQADpTranslateGateDn2Id(
								pwcsDomainController,
								fServerName,
								var.CastToStruct(),
								&pguidGates,
								&dwNumGates
								);

			//
			// There might be a success with dwNumGates	= 0
			// in case of deleted objects
			//
            if (SUCCEEDED(hr1) && (dwNumGates > 0))
            {
                pSiteGateList->AddSiteGates(
                         dwNumGates,
                         pguidGates
                         );
            }
        }
    }

    return(MQ_OK);
}


HRESULT MQADpTranslateGateDn2Id(
        IN  LPCWSTR             pwcsDomainController,
		IN  bool				fServerName,
        IN  const PROPVARIANT*  pvarGatesDN,
        OUT GUID **      ppguidLinkSiteGates,
        OUT DWORD *      pdwNumLinkSiteGates
        )
/*++

Routine Description:
    This routine translate PROPID_L_GATES_DN into unique-id array
    of the gates.

Arguments:
    pvarGatesDN -   varaint containing PROPID_L_GATES_DN

Return Value:
--*/
{
    //
    //  For each gate translate its DN to unique id
    //
    if ( pvarGatesDN->calpwstr.cElems == 0)
    {
        *pdwNumLinkSiteGates = 0;
        *ppguidLinkSiteGates = NULL;
        return( MQ_OK);
    }
    //
    //  there are gates
    //
    AP<GUID> pguidGates = new GUID[ pvarGatesDN->calpwstr.cElems];
    PROPID prop = PROPID_QM_MACHINE_ID;
    DWORD  dwNextToFill = 0;
    PROPVARIANT var;
    var.vt = VT_CLSID;
    HRESULT hr = MQ_OK;
    for ( DWORD i = 0; i < pvarGatesDN->calpwstr.cElems; i++)
    {
        var.puuid = &pguidGates[ dwNextToFill];
        CMqConfigurationObject object(NULL, NULL, pwcsDomainController, fServerName);
        object.SetObjectDN( pvarGatesDN->calpwstr.pElems[i]);

        hr = g_AD.GetObjectProperties(
                    adpGlobalCatalog,
                    &object,
                    1,
                    &prop,
                    &var);

        if ( SUCCEEDED(hr))
        {
            dwNextToFill++;
        }


    }
    if ( dwNextToFill > 0)
    {
        //
        //  succeeded to translate some or all gates, return them
        //
        *pdwNumLinkSiteGates = dwNextToFill;
        *ppguidLinkSiteGates = pguidGates.detach();
        return( MQ_OK);

    }
    //
    //  Failed to translate gates
    //
    *pdwNumLinkSiteGates = 0;
    *ppguidLinkSiteGates = NULL;
    return MQ_OK;
}

/*====================================================

RoutineName: InitPropertyTranslationMap

Arguments:  initialize property translation map

Return Value: none

=====================================================*/
void MQADpInitPropertyTranslationMap()
{
    //
    // Populate  g_PropDictionary
    //

    DWORD i;
    const translateProp * pProperty = QueueTranslateInfo;
    for (i = 0; i < ARRAY_SIZE(QueueTranslateInfo); i++, pProperty++)
    {
        g_PropDictionary.SetAt(pProperty->propid, pProperty);
    }

    pProperty = MachineTranslateInfo;
    for (i = 0; i < ARRAY_SIZE(MachineTranslateInfo); i++, pProperty++)
    {
        g_PropDictionary.SetAt(pProperty->propid, pProperty);
    }

    pProperty = EnterpriseTranslateInfo;
    for (i = 0; i < ARRAY_SIZE(EnterpriseTranslateInfo); i++, pProperty++)
    {
        g_PropDictionary.SetAt(pProperty->propid, pProperty);
    }

    pProperty = SiteLinkTranslateInfo;
    for (i = 0; i < ARRAY_SIZE(SiteLinkTranslateInfo); i++, pProperty++)
    {
        g_PropDictionary.SetAt(pProperty->propid, pProperty);
    }

    pProperty = UserTranslateInfo;
    for (i = 0; i < ARRAY_SIZE(UserTranslateInfo); i++, pProperty++)
    {
        g_PropDictionary.SetAt(pProperty->propid, pProperty);
    }

    pProperty = MQUserTranslateInfo;
    for (i = 0; i < ARRAY_SIZE(MQUserTranslateInfo); i++, pProperty++)
    {
        g_PropDictionary.SetAt(pProperty->propid, pProperty);
    }

    pProperty = SiteTranslateInfo;
    for (i = 0; i < ARRAY_SIZE(SiteTranslateInfo); i++, pProperty++)
    {
        g_PropDictionary.SetAt(pProperty->propid, pProperty);
    }

    pProperty = ServerTranslateInfo;
    for (i = 0; i < ARRAY_SIZE(ServerTranslateInfo); i++, pProperty++)
    {
        g_PropDictionary.SetAt(pProperty->propid, pProperty);
    }

    pProperty = SettingTranslateInfo;
    for (i = 0; i < ARRAY_SIZE(SettingTranslateInfo); i++, pProperty++)
    {
        g_PropDictionary.SetAt(pProperty->propid, pProperty);
    }

    pProperty = ComputerTranslateInfo;
    for (i = 0; i < ARRAY_SIZE(ComputerTranslateInfo

        ); i++, pProperty++)
    {
        g_PropDictionary.SetAt(pProperty->propid, pProperty);
    }


}


bool MQADpIsDSOffline(
        IN HRESULT      hr
        )
/*++

Routine Description:
    The routine check if the return code
    indicates no connectivity to ActiveDirectory.

Arguments:
    HRESULT hr - return code of last operation

Return Value
	true   - if no connectivity to ActiveDirectory
    false  - oterwise

--*/
{
    switch (hr)
    {
    case HRESULT_FROM_WIN32(ERROR_NO_SUCH_DOMAIN):
	case HRESULT_FROM_WIN32(ERROR_DS_SERVER_DOWN):
	case HRESULT_FROM_WIN32(ERROR_DS_BUSY):
    case MQ_ERROR_NO_DS:
        return true;
        break;
    default:
        return false;
        break;
    }
}

HRESULT MQADpConvertToMQCode(
                         IN HRESULT   hr,
                         IN AD_OBJECT eObject)
/*++

Routine Description:

Arguments:

Return Value:
--*/
{
    if (hr == MQ_OK)
    {
        return hr;
    }

	if(IsLocalUser())
	{
	    return MQ_ERROR_DS_LOCAL_USER;
	}

    if (MQADpIsDSOffline(hr))
    {
        return MQ_ERROR_NO_DS;
    }
    switch ( hr)
    {
        case HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS):
        case HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS):  //BUGBUG alexdad to throw after transition
        {
        //
        //  Object exists
        //
            switch( eObject)
            {
            case eQUEUE:
                return MQ_ERROR_QUEUE_EXISTS;
                break;
            case eROUTINGLINK:
                return MQDS_E_SITELINK_EXISTS;
                break;
            case eUSER:
                return MQ_ERROR_INTERNAL_USER_CERT_EXIST;
                break;
            case eMACHINE:
                return MQ_ERROR_MACHINE_EXISTS;
                break;
            case eCOMPUTER:
                return MQDS_E_COMPUTER_OBJECT_EXISTS;
                break;
            default:
                return hr;
                break;
            }
        }
        break;

        case HRESULT_FROM_WIN32(ERROR_DS_DECODING_ERROR):
        case HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT):
        {
        //
        //  Object not found
        //
            switch( eObject)
            {
            case eQUEUE:
                return MQ_ERROR_QUEUE_NOT_FOUND;
                break;
           case eMACHINE:
                return MQ_ERROR_MACHINE_NOT_FOUND;
                break;
            default:
                return MQDS_OBJECT_NOT_FOUND;
                break;
            }
        }
        break;

        case E_ADS_BAD_PATHNAME:
        {
            //
            //  wrong pathname
            //
            switch( eObject)
            {
            case eQUEUE:
                //
                // creating queue with not allowed chars
                //
                return MQ_ERROR_ILLEGAL_QUEUE_PATHNAME;
                break;

            default:
                return hr;
                break;
            }

        }
        break;

        case HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED):
            return MQ_ERROR_ACCESS_DENIED;

            break;

        //
        // This is an internal warning that should not be returned out of the DS.
		// every warning will convert into error in the RunTime. ilanh 05-Sep-2000 (bug 6035)
		//
        case MQSec_I_SD_CONV_NOT_NEEDED:
            return(MQ_OK);
            break;

        default:
            return hr;
            break;
    }
}


HRESULT MQADpComposeName(
               IN  LPCWSTR   pwcsPrefix,
               IN  LPCWSTR   pwcsSuffix,
               OUT LPWSTR * pwcsFullName
               )
/*++

Routine Description:

Arguments:

Return Value:
--*/
{
    //
    //  compose a distinguished name of an object
    //  format : CN=prefix, suffix
    //

    DWORD LenSuffix = lstrlen(pwcsSuffix);
    DWORD LenPrefix = lstrlen(pwcsPrefix);
    DWORD Length =
            x_CnPrefixLen +                   // "CN="
            LenPrefix +                       // "pwcsPrefix"
            1 +                               //","
            LenSuffix +                       // "pwcsSuffix"
            1;                                // '\0'

    *pwcsFullName = new WCHAR[Length];

    DWORD dw = swprintf(
        *pwcsFullName,
         L"%s"             // "CN="
         L"%s"             // "pwcsPrefix"
         TEXT(",")
         L"%s",            // "pwcsSuffix"
        x_CnPrefix,
        pwcsPrefix,
        pwcsSuffix
        );
    DBG_USED( dw);
	ASSERT( dw < Length);

    return(MQ_OK);


}


/*====================================================
    CAdsi::CompareDefaultValue()
    check the user property val + rel indicates that the
    query should return objects with default values
=====================================================*/
static BOOL CompareDefaultValue(
           IN const ULONG           rel,
           IN const MQPROPVARIANT * pvarUser,
           IN const MQPROPVARIANT * pvarDefaultValue
           )
{
    if ( pvarDefaultValue == NULL)
    {
        return(FALSE);
    }
    if ( pvarUser->vt != pvarDefaultValue->vt )
    {
        return(FALSE);
    }
    switch ( pvarDefaultValue->vt)
    {
        case VT_EMPTY:
            if ( rel == PREQ)
            {
                return TRUE;
            }
            return(FALSE);
            break;

        case VT_I2:
            if ( rel == PREQ)
            {
                return( pvarDefaultValue->iVal == pvarUser->iVal);
            }
            if ( rel == PRNE)
            {
                return( pvarDefaultValue->iVal != pvarUser->iVal);
            }
            if (rel == PRGT)
            {
                 return( pvarDefaultValue->iVal > pvarUser->iVal);
            }
            if (rel == PRGE)
            {
                 return( pvarDefaultValue->iVal >= pvarUser->iVal);
            }
            if (rel == PRLT)
            {
                 return( pvarDefaultValue->iVal < pvarUser->iVal);
            }
            if (rel == PRLE)
            {
                 return( pvarDefaultValue->iVal <= pvarUser->iVal);
            }
            return(FALSE);
            break;

        case VT_I4:
            if ( rel == PREQ)
            {
                return( pvarDefaultValue->lVal == pvarUser->lVal);
            }
            if ( rel == PRNE)
            {
                return( pvarDefaultValue->lVal != pvarUser->lVal);
            }
            if (rel == PRGT)
            {
                 return( pvarDefaultValue->lVal > pvarUser->lVal);
            }
            if (rel == PRGE)
            {
                 return( pvarDefaultValue->lVal >= pvarUser->lVal);
            }
            if (rel == PRLT)
            {
                 return( pvarDefaultValue->lVal < pvarUser->lVal);
            }
            if (rel == PRLE)
            {
                 return( pvarDefaultValue->lVal <= pvarUser->lVal);
            }
            return(FALSE);
            break;

        case VT_UI1:
            if ( rel == PREQ)
            {
                return( pvarDefaultValue->bVal == pvarUser->bVal);
            }
            if ( rel == PRNE)
            {
                return( pvarDefaultValue->bVal != pvarUser->bVal);
            }
            if (rel == PRGT)
            {
                 return( pvarDefaultValue->bVal > pvarUser->bVal);
            }
            if (rel == PRGE)
            {
                 return( pvarDefaultValue->bVal >= pvarUser->bVal);
            }
            if (rel == PRLT)
            {
                 return( pvarDefaultValue->bVal < pvarUser->bVal);
            }
            if (rel == PRLE)
            {
                 return( pvarDefaultValue->bVal <= pvarUser->bVal);
            }
            return(FALSE);
            break;

        case VT_UI2:
            if ( rel == PREQ)
            {
                return( pvarDefaultValue->uiVal == pvarUser->uiVal);
            }
            if ( rel == PRNE)
            {
                return( pvarDefaultValue->uiVal != pvarUser->uiVal);
            }
            if (rel == PRGT)
            {
                 return( pvarDefaultValue->uiVal > pvarUser->uiVal);
            }
            if (rel == PRGE)
            {
                 return( pvarDefaultValue->uiVal >= pvarUser->uiVal);
            }
            if (rel == PRLT)
            {
                 return( pvarDefaultValue->uiVal < pvarUser->uiVal);
            }
            if (rel == PRLE)
            {
                 return( pvarDefaultValue->uiVal <= pvarUser->uiVal);
            }
            return(FALSE);
            break;

        case VT_UI4:
            if ( rel == PREQ)
            {
                return( pvarDefaultValue->ulVal == pvarUser->ulVal);
            }
            if ( rel == PRNE)
            {
                return( pvarDefaultValue->ulVal != pvarUser->ulVal);
            }
            if (rel == PRGT)
            {
                 return( pvarDefaultValue->ulVal > pvarUser->ulVal);
            }
            if (rel == PRGE)
            {
                 return( pvarDefaultValue->ulVal >= pvarUser->ulVal);
            }
            if (rel == PRLT)
            {
                 return( pvarDefaultValue->ulVal < pvarUser->ulVal);
            }
            if (rel == PRLE)
            {
                 return( pvarDefaultValue->ulVal <= pvarUser->ulVal);
            }
            return(FALSE);
            break;

        case VT_LPWSTR:
            if ( rel == PREQ)
            {
                return ( !wcscmp( pvarDefaultValue->pwszVal, pvarUser->pwszVal));
            }
            if ( rel == PRNE)
            {
                return ( wcscmp( pvarDefaultValue->pwszVal, pvarUser->pwszVal));
            }
            return(FALSE);
            break;

        case VT_BLOB:
            ASSERT( rel == PREQ);
            if ( pvarDefaultValue->blob.cbSize != pvarUser->blob.cbSize)
            {
                return(FALSE);
            }
            return( !memcmp( pvarDefaultValue->blob.pBlobData,
                             pvarUser->blob.pBlobData,
                             pvarUser->blob.cbSize));
            break;

        case VT_CLSID:
            if ( rel == PREQ)
            {
                return( *pvarDefaultValue->puuid == *pvarUser->puuid);
            }
            if ( rel == PRNE)
            {
                 return( *pvarDefaultValue->puuid != *pvarUser->puuid);
            }
            return(FALSE);
            break;


        default:
            ASSERT(0);
            return(FALSE);
            break;

    }
}


static
void
CheckAndReallocateSearchFilterBuffer(
	 AP<WCHAR>& pwszSearchFilter,
	 LPWSTR* ppw,
	 DWORD* pBufferSize,
	 DWORD FilledSize,
	 DWORD RequiredSize
	 )
/*++
Routine Description:
	Check if the reminder of buffer size is enough for the required size.
	If the buffer is not enough, reallocate a new buffer (twice the size of the original buffer + RequiredSize)
	copy the filled buffer data to the new buffer, free the old buffer and update the pointer that points to
	the next location to be filled.

Arguments:
	pwszSearchFilter - Current search filter buffer
	ppw - Pointer to the next buffer location to be filled (can be NULL)
	pBufferSize - pointer to the search filter buffer size.
	FilledSize - the filled buffer size in pBuffer (including the NULL terminating).
	RequiredSize - the required free size in pBuffer.

Returned Value:
	None

--*/
{
	ASSERT(pwszSearchFilter != NULL);
	ASSERT(ppw != NULL);
	ASSERT(pBufferSize != NULL);

	if(RequiredSize <= (*pBufferSize - FilledSize))
		return;

	//
	// Required buffer size is bigger than the remaining buffer size
	// Allocate twice than previous size
	//
	ASSERT(numeric_cast<DWORD>(*ppw - pwszSearchFilter.get() + 1) == FilledSize);
	TrTRACE(DS, "Reallocation buffer: BufferSize = %d, RequiredSize = %d, FilledSize = %d", *pBufferSize, RequiredSize, FilledSize);

	*pBufferSize = *pBufferSize * 2 + RequiredSize;
	AP<WCHAR> TempBuffer = new WCHAR[*pBufferSize];


	//
	// Copy previous buffer
	// FilledSize include the null terminating
	//
    wcsncpy(TempBuffer, pwszSearchFilter, FilledSize);
	pwszSearchFilter.free();
	pwszSearchFilter = TempBuffer.detach();

	//
	// Update the pointer to the new allocated buffer
	//
	*ppw = pwszSearchFilter + FilledSize - 1;
}


HRESULT MQADpRestriction2AdsiFilter(
        IN  const MQRESTRICTION * pMQRestriction,
        IN  LPCWSTR               pwcsObjectCategory,
        IN  LPCWSTR               pwszObjectClass,
        OUT LPWSTR   *            ppwszSearchFilter
        )
/*++
Routine Description:
	Create the ADSI search filter string from MQRESTRICTION.

Arguments:
	pMQRestriction - Restrictions structure
	pwcsObjectCategory - object category string
	pwszObjectClass - object class string
	ppwszSearchFilter - Search filter buffer pointer, this buffer will be allocated and fill by the function 													

Returned Value:
	HRESULT

--*/
{
	ASSERT(pwcsObjectCategory != NULL);
	ASSERT(pwszObjectClass != NULL);
	ASSERT(ppwszSearchFilter != NULL);

	DWORD BufferSize = 1000;
    AP<WCHAR> pwszSearchFilter = new WCHAR[BufferSize];
	DWORD FilledSize = 1;  // Null terminated

    if ((pMQRestriction == NULL) || (pMQRestriction->cRes == 0))
    {
		int n = _snwprintf(
					pwszSearchFilter,
					BufferSize,
					 L"%s%s%s",
					x_ObjectCategoryPrefix,
					pwcsObjectCategory,
					x_ObjectCategorySuffix
					);

		ASSERT(numeric_cast<DWORD>(n) == (x_ObjectCategoryPrefixLen + wcslen(pwcsObjectCategory) + x_ObjectClassSuffixLen));
		DBG_USED(n);

		*ppwszSearchFilter = pwszSearchFilter.detach();
        return MQ_OK;
    }

    LPWSTR pw = pwszSearchFilter;

	DWORD RequiredSize = wcslen(L"(&");
	CheckAndReallocateSearchFilterBuffer(pwszSearchFilter, &pw, &BufferSize, FilledSize, RequiredSize);
    wcscpy(pw, L"(&");
    pw += RequiredSize;
	FilledSize += RequiredSize;

    //
    //  add the object class restriction
    //
	RequiredSize = x_ObjectCategoryPrefixLen + wcslen(pwcsObjectCategory) + x_ObjectClassSuffixLen;
	CheckAndReallocateSearchFilterBuffer(pwszSearchFilter, &pw, &BufferSize, FilledSize, RequiredSize);
	int n = _snwprintf(
				pw,
				BufferSize - FilledSize,
				 L"%s%s%s",
				x_ObjectCategoryPrefix,
				pwcsObjectCategory,
				x_ObjectCategorySuffix
				);

	ASSERT(numeric_cast<DWORD>(n) == RequiredSize);
	pw += RequiredSize;
	FilledSize += RequiredSize;

    //
    //  For queue properties, there is special handling
    //  incase of default values
    //
    BOOL fNeedToCheckDefaultValues = FALSE;
    if (!wcscmp( MSMQ_QUEUE_CLASS_NAME, pwszObjectClass))
    {
        fNeedToCheckDefaultValues = TRUE;
    }

    for (DWORD iRes = 0; iRes < pMQRestriction->cRes; iRes++)
    {

        //
        // Get property info
        //
        const translateProp *pTranslate;
        if(!g_PropDictionary.Lookup(pMQRestriction->paPropRes[iRes].prop, pTranslate))
        {
            ASSERT(0);
			TrERROR(DS, "Failed to find prop %d in the dictionary", pMQRestriction->paPropRes[iRes].prop);
            return LogHR(MQ_ERROR_DS_ERROR, s_FN, 1580);
        }

        AP<WCHAR> pwszVal;

		//
        // Get property value, string representation
		//
        HRESULT hr = MqPropVal2String(
						&pMQRestriction->paPropRes[iRes].prval,
						pTranslate->vtDS,
						&pwszVal
						);

        if (FAILED(hr))
        {
			TrERROR(DS, "MqPropVal2String failed, hr = 0x%x", hr);
            return LogHR(hr, s_FN, 650);
        }

        //
        //  Is the property compared to its default value
        //
        BOOL    fAddPropertyNotPresent = FALSE;
        if ( fNeedToCheckDefaultValues)
        {
            fAddPropertyNotPresent = CompareDefaultValue(
											pMQRestriction->paPropRes[iRes].rel,
											&pMQRestriction->paPropRes[iRes].prval,
											pTranslate->pvarDefaultValue
											);
        }
        DWORD dwBracks = 0;
        if ( fAddPropertyNotPresent)
        {
            //
            //  Add additional restriction that locate all object where
            //  the property is not present.
            //
			RequiredSize = x_AttributeNotIncludedPrefixLen + wcslen(pTranslate->wcsPropid) + x_AttributeNotIncludedSuffixLen;
			CheckAndReallocateSearchFilterBuffer(pwszSearchFilter, &pw, &BufferSize, FilledSize, RequiredSize);
			n = _snwprintf(
					pw,
					BufferSize - FilledSize,
					L"%s%s%s",
					x_AttributeNotIncludedPrefix,
					pTranslate->wcsPropid,
					x_AttributeNotIncludedSuffix
					);

			ASSERT(numeric_cast<DWORD>(n) == RequiredSize);
			pw += RequiredSize;
			FilledSize += RequiredSize;
            dwBracks++;
        }

		//
        // Prefix part
		//
		CheckAndReallocateSearchFilterBuffer(pwszSearchFilter, &pw, &BufferSize, FilledSize, x_PropertyPrefixLen);
        wcscpy(pw, x_PropertyPrefix);
        pw += x_PropertyPrefixLen;
		FilledSize += x_PropertyPrefixLen;

		//
		// Relation part
		//
        WCHAR wszRel[10];
        switch(pMQRestriction->paPropRes[iRes].rel)
        {
        case PRLT:
			RequiredSize = wcslen(L"!(");
			CheckAndReallocateSearchFilterBuffer(pwszSearchFilter, &pw, &BufferSize, FilledSize, RequiredSize);
            wcscpy(pw, L"!(");
            pw += RequiredSize;
			FilledSize += RequiredSize;

            wcscpy(wszRel, L">=");
            dwBracks++;
            break;

        case PRLE:
            wcscpy(wszRel, L"<=");
            break;

        case PRGT:
			RequiredSize = wcslen(L"!(");
			CheckAndReallocateSearchFilterBuffer(pwszSearchFilter, &pw, &BufferSize, FilledSize, RequiredSize);
            wcscpy(pw, L"!(");
            pw += RequiredSize;
			FilledSize += RequiredSize;

            wcscpy(wszRel, L"<=");
            dwBracks++;
            break;

        case PRGE:
            wcscpy(wszRel, L">=");
            break;

        case PREQ:
            wcscpy(wszRel, L"=");
            break;

        case PRNE:
			RequiredSize = wcslen(L"!(");
			CheckAndReallocateSearchFilterBuffer(pwszSearchFilter, &pw, &BufferSize, FilledSize, RequiredSize);
            wcscpy(pw, L"!(");
            pw += RequiredSize;
			FilledSize += RequiredSize;

            wcscpy(wszRel, L"=");
            dwBracks++;
            break;

        default:
            return LogHR(MQ_ERROR_ILLEGAL_RELATION, s_FN, 1590);
        }

		//
        // Property name
		//
		RequiredSize = wcslen(pTranslate->wcsPropid);
		CheckAndReallocateSearchFilterBuffer(pwszSearchFilter, &pw, &BufferSize, FilledSize, RequiredSize);
        wcscpy(pw, pTranslate->wcsPropid);
        pw += RequiredSize;
		FilledSize += RequiredSize;

		//
        // Property condition
		//
		RequiredSize = wcslen(wszRel);
		CheckAndReallocateSearchFilterBuffer(pwszSearchFilter, &pw, &BufferSize, FilledSize, RequiredSize);
        wcscpy(pw, wszRel);
        pw += RequiredSize;
		FilledSize += RequiredSize;

		//
        // Property value
		//
		RequiredSize = wcslen(pwszVal);
		CheckAndReallocateSearchFilterBuffer(pwszSearchFilter, &pw, &BufferSize, FilledSize, RequiredSize);
        wcscpy(pw, pwszVal);
        pw += RequiredSize;
		FilledSize += RequiredSize;

		//
        // Property suffix
		//
        for (DWORD is=0; is < dwBracks; is++)
        {
			CheckAndReallocateSearchFilterBuffer(pwszSearchFilter, &pw, &BufferSize, FilledSize, x_PropertySuffixLen);
            wcscpy(pw, x_PropertySuffix);
            pw += x_PropertySuffixLen;
            FilledSize += x_PropertySuffixLen;
        }

		//
        // Relation closing bracket
		//
		CheckAndReallocateSearchFilterBuffer(pwszSearchFilter, &pw, &BufferSize, FilledSize, x_PropertySuffixLen);
        wcscpy(pw, x_PropertySuffix);
        pw += x_PropertySuffixLen;
        FilledSize += x_PropertySuffixLen;
    }

	CheckAndReallocateSearchFilterBuffer(pwszSearchFilter, &pw, &BufferSize, FilledSize, x_PropertySuffixLen);
    wcscpy(pw, x_PropertySuffix);
    pw += x_PropertySuffixLen;
    FilledSize += x_PropertySuffixLen;

	*ppwszSearchFilter = pwszSearchFilter.detach();
    return MQ_OK;
}


void MQADpCheckAndNotifyOffline(
            IN HRESULT      hr
            )
/*++

Routine Description:
    The routine check if the return code of the last operation
    indicates no connectivity to ActiveDirectory, and if so informs
    the application (if requested to do so)

Arguments:
    HRESULT hr - return code of last operation

Return Value
	none

--*/
{
    //
    //  Have we been requested to inform about offline state
    //
    if (g_pLookDS == NULL)
    {
        return;
    }

    //
    //  Does the return-code of the last operation indicat
    //  an offline state
    //
    if (MQADpIsDSOffline(hr))
    {
        g_pLookDS();
    }

}


CBasicQueryHandle *
MQADpProbQueryHandle(
        IN HANDLE hQuery)
/*++

Routine Description:
    The routine routine verifies the query handle

Arguments:
    HANDLE hQuery

Return Value
	CBasicQueryHandle * or for invalid handles it raise exception.

--*/
{
    CBasicQueryHandle * phQuery = reinterpret_cast<CBasicQueryHandle *>(hQuery);
    //
    // Verify the handle
    //
    __try
    {
        if (phQuery->Verify())
            return phQuery;

    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        //NULL
    }

    RaiseException((DWORD)STATUS_INVALID_HANDLE, 0, 0, 0);
    return NULL;
}

CQueueDeletionNotification *
MQADpProbQueueDeleteNotificationHandle(
        IN HANDLE hQuery)
/*++

Routine Description:
    The routine routine verifies the queue delete notification handle

Arguments:
    HANDLE hQuery

Return Value
	CQueueDeletionNotification * or for invalid handles it raise exception.

--*/
{
    CQueueDeletionNotification * phNotification = reinterpret_cast<CQueueDeletionNotification *>(hQuery);
    //
    // Verify the handle
    //
    __try
    {
        if (phNotification->Verify())
            return phNotification;

    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        //NULL
    }

    RaiseException((DWORD)STATUS_INVALID_HANDLE, 0, 0, 0);
    return NULL;
}


HRESULT
MQADpCheckSortParameter(
    IN const MQSORTSET* pSort)
/*++

Routine Description:
    This routine verifies that the sort parameter doesn't
    contain the same property with conflicting sort-order.
    In MSMQ 1.0  ODBC\SQL returned an error in such case.
    NT5 ignores it.
    This check is added on the server side, in order to
    support old clients.

Arguments:

Return Value:
    MQ_OK - if sort parameter doesn't contain conflicting sort-order of the same property
    MQ_ERROR_ILLEGAL_SORT - otherwise
--*/
{

    if ( pSort == NULL)
    {
        return(MQ_OK);
    }


    const MQSORTKEY * pSortKey = pSort->aCol;
    for ( DWORD i = 0; i < pSort->cCol; i++, pSortKey++)
    {
        const MQSORTKEY * pPreviousSortKey = pSort->aCol;
        for ( DWORD j = 0; j< i; j++, pPreviousSortKey++)
        {
            if ( pPreviousSortKey->propColumn == pSortKey->propColumn)
            {
                //
                //  is it the same sorting order?
                //
                if (pPreviousSortKey->dwOrder !=  pSortKey->dwOrder)
                {
                    return LogHR(MQ_ERROR_ILLEGAL_SORT, s_FN, 420);
                }
            }
        }
    }
    return(MQ_OK);
}

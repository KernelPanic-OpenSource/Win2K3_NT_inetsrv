/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: secd5to4.cpp

Abstract:
    Conversion of NT5 security descriptor to NT4 format.

Author:
    Doron Juster (DoronJ)  24-May-1998

Revision History:

--*/

#include <stdh_sec.h>
#include "acssctrl.h"
#include <mqnames.h>
#include "mqexception.h"

#include "secd5to4.tmh"

static WCHAR *s_FN=L"acssctrl/secd5to4";

//+----------------------------
//
//  void GetpSidAndObj()
//
//+----------------------------

void  
GetpSidAndObj( 
	IN  ACCESS_ALLOWED_ACE*   pAce,
	OUT PSID                 *ppSid,
	OUT BOOL                 *pfObj,
	OUT GUID                **ppguidObj /* = NULL */
	)
/*++
Routine Description:
	Get ACE information: Sid, Obj ACE, Guid Obj ACE.

Arguments:
	pAce - ACE pointer to inquire.
	ppSid - ACE sid.
	pfObj - flag to indicate if this is a OBJECT_ACE_TYPE.
	ppguidObj - Guid of the object ACE.

Returned Value:
	None

--*/
{
	ASSERT(ppSid != NULL);
	ASSERT(pfObj != NULL);
	
	ACCESS_ALLOWED_OBJECT_ACE* pObjAce = (ACCESS_ALLOWED_OBJECT_ACE*) pAce;

    BOOL fObj = ((pAce->Header.AceType == ACCESS_ALLOWED_OBJECT_ACE_TYPE) ||
                 (pAce->Header.AceType == ACCESS_DENIED_OBJECT_ACE_TYPE)  ||
                 (pAce->Header.AceType == SYSTEM_AUDIT_OBJECT_ACE_TYPE));

    *ppSid = NULL;
    *pfObj = fObj;

    if (!fObj)
    {
        *ppSid = (PSID) &(pAce->SidStart);
        return;
    }

    //
	// Object type ACE
	// Get the sid location
	// If both guids present (ACE_INHERITED_OBJECT_TYPE_PRESENT, ACE_OBJECT_TYPE_PRESENT)
	// The sid location is at SidStart.
	// if only one guid is present, sid location is at InheritedObjectType
	// If none of the guids present, sid location is at ObjectType
	//
    if (pObjAce->Flags & ACE_INHERITED_OBJECT_TYPE_PRESENT)
    {
        //
        // "inherited" guid is present.
        //
        if (pObjAce->Flags & ACE_OBJECT_TYPE_PRESENT)
        {
            //
            // Both guids are present.
            // ACE Structure contain spaces for both guids.
            //
            *ppSid = (PSID) &(pObjAce->SidStart);
        }
        else
        {
        	//
        	// This is not a ACE with extended rights. Use this flag in order that its mask will be taken into account.
        	//
        	*pfObj = FALSE;
            //
            // Only one guid presented. structure is shorter.
            //
            *ppSid = (PSID) &(pObjAce->InheritedObjectType);
        }
    }
    else if (pObjAce->Flags & ACE_OBJECT_TYPE_PRESENT)
    {
        //
        // In this case, the structure is shorter, and sid start
        // at the "Inherited" member. April98 MSDN is wrong about
        // this structure.
        //
        *ppSid = (PSID) &(pObjAce->InheritedObjectType);
    }
    else
    {
        //
        // Structure even shorter. no guid is present. 
    	// This is not a ACE with extended rights. Use this flag in order that its mask will be taken into account.
    	//
        //
        *pfObj = FALSE;
        ASSERT(pObjAce->Flags == 0);
        *ppSid = (PSID) &(pObjAce->ObjectType);
    }
    
    if ((pObjAce->Flags & ACE_OBJECT_TYPE_PRESENT) && ppguidObj)
    {
        *ppguidObj = &(pObjAce->ObjectType);
    }
    
}

//+---------------------------------------
//
//  DWORD _MapNt5RightsToNt4Ace()
//
//+---------------------------------------

static 
DWORD 
_MapNt5RightsToNt4Ace( 
	IN DWORD               dwObjectType,
	IN BOOL                fObj,
	IN ACCESS_ALLOWED_ACE* pAce,
	IN DWORD               dwPrevMask 
	)
{
    if (dwPrevMask == g_dwFullControlNT4[dwObjectType])
    {
        //
        // already full control. Ignore.
        //
        return dwPrevMask;
    }

    DWORD dwMask = dwPrevMask;

    if (fObj)
    {
        struct RIGHTSMAP  *psMap = g_psExtendRightsMap5to4[dwObjectType];
        DWORD dwSize =  g_pdwExtendRightsSize5to4[dwObjectType];

        if (psMap)
        {
            BOOL fFound = FALSE;
            ACCESS_ALLOWED_OBJECT_ACE* pObjAce = (ACCESS_ALLOWED_OBJECT_ACE*) pAce;
            GUID *pGuid = &(pObjAce->ObjectType);

            if ((pObjAce->Mask & MSMQ_EXTENDED_RIGHT_MASK) == MSMQ_EXTENDED_RIGHT_MASK)
            {
                //
                // This is an extended right object ace. It may be relevant.
                //
                ASSERT(pObjAce->Flags & ACE_OBJECT_TYPE_PRESENT);

                for (DWORD j = 0; j < dwSize; j++)
                {
                    if (memcmp( 
							pGuid,
							&(psMap[j].guidRight),
							sizeof(GUID)
							) == 0)
                    {
                        dwMask |= psMap[j].dwPermission5to4;
                        fFound = TRUE;
                        break;
                    }
                }
            }

            if (!fFound                          &&
                 (dwObjectType == MQDS_MACHINE)  &&
                 (memcmp(pGuid, &g_guidCreateQueue, sizeof(GUID)) == 0))
            {
                //
                // Handle the special case where the DACL doesn't have the
                // "CreateAllChild" bit turned on, but rather it has
                // "CreatemSMQQueue" guid present.
                //
                if (pObjAce->Mask & RIGHT_DS_CREATE_CHILD)
                {
                    dwMask |= MQSEC_CREATE_QUEUE;
                }
            }
        }
        else
        {
            //
            // Only queue and machine objects can have extended rights.
            //
            ASSERT((dwObjectType != MQDS_QUEUE) && (dwObjectType != MQDS_MACHINE));
        }
    }
    else
    {
        //
        // Map the DS specific rights to MSMQ specific rights
        //
        DWORD dwDSRights = pAce->Mask;
        ASSERT(dwDSRights != 0);

        if (dwDSRights == GENERIC_ALL_MAPPING)
        {
            //
            // Full control.
            // Don't look for other allow aces for this sid.
            //
            return  g_dwFullControlNT4[dwObjectType];
        }

        DWORD  *pdwMap = g_padwRightsMap5to4[dwObjectType];
        ASSERT(pdwMap);

        for (DWORD j = 0; j < NUMOF_ADS_SPECIFIC_RIGHTS; j++)
        {
            DWORD dwRight =  dwDSRights & 0x01;
            if (dwRight)
            {
                dwMask |= pdwMap[j];
            }
            dwDSRights >>= 1;
        }

        //
        // Copy the standard rights.
        //
        dwDSRights = pAce->Mask;
        DWORD dwStandard = dwDSRights & STANDARD_RIGHTS_ALL;
        dwMask |= dwStandard;

        if (dwObjectType == MQDS_SITE)
        {
            //
            // the single NT5 DS right CreateChild is mapped to three
            // MSMQ1.0 rights: createFRS, createBSC and createMachine.
            // The translation table contain only the createMachine right.
            // Add all the others.
            //
            if (dwMask & MQSEC_CREATE_MACHINE)
            {
                dwMask |= (MQSEC_CREATE_FRS | MQSEC_CREATE_BSC | MQSEC_CREATE_MACHINE);
            }
        }
    }

    return dwMask;
}

BOOL  
IsNewSid( 
	PSID pSid,
	SID** ppSids,
	DWORD* pdwNumofSids 
	)
{
    for (DWORD j = 0; j < *pdwNumofSids; j++)
    {
        PSID pSidOld = ppSids[j];

        if (EqualSid(pSid, pSidOld))
        {
			//
			// ACE sid already exist in the Sids list.
			//
            return FALSE;
        }
    }

    //
    // New sid, add it to the Sids list
    //
    DWORD dwSize = GetLengthSid(pSid);
    ASSERT(dwSize) ;

	AP<SID> NewSid = (SID*) new BYTE[dwSize];
    if (!CopySid(dwSize, NewSid, pSid))
    {
		//
		// We failed to copy the sid - just don't add it. 
		//
		DWORD gle = GetLastError();
		TrERROR(SECURITY, "CopySid failed. Error: %!winerr!", gle);
		ASSERT(("CopySid failed", gle == ERROR_SUCCESS));
		return TRUE;
    }

    DWORD dwIndex = *pdwNumofSids;
    ppSids[dwIndex] = NewSid.detach();
    *pdwNumofSids = dwIndex + 1;

    return TRUE;
}


//+----------------------------
//
//  BOOL  _IsNewNt5Sid()
//
//+----------------------------

static 
BOOL  
_IsNewNt5Sid( 
	ACCESS_ALLOWED_ACE*   pAce,
	SID                 **ppSids,
	DWORD                *pdwNumofSids 
	)
{
    BOOL fObj;
    PSID pSid = NULL;

    GetpSidAndObj( 
		pAce,
		&pSid,
		&fObj 
		);

    ASSERT(pSid && IsValidSid(pSid));

	return IsNewSid(pSid, ppSids, pdwNumofSids);
}

//+-------------------------------
//
//  void _SetNt4AuditMasks()
//
//+-------------------------------

inline 
static 
void 
_SetNt4AuditMasks( 
	IN  SYSTEM_AUDIT_ACE *pAce,
	IN  DWORD             dwObjectType,
	IN  BOOL              fObj,
	OUT ACCESS_MASK      *pdwFail,
	OUT ACCESS_MASK      *pdwSuccess 
	)
{
    BYTE bFlags = pAce->Header.AceFlags;

    if (bFlags & FAILED_ACCESS_ACE_FLAG)
    {
        *pdwFail = _MapNt5RightsToNt4Ace(  
						dwObjectType,
						fObj,
						(ACCESS_ALLOWED_ACE*) pAce,
						*pdwFail 
						);
    }

    if (bFlags & SUCCESSFUL_ACCESS_ACE_FLAG)
    {
        *pdwSuccess = _MapNt5RightsToNt4Ace(  
							dwObjectType,
							fObj,
							(ACCESS_ALLOWED_ACE*) pAce,
							*pdwSuccess 
							);
    }
}

//+---------------------------------------------
//
//  static HRESULT _ConvertSaclToNT4Format()
//
//+---------------------------------------------

static 
HRESULT 
_ConvertSaclToNT4Format( 
	IN  DWORD   dwObjectType,
	IN  PACL    pAcl5,
	OUT DWORD  *pdwAcl4Len,
	OUT PACL   *ppAcl4 
	)
{
    ASSERT(IsValidAcl(pAcl5));

    DWORD dwAclSize = (DWORD) pAcl5->AclSize;
	DWORD dwNumberOfACEs = (DWORD) pAcl5->AceCount;
	if (dwNumberOfACEs == 0)
    {
        //
        // SACL without ACEs. nothing to convert.
        //
        *pdwAcl4Len = 0;
        *ppAcl4 = NULL;
        return MQSec_OK;
    }

    *pdwAcl4Len = dwAclSize;
    AP<ACL> TempAcl = (PACL) new BYTE[*pdwAcl4Len];
    *ppAcl4 = TempAcl;
    if (!InitializeAcl(*ppAcl4, *pdwAcl4Len, ACL_REVISION))
    {
		DWORD gle = GetLastError();
		TrERROR(SECURITY, "InitializeAcl failed. Error: %!winerr!", gle);
		ASSERT(("InitializeAcl failed", gle == ERROR_SUCCESS));
		return HRESULT_FROM_WIN32(gle);
    }

    //
    // First, group aces by SID, then by audit type (fail or success).
    //
    // We build an array of SIDs that we handled up to now. We combine
    // aces of same sid into one ace. This operation does not change the
    // semantic of the acl, just make it more efficient.
    //
    SID  **ppSids5 = (SID**) new PSID[dwNumberOfACEs];
    aPtrs<SID> apSids(ppSids5, dwNumberOfACEs);
    DWORD dwNumSids = 0;

    DWORD i = 0;

    do
    {
	    SYSTEM_AUDIT_ACE *pAce;
        if (!GetAce(pAcl5, i, (LPVOID* )&(pAce)))
        {
            TrERROR(SECURITY, "Failed to get ACE (index=%lu) while converting SACL from NT5 form. %!winerr!", i, GetLastError());
            return MQSec_E_SDCONVERT_GETACE ;
        }

        if (!_IsNewNt5Sid( 
				(ACCESS_ALLOWED_ACE*) pAce,
				ppSids5,
				&dwNumSids 
				))
        {
            i++;
            continue;
        }

        //
        // First ace in the group. Now look for contigous aces with same sid.
        //
        BOOL fObj;
        PSID pSid = NULL;

        GetpSidAndObj( 
			(ACCESS_ALLOWED_ACE*) pAce,
			&pSid,
			&fObj 
			);

        ASSERT(pSid && IsValidSid(pSid));

        ACCESS_MASK  dwMaskFail = 0;
        ACCESS_MASK  dwMaskSuccess = 0;
        _SetNt4AuditMasks( 
			pAce,
			dwObjectType,
			fObj,
			&dwMaskFail,
			&dwMaskSuccess 
			);

        i++;
        DWORD j = i;

        while (j < dwNumberOfACEs)
        {
            if (!GetAce(pAcl5, j, (LPVOID* )&(pAce)))
            {
                TrERROR(SECURITY, "Failed to get ACE (index=%lu) while looking for next ACE in NT4 format. %!winerr!", i, GetLastError());
                return MQSec_E_SDCONVERT_GETACE ;
            }

            PSID pPresentSid = NULL;

            GetpSidAndObj( 
				(ACCESS_ALLOWED_ACE*) pAce,
				&pPresentSid,
				&fObj 
				);

            ASSERT(pPresentSid && IsValidSid(pPresentSid));

            if (EqualSid(pSid, pPresentSid))
            {
                _SetNt4AuditMasks( 
					pAce,
					dwObjectType,
					fObj,
					&dwMaskFail,
					&dwMaskSuccess 
					);
            }
            j++;
        }

        //
        // Now we have a group of aces of same SID, and we have the masks.
        //
        if (dwMaskSuccess != 0)
        {
            if (dwMaskSuccess != 0)
            {
                if (!AddAuditAccessAce( 
						*ppAcl4,
						ACL_REVISION,
						dwMaskSuccess,
						pSid,
						TRUE,     // success
						(dwMaskFail == dwMaskSuccess) 
						))
            	{
            		DWORD gle = GetLastError();
            		TrERROR(SECURITY, "AddAuditAccessAce failed. Error: %!winerr!", gle);
            		ASSERT(("AddAuditAccessAce failed", gle == ERROR_SUCCESS));
            		return HRESULT_FROM_WIN32(gle);
            	}

            }
        }

        if ((dwMaskFail != 0) && (dwMaskFail != dwMaskSuccess))
        {
            if (!AddAuditAccessAce( 
					*ppAcl4,
					ACL_REVISION,
					dwMaskFail,
					pSid,
					FALSE,     // success
					TRUE		// failure
					))
            {
            		DWORD gle = GetLastError();
            		TrERROR(SECURITY, "AddAuditAccessAce failed. Error: %!winerr!", gle);
            		ASSERT(("AddAuditAccessAce failed", gle == ERROR_SUCCESS));
            		return HRESULT_FROM_WIN32(gle);
            }
        }
    }
    while (i < dwNumberOfACEs);

	TempAcl.detach();
	
    return MQSec_OK;
}

//+---------------------------------------
//
//  HRESULT _ConvertGroupOfNt5DaclAces()
//
//  convert a group of ACEs, all of them having same type.
//
//+---------------------------------------

static 
HRESULT 
_ConvertGroupOfNt5DaclAces( 
	DWORD  dwObjectType,
	PACL   pInAcl5,
	DWORD  iFirst,
	DWORD  iLast,
	BOOL   fAllow,
	PACL   pOutAcl4 
	)
{
	DWORD   dwNumberOfACEs = iLast - iFirst + 1;
    DWORD   i = iFirst;

    //
    // We build an array of SIDs that we handled up to now. We combine
    // aces of same sid into one ace. This operation does not change the
    // semantic of the acl, just make it more efficient.
    //
    SID  **ppSids5 = (SID**) new PSID[dwNumberOfACEs];
    aPtrs<SID> apSids(ppSids5, dwNumberOfACEs);
    DWORD    dwNumSids = 0;

    do
    {
	    ACCESS_ALLOWED_ACE *pAce;
        if (!GetAce(pInAcl5, i, (LPVOID* )&(pAce)))
        {
            TrERROR(SECURITY, "Failed to get ACE (index=%lu) while combining same SID NT5 ACEs. %!winerr!", i, GetLastError());
            return MQSec_E_SDCONVERT_GETACE ;
        }

        if (!_IsNewNt5Sid( 
				pAce,
				ppSids5,
				&dwNumSids 
				))
        {
            i++;
            continue;
        }

        //
        // this ace start a group of aces for a given sid.
        // on MSMQ1.0 we don't support inheritance of ace, so just OR the
        // masks of all  aces of this sid and create a NT5 ace.
        //
        PSID pSid = NULL;
        BOOL fObj = FALSE;
        GetpSidAndObj( 
			pAce,
			&pSid,
			&fObj 
			);

        ASSERT(pSid && IsValidSid(pSid));

        DWORD dwMSMQRights = _MapNt5RightsToNt4Ace( 
									dwObjectType,
									fObj,
									pAce,
									0 
									);
        i++;
        DWORD j = i;

        while (j <= iLast)
        {
            if (!GetAce(pInAcl5, j, (LPVOID* )&(pAce)))
            {
                TrERROR(SECURITY, "Failed to get ACE (index=%lu) while looking for next NT5 ACE in a group. %!winerr!", i, GetLastError());
                return MQSec_E_SDCONVERT_GETACE ;
            }

            PSID pPresentSid = NULL;
            GetpSidAndObj( 
				pAce,
				&pPresentSid,
				&fObj 
				);

            ASSERT(pPresentSid && IsValidSid(pPresentSid));

            if (EqualSid(pSid, pPresentSid))
            {
                dwMSMQRights = _MapNt5RightsToNt4Ace( 
									dwObjectType,
									fObj,
									pAce,
									dwMSMQRights 
									);
            }
            j++;
        }

        BOOL f = FALSE;
        if (dwMSMQRights == 0)
        {
            //
            // Ignore. this is probably the result of access bits or
            // extended rights which are not supported on MSMQ1.0
            // don't add ACE with null access mask. It's meaningless
            // in MSMQ1.0
            //
            f = TRUE;
        }
        else if (fAllow)
        {
            if (!AddAccessAllowedAce( 
					pOutAcl4,
					ACL_REVISION,
					dwMSMQRights,
					pSid 
					))
            {
        		DWORD gle = GetLastError();
        		TrERROR(SECURITY, "AddAuditAccessAce failed. Error: %!winerr!", gle);
        		ASSERT(("AddAuditAccessAce failed", gle == ERROR_SUCCESS));
        		return HRESULT_FROM_WIN32(gle);	
            }
        }
        else
        {
            if (!AddAccessDeniedAceEx( 
					pOutAcl4,
					ACL_REVISION,
					0,
					dwMSMQRights,
					pSid 
					))
			{
        		DWORD gle = GetLastError();
        		TrERROR(SECURITY, "AddAuditAccessAce failed. Error: %!winerr!", gle);
        		ASSERT(("AddAccessDeniedAceEx failed", gle == ERROR_SUCCESS));
        		return HRESULT_FROM_WIN32(gle);	
            }
        }
    }
    while (i <= iLast);

    return MQSec_OK;
}

//+---------------------------------------------
//
//  static HRESULT _ConvertDaclToNT4Format()
//
//+---------------------------------------------

static 
HRESULT 
_ConvertDaclToNT4Format( 
	IN  DWORD   dwObjectType,
	IN  PACL    pAcl5,
	OUT DWORD  *pdwAcl4Len,
	OUT PACL   *ppAcl4 
	)
{
    ASSERT(IsValidAcl(pAcl5));
    HRESULT hr = MQSec_OK;

    DWORD dwAclSize = (DWORD) pAcl5->AclSize;
	DWORD dwNumberOfACEs = (DWORD) pAcl5->AceCount;

    //
    // See if we're running in the context of the replication service.
    // If so, then add an ALLOW_ACE for the enterprise object and let
    // everyone register certificate. This is a workaround for bug 5054,
    // to allow users in NT4 sites to register certifiates after PEc
    // migrated to Win2k. Otherwise, the NT4 MQIS servers will reject such
    // request and won't forward write-requests to the PEC.
    // Reason for the bug- we don't have a "register certificate" bit in the
    // security descriptor of the msmqService object, so there is nothing
    // we can translate to NT4 format. Anyway, mixed-mode msmq must relax
    // security, so this is just another aspect of this relaxation. There is
    // no security hole here, as MQIS servers do perform all necessary
    // validations of the certificate before issuing the write request.
    // Even in NT4, the "create user" bit was quite meaningless.
    //
    static BOOL s_fInReplService = FALSE;
    static BOOL s_fCheckReplService = TRUE;

    if (s_fCheckReplService)
    {
        HMODULE  hMq = GetModuleHandle(MQ1REPL_DLL_NAME);
        if (hMq)
        {
            s_fInReplService = TRUE;
        }
        s_fCheckReplService = FALSE;
    }

    if (s_fInReplService && (dwObjectType == MQDS_ENTERPRISE))
    {
        dwAclSize += (sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(MQSec_GetWorldSid()));
    }

    //
    // The size of NT4 format can't be larger than the size of NT5.
    // In worst case, when all ACEs are added to NT4 ACL, NT5
    // OBJECT_ACEs are converted to NT4 ACEs which are smaller.
    // So allocating same size is safe, although may waste some memory.
    // The alternative is to scan the NT5 acl before starting conversion
    // and calculating the required size.
    //
    *pdwAcl4Len = dwAclSize;
    AP<ACL> TempAcl = (PACL) new BYTE[*pdwAcl4Len];
    *ppAcl4 = TempAcl;
    if (!InitializeAcl(*ppAcl4, *pdwAcl4Len, ACL_REVISION))
    {
		DWORD gle = GetLastError();
		TrERROR(SECURITY, "InitializeAcl failed. Error: %!winerr!", gle);
		ASSERT(("InitializeAcl failed", gle == ERROR_SUCCESS));
		return HRESULT_FROM_WIN32(gle);	
   	}

    //
    // We're not assuming that the input acl is canonical. So we'll handle
    // groups of identical aces (i.e., aces of same type) at a time. Each
    // group will be converted to canonical NT5 format. So if input is
    // canonical, output will be canonical too.
    //
    DWORD i = 0;

    do
    {
        //
        // first ace in the group.
        //
	    ACCESS_ALLOWED_ACE *pAce;
        if (!GetAce(pAcl5, i, (LPVOID* )&(pAce)))
        {
            TrERROR(SECURITY, "Failed to get ACE (index=%lu) while looking for first NT5 ACE to convert. %!winerr!", i, GetLastError());
            return MQSec_E_SDCONVERT_GETACE ;
        }

        //
        // First verify it's an effective ACE. We don't take into account
        // INHRIT_ONLY aces. See ACE_HEADER documentation in MSDN.
        //
        if (pAce->Header.AceFlags & INHERIT_ONLY_ACE)
        {
            i++;
            continue;
        }

        DWORD iFirst = i;
        DWORD iLast = i;
        BOOL fAllow =
                ((pAce->Header.AceType == ACCESS_ALLOWED_ACE_TYPE) ||
                 (pAce->Header.AceType == ACCESS_ALLOWED_OBJECT_ACE_TYPE));
        i++;

        //
        // now look for other aces with same type.
        //
        while (i < dwNumberOfACEs)
        {
            if (!GetAce(pAcl5, i, (LPVOID* )&(pAce)))
            {
                TrERROR(SECURITY, "Failed to get ACE (index=%lu) while looking for other NT5 ACEs with same type. %!winerr!", i, GetLastError());
                return MQSec_E_SDCONVERT_GETACE ;
            }

            if (pAce->Header.AceFlags & INHERIT_ONLY_ACE)
            {
                //
                // This ACE is not an effective one for this object.
                // End of present group.
                //
                break;
            }

            BOOL fPresentAllow =
                ((pAce->Header.AceType == ACCESS_ALLOWED_ACE_TYPE) ||
                 (pAce->Header.AceType == ACCESS_ALLOWED_OBJECT_ACE_TYPE));

            if (fAllow == fPresentAllow)
            {
                iLast = i;
                i++;
            }
            else
            {
                break;
            }
        }

        //
        // Handle all aces from iFirst to iLast.
        //
        hr = _ConvertGroupOfNt5DaclAces( 
					dwObjectType,
					pAcl5,
					iFirst,
					iLast,
					fAllow,
					*ppAcl4 
					);
    }
    while (i < dwNumberOfACEs);

    if (s_fInReplService && (dwObjectType == MQDS_ENTERPRISE))
    {
        if (!AddAccessAllowedAce( 
				*ppAcl4,
				ACL_REVISION,
				MQSEC_CREATE_USER,
				MQSec_GetWorldSid() 
				))
        	{
				DWORD gle = GetLastError();
				TrERROR(SECURITY, "AddAccessAllowedAce failed. Error: %!winerr!", gle);
				ASSERT(("AddAccessAllowedAce failed", gle == ERROR_SUCCESS));
				return HRESULT_FROM_WIN32(gle);	
        	}

    }
    
	TempAcl.detach();
	
    return MQSec_OK;
}

//+-----------------------------------------------------------------
//
//  BOOL _AlreadyNT4Format()
//
//  check if security descriptor is already in NT4 format. We're
//  checking the dacl revision. if it's ACL_REVISION then it's NT4.
//  NT5 DS must use the ACL_REVISION_DS.
//
//+-----------------------------------------------------------------

static BOOL _AlreadyNT4Format(IN  SECURITY_DESCRIPTOR  *pSD5)
{
    BOOL  bPresent = FALSE;
    BOOL  bDefaulted;
    PACL  pAcl5;

    if(!GetSecurityDescriptorDacl( 
					pSD5,
					&bPresent,
					&pAcl5,
					&bDefaulted 
					))
	{
		DWORD gle = GetLastError();
		TrERROR(SECURITY, "GetSecurityDescriptorDacl() failed, gle = 0x%x", gle);
		throw bad_win32_error(gle);
	}

    if (!bPresent)
    {
        //
        // dacl not present. Try the sacl.
        //
        if(!GetSecurityDescriptorSacl( 
					pSD5,
					&bPresent,
					&pAcl5,
					&bDefaulted 
					))
		{
			DWORD gle = GetLastError();
			TrERROR(SECURITY, "GetSecurityDescriptorSacl() failed, gle = 0x%x", gle);
			throw bad_win32_error(gle);
		}

    }

    if (bPresent && (pAcl5 != NULL))
    {
        //
        // If the input is indeed in nt4 format, then it's legal to
        // have a "present" DACL that is NULL. So if acl is not null, we'll
        // check the version, but if it's null, then it's already in
        // nt4 format.
        //
	    return (pAcl5->AclRevision == ACL_REVISION);
    }

    //
    // If both dacl and sacl are not present, or are present as NULL, then
    // consider the SD as being in NT4 format and don't do anything with it.
    //
    return TRUE;
}

//+-----------------------------------------------------------------
//
//  HRESULT MQSec_ConvertSDToNT4Format()
//
//  Description: Convert a security descriptor from NT5 format to NT4
//      compatiblae format.
//
//+-----------------------------------------------------------------

HRESULT 
APIENTRY  
MQSec_ConvertSDToNT4Format(
	IN  DWORD                 dwObjectType,
	IN  SECURITY_DESCRIPTOR  *pSD5,
	OUT DWORD                *pdwSD4Len,
	OUT SECURITY_DESCRIPTOR **ppSD4,
	IN  SECURITY_INFORMATION  sInfo 
	)
{
    if (!pSD5 || !ppSD4)
    {
        ASSERT(0);
        return LogHR(MQSec_E_NULL_SD, s_FN, 80);
    }

    HRESULT hr = MQSec_OK;
    *ppSD4 = NULL;

    if (!IsValidSecurityDescriptor(pSD5))
    {
    	DWORD gle = GetLastError();
        TrERROR(SECURITY, "Can't convert an invalid NT5 Security Descriptor. Error: %!winerr!", gle);
        ASSERT(0);
        return MQSec_E_SD_NOT_VALID;
    }

    //
    // Make sure input descriptor is self-relative.
    //
    DWORD dwRevision = 0;
    SECURITY_DESCRIPTOR_CONTROL sdC;

	if (!GetSecurityDescriptorControl(pSD5, &sdC, &dwRevision))
	{
		DWORD gle = GetLastError();
		TrERROR(SECURITY, "GetSecurityDescriptorControl failed. Error: %!winerr!", gle);
		ASSERT(("GetSecurityDescriptorControl failed", gle == ERROR_SUCCESS));
		return HRESULT_FROM_WIN32(gle);
	}

    try
	{
		
	
		if (!(sdC & SE_SELF_RELATIVE))
		{
			TrERROR(SECURITY, "Can't convert a non self-relative NT5 Security Descriptor");
			return MQSec_E_NOT_SELF_RELATIVE;
		}
		else if (_AlreadyNT4Format(pSD5))
		{
			return LogHR(MQSec_I_SD_CONV_NOT_NEEDED, s_FN, 110);
		}
	}
	catch(bad_win32_error& exp)
	{
		TrERROR(SECURITY, "catch bad_win32_error exception, error = 0x%x", exp.error());
		return LogHR(HRESULT_FROM_WIN32(exp.error()), s_FN, 115);
	}


    if ((dwObjectType != MQDS_QUEUE)   &&
        (dwObjectType != MQDS_MACHINE) &&
        (dwObjectType != MQDS_CN)      &&
        (dwObjectType != MQDS_SITE)    &&
        (dwObjectType != MQDS_ENTERPRISE))
    {
        //
        // BUGBUG Temporary.
        //
        return LogHR(MQSec_I_SD_CONV_NOT_NEEDED, s_FN, 120);
    }

    SECURITY_DESCRIPTOR sd;

    if (!InitializeSecurityDescriptor( 
					&sd,
					SECURITY_DESCRIPTOR_REVISION 
					))
    {
        TrERROR(SECURITY, "Can't initialize security descriptor while converting from NT5 to NT4 format. %!winerr!", GetLastError());
        return MQSec_E_INIT_SD;
    }

    //
    // Handle owner.
    //
    PSID pOwner = NULL;
    if (sInfo & OWNER_SECURITY_INFORMATION)
    {
        BOOL bOwnerDefaulted = FALSE;

        if (!GetSecurityDescriptorOwner( 
					pSD5,
					&pOwner,
					&bOwnerDefaulted 
					))
        {
			DWORD gle = GetLastError();
			TrERROR(SECURITY, "GetSecurityDescriptorOwner failed. Error: %!winerr!", gle);
			ASSERT(("GetSecurityDescriptorOwner failed", gle == ERROR_SUCCESS));
			return HRESULT_FROM_WIN32(gle);
        }
        
        ASSERT(pOwner != NULL);
        ASSERT(IsValidSid(pOwner));

        //
        // BUGBUG
        // If this is a local user, set the owner to be the anonymous
        // logon user.
        //
        if (!SetSecurityDescriptorOwner(&sd, pOwner, bOwnerDefaulted))
        {
			DWORD gle = GetLastError();
			TrERROR(SECURITY, "SetSecurityDescriptorOwner failed. Error: %!winerr!", gle);
			ASSERT(("SetSecurityDescriptorOwner failed", gle == ERROR_SUCCESS));
			return HRESULT_FROM_WIN32(gle);
        }
    }

    //
    // Handle group
    //
    PSID pGroup = NULL;
    if (sInfo & GROUP_SECURITY_INFORMATION)
    {
        BOOL bGroupDefaulted = FALSE;

        if (!GetSecurityDescriptorGroup( 
					pSD5,
					&pGroup,
					&bGroupDefaulted 
					))
        {
			DWORD gle = GetLastError();
			TrERROR(SECURITY, "GetSecurityDescriptorGroup failed. Error: %!winerr!", gle);
			ASSERT(("GetSecurityDescriptorGroup failed", gle == ERROR_SUCCESS));
			return HRESULT_FROM_WIN32(gle);
        }

        ASSERT(pGroup != NULL);
        ASSERT(IsValidSid(pGroup));

        if (!SetSecurityDescriptorGroup(&sd, pGroup, bGroupDefaulted))
        {
			DWORD gle = GetLastError(); 
			TrERROR(SECURITY, "SetSecurityDescriptorGroup failed. Error: %!winerr!", gle);
			ASSERT(("SetSecurityDescriptorGroup failed", gle == ERROR_SUCCESS));
			return HRESULT_FROM_WIN32(gle);
        }
        
    }

    //
    // Handle SACL
    //
    BOOL   bPresent = FALSE;
    BOOL   bDefaulted = FALSE;
    PACL   pAcl5;
    DWORD  dwAclLen;
    P<ACL> pSacl4 = NULL;

    if (sInfo & SACL_SECURITY_INFORMATION)
    {
        if (!GetSecurityDescriptorSacl( 
					pSD5,
					&bPresent,
					&pAcl5,
					&bDefaulted 
					))
        {
			DWORD gle = GetLastError();
			TrERROR(SECURITY, "GetSecurityDescriptorSacl failed. Error: %!winerr!", gle);
			ASSERT(("GetSecurityDescriptorSacl failed", gle == ERROR_SUCCESS));
			return HRESULT_FROM_WIN32(gle);
        }

        hr = MQSec_OK;
        if (bPresent)
        {
            if (pAcl5)
            {
                //
                // It's legal to have NULL Sacl. No need to convert it.
                //
                hr = _ConvertSaclToNT4Format( 
							dwObjectType,
							pAcl5,
							&dwAclLen,
							&pSacl4 
							);

                if (FAILED(hr))
                {
                    return LogHR(hr, s_FN, 140);
                }
                if (pSacl4)
                {
                    ASSERT(IsValidAcl(pSacl4));
                }
                else
                {
                    //
                    // that's OK. sometimes there is a SACL wihtout any
                    // ACEs. let's convert it to no-SACL.
                    //
                    ASSERT(dwAclLen == 0);
                    bPresent = FALSE;
                }
            }
            else
            {
                //
                // that's kind of bug in NT. Flag is set but sacl is null.
                // trying to set this SecurityDescriptor with this value will
                // fail, with error 0x13 from ldap.
                // Reset the present flag.
                //
                bPresent = FALSE;
            }
        }
        if (!SetSecurityDescriptorSacl( 
					&sd,
					bPresent,
					pSacl4,
					bDefaulted
					))
        {
			DWORD gle = GetLastError();
			TrERROR(SECURITY, "SetSecurityDescriptorSacl failed. Error: %!winerr!", gle);
			ASSERT(("SetSecurityDescriptorSacl failed", gle == ERROR_SUCCESS));
			return HRESULT_FROM_WIN32(gle);
        }
    }

    //
    // Handle DACL
    //
    //
    // BUGBUG. Workaround for beta2. Send to NT4 a SD with NUL DACL,
    // which mean full control to everyone.
    //
    P<ACL> pDacl4 = NULL;
    if (sInfo & DACL_SECURITY_INFORMATION)
    {
        if (!GetSecurityDescriptorDacl( 
					pSD5,
					&bPresent,
					&pAcl5,
					&bDefaulted 
					))
        {
			DWORD gle = GetLastError();
			TrERROR(SECURITY, "GetSecurityDescriptorDacl failed. Error: %!winerr!", gle);
			ASSERT(("GetSecurityDescriptorDacl failed", gle == ERROR_SUCCESS));
			return HRESULT_FROM_WIN32(gle);
        }

        hr = MQSec_OK;
        if (bPresent)
        {
            hr = _ConvertDaclToNT4Format( 
						dwObjectType,
						pAcl5,
						&dwAclLen,
						&pDacl4 
						);
            if (FAILED(hr))
            {
                return LogHR(hr, s_FN, 150);
            }
            ASSERT(IsValidAcl(pDacl4));
        }
        if (!SetSecurityDescriptorDacl( 
					&sd,
					bPresent,
					pDacl4,
					bDefaulted
					))
        {
			DWORD gle = GetLastError();
			TrERROR(SECURITY, "SetSecurityDescriptorDacl failed. Error: %!winerr!", gle);
			ASSERT(("SetSecurityDescriptorDacl failed", gle == ERROR_SUCCESS));
			return HRESULT_FROM_WIN32(gle);
        }
    }

    //
    // Convert the descriptor to a self relative format.
    //
    DWORD dwLen = 0;
    MakeSelfRelativeSD(&sd, NULL, &dwLen);
    DWORD gle = GetLastError();
    if (gle != ERROR_INSUFFICIENT_BUFFER)
    {
		TrERROR(SECURITY, "MakeSelfRelativeSD failed. Error: %!winerr!", gle);
		return HRESULT_FROM_WIN32(gle);
    }
    
	AP<SECURITY_DESCRIPTOR> TempSD = (SECURITY_DESCRIPTOR *) new char[dwLen];
    if (!MakeSelfRelativeSD(&sd, TempSD, &dwLen))
    {
		gle = GetLastError();
		TrERROR(SECURITY, "MakeSelfRelativeSD failed. Error: %!winerr!", gle);
		ASSERT(("MakeSelfRelativeSD failed", gle == ERROR_SUCCESS));
		return HRESULT_FROM_WIN32(gle);
    }

    *pdwSD4Len = dwLen;

    *ppSD4 = TempSD.detach();

    return MQSec_OK;
}


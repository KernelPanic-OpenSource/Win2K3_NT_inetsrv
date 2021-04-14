/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    DumpAuthzUtl.cpp

Abstract:

    Dump authz related information utilities.

Author:

    Ilan Herbst (ilanh) 14-Apr-2001

--*/

#include "stdh.h"
#include "Authz.h"
#include "sddl.h"
#include "cm.h"
#include "tr.h"
#include "mqexception.h"
#include "autoreln.h"
#include "mqsec.h"
#include "DumpAuthzUtl.h"

#include "DumpAuthzUtl.tmh"

static WCHAR *s_FN=L"DumpAuthzUtl";

static
bool 
DumpAccessCheckFailure()
/*++

Routine Description:
    Read DumpAccessCheckFailure flag from registry

Arguments:
	None

Return Value:
	true if DumpAccessCheckFailure is set.
--*/
{
	//
	// Reading this registry only at first time.
	//
	static bool s_fInitialized = false;
	static bool s_fDumpAccessCheck = false;

	if(s_fInitialized)
	{
		return s_fDumpAccessCheck;
	}

	const RegEntry xRegEntry(TEXT("security"), TEXT("DumpAccessCheckFailure"), 0);
	DWORD DumpAccessCheckValue;
	CmQueryValue(xRegEntry, &DumpAccessCheckValue);

	s_fDumpAccessCheck = (DumpAccessCheckValue != 0);
	s_fInitialized = true;

	TrTRACE(SECURITY, "DumpAccessCheckFailure value = %d", DumpAccessCheckValue);

	return s_fDumpAccessCheck;
}


#define GET_PSID_FROM_PACE(pAce) (&((PACCESS_ALLOWED_ACE)pAce)->SidStart)
#define GET_ACE_MASK(pAce) (((PACCESS_DENIED_ACE)pAce)->Mask)
#define GET_ACE_TYPE(pAce) (((PACCESS_DENIED_ACE)pAce)->Header.AceType)


void 
IsPermissionGranted(
	PSECURITY_DESCRIPTOR pSD,
	DWORD Permission,
	bool* pfAllGranted, 
	bool* pfEveryoneGranted, 
	bool* pfAnonymousGranted 
	)
/*++

Routine Description:
	Check if we allow all (everyone + anonymous) a permission in
	a security descriptor.
	if there is no deny ace, also return if Everyone or Anonymous are
	explicitly grant the permissions.

Arguments:
	pSD - pointer to the security descriptor 
	Permission - requested permission.
	pfAllGranted - [out] flag that indicate if all grant the permissions.
	pfEveryoneGranted - [out] flag that indicate if Everyone explicitly grant the permissions.
	pfAnonymousGranted - [out] flag that indicate if Anonymous explicitly grant the permissions.

Returned Value:
	true if we allow all the permission, false otherwise
	
--*/
{
	ASSERT((pSD != NULL) && IsValidSecurityDescriptor(pSD));

	*pfAllGranted = false;
	*pfEveryoneGranted = false;
	*pfAnonymousGranted = false;

	//
    // get the DACL of the queue security descriptor.
	//
    BOOL bDaclPresent;
    PACL pDacl;
    BOOL bDaclDefaulted;
    if(!GetSecurityDescriptorDacl(
						pSD, 
						&bDaclPresent, 
						&pDacl, 
						&bDaclDefaulted
						))
	{
        DWORD gle = GetLastError();
		TrERROR(SECURITY, "GetSecurityDescriptorDacl() failed, %!winerr!", gle);
		LogBOOL(FALSE, s_FN, 20);
		return;
	}

	//
    // If there is no DACL, or it is NULL, access is granted for all.
	//
    if (!bDaclPresent || !pDacl)
    {
		TrTRACE(SECURITY, "no DACL, or NULL DACL, access is granted for all");
		*pfAllGranted = true;
		*pfEveryoneGranted = true;
		*pfAnonymousGranted = true;
		return;
    }

    ACL_SIZE_INFORMATION AclSizeInfo;
    if(!GetAclInformation(
					pDacl, 
					&AclSizeInfo, 
					sizeof(AclSizeInfo), 
					AclSizeInformation
					))
	{
        DWORD gle = GetLastError();
		TrERROR(SECURITY, "GetAclInformation() failed, %!winerr!", gle);
		LogBOOL(FALSE, s_FN, 40);
		return;
	}

	//
    // If the DACL is empty, deny access from all.
	//
    if (AclSizeInfo.AceCount == 0)
    {
		TrTRACE(SECURITY, "empty DACL, deny access from all");
		LogBOOL(FALSE, s_FN, 50);
		return;
    }

	bool fEveryoneGranted = false;
	bool fAnonymousGranted = false;
    for (DWORD i = 0; i < AclSizeInfo.AceCount; i++)
    {
		LPVOID pAce;

        if(!GetAce(pDacl, i, &pAce))
        {
	        DWORD gle = GetLastError();
			TrERROR(SECURITY, "GetAce() failed, %!winerr!", gle);
			return;
        }

        //
		// Ignore unknown ACEs
		//
        if (!(GET_ACE_TYPE(pAce) == ACCESS_ALLOWED_ACE_TYPE) &&
            !(GET_ACE_TYPE(pAce) == ACCESS_DENIED_ACE_TYPE))
        {
            continue;
        }

		//
        // See if we have the permission bit set in the ACE.
		//
        if (GET_ACE_MASK(pAce) & Permission)
        {
			if(GET_ACE_TYPE(pAce) == ACCESS_DENIED_ACE_TYPE)
			{
				//
				// Found a deny on the requested permission
				//
				TrTRACE(SECURITY, "found deny ACE");
				LogBOOL(FALSE, s_FN, 60);
				return;
			}

			ASSERT(GET_ACE_TYPE(pAce) == ACCESS_ALLOWED_ACE_TYPE);
			
            if(EqualSid(MQSec_GetWorldSid(), GET_PSID_FROM_PACE(pAce)))
			{
				fEveryoneGranted = true;
				TrTRACE(SECURITY, "Everyone allowed access");
				continue;
			}

            if(EqualSid(MQSec_GetAnonymousSid(), GET_PSID_FROM_PACE(pAce)))
			{
				TrTRACE(SECURITY, "Anonymous allowed access");
				fAnonymousGranted = true;
			}

		}

	}

	*pfEveryoneGranted = fEveryoneGranted;
	*pfAnonymousGranted = fAnonymousGranted;

	*pfAllGranted = (fEveryoneGranted && fAnonymousGranted);
}


static 
void 
PrintSid(
	PSID pSid
	)
/*++
Routine Description:
	Print text sid and user information.

Arguments:
	pSid - pointer to the sid

Returned Value:
	None

--*/
{

	//
	// string sid
	//
	LPWSTR pStringSid = NULL;
	if(!ConvertSidToStringSid(pSid, &pStringSid))
	{
        DWORD gle = GetLastError();
		TrERROR(SECURITY, "ConvertSidToStringSid failed, %!winerr!", gle);
		return;
	}

    CAutoLocalFreePtr pFreeSid = reinterpret_cast<BYTE*>(pStringSid);

	//
	// map sid to domain\user account
	//
    WCHAR NameBuffer[128];
    WCHAR DomainBuffer[128];
    ULONG NameLength = TABLE_SIZE(NameBuffer);
    ULONG DomainLength = TABLE_SIZE(DomainBuffer);
    SID_NAME_USE SidUse;
    if (!LookupAccountSid( 
			NULL,
			pSid,
			NameBuffer,
			&NameLength,
			DomainBuffer,
			&DomainLength,
			&SidUse
			))
    {
        DWORD gle = GetLastError();
		TrTRACE(SECURITY, "%ls", pStringSid);
		TrERROR(SECURITY, "LookupAccountSid failed, %!winerr!", gle);
		return;
    }

	if(DomainBuffer[0] == '\0')
	{
		TrTRACE(SECURITY, "%ls, %ls", pStringSid, NameBuffer);
		return;
	}

	TrTRACE(SECURITY, "%ls, %ls\\%ls", pStringSid, DomainBuffer, NameBuffer);

}


typedef struct _ACCESS_ALLOWED_OBJECT_ACE_1 {
    ACE_HEADER Header;
    ACCESS_MASK Mask;
    DWORD Flags;
    GUID ObjectType;
    DWORD SidStart;
} ACCESS_ALLOWED_OBJECT_ACE_1 ;


static
void
PrintACEs(
	PACL pAcl
	)
/*++
Routine Description:
	Print list of ACEs

Arguments:
	pAcl - pointer to the ACL to be printed

Returned Value:
	None

--*/
{
	TrTRACE(SECURITY, "Revision: %d,  Numof ACEs: %d", pAcl->AclRevision, pAcl->AceCount);

    for (DWORD i = 0; i < pAcl->AceCount; i++)
    {
		ACCESS_ALLOWED_ACE* pAce;
        if(!GetAce(
				pAcl, 
				i, 
				(LPVOID*)&(pAce)
				))
        {
			DWORD gle = GetLastError();
			TrERROR(SECURITY, "GetAce() failed, %!winerr!", gle);
			throw bad_win32_error(gle);
        }

        DWORD AceType = pAce->Header.AceType;
		if((AceType > ACCESS_MAX_MS_OBJECT_ACE_TYPE) || 
		   (AceType == ACCESS_ALLOWED_COMPOUND_ACE_TYPE))
		{
			//
			// Handle only obj ACE and normal ACE.
			//
			TrTRACE(SECURITY, "ACE(%d), Unknown AceType %d", i, AceType);
			throw bad_win32_error(ERROR_INVALID_ACL);
		}

	    bool fObjAce = false;
		if((AceType >= ACCESS_MIN_MS_OBJECT_ACE_TYPE) && 
		   (AceType <= ACCESS_MAX_MS_OBJECT_ACE_TYPE))
		{
			fObjAce = true;
		}

		TrTRACE(SECURITY, "ACE(%d), AceType - %d, Mask- 0x%x", i, AceType, pAce->Mask);

		ACCESS_ALLOWED_OBJECT_ACE* pObjAce = reinterpret_cast<ACCESS_ALLOWED_OBJECT_ACE*>(pAce);

		PSID pSid = reinterpret_cast<PSID>(&(pAce->SidStart));
        if(fObjAce)
        {
			TrTRACE(SECURITY, "ObjFlags - 0x%x", pObjAce->Flags);

            if (pObjAce->Flags == ACE_OBJECT_TYPE_PRESENT)
            {
	            ACCESS_ALLOWED_OBJECT_ACE_1* pObjAce1 = reinterpret_cast<ACCESS_ALLOWED_OBJECT_ACE_1*>(pObjAce);
				pSid = reinterpret_cast<PSID>(&(pObjAce1->SidStart));
            }
        }

        if (pObjAce->Flags == ACE_OBJECT_TYPE_PRESENT)
        {
			TrTRACE(SECURITY, "ObjectType - %!guid!", &pObjAce->ObjectType);
        }

//		TrTRACE(SECURITY, "%!sid!", pSid);
		PrintSid(pSid);
    }
}


static
void
ShowOGandSID(
	PSID pSid, 
	BOOL fDefaulted
	)
/*++
Routine Description:
	Print owner\group and sid

Arguments:
	pSid - pointer to sid
	Defaulted - flag that indicate if defaulted

Returned Value:
	None

--*/
{
	if(fDefaulted)
	{
		TrTRACE(SECURITY, "Defaulted");
	}
	else
	{
		TrTRACE(SECURITY, "NotDefaulted");
	}

    if (!pSid)
    {
		TrTRACE(SECURITY, "Not available");
		return;
    }

//	TrTRACE(SECURITY, "%!sid!", pSid);
    PrintSid(pSid);
}


void
PrintAcl(
    BOOL fAclExist,
    BOOL fDefaulted,
    PACL pAcl
	)
/*++
Routine Description:
	Print Acl

Arguments:
	pSid - pointer to sid
	Defaulted - flag that indicate if defaulted

Returned Value:
	None

--*/
{
	if (!fAclExist)
    {
		TrTRACE(SECURITY, "NotPresent");
		return;
    }

	if(fDefaulted)
	{
		TrTRACE(SECURITY, "Defaulted");
	}
	else
	{
		TrTRACE(SECURITY, "NotDefaulted");
	}

	if (pAcl == NULL)
    {
		TrTRACE(SECURITY, "NULL");
		return;
	}

    PrintACEs(pAcl);
}


static
void  
ShowNT5SecurityDescriptor( 
	PSECURITY_DESCRIPTOR pSD
	)
/*++
Routine Description:
	Print Security descriptor

Arguments:
	pSD - pointer to security descriptor

Returned Value:
	None

--*/
{
	ASSERT((pSD != NULL) && IsValidSecurityDescriptor(pSD));
	if((pSD == NULL) || !IsValidSecurityDescriptor(pSD))
	{
		TrERROR(SECURITY, "invalid security descriptor or NULL security descriptor");
		throw bad_win32_error(ERROR_INVALID_SECURITY_DESCR);
	}

    DWORD dwRevision = 0;
    SECURITY_DESCRIPTOR_CONTROL sdC;
    if(!GetSecurityDescriptorControl(pSD, &sdC, &dwRevision))
	{
		DWORD gle = GetLastError();
		TrERROR(SECURITY, "GetSecurityDescriptorControl() failed, %!winerr!", gle);
		throw bad_win32_error(gle);
	}
	
	TrTRACE(SECURITY, "SecurityDescriptor");
	TrTRACE(SECURITY, "Control - 0x%x, Revision - %d", (DWORD) sdC, dwRevision);

	//
	// Owner
	//
    PSID  pSid;
    BOOL  Defaulted = FALSE;
    if (!GetSecurityDescriptorOwner(pSD, &pSid, &Defaulted))
    {
		DWORD gle = GetLastError();
		TrERROR(SECURITY, "GetSecurityDescriptorOwner() failed, %!winerr!", gle);
		throw bad_win32_error(gle);
    }

	TrTRACE(SECURITY, "Owner information:");
    ShowOGandSID(pSid, Defaulted);

	//
	// Group
	//
    if (!GetSecurityDescriptorGroup(pSD, &pSid, &Defaulted))
    {
		DWORD gle = GetLastError();
		TrERROR(SECURITY, "GetSecurityDescriptorGroup() failed, %!winerr!", gle);
		throw bad_win32_error(gle);
    }

	TrTRACE(SECURITY, "Group information:");
    ShowOGandSID(pSid, Defaulted);


	//
	// DACL
	//
    BOOL fAclExist;
    PACL pAcl;
    if (!GetSecurityDescriptorDacl(pSD, &fAclExist, &pAcl, &Defaulted))
    {
		DWORD gle = GetLastError();
		TrERROR(SECURITY, "GetSecurityDescriptorDacl() failed, %!winerr!", gle);
		throw bad_win32_error(gle);
    }

	TrTRACE(SECURITY, "DACL information:");
	PrintAcl(fAclExist, Defaulted, pAcl); 

	//
	// SACL
	//
    if (!GetSecurityDescriptorSacl(pSD, &fAclExist, &pAcl, &Defaulted))
    {
		DWORD gle = GetLastError();
		TrERROR(SECURITY, "GetSecurityDescriptorSacl() failed, %!winerr!", gle);
		throw bad_win32_error(gle);
    }

	TrTRACE(SECURITY, "SACL information:");
	PrintAcl(fAclExist, Defaulted, pAcl); 
}


static
void
GetClientContextInfo(
	AUTHZ_CLIENT_CONTEXT_HANDLE ClientContext
	)
/*++

Routine Description:
	Get Client context info.

Arguments:
	AuthzClientContext - client context

Returned Value:
	None
	
--*/
{
	//
	// UserSid
	//
	DWORD BufferSize = 0;

	AuthzGetInformationFromContext(
		  ClientContext,
		  AuthzContextInfoUserSid,
		  0,
		  &BufferSize,
		  NULL
		  );

	AP<BYTE> pToken = new BYTE[BufferSize];
	if(!AuthzGetInformationFromContext(
			  ClientContext,
			  AuthzContextInfoUserSid,
			  BufferSize,
			  &BufferSize,
			  pToken
			  ))
	{
		DWORD gle = GetLastError();
		TrERROR(SECURITY, "AuthzGetContextInformation(AuthzContextInfoUserSid)  failed, %!winerr!", gle);
		throw bad_win32_error(gle);
	}

	TrTRACE(SECURITY, "AuthzContextInfoUserSid");
    PSID pSid = (PSID) (((TOKEN_USER*) pToken.get())->User.Sid);
//	TrTRACE(SECURITY, "%!sid!", pSid);
	PrintSid(pSid);

	//
	// GroupsSids
	//
	AuthzGetInformationFromContext(
		  ClientContext,
		  AuthzContextInfoGroupsSids,
		  0,
		  &BufferSize,
		  NULL
		  );

	AP<BYTE> pTokenGroup = new BYTE[BufferSize];
	if(!AuthzGetInformationFromContext(
			  ClientContext,
			  AuthzContextInfoGroupsSids,
			  BufferSize,
			  &BufferSize,
			  pTokenGroup
			  ))
	{
		DWORD gle = GetLastError();
		TrERROR(SECURITY, "AuthzGetContextInformation(AuthzContextInfoGroupsSids)  failed, %!winerr!", gle);
		throw bad_win32_error(gle);
	}

	DWORD GroupCount = (((TOKEN_GROUPS*) pTokenGroup.get())->GroupCount);
	TrTRACE(SECURITY, "AuthzContextInfoGroupsSids, GroupCount = %d", GroupCount);

	for(DWORD i=0; i < GroupCount; i++)
	{
		PSID pSid = (PSID) (((TOKEN_GROUPS*) pTokenGroup.get())->Groups[i].Sid);
		TrTRACE(SECURITY, "Group %d: ", i);
//		TrTRACE(SECURITY, "%!sid!", pSid);
		PrintSid(pSid);
	}
	
	//
	// RestrictedSids
	//
	AuthzGetInformationFromContext(
		  ClientContext,
		  AuthzContextInfoRestrictedSids,
		  0,
		  &BufferSize,
		  NULL
		  );

	AP<BYTE> pRestrictedSids = new BYTE[BufferSize];
	if(!AuthzGetInformationFromContext(
			  ClientContext,
			  AuthzContextInfoRestrictedSids,
			  BufferSize,
			  &BufferSize,
			  pRestrictedSids
			  ))
	{
		DWORD gle = GetLastError();
		TrERROR(SECURITY, "AuthzGetContextInformation(AuthzContextInfoRestrictedSids)  failed, %!winerr!", gle);
		throw bad_win32_error(gle);
	}

	GroupCount = (((TOKEN_GROUPS*) pRestrictedSids.get())->GroupCount);
	TrTRACE(SECURITY, "AuthzContextInfoRestrictedSids, GroupCount = %d", GroupCount);

	for(DWORD i=0; i < GroupCount; i++)
	{
		PSID pSid = (PSID) (((TOKEN_GROUPS*) pRestrictedSids.get())->Groups[i].Sid);
		TrTRACE(SECURITY, "Group %d: ", i);
//		TrTRACE(SECURITY, "%!sid!", pSid);
		PrintSid(pSid);
	}
}


bool
IsAllGranted(
	DWORD Permissions,
	PSECURITY_DESCRIPTOR pSD
	)
/*++

Routine Description:
	Checks if all granted permission.

Arguments:
	permissions - requested permissions.
	pSD - security descriptor.

Returned Value:
	true if all grant the permission.
	
--*/
{
	bool fAllGranted = false;
	bool fEveryoneGranted = false;
	bool fAnonymousGranted = false;

	IsPermissionGranted(
		pSD, 
		Permissions,
		&fAllGranted, 
		&fEveryoneGranted, 
		&fAnonymousGranted 
		);

	TrTRACE(SECURITY, "IsAllGranted = %d", fAllGranted);
	return fAllGranted;
}


bool
IsEveryoneGranted(
	DWORD Permissions,
	PSECURITY_DESCRIPTOR pSD
	)
/*++

Routine Description:
	Checks if everyone granted permission.

Arguments:
	permissions - requested permissions.
	pSD - security descriptor.

Returned Value:
	true if everyone grant the permission.
	
--*/
{
	bool fAllGranted = false;
	bool fEveryoneGranted = false;
	bool fAnonymousGranted = false;

	IsPermissionGranted(
		pSD, 
		Permissions,
		&fAllGranted, 
		&fEveryoneGranted, 
		&fAnonymousGranted 
		);

	TrTRACE(SECURITY, "IsEveryoneGranted = %d", fEveryoneGranted);
	return fEveryoneGranted;
}


void
DumpAccessCheckFailureInfo(
	DWORD permissions,
	PSECURITY_DESCRIPTOR pSD,
	AUTHZ_CLIENT_CONTEXT_HANDLE ClientContext
	)
/*++

Routine Description:
	DumpAccessCheckFailureInfo if registry DumpAccessCheckFailure
	is set.

Arguments:
	permissions - requested permissions.
	pSD - security descriptor.
	ClientContext - authz client context handle

Returned Value:
	None
	
--*/
{
	if(DumpAccessCheckFailure())
	{
		TrTRACE(SECURITY, "requested permission = 0x%x", permissions);

		try
		{
			GetClientContextInfo(ClientContext);
			ShowNT5SecurityDescriptor(pSD);
		}
		catch(bad_win32_error& exp)
		{
			TrERROR(SECURITY, "catch bad_win32_error exception, error = 0x%x", exp.error());
		}
	}
}

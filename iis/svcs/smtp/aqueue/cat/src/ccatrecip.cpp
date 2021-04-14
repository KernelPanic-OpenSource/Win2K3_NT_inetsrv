//+------------------------------------------------------------
//
// Copyright (C) 1998, Microsoft Corporation
//
// File: ccatrecip.cpp
//
// Contents: Implementation of classes:
//
// Classes:
//   CIMsgRecipListAddr
//   CCatRecip
//
// Functions:
//   CIMsgRecipListAddr::CIMsgRecipListAddr
//   CIMsgRecipListAddr::~CIMsgRecipListAddr
//   CIMsgRecipListAddr::GetSpecificOrigAddress
//   CIMsgRecipListAddr::CreateNewCatAddr
//   CIMsgRecipListAddr::HrAddAddresses
//   CIMsgRecipListAddr::SetUnresolved
//   CIMsgRecipListAddr::SetDontDeliver
//   CIMsgRecipListAddr::SetMailMsgCatStatus
//
//   CCatRecip::CCatRecip
//   CCatRecip::AddDLMember
//   CCatRecip::AddForward
//   CCatRecip::HrCompletion
//   CCatRecip::HandleFailure
//
// History:
// jstamerj 980325 15:32:17: Created.
//
//-------------------------------------------------------------

#include "precomp.h"

//
// class CIMsgRecipListAddr
//

//+------------------------------------------------------------
//
// Function: CIMsgRecipListAddr::CIMsgRecipListAddr
//
// Synopsis: Initializes member data
//
// Arguments:
//   pStore: EmailIDStore to use
//   pIRC:   per IMsg resolve list context to use
//   prlc:   store context to use
//   hLocalDomainContext: local domain context to use
//   pBifMgr: bifurcation manager to use for getting other
//            IMailMsgRecipientsAdd interaces
//   pRecipsAdd: IMailMsgRecipientsAdd of the original recipient to
//               resolve
//   dwRecipIndex: Index in pRecipsAdd of the original recipient to
//                 resolve
//   fPrimary: TRUE means original recipient was added as primary
//             FALSE means original recipient was added as secondary
//
// Returns: NOTHING
//
// History:
// jstamerj 980325 12:54:02: Created.
//
//-------------------------------------------------------------
CIMsgRecipListAddr::CIMsgRecipListAddr(
    CICategorizerListResolveIMP    *pCICatListResolve) :
    CCatAddr(pCICatListResolve)
{
    CatFunctEnterEx((LPARAM)this, "CIMsgRecipListAddr::CIMsgRecipListAddr");
    CatFunctLeave();
}

//+------------------------------------------------------------
//
// Function: CIMsgRecipListAddr::~CIMsgRecipListAddr
//
// Synopsis: Releases the IMailMsgRecipientsAdd reference
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 980325 12:59:50: Created.
//
//-------------------------------------------------------------
CIMsgRecipListAddr::~CIMsgRecipListAddr()
{
    CatFunctEnterEx((LPARAM)this, "CImsgRecipListAddr::~CIMsgRecipListAddr");
    CatFunctLeave();
}


//+------------------------------------------------------------
//
// Function: CIMsgRecipListAddr::GetSpecificOrigAddress
//
// Synopsis: Attempts to retrieve a specified original address
//
// Arguments:
//  CAType: Address type to retrieve
//  psz: Buffer to receive address string
//  dwcc: Size of that buffer
//
// Returns:
//  S_OK: Success
//  CAT_IMSG_E_PROPNOTFOUND: this recipient does not have that address
//  or other error from mailmsg
//
// History:
// jstamerj 1998/07/30 20:20:22: Created.
//
//-------------------------------------------------------------
HRESULT CIMsgRecipListAddr::GetSpecificOrigAddress(
    CAT_ADDRESS_TYPE        CAType,
    LPSTR                   psz,
    DWORD                   dwcc)
{
    HRESULT hr;
    IMailMsgRecipientsAdd *pRecipsAdd;
    DWORD dwRecipIndex;

    CatFunctEnterEx((LPARAM)this, "CIMsgRecipListAddr::GetSpecificOrigAddress");

    hr = GetIMsgRecipInfo(&pRecipsAdd, &dwRecipIndex, NULL, NULL);
    if(FAILED(hr)) {
        ERROR_LOG_ADDR(this, "GetIMsgRecipInfo");
        CatFunctLeaveEx((LPARAM)this);
        return hr;
    }

    hr = pRecipsAdd->GetStringA(
        dwRecipIndex,
        PropIdFromCAType(CAType),
        dwcc,
        psz);

    pRecipsAdd->Release();

    DebugTrace((LPARAM)this, "Item/GetStringA returned hr %08lx", hr);

    if(psz[0] == '\0')
        hr = CAT_IMSG_E_PROPNOTFOUND;

    if(FAILED(hr) && (hr != CAT_IMSG_E_PROPNOTFOUND))
    {
        ERROR_LOG_ADDR(this, "pRecipsAdd->GetStringA");
    }

    CatFunctLeaveEx((LPARAM)this);
    return hr;
}



//+------------------------------------------------------------
//
// Function: CIMsgRecipListAddr::CreateNewCatAddr
//
// Synopsis: CCatRecip methods call this function when they need to
//           create a new CCatRecip object and a corresponding recipient
//           in the member m_pRecipsAdd with one address only.  On
//           success, the CCatAddr is returned with a refcount of
//           one.
//
// Arguments:
//   CAType: Address type of new CCatRecip object
//   pszAddress: Address string.  If NULL, a new CCatRecip object is
//               created with properties set to point to the current
//               mailmsg recipient (AddPrimary/AddSecondary is not
//               called)
//   ppCCatAddr: Pointer to pointer to CCatAddr object that is set to
//               the newly allocated CCatRecip
//   fPrimary: if TRUE, add via AddPrimary
//             if FALSE, add via AddSecondary
//
// Returns:
//   S_OK: Success
//   E_OUTOFMEMORY: duh
//   CAT_E_PROP_NOT_FOUND: a required ICategorizerItem prop was not set
//   or Error from IMsg
//
// History:
// jstamerj 980325 14:15:50: Created.
//
//-------------------------------------------------------------
HRESULT CIMsgRecipListAddr::CreateNewCatAddr(
    CAT_ADDRESS_TYPE CAType,
    LPTSTR pszAddress,
    CCatAddr **ppCCatAddr,
    BOOL   fPrimary)
{
    CatFunctEnterEx((LPARAM)this, "CIMsgRecipListAddr::CreateNewCatAddr");
    DWORD dwNewRecipIndex = 0;
    HRESULT hr = S_OK;
    LPCTSTR psz = pszAddress;
    DWORD dwPropId = 0;
    IMailMsgRecipientsAdd *pRecipsAdd = NULL;
    IMailMsgProperties *pIMailMsgProps = NULL;
    DWORD dwOrigIndex = 0;
    DWORD dwLevel = 0;
    ICategorizerItem *pICatItemNew = NULL;
    
    DebugTrace((LPARAM)this, "CAType = %d", CAType);

    if(pszAddress) 
    {
        DebugTrace((LPARAM)this, "pszAddress = %s", pszAddress);
    } 
    else 
    {
        DebugTrace((LPARAM)this, "pszAddress = NULL");
    }

    DebugTrace((LPARAM)this, "fPrimary = %d", fPrimary);

    // Retrieve IMsg interface/recip index
    hr = GetIMsgRecipInfo(&pRecipsAdd, &dwOrigIndex, NULL, &pIMailMsgProps);
    if(FAILED(hr)) 
    {
        ERROR_LOG_ADDR(this, "GetIMsgRecipInfo");
        pRecipsAdd = NULL;
        pIMailMsgProps = NULL;
        goto CLEANUP;
    }
    //
    // Get the recipient level
    //
    dwLevel = DWLevel() + 1;
    //
    // Unknown dwLevel is -1, so -1 + 1 = 0
    // If dwLevel is unknown, new value will be zero.
    //

    if(pszAddress == NULL) 
    {
        //
        // Create new CCatAddr pointing to the CURRENT recipient
        //
        dwNewRecipIndex = dwOrigIndex;
    } 
    else 
    {
        //
        // Get the equivilant mailmsg propID
        //
        dwPropId = PropIdFromCAType(CAType);

        if(fPrimary) 
        {
            hr = pRecipsAdd->AddPrimary(
                1,
                &psz,
                &dwPropId,
                &dwNewRecipIndex,
                pRecipsAdd,
                dwOrigIndex);
            _ASSERT(hr != CAT_IMSG_E_DUPLICATE);
        } 
        else 
        {
            hr = pRecipsAdd->AddSecondary(
                1,
                &psz,
                &dwPropId,
                &dwNewRecipIndex,
                pRecipsAdd,
                dwOrigIndex);
        }
        DebugTrace((LPARAM)this, "AddPrimary/AddSecondary returned hr %08lx", hr);
    }

    if(hr == CAT_IMSG_E_DUPLICATE) 
    {
        INCREMENT_COUNTER(MailmsgDuplicateCollisions);
        goto CLEANUP;
    } 
    else if(FAILED(hr))
    {
        ERROR_LOG_ADDR(this, "AddPrimary / AddSecondary");
        goto CLEANUP;
    }

    //
    // Alloc an ICategorizerItem so that it can set all the necessary properties
    //
    hr = m_pCICatListResolve->AllocICategorizerItem(
        SOURCE_RECIPIENT,
        &pICatItemNew);
    ERROR_CLEANUP_LOG_ADDR(this, "m_pCICatListResolve->AllocICategorizerItem");
    //
    // Set important ICategorizerItem props on the newborn
    //
    hr = PutIMsgRecipInfo(
        &pRecipsAdd,
        &dwNewRecipIndex,
        &fPrimary,
        &pIMailMsgProps,
        // Only set dwLevel if the old value is known
        (dwLevel == 0) ? NULL : &dwLevel,
        pICatItemNew);

    // This should never fail
    _ASSERT(SUCCEEDED(hr));

    //
    // Get the CCatAddr object
    // This should never fail as no sinks have had the chance
    // to muck with properties yet.
    //
    hr = m_pCICatListResolve->GetCCatAddrFromICategorizerItem(
        pICatItemNew,
        ppCCatAddr);

    _ASSERT(SUCCEEDED(hr));
    //
    // Reset the display name
    //
    hr = ((CIMsgRecipListAddr *)
          *ppCCatAddr)->HrSetDisplayNameProp(NULL);
    ERROR_CLEANUP_LOG_ADDR(this, "HrSetDisplayNameProp");

 CLEANUP:
    if(FAILED(hr))
    {
        *ppCCatAddr = NULL;
        if(pICatItemNew)
            pICatItemNew->Release();
    }
    if(pRecipsAdd)
        pRecipsAdd->Release();
    if(pIMailMsgProps)
        pIMailMsgProps->Release();

    CatFunctLeave();
    return hr;
}

//+------------------------------------------------------------
//
// Function: CIMsgRecipListAddr::HrAddAddresses
//
// Synopsis: Add the addresses contained in the arrays to the IMsg
//           recipient we contain
//
// Arguments:
//  dwNumAddresses: Number of new addresses
//  rgCAType: Array of address types
//  rgpsz: Array of pointers to address strings
//
// Returns:
//  S_OK: Success
//  CAT_E_FORWARD_LOOP: One or more of the new addresses is a
//  duplicate of a recipient in the parent chain
//  CAT_E_NO_SMTP_ADDRESS: Did not add the new addresses because there
//  is no SMTP address
//
// History:
// jstamerj 980325 14:21:56: Created.
//
//-------------------------------------------------------------
HRESULT CIMsgRecipListAddr::HrAddAddresses(
    DWORD dwNumAddresses,
    CAT_ADDRESS_TYPE *rgCAType,
    LPTSTR *rgpsz)
{
    CatFunctEnterEx((LPARAM)this, "CIMsgRecipListAddr::AddAddresses");
    HRESULT hr, hrDupCheck;

    IMailMsgRecipientsAdd *pRecipsAdd;
    DWORD dwOrigIndex;
    BOOL fPrimary;
    DWORD dwCount;
    DWORD dwNewIndex;
    DWORD dwPropIds[CAT_MAX_ADDRESS_TYPES];
    BOOL  fSMTPAddress;
    DWORD dwSMTPAddressIdx = 0;

    _ASSERT(dwNumAddresses > 0);
    _ASSERT(dwNumAddresses <= CAT_MAX_ADDRESS_TYPES);

    // Retrieve IMsg interface/recip index
    hr = GetIMsgRecipInfo(&pRecipsAdd, &dwOrigIndex, &fPrimary, NULL);
    if(FAILED(hr)) {
        ERROR_LOG_ADDR(this, "GetIMsgRecipInfo");
        return hr;
    }
    //
    // Initialize the array of address types
    //
    fSMTPAddress = FALSE;
    for(dwCount = 0; dwCount < dwNumAddresses; dwCount++) {


        dwPropIds[dwCount] = PropIdFromCAType(rgCAType[dwCount]);
        if(rgCAType[dwCount] == CAT_SMTP) {
            fSMTPAddress = TRUE;
            dwSMTPAddressIdx = dwCount;
        }
    }

    if(fSMTPAddress == FALSE) {
        ErrorTrace((LPARAM)this, "Not delivering to recipient without an SMTP address");
        hr = CAT_E_NO_SMTP_ADDRESS;
        goto CLEANUP;
    }
    //
    // Validate the SMTP address
    //
    hr = HrValidateAddress(
        rgCAType[dwSMTPAddressIdx],
        rgpsz[dwSMTPAddressIdx]);
    if(FAILED(hr)) {
        ErrorTrace((LPARAM)this, "Default SMTP address is invalid: %s",
                   rgpsz[dwSMTPAddressIdx]);
        ERROR_LOG_ADDR(this, "HrValidateAddress");
        hr = HrHandleInvalidAddress();
        goto CLEANUP;
    }

    //
    // Call IMsg add with new addresses
    // If we're primary, we remain primary.
    //
    if(fPrimary) {
        //
        // We need to check for a loop here too since there could be a
        // loop where some other recipient is forwarding to one of our
        // non-default proxy addresses (bug #70220)
        //
        hr = CheckForLoop(
            dwNumAddresses,
            rgCAType,
            rgpsz,
            FALSE); // No need to check ourself for a duplicate

        if(FAILED(hr))
        {
            ERROR_LOG_ADDR(this, "CheckForLoop");
        }
        else
        {
            hr = pRecipsAdd->AddPrimary(
                dwNumAddresses,
                (LPCSTR *)rgpsz,
                dwPropIds,
                &dwNewIndex,
                pRecipsAdd,
                dwOrigIndex);
            _ASSERT(hr != CAT_IMSG_E_DUPLICATE);
            
            if(FAILED(hr))
            {
                ERROR_LOG_ADDR(this, "pRecipsAdd->AddPrimary");
            }
        }

    } else {

        hr = pRecipsAdd->AddSecondary(
            dwNumAddresses,
            (LPCSTR *)rgpsz,
            dwPropIds,
            &dwNewIndex,
            pRecipsAdd,
            dwOrigIndex);

        if(hr == CAT_IMSG_E_DUPLICATE) {

            INCREMENT_COUNTER(MailmsgDuplicateCollisions);
            //
            // The duplicate might be us (the recipient in the mailmsg
            // before resolution)
            //
            hrDupCheck = CheckForDuplicateCCatAddr(
                dwNumAddresses,
                rgCAType,
                rgpsz);

            if(hrDupCheck == CAT_IMSG_E_DUPLICATE) {
                //
                // So we do collide with our parent.
                // Remove if from duplicate detection and try again.
                //
                hr = RemoveFromDuplicateRejectionScheme(TRUE);

                if(SUCCEEDED(hr)) {
                    hr = pRecipsAdd->AddSecondary(
                        dwNumAddresses,
                        (LPCSTR *)rgpsz,
                        dwPropIds,
                        &dwNewIndex,
                        pRecipsAdd,
                        dwOrigIndex);

                    if(hr == CAT_IMSG_E_DUPLICATE)
                        INCREMENT_COUNTER(MailmsgDuplicateCollisions);

                }

            } else if(FAILED(hrDupCheck)) {

                ERROR_LOG_ADDR(this, "pRecipsAdd->AddSecondary");
                //
                // Return the error
                //
                hr = hrDupCheck;
                ERROR_LOG_ADDR(this, "CheckForDuplicateCCatAddr");

            } else {

                ERROR_LOG_ADDR(this, "pRecipsAdd->AddSecondary");

            }

            if(hr == CAT_IMSG_E_DUPLICATE) {
                //
                // If hr is STILL Duplicate, check to see if it's a loop
                // we've encountered
                //
                hrDupCheck = CheckForLoop(
                    dwNumAddresses,
                    rgCAType,
                    rgpsz,
                    FALSE); // No need to check ourself for a
                            // duplicate

                if(FAILED(hrDupCheck)) {
                    //
                    // Return the error -- this could be
                    // CAT_E_FORWARD_LOOP
                    //
                    hr = hrDupCheck;
                    ERROR_LOG_ADDR(this, "CheckForLoop");
                }
            }
        }
    }

    DebugTrace((LPARAM)this, "AddPrimary/AddSecondary returned hr %08lx", hr);

    if(SUCCEEDED(hr)) {
        // Since we were just adding addresses for the same recipient,
        // always mark the old recipient as "Don't deliver"
        hr = SetDontDeliver(TRUE);
        if(SUCCEEDED(hr)) {
            //
            // Relase old Recipient, update this to point to the new
            // recipient
            // IMailMsgRecipients and fPrimary always remain the same
            // for default processing
            //
            hr = PutIMailMsgRecipientsAddIndex(dwNewIndex, this);
            if(FAILED(hr))
            {
                ERROR_LOG_ADDR(this, "PutIMailMsgRecipientsAddIndex");
            }

        } else {

            ERROR_LOG_ADDR(this, "SetDontDeliver");
        }
    }
 CLEANUP:
    pRecipsAdd->Release();
    CatFunctLeave();
    return hr;
}

//+------------------------------------------------------------
//
// Function: CIMsgRecipListAddr::SetUnresolved
//
// Synopsis: Sets recipient property on IMsg indicating this recipient
//           could not be resolved -- this will cause NDR generation for the
//           recipient
//
// Arguments:
//  HrReason: Reason why address is unresolved
//
// Returns:
//  S_OK: Success
//  Or error from IMsg
//
// History:
// jstamerj 980325 14:29:45: Created.
//
//-------------------------------------------------------------
HRESULT CIMsgRecipListAddr::SetUnresolved(
    HRESULT HrStatus)
{
    CatFunctEnterEx((LPARAM)this, "CIMsgRecipListAddr::SetUnresolved");
    HRESULT hr;
    IMailMsgRecipientsAdd *pRecipsAdd;
    IMailMsgProperties *pIMailMsgProps;
    DWORD dwRecipIndex;
    DWORD dwFlags = 0;

    LogNDREvent(HrStatus);

    INCREMENT_COUNTER(NDRdRecipients);

    switch(HrStatus) {

     case HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND):
         INCREMENT_COUNTER(UnresolvedRecipients);
         break;

     case CAT_E_MULTIPLE_MATCHES:
         INCREMENT_COUNTER(AmbiguousRecipients);
         break;

     case CAT_E_ILLEGAL_ADDRESS:
         INCREMENT_COUNTER(IllegalRecipients);
         break;

     case CAT_E_FORWARD_LOOP:
         INCREMENT_COUNTER(LoopRecipients);
         break;

     default:
         INCREMENT_COUNTER(GenericFailureRecipients);
         break;
    }

    // Retrieve IMsg interface/recip index
    hr = GetIMsgRecipInfo(&pRecipsAdd, &dwRecipIndex, NULL, &pIMailMsgProps);
    if(FAILED(hr)) {
        ERROR_LOG_ADDR(this, "GetIMsgRecipInfo");
        return hr;
    }

    hr = pRecipsAdd->GetDWORD(dwRecipIndex,
                              IMMPID_RP_RECIPIENT_FLAGS,
                              &dwFlags);
    if(SUCCEEDED(hr) || (hr == CAT_IMSG_E_PROPNOTFOUND)) {

        dwFlags |= (RP_ERROR_CONTEXT_CAT | RP_UNRESOLVED);

        hr = pRecipsAdd->PutDWORD(dwRecipIndex,
                                  IMMPID_RP_RECIPIENT_FLAGS,
                                  dwFlags);

        if(SUCCEEDED(hr)) {

            hr = pRecipsAdd->PutDWORD(
                dwRecipIndex,
                IMMPID_RP_ERROR_CODE,
                HrStatus);

            if(SUCCEEDED(hr)) {

                hr = SetMailMsgCatStatus(
                    pIMailMsgProps,
                    CAT_W_SOME_UNDELIVERABLE_MSGS);
            } else {
                ERROR_LOG_ADDR(this, "SetMailMsgCatStatus");
            }
        } else {
            ERROR_LOG_ADDR(this, "pRecipsAdd->PutDWORD");
        }
    } else {
        ERROR_LOG_ADDR(this, "pRecipsAdd->GetDWORD(IMMPID_RP_RECIPIENT_FLAGS)");
    }
    
    pRecipsAdd->Release();
    pIMailMsgProps->Release();

    CatFunctLeaveEx((LPARAM)this);
    return hr;
}

//+------------------------------------------------------------
//
// Function: CIMsgRecipListAddr::SetDontDeliver
//
// Synopsis: Sets the IMsg property on a recipient that indicates this
//           recipient should be removed upon a call to WriteList
//
// Arguments:
//   fDontDeliver: TRUE means remove recipient on a WriteList
//                 FALSE means clear DontDeliver property, don't
//                 remove on a WriteList
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 980325 14:34:44: Created.
//
//-------------------------------------------------------------
HRESULT CIMsgRecipListAddr::SetDontDeliver(BOOL fDontDeliver)
{
    CatFunctEnterEx((LPARAM)this, "CIMsgRecipListAddr::SetDontDeliver");
    IMailMsgRecipientsAdd *pRecipsAdd;
    DWORD dwRecipIndex;
    HRESULT hr;

    // Retrieve IMsg interface/recip index
    hr = GetIMsgRecipInfo(&pRecipsAdd, &dwRecipIndex, NULL, NULL);
    if(FAILED(hr)) {
        ERROR_LOG_ADDR(this, "GetIMsgRecipInfo");
        return hr;
    }

    hr = pRecipsAdd->PutBool(dwRecipIndex,
                             IMMPID_RPV_DONT_DELIVER,
                             fDontDeliver);
    if(FAILED(hr)) {
        ERROR_LOG_ADDR(this, "pRecipsAdd->PutBool");
    }
    pRecipsAdd->Release();
    CatFunctLeaveEx((LPARAM)this);
    return hr;
}


//+------------------------------------------------------------
//
// Function: CIMsgRecipListAddr::RemoveFromDuplicateRejectionScheme
//
// Synopsis: Sets the IMsg property to indicate this recipient's names
//           should be ignored for duplicate detection
//
// Arguments:
//   fRemove: TRUE means remove recipient for dup detection
//            FALSE means clear property, don't remove recip for dup detection.
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 980325 14:34:44: Created.
//
//-------------------------------------------------------------
HRESULT CIMsgRecipListAddr::RemoveFromDuplicateRejectionScheme(BOOL fRemove)
{
    CatFunctEnterEx((LPARAM)this, "CIMsgRecipListAddr::SetDontDeliver");
    IMailMsgRecipientsAdd *pRecipsAdd;
    DWORD dwRecipIndex;
    HRESULT hr;

    // Retrieve IMsg interface/recip index
    hr = GetIMsgRecipInfo(&pRecipsAdd, &dwRecipIndex, NULL, NULL);
    if(FAILED(hr)) {
        ERROR_LOG_ADDR(this, "GetIMsgRecipInfo");
        return hr;
    }

    hr = pRecipsAdd->PutBool(dwRecipIndex,
                             IMMPID_RPV_NO_NAME_COLLISIONS,
                             fRemove);
    if(FAILED(hr)) {
        ERROR_LOG_ADDR(this, "pRecipsAdd->PutBool");
    }
    pRecipsAdd->Release();
    CatFunctLeave();
    return hr;
}

//
// class CCatRecip
//

//+------------------------------------------------------------
//
// Function: CCatRecip::CCatRecip
//
// Synopsis: initialize member data
//
// Arguments:
//   See CIMsgRecipListAddr::CIMsgRecipListAddr
//
// Returns: NOTHING
//
// History:
// jstamerj 980325 14:36:30: Created.
//
//-------------------------------------------------------------
CCatRecip::CCatRecip(
    CICategorizerListResolveIMP *pCICatListResolve) :
    CCatExpandableRecip(pCICatListResolve)
{
    CatFunctEnterEx((LPARAM)this, "CCatRecip::CCatRecip");

    INCREMENT_COUNTER(RecipsInMemory);

    CatFunctLeave();
}


//+------------------------------------------------------------
//
// Function: CCatRecip::~CCatRecip
//
// Synopsis: Decrement count of recips in memory
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/02/24 19:26:01: Created.
//
//-------------------------------------------------------------
CCatRecip::~CCatRecip()
{
    DECREMENT_COUNTER(RecipsInMemory);
}



//+------------------------------------------------------------
//
// Function: CCatRecip::AddDLMember
//
// Synopsis: EmailIDStore calls this function once for every DL Member
//           when setting properties on a DL.  It is called before
//           CCatRecip::HrCompletion
//
// Arguments:
//   CAType: Known address type of the DL Member
//   pszAddress: pointer to the address string
//
// Returns:
//  S_OK: Success, issued a pending LDAP search
//  S_FALSE: Success, did not issue a search
//  Or, error from IMsg
//
// History:
// jstamerj 980325 14:39:20: Created.
//
//-------------------------------------------------------------
HRESULT CCatRecip::AddDLMember(CAT_ADDRESS_TYPE CAType, LPTSTR pszAddress)
{
    HRESULT hr;
    CCatAddr *pMember = NULL;

    CatFunctEnterEx((LPARAM)this, "CCatRecip::AddDLMember");
    DebugTrace((LPARAM)this, "CAType: %d", CAType);
    DebugTrace((LPARAM)this, "pszAddress: %s", pszAddress);

    hr = GetListResolveStatus();
    if(FAILED(hr)) {

        ErrorTrace((LPARAM)this, "Not adding DL member since list resolve failed");
        ERROR_LOG_ADDR(this, "GetListResolveStatus")
        // Signal to ldapstor to stop resolution
        goto CLEANUP;
    }

    //
    // Validate the new address first
    //
    hr = HrValidateAddress(CAType, pszAddress);
    if(FAILED(hr)) {

        ErrorTrace((LPARAM)this, "Invalid member address");
        ERROR_LOG_ADDR(this, "HrValidateAddress");
        hr = HrHandleInvalidAddress();
        goto CLEANUP;
    }


    // Create a new CCatAddr to handle resolution of this DL Member
    hr = CreateNewCatAddr(
        CAType,
        pszAddress,
        &pMember,
        FALSE);

    if(hr == CAT_IMSG_E_DUPLICATE) {
        DebugTrace((LPARAM)this, "Resolution failed with e_duplicate");
        // Fine, DL member was a duplicate so we won't be
        // re-resolving it.  Let it be.
    } else if(SUCCEEDED(hr)) {

        // Great....dispatch the query to the store
        hr = pMember->HrResolveIfNecessary();
        pMember->Release();
    } else {
        ERROR_LOG_ADDR(this, "CreateNewCatAddr");
    }

 CLEANUP:
    if(hr == CAT_IMSG_E_DUPLICATE)
        hr = S_FALSE;

    DebugTrace((LPARAM)this, "returning hr %08lx", hr);
    CatFunctLeave();
    return hr;
}

//+------------------------------------------------------------
//
// Function: CCatRecip::AddDynamicDlMember
//
// Synopsis: Add a DL member that has already been looked up in the DS
//
// Arguments:
//  pICatItemAttr: the attributes of the DL member
//
// Returns:
//  S_OK: Success
//  MAILTRANSPORT_S_PENDING: doing an async operation, will call your
//  completion routine when I am finished
//
// History:
// jstamerj 1998/09/29 21:30:26: Created.
//
//-------------------------------------------------------------
HRESULT CCatRecip::AddDynamicDLMember(
    ICategorizerItemAttributes *pICatItemAttr)
{
    HRESULT hr;
    CCatAddr *pMember = NULL;
    ATTRIBUTE_ENUMERATOR enumerator_dn;
    BOOL fEnumeratingDN = FALSE;
    LPSTR pszDistinguishedNameAttr = NULL;
    LPSTR pszDistinguishedName = NULL;
    ICategorizerUTF8Attributes *pIUTF8Attr = NULL;

    CatFunctEnterEx((LPARAM)this, "CCatRecip::AddDynamicDlMember");

    _ASSERT(pICatItemAttr);

    hr = GetListResolveStatus();
    if(FAILED(hr)) {

        ErrorTrace((LPARAM)this, "Not adding DL member since list resolve failed");
        ERROR_LOG_ADDR(this, "GetListResolveStatus");
        // Signal to ldapstor to stop resolution
        goto CLEANUP;
    }

    hr = GetICatParams()->GetDSParameterA(
        DSPARAMETER_ATTRIBUTE_DEFAULT_DN,
        &pszDistinguishedNameAttr);
    if(FAILED(hr)) {
        //
        // Fail entire message categorization
        //
        ErrorTrace((LPARAM)this,
            "Failing entire message categorization because we couldn\'t fetch the DN attribute name");
        ERROR_LOG_ADDR(this, "GetDSParameterA(DSPARAMETER_ATTRIBUTE_DEFAULT_DN)");
        hr = SetListResolveStatus(hr);
        _ASSERT(SUCCEEDED(hr));
        goto CLEANUP;
    }

    hr = pICatItemAttr->QueryInterface(
        IID_ICategorizerUTF8Attributes,
        (void **)&pIUTF8Attr);
    if (FAILED(hr)) {
        ERROR_LOG_ADDR(this, "pICatItemAttr->QueryInterface(IID_ICategorizerUTF8Attributes");
        hr = S_OK;
        goto CLEANUP;
    }

    hr = pIUTF8Attr->BeginUTF8AttributeEnumeration(
        pszDistinguishedNameAttr,
        &enumerator_dn);
    if (hr == CAT_E_PROPNOTFOUND) {
        //
        // Silently skip this recip
        //
        ErrorTrace((LPARAM)this,
            "DN attribute \'%s\' not present in results; skipping recip",
            pszDistinguishedNameAttr);
        ERROR_LOG_ADDR(this, "pIUTF8Attr->BeginUTF8AttributeEnumeration(dn)");
        hr = S_OK;
        goto CLEANUP;

    } else if (FAILED(hr)) {
        //
        // Enumeration failed for some other reason.
        // Fail entire message categorization.
        //
        ErrorTrace((LPARAM)this,
            "Failing entire message categorization because enumeration of attribute \'%s\' failed with %08lx",
            pszDistinguishedNameAttr, hr);
        ERROR_LOG_ADDR(this, "pIUTF8Attr->BeginUTF8AttributeEnumeration(dn)");
        hr = SetListResolveStatus(hr);
        _ASSERT(SUCCEEDED(hr));
        goto CLEANUP;
    }

    fEnumeratingDN = TRUE;

    hr = pIUTF8Attr->GetNextUTF8AttributeValue(
        &enumerator_dn,
        &pszDistinguishedName);
    if (hr == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS)) {
        //
        // silently skip this recip
        //
        ErrorTrace((LPARAM)this,
            "attribute \'%s\' present but with no values; skipping recip",
            pszDistinguishedNameAttr);
        ERROR_LOG_ADDR(this, "pIUTF8Attr->GetNextUTF8AttributeValue");
        hr = S_OK;
        goto CLEANUP;

    } else if (FAILED(hr)) {
        //
        // fail entire message categorization
        //
        ErrorTrace((LPARAM)this,
            "Failed to enumerate DN attribute \'%s\' with hr %08lx",
            pszDistinguishedNameAttr, hr);
        ERROR_LOG_ADDR(this, "pIUTF8Attr->GetNextUTF8AttributeValue");
        hr = SetListResolveStatus(hr);
        _ASSERT(SUCCEEDED(hr));
        goto CLEANUP;
    }

    //
    // Create a new CCatAddr for this member
    //
    hr = CreateNewCatAddr(
        CAT_DN,
        pszDistinguishedName,
        &pMember,
        FALSE);

    if (hr == CAT_IMSG_E_DUPLICATE) {
        //
        // silently skip this recip
        //
        ErrorTrace((LPARAM)this, "duplicate address detected; skipping recip");
        hr = S_OK;
        goto CLEANUP;

    } else if (FAILED(hr)) {

        ERROR_LOG_ADDR(this, "CreateNewCatAddr");
        goto CLEANUP;
    }
    //
    // Since we've already looked up the attributes, just set the
    // ICatItemAttr property of the new guy and trigger
    // ProcessItem/ExpandItem/CompleteItem
    //
    hr = pMember->PutHRESULT(
        ICATEGORIZERITEM_HRSTATUS,
        S_OK);
    ERROR_CLEANUP_LOG_ADDR(this, "pMember->PutHRESULT");

    hr = pMember->PutICategorizerItemAttributes(
        ICATEGORIZERITEM_ICATEGORIZERITEMATTRIBUTES,
        pICatItemAttr);
    ERROR_CLEANUP_LOG_ADDR(this, "pMember->PutICategorizerItemAttributes");

    //
    // Simulate DS completion
    //
    IncPendingLookups();
    pMember->LookupCompletion();
    hr = S_OK;

 CLEANUP:
    if(pMember)
        pMember->Release();

    if (fEnumeratingDN) {
        pIUTF8Attr->EndUTF8AttributeEnumeration(
            &enumerator_dn);
    }

    if (pIUTF8Attr)
        pIUTF8Attr->Release();

    DebugTrace((LPARAM)this, "returning hr %08lx", hr);
    CatFunctLeaveEx((LPARAM)this);
    return hr;
}




//+------------------------------------------------------------
//
// Function: CCatRecip::AddForward
//
// Synopsis: EMailIDStore calls this function once for every
//           forwarding address the recipient has.  It is called before
//           CCatRecip::HrCompletion.  On any unhandleable errors,
//           this function sets a list resolve error (instead of
//           returning an error)
//
// Arguments:
//   CAType: Known address type of the forwarding address
//   szForwardingAddres: The forwarding address
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 980325 14:48:49: Created.
//
//-------------------------------------------------------------
HRESULT CCatRecip::AddForward(
    CAT_ADDRESS_TYPE CAType,
    LPTSTR szForwardingAddress)
{
    CatFunctEnterEx((LPARAM)this, "CCatRecip::AddForward");
    DebugTrace((LPARAM)this, "CAType: %d", CAType);
    DebugTrace((LPARAM)this, "szForwardingAddress: %s", szForwardingAddress);
    HRESULT hr;
    CCatAddr *pCCatAddr;
    BOOL fPrimary;

    //
    // Is the forwarding address valid?
    //
    hr = HrValidateAddress(CAType, szForwardingAddress);

    if(FAILED(hr)) {

        ErrorTrace((LPARAM)this, "Forwarding address string is invalid");
        ERROR_LOG_ADDR(this, "HrValidateAddress");
        hr = HrHandleInvalidAddress();
        goto CLEANUP;
    }


    hr = GetFPrimary(&fPrimary);
    ERROR_CLEANUP_LOG_ADDR(this, "GetFPrimary");
    //
    // jstamerj 1998/07/31 19:58:53:
    //  If we're in the primary chain, we MUST check to see if we're
    //  in a forwarding loop before we call AddPrimary
    //  This is the place to do it
    //
    if(fPrimary) {
        //
        // Check for a loop before adding the forwarding address
        //
        hr = CheckForLoop(
            CAType,
            szForwardingAddress,
            TRUE);  // Check this object too (you could forward to yourself)
        ERROR_CLEANUP_LOG_ADDR(this, "CheckForLoop");
    }

    // Create the new address object with the address we know about
    hr = CreateNewCatAddr(CAType,
                          szForwardingAddress,
                          &pCCatAddr,
                          fPrimary);

    if(hr == CAT_IMSG_E_DUPLICATE) {

        _ASSERT(fPrimary == FALSE);
        DebugTrace((LPARAM)this, "Duplicate from CreateNewCatAddr, checking for a loop");

        //
        // Did we hit duplicate because we're in a loop?
        //
        hr = CheckForLoop(
            CAType,
            szForwardingAddress,
            TRUE);  // CHeck this object too
        if(FAILED(hr)) {

            ERROR_LOG_ADDR(this, "CheckForLoop");
        }

    } else if(SUCCEEDED(hr)) {
        //
        // Since this is forwarding, we need to set the parent
        // ICatItem pointer (to be able to do loop detection)
        //
        hr = PutICategorizerItemParent(
            this,
            pCCatAddr);

        _ASSERT(SUCCEEDED(hr));

        //
        // Resolve the new address
        //
        hr = pCCatAddr->HrResolveIfNecessary();

        if(FAILED(hr)) {
            ErrorTrace((LPARAM)this, "Unable to dispatch query for forwarding address");
            ERROR_LOG_ADDR(this, "pCCatAddr->HrResolveIfNecessary");
        }
        pCCatAddr->Release();

    } else {
        
        ERROR_LOG_ADDR(this, "CreateNewCatAddr");
    }
 CLEANUP:
    if(FAILED(hr) && (hr != CAT_E_FORWARD_LOOP)) {

        ErrorTrace((LPARAM)this, "Setting the list resolve error:%08lx", hr);
        _VERIFY(SUCCEEDED(SetListResolveStatus(hr)));
    }

    CatFunctLeaveEx((LPARAM)this);
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CCatRecip::HrCompleteItem_Default
//
// Synopsis: Handle the CompleteItem call; finally make decisions
// about what to do concerning HrStatus failures.
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/07/31 18:50:01: Created.
//
//-------------------------------------------------------------
HRESULT CCatRecip::HrCompleteItem_Default()
{
    HRESULT hr;

    CatFunctEnterEx((LPARAM)this, "CCatRecip::HrCompleteItem_Default");

    hr = GetItemStatus();
    //
    // Try to handle failures
    //
    if(FAILED(hr)) {

        hr = HandleFailure(hr);
        //
        // If we couldn't handle the recipient failure, fail the whole message
        // categorization
        //
        if(FAILED(hr)) {
            _VERIFY(SUCCEEDED(SetListResolveStatus(hr)));
        }
    }

    CatFunctLeaveEx((LPARAM)this);
    return S_OK;
}



//+------------------------------------------------------------
//
// Function: CCatRecip::HandleFailure
//
// Synopsis: When a completion happens with a failure status, this is
// the helper routine to handle the failure.  If the failure can be
// handeled, S_OK is returned.  If not, the failure itself is returned
//
// Arguments:
//  HrFailure: the failure error code
//
// Returns:
//  S_OK: Success
//  or error from Mailmsg
//
// History:
// jstamerj 1998/07/21 18:00:47: Created.
//
//-------------------------------------------------------------
HRESULT CCatRecip::HandleFailure(
    HRESULT HrFailure)
{
    HRESULT hr;

    CatFunctEnterEx((LPARAM)this, "CCatRecip::HandleFailure");

    _ASSERT(FAILED(HrFailure));

    switch(HrFailure) {

     case HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND):
     {
         //
         // Address was not found but it is not a local address anyway
         //
         DebugTrace((LPARAM)this, "Remote address not found in DS");

         hr = S_OK;
     }

     case CAT_E_FORWARD_LOOP:
     case CAT_IMSG_E_DUPLICATE:
     case CAT_E_NO_SMTP_ADDRESS:
     {
         //
         // This guy was either a failed resolve where it turns out we
         // already resolved the recipient (in another place in the
         // recip list) or a recipient in a detected loop.  Don't do
         // anything here, DSN flags were/will be set in HandleLoopHead
         //
         hr = S_OK;
         break;
     }

     case CAT_E_BAD_RECIPIENT:
     {
         //
         // A generic recipient error code that indicates this
         // recipient should be NDR'd.  An optional reason can be set
         // in the ICATEGORIZERITEM_HRNDR property
         //
         HRESULT hrReason;
         hr = GetHRESULT(
             ICATEGORIZERITEM_HRNDRREASON,
             &hrReason);

         if(FAILED(hr)) {
             //
             // Use the generic error code for the NDR reason also
             //
             hrReason = CAT_E_BAD_RECIPIENT;
         }

         ErrorTrace((LPARAM)this, "NDRing recipient with error code %08lx",
                    hrReason);

         hr = SetUnresolved(hrReason);
         if(FAILED(hr)) {

             ERROR_LOG_ADDR(this, "SetUnresolved");
         }
         break;
     }
     case CAT_E_DELETE_RECIPIENT:
     {
         //
         // Don't deliver to this recipient
         //
         hr = SetDontDeliver(TRUE);
         if(FAILED(hr)) {

             ERROR_LOG_ADDR(this, "SetDontDeliver");
         }
         break;
     }

     default:
     {
         //
         // EmailIDStore is informing us of an unrecoverable error
         // There's nothing we can do to handle this error except
         // return it (HrCompletion will then SetListResolveStatus)
         //
         ErrorTrace((LPARAM)this, "Unrecoverable error returned from EmailIDStore: hr %08lx", HrFailure);
         hr = HrFailure;
         ERROR_LOG_ADDR(this, "--unhandeled recip error--");
         break;
     }
    }

    CatFunctLeaveEx((LPARAM)this);
    return hr;
}



//+------------------------------------------------------------
//
// Function: CIMsgRecipListAddr::CheckForLoop
//
// Synopsis: Helper routine to check for a loop in our ancestors
//
// Arguments:
//  dwNumAddresses: Number of addresses to check
//  rgCAType: array of address types
//  rgpsz: array of string pointers
//  fCheckSelf: Check for a dupicate with this CCatAddr?
//
// Returns:
//  S_OK: Success, no loops
//  CAT_E_FORWARD_LOOP: Detected a loop and called HandleLoopHead
//  successfully
//  or error from CheckAncestorsForDuplicate/HandleLoopHead
//
// History:
// jstamerj 1998/08/01 16:05:51: Created.
//
//-------------------------------------------------------------
HRESULT CIMsgRecipListAddr::CheckForLoop(
    DWORD dwNumAddresses,
    CAT_ADDRESS_TYPE *rgCAType,
    LPSTR *rgpsz,
    BOOL fCheckSelf)
{
    HRESULT hr;
    CCatAddr *pCCatAddrDup;

    CatFunctEnterEx((LPARAM)this, "CCatRecip::CheckForLoop");

    hr = CheckAncestorsForDuplicate(
        dwNumAddresses,
        rgCAType,
        rgpsz,
        fCheckSelf,
        &pCCatAddrDup);

    if (hr == CAT_IMSG_E_DUPLICATE) {
        //
        // We've got a loop!
        //
        ErrorTrace((LPARAM)this, "Loop detected!");
        ERROR_LOG_ADDR(this, "CheckAncestorsForDuplicate");

        //
        // Generate the DSN on the CCatAddr at the top of the loop
        //
        hr = pCCatAddrDup->HandleLoopHead();
        if(SUCCEEDED(hr)) {
            //
            // Return error to caller
            //
            hr = CAT_E_FORWARD_LOOP;
        }
        else
        {
            ERROR_LOG_ADDR(this, "pCCatAddrDup->HandleLoopHead");
        }
        pCCatAddrDup->Release();
    }
    DebugTrace((LPARAM)this, "Returning hr %08lx", hr);
    CatFunctLeaveEx((LPARAM)this);
    return hr;
}


//+------------------------------------------------------------
//
// Function: CIMsgRecipListAddr::CheckForLoop
//
// Synopsis: Same as above with different style parameters
//
// Arguments:
//  CAType: Addres type of pszAddress
//  pszAddress: Address string
//  fCheckSelf: Check this CCatAddr for a duplicate as well?
//
// Returns:
//  S_OK: Success
//  CAT_E_FORWARD_LOOP: Detected a loop and called HandleLoopHead
//  successfully
//  or error from CheckAncestorsForDuplicate/HandleLoopHead
//
// History:
// jstamerj 1998/08/01 16:10:28: Created.
//
//-------------------------------------------------------------
HRESULT CIMsgRecipListAddr::CheckForLoop(
    CAT_ADDRESS_TYPE        CAType,
    LPTSTR                  pszAddress,
    BOOL                    fCheckSelf)
{
    HRESULT hr;
    CatFunctEnterEx((LPARAM)this, "CCatRecip::CheckForLoop");

    hr = CheckForLoop(
        1,              // Number of addresses
        &CAType,        // TYpe array
        &pszAddress,    // String ptr array
        fCheckSelf);

    CatFunctLeaveEx((LPARAM)this);
    return hr;
}



//+------------------------------------------------------------
//
// Function: CIMsgRecipListAddr::HrSetDisplayNameProp
//
// Synopsis: Sets the mailmsg recipient display name property.
//
// Arguments:
//  pwszDisplayName: Display name value.  If NULL, function will set
//  display name to "".
//
// Returns:
//  S_OK: Success
//  error from mailmsg
//
// History:
// jstamerj 2001/04/03 17:21:17: Created.
//
//-------------------------------------------------------------
HRESULT CIMsgRecipListAddr::HrSetDisplayNameProp(
    IN  LPWSTR pwszDisplayName)
{
    HRESULT hr = S_OK;
    DWORD dwRecipIdx = 0;
    IMailMsgRecipientsAdd *pRecipsAdd = NULL;
    CatFunctEnterEx((LPARAM)this,
                      "CIMsgRecipListAddr::HrSetDisplayNameProp");
    
    hr = GetIMsgRecipInfo(&pRecipsAdd, &dwRecipIdx, NULL, NULL);
    ERROR_CLEANUP_LOG_ADDR(this, "GetIMsgRecipInfo");

    hr = pRecipsAdd->PutStringW(
        dwRecipIdx,
        IMMPID_RP_DISPLAY_NAME,
        pwszDisplayName ? pwszDisplayName : L"");
    ERROR_CLEANUP_LOG_ADDR(this, "pRecipsAdd->PutStringW");

    hr = S_OK;

 CLEANUP:
    if(pRecipsAdd)
        pRecipsAdd->Release();

    DebugTrace((LPARAM)this, "returning %08lx", hr);
    CatFunctLeaveEx((LPARAM)this);
    return hr;
} // CIMsgRecipListAddr::HrSetDisplayNameProp



//+------------------------------------------------------------
//
// Function: CIMsgRecipListAddr::LogNDREvent
//
// Synopsis: Log an NDR event
//
// Arguments:
//  hrNDRReason: Reason for NDR
//
// Returns: Nothing
//
// History:
// jstamerj 2001/12/12 23:39:20: Created.
//
//-------------------------------------------------------------
VOID CIMsgRecipListAddr::LogNDREvent(
    IN  HRESULT hrNDRReason)
{
    HRESULT hr = S_OK;
    LPCSTR rgSubStrings[4];
    CHAR szErr[16];
    CHAR szAddress[CAT_MAX_INTERNAL_FULL_EMAIL];
    CHAR szAddressType[CAT_MAX_ADDRESS_TYPE_STRING];

    CatFunctEnter("CIMstRecipListAddr::LogNDREvent");

    //
    // Get the address
    //
    hr = HrGetAddressStringFromICatItem(
        this,
        sizeof(szAddressType) / sizeof(szAddressType[0]),
        szAddressType,
        sizeof(szAddress) / sizeof(szAddress[0]),
        szAddress);
    
    if(FAILED(hr))
    {
        //
        // Still log an event, but use "unknown" for address type/string
        //
        lstrcpyn(szAddressType, "unknown",
                 sizeof(szAddressType) / sizeof(szAddressType[0]));
        lstrcpyn(szAddress, "unknown",
                 sizeof(szAddress) / sizeof(szAddress[0]));
        hr = S_OK;
    }

    rgSubStrings[0] = szAddressType;
    rgSubStrings[1] = szAddress;

    _snprintf(szErr, sizeof(szErr), "0x%08lx", hrNDRReason);

    rgSubStrings[2] = szErr;
    rgSubStrings[3] = NULL;

    //
    // Can we log an event?
    //
    if(GetISMTPServerEx() == NULL)
    {
        FatalTrace((LPARAM)0, "Unable to log func NDR event; NULL pISMTPServerEx");
        for(DWORD dwIdx = 0; dwIdx < 4; dwIdx++)
        {
            if( rgSubStrings[dwIdx] != NULL )
            {
                FatalTrace((LPARAM)0, "Event String %d: %s",
                           dwIdx, rgSubStrings[dwIdx]);
            }
        }
    }
    else
    {
        CatLogEvent(
            GetISMTPServerEx(),
            CAT_EVENT_NDR_RECIPIENT,
            4,
            rgSubStrings,
            hrNDRReason,
            szErr,
            LOGEVENT_FLAG_ALWAYS,
            LOGEVENT_LEVEL_MAXIMUM,
            3
        );
    }
}



//+------------------------------------------------------------
//
// Function: CCatRecip::HandleLoopHead
//
// Synopsis: This is called when it is determined that this CCatAddr
// is the first in a loop chain.  It ensures an NDR will be generated
// for this recipient
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success
//  or error from MailMsg
//
// History:
// jstamerj 1998/08/01 16:41:44: Created.
//
//-------------------------------------------------------------
HRESULT CCatRecip::HandleLoopHead()
{
    HRESULT hr = CAT_E_FORWARD_LOOP;

    CatFunctEnterEx((LPARAM)this, "CCatRecip::HandleLoopHead");

    ERROR_LOG_ADDR(this, "--Handle Loop Head--");
    //
    // Set the status on this recipient to loop error, and UNSET Don't
    // Deliver so an NDR gets generated
    //
    hr = SetRecipientStatus(
        CAT_E_BAD_RECIPIENT);

    if(SUCCEEDED(hr)) {
        //
        // Set the reason
        //
        hr = SetRecipientNDRCode(
            CAT_E_FORWARD_LOOP);

        if(SUCCEEDED(hr)) {
            //
            // Set DSN flags
            //
            hr = SetUnresolved(CAT_E_FORWARD_LOOP);

            if(SUCCEEDED(hr)) {
                //
                // Make sure DSN will be generated even if we previously
                // wern't planning to deliver to this recipient
                //
                hr = SetDontDeliver(FALSE);
                if(FAILED(hr)) {

                    ERROR_LOG_ADDR(this, "SetDontDeliver");
                }
            } else {
                
                ERROR_LOG_ADDR(this, "SetUnresolved");
            }
        } else {
            
            ERROR_LOG_ADDR(this, "SetRecipientNDRCode");
        }
    } else {

        ERROR_LOG_ADDR(this, "SetRecipientStatus");
    }
    DebugTrace((LPARAM)this, "Returning hr %08lx", hr);

    return hr;
}


//+------------------------------------------------------------
//
// Function: CCatRecip::HrHandleInvalidAddress
//
// Synopsis: Do what needs to be done when an invalid address is
// detected (either forwarding to an invalid address or a DL member
// with an invalid address or a new address that is invalid)
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/08/18 18:53:45: Created.
//
//-------------------------------------------------------------
HRESULT CCatRecip::HrHandleInvalidAddress()
{
    HRESULT hr = CAT_E_ILLEGAL_ADDRESS;

    CatFunctEnterEx((LPARAM)this, "CCatRecip::HandleInvalidAddress");

    ERROR_LOG_ADDR(this, "--Handle Invalid Address--");
    //
    // Set the status on this recipient to casue an NDR
    //
    hr = SetRecipientStatus(
        CAT_E_BAD_RECIPIENT);

    //
    // That should never fail
    //
    _ASSERT(SUCCEEDED(hr));

    //
    // Set the status on this recipient to invalid address error
    //
    hr = SetRecipientNDRCode(
        CAT_E_ILLEGAL_ADDRESS);

    //
    // That should never fail
    //
    _ASSERT(SUCCEEDED(hr));

    CatFunctLeaveEx((LPARAM)this);
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CCatRecip::LookupCompletion
//
// Synopsis: Lookup completion routine for a recipient.  Implement
//           defer logic so that RecipLookupCompletion is called after the
//           sender is resolved.
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/03/18 10:10:47: Created.
//
//-------------------------------------------------------------
VOID CCatRecip::LookupCompletion()
{
    CatFunctEnterEx((LPARAM)this, "CCatRecip::LookupCompletion");

    INCREMENT_COUNTER(AddressLookupCompletions);

    m_pCICatListResolve->ResolveRecipientAfterSender(this);

    CatFunctLeaveEx((LPARAM)this);
} // CCatRecip::LookupCompletion


//+------------------------------------------------------------
//
// Function: CCatRecip::RecipLookupCompletion
//
// Synopsis: Handle lookup completion from the emailidstore
//
// Arguments:
//
// Returns: NOTHING
//
// History:
// jstamerj 1998/12/01 14:36:08: Created.
// jstamerj 1999/03/18 10:08:26: Removed return value, removed async
//                               completion to asyncctx.  Renamed to
//                               RecipLookupCompletion and removed
//                               defer logic
//
//-------------------------------------------------------------
VOID CCatRecip::RecipLookupCompletion()
{
    HRESULT hr = S_OK;

    CatFunctEnterEx((LPARAM)this, "CCatRecip::RecipLookupCompletion");

    hr = GetItemStatus();

    if(FAILED(hr)) 
    {
        //
        // Recipient status indicates failure -- decide now if we
        // should NDR
        //
        switch(hr) 
        {
         case HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND):
         {
             //
             // Address was not found.  Determine if the original
             // address we looked up looks local mailbox or not.
             //
             INCREMENT_COUNTER(AddressLookupsNotFound);

             DebugTrace((LPARAM)this, "Address was not found in DS.  Checking locality");

             BOOL fNDR;
             hr = HrNdrUnresolvedRecip(&fNDR);

             if(SUCCEEDED(hr) && fNDR) 
             {
                 //
                 // It's local and we need to NDR this recip
                 //
                 ErrorTrace((LPARAM)this, "Address appears to be local but was not found in DS.  Setting unresolved property.");
                 //
                 // Set NDR Status and the reason
                 //
                 hr = SetRecipientStatus(CAT_E_BAD_RECIPIENT);
                 if(SUCCEEDED(hr)) 
                 {
                     hr = SetRecipientNDRCode(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));
                     if(FAILED(hr)) {

                         ERROR_LOG_ADDR(this, "SetRecipientNDRCode");
                     }
                 } else {
                     
                     ERROR_LOG_ADDR(this, "SetRecipientStatus");
                 }
             } else if(FAILED(hr)) {

                 ERROR_LOG_ADDR(this, "HrNdrUnresolvedRecip");
             }
             break;
         }

         case CAT_E_MULTIPLE_MATCHES:
         case CAT_E_ILLEGAL_ADDRESS:
         case CAT_E_NO_FILTER:
         {
             //
             // Multiple entries for this guy exist in the DS or this guy
             // has an illegal address/forwards to an illegal address
             //
             ErrorTrace((LPARAM)this, "NDRing recipient, reason%08lx",
                        hr);

             hr = SetRecipientNDRCode(hr);
             if(SUCCEEDED(hr)) 
             {
                 hr = SetRecipientStatus(CAT_E_BAD_RECIPIENT);
                 if(FAILED(hr)) {
                     
                     ERROR_LOG_ADDR(this, "SetRecipientStatus");
                 }
             } else {
                 
                 ERROR_LOG_ADDR(this, "SetRecipientNDRCode");
             }
             break;
         }
         case CAT_E_BAD_RECIPIENT:
         {
             //
             // We processed this recipient earlier and returned defer
             //
             hr = S_OK;
             break;
         }
         default:
         {
             //
             // EmailIDStore is informing us of an unrecoverable error
             // There's nothing we can do to handle this error except
             // fail the entire message categorization
             //
             ErrorTrace((LPARAM)this, "Unrecoverable error returned from EmailIDStore: hr %08lx", hr);
             break;
         }
        }
        if(FAILED(hr))
            goto CLEANUP;
    }

    //
    // Set this recipient's display name before triggering events
    //
    hr = HrSetDisplayName();
    ERROR_CLEANUP_LOG_ADDR(this, "HrSetDisplayName");

    //
    // If we handeled the error, go ahead and trigger events.
    // Otherwise, we're failing the message categorization so forget
    // it.
    //
    CCatAddr::LookupCompletion();

 CLEANUP:
    if(FAILED(hr)) {

        ErrorTrace((LPARAM)this, "failing msg categorization hr %08lx", hr);
        _VERIFY(SUCCEEDED(SetListResolveStatus(hr)));
    }
    DecrPendingLookups(); // Matches IncPendingLookups() in CCatAdddr::HrDispatchQuery
    CatFunctLeaveEx((LPARAM)this);
}




//+------------------------------------------------------------
//
// Function: CCatRecip::HrProcessItem_Default
//
// Synopsis: The default sink code for the ProcessItem event.
//           Override CCatAddr's implementation so that we can catch
//           and handle errors from AddAddresses
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success
// jstamerj 1998/12/01 14:47:15: //
// History:
// jstamerj 980325 14:57:05: Created.
//
//-------------------------------------------------------------
HRESULT CCatRecip::HrProcessItem_Default()
{
    HRESULT hr = S_OK;
    HRESULT hrItemStatus = S_OK;
    BOOL fPrimary = FALSE;
    CatFunctEnterEx((LPARAM)this, "CCatRecip::HrProcessItem_Default");

    hr = GetFPrimary(&fPrimary);
    ERROR_CLEANUP_LOG_ADDR(this, "GetFPrimary");
    //
    // CHeck the recipient status
    //
    hrItemStatus = GetItemStatus();
    if(SUCCEEDED(hrItemStatus)) {
        //
        // Add all known addresses to the new address list
        //
        hr = HrAddNewAddressesFromICatItemAttr();
        switch(hr)
        {
         case CAT_E_NO_SMTP_ADDRESS:
             //
             // If this is a primary recipient, NDR
             // Otherwise, fall through and delete this recipient
             //
             if(fPrimary)
             {
                 DebugTrace((LPARAM)this, "NDRing primary recipient without SMTP address");
                 // NDR
                 hr = SetRecipientNDRCode(
                     CAT_E_NO_SMTP_ADDRESS);
                 if(SUCCEEDED(hr))
                     hr = SetRecipientStatus(
                         CAT_E_BAD_RECIPIENT);
                 break;
             }
             else
             {
                 //
                 // Log event when we are deleting recip
                 //
                 ERROR_LOG_ADDR(this, "HrAddNewAddressesFromiCatItemAttr");;
             }
             //
             // Fall through for secondary recipients
             //
         case CAT_IMSG_E_DUPLICATE:
         case CAT_E_FORWARD_LOOP:
         case CAT_E_DELETE_RECIPIENT:

             DebugTrace((LPARAM)this, "AddAddresses failed, removing recip, hr %08lx", hr);
             //
             // Set the recip status to an error so we don't do
             // anything stupid later (like spinning off a resolve for
             // an alternate recipient later)
             //
             hr = SetRecipientStatus(hr);
             if(SUCCEEDED(hr)) 
             {
                 //
                 // Don't deliver to this partialy resolved recipient
                 //
                 hr = SetDontDeliver(TRUE);
                 if(FAILED(hr))
                 {
                     ERROR_LOG_ADDR(this, "SetDontDeliver");
                 }
             }
             else
             {
                 ERROR_LOG_ADDR(this, "SetRecipientStatus");
             }
             break;
         default:
             // Do nothing
             break;
        }
    }
 CLEANUP:
    //
    // Fail the categorization if the above calls failed
    //
    if(FAILED(hr)) 
    {
        ErrorTrace((LPARAM)this, "Setting list resolve error %08lx", hr);
        hr = SetListResolveStatus(hr);
        _ASSERT(SUCCEEDED(hr));
    }
    CatFunctLeaveEx((LPARAM)this);
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CCatRecip::HrExpandItem_Default
//
// Synopsis: Handle the ExpandItem event
//
// Arguments:
//  pfnCompletion: Async completion routine
//  pContext: Context to pass to async completion
//
// Returns:
//  S_OK: Success, will NOT call async completion
//  MAILTRANSPORT_S_PENDING: Will call async completion
//
// History:
// jstamerj 1998/07/31 18:29:57: Created.
//
//-------------------------------------------------------------
HRESULT CCatRecip::HrExpandItem_Default(
    PFN_EXPANDITEMCOMPLETION pfnCompletion,
    PVOID pContext)
{
    HRESULT hr;
    HRESULT hrRet = S_OK;

    CatFunctEnterEx((LPARAM)this, "CCatRecip::HrExpandItem_Default");

    //
    // CHeck the recipient status
    //
    hr = GetItemStatus();
    if(SUCCEEDED(hr)) {
        //
        // Call AddDlMember/AddForward once per DL member or
        // forwarding address
        //
        hr = HrAddDlMembersAndForwardingAddresses(
            pfnCompletion,
            pContext);

        DebugTrace((LPARAM)this, "HrAddDlMembersAndForwardingAddresses returned hr %08lx", hr);
        //
        // if hr is a failure value, something must have failed; so we fail
        // the whole message categorization
        //
        if(FAILED(hr)) {
            ERROR_LOG_ADDR(this, "HrAddDlMembersAndForwardingAddresses");
            _VERIFY(SUCCEEDED(SetListResolveStatus(hr)));

        } else {
            //
            // Return the status returned from HrAddDlMembers...
            // It could be S_OK or S_PENDING
            //
            hrRet = hr;
        }
    }

    CatFunctLeaveEx((LPARAM)this);
    return hrRet;
}


//+------------------------------------------------------------
//
// Function: CCatRecipient::HrNeedsResolving
//
// Synopsis: Determines if this recipient should be resolved or not
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success, it needs resolving
//  S_FALSE: Success, it doesn't need to be resolved
//
// History:
// jstamerj 1998/10/27 15:45:22: Created.
//
//-------------------------------------------------------------
HRESULT CCatRecip::HrNeedsResolveing()
{
    DWORD dwFlags;
    HRESULT hr;

    CatFunctEnterEx((LPARAM)this, "CCatRecip::HrNeedsResolveing");

    dwFlags = GetCatFlags();

    //
    // Do we resolve recipients at all?
    //
    if(! (dwFlags & SMTPDSFLAG_RESOLVERECIPIENTS))
        return S_FALSE;

#define ISTRUE( x ) ( (x) != 0 ? TRUE : FALSE )
    //
    // Do we need to check if the address is local or not?
    //
    if( ISTRUE(dwFlags & SMTPDSFLAG_RESOLVELOCAL) !=
        ISTRUE(dwFlags & SMTPDSFLAG_RESOLVEREMOTE)) {
        //
        // We're resolving either local or remote (not both)
        //
        BOOL fLocal;

        hr = HrIsOrigAddressLocal(&fLocal);

        if(FAILED(hr)) {
            ERROR_LOG_ADDR(this, "HrIsOrigAddressLocal");
            return hr;
        }
        //
        // Resolve if it's local and we're resolving local addrs
        //
        if( (dwFlags & SMTPDSFLAG_RESOLVELOCAL) &&
            (fLocal))
            return S_OK;
        //
        // Resolve if it's remote and we're resolving remote addrs
        //
        if( (dwFlags & SMTPDSFLAG_RESOLVEREMOTE) &&
            (!fLocal))
            return S_OK;
        //
        // else Don't resolve
        //
        return S_FALSE;
    }
    //
    // 2 possabilities -- local and remote bits are on OR local and
    // remote bits are off
    //
    _ASSERT( ISTRUE(dwFlags & SMTPDSFLAG_RESOLVELOCAL) ==
             ISTRUE(dwFlags & SMTPDSFLAG_RESOLVEREMOTE));

    if(dwFlags & SMTPDSFLAG_RESOLVELOCAL) {
        //
        // Both bits are on; Resolve
        //
       _ASSERT(dwFlags & SMTPDSFLAG_RESOLVEREMOTE);

        return S_OK;

    } else {
        //
        // local and remote are disabled; don't resolve
        //
        return S_FALSE;
    }
}



//+------------------------------------------------------------
//
// Function: CCatRecip::HrSetDisplayName
//
// Synopsis: Set this recipients IMMPID_RP_DISPLAY_NAME property.
// Normally, this is set to the "displayName" attribute.  Hello, if
// that attribute is not available, IMMPID_RP_DISPLAY_NAME will bet
// set to L"".
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY
//
// History:
// jstamerj 2001/04/03 16:25:27: Created.
//
//-------------------------------------------------------------
HRESULT CCatRecip::HrSetDisplayName()
{
    HRESULT hr = S_OK;
    ICategorizerParameters *pICatParams = NULL;
    ICategorizerItemAttributes *pICatItemAttr = NULL;
    ICategorizerItemRawAttributes *pIRaw = NULL;
    LPSTR pszDisplayNameAttr = NULL;
    DWORD dwcbDisplayName = 0;
    LPVOID pvDisplayName = NULL;
    LPWSTR pwszDisplayName = NULL;
    BOOL  fEnumerating = FALSE;
    ATTRIBUTE_ENUMERATOR enumerator;
    
    CatFunctEnterEx((LPARAM)this, "CCatRecip::HrSetDisplayName");

    pICatParams = GetICatParams();
    _ASSERT(pICatParams);

    hr = pICatParams->GetDSParameterA(
        PHAT_DSPARAMETER_ATTRIBUTE_DISPLAYNAME,
        &pszDisplayNameAttr);
    if(FAILED(hr) || (pszDisplayNameAttr == NULL))
    {
        hr = S_OK;
        goto CLEANUP;
    }

    hr = GetICategorizerItemAttributes(
        ICATEGORIZERITEM_ICATEGORIZERITEMATTRIBUTES,
        &pICatItemAttr);
    
    if(FAILED(hr) || (pICatItemAttr == NULL)) 
    {
        pICatItemAttr = NULL;
        hr = S_OK;
        goto CLEANUP;
    }

    hr = pICatItemAttr->QueryInterface(
        IID_ICategorizerItemRawAttributes,
        (LPVOID *)&pIRaw);
    ERROR_CLEANUP_LOG_ADDR(this, "pICatItemAttr->QueryInterface(IID_ICategorizerItemRawAttributes)");

    hr = pIRaw->BeginRawAttributeEnumeration(
        pszDisplayNameAttr,
        &enumerator);
    if(FAILED(hr))
    {
        //
        // No display name
        //
        hr = S_OK;
        goto CLEANUP;
    }

    fEnumerating = TRUE;

    hr = pIRaw->GetNextRawAttributeValue(
        &enumerator,
        &dwcbDisplayName,
        &pvDisplayName);
    if(FAILED(hr))
    {
        //
        // No display name
        //
        hr = S_OK;
        goto CLEANUP;
    }

    hr = HrConvertToUnicodeWithAlloc(
        CP_UTF8,
        dwcbDisplayName,
        (LPSTR) pvDisplayName,
        &pwszDisplayName);
    ERROR_CLEANUP_LOG_ADDR(this, "HrConvertToUnicodeWithAlloc");

    hr = HrSetDisplayNameProp(pwszDisplayName);
    ERROR_CLEANUP_LOG_ADDR(this, "HrSetDisplayNameProp");

 CLEANUP:

    if(pwszDisplayName)
        CodePageConvertFree(pwszDisplayName);
    if(fEnumerating)
        pIRaw->EndRawAttributeEnumeration(
            &enumerator);
    if(pIRaw)
        pIRaw->Release();
    if(pICatItemAttr)
        pICatItemAttr->Release();

    DebugTrace((LPARAM)this, "returning %08lx", hr);
    CatFunctLeaveEx((LPARAM)this);
    return hr;
} // CCatRecip::HrSetDisplayName




//
// class CCatExpandableRecip
//

//+------------------------------------------------------------
//
// Function: CCatExpandableRecip::HrAddDlMembersAndForwardingAddresses
//
// Synopsis: Dig through the ICatItemAttr and figure out wether to
//           call HrAddDlMembers or HrAddForwardingAddresses
//
// Arguments:
//  PFN_EXPANDITEMCOMPLETION pfnCompletion: Async completion routine
//  PVOID pContext: completion routine context
//
// Returns:
//  S_OK: Success, will not call completion routine
//  MAILTRANSPORT_S_PENDING: Will call completion routine
//  or error from mailmsg/icatitem/HrAddDlMembers/HrAddForwardingAddresses
//
// History:
// jstamerj 1998/09/29 11:28:54: Created.
//
//-------------------------------------------------------------
HRESULT CCatExpandableRecip::HrAddDlMembersAndForwardingAddresses(
    PFN_EXPANDITEMCOMPLETION pfnCompletion,
    PVOID pContext)
{
    HRESULT hr;
    ICategorizerItemAttributes *pICatItemAttr = NULL;
    ICategorizerParameters *pICatParams;
    LPSTR pszX500DL = NULL;
    LPSTR pszSMTPDL = NULL;
    LPSTR pszDynamicDL = NULL;
    LPSTR pszObjectClassAttribute;
    LPSTR pszObjectClass;
    DLOBJTYPE dlt;
    ATTRIBUTE_ENUMERATOR enumerator;

    CatFunctEnterEx((LPARAM)this, "CCatExpandableRecip::HrAddDlMembersAndForwardingAddresses");

    pICatParams = GetICatParams();
    _ASSERT(pICatParams);

    hr = GetICategorizerItemAttributes(
        ICATEGORIZERITEM_ICATEGORIZERITEMATTRIBUTES,
        &pICatItemAttr);

    if(FAILED(hr)) {
        pICatItemAttr = NULL;
        goto CLEANUP;
    }

    //
    // Fetch DL objectclasses from IDSParams
    //  On failure, the LPSTR will remain pointed to NULL
    //
    pICatParams->GetDSParameterA(
        DSPARAMETER_OBJECTCLASS_DL_X500,
        &pszX500DL);
    pICatParams->GetDSParameterA(
        DSPARAMETER_OBJECTCLASS_DL_SMTP,
        &pszSMTPDL);
    pICatParams->GetDSParameterA(
        DSPARAMETER_OBJECTCLASS_DL_DYNAMIC,
        &pszDynamicDL);

    //
    // Fetch objectclass attribute string from IDSParams
    //
    hr = pICatParams->GetDSParameterA(
        DSPARAMETER_ATTRIBUTE_OBJECTCLASS,
        &pszObjectClassAttribute);

    if(FAILED(hr))
        goto CLEANUP;

    //
    // Now, try to match a DL objectClass with something in
    // pICatItemAttr
    //
    hr = pICatItemAttr->BeginAttributeEnumeration(
        pszObjectClassAttribute,
        &enumerator);
    ERROR_CLEANUP_LOG_ADDR(this, "pICatItemAttr->BeginAttributeEnumeartion(objectClass)");

    hr = pICatItemAttr->GetNextAttributeValue(
        &enumerator,
        &pszObjectClass);

    for (dlt = DLT_NONE; SUCCEEDED(hr) && (dlt == DLT_NONE);) {
        if (pszX500DL && (lstrcmpi(pszObjectClass, pszX500DL) == 0)) {

            dlt = DLT_X500;

        } else if (pszSMTPDL && (lstrcmpi(pszObjectClass, pszSMTPDL) == 0)) {

            dlt = DLT_SMTP;

        } else if (pszDynamicDL && (lstrcmpi(pszObjectClass, pszDynamicDL) == 0)) {

            dlt = DLT_DYNAMIC;
        }
        hr = pICatItemAttr->GetNextAttributeValue(
            &enumerator,
            &pszObjectClass);
    }
    pICatItemAttr->EndAttributeEnumeration(
        &enumerator);

    //
    // Call the appropriate routine
    //
    if(dlt == DLT_NONE) {

        hr = HrAddForwardingAddresses();
        _ASSERT(hr != MAILTRANSPORT_S_PENDING);
        ERROR_CLEANUP_LOG_ADDR(this, "HrAddForwardingAddresses");

    } else {

        hr = HrAddDlMembers(
            dlt,
            pfnCompletion,
            pContext);
        ERROR_CLEANUP_LOG_ADDR(this, "HrAddDlMembers");
    }

 CLEANUP:
    if(pICatItemAttr)
        pICatItemAttr->Release();

    DebugTrace((LPARAM)this, "Returning hr %08lx", hr);
    CatFunctLeaveEx((LPARAM)this);
    return hr;
}


//+------------------------------------------------------------
//
// Function: CCatExpandableRecip::HrAddForwardingAddresses
//
// Synopsis: Call AddForward once for every forwarding address found
//           in ICatItemAttr
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/09/29 13:56:37: Created.
//
//-------------------------------------------------------------
HRESULT CCatExpandableRecip::HrAddForwardingAddresses()
{
    HRESULT hr;
    ICategorizerParameters *pICatParams;
    ICategorizerItemAttributes *pICatItemAttr = NULL;
    ATTRIBUTE_ENUMERATOR enumerator;
    LPSTR pszForwardingSMTPAttribute;
    LPSTR pszForwardingSMTPAddress;
    BOOL fForwarding = FALSE;

    CatFunctEnterEx((LPARAM)this, "CCatExpandableRecip::HrAddForwardingAddresses");

    pICatParams = GetICatParams();
    _ASSERT(pICatParams);

    hr = GetICategorizerItemAttributes(
        ICATEGORIZERITEM_ICATEGORIZERITEMATTRIBUTES,
        &pICatItemAttr);

    if(FAILED(hr)) {
        ERROR_LOG_ADDR(this, "GetICategorizerItemAttributes");
        pICatItemAttr = NULL;
        goto CLEANUP;
    }

    //
    // Get the Forwarding address(es)
    //
    hr = pICatParams->GetDSParameterA(
        DSPARAMETER_ATTRIBUTE_FORWARD_SMTP,
        &pszForwardingSMTPAttribute);
    if(SUCCEEDED(hr)) {
        hr = pICatItemAttr->BeginAttributeEnumeration(
            pszForwardingSMTPAttribute,
            &enumerator);
        if(SUCCEEDED(hr)) {
            hr = pICatItemAttr->GetNextAttributeValue(
                &enumerator,
                &pszForwardingSMTPAddress);
            while(SUCCEEDED(hr)) {
                //
                // jstamerj 980317 15:53:34: Adding support for multiple
                // forwarding addresses -- send to all of them.
                //
                _VERIFY(SUCCEEDED(
                    AddForward( CAT_SMTP,
                                pszForwardingSMTPAddress )));
                //
                // Remember that we're forwarding to at least one address
                //
                fForwarding = TRUE;

                hr = pICatItemAttr->GetNextAttributeValue(
                    &enumerator,
                    &pszForwardingSMTPAddress);

            }
            pICatItemAttr->EndAttributeEnumeration(&enumerator);
        }
    }
    //
    // Check our recipient status -- if it's a failure, that means
    // we're NDRing this recipient due to an invalid address,
    // forward loop, etc.  In this case, we don't want to mark
    // "Don't Deliver"
    //
    if(fForwarding && SUCCEEDED(GetItemStatus())) {
        //
        // Don't deliver to the original recipient when we're
        // forwarding
        //
        hr = SetDontDeliver(TRUE);
        ERROR_CLEANUP_LOG_ADDR(this, "SetDontDeliver");

    } else {
        //
        // Don't return errors from attribute enumeration calls
        //
        hr = S_OK;
    }

 CLEANUP:
    if(pICatItemAttr)
        pICatItemAttr->Release();

    DebugTrace((LPARAM)this, "Returning hr %08lx", hr);
    CatFunctLeaveEx((LPARAM)this);
    return hr;
}


//+------------------------------------------------------------
//
// Function: CCatExpandableRecip::HrAddDlMembers
//
// Synopsis: Call AddDlMember (or AddDynamicDlMember) once for every
//           DlMember
//
// Arguments:
//  dlt: The type of the DL we're expanding
//  PFN_EXPANDITEMCOMPLETION pfnCompletion: Async completion routine
//  PVOID pContext: completion routine context
//
// Returns:
//  S_OK: Success, will not call completion routine
//  MAILTRANSPORT_S_PENDING: Will call completion routine
//  or error from mailmsg/icatitem
//
// History:
// jstamerj 1998/09/29 14:09:56: Created.
//
//-------------------------------------------------------------
HRESULT CCatExpandableRecip::HrAddDlMembers(
    DLOBJTYPE dlt,
    PFN_EXPANDITEMCOMPLETION pfnCompletion,
    PVOID pContext)
{
    HRESULT hr;
    DWORD dwNumMembers = 0;
    PDLCOMPLETIONCONTEXT pDLContext = NULL;

    CatFunctEnterEx((LPARAM)this, "CCatExpandableRecip::HrAddDlMembers");
    //
    // Since we're a DL, don't deliver to the DL object
    //
    hr = SetDontDeliver(TRUE);
    ERROR_CLEANUP_LOG_ADDR(this, "SetDontDeliver");

    switch(dlt) {

     case DLT_X500:
     case DLT_SMTP:
     {
         LPSTR pszMemberAttribute;
         ICategorizerParameters *pICatParams;

         pICatParams = GetICatParams();
         _ASSERT(pICatParams);

         hr = pICatParams->GetDSParameterA(
             DSPARAMETER_ATTRIBUTE_DL_MEMBERS,
             &pszMemberAttribute);

         if(SUCCEEDED(hr)) {

             hr = HrExpandAttribute(
                 NULL,
                 (dlt == DLT_X500) ? CAT_DN : CAT_SMTP,
                 pszMemberAttribute,
                 &dwNumMembers);

             if(SUCCEEDED(hr) && (dwNumMembers == 0)) {
                 //
                 // This might be a paged DL
                 //  Since paged DLs require additional special LDAP
                 //  lookups, use a store function to expand it -- it will
                 //  return S_PENDING and call AddDLMember once per member
                 //
                 pDLContext = AllocDlCompletionContext(this, pfnCompletion, pContext);
                 if(pDLContext == NULL) {

                     hr = E_OUTOFMEMORY;
                     ERROR_LOG_ADDR(this, "AllocDlCompletionContext");

                 } else {

                     hr = GetCCategorizer()->GetEmailIDStore()->HrExpandPagedDlMembers(
                         this,
                         GetResolveListContext(),
                         (dlt == DLT_X500) ? CAT_DN : CAT_SMTP,
                         DlExpansionCompletion,
                         pDLContext);
                     if(FAILED(hr)) {

                         ERROR_LOG_ADDR(this, "HrExpandPagedDlMembers");
                     }
                 }
             } else if(FAILED(hr)) {

                 ERROR_LOG_ADDR(this, "HrExpandAttribute");
             }
         } else {
             ERROR_LOG_ADDR(this, "pICatParams->GetDSParameterA(members)");
         }
         break;
     }

     case DLT_DYNAMIC:
         //
         // Since dynamic DLs require additional special LDAP lookups,
         // use a store function to expand them.  It will return
         // S_PENDING and call AddDynamicDLMember once per member
         //
         pDLContext = AllocDlCompletionContext(this, pfnCompletion, pContext);
         if(pDLContext == NULL) {

             hr = E_OUTOFMEMORY;
             ERROR_LOG_ADDR(this, "AllocDlCompletionContext");

         } else {
             hr = GetCCategorizer()->GetEmailIDStore()->HrExpandDynamicDlMembers(
                 this,
                 GetResolveListContext(),
                 DlExpansionCompletion,
                 pDLContext);
             if(FAILED(hr)) {

                 ERROR_LOG_ADDR(this, "HrExpandDynamicDlMembers");
             }
         }
    }

 CLEANUP:
    if((hr != MAILTRANSPORT_S_PENDING) && (pDLContext != NULL))
        delete pDLContext;

    DebugTrace((LPARAM)this, "returning hr %08lx", hr);
    CatFunctLeaveEx((LPARAM)this);
    return hr;
}


//+------------------------------------------------------------
//
// Function: CCatExpandableRecip::DlExpansionCompletion
//
// Synopsis: Handle completion of the expansion of a paged/dynamic DL
//
// Arguments:
//  hrStatus: Status of the expansion
//  pContext: Our context
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1999/01/29 21:17:46: Created.
//
//-------------------------------------------------------------
VOID CCatExpandableRecip::DlExpansionCompletion(
    HRESULT hrStatus,
    PVOID pContext)
{
    PDLCOMPLETIONCONTEXT pDLContext;

    CatFunctEnterEx((LPARAM)pContext, "CCatExpandableRecip::DlExpansionCompletion");

    pDLContext = (PDLCOMPLETIONCONTEXT)pContext;
    _ASSERT(pDLContext);

    DebugTrace((LPARAM)pContext, "hrStatus %08lx", hrStatus);

    if(FAILED(hrStatus)) {

        HRESULT hr = hrStatus;

        ErrorTrace((LPARAM)pContext, "DlExpansion failed hr %08lx",
                   hrStatus);
        ERROR_LOG_ADDR_STATIC(
            pDLContext->pCCatAddr,
            "async",
            pDLContext->pCCatAddr,
            pDLContext->pCCatAddr->GetISMTPServerEx());

        _VERIFY(SUCCEEDED(pDLContext->pCCatAddr->SetListResolveStatus(hrStatus)));
    }
    //
    // Notify that the expanditem event is complete
    //
    pDLContext->pfnCompletion(pDLContext->pContext);

    delete pDLContext;

    CatFunctLeaveEx((LPARAM)pContext);
}



//+------------------------------------------------------------
//
// Function: CCatExpandableRecip::HrExpandAttribute
//
// Synopsis: Call AddDlMember(CAType, *) for every attribute value
//
// Arguments:
//  pICatItemAttr: Optional ICategorizerItemAttribute to use (if NULL,
//                 will attempt retrieval from ICatItem)
//  CAType: The address type of the DL.
//  pszAttributeName: Attribute name to use
//  pdwNumberMembers: optional pointer to a DWORD to increment once
//                    per member added (not initialized here)
//
// Returns:
//  S_OK: Success
//  or error from ICatItemAttr
//
// History:
// jstamerj 1998/09/23 17:54:57: Created.
//
//-------------------------------------------------------------
HRESULT CCatExpandableRecip::HrExpandAttribute(
    ICategorizerItemAttributes *pICatItemAttrIN,
    CAT_ADDRESS_TYPE CAType,
    LPSTR pszAttributeName,
    PDWORD pdwNumberMembers)
{
    HRESULT hr;
    CMembersInsertionRequest *pCInsertionRequest = NULL;
    ICategorizerItemAttributes *pICatItemAttr = NULL;
    ICategorizerUTF8Attributes *pIUTF8 = NULL;
    ATTRIBUTE_ENUMERATOR enumerator;
    DWORD dwcMembers;
    BOOL fEndEnumeration = FALSE;

    CatFunctEnterEx((LPARAM)this,
                      "CCatExpandableRecip::HrExpandAttribute");

    _ASSERT(pszAttributeName);

    if(pICatItemAttrIN) {
        //
        // Use specified attributes interface
        //
        pICatItemAttr = pICatItemAttrIN;
        pICatItemAttr->AddRef();

    } else {
        //
        // Use default attribute interface
        //
        hr = GetICategorizerItemAttributes(
            ICATEGORIZERITEM_ICATEGORIZERITEMATTRIBUTES,
            &pICatItemAttr);

        if(FAILED(hr)) {
            pICatItemAttr = NULL;
            ERROR_LOG_ADDR(this, "GetICategorizerItemAttributes");
            goto CLEANUP;
        }
    }

    hr = pICatItemAttr->QueryInterface(
        IID_ICategorizerUTF8Attributes,
        (LPVOID *) &pIUTF8);
    ERROR_CLEANUP_LOG_ADDR(this, "pICatItemAttr->QueryInterface(utf8)");

    DebugTrace((LPARAM)this, "Attribute name: %s", pszAttributeName);

    hr = pIUTF8->BeginUTF8AttributeEnumeration(
        pszAttributeName,
        &enumerator);
    ERROR_CLEANUP_LOG_ADDR(this, "pIUTF8->BeginUTF8AttributeEnumeration");

    fEndEnumeration = TRUE;
    //
    // Get the count of values (members)
    //
    hr = pIUTF8->CountUTF8AttributeValues(
        &enumerator,
        &dwcMembers);
    ERROR_CLEANUP_LOG_ADDR(this, "pIUTF8->CountUTF8AttributeValues");

    if(pdwNumberMembers)
        (*pdwNumberMembers) += dwcMembers;

    if(dwcMembers > 0) {

        pCInsertionRequest = new CMembersInsertionRequest(
            this,
            pIUTF8,
            &enumerator,
            CAType);
        if(pCInsertionRequest == NULL) {
            hr = E_OUTOFMEMORY;
            ERROR_LOG_ADDR(this, "new CMembersInsertionRequest");
            goto CLEANUP;
        }
        //
        // The destructor of CMembersInseritonRequest will now end the
        // attribute enumeration
        //
        fEndEnumeration = FALSE;

        hr = HrInsertInsertionRequest(
            pCInsertionRequest);
        ERROR_CLEANUP_LOG_ADDR(this, "HrInsertInsertionRequest");
    }

 CLEANUP:
    //
    // Don't return prop not found errors
    //
    if((hr == CAT_E_PROPNOTFOUND) ||
       (hr == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS)))

       hr = S_OK;

    if(fEndEnumeration)
        pIUTF8->EndUTF8AttributeEnumeration(&enumerator);
    if(pIUTF8)
        pIUTF8->Release();
    if(pICatItemAttr)
        pICatItemAttr->Release();
    if(pCInsertionRequest)
        pCInsertionRequest->Release();

    DebugTrace((LPARAM)this, "Returning hr %08lx", hr);
    CatFunctLeaveEx((LPARAM)this);
    return hr;
}


//+------------------------------------------------------------
//
// Function: CCatDLRecip::CCatDLRecip
//
// Synopsis: Construct the DL recipient
//
// Arguments:
//  pIListResolve: the list resolve object to handle expanding this DL
//
// Returns: NOTHING
//
// History:
// jstamerj 1998/12/05 16:15:20: Created.
//
//-------------------------------------------------------------
CCatDLRecip::CCatDLRecip(
    CICategorizerDLListResolveIMP *pIListResolve) :
    CCatRecip(pIListResolve)
{
    _ASSERT(pIListResolve);
    m_pIListResolve = pIListResolve;
    m_pIListResolve->AddRef();
}


//+------------------------------------------------------------
//
// Function: CCatDLRecip::~CCatDLRecip
//
// Synopsis: release references held by this object
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1998/12/05 16:19:47: Created.
//
//-------------------------------------------------------------
CCatDLRecip::~CCatDLRecip()
{
    if(m_pIListResolve)
        m_pIListResolve->Release();
}




//+------------------------------------------------------------
//
// Function: CCatDLRecip::LookupCompletion
//
// Synopsis: Handle the DS lookup completion of a recipient we're only expanding for
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1998/12/05 15:51:13: Created.
// jstamerj 1999/03/18 10:14:35: Removed return value; removed async
//                               completion to asyncctx
//
//-------------------------------------------------------------
VOID CCatDLRecip::LookupCompletion()
{
    HRESULT hr;

    CatFunctEnterEx((LPARAM)this, "CCatDLRecip::HrLookupCompletion");

    hr = GetItemStatus();

    if(FAILED(hr)) {

        switch(hr) {
         case HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND):
            //
            // This object was not in the DS.  We do nothing
            //
            hr = S_OK;
            break;

         case CAT_E_MULTIPLE_MATCHES:
         case CAT_E_ILLEGAL_ADDRESS:
             //
             // These are caused by DS misconfiguration.  Instead of
             // failing the entire expand, we'll just ignore the
             // recipients that have these problems.
             //
             hr = S_OK;
             break;
         default:
             //
             // We have no choice but to fail the list resolve for any
             // other error
             //
             ErrorTrace((LPARAM)this, "Unrecoverable error returned from EmailIDStore: hr %08lx", hr);
             ERROR_LOG_ADDR(this, "--emailIDStore--");
             break;
        }

    } else {
        //
        // Original recipient status was SUCCESS
        //
        // Call HrAddNewAddressesFromICatItemAttr -- it will dig out all
        // the addresses from ICatItemAttr and call
        // CCatDLRecip::HrAddAddresses -- here we will notify the
        // DLListResolve of the new addresses
        //
        hr = HrAddNewAddressesFromICatItemAttr();
        if(FAILED(hr)) {
            ERROR_LOG_ADDR(this, "HrAddNewAddressesFromICatItemAttr");
        }

        if(SUCCEEDED(hr) || (hr == CAT_E_NO_SMTP_ADDRESS)) {
            //
            // Should we keep resolving?
            //
            hr = m_pIListResolve->HrContinueResolve();
            if(hr == S_OK) {
                //
                // Assume async operation
                //
                IncPendingLookups();

                //
                // Go ahead and expand this if it's a DL
                //
                hr = HrAddDlMembersAndForwardingAddresses(
                    ExpansionCompletion,
                    this);

                if(hr != MAILTRANSPORT_S_PENDING)
                    DecrPendingLookups();
                if(FAILED(hr)) {
                    ERROR_LOG_ADDR(this, "HrAddDlMembersAndForwardingAddresses");
                }
                //
                // MUST preserve return code: MAILTRANSPORT_S_PENDING
                //
            } else if(hr == S_FALSE) {
                hr = S_OK;
            } else {
                ERROR_LOG_ADDR(this, "HrContinueResolve");
            }

        }


        if((hr == CAT_IMSG_E_DUPLICATE) || (hr == CAT_E_FORWARD_LOOP)) {

            DebugTrace((LPARAM)this, "Duplicate collision on AddAddresses hr %08lx", hr);
            //
            // We're just trying to expand DL -- we don't care about
            // loops and such.  However, let's not leave a partially
            // resolved recipient in the recip list
            //
            hr = SetDontDeliver(TRUE);
            if(FAILED(hr)) {
                
                ERROR_LOG_ADDR(this, "SetDontDeliver");
            }
        }
    }
    //
    // Fail the DL expansion if any of the above fails
    //
    if(FAILED(hr)) {
        ErrorTrace((LPARAM)this, "Setting list resolve error %08lx", hr);
        hr = SetListResolveStatus(hr);
        _ASSERT(SUCCEEDED(hr));
    }
    DecrPendingLookups(); // Matches IncPendingLookups() in CCatAdddr::HrDispatchQuery
    CatFunctLeaveEx((LPARAM)this);
}


//+------------------------------------------------------------
//
// Function: CCatDLRecip::HrAddAddresses
//
// Synopsis: Catch the default AddAddresses and notify m_pIListResolve
//
// Arguments:
//  dwNumAddresses: the number of addresses found
//  rgCAType: array of address types
//  rgpsz: array of address strings
//
// Returns:
//  S_OK: Success
//  return value from CIMsgRecipListAddr::HrAddAddresses
//
// History:
// jstamerj 1998/12/05 16:42:12: Created.
//
//-------------------------------------------------------------
HRESULT CCatDLRecip::HrAddAddresses(
    DWORD dwNumAddresses,
    CAT_ADDRESS_TYPE *rgCAType,
    LPTSTR *rgpsz)
{
    HRESULT hr;

    CatFunctEnterEx((LPARAM)this, "CCatDLRecip::HrAddAddresses");

    hr = m_pIListResolve->HrNotifyAddress(
        dwNumAddresses,
        rgCAType,
        rgpsz);

    if(SUCCEEDED(hr)) {
        //
        // Add addresses to mailmsg
        //
        hr = CIMsgRecipListAddr::HrAddAddresses(
            dwNumAddresses,
            rgCAType,
            rgpsz);
        if(FAILED(hr))
        {
            ERROR_LOG_ADDR(this, "CIMsgRecipListAddr::HrAddAddresses");
        }

    } else {

        ERROR_LOG_ADDR(this, "m_pIListResolve->HrNotifyAddress");
    }

    DebugTrace((LPARAM)this, "returning hr %08lx", hr);
    CatFunctLeaveEx((LPARAM)this);
    return hr;
}


//+------------------------------------------------------------
//
// Function: CCatDLRecip::AddForward
//
// Synopsis: Catch the AddForward call.  Since we do not care about
//           forwarding addresses, do nothing
//
// Arguments:
//  CAType: addresses type of the forwarding address
//  pszForwardingAddress: the address string
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/12/05 16:52:58: Created.
//
//-------------------------------------------------------------
HRESULT CCatDLRecip::AddForward(
    CAT_ADDRESS_TYPE CAType,
    LPSTR pszForwardingAddress)
{
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: CCatDLRecip::AddDLMember
//
// Synopsis: Kick off a resolve after we discover this object is a DL
//
// Arguments:
//  CAType: address type we have for this DL member
//  pszAddress: address we have for this DL member
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/12/05 16:54:47: Created.
//
//-------------------------------------------------------------
HRESULT CCatDLRecip::AddDLMember(
    CAT_ADDRESS_TYPE CAType,
    LPSTR pszAddress)
{
    HRESULT hr;

    CatFunctEnterEx((LPARAM)this, "CCatDLRecip:AddDlMember");
    //
    // Notify the DLListResolve about the new address
    //
    hr = m_pIListResolve->HrNotifyAddress(
        1,
        &CAType,
        &pszAddress);

    //
    // Do we keep resolving?
    //
    if(hr == S_OK) {
        //
        // kick off async resolve
        //
        hr = CCatRecip::AddDLMember(
            CAType,
            pszAddress);
        if(FAILED(hr)) {
            
            ERROR_LOG_ADDR(this, "CCatRecip::AddDLMember");
        }

    } else if(SUCCEEDED(hr)) {
        //
        // Remove S_FALSE
        //
        hr = S_OK;

    } else {

        ERROR_LOG_ADDR(this, "m_pIListResolve->HrNotifyAddress");
    }

    DebugTrace((LPARAM)this, "returning hr %08lx", hr);
    CatFunctLeaveEx((LPARAM)this);
    return hr;
}


//+------------------------------------------------------------
//
// Function: CCatDLRecip::ExpansionCompletion
//
// Synopsis: Handle async DL expansion completion
//
// Arguments:
//  pContext: a CCatDLRecip in disguise
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/03/18 13:26:20: Created.
//
//-------------------------------------------------------------
VOID CCatDLRecip::ExpansionCompletion(
    PVOID pContext)
{
    CCatDLRecip *pRecip;

    CatFunctEnterEx((LPARAM)pContext, "CCatDLRecip::ExpansionCompletion");

    pRecip = (CCatDLRecip *)pContext;
    pRecip->DecrPendingLookups();

    CatFunctLeaveEx((LPARAM)pContext);
} // CCatDLRecip::ExpansionCompletion


//+------------------------------------------------------------
//
// Function: CMembersInsertionRequest::HrInsertSearches
//
// Synopsis: Insert LDAP searches for the next few DL members
//
// Arguments:
//  dwcSearches: Number of searches we may insert
//
// Returns:
//  S_OK: Success
//  error: Stop calling HrInsertSearches
//
// History:
// jstamerj 1999/03/25 13:56:46: Created.
//
//-------------------------------------------------------------
HRESULT CMembersInsertionRequest::HrInsertSearches(
    DWORD dwcSearches)
{
    HRESULT hr = S_OK;
    LPSTR pszMember = NULL;
    DWORD dwc;

    CatFunctEnterEx((LPARAM)this, "CMembersInsertionRequest::HrInsertSearches");

    dwc = 0;
    while(SUCCEEDED(hr) && (dwc < dwcSearches)) {

        hr = m_pUTF8Attributes->GetNextUTF8AttributeValue(
            &m_enumerator,
            &pszMember);
        //    
        // GetNextUTF8AttributeValue will return HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS)
        // when we are at the end of the enumeration.
        //
        if(SUCCEEDED(hr)) {
            hr = m_pDLRecipAddr->AddDLMember(m_CAType, pszMember);
            if(hr == S_OK)
                dwc++;
            else if(FAILED(hr)) {
                ERROR_LOG_ADDR(m_pDLRecipAddr, "m_pDLRecipAddr->AddDLMember");
                _VERIFY(SUCCEEDED(m_pDLRecipAddr->SetListResolveStatus(hr)));
            }
        }
    }

    if(FAILED(hr))
        m_hr = hr;

    DebugTrace((LPARAM)this, "returning %08lx", hr);
    CatFunctLeaveEx((LPARAM)this);
    return hr;
} // CMembersInsertionRequest::HrInsertSearches


//+------------------------------------------------------------
//
// Function: CMemberInsertionRequest::NotifyDeQueue
//
// Synopsis: Callback to notify us that our request is being removed
//           from the store's queue
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/03/25 14:11:12: Created.
//
//-------------------------------------------------------------
VOID CMembersInsertionRequest::NotifyDeQueue(
    HRESULT hrReason)
{
    HRESULT hr;
    CatFunctEnterEx((LPARAM)this, "CMemberInsertionRequest::NotifyDeQueue");
    //
    // If we still have things left to resolve, reinsert this
    // insertion request
    //
    hr = hrReason;
    if(SUCCEEDED(m_hr)) {
        if( (hr == CAT_E_DBCONNECTION) || (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED))) {

            hr = m_pDLRecipAddr->HrInsertInsertionRequest(
                this);
        }

        if(FAILED(hr))
        {
            ERROR_LOG_ADDR(m_pDLRecipAddr,
                           "m_pDLRecipAddr->HrInsertInsertionRequest");
            _VERIFY(SUCCEEDED(m_pDLRecipAddr->SetListResolveStatus(hr)));
        }
    }

    CatFunctLeaveEx((LPARAM)this);
} // CMemberInsertionRequest::NotifyDeQueue

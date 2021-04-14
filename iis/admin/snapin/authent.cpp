/*++

   Copyright    (c)    1994-2001    Microsoft Corporation

   Module  Name :
        authent.cpp

   Abstract:
        WWW Authentication Dialog

   Author:
        Ronald Meijer (ronaldm)
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:

--*/
#include "stdafx.h"
#include "resource.h"
#include "common.h"
#include "inetprop.h"
#include "inetmgrapp.h"
#include "supdlgs.h"
#include "certmap.h"
#include "authent.h"
#define INITGUID
#include <initguid.h>
#include <dsclient.h>
#include <wincrui.h>
#include <Dsgetdc.h>
#include <Lm.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define HIDD_DOMAINACCTS		0x50334

CAuthenticationDlg::CAuthenticationDlg(
    IN LPCTSTR lpstrServerName, 
    IN DWORD   dwInstance,      
    IN CString & strBasicDomain,
    IN CString & strRealm,
    IN DWORD & dwAuthFlags,
    IN DWORD & dwAccessPermissions,
    IN CString & strUserName,
    IN CStrPassword & strPassword,
    IN BOOL & fPasswordSync,
    IN BOOL fAdminAccess,
    IN BOOL fHasDigest,
    IN CWnd * pParent           OPTIONAL
    )
/*++

Routine Description:

    Authentication dialog constructor

Arguments:

    LPCTSTR lpstrServerName     : Server name
    DWORD   dwInstance          : Instance number
    CString & strBasicDomain    : Basic domain name
    DWORD & dwAuthFlags         : Authorization flags
    DWORD & dwAccessPermissions : Access permissions
    CString & strUserName       : Anonymous user name
    CStrPassword & strPassword  : Anonymous user pwd
    BOOL & fPasswordSync        : Password sync setting
    BOOL fAdminAccess           : TRUE if user has admin access
    BOOL fHasDigest             : TRUE if machine supports digest auth.
    CWnd * pParent              : Optional parent window

Return Value:

    N/A

--*/
    : CDialog(CAuthenticationDlg::IDD, pParent),
      m_strServerName(lpstrServerName),
      m_strBasicDomain(strBasicDomain),
      m_strRealm(strRealm),
      m_strUserName(strUserName),
      m_strPassword(strPassword),
      m_dwInstance(dwInstance),
      m_dwAuthFlags(dwAuthFlags),
      m_dwAccessPermissions(dwAccessPermissions),
      m_fAdminAccess(fAdminAccess),
      m_fHasDigest(fHasDigest),
      m_fPasswordSync(fPasswordSync),
      m_fPasswordSyncChanged(FALSE),
      m_fUserNameChanged(FALSE),
      m_fPasswordSyncMsgShown(FALSE),
      m_fChanged(FALSE),
      m_fInDomain(TRUE),
      m_fHasPassport(TRUE)
{
#if 0 // Class Wizard happy
    //{{AFX_DATA_INIT(CAuthenticationDlg)
    m_fClearText = FALSE;
    m_fDigest = FALSE;
    m_fChallengeResponse = FALSE;
    m_fAnonymous = FALSE;
    //}}AFX_DATA_INIT

#endif // 0

    m_fClearText = IS_FLAG_SET(m_dwAuthFlags, MD_AUTH_BASIC);
    m_fDigest = IS_FLAG_SET(m_dwAuthFlags, MD_AUTH_MD5);
    m_fChallengeResponse = IS_FLAG_SET(m_dwAuthFlags, MD_AUTH_NT);
    m_fAnonymous = IS_FLAG_SET(m_dwAuthFlags, MD_AUTH_ANONYMOUS);
    m_fPassport = IS_FLAG_SET(m_dwAuthFlags, MD_AUTH_PASSPORT);
}


void 
CAuthenticationDlg::DoDataExchange(
    IN CDataExchange * pDX
    )
{
	if (pDX->m_bSaveAndValidate && !m_fChanged)
    {
       return;
    }
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CAuthenticationDlg)
    DDX_Control(pDX, IDC_CHECK_ANONYMOUS, m_check_Anonymous);
    DDX_Check(pDX, IDC_CHECK_ANONYMOUS, m_fAnonymous);
    DDX_Control(pDX, IDC_EDIT_USERNAME, m_edit_UserName);
    DDX_Control(pDX, IDC_EDIT_PASSWORD, m_edit_Password);
    DDX_Check(pDX, IDC_CHECK_ENABLE_PW_SYNCHRONIZATION, m_fPasswordSync);
    DDX_Control(pDX, IDC_CHECK_ENABLE_PW_SYNCHRONIZATION, m_chk_PasswordSync);
    DDX_Check(pDX, IDC_CHECK_CLEAR_TEXT, m_fClearText);
    DDX_Check(pDX, IDC_CHECK_DIGEST, m_fDigest);
    DDX_Check(pDX, IDC_CHECK_NT_CHALLENGE_RESPONSE, m_fChallengeResponse);
    DDX_Check(pDX, IDC_PASSPORT, m_fPassport);
    DDX_Control(pDX, IDC_CHECK_NT_CHALLENGE_RESPONSE, m_check_ChallengeResponse);
    DDX_Control(pDX, IDC_CHECK_DIGEST, m_check_Digest);
    DDX_Control(pDX, IDC_CHECK_CLEAR_TEXT, m_check_ClearText);
    DDX_Control(pDX, IDC_BASDOM, m_edit_BasicDomain);
    DDX_Control(pDX, IDC_BASDOM_SELECT, m_btn_SelectDomain);
    DDX_Control(pDX, IDC_REALM, m_edit_Realm);
    DDX_Control(pDX, IDC_REALM_SELECT, m_btn_SelectRealm);
    DDX_Control(pDX, IDC_PASSPORT, m_chk_Passport);
    //}}AFX_DATA_MAP
    DDX_Text(pDX, IDC_EDIT_USERNAME, m_strUserName);
    DDV_MinMaxChars(pDX, m_strUserName, 1, UNLEN);
    DDX_Text(pDX, IDC_BASDOM, m_strBasicDomain);
    DDX_Text(pDX, IDC_REALM, m_strRealm);
    //
    // Some people have a tendency to add "\\" before
    // the computer name in user accounts.  Fix this here.
    //
    m_strUserName.TrimLeft();
    while (*m_strUserName == '\\')
    {
        m_strUserName = m_strUserName.Mid(2);
    }

    //
    // Display the remote password sync message if
    // password sync is on, the account is not local,
    // password sync has changed or username has changed
    // and the message hasn't already be shown.
    //
	if (pDX->m_bSaveAndValidate)
	{
		BOOL bLocal;
		CString user, domain;
		CError err = CredUIParseUserName(
				m_strUserName, 
				user.GetBuffer(CRED_MAX_USERNAME_LENGTH), CRED_MAX_USERNAME_LENGTH,
				domain.GetBuffer(MAX_PATH), MAX_PATH);
		user.ReleaseBuffer();
		domain.ReleaseBuffer();
		bLocal = domain.IsEmpty() || domain.CompareNoCase(m_strServerName) == 0;
		if (m_dwVersionMajor < 6)
		{
			if (m_fPasswordSync && !bLocal 
				&& (m_fPasswordSyncChanged || m_fUserNameChanged)
				&& !m_fPasswordSyncMsgShown
				)
			{
				//
				// Don't show it again
				//
				m_fPasswordSyncMsgShown = TRUE;
				if (IDYES != ::AfxMessageBox(IDS_WRN_PWSYNC, MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION ))
				{
					pDX->Fail();
				}
			}
		}

        // See if we need to get the new password...
        if (m_fAnonymous)
        {
            // only save password/and ask for password confirmation
            // if anonymous is enabled.
            DDX_Password_SecuredString(pDX, IDC_EDIT_PASSWORD, m_strPassword, g_lpszDummyPassword);
        }

		// Convert to standard domain\user format
		if (!bLocal)
		{
			m_strUserName = domain;
			m_strUserName += _T('\\');
			m_strUserName += user;
		}
	}
	else
	{
        DDX_Password_SecuredString(pDX, IDC_EDIT_PASSWORD, m_strPassword, g_lpszDummyPassword);
	}

	if (!m_fPasswordSync)
	{
        DDV_MaxCharsBalloon_SecuredString(pDX, m_strPassword, PWLEN);
	}

	if (pDX->m_bSaveAndValidate)
	{
		m_fChanged = FALSE;
	}

    //CString csTempPassword;m_strPassword.CopyTo(csTempPassword);
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CAuthenticationDlg, CDialog)
    //{{AFX_MSG_MAP(CAuthenticationDlg)
    ON_BN_CLICKED(IDC_CHECK_ANONYMOUS, OnCheckAnonymous)
    ON_BN_CLICKED(IDC_BUTTON_BROWSE_USERS, OnButtonBrowseUsers)
    ON_BN_CLICKED(IDC_CHECK_ENABLE_PW_SYNCHRONIZATION, OnCheckEnablePwSynchronization)
    ON_EN_CHANGE(IDC_EDIT_USERNAME, OnChangeEditUsername)
    ON_BN_CLICKED(IDC_CHECK_CLEAR_TEXT, OnCheckClearText)
    ON_BN_CLICKED(IDC_CHECK_DIGEST, OnCheckDigest)
    ON_BN_CLICKED(IDC_CHECK_NT_CHALLENGE_RESPONSE, OnItemChanged)
    ON_BN_CLICKED(IDC_BASDOM_SELECT, OnButtonSelectDomain)
    ON_BN_CLICKED(IDC_REALM_SELECT, OnButtonSelectRealm)
    ON_BN_CLICKED(IDC_PASSPORT, OnCheckPassport)
    //}}AFX_MSG_MAP
    ON_EN_CHANGE(IDC_EDIT_PASSWORD, OnItemChanged)
    ON_EN_CHANGE(IDC_EDIT_DOMAIN_NAME, OnItemChanged)
    ON_EN_CHANGE(IDC_BASDOM, OnItemChanged)
    ON_EN_CHANGE(IDC_REALM, OnItemChanged)
END_MESSAGE_MAP()

void
CAuthenticationDlg::OnItemChanged()
{
	m_fChanged = TRUE;
}

void
CAuthenticationDlg::SetControlStates()
/*++

Routine Description:
    Set control states depending on current data in the dialog

--*/
{
    m_edit_UserName.EnableWindow(m_fAnonymous);
    GetDlgItem(IDC_BUTTON_BROWSE_USERS)->EnableWindow(m_fAnonymous);
    m_chk_PasswordSync.EnableWindow(m_dwVersionMajor < 6 && m_fAnonymous);
	m_edit_Password.EnableWindow((m_dwVersionMajor >= 6 || !m_fPasswordSync) && m_fAnonymous);
    // Windows
    m_check_ChallengeResponse.EnableWindow(!m_fPassport);
    // Basic
    m_check_ClearText.EnableWindow(!m_fPassport);
    // Digest
    m_check_Digest.EnableWindow(m_fHasDigest && !m_fPassport);
    // disable both domain fields if nothing is selected
    GetDlgItem(IDC_STATIC_DOMAIN)->EnableWindow(FALSE);
    m_edit_BasicDomain.EnableWindow(FALSE);
    m_btn_SelectDomain.EnableWindow(FALSE);
    GetDlgItem(IDC_STATIC_REALM)->EnableWindow(FALSE);
    m_edit_Realm.EnableWindow(FALSE);
    m_btn_SelectRealm.EnableWindow(FALSE);

    if (m_fPassport)
    {
        GetDlgItem(IDC_STATIC_DOMAIN)->EnableWindow(TRUE);
        m_edit_BasicDomain.EnableWindow(TRUE);
        m_btn_SelectDomain.EnableWindow(m_fInDomain);
    }
    else
    {
        if (m_fDigest && !m_fClearText)
        {
            GetDlgItem(IDC_STATIC_REALM)->EnableWindow(TRUE);
            m_edit_Realm.EnableWindow(TRUE);
            m_btn_SelectRealm.EnableWindow(m_fInDomain);
        }
        else if (m_fClearText)
        {
            GetDlgItem(IDC_STATIC_REALM)->EnableWindow(TRUE);
            m_edit_Realm.EnableWindow(TRUE);
            m_btn_SelectRealm.EnableWindow(m_fInDomain);

            GetDlgItem(IDC_STATIC_DOMAIN)->EnableWindow(TRUE);
            m_edit_BasicDomain.EnableWindow(TRUE);
            m_btn_SelectDomain.EnableWindow(m_fInDomain);
        }
    }
}

BOOL 
CAuthenticationDlg::OnInitDialog() 
{
    CDialog::OnInitDialog();
    
    // Check if computer is joined to domain
    COMPUTER_NAME_FORMAT fmt = ComputerNamePhysicalDnsDomain;
    TCHAR buf[MAX_PATH];
    DWORD n = MAX_PATH;
    m_fInDomain = (GetComputerNameEx(fmt, buf, &n) && n > 0);

    SetControlStates();  

    if (m_dwVersionMajor < 6)
    {
		// Show/Hide Passport stuff
        m_chk_Passport.EnableWindow(FALSE);
	}
	else
	{
		// Hide password syncronization
		GetDlgItem(IDC_CHECK_ENABLE_PW_SYNCHRONIZATION)->EnableWindow(FALSE);
		GetDlgItem(IDC_CHECK_ENABLE_PW_SYNCHRONIZATION)->ShowWindow(SW_HIDE);
		m_fPasswordSync = FALSE;
		m_fPasswordSyncChanged = FALSE;
		m_fPasswordSyncMsgShown = TRUE;
    }

    return TRUE;  
}

void 
CAuthenticationDlg::OnButtonBrowseUsers()
{
    CString str;

    if (GetIUsrAccount(m_strServerName, this, str))
    {
        //
        // If the name is non-local (determined by having
        // a slash in the name, password sync is disabled,
        // and a password should be entered.
        //
        m_edit_UserName.SetWindowText(str);
	    CString user, domain;
		CError err = CredUIParseUserName(str, 
			  user.GetBuffer(CRED_MAX_USERNAME_LENGTH), CRED_MAX_USERNAME_LENGTH,
			  domain.GetBuffer(MAX_PATH), MAX_PATH);
		user.ReleaseBuffer();
		domain.ReleaseBuffer();
		if (m_dwVersionMajor < 6)
		{
			m_fPasswordSync = 
				domain.IsEmpty() || domain.CompareNoCase(m_strServerName) == 0;
			m_chk_PasswordSync.SetCheck(m_fPasswordSync);
		}
		if (!m_fPasswordSync)
		{
			m_edit_Password.SetWindowText(_T(""));
			m_edit_Password.SetFocus();
		}
        OnItemChanged();
    }
}

void CAuthenticationDlg::OnButtonSelectDomain()
{
    HRESULT hr = BrowseDomain(m_strBasicDomain);
    if (SUCCEEDED(hr))
    {
        UpdateData(FALSE);
        OnItemChanged();
    }
}

void CAuthenticationDlg::OnButtonSelectRealm()
{
    HRESULT hr = BrowseDomain(m_strRealm);
    if (SUCCEEDED(hr))
    {
        UpdateData(FALSE);
        OnItemChanged();
    }
}

HRESULT
CAuthenticationDlg::BrowseDomain(CString& domain)
{
   CString prev = domain;
   CComPtr<IDsBrowseDomainTree> spDsDomains;

   CError err = ::CoCreateInstance(CLSID_DsDomainTreeBrowser,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IDsBrowseDomainTree,
                          reinterpret_cast<void **>(&spDsDomains));
   if (err.Succeeded())
   {
      err = spDsDomains->SetComputer(m_strServerName, NULL, NULL); // use default credential
      if (err.Succeeded())
      {
         LPTSTR pDomainPath = NULL;
         err = spDsDomains->BrowseTo(m_hWnd, &pDomainPath, 
            /*DBDTF_RETURNINOUTBOUND |*/ DBDTF_RETURNEXTERNAL | DBDTF_RETURNMIXEDDOMAINS);
         if (err.Succeeded() && pDomainPath != NULL)
         {
            domain = pDomainPath;
            if (domain.CompareNoCase(prev) != 0)
            {
				OnItemChanged();
            }
            CoTaskMemFree(pDomainPath);
         }
// When user click on Cancel in this browser, it returns 80070001 (Incorrect function). 
// I am not quite sure what does it mean. We are filtering out the case when domain browser doesn't
// work at all (in workgroup), so here we could safely skip error processing.
//         else
//         {
//            err.MessageBox();
//         }
      }
   }
   return err;
}

void
CAuthenticationDlg::OnCheckEnablePwSynchronization()
{
    m_fPasswordSyncChanged = TRUE;
    m_fPasswordSync = !m_fPasswordSync;
    OnItemChanged();
    SetControlStates();
    if (!m_fPasswordSync )
    {
        m_edit_Password.SetSel(0,-1);
        m_edit_Password.SetFocus();
    }
}

void 
CAuthenticationDlg::OnChangeEditUsername() 
{
    m_fUserNameChanged = TRUE;
    OnItemChanged();
}

void 
CAuthenticationDlg::OnCheckClearText() 
{
    if (m_check_ClearText.GetCheck() == 1)
    {
        CClearTxtDlg dlg;
        if (dlg.DoModal() != IDOK)
        {
            m_check_ClearText.SetCheck(0);
            return;
        }
    }

    m_fClearText = !m_fClearText;
    OnItemChanged();
    SetControlStates();
}



void 
CAuthenticationDlg::OnCheckDigest() 
{
    ASSERT(m_fHasDigest);

    if (m_check_Digest.GetCheck() == 1)
    {
		CString cap, msg;
		msg.LoadString(IDS_WRN_DIGEST);
		cap.LoadString(IDS_APP_NAME);
		if (IDNO == IisMessageBox(m_hWnd, IDS_WRN_DIGEST, MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2, HIDD_DOMAINACCTS))
        {
            m_check_Digest.SetCheck(0);
            return;
        }
    }

    m_fDigest = !m_fDigest;
    OnItemChanged();
    SetControlStates();
}



void 
CAuthenticationDlg::OnCheckAnonymous() 
{
    m_fAnonymous = !m_fAnonymous;
    OnItemChanged();
    SetControlStates();
}

void
CAuthenticationDlg::OnCheckPassport()
{
    m_fPassport = !m_fPassport;
    OnItemChanged();
    SetControlStates();
}

void 
CAuthenticationDlg::OnOK() 
{
    if (UpdateData(TRUE))
    {
        SET_FLAG_IF(m_fPassport, m_dwAuthFlags, MD_AUTH_PASSPORT);
        SET_FLAG_IF(m_fClearText, m_dwAuthFlags, MD_AUTH_BASIC);
        SET_FLAG_IF(m_fChallengeResponse, m_dwAuthFlags, MD_AUTH_NT);
        SET_FLAG_IF(m_fAnonymous, m_dwAuthFlags, MD_AUTH_ANONYMOUS);
        SET_FLAG_IF(m_fDigest, m_dwAuthFlags, MD_AUTH_MD5);

        //
        // Provide warning if no authentication is selected
        //
        if (!m_dwAuthFlags 
         && !m_dwAccessPermissions 
         && !NoYesMessageBox(IDS_WRN_NO_AUTH)
           )
        {
            //
            // Don't dismiss the dialog
            //
            return;
        }

        CDialog::OnOK();
    }
}

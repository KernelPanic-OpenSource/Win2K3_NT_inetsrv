/*++

   Copyright    (c)    1994-2001    Microsoft Corporation

   Module  Name :
        mime.cpp

   Abstract:
        Mime mapping dialog

   Author:
        Ronald Meijer (ronaldm)
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:

--*/
#include "stdafx.h"
#include "common.h"
#include "resource.h"
#include "mime.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



CMimeEditDlg::CMimeEditDlg(
    IN CWnd * pParent OPTIONAL
    )
    : m_strExt(),
      m_strMime(),
      CDialog(CMimeEditDlg::IDD, pParent)
{
}

CMimeEditDlg::CMimeEditDlg(
    IN LPCTSTR lpstrExt,
    IN LPCTSTR lpstrMime,
    IN CWnd * pParent OPTIONAL
    )
    : m_strExt(lpstrExt),
      m_strMime(lpstrMime),
      CDialog(CMimeEditDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CMimeEditDlg)
    //}}AFX_DATA_INIT
}

void 
CMimeEditDlg::DoDataExchange(
    IN CDataExchange * pDX
    )
{
    CDialog::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CMimeEditDlg)
    DDX_Control(pDX, IDOK, m_button_Ok);
    DDX_Control(pDX, IDC_EDIT_MIME, m_edit_Mime);
    DDX_Control(pDX, IDC_EDIT_EXTENT, m_edit_Extent);
	DDX_Text(pDX, IDC_EDIT_MIME, m_strMime);
	DDV_MaxCharsBalloon(pDX, m_strMime, 100);
	DDX_Text(pDX, IDC_EDIT_EXTENT, m_strExt);
	DDV_MaxCharsBalloon(pDX, m_strExt, 100);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CMimeEditDlg, CDialog)
    ON_EN_CHANGE(IDC_EDIT_MIME,  OnItemChanged)
    ON_EN_CHANGE(IDC_EDIT_EXTENT, OnItemChanged)
END_MESSAGE_MAP()

void 
CMimeEditDlg::SetControlStates()
{
    m_button_Ok.EnableWindow(
        m_edit_Extent.GetWindowTextLength() > 0
     && m_edit_Mime.GetWindowTextLength() > 0
        );
}

void 
CMimeEditDlg::OnItemChanged()
{
    SetControlStates();
}

BOOL 
CMimeEditDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    m_edit_Extent.SetWindowText(m_strExt);
    m_edit_Mime.SetWindowText(m_strMime);

    SetControlStates();
    
    return TRUE;
}

void 
CMimeEditDlg::OnOK()
{
    CDialog::OnOK();
    CleanExtension(m_strExt);
}

CMimeDlg::CMimeDlg(
    IN CStringListEx & strlMimeTypes,
    IN CWnd * pParent               OPTIONAL
    )
/*++

Routine Description:

    Constructor for the MIME listing dialog

Arguments:

    CStringListEx & strlMimeTypes : Listing of mime types to edit
    CWnd * pParent                : Optional parent window or NULL

Return Value:

    N/A

--*/
    : m_fDirty(FALSE),
      m_strlMimeTypes(strlMimeTypes),
      CDialog(CMimeDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CMimeDlg)
    //}}AFX_DATA_INIT
}



void
CMimeDlg::DoDataExchange(
    IN OUT CDataExchange * pDX
    )
/*++

Routine Description:

    Initialise/Store control data

Arguments:

    CDataExchange * pDX - DDX/DDV control structure

Return Value:

    None

--*/

{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CMimeDlg)
    DDX_Control(pDX, IDOK, m_button_Ok);
    DDX_Control(pDX, IDC_EDIT_EXTENSION, m_edit_Extention);
    DDX_Control(pDX, IDC_EDIT_CONTENT_TYPE, m_edit_ContentType);
    DDX_Control(pDX, IDC_BUTTON_REMOVE_MIME, m_button_Remove);
    DDX_Control(pDX, IDC_BUTTON_EDIT_MIME, m_button_Edit);
    //}}AFX_DATA_MAP

    DDX_Control(pDX, IDC_LIST_MIME_TYPES, m_list_MimeTypes);
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CMimeDlg, CDialog)
    //{{AFX_MSG_MAP(CMimeDlg)
    ON_BN_CLICKED(IDC_BUTTON_EDIT_MIME, OnButtonEdit)
    ON_BN_CLICKED(IDC_BUTTON_NEW_TYPE, OnButtonNewType)
    ON_BN_CLICKED(IDC_BUTTON_REMOVE_MIME, OnButtonRemove)
    ON_LBN_DBLCLK(IDC_LIST_MIME_TYPES, OnDblclkListMimeTypes)
    ON_LBN_SELCHANGE(IDC_LIST_MIME_TYPES, OnSelchangeListMimeTypes)
    //}}AFX_MSG_MAP

    ON_EN_CHANGE(IDC_EDIT_CONTENT_TYPE, OnItemChanged)
    ON_EN_CHANGE(IDC_EDIT_EXTENSION, OnItemChanged)
    ON_COMMAND(ID_HELP, OnHelp)

END_MESSAGE_MAP()



void 
CMimeDlg::SetControlStates()
/*++

Routine Description:

    Enable/disable controls depending on current dialog data

Arguments:

    None

Return Value:

    None

--*/
{
    m_button_Remove.EnableWindow(m_list_MimeTypes.GetSelCount() > 0);
    m_button_Edit.EnableWindow(m_list_MimeTypes.GetSelCount() == 1);
    m_button_Ok.EnableWindow(m_fDirty);
}



BOOL
CMimeDlg::BuildDisplayString(
    IN  CString & strIn,
    OUT CString & strOut
    )
/*++

Routine Description:

    Build a listbox-suitable display string for the mime type

Arguments:

    CString & strIn     : Input string in metabase format
    CString & strOut    : Output string in display format

Return Value:

    TRUE if successfull, FALSE otherwise

--*/
{
    BOOL fSuccess = FALSE;

    int nComma = strIn.Find(_T(','));
    if (nComma >= 0)
    {
        CString strExt = strIn.Left(nComma);
        CString strMime = strIn.Mid(nComma + 1);

        try
        {
            BuildDisplayString(strExt, strMime, strOut);
            ++fSuccess;
        }
        catch(CMemoryException * e)
        {
            TRACEEOLID("Mem exception in BuildDisplayString");
            e->ReportError();
            e->Delete();
        }
    }

    return fSuccess;
}



BOOL
CMimeDlg::CrackDisplayString(
    IN  CString & strIn,
    OUT CString & strExt,
    OUT CString & strMime
    )
/*++

Routine Description:

    Parse a display-formatted mime mapping string, and break into
    component parts

Arguments:

    CString & strIn     : Input string in display format
    CString & strExt    : Output extension string
    CString & strMime   : Output MIME string.


Return Value:

    TRUE if successfull, FALSE otherwise

--*/
{
    BOOL fSuccess = FALSE;

    try
    {
        int nTab = strIn.Find(_T('\t'));
        if (nTab >= 0)
        {
            strExt = strIn.Left(nTab);
            strMime = strIn.Mid(nTab + 1);

            ++fSuccess;
        }
    }
    catch(CMemoryException * e)
    {
        TRACEEOLID("Mem exception in CrackDisplayString");
        e->ReportError();
        e->Delete();
    }

    return fSuccess;
}



int
CMimeDlg::FindMimeType(
    IN const CString & strTargetExt
    )
/*++

Routine Description:

    Find a mime type by its extention.  The return value
    is the listbox index where the item may be found, or
    -1 if the item doesn't exist

Arguments:

    const CString & strTargetExt : Target extension we're searching for

Return Value:

    The index of the MIME mapping for this extension if found, or -1
    otherwise.

--*/
{
    CString str;
    CString strExt;
    CString strMime;

    //
    // CODEWORK: Change to binsearch 
    //
    for (int n = 0; n < m_list_MimeTypes.GetCount(); ++n)
    {
        m_list_MimeTypes.GetText(n, str);    
        if (CrackDisplayString(str, strExt, strMime))
        {
            if (!strExt.CompareNoCase(strTargetExt))
            {
                //
                // Found it.
                //
                return n;
            }
        }        
    }

    //
    // Not found
    //
    return -1;
}



void 
CMimeDlg::FillListBox()
/*++

Routine Description:

    Move the mime mappings from the string list to
    the listbox

Arguments:

    None.

Return Value:

    None.

--*/
{
    BeginWaitCursor();

    POSITION pos = m_strlMimeTypes.GetHeadPosition();

    while(pos)
    {
        CString & str = m_strlMimeTypes.GetNext(pos);
        CString strOut;

        if (BuildDisplayString(str, strOut))
        {
            m_list_MimeTypes.AddString(strOut);
        }
    }

    EndWaitCursor();
}



void 
CMimeDlg::FillFromListBox()
/*++

Routine Description:

    Reverse the above; Move the contents of the listbox
    back to the stringlist

Arguments:

    None.

Return Value:

    None.

--*/

{
    CString str;
    CString strExt;
    CString strMime;

    BeginWaitCursor();

    m_strlMimeTypes.RemoveAll();

    for (int n = 0; n < m_list_MimeTypes.GetCount(); ++n)
    {
        m_list_MimeTypes.GetText(n, str);    
        if (CrackDisplayString(str, strExt, strMime))
        {
            BuildMetaString(strExt, strMime, str);
            m_strlMimeTypes.AddTail(str);
        }
    }

    EndWaitCursor();
}



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



BOOL 
CMimeDlg::OnInitDialog() 
{
    CDialog::OnInitDialog();

    m_list_MimeTypes.Initialize();

    FillListBox();
    SetControlStates(); 
    
	GetDlgItem(IDC_BUTTON_NEW_TYPE)->SetFocus();

    return FALSE;
}



void 
CMimeDlg::OnButtonEdit()
{
    int nCurSel = m_list_MimeTypes.GetCurSel();

    if (nCurSel >= 0)
    {
        CString str;
        CString strExt;
        CString strMime;

        m_list_MimeTypes.GetText(nCurSel, str);

        if (CrackDisplayString(str, strExt, strMime))
        {
            CMimeEditDlg dlg(strExt, strMime, this);

            if (dlg.DoModal() == IDOK)
            {
                strExt = dlg.m_strExt;
                strMime = dlg.m_strMime;

                BuildDisplayString(strExt, strMime, str);
                m_list_MimeTypes.DeleteString(nCurSel);
                nCurSel = m_list_MimeTypes.AddString(str);
                m_list_MimeTypes.SetCurSel(nCurSel);
                m_fDirty = TRUE;

                OnSelchangeListMimeTypes();        
            }
        }
    }
}



void 
CMimeDlg::OnButtonNewType() 
/*++

Routine Description:

    'New' button has been pressed.  Create new MIME mapping, and
    bring up configuration on it.

Arguments:

    None.

Return Value:

    None.

--*/
{
    CMimeEditDlg dlg(this);

    if (dlg.DoModal() == IDOK)
    {
        CString str;
        CString strExt = dlg.m_strExt;
        CString strMime = dlg.m_strMime;

        //
        // Check to see if this extension already existed
        // in the list
        //
        int nOldSel = FindMimeType(strExt);
        if (nOldSel >= 0)
        {
            //
            // Yes, ask to have it replaced
            //
            if (::AfxMessageBox(IDS_REPLACE_MIME, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2
                ) == IDYES)
            {
                //
                // Kill the old one
                //
                m_list_MimeTypes.DeleteString(nOldSel);
            }
            else
            {
                //
                // Nope..
                //
                return;    
            }
        }

        BuildDisplayString(strExt, strMime, str);
        int nCurSel = m_list_MimeTypes.AddString(str);
        m_list_MimeTypes.SetCurSel(nCurSel);
        m_fDirty = TRUE;

        OnSelchangeListMimeTypes();
    }
}

void 
CMimeDlg::OnButtonRemove()
{
    if (::AfxMessageBox(IDS_REMOVE_MIME, 
        MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2
        ) != IDYES)
    {
        //
        // Changed his mind
        //
        return;
    }

    int nCurSel = m_list_MimeTypes.GetCurSel();

    int nSel = 0;
    while (nSel < m_list_MimeTypes.GetCount())
    {
        if (m_list_MimeTypes.GetSel(nSel))
        {
            m_list_MimeTypes.DeleteString(nSel);   
            m_fDirty = TRUE;
            continue;
        }

        ++nSel;
    }

    if (m_fDirty)
    {
        if (nCurSel > 0)
        {
            --nCurSel;
        }
        m_list_MimeTypes.SetCurSel(nCurSel);
        OnSelchangeListMimeTypes();
    }

    if (m_list_MimeTypes.GetCount() == 0)
    {
        GetDlgItem(IDC_BUTTON_NEW_TYPE)->SetFocus();
    }
}

void 
CMimeDlg::OnItemChanged()
{
    SetControlStates();
}

void 
CMimeDlg::OnDblclkListMimeTypes()
{
    OnButtonEdit();
}

void
CMimeDlg::OnSelchangeListMimeTypes() 
{
    //
    // Update the text in the description box
    //
    int nCurSel = m_list_MimeTypes.GetCurSel();
    
    if (nCurSel >= 0)
    {
        CString str;
        CString strExt;
        CString strMime;

        m_list_MimeTypes.GetText(nCurSel, str);

        if (CrackDisplayString(str, strExt, strMime))
        {
            m_edit_Extention.SetWindowText(strExt);
            m_edit_ContentType.SetWindowText(strMime);
        }
    }
    else
    {
        m_edit_Extention.SetWindowText(_T(""));
        m_edit_ContentType.SetWindowText(_T(""));
    }

    SetControlStates();
}

void 
CMimeDlg::OnOK()
{
    if (m_fDirty)
    {
        FillFromListBox();
    }

    CDialog::OnOK();
}

void
CMimeDlg::OnHelp()
{
}
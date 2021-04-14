//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

   globals.cpp

Abstract:

   Implementation of various utility routines

Author:

    RaphiR


--*/
//////////////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include "mqutil.h"
#include "resource.h"
#include "globals.h"
#include "tr.h"
#include "Fn.h"
#include "Ntdsapi.h"
#include "mqcast.h"
#include "localutl.h"
#include "mqppage.h"
#include "infodlg.h"
#include "ldaputl.h"
#include <clusapi.h>
#include <resapi.h>
#include "autorel3.h"

#include "globals.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

int CALLBACK SortByString(LPARAM, LPARAM, LPARAM){ return 1;}
int CALLBACK SortByULONG(LPARAM, LPARAM, LPARAM){return 1;}
int CALLBACK SortByINT(LPARAM, LPARAM, LPARAM){return 1;}
int CALLBACK SortByCreateTime(LPARAM, LPARAM, LPARAM){return 1;}
int CALLBACK SortByModifyTime(LPARAM, LPARAM, LPARAM){return 1;}


//
// Declaration of Propvariant Handlers
//
VTUI1Handler        g_VTUI1;
VTUI2Handler        g_VTUI2;
VTUI4Handler        g_VTUI4;
VTUI8Handler        g_VTUI8;
VTLPWSTRHandler     g_VTLPWSTR;
VTCLSIDHandler      g_VTCLSID;
VTVectUI1Handler    g_VectUI1;
VTVectLPWSTRHandler g_VectLPWSTR;
//
// Global default column width
//
int g_dwGlobalWidth = 150;

//
// Static text
//
CString s_finishTxt;

//
// static functions
//
static BOOL MQErrorToString(CString &str, HRESULT err);

//
// CErrorCapture
//
CString *CErrorCapture::ms_pstrCurrentErrorBuffer = 0;


extern HMODULE	    g_hResourceMod;


LPWSTR newwcs(LPCWSTR p)
{
    if(p == 0)
        return 0;

    LPWSTR dup = new WCHAR[wcslen(p) + 1];
    return wcscpy(dup, p);
}


/*====================================================

EnumToString

Scans a list of EnumItems, and return in <str> the
string corresponding to a value <dwVal>

=====================================================*/
void EnumToString(DWORD dwVal, EnumItem * pEnumList, DWORD dwListSize, CString & str)
{
    DWORD i=0;

    for(i = 0; i < dwListSize; i++)
    {
        if(pEnumList->val == dwVal)
        {
              str.LoadString(pEnumList->StringId);
              return;
        }
        pEnumList++;
    }


    //
    // Value not found
    //
    str.LoadString(IDS_UNKNOWN);
}


/*====================================================

TimeToString

Gets a PROPVARIANT datevalue, and return it as a string

=====================================================*/
void CALLBACK TimeToString(const PROPVARIANT *pPropVar, CString &str)
{
    ASSERT(pPropVar->vt == VT_UI4);

	//
	// If time is not set, do not display it.
	// Converting time that is not set will show dd/mm/1970 date
	//
    if ( pPropVar->ulVal == 0 )
	{
		str = L"";
		return;
	}

	BOOL fShort = 0;
    TCHAR bufDate[128], bufTime[128];
    SYSTEMTIME sysTime;
	DWORD dwDateFormat;
	DWORD dwTimeFormat;
    CTime time(pPropVar->ulVal);

    sysTime.wYear = (WORD)time.GetYear();
    sysTime.wMonth = (WORD)time.GetMonth();
    sysTime.wDayOfWeek = (WORD)time.GetDayOfWeek();
    sysTime.wDay = (WORD)time.GetDay();
    sysTime.wHour = (WORD)time.GetHour();
    sysTime.wMinute = (WORD)time.GetMinute();
    sysTime.wSecond = (WORD)time.GetSecond();
    sysTime.wMilliseconds = 0;

   if(fShort)
   {
      dwDateFormat = DATE_SHORTDATE;
      dwTimeFormat = TIME_NOSECONDS;
   }
   else
   {
      dwDateFormat = DATE_LONGDATE;
      dwTimeFormat = 0;
   }

    GetDateFormat(
        LOCALE_USER_DEFAULT,
        dwDateFormat,   // flags specifying function options
        &sysTime,       // date to be formatted
        0,              // date format string - zero means default for locale
        bufDate,        // buffer for storing formatted string
        TABLE_SIZE(bufDate) // size of buffer
        );

    GetTimeFormat(
        LOCALE_USER_DEFAULT,
        dwTimeFormat,   // flags specifying function options
        &sysTime,       // date to be formatted
        0,              // time format string - zero means default for locale
        bufTime,        // buffer for storing formatted string
        TABLE_SIZE(bufTime) // size of buffer
        );

    str = bufDate;
    str += " ";
    str += bufTime;

}


/*====================================================

BoolToString

Gets a PROPVARIANT boolean, and return a Yes/No string
accordingly

=====================================================*/
void CALLBACK BoolToString(const PROPVARIANT *pPropVar, CString &str)
{
   ASSERT(pPropVar->vt == VT_UI1);


   str.LoadString(pPropVar->bVal ? IDS_YES: IDS_NO);

}

/*====================================================

QuotaToString

Gets a PROPVARIANT qouta, returns a number or "Infinite"

=====================================================*/
void CALLBACK QuotaToString(const PROPVARIANT *pPropVar, CString &str)
{
   ASSERT(pPropVar->vt == VT_UI4);
   if (pPropVar->ulVal == INFINITE)
   {
       str.LoadString(IDS_INFINITE_QUOTA);
   }
   else
   {
       str.Format(TEXT("%d"), pPropVar->ulVal);
   }
}


/*====================================================

ItemDisplay

Gets an ItemDisplay entry, and call the appropriate
display function.
=====================================================*/
void ItemDisplay(const PropertyDisplayItem * pItem,PROPVARIANT * pPropVar, CString & szTmp)
{

    VTHandler * pvth = pItem->pvth;

    if(pItem->pfnDisplay == NULL)
    {
		//
		// Special treatment for VT_NULL
		//
		if(pPropVar->vt == VT_NULL)
		{
			szTmp = L"";
			return;
		}

        //
        // If no function defined, call the Propvariant handler
        //
        pvth->Display(pPropVar, szTmp);
    }
    else
    {
        //
        // Call the function defined
        //
        (pItem->pfnDisplay)(pPropVar, szTmp);
    }

}

/*====================================================

GetPropertyString

Given a property ID, and a array of Property display item,
returns a string of the property value
=====================================================*/
void GetPropertyString(const PropertyDisplayItem * pItem, PROPID pid, PROPVARIANT *pPropVar, CString & strResult)
{
    //
    // Scan the list
    //
    while(pItem->itemPid != 0)
    {
        if(pItem->itemPid == pid)
        {
            //
            // Property ID match, call the display function
            //
            ItemDisplay(pItem, pPropVar, strResult);
            return;
        }

        pItem++;
        pPropVar++;
    }

    ASSERT(0);

    return;


}

/*====================================================

GetProperyVar

Given a property ID, and a array of Property display item,
returns a the variant of the property value
=====================================================*/
void GetPropertyVar(const PropertyDisplayItem * pItem, PROPID pid, PROPVARIANT *pPropVar, PROPVARIANT ** ppResult)
{
    ASSERT(ppResult != NULL);
    ASSERT(pPropVar != NULL);
    //
    // Scan the list
    //
    while(pItem->itemPid != 0)
    {
        if(pItem->itemPid == pid)
        {
            //
            // Property ID match
            //
            *ppResult = pPropVar;
            return;
        }

        pItem++;
        pPropVar++;
    }

    ASSERT(0);
 }


//////////////////////////////////////////////////////////////////////////////
/*++

QueuePathnameToName

	Translates a pathname to a short name (display function for PropertyDisplayItem)

--*/
//////////////////////////////////////////////////////////////////////////////
void CALLBACK QueuePathnameToName(const PROPVARIANT *pPropVar, CString &str)
{
	if(pPropVar->vt != VT_LPWSTR)
	{
		str = L"";
		return;
	}

    ExtractQueueNameFromQueuePathName(str, pPropVar->pwszVal);

	return;
}


/*==============================================================

MessageDSError

  This function displays a DS error:
    Can not <operation> <object>.\n<error description>.
    "Can not %1 %2.\n%3."

Return Value:

================================================================*/
int
MessageDSError(
    HRESULT rc,                     //  Error code
    UINT nIDOperation,              //  Operation string identifier,
                                    //  e.g., get premissions, delete queue, etc.
    LPCTSTR pObjectName /* = 0*/,   //  object that operation performed on
    UINT nType /* = MB_OK | MB_ICONERROR */,
    UINT nIDHelp /* = (UINT) -1*/
    )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CString csErrorText;
    CString csPrompt;

    CString csOperation;
    AfxFormatString1(csOperation, nIDOperation, pObjectName);

    MQErrorToMessageString(csErrorText, rc);
    AfxFormatString2(csPrompt, IDS_DS_ERROR, csOperation, csErrorText);

    return CErrorCapture::DisplayError(csPrompt, nType, nIDHelp);
}


/*=======================================================

MQErrorToString

  Translate an MQError to a string

========================================================*/
BOOL MQErrorToString(CString &str, HRESULT err)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
    DWORD rc;
    TCHAR p[512];

    //
    // For MSMQ error code, we will take the message from MQUTIL.DLL based on the full
    // HRESULT. For Win32 error codes, we get the message from the system..
    // For other error codes, we assume they are DS error codes, and get the code
    // from ACTIVEDS dll.
    //

    DWORD dwErrorCode = err;
    HMODULE hLib = 0;
    DWORD dwFlags = FORMAT_MESSAGE_MAX_WIDTH_MASK;

    switch (HRESULT_FACILITY(err))
    {
        case FACILITY_MSMQ:
            dwFlags |= FORMAT_MESSAGE_FROM_HMODULE;
            hLib = g_hResourceMod;
            break;

        case FACILITY_NULL:
        case FACILITY_WIN32:
            dwFlags |= FORMAT_MESSAGE_FROM_SYSTEM;
            dwErrorCode = HRESULT_CODE(err);
            break;

        default:
            dwFlags |= FORMAT_MESSAGE_FROM_HMODULE;
            hLib = LoadLibrary(TEXT("ACTIVEDS.DLL"));
            break;
    }

    rc = FormatMessage( dwFlags,
                        hLib,
                        err,
                        0,
                        p,
                        TABLE_SIZE(p),
                        NULL);

    if (0 != hLib && g_hResourceMod != hLib)
    {
        FreeLibrary(hLib);
    }

    if(rc != 0)
    {
        str.FormatMessage(IDS_DISPLAY_ERROR_FORMAT, p);
    }

    return(rc);
}

/*==============================================================

MQErrorToMessageString

    Will set csMessage to an error message, displayable in a message box
================================================================*/
BOOL
MQErrorToMessageString(
    CString &csErrorText,          //  Returned message
    HRESULT rc                     //  Error code
    )
{
    if(!MQErrorToString(csErrorText,rc))
    {
        csErrorText.FormatMessage(IDS_UNKNOWN_ERROR_FORMAT, rc);
        return FALSE;
    }
    return TRUE;
}


/*==============================================================

DisplayErrorAndReason

    Will set strErrorText to an error message, displayable in a message box
================================================================*/
void
DisplayErrorAndReason(
	UINT uiErrorMsgProblem,
    UINT uiErrorMsgReason,
	CString strObject,
	HRESULT errorCode
	)
{
	CString strErrorProblem, strErrorReason;

	if ( strObject == L"" )
	{
		if(uiErrorMsgProblem == IDS_OP_CREATE_COMPUTER || uiErrorMsgProblem == IDS_CREATE_SITE_FAILED)
		{
			strErrorProblem.FormatMessage(uiErrorMsgProblem, L"\" \"");
		}
		else
		{
			strErrorProblem.LoadString(uiErrorMsgProblem);
		}
	}
	else
	{
		strErrorProblem.FormatMessage(uiErrorMsgProblem, strObject);
	}

	strErrorReason.LoadString(uiErrorMsgReason);
	strErrorProblem += (L"\n" + strErrorReason);

	if ( errorCode != 0 )
	{
		CString strErrorCode;
		strErrorCode.FormatMessage(IDS_ERROR_DW_FORMAT, errorCode);
		strErrorProblem += strErrorCode;
	}

	CErrorCapture::DisplayError(strErrorProblem, MB_OK | MB_ICONERROR, static_cast<UINT>(-1));
}


/*==============================================================

DisplayErrorFromCOM

    Will set strErrorText to an error message, displayable in a message box
================================================================*/
void
DisplayErrorFromCOM(
	UINT uiErrorMsg,
	const _com_error& e
	)
{ 

	CString strError;
	_bstr_t Description = e.Description();
	if ((LPCWSTR)Description == NULL)
	{
		strError.FormatMessage(uiErrorMsg, L"");
	} 
	else
	{
		WCHAR strDesc[256];
		wcscpy(strDesc, Description);		
		strError.FormatMessage(uiErrorMsg, strDesc);
	}

	AfxMessageBox(strError, MB_OK | MB_ICONERROR);
}


/*===================================================*
 *
 * DDX functions
 *
 *===================================================*/
//
// DDX function for numbers. When the number is INFINITE, the check box
// is not checked.
// (Good for queue qouta, for example)
//
void AFXAPI DDX_NumberOrInfinite(CDataExchange* pDX, int nIDCEdit, int nIDCCheck, DWORD& dwNumber)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HWND hWndCheck = pDX->PrepareCtrl(nIDCCheck);
    HWND hWndEdit = pDX->PrepareEditCtrl(nIDCEdit);

    if (pDX->m_bSaveAndValidate)
    {
        BOOL_PTR fChecked = SendMessage(hWndCheck, BM_GETCHECK, 0 ,0);
        if (!fChecked)
        {
            dwNumber = INFINITE;
        }
        else
        {
            //
            // Avoid negatives - MFC just translate them to positive
            // 2's complement numbers. We want to avoid this
            //
            int nLen = ::GetWindowTextLength(hWndEdit);
            CString strTemp;

            GetWindowText(hWndEdit, strTemp.GetBufferSetLength(nLen), nLen+1);
            strTemp.ReleaseBuffer();

            if (!(strTemp.SpanExcluding(x_tstrDigitsAndWhiteChars)).IsEmpty())
            {
                AfxMessageBox(AFX_IDP_PARSE_UINT);
                pDX->Fail();
            }

            DDX_Text(pDX, nIDCEdit, dwNumber);
            //
            // If number was out of range, DDX_Text returns Infinite
            //
            if (INFINITE == dwNumber)
            {
	            TCHAR szMax[32];
	            wsprintf(szMax, TEXT("%lu"), INFINITE-1);

	            CString prompt;
	            AfxFormatString2(prompt, AFX_IDP_PARSE_INT_RANGE, TEXT("0"), szMax);
	            AfxMessageBox(prompt, MB_ICONEXCLAMATION, AFX_IDP_PARSE_INT_RANGE);
	            prompt.Empty(); // exception prep
	            pDX->Fail();
            }
        }
    }
    else
    {
        if (INFINITE == dwNumber)
        {
            //
            // Note: The caller must handle BN_CLICKED to re-enable the edit box
            //
            SendMessage(hWndCheck, BM_SETCHECK, FALSE ,0);
            EnableWindow(hWndEdit, FALSE);
        }
        else
        {
            SendMessage(hWndCheck, BM_SETCHECK, TRUE ,0);
            DDX_Text(pDX, nIDCEdit, dwNumber);
        }
    }
}

void OnNumberOrInfiniteCheck(CWnd *pwnd, int idEdit, int idCheck)
{
    CEdit *pEdit = (CEdit*)pwnd->GetDlgItem(idEdit);
    CButton *pCheck = (CButton*)pwnd->GetDlgItem(idCheck);
    BOOL fChecked = pCheck->GetCheck();

    pEdit->EnableWindow(fChecked);
}


//------------- DDX_TEXT (for GUID) ---------------------------
// This function compliment the standard DDX_ function set, and
// used to exchange a a GUID property with an edit box
// in DoDataExchange.

void AFXAPI DDX_Text(CDataExchange* pDX, int nIDC, GUID& guid)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HWND hWndCtrl = pDX->PrepareEditCtrl(nIDC);

    if (pDX->m_bSaveAndValidate)
    {
        int nLen = ::GetWindowTextLength(hWndCtrl);
        CString strTemp;

        ::GetWindowText(hWndCtrl, strTemp.GetBufferSetLength(nLen), nLen+1);
        strTemp.ReleaseBuffer();

        if (IIDFromString((LPTSTR)((LPCTSTR)strTemp), &guid))   // throws exception
        {
            //
            // Can't convert string to guid
            //
            CString strMessage;
            strMessage.LoadString(IDE_INVALIDGUID);

            AfxMessageBox(strMessage);
            pDX->Fail();    // throws exception
        }
    }
    else
    {
        TCHAR szTemp[x_dwMaxGuidLength];
        StringFromGUID2(guid, szTemp, TABLE_SIZE(szTemp));
        ::SetWindowText(hWndCtrl, szTemp);
    }
}


/*====================================================
MqsnapCreateQueue
Creates a queue given a set of parameters and a list of properties IDs. The list must
contain some or all of PROPID_Q_PATHNAME, PROPID_Q_LABEL, PROPID_Q_TRANSACTION and
PROPID_Q_TYPE. Parameters that do not have a corresponding property ID in the list are
ignored.

Arguments:

Return Value:

=====================================================*/
HRESULT MqsnapCreateQueue(CString& strPathName, BOOL fTransactional,
                       CString& strLabel, GUID* pTypeGuid,
                       PROPID aProp[], UINT cProp,
                       CString *pStrFormatName /* = 0 */)
{
#define MAX_EXPCQ_PROPS 10

    ASSERT(cProp <= MAX_EXPCQ_PROPS);

    PROPVARIANT apVar[MAX_EXPCQ_PROPS];
    HRESULT hr = MQ_OK, hr1 = MQ_OK;
    UINT iProp;

    for (iProp = 0; iProp<cProp; iProp++)
    {
        switch (aProp[iProp])
        {
            case PROPID_Q_PATHNAME:
                apVar[iProp].vt = VT_LPWSTR;

                apVar[iProp].pwszVal = (LPTSTR)(LPCTSTR)(strPathName);
                break;

            case PROPID_Q_LABEL:
                apVar[iProp].vt = VT_LPWSTR;
                apVar[iProp].pwszVal = (LPTSTR)(LPCTSTR)(strLabel);
                break;

            case PROPID_Q_TYPE:
                apVar[iProp].vt = VT_CLSID;
                apVar[iProp].puuid = pTypeGuid;
                break;

            case PROPID_Q_TRANSACTION:
                apVar[iProp].vt = VT_UI1;
                apVar[iProp].bVal = (UCHAR)fTransactional;
                break;

            default:
                ASSERT(0);
                break;
        }

    }

    MQQUEUEPROPS mqp = {cProp, aProp, apVar, 0};

    DWORD dwFormatLen;
    if (0 != pStrFormatName)
    {
        dwFormatLen = MAX_QUEUE_FORMATNAME;
        hr = MQCreateQueue(0, &mqp, pStrFormatName->GetBuffer(MAX_QUEUE_FORMATNAME), &dwFormatLen);
        pStrFormatName->ReleaseBuffer();
    }
    else
    {
        dwFormatLen=0;
        if ((hr1 = MQCreateQueue(0, &mqp, 0, &dwFormatLen)) != MQ_INFORMATION_FORMATNAME_BUFFER_TOO_SMALL)
           hr = hr1;
    }

    return hr;
}

/*====================================================
CreateEmptyQueue


Arguments:

Return Value:

=====================================================*/
HRESULT CreateEmptyQueue(CString &csDSName,
                         BOOL fTransactional, CString &csMachineName, CString &csPathName,
                         CString *pStrFormatName /* = 0 */)
{
    PROPID  aProp[] = {PROPID_Q_PATHNAME,
                       PROPID_Q_LABEL,
                       PROPID_Q_TRANSACTION
                       };

    csPathName.Format(TEXT("%s\\%s"), csMachineName, csDSName);

    HRESULT hr =  MqsnapCreateQueue(csPathName, fTransactional, csDSName, 0,
                                    aProp, sizeof(aProp) / sizeof(aProp[0]), pStrFormatName);
    if (hr != MQ_ERROR_INVALID_OWNER)
    {
        //
        // Either success or real error. Return.
        //
        return hr;
    }

    CString csNetBiosName;
    if (!GetNetbiosName(csMachineName, csNetBiosName))
    {
        //
        // Already a netbios name. No need to proceed
        //
        return hr;
    }

    //
    // This may be an NT4 server, and we may be using a full DNS name. Try again with
    // Netbios name  (fix for 5076, YoelA, 16-Sep-99)
    //
    csPathName.Format(TEXT("%s\\%s"), csNetBiosName, csDSName);

    return MqsnapCreateQueue(csPathName, fTransactional, csDSName, 0,
                             aProp, sizeof(aProp) / sizeof(aProp[0]), pStrFormatName);
}

/*====================================================
CreateTypedQueue
  ( for report and test queues )

Arguments:

Return Value:

=====================================================*/

HRESULT CreateTypedQueue(CString& strPathname, CString& strLabel, GUID& TypeGuid)
{
    PROPID  aProp[] = {PROPID_Q_PATHNAME,
                       PROPID_Q_LABEL,
                       PROPID_Q_TYPE,
                       };

    return MqsnapCreateQueue(strPathname, FALSE, strLabel, &TypeGuid,
                          aProp, sizeof(aProp) / sizeof(aProp[0]));
}


/*==============================================================

CaubToString

  This function converts a CAUI1 buffer to a string displayable in the
  right hand side of an hex editor (every byte is a character, control characters
  are replaced by '.')
Arguments:

Return Value:

================================================================*/
void CaubToString(const CAUB* pcaub, CString& strResult)
{
    LPTSTR pstr = strResult.GetBuffer(pcaub->cElems);
    for (DWORD i=0; i<pcaub->cElems; i++)
    {
        //
        // Replace control characters with "."
        //
        if (pcaub->pElems[i]>=32)
        {
            pstr[i] = pcaub->pElems[i];
        }
        else
        {
            pstr[i] = L'.';
        }
    }
    strResult.ReleaseBuffer();
}

/*==============================================================

MoveSelected

  This function moves all the selected items from one list box to another.
  (Usefull for implementation of Add/Remove buttons)

Arguments:
  plbSource - pointer to source list box from which the selected item
              should be moved to the destination list box

  plbDest - pointer to the destination list box

Return Value:

================================================================*/
void
MoveSelected(
    CListBox* plbSource,
    CListBox* plbDest
    )
{
    try
    {
        int nTotalItems = plbSource->GetCount();
        AP<int> piRgIndex = new int[nTotalItems];
        int nSel =  plbSource->GetSelItems(nTotalItems, piRgIndex );
        int i;

        for (i=0; i<nSel; i++)
        {
            CString strItem;
            plbSource->GetText(piRgIndex[i], strItem);

            DWORD_PTR dwItemData = plbSource->GetItemData(piRgIndex[i]);

            int iDestIndex = plbDest->AddString(strItem);
            plbDest->SetItemData(iDestIndex, dwItemData);
        }

        for (i=0; i<nSel; i++)
        {
            plbSource->DeleteString(piRgIndex[i]-i);
        }
    }
    catch (CException* pException)
    {
        TrERROR(GENERAL, "Exception  2 in MoveSelected");
        pException->ReportError();
        pException->Delete();
    }
}

//
// Helper routine for handling LPWSTR map
//
template<>
BOOL
AFXAPI
CompareElements(
    const LPCWSTR* MapName1,
    const LPCWSTR* MapName2
    )
{
    return (wcscmp(*MapName1, *MapName2) == 0);
}


template<>
UINT
AFXAPI
HashKey(
    LPCWSTR key
    )
{
    UINT nHash = 0;
    while (*key)
    {
        nHash = (nHash<<5) + nHash + *key++;
    }

    return nHash;
}


/*==============================================================
DDV_NotEmpty

    The routine checks that the input is not empty

Arguments:
    pDX - pointer to Data exchange
    str - pointer to the selected string
    uiErrorMessage - Error message if the validation fails

Returned Value:
    None.

================================================================*/
void DDV_NotEmpty(
    CDataExchange* pDX,
    CString& str,
    UINT uiErrorMessage
    )
{
    if (pDX->m_bSaveAndValidate)
    {
        if (str.IsEmpty())
        {
            AfxMessageBox(uiErrorMessage);
            pDX->Fail();
        }
    }
}

//
// String comparison function for sorting queue nqmes
//
int __cdecl QSortCompareQueues( const void *arg1, const void *arg2 )
{
   /* Compare all of both strings: */
   return _wcsicmp( * (WCHAR ** ) arg1, * (WCHAR ** ) arg2 );
}


template<>
void AFXAPI DestructElements(PROPVARIANT *pElements, INT_PTR nCount)
{
    DWORD i,j;
    for (i=0; i<(DWORD)nCount; i++)
    {
        switch(pElements[i].vt)
        {
            case VT_CLSID:
                MQFreeMemory(pElements[i].puuid);
                break;

            case VT_LPWSTR:
                MQFreeMemory(pElements[i].pwszVal);
                break;

            case VT_BLOB:
                MQFreeMemory(pElements[i].blob.pBlobData);
                break;

            case (VT_VECTOR | VT_CLSID):
                MQFreeMemory(pElements[i].cauuid.pElems);
                break;

            case (VT_VECTOR | VT_LPWSTR):
                for(j = 0; j < pElements[i].calpwstr.cElems; j++)
                {
                    MQFreeMemory(pElements[i].calpwstr.pElems[j]);
                }
                MQFreeMemory(pElements[i].calpwstr.pElems);
                break;

            case (VT_VECTOR | VT_VARIANT):
                DestructElements((PROPVARIANT*)pElements[i].capropvar.pElems, pElements[i].capropvar.cElems);
                MQFreeMemory(pElements[i].calpwstr.pElems);
                break;
            default:
                break;
        }

        pElements[i].vt = VT_EMPTY;
    }
}

//
// CompareVariants - compare two variants
//
#define _COMPAREVARFIELD(field) (propvar1->field == propvar2->field ?  0 : \
                                    propvar1->field > propvar2->field ? 1 : -1)

int CompareVariants(PROPVARIANT *propvar1, PROPVARIANT *propvar2)
{
    if (propvar1->vt != propvar2->vt)
    {
        //
        // Can't compare. Consider them equal
        //
        return 0;
    }

    char *pChar1, *pChar2;
    size_t nChar1, nChar2;
    switch (propvar1->vt)
    {
        case VT_UI1:
            return _COMPAREVARFIELD(bVal);

        case VT_UI2:
        case VT_I2:
            return _COMPAREVARFIELD(iVal);

        case VT_UI4:
        case VT_I4:
            return _COMPAREVARFIELD(lVal);

        case VT_UI8:
            return _COMPAREVARFIELD(uhVal.QuadPart);

        case VT_CLSID:
        {
            for (int i = 0; i < sizeof(GUID); i++)
            {
                if (((BYTE*)propvar1->puuid)[i] > ((BYTE*)propvar2->puuid)[i])
                {
                    return 1;
                }
                else if (((BYTE*)propvar1->puuid)[i] < ((BYTE*)propvar2->puuid)[i])
                {
                    return -1;
                }
            }

            return 0;
        }

        case VT_LPWSTR:
        {
            int nCompResult = CompareString(0,0, propvar1->pwszVal, -1, propvar2->pwszVal, -1);
            switch(nCompResult)
            {
                case CSTR_LESS_THAN:
                    return -1;

                case CSTR_EQUAL:
                    return 0;

                case CSTR_GREATER_THAN:
                    return 1;
            }
        }

        case VT_BLOB:
            pChar1 = (char *)propvar1->blob.pBlobData;
            pChar2 = (char *)propvar2->blob.pBlobData;
            nChar1 = propvar1->blob.cbSize;
            nChar2 = propvar2->blob.cbSize;
            //
            // Fall through
            //
        case (VT_VECTOR | VT_UI1):
            if (propvar1->vt == (VT_VECTOR | VT_UI1)) // not fallen through
            {
                pChar1 = (char *)propvar1->caub.pElems;
                pChar2 = (char *)propvar2->caub.pElems;
                nChar1 = propvar1->caub.cElems;
                nChar2 = propvar2->caub.cElems;
            }
            {
                int iResult = memcmp(pChar1, pChar2, min (nChar1, nChar2));
                if (iResult == 0)
                {
                    if (nChar1 > nChar2)
                    {
                        return 1;
                    }
                    else if (nChar2 > nChar1)
                    {
                        return -1;
                    }
                    else
                    {
                        return 0;
                    }
                }
                else
                {
                    return iResult;
                }
            }

        case (VT_VECTOR | VT_CLSID):
        case (VT_VECTOR | VT_LPWSTR):
        case (VT_VECTOR | VT_VARIANT):
        default:
            //
            // can't compare these types - consider them equal
            //
            return 0;
    }
}
#undef _COMPAREVARFIELD

/*==============================================================
InsertColumnsFromDisplayList

This routine inserts columns into header using a display list.
It should be called from InsertColumns functions of classes derived from
CSnapinNode.
================================================================*/

HRESULT   InsertColumnsFromDisplayList(IHeaderCtrl* pHeaderCtrl,
                                       const PropertyDisplayItem *aDisplayList)
{
    DWORD i = 0;
    DWORD dwColId = 0;
    while(aDisplayList[i].itemPid != 0)
    {
        if (aDisplayList[i].uiStringID != NO_TITLE)
        {
            CString title;
            title.LoadString(aDisplayList[i].uiStringID);

            pHeaderCtrl->InsertColumn(dwColId, title, LVCFMT_LEFT,aDisplayList[i].iWidth);
            dwColId++;
        }
        i++;
    }

    return(S_OK);
}


//
// GetDsServer - returns the current DS Server
//
HRESULT GetDsServer(CString &strDsServer)
{
    MQMGMTPROPS	  mqProps;
    PROPID  propIdDsServ = PROPID_MGMT_MSMQ_DSSERVER;
    PROPVARIANT propVarDsServ;
    propVarDsServ.vt = VT_NULL;

	mqProps.cProp = 1;
	mqProps.aPropID = &propIdDsServ;
	mqProps.aPropVar = &propVarDsServ;
	mqProps.aStatus = NULL;

    HRESULT hr = MQMgmtGetInfo(0, MO_MACHINE_TOKEN, &mqProps);

    if(FAILED(hr))
    {
       return(hr);
    }

	ASSERT(propVarDsServ.vt == (VT_LPWSTR));

    strDsServer = propVarDsServ.pwszVal;
    MQFreeMemory(propVarDsServ.pwszVal);

    return S_OK;
}

//
// GetComputerNameIntoString - Puts the local computer name in a string
//
HRESULT GetComputerNameIntoString(CString &strComputerName)
{
    const DWORD x_dwComputerNameLen = 256;
    DWORD dwComputerNameLen = x_dwComputerNameLen;

    HRESULT hr = GetComputerNameInternal(strComputerName.GetBuffer(dwComputerNameLen), &dwComputerNameLen);
    strComputerName.ReleaseBuffer();

    return hr;
}

//
// GetSiteForeignFlag - retrieves a site's foreign flag from the DS
//
HRESULT
GetSiteForeignFlag(
    const GUID* pSiteId,
    BOOL *fForeign,
	BOOL fLocalMgmt,
	const CString& strDomainController
    )
{
    //
    // By default (and in case of error) return FALSE
    //
    *fForeign = FALSE;
    //
    // Get the site foreign flag
    //
    PROPID pid = PROPID_S_FOREIGN;
    PROPVARIANT var;
    var.vt = VT_NULL;
   
    HRESULT hr = ADGetObjectPropertiesGuid(
                    eSITE,
                    fLocalMgmt ? MachineDomain() : GetDomainController(strDomainController),
					fLocalMgmt ? false : true,	// fServerName
                    pSiteId,
                    1,
                    &pid,
                    &var
                    );
    if (FAILED(hr))
    {
        if (MQ_ERROR == hr)
        {
            //
            // In this case, we assume we are working against NT4 server.
            // There are no foreign sites in NT4, so we return FALSE in fForeign
            //
            return MQ_OK;
        }
        //
        // Another error - report error and return it.
        //
        CString strSite;
        strSite.LoadString(IDS_SITE);
        MessageDSError(hr, IDS_OP_GET_PROPERTIES_OF, strSite);
        return hr;
    }

    *fForeign = var.bVal;
    return MQ_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++
GetNetbiosName
This function gets a full DNS name and returns the Netbios name
return value - TRUE if Netbios name different than Full Dns Name
--*/
//////////////////////////////////////////////////////////////////////////////
BOOL GetNetbiosName(CString &strFullDnsName, CString &strNetbiosName)
{
    DWORD dwFirstDot = strFullDnsName.Find(L".");
    if (dwFirstDot == -1)
    {
        //
        // It is already a netbios name. Return it
        //
        strNetbiosName = strFullDnsName;
        return FALSE;
    }

    strNetbiosName = strFullDnsName.Left(dwFirstDot);
    return TRUE;
}

//
//  Temporary - until logging mechanism is decided
//  currently doesn't perform logging
//
void LogMsgHR(HRESULT /*hr*/, LPWSTR /*wszFileName*/, USHORT /*usPoint*/)
{
}

//
// Nedded for linking with fn.lib
//
WCHAR g_MachineName[MAX_COMPUTERNAME_LENGTH+1] = L"";
DWORD g_MachineNameLength = MAX_COMPUTERNAME_LENGTH+1;
static bool s_fMachineName = false;

LPCWSTR
McComputerName(
	void
	)
{
	if (s_fMachineName)
	{
		return g_MachineName;
	}

    HRESULT hr = GetComputerNameInternal( 
                     g_MachineName,
                     &g_MachineNameLength
                     );
    if(FAILED(hr))
    {
		TrERROR(GENERAL, "Failed to get computer name. Error: %!hresult!", hr);
		throw bad_hresult(hr);
    }
    
    s_fMachineName = true;
    return g_MachineName; 
}

//
// Nedded for linking with fn.lib
//
DWORD
McComputerNameLen(
	void
	)
{
	if (s_fMachineName)
	{
		return g_MachineNameLength;
	}
	
	McComputerName();
	ASSERT(s_fMachineName);
	
	return g_MachineNameLength;
}


void
DDV_ValidFormatName(
	CDataExchange* pDX,
	CString& str
	)
{
	if (!pDX->m_bSaveAndValidate)
		return;

	QUEUE_FORMAT qf;
	AP<WCHAR> strToFree;
	BOOL fRes = FnFormatNameToQueueFormat(str, &qf, &strToFree);

	if ( !fRes ||
		qf.GetType() == QUEUE_FORMAT_TYPE_DL ||
		qf.GetType() == QUEUE_FORMAT_TYPE_MULTICAST )
	{
        CString strNewAlias;
        strNewAlias.LoadString(IDS_ALIAS);

        MessageDSError(MQ_ERROR_ILLEGAL_FORMATNAME, IDS_OP_SET_FORMATNAME_PROPERTY, strNewAlias);
        pDX->Fail();
	}
}


void
SetScrollSizeForList(
	CListBox* pListBox
	)
{
	int dx=0;

	//
	// Find maximal string among all strings in the ListBox
	//
	CDC* pDC = pListBox->GetDC();
	
	for (int i = 0; i < pListBox->GetCount(); i++)
	{
		CString strItem;
		CSize sizeOfStr;

		pListBox->GetText( i, strItem );
		sizeOfStr = pDC->GetTextExtent(strItem);

		dx = max(dx, sizeOfStr.cx);
	}

	pListBox->ReleaseDC(pDC);

	pListBox->SetHorizontalExtent(dx);
}


BOOL
TryStopMSMQServiceIfStarted(BOOL* pfServiceWasRunning, CWnd* pWnd)
{
    BOOL fServiceIsRunning;
    if (!GetServiceRunningState(&fServiceIsRunning))
    {
        //
        // Failed to get service status - do not do anything
		// GetServiceRunningState() displays error in this case
        //
        return FALSE;        
    }


    //
    // The service is running, ask the user whether it wants to stop it.
    // If the service is not stopped, we do not move the files and do not
    // update the registry.
    //
    if (fServiceIsRunning)
    {
		CString strMessage;
        strMessage.LoadString(IDS_Q_STOP_SRVICE);           
        
		if (AfxMessageBox(strMessage, 
						MB_YESNO | MB_ICONQUESTION) == IDNO)
        {
            //
            // So, the user doesn't want to stop the service, be it.
            //
            return FALSE;            
        }

        //
        // Stop the service.
        //
        CString strStopService;

        strStopService.LoadString(IDS_STOP_SERVICE);
        CInfoDlg StopServiceDlg(strStopService, pWnd);

        if (!StopService())
        {
            //
            // Failed to stop the service.
            //
            return FALSE;            
        }
    }

	*pfServiceWasRunning = fServiceIsRunning;

	return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
/*++

FreeMqProps

  Free all properties allocated by MSMQ

--*/
//////////////////////////////////////////////////////////////////////////////
void FreeMqProps(MQMGMTPROPS * mqProps)
{
	//
	// Free all the properties allocated by MSMQ
	//
	for(DWORD i = 0; i < mqProps->cProp; i++)
	{
		switch(mqProps->aPropVar[i].vt)
		{
		case VT_NULL:
		case VT_UI4:
		case VT_I4:
        case VT_UI1:
        case VT_I2:
			break;

		case VT_LPWSTR:
			MQFreeMemory(mqProps->aPropVar[i].pwszVal);
			break;

		case VT_CLSID:
			MQFreeMemory(mqProps->aPropVar[i].puuid);
			break;

		case VT_BLOB:
			MQFreeMemory(mqProps->aPropVar[i].blob.pBlobData);
			break;

		case (VT_VECTOR | VT_LPWSTR):
			{
				for(DWORD j = 0; j < mqProps->aPropVar[i].calpwstr.cElems; j++)
					MQFreeMemory(mqProps->aPropVar[i].calpwstr.pElems[j]);

				MQFreeMemory(mqProps->aPropVar[i].calpwstr.pElems);

				break;
			}

		default:

			ASSERT(0);
		}
	}

	//
	// Remove other allocations
	//
	delete [] mqProps->aStatus;
	delete [] mqProps->aPropID;
	delete [] mqProps->aPropVar;

	//
	// To be on the safe side...
	// 
	mqProps->cProp = 0;
	mqProps->aPropID = NULL;
	mqProps->aPropVar = NULL;
	mqProps->aStatus = NULL;


}


//////////////////////////////////////////////////////////////////////////////
/*++

GetStringPropertyValue

  Return the string value of a pid

--*/
//////////////////////////////////////////////////////////////////////////////
void GetStringPropertyValue(const PropertyDisplayItem * pItem, PROPID pid, PROPVARIANT *pPropVar, CString &str)
{
	if (pItem->itemPid == 0)
	{
		ASSERT(0);
		str = L"";
		return;
	}
	
	PROPVARIANT * pProp;
	GetPropertyVar(pItem, pid, pPropVar, &pProp);

	if(pProp->vt == VT_NULL)
	{
		str = L"";
		return;
	}

	ASSERT(pProp->vt == VT_LPWSTR);
	str = pProp->pwszVal;
 }


BOOL IsClusterVirtualServer(LPCWSTR wcsMachineName)
{
	if (!IsLocalSystemCluster())
	{
		TrTRACE(GENERAL, "%ls is not a cluster machine.", wcsMachineName);
		return FALSE;
	}

	CAutoCluster hCluster( OpenCluster(NULL) );

	if ( hCluster == NULL )
	{
		TrERROR(GENERAL, "Failed to get handle to Cluster that %ls is part of", wcsMachineName);
		return FALSE;
	}

	CClusterNode hNode( OpenClusterNode(hCluster, wcsMachineName) );

	if ( hNode != NULL )
	{
		TrTRACE(GENERAL, "%ls is a cluster physical node.", wcsMachineName);
		return FALSE;
	}

	TrTRACE(GENERAL, "%ls is a cluster virtual server.", wcsMachineName);
	return TRUE;
}

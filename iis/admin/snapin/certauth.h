// Machine generated IDispatch wrapper class(es) created by Microsoft Visual C++

// NOTE: Do not modify the contents of this file.  If this class is regenerated by
//  Microsoft Visual C++, your modifications will be overwritten.

#ifndef __CERTAUTH_H_
#define __CERTAUTH_H_

// Dispatch interfaces referenced by this interface
class COleFont;

/////////////////////////////////////////////////////////////////////////////
// CCertAuth wrapper class

class CCertAuth : public CWnd
{
protected:
    DECLARE_DYNCREATE(CCertAuth)
public:
    CLSID const& GetClsid()
    {
        static CLSID const clsid
            = { 0x996ff6f, 0xb6a1, 0x11d0, { 0x92, 0x92, 0x0, 0xc0, 0x4f, 0xb6, 0x67, 0x8b } };
        return clsid;
    }
    virtual BOOL Create(LPCTSTR lpszClassName,
        LPCTSTR lpszWindowName, DWORD dwStyle,
        const RECT& rect,
        CWnd* pParentWnd, UINT nID,
        CCreateContext* pContext = NULL)
    { return CreateControl(GetClsid(), lpszWindowName, dwStyle, rect, pParentWnd, nID); }

    BOOL Create(LPCTSTR lpszWindowName, DWORD dwStyle,
        const RECT& rect, CWnd* pParentWnd, UINT nID,
        CFile* pPersist = NULL, BOOL bStorage = FALSE,
        BSTR bstrLicKey = NULL)
    { return CreateControl(GetClsid(), lpszWindowName, dwStyle, rect, pParentWnd, nID,
        pPersist, bStorage, bstrLicKey); }

// Attributes
public:
    COleFont GetFont();
    void SetFont(LPDISPATCH);
    short GetBorderStyle();
    void SetBorderStyle(short);
    BOOL GetEnabled();
    void SetEnabled(BOOL);
    CString GetCaption();
    void SetCaption(LPCTSTR);

// Operations
public:
    void SetMachineName(LPCTSTR szMachineName);
    void SetServerInstance(LPCTSTR szServerInstance);
    void DoClick(long dwButtonNumber);
    void AboutBox();
};

#endif // __CERTAUTH_H_



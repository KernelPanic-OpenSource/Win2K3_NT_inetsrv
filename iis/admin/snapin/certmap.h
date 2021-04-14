#ifndef __CERTMAP_H__
#define __CERTMAP_H__

// Machine generated IDispatch wrapper class(es) created by Microsoft Visual C++

// NOTE: Do not modify the contents of this file.  If this class is regenerated by
//  Microsoft Visual C++, your modifications will be overwritten.


// Dispatch interfaces referenced by this interface
class COleFont;

/////////////////////////////////////////////////////////////////////////////
// CCertmap wrapper class

class CCertmap : public CWnd
{
protected:
    DECLARE_DYNCREATE(CCertmap)
public:
    CLSID const& GetClsid()
    {
        static CLSID const clsid
            = { 0xbbd8f29b, 0x6f61, 0x11d0, { 0xa2, 0x6e, 0x8, 0x0, 0x2b, 0x2c, 0x6f, 0x32 } };
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
    BOOL GetEnabled();
    void SetEnabled(BOOL);
    short GetBorderStyle();
    void SetBorderStyle(short);
    CString GetCaption();
    void SetCaption(LPCTSTR);

// Operations
public:
    void SetServerInstance(LPCTSTR szServerInstance);
    void SetMachineName(LPCTSTR szMachineName);
};

#endif // __CERTMAP_H__

/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        strfrn.h

   Abstract:

        String Functions

   Author:

        Ronald Meijer (ronaldm)
        Munged for setup by BoydM

   Project:

        Internet Services Manager

   Revision History:

--*/

#ifndef _STRFN_H
#define _STRFN_H


//
// Memory Allocation Macros
//
#define AllocMem(cbSize)\
    ::LocalAlloc(LPTR, cbSize)

#define FreeMem(lp)\
    ::LocalFree(lp)

#define AllocMemByType(citems, type)\
    (type *)AllocMem(citems * sizeof(type))


//
// Debug Formatting Macros
//
   #define TRACEOUT(x)        { ; }
   #define TRACEEOL(x)        { ; }
   #define TRACEEOLID(x)      { ; }
   #define TRACEEOLERR(err,x) { ; }


//
// Helper Macros
//

//
// Get number of array elements
//
#define ARRAY_SIZE(a)    (sizeof(a)/sizeof(a[0]))

//
// Compute size of string array in characters.  That is, don't count
// the terminal null.
//
#define STRSIZE(str)     (ARRAY_SIZE(str)-1)

#define AllocTString(cch)\
    (LPTSTR)AllocMem((cch) * sizeof(TCHAR))
#define AllocWString(cch)\
    (LPWSTR)AllocMem((cch) * sizeof(WCHAR))

#define IS_NETBIOS_NAME(lpstr) (*lpstr == _T('\\'))

//
// Return the portion of a computer name without the backslashes
//
#define PURE_COMPUTER_NAME(lpstr) (IS_NETBIOS_NAME(lpstr) ? lpstr + 2 : lpstr)

//
// Convert CR/LF to LF
//
BOOL 
PCToUnixText(
    OUT LPWSTR & lpstrDestination,
    IN  const CString strSource
    );

//
// Expand LF to CR/LF (no allocation necessary)
//
BOOL 
UnixToPCText(
    OUT CString & strDestination,
    IN  LPCWSTR lpstrSource
    );

//
// Straight copy
//
BOOL
TextToText(
    OUT LPWSTR & lpstrDestination,
    IN  const CString & strSource
    );

LPWSTR
AllocWideString(
    IN LPCTSTR lpString
    );

LPSTR AllocAnsiString(
    IN LPCTSTR lpString
    );


LPTSTR AllocString(
    IN LPCTSTR lpString
    );

#ifdef UNICODE

    //
    // Copy W string to T string
    // 
    #define WTSTRCPY(dst, src, cch) \
        lstrcpy(dst, src)

    //
    // Copy T string to W string
    //
    #define TWSTRCPY(dst, src, cch) \
        lstrcpy(dst, src)

    //
    // Reference a T String as a W String (a nop in Unicode)
    //
    #define TWSTRREF(str)   ((LPWSTR)str)

#else

    //
    // Convert a T String to a temporary W Buffer, and
    // return a pointer to this internal buffer
    //
    LPWSTR ReferenceAsWideString(LPCTSTR str);

    //
    // Copy W string to T string
    // 
    #define WTSTRCPY(dst, src, cch) \
        WideCharToMultiByte(CP_ACP, 0, src, -1, dst, cch, NULL, NULL)

    //
    // Copy T string to W string
    //
    #define TWSTRCPY(dst, src, cch) \
        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, src, -1, dst, cch)

    //
    // Reference a T String as a W String 
    //
    #define TWSTRREF(str)   ReferenceAsWideString(str)

#endif // UNICODE

//
// Determine if the given string is a UNC name
//
BOOL IsUNCName(
    IN const CString & strDirPath
    );

//
// Determine if the path is a fully qualified path in the context
// of the local machine
//
BOOL IsFullyQualifiedPath(
    IN const CString & strDirPath
    );

//
// Determine if the given string is an URL path
//
BOOL IsURLName(
    IN const CString & strDirPath
    );

//
// Determine if the given string describes a relative URL path
//
inline BOOL IsRelURLPath(
    IN LPCTSTR lpPath
    )
{
    ASSERT(lpPath != NULL);
    return *lpPath == _T('/');
}

//
// Determine if the given path describes a wild-carded redirection
// path (starts with *;)
//
inline BOOL IsWildcardedRedirectPath(
    IN LPCTSTR lpPath
    )
{
    ASSERT(lpPath != NULL);
    return lpPath[0] == '*' && lpPath[1] == ';';
}

//
// Determine if the account is local (doesn't have a computer name)
//
inline BOOL MyIsLocalAccount(
    IN CString & strAccount
    )
{
    return strAccount.Find(_T('\\')) == -1;
}

//
// Convert local path to UNC path
//
LPCTSTR MakeUNCPath(
    IN OUT CString & strDir,
    IN LPCTSTR lpszOwner,
    IN LPCTSTR lpszDirectory
    );

//
// Given domain\username, split into user name and domain
//
BOOL SplitUserNameAndDomain(
    IN OUT CString & strUserName,
    IN CString & strDomainName
    );

//
// Convert double-null terminated string to a CStringList
//
DWORD
ConvertDoubleNullListToStringList(
    IN  LPCTSTR lpstrSrc,
    OUT CStringList & strlDest,
    IN  int cChars = -1
    );

DWORD
ConvertWDoubleNullListToStringList(
    IN  PWCHAR lpstrSrc,
    OUT CStringList & strlDest,
    IN  int cChars = -1
    );

//
// Go from a CStringList to a WIDE double null terminated list
//
DWORD
ConvertStringListToWDoubleNullList(
    IN  CStringList & strlSrc,
    OUT DWORD & cchDest,
    OUT PWCHAR & lpstrDest
    );

//
// Go from a CStringList to a double null terminated list
//
DWORD
ConvertStringListToDoubleNullList(
    IN  CStringList & strlSrc,
    OUT DWORD & cchDest,
    OUT LPTSTR & lpstrDest
    );


//
// Convert separated list of strings to CStringList
//
int
ConvertSepLineToStringList(
    IN  LPCTSTR lpstrIn,
    OUT CStringList & strlOut,
    IN  LPCTSTR lpstrSep
    );

//
// Reverse function of the above
//
LPCTSTR
ConvertStringListToSepLine(
    IN  CStringList & strlIn,
    OUT CString & strOut,
    IN  LPCTSTR lpstrSep
    );

//
// Private strtok
//
LPTSTR 
StringTok(
    IN LPTSTR string,
    IN LPCTSTR control
    );

//
// CString.Find() that's not case-sensitive
//
int 
CStringFindNoCase(
    IN const CString & strSrc,
    IN LPCTSTR lpszSub
    );

//
// Replace the first occurrance of one string
// inside another one.  Return error code
//
DWORD
ReplaceStringInString(
    OUT IN CString & strBuffer,
    IN  CString & strTarget,
    IN  CString & strReplacement,
    IN  BOOL fCaseSensitive
    );


class CStringListEx : public CStringList
/*++

Class Description:

    Superclass of CStringList with comparison and assignment
    operators.

Public Interface:

    operator ==       Comparison operator
    operator !=       Comparison operator
    operator =        Assignment operator  

--*/
{
//
// ctor
//
public:
    CStringListEx(int nBlockSize = 10) : CStringList(nBlockSize) {};

//
// Operators
//
public:
    BOOL operator == (const CStringList & strl);           
    BOOL operator != (const CStringList & strl) { return !operator ==(strl); }
    CStringListEx & operator =(const CStringList & strl);
};


#ifdef _DOS

typedef struct tagINTLFORMAT
{
    WORD wDateFormat;
    CHAR szCurrencySymbol[5];
    CHAR szThousandSep[2];
    CHAR szDecimalPoint[2];
    CHAR szDateSep[2];
    CHAR szTimeSep[2];
    BYTE bCurrencyFormat;
    BYTE bCurrencyDecimals;
    BYTE bTimeFormat;
    DWORD dwMapCall;
    CHAR szDataSep[2];
    BYTE bReserved[5];
} INTLFORMAT;

BOOL _dos_getintlsettings(INTLFORMAT * pStruct);

#endif // _DOS



class CINumber
/*++

Class Description:

    Base class for international-friendly number formatting

Public Interface:

NOTES: Consider making this class a template

--*/
{
public:
    static BOOL Initialize(BOOL fUserSetting = TRUE);
    static CString * s_pstrBadNumber;
    static BOOL UseSystemDefault();
    static BOOL UseUserDefault();
    static BOOL IsInitialized();
    static LPCTSTR QueryThousandSeperator();
    static LPCTSTR QueryDecimalPoint();
    static LPCTSTR QueryCurrency();
    static double BuildFloat(const LONG lInteger, const LONG lFraction);
    static LPCTSTR ConvertLongToString(const LONG lSrc, CString & str);
    static LPCTSTR ConvertFloatToString(
        IN const double flSrc, 
        IN int nPrecision, 
        OUT CString & str
        );

    static BOOL ConvertStringToLong(LPCTSTR lpsrc, LONG & lValue);
    static BOOL ConvertStringToFloat(LPCTSTR lpsrc, double & flValue);

protected:
    CINumber();
    ~CINumber();

protected:
    friend BOOL InitIntlSettings();
    friend void TerminateIntlSettings();
    static BOOL Allocate();
    static void DeAllocate();
    static BOOL IsAllocated();

protected:
    static CString * s_pstr;

private:
    static CString * s_pstrThousandSeperator;
    static CString * s_pstrDecimalPoint;
    static CString * s_pstrCurrency;
    static BOOL s_fCurrencyPrefix;
    static BOOL s_fInitialized;
    static BOOL s_fAllocated;
};



class CILong : public CINumber
/*++

Class Description:

    International-friendly LONG number

Public Interface:


--*/
{
public:
    //
    // Constructors
    //
    CILong();
    CILong(LONG lValue);
    CILong(LPCTSTR lpszValue);

public:
    //
    // Assignment Operators
    //
    CILong & operator =(LONG lValue);
    CILong & operator =(LPCTSTR lpszValue);

    //
    // Shorthand Operators
    //
    CILong & operator +=(const LONG lValue);
    CILong & operator +=(const LPCTSTR lpszValue);
    CILong & operator +=(const CILong& value);
    CILong & operator -=(const LONG lValue);
    CILong & operator -=(const LPCTSTR lpszValue);
    CILong & operator -=(const CILong& value);
    CILong & operator *=(const LONG lValue);
    CILong & operator *=(const LPCTSTR lpszValue);
    CILong & operator *=(const CILong& value);
    CILong & operator /=(const LONG lValue);
    CILong & operator /=(const LPCTSTR lpszValue);
    CILong & operator /=(const CILong& value);

    //
    // Comparison operators
    //
    BOOL operator ==(LONG value);
    BOOL operator !=(CILong& value);

    //
    // Conversion operators
    //
    operator const LONG() const;
    operator LPCTSTR() const;

    inline friend CArchive & AFXAPI operator<<(CArchive & ar, CILong & value)
    {
        return (ar << value.m_lValue);
    }

    inline friend CArchive & AFXAPI operator>>(CArchive & ar, CILong & value)
    {
        return (ar >> value.m_lValue);
    }

#ifdef _DEBUG
    //
    // CDumpContext stream operator
    //
    inline friend CDumpContext & AFXAPI operator<<(
        CDumpContext& dc, 
        const CILong& value
        )
    {
        return (dc << value.m_lValue);
    }

#endif // _DEBUG

protected:
    LONG m_lValue;
};



class CIFloat : public CINumber
/*++

Class Description:

    International-friendly floating point number    

Public Interface:

--*/
{
public:
    //
    // Constructors
    //
    CIFloat(int nPrecision = 2);
    CIFloat(double flValue, int nPrecision = 2);
    CIFloat(LONG lInteger, LONG lFraction, int nPrecision = 2);
    CIFloat(LPCTSTR lpszValue, int nPrecision = 2);

public:
    //
    // Precision functions
    //
    int QueryPrecision() const;
    void SetPrecision(int nPrecision);

    //
    // Assignment Operators
    //
    CIFloat & operator =(double flValue);
    CIFloat & operator =(LPCTSTR lpszValue);

    //
    // Shorthand Operators
    //
    CIFloat & operator +=(const double flValue);
    CIFloat & operator +=(const LPCTSTR lpszValue);
    CIFloat & operator +=(const CIFloat& value);
    CIFloat & operator -=(const double flValue);
    CIFloat & operator -=(const LPCTSTR lpszValue);
    CIFloat & operator -=(const CIFloat& value);
    CIFloat & operator *=(const double flValue);
    CIFloat & operator *=(const LPCTSTR lpszValue);
    CIFloat & operator *=(const CIFloat& value);
    CIFloat & operator /=(const double flValue);
    CIFloat & operator /=(const LPCTSTR lpszValue);
    CIFloat & operator /=(const CIFloat& value);

    //
    // Conversion operators
    //
    operator const double() const;
    operator LPCTSTR() const;

    //
    // Persistence Operators
    //
    inline friend CArchive& AFXAPI operator<<(CArchive& ar, CIFloat& value)
    {
        return (ar << value.m_flValue);
    }

    inline friend CArchive& AFXAPI operator>>(CArchive& ar, CIFloat& value)
    {
        return (ar >> value.m_flValue);
    }

#ifdef _DEBUG

    //
    // CDumpContext stream operator
    //
    inline friend CDumpContext& AFXAPI operator<<(
        CDumpContext& dc, 
        const CIFloat& value
        )
    {
        return (dc << value.m_flValue);
    }

#endif // _DEBUG

protected:
    double m_flValue;
    int m_nPrecision;
};


//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline /* static */ BOOL CINumber::UseSystemDefault()
{
    return Initialize(FALSE);
}

inline /* static */ BOOL CINumber::UseUserDefault()
{
    return Initialize(TRUE);
}

inline /* static */ BOOL CINumber::IsInitialized()
{
    return s_fInitialized;
}

inline /* static */ LPCTSTR CINumber::QueryThousandSeperator()
{
    return (LPCTSTR)*s_pstrThousandSeperator;
}

inline /* static */ LPCTSTR CINumber::QueryDecimalPoint()
{
    return (LPCTSTR)*s_pstrDecimalPoint;
}

inline /* static */ LPCTSTR CINumber::QueryCurrency()
{
    return (LPCTSTR)*s_pstrCurrency;
}

inline /* static */ BOOL CINumber::IsAllocated()
{
    return s_fAllocated;
}

inline BOOL CILong::operator ==(LONG value)
{
    return m_lValue == value;
}

inline BOOL CILong::operator !=(CILong& value)
{
    return m_lValue != value.m_lValue;
}

inline CILong::operator const LONG() const
{
    return m_lValue;
}

inline CILong::operator LPCTSTR() const
{
    return CINumber::ConvertLongToString(m_lValue, *CINumber::s_pstr);
}

inline int CIFloat::QueryPrecision() const
{
    return m_nPrecision;
}

inline void CIFloat::SetPrecision(int nPrecision)
{
    m_nPrecision = nPrecision;
}

inline CIFloat::operator const double() const
{
    return m_flValue;
}

inline CIFloat::operator LPCTSTR() const
{
    return CINumber::ConvertFloatToString(
        m_flValue, 
        m_nPrecision, 
        *CINumber::s_pstr
        );
}

#endif // _STRFN_H

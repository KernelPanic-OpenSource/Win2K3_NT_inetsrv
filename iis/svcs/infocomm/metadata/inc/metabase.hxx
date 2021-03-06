/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    metabase.hxx

Abstract:

    IIS MetaBase defines and declarations.

Author:

    Michael W. Thomas            17-May-96

Revision History:

--*/

#ifndef _metabase_
#define _metabase_

#ifdef UNICODE
#define MD_STRCPY(dest,src)     wcscpy(dest,src)
#define MD_STRCMP(p1,p2)        wcscmp(p1,p2)
#define MD_STRNCMP(p1,p2,count) wcsncmp(p1,p2,count)
#define MD_STRNCPY(p1,p2,count) wcsncpy(p1,p2,count)
#define MD_STRLEN(p)            wcslen(p)
#define MD_SPRINTF              swprintf
#define MD_STRCAT(dest,src)     wcscat(dest,src)
#define MD_ISDIGIT(c)           iswdigit(c)
#else
#define MD_STRCPY(dest,src)     strcpy(dest,src)
#define MD_STRCMP(p1,p2)        strcmp(p1,p2)
#define MD_STRICMP(p1,p2)       lstrcmpi(p1,p2)
#define MD_STRNICMP(p1,p2,count) _mbsnicmp((PUCHAR)p1,(PUCHAR)p2,count)
#define MD_STRNCPY(p1,p2,count) strncpy((PUCHAR)p1,(PUCHAR)p2,count)
#define MD_STRLEN(p)            _mbslen((PUCHAR)p)
#define MD_STRBYTES(p)          strlen(p)
#define MD_SPRINTF              wsprintf
#define MD_STRCAT(dest,src)     strcat(dest,src)


#define MD_ISDIGIT(c)           isdigit((UCHAR)(c))
#define MD_STRCHR(str, c)       _mbschr((const UCHAR *)(str), c)
#define MD_STRSTR(str1, str2)   (LPSTR)_mbsstr((const UCHAR *)(str1), (const UCHAR *)(str2))
#endif

#ifdef _X86_
#define MD_ALIGN_ADJUST(p) 0
#else
#define MD_ALIGN_ADJUST(p) (PtrToUlong(p) % 4)
#endif

#define MD_COPY(dest,src,length)       memcpy(dest,src,length)
#define MD_CMP(dest,src,length)        memcmp(dest,src,length)

#define MD_ASSERT(p)            DBG_ASSERT(p)
#define MD_REQUIRE(p)           DBG_REQUIRE(p)

#define LESSOROF(p1,p2) ((p1) < (p2)) ? (p1) : (p2)
#define GREATEROF(p1,p2) ((p1) > (p2)) ? (p1) : (p2)

enum METADATA_IDS {
    MD_ID_NONE,
    MD_ID_OBJECT,
    MD_ID_ROOT_OBJECT,
    MD_ID_DATA,
    MD_ID_REFERENCE,
    MD_ID_CHANGE_NUMBER,
    MD_ID_MAJOR_VERSION_NUMBER,
    MD_ID_MINOR_VERSION_NUMBER,
    MD_ID_SESSION_KEY
    };

#define MD_OBJECT_ID_STRING        TEXT("OBJECT")
#define MD_ROOT_OBJECT_ID_STRING   TEXT("MASTERROOTOBJECT")
#define MD_DATA_ID_STRING          TEXT("DATA")
#define MD_REFERENCE_ID_STRING     TEXT("REFERENCE")
#define MD_CHANGE_NUMBER_ID_STRING TEXT("CHANGENUMBER")
#define MD_MAJOR_VERSION_NUMBER_ID_STRING TEXT("MAJORVERSIONNUMBER")
#define MD_MINOR_VERSION_NUMBER_ID_STRING TEXT("MINORVERSIONNUMBER")
#define MD_SESSION_KEY_ID_STRING   TEXT("SESSIONKEY")

#define MD_SIGNATURE_ID_STRING     TEXT("METADATA_SIGNATURE")
#define MD_BLANK_NAME_ID_STRING    TEXT("NONAME")
#define MD_BLANK_PSEUDO_NAME       TEXT(")*(&%^BLANK_NAME!$@%#^")
#define MD_TIMESTAMP_ID_STRING     TEXT("TIMESTAMP")
#define MD_DT_SUFFIX               TEXT("_DATATYPE")
#define MD_UT_SUFFIX               TEXT("_USERTYPE")
#define MD_ATTR_SUFFIX             TEXT("_ATTR")

#define MD_TERMINATE_BYTE          0xfd
#define MD_ESCAPE_BYTE             0xfe
#define NEEDS_ESCAPE(c) ((c) == MD_ESCAPE_BYTE)
#define FIRSTDATAPTR(pbufLine) ((PBYTE)pbufLine->QueryPtr() + 1)
#define DATAOBJECTBASESIZE (1 + (4 * sizeof(DWORD)))
#define BASEMETAOBJECTLENGTH (1 + sizeof(FILETIME))

#define MD_MAX_PATH_LEN         4096
#define MD_MAX_WHITE_SPACE      20
#define MD_MAX_DWORD_STRING     10
#define MD_UUENCODE_FACTOR      2

#define MD_MASTER_ROOT_NAME                          TEXT("MasterRoot")
#define MD_DEFAULT_DLL_FILE_NAME                     TEXT("metadata.dll")
#define MD_DEFAULT_DATA_FILE_NAME                    TEXT("MetaBase.xml")
#define MD_DEFAULT_DATA_FILE_NAMEW                   L"MetaBase.xml"
#define MD_SCHEMA_FILE_NAME                          TEXT("MBSchema.xml")
#define MD_HISTORY_FILE_SUBDIRW                      L"History\\"
#define MD_CCH_HISTORY_FILE_SUBDIRW                  ((sizeof(MD_HISTORY_FILE_SUBDIRW)/sizeof(WCHAR))-1)
#define MD_SCHEMA_EXTENSION_FILE_NAMEW               L"MBSchExt.xml"
#define MD_CCH_SCHEMA_EXTENSION_FILE_NAMEW           ((sizeof(MD_SCHEMA_EXTENSION_FILE_NAMEW)/sizeof(WCHAR))-1)
#define MD_HISTORY_FILE_SEARCH_EXTENSIONW            L"_??????????_??????????"
#define MD_CCH_HISTORY_FILE_SEARCH_EXTENSIONW        ((sizeof(MD_HISTORY_FILE_SEARCH_EXTENSIONW)/sizeof(WCHAR))-1)
#define MD_ERROR_FILE_NAME_EXTENSIONW                L"Error"
#define MD_CCH_ERROR_FILE_NAME_EXTENSIONW            ((sizeof(MD_ERROR_FILE_NAME_EXTENSIONW)/sizeof(WCHAR))-1)
#define MD_ERROR_FILE_SEARCH_EXTENSIONW              L"_??????????"
#define MD_CCH_ERROR_FILE_SEARCH_EXTENSIONW          ((sizeof(MD_ERROR_FILE_SEARCH_EXTENSIONW)/sizeof(WCHAR))-1)
#define MD_TEMP_DATA_FILE_EXT                        TEXT(".tmp")
#define MD_TEMP_DATA_FILE_EXTW                       L".tmp"
#define MD_BACKUP_DATA_FILE_EXT                      TEXT(".bak")

#define MD_DEFAULT_BACKUP_PATH_NAME                  TEXT("MetaBack")
#define MD_SCHEMA_SUFFIX                             TEXT(".SC")
#define MD_BACKUP_SUFFIX                             TEXT(".MD")
#define MD_BACKUP_SUFFIXW                            L".MD"
#define MD_BACKUP_INVALID_CHARS_W                    L"/\\*.?\"&!@#$%^()=+|`~"
#define MD_BACKUP_INVALID_CHARS_A                    "/\\*.?\"&!@#$%^()=+|`~"


#define SETUP_REG_KEY               TEXT("SOFTWARE\\Microsoft\\InetStp")
#define INSTALL_PATH_VALUE          TEXT("InstallPath")
#define MD_UNSECUREDREAD_VALUE      TEXT("MetabaseUnSecuredRead")
#define MD_SETMAJORVERSION_VALUE    TEXT("MetabaseSetMajorVersion")
#define MD_SETMINORVERSION_VALUE    TEXT("MetabaseSetMinorVersion")
#define MD_CURRENT_INSTALL_STATE    TEXT("CurrentInstallState")
#define MD_INSTALL_STATE_INSTALLING 0x00000001

#define MD_PATH_DELIMETER       MD_PATH_DELIMETERA
#define MD_ALT_PATH_DELIMETER   MD_ALT_PATH_DELIMETERA

#define MD_PATH_DELIMETERA      (CHAR)'/'
#define MD_ALT_PATH_DELIMETERA  (CHAR)'\\'

#define MD_PATH_DELIMETERW      (WCHAR)'/'
#define MD_ALT_PATH_DELIMETERW  (WCHAR)'\\'

#define MD_DEFAULT_HISTORY_MAJOR_NUM_DIGITS 10
#define MD_DEFAULT_HISTORY_MINOR_NUM_DIGITS 10

#define SKIP_DELIMETER(p1,p2)   if (*p1 == p2) p1++;

#define SKIP_PATH_DELIMETER(p1)  SKIP_PATH_DELIMETERA(p1)
#define SKIP_PATH_DELIMETERA(p1) if ((*(LPSTR)p1 == MD_PATH_DELIMETERA) || (*(LPSTR)p1 == MD_ALT_PATH_DELIMETERA)) {(LPSTR)p1++;}
#define SKIP_PATH_DELIMETERW(p1) if ((*(LPWSTR)p1 == MD_PATH_DELIMETERW) || (*(LPWSTR)p1 == MD_ALT_PATH_DELIMETERW)) {((LPWSTR)p1)++;}

//#define MD_BINARY_STRING        TEXT("BINARY")
//#define MD_STRING_STRING        TEXT("STRING")
//#define MD_DWORD_STRING         TEXT("DWORD")
//#define MD_INHERIT_STRING       TEXT("INHERIT")
#define MD_SIGNATURE_STRINGA    "*&$MetaData$&*"
#define MD_SIGNATURE_STRINGW    L##"*&$MetaData$&*"

// iis4=1
// iis5=2
// iis5.1=2
#define MD_MAJOR_VERSION_NUMBER   2

// iis4=0
// iis5=0
// iis5.1=1
#define MD_MINOR_VERSION_NUMBER   1


#define METADATA_MAX_STRING_LEN         4096

#define MAX_RECORD_BUFFER       1024

#define READWRITE_BUFFER_LENGTH 128 * 1024

#define EVENT_ARRAY_LENGTH      2

#define EVENT_READ_INDEX        0

#define EVENT_WRITE_INDEX       1

#define OPEN_WAIT_INTERVAL      1000

#define DATA_HASH_TABLE_LEN     67

#define DATA_HASH(ID)           ((ID) % DATA_HASH_TABLE_LEN)

#define MD_SHUTDOWN_WAIT_SECONDS 7

enum MD_SINK_ROUTINES {
    MD_SINK_MAIN,
    MD_SINK_SHUTDOWN,
    MD_SINK_EVENT
};

#define MD_XML_SCHEMA_TIMESTAMPW                       L"XMLSchemaTimeStamp"
#define MD_BIN_SCHEMA_TIMESTAMPW                       L"BINSchemaTimeStamp"
#define MD_GLOBAL_LOCATIONW                            L"."
#define MD_EDIT_WHILE_RUNNING_MAJOR_VERSION_NUMBERW    L"HistoryMajorVersionNumber"
#define MD_EDIT_WHILE_RUNNING_TEMP_DATA_FILE_NAMEW     L"EditWhileRunning_Metabase.xml"
#define MD_EDIT_WHILE_RUNNING_TEMP_SCHEMA_FILE_NAMEW   L"EditWhileRunning_MBSchema.xml"
#define MD_SESSION_KEYW                                L"SessionKey"
#define MD_ENABLE_EDIT_WHILE_RUNNINGW                  L"EnableEditWhileRunning"
#define MD_ENABLE_HISTORYW                             L"EnableHistory"
#define MD_MAX_HISTORY_FILESW                          L"MaxHistoryFiles"
#define MD_CHANGE_NUMBERW                              L"ChangeNumber"
#define MD_MAX_ERROR_FILESW                            L"MaxErrorFiles"

#define MD_GLOBAL_LOCATIONW                            L"."
#define MD_CH_LOC_NO_PROPERTYW                         L'#'

#define MD_COUNT_MAX_HISTORY_FILES                     10
#define MD_MAX_HISTORY_FILES_ALLOC_SIZE                20   // Max allocations
#define MD_COUNT_MAX_ERROR_FILES                       10
#define MD_MAX_CHILD_OBJECTS                           8

#define MD_LONG_STRING_PREFIXW                         L"\\\\?\\"
#define MD_CCH_LONG_STRING_PREFIXW                     ((sizeof(MD_LONG_STRING_PREFIXW)/sizeof(WCHAR))-1)
#define MD_CCH_MAX_ULONG                               10
#define MD_CH_EXTN_SEPERATORW                          L'.'
#define MD_CH_UNDERSCOREW                              L'_'
#define MD_UNDERSCOREW                                 L"_"
#define MD_CCH_UNDERSCOREW                             ((sizeof(MD_UNDERSCOREW)/sizeof(WCHAR))-1)

typedef struct _METABASE_FILE_DATA
{
    ULONG    ulVersionMinor;
    ULONG    ulVersionMajor;
    FILETIME ftLastWriteTime;

}METABASE_FILE_DATA;


#endif

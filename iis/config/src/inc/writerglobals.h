#pragma once

extern LPCWSTR  g_wszBeginFile0;
extern ULONG	g_cchBeginFile0;
extern LPCWSTR	g_wszBeginFile1;
extern ULONG	g_cchBeginFile1;
extern LPCWSTR	g_wszEndFile;					
extern ULONG	g_cchEndFile;					
extern LPCWSTR	g_BeginLocation;
extern ULONG	g_cchBeginLocation;				
extern LPCWSTR	g_Location;						
extern ULONG	g_cchLocation;					
extern LPCWSTR	g_EndLocationBegin;				
extern ULONG	g_cchEndLocationBegin;			
extern LPCWSTR	g_EndLocationEnd;				
extern ULONG	g_cchEndLocationEnd;				
extern LPCWSTR  g_CloseQuoteBraceRtn;			
extern ULONG	g_cchCloseQuoteBraceRtn;			
extern LPCWSTR  g_Rtn;							
extern ULONG	g_cchRtn;						
extern LPCWSTR  g_EqQuote;						
extern ULONG	g_cchEqQuote;					
extern LPCWSTR  g_QuoteRtn;						
extern ULONG	g_cchQuoteRtn;					
extern LPCWSTR  g_TwoTabs;						
extern ULONG	g_cchTwoTabs;					
extern LPCWSTR	g_NameEq;						
extern ULONG	g_cchNameEq;						
extern LPCWSTR	g_IDEq;							
extern ULONG	g_cchIDEq;						
extern LPCWSTR	g_ValueEq;						
extern ULONG	g_cchValueEq;					
extern LPCWSTR	g_TypeEq;						
extern ULONG	g_cchTypeEq;						
extern LPCWSTR	g_UserTypeEq;					
extern ULONG	g_cchUserTypeEq;					
extern LPCWSTR	g_AttributesEq;					
extern ULONG	g_cchAttributesEq;				
extern LPCWSTR	g_BeginGroup;					
extern ULONG	g_cchBeginGroup;					
extern LPCWSTR	g_EndGroup;						
extern ULONG	g_cchEndGroup;					
extern LPCWSTR	g_BeginCustomProperty;			
extern ULONG	g_cchBeginCustomProperty;		
extern LPCWSTR	g_EndCustomProperty;				
extern ULONG	g_cchEndCustomProperty;			
extern LPCWSTR	g_ZeroHex;						
extern ULONG	g_cchZeroHex;
extern LPCWSTR	g_wszIIsConfigObject;
extern LPCWSTR  g_BeginComment;
extern ULONG    g_cchBeginComment;
extern LPCWSTR  g_EndComment;
extern ULONG    g_cchEndComment;

extern WORD    BYTE_ORDER_MASK;
extern DWORD	UTF8_SIGNATURE;

extern LPWSTR  g_wszByID;
extern LPWSTR  g_wszByName;
extern LPWSTR  g_wszByTableAndColumnIndexOnly;
extern LPWSTR  g_wszByTableAndColumnIndexAndNameOnly;
extern LPWSTR  g_wszByTableAndColumnIndexAndValueOnly;
extern LPWSTR  g_wszByTableAndTagNameOnly;
extern LPWSTR  g_wszByTableAndTagIDOnly;
extern LPWSTR  g_wszUnknownName;
extern ULONG   g_cchUnknownName;             
extern LPWSTR  g_UT_Unknown;
extern ULONG   g_cchUT_Unknown;
extern LPWSTR  g_T_Unknown;
extern LPWSTR  g_wszTrue;
extern ULONG   g_cchTrue;
extern LPWSTR  g_wszFalse;
extern ULONG   g_cchFalse;
extern ULONG   g_cchMaxBoolStr;

extern LPCWSTR g_wszHistorySlash;
extern ULONG   g_cchHistorySlash;
extern LPCWSTR g_wszMinorVersionExt;
extern ULONG   g_cchMinorVersionExt;
extern LPCWSTR g_wszDotExtn;
extern ULONG   g_cchDotExtn;
extern WCHAR   g_wchBackSlash;
extern WCHAR   g_wchFwdSlash;
extern WCHAR   g_wchDot;

extern ULONG  g_cchTemp;
extern WCHAR  g_wszTemp[];
extern LPCWSTR g_wszBeginSchema;
extern ULONG   g_cchBeginSchema;
extern LPCWSTR g_wszEndSchema;
extern ULONG   g_cchEndSchema;
extern LPCWSTR g_wszBeginCollection;
extern ULONG   g_cchBeginCollection;
extern LPCWSTR g_wszEndBeginCollectionMB;
extern ULONG   g_cchEndBeginCollectionMB;
extern LPCWSTR g_wszEndBeginCollectionCatalog;
extern ULONG   g_cchEndBeginCollectionCatalog;
extern LPCWSTR g_wszInheritsFrom;
extern ULONG   g_cchInheritsFrom;
extern LPCWSTR g_wszEndCollection;
extern ULONG   g_cchEndCollection;
extern LPCWSTR g_wszBeginPropertyShort; 
extern ULONG   g_cchBeginPropertyShort;
extern LPCWSTR g_wszMetaFlagsExEq;
extern ULONG   g_cchMetaFlagsExEq;
extern LPCWSTR g_wszMetaFlagsEq;
extern ULONG   g_cchMetaFlagsEq;
extern LPCWSTR g_wszEndPropertyShort; 
extern ULONG   g_cchEndPropertyShort; 
extern LPCWSTR g_wszBeginPropertyLong;
extern ULONG   g_cchBeginPropertyLong;
extern LPCWSTR g_wszPropIDEq;
extern ULONG   g_cchPropIDEq;
extern LPCWSTR g_wszPropTypeEq;
extern ULONG   g_cchPropTypeEq;
extern LPCWSTR g_wszPropUserTypeEq;
extern ULONG   g_cchPropUserTypeEq;
extern LPCWSTR g_wszPropAttributeEq;
extern ULONG   g_cchPropAttributeEq;
extern LPWSTR  g_wszPropMetaFlagsEq; 
extern ULONG   g_cchPropMetaFlagsEq; 
extern LPWSTR  g_wszPropMetaFlagsExEq;
extern ULONG   g_cchPropMetaFlagsExEq;
extern LPWSTR  g_wszPropDefaultEq;    
extern ULONG   g_cchPropDefaultEq;    
extern LPWSTR  g_wszPropMinValueEq;   
extern ULONG   g_cchPropMinValueEq;   
extern LPWSTR  g_wszPropMaxValueEq;   
extern ULONG   g_cchPropMaxValueEq;  
extern LPWSTR  g_wszEndPropertyLongNoFlag;
extern ULONG   g_cchEndPropertyLongNoFlag;
extern LPWSTR  g_wszEndPropertyLongBeforeFlag;
extern ULONG   g_cchEndPropertyLongBeforeFlag;
extern LPWSTR  g_wszEndPropertyLongAfterFlag;
extern ULONG   g_cchEndPropertyLongAfterFlag; 
extern LPCWSTR g_wszBeginFlag; 
extern ULONG   g_cchBeginFlag; 
extern LPCWSTR g_wszFlagValueEq;   
extern ULONG   g_cchFlagValueEq;   
extern LPCWSTR g_wszEndFlag;
extern ULONG   g_cchEndFlag;
extern LPCWSTR g_wszEndPropertyShort;

extern LPCWSTR g_wszOr;
extern ULONG   g_cchOr;
extern LPCWSTR g_wszOrManditory;
extern ULONG    g_cchOrManditory;
extern LPCWSTR g_wszFlagIDEq;
extern ULONG   g_cchFlagIDEq;
extern LPCWSTR g_wszContainerClassListEq;
extern ULONG   g_cchContainerClassListEq;

extern LPCWSTR g_wszSlash;
extern ULONG   g_cchSlash;
extern LPCWSTR g_wszLM;
extern ULONG   g_cchLM;
extern LPCWSTR g_wszSchema;
extern ULONG   g_cchSchema;
extern LPCWSTR g_wszSlashSchema;
extern ULONG   g_cchSlashSchema;
extern LPCWSTR g_wszSlashSchema;
extern ULONG   g_cchSlashSchema;
extern LPCWSTR g_wszSlashSchemaSlashProperties;
extern ULONG   g_cchSlashSchemaSlashProperties;
extern LPCWSTR g_wszSlashSchemaSlashPropertiesSlashNames;
extern ULONG   g_cchSlashSchemaSlashPropertiesSlashNames;
extern LPCWSTR g_wszSlashSchemaSlashPropertiesSlashTypes;
extern ULONG   g_cchSlashSchemaSlashPropertiesSlashTypes;
extern LPCWSTR g_wszSlashSchemaSlashPropertiesSlashDefaults;
extern ULONG   g_cchSlashSchemaSlashPropertiesSlashDefaults;
extern LPCWSTR g_wszSlashSchemaSlashClasses;
extern ULONG   g_cchSlashSchemaSlashClasses;
extern LPWSTR  g_wszEmptyMultisz;
extern ULONG   g_cchEmptyMultisz;
extern LPWSTR  g_wszEmptyWsz;
extern ULONG   g_cchEmptyWsz;
extern LPCWSTR g_wszComma;
extern ULONG   g_cchComma;
extern LPCWSTR g_wszMultiszSeperator;
extern ULONG   g_cchMultiszSeperator;


extern LPCWSTR g_aSynIDToWszType[];


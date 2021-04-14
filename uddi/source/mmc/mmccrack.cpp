#include <mmc.h>
#include <winuser.h>
#include <tchar.h>

#include "globals.h"

#define MMC_CRACK_MESSAGES 1

static TCHAR *MMCN_CrackVerb(MMC_CONSOLE_VERB verb)
{
	static TCHAR error[64];

	switch( verb ) 
	{
    case MMC_VERB_NONE:
		return _T("NONE");
    case MMC_VERB_OPEN:
		return _T("OPEN");
    case MMC_VERB_COPY:
		return _T("COPY");
    case MMC_VERB_PASTE:
		return _T("PASTE");
    case MMC_VERB_DELETE:
		return _T("DELETE");
    case MMC_VERB_PROPERTIES:
        return _T("PROPERTIES");
    case MMC_VERB_RENAME:
        return _T("RENAME");
    case MMC_VERB_REFRESH:
        return _T("REFRESH");
    case MMC_VERB_PRINT:
        return _T("PRINT");
    case MMC_VERB_CUT:
        return _T("CUT");
    default:
        _sntprintf( error, ARRAYLEN(error) - 1, _T("Unknown verb id %d"), verb );
		error[ ARRAYLEN(error) - 1 ] = NULL;
        return error;
	}
}

void MMCN_Crack(
		BOOL bComponentData,
        IDataObject *pDataObject,
        IComponentData *pCompData,
        IComponent *pComp,
        MMC_NOTIFY_TYPE event,
        LPARAM arg,
        LPARAM param )
{

#ifdef MMC_CRACK_MESSAGES

	TCHAR message[256] = {0};

	if( TRUE == bComponentData )
		OutputDebugString(_T("IComponentData::Notify( "));
	else
		OutputDebugString(_T("IComponent::Notify( "));

	_sntprintf( message, ARRAYLEN( message ), _T( "DataObject: %08p, CompData: %08p, Comp: %08p ) - "), pDataObject, pCompData, pComp );
	message[ ARRAYLEN(message) - 1 ] = NULL;
	OutputDebugString( message );

	switch( event )
	{
	case MMCN_ACTIVATE:
			_sntprintf( message, ARRAYLEN(message) - 1, _T("MMCN_ACTIVATE\n\tActivate: %d\n"), arg);
			break;

	case MMCN_ADD_IMAGES:
			_sntprintf( message, ARRAYLEN(message) - 1, _T("MMCN_ADD_IMAGES\n\tImage List: %08x\n\tScope Item: %08x\n"), arg, param);
			break;

	case MMCN_BTN_CLICK:
			_sntprintf( message, ARRAYLEN(message) - 1, _T("MMCN_BTN_CLICK\n\tVerb: %ws\n"), MMCN_CrackVerb((MMC_CONSOLE_VERB)param));
			break;

	case MMCN_COLUMN_CLICK:
			_sntprintf( message, ARRAYLEN(message) - 1, _T("MMCN_COLUMN_CLICK\n\tColumn: %d\n\tSort Option: %d\n"), arg,       param);
			break;

	case MMCN_COLUMNS_CHANGED:
			_sntprintf( message, ARRAYLEN(message) - 1, _T("MMCN_COLUMNS_CHANGED\n"));
			break;

	case MMCN_CONTEXTHELP:
			_sntprintf( message, ARRAYLEN(message) - 1, _T("MMCN_CONTEXTHELP\n"));
			break;

	case MMCN_CONTEXTMENU:
			_sntprintf( message, ARRAYLEN(message) - 1, _T("MMCN_CONTEXTMENU\n"));
			break;

	case MMCN_CUTORMOVE:
			_sntprintf( message, ARRAYLEN(message) - 1, _T("MMCN_CUTORMOVE\n"));
			break;

	case MMCN_DBLCLICK:
			_sntprintf( message, ARRAYLEN(message) - 1, _T("MMCN_DBLCLICK\n"));
			break;

	case MMCN_DELETE:
			_sntprintf( message, ARRAYLEN(message) - 1, _T("MMCN_DELETE\n"));
			break;

	case MMCN_DESELECT_ALL:
			_sntprintf( message, ARRAYLEN(message) - 1, _T("MMCN_DESELECT_ALL\n"));
			break;

	case MMCN_EXPAND:
			_sntprintf( message, ARRAYLEN(message) - 1, _T("MMCN_EXPAND\n\tExpand: %d\n\tScope Item: %08x\n"), arg, param);
			break;

	case MMCN_EXPANDSYNC:
			_sntprintf( message, ARRAYLEN(message) - 1, _T("MMCN_EXPANDSYNC\n"));
			break;

	case MMCN_FILTERBTN_CLICK:
			_sntprintf( message, ARRAYLEN(message) - 1, _T("MMCN_FILTERBTN_CLICK\n\tColumn: %d\n"), arg);
			break;

	case MMCN_FILTER_CHANGE:
			_sntprintf( message, ARRAYLEN(message) - 1, _T("MMCN_FILTER_CHANGE\n\tColumn: %d\n"), param);
			break;

	case MMCN_HELP:
			_sntprintf( message, ARRAYLEN(message) - 1, _T("MMCN_HELP\n"));
			break;

	case MMCN_INITOCX:
			_sntprintf( message, ARRAYLEN(message) - 1, _T("MMCN_INITOCX\n\tIUnknown: %08x\n"), param);
			break;

	case MMCN_LISTPAD:
			_sntprintf( message, ARRAYLEN(message) - 1, _T("MMCN_LISTPAD\n\tConnect: %d\n"), arg);
			break;

	case MMCN_MENU_BTNCLICK:
			_sntprintf( message, ARRAYLEN(message) - 1, _T("MMCN_MENU_BTNCLICK\n"));
			break;

	case MMCN_MINIMIZED:
			_sntprintf( message, ARRAYLEN(message) - 1, _T("MMCN_MINIMIZED\n\tMinimized: %d\n"), arg);
			break;

	case MMCN_PASTE:
			_sntprintf( message, ARRAYLEN(message) - 1, _T("MMCN_PASTE\n\tCopy: %d\n"), NULL == param );
			break;

	case MMCN_PRELOAD:
			_sntprintf( message, ARRAYLEN(message) - 1, _T("MMCN_PRELOAD\n\tScope Item: %08x\n"), arg);
			break;

	case MMCN_PRINT:
			_sntprintf( message, ARRAYLEN(message) - 1, _T("MMCN_PRINT\n"));
			break;

	case MMCN_PROPERTY_CHANGE:
			_sntprintf( message, ARRAYLEN(message) - 1, _T("MMCN_PROPERTY_CHANGE\n\tScope: %d\n\tArg: %08x\n"), arg, param);
			break;

	case MMCN_QUERY_PASTE:
			_sntprintf( message, ARRAYLEN(message) - 1, _T("MMCN_QUERY_PASTE\n"));
			break;

	case MMCN_REFRESH:
			_sntprintf( message, ARRAYLEN(message) - 1, _T("MMCN_REFRESH\n"));
			break;

	case MMCN_REMOVE_CHILDREN:
			_sntprintf( message, ARRAYLEN(message) - 1, _T("MMCN_REMOVE_CHILDREN\n\tScope Item: %08x\n"), arg);
			break;

	case MMCN_RENAME:
			_sntprintf( message, ARRAYLEN(message) - 1, _T("MMCN_RENAME\n\tNew Name: '%ws'\n"), (LPOLESTR)param);
			break;

	case MMCN_RESTORE_VIEW:
			_sntprintf( message, ARRAYLEN(message) - 1, _T("MMCN_RESTORE_VIEW\n"));
			break;

	case MMCN_SELECT:
			_sntprintf( message, ARRAYLEN(message) - 1, _T("MMCN_SELECT\n\tScope: %d\n\tSelect: %08x\n"), LOWORD(arg), HIWORD(arg));
			break;

	case MMCN_SHOW:
			_sntprintf( message, ARRAYLEN(message) - 1, _T("MMCN_SHOW\n\tShow: %d\n\tScope Item: %08x\n"), arg, param);
			break;

	case MMCN_SNAPINHELP:
			_sntprintf( message, ARRAYLEN(message) - 1, _T("MMCN_SNAPINHELP\n"));
			break;

	case MMCN_VIEW_CHANGE:
			_sntprintf( message, ARRAYLEN(message) - 1, _T("MMCN_VIEW_CHANGE\n\tData: %08x\n\tHint: %08x\n"), arg, param);
			break;
	}
	
	message[ ARRAYLEN(message) - 1 ] = NULL;

	if( message[0] != 0 )
		OutputDebugString(message);
	else
		OutputDebugString(_T("unknown event\n"));
#endif
}

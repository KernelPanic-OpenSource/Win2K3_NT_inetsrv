/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996 Microsoft Corporation
//
//	Module Name:
//		BasePage.inl
//
//	Abstract:
//		Implementation of inline methods of the CBasePropertyPage class.
//
//	Author:
//		David Potter (davidp)	October 2, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _BASEPAGE_INL_
#define _BASEPAGE_INL_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _BASEPAGE_H_
#include "BasePage.h"
#endif

/////////////////////////////////////////////////////////////////////////////

IWCWizardCallback * CBasePropertyPage::PiWizardCallback(void) const
{
	ASSERT(Peo() != NULL);
	return Peo()->PiWizardCallback();
}

BOOL CBasePropertyPage::BWizard(void) const
{
	ASSERT(Peo() != NULL);
	return Peo()->BWizard();
}

HCLUSTER CBasePropertyPage::Hcluster(void) const
{
	ASSERT(Peo() != NULL);
	return Peo()->Hcluster();
}

/////////////////////////////////////////////////////////////////////////////

#endif // _BASEPAGE_INL_
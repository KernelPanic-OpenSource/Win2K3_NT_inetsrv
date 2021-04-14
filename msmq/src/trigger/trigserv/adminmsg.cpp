//*******************************************************************
//
// Class Name  : CAdminMessage
//
// Author      : James Simpson (Microsoft Consulting Services)
// 
// Description : This class represents a message sent to the admin 
//               thread. 
// 
// When     | Who       | Change Description
// ------------------------------------------------------------------
// 15/01/99 | jsimpson  | Initial Release
//
//*******************************************************************
#include "stdafx.h"
#include "adminmsg.hpp"

#include "adminmsg.tmh"

//*******************************************************************
//
// Method      : Constructor
//
// Description : 
//
//*******************************************************************
CAdminMessage::CAdminMessage(eMsgTypes eMsgType)
{
	m_eMsgType = eMsgType;
}

//*******************************************************************
//
// Method      : Destructor
//
// Description : 
//
//*******************************************************************
CAdminMessage::~CAdminMessage()
{
}
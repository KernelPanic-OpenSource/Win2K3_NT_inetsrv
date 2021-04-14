/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:

    Mtm.cpp

Abstract:

    Multicast Transport Manager general functions

Author:

    Shai Kariv  (shaik)  27-Aug-00

Environment:

    Platform-independent

--*/

#include <libpch.h>
#include <mqwin64a.h>
#include <mqsymbls.h>
#include <mqformat.h>
#include "Mtm.h"
#include "Mtmp.h"
#include "mmt.h"

#include "mtm.tmh"

VOID 
MtmCreateTransport(
    IMessagePool* pMessageSource,
	ISessionPerfmon* pPerfmon,
	MULTICAST_ID id
    ) 
/*++

Routine Description:

    Handle new queue notification. Create a new message transport.

Arguments:

    pMessageSource  - Pointer to message source interface.

    id - The multicast address and port.

Returned Value:

    None.

--*/
{
    MtmpAssertValid();

    ASSERT(pMessageSource != NULL);

	MtmpCreateNewTransport(
			pMessageSource, 
			pPerfmon,
            id
			);
} // MtmCreateTransport


VOID
MtmTransportClosed(
    MULTICAST_ID id
    )
/*++

Routine Description:

    Notification for closing connection. Removes the transport from the
    internal database and checkes if a new transport should be created (the associated 
    queue is in idle state or not)

Arguments:

    id - The multicast address and port.

Returned Value:

    None.

--*/
{
    MtmpAssertValid();

    WCHAR buffer[MAX_PATH];
    MQpMulticastIdToString(id, buffer, TABLE_SIZE(buffer));
    TrTRACE(NETWORKING, "MtmTransportClosed. transport to: %ls", buffer);

    MtmpRemoveTransport(id);

} // MtmTransportClosed


VOID 
MtmTransportPause(
    MULTICAST_ID id
    )
{
	R<CMulticastTransport> mcTransport = MtmGetTransport(id);
	if (mcTransport.get() == NULL)
		return;

	if(WPP_LEVEL_COMPID_ENABLED(rsTrace, NETWORKING))
	{
		WCHAR buffer[MAX_PATH];
		MQpMulticastIdToString(id, buffer, TABLE_SIZE(buffer));
		TrTRACE(NETWORKING, "Pause multicast transport to: %ls", buffer);
	}

	mcTransport->Pause();
}


VOID
MtmDisconnect(
	VOID
	)
{
	for(R<CMulticastTransport> p = MtmFindFirst(); (p.get() != NULL); p = MtmFindFirst())
	{
		if(WPP_LEVEL_COMPID_ENABLED(rsTrace, NETWORKING))
		{
			WCHAR buffer[MAX_PATH];
			MQpMulticastIdToString(p->MulticastId(), buffer, TABLE_SIZE(buffer));
			TrTRACE(NETWORKING, "Pause multicast transport to: %ls", buffer);
		}

		p->Pause();
	}
}
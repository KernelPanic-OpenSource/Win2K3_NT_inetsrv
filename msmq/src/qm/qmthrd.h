/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    qmthrd.h

Abstract:



Author:

    Uri Habusha (urih)

--*/
#ifndef __QMTHRD_H__
#define __QMTHRD_H__


#include <Ex.h>
#include "Qmp.h"
#include "ac.h"

class CTransportBase;
struct CDebugSection;
struct CSessionSection;


VOID WINAPI PutPacketFailed(EXOVERLAPPED* pov);
VOID WINAPI PutPacketSucceeded(EXOVERLAPPED* pov);

VOID WINAPI PutOrderedPacketFailed(EXOVERLAPPED* pov);
VOID WINAPI PutOrderedPacketSucceeded(EXOVERLAPPED* pov);

VOID WINAPI GetInternalMessageSucceeded(EXOVERLAPPED* pov);
VOID WINAPI GetInternalMessageFailed(EXOVERLAPPED* pov);

VOID WINAPI GetMsgSucceeded(EXOVERLAPPED* pov);
VOID WINAPI GetMsgFailed(EXOVERLAPPED* pov);

VOID WINAPI GetNonactiveMessageSucceeded(EXOVERLAPPED* pov);
VOID WINAPI GetNonactiveMessageFailed(EXOVERLAPPED* pov);

VOID WINAPI GetServiceRequestSucceeded(EXOVERLAPPED* pov);
VOID WINAPI GetServiceRequestFailed(EXOVERLAPPED* pov);



//
// QMOV_ACGetRequest
//
struct QMOV_ACGetRequest 
{
    EXOVERLAPPED qmov;
    CACRequest request;

    QMOV_ACGetRequest() :
        qmov(GetServiceRequestSucceeded, GetServiceRequestFailed),
        request(CACRequest::rfAck)
    {
    }

};

//
//   QMOV_ACGetMsg
//
struct QMOV_ACGetMsg
{
    EXOVERLAPPED    qmov;
    HANDLE          hGroup;            // handle to the group from which the packet is gotten
    CTransportBase* pSession;          // Pointer to session object
    CACPacketPtrs   packetPtrs;   // packet pointers

    QMOV_ACGetMsg(
		EXOVERLAPPED::COMPLETION_ROUTINE pSuccess,
        EXOVERLAPPED::COMPLETION_ROUTINE pFailure
        ) :
        qmov(pSuccess, pFailure),
        hGroup(0),
        pSession(0)
    {
        packetPtrs.pPacket = NULL;
        packetPtrs.pDriverPacket = NULL;
    }
};

//
//  QMOV_ACGetInternalMsg
//
struct QMOV_ACGetInternalMsg
{
    EXOVERLAPPED   qmov;
    HANDLE         hQueue;             // Handle to the queue
    CACPacketPtrs  packetPtrs;   // pointers to packet
    LPRECEIVE_COMPLETION_ROUTINE  lpCallbackReceiveRoutine;

    QMOV_ACGetInternalMsg(HANDLE h, LPRECEIVE_COMPLETION_ROUTINE  pCallback) :
        hQueue(h),
        lpCallbackReceiveRoutine(pCallback),
        qmov(GetInternalMessageSucceeded, GetInternalMessageFailed)
    {
        packetPtrs.pPacket = NULL;
        packetPtrs.pDriverPacket = NULL;
    }
};


//
// QMOV_ACPut
//
struct QMOV_ACPut
{
    EXOVERLAPPED   qmov;
    CTransportBase* pSession;          // pointer to session. Used for sending storage ACK
    DWORD_PTR      dwPktStoreAckNo;    // storage Ack No.

    QMOV_ACPut() :
        qmov(PutPacketSucceeded, PutPacketFailed)
    {
    }
};

//
// QMOV_ACPutOrdered
//
class QMOV_ACPutOrdered 
{
public:
    EXOVERLAPPED   qmov;
    CTransportBase* pSession;          // pointer to session. Used for sending storage ACK
    DWORD_PTR      dwPktStoreAckNo;    // storage Ack No.
	HANDLE          hQueue;
    CACPacketPtrs   packetPtrs;   // packet pointers

    QMOV_ACPutOrdered() :
        qmov(PutOrderedPacketSucceeded, PutOrderedPacketFailed)
    {
    }
};


//
// Function decleration
//
void
QmpGetPacketMustSucceed(
    IN HANDLE hGroup,
    IN QMOV_ACGetMsg* pGetOverlapped
    );


HRESULT
CreateAcPutPacketRequest(
    IN CTransportBase* pSession,
    IN DWORD_PTR dwPktStoreAckNo,
    OUT QMOV_ACPut** ppov
    );


HRESULT
CreateAcPutOrderedPacketRequest(
    IN  CQmPacket *pPkt,
    IN  HANDLE  hQueue,
    IN  CTransportBase  *pSession,
    IN  DWORD_PTR dwPktStoreAckNo,
    OUT QMOV_ACPutOrdered** ppAcPutov
    );

#endif //  __QMTHRD_H__





/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    message.cpp

Abstract:

    This module contains code involved with Message APIs.

Author:

    Erez Haba (erezh) 24-Dec-95

Revision History:

--*/

#include "stdh.h"
#include "ac.h"
#include <_secutil.h>
#include "mqutil.h"
#include <mqcrypt.h>
#include "rtsecutl.h"
#include "acdef.h"
#include "rtprpc.h"
#include "objbase.h"
#define _MTX_NOFORCE_LIBS
#include "comsvcs.h"
#include "TXDTC.H"
#include "xactmq.h"
#include <mqsec.h>
#include <ph.h>
#include <rtdep.h>
#include <SignMqf.h>
#include <autohandle.h>
#include "cry.h"
#include "SignMessageXml.h"
#include "mqformat.h"
#include "mqfutils.h"
#include "authlevel.h"

#include "message.tmh"

extern GUID  g_QMId;

static WCHAR *s_FN=L"rt/message";



static
bool
NeedToSignMqf(
	IN LPCWSTR pwszTargetFormatName
	)
/*++
Routine Description:
	Check if we need to sign with Mqf signature.
	check if Target Queue is MQF or DL.

Arguments:
	pwszTargetFormatName - Target queue FormatName

Returned Value:
	true if we must sign with MQF signature, false if not

--*/
{
	//
	// Check Target Queue FormatName
	//
	AP<QUEUE_FORMAT> pMqf;
	DWORD nMqf;
	CStringsToFree StringsToFree;
	if (!FnMqfToQueueFormats(
			pwszTargetFormatName,
			pMqf,
			&nMqf,
			StringsToFree
			))
	{
		ASSERT(("FnMqfToQueueFormats failed, we should catch this earlier", 0));
		return false;
	}

	ASSERT(nMqf > 0);

	return MQpNeedDestinationMqfHeader(pMqf, nMqf);
}


static
bool
CanSignMqf(
	IN LPCWSTR pwszTargetFormatName,
	IN const CACSendParameters* pSendParams
	)
/*++
Routine Description:
	Check if we need to sign with Mqf signature.
	Check if Response or Admin queues are MQF
	or if Target Queue is MQF or DL.

Arguments:
	pwszTargetFormatName - Target queue FormatName
    pSendParams - pointer to send params.

Returned Value:
	true if we need to sign with MQF signature, false if not

--*/
{
	//
	// Check Target Queue FormatName
	//
	AP<QUEUE_FORMAT> pMqf;
	DWORD nMqf;
	CStringsToFree StringsToFree;
	if (!FnMqfToQueueFormats(
			pwszTargetFormatName,
			pMqf,
			&nMqf,
			StringsToFree
			))
	{
		ASSERT(("FnMqfToQueueFormats failed, we should catch this earlier", 0));
		return false;
	}

	ASSERT(nMqf > 0);

	return MQpNeedMqfHeaders(pMqf, nMqf, pSendParams);
}



//---------------------------------------------------------
//
//  GetThreadEvent(...)
//
//  Description:
//
//      Get RT event for this thread. Get it either from
//      The TLS or create a new one.
//
//  Return Value:
//
//      The event handle
//
//---------------------------------------------------------

HRESULT GetThreadEvent(HANDLE& hEvent)
{
    hEvent = TlsGetValue(g_dwThreadEventIndex);
    if (hEvent != NULL)
    {
        return MQ_OK;
    }
	
	DWORD gle = GetLastError();
	if(gle != NO_ERROR)
	{
		TrERROR(GENERAL, "Failed to get event from TLS. Tls index = %d. %!winerr!", g_dwThreadEventIndex, gle);
		return HRESULT_FROM_WIN32(gle);
	}

    //
    //  Event was never allocated for this thread.
    //
    hEvent = CreateEvent(0, TRUE, TRUE, 0);

    if(hEvent == NULL)
    { 
    	gle = GetLastError();
		TrERROR(GENERAL, "Failed to create event for thread. %!winerr!", gle);
        return HRESULT_FROM_WIN32(gle);
    }

    //
    //  Set the Event first bit to disable completion port posting
    //
    hEvent = (HANDLE)((DWORD_PTR)hEvent | (DWORD_PTR)0x1);

    BOOL fSuccess = TlsSetValue(g_dwThreadEventIndex, hEvent);
	if(!fSuccess)
	{
		gle = GetLastError();
		TrERROR(GENERAL, "Failed to set TLS value.TLS index = %d. %!winerr!",g_dwThreadEventIndex, gle);
		CloseHandle(hEvent);
		return HRESULT_FROM_WIN32(gle);
	}

    return MQ_OK;
}


static
HRESULT
CalcSignutureTypes(
    IN OUT CACSendParameters *pSendParams,
    OUT ULONG*				pulAuthLevel,
	IN LPCWSTR				pwszTargetFormatName,
	IN const CACGetQueueHandleProperties& qhp
	)
/*++

Routine Description:
	
	Decide which signature types(versions) should be sign.

Arguments:
    pSendParams - pointer to send params.
  	pulAuthLevel - authentication level
	pwszTargetFormatName - targert format name
	qhp - queue handle properties, this have the information which protocols are in use

Returned Value:
    MQ_OK, if successful, else error code.

--*/
{
	ASSERT(pSendParams->MsgProps.ulAuthLevel != MQMSG_AUTH_LEVEL_NONE);
	ASSERT(qhp.fProtocolSrmp || qhp.fProtocolMsmq);

	ULONG ulAuthLevelSrmp = MQMSG_AUTH_LEVEL_NONE;
	if(qhp.fProtocolSrmp)
	{
		//
		// for Srmp protocol authentication we currently have only one type
		//
		ulAuthLevelSrmp = MQMSG_AUTH_LEVEL_XMLDSIG_V1;
		TrTRACE(SECURITY, "RT: SignutureTypes(), ProtocolSrmp signature MQMSG_AUTH_LEVEL_XMLDSIG_V1");
	}

	ULONG ulAuthLevelMsmq = MQMSG_AUTH_LEVEL_NONE;

	if(qhp.fProtocolMsmq)
	{
		ulAuthLevelMsmq = pSendParams->MsgProps.ulAuthLevel;

		if(pSendParams->MsgProps.ulAuthLevel == MQMSG_AUTH_LEVEL_ALWAYS)
		{
			//
			// See if registry is configured to compute only one signature.
			//
			static DWORD s_dwAuthnLevel =  DEFAULT_SEND_MSG_AUTHN;
			static BOOL  s_fAuthnAlreadyRead = FALSE;

			if (!s_fAuthnAlreadyRead)
			{
				DWORD dwSize = sizeof(DWORD);
				DWORD dwType = REG_DWORD;

				LONG res = GetFalconKeyValue(
									  SEND_MSG_AUTHN_REGNAME,
									 &dwType,
									 &s_dwAuthnLevel,
									 &dwSize
									 );

				if (res != ERROR_SUCCESS)
				{
					s_dwAuthnLevel =  DEFAULT_SEND_MSG_AUTHN;
					TrTRACE(SECURITY, "RT: SignutureTypes(), registry key not exist using default = %d", DEFAULT_SEND_MSG_AUTHN);
				}
				else if (!IS_VALID_AUTH_LEVEL(s_dwAuthnLevel))
				{
					//
					// Allow only AUTH_LEVEL_MASK bits to be set
					// Wrong value in registry. Use the default, to have
					// predictable results.
					//
					TrWARNING(SECURITY, "RT: SignutureTypes(), Wrong registry value %d (invalid bits), using default = %d", s_dwAuthnLevel, DEFAULT_SEND_MSG_AUTHN);
					s_dwAuthnLevel = DEFAULT_SEND_MSG_AUTHN;
				}
				else if (IS_AUTH_LEVEL_ALWAYS_BIT(s_dwAuthnLevel) && (s_dwAuthnLevel != MQMSG_AUTH_LEVEL_ALWAYS))
				{
					//
					// MQMSG_AUTH_LEVEL_ALWAYS bit can not be set with other bits
					// Wrong value in registry. Use the default, to have
					// predictable results.
					//
					TrWARNING(SECURITY, "RT: SignutureTypes(), Wrong registry value %d (ALWAYS bit set with other bits), using default = %d", s_dwAuthnLevel, DEFAULT_SEND_MSG_AUTHN);
					s_dwAuthnLevel = DEFAULT_SEND_MSG_AUTHN;
				}

				s_fAuthnAlreadyRead = TRUE;

				//
				// This should be the default.
				// by default, authenticate only with old style, to prevent
				// performance hit and to be backward  compatible.
				//
				ASSERT(DEFAULT_SEND_MSG_AUTHN == MQMSG_AUTH_LEVEL_SIG10);
			}
			ulAuthLevelMsmq = s_dwAuthnLevel;
			TrTRACE(SECURITY, "RT: SignutureTypes(), MQMSG_AUTH_LEVEL_ALWAYS(read registry): ulAuthLevelMsmq = %d", ulAuthLevelMsmq);
		}

		if(ulAuthLevelMsmq == MQMSG_AUTH_LEVEL_ALWAYS)
		{
			//
			// We were asked to compute all possible signatures.
			// Replace the MQMSG_AUTH_LEVEL_ALWAYS with value that set all the signature bits
			//
			ulAuthLevelMsmq = (MQMSG_AUTH_LEVEL_SIG10 | MQMSG_AUTH_LEVEL_SIG20 | MQMSG_AUTH_LEVEL_SIG30);
		}

		if(IS_AUTH_LEVEL_SIG20_BIT(ulAuthLevelMsmq))
		{
			//
			// The user ask to compute MSMQ20 signature
			// Check if we can sign MSMQ20 signature
			//
			if(NeedToSignMqf(pwszTargetFormatName))
			{
				//
				// ISSUE-2000/11/05-ilanhh should we return error if registry is configured to MQMSG_AUTH_LEVEL_SIG20 ?
				// or just if user application asked for MQMSG_AUTH_LEVEL_SIG20
				// pSendParams->MsgProps.ulAuthLevel is the user application input
				// ulAuthLevel is also the registry setting in case MQMSG_AUTH_LEVEL_ALWAYS
				//
				if(ulAuthLevelMsmq == MQMSG_AUTH_LEVEL_SIG20)		
				{
					//
					// User ask specifically for MSMQ20 signature
					//
					TrERROR(SECURITY, "User (or registry) ask specifically for MSMQ20 signature, but DestinationQueue is Mqf or DL");
					return MQ_ERROR_CANNOT_SIGN_DATA_EX;
				}

				//
				// MSMQ20 will failes, remove MQMSG_AUTH_LEVEL_SIG20 bit
				//
				CLEAR_AUTH_LEVEL_SIG20_BIT(ulAuthLevelMsmq);

				//
				// Turn on MQMSG_AUTH_LEVEL_SIG10, MQMSG_AUTH_LEVEL_SIG30 signature bits
				//
				SET_AUTH_LEVEL_SIG10_BIT(ulAuthLevelMsmq);
				SET_AUTH_LEVEL_SIG30_BIT(ulAuthLevelMsmq);

				TrWARNING(SECURITY, "RT: SignutureTypes(), DestinationQueue is Mqf or DL, Replace MSMQ20 signature with MSMQ10 and MSMQ30 signatures");
			}

			//
			// ISSUE-2000/11/05-ilanhh we don't convert MQMSG_AUTH_LEVEL_SIG20 to MQMSG_AUTH_LEVEL_SIG30
			// if we NeedMqfSignature but not MustMqfSignature (Admin, Response queues)
			// in that case the user should ask for MQMSG_AUTH_LEVEL_SIG30 if he wants it.
			//
		}

		if(IS_AUTH_LEVEL_SIG30_BIT(ulAuthLevelMsmq))
		{
			//
			// We will not prepare MSMQ30 signature if we don't have MQF headers
			// BUGBUG: need to support this. ilanh 05-Nov-2000
			//
			if(!CanSignMqf(pwszTargetFormatName, pSendParams))
			{
				//
				// Mqf headers will not be include in the packet
				// Remove MQMSG_AUTH_LEVEL_SIG30 bit
				// and replace it with MQMSG_AUTH_LEVEL_SIG20 signature
				// which in this case is almost the same
				//
				CLEAR_AUTH_LEVEL_SIG30_BIT(ulAuthLevelMsmq);
				SET_AUTH_LEVEL_SIG20_BIT(ulAuthLevelMsmq);

				TrWARNING(SECURITY, "RT: SignutureTypes(), We dont have any MQF headers, Replace MSMQ30 signature with MSMQ20 signatures");
			}
		}
	}

	ULONG ulAuthLevel = ulAuthLevelMsmq | ulAuthLevelSrmp;
	pSendParams->MsgProps.ulAuthLevel = ulAuthLevel;
	*pulAuthLevel = ulAuthLevel;
	return MQ_OK;
}


static
bool
ShouldSignMessage(
    IN CACMessageProperties* pMsgProps
	)
/*++

Routine Description:
	
	Check if we should sign the message.

Arguments:
    pMsgProps - pointer to message properties.

Returned Value:
    true if the message should be signed, false if not.

--*/
{
	if(pMsgProps->ulAuthLevel == MQMSG_AUTH_LEVEL_NONE)
	{
		TrTRACE(SECURITY, "RT: ShouldSignMessage() = false");
		return false;
	}

	ASSERT(IS_VALID_AUTH_LEVEL(pMsgProps->ulAuthLevel));

	TrTRACE(SECURITY, "RT: ShouldSignMessage() = true");
	return true;

}


//+-------------------------------------
//
//  HRESULT  _BeginToSignMessage()
//
//+-------------------------------------

static
HRESULT
_BeginToSignMessage(
	IN CACMessageProperties * pMsgProps,
	IN PMQSECURITY_CONTEXT    pSecCtx,
	OUT HCRYPTHASH          * phHash
	)
{
    HRESULT hr;
    DWORD   dwErr;

    ASSERT(pSecCtx);

    if (!pSecCtx->hProv)
    {
        //
        // Import the private key into process hive.
        //
        hr = RTpImportPrivateKey(pSecCtx);
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 20);
        }
    }
    ASSERT(pSecCtx->hProv);

    //
    // Create the hash object.
    //
    if (!CryptCreateHash(
            pSecCtx->hProv,
            *pMsgProps->pulHashAlg,
            0,
            0,
            phHash
			))
    {
        dwErr = GetLastError();
        TrERROR(SECURITY, "RT: _BeginToSignMessage(), fail at CryptCreateHash(), err- %lxh", dwErr);

        LogNTStatus(dwErr, s_FN, 29);
        return LogHR(MQ_ERROR_CORRUPTED_SECURITY_DATA, s_FN, 30);
    }

    return MQ_OK;
}

//-------------------------------------------------------------------------
//
//  HRESULT SignMessage()
//
//  Description:
//
//      Signs the messag body. compute the hash, and sign it with private
//      key. This add a signature section to the packet
//
//  Return Value:
//
//      MQ_OK, if successful, else error code.
//
//-------------------------------------------------------------------------

static
HRESULT
SignMessage(
	IN CACSendParameters * pSendParams,
	IN PMQSECURITY_CONTEXT pSecCtx
	)
{
    HCRYPTHASH  hHash = NULL;

    CACMessageProperties * pMsgProps = &pSendParams->MsgProps;

    HRESULT hr =  _BeginToSignMessage(
						pMsgProps,
						pSecCtx,
						&hHash
						);
    if (FAILED(hr))
    {
        return hr;
    }
    CHCryptHash hAutoRelHash = hHash;

	//
	// Prepare old QueueFormat response and admin queues
	//
    QUEUE_FORMAT   ResponseQueueFormat;
    QUEUE_FORMAT * pResponseQueueFormat = &ResponseQueueFormat;
    MQpMqf2SingleQ(pSendParams->nResponseMqf, pSendParams->ResponseMqf, &pResponseQueueFormat);

    QUEUE_FORMAT   AdminQueueFormat;
    QUEUE_FORMAT * pAdminQueueFormat = &AdminQueueFormat;
    MQpMqf2SingleQ(pSendParams->nAdminMqf, pSendParams->AdminMqf, &pAdminQueueFormat);

	hr = HashMessageProperties( // Compute the hash value for the mesage body.
            hHash,
            pMsgProps->ppCorrelationID ? *pMsgProps->ppCorrelationID : NULL,
            PROPID_M_CORRELATIONID_SIZE,
            pMsgProps->pApplicationTag ? *pMsgProps->pApplicationTag : DEFAULT_M_APPSPECIFIC,
            pMsgProps->ppBody ? *pMsgProps->ppBody : NULL,
            pMsgProps->ulBodyBufferSizeInBytes,
            pMsgProps->ppTitle ? *pMsgProps->ppTitle : NULL,
            pMsgProps->ulTitleBufferSizeInWCHARs * sizeof(WCHAR),
            pResponseQueueFormat,
            pAdminQueueFormat
			);

    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 40);
    }

    if (!CryptSignHash(        // Sign the mesage.
            hHash,
			pSecCtx->dwPrivateKeySpec,
            NULL,
            0,
            *(pMsgProps->ppSignature),
            &pMsgProps->ulSignatureSize
			))
    {
        DWORD dwErr = GetLastError();
        TrERROR(SECURITY, "CryptSignHash() failed, err- %!winerr!", dwErr);
        return MQ_ERROR_CORRUPTED_SECURITY_DATA;
    }

    //
    // On receiver side, only signature size indicate that message was
    // signed by sender. Verify the size is indeed non-zero
    //
    if (pMsgProps->ulSignatureSize == 0)
    {
        TrERROR(SECURITY, "RT: SignMessage(), CryptSignHash return with zero signature size");

        ASSERT(pMsgProps->ulSignatureSize != 0);
        return LogHR(MQ_ERROR_CORRUPTED_SECURITY_DATA, s_FN, 60);
    }

	TrTRACE(SECURITY, "RT: SignMessage() MSMQ10 signature complete ok");
    return(MQ_OK);
}

//---------------------------------------------------------
//
//  _SignMessageEx
//
//  Description:
//
//    Signs properties that were not signed in msmq1.0
//    Properties we sign here:
//    - target queue
//    - source qm guid
//
//  Return Value:
//
//      MQ_OK, if successful, else error code.
//
//---------------------------------------------------------

static
HRESULT
_SignMessageEx(
	IN LPCWSTR				  pwszTargetFormatName,
	IN OUT CACSendParameters  *pSendParams,
	IN PMQSECURITY_CONTEXT     pSecCtx,
	OUT BYTE                  *pSignBufIn,
	OUT DWORD                 *pdwSignSize
	)
{
	//
    // Prepare the necessray structers to be included in packet.
    //
    struct _SecuritySectionEx *pSecEx = (struct _SecuritySectionEx *) pSignBufIn;
    struct _SecuritySubSectionEx *pSubSecEx = (struct _SecuritySubSectionEx *) (&(pSecEx->aData[0]));

    ULONG  ulTestLen = 0;
    USHORT ulTestSections = 0;

#ifdef _DEBUG
{
    //
    // Simulate subsection that precede the signature. To verify that
    // present code is forward compatible if we'll want to add new
    // subsections in future releases.
    //
	BYTE* pSubPtr = NULL;
    static DWORD s_dwPrefixCount = 0;
    static BOOL  s_fPreAlreadyRead = FALSE;

    if (!s_fPreAlreadyRead)
    {
        DWORD dwSize = sizeof(DWORD);
        DWORD dwType = REG_DWORD;

        LONG res = GetFalconKeyValue(
					   PREFIX_SUB_SECTIONS_REGNAME,
					   &dwType,
					   &s_dwPrefixCount,
					   &dwSize
					   );

        if (res != ERROR_SUCCESS)
        {
            s_dwPrefixCount = 0;
        }
        s_fPreAlreadyRead = TRUE;
    }

    for ( USHORT j = 0 ; j < (USHORT) s_dwPrefixCount ; j++ )
    {
        ulTestSections++;
        pSubSecEx->eType = e_SecInfo_Test;
        pSubSecEx->_u.wFlags = 0;
        pSubSecEx->wSubSectionLen = (USHORT) ( (j * 7) + 1 +
                                    sizeof(struct _SecuritySubSectionEx));

        ulTestLen += ALIGNUP4_ULONG(pSubSecEx->wSubSectionLen);
        pSubPtr = ((BYTE*) pSubSecEx) + ALIGNUP4_ULONG(pSubSecEx->wSubSectionLen);
        pSubSecEx = (struct _SecuritySubSectionEx *) pSubPtr;
    }
}
#endif

    pSubSecEx->eType = e_SecInfo_User_Signature_ex;
    pSubSecEx->_u.wFlags = 0;
    pSubSecEx->_u._UserSigEx.m_bfTargetQueue = 1;
    pSubSecEx->_u._UserSigEx.m_bfSourceQMGuid = 1;
    pSubSecEx->_u._UserSigEx.m_bfUserFlags = 1;
    pSubSecEx->_u._UserSigEx.m_bfConnectorType = 1;

    BYTE *pSignBuf = (BYTE*) &(pSubSecEx->aData[0]);

    //
    // start signing (create the hash object).
    //
    HCRYPTHASH hHash;

    CACMessageProperties * pMsgProps = &pSendParams->MsgProps;

    HRESULT hr = _BeginToSignMessage(
					 pMsgProps,
                     pSecCtx,
                     &hHash
					 );

    if (FAILED(hr))
    {
        return hr;
    }
    CHCryptHash hAutoRelHash = hHash;

	//
	// Prepare old QueueFormat response and admin queue
	//
    QUEUE_FORMAT   ResponseQueueFormat;
    QUEUE_FORMAT * pResponseQueueFormat = &ResponseQueueFormat;
    MQpMqf2SingleQ(pSendParams->nResponseMqf, pSendParams->ResponseMqf, &pResponseQueueFormat);

    QUEUE_FORMAT   AdminQueueFormat;
    QUEUE_FORMAT * pAdminQueueFormat = &AdminQueueFormat;
    MQpMqf2SingleQ(pSendParams->nAdminMqf, pSendParams->AdminMqf, &pAdminQueueFormat);

    hr = HashMessageProperties(
             hHash,
             pMsgProps->ppCorrelationID ? *pMsgProps->ppCorrelationID : NULL,
             PROPID_M_CORRELATIONID_SIZE,
             pMsgProps->pApplicationTag ? *pMsgProps->pApplicationTag : DEFAULT_M_APPSPECIFIC,
             pMsgProps->ppBody ? *pMsgProps->ppBody : NULL,
             pMsgProps->ulBodyBufferSizeInBytes,
             pMsgProps->ppTitle ? *pMsgProps->ppTitle : NULL,
             pMsgProps->ulTitleBufferSizeInWCHARs * sizeof(WCHAR),
             pResponseQueueFormat,
             pAdminQueueFormat
			 );

    if (FAILED(hr))
    {
        return(hr);
    }

    //
    // Prepare structure of flags.
    //
    struct _MsgFlags sUserFlags;
    memset(&sUserFlags, 0, sizeof(sUserFlags));

    sUserFlags.bDelivery = DEFAULT_M_DELIVERY;
    sUserFlags.bPriority = DEFAULT_M_PRIORITY;
    sUserFlags.bAuditing = DEFAULT_M_JOURNAL;
    sUserFlags.bAck      = DEFAULT_M_ACKNOWLEDGE;
    sUserFlags.usClass   = MQMSG_CLASS_NORMAL;

    if (pMsgProps->pDelivery)
    {
        sUserFlags.bDelivery = *(pMsgProps->pDelivery);
    }
    if (pMsgProps->pPriority)
    {
        sUserFlags.bPriority = *(pMsgProps->pPriority);
    }
    if (pMsgProps->pAuditing)
    {
        sUserFlags.bAuditing = *(pMsgProps->pAuditing);
    }
    if (pMsgProps->pAcknowledge)
    {
        sUserFlags.bAck      = *(pMsgProps->pAcknowledge);
    }
    if (pMsgProps->pClass)
    {
        sUserFlags.usClass   = *(pMsgProps->pClass);
    }
    if (pMsgProps->pulBodyType)
    {
        sUserFlags.ulBodyType = *(pMsgProps->pulBodyType);
    }

    GUID guidConnector = GUID_NULL;
    const GUID *pConnectorGuid = &guidConnector;
    if (pMsgProps->ppConnectorType)
    {
        pConnectorGuid = *(pMsgProps->ppConnectorType);
    }

    //
    // Prepare array of properties to hash.
    // (_MsgHashData already include one property).
    //
    DWORD dwStructSize = sizeof(struct _MsgHashData) +
                            (3 * sizeof(struct _MsgPropEntry));
    P<struct _MsgHashData> pHashData =
                        (struct _MsgHashData *) new BYTE[ dwStructSize ];

    pHashData->cEntries = 4;
    (pHashData->aEntries[0]).dwSize = (1 + wcslen(pwszTargetFormatName)) * sizeof(WCHAR);
    (pHashData->aEntries[0]).pData = (const BYTE*) pwszTargetFormatName;
    (pHashData->aEntries[1]).dwSize = sizeof(GUID);
    (pHashData->aEntries[1]).pData = (const BYTE*) &g_QMId;
    (pHashData->aEntries[2]).dwSize = sizeof(sUserFlags);
    (pHashData->aEntries[2]).pData = (const BYTE*) &sUserFlags;
    (pHashData->aEntries[3]).dwSize = sizeof(GUID);
    (pHashData->aEntries[3]).pData = (const BYTE*) pConnectorGuid;

    hr = MQSigHashMessageProperties(hHash, pHashData);
    if (FAILED(hr))
    {
        return hr;
    }

    //
    // sign the has with private key.
    //
    if (!CryptSignHash(
            hHash,
			pSecCtx->dwPrivateKeySpec,
            NULL,
            0,
            pSignBuf,
            pdwSignSize
			))
    {
    	DWORD gle = GetLastError();
        TrERROR(SECURITY, "CryptSignHash() failed, err = %!winerr!", gle);
        return MQ_ERROR_CANNOT_SIGN_DATA_EX;
    }

    //
    // On receiver side, only signature size indicate that message was
    // signed by sender. Verify the size is indeed non-zero
    //
    if (*pdwSignSize == 0)
    {
        TrERROR(SECURITY, "_SignMessageEx(), CryptSignHash return with zero signature size");

        ASSERT(*pdwSignSize != 0);
        return MQ_ERROR_CANNOT_SIGN_DATA_EX;
    }

    pSubSecEx->wSubSectionLen = (USHORT)
                    (sizeof(struct _SecuritySubSectionEx) + *pdwSignSize);
    ULONG ulSignExLen = ALIGNUP4_ULONG(pSubSecEx->wSubSectionLen);

#ifdef _DEBUG
{
	BYTE* pSubPtr = NULL;
    //
    // Simulate subsection that succeed the signature. To verify that
    // present code is forward compatible if we'll want to add new
    // subsections in future releases.
    //
    static DWORD s_dwPostfixCount = 0;
    static BOOL  s_fPostAlreadyRead = FALSE;

    if (!s_fPostAlreadyRead)
    {
        DWORD dwSize = sizeof(DWORD);
        DWORD dwType = REG_DWORD;

        LONG res = GetFalconKeyValue(
					   POSTFIX_SUB_SECTIONS_REGNAME,
					   &dwType,
					   &s_dwPostfixCount,
					   &dwSize
					   );
        if (res != ERROR_SUCCESS)
        {
            s_dwPostfixCount = 0;
        }
        s_fPostAlreadyRead = TRUE;
    }

    pSubPtr = ((BYTE*) pSubSecEx) + ulSignExLen;

    for ( USHORT j = 0; j < (USHORT) s_dwPostfixCount; j++ )
    {
        ulTestSections++;
        pSubSecEx = (struct _SecuritySubSectionEx *) pSubPtr;
        pSubSecEx->eType = e_SecInfo_Test;
        pSubSecEx->_u.wFlags = 0;
        pSubSecEx->wSubSectionLen = (USHORT) ( (j * 11) + 1 +
                                   sizeof(struct _SecuritySubSectionEx));

        ulTestLen += ALIGNUP4_ULONG(pSubSecEx->wSubSectionLen);
        pSubPtr = ((BYTE*) pSubSecEx) + ALIGNUP4_ULONG(pSubSecEx->wSubSectionLen);
    }
}
#endif

    pSecEx->cSubSectionCount = (USHORT) (1 + ulTestSections);
    pSecEx->wSectionLen = (USHORT) ( sizeof(struct _SecuritySectionEx)   +
                                     ulSignExLen                         +
                                     ulTestLen );

    *pdwSignSize = pSecEx->wSectionLen;

	TrTRACE(SECURITY, "RT: _SignMessageEx() MSMQ20 signature complete ok");
    return MQ_OK;
}

//+-------------------------------------------------------
//
//  BOOL  ShouldEncryptMessage()
//
//  Return TRUE, if the message should be encrypted.
//
//+-------------------------------------------------------

static
BOOL
ShouldEncryptMessage(
    CACMessageProperties * pMsgProps,
    enum enumProvider    * peProvider
    )
{
    BOOL bRet = FALSE;

    if (!pMsgProps->ulBodyBufferSizeInBytes)
    {
        //
        // No message body, nothing to encrypt.
        //
        return(FALSE);
    }

    switch (*pMsgProps->pulPrivLevel)
    {
    case MQMSG_PRIV_LEVEL_NONE:
        bRet = FALSE;
        break;

    case MQMSG_PRIV_LEVEL_BODY_BASE:
        *peProvider = eBaseProvider;
        bRet = TRUE;
        break;

    case MQMSG_PRIV_LEVEL_BODY_ENHANCED:
        *peProvider = eEnhancedProvider;
        bRet = TRUE;
        break;
    }

    return(bRet);
}

//=--------------------------------------------------------------------------=
// HELPER: GetCurrentViperTransaction
//
// Gets current COM+ transaction if there is one...
//
// CoGetObjectContext is exported by OLE32.dll
// IObjectContextInfo is defined in the latest COM+ SDK (part of the platform SDK)
//=--------------------------------------------------------------------------=

static HRESULT  GetCurrentViperTransaction(OUT ITransaction **ppITransaction)
{
    *ppITransaction = NULL;
    IObjectContextInfo *pInfo  = NULL;

    HRESULT hr = CoGetObjectContext(IID_IObjectContextInfo, (void **)&pInfo);
    if (SUCCEEDED(hr) && pInfo)
    {
        //
        // Win bug 606598.
        // We're in the context of COM+.
        // if GetTransaction() succeed then:
        // 1. if pTransaction is NULL, then we're outside of a transaction.
        // 2. if not-NULL, then we're inside transaction context.
        // If GetTransaction fail then we fail the api.
        //
    	hr = pInfo -> GetTransaction((IUnknown **) ppITransaction);
	    pInfo -> Release();

        if (FAILED(hr))
        {
		    TrERROR(XACT_GENERAL, "GetTransaction() failed, hr- 0x%lx", hr);
        }
    }
    else if (hr == E_NOINTERFACE)
    {
        //
        // We know for sure the we're not in the context of COM+.
        // Go on without transaction.
        //
        hr = MQ_OK ;
    }
    else
    {
        //
        // Let's fail the api. We have no idea what happened.
        //
		TrERROR(XACT_GENERAL, "CoGetObjectContext() failed, hr- 0x%lx", hr);
        hr = MQ_ERROR_DTC_CONNECT ;
    }

    return hr ;
}

//=--------------------------------------------------------------------------=
// HELPER: GetCurrentXATransaction
// Gets current XA transaction if there is one...
//=--------------------------------------------------------------------------=

static HRESULT  GetCurrentXATransaction(ITransaction **ppITransaction)
{
    *ppITransaction = NULL ;
    IXATransLookup *pXALookup = NULL;
    HRESULT         hr = MQ_OK;
    IUnknown       *punkDtc = NULL;

    hr = XactGetDTC(&punkDtc);

    if (FAILED(hr) || punkDtc==NULL)
    {
        LogHR(hr, s_FN, 80);
        return  MQ_ERROR_DTC_CONNECT ;
    }

    // Get the DTC  ITransactionImportWhereabouts interface
    hr = punkDtc->QueryInterface (IID_IXATransLookup, (void **)(&pXALookup));
    punkDtc->Release();
    if (FAILED(hr))
    {
        LogHR(hr, s_FN, 90);
        return hr ;
    }
    ASSERT(pXALookup);

    hr = pXALookup->Lookup(ppITransaction);
    pXALookup->Release();

    if (hr == XACT_E_NOTRANSACTION)
    {
        //
        // Win bug 606598.
        // We're outside of a XA transaction, for sure.
        //
        *ppITransaction = NULL ;
        hr = MQ_OK ;
    }
    else if (FAILED(hr))
    {
        LogHR(hr, s_FN, 100);
    }

    return hr ;
}


static
HRESULT
GetCertAndSecurityContext(
    IN OUT CACMessageProperties *pMsgProps,
	OUT BYTE **ppUserCert,
    IN OUT PMQSECURITY_CONTEXT& pSecCtx,
	OUT P<MQSECURITY_CONTEXT>& pSecCtxToFree
	)
/*++

Routine Description:
	Check and initialize the User Certificate and the security context.
	Note: This function replaced the block in RTpSendMessage that dealt with this

Arguments:
    pMsgProps - pointer to send message properties.
	ppUserCert - pointer to the user certificate.
	pSecCtx - pointer to the security context.
	pSecCtxToFree - auto pointer for releasing the created temporary security context

Returned Value:
    MQ_OK, if successful, else error code.

--*/
{
    BOOL bShouldGetCertInfo = TRUE;

    if (!pSecCtx)
    {
        //
        // Security context NOT provided by caller, in a
        // message property.
        //
        if (!pMsgProps->ppSenderCert)
        {
            //
            // Caller also did not provide a certificate in the
            // message properties array. In this case we take the
            // cached security context of the process.
            //

            HRESULT hr = InitSecurityContextCertInfo();
            if(FAILED(hr))
			{
				TrERROR(GENERAL, "Failed to init the certificate info. %!hresult!", hr);
				return hr;
			}
            
            if (!g_pSecCntx->pUserCert)
            {
                //
                // The process does not have an internal
                // certificate, there is nothing that we can do
                // but fail.
                //
                return LogHR(MQ_ERROR_NO_INTERNAL_USER_CERT, s_FN, 152);
            }
            *ppUserCert = g_pSecCntx->pUserCert;
            pMsgProps->ppSenderCert = ppUserCert;
            pMsgProps->ulSenderCertLen = g_pSecCntx->dwUserCertLen;
            pSecCtx = g_pSecCntx;
            bShouldGetCertInfo = FALSE;
        }
    }
    else
    {
        if (!pMsgProps->ppSenderCert)
        {
            //
            // Caller provided a security context, but not a
            // certificate. We take the certificate from the
            // security context.
            //
            *ppUserCert = pSecCtx->pUserCert;
            pMsgProps->ppSenderCert = ppUserCert;
            pMsgProps->ulSenderCertLen = pSecCtx->dwUserCertLen;
            bShouldGetCertInfo = FALSE;
        }
        else
        {
            //
            // We have a security context and a certificate in
            // PROPID_M_USER_CERT. In this case, we should use
            // the certificate in PROPID_M_USER_CERT. We can use
            // the cashed certificate information in the security
            // context, if the certificate in the security context
            // is the same as in PROPID_M_USER_CERT.
            //
            bShouldGetCertInfo =
                (pSecCtx->dwUserCertLen != pMsgProps->ulSenderCertLen) ||
                (memcmp(
                     pSecCtx->pUserCert,
                     *pMsgProps->ppSenderCert,
                     pMsgProps->ulSenderCertLen
					 ) != 0);
        }
    }

    if (bShouldGetCertInfo)
    {
        //
        // Caller provided a certificate, but not a security
        // context.  Get all the information for the certificate.
        // We put the certificate information in a temporary
        // security context.
        //
        ASSERT(pMsgProps->ppSenderCert);

	    pSecCtxToFree = AllocSecurityContext();
		PMQSECURITY_CONTEXT pTmpSecCtx = pSecCtxToFree.get();

        HRESULT hr = GetCertInfo(
                         false,
						 pTmpSecCtx->fLocalSystem,
						 pMsgProps->ppSenderCert,
						 &pMsgProps->ulSenderCertLen,
						 &pTmpSecCtx->hProv,
						 &pTmpSecCtx->wszProvName,
						 &pTmpSecCtx->dwProvType,
						 &pTmpSecCtx->bDefProv,
						 &pTmpSecCtx->bInternalCert,
						 &pTmpSecCtx->dwPrivateKeySpec
						 );

        //
        // The caller can not provide the internal certificate as
        // a message property, only his own externel certificate.
        // ASSERT this condition.
        //
        ASSERT(!(pTmpSecCtx->bInternalCert));

		//
		// dwPrivateKeySpec	must be AT_SIGNATURE or AT_KEYEXCHANGE
		//
        ASSERT((pTmpSecCtx->dwPrivateKeySpec == AT_SIGNATURE) ||
			   (pTmpSecCtx->dwPrivateKeySpec == AT_KEYEXCHANGE));

        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 160);
        }

        if (pSecCtx)
        {
            //
            // If we got the certificate from PROPID_M_USER_CERT,
            // but we have also a security context, we should get
            // the sender ID from the security context. So copy
            // the sender ID from the security context that we
            // get from the application into the temporary
            // security context.
            //
            pTmpSecCtx->fLocalUser = pSecCtx->fLocalUser;

            if (!pSecCtx->fLocalUser)
            {
                pTmpSecCtx->dwUserSidLen = pSecCtx->dwUserSidLen;
                pTmpSecCtx->pUserSid = new BYTE[pSecCtx->dwUserSidLen];
                BOOL bRet = CopySid(
                                pSecCtx->dwUserSidLen,
                                pTmpSecCtx->pUserSid,
                                pSecCtx->pUserSid
								);
                ASSERT(bRet);
				DBG_USED(bRet);
            }
        }
        else
        {
            pTmpSecCtx->fLocalUser = g_pSecCntx->fLocalUser;
        }

        pSecCtx = pTmpSecCtx;
    }

    ASSERT(pSecCtx);
	return MQ_OK;
}


static
HRESULT
SignMessageMsmq12(
    IN PMQSECURITY_CONTEXT pSecCtx,
    IN ULONG ulAuthLevel,
    IN ULONG* pulProvNameSizeAll,
	IN LPCWSTR pwszTargetFormatName,
	IN OUT CACSendParameters *pSendParams,
	IN OUT BYTE *pabMessageSignature,
	OUT AP<BYTE>& pSignatureMqf
	)
/*++

Routine Description:
	Create the Signature for msmq1.0 and msmq2.0
	Note: This function replaced the block in RTpSendMessage that dealt with this

Arguments:
	pSecCtx - pointer to the security context.
	ppUserCert - pointer to the user certificate.
  	ulAuthLevel - authentication level
	pulProvNameSizeAll - provider name size including the extra sections
	pwszTargetFormatName - targert format name
    pSendParams - pointer to send params.
	pabMessageSignature - pointer to signature buffer.
	pSignatureMqf - auto pointer of byte for the signature mqf

Returned Value:
    MQ_OK, if successful, else error code.

--*/
{
	//
	// Get only the AUTH_LEVEL_MSMQ_PROTOCOL bits
	//
	ULONG ulAuthLevelMsmq = GET_AUTH_LEVEL_MSMQ_PROTOCOL(ulAuthLevel);
	TrTRACE(SECURITY, "RT: SignMessageMsmq12(), AUTH_LEVEL_MSMQ_PROTOCOL = %d", ulAuthLevelMsmq);

	//
	// Sign the message.
	//
	ASSERT(ulAuthLevelMsmq != MQMSG_AUTH_LEVEL_ALWAYS);
	ASSERT(ulAuthLevelMsmq != MQMSG_AUTH_LEVEL_NONE);

	if(IS_AUTH_LEVEL_SIG10_BIT(ulAuthLevelMsmq))
	{
		HRESULT hr = SignMessage(pSendParams, pSecCtx);
		if(FAILED(hr))
		{
			return hr;
		}
		ASSERT(pSendParams->MsgProps.ulSignatureSize != 0);
	}
	else
	{
		//
		// Sign only with win2k style or Mqfsignature.
		// make the "msmq1.0" signature dummy, with a single
		// null dword. It's too risky to have a null pointer
		// as msmq1.0 signature, so a dummy value is better.
		// win2k code will ignore it anyway.
		//
		pSendParams->MsgProps.ulSignatureSize = 4;
		memset(pabMessageSignature, 0, pSendParams->MsgProps.ulSignatureSize);
	}

	//
	// Now create the "Extra" signature. Sign all those
	// properties that were not signed on msmq1.0.
	//
	BYTE abMessageSignatureEx[MAX_MESSAGE_SIGNATURE_SIZE_EX];
	DWORD dwSignSizeEx = sizeof(abMessageSignatureEx);

	if (ulAuthLevelMsmq == MQMSG_AUTH_LEVEL_SIG10)
	{
		//
		// enhanced signature (win2k style) not needed.
		//
		dwSignSizeEx = 0;
	}
	else
	{
		ASSERT(IS_AUTH_LEVEL_SIG20_BIT(ulAuthLevelMsmq) || IS_AUTH_LEVEL_SIG30_BIT(ulAuthLevelMsmq));
		
		//
		// Currently if need EX signature check if we need to sign with Mqf Signature
		//
		HRESULT hr;
		if(IS_AUTH_LEVEL_SIG30_BIT(ulAuthLevelMsmq))
		{
			hr = SignMqf(
						pSecCtx,
						pwszTargetFormatName,
						pSendParams,
						pSignatureMqf,							
						&pSendParams->SignatureMqfSize			
						);
			if(FAILED(hr))
			{
				return hr;
			}
		}

		if(!IS_AUTH_LEVEL_SIG20_BIT(ulAuthLevelMsmq))
		{
			return MQ_OK;
		}

		hr = _SignMessageEx(
				 pwszTargetFormatName,
				 pSendParams,
				 pSecCtx,
				 abMessageSignatureEx,
				 &dwSignSizeEx
				 );

		if(FAILED(hr))
		{
			return hr;
		}

		ASSERT(dwSignSizeEx != 0);
	}

	//
	// Copy the Ex signature to the standard signature buffer.
	// The driver will separate them and insert them in the
	// packet in the proper place. This is necessary to keep
	// the send parameters buffer without changes.
	//
	if (dwSignSizeEx == 0)
	{
		//
		// Signature not created. That's ok.
		//
	}
	else if ((dwSignSizeEx + pSendParams->MsgProps.ulSignatureSize) <=
								 MAX_MESSAGE_SIGNATURE_SIZE_EX)
	{
		memcpy(
			&(pabMessageSignature[pSendParams->MsgProps.ulSignatureSize ]),
			abMessageSignatureEx,
			dwSignSizeEx
			);

		pSendParams->MsgProps.ulSignatureSize += dwSignSizeEx;

		//
		// Compute size of authentication "provider" field. This
		// field contain the provider name and extra authentication
		// data that was added for post win2k rtm.
		//
		*pulProvNameSizeAll = dwSignSizeEx +
					 ALIGNUP4_ULONG(ComputeAuthProvNameSize(&pSendParams->MsgProps));
		pSendParams->MsgProps.pulAuthProvNameLenProp = pulProvNameSizeAll;
	}
	else
	{
		ASSERT(("Total size of msmq2.0 signature_ex > MAX_MESSAGE_SIGNATURE_SIZE_EX", 0));
		LogHR(MQ_ERROR, s_FN, 170);
	}
	return MQ_OK;
}


static
HRESULT
HandleSignature(
    IN QUEUEHANDLE  hQueue,
	IN OUT CACSendParameters *pSendParams,
    OUT PMQSECURITY_CONTEXT& pSecCtx,
	OUT P<MQSECURITY_CONTEXT>& pSecCtxToFree,
	OUT BYTE **ppUserCert,
    OUT ULONG *pulAuthLevel,
    OUT ULONG *pulProvNameSizeAll,
	OUT WCHAR **ppProvName,
	OUT BYTE **ppabMessageSignature,
	IN ULONG abMessageSignatureSize,
	OUT AP<char>& pSignatureElement,
	OUT AP<BYTE>& pSignatureMqf
	)
/*++

Routine Description:
	Handle the Signature
	Note: This function replaced the block in RTpSendMessage that dealt with the signature

Arguments:
	hQueue - queue handle
    pSendParams - pointer to send params.
	pSecCtx - pointer to the security context.
	pSecCtxToFree - auto pointer for releasing the created temporary security context
	ppUserCert - pointer to the user certificate.
  	pulAuthLevel - pointer to long, authentication level
	pulProvNameSizeAll - pointer to long, provider name size including the extra sections
	ppProvName - pointer to provider name wstring.
	ppabMessageSignature - pointer to pointer of the signature buffer.
	abMessageSignatureSize - Signature buffer size
	pSignatureElement - auto pointer of char for the XMLDSIG signature element string
	pSignatureMqf - auto pointer of byte for the mqf signature

Returned Value:
    MQ_OK, if successful, else error code.

--*/
{
	//
	// Signature is given by the user
	//
    if (pSendParams->MsgProps.ppSignature)
    {
        if (!pSecCtx && !pSendParams->MsgProps.ppSenderCert)
        {
            return LogHR(MQ_ERROR_INSUFFICIENT_PROPERTIES, s_FN, 140);
        }
        if (!pSendParams->MsgProps.ppSenderCert)
        {
            //
            // We have a security context and no certificate. We
            // take the certificate from the security context.
            //
            *ppUserCert = pSecCtx->pUserCert;
            pSendParams->MsgProps.ppSenderCert = ppUserCert;
            pSendParams->MsgProps.ulSenderCertLen = pSecCtx->dwUserCertLen;
        }

        if (pSendParams->MsgProps.ppwcsProvName)
        {
            ASSERT(pSendParams->MsgProps.pulProvType);
            pSendParams->MsgProps.fDefaultProvider = FALSE;
        }

		return MQ_OK;
    }

	//
	// Should not sign the message --> signature length = 0
	//
	if (!ShouldSignMessage(&pSendParams->MsgProps))
	{
        pSendParams->MsgProps.ulSignatureSize = 0;
		return MQ_OK;
	}

	//
	// Get Target Queue name and decide whether it is Http message
	//
	DWORD dwTargetFormatNameLength = 0;
	HRESULT hr = ACHandleToFormatName(
					 hQueue,
					 NULL,
					 &dwTargetFormatNameLength
					 );

	if(hr != MQ_ERROR_FORMATNAME_BUFFER_TOO_SMALL)
	{
		//
		// We got other error then the expected MQ_ERROR_FORMATNAME_BUFFER_TOO_SMALL
		// e.g. MQ_ERROR_STALE_HANDLE
		//
		ASSERT(hr != MQ_OK);
        return LogHR(hr, s_FN, 144);
	}

	ASSERT(hr == MQ_ERROR_FORMATNAME_BUFFER_TOO_SMALL);
	ASSERT(dwTargetFormatNameLength > 0);

	AP<WCHAR> pwszTargetFormatName = new WCHAR[dwTargetFormatNameLength];

	hr = ACHandleToFormatName(
			 hQueue,
			 pwszTargetFormatName.get(),
			 &dwTargetFormatNameLength
			 );

	ASSERT(hr != MQ_ERROR_FORMATNAME_BUFFER_TOO_SMALL);

    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 145);
    }

	//
	// Call ACGetQueueHandleProperties to get the types of signatures needed
	// this is based on the queue types we have in the handle.
	//
	CACGetQueueHandleProperties	qhp;
	hr = ACGetQueueHandleProperties(
			 hQueue,
			 qhp
			 );

	if (FAILED(hr))
	{
		return LogHR(hr, s_FN, 146);
	}

	TrTRACE(SECURITY, "RT: HandleSignature(), fProtocolSrmp = %d, fProtocolMsmq = %d", qhp.fProtocolSrmp, qhp.fProtocolMsmq);

	if(!qhp.fProtocolSrmp && !qhp.fProtocolMsmq)
	{
		//
		// Neither of protocols exists!
		// this will be the case for DL= that is empty
		//
		TrTRACE(SECURITY, "RT: HandleSignature(), We have no protocol (empty DL)");
		return LogHR(MQ_OK, s_FN, 147);
	}
	
	hr = CalcSignutureTypes(
			pSendParams,
			pulAuthLevel,
			pwszTargetFormatName.get(),
			qhp
			);

	if (FAILED(hr))
	{
		return LogHR(hr, s_FN, 148);
	}

	//
	// pSecCtx is reference parameters
	//
	hr = GetCertAndSecurityContext(
			 &pSendParams->MsgProps,
			 ppUserCert,
			 pSecCtx,
			 pSecCtxToFree
			 );

	if (FAILED(hr))
	{
		return LogHR(hr, s_FN, 150);
	}

    //
    // Fill the SendParam with the provider information for the
    // certificate.
    //
    if (pSecCtx->wszProvName == NULL)
    {
        //
        // we don't have a provider, so we can't sign.
        //
        ASSERT(pSecCtx->hProv == NULL);
        if (pSendParams->MsgProps.ppSenderCert == NULL)
        {
            //
            // we don't have a certificate. That's a
            // user error.
            //
            return LogHR(MQ_ERROR_CERTIFICATE_NOT_PROVIDED, s_FN, 162);
        }
        else
        {
            return LogHR(MQ_ERROR_CORRUPTED_SECURITY_DATA, s_FN, 164);
        }
    }

    *ppProvName = pSecCtx->wszProvName;
    pSendParams->MsgProps.ppwcsProvName = ppProvName;
    pSendParams->MsgProps.ulProvNameLen = wcslen(pSecCtx->wszProvName) + 1;
    pSendParams->MsgProps.pulProvType = &pSecCtx->dwProvType;
    pSendParams->MsgProps.fDefaultProvider = pSecCtx->bDefProv;

	if(qhp.fProtocolMsmq)
	{
		//
		// msmq1.0 and msmq2.0 signature treatment.
		//
		// Set the buffer for the signature.
		//
		pSendParams->MsgProps.ppSignature = ppabMessageSignature;
		pSendParams->MsgProps.ulSignatureSize = abMessageSignatureSize;

		hr = SignMessageMsmq12(
				 pSecCtx,
				 *pulAuthLevel,
				 pulProvNameSizeAll,
				 pwszTargetFormatName.get(),
				 pSendParams,
				 *ppabMessageSignature,
				 pSignatureMqf
				 );

		if(FAILED(hr))
		{
			return LogHR(hr, s_FN, 168);
		}
	}	
	
	if(!qhp.fProtocolSrmp)
	{
		return LogHR(MQ_OK, s_FN, 169);
	}

	//
	// Handle Srmp message signature
	//
	try
	{
		BYTE** ppSignatureElementByte = reinterpret_cast<BYTE **>(&pSignatureElement);

		hr = SignMessageXmlDSig(
				 pSecCtx,
				 pSendParams,
				 pSignatureElement
				 );


		if(FAILED(hr))
		{
			return LogHR(hr, s_FN, 166);
		}

		pSendParams->ulXmldsigSize = strlen(pSignatureElement.get());
		pSendParams->ppXmldsig = ppSignatureElementByte;

		return MQ_OK;
	}
	catch (const bad_CryptoApi& exp)
	{
        TrERROR(SECURITY, "RT: SignMessageXmlDSig(), bad Crypto Class Api Excption ErrorCode = %x", exp.error());
		DBG_USED(exp);

		return LogHR(MQ_ERROR_CORRUPTED_SECURITY_DATA, s_FN, 167);
	}
	catch (const bad_alloc&)
	{
        TrERROR(SECURITY, "RT: SignMessageXmlDSig(), bad_alloc Excption");
		return MQ_ERROR_INSUFFICIENT_RESOURCES;
	}
}


//---------------------------------------------------------
//
//  RTpSendMessage(...)
//
//  Description:
//
//      Helper function for MQSendMessage
//
//  Return Value:
//
//      HRESULT success code
//
//---------------------------------------------------------
static
HRESULT
RTpSendMessage(
    IN QUEUEHANDLE  hQueue,
    IN MQMSGPROPS*  pmp,
    IN ITransaction *pTransaction
    )
{
    HRESULT hr;
    XACTUOW Uow;


    BYTE* pUserSid;
    BYTE* pUserCert;
    WCHAR* pProvName;

    CMQHResult rc , rc1;
    rc = MQ_OK;
    rc1 = MQ_OK;

    CACSendParameters SendParams;
    BOOL         fSingleTransaction = FALSE;

    //
    // Set defaults.
	//
    ULONG ulDefHashAlg = PROPID_M_DEFUALT_HASH_ALG;
    ULONG ulDefEncryptAlg = PROPID_M_DEFUALT_ENCRYPT_ALG;
    ULONG ulDefPrivLevel = DEFAULT_M_PRIV_LEVEL;
    ULONG ulDefSenderIdType = DEFAULT_M_SENDERID_TYPE;
    ULONG ulSenderIdTypeNone = MQMSG_SENDERID_TYPE_NONE;

    SendParams.MsgProps.pulHashAlg = &ulDefHashAlg;
    SendParams.MsgProps.pulPrivLevel = &ulDefPrivLevel;
    SendParams.MsgProps.pulEncryptAlg = &ulDefEncryptAlg;
    SendParams.MsgProps.pulSenderIDType = &ulDefSenderIdType;
    SendParams.MsgProps.fDefaultProvider = TRUE;
    SendParams.MsgProps.ulAuthLevel = DEFAULT_M_AUTH_LEVEL;

    //
    //  Parse message properties
    //
    PMQSECURITY_CONTEXT pSecCtx;
    CStringsToFree ResponseStringsToFree, AdminStringsToFree;
    rc1 = RTpParseSendMessageProperties(
            SendParams,
            pmp->cProp,
            pmp->aPropID,
            pmp->aPropVar,
            pmp->aStatus,
            &pSecCtx,
            ResponseStringsToFree,
            AdminStringsToFree
			);

    if(FAILED(rc1))
    {
        return LogHR(rc1, s_FN, 110);
    }

    //
    // Look for Viper transaction if any
    //

    //
    // Ref - wrapper to ensure autorelease of the transaction
    //
    R<ITransaction> ref;
    if (pTransaction == MQ_MTS_TRANSACTION)
    {
        hr = GetCurrentViperTransaction(&pTransaction);
		if(FAILED(hr))
		{
            return LogHR(hr, s_FN, 112);
		}
	    ref = pTransaction;
	}
    else if (pTransaction == MQ_XA_TRANSACTION)
    {
        hr = GetCurrentXATransaction(&pTransaction);
		if(FAILED(hr))
		{
            return LogHR(hr, s_FN, 114);
		}
        ref = pTransaction;
    }
    else if (pTransaction == MQ_SINGLE_MESSAGE)
    {
        hr = MQBeginTransaction(&pTransaction);
        if(FAILED(hr))
        {
            return LogHR(hr, s_FN, 120);
        }

        fSingleTransaction    = TRUE;
		if(pTransaction != NULL)
		{
	        ref = pTransaction;
		}
    }

    //
    // Enlist QM in the transaction (with caching);
    //
    if (pTransaction)
    {
        hr = RTpProvideTransactionEnlist(pTransaction, &Uow);
        SendParams.MsgProps.pUow = &Uow;

        if(FAILED(hr))
        {
            LogHR(hr, s_FN, 130);
            return hr;
        }
    }

	//
    // Change values for the transaction case
	//
    static UCHAR Delivery;
    static UCHAR Priority;

    if (pTransaction)
    {
        Delivery = MQMSG_DELIVERY_RECOVERABLE;
        Priority = 0;

        SendParams.MsgProps.pDelivery = &Delivery;
        SendParams.MsgProps.pPriority = &Priority;
    }

    //
    // Treat security
    //
    if (!g_pSecCntx)
    {
        //
        //  It might not be initialized if the queue was
        //  not opened for send;
        //
        InitSecurityContext();
    }

    BYTE abMessageSignature[MAX_MESSAGE_SIGNATURE_SIZE_EX];
    BYTE* pabMessageSignature = abMessageSignature;
	AP<char> pSignatureElement;
	AP<BYTE> pSignatureMqf;
    ULONG ulProvNameSizeAll = 0;
    ULONG ulAuthLevel = 0;
	P<MQSECURITY_CONTEXT> pSecCtxToFree;

	hr = HandleSignature(
			 hQueue,
			 &SendParams,
			 pSecCtx,
			 pSecCtxToFree,
			 &pUserCert,
			 &ulAuthLevel,
			 &ulProvNameSizeAll,
			 &pProvName,
			 &pabMessageSignature,
			 TABLE_SIZE(abMessageSignature),
			 pSignatureElement,
			 pSignatureMqf
			 );

	if(FAILED(hr))
	{
		TrERROR(GENERAL, "Failed to handle signature. %!hresult!", hr);
		return hr;
	}

    if(!SendParams.MsgProps.ppSenderID && *SendParams.MsgProps.pulSenderIDType == MQMSG_SENDERID_TYPE_SID)
    {
        if ((pSecCtx && pSecCtx->fLocalUser) ||
            (!pSecCtx && g_pSecCntx->fLocalUser))
        {
            //
            // In case this is a local user, we do not send the user's
            // SID with the message, eventhough the application asked
            // to send it.
            //
            SendParams.MsgProps.pulSenderIDType = &ulSenderIdTypeNone;
        }
        else
        {
            //
            // We should pass the sender ID. Either get it from the
            // security context, if available, or get it from the
            // cached process security context.
            //
            if (!pSecCtx || !pSecCtx->pUserSid)
            {
                if (!g_pSecCntx->pUserSid)
                {
                    //
                    // The cahced process context does not contain the
                    // sender's SID. There is nothing that we can do but
                    // fail.
                    //
                    return LogHR(MQ_ERROR_COULD_NOT_GET_USER_SID, s_FN, 172);
                }
                pUserSid = g_pSecCntx->pUserSid;
                SendParams.MsgProps.uSenderIDLen = (USHORT)g_pSecCntx->dwUserSidLen;
            }
            else
            {
                pUserSid = pSecCtx->pUserSid;
                SendParams.MsgProps.uSenderIDLen = (USHORT)pSecCtx->dwUserSidLen;
            }
            SendParams.MsgProps.ppSenderID = &pUserSid;
        }
    }

    if (SendParams.MsgProps.ppSymmKeys)
    {
        //
        // the application supplied the symmetric key. In such a case
        // doesn't do any encryption
        //
        //
        // When the symm key is supplied, we assume that the body is encrypted and
        // we mark it as such and ignore PROPID_M_PRIV_LEVEL.
        //
        if (SendParams.MsgProps.pulPrivLevel &&
            (*(SendParams.MsgProps.pulPrivLevel) == MQMSG_PRIV_LEVEL_BODY_ENHANCED))
        {
            //
            // priv level supplied by caller.
            //
        }
        else
        {
            //
            // use default.
            //
            ulDefPrivLevel = MQMSG_PRIV_LEVEL_BODY_BASE;
            SendParams.MsgProps.pulPrivLevel = &ulDefPrivLevel;
        }
        SendParams.MsgProps.bEncrypted = TRUE;
    }
    else
    {
        enum enumProvider eProvider;
        if (ShouldEncryptMessage(&SendParams.MsgProps, &eProvider))
        {
            //
            // If we should use a block cypher enlarge the allocated
            // space for the message body, so it will be able to accomodate
            // the encrypted data.
            //

            if (*SendParams.MsgProps.pulEncryptAlg == CALG_RC2)
            {
                //
                // Make more room for RC2 encryption.
                //
                DWORD dwBlockSize;
                hr = MQSec_GetCryptoProvProperty( eProvider,
                                                  eBlockSize,
                                                  NULL,
                                                 &dwBlockSize );
                if (FAILED(hr))
                {
                    return LogHR(hr, s_FN, 180);
                }

                SendParams.MsgProps.ulAllocBodyBufferInBytes +=
                                          ((2 * dwBlockSize) - 1);
                SendParams.MsgProps.ulAllocBodyBufferInBytes &= ~(dwBlockSize - 1);
            }

            DWORD dwSymmSize;
            hr = MQSec_GetCryptoProvProperty( eProvider,
                                              eSessionKeySize,
                                              NULL,
                                             &dwSymmSize );
            if (FAILED(hr))
            {
                return LogHR(hr, s_FN, 190);
            }

            SendParams.MsgProps.ulSymmKeysSize = dwSymmSize;
        }
    }

    //
    //  Call AC driver
    //
    OVERLAPPED ov = {0};
    hr = GetThreadEvent(ov.hEvent);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 200);
    }



    rc = ACSendMessage(
            hQueue,
            SendParams,
            &ov
            );
    LogHR(rc, s_FN, 298);

    if (rc == MQ_INFORMATION_OPERATION_PENDING)
    {
        //
        //  Wait for send completion
        //
        DWORD dwResult;
        dwResult = WaitForSingleObject(
                        ov.hEvent,
                        INFINITE
                        );

        //
        //  BUGBUG: MQSendMessage, must succeed in WaitForSingleObject
        //
        ASSERT(dwResult == WAIT_OBJECT_0);

        rc = DWORD_PTR_TO_DWORD(ov.Internal);
    }

	if (SUCCEEDED(rc))
	{
		//
		// Log to tracing that a message was sent.
		// Do this only if we are in the proper tracing level
		//
		if (WPP_LEVEL_COMPID_ENABLED(rsTrace, PROFILING))
		{
			DWORD dwMessageDelivery = (NULL != SendParams.MsgProps.pDelivery) ? *(SendParams.MsgProps.pDelivery) : -1;
			DWORD dwMessageClass = (NULL != SendParams.MsgProps.pAcknowledge) ? *(SendParams.MsgProps.pAcknowledge) : -1;
			WCHAR *wszLabel = L"NO LABEL";							
			DWORD dwLabelLen = wcslen(wszLabel);									
			if (NULL != SendParams.MsgProps.ppTitle && NULL != *(SendParams.MsgProps.ppTitle))
			{													
				wszLabel = *(SendParams.MsgProps.ppTitle);			
				dwLabelLen = SendParams.MsgProps.ulTitleBufferSizeInWCHARs; 
			}														
																	
			WCHAR wszQueueName1[200];
			DWORD dwQueueNameLength = TABLE_SIZE(wszQueueName1) - 1;
			HRESULT hr = MQHandleToFormatName(hQueue, wszQueueName1, &dwQueueNameLength);
			WCHAR *wszQName;
			if (SUCCEEDED(hr))
			{
				wszQName = wszQueueName1;
			}
			else
			{
		    	TrERROR(GENERAL, "Failed in MQHandleToFormatName: %!hresult!", hr);
				wszQName = L"*** Unknown Queue ***";																	
			}
			TrTRACE(PROFILING, "MESSAGE TRACE - State:%ls   Queue:%ls    Delivery:0x%x   Class:0x%x   Label:%.*ls", 
				L"Sending message from RT",  
				wszQName,
				dwMessageDelivery,
				dwMessageClass,
				xwcs_t(wszLabel, dwLabelLen));
		}
	}

	delete [] SendParams.AdminMqf;
	delete [] SendParams.ResponseMqf;

    if(FAILED(rc))
    {
        //
        //  ACSendMessage failed (immidiatly or after waiting)
        //
        return LogHR(rc, s_FN, 200);
    }


    if (fSingleTransaction)
    {
		//
        // RPC call to QM for prepare/commit
		//
        rc = pTransaction->Commit(0,0,0);
        if(FAILED(rc))
        {
            return LogHR(rc, s_FN, 210);
        }
    }

	//
	// Bugfix of bug 5588. niraides 18-Jul-2000
	//
    return rc1;

}  // RTpSendMessage


//---------------------------------------------------------
//
//  MQSendMessage(...)
//
//  Description:
//
//      Falcon API.
//      Send a message to a queue
//
//  Return Value:
//
//      HRESULT success code
//
//---------------------------------------------------------
EXTERN_C
HRESULT
APIENTRY
MQSendMessage(
    IN QUEUEHANDLE  hQueue,
    IN MQMSGPROPS*  pmp,
    IN ITransaction *pTransaction
    )
{
	if(g_fDependentClient)
		return DepSendMessage(
					hQueue,
					pmp,
					pTransaction
					);

	HRESULT hr = RtpOneTimeThreadInit();
	if(FAILED(hr))
		return hr;

    CMQHResult rc;

    __try
    {
        rc = RTpSendMessage(hQueue, pmp, pTransaction);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        //
        //  The exception is due to invalid parameter
        //
        rc = GetExceptionCode();
    }

	if(FAILED(rc))
	{
		TrERROR(GENERAL, "Failed to send a message. %!hresult!", rc);
	}

    return rc;
}



//---------------------------------------------------------
//
//  RtpReceiveMessage(...)
//
//  Description:
//
//      Rt Internal - Receive a message from a queue.
//
//  Return Value:
//
//      HRESULT success code
//
//---------------------------------------------------------

HRESULT
RtpReceiveMessage(
    IN HANDLE hQueue,
    IN DWORD dwTimeout,
    IN DWORD dwAction,
    IN MQMSGPROPS* pmp,
    IN OUT LPOVERLAPPED lpOverlapped,
    IN PMQRECEIVECALLBACK fnReceiveCallback,
    IN HANDLE hCursor,
    IN ITransaction *pTransaction,
    ULONGLONG ullLookupId,
    bool fUseLookupId
    )
{
    CMQHResult rc, rc1;
    XACTUOW Uow;
    HRESULT hr = MQ_OK;

    R<ITransaction> TransactionGenerated;

    rc = MQ_OK;
    rc1 = MQ_OK;

	//
	// Look for Viper transaction if any
	//
	if (pTransaction == MQ_MTS_TRANSACTION)
	{
		hr = GetCurrentViperTransaction(&pTransaction) ;
        if (FAILED(hr))
        {
        	TrERROR(GENERAL, "Failed to get the current Viper transaction. %!hresult!", hr);
			return hr;
        }
	    TransactionGenerated = pTransaction ;
	}
	else if (pTransaction == MQ_XA_TRANSACTION)
	{
		hr = GetCurrentXATransaction(&pTransaction) ;
        if (FAILED(hr))
        {
        	TrERROR(GENERAL, "Failed to get the current XA transaction. %!hresult!", hr);
			return hr;
        }
	    TransactionGenerated = pTransaction ;
	}
	else if (pTransaction == MQ_SINGLE_MESSAGE)
	{
		pTransaction = NULL;
	}

	if ((dwAction & MQ_ACTION_PEEK_MASK) == MQ_ACTION_PEEK_MASK ||
		(dwAction & MQ_LOOKUP_PEEK_MASK) == MQ_LOOKUP_PEEK_MASK)
	{
		// PEEK cannot be transacted, but can work with transacted queue
		if (pTransaction != NULL)
		{
			TrERROR(GENERAL, "Error. Peek cannot be transacted.");
			return MQ_ERROR_TRANSACTION_USAGE;
		}
	}

	// Check usage: transaction urges synchronous operation
	if (pTransaction)
	{
		if (lpOverlapped || (fnReceiveCallback!=NULL))  // Transacted Receive is synchronous only
		{
			TrERROR(GENERAL, "Error. Transacted receive must be synchronous.");
			return MQ_ERROR_TRANSACTION_USAGE;
		}
	}

	CACReceiveParameters ReceiveParams;
	ReceiveParams.RequestTimeout = dwTimeout;
	ReceiveParams.Action = dwAction;
	ReceiveParams.Cursor = (hCursor != 0) ? CI2CH(hCursor) : 0;
	ReceiveParams.LookupId = ullLookupId;

	// Enlist QM in the transaction (for the first time);
	// Check that the transaction state is correct
	if (pTransaction)
	{
		// Enlist QM in transaction, if it isn't enlisted already
		hr = RTpProvideTransactionEnlist(pTransaction, &Uow);
		ReceiveParams.MsgProps.pUow = &Uow;

		if(FAILED(hr))
		{
			TrERROR(GENERAL, "Failed to enlist QM in transaction. %!hresult!", hr);
			return hr;
		}
	}

	//
	//  Parse properties
	//
	if(pmp !=0)
	{
		//
		//  Parse message properties, an exception can be raised on access to
		//  pmp fields
		//
		rc1 = RTpParseReceiveMessageProperties(
				ReceiveParams,
				pmp->cProp,
				pmp->aPropID,
				pmp->aPropVar,
				pmp->aStatus);

		if(FAILED(rc1))
		{
			TrERROR(GENERAL, "Failed to parse the message properties. %!hresult!", rc1);
			return rc1;
		}
	}

	OVERLAPPED ov = {0};
	LPOVERLAPPED pov;

	CAutoCallbackDescriptor CallbackDescriptor;

	if (fnReceiveCallback)
	{
		try
		{
			CreateAsyncRxRequest(
				CallbackDescriptor,
				hQueue,
				dwTimeout,
				dwAction,
				pmp,
				lpOverlapped,
				fnReceiveCallback,
				hCursor
				);

			pov = CallbackDescriptor.GetOverlapped();
		}
		catch(const bad_win32_error& err)
		{
			return RTpConvertToMQCode(HRESULT_FROM_WIN32(err.error()));
		}
		catch(const bad_hresult& hr)
		{
			return RTpConvertToMQCode(hr.error());
		}
		catch(const bad_alloc&)
		{
			TrERROR(GENERAL, "Failed to create callback request entry because of low resources.");
			return MQ_ERROR_INSUFFICIENT_RESOURCES;
		}
		catch(const exception&)
		{
			TrERROR(GENERAL, "An exception was thrown while creating a callback request entry.");
			return MQ_ERROR_INSUFFICIENT_RESOURCES;
		}
	}
	else if(lpOverlapped != 0)
	{
		//
		//  Asynchronous (event or completion port)
		//
		pov = lpOverlapped;
	}
	else
	{
		//
		//  Synchronous, uses the TLS event
		//
		hr = GetThreadEvent(ov.hEvent);
        if FAILED(hr)
        {
       		TrERROR(GENERAL, "Failed to get thread event. %!hresult!", hr);
            return hr;
        }
		pov = &ov;
	}

	//
	//  Call AC driver
	//
	ReceiveParams.Asynchronous = (pov != &ov);

	if (fUseLookupId)
	{
		ASSERT(ReceiveParams.Cursor == 0);
		ASSERT(ReceiveParams.RequestTimeout == 0);

		rc = ACReceiveMessageByLookupId(
				hQueue,
				ReceiveParams,
				pov
				);
	}
	else
	{
		rc = ACReceiveMessage(
				hQueue,
				ReceiveParams,
				pov
				);
	}

	if(FAILED(rc))
	{
		TrERROR(GENERAL, "Failed to receive message from the driver. %!hresult!", rc);
	}
	if((rc == MQ_INFORMATION_OPERATION_PENDING) && (pov == &ov))
	{
		//
		//  Wait for receive completion
		//
		DWORD dwResult;
		dwResult = WaitForSingleObject(
						ov.hEvent,
						INFINITE
						);

		//
		//  BUGBUG: MQReceiveMessage, must succeed in WaitForSingleObject
		//
		ASSERT(dwResult == WAIT_OBJECT_0);

		rc = DWORD_PTR_TO_DWORD(ov.Internal);
	}

	if(FAILED(rc))
	{
		//
		//  ACReceiveMessage failed (immidiatly or after waiting)
		//
		if (rc == MQ_ERROR_IO_TIMEOUT)
		{
			TrWARNING(GENERAL, "Failed to receive message from the driver, Erorr: MQ_ERROR_IO_TIMEOUT");
		}
		else
		{
			TrERROR(GENERAL, "Failed to receive message from the driver after waiting on an event. %!hresult!", rc);
		}
		return rc;
	}
	else if(fnReceiveCallback)
	{
		CallbackDescriptor.detach();
	}

	if (rc == MQ_OK)
    {
        //
        //  return message parsing return code
        //  NOTE: only if rc == MQ_OK otherwise PENDING will not pass through
        //
        return rc1;
    }

	return rc;
}

//---------------------------------------------------------
//
//  MQReceiveMessage(...)
//
//  Description:
//
//      Falcon API.
//      Receive a message from a queue.
//
//  Return Value:
//
//      HRESULT success code
//
//---------------------------------------------------------

EXTERN_C
HRESULT
APIENTRY
MQReceiveMessage(
    IN HANDLE hQueue,
    IN DWORD dwTimeout,
    IN DWORD dwAction,
    IN MQMSGPROPS* pmp,
    IN OUT LPOVERLAPPED lpOverlapped,
    IN PMQRECEIVECALLBACK fnReceiveCallback,
    IN HANDLE hCursor,
    IN ITransaction *pTransaction
    )
{
	if(g_fDependentClient)
		return DepReceiveMessage(
					hQueue,
					dwTimeout,
					dwAction,
					pmp,
					lpOverlapped,
					fnReceiveCallback,
					hCursor,
					pTransaction
					);

	HRESULT hr = RtpOneTimeThreadInit();
	if(FAILED(hr))
		return hr;

    __try
	{
		return RtpReceiveMessage(
					hQueue,
					dwTimeout,
					dwAction,
					pmp,
					lpOverlapped,
					fnReceiveCallback,
					hCursor,
					pTransaction,
					0,
					false
					);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{		
		HRESULT rc = GetExceptionCode();
		return LogHR(HRESULT_FROM_WIN32(rc), s_FN, 315);
	}
}


//---------------------------------------------------------
//
//  MQReceiveMessageByLookupId(...)
//
//  Description:
//
//      Falcon API.
//      Receive a message from a queue.using a lookup ID
//
//  Return Value:
//
//      HRESULT success code
//
//---------------------------------------------------------

EXTERN_C
HRESULT
APIENTRY
MQReceiveMessageByLookupId(
    IN HANDLE hQueue,
    IN ULONGLONG ullLookupId,
    IN DWORD dwLookupAction,
    IN MQMSGPROPS* pmp,
    IN OUT LPOVERLAPPED lpOverlapped,
    IN PMQRECEIVECALLBACK fnReceiveCallback,
    IN ITransaction *pTransaction
    )
{
	if(g_fDependentClient)
		return MQ_ERROR_NOT_SUPPORTED_BY_DEPENDENT_CLIENTS;

	HRESULT hr = RtpOneTimeThreadInit();
	if(FAILED(hr))
		return hr;

    switch (dwLookupAction)
    {
        case MQ_LOOKUP_PEEK_FIRST:
            if (ullLookupId != 0)
            {
                return MQ_ERROR_INVALID_PARAMETER;
            }
            ullLookupId = 0;
            dwLookupAction = MQ_LOOKUP_PEEK_NEXT;
            break;

        case MQ_LOOKUP_PEEK_LAST:
            if (ullLookupId != 0)
            {
                return MQ_ERROR_INVALID_PARAMETER;
            }
            ullLookupId = 0xFFFFFFFFFFFFFFFFui64;
            dwLookupAction = MQ_LOOKUP_PEEK_PREV;
            break;

        case MQ_LOOKUP_RECEIVE_FIRST:
            if (ullLookupId != 0)
            {
                return MQ_ERROR_INVALID_PARAMETER;
            }
            ullLookupId = 0;
            dwLookupAction = MQ_LOOKUP_RECEIVE_NEXT;
            break;

        case MQ_LOOKUP_RECEIVE_LAST:
            if (ullLookupId != 0)
            {
                return MQ_ERROR_INVALID_PARAMETER;
            }
            ullLookupId = 0xFFFFFFFFFFFFFFFFui64;
            dwLookupAction = MQ_LOOKUP_RECEIVE_PREV;
            break;

        default:
            NULL;
            break;
    }

    __try
	{
		return RtpReceiveMessage(
					hQueue,
					0,
					dwLookupAction,
					pmp,
					lpOverlapped,
					fnReceiveCallback,
					0,
					pTransaction,
					ullLookupId,
					true
					);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{		
		HRESULT rc = GetExceptionCode();
		return LogHR(HRESULT_FROM_WIN32(rc), s_FN, 316);
	}
}

//---------------------------------------------------------
//
//  MQGetOverlappedResult(...)
//
//  Description:
//
//      Falcon API.
//      Translate and overlapping operation result code.
//
//  Return Value:
//
//      HRESULT success code
//
//---------------------------------------------------------

EXTERN_C
HRESULT
APIENTRY
MQGetOverlappedResult(
    IN LPOVERLAPPED lpOverlapped
    )
{
	if(g_fDependentClient)
		return DepGetOverlappedResult(lpOverlapped);

	HRESULT hr = RtpOneTimeThreadInit();
	if(FAILED(hr))
		return hr;

	return LogHR(RTpConvertToMQCode(DWORD_PTR_TO_DWORD(lpOverlapped->Internal)), s_FN, 320);
}

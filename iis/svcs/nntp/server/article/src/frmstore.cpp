/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    fromclnt.cpp

Abstract:

	Contains InFeed, Article, and Fields code specific to FromStore Infeeds

	As the name suggests, these for for processing articles that come from
	clients. The idea is to be very strict in what is allowed from the client.
	If an article doesn't match spec, it is either fixed or rejected.


Author:

    Carl Kadie (CarlK)     05-Dec-1995

Revision History:

--*/

#ifdef  _NO_TEMPLATES_
#define DEFINE_CGROUPLST_FUNCTIONS
#endif

#include "stdinc.h"
//#include <artcore.h>
//#include    <stdlib.h>

#define FROMSTOREART_SIGNATURE  (DWORD) 'ArtC'

//
// CPool is used to allocate memory while processing an article.
//

CPool  CFromStoreArticle::g_ArticlePool(FROMSTOREART_SIGNATURE);

//
//  Largest possible CFromStoreArticle derived object
//
#define MAX_ARTICLE_SIZE    sizeof( CFromStoreArticle )

//
// An upperbound on the number of article objects that can
// exist at any time.
//
const   unsigned    cbMAX_ARTICLE_SIZE = MAX_ARTICLE_SIZE ;

void*
CFromStoreArticle::operator      new(    size_t  size )
{
        _ASSERT( size <= cbMAX_ARTICLE_SIZE ) ;
        return  g_ArticlePool.Alloc() ;
}

void
CFromStoreArticle::operator      delete( void*   pv )
{
        g_ArticlePool.Free( pv ) ;
}

BOOL
CFromStoreArticle::InitClass(
					void
					)
/*++

Routine Description:

    Preallocates memory for CArticle objects

Arguments:

    None.

Return Value:

    TRUE, if successful. FALSE, otherwise.

--*/
{
	return	g_ArticlePool.ReserveMemory( MAX_ARTICLES, cbMAX_ARTICLE_SIZE ) ;
}


BOOL
CFromStoreArticle::TermClass(
					void
					)
/*++

Routine Description:

    Called when objects are freed.

Arguments:

    None.

Return Value:

    TRUE

--*/
{

	_ASSERT( g_ArticlePool.GetAllocCount() == 0 ) ;

	BOOL b;

	b =	g_ArticlePool.ReleaseMemory() ;
    //delete g_ArticlePool;
    return b;

}


BOOL
CFromStoreArticle::fValidate(
							//CPCString& pcHub,
							//const char * szCommand,
							//CInFeed*	pInFeed,
							CNntpReturn & nntpReturn
							)
/*++

Routine Description:

	Validates an article from a client. Does not change the article
	except to fix (if necessary) the capitalization of some header keywords.

Arguments:

	szCommand - The arguments (if any) used to post/xreplic/etc this article.
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{

	//
	// Check article state
	//

	_ASSERT(asPreParsed == m_articleState);

    //
    // Check required and optional fields
    //

	CField * rgPFields [] = {
            &m_fieldControl,
			&m_fieldSubject,
			&m_fieldNewsgroups,
			&m_fieldDistribution,
			&m_fieldFrom,
			&m_fieldDate,
			&m_fieldFollowupTo,
			&m_fieldReplyTo,
			&m_fieldApproved,
			&m_fieldSender,
			&m_fieldOrganization,
			&m_fieldSummary,
			&m_fieldReferences,
			&m_fieldLines,
			&m_fieldKeyword,
			&m_fieldExpires,
			&m_fieldPath,
			//&m_fieldNNTPPostingHost,  // forget about NNTP Posting Host for now...
			&m_fieldMessageID	// must be last - as we may not want to parse it !
				};
	DWORD cFields = sizeof(rgPFields)/sizeof(CField *);

    // By default, we honor Message-Id
    cFields;

	if (!fFindAndParseList((CField * *) rgPFields, cFields, nntpReturn))
	{
    	return (nntpReturn.fFalse());
        //return FALSE;
    }

	CPCString	pcDate = m_fieldDate.pcGet() ;
	if( pcDate.m_pch != 0 ) {

		if( !AgeCheck( pcDate ) ) {
			nntpReturn.fSet( nrcArticleDateTooOld ) ;
			return	FALSE ;
		}
	}


	//
	// Confirm (and fix, if necessary) the capitalization of the fields
	//

	if (!fConfirmCapsList((CField * *) rgPFields, cFields, nntpReturn))
		return nntpReturn.fFalse();
        //return FALSE;
//
	//!!!CLIENT LATER Not doing anything with control messages
	//

	/* !!!CLIENT LATER
	Body
	SHOULD limit signatures -- !!!LATER
	Early  difficulties in inferring return addresses from article headers led to "signatures": short closing texts,  automatically  added  to  the end of articles by posting agents, identifying the poster and giving his network addresses etc.  If  a  poster

 or posting agent does append a signature to an article, the signature SHOULD be preceded with  a  delimiter line  containing  (only)  two hyphens (ASCII 45) followed by one blank (ASCII  32).   Posting  agents  SHOULD  limit  the length  of  signatures

 since  verbose  excess bordering on abuse is common if no restraint is imposed;  4  lines  is  a common limit.

	Whole Article
	No NULL character allowed
	Header and body lines MAY contain any ASCII characters other than CR (ASCII 13), LF (ASCII 10), and NUL (ASCII 0).
	NO char > oct 127 allowed (unless co-operating)
	Articles  MUST  not  contain  any octet with value exceeding 127, i.e. any octet that is not an ASCII character.
	Limit to 60K -- LATER
	Posters SHOULD limit  posted  articles  to  at  most  60,000 octets,  including  headers  and EOL representations, unless the articles are being posted only within a cooperating sub-net which is known to be capable of handling larger articles gracefully.


  Posting agents presented with a  large  article SHOULD warn the poster and request confirmation.
	*/

	return nntpReturn.fSetOK();
}


BOOL
CFromStoreArticle::fMungeHeaders(
							 CPCString& pcHub,
							 CPCString& pcDNS,
							 //CNAMEREFLIST & grouplist,
							 DWORD remoteIpAddress,
							 CNntpReturn & nntpReturn
			  )

/*++

Routine Description:

	Modify the headers of the article.

Arguments:

	grouplist - A list: for each newsgroup its name, and the article number in that group.
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	//
	// clear the return code object
	//

	nntpReturn.fSetClear();

	/* !!!CLIENT LATER
	Must validate encodings see From parsing
	Posting agents MUST ensure that any material  resembling  an  encoded  word (complete  with  all delimiters), in a context where encoded words may appear, really is an encoded word.

	*/

	if (!(
  			   m_fieldMessageID.fSet(*this, pcDNS, nntpReturn)
  			&& m_fieldNewsgroups.fSet(*this, nntpReturn)
  			&& m_fieldDistribution.fSet(*this, nntpReturn)
  			&& m_fieldDate.fSet(*this, nntpReturn)
  			&& m_fieldLines.fSet(*this, nntpReturn)
  			&& m_fieldOrganization.fSet(*this, nntpReturn)
  			&& m_fieldPath.fSet(*this, pcHub, nntpReturn)
			/*&& m_fieldXref.fSet(pcHub, grouplist, *this, m_fieldNewsgroups, nntpReturn)*/
			&& m_fieldNNTPPostingHost.fSet(*this, remoteIpAddress, nntpReturn)
			/* && m_fieldXAuthLoginName.fSet(*this, nntpReturn) */
			&& fDeleteEmptyHeader(nntpReturn)
			&& fSaveHeader(nntpReturn)
		))
		return nntpReturn.fFalse();

	return nntpReturn.fSetOK();
}



BOOL
CFromStoreArticle::fCheckBodyLength(
				 CNntpReturn & nntpReturn
				 )
/*++

Routine Description:

	Checks if the length of the body is within bounds.

Arguments:

	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{

	//
	//!!!CLIENT NEXT need to add a real body length check here
	//

	return nntpReturn.fSetOK();
}



BOOL
CFromStoreNewsgroupsField::fSet(
				   				 CFromStoreArticle & article,
								 CNntpReturn & nntpReturn
								 )
/*++

Routine Description:


	Rewrites the Newsgroups line, fixing some problems such as
	extra whitespace and duplicates.


Arguments:

	article - The article being processed.
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{

	//
	// clear the return code object
	//

	nntpReturn.fSetClear();

	//
	// Check article state
	//

	_ASSERT(fsParsed == m_fieldState);//real

	CPCString pcLine;


	//
	// max size needed is the size of the old line
	//

	const DWORD cchMaxNewsgroups =
			(m_pHeaderString->pcLine).m_cch
			+ 2 // for the newline
			+ 1; // for a terminating null


	//
	// Allocate memory for line within a PCString.
	//

	pcLine.m_pch  = article.pAllocator()->Alloc(cchMaxNewsgroups);
	if (!pcLine.m_pch)
		return nntpReturn.fSet(nrcMemAllocationFailed, __FILE__, __LINE__);

	//
	// Start with "Newsgroups: "
	//

	pcLine << szKwNewsgroups << (char) ' ';

	//
	// Loop through the newsgroups
	//

	_ASSERT(0 < cGet());//real
	char const * szNewsgroup = multiSzGet();
	do
	{

	//
	// Start with newsgroup name, then add comma
	//

		pcLine << szNewsgroup << (char) ',';

		//
		// go to first char after next null
		//

		while ('\0' != szNewsgroup[0])
			szNewsgroup++;
		szNewsgroup++;
	} while ('\0' != szNewsgroup[0]);

	//
	// Remove the last ","
	//

	pcLine.vSkipEnd(1);

	pcLine	<< "\r\n";
	pcLine.vMakeSz(); // terminate the string

	//
	// confirm that we allocated enough memory
	//

	_ASSERT(cchMaxNewsgroups-1 >= pcLine.m_cch);//real

	if (!(
  		article.fRemoveAny(szKwNewsgroups, nntpReturn)
		&& article.fAdd(pcLine.sz(), pcLine.pchMax(), nntpReturn)
		))
	{
		//
		// If anything went wrong, free the memory.
		//

		article.pAllocator()->Free(pcLine.m_pch);
		return nntpReturn.fFalse();
	}

	return nntpReturn.fSetOK();
}


BOOL
CFromStoreDistributionField::fSet(
				   				 CFromStoreArticle & article,
								 CNntpReturn & nntpReturn
								 )
/*++

Routine Description:


	Rewrites the Distribution line, fixing some problems such as
	extra whitespace and duplicates.


Arguments:

	article - The article being processed.
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{

	//
	// clear the return code object
	//

	nntpReturn.fSetClear();

	//
	// If it was not found exists, then just return
	//

	if (fsNotFound == m_fieldState)
		return nntpReturn.fSetOK();


	//
	// Otherwise, fix up what was found.
	//

	//
	// Check article state
	//

	_ASSERT(fsParsed == m_fieldState);//real

	CPCString pcLine;


	//
	// max size needed is the size of the old line
	//

	const DWORD cchMaxDistribution =
			(m_pHeaderString->pcLine).m_cch
			+ 2 // for the newline
			+ 1; // for a terminating null


	//
	// Allocate memory for line within a PCString.
	//

	pcLine.m_pch  = article.pAllocator()->Alloc(cchMaxDistribution);
	if (!pcLine.m_pch)
		return nntpReturn.fSet(nrcMemAllocationFailed, __FILE__, __LINE__);

	//
	// Start with "Distribution: "
	//

	pcLine << szKwDistribution << (char) ' ';

	//
	// Loop through the Distribution
	//

	_ASSERT(0 <= (int)cGet());//real
	char const * szDistributrionValue = multiSzGet();
	do
	{

	//
	// Start with distribution value and add comma
	//

		pcLine << szDistributrionValue << (char) ',';

		//
		// go to first char after next null
		//

		while ('\0' != szDistributrionValue[0])
			szDistributrionValue++;
		szDistributrionValue++;
	} while ('\0' != szDistributrionValue[0]);

	//
	// Remove the last ","
	//

	pcLine.vSkipEnd(1);

	pcLine	<< "\r\n";
	pcLine.vMakeSz(); // terminate the string

	//
	// confirm that we allocated enough memory
	//

	_ASSERT(cchMaxDistribution-1 >= pcLine.m_cch);//real

	if (!(
  		article.fRemoveAny(szKwDistribution, nntpReturn)
		&& article.fAdd(pcLine.sz(), pcLine.pchMax(), nntpReturn)
		))
	{
		//
		// If anything went wrong, free the memory.
		//

		article.pAllocator()->Free(pcLine.m_pch);
		return nntpReturn.fFalse();
	}

	return nntpReturn.fSetOK();
}



BOOL
CFromStoreDateField::fSet(
				   				 CFromStoreArticle & article,
								 CNntpReturn & nntpReturn
								 )
/*++

Routine Description:


	If the date field is missing, adds it.


Arguments:

	article - The article being processed.
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{

	//
	// clear the return code object
	//

	nntpReturn.fSetClear();

	//
	// Check article state
	//
	//
	// If it already exists, then just return
	//

	if (fsParsed == m_fieldState)
		return nntpReturn.fSetOK();


	//
	// Otherwise, add it.
	//

	_ASSERT(fsNotFound == m_fieldState);//real
	CPCString pcLine;

	//
	// max size needed is
	//

	const DWORD cchMaxDate =
			STRLEN(szKwDate)	// for the Date keyword
			+ 1					// space following the keyword
			+ cMaxArpaDate		// bound on the data string
			+ 2 // for the newline
			+ 1; // for a terminating null

	//
	// Allocate memory for line within a PCString.
	//

	pcLine.m_pch  = article.pAllocator()->Alloc(cchMaxDate);
	if (!pcLine.m_pch)
		return nntpReturn.fSet(nrcMemAllocationFailed, __FILE__, __LINE__);

	char szDateBuf[cMaxArpaDate];

	//
	// Start with "Date: ", then add the date and the newline
	//

	pcLine << szKwDate << (char) ' ' << (GetArpaDate(szDateBuf)) << "\r\n";
	pcLine.vMakeSz(); // terminate the string

	//
	// confirm that we allocated enough memory
	//

	_ASSERT(cchMaxDate-1 >= pcLine.m_cch);//real

	if (!article.fAdd(pcLine.sz(), pcLine.pchMax(), nntpReturn))
	{
		//
		// If anything went wrong, free the memory.
		//

		article.pAllocator()->Free(pcLine.m_pch);
		nntpReturn.fFalse();
	}


	return nntpReturn.fSetOK();
}

/*
Organization: Optional � But if not and default is given, create
The Organization header content is a short phrase  identify-ing the poster�s organization:
Organization-content = nonblank-text
This header is typically supplied by the posting agent.  The Organization content SHOULD  mention  geographical  location (e.g.  city  and  country)  when  it is not obvious from the organization?s name.  policy.  Posting agents SHOULD permit the poster t


o override a local default Organization header.
*/

BOOL
CFromStoreOrganizationField::fSet(
   				   				 CFromStoreArticle & article,
								 CNntpReturn & nntpReturn
								 )
/*++

Routine Description:


	Optional, But if not given my user and default is given, create


Arguments:

	country -


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{

//
//!!!CLIENT NEXT - add this code
//

	return nntpReturn.fSetOK();
}

//
//!!!constize
//

/*
Restricted Syntax
The  From header contains the electronic address, and possibly the full name, of the article�s author:
From-content  = address [ space "(" paren-phrase ")" ]
/  [ plain-phrase space ] "<" address ">"
paren-phrase  = 1*( paren-char / space / encoded-word )
paren-char    = <ASCII printable character except ()<>\>
plain-phrase  = plain-word *( space plain-word )
plain-word    = unquoted-word / quoted-word / encoded-word
unquoted-word = 1*unquoted-char
unquoted-char = <ASCII printable character except !()<>@,;:\".[]>
               quoted-word   = quote 1*( quoted-char / space ) quote
               quote         = <" (ASCII 34)>
               quoted-char   = <ASCII printable character except "()<>\>
               address       = local-part "@" domain
               local-part    = unquoted-word *( "." unquoted-word )
               domain        = unquoted-word *( "." unquoted-word )

(Encoded words are described in section 4.5.)  The full name is  distinguished  from  the  electronic  address  either by enclosing the former in parentheses (making  it  resemble  a MAIL  comment, after the address) or by enclosing the latter in angle br


ackets.  The second form is  preferred.   In  the first  form, encoded words inside the full name MUST be composed  entirely  of  <paren-char>s.   In  the  second  form, encoded  words  inside the full name may not contain characters other than letters (o

f either case),  digits,  and  the characters "!", "*", "+", "-", "/", "=", and "_".  The local part is case-sensitive (except that all case counterparts of "postmaster"  are  deemed  equivalent),  the domain is case-insensitive, and all other parts of  t


he  From  content  are comments  which  MUST  be  ignored  by news software (except insofar as reading agents may wish to display  them  to  the reader).   Posters  and  posting  agents MUST restrict them-selves to this subset of the MAIL From syntax; rel


ayers  MAY accept  a  broader subset, but see the discussion in section 9.1.
Avoid "!" and "@" in full names
Posters  and  posting agents SHOULD avoid use of the characters "!" and "@" in full names, as they may trigger unwanted header rewriting by old, simple-minded news software.
"." and "," must be quoted
NOTE: Also, the characters "." and ",", not infrequently found in names (e.g., "John  W.  Campbell, Jr."), are NOT, repeat NOT, allowed in an unquoted word.  A From header like the following  MUST  not be written without the quotation marks:
                    From:	"John W. Campbell, Jr." <editor@analog.com>

*/
/*
 Three permissible forms documented in RFC 1036 should be supported.  Full names within this header line can only contain printable ASCII (0x20 to 0x7E) except "(", ")", "<", ">".  The following characters are inadvisable: ",", ":", "@", "!", "/", "=", ";


".  Test cases include non-printable characters, empty header line (can?t be empty), missing/duplicate "@" address delimiter, multiple address, name lists (not supported), missing address, invalid address, inadvisable characters in name, etc.
 */

BOOL
CFromStoreXAuthLoginNameField::fSet(
				 				 CFromStoreArticle & article,
								 CNntpReturn & nntpReturn
								 )
/*++

Routine Description:


	replace with our value


Arguments:

	article - The article being processed.
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	//
	// clear the return code object
	//

	nntpReturn.fSetClear();

	//
	// Check article state
	//

	_ASSERT(fsInitialized == m_fieldState);//real
	CPCString pcLine;


	//
	// max size needed is
	//

	const DWORD cchMaxXAuthLoginName =
			STRLEN(szKwXAuthLoginName)	// for the XAuthLoginName keyword
			+ 1					// space following the keyword
			//+ cMaxLoginName		// bound on the data string
            + MAX_PATH		// bound on the data string
			+ 2 // for the newline
			+ 1; // for a terminating null

	//
	// Allocate memory for line within a PCString.
	//

	pcLine.m_pch  = article.pAllocator()->Alloc(cchMaxXAuthLoginName);
	if (!pcLine.m_pch)
		return nntpReturn.fSet(nrcMemAllocationFailed, __FILE__, __LINE__);

	//
	// Start with "XAuthLoginName: "
	//

	wsprintf(pcLine.m_pch, "%s ", szKwXAuthLoginName);
	pcLine.m_cch = STRLEN(szKwXAuthLoginName)	+ 1;

	//
	// Add the data value and newline
	//

	pcLine << (article.m_szLoginName) << "\r\n";
	pcLine.vMakeSz(); // Terminate the string

	//
	// confirm that we allocated enough memory
	//

	_ASSERT(cchMaxXAuthLoginName-1 >= pcLine.m_cch);//real

	if (!(
  		article.fRemoveAny(szKwXAuthLoginName, nntpReturn)
		&& article.fAdd(pcLine.sz(), pcLine.pchMax(), nntpReturn)
		))
	{
		//
		// If anything went wrong, free the memory.
		//

		article.m_pAllocator->Free(pcLine.m_pch);

		return nntpReturn.fFalse();
	}


	return nntpReturn.fSetOK();
}






//
//!!!CLIENT NEXT: In Newsgroups: parse need to check for illegal groups like "control" and "poster"
//

/*
Followup-To: Optional
Email address is not allowed. Like newsgroups line or "poster" (capitalization?)
NOTE: The way to request that followups be  mailed to  a specific address other than that in the From line is  to  supply  "Followup-To: poster"  and  a Reply-To header.  Putting a mailing address in the Followup-To  line  is  incorrect;  posting  agent

should reject or rewrite such headers.
*/
/*
Reply-To: Optional
Must be a valid email address
*/

/*

//
///!!!CLIENT NEXT should Hubname be lower case?
///!!!CLIENT NEXT should Hubname be the domain name (for message id?)
//

/*
Message-ID: Replace any with own
How to create a messageid
5.3. Message-ID
The  Message-ID  header contains the article�s message ID, a unique identifier  distinguishing  the  article  from  every other article:
Message-ID-content  = message-id
message-id          = "<" local-part "@" domain ">"
As  with  From addresses, a message ID�s local part is case-sensitive and its domain is case-insensitive.  The  "<"  and ">"  are  parts  of the message ID, not peculiarities of the Message-ID header.
NOTE: News message IDs are a restricted subset  of MAIL message IDs.  In particular, no existing news software copes properly with MAIL quoting  conventions  within  the local part, so they are forbid-den.  This is unfortunate, particularly for  X.400 gat

es on gatewaying in section 10.
The domain in the message ID SHOULD  be  the  full  Internet domain name of the posting agent?s host.  Use of the ".uucp" pseudo-domain (for hosts registered in the UUCP maps) or the ".bitnet"  pseudo-domain  (for Bitnet hosts) is permissible, but SHOUL

be avoided.
Posters and posting agents MUST generate the local part of a
message ID using an algorithm which obeys the specified syn-
tax (words separated by ".",  with  certain  characters  not
permitted)  (see  section  5.2  for  details),  and will not repeat itself (ever).  The  algorithm  SHOULD  not  generate message  IDs which differ only in case of letters.  Note the specification in section 6.5 of a recommended convention for indicatin

 subject  changes.  Otherwise the algorithm is up to the implementor.
NOTE: The crucial use of message IDs is to distinguish  circulating  articles  from  each other and from articles circulated recently.  They are  also potentially  useful  as  permanent  indexing keys, hence the requirement for permanent  uniqueness...  b


ut   indexers  cannot  absolutely  rely  on  this because the earlier RFCs  urged  it  but  did  not demand  it.  All major implementations have always generated  permanently-unique   message   IDs   by design,  but  in  some  cases this is sensitive to p


roper administration,  and  duplicates  may  have occurred by accident.
NOTE:  The most popular method of generating local parts is to use the date and time, plus  some  way of distinguishing between simultaneous postings on the same host (e.g. a process number), and  encode them  in a suitably-restricted alphabet.  An olde

but now  less-popular  alternative  is  to  use  a sequence  number,  incremented  each time the host generates a new message ID; this is workable,  but requires  careful  design  to  cope  properly with simultaneous  posting  attempts,  and  is  not  a

robust  in  the presence of crashes and other malfunctions.
NOTE: Some buggy news software  considers  message
IDs  completely case-insensitive, hence the advice
to  avoid  relying  on  case  distinctions.    The
restrictions  placed  on  the  "alphabet" of local
parts and domains in section 5.2 have  the  useful side effect of making it unnecessary to parse message IDs in complex ways to break them into  case-sensitive and case-insensitive portions.

*/
BOOL
CFromStoreMessageIDField::fSet(
				 				 CFromStoreArticle  & article,
								 CPCString & pcHub,
								 CNntpReturn & nntpReturn
								 )
/*++

Routine Description:


	Replaces any messageid field, with a newly created one.


  Form: <1993Jun27.0645330123.1778.343@localmachinename>

Arguments:

	article - The article being processed.
	pcHub - The name of the hub the current machine is part of.
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{

	//
	// clear the return code object
	//

	nntpReturn.fSetClear();

	//
	// Check article state
	//

	if( m_fieldState == fsParsed ) {
		return	nntpReturn.fSetOK() ;
	}

	CPCString pcLine;


	//
	// max size needed is
	//

#if 0
	const DWORD cchMaxMessageID =
			STRLEN(szKwMessageID)	// for the MessageID keyword
			+ 1					// space following the keyword
			+ 5					// <..@>
			+ cMaxMessageIDDate // The message id date
			+ 20				// Two dwords
			+ pcHub.m_cch		// the hub name
			+ 2 // for the newline
			+ 1; // for a terminating null
#endif

	const DWORD cchMaxMessageID =
			STRLEN(szKwMessageID)	// for the MessageID keyword
			+ 1					// space following the keyword
			+ 4					// <..@>
			+ cMaxMessageIDDate // The message id date
			+ 10				// One dword
			+ pcHub.m_cch		// the hub name
			+ 2 // for the newline
			+ 1; // for a terminating null

	//
	// The message-id created (without the newlines) must be less than the max
	//

	_ASSERT(cchMaxMessageID - 2 < MAX_MSGID_LEN);

	//
	// Allocate memory for line within a PCString.
	//

	pcLine.m_pch  = article.pAllocator()->Alloc(cchMaxMessageID);
	if (!pcLine.m_pch)
		return nntpReturn.fSet(nrcMemAllocationFailed, __FILE__, __LINE__);

	//
	// Start with "MessageID: <"
	//

	wsprintf(pcLine.m_pch, "%s <", szKwMessageID);
	pcLine.m_cch = STRLEN(szKwMessageID)	+ 2;

	char szMessageIDBuf[cMaxMessageIDDate];

	CArticleRef artRef = article.articleRef();

	pcLine
			//
			// Add the local part
			//
			<< (GetMessageIDDate( artRef.m_groupId, artRef.m_articleId, szMessageIDBuf))
			<< (char) '.'
			//<< (GetCurrentProcessId())
			//<< '.'
			<< (const DWORD) (GetCurrentThreadId())
			//
			// Add '@' and domain and '>' and newline
			//
			<< (char) '@'
			<< pcHub ///!!!CLIENT NEXT I need the local machine rather than the hub
			<< ">\r\n";
	pcLine.vMakeSz(); // terminate the string

	//
	// confirm that we allocated enough memory
	//

	//_ASSERT(cchMaxMessageID-1-STRLEN(szKwMessageID)-1 >= pcLine.m_cch);//real
	_ASSERT(cchMaxMessageID >= pcLine.m_cch+1);//real

	if (!(
  		article.fRemoveAny(szKwMessageID, nntpReturn)//!!!CLIENT NEXT -- this really only needs to be called of state is parsed
		&& article.fAdd(pcLine.sz(), pcLine.pchMax(), nntpReturn)
		))
	{
		article.pAllocator()->Free(pcLine.m_pch);
		return nntpReturn.fFalse();
	}


	//
	//Also save the value (without newlines but with room for a terminating
	//a '\0') in m_szMessageID
	//

	DWORD cchMessageID = pcLine.m_cch - 2 - STRLEN(szKwMessageID) - 1;
	strncpy(m_szMessageID, pcLine.m_pch + STRLEN(szKwMessageID)	+ 1, cchMessageID);
	m_szMessageID[cchMessageID] ='\0';
	_ASSERT('<' == m_szMessageID[0] && '>' == m_szMessageID[cchMessageID-1]);


	return nntpReturn.fSetOK();
}


BOOL
CFromStorePathField::fSet(
						   CFromStoreArticle & article,
						   CPCString & pcHub,
						   CNntpReturn & nntpReturn
						   )
/*++

Routine Description:


	Replaces any existing Path header with a newly created one that
	contains only the name of the hub.


Arguments:

	article - The article being processed.
	pcHub - The name of the hub the current machine is part of.
	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	//
	// clear the return code object
	//

	nntpReturn.fSetClear();

	//
	// Check article state
	//

	_ASSERT(fsInitialized != m_fieldState);//real

	CPCString pcLine;


	//
	// max size needed is
	//

	DWORD	cbOldPath = 0 ;
	if( m_pHeaderString && m_pHeaderString->pcValue.m_pch ) {
		cbOldPath = m_pHeaderString->pcValue.m_cch + 1 ;	//include 1 for extra '!'
	}

	const DWORD cchMaxPath =
			STRLEN(szKwPath)	// for the Path keyword
			+ 1					// space following the keyword
			+ pcHub.m_cch		// the hub name
			+ 2 // for the newline
			+ cbOldPath // in case there already is a path header !
			+ 1; // for a terminating null

	//
	// Allocate memory for line within a PCString.
	//

	pcLine.m_pch  = article.pAllocator()->Alloc(cchMaxPath);
	if (!pcLine.m_pch)
		return nntpReturn.fSet(nrcMemAllocationFailed, __FILE__, __LINE__);

	//
	// Start with "Path: <hubname>"
	//

	pcLine << szKwPath << (char) ' ' << pcHub ;		//	<< "\r\n";

	if( m_pHeaderString && m_pHeaderString->pcValue.m_pch ) {
		_ASSERT( m_pHeaderString->pcValue.m_cch != 0 ) ;
		pcLine << "!" << (m_pHeaderString->pcValue) ;
	}

	pcLine << "\r\n" ;

	pcLine.vMakeSz();

	//
	// confirm that we allocated enough memory
	//

	_ASSERT(cchMaxPath-1 == pcLine.m_cch);//real

	if (!(
  		article.fRemoveAny(szKwPath, nntpReturn)//!!!CLIENT NEXT -- this really only needs to be called of state is parsed
		&& article.fAdd(pcLine.sz(), pcLine.pchMax(), nntpReturn)
		))
	{
		//
		// If anything went wrong, free the memory.
		//

		article.pAllocator()->Free(pcLine.m_pch);

		return nntpReturn.fFalse();
	}

	return nntpReturn.fSetOK();
}


BOOL
CFromStoreNNTPPostingHostField::fSet(
									  CFromStoreArticle & article,
									  DWORD remoteIpAddress,
									  CNntpReturn & nntpReturn
									  )
/*++

Routine Description:

	Behavior is governed by global set by a reg key.
	In any case, this removes any old NNTPPostingHost headers.

	If global is set, a new NNTP-Posting-Host header is added
	else no new header is added. default behavior is to NOT add
	this header.

Arguments:

	article - The article being processed.
	remoteIpAddress - client IP address
	nntpReturn - The return value for this function call



Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	//
	// clear the return code object
	//

	nntpReturn.fSetClear();
	return nntpReturn.fSetOK();
}


BOOL
CFromStoreDistributionField::fParse(
									 CArticleCore & article,
									 CNntpReturn & nntpReturn
									 )
/*++

Routine Description:

  Parses the Distribution field. Here is the grammer from Son of 1036:

               Newsgroups-content  = newsgroup-name *( ng-delim newsgroup-name )
               newsgroup-name      = plain-component *( "." component )
               component           = plain-component / encoded-word
               plain-component     = component-start *13component-rest
               component-start     = lowercase / digit
               lowercase           = <letter a-z>
               component-rest      = component-start / "+" / "-" / "_"
               ng-delim            = ","


Arguments:

	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	//
	// clear the return code object
	//

	nntpReturn.fSetClear();

	//
	// Check article state
	//

	_ASSERT(fsFound == m_fieldState);//real

	_ASSERT(m_pHeaderString); //real


	//
	// Record the allocator
	//

	m_pAllocator = article.pAllocator();

	if (!fParseSplit(TRUE, m_multiSzDistribution, m_cDistribution, " \t\r\n,",
			article, nntpReturn))
		return FALSE;

	//
	//Check for duplicates
	//

	DWORD cOldCount = m_cDistribution;
	if (!fMultiSzRemoveDupI(m_multiSzDistribution, m_cDistribution, m_pAllocator))
		nntpReturn.fSet(nrcMemAllocationFailed, __FILE__, __LINE__);

	if( m_cDistribution == 0 ) {
		return	nntpReturn.fSetOK() ;
	}

	//
	// check for illegal characters and substrings in Distribution name
	//

	char const * szDistribution = m_multiSzDistribution;
	do
	{
		if ('\0' == szDistribution[0]
			|| !fTestAComponent(szDistribution)
			)
		return nntpReturn.fSet(nrcArticleFieldIllegalComponent, szDistribution, szKeyword());

		//
		// go to first char after next null
		//

		while ('\0' != szDistribution[0])
			szDistribution++;
		szDistribution++;
	} while ('\0' != szDistribution[0]);

	return nntpReturn.fSetOK();
}

BOOL
CFromStoreLinesField::fParse(
						 CArticleCore & article,
						 CNntpReturn & nntpReturn
						 )
/*++

Routine Description:

	Parses the Lines field.

Arguments:

	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	//
	// clear the return code object
	//

	nntpReturn.fSetClear();

	//
	// Check article state
	//

	_ASSERT(fsFound == m_fieldState);//real

	_ASSERT(m_pHeaderString); //real

	if (!fParseSimple(TRUE, m_pc, nntpReturn))
		return nntpReturn.fFalse();

	char * pchMax = m_pc.pchMax();
	for (char * pch = m_pc.m_pch; pch < pchMax; pch++)
	{
		if (!(UCHAR)isdigit(*pch))
			return nntpReturn.fSet(nrcArticleFieldBadChar, (BYTE) *pch, szKeyword());
	}

	return nntpReturn.fSetOK();
}

BOOL
CFromStoreReferencesField::fParse(
								   CArticleCore & article,
								   CNntpReturn & nntpReturn
								   )
/*++

Routine Description:

	Parses the References field.

Arguments:

	nntpReturn - The return value for this function call


Return Value:

	TRUE, if successful. FALSE, otherwise.

--*/
{
	//
	// clear the return code object
	//

	nntpReturn.fSetClear();

	//
	// Check article state
	//

	_ASSERT(fsFound == m_fieldState);//real

	_ASSERT(m_pHeaderString); //real


	//
	// Record the allocator
	//

	m_pAllocator = article.pAllocator();

	if (!fParseSplit(FALSE, m_multiSzReferences, m_cReferences, szWSNLChars,
			article, nntpReturn))
		return nntpReturn.fFalse();

	//
	// check for illegal characters and substrings in References name
	//

	char const * szReferences = m_multiSzReferences;
	do
	{
		if (!fTestAMessageID(szReferences, nntpReturn))
			return nntpReturn.fFalse();

		//
		// go to first char after next null
		//

		while ('\0' != szReferences[0])
			szReferences++;
		szReferences++;
	} while ('\0' != szReferences[0]);

	return nntpReturn.fSetOK();
}


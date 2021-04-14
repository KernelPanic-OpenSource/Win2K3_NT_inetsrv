/*-------------------------------------------------------------------------
  timeconv.h
  	Function prototypes for time conversion functions.


  Copyright (C) 1994  Microsoft Corporation.

  Author
  	Lindsay Harris	- lindsayh

  History
	14:08 on Wed 20 Apr 1994    -by-    Lindsay Harris   [lindsayh]
  	First version, now that there are 2 time functions!

  --------------------------------------------------------------------------*/

#if  !defined( _TIMECONV_H )

#define	_TIMECONV_H


/*
 *  Convert an ARPA/Internet time/date string to time_t format.  Used when
 *  generating the index data for Usenet news feed.
 */

DWORD   dwConvertAsciiTime( char * );


const DWORD cMaxArpaDate = 33;
/*
 *  Generate an ARPA/Internet time format string for current time.
 *  You must pass in a buffer of type char [cMaxArpaDate]
 */

char  *
GetArpaDate( char rgBuf[ cMaxArpaDate ] );

const DWORD cMaxMessageIDDate = 12;	// (64 / 6) + 2
/*
 *  Generate a time format string for current time that
 *  can be used to generate part of a message id.
 *  You must pass in a buffer of type char [cMaxMessageIDDate]
 */

char  *
GetMessageIDDate( DWORD GroupId, DWORD ArticleId, char rgBuf[ cMaxMessageIDDate ] );

/*
 *  Convert a structure of type SYSTEMTIME to a time_t value.
 *  Returns 0 if the date is before 1970.
 */

time_t SystemTimeToTime_T(SYSTEMTIME & st);

/*
 * Convert between changes in FILETIMEs and changes in time_t's (and visa versa)
 */

time_t dTime_tFromDFiletime(const FILETIME & li);

/*
 *   Add, Subtract, and Compare FILETIMES
 */

FILETIME filetimeSubtract(const FILETIME & ft1, const FILETIME & ft2);
BOOL filetimeGreaterThan(const FILETIME & ft1, const FILETIME & ft2);

#endif		// _TIMECONV_H

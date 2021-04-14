/*
 * Copyright (c) 1983, 1995 Eric P. Allman
 * Copyright (c) 1988, 1993
 *    The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by the University of
 *    California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "smtpinc.h"
#include <listmacr.h>
#include "headers.hxx"

int strcasecmp(char *s1, char *s2);
int strncasecmp(char *s1, char *s2, int n);

struct hdrinfo
{
    char    *hi_field;    /* the name of the field */
    unsigned int    hi_flags;    /* status bits, see below */
};

struct hdrinfo    HdrInfo[] =
{
        /* originator fields, most to least significant  */
    "resent-sender",      H_FROM|H_RESENT,
    "resent-from",        H_FROM|H_RESENT,
    "resent-reply-to",    H_FROM|H_RESENT,
    "sender",             H_FROM,
    "from",               H_FROM,
    "reply-to",           H_FROM,
    "full-name",          0,
    "return-receipt-to",  H_FROM|H_RECEIPTTO,
    "errors-to",          H_FROM|H_ERRORSTO,

        /* destination fields */
    "to",                 H_RCPT,
    "resent-to",          H_RCPT|H_RESENT,
    "cc",                 H_RCPT,
    "resent-cc",          H_RCPT|H_RESENT,
    "bcc",                H_RCPT,
    "resent-bcc",         H_RCPT|H_RESENT,
    "apparently-to",      H_RCPT,

        /* message identification and control */
    "message-id",         H_MID,
    "resent-message-id",  H_RESENT,

        /* date fields */
    "date",               H_DATE,
    "resent-date",        H_RESENT,

        /* trace fields */
    "received",           0,
    "x400-received",      0,
    "via",                0,
    "mail-from",          H_TRACE,

        /* miscellaneous fields */
    "comments",           0,
    "return-path",        H_RETURNPATH,
    "subject",            H_SUBJECT,

        /* X-Headers */
    "x-sender",           H_X_SENDER,
    "x-receiver",         H_X_RECEIVER,


    // 10/15/98 - MikeSwa Added supersedes headers
    "x-msgguid",          H_X_MSGGUID,
    "x-supersedesmsgguid",H_X_SUPERSEDES_MSGGUID,

    // 12/18/98 -- pgopi. Added X-originalArrivalTime header
    "x-originalarrivaltime" , H_X_ORIGINAL_ARRIVAL_TIME,
    
    NULL,                 0,
};


#define xalloc(size) HeapAlloc(GetProcessHeap(), 0, size)
/*
**  CHOMPHEADER -- process and save a header line.
**
**    Called by collect and by readcf to deal with header lines.
**
**    Parameters:
**        line -- header as a text line.
**        def -- if set, this is a default value.
**        hdrp -- a pointer to the place to save the header.
**        e -- the envelope including this header.
**
**    Returns:
**        flags for this header.
**
**    Side Effects:
**        The header is saved on the header list.
**        Contents of 'line' are destroyed.
*/
BOOL ChompHeader(char * line, DWORD& HeaderFlags, char ** ppszValueBuf)
{
    register char *p = line;
    HEADERVAL *h = NULL;
    char *fname = NULL;
    char *fvalue = NULL;
    char *oldfvalue = NULL;
    struct hdrinfo *hi = NULL;
    DWORD fNameSize;


    /* find canonical name */
    fname = p;

    while (isascii((UCHAR)*p) && isgraph((UCHAR)*p) && *p != ':')
             p++;

    fvalue = p;

    while (isascii((UCHAR)*p) && isspace((UCHAR)*p))
           p++;

    //See if we had anything atall
    if(p == line)
    {
        //We had a seperator CRLF
        HeaderFlags |= H_EOH;
        return (FALSE);
    }

    if (*p++ != ':' || fname == fvalue)
    {
        //syserr("553 header syntax error, line \"%s\"", line);
        return (FALSE);
    }

    fvalue = p;

    if(ppszValueBuf)
        *ppszValueBuf = fvalue;

    fNameSize = (DWORD)((p - line) - 1);

    /* see if it is a known type */
    for (hi = HdrInfo; hi->hi_field != NULL; hi++)
     {
         if (strncasecmp(hi->hi_field, fname, fNameSize) == 0)
          {
              HeaderFlags |= hi->hi_flags; //set our flag
              break;
          }
      }

    return (TRUE);
}


#if 0
/*
**  ADDHEADER -- add a header entry to the end of the queue.
**
**    This bypasses the special checking of chompheader.
**
**    Parameters:
**        field -- the name of the header field.
**        value -- the value of the field.
**        hp -- an indirect pointer to the header structure list.
**
**    Returns:
**        none.
**
**    Side Effects:
**        adds the field on the list of headers for this envelope.
*/

void
addheader(field, value, hdrlist)
    char *field;
    char *value;
    HDR **hdrlist;
{
    register HDR *h;
    register struct hdrinfo *hi;
    HDR **hp;

    /* find info struct */
    for (hi = HdrInfo; hi->hi_field != NULL; hi++)
    {
        if (strcasecmp(field, hi->hi_field) == 0)
            break;
    }

    /* find current place in list -- keep back pointer? */
    for (hp = hdrlist; (h = *hp) != NULL; hp = &h->h_link)
    {
        if (strcasecmp(field, h->h_field) == 0)
            break;
    }

    /* allocate space for new header */
    h = (HDR *) xalloc(sizeof *h);
    h->h_field = field;
    h->h_value = newstr(value);
    h->h_link = *hp;
    h->h_flags = hi->hi_flags | H_DEFAULT;
    clrbitmap(h->h_mflags);
    *hp = h;
}
/*
**  HVALUE -- return value of a header.
**
**    Only "real" fields (i.e., ones that have not been supplied
**    as a default) are used.
**
**    Parameters:
**        field -- the field name.
**        header -- the header list.
**
**    Returns:
**        pointer to the value part.
**        NULL if not found.
**
**    Side Effects:
**        none.
*/

char * hvalue(char * field, HDR * header)
{
    register HDR *h;

    for (h = header; h != NULL; h = h->h_link)
    {
        if (!bitset(H_DEFAULT, h->h_flags) &&
            strcasecmp(h->h_field, field) == 0)
            return (h->h_value);
    }
    return (NULL);
}
#endif

/*
**  ISHEADER -- predicate telling if argument is a header.
**
**    A line is a header if it has a single word followed by
**    optional white space followed by a colon.
**
**    Header fields beginning with two dashes, although technically
**    permitted by RFC822, are automatically rejected in order
**    to make MIME work out.  Without this we could have a technically
**    legal header such as ``--"foo:bar"'' that would also be a legal
**    MIME separator.
**
**    Parameters:
**        h -- string to check for possible headerness.
**
**    Returns:
**        TRUE if h is a header.
**        FALSE otherwise.
**
**    Side Effects:
**        none.
*/

BOOL IsHeader(char *h)
{
    register char *s = h;

    if (s[0] == '-' && s[1] == '-')
        return FALSE;

    while (*s > ' ' && *s != ':' && *s != '\0')
        s++;

    if (h == s)
        return FALSE;

    /* following technically violates RFC822 */
    while (isascii((UCHAR)*s) && isspace((UCHAR)*s))
        s++;

    return (*s == ':');
}

#if 0
/*
**  EATHEADER -- run through the stored header and extract info.
**
**    Parameters:
**        e -- the envelope to process.
**        full -- if set, do full processing (e.g., compute
**            message priority).  This should not be set
**            when reading a queue file because some info
**            needed to compute the priority is wrong.
**
**    Returns:
**        none.
**
**    Side Effects:
**        Sets a bunch of global variables from information
**            in the collected header.
**        Aborts the message if the hop count is exceeded.
*/

void
eatheader(e, full)
    register ENVELOPE *e;
    bool full;
{
    register HDR *h;
    register char *p;
    int hopcnt = 0;
    char *msgid;
    char buf[MAXLINE];

    /*
    **  Set up macros for possible expansion in headers.
    */

    define('f', e->e_sender, e);
    define('g', e->e_sender, e);
    if (e->e_origrcpt != NULL && *e->e_origrcpt != '\0')
        define('u', e->e_origrcpt, e);
    else
        define('u', NULL, e);

    /* full name of from person */
    p = hvalue("full-name", e->e_header);
    if (p != NULL)
        define('x', p, e);

    if (tTd(32, 1))
        printf("----- collected header -----\n");
    msgid = NULL;
    for (h = e->e_header; h != NULL; h = h->h_link)
    {
        if (tTd(32, 1))
            printf("%s: ", h->h_field);
        if (h->h_value == NULL)
        {
            if (tTd(32, 1))
                printf("<NULL>\n");
            continue;
        }

        /* do early binding */
        if (bitset(H_DEFAULT, h->h_flags))
        {
            if (tTd(32, 1))
            {
                printf("(");
                xputs(h->h_value);
                printf(") ");
            }
            expand(h->h_value, buf, sizeof buf, e);
            if (buf[0] != '\0')
            {
                if (bitset(H_FROM, h->h_flags))
                {
                    extern char *crackaddr();

                    expand(crackaddr(buf), buf, sizeof buf, e);
                }
                h->h_value = newstr(buf);
                h->h_flags &= ~H_DEFAULT;
            }
        }

        if (tTd(32, 1))
        {
            xputs(h->h_value);
            printf("\n");
        }

        /* count the number of times it has been processed */
        if (bitset(H_TRACE, h->h_flags))
            hopcnt++;

        /* send to this person if we so desire */
        if (GrabTo && bitset(H_RCPT, h->h_flags) &&
            !bitset(H_DEFAULT, h->h_flags) &&
            (!bitset(EF_RESENT, e->e_flags) || bitset(H_RESENT, h->h_flags)))
        {
            int saveflags = e->e_flags;

            (void) sendtolist(h->h_value, NULLADDR,
                      &e->e_sendqueue, 0, e);

            /* delete fatal errors generated by this address */
            if (!GrabTo && !bitset(EF_FATALERRS, saveflags))
                e->e_flags &= ~EF_FATALERRS;
        }

        /* save the message-id for logging */
        p = "resent-message-id";
        if (!bitset(EF_RESENT, e->e_flags))
            p += 7;
        if (strcasecmp(h->h_field, p) == 0)
        {
            msgid = h->h_value;
            while (isascii((UCHAR)*msgid) && isspace((UCHAR)*msgid))
                msgid++;
        }

        /* see if this is a return-receipt header */
        if (bitset(H_RECEIPTTO, h->h_flags))
            e->e_receiptto = h->h_value;
    }
    if (tTd(32, 1))
        printf("----------------------------\n");

    /* if we are just verifying (that is, sendmail -t -bv), drop out now */
    if (OpMode == MD_VERIFY)
        return;

    /* store hop count */
    if (hopcnt > e->e_hopcount)
        e->e_hopcount = hopcnt;

    /* message priority */
    p = hvalue("precedence", e->e_header);
    if (p != NULL)
        e->e_class = priencode(p);
    if (e->e_class < 0)
        e->e_timeoutclass = TOC_NONURGENT;
    else if (e->e_class > 0)
        e->e_timeoutclass = TOC_URGENT;
    if (full)
    {
        e->e_msgpriority = e->e_msgsize
                 - e->e_class * WkClassFact
                 + e->e_nrcpts * WkRecipFact;
    }

    /* message timeout priority */
    p = hvalue("priority", e->e_header);
    if (p != NULL)
    {
        /* (this should be in the configuration file) */
        if (strcasecmp(p, "urgent"))
            e->e_timeoutclass = TOC_URGENT;
        else if (strcasecmp(p, "normal"))
            e->e_timeoutclass = TOC_NORMAL;
        else if (strcasecmp(p, "non-urgent"))
            e->e_timeoutclass = TOC_NONURGENT;
    }

    /* date message originated */
    p = hvalue("posted-date", e->e_header);
    if (p == NULL)
        p = hvalue("date", e->e_header);
    if (p != NULL)
        define('a', p, e);

    /* check to see if this is a MIME message */
    if ((e->e_bodytype != NULL &&
         strcasecmp(e->e_bodytype, "8BITMIME") == 0) ||
        hvalue("MIME-Version", e->e_header) != NULL)
    {
        e->e_flags |= EF_IS_MIME;
        if (HasEightBits)
            e->e_bodytype = "8BITMIME";
    }
    else if ((p = hvalue("Content-Type", e->e_header)) != NULL)
    {
        /* this may be an RFC 1049 message */
        p = strpbrk(p, ";/");
        if (p == NULL || *p == ';')
        {
            /* yep, it is */
            e->e_flags |= EF_DONT_MIME;
        }
    }

    /*
    **  From person in antiquated ARPANET mode
    **    required by UK Grey Book e-mail gateways (sigh)
    */

    if (OpMode == MD_ARPAFTP)
    {
        register struct hdrinfo *hi;

        for (hi = HdrInfo; hi->hi_field != NULL; hi++)
        {
            if (bitset(H_FROM, hi->hi_flags) &&
                (!bitset(H_RESENT, hi->hi_flags) ||
                 bitset(EF_RESENT, e->e_flags)) &&
                (p = hvalue(hi->hi_field, e->e_header)) != NULL)
                break;
        }
        if (hi->hi_field != NULL)
        {
            if (tTd(32, 2))
                printf("eatheader: setsender(*%s == %s)\n",
                    hi->hi_field, p);
            setsender(p, e, NULL, TRUE);
        }
    }

    /*
    **  Log collection information.
    */

# ifdef LOG
    if (bitset(EF_LOGSENDER, e->e_flags) && LogLevel > 4)
        logsender(e, msgid);
# endif /* LOG */
    e->e_flags &= ~EF_LOGSENDER;
}
/*
**  LOGSENDER -- log sender information
**
**    Parameters:
**        e -- the envelope to log
**        msgid -- the message id
**
**    Returns:
**        none
*/

void
logsender(e, msgid)
    register ENVELOPE *e;
    char *msgid;
{
# ifdef LOG
    char *name;
    register char *sbp;
    register char *p;
    int l;
    char hbuf[MAXNAME + 1];
    char sbuf[MAXLINE + 1];
    char mbuf[MAXNAME + 1];

    /* don't allow newlines in the message-id */
    if (msgid != NULL)
    {
        l = strlen(msgid);
        if (l > sizeof mbuf - 1)
            l = sizeof mbuf - 1;
        bcopy(msgid, mbuf, l);
        mbuf[l] = '\0';
        p = mbuf;
        while ((p = strchr(p, '\n')) != NULL)
            *p++ = ' ';
    }

    if (bitset(EF_RESPONSE, e->e_flags))
        name = "[RESPONSE]";
    else if ((name = macvalue('_', e)) != NULL)
        ;
    else if (RealHostName == NULL)
        name = "localhost";
    else if (RealHostName[0] == '[')
        name = RealHostName;
    else
    {
        name = hbuf;
        (void) sprintf(hbuf, "%.80s", RealHostName);
        if (RealHostAddr.sa.sa_family != 0)
        {
            p = &hbuf[strlen(hbuf)];
            (void) sprintf(p, " (%s)",
                anynet_ntoa(&RealHostAddr));
        }
    }

    /* some versions of syslog only take 5 printf args */
#  if (SYSLOG_BUFSIZE) >= 256
    sbp = sbuf;
    sprintf(sbp, "from=%.200s, size=%ld, class=%d, pri=%ld, nrcpts=%d",
        e->e_from.q_paddr == NULL ? "<NONE>" : e->e_from.q_paddr,
        e->e_msgsize, e->e_class, e->e_msgpriority, e->e_nrcpts);
    sbp += strlen(sbp);
    if (msgid != NULL)
    {
        sprintf(sbp, ", msgid=%.100s", mbuf);
        sbp += strlen(sbp);
    }
    if (e->e_bodytype != NULL)
    {
        (void) sprintf(sbp, ", bodytype=%.20s", e->e_bodytype);
        sbp += strlen(sbp);
    }
    p = macvalue('r', e);
    if (p != NULL)
        (void) sprintf(sbp, ", proto=%.20s", p);
    syslog(LOG_INFO, "%s: %s, relay=%s",
        e->e_id, sbuf, name);

#  else            /* short syslog buffer */

    syslog(LOG_INFO, "%s: from=%s",
        e->e_id, e->e_from.q_paddr == NULL ? "<NONE>" :
                shortenstring(e->e_from.q_paddr, 83));
    syslog(LOG_INFO, "%s: size=%ld, class=%ld, pri=%ld, nrcpts=%d",
        e->e_id, e->e_msgsize, e->e_class,
        e->e_msgpriority, e->e_nrcpts);
    if (msgid != NULL)
        syslog(LOG_INFO, "%s: msgid=%s", e->e_id, mbuf);
    sbp = sbuf;
    sprintf(sbp, "%s:", e->e_id);
    sbp += strlen(sbp);
    if (e->e_bodytype != NULL)
    {
        sprintf(sbp, " bodytype=%s,", e->e_bodytype);
        sbp += strlen(sbp);
    }
    p = macvalue('r', e);
    if (p != NULL)
    {
        sprintf(sbp, " proto=%s,", p);
        sbp += strlen(sbp);
    }
    syslog(LOG_INFO, "%s relay=%s", sbuf, name);
#  endif
# endif
}
/*
**  PRIENCODE -- encode external priority names into internal values.
**
**    Parameters:
**        p -- priority in ascii.
**
**    Returns:
**        priority as a numeric level.
**
**    Side Effects:
**        none.
*/

int
priencode(p)
    char *p;
{
    register int i;

    for (i = 0; i < NumPriorities; i++)
    {
        if (!strcasecmp(p, Priorities[i].pri_name))
            return (Priorities[i].pri_val);
    }

    /* unknown priority */
    return (0);
}
/*
**  CRACKADDR -- parse an address and turn it into a macro
**
**    This doesn't actually parse the address -- it just extracts
**    it and replaces it with "$g".  The parse is totally ad hoc
**    and isn't even guaranteed to leave something syntactically
**    identical to what it started with.  However, it does leave
**    something semantically identical.
**
**    This algorithm has been cleaned up to handle a wider range
**    of cases -- notably quoted and backslash escaped strings.
**    This modification makes it substantially better at preserving
**    the original syntax.
**
**    Parameters:
**        addr -- the address to be cracked.
**
**    Returns:
**        a pointer to the new version.
**
**    Side Effects:
**        none.
**
**    Warning:
**        The return value is saved in local storage and should
**        be copied if it is to be reused.
*/

char *
crackaddr(addr)
    register char *addr;
{
    register char *p;
    register char c;
    int cmtlev;
    int realcmtlev;
    int anglelev, realanglelev;
    int copylev;
    bool qmode;
    bool realqmode;
    bool skipping;
    bool putgmac = FALSE;
    bool quoteit = FALSE;
    bool gotangle = FALSE;
    bool gotcolon = FALSE;
    register char *bp;
    char *buflim;
    char *bufhead;
    char *addrhead;
    static char buf[MAXNAME + 1];

    if (tTd(33, 1))
        printf("crackaddr(%s)\n", addr);

    /* strip leading spaces */
    while (*addr != '\0' && isascii((UCHAR)*addr) && isspace((UCHAR)*addr))
        addr++;

    /*
    **  Start by assuming we have no angle brackets.  This will be
    **  adjusted later if we find them.
    */

    bp = bufhead = buf;
    buflim = &buf[sizeof buf - 5];
    p = addrhead = addr;
    copylev = anglelev = realanglelev = cmtlev = realcmtlev = 0;
    qmode = realqmode = FALSE;

    while ((c = *p++) != '\0')
    {
        /*
        **  If the buffer is overful, go into a special "skipping"
        **  mode that tries to keep legal syntax but doesn't actually
        **  output things.
        */

        skipping = bp >= buflim;

        if (copylev > 0 && !skipping)
            *bp++ = c;

        /* check for backslash escapes */
        if (c == '\\')
        {
            /* arrange to quote the address */
            if (cmtlev <= 0 && !qmode)
                quoteit = TRUE;

            if ((c = *p++) == '\0')
            {
                /* too far */
                p--;
                goto putg;
            }
            if (copylev > 0 && !skipping)
                *bp++ = c;
            goto putg;
        }

        /* check for quoted strings */
        if (c == '"' && cmtlev <= 0)
        {
            qmode = !qmode;
            if (copylev > 0 && !skipping)
                realqmode = !realqmode;
            continue;
        }
        if (qmode)
            goto putg;

        /* check for comments */
        if (c == '(')
        {
            cmtlev++;

            /* allow space for closing paren */
            if (!skipping)
            {
                buflim--;
                realcmtlev++;
                if (copylev++ <= 0)
                {
                    *bp++ = ' ';
                    *bp++ = c;
                }
            }
        }
        if (cmtlev > 0)
        {
            if (c == ')')
            {
                cmtlev--;
                copylev--;
                if (!skipping)
                {
                    realcmtlev--;
                    buflim++;
                }
            }
            continue;
        }
        else if (c == ')')
        {
            /* syntax error: unmatched ) */
            if (copylev > 0 && !skipping)
                bp--;
        }

        /* check for group: list; syntax */
        if (c == ':' && anglelev <= 0 && !gotcolon && !ColonOkInAddr)
        {
            register char *q;

            if (*p == ':')
            {
                /* special case -- :: syntax */
                if (cmtlev <= 0 && !qmode)
                    quoteit = TRUE;
                if (copylev > 0 && !skipping)
                {
                    *bp++ = c;
                    *bp++ = c;
                }
                p++;
                goto putg;
            }

            gotcolon = TRUE;

            bp = bufhead;
            if (quoteit)
            {
                *bp++ = '"';

                /* back up over the ':' and any spaces */
                --p;
                while (isascii((UCHAR)*--p) && isspace((UCHAR)*p))
                    continue;
                p++;
            }
            for (q = addrhead; q < p; )
            {
                c = *q++;
                if (bp < buflim)
                {
                    if (quoteit && c == '"')
                        *bp++ = '\\';
                    *bp++ = c;
                }
            }
            if (quoteit)
            {
                if (bp == &bufhead[1])
                    bp--;
                else
                    *bp++ = '"';
                while ((c = *p++) != ':')
                {
                    if (bp < buflim)
                        *bp++ = c;
                }
                *bp++ = c;
            }

            /* any trailing white space is part of group: */
            while (isascii((UCHAR)*p) && isspace((UCHAR)*p) && bp < buflim)
                *bp++ = *p++;
            copylev = 0;
            putgmac = quoteit = FALSE;
            bufhead = bp;
            addrhead = p;
            continue;
        }

        if (c == ';' && copylev <= 0 && !ColonOkInAddr)
        {
            if (bp < buflim)
                *bp++ = c;
        }

        /* check for characters that may have to be quoted */
        if (strchr(MustQuoteChars, c) != NULL)
        {
            /*
            **  If these occur as the phrase part of a <>
            **  construct, but are not inside of () or already
            **  quoted, they will have to be quoted.  Note that
            **  now (but don't actually do the quoting).
            */

            if (cmtlev <= 0 && !qmode)
                quoteit = TRUE;
        }

        /* check for angle brackets */
        if (c == '<')
        {
            register char *q;

            /* assume first of two angles is bogus */
            if (gotangle)
                quoteit = TRUE;
            gotangle = TRUE;

            /* oops -- have to change our mind */
            anglelev = 1;
            if (!skipping)
                realanglelev = 1;

            bp = bufhead;
            if (quoteit)
            {
                *bp++ = '"';

                /* back up over the '<' and any spaces */
                --p;
                while (isascii((UCHAR)*--p) && isspace((UCHAR)*p))
                    continue;
                p++;
            }
            for (q = addrhead; q < p; )
            {
                c = *q++;
                if (bp < buflim)
                {
                    if (quoteit && c == '"')
                        *bp++ = '\\';
                    *bp++ = c;
                }
            }
            if (quoteit)
            {
                if (bp == &buf[1])
                    bp--;
                else
                    *bp++ = '"';
                while ((c = *p++) != '<')
                {
                    if (bp < buflim)
                        *bp++ = c;
                }
                *bp++ = c;
            }
            copylev = 0;
            putgmac = quoteit = FALSE;
            continue;
        }

        if (c == '>')
        {
            if (anglelev > 0)
            {
                anglelev--;
                if (!skipping)
                {
                    realanglelev--;
                    buflim++;
                }
            }
            else if (!skipping)
            {
                /* syntax error: unmatched > */
                if (copylev > 0)
                    bp--;
                quoteit = TRUE;
                continue;
            }
            if (copylev++ <= 0)
                *bp++ = c;
            continue;
        }

        /* must be a real address character */
    putg:
        if (copylev <= 0 && !putgmac)
        {
            *bp++ = MACROEXPAND;
            *bp++ = 'g';
            putgmac = TRUE;
        }
    }

    /* repair any syntactic damage */
    if (realqmode)
        *bp++ = '"';
    while (realcmtlev-- > 0)
        *bp++ = ')';
    while (realanglelev-- > 0)
        *bp++ = '>';
    *bp++ = '\0';

    if (tTd(33, 1))
        printf("crackaddr=>`%s'\n", buf);

    return (buf);
}
/*
**  PUTHEADER -- put the header part of a message from the in-core copy
**
**    Parameters:
**        mci -- the connection information.
**        h -- the header to put.
**        e -- envelope to use.
**
**    Returns:
**        none.
**
**    Side Effects:
**        none.
*/

/*
 * Macro for fast max (not available in e.g. DG/UX, 386/ix).
 */
#ifndef MAX
# define MAX(a,b) (((a)>(b))?(a):(b))
#endif

void
putheader(mci, h, e)
    register MCI *mci;
    register HDR *h;
    register ENVELOPE *e;
{
    char buf[MAX(MAXLINE,BUFSIZ)];
    char obuf[MAXLINE];

    if (tTd(34, 1))
        printf("--- putheader, mailer = %s ---\n",
            mci->mci_mailer->m_name);

    mci->mci_flags |= MCIF_INHEADER;
    for (; h != NULL; h = h->h_link)
    {
        register char *p = h->h_value;
        extern bool bitintersect();

        if (tTd(34, 11))
        {
            printf("  %s: ", h->h_field);
            xputs(p);
        }

        /* suppress Content-Transfer-Encoding: if we are MIMEing */
        if (bitset(H_CTE, h->h_flags) &&
            bitset(MCIF_CVT8TO7|MCIF_INMIME, mci->mci_flags))
        {
            if (tTd(34, 11))
                printf(" (skipped (content-transfer-encoding))\n");
            continue;
        }

        if (bitset(MCIF_INMIME, mci->mci_flags))
            goto vanilla;

        if (bitset(H_CHECK|H_ACHECK, h->h_flags) &&
            !bitintersect(h->h_mflags, mci->mci_mailer->m_flags))
        {
            if (tTd(34, 11))
                printf(" (skipped)\n");
            continue;
        }

        /* handle Resent-... headers specially */
        if (bitset(H_RESENT, h->h_flags) && !bitset(EF_RESENT, e->e_flags))
        {
            if (tTd(34, 11))
                printf(" (skipped (resent))\n");
            continue;
        }

        /* suppress return receipts if requested */
        if (bitset(H_RECEIPTTO, h->h_flags) &&
            bitset(EF_NORECEIPT, e->e_flags))
        {
            if (tTd(34, 11))
                printf(" (skipped (receipt))\n");
            continue;
        }

        /* macro expand value if generated internally */
        if (bitset(H_DEFAULT, h->h_flags))
        {
            expand(p, buf, sizeof buf, e);
            p = buf;
            if (p == NULL || *p == '\0')
            {
                if (tTd(34, 11))
                    printf(" (skipped -- null value)\n");
                continue;
            }
        }

        if (tTd(34, 11))
            printf("\n");

        if (bitset(H_STRIPVAL, h->h_flags))
        {
            /* empty field */
            (void) sprintf(obuf, "%s:", h->h_field);
            putline(obuf, mci);
        }
        else if (bitset(H_FROM|H_RCPT, h->h_flags))
        {
            /* address field */
            bool oldstyle = bitset(EF_OLDSTYLE, e->e_flags);

            if (bitset(H_FROM, h->h_flags))
                oldstyle = FALSE;
            commaize(h, p, oldstyle, mci, e);
        }
        else
        {
            /* vanilla header line */
            register char *nlp;

vanilla:
            (void) sprintf(obuf, "%s: ", h->h_field);
            while ((nlp = strchr(p, '\n')) != NULL)
            {
                *nlp = '\0';
                (void) strcat(obuf, p);
                *nlp = '\n';
                putline(obuf, mci);
                p = ++nlp;
                obuf[0] = '\0';
            }
            (void) strcat(obuf, p);
            putline(obuf, mci);
        }
    }

    /*
    **  If we are converting this to a MIME message, add the
    **  MIME headers.
    */

#if MIME8TO7
    if (bitset(MM_MIME8BIT, MimeMode) &&
        bitset(EF_HAS8BIT, e->e_flags) &&
        !bitset(EF_DONT_MIME, e->e_flags) &&
        !bitnset(M_8BITS, mci->mci_mailer->m_flags) &&
        !bitset(MCIF_CVT8TO7, mci->mci_flags))
    {
        if (hvalue("MIME-Version", e->e_header) == NULL)
            putline("MIME-Version: 1.0", mci);
        if (hvalue("Content-Type", e->e_header) == NULL)
        {
            sprintf(obuf, "Content-Type: text/plain; charset=%s",
                defcharset(e));
            putline(obuf, mci);
        }
        if (hvalue("Content-Transfer-Encoding", e->e_header) == NULL)
            putline("Content-Transfer-Encoding: 8bit", mci);
    }
#endif
}
/*
**  COMMAIZE -- output a header field, making a comma-translated list.
**
**    Parameters:
**        h -- the header field to output.
**        p -- the value to put in it.
**        oldstyle -- TRUE if this is an old style header.
**        mci -- the connection information.
**        e -- the envelope containing the message.
**
**    Returns:
**        none.
**
**    Side Effects:
**        outputs "p" to file "fp".
*/

void
commaize(h, p, oldstyle, mci, e)
    register HDR *h;
    register char *p;
    bool oldstyle;
    register MCI *mci;
    register ENVELOPE *e;
{
    register char *obp;
    int opos;
    int omax;
    bool firstone = TRUE;
    char obuf[MAXLINE + 3];

    /*
    **  Output the address list translated by the
    **  mailer and with commas.
    */

    if (tTd(14, 2))
        printf("commaize(%s: %s)\n", h->h_field, p);

    obp = obuf;
    (void) sprintf(obp, "%s: ", h->h_field);
    opos = strlen(h->h_field) + 2;
    obp += opos;
    omax = mci->mci_mailer->m_linelimit - 2;
    if (omax < 0 || omax > 78)
        omax = 78;

    /*
    **  Run through the list of values.
    */

    while (*p != '\0')
    {
        register char *name;
        register int c;
        char savechar;
        int flags;
        auto int stat;

        /*
        **  Find the end of the name.  New style names
        **  end with a comma, old style names end with
        **  a space character.  However, spaces do not
        **  necessarily delimit an old-style name -- at
        **  signs mean keep going.
        */

        /* find end of name */
        while ((isascii((UCHAR)*p) && isspace((UCHAR)*p)) || *p == ',')
            p++;
        name = p;
        for (;;)
        {
            auto char *oldp;
            char pvpbuf[PSBUFSIZE];

            (void) prescan(p, oldstyle ? ' ' : ',', pvpbuf,
                       sizeof pvpbuf, &oldp, NULL);
            p = oldp;

            /* look to see if we have an at sign */
            while (*p != '\0' && isascii((UCHAR)*p) && isspace((UCHAR)*p))
                p++;

            if (*p != '@')
            {
                p = oldp;
                break;
            }
            p += *p == '@' ? 1 : 2;
            while (*p != '\0' && isascii((UCHAR)*p) && isspace((UCHAR)*p))
                p++;
        }
        /* at the end of one complete name */

        /* strip off trailing white space */
        while (p >= name &&
               ((isascii((UCHAR)*p) && isspace((UCHAR)*p)) || *p == ',' || *p == '\0'))
            p--;
        if (++p == name)
            continue;
        savechar = *p;
        *p = '\0';

        /* translate the name to be relative */
        flags = RF_HEADERADDR|RF_ADDDOMAIN;
        if (bitset(H_FROM, h->h_flags))
            flags |= RF_SENDERADDR;
#if USERDB
        else if (e->e_from.q_mailer != NULL &&
             bitnset(M_UDBRECIPIENT, e->e_from.q_mailer->m_flags))
        {
            extern char *udbsender();
            char *q;

            q = udbsender(name);
            if (q != NULL)
                name = q;
        }
#endif
        stat = EX_OK;
        name = remotename(name, mci->mci_mailer, flags, &stat, e);
        if (*name == '\0')
        {
            *p = savechar;
            continue;
        }

        /* output the name with nice formatting */
        opos += strlen(name);
        if (!firstone)
            opos += 2;
        if (opos > omax && !firstone)
        {
            (void) strcpy(obp, ",\n");
            putline(obuf, mci);
            obp = obuf;
            (void) strcpy(obp, "        ");
            opos = strlen(obp);
            obp += opos;
            opos += strlen(name);
        }
        else if (!firstone)
        {
            (void) strcpy(obp, ", ");
            obp += 2;
        }

        while ((c = *name++) != '\0' && obp < &obuf[MAXLINE])
            *obp++ = c;
        firstone = FALSE;
        *p = savechar;
    }
    (void) strcpy(obp, "\n");
    putline(obuf, mci);
}
/*
**  COPYHEADER -- copy header list
**
**    This routine is the equivalent of newstr for header lists
**
**    Parameters:
**        header -- list of header structures to copy.
**
**    Returns:
**        a copy of 'header'.
**
**    Side Effects:
**        none.
*/

HDR *
copyheader(header)
    register HDR *header;
{
    register HDR *newhdr;
    HDR *ret;
    register HDR **tail = &ret;

    while (header != NULL)
    {
        newhdr = (HDR *) xalloc(sizeof(HDR));
        STRUCTCOPY(*header, *newhdr);
        *tail = newhdr;
        tail = &newhdr->h_link;
        header = header->h_link;
    }
    *tail = NULL;

    return ret;
}
#endif*/

//-----------------------------------------------------------------------------
//  Description:
//      Given a header, this function parses it for CRLF<LWSP> sequences
//      unfolds it into a separate buffer, and returns the unfolded line. If
//      the header passed in is not folded, no unfolded line is returned.
//  Arguments:
//      IN CHAR *pszHeader - The folded header. *MUST* be a NULL terminated
//          string.
//      OUT CHAR **ppszUnfolded - If pszValueBuf is not folded, NULL is
//          returned otherwise a new buffer is allocated and pszValueBuf
//          unfolded into the buffer, and the buffer is returned in this
//          parameter. The buffer must be freed with FreeUnfoldedBuffer().
//  Returns:
//      TRUE on success.
//      FALSE on out of memory errors.
//-----------------------------------------------------------------------------
BOOL UnfoldHeader(char *pszHeader, char **ppszUnfolded)
{
    int cbHeader = 0;
    BOOL fUnfold = FALSE;

    *ppszUnfolded = NULL;

    // Check for embedded CRLF<LWSP> AND get length of pszHeader at the same time
    for(cbHeader = 0; pszHeader[cbHeader]; cbHeader++)
    {
        if(!fUnfold &&

           pszHeader[cbHeader] == '\r' &&

           (pszHeader[cbHeader + 1] &&
            pszHeader[cbHeader + 1] == '\n') &&

           (pszHeader[cbHeader + 2] &&
           (pszHeader[cbHeader + 2] == ' ' || pszHeader[cbHeader + 2] == '\t')))

        {
            fUnfold = TRUE;
        }
    }

    if(!fUnfold)
        return TRUE;

    *ppszUnfolded = new char[cbHeader + 1];
    if(!*ppszUnfolded)
        return FALSE;

    //
    // Copy pszHeader to *ppszUnfolded while skipping CRLF<LWSP> sequences
    //

    int i = 0;
    int j = 0;
    while(j < cbHeader)
    {
        if(pszHeader[j] == '\r' && (j+1 < cbHeader && pszHeader[j+1] == '\n'))
        {
            j += 2; // skip the CRLF we found

            while(j < cbHeader && (pszHeader[j] == ' ' || pszHeader[j] == '\t'))
                j++; // skip LWSP

            if(j >= cbHeader)
                break;
        }
        (*ppszUnfolded)[i++] = pszHeader[j++];
    }
    _ASSERT(i < cbHeader);
    (*ppszUnfolded)[i] = '\0';
    return TRUE;
}

void FreeUnfoldedHeader(char *pszHeader)
{
    delete [] pszHeader;
}

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//--------------------------------------------------------------------------

/***
*w4io.h - fake FILE structure for Win 4 printf/sprintf/debug printf support
*
*/

#ifndef NULL
#  define NULL 0
#endif

struct w4io
{
    union
    {
        struct
        {
            wchar_t *_pwcbuf;   // wchar_t output buffer
            wchar_t *_pwcstart;
        } wc;
        struct
        {
            char *_pchbuf;      // char output buffer
            char *_pchstart;
        } ch;
    } buf ;
    unsigned int cchleft;       // output buffer character count
    void (_cdecl *writechar)(int ch,
                             int num,
                             struct w4io *f,
                             int *pcchwritten);
};

#define pwcbuf          buf.wc._pwcbuf
#define pwcstart        buf.wc._pwcstart
#define pchbuf          buf.ch._pchbuf
#define pchstart        buf.ch._pchstart

#define REG1 register
#define REG2 register

/* prototypes */
int _cdecl w4iooutput(struct w4io *stream, const char *format, va_list argptr);

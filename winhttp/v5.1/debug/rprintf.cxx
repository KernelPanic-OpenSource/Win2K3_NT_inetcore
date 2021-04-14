/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    rprintf.cxx

Abstract:

    Contains my own version of printf(), sprintf() and vprintf(). Useful since
    adding new printf escape sequences becomes easy

    Contents:
        rprintf     limited re-entrant version of printf
        rsprintf    limited re-entrant version of sprintf
        _sprintf    routine which does the work

Author:

    Richard L Firth (rfirth) 20-Jun-1995

Revision History:

    29-Aug-1989 rfirth
        Created

	27-Jul-2001 sergekh
		_sprintf modified to request buffer size to avoid overruns

	11-Apr-2002 lcleeton
	    _sprintf modified to print sockaddr's as IP address literal strings - new token %A
--*/

#include <wininetp.h>
#include "rprintf.h"

//
// defines for flags word
//

#define F_SPACES        0x00000001  // prefix field with spaces
#define F_ZEROES        0x00000002  // prefix field with zeroes
#define F_MINUS         0x00000004  // field is left justified
#define F_HASH          0x00000008  // hex field is prefixed with 0x/0X
#define F_XUPPER        0x00000010  // hex field has upper case letters
#define F_LONG          0x00000020  // long int/hex/oct prefix
#define F_PLUS          0x00000040  // prefix +'ve signed number with +
#define F_DOT           0x00000080  // separator for field and precision
#define F_NEAR          0x00000100  // far pointer has near prefix
#define F_FAR           0x00000200  // near pointer has far prefix
#define F_SREPLICATE    0x00000400  // this field replicated
#define F_EREPLICATE    0x00000800  // end of replications
#define F_UNICODE       0x00001000  // string is wide character (%ws/%wq)
#define F_QUOTING       0x00002000  // strings enclosed in double quotes
#define F_ELLIPSE       0x00004000  // a sized, quoted string ends in "..."

#define BUFFER_SIZE     1024

//
// minimum field widths for various ASCII representations of numbers
//

#define MIN_BIN_WIDTH   16          // minimum field width in btoa
#define MIN_HEX_WIDTH   8           // minimum field width in xtoa
#define MIN_INT_WIDTH   10          // minimum field width in itoa
#define MIN_LHEX_WIDTH  8           // minimum field width in long xtoa
#define MIN_LINT_WIDTH  10          // minimum field width in long itoa
#define MIN_LOCT_WIDTH  11          // minimum field width in long otoa
#define MIN_OCT_WIDTH   11          // minimum field width in otoa
#define MIN_UINT_WIDTH  10          // minimum field width in utoa

//
// character defines
//

#define EOSTR           '\0'
#define CR              '\x0d'
#define LF              '\x0a'

#if !defined(min)

#define min(a, b)   ((a)<(b)) ? (a) : (b)

#endif

PRIVATE int     _atoi(char**);
PRIVATE void    convert(char**, char*, ULONG_PTR, int, int, unsigned, char(*)(ULONG_PTR*));
PRIVATE char    btoa(ULONG_PTR *);
PRIVATE char    otoa(ULONG_PTR *);
PRIVATE char    utoa(ULONG_PTR *);
PRIVATE char    xtoa(ULONG_PTR *);
PRIVATE char    Xasc(ULONG_PTR *);

/***    rprintf - a re-entrant cut-down version of printf. Understands usual
 *                  printf format characters introduced by '%' plus one or
 *                  two additions
 *
 *      ENTRY   format  - pointer to buffer containing format string defining
 *                        the output. As per usual printf the arguments to
 *                        fill in the blanks in the format string are on the
 *                        the stack after the format string
 *
 *              <args>  - arguments on stack, size and type determined from
 *                        the format string
 *
 *      EXIT    format string used to convert arguments (if any) and print
 *              the results to stdout.
 *              The number of character copied is the value returned
 */

#ifdef UNUSED
int cdecl rprintf(char* format, ...) {

    int charsPrinted = 0;
    char buffer[BUFFER_SIZE];
    DWORD nwritten;
    va_list args;

    /* print the output into  buffer then print the formatted buffer to the
     * screen
     */

    va_start(args, format);
    charsPrinted = _sprintf(buffer, format, args);
    va_end(args);

    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),
                              buffer,
                              charsPrinted,
                              &nwritten,
                              0
                              );
    return nwritten;
}
#endif

/***    rsprintf - a re-entrant cut-down version of sprintf. See rprintf for
 *                  details
 *
 *      ENTRY   buffer  - pointer to the buffer which will receive the
 *                        formatted output
 *
 *				buf_size - size of buffer to control overrun
 *
 *              format  - pointer to buffer which defines the formatted
 *                        output. Consists of normal printing characters
 *                        and printf-style format characters (see rprintf)
 *
 *      EXIT    characters from format string and arguments converted to
 *              character format based on format string are copied into the
 *              buffer
 *              The number of character copied is the value returned
 */

int cdecl rsprintf(char* buffer, size_t buf_size, char* format, ...) {

    va_list args;
    int n;

    va_start(args, format);
    n = _sprintf(buffer, buf_size, format, args);
    va_end(args);
    return n;
}

/***    _sprintf - performs the sprintf function. Receives an extra parameter
 *                  on the stack which is the pointer to the variable argument
 *                  list of rprintf and rsprintf
 *
 *      ENTRY   buffer  - pointer to buffer which will receive the output
 *
 *				buf_size - size of buffer to control overrun
 *
 *              format  - pointer to the format string
 *
 *              args    - variable argument list which will be used to fill in
 *                        the escape sequences in the format string
 *
 *      EXIT    The characters in the format string are used to convert the
 *              arguments and copy them to the buffer.
 *              The number of character copied is the value returned
 */


int cdecl _sprintf(char* buffer, size_t buf_size, char* format, va_list args) {

#define CHECK_BUF		if (tail == buffer) goto Quit;

    char*       original = buffer;
    int         FieldWidth;
    int         FieldPrecision;
    int         FieldLen;
    BOOL        SubDone;
    int         StrLen;
    int         i;
    DWORD       flags;
    int         replications;
    char*       s;

	//calculate tail and reserve space for 0
	char*		tail = buffer + buf_size - 1;

	if (buf_size == 0)
		return 0;
	else if (buf_size == 1)
		goto Quit;

    while (*format) {
        switch ((unsigned)*format) {
        case '\n':

            //
            // convert line-feed to carriage-return, line-feed. But only if the
            // format string doesn't already contain a carriage-return directly
            // before the line-feed! This way we can make multiple calls into
            // this function, with the same buffer, and only once expand the
            // line-feed
            //

            if (*(buffer - 1) != CR) {
                *buffer++ = CR; CHECK_BUF;
            }
            *buffer++ = LF; CHECK_BUF;
            break;

        case '%':
            SubDone = FALSE;
            flags = 0;
            FieldWidth = 0;
            FieldPrecision = 0;
            replications = 1;   /* default replication is 1 */
            while (!SubDone) {
                switch ((unsigned)*++format) {
                case '%':
                    *buffer++ = '%'; CHECK_BUF;
                    SubDone = TRUE;
                    break;

                case ' ':
                    flags |= F_SPACES;
                    break;

                case '#':
                    flags |= F_HASH;
                    break;

                case '-':
                    flags |= F_MINUS;
                    break;

                case '+':
                    flags |= F_PLUS;
                    break;

                case '.':
                    flags |= F_DOT;
                    break;

                case '*':
                    if (flags & F_DOT) {
                        FieldPrecision = va_arg(args, int);
                    } else {
                        FieldWidth = va_arg(args, int);
                    }
                    break;

                case '@':
                    replications = _atoi(&format);
                    break;

                case '[':
                    flags |= F_SREPLICATE;
                    break;

                case ']':
                    flags |= F_EREPLICATE;
                    break;

                case '0':
                    /* if this is leading zero then caller wants
                     * zero prefixed number of given width (%04x)
                     */
                    if (!(flags & F_ZEROES)) {
                        flags |= F_ZEROES;
                        break;
                    }

                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    if (flags & F_DOT) {
                        FieldPrecision = _atoi(&format);
                    } else {
                        FieldWidth = _atoi(&format);
                    }
                    break;

                case 'A':
                    CHAR            AddrBuff[INET6_ADDRSTRLEN+1];
                    DWORD           AddrStrLen;
                    LPSOCKADDR_IN   lpSin;        
                    LPSOCKADDR_IN6  lpSin6;
                    DWORD           AError;

                    
                    lpSin = va_arg(args, LPSOCKADDR_IN);
                    lpSin6 = (LPSOCKADDR_IN6)lpSin;

                    if (lpSin == NULL) {
                        
                        strcpy(AddrBuff, "<null>");
                        AddrStrLen = lstrlen(AddrBuff);

                    } else {
                    
                        if (IsBadReadPtr(lpSin, sizeof(u_short))) {
                            goto Quit;
                        }
                        if ((lpSin->sin_family != AF_INET) && (lpSin->sin_family != AF_INET6)) {
                               goto Quit;   
                        }
                        if (lpSin->sin_family == AF_INET) {
                            if (IsBadReadPtr(lpSin, sizeof(SOCKADDR_IN))) {
                                goto Quit;
                            }
                        } else {
                            if (IsBadReadPtr(lpSin, sizeof(SOCKADDR_IN6))) {
                                goto Quit;
                            }
                        }
                        AddrStrLen = sizeof(AddrBuff);
                        AError = _I_WSAAddressToStringA(
                                    (LPSOCKADDR)lpSin, 
                                    (lpSin->sin_family == AF_INET) ? sizeof(SOCKADDR_IN) : sizeof(SOCKADDR_IN6),
                                    NULL,
                                    AddrBuff,
                                    &AddrStrLen
                                    );
                        if (AError) {
                            goto Quit;
                        }
                    }
                    
                    // let %s handling do the rest!

                    s = AddrBuff;
                    replications = 0;
                    flags &= ~F_UNICODE;
                    goto s_continue;
                   
                case 'b':

                    //
                    // Binary representation
                    //

                    while (replications--) {
                        convert(&buffer,
								tail,
                                va_arg(args, unsigned int),
                                (FieldWidth) ? FieldWidth : MIN_BIN_WIDTH,
                                MIN_BIN_WIDTH,
                                flags,
                                btoa
                                );
						CHECK_BUF;
                    }
                    SubDone = TRUE;
                    break;

                case 'B':

                    //
                    // Boolean representation
                    //

                    if (va_arg(args, BOOL)) {
                        *buffer++ = 'T'; CHECK_BUF;
                        *buffer++ = 'R'; CHECK_BUF;
                        *buffer++ = 'U'; CHECK_BUF;
                        *buffer++ = 'E'; CHECK_BUF;
                    } else {
                        *buffer++ = 'F'; CHECK_BUF;
                        *buffer++ = 'A'; CHECK_BUF;
                        *buffer++ = 'L'; CHECK_BUF;
                        *buffer++ = 'S'; CHECK_BUF;
                        *buffer++ = 'E'; CHECK_BUF;
                    }
                    SubDone = TRUE;
                    break;

                case 'c':

                    //
                    // assume that a character is the size of the
                    // width of the stack which in turn has the same
                    // width as an integer
                    //

                    --FieldWidth;
                    while (replications--) {
                        for (i = 0; i < FieldWidth; i++) {
                            *buffer++ = ' '; CHECK_BUF;
                        }
                        *buffer++ = (char) va_arg(args, int); CHECK_BUF;
                    }
                    SubDone = TRUE;
                    break;

                case 'd':
                case 'i':
                    while (replications--) {

                        long l;

                        l = (flags & F_LONG) ? va_arg(args, long) : (long)va_arg(args, int);
                        if (l < 0) {
                            *buffer++ = '-'; CHECK_BUF;
                            if (flags & F_LONG) {
                                l = -(long)l;
                            } else {
                                l = -(int)l;
                            }
                        } else if (flags & F_PLUS) {
                            *buffer++ = '+'; CHECK_BUF;
                        }
                        convert(&buffer,
								tail,
                                l,
                                FieldWidth,
                                (flags & F_LONG) ? MIN_LINT_WIDTH : MIN_INT_WIDTH,
                                flags,
                                utoa
                                );
						CHECK_BUF;
                    }
                    SubDone = TRUE;
                    break;

                case 'e':
                    /* not currently handled */
                    break;

                case 'f':
                    /* not currently handled */
                    break;

                case 'F':
                    flags |= F_FAR;
                    break;

                case 'g':
                case 'G':
                    /* not currently handled */
                    break;

                case 'h':
                    /* not currently handled */
                    break;

                case 'l':
                    flags |= F_LONG;
                    break;

                case 'L':
                    /* not currently handled */
                    break;

                case 'n':
                    *(va_arg(args, int*))  = (int)(buffer - original);
                    SubDone = TRUE;
                    break;

                case 'N':
                    flags |= F_NEAR;
                    break;

                case 'o':
                    while (replications--) {
                        convert(&buffer,
								tail,
                                (flags & F_LONG) ? va_arg(args, unsigned long) : (unsigned long)va_arg(args, unsigned),
                                FieldWidth,
                                (flags & F_LONG) ? MIN_LOCT_WIDTH : MIN_OCT_WIDTH,
                                flags,
                                otoa
                                );
						CHECK_BUF;
                    }
                    SubDone = TRUE;
                    break;

                case 'p':
                    while (replications--) {


                        if (!(flags & F_NEAR)) {
                            convert(&buffer,
									tail,
                                    (ULONG_PTR) va_arg(args, char near *),
                                    MIN_HEX_WIDTH,
                                    MIN_HEX_WIDTH,
                                    flags | F_XUPPER,
                                    Xasc
                                    );
							CHECK_BUF;
                            *buffer++ = ':'; CHECK_BUF;
                        }
                        convert(&buffer,
								tail,
                                (ULONG_PTR)va_arg(args, unsigned),
                                MIN_HEX_WIDTH,
                                MIN_HEX_WIDTH,
                                flags | F_XUPPER,
                                Xasc
                                );
						CHECK_BUF;
                    }
                    SubDone = TRUE;
                    break;

                case 'Q':       // quoted unicode string
                    flags |= F_UNICODE;
                    // *** FALL THROUGH ***

                case 'q':
                    *buffer++ = '"'; CHECK_BUF;
                    flags |= F_QUOTING;

                    //
                    // *** FALL THROUGH ***
                    //

                case 's':
                    while (replications--) {


                        s = va_arg(args, char*);

s_continue:             // can jump here from case 'A' above

                        //  If the string has a field precision, IsBadStringPtr()
                        //should not check past that many characters because the string
                        //may not even be null-terminated (and its sufficient to check
                        //up to FieldPrecision characters).
                        //  If the string does not have a FieldPrecision, we tell
                        //IsBadStringPtr to check up to the null terminator with a -1.
                        int iIsBadStringPtrCheckSize = FieldPrecision ? FieldPrecision : -1;
                    
                        if ( s != NULL
                             && (flags & F_UNICODE
                                 ? IsBadStringPtrW( (LPWSTR)s,iIsBadStringPtrCheckSize)
                                 : IsBadStringPtrA( (LPSTR)s,iIsBadStringPtrCheckSize)))
                        {
                            *buffer++ = '('; CHECK_BUF;
                            *buffer++ = 'b'; CHECK_BUF;
                            *buffer++ = 'o'; CHECK_BUF;
                            *buffer++ = 'g'; CHECK_BUF;
                            *buffer++ = 'u'; CHECK_BUF;
                            *buffer++ = 's'; CHECK_BUF;
                            *buffer++ = 'S'; CHECK_BUF;
                            *buffer++ = 't'; CHECK_BUF;
                            *buffer++ = 'r'; CHECK_BUF;
                            *buffer++ = 'i'; CHECK_BUF;
                            *buffer++ = 'n'; CHECK_BUF;
                            *buffer++ = 'g'; CHECK_BUF;
                            *buffer++ = 'P'; CHECK_BUF;
                            *buffer++ = 't'; CHECK_BUF;
                            *buffer++ = 'r'; CHECK_BUF;
                            *buffer++ = ':'; CHECK_BUF;
                            convert(&buffer,
                                    tail,
                                    (ULONG_PTR)va_arg(args, unsigned),
                                    MIN_HEX_WIDTH,
                                    MIN_HEX_WIDTH,
                                    flags | F_XUPPER,
                                    Xasc);
                            CHECK_BUF;
                            *buffer++ = ')'; CHECK_BUF;
                            
                        }
                        else if (s != NULL)
                        {
                            // darrenmi 2/24/00 Note that if the string has a field precision,
                            // it's not always null terminated!! Don't depend on it being psz
                            // and stop when we hit our max length.
                            StrLen = 0;

                            if (flags & F_UNICODE) {
                                WCHAR   *pWork = (LPWSTR)s;
                                while((!FieldPrecision || StrLen < FieldPrecision) && *pWork)
                                {
                                    pWork++;
                                    StrLen++;
                                }
                            } else {
                                CHAR    *pWork = s;
                                while((!FieldPrecision || StrLen < FieldPrecision) && *pWork)
                                {
                                    pWork++;
                                    StrLen++;
                                }
                            }
                            
                            FieldLen = (FieldPrecision)
                                        ? min(StrLen, FieldPrecision)
                                        : StrLen
                                        ;
                            if ((flags & F_QUOTING) && (FieldPrecision > 3) && (FieldLen < StrLen)) {
                                FieldLen -= 3;
                                flags |= F_ELLIPSE;
                            }

                            for (i = 0; i < (FieldWidth - FieldLen); i++) {
                                *buffer++ = ' '; CHECK_BUF;
                            }

                            if (flags & F_UNICODE) {

                                char wbuf[4096];
                                int wi;

                                WideCharToMultiByte(CP_ACP, 0,
                                        (LPWSTR)s, -1,
                                        wbuf, 4096,
                                        NULL, NULL);

                                for (wi = 0; wbuf[wi] && FieldLen; ++wi) {
                                    *buffer = wbuf[wi];

                                    //
                                    // if this is a quoted string, and it contains
                                    // \r and/or \n, then we reverse-convert these
                                    // characters, since we don't want then to
                                    // break the string. Do the same for \t
                                    //

                                    if (flags & F_QUOTING) {

                                        char ch;

                                        ch = *buffer;
                                        if ((ch == '\r') || (ch == '\n') || (ch == '\t')) {
                                            *buffer++ = '\\'; CHECK_BUF;
                                            *buffer = (ch == '\r')
                                                        ? 'r'
                                                        : (ch == '\n')
                                                            ? 'n'
                                                            : 't'
                                                            ;
                                        }
                                    }
                                    ++buffer; CHECK_BUF;
                                    --FieldLen;
                                }
                            } else {
                                while (*s && FieldLen) {
                                    *buffer = *s++;

                                    //
                                    // if this is a quoted string, and it contains
                                    // \r and/or \n, then we reverse-convert these
                                    // characters, since we don't want then to
                                    // break the string. Do the same for \t
                                    //

                                    if (flags & F_QUOTING) {

                                        char ch;

                                        ch = *buffer;
                                        if ((ch == '\r') || (ch == '\n') || (ch == '\t')) {
                                            *buffer++ = '\\'; CHECK_BUF;
                                            *buffer = (ch == '\r')
                                                        ? 'r'
                                                        : (ch == '\n')
                                                            ? 'n'
                                                            : 't'
                                                            ;
                                        }
                                    }
                                    ++buffer; CHECK_BUF;
                                    --FieldLen;
                                }
                            }
                            if (flags & F_ELLIPSE) {
                                *buffer++ = '.'; CHECK_BUF;
                                *buffer++ = '.'; CHECK_BUF;
                                *buffer++ = '.'; CHECK_BUF;
                            }
                        } else if (!(flags & F_QUOTING)) {
                            *buffer++ = '('; CHECK_BUF;
                            *buffer++ = 'n'; CHECK_BUF;
                            *buffer++ = 'u'; CHECK_BUF;
                            *buffer++ = 'l'; CHECK_BUF;
                            *buffer++ = 'l'; CHECK_BUF;
                            *buffer++ = ')'; CHECK_BUF;
                        }
                    }
                    if (flags & F_QUOTING) {
                        *buffer++ = '"'; CHECK_BUF;
                    }
                    SubDone = TRUE;
                    break;

                case 'S':
                    break;

                case 'u':
                    while (replications--) {
                        convert(&buffer,
								tail,
                                va_arg(args, unsigned),
                                FieldWidth,
                                MIN_UINT_WIDTH,
                                flags,
                                utoa
                                );
						CHECK_BUF;
                    }
                    SubDone = TRUE;
                    break;

                case 'w':
                    flags |= F_UNICODE;
                    break;

                case 'X':
                    flags |= F_XUPPER;

                    //
                    // *** FALL THROUGH ***
                    //

                case 'x':
                    while (replications--) {
                        if (flags & F_HASH) {
                            *buffer++ = '0'; CHECK_BUF;
                            *buffer++ = (flags & F_XUPPER) ? (char)'X' : (char)'x'; CHECK_BUF;
                        }
                        convert(&buffer,
								tail,
                                (flags & F_LONG) ? va_arg(args, unsigned long) : va_arg(args, unsigned),
                                FieldWidth,
                                (flags & F_LONG) ? MIN_LHEX_WIDTH : MIN_HEX_WIDTH,
                                flags,
                                (flags & F_XUPPER) ? Xasc : xtoa
                                );
						CHECK_BUF;
                    }
                    SubDone = TRUE;
                    break;
                } /* switch <%-specifier> */
            }
            break;

        default:
            *buffer++ = *format; CHECK_BUF;
        } /* switch <character> */
        ++format;
    } /* while */
Quit:
    *buffer = EOSTR;
    return (int)(buffer - original);
}

/***    _atoi - ascii to integer conversion used to get the field width out
 *              of the format string
 *
 *      ENTRY   p - pointer to pointer to format string
 *
 *      EXIT    returns the number found in the prefix string as a (16-bit)
 *              int format string pointer is updated past the field width
 */

PRIVATE
int _atoi(char** p) {

    int n = 0;
    int i = 5;

    while ((**p >= '0' && **p <= '9') && i--) {
        n = n*10+((int)(*(*p)++)-(int)'0');
    }

    /* put the format pointer back one since the major loop tests *++format */

    --*p;
    return n;
}

/***    convert - convert number to representation defined by procedure
 *
 *      ENTRY   buffer  - pointer to buffer to receive conversion
 *				tail	- pointer to the end of buffer to prevent overrun
 *              n       - number to convert
 *              width   - user defined field width
 *              mwidth  - minimum width for representation
 *              flags   - flags controlling conversion
 *              proc    - pointer to conversion routine
 *
 *      EXIT    buffer is updated to point past the number representation
 *              just put into it
 */

PRIVATE
void
convert(
    char** buffer,
	char* tail,
    ULONG_PTR n,
    int width,
    int mwidth,
    unsigned flags,
    char (*proc)(ULONG_PTR*)
    )
{
    char    numarray[33];
    int     MinWidth;
    int     i;

    MinWidth = (width < mwidth) ? mwidth : width;
    i = MinWidth;
    do {
        numarray[--i] = (*proc)(&n);
    } while (n);
    while (width > MinWidth-i) {
        numarray[--i] = (char)((flags & F_SPACES) ? ' ' : '0');
    }
    while (i < MinWidth) {
        *(*buffer)++ = numarray[i++];
		if (*buffer == tail)
			break;
    }
}

/***    btoa - return next (least significant) char in a binary to ASCII
 *              conversion
 *
 *      ENTRY   pn  - pointer to number to convert
 *
 *      EXIT    returns next (LS) character, updates original number
 */

PRIVATE
char btoa(ULONG_PTR *pn) {

    char    rch;

    rch = (char)(*pn&1)+'0';
    *pn >>= 1;
    return rch;
}

/***    otoa - return next (least significant) char in an octal to ASCII
 *              conversion
 *
 *      ENTRY   pn  - pointer to number to convert
 *
 *      EXIT    returns next (LS) character, updates original number
 */

PRIVATE
char otoa(ULONG_PTR *pn) {

    char    rch;

    rch = (char)'0'+(char)(*pn&7);
    *pn >>= 3;
    return rch;
}

/***    utoa - return next (least significant) char in an unsigned int to
 *              ASCII conversion
 *
 *      ENTRY   pn  - pointer to number to convert
 *
 *      EXIT    returns next (LS) character, updates original number
 */

PRIVATE
char utoa(ULONG_PTR *pn) {

    char    rch;

    rch = (char)'0'+(char)(*pn%10);
    *pn /= 10;
    return rch;
}

/***    xtoa - return next (least significant) char in a hexadecimal to
 *              ASCII conversion. Returns lower case hex characters
 *
 *      ENTRY   pn  - pointer to number to convert
 *
 *      EXIT    returns next (LS) character, updates original number
 */

PRIVATE
char xtoa(ULONG_PTR *pn) {

    ULONG_PTR   n = *pn & 0x000f;
    char        rch = (n <= 9) /* decimal digit? */
                      ? (char)(n+'0')
                      : (char)(n+'0'+('a'-'9'-1));

    *pn >>= 4;
    return rch;
}

/***    Xasc - return next (least significant) char in a hexadecimal to
 *              ASCII conversion. Returns upper case hex characters
 *
 *      ENTRY   pn  - pointer to number to convert
 *
 *      EXIT    returns next (LS) character, updates original number
 */

PRIVATE
char Xasc(ULONG_PTR *pn) {

    ULONG_PTR   n = *pn & 0x000f;
    char        rch = (n <= 9) /* decimal digit? */
                        ? (char)(n+'0')
                        : (char)(n+'0'+('A'-'9'-1));

    *pn >>= 4;
    return rch;
}

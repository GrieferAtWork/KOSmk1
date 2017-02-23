/* MIT License
 *
 * Copyright (c) 2017 GrieferAtWork
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef __FORMAT_PRINTER_H__
#define __FORMAT_PRINTER_H__ 1

#include <stdarg.h>
#include <kos/compiler.h>
#include <kos/types.h>

#ifndef __ASSEMBLY__
__DECL_BEGIN

/* Callback functions prototypes provided to format functions. */
typedef int (*pformatprinter) __P((char const *__restrict __data,
                                   __size_t __maxchars,
                                   void *__closure));
typedef int (*pformatscanner) __P((int *__restrict __ch, void *__closure));
typedef int (*pformatreturn) __P((int __ch, void *__closure));

//////////////////////////////////////////////////////////////////////////
// Generic printf implementation
// Taking a regular printf-style format string and arguments, these
// functions will call the given 'PRINTER' callback with various strings
// that, when put together, result in the desired formated text.
//  - 'PRINTER' obviously is called with the text parts in their correct order
//  - If 'PRINTER' returns something other than '0', the
//    function returns immediately, yielding that same value.
//  - The strings passed to 'PRINTER' may not necessarily be zero-terminated,
//    though the second argument 'maxchars' might be greater than the actual
//    size of the 'data' argument.
//    Therefor, the arguments should only be used in strn-style function calls,
//    with the real size of the given string being determinable through 'strnlen'
// Supported extensions:
//  - '%q'-format mode: Semantics equivalent to '%s', this modifier escapes the string using
//                      'format_quote' with flags set of 'FORMAT_QUOTE_FLAG_NONE', or
//                      'PRINTF_FLAG_PREFIX' when the '#' flag was used (e.g.: '%#q').
//  - '%I'   length modifier: Integral length equivalent to sizeof(size_t).
//  - '%I8'  length modifier: Integral length equivalent to sizeof(int8_t).
//  - '%I16' length modifier: Integral length equivalent to sizeof(int16_t).
//  - '%I32' length modifier: Integral length equivalent to sizeof(int32_t).
//  - '%I64' length modifier: Integral length equivalent to sizeof(int64_t).
// >>> Possible (and actual) uses:
//  - printf:           Unbuffered output into any kind of stream/file.
//  - sprintf/snprintf: Unsafe/Counted string formatting into a user-supplied buffer.
//  - strdupf:          Output into dynamically allocated heap memory,
//                      increasing the buffer when it gets filled completely.
//  - k_syslogf:        Unbuffered system-log output.
extern __nonnull((1,3)) __attribute_vaformat(__printf__,3,4) int
format_printf __P((pformatprinter __printer, void *__closure,
                   char const *__restrict __format, ...));
extern __nonnull((1,3)) __attribute_vaformat(__printf__,3,0) int
format_vprintf __P((pformatprinter __printer, void *__closure,
                    char const *__restrict __format, va_list __args));


//////////////////////////////////////////////////////////////////////////
// Generic scanf implementation
// Taking a regular scanf-style format string and argument, these
// functions will call the given 'SCANNER' function which in
// return should successively yield a character at a time from
// some kind of input source.
//  - If 'SCANNER' returns non-zero, scanning aborts and that value is returned.
//  - In addition, one of the error codes below may be returned,
//    indicating an unexpected character, or conversion error.
//  - The user may use 'SCANNER' to track the last read character to get
//    additional information about what character caused the scan to fail.
//  - The given 'SCANNER' should indicate EOF by setting '*ch' to a negative value
//  - This implementation supports the following extensions:
//    - '%[A-Z]'   -- Character ranges in scan patterns
//    - '%[^abc]'  -- Inversion of a scan pattern
//    - '\n'       -- Skip any kind of linefeed ('\n', '\r', '\r\n')
//    - '%$s'      -- '$'-modifier, available for any format outputing a string.
//                    This modifier reads a 'size_t' from the argument list,
//                    that specifies the size of the following string buffer:
//                 >> char buffer[64];
//                 >> sscanf(data,"My name is %$s\n",sizeof(buffer),buffer);
// format -> %[*][$][width][length]specifier
// @return: The number of parsed arguments, or an non-zero value returned by 'SCANNER' or 'RETURNCH'
extern __nonnull((1,4)) __attribute_vaformat(__scanf__,4,5) int
format_scanf __P((pformatscanner __scanner, pformatreturn __returnch,
                  void *__closure, char const *__restrict __format, ...));
extern __nonnull((1,4)) __attribute_vaformat(__scanf__,4,0) int
format_vscanf __P((pformatscanner __scanner, pformatreturn __returnch,
                   void *__closure, char const *__restrict __format, va_list __args));


//////////////////////////////////////////////////////////////////////////
// The format-generic version of strftime-style formatting.
// NOTE: Besides supported the standard, name extensions, as already supported
//       by deemon are supported as well (Documentation taken from deemon):
// WARNING: Not all extensions from deemon are implemented (no milliseconds, and no *s (years, mouths, etc.) are supported)
// NOTE: Since I really think that the format modifiers for strftime are just total garbage,
//       and only designed to be short, but not readable, I've added a new, extended modifier
//       that is meant to fix this:
//       >> format_strftime(...,"%[S:WD], the %[n:D] of %[S:M] %[Y], %[2:H]:%[2:MI]:%[2:S]",time(NULL));
//       -> Tuesday, the 8th of November 2016, 17:45:38
//       attribute ::= ('year'|'month'|'mweek'|'yweek'|'wday'|'mday'|
//                      'yday'|'hour'|'minute'|'second'|'msecond'|
//                      'Y'|'M'|'MW'|'YW'|'WD'|'MD'|'D'|'YD'|'H'|'MI'|'I'|'S'|'MS');
//       exftime ::= '%[' ## [('S'|'s'|(['n'|' '] ## ('0'..'9')##...)] ## ':')] ## attribute ## ']';
//       - The format modifier always starts with '%[' and ends with ']', with the actual text in-between
//       - Optional representation prefix (before ':'):
//         - 's': Short representation of the attribute (only allowed for 'wday' and 'month')
//         - 'S': Long representation of the attribute (only allowed for 'wday' and 'month')
//         - 'n': nth representation of the attribute (can be prefixed infront of width modifier; (1st, 2nd, 3rd, 4th, 5th, ...))
//         - ' ': Fill empty space created by a large width modifier with ' ' instead of '0'
//       - Optional width prefix (before ':'):
//         - Only allowed without a representation prefix, the 'n' or ' ' prefix.
//       - Attribute name:
//         - One of the names listed above, this part describes which attribute to refer to.
//         - The attributes match the member names of the time object, with the following aliases provided:
//           - 'Y'      --> 'year'
//           - 'M'      --> 'month'
//           - 'WD'     --> 'wday'
//           - 'MD'|'D' --> 'mday'
//           - 'YD'     --> 'yday'
//           - 'H'      --> 'hour'
//           - 'MI'|'I' --> 'minute'
//           - 'S'      --> 'second'
struct tm;
extern __nonnull((1,4)) __attribute_vaformat(__strftime__,3,4) int
format_strftime __P((pformatprinter __printer, void *__closure,
                     char const *__restrict __format, struct tm const *__tm));


//////////////////////////////////////////////////////////////////////////
// Do C-style quotation on the given text, printing
// all of its escaped portions to the given printer.
// Input:
// >> Hello "World" W
// >> hat a great day.
// Output #1: >> \"Hello \"World\" W\nhat a great day.\"
// Output #2: >> Hello \"World\" W\nhat a great day.
// NOTE: Output #2 is generated if the 'FORMAT_QUOTE_FLAG_PRINTRAW' is set.
// This function escapes all control and non-ascii characters,
// preferring octal encoding for control characters and hex-encoding
// for other non-ascii characters, a behavior that may be modified
// with the 'FORMAT_QUOTE_FLAG_FORCE*' flags.
// @param: PRINTER: A function called for all quoted portions of the text.
// @param: MAXTEXT: strnlen-style maxlen for the given TEXT,
//                  unless the 'FORMAT_QUOTE_FLAG_QUOTEALL' flag is
//                  set, in which case it is the exact amount of characters
//                  to quote from 'TEXT', including '\0' characters.
// @return: 0: The given text was successfully printed.
// @return: *: The first non-ZERO(0) return value of PRINTER.
extern __nonnull((1,3)) int
format_quote __P((pformatprinter __printer, void *__closure,
                  char const *__restrict __text, __size_t __maxtext,
                  __u32 __flags));
#define FORMAT_QUOTE_FLAG_NONE     0x00000000
#define FORMAT_QUOTE_FLAG_PRINTRAW 0x00000001 /*< Don't surround the quoted text with "..."; */
#define FORMAT_QUOTE_FLAG_FORCEHEX 0x00000002 /*< Force hex encoding of all control characters without special strings ('\n', etc.). */
#define FORMAT_QUOTE_FLAG_FORCEOCT 0x00000004 /*< Force octal encoding of all non-ascii characters. */
#define FORMAT_QUOTE_FLAG_NOCTRL   0x00000008 /*< Disable special encoding strings such as '\r', '\n' or '\e' */
#define FORMAT_QUOTE_FLAG_QUOTEALL 0x00000010 /*< MAXTEXT is the exact length of the given TEXT. */
#define FORMAT_QUOTE_FLAG_UPPERPRE 0x00000020 /*< Use uppercase characters for hex (e.g.: '\Xab'). */
#define FORMAT_QUOTE_FLAG_UPPERSUF 0x00000040 /*< Use uppercase characters for hex (e.g.: '\xAB'). */



struct stringprinter {
 char *sp_bufpos; /*< [1..1][>= sp_buffer][<= sp_bufend] . */
 char *sp_buffer; /*< [1..1] Allocate buffer base pointer. */
 char *sp_bufend; /*< [1..1] Buffer end (Pointer to currently allocated '\0'-character). */
};

//////////////////////////////////////////////////////////////////////////
// Helper functions for using any pformatprinter-style
// function to print into a dynamically allocated string.
// >> struct stringprinter printer; int error; char *text;
// >> if (stringprinter_init(&printer,0)) handle_error();
// >> if (format_printf(&stringprinter_print,&printer,"Hello %s","dynamic world")) {
// >>   stringprinter_quit(&printer);
// >>   handle_error();
// >> } else {
// >>   text = stringprinter_pack(&printer,NULL);
// >>   //stringprinter_quit(&printer); // No-op after pack has been called
// >> }
// >> ...
// >> free(text);
// @param: HINT: A hint as to how big the initial buffer should be allocated as (Pass ZERO if unknown).
extern              __nonnull((1)) int   stringprinter_init __P((struct stringprinter *__restrict __self, __size_t __hint));
extern __retnonnull __nonnull((1)) char *stringprinter_pack __P((struct stringprinter *__restrict __self, __size_t *__length));
extern              __nonnull((1)) void  stringprinter_quit __P((struct stringprinter *__restrict __self));
extern int stringprinter_print __P((char const *__restrict __data, __size_t __maxchars, void *__closure));
 



__DECL_END
#endif /* !__ASSEMBLY__ */


#endif /* !__FORMAT_PRINTER_H__ */

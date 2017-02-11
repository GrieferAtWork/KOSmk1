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

typedef int (*pformatprinter) __P((char const *__restrict __data,
                                   __size_t __maxchars,
                                   void *__restrict __closure));
typedef int (*pformatscanner) __P((int *__restrict __ch, void *__restrict __closure));
typedef int (*pformatreturn) __P((int __ch, void *__restrict __closure));

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
// >>> Possible (and actual) uses:
//  - printf:           Unbuffered output into any kind of stream/file.
//  - sprintf/snprintf: Unsafe/Counted string formatting into a user-supplied buffer.
//  - strdupf:          Output into dynamically allocated heap memory,
//                      increasing the buffer when it gets filled completely.
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
//         - One of the names listed above, this part describes which attribute to referr to.
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


__DECL_END
#endif /* !__ASSEMBLY__ */


#endif /* !__FORMAT_PRINTER_H__ */

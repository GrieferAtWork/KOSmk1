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
#ifndef __CTYPE_C__
#define __CTYPE_C__ 1

#include <ctype.h>
#include <kos/compiler.h>

__DECL_BEGIN

/* -- The character map. */
__public __u8 const __ctype_map[256] = {
 0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x11,0x11,0x11,0x11,0x11,0x01,0x01,
 0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
 0x18,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,
 0x2c,0x2c,0x2c,0x2c,0x2c,0x2c,0x2c,0x2c,0x2c,0x2c,0x0e,0x0e,0x0e,0x0e,0x0e,0x0e,
 0x0e,0x8c,0x8c,0x8c,0x8c,0x8c,0x8c,0x8c,0x8c,0x8c,0x8c,0x8c,0x8c,0x8c,0x8c,0x8c,
 0x8c,0x8c,0x8c,0x8c,0x8c,0x8c,0x8c,0x8c,0x8c,0x8c,0x8c,0x0c,0x0e,0x0e,0x0e,0xce,
 0x0e,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,
 0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x4c,0x0e,0x0e,0x0e,0x0e,0x01,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};

#ifdef ____ctype_tohex_defined
__public char const __ctype_tohex[2][16] = {
 {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'},
 {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'}};
#endif

#ifndef __CONFIG_MIN_LIBC__
/* Export all the functions again to allow for extern-bindings. */
__public int isalnum(int ch) { return (__ctype_map[ch&0xff]&(__F_IS_DIGIT|__F_IS_LOWER|__F_IS_UPPER))!=0; }
__public int isalpha(int ch) { return (__ctype_map[ch&0xff]&(__F_IS_LOWER|__F_IS_UPPER))!=0; }
__public int isblank(int ch) { return ch == '\t' || ch == ' '; }
__public int iscntrl(int ch) { return (__ctype_map[ch&0xff]&__F_IS_CNTRL)!=0; }
__public int isdigit(int ch) { return (__ctype_map[ch&0xff]&__F_IS_DIGIT)!=0; }
__public int isgraph(int ch) { return (__ctype_map[ch&0xff]&__F_IS_GRAPH)!=0; }
__public int islower(int ch) { return (__ctype_map[ch&0xff]&__F_IS_LOWER)!=0; }
__public int isprint(int ch) { return (__ctype_map[ch&0xff]&__F_IS_PRINT)!=0; }
__public int ispunct(int ch) { return (__ctype_map[ch&0xff]&__F_IS_PUNCT)!=0; }
__public int isspace(int ch) { return (__ctype_map[ch&0xff]&__F_IS_SPACE)!=0; }
__public int isupper(int ch) { return (__ctype_map[ch&0xff]&__F_IS_UPPER)!=0; }
__public int isxdigit(int ch) { return (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F'); }
__public int tolower(int ch) { return ch >= 'A' && ch <= 'Z' ? ch+('a'-'A') : ch; }
__public int toupper(int ch) { return ch >= 'a' && ch <= 'z' ? ch+('A'-'a') : ch; }
__public int _tolower(int ch) { __assertf(isupper(ch),"'%c' is not uppercase",ch); return ch+('a'-'A'); }
__public int _toupper(int ch) { __assertf(islower(ch),"'%c' is not lowercase",ch); return ch+('A'-'a'); }
#endif /* !__CONFIG_MIN_LIBC__ */

__DECL_END

#endif /* !__CTYPE_C__ */

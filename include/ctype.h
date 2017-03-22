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
#ifndef __CTYPE_H__
#ifndef _CTYPE_H
#ifndef _INC_CTYPE
#define __CTYPE_H__ 1
#define _CTYPE_H 1
#define _INC_CTYPE 1

#include <kos/compiler.h>

#ifndef __ASSEMBLY__
#include <kos/config.h>
#include <kos/types.h>
#ifdef __DEBUG__
#include <kos/assert.h>
#endif

#undef iscntrl
#undef ispunct
#undef isgraph
#undef isprint
#undef isspace
#undef isdigit
#undef islower
#undef isupper
#undef isalpha
#undef isalnum
#undef tolower
#undef toupper
#undef _tolower
#undef _toupper

__DECL_BEGIN

#ifndef ____ctype_map_defined
#define ____ctype_map_defined 1
extern __u8 const __ctype_map[256];
#endif

#ifndef ____ctype_tohex_defined
#define ____ctype_tohex_defined 1
extern char const __ctype_tohex[2][16];
#endif

#ifndef __STDC_PURE__
#ifdef __INTELLISENSE__
#define _tohex  tohex
#define _tohex2 tohex2
#else
#define tohex  _tohex
#define tohex2 _tohex2
#endif
#endif

#ifndef __F_IS_CNTRL
#define __F_IS_CNTRL 0x01
#define __F_IS_PUNCT 0x02
#define __F_IS_GRAPH 0x04
#define __F_IS_PRINT 0x08
#define __F_IS_SPACE 0x10
#define __F_IS_DIGIT 0x20
#define __F_IS_LOWER 0x40
#define __F_IS_UPPER 0x80
#endif

#ifndef __CTYPE_C__
/* Declare everything as a function to allow for their addresses to be taken */
__local __wunused __constcall int isalnum(int __ch)  { return (__ctype_map[__ch&0xff]&(__F_IS_DIGIT|__F_IS_LOWER|__F_IS_UPPER))!=0; }
__local __wunused __constcall int isalpha(int __ch)  { return (__ctype_map[__ch&0xff]&(__F_IS_LOWER|__F_IS_UPPER))!=0; }
__local __wunused __constcall int isblank(int __ch)  { return __ch == '\t' || __ch == ' '; }
__local __wunused __constcall int iscntrl(int __ch)  { return (__ctype_map[__ch&0xff]&__F_IS_CNTRL)!=0; }
__local __wunused __constcall int isdigit(int __ch)  { return (__ctype_map[__ch&0xff]&__F_IS_DIGIT)!=0; }
__local __wunused __constcall int isgraph(int __ch)  { return (__ctype_map[__ch&0xff]&__F_IS_GRAPH)!=0; }
__local __wunused __constcall int islower(int __ch)  { return (__ctype_map[__ch&0xff]&__F_IS_LOWER)!=0; }
__local __wunused __constcall int isprint(int __ch)  { return (__ctype_map[__ch&0xff]&__F_IS_PRINT)!=0; }
__local __wunused __constcall int ispunct(int __ch)  { return (__ctype_map[__ch&0xff]&__F_IS_PUNCT)!=0; }
__local __wunused __constcall int isspace(int __ch)  { return (__ctype_map[__ch&0xff]&__F_IS_SPACE)!=0; }
__local __wunused __constcall int isupper(int __ch)  { return (__ctype_map[__ch&0xff]&__F_IS_UPPER)!=0; }
__local __wunused __constcall int isxdigit(int __ch) { return (__ch >= 'a' && __ch <= 'f') || (__ch >= 'A' && __ch <= 'F'); }

#ifndef __CCTYPE__
#define isalnum(ch) ((__ctype_map[(ch)&0xff]&(__F_IS_DIGIT|__F_IS_LOWER|__F_IS_UPPER))!=0)
#define isalpha(ch) ((__ctype_map[(ch)&0xff]&(__F_IS_LOWER|__F_IS_UPPER))!=0)
#define iscntrl(ch) ((__ctype_map[(ch)&0xff]&__F_IS_CNTRL)!=0)
#define isdigit(ch) ((__ctype_map[(ch)&0xff]&__F_IS_DIGIT)!=0)
#define isgraph(ch) ((__ctype_map[(ch)&0xff]&__F_IS_GRAPH)!=0)
#define islower(ch) ((__ctype_map[(ch)&0xff]&__F_IS_LOWER)!=0)
#define isprint(ch) ((__ctype_map[(ch)&0xff]&__F_IS_PRINT)!=0)
#define ispunct(ch) ((__ctype_map[(ch)&0xff]&__F_IS_PUNCT)!=0)
#define isspace(ch) ((__ctype_map[(ch)&0xff]&__F_IS_SPACE)!=0)
#define isupper(ch) ((__ctype_map[(ch)&0xff]&__F_IS_UPPER)!=0)
#endif /* !__CCTYPE__ */

__local __wunused __constcall int tolower(int __ch) { return __ch >= 'A' && __ch <= 'Z' ? __ch+('a'-'A') : __ch; }
__local __wunused __constcall int toupper(int __ch) { return __ch >= 'a' && __ch <= 'z' ? __ch+('A'-'a') : __ch; }
#ifdef ____ctype_tohex_defined
__local __wunused __constcall char _tohex(int __uppercase, int __x) { __assertf(__x < 16,"%d is not a hex",__x); return __ctype_tohex[!!__uppercase][__x]; }
__local __wunused __constcall char _tohex2(int __uppercase, int __x) { return __ctype_tohex[!!__uppercase][__x&0xf]; }
#else
__local __wunused __constcall char _tohex(int __uppercase, int __x) { __assertf(__x < 16,"%d is not a hex",x); return __x >= 10 ? ((__uppercase ? 'A' : 'a')+(__x-10)) : '0'+__x; }
__local __wunused __constcall char _tohex2(int __uppercase, int __x) { return __x >= 10 ? ((__uppercase ? 'A' : 'a')+(__x-10)) : '0'+__x; }
#endif

#ifdef __INTELLISENSE__
extern char _tooct(int __x);
#ifndef __STDC_PURE__
extern char tooct(int __x);
#endif
#else
#define _tooct(x) ('0'+(x))
#ifndef __STDC_PURE__
#define tooct      _tooct
#endif
#endif

#ifdef __DEBUG__
__local __wunused __constcall int _tolower(int __ch) { __assertf(isupper(__ch),"'%c' is not uppercase",__ch); return __ch+('a'-'A'); }
__local __wunused __constcall int _toupper(int __ch) { __assertf(islower(__ch),"'%c' is not lowercase",__ch); return __ch+('A'-'a'); }
__forcelocal __wunused __constcall int __tolower_d(int __ch __LIBC_DEBUG__PARAMS) { __assert_atf("_tolower(...)",0,isupper(__ch),"'%c' is not uppercase",__ch); return __ch+('a'-'A'); }
__forcelocal __wunused __constcall int __toupper_d(int __ch __LIBC_DEBUG__PARAMS) { __assert_atf("_toupper(...)",0,islower(__ch),"'%c' is not lowercase",__ch); return __ch+('A'-'a'); }
__forcelocal __wunused __constcall int __tolower_x(int __ch __LIBC_DEBUG_X__PARAMS) { __assert_xatf("_tolower(...)",0,isupper(__ch),"'%c' is not uppercase",__ch); return __ch+('a'-'A'); }
__forcelocal __wunused __constcall int __toupper_x(int __ch __LIBC_DEBUG_X__PARAMS) { __assert_xatf("_toupper(...)",0,islower(__ch),"'%c' is not lowercase",__ch); return __ch+('A'-'a'); }
#ifdef __LIBC_DEBUG_X__ARGS
#define _tolower(ch) __tolower_x(ch __LIBC_DEBUG_X__ARGS)
#define _toupper(ch) __toupper_x(ch __LIBC_DEBUG_X__ARGS)
#else
#define _tolower(ch) __tolower_d(ch __LIBC_DEBUG__ARGS)
#define _toupper(ch) __toupper_d(ch __LIBC_DEBUG__ARGS)
#endif
#else
__local int _tolower(int __ch) { return __ch+('a'-'A'); }
__local int _toupper(int __ch) { return __ch+('A'-'a'); }
#ifndef __CCTYPE__
#define _tolower(__ch) ((__ch)+('a'-'A'))
#define _toupper(__ch) ((__ch)+('A'-'a'))
#endif /* !__CCTYPE__ */
#endif
#endif /* !__CTYPE_C__ */

#ifndef __INTELLISENSE__
#ifdef ____ctype_tohex_defined
#ifndef __DEBUG__
#define _tohex(__uppercase,__x)  (__ctype_tohex[!!(__uppercase)][(__x)])
#endif /* !__DEBUG__ */
#define _tohex2(__uppercase,__x) (__ctype_tohex[!!(__uppercase)][(__x)&0xf])
#endif /* ____ctype_tohex_defined */
#endif

__DECL_END
#endif /* !__ASSEMBLY__ */

#endif /* !_INC_CTYPE */
#endif /* !_CTYPE_H */
#endif /* !__CTYPE_H__ */

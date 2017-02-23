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
#ifndef __STDLIB_H__
#ifndef _STDLIB_H
#ifndef _INC_STDLIB
#define __STDLIB_H__ 1
#define _STDLIB_H 1
#define _INC_STDLIB 1

#include <kos/compiler.h>
#include <kos/config.h>
#include <malloc.h> /* Pull in malloc & friends... */
#include <__null.h>

__DECL_BEGIN

#ifndef __size_t_defined
#define __size_t_defined 1
typedef __size_t size_t;
#endif

#define	EXIT_FAILURE	1	/* Failing exit status. */
#define	EXIT_SUCCESS	0	/* Successful exit status. */

#ifdef __HAVE_ATEXIT
#ifndef __KERNEL__
/* Register functions to be called on exit (Not available in kernel mode). */
extern __nonnull((1)) int atexit __P((void (*__func)(void)));
extern __nonnull((1)) int on_exit __P((void (*__func)(int __exitcode, void *__closure), void *__closure));
#endif /* !__KERNEL__ */
#endif /* !__HAVE_ATEXIT */

#ifndef __NO_asmname
extern __noreturn void _Exit __P((int __exitcode)) __asmname("_exit");
#else /* !__NO_asmname */
#ifndef ___exit_defined
#define ___exit_defined 1
extern __noreturn void _exit __P((int __exitcode));
#endif /* !___exit_defined */
#define _Exit  _exit
#endif /* __NO_asmname */

extern __noreturn void exit __P((int __exitcode));
extern __noreturn __coldcall void abort __P((void));

// KOS extension: Same as calling '_Exit', but with the argument type
//                being the size of a pointer, you can pass bigger values on
//                64-bit architectures (Better integration with <kos/task.h>).
extern __noreturn void __Exit __P((__uintptr_t __exitcode));

#ifndef __CONFIG_MIN_LIBC__
extern __wunused __constcall int abs __P((int __n));
extern __wunused __constcall long labs __P((long __n));
#ifndef __NO_longlong
extern __wunused __constcall long long llabs __P((long long __n));
#endif /* !__NO_longlong */
#else
__local __wunused __constcall int abs __D1(int,__n) { return __n < 0 ? -__n : __n; }
__local __wunused __constcall long labs __D1(long,__n) { return __n < 0 ? -__n : __n; }
#ifndef __NO_longlong
__local __wunused __constcall long long llabs __D1(long long,__n) { return __n < 0 ? -__n : __n; }
#endif /* !__NO_longlong */
#endif

//////////////////////////////////////////////////////////////////////////
// Operate with environment variables.
// NOTE: KOS does support modifying environment variables,
//       though you should
extern __nonnull((1,2)) int setenv __P((char const *__restrict __name, char const *__value, int __overwrite));
extern __nonnull((1)) int unsetenv __P((char const *__restrict __name));
extern int clearenv __P((void));
extern __wunused __nonnull((1)) char *getenv __P((char const *__restrict __name));

//////////////////////////////////////////////////////////////////////////
// strn-style environment string routines.
extern __wunused __nonnull((1)) char *_getnenv __P((char const *__restrict __name, __size_t __maxname));
extern __wunused __nonnull((1,3)) int _setnenv __P((char const *__restrict __name, __size_t __maxname,
                                                     char const *__value, __size_t __maxvalue,
                                                     int __overwrite));
extern __wunused __nonnull((1)) int _unsetnenv __P((char const *__restrict __name, __size_t __maxname));

// NOTE: The given string will not be used in the
//       environment, but instead libc uses a copy of it.
//    >> The parameter isn't made constant for compatibility
extern __nonnull((1)) int putenv __P((char *__string));

extern __wunused int atoi __P((char const *__string));
extern __wunused long atol __P((char const *__string));
#ifndef __NO_longlong
extern __wunused long long atoll __P((char const *__string));
#endif /* !__NO_longlong */

extern __wunused int system __P((char const *__command));

#ifndef __CONFIG_MIN_LIBC__
extern __nonnull((1,4)) void qsort(void *__restrict base, __size_t num, __size_t size,
                                   int (*compar)(void const *p1, void const *p2));
#endif /* !__CONFIG_MIN_LIBC__ */

#ifndef __ATOI_C__ /* Work around symbol type errors (It gets ugly...) */
extern __wunused float strtof __P((char const *__string, char **__endptr));
extern __wunused double strtod __P((char const *__string, char **__endptr));
extern __wunused long double strtold __P((char const *__string, char **__endptr));
extern __wunused long strtol __P((char const *__string, char **__endptr, int base));
extern __wunused unsigned long strtoul __P((char const *__string, char **__endptr, int base));
#ifndef __NO_longlong
extern __wunused long long strtoll __P((char const *__string, char **__endptr, int base));
extern __wunused unsigned long long strtoull __P((char const *__string, char **__endptr, int base));
#endif /* !__NO_longlong */
#endif

// KOS libc extensions
extern __wunused __s32 _strtos32 __P((char const *__string, char **__endptr, int base));
extern __wunused __s64 _strtos64 __P((char const *__string, char **__endptr, int base));
extern __wunused __u32 _strtou32 __P((char const *__string, char **__endptr, int base));
extern __wunused __u64 _strtou64 __P((char const *__string, char **__endptr, int base));
extern __wunused __s32 _strntos32 __P((char const *__string, __size_t max_chars, char **__endptr, int base));
extern __wunused __s64 _strntos64 __P((char const *__string, __size_t max_chars, char **__endptr, int base));
extern __wunused __u32 _strntou32 __P((char const *__string, __size_t max_chars, char **__endptr, int base));
extern __wunused __u64 _strntou64 __P((char const *__string, __size_t max_chars, char **__endptr, int base));

#ifndef __STDC_PURE__
#ifndef __NO_asmname
extern __wunused __s32 strtos32 __P((char const *__string, char **__endptr, int base)) __asmname("_strtos32");
extern __wunused __s64 strtos64 __P((char const *__string, char **__endptr, int base)) __asmname("_strtos64");
extern __wunused __u32 strtou32 __P((char const *__string, char **__endptr, int base)) __asmname("_strtou32");
extern __wunused __u64 strtou64 __P((char const *__string, char **__endptr, int base)) __asmname("_strtou64");
extern __wunused __s32 strntos32 __P((char const *__string, __size_t max_chars, char **__endptr, int base)) __asmname("_strntos32");
extern __wunused __s64 strntos64 __P((char const *__string, __size_t max_chars, char **__endptr, int base)) __asmname("_strntos64");
extern __wunused __u32 strntou32 __P((char const *__string, __size_t max_chars, char **__endptr, int base)) __asmname("_strntou32");
extern __wunused __u64 strntou64 __P((char const *__string, __size_t max_chars, char **__endptr, int base)) __asmname("_strntou64");
#else
#define strtos32  _strtos32
#define strtos64  _strtos64
#define strtou32  _strtou32
#define strtou64  _strtou64
#define strntos32 _strntos32
#define strntos64 _strntos64
#define strntou32 _strntou32
#define strntou64 _strntou64
#endif
#endif

#ifndef __CONFIG_MIN_LIBC__
#ifndef __div_t_defined
#define __div_t_defined 1
typedef struct { int quot,rem; } div_t;
typedef struct { long quot,rem; } ldiv_t;
#ifndef __NO_longlong
typedef struct { long long quot,rem; } lldiv_t;
#endif /* !__NO_longlong */
#endif  /* _DIV_T_DEFINED */

extern __wunused __constcall div_t div(int __numer, int __denom);
extern __wunused __constcall ldiv_t ldiv(long __numer, long __denom);
#ifndef __NO_longlong
extern __wunused __constcall lldiv_t lldiv(long long __numer, long long __denom);
#endif /* !__NO_longlong */

#ifdef __cplusplus
extern "C++" {
__local __wunused __constcall long int abs(long int __n) { return labs(__n); }
__local __wunused __constcall ldiv_t div(long __numer, long __denom) { return ldiv(__numer,__denom); }
#ifndef __NO_longlong
__local __wunused __constcall long long abs(long long __n) { return llabs(__n); }
__local __wunused __constcall lldiv_t div(long long __numer, long long __denom) { return lldiv(__numer,__denom); }
#endif /* !__NO_longlong */
}
#endif
#endif /* !__CONFIG_MIN_LIBC__ */

__DECL_END

#ifndef __INTELLISENSE__
#ifdef __GNUC__
#define abs(n) \
 __xblock({ __typeof__(n) const __absn = (n);\
            __xreturn __absn < 0 ? -__absn : __absn;\
 })
#define labs(n)  abs(n)
#define llabs(n) abs(n)
#endif /* __GNUC__ */
#endif

#endif /* !_INC_STDLIB */
#endif /* !_STDLIB_H */
#endif /* !__STDLIB_H__ */

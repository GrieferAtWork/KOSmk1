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
#ifndef __STRINGS_H__
#ifndef _STRINGS_H
#define __STRINGS_H__ 1
#define _STRINGS_H 1

#include <kos/compiler.h>
#ifndef __ASSEMBLY__
#include <kos/config.h>
#include <kos/types.h>
#if !defined(__INTELLISENSE__)\
  && defined(__LIBC_USE_ARCH_OPTIMIZATIONS)
#include <kos/arch/string.h>
#endif

__DECL_BEGIN

#ifdef __INTELLISENSE__
extern __wunused __purecall __nonnull((1,2)) int bcmp __P((const void *__a, const void *__b, __size_t __n)) __asmname("memcmp");
extern __wunused __purecall __nonnull((1)) char *index __P((char const *__s, int __c)) __asmname("strchr");
extern __wunused __purecall __nonnull((1)) char *rindex __P((char const *__s, int __c)) __asmname("strrchr");
extern __purecall __nonnull((1,2)) int strcasecmp __P((char const *__s1, char const *__s2)) __asmname("_stricmp");
extern __purecall __nonnull((1,2)) int strncasecmp __P((char const *__s1, char const *__s2, __size_t __n)) __asmname("_strnicmp");
extern __nonnull((1,2)) void bcopy(const void *__src, void *__dst, __size_t __n); /*< memcpy. */
extern __nonnull((1)) void bzero(void *__s, __size_t __n); /* memset(0). */
extern __wunused __constcall int ffs(int __i);
#else
/* Directly link arch-optimized routines. */

#if defined(karch_memcmp) && defined(__LIBC_USE_ARCH_OPTIMIZATIONS)
#   define bcmp   karch_memcmp
#elif !defined(__NO_asmname)
extern __wunused __purecall __nonnull((1,2)) int bcmp __P((void const *__a, void const *__b, __size_t __bytes)) __asmname("memcmp");
#else
#   define bcmp   memcmp
#ifndef __memcmp_defined
#define __memcmp_defined 1
extern __wunused __purecall __nonnull((1,2)) int memcmp __P((void const *__a, void const *__b, __size_t __bytes));
#endif /* !__memcmp_defined */
#endif /* bcmp... */

#if defined(karch_strchr) && defined(__LIBC_USE_ARCH_OPTIMIZATIONS)
#   define index  karch_strchr
#elif !defined(__NO_asmname)
#ifdef __cplusplus
extern "C++" {
extern __wunused __purecall __nonnull((1)) char *index __P((char *__restrict __haystack, int __needle)) __asmname("strchr");
extern __wunused __purecall __nonnull((1)) char const *index __P((char const *__restrict __haystack, int __needle)) __asmname("strchr");
}
#else
extern __wunused __purecall __nonnull((1)) char *index __P((char const *__restrict __haystack, int __needle)) __asmname("strchr");
#endif
#else
#   define index  strchr
#ifndef __strchr_defined
#define __strchr_defined 1
extern __wunused __purecall __nonnull((1)) char *strchr __P((char const *__restrict __haystack, int __needle));
#endif /* !__strchr_defined */
#endif /* index... */

#if defined(karch_rstrchr) && defined(__LIBC_USE_ARCH_OPTIMIZATIONS)
#   define rindex  karch_rstrchr
#elif !defined(__NO_asmname)
#ifdef __cplusplus
extern "C++" {
extern __wunused __purecall __nonnull((1)) char *rindex __P((char *__restrict __haystack, int __needle)) __asmname("strrchr");
extern __wunused __purecall __nonnull((1)) char const *rindex __P((char const *__restrict __haystack, int __needle)) __asmname("strrchr");
}
#else
extern __wunused __purecall __nonnull((1)) char *rindex __P((char const *__restrict __haystack, int __needle)) __asmname("strrchr");
#endif
#else
#   define rindex  strrchr
#ifndef __strrchr_defined
#define __strrchr_defined 1
extern __wunused __purecall __nonnull((1)) char *strrchr __P((char const *__restrict __haystack, int __needle));
#endif /* !__strrchr_defined */
#endif /* index... */

#if defined(karch_stricmp) && defined(__LIBC_USE_ARCH_OPTIMIZATIONS)
#   define strcasecmp    karch_stricmp
#elif !defined(__NO_asmname)
extern __wunused __purecall __nonnull((1,2)) int strcasecmp __P((char const *a, char const *__b)) __asmname("_stricmp");
#else /* !__NO_asmname */
#   define strcasecmp    _stricmp
#ifndef ___stricmp_defined
#define ___stricmp_defined 1
extern __wunused __purecall __nonnull((1,2)) int _stricmp __P((char const *a, char const *__b));
#endif /* !___stricmp_defined */
#endif /* __NO_asmname */

#if defined(karch_strincmp) && defined(__LIBC_USE_ARCH_OPTIMIZATIONS)
#   define strncasecmp   karch_strincmp
#elif !defined(__NO_asmname)
extern __wunused __purecall __nonnull((1,2)) int strncasecmp __P((char const *a, char const *__b, __size_t __maxchars)) __asmname("_strincmp");
#else /* !__NO_asmname */
#   define strncasecmp   _strnicmp
#ifndef ___strincmp_defined
#define ___strincmp_defined 1
extern __wunused __purecall __nonnull((1,2)) int _strincmp __P((char const *a, char const *__b, __size_t __maxchars));
#endif /* !___strincmp_defined */
#endif /* __NO_asmname */

#if defined(karch_memcpy) && defined(__LIBC_USE_ARCH_OPTIMIZATIONS)
#   define bcopy(src,dst,n) (void)karch_memcpy(dst,src,n)
#else /* karch_memcpy && !DEBUG */
#   define bcopy(src,dst,n) (void)memcpy(dst,src,n)
#ifndef __memcpy_defined
#define __memcpy_defined 1
extern __retnonnull __nonnull((1,2)) void *
memcpy __P((void *__restrict __dst,
            void const *__restrict __src,
            __size_t __bytes));
#endif /* !__memcpy_defined */
#endif /* !karch_memcpy || DEBUG */

#if defined(karch_memset) && defined(__LIBC_USE_ARCH_OPTIMIZATIONS)
#   define bzero(s,n)       (void)karch_memset(s,0,n)
#else /* karch_memset && !DEBUG */
#   define bzero(s,n)       (void)memset(s,0,n)
#ifndef __memset_defined
#define __memset_defined 1
extern __retnonnull __nonnull((1)) void *
memset __P((void *__restrict __dst,
            int __byte, __size_t __bytes));
#endif /* !__memset_defined */
#endif /* !karch_memset || DEBUG */

#if defined(__GNUC__) || __has_builtin(__builtin_ffs)
#   define ffs   __builtin_ffs
#elif defined(karch_ffs)
#   define ffs   karch_ffs
#else
extern __wunused __constcall int ffs __P((int __i));
#endif

#endif /* !__INTELLISENSE__ */

__DECL_END


#endif /* !__ASSEMBLY__ */

#endif /* !_STRINGS_H */
#endif /* !__STRINGS_H__ */

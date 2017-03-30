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
#ifndef __STRING_OPTIMIZATIONS_H__
#define __STRING_OPTIMIZATIONS_H__ 1

#include <kos/arch/string.h>
#include <kos/compiler.h>
#include <kos/config.h>
#include <kos/types.h>
#include <string.h>

#ifndef __ASSEMBLY__
__DECL_BEGIN

#ifdef __GNUC__
/* Bring some gcc-specific compiler-optimizations into the game. */
#define __constant_strcmp        __builtin_strcmp
#define __constant_strncmp       __builtin_strncmp
#define __constant_strchr        __builtin_strchr
#define __constant_strrchr       __builtin_strrchr

/* All of these must be implemented as macros expanding their arguments a million times,
 * because otherwise GCC's __builtin_constant_p doesn't return true for compile-time strings.
 * >> This is due to a shortcoming in GCC that prevents constant_p in strings from
 *    propagating into inline functions the way it does for integral constants.
 * NOTE through, that these functions have carefully been written to not evaluate
 *      the value of their arguments more than once at runtime, and always evaluate
 *      it if it isn't known at compile-time, such as when having side-effects.
 *      >> This simply means that these are _always_ safe
 *         to use instead of their regular counterparts.
 *        (So long as you don't count an otherwise larger assembly as a side-effect). */
#define __optimized_strlen(s) \
 (__builtin_constant_p(s) ? __builtin_strlen(s) : (strlen)(s))
#define __optimized_strnlen_sc(s,maxlen) \
 __xblock({ __size_t const __slen = __builtin_strlen(s);\
            __size_t const __mlen = (maxlen);\
            __xreturn __slen < __mlen ? __slen : __mlen;\
 })
#define __optimized_strnlen(s,maxlen) \
 (__builtin_constant_p(s) \
  ? __optimized_strnlen_sc(s,maxlen)\
  : (__builtin_constant_p(maxlen)\
  ? (!(maxlen) ? 0\
  :  ((maxlen) == (__size_t)-1) ? (strlen)(s)\
  : (strnlen)(s,maxlen))\
  : (strnlen)(s,maxlen)))
#ifdef __arch_generic_memchr
#define __optimized_strchr(s,needle) \
 ((__builtin_constant_p(s) && __builtin_constant_p(needle))\
  ? __builtin_strchr(s,needle) : (char *)(strchr)(s,needle))
#define __optimized_strnchr(s,needle,maxlen) \
 (__builtin_constant_p(maxlen)\
  ? ((maxlen) == (__size_t)-1 ? (char *)(strchr)(s,needle)\
  :  (maxlen) == 0 ? NULL : (char *)(strnchr)(s,needle,maxlen))\
  : (char *)(strnchr)(s,needle,maxlen))
#else
#define __optimized_strchr(s,needle) \
 (__builtin_constant_p(s) ? (__builtin_constant_p(needle) ? __builtin_strchr(s,needle)\
  : (char *)arch_memchr(s,needle,__builtin_strlen(s)*sizeof(char)))\
  : (char *)(strchr)(s,needle))
#define __optimized_strnchr(s,needle,maxlen) \
 (__builtin_constant_p(s)\
  ? (char *)arch_memchr(s,needle,__optimized_strnlen(s,maxlen)*sizeof(char)) :\
  __builtin_constant_p(maxlen)\
  ? ((maxlen) == (__size_t)-1 ? (char *)(strchr)(s,needle)\
  :  (maxlen) == 0 ? NULL : (char *)(strnchr)(s,needle,maxlen))\
  : (char *)(strnchr)(s,needle,maxlen))
#endif
#ifdef __arch_generic_memrchr
#define __optimized_strrchr(s,needle) \
 ((__builtin_constant_p(s) && __builtin_constant_p(needle))\
  ? __builtin_strrchr(s,needle) : (char *)(strrchr)(s,needle))
#define __optimized_strnrchr(s,needle,maxlen) \
 (__builtin_constant_p(maxlen)\
  ? ((maxlen) == (__size_t)-1 ? (char *)(strrchr)(s,needle)\
  :  (maxlen) == 0 ? NULL : (char *)(strnrchr)(s,needle,maxlen))\
  : (char *)(strnrchr)(s,needle,maxlen))
#else
#define __optimized_strrchr(s,needle) \
 (__builtin_constant_p(s) ? (__builtin_constant_p(needle) ? __builtin_strrchr(s,needle)\
  : (char *)arch_memrchr(s,needle,__builtin_strlen(s)*sizeof(char)))\
  : (char *)(strrchr)(s,needle))
#define __optimized_strnrchr(s,needle,maxlen) \
 (__builtin_constant_p(s)\
  ? (char *)arch_memrchr(s,needle,__optimized_strnlen(s,maxlen)*sizeof(char)) :\
  __builtin_constant_p(maxlen)\
  ? ((maxlen) == (__size_t)-1 ? (char *)(strrchr)(s,needle)\
  :  (maxlen) == 0 ? NULL : (char *)(strnrchr)(s,needle,maxlen))\
  : (char *)(strnrchr)(s,needle,maxlen))
#endif

#ifdef __arch_generic_memcmp
#define __optimized_strcmp(a,b) \
 ((__builtin_constant_p(a) && __builtin_constant_p(b))\
  ? __builtin_strcmp(a,b) : (strcmp)(a,b))
#define __optimized_strncmp(a,b,n) \
 ((__builtin_constant_p(a) && __builtin_constant_p(b))\
  ? __builtin_strncmp(a,b,n) : (strncmp)(a,b,n))
#else
#define __optimized_strcmp(a,b) \
 (__builtin_constant_p(a)\
  ? (__builtin_constant_p(b) ? __builtin_strcmp(a,b)\
  : arch_memcmp(a,b,(__builtin_strlen(a)+1)*sizeof(char)))\
  : __builtin_constant_p(b)\
  ? arch_memcmp(a,b,(__builtin_strlen(b)+1)*sizeof(char))\
  : (strcmp)(a,b))
#define __optimized_strncmp(a,b,n) \
 (__builtin_constant_p(a)\
  ? (__builtin_constant_p(b) ? __builtin_strncmp(a,b,n)\
  : arch_memcmp(a,b,(__optimized_strnlen_sc(a,n)+1)*sizeof(char)))\
  : __builtin_constant_p(b)\
  ? arch_memcmp(a,b,(__optimized_strnlen_sc(b,n)+1)*sizeof(char))\
  : (strncmp)(a,b,n))
#endif

#endif /* __GNUC__... */

/* WARNING: These optimizations are kind-of intensive
 * (evaluating potentially long expressions _a_lot_of_times).
 * >> So be careful what you wish for before you enable these.
 *  - Compilation may take _much_ longer with them!
 * NOTE: Enabled by default for '-On | n >= 2' */
#if !defined(__INTELLISENSE__) && !defined(__STRING_C__) &&\
    (defined(__OPTIMIZE__) && (__OPTIMIZE__+0) >= 2)
#   undef strlen
#   undef strnlen
#   undef strchr
#   undef strnchr
#   undef strrchr
#   undef strnrchr
#   undef strcmp
#   undef strncmp
#   define strlen(s)       __optimized_strlen(s)
#   define strnlen(s,n)    __optimized_strnlen(s,n)
#   define strchr(s,b)     __optimized_strchr(s,b)
#   define strnchr(s,b,n)  __optimized_strnchr(s,b,n)
#   define strrchr(s,b)    __optimized_strrchr(s,b)
#   define strnrchr(s,b,n) __optimized_strnrchr(s,b,n)
#   define strcmp(a,b)     __optimized_strcmp(a,b)
#   define strncmp(a,b,n)  __optimized_strncmp(a,b,n)
#endif /* ... */

__DECL_END
#endif /* !__ASSEMBLY__ */

#endif /* !__STRING_OPTIMIZATIONS_H__ */

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
#ifndef __STDINT_H__
#ifndef _STDINT_H
#ifndef _INC_STDINT
#define __STDINT_H__ 1
#define _STDINT_H 1
#define _INC_STDINT 1

#include <kos/compiler.h>
#include <kos/types.h>
#include <kos/arch.h>

__DECL_BEGIN

#ifndef __ASSEMBLY__

#ifndef __int_fast8_t_defined
#define __int_fast8_t_defined 1
typedef __s8   int_fast8_t;
typedef __u8  uint_fast8_t;
typedef __s16  int_fast16_t;
typedef __u16 uint_fast16_t;
typedef __s32  int_fast32_t;
typedef __u32 uint_fast32_t;
typedef __s64  int_fast64_t;
typedef __u64 uint_fast64_t;
#endif

#ifndef __int_least8_t_defined
#define __int_least8_t_defined 1
typedef __s8   int_least8_t;
typedef __u8  uint_least8_t;
typedef __s16  int_least16_t;
typedef __u16 uint_least16_t;
typedef __s32  int_least32_t;
typedef __u32 uint_least32_t;
typedef __s64  int_least64_t;
typedef __u64 uint_least64_t;
#endif

#ifndef __int8_t_defined
#define __int8_t_defined 1
typedef __s8   int8_t;
typedef __s16  int16_t;
typedef __s32  int32_t;
typedef __s64  int64_t;
#endif

#ifndef __uint8_t_defined
#define __uint8_t_defined 1
typedef __u8  uint8_t;
typedef __u16 uint16_t;
typedef __u32 uint32_t;
typedef __u64 uint64_t;
#endif

#ifndef __intptr_t_defined
#define __intptr_t_defined 1
typedef __intptr_t  intptr_t;
#endif

#ifndef __uintptr_t_defined
#define __uintptr_t_defined 1
typedef __uintptr_t uintptr_t;
#endif

#ifndef __intmax_t_defined
#define __intmax_t_defined 1
typedef __intmax_t   intmax_t;
typedef __uintmax_t uintmax_t;
#endif

#endif /* !__ASSEMBLY__ */

#ifdef __ASSEMBLY__
#define INT8_C(c)    c
#define INT16_C(c)   c
#define INT32_C(c)   c
#define INT64_C(c)   c
#define UINT8_C(c)   c
#define UINT16_C(c)  c
#define UINT32_C(c)  c
#define UINT64_C(c)  c
#define INTMAX_C(c)  c
#define UINTMAX_C(c) c
#else
#ifdef __INT8_C
#   define INT8_C    __INT8_C
#else
#   define INT8_C(c) c
#endif

#ifdef __INT16_C
#   define INT16_C    __INT16_C
#else
#   define INT16_C(c) c
#endif

#ifdef __INT32_C
#   define INT32_C    __INT32_C
#else
#   define INT32_C(c) c
#endif

#ifdef __INT64_C
#   define INT64_C    __INT64_C
#else
#   define INT64_C(c) c##ll
#endif

#ifdef __UINT8_C
#   define UINT8_C    __UINT8_C
#else
#   define UINT8_C(c) c##u
#endif

#ifdef __UINT16_C
#   define UINT16_C    __UINT16_C
#else
#   define UINT16_C(c) c##u
#endif

#ifdef __UINT32_C
#   define UINT32_C    __UINT32_C
#else
#   define UINT32_C(c) c##u
#endif

#ifdef __UINT64_C
#   define UINT64_C    __UINT64_C
#else
#   define UINT64_C(c) c##ull
#endif

#ifdef __INTMAX_C
#   define INTMAX_C    __INTMAX_C
#else
#   define INTMAX_C    INT64_C
#endif

#ifdef __UINTMAX_C
#   define UINTMAX_C    __UINTMAX_C
#else
#   define UINTMAX_C    UINT64_C
#endif
#endif


#ifndef INT8_MIN
#   define INT8_MIN     (-INT8_C(127)-INT8_C(1))
#   define INT16_MIN   (-INT16_C(32767)-INT16_C(1))
#   define INT32_MIN   (-INT32_C(2147483647)-INT32_C(1))
#   define INT64_MIN   (-INT64_C(9223372036854775807)-INT64_C(1))
#   define INT8_MAX       INT8_C(127)
#   define INT16_MAX     INT16_C(32767)
#   define INT32_MAX     INT32_C(2147483647)
#   define INT64_MAX     INT64_C(9223372036854775807)
#   define UINT8_MAX     UINT8_C(255)
#   define UINT16_MAX   UINT16_C(65535)
#   define UINT32_MAX   UINT32_C(4294967295)
#   define UINT64_MAX   UINT64_C(18446744073709551615)
#endif

#if __SIZEOF_POINTER__ == 8
#   define   SIZE_MAX UINT64_MAX
#   define _SSIZE_MIN  INT64_MIN
#   define _SSIZE_MAX  INT64_MAX
#else
#   define   SIZE_MAX UINT32_MAX
#   define _SSIZE_MIN  INT32_MIN
#   define _SSIZE_MAX  INT32_MAX
#endif

#define INT_LEAST8_MIN    INT8_MIN
#define INT_LEAST16_MIN   INT16_MIN
#define INT_LEAST32_MIN   INT32_MIN
#define INT_LEAST64_MIN   INT64_MIN
#define INT_LEAST8_MAX    INT8_MAX
#define INT_LEAST16_MAX   INT16_MAX
#define INT_LEAST32_MAX   INT32_MAX
#define INT_LEAST64_MAX   INT64_MAX
#define UINT_LEAST8_MAX   UINT8_MAX
#define UINT_LEAST16_MAX  UINT16_MAX
#define UINT_LEAST32_MAX  UINT32_MAX
#define UINT_LEAST64_MAX  UINT64_MAX

#define INT_FAST8_MIN     INT8_MIN
#define INT_FAST16_MIN    INT16_MIN
#define INT_FAST32_MIN    INT32_MIN
#define INT_FAST64_MIN    INT64_MIN
#define INT_FAST8_MAX     INT8_MAX
#define INT_FAST16_MAX    INT16_MAX
#define INT_FAST32_MAX    INT32_MAX
#define INT_FAST64_MAX    INT64_MAX
#define UINT_FAST8_MAX    UINT8_MAX
#define UINT_FAST16_MAX   UINT16_MAX
#define UINT_FAST32_MAX   UINT32_MAX
#define UINT_FAST64_MAX   UINT64_MAX

#define INTPTR_MIN   _SSIZE_MIN
#define INTPTR_MAX   _SSIZE_MAX
#define UINTPTR_MAX    SIZE_MAX

#define PTRDIFF_MIN  _SSIZE_MIN
#define PTRDIFF_MAX  _SSIZE_MAX

#define INTMAX_MIN   INT64_MIN
#define INTMAX_MAX   INT64_MAX
#define UINTMAX_MAX UINT64_MAX

// TODO...
//#define WCHAR_MIN    0
//#define WCHAR_MAX    UINT16_MAX
//#define WINT_MIN     0
//#define WINT_MAX     UINT16_MAX

//#define SIG_ATOMIC_MIN    INT32_MIN
//#define SIG_ATOMIC_MAX    INT32_MAX

#ifndef __STDC_PURE__
#define SSIZE_MIN  _SSIZE_MIN
#define SSIZE_MAX  _SSIZE_MAX
#endif


__DECL_END

#endif /* !_INC_STDINT */
#endif /* !_STDINT_H */
#endif /* !__STDINT_H__ */

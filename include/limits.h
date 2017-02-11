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
#ifndef __LIMITS_H__
#ifndef _LIMITS_H
#ifndef _INC_LIMITS
#define __LIMITS_H__ 1
#define _LIMITS_H 1
#define _INC_LIMITS 1

#include <kos/compiler.h>

__DECL_BEGIN

#define __INT8_MIN    (-127-1)
#define __INT8_MAX      127
#define __UINT8_MAX     255u
#define __INT16_MIN   (-32767-1)
#define __INT16_MAX     32767
#define __UINT16_MAX    65535u
#define __INT32_MIN   (-2147483647-1)
#define __INT32_MAX     2147483647
#define __UINT32_MAX    4294967295u
#define __INT64_MIN   (-9223372036854775807ll-1)
#define __INT64_MAX     9223372036854775807ll
#define __UINT64_MAX    18446744073709551615ull

#ifdef __CHAR_BIT__
#   define CHAR_BIT   __CHAR_BIT__
#else
#   define CHAR_BIT   8
#endif

#ifdef __SCHAR_MIN__
#   define SCHAR_MIN  __SCHAR_MIN__
#else
#   define SCHAR_MIN  __INT8_MIN
#endif

#ifdef __SCHAR_MAX__
#   define SCHAR_MAX  __SCHAR_MAX__
#else
#   define SCHAR_MAX  __INT8_MAX
#endif

#ifdef __UCHAR_MAX__
#   define UCHAR_MAX  __UCHAR_MAX__
#else
#   define UCHAR_MAX  __UINT8_MAX
#endif

#ifdef __CHAR_MIN__
#   define CHAR_MIN   __CHAR_MIN__
#elif defined(__CHAR_UNSIGNED__)
#   define CHAR_MIN   0
#else
#   define CHAR_MIN   SCHAR_MIN
#endif

#ifdef __CHAR_MAX__
#   define CHAR_MAX   __CHAR_MAX__
#elif defined(__CHAR_UNSIGNED__)
#   define CHAR_MAX   UCHAR_MAX
#else
#   define CHAR_MAX   SCHAR_MAX
#endif

//#define MB_LEN_MAX 1 // far, far, far, far, far, far away...

#ifdef __SHRT_MIN__
#   define SHRT_MIN   __SHRT_MIN__
#else
#   define SHRT_MIN   __INT16_MIN
#endif

#ifdef __SHRT_MAX__
#   define SHRT_MAX   __SHRT_MAX__
#else
#   define SHRT_MAX   __INT16_MAX
#endif

#ifdef __USHRT_MAX__
#   define USHRT_MAX  __USHRT_MAX__
#else
#   define USHRT_MAX  __UINT16_MAX
#endif

#ifdef __INT_MIN__
#   define INT_MIN    __INT_MIN__
#else
#   define INT_MIN    __INT32_MIN
#endif

#ifdef __INT_MAX__
#   define INT_MAX    __INT_MAX__
#else
#   define INT_MAX    __INT32_MAX
#endif

#ifdef __UINT_MAX__
#   define UINT_MAX   __UINT_MAX__
#else
#   define UINT_MAX   __UINT32_MAX
#endif

#ifdef __LONG_MIN__
#   define LONG_MIN   __LONG_MIN__
#elif defined(__i386__) || \
     (defined(__SIZEOF_LONG__) && __SIZEOF_LONG__ == 4)
#   define LONG_MIN   __INT32_MIN
#else
#   define LONG_MIN   __INT64_MIN
#endif

#ifdef __LONG_MAX__
#   define LONG_MAX   __LONG_MAX__
#elif defined(__i386__) || \
     (defined(__SIZEOF_LONG__) && __SIZEOF_LONG__ == 4)
#   define LONG_MAX   __INT32_MAX
#else
#   define LONG_MAX   __INT64_MAX
#endif

#ifdef __ULONG_MAX__
#   define ULONG_MAX  __ULONG_MAX__
#elif defined(__i386__) || \
     (defined(__SIZEOF_LONG__) && __SIZEOF_LONG__ == 4)
#   define ULONG_MAX  __UINT32_MAX
#else
#   define ULONG_MAX  __UINT64_MAX
#endif

#ifdef __LLONG_MIN__
#   define LLONG_MIN  __LLONG_MIN__
#else
#   define LLONG_MIN  __INT64_MIN
#endif

#ifdef __LLONG_MAX__
#   define LLONG_MAX  __LLONG_MAX__
#else
#   define LLONG_MAX  __INT64_MAX
#endif

#ifdef __ULLONG_MAX__
#   define ULLONG_MAX __ULLONG_MAX__
#else
#   define ULLONG_MAX __UINT64_MAX
#endif


#ifndef __STDC_PURE__
#ifndef INT8_MIN /* These are actually defined in <stdint.h> */
#define INT8_MIN    __INT8_MIN
#define INT8_MAX    __INT8_MAX
#define UINT8_MAX   __UINT8_MAX
#define INT16_MIN   __INT16_MIN
#define INT16_MAX   __INT16_MAX
#define UINT16_MAX  __UINT16_MAX
#define INT32_MIN   __INT32_MIN
#define INT32_MAX   __INT32_MAX
#define UINT32_MAX  __UINT32_MAX
#define INT64_MIN   __INT64_MIN
#define INT64_MAX   __INT64_MAX
#define UINT64_MAX  __UINT64_MAX
#endif
#endif /* !__STDC_PURE__ */



#undef PATH_MAX
#define PATH_MAX 256 /*< Not ~really~ a limit, but used as buffer size hint in many places. */


__DECL_END

#endif /* !_INC_LIMITS */
#endif /* !_LIMITS_H */
#endif /* !__LIMITS_H__ */

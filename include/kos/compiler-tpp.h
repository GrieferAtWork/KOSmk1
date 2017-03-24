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

#ifndef __compiler_pragma
#define __PRIVATE_COMPILER_PRAGMA(x) _Pragma(#x)
#define __compiler_pragma(x) __PRIVATE_COMPILER_PRAGMA(x)
#endif

#ifdef __DEEMON__
/* Using deemon as preprocessor. */
#pragma warning(disable: 1) /* Redefining builtin keyword. */

//#undef __DEEMON__ /* Keep this in case we need to detect this again... */
#undef __DEEMON_API__
#undef __DEEMON_REVISION__

/* Get rid of deemon's predefined macros. */
#undef _INTEGRAL_MAX_BITS
#undef __CHAR_MIN__
#undef __CHAR_MAX__
#undef __SCHAR_MIN__
#undef __SCHAR_MAX__
#undef __UCHAR_MAX__
#undef __SHORT_MIN__
#undef __SHORT_MAX__
#undef __USHORT_MAX__
#undef __INT_MIN__
#undef __INT_MAX__
#undef __UINT_MAX__
#undef __LONG_MIN__
#undef __LONG_MAX__
#undef __ULONG_MAX__
#undef __LLONG_MIN__
#undef __LLONG_MAX__
#undef __ULLONG_MAX__
#undef __SIZEOF_CHAR__
#undef __SIZEOF_WCHAR__
#undef __SIZEOF_CHAR16__
#undef __SIZEOF_CHAR32__
#undef __SIZEOF_BOOL__
#undef __SIZEOF_SHORT__
#undef __SIZEOF_INT__
#undef __SIZEOF_LONG__
#undef __SIZEOF_LONG_LONG__
#undef __SIZEOF_LLONG__
#undef __SIZEOF_POINTER__
#undef __SIZEOF_INTMAX__
#undef __SIZEOF_INT_LEAST8__
#undef __SIZEOF_INT_LEAST16__
#undef __SIZEOF_INT_LEAST32__
#undef __SIZEOF_INT_LEAST64__
#undef __SIZEOF_INT_FAST8__
#undef __SIZEOF_INT_FAST16__
#undef __SIZEOF_INT_FAST32__
#undef __SIZEOF_INT_FAST64__
#undef __SIZEOF_TIME_T__
#undef __INT8_TYPE__
#undef __INT8_MIN__
#undef __INT8_MAX__
#undef __INT16_TYPE__
#undef __INT16_MIN__
#undef __INT16_MAX__
#undef __INT32_TYPE__
#undef __INT32_MIN__
#undef __INT32_MAX__
#undef __INT64_TYPE__
#undef __INT64_MIN__
#undef __INT64_MAX__
#undef __UINT8_TYPE__
#undef __UINT8_MIN__
#undef __UINT8_MAX__
#undef __UINT16_TYPE__
#undef __UINT16_MIN__
#undef __UINT16_MAX__
#undef __UINT32_TYPE__
#undef __UINT32_MIN__
#undef __UINT32_MAX__
#undef __UINT64_TYPE__
#undef __UINT64_MIN__
#undef __UINT64_MAX__
#undef __INT_LEAST8_TYPE__
#undef __INT_LEAST8_MIN__
#undef __INT_LEAST8_MAX__
#undef __INT_LEAST16_TYPE__
#undef __INT_LEAST16_MIN__
#undef __INT_LEAST16_MAX__
#undef __INT_LEAST32_TYPE__
#undef __INT_LEAST32_MIN__
#undef __INT_LEAST32_MAX__
#undef __INT_LEAST64_TYPE__
#undef __INT_LEAST64_MIN__
#undef __INT_LEAST64_MAX__
#undef __UINT_LEAST8_TYPE__
#undef __UINT_LEAST8_MIN__
#undef __UINT_LEAST8_MAX__
#undef __UINT_LEAST16_TYPE__
#undef __UINT_LEAST16_MIN__
#undef __UINT_LEAST16_MAX__
#undef __UINT_LEAST32_TYPE__
#undef __UINT_LEAST32_MIN__
#undef __UINT_LEAST32_MAX__
#undef __UINT_LEAST64_TYPE__
#undef __UINT_LEAST64_MIN__
#undef __UINT_LEAST64_MAX__
#undef __INT_FAST8_TYPE__
#undef __INT_FAST8_MIN__
#undef __INT_FAST8_MAX__
#undef __INT_FAST16_TYPE__
#undef __INT_FAST16_MIN__
#undef __INT_FAST16_MAX__
#undef __INT_FAST32_TYPE__
#undef __INT_FAST32_MIN__
#undef __INT_FAST32_MAX__
#undef __INT_FAST64_TYPE__
#undef __INT_FAST64_MIN__
#undef __INT_FAST64_MAX__
#undef __UINT_FAST8_TYPE__
#undef __UINT_FAST8_MIN__
#undef __UINT_FAST8_MAX__
#undef __UINT_FAST16_TYPE__
#undef __UINT_FAST16_MIN__
#undef __UINT_FAST16_MAX__
#undef __UINT_FAST32_TYPE__
#undef __UINT_FAST32_MIN__
#undef __UINT_FAST32_MAX__
#undef __UINT_FAST64_TYPE__
#undef __UINT_FAST64_MIN__
#undef __UINT_FAST64_MAX__
#undef __INTMAX_TYPE__
#undef __INTMAX_MIN__
#undef __INTMAX_MAX__
#undef __UINTMAX_TYPE__
#undef __UINTMAX_MIN__
#undef __UINTMAX_MAX__
#undef __INTPTR_TYPE__
#undef __INTPTR_MIN__
#undef __INTPTR_MAX__
#undef __UINTPTR_TYPE__
#undef __UINTPTR_MIN__
#undef __UINTPTR_MAX__
#undef __PTRDIFF_TYPE__
#undef __PTRDIFF_MIN__
#undef __PTRDIFF_MAX__
#undef __SIZE_TYPE__
#undef __SIZE_MIN__
#undef __SIZE_MAX__
#undef __WCHAR_MIN__
#undef __WCHAR_MAX__
#undef __CHAR16_MIN__
#undef __CHAR16_MAX__
#undef __CHAR32_MIN__
#undef __CHAR32_MAX__
#undef __SIZEOF_FLOAT__
#undef __SIZEOF_DOUBLE__
#undef __SIZEOF_LONG_DOUBLE__
#undef __FLT_RADIX__
#undef __FLT_ROUNDS__
#undef __FLT_DIG__
#undef __FLT_MANT_DIG__
#undef __FLT_MAX__
#undef __FLT_MIN__
#undef __FLT_MIN_EXP__
#undef __FLT_MIN_10_EXP__
#undef __FLT_MAX_EXP__
#undef __FLT_MAX_10_EXP__
#undef __FLT_EPSILON__
#undef __DBL_DIG__
#undef __DBL_MANT_DIG__
#undef __DBL_MAX__
#undef __DBL_MIN__
#undef __DBL_MIN_EXP__
#undef __DBL_MIN_10_EXP__
#undef __DBL_MAX_EXP__
#undef __DBL_MAX_10_EXP__
#undef __DBL_EPSILON__
#undef __LDBL_DIG__
#undef __LDBL_MANT_DIG__
#undef __LDBL_MAX__
#undef __LDBL_MIN__
#undef __LDBL_MIN_EXP__
#undef __LDBL_MIN_10_EXP__
#undef __LDBL_MAX_EXP__
#undef __LDBL_MAX_10_EXP__
#undef __LDBL_EPSILON__
#undef __SIZEOF_UID_T__
#undef __SIZEOF_GID_T__
#undef __SIZEOF_PID_T__

#include "compiler-predef.h"

#define __signed     signed
#define __unsigned   unsigned
#define __signed__   signed
#define __unsigned__ unsigned
#endif


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

#ifndef __PP_VA_NARGS
/* TPP Has its own way of implementing what gcc does as ',##__VA_ARGS__'.
 * NOTE: This implementation is capable of detecting 0 arguments. */
#define __PP_PRIVATE_VA_NARGS(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56,_57,_58,_59,N,...) N
#define __PP_VA_NARGS(...) __PP_PRIVATE_VA_NARGS(__VA_ARGS__ __VA_COMMA__ 59,58,57,56,55,54,53,52,51,50,49,48,47,46,45,44,43,42,41,40,39,38,37,36,35,34,33,32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0)
#endif /* !__PP_VA_NARGS */

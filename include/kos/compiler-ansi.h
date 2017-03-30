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

#ifndef __P
#define __P(x) x
#endif

/*[[[deemon
#include <util>
for (local x: util::range(16)) {
	print "#define __D"+x+"(",;
	for (local y: util::range(x)) print (y ? "," : "")+"type"+(y+1)+",arg"+(y+1),;
	print ") (",;
	if (!x) print "void",;
	for (local y: util::range(x)) print (y ? ", " : "")+"type"+(y+1)+" arg"+(y+1),;
	print ")";
}
]]]*/
#define __D0() (void)
#define __D1(type1,arg1) (type1 arg1)
#define __D2(type1,arg1,type2,arg2) (type1 arg1, type2 arg2)
#define __D3(type1,arg1,type2,arg2,type3,arg3) (type1 arg1, type2 arg2, type3 arg3)
#define __D4(type1,arg1,type2,arg2,type3,arg3,type4,arg4) (type1 arg1, type2 arg2, type3 arg3, type4 arg4)
#define __D5(type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5) (type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5)
#define __D6(type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6) (type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6)
#define __D7(type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6,type7,arg7) (type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6, type7 arg7)
#define __D8(type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6,type7,arg7,type8,arg8) (type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6, type7 arg7, type8 arg8)
#define __D9(type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6,type7,arg7,type8,arg8,type9,arg9) (type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6, type7 arg7, type8 arg8, type9 arg9)
#define __D10(type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6,type7,arg7,type8,arg8,type9,arg9,type10,arg10) (type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6, type7 arg7, type8 arg8, type9 arg9, type10 arg10)
#define __D11(type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6,type7,arg7,type8,arg8,type9,arg9,type10,arg10,type11,arg11) (type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6, type7 arg7, type8 arg8, type9 arg9, type10 arg10, type11 arg11)
#define __D12(type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6,type7,arg7,type8,arg8,type9,arg9,type10,arg10,type11,arg11,type12,arg12) (type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6, type7 arg7, type8 arg8, type9 arg9, type10 arg10, type11 arg11, type12 arg12)
#define __D13(type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6,type7,arg7,type8,arg8,type9,arg9,type10,arg10,type11,arg11,type12,arg12,type13,arg13) (type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6, type7 arg7, type8 arg8, type9 arg9, type10 arg10, type11 arg11, type12 arg12, type13 arg13)
#define __D14(type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6,type7,arg7,type8,arg8,type9,arg9,type10,arg10,type11,arg11,type12,arg12,type13,arg13,type14,arg14) (type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6, type7 arg7, type8 arg8, type9 arg9, type10 arg10, type11 arg11, type12 arg12, type13 arg13, type14 arg14)
#define __D15(type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6,type7,arg7,type8,arg8,type9,arg9,type10,arg10,type11,arg11,type12,arg12,type13,arg13,type14,arg14,type15,arg15) (type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6, type7 arg7, type8 arg8, type9 arg9, type10 arg10, type11 arg11, type12 arg12, type13 arg13, type14 arg14, type15 arg15)
//[[[end]]]

#define __PRIVATE_PP_CAT_3(a,b,c) a##b##c
#define __PRIVATE_PP_CAT_2(a,b)   a##b
#define __PP_CAT_3(a,b,c)         __PRIVATE_PP_CAT_3(a,b,c)
#define __PP_CAT_2(a,b)           __PRIVATE_PP_CAT_2(a,b)
#define __PP_STR2(x)              #x
#define __PP_STR(x)               __PP_STR2(x)

#ifndef __PP_MUL8
#   define __PRIVATE_PP_MUL8_1  8
#   define __PRIVATE_PP_MUL8_2  16
#   define __PRIVATE_PP_MUL8_4  32
#   define __PRIVATE_PP_MUL8_8  64
#   define __PRIVATE_PP_MUL8(x) __PRIVATE_PP_MUL8_##x
#   define __PP_MUL8(x)         __PRIVATE_PP_MUL8(x)
#endif /* !__PP_MUL8 */

#if defined(_MSC_VER) || defined(__INTELLISENSE__)
# define PP_PRIVATE_VA_NARGS_I(x,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56,_57,_58,_59,N,...) N
# define PP_PRIVATE_VA_NARGS_X(...) (~,__VA_ARGS__,59,58,57,56,55,54,53,52,51,50,49,48,47,46,45,44,43,42,41,40,39,38,37,36,35,34,33,32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0)
# define PP_VA_NARGS(...) PP_FORCE_EXPAND_1(PP_PRIVATE_VA_NARGS_I PP_PRIVATE_VA_NARGS_X(__VA_ARGS__))
#elif defined(__TPP_VERSION__) && __TPP_VERSION__ >= 103
#elif defined(__GNUC__)
#else
#endif

#ifndef __PP_VA_NARGS
/* Implement va_nargs using a standard-compliant preprocessor.
 * NOTE: This implementation is _NOT_ capable of detecting 0 arguments. */
#define __PP_FORCE_EXPAND(...) __VA_ARGS__
#define __PP_PRIVATE_VA_NARGS2(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56,_57,_58,_59,N,...) N
#define __PP_PRIVATE_VA_NARGS(...) __PP_PRIVATE_VA_NARGS2(__VA_ARGS__,59,58,57,56,55,54,53,52,51,50,49,48,47,46,45,44,43,42,41,40,39,38,37,36,35,34,33,32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1)
#define __PP_VA_NARGS(...) __PP_FORCE_EXPAND(__PP_PRIVATE_VA_NARGS(__VA_ARGS__))
#endif /* !__PP_VA_NARGS */

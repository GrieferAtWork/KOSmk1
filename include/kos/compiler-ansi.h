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

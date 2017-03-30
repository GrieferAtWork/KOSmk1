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

#ifndef __PE__
#error "Microsoft compilers can only be used in PE-mode. Make sure to pass -D__PE__ on the command line"
#endif

#ifndef __compiler_FUNCTION
#define __compiler_FUNCTION()   __FUNCTION__
#endif
#ifndef __export
#define __export      __declspec(dllexport)
#endif
#ifndef __import
#define __import      __declspec(dllimport)
#endif
#ifndef __noreturn
#define __noreturn    __declspec(noreturn)
#endif
#ifndef __noinline
#define __noinline    __declspec(noinline)
#endif
#ifndef __constcall
#define __constcall   __declspec(noalias)
#endif
#ifndef __malloccall
#define __malloccall  __declspec(restrict)
#endif
#ifndef __attribute_forceinline
#define __attribute_forceinline __forceinline
#endif
#ifndef __attribute_inline
#define __attribute_inline      __inline
#endif
#ifndef __attribute_thread
#define __attribute_thread __declspec(thread)
#endif

#ifndef __attribute_section
#define __attribute_section(name) __declspec(allocate(name))
#endif

#ifndef __compiler_assume
#define __compiler_assume   __assume
#endif
#ifndef __compiler_alignof
#define __compiler_alignof  __alignof
#endif

#ifndef __compiler_unreachable
#define __compiler_unreachable() __assume(0)
#endif

#ifndef __COMPILER_UNIQUE
#ifdef __COUNTER__
#define __COMPILER_UNIQUE(group) __PP_CAT_3(__hidden_,group,__COUNTER__)
#endif
#endif /* !__COMPILER_UNIQUE */

#ifndef __STATIC_ASSERT
#define __STATIC_ASSERT(expr) \
 extern __attribute_unused int __COMPILER_UNIQUE(sassert)[!!(expr)?1:-1]
#endif

#ifndef __compiler_pragma
#define __compiler_pragma __pragma
#endif

#ifndef __COMPILER_PACK_PUSH
#define __COMPILER_PACK_PUSH(n) __pragma(pack(push,n))
#define __COMPILER_PACK_POP     __pragma(pack(pop))
#endif

#ifndef __PP_VA_NARGS
/* Implement va_nargs using the microsoft virtual C preprocessor.
 * NOTE: This implementation is capable of detecting 0 arguments. */
#define __PP_FORCE_EXPAND_0(...) __VA_ARGS__
#define __PP_FORCE_EXPAND_1(...) __PP_FORCE_EXPAND_0(__VA_ARGS__)
#define __PP_PRIVATE_VA_NARGS_I(x,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56,_57,_58,_59,N,...) N
#define __PP_PRIVATE_VA_NARGS_X(...) (~,__VA_ARGS__,59,58,57,56,55,54,53,52,51,50,49,48,47,46,45,44,43,42,41,40,39,38,37,36,35,34,33,32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0)
#define __PP_VA_NARGS(...) __PP_FORCE_EXPAND_1(__PP_PRIVATE_VA_NARGS_I __PP_PRIVATE_VA_NARGS_X(__VA_ARGS__))
#endif /* !__PP_VA_NARGS */

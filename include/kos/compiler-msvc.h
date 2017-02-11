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

#ifndef __compiler_assume
#define __compiler_assume   __assume
#endif

#ifndef __compiler_unreachable
#define __compiler_unreachable() __assume(0)
#endif

#ifndef __compiler_UNIQUE
#ifdef __COUNTER__
#define __compiler_UNIQUE(group) __PP_CAT_3(__hidden_,group,__COUNTER__)
#endif
#endif /* !__compiler_UNIQUE */

#ifndef __STATIC_ASSERT
#define __STATIC_ASSERT(expr) \
 extern __attribute_unused int __compiler_UNIQUE(sassert)[!!(expr)?1:-1]
#endif

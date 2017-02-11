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
#ifndef __VADEFS_H__
#ifndef _INC_VADEFS
#define __VADEFS_H__ 1
#define _INC_VADEFS 1

// === Emulated MSVC header

#include <kos/compiler.h>

__DECL_BEGIN

#if defined(__GNUC__)
typedef __builtin_va_list va_list;
#   define _crt_va_start  __builtin_va_start
#   define _crt_va_end    __builtin_va_end
#   define __va_copy      __builtin_va_copy
#   define _crt_va_arg    __builtin_va_arg
#else /* GCC */
typedef char *va_list;
#if defined(__i386__) || defined(__i386) || defined(i386)
#   define __VA_ALIGN           3
#elif defined(__x86_64__)
#   define __VA_ALIGN           7
#else
#   define __VA_ALIGN           (sizeof(int)-1)
#endif
#   define __VA_SIZE(x)             ((sizeof(x)+__VA_ALIGN)&~__VA_ALIGN)
#   define _crt_va_start(args,start) (void)((args)=((va_list)&(start))+__VA_SIZE(start))
#   define _crt_va_end(args)         (void)(args)
#   define __va_copy(dst,src)        (void)((dst)=(src))
#   define _crt_va_arg(args,T)       (*(T *)(((args) += __VA_SIZE(T))-__VA_SIZE(T)))

// Compatibility with gcc builtins
#   define __gnuc_va_list      va_list
#   define __builtin_va_list   va_list
#   define __builtin_va_start  _crt_va_start
#   define __builtin_va_end    _crt_va_end
#   define __builtin_va_copy   __va_copy
#   define __builtin_va_arg    _crt_va_arg
#endif /* ... */

__DECL_END

#endif /* !_INC_VADEFS */
#endif /* !__VADEFS_H__ */

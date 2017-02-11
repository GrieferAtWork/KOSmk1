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
#ifndef __STDDEF_H__
#ifndef _STDDEF_H
#ifndef _INC_STDDEF
#define __STDDEF_H__ 1
#define _STDDEF_H 1
#define _INC_STDDEF 1

#include <__null.h>
#include <kos/compiler.h>
#include <kos/types.h>

__DECL_BEGIN

#undef offsetof
#if __has_builtin(__builtin_offsetof)
#   define offsetof(T,member) __builtin_offsetof(T,member)
#else
#   define offsetof(T,member) ((__size_t)&((T*)0)->member)
#endif

#define _offsetafter(T,member) ((__size_t)((&((T*)0)->member)+1))

#ifndef __STDC_PURE__
#   define offsetafter _offsetafter
#endif

#ifndef __size_t_defined
#define __size_t_defined 1
typedef __size_t size_t;
#endif

#ifndef __STDC_PURE__
#ifndef __ssize_t_defined
#define __ssize_t_defined 1
typedef __ssize_t ssize_t;
#endif
#endif

#ifndef __ptrdiff_t_defined
#define __ptrdiff_t_defined 1
typedef __ptrdiff_t ptrdiff_t;
#endif

#ifndef __max_align_t_defined
#define __max_align_t_defined 1
typedef __max_align_t max_align_t;
#endif

#ifndef __cplusplus
#ifndef __wchar_t_defined
#define __wchar_t_defined
typedef __WCHAR_TYPE__ wchar_t;
#endif
#endif /* __cplusplus */

__DECL_END

#endif /* !_INC_STDDEF */
#endif /* !_STDDEF_H */
#endif /* !__STDDEF_H__ */

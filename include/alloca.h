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
#ifndef __ALLOCA_H__
#ifndef _ALLOCA_H
#define __ALLOCA_H__ 1
#define _ALLOCA_H 1

#include <kos/compiler.h>
#ifndef __ASSEMBLY__
#include <kos/__alloca.h>
#if !defined(__KERNEL__) || defined(__INTELLISENSE__)
#include <kos/types.h>
#endif

__DECL_BEGIN

#undef alloca

#if !defined(__KERNEL__) || defined(__INTELLISENSE__)
// libc exports an alloca function
// HINT: This function also has an alias named '__libc_alloca'
// >> So with that in in mind, you can go ahead and use this for whatever.
extern __wunused /*__malloccall*/ void *alloca __P((__size_t __size));
#endif
#ifndef __INTELLISENSE__
#   define alloca    __alloca
#endif
#   define _oalloca(T)    ((T *)__alloca(sizeof(T)))
#ifndef __STDC_PURE__
#   define oalloca   _oalloca
#endif /* !__STDC_PURE__ */

#ifdef __STRING_H__
#ifdef __INTELLISENSE__
extern void *_calloca(__size_t count, __size_t size);
extern void *calloca(__size_t count, __size_t size);
#else
#define _calloca(count,size) \
 __xblock({ __size_t const __s = (count)*(size);\
            void *const __res = __alloca(__s);\
            __xreturn memset(__res,0,__s);\
 })
#ifndef __STDC_PURE__
#   define calloca   _calloca
#endif /* !__STDC_PURE__ */
#endif
#   define _ocalloca(T)   ((T *)_calloca(1,sizeof(T)))
#ifndef __STDC_PURE__
#   define ocalloca  _ocalloca
#endif /* !__STDC_PURE__ */
#endif /* __STRING_H__ */

__DECL_END
#endif /* !__ASSEMBLY__ */

#endif /* !_ALLOCA_H */
#endif /* !__ALLOCA_H__ */

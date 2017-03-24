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
#ifndef __MEMORY_H__
#ifndef _INC_MEMORY
#define __MEMORY_H__ 1
#define _INC_MEMORY 1

#include <kos/compiler.h>
#ifndef __ASSEMBLY__
#include <kos/types.h>

__DECL_BEGIN

#ifndef __size_t_defined
#define __size_t_defined 1
typedef __size_t size_t;
#endif

#ifndef __CONFIG_MIN_LIBC__
#ifndef ___memicmp_defined
#define ___memicmp_defined 1
extern __wunused __purecall __nonnull((1,2)) int _memicmp __P((void const *__a, void const *__b, __size_t __bytes));
#endif /* !___memicmp_defined */
#ifndef __STDC_PURE__
#ifndef __memicmp_defined
#define __memicmp_defined 1
#if !defined(__NO_asmname) || defined(__INTELLISENSE__)
extern __wunused __purecall __nonnull((1,2)) int memicmp __P((void const *__a, void const *__b, __size_t __bytes)) __asmname("_memicmp");
#else /* !__NO_asmname */
#define memicmp  _memicmp
#endif /* __NO_asmname */
#endif /* !__memicmp_defined */
#endif /* !__STDC_PURE__ */
#endif /* !__CONFIG_MIN_LIBC__ */

#ifndef __memchr_defined
#define __memchr_defined 1
#if defined(__cplusplus) && !defined(__NO_asmname) && !defined(__STRING_C__)
extern "C++" {
extern __wunused __purecall __nonnull((1)) void *memchr(void *__restrict __p, int __needle, size_t __bytes) __asmname("memchr");
extern __wunused __purecall __nonnull((1)) void const *memchr(void const *__restrict __p, int __needle, size_t __bytes) __asmname("memchr");
}
#else /* C++ */
extern __wunused __purecall __nonnull((1)) void *memchr __P((void const *__restrict __p, int __needle, size_t __bytes));
#endif /* C */
#endif /* !__memchr_defined */

#ifndef __memcmp_defined
#define __memcmp_defined 1
extern __wunused __purecall __nonnull((1,2)) int memcmp __P((void const *__a, void const *__b, __size_t __bytes));
#endif /* !__memcmp_defined */

#ifndef __memcpy_defined
#define __memcpy_defined 1
extern __retnonnull __nonnull((1,2)) void *
memcpy __P((void *__restrict __dst,
            void const *__restrict __src,
            __size_t __bytes));
#endif

#ifndef __memccpy_defined
#define __memccpy_defined 1
extern __nonnull((1,2)) void *memccpy __P((void *__restrict __dst, void const *__restrict __src, int __c, __size_t __bytes));
#endif

#ifndef __NO_asmname
extern __nonnull((1,2)) void *_memccpy __P((void *__restrict __dst, void const *__restrict __src, int __c, __size_t __bytes)) __asmname("memccpy");
#else /* !__NO_asmname */
#define _memccpy  memccpy
#endif /* __NO_asmname */

#ifdef _WIN32
#undef memcpy_s
#define memcpy_s(dst,dstsize,src,bytes)  memcpy(dst,src,bytes)
#endif

__DECL_END

#if !defined(__INTELLISENSE__) && defined(__LIBC_USE_ARCH_OPTIMIZATIONS)
#include <kos/arch/string.h>
/* Pull in arch-specific, optimized string operations */
#define memchr    arch_memchr
#define memcpy    arch_memcpy
#define memcmp    arch_memcmp
#endif /* __LIBC_USE_ARCH_OPTIMIZATIONS */

#endif /* !__ASSEMBLY__ */

#endif /* !_INC_MEMORY */
#endif /* !__MEMORY_H__ */

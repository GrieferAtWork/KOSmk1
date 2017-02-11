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
#ifndef ____KOS_ALLOCA_H__
#define ____KOS_ALLOCA_H__ 1

#include <kos/compiler.h>

#ifndef __ASSEMBLY__
#if !defined(__GNUC__) && !__has_builtin(__builtin_alloca)
#if defined(__GNUC__) /* TODO: Other compilers that support inline assembly. */
// Alloca implemented using inline assembly
// NOTE: When it can be used, this version is faster as it doesn'
//       have to step around arguments and the return address.
#define __builtin_alloca(s) \
 __xblock({ void *__ares;\
            __asm_volatile__("sub %1, %%esp\n"\
                             "mov %%esp, %0\n"\
            : "=r" (__ares) : "r" (s));\
            __xreturn __ares;\
 })
#else
#include <kos/types.h>
#define __LIBC_MUST_IMPLEMENT_LIBC_ALLOCA /*< Only meaningful when building the kernel. */
__DECL_BEGIN
extern __wunused __malloccall void *__libc_alloca(__size_t size);
#define __builtin_alloca   __libc_alloca
__DECL_END
#endif
#endif

#ifdef __KERNEL__
#include <kos/config.h>
#ifdef __KERNEL_HAVE_DEBUG_STACKCHECKS
#include <kos/types.h>
__DECL_BEGIN
extern void debug_checkalloca_d __P((__size_t bytes __LIBC_DEBUG__PARAMS));
__DECL_END
#define __alloca(s) \
 __xblock({ __size_t const __alloca_s = (s); \
            debug_checkalloca_d(__alloca_s __LIBC_DEBUG__ARGS);\
            __xreturn __builtin_alloca(__alloca_s);\
 })
#endif /* __KERNEL_HAVE_DEBUG_STACKCHECKS */
#endif /* __KERNEL__ */

#ifndef __alloca
#define __alloca    __builtin_alloca
#endif /* !__alloca */
#endif /* !__ASSEMBLY__ */

#endif /* !____KOS_ALLOCA_H__ */

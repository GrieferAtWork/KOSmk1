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
#ifndef __KOS_KERNEL_PANIC_H__
#define __KOS_KERNEL_PANIC_H__ 1

#include <kos/config.h>
#include <kos/compiler.h>
#ifdef __KERNEL__
#ifndef __ASSEMBLY__

__DECL_BEGIN

extern __noinline __noclone __noreturn __coldcall
#if    __LIBC_HAVE_DEBUG_PARAMS == 3
       __attribute_vaformat(__printf__,4,5)
#elif  __LIBC_HAVE_DEBUG_PARAMS == 0
       __attribute_vaformat(__printf__,1,2)
#else
#      error "FIXME"
#endif
void k_syspanic(__LIBC_DEBUG_PARAMS_ char const *fmt, ...);

//////////////////////////////////////////////////////////////////////////
// Invoke kernel panic.
// >> Similar to failing an assertion, but cannot be disabled
//    through a preprocessor switch and should therefor be used
//    as rare as possible.
// >> Main uses are handling of allocation failure in kernel
//    initialization routines, such as allocating the kernel's
//    own page directory.
// USE:
// >> static void *buffer;
// >> void kernel_initialize_buffer(void) {
// >>   buffer = malloc(4096);
// >>   if (!buffer) PANIC("Failed to allocated buffer");
// >> }
#define PANIC(...)   k_syspanic(__LIBC_DEBUG_ARGS_ __VA_ARGS__)

__DECL_END

#endif /* !__ASSEMBLY__ */
#endif /* !__KERNEL__ */


#endif /* !__KOS_KERNEL_PANIC_H__ */

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
#ifndef __KOS_TASK_TLS_H__
#define __KOS_TASK_TLS_H__ 1

#include <kos/config.h>
#include <kos/compiler.h>
#ifndef __KERNEL__
#ifndef __NO_PROTOTYPES
#include <kos/errno.h>
#include <kos/types.h>
#include <kos/syscall.h>
#endif /* !__NO_PROTOTYPES */
#endif /* !__KERNEL__ */

__DECL_BEGIN

#ifndef __KERNEL__
#ifndef __NO_PROTOTYPES

//////////////////////////////////////////////////////////////////////////
// Allocate/Free dynamic, thread-local storage memory.
// Using 'kproc_tlsalloc', you can allocate a new block of thread-local storage
// that can be accessed in any thread using the arch-specific TLS register (%fs/%gs).
// The offset that must be added to access the TLS address is then stored in 'tls_offset'.
// A TLS block can later be freed using 'kproc_tlsfree', by specifying
// the same offset as previously returned in a call to 'kproc_tlsalloc'.
// @param: TEMPLATE: A template data block that will be used to initialize TLS blocks.
//             NOTE: The data pointed to here will be copied during this call,
//                   meaning you won't have to keep the template around.
//             HINT: Passing NULL for 'TEMPLATE' causes the kernel to
//                   generate a template filled with nothing but ZEROes.
// @param: TEMPLATE_SIZE: The size of the given template, or to be more precise:
//                        the amount of TLS memory to allocate.
// @param: TLS_OFFSET: Output parameter to store the newly allocated TLS offset inside of.
// @return: KE_OK:    Successfully allocated a new TLS block.
// @return: KE_NOMEM: Not enough available memory to complete the operation.
__local _syscall1(kerrno_t,kproc_tlsfree,__ptrdiff_t,tls_offset);
#ifndef __ASSEMBLY__
__local kerrno_t
kproc_tlsalloc(void const *__template,
               __size_t __template_size,
               __ptrdiff_t *__restrict tls_offset) {
 kerrno_t __error;
 __asm_volatile__("int $" __PP_STR(__SYSCALL_INTNO) "\n"
                  : "=a" (__error), "=c" (*tls_offset)
                  : "a" (SYS_kproc_tlsalloc)
                  , "b" (__template)
                  , "c" (__template_size)
                  : "memory");
 return __error;
}
#endif /* !__ASSEMBLY__ */

#endif /* !__NO_PROTOTYPES */
#endif /* !__KERNEL__ */

__DECL_END

#endif /* !__KOS_TASK_TLS_H__ */

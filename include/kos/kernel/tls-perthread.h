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
#ifndef __KOS_KERNEL_TLS_PERTHREAD_H__
#define __KOS_KERNEL_TLS_PERTHREAD_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/kernel/object.h>
#include <kos/kernel/paging.h>
#include <kos/kernel/rwlock.h>
#include <kos/kernel/gdt.h>
#include <kos/types.h>
#include <kos/task-tls.h>
#include <stdint.h>
#include <malloc.h>

/* Task/Thread local storage */

__DECL_BEGIN

#define KOBJECT_MAGIC_TLSPT  0x77597   /*< TLSPT */
#ifdef __i386__
#   define KTLS_SEGMENT_REGISTER  gs
#elif defined(__x86_64__)
#   define KTLS_SEGMENT_REGISTER  fs
#else
#   error "FIXME"
#endif


#ifndef __ASSEMBLY__
/* Offset from the TLS segment register. */
typedef __ptrdiff_t ktls_addr_t;
#endif /* !__ASSEMBLY__ */

#define kassert_ktlspt(self)  kassert_object(self,KOBJECT_MAGIC_TLSPT)

#define KUTHREAD_PAGESIZE     1 /* (align(sizeof(struct kuthread),PAGESIZE)/PAGESIZE) */
#define KTLSPT_SIZEOF        (KOBJECT_SIZEOFHEAD+4+2*__SIZEOF_POINTER__)
#define KTLS_UREGION_FLAGS  \
 (KSHMREGION_FLAG_READ|KSHMREGION_FLAG_WRITE| /* Allow read-write access. */\
  KSHMREGION_FLAG_LOSEONFORK|                 /* Loose this mapping during a fork(). */\
  KSHMREGION_FLAG_SHARED                      /* Prevent COW semantics and allow us to keep an additional reference. */)

#ifndef __ASSEMBLY__
struct kshmregion;
struct ktlspt {
 KOBJECT_HEAD
 ksegid_t                              pt_segid;   /*< [const] The segment ID used for thread-local storage. */
 __u16                                 pt_padding; /*< Padding... */
 __pagealigned __user struct kuthread *pt_uthread; /*< [1..1][lock(:t_proc->p_shm.s_lock)] Pointer to the user-mapped TLS block (Base address of the 'pt_segid' segment). */
 __ref struct kshmregion              *pt_uregion; /*< [1..1][lock(:t_proc->p_shm.s_lock)] SHM region describing the 'pt_uthread' mapping.
                                                        NOTE: This region uses flags described by 'KTLS_UREGION_FLAGS'. */
};
#define KTLSPT_INIT {KOBJECT_INIT(KOBJECT_MAGIC_TLSPT) KSEG_NULL,0,NULL,NULL}

//////////////////////////////////////////////////////////////////////////
// Set the userspace accessible task name to 'name'.
extern void ktlspt_setname(struct ktlspt *__restrict self,
                           char const *__restrict name);

//////////////////////////////////////////////////////////////////////////
// Raise a given exception 'exinfo' in the task associated with 'SELF'.
// For this purpose, one exception handler is popped of the handler stack
// and the register state described by 'regs' is stored in the exinfo
// fields of the uthread before being update to describe the previously
// popped exception handler.
// NOTE: The caller is responsible to ensure that the given task is
//       suitable for use of TLS memory (aka. isn't a kernel thread)
// HINT: You should also make sure that any IRQ exception you're raising
//       actually originated from ring-#3 (aka. checking the 'cs' register)
// @return: KE_OK:    Successfully performed all of the above.
//                    You may no use 'regs' to return to userspace.
// @return: KE_FAULT: The userspace exception handler chain contained a
//                    faulty pointer that prevented dereferencing of
//                    the non-NULL handler entry.
// @return: KE_NOENT: No userspace exception handler was defined.
extern __wunused __nonnull((1,2,3)) kerrno_t
ktask_raise_exception(struct ktask *__restrict self,
                      struct kirq_registers *__restrict regs,
                      struct kexinfo const *__restrict exinfo);

//////////////////////////////////////////////////////////////////////////
// Similar to 'ktask_raise_exception', but the exception raised is an IRQ.
// @return: KE_OK:    Successfully raised the exception.
// @return: KE_FAULT: The userspace exception handler chain contained a
//                    faulty pointer that prevented dereferencing of
//                    the non-NULL handler entry.
// @return: KE_NOENT: No userspace exception handler was defined.
// @return: KE_INVAL: No userspace exception number associated with the IRQ error.
extern __wunused __nonnull((1,2)) kerrno_t
ktask_raise_irq_exception(struct ktask *__restrict self,
                          struct kirq_registers *__restrict regs);

#endif /* !__ASSEMBLY__ */

__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_TLS_PERTHREAD_H__ */

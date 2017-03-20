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

#ifndef __ASSEMBLY__
#define KTLS_UTHREAD_PAGESIZE  1 /* (align(sizeof(struct ktls_uthread),PAGESIZE)/PAGESIZE) */
struct ktls_uthread {
 /* This structure is located at offset ZERO, and actual TLS variables as located below. */
 /* Per-thread user structure at positive offsets. */
 __user struct ktls_uthread *u_self; /*< [1..1] Structure self-pointer. */
 /* TODO: Additional OS runtime-support must be added here, such
  *       as userspace exception handling and stack address/size. */
};
#endif /* !__ASSEMBLY__ */

#define KTLSPT_SIZEOF        (KOBJECT_SIZEOFHEAD+4+2*__SIZEOF_POINTER__)
#define KTLS_UREGION_FLAGS  \
 (KSHMREGION_FLAG_READ|KSHMREGION_FLAG_WRITE| /* Allow read-write access. */\
  KSHMREGION_FLAG_LOSEONFORK|                 /* Loose this mapping during a fork(). */\
  KSHMREGION_FLAG_SHARED                      /* Prevent COW semantics and allow us to keep an additional reference. */)

#ifndef __ASSEMBLY__
struct kshmregion;
struct ktlspt {
 KOBJECT_HEAD
 ksegid_t                                  pt_segid;   /*< [const] The segment ID used for thread-local storage. */
 __u16                                     pt_padding; /*< Padding... */
 __pagealigned __user struct ktls_uthread *pt_uthread; /*< [1..1][lock(:t_proc->p_shm.s_lock)] Pointer to the user-mapped TLS block (Base address of the 'pt_segid' segment). */
 __ref struct kshmregion                  *pt_uregion; /*< [1..1][lock(:t_proc->p_shm.s_lock)] SHM region describing the 'pt_uthread' mapping.
                                                            NOTE: This region uses flags described by 'KTLS_UREGION_FLAGS'. */
};
#define KTLSPT_INIT {KOBJECT_INIT(KOBJECT_MAGIC_TLSPT) KSEG_NULL,0,NULL,NULL}
#endif /* !__ASSEMBLY__ */

__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_TLS_PERTHREAD_H__ */

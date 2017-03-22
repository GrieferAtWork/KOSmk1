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
#ifndef __KOS_KERNEL_TLS_H__
#define __KOS_KERNEL_TLS_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/kernel/object.h>
#include <kos/kernel/paging.h>
#include <kos/kernel/rwlock.h>
#include <kos/kernel/tls-perthread.h>
#include <kos/types.h>
#include <stdint.h>
#include <malloc.h>

/* Task/Thread local storage */

__DECL_BEGIN

#define KOBJECT_MAGIC_TLSMAN 0x77522A5 /*< TLSMAN */

#ifndef __ASSEMBLY__
#define kassert_ktlsman(self) kassert_object(self,KOBJECT_MAGIC_TLSMAN)

struct ktlsmapping {
 struct ktlsmapping       *tm_prev;   /*< [0..1][lock(:tls_lock)][owned] TLS mapping with a lower offset than this. */
 __ref struct kshmregion  *tm_region; /*< [1..1][const] Region of memory. */
 __pagealigned ktls_addr_t tm_offset; /*< [const] Offset of this mapping from the TLS register (lowest address). */
 __size_t                  tm_pages;  /*< [!0][const][== tm_region->sre_chunk.sc_pages] Mirror of the total page counter in tm_region. */
};

struct ktlsman {
 /* TLS Manager (per-process)
  * NOTE: When both 'tls_lock' (write-mode), as well as the associated
  *       process's 'KPROC_LOCK_THREADS' lock are required to be held,
  *       'tls_lock' (write-mode) must be acquired first. */
 KOBJECT_HEAD
 struct krwlock      tls_lock;      /*< Lock for this TLS manager. */
 struct ktlsmapping *tls_hiend;     /*< [lock(tls_lock)][owned] High-address end of the TLS mapping chain. */
 __size_t            tls_map_pages; /*< [lock(tls_lock)] Total amount of continuous pages required for TLS mappings. */
 __size_t            tls_all_pages; /*< [lock(tls_lock)] Total size (in pages) of continuous, unused user address space required to map this chain in SHM (Including any required for 'struct kuthread'). */
};
#define KTLSMAN_INITROOT \
 {KOBJECT_INIT(KOBJECT_MAGIC_TLSMAN) KRWLOCK_INIT,NULL,0,KUTHREAD_PAGESIZE}

//////////////////////////////////////////////////////////////////////////
// Initialize/Finalize a given TLS manager.
extern void ktlsman_init(struct ktlsman *__restrict self);
extern void ktlsman_quit(struct ktlsman *__restrict self);


struct ktlspt;
struct kproc;
struct kshmregion;

//////////////////////////////////////////////////////////////////////////
// Initializes a given TLS Manager as the copy of another.
// NOTE: The caller must hold a lock to 'right->tls_lock' (read-mode)
// @return: KE_OK:    Successfully initialized the given TLS manager.
// @return: KE_NOMEM: Not enough memory to complete the operation.
extern __wunused __nonnull((1,2)) kerrno_t
ktlsman_initcopy_unlocked(struct ktlsman *__restrict self,
                          struct ktlsman const *__restrict right);

//////////////////////////////////////////////////////////////////////////
// Allocate/Free space of sufficient size for a given SHM region.
// Using COW semantics, upon success that region will be mapped in
// all tasks (threads) in the given process at the returned offset.
// >> Since COW is used, the actual physical TLS data is only copied
//    once any thread begins writing to the associated pages.
// NOTE: Memory stored in the given region is used as a template.
// NOTE: During remapping, the TLS vector of some threads might not
//       be possible to be extended. - In such cases, those threads
//       are suspended momentarily, and their LDT register is updated
//       to reflect some relocated mapping.
// @param: offset: Upon success, the offset from the TLS segment is stored here.
//                 In most situations, that offset is then re-used during relocations to
//                 allow the associated binary to talk with variables in its TLS segment.
// @param: page_alignment: Multi-page alignment. - Can be used to align the region by more than one page. (Must not be ZERO(0))
// @return: KE_OK:         Successfully allocated a new TLS memory region and mapped it into all threads.
// @return: KE_NOMEM:      Not enough available memory to create required mappings.
// @return: KE_NOSPC:      Failed to locate enough unused address ranges to map the region in all existing threads.
// @return: KE_OVERFLOW:   A reference counter would have overflown.
// @return: KE_INTR:       The calling process was interrupted.
// @return: KE_DESTROYED:  The TLS manager or thread list of the given process was closed.
extern __crit __wunused __nonnull((1,2,3)) kerrno_t
kproc_tls_alloc_inherited(struct kproc *__restrict self,
                          __ref struct kshmregion *__restrict region,
                          ktls_addr_t *__restrict offset);
extern __crit __wunused __nonnull((1,2,3)) kerrno_t
kproc_tls_alloc(struct kproc *__restrict self,
                struct kshmregion *__restrict region,
                ktls_addr_t *__restrict offset);

//////////////////////////////////////////////////////////////////////////
// Free a given SHM region previously allocated as TLS memory.
// Upon success, the region will be unmapped from all running threads.
// @return: KE_OK:    Successfully unmapped the given region.
// @return: KE_NOENT: The given region was not mapped.
extern __crit __nonnull((1,2)) kerrno_t
kproc_tls_free_region(struct kproc *__restrict self,
                      struct kshmregion *__restrict region);
extern __crit __nonnull((1)) kerrno_t
kproc_tls_free_offset(struct kproc *__restrict self,
                      ktls_addr_t offset);


//////////////////////////////////////////////////////////////////////////
// Allocate an LDT entry for the given task and bind all existing TLS mappings.
// The caller is expected to hold a lock to:
//   - self->p_shm.s_lock   (write-lock)
//   - self->p_tls.tls_lock (read-lock)
// Upon success, the 'task->t_tls' object is initialized and the
// caller may choose to assign 'pt_segid' to any segment register
// to gain access to previously mapped TLS memory.
// NOTES:
//   - This function will also allocate an SHM region used for the uthread structure.
//   - To allow ring-#3 code easy access to stack limits, call this function
//     _AFTER_ you have already allocated the task's userspace stack.
// WARNING: Call this function during task initialization is _NOT_ optional!
// @return: KE_OK:    Successfully initialized the TLS block of the given task.
// @return: KE_NOMEM: Not enough available memory.
// @return: KE_NOSPC: No unmapped memory range of sufficient size for the TLS block was found.
extern __crit __wunused __nonnull((1,2)) kerrno_t
kproc_tls_alloc_pt_unlocked(struct kproc *__restrict self,
                            struct ktask *__restrict task);

//////////////////////////////////////////////////////////////////////////
// Called during task cleanup: unmap all TLS regions from the given
//                             task and delete its LDT entry.
// The caller is expected to hold a lock to:
//   - self->p_shm.s_lock   (write-lock)
extern __crit __nonnull((1,2)) void
kproc_tls_free_pt_unlocked(struct kproc *__restrict self,
                           struct ktask *__restrict task);

//////////////////////////////////////////////////////////////////////////
// Called during fork() on the calling task:
//   - Copy the TLS uthread data from 'right' into 'self'.
extern __crit __nonnull((1,2)) void
ktlspt_copyuthread_unlocked(struct ktlspt *__restrict self,
                            struct ktlspt const *__restrict right);

//////////////////////////////////////////////////////////////////////////
// Called after 'kproc_tls_alloc_pt_unlocked' to fill the TLS uthread
// data block with some useful information, such as stack info & ids.
// WARNING: All data that we put into the uthread block can be accessed &
//          overwritten by the user (When reading, don't believe anything!).
// NOTE: The caller is responsible to hold a lock on 'self->p_shm.s_lock' (read-mode)
extern __crit __nonnull((1,2)) void
kproc_tls_pt_setup(struct kproc *__restrict self,
                   struct ktask *__restrict task);


__DECL_END
#endif /* !__ASSEMBLY__ */
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_TLS_H__ */

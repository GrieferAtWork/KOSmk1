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
#include <kos/types.h>
#include <stdint.h>
#include <malloc.h>

__DECL_BEGIN

#define KOBJECT_MAGIC_TLSMAN 0x77522A5 // TLSMAN
#define KOBJECT_MAGIC_TLSPT  0x77597   // TLSPT

#ifndef __ASSEMBLY__
#define kassert_ktlsman(self) kassert_object(self,KOBJECT_MAGIC_TLSMAN)
#define kassert_ktlspt(self)  kassert_object(self,KOBJECT_MAGIC_TLSPT)

#define KTLS_LOGICAL_MAXIMUM  SIZE_MAX

// Task local storage

struct ktlsman {
 // TLS Manager, stored by a task context
 KOBJECT_HEAD
 __size_t tls_cnt;   /*< Current amount of TLS ids in use. */
 __size_t tls_max;   /*< Max amount of allowed TLS ids. */
 __size_t tls_usedc; /*< Highest valid TLS index+1. */
 __u8    *tls_usedv; /*< [0..tls_usedc/8][owned] Bit-vector masking valid TLS ids. */
 __size_t tls_free;  /*< Hint towards a free TLS slot (may not actually be valid). */
};
#define KTLSMAN_INITROOT \
 {KOBJECT_INIT(KOBJECT_MAGIC_TLSMAN) 0,KTLS_LOGICAL_MAXIMUM,0,NULL,0}

#ifdef __INTELLISENSE__
extern void ktlsman_initroot(struct ktlsman *__restrict self);
extern void ktlsman_quit(struct ktlsman *__restrict self);
extern void ktlsman_clear(struct ktlsman *__restrict self);
#else
#define ktlsman_initroot(self) \
 __xblock({ struct ktlsman *const __ktlsself = (self);\
            kobject_init(__ktlsself,KOBJECT_MAGIC_TLSMAN);\
            __ktlsself->tls_cnt   = 0;\
            __ktlsself->tls_max   = KTLS_LOGICAL_MAXIMUM;\
            __ktlsself->tls_usedc = 0;\
            __ktlsself->tls_usedv = NULL;\
            __ktlsself->tls_free  = 0;\
            (void)0;\
 })
#define ktlsman_quit(self)  free((self)->tls_usedv)
#define ktlsman_clear(self) \
 __xblock({ struct ktlsman *const __ktlsself = (self);\
            free(__ktlsself->tls_usedv);\
            __ktlsself->tls_cnt   = 0;\
            __ktlsself->tls_usedc = 0;\
            __ktlsself->tls_usedv = NULL;\
            __ktlsself->tls_free  = 0;\
            (void)0;\
 })
#endif


//////////////////////////////////////////////////////////////////////////
// Initializes a given TLS Manager as the copy of another.
// @return: KE_NOMEM: Not enough memory to complete the operation
extern kerrno_t ktlsman_initcopy(struct ktlsman *self, struct ktlsman const *right);

//////////////////////////////////////////////////////////////////////////
// Allocates a new TLS identifier
// @return: KE_OK:    A new slot was allowed and '*result' was filled with it.
// @return: KS_FOUND: An free slot was reused (NOT AN ERROR)
// @return: KE_NOMEM: Not enough memory available to complete the operation.
// @return: KE_ACCES: Too many TLS ids.
extern __wunused __nonnull((1,2)) kerrno_t
ktlsman_alloctls(struct ktlsman *__restrict self,
                 __ktls_t *__restrict result);

//////////////////////////////////////////////////////////////////////////
// Frees a previously allocated TLS identifier
// NOTE: The caller is responsible for invalidating the
//       given slot in all tasks that may have used it prior.
extern void ktlsman_freetls(struct ktlsman *__restrict self, __ktls_t slot);

//////////////////////////////////////////////////////////////////////////
// Returns true (non-ZERO) if the given slot is valid.
extern int ktlsman_validtls(struct ktlsman const *__restrict self, __ktls_t slot);
#endif /* !__ASSEMBLY__ */


#define KTLSPT_SIZEOF        (KOBJECT_SIZEOFHEAD+__SIZEOF_SIZE_T__+__SIZEOF_POINTER__)
#define KTLSPT_OFFSETOF_VECC (KOBJECT_SIZEOFHEAD)
#define KTLSPT_OFFSETOF_VECV (KOBJECT_SIZEOFHEAD+__SIZEOF_SIZE_T__)

#ifndef __ASSEMBLY__
struct ktlspt {
 KOBJECT_HEAD
 // TLS Per-task data block, found stored in each task
 __size_t tpt_vecc; /*< Allocated TLS vector size. */
 void   **tpt_vecv; /*< [?..?][0..tpt_vecc] Vector of TLS values (index == tls-id). */
};
#define KTLSPT_INIT {KOBJECT_INIT(KOBJECT_MAGIC_TLSPT) 0,NULL}


extern __crit __nonnull((1,2)) kerrno_t
ktlspt_initcopy(struct ktlspt *__restrict self,
                struct ktlspt const *__restrict right);

#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Initialize/Finalize a given TLS per-task data block.
extern __nonnull((1)) void ktlspt_init(struct ktlspt *__restrict self);
extern __nonnull((1)) void ktlspt_quit(struct ktlspt *__restrict self);

//////////////////////////////////////////////////////////////////////////
// Clear all stored TLS values.
extern __nonnull((1)) void ktlspt_clear(struct ktlspt *__restrict self);
#else
#define ktlspt_init(self) \
 __xblock({ struct ktlspt *const __ktself = (self);\
            kobject_init(__ktself,KOBJECT_MAGIC_TLSPT);\
            __ktself->tpt_vecc = 0;\
            __ktself->tpt_vecv = NULL;\
            (void)0;\
 })
#define ktlspt_quit(self) \
 __xblock({ struct ktlspt *const __ktself = (self);\
            kassert_ktlspt(__ktself);\
            free(__ktself->tpt_vecv);\
            (void)0;\
 })
#define ktlspt_clear(self) \
 __xblock({ struct ktlspt *const __ktself = (self);\
            kassert_ktlspt(__ktself);\
            free(__ktself->tpt_vecv);\
            __ktself->tpt_vecv = NULL;\
            __ktself->tpt_vecc = 0;\
            (void)0;\
 })
#endif


#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Returns the value associated with a given slot.
// >> Returns NULL for uninitialized slots.
// >> NOTE: Since there should be no situation in which
//          a caller should care whether or not a given
//          TLS id is invalid or just uninitialized, this
//          system isn't even capable of differenciating.
// @return: NULL: The given slot is either invalid, or uninitialized.
extern __wunused __nonnull((1)) void *
ktlspt_get(struct ktlspt const *__restrict self,
           __ktls_t slot);
#else
#define ktlspt_get(self,slot) \
 __xblock({ struct ktlspt const *const __ktself = (self);\
            __ktls_t const __ktslot = (slot);\
            __xreturn __ktslot < __ktself->tpt_vecc ? __ktself->tpt_vecv[__ktslot] : NULL;\
 })

#endif

//////////////////////////////////////////////////////////////////////////
// Delete the TLS entry associated with the given slot,
// possibly freeing unused vector memory along the way.
//  - This function is a no-op if the given slot
//    is invalid, or simply was never initialized.
// NOTE: This function should also be called for every task
//       inside a given task context when a given slow is deleted.
extern __nonnull((1)) void
ktlspt_del(struct ktlspt *__restrict self,
           __ktls_t slot);

//////////////////////////////////////////////////////////////////////////
// Sets the value of a given TLS slot.
// WARNING: The caller is responsible to ensure that the given slot
//          is valid, and failure to do so will lead to a security
//          hole of user-processes being able to overallocate TLS
//          vectors to the point of hawking up all the available memory.
// @return: KE_OK:    The TLS value was successfully set.
// @return: KS_FOUND: The given 'slot' was already allocated and a
//                    potential previous value was overwritten.
//                    NOTE: This does not mean that this was
//                          the first time 'slot' was set.
// @return: KE_NOMEM: The given 'slot' had yet to be set, and when attempting
//                    to set it just now, the kernel failed to reallocate its
//                    vector stored TLS values.
extern __wunused __nonnull((1)) kerrno_t
ktlspt_set(struct ktlspt *__restrict self,
           __ktls_t slot, void *value);

__DECL_END
#endif /* !__ASSEMBLY__ */
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_TLS_H__ */

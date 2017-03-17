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
#ifndef __KOS_KERNEL_PROCPERM_H__
#define __KOS_KERNEL_PROCPERM_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/compiler.h>
#include <kos/atomic.h>
#include <kos/types.h>
#include <kos/task.h>
#include <kos/perm.h>
#include <kos/kernel/object.h>

__DECL_BEGIN

#define KOBJECT_MAGIC_PROCPERM  0x960C9E53 /*< PROCPERM. */
#define kassert_kprocperm(self) kassert_object(self,KOBJECT_MAGIC_PROCPERM)

#ifndef __ASSEMBLY__
struct ktask;
struct kprocperm {
 KOBJECT_HEAD
union{struct{
 __atomic __ref struct ktask *pp_gpbarrier; /*< Barrier: GETPROP/VISIBLE. */
 __atomic __ref struct ktask *pp_spbarrier; /*< Barrier: SETPROP. */
 __atomic __ref struct ktask *pp_gmbarrier; /*< Barrier: GETMEM. */
 __atomic __ref struct ktask *pp_smbarrier; /*< Barrier: SETMEM. */
};
 __atomic __ref struct ktask *pp_barriers[KSANDBOX_BARRIER_COUNT];
};
 /* NOTE: Priority limits only affect a task's ability of setting the
  *       priorities of some tasks, or having set their own priority in some way. */
 __atomic ktaskprio_t         pp_priomin; /*< Lowest allowed priority to be set. */
 __atomic ktaskprio_t         pp_priomax; /*< Greatest allowed priority to be set. */
#if KCONFIG_HAVE_TASK_NAMES
 __atomic __size_t            pp_namemax; /*< Max allowed task name length to be set. */
#endif /* KCONFIG_HAVE_TASK_NAMES */
 __atomic __size_t            pp_pipemax; /*< Max allowed pipe size to be set. */
 __atomic __size_t            pp_thrdmax; /*< Max amount of threads running in this process (TODO: Not enforced). */
 __atomic __u16               pp_flags[KPERM_FLAG_GROUPCOUNT]; /*< Permission flags of the various permission groups. */
#define KPROCSTATE_FLAG_NONE      0x00000000
#define KPROCSTATE_FLAG_SYSLOGINL 0x00000001 /*< The syslog didn't end with a linefeed (continue writing in-line) */
#define KPROCSTATE_SHIFT_LOGPRIV  28
#define KPROCSTATE_MASK_LOGPRIV   0xf0000000 /*< Mask specifying the syslog privilege level of this process.
                                                 NOTE: The process may only log with levels >= this value. */
 __atomic __u32               pp_state;   /*< [atomic] Process-specific state flags. */
};
#define kprocperm_getpriomin(self)       katomic_load((self)->pp_priomin)
#define kprocperm_getpriomax(self)       katomic_load((self)->pp_priomax)
#if KCONFIG_HAVE_TASK_NAMES
#define kprocperm_getnamemax(self)       katomic_load((self)->pp_namemax)
#endif /* KCONFIG_HAVE_TASK_NAMES */
#define kprocperm_getthrdmax(self)       katomic_load((self)->pp_pipemax)
#define kprocperm_getpipemax(self)       katomic_load((self)->pp_pipemax)
#define kprocperm_getlogpriv(self) (int)(katomic_load((self)->pp_state) >> KPROCSTATE_SHIFT_LOGPRIV)


//////////////////////////////////////////////////////////////////////////
// Initialize the given process permissions with root privileges,
// or as a copy those found in the given 'right' permissions.
// @return: KE_OK:       Successfully initialized the given permissions.
// @return: KE_OVERFLOW: A reference counter would have overflown.
extern __crit __wunused __nonnull((1)) kerrno_t
kprocperm_initroot(struct kprocperm *__restrict self);
extern __crit __wunused __nonnull((1,2)) kerrno_t
kprocperm_initcopy(struct kprocperm *__restrict self,
                   struct kprocperm const *__restrict right);

//////////////////////////////////////////////////////////////////////////
// Destroy a previously initializes process permissions structure.
extern __crit void kprocperm_quit(struct kprocperm *__restrict self);


//////////////////////////////////////////////////////////////////////////
// Returns the highest-level task associated with any of the given barriers.
// WARNING: This function is not thread-safe and does not return a reference.
//          The caller is responsible to implement a locking mechanism.
// EXCEPTION: If only one barrier is referred to, this function always returns
//            a stable pointer, though that pointer cannot be safely
//            dereferenced unless the caller is holding the same lock
//            used to guard multi-barrier calls.
// @return: * : The highest-level task contained with all given barriers.
extern __retnonnull __nomp __nonnull((1)) struct ktask *
kprocperm_getmaxbarrier(struct kprocperm const *__restrict self,
                        ksandbarrier_t barrier);

#if 0
//////////////////////////////////////////////////////////////////////////
// Set the task associated with all barriers described
// by the given mode, overwriting any previous barriers.
// NOTES:
//   - The caller should ensure that that no newly set
//     barrier has a privilege level greater than those
//     previously set. (For security)
//   - Setting a barrier that was already set before
//     has no negative side-effects.
//   - This function is actually thread-safe on its own, but
//     when ever used in conjunction with 'kprocperm_getmaxbarrier',
//     the caller must still hold their exclusive lock.
// WARNING: Dropping references from the old barriers may cause undefined
//          behavior while the caller is still holding their lock.
// @return: KE_OK:       Successfully overwritten all barriers associated with the given mode.
// @return: KE_OVERFLOW: The reference counter of the given task would have overflown.
extern __crit __retnonnull __nonnull((1,2)) kerrno_t
kprocperm_setbarrier(struct kprocperm *__restrict self,
                     struct ktask *__restrict barrier,
                     ksandbarrier_t mode);
#endif

#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Similar to 'kprocperm_getmaxbarrier', but includes special
// compile-time optimizations for faster lookup of single barriers.
__local __retnonnull __nomp __nonnull((1)) struct ktask *
kprocperm_getbarrier(struct kprocperm const *__restrict self,
                     ksandbarrier_t barrier);
#endif



#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Returns TRUE(1) if the given permissions flag is set, FALSE(0) otherwise.
extern __wunused int
kprocperm_hasflag(struct kprocperm const *__restrict self,
                  kperm_flag_t flag);
#else
#define kprocperm_hasflag(self,flag) \
  (((self)->pp_flags[KPERM_FLAG_GETGROUP(flag)]&\
                    KPERM_FLAG_GETMASK(flag)\
    ) == KPERM_FLAG_GETMASK(flag))

#define kprocperm_getbarrier(self,barrier) \
 (__builtin_constant_p(barrier) \
  ? __kprocperm_constant_getmaxbarrier(self,barrier)\
  :            kprocperm_getmaxbarrier(self,barrier))
__local __retnonnull __nomp struct ktask *
__kprocperm_constant_getmaxbarrier(struct kprocperm const *__restrict self,
                                   ksandbarrier_t barrier) {
 switch (barrier) {
  case KSANDBOX_BARRIER_NOSETMEM : return katomic_load(self->pp_smbarrier);
  case KSANDBOX_BARRIER_NOGETMEM : return katomic_load(self->pp_gmbarrier);
  case KSANDBOX_BARRIER_NOSETPROP: return katomic_load(self->pp_spbarrier);
  case KSANDBOX_BARRIER_NOGETPROP: return katomic_load(self->pp_gpbarrier);
  default: return kprocperm_getmaxbarrier(self,barrier);
 }
}
#endif


//////////////////////////////////////////////////////////////////////////
// Fill in information about the given permission.
// NOTE: The caller is responsible to ensure get_perm permissions are set.
// NOTE: The caller is responsible to ensure that the 'perm' buffer is
//       of sufficient size for any permission that might be requested.
//       >> This function does not check 'p_size' to be sufficent before filling in info.
//       >> Just provide a buffer of 'offsetof(struct kperm,p_data)+KPERM_MAXDATASIZE' bytes.
// @return: KE_OK:        Filled in the given permission ('p_size' is set to the required size)
//                        NOTE: Not all required memory may have been filled in
//                              if the old value of 'p_size' was lower than the new.
// @return: KE_INVAL:     Unsupported permission ('perm' is not modified).
// @return: KE_DESTROYED: The given process was destroyed.
extern __crit __wunused __nonnull((1,2)) kerrno_t
kproc_perm_get(struct kproc *__restrict self,
               struct kperm *perm);

//////////////////////////////////////////////////////////////////////////
// Set/exchange the given permission.
// NOTE: The caller is responsible to ensure set_perm permissions are set.
// NOTE: When the given permission refers to flags, the specified flags are removed.
// @return: KE_OK:        Set/exchanged the given permission (NOTE: When exchanging, 'p_size' is set to the required size)
// @return: KE_BUFSIZE:   The data block of the given permission was too small.
// @return: KE_INVAL:     Unsupported permission.
// @return: KE_ACCES:     Attempted to widen a permission restriction.
// @return: KE_DESTROYED: The given process was destroyed.
extern __crit __wunused __nonnull((1,2)) kerrno_t
kproc_perm_set(struct kproc *__restrict self,
               struct kperm const *perm);
extern __crit __wunused __nonnull((1,2)) kerrno_t
kproc_perm_xch(struct kproc *__restrict self,
               struct kperm *perm);

#endif /* !__ASSEMBLY__ */

__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_PROCPERM_H__ */

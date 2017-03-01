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
#ifndef __KOS_KERNEL_PROCPERM_C__
#define __KOS_KERNEL_PROCPERM_C__ 1

#include <kos/config.h>
#include <kos/compiler.h>
#include <kos/types.h>
#include <kos/atomic.h>
#include <kos/task.h>
#include <kos/kernel/proc.h>
#include <kos/kernel/task.h>
#include <kos/kernel/procperm.h>
#include <kos/kernel/procenv.h>

__DECL_BEGIN

//////////////////////////////////////////////////////////////////////////
// Initialize the given process permissions with root privileges,
// or as a copy those found in the given 'right' permissions.
// @return: KE_OK:       Successfully initialized the given permissions.
// @return: KE_OVERFLOW: A reference counter would have overflown.
__crit kerrno_t
kprocperm_initroot(struct kprocperm *__restrict self) {
 __u32 refcnt; int i;
 struct ktask *taskgroup;
 KTASK_CRIT_MARK
 kassertobj(self);
 kobject_init(self,KOBJECT_MAGIC_PROCPERM);
 taskgroup = ktask_zero(); do {
  refcnt = katomic_load(taskgroup->t_refcnt);
  if __unlikely(refcnt > ((__u32)-1)-KSANDBOX_BARRIER_COUNT) return KE_OVERFLOW;
 } while (!katomic_cmpxch(taskgroup->t_refcnt,refcnt,refcnt+KSANDBOX_BARRIER_COUNT));
 for (i = 0; i != KSANDBOX_BARRIER_COUNT; ++i) {
  self->pp_barriers[i] = taskgroup; /*< Inherit reference. */
 }
 self->pp_priomin      = KTASKPRIO_MIN;
 self->pp_priomax      = KTASKPRIO_MAX;
 self->pp_namemax      = (size_t)-1;
 self->pp_pipemax      = (size_t)-1;
 self->pp_thrdmax      = (size_t)-1;
 self->pp_state        = KPROCSTATE_FLAG_NONE;
 memset(self->pp_flags,0xff,sizeof(self->pp_flags)); /*< Set all permissions for root tasks. */
 return KE_OK;
}
__crit kerrno_t
kprocperm_initcopy(struct kprocperm *__restrict self,
                   struct kprocperm const *__restrict right) {
 kerrno_t error; int i;
 struct ktask *task;
 kassertobj(self);
 kassert_kprocperm(right);
 kobject_init(self,KOBJECT_MAGIC_PROCPERM);
 for (i = 0; i != KSANDBOX_BARRIER_COUNT; ++i) {
  task = katomic_load(right->pp_barriers[i]);
  kassert_ktask(task);
  error = ktask_incref(task);
  if __unlikely(KE_ISERR(error)) {
   while (i--) ktask_decref(self->pp_barriers[i]);
   return error;
  }
  self->pp_barriers[i] = task;
 }
 self->pp_priomin = katomic_load(right->pp_priomin);
 self->pp_priomax = katomic_load(right->pp_priomax);
 if __unlikely(self->pp_priomax < self->pp_priomin) {
  /* Could theoretically happen due to race conditions... */
  self->pp_priomax = self->pp_priomin;
 }
#if KCONFIG_HAVE_TASK_NAMES
 self->pp_namemax = katomic_load(right->pp_namemax);
#endif
 self->pp_pipemax = katomic_load(self->pp_pipemax);
 self->pp_thrdmax = katomic_load(self->pp_thrdmax);
 for (i = 0; i != KPERM_FLAG_GROUPCOUNT; ++i) {
  self->pp_flags[i] = katomic_load(right->pp_flags[i]);
 }
 self->pp_state = katomic_load(self->pp_state);
 return KE_OK;
}

//////////////////////////////////////////////////////////////////////////
// Destroy a previously initializes process permissions structure.
__crit void kprocperm_quit(struct kprocperm *__restrict self) {
 int i;
 kassert_kprocperm(self);
 for (i = 0; i != KSANDBOX_BARRIER_COUNT; ++i) {
  ktask_decref(self->pp_barriers[i]);
 }
}


//////////////////////////////////////////////////////////////////////////
// Returns the highest-level task associated with any of the given barriers.
// WARNING: This function is not thread-safe and does not return a reference.
//          The caller is responsible to implement a locking mechanism.
// EXCEPTION: If only one barrier is referred to, this function always returns
//            a stable pointer, though that pointer cannot be safely
//            dereferenced unless the caller is holding the same lock
//            used to guard multi-barrier calls.
// @return: * : The highest-level task contained with all given barriers.
__nomp struct ktask *
kprocperm_getmaxbarrier(struct kprocperm const *__restrict self,
                        ksandbarrier_t barrier) {
 struct ktask *reqbarrs[KSANDBOX_BARRIER_COUNT];
 struct ktask **iter,**end,*result;
 kassert_kprocperm(self);
#if 0
 /* Simple case: Single/no barrier */
 switch (barrier&0xf) {
  case KSANDBOX_BARRIER_NOSETMEM:  return katomic_load(self->pp_smbarrier);
  case KSANDBOX_BARRIER_NOGETMEM:  return katomic_load(self->pp_gmbarrier);
  case KSANDBOX_BARRIER_NOSETPROP: return katomic_load(self->pp_spbarrier);
  case KSANDBOX_BARRIER_NOGETPROP: return katomic_load(self->pp_gpbarrier);
  case KSANDBOX_BARRIER_NONE:      return ktask_self();
  default: break;
 }
#endif
 /* Complicated case: multiple barriers requested */
 end = reqbarrs;
 /* Collect all requested barriers */
 if (barrier&KSANDBOX_BARRIER_NOSETMEM)  *end++ = self->pp_smbarrier;
 if (barrier&KSANDBOX_BARRIER_NOGETMEM)  *end++ = self->pp_gmbarrier;
 if (barrier&KSANDBOX_BARRIER_NOSETPROP) *end++ = self->pp_spbarrier;
 if (barrier&KSANDBOX_BARRIER_NOGETPROP) *end++ = self->pp_gpbarrier;
 assert(end != reqbarrs); iter = reqbarrs;
 assert(iter != end);     result = *iter++;
 /* Select the barrier with the lowest prevalence.
  * HINT: This is the part that's not thread-safe. */
 for (; iter != end; ++iter) {
  if (*iter != result && ktask_ischildof(*iter,result)) result = *iter;
 }
 return result;
}


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
__crit kerrno_t
kprocperm_setbarrier(struct kprocperm *__restrict self,
                     struct ktask *__restrict barrier,
                     ksandbarrier_t mode) {
 __u32 old_refs,max_refs,more_refs = 0;
 KTASK_CRIT_MARK
 kassert_kprocperm(self);
 kassert_ktask(barrier);
 if (mode&KSANDBOX_BARRIER_NOSETMEM)  ++more_refs;
 if (mode&KSANDBOX_BARRIER_NOGETMEM)  ++more_refs;
 if (mode&KSANDBOX_BARRIER_NOSETPROP) ++more_refs;
 if (mode&KSANDBOX_BARRIER_NOGETPROP) ++more_refs;
 if __unlikely(!more_refs) return KE_OK; /*< Nothing to do here... */
 max_refs = ((__u32)-1)-more_refs;
 do { /* Add the new references to the counter. */
  old_refs = katomic_load(barrier->t_refcnt);
  if (old_refs > max_refs) return KE_OVERFLOW;
 } while (!katomic_cmpxch(barrier->t_refcnt,old_refs,old_refs+new_refs));
 if (mode&KSANDBOX_BARRIER_NOSETMEM)  ktask_decref(katomic_xch(self->pp_smbarrier,barrier));
 if (mode&KSANDBOX_BARRIER_NOGETMEM)  ktask_decref(katomic_xch(self->pp_gmbarrier,barrier));
 if (mode&KSANDBOX_BARRIER_NOSETPROP) ktask_decref(katomic_xch(self->pp_spbarrier,barrier));
 if (mode&KSANDBOX_BARRIER_NOGETPROP) ktask_decref(katomic_xch(self->pp_gpbarrier,barrier));
 return KE_OK;
}
#endif

#ifndef __INTELLISENSE__
#define GET
#include "procperm-getset.c.inl"
#define SET
#include "procperm-getset.c.inl"
#define XCH
#include "procperm-getset.c.inl"
#endif

__DECL_END

#endif /* !__KOS_KERNEL_PROCPERM_C__ */

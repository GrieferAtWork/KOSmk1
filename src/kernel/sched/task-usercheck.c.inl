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
#ifndef __KOS_KERNEL_SCHED_TASK_USERCHECK_C_INL__
#define __KOS_KERNEL_SCHED_TASK_USERCHECK_C_INL__ 1

#include <kos/compiler.h>
#include <kos/kernel/task.h>
#include <kos/kernel/proc.h>

/* Task operations that perform permission check. */
__DECL_BEGIN

kerrno_t _ktask_terminate(struct ktask *__restrict self, void *exitcode) {
 kassert_ktask(self);
 if __unlikely(!ktask_accesssp(self)) return KE_ACCES;
 return _ktask_terminate_k(self,exitcode);
}
kerrno_t ktask_terminate(struct ktask *__restrict self, void *exitcode) {
 kassert_ktask(self);
 if __unlikely(!ktask_accesssp(self)) return KE_ACCES;
 return ktask_terminate_k(self,exitcode);
}
kerrno_t ktask_terminateex(struct ktask *__restrict self,
                           void *exitcode, __ktaskopflag_t mode) {
 kassert_ktask(self);
 if __unlikely(!ktask_accesssp(self)) return KE_ACCES;
 return ktask_terminateex_k(self,exitcode,mode);
}


kerrno_t ktask_suspend(struct ktask *__restrict self) {
 kassert_ktask(self);
 if __unlikely(!ktask_accesssp(self)) return KE_ACCES;
 return ktask_suspend_k(self);
}
kerrno_t ktask_resume(struct ktask *__restrict self) {
 kassert_ktask(self);
 if __unlikely(!ktask_accesssp(self)) return KE_ACCES;
 return ktask_resume_k(self);
}
kerrno_t ktask_suspend_r(struct ktask *__restrict self) {
 kassert_ktask(self);
 if __unlikely(!ktask_accesssp(self)) return KE_ACCES;
 return ktask_suspend_kr(self);
}
kerrno_t ktask_resume_r(struct ktask *__restrict self) {
 kassert_ktask(self);
 if __unlikely(!ktask_accesssp(self)) return KE_ACCES;
 return ktask_resume_kr(self);
}

kerrno_t ktask_setpriority(struct ktask *self, ktaskprio_t v) {
 struct kproc *proc; struct ktask *accessor = ktask_self();
 if __unlikely(!ktask_accesssp_ex(self,accessor)) return KE_ACCES;
 proc = ktask_getproc(accessor);
 if (v < kprocperm_getpriomin(kproc_getperm(proc))) {
  /* Check special case: KTASK_PRIORITY_SUSPENDED */
  if (v != KTASK_PRIORITY_SUSPENDED) return KE_RANGE;
  if (!kproc_hasflag(proc,KPERM_FLAG_PRIO_SUSPENDED)) return KE_ACCES;
 } else if (v > kprocperm_getpriomax(kproc_getperm(proc))) {
  /* Check special case: KTASK_PRIORITY_REALTIME */
  if (v != KTASK_PRIORITY_REALTIME) return KE_RANGE;
  if (!kproc_hasflag(proc,KPERM_FLAG_PRIO_REALTIME)) return KE_ACCES;
 }
 ktask_setpriority_k(self,v);
 return KE_OK;
}

struct ktask *ktask_parent(struct ktask const *__restrict self) {
 kassert_ktask(self);
 if (self == kproc_getgpbarrier(kproc_self()))
  return (struct ktask *)self;
 return ktask_parent_k(self);
}
size_t ktask_getparid(struct ktask const *__restrict self) {
 kassert_ktask(self);
 if (self == kproc_getgpbarrier(kproc_self())) return 0;
 return ktask_getparid_k(self);
}

kerrno_t ktask_setalarm(struct ktask *__restrict self,
                        struct timespec const *__restrict abstime,
                        struct timespec *__restrict oldabstime) {
 if __unlikely(!ktask_accesssp(self)) return KE_ACCES;
 return ktask_setalarm_k(self,abstime,oldabstime);
}


__DECL_END

#endif /* !__KOS_KERNEL_SCHED_TASK_USERCHECK_C_INL__ */

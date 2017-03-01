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
#ifndef __SCHED_C__
#define __SCHED_C__ 1

#include <errno.h>
#include <sched.h>
#include <kos/compiler.h>
#include <sys/types.h>
#include <kos/errno.h>
#ifdef __KERNEL__
#include <kos/timespec.h>
#include <kos/kernel/proc.h>
#else
#include <sys/mman.h>
#include <kos/task.h>
#endif

__DECL_BEGIN

#undef sched_yield
__public int sched_yield(void) {
 ktask_yield();
 return 0;
}

__public int sched_get_priority_max(int __unused(policy)) {
#ifdef __KERNEL__
 return (int)katomic_load(kproc_self()->p_perm.ts_priomax);
#else
 return 20; // TODO: Lookup value in sandbox
#endif
}
__public int sched_get_priority_min(int __unused(policy)) {
#ifdef __KERNEL__
 return (int)katomic_load(kproc_self()->p_perm.ts_priomin);
#else
 return -20; // TODO: Lookup value in sandbox
#endif
}
__public int sched_getscheduler(pid_t pid) {
 return SCHED_RR;
}
__public int sched_getparam(pid_t pid, struct sched_param *param) {
#ifdef __KERNEL__
 __ref struct kproc *proc; struct ktask *roottask;
 kerrno_t error; kassertobj(param);
 KTASK_CRIT_BEGIN
 proc = kproclist_getproc(pid);
 if __unlikely(!proc) { error = KE_INVAL; goto end; }
 roottask = kproc_getroottask(proc);
 kproc_decref(proc);
 if __unlikely(!roottask) { error = KE_DESTROYED; goto end; }
 param->sched_priority = ktask_getpriority(roottask);
 ktask_decref(roottask);
end:
 KTASK_CRIT_END
 return error;
#else
 ktaskprio_t prio; int error;
 kproc_t proc = kproc_openpid(pid);
 if __unlikely(KE_ISERR(proc)) {
  __set_errno(-proc);
  return -1;
 }
 error = ktask_getpriority(proc,&prio);
 param->sched_priority = prio;
 kfd_close(proc);
 if __unlikely(KE_ISERR(error)) {
  __set_errno(-proc);
  return -1;
 }
 return error;
#endif
}
__public int sched_setparam(pid_t pid, struct sched_param const *param) {
#ifdef __KERNEL__
 struct kproc *proc; struct ktask *roottask;
 kerrno_t error; kassertobj(param);
 KTASK_CRIT_BEGIN
 proc = kproclist_getproc(pid);
 if __unlikely(!proc) { error = KE_INVAL; goto end; }
 roottask = kproc_getroottask(proc);
 kproc_decref(proc);
 if __unlikely(!roottask) { error = KE_DESTROYED; goto end; }
 error = ktask_setpriority(roottask,param->sched_priority);
 ktask_decref(roottask);
end:
 KTASK_CRIT_END
 return error;
#else
 int error;
 kproc_t proc = kproc_openpid(pid);
 if __unlikely(KE_ISERR(proc)) {
  __set_errno(-proc);
  return -1;
 }
 error = ktask_setpriority(proc,param->sched_priority);
 kfd_close(proc);
 if __unlikely(KE_ISERR(error)) {
  __set_errno(-proc);
  return -1;
 }
 return error;
#endif
}

__public int sched_rr_get_interval(pid_t pid, struct timespec *interval) {
 // TODO: This can be calculated using jiffies
 // >> Put in something that makes sense...
 (void)pid;
 interval->tv_sec = 0;
 interval->tv_nsec = 100;
 return 0;
}
__public int sched_setscheduler(pid_t pid, int __unused(policy),
                                struct sched_param const *param) {
 return sched_setparam(pid,param);
}

__DECL_END

#endif /* !__SCHED_C__ */

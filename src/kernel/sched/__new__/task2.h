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
#ifndef __KOS_KERNEL_TASK2_H__
#define __KOS_KERNEL_TASK2_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/attr.h>
#include <kos/errno.h>
#include <kos/compiler.h>
#include <kos/kernel/features.h>
#include <kos/kernel/object.h>
#include <kos/kernel/types.h>
#include <kos/kernel/tss.h>
#include <kos/kernel/signal.h>

__DECL_BEGIN

#define KCPU2_SELF_SEGMENT    fs
#define KCPU2_SELF_SEGMENT_S "fs"

#define KCPU2_OFFSETOF_CURRENT   (KOBJECT_SIZEOFHEAD)
#define KCPU2_OFFSETOF_SELF      (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__)
#define KCPU2_OFFSETOF_LOCKS     (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__*2)
#define KCPU2_OFFSETOF_FLAGS     (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__*2+1)
#define KCPU2_OFFSETOF_ID        (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__*2+2)
#define KCPU2_OFFSETOF_LAPICVER  (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__*2+3)
#define KCPU2_OFFSETOF_SEGID     (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__*2+4)
#define KCPU2_OFFSETOF_SLEEPING  (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__*2+8)
#define KCPU2_OFFSETOF_NRUNNING  (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__*3+8)
#define KCPU2_OFFSETOF_NSLEEPING (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__*3+8+__SIZEOF_SIZE_T__)
#define KCPU2_OFFSETOF_TSS       (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__*3+8+__SIZEOF_SIZE_T__*2)


#if defined(__i386__)
/* Try to reduce stack overhead during scheduler calls. */
#define __SCHED_CALL __fastcall
#else
#define __SCHED_CALL
#endif

#define KOBJECT_MAGIC_TASK2     0x7A5F     /*< TASK. */
#define KOBJECT_MAGIC_TASKLIST2 0x7A5F7157 /*< TASKLIST. */

#define KTASK_STACK_SIZE_DEF   0x4000

#ifndef __ASSEMBLY__
struct kproc;
struct ktask2;
struct kcpu2;

#define kassert_ktask2(self)     kassert_refobject(self,t_refcnt,KOBJECT_MAGIC_TASK2)
#define kassert_ktasklist2(self) kassert_object(self,KOBJECT_MAGIC_TASKLIST2)

#define KTASK2_OFFSETOF_USERREGS  (KOBJECT_SIZEOFHEAD)
#define KTASK2_OFFSETOF_USERCR3   (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__)
#define KTASK2_OFFSETOF_CPU       (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__*2)
#define KTASK2_OFFSETOF_PROC      (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__*3)
#define KTASK2_OFFSETOF_TIMEOUT   (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__*4)
struct ktask2 {
 KOBJECT_HEAD
 __user struct kirq_userregisters *t_userregs; /*< [?..?] When not in the foreground, a user-mapped pointer to a backup of all registers (Otherwise undefined). */
 __kernel struct kpagedir         *t_usercr3;  /*< [?..?] When not in the foreground, the physical address of the page directory associated with this task. */
 __atomic struct kcpu2            *t_cpu;      /*< [0..1][lock(KTASK2_LOCK_CPU)] The CPU that this task is either sleeping, or running under. */
 struct kproc                     *t_proc;     /*< [1..1] The process associated with this task. */
#define KTASK2_LOCK_RESCHED        0x01
#define KTASK2_LOCK_CPU            0x02 /*< [order(> KTASK2_LOCK_RESCHED,> t_cpu:KCPU2_LOCK_TASKS)] Write-lock required for setting 't_cpu'. */
 __atomic __u8                     t_locks;    /*< 8 atomic spinlocks used to guard various parts of this task. */
#define KTASK2_STATE_RUNNING       0x00 /*< Task is currently running. */
#define KTASK2_STATE_SUSPENDED     0x01 /*< Task is suspended by use of ktask_suspend_k(), or was created suspended. */
#define KTASK2_STATE_TERMINATED    0x03 /*< Task has finished, or was terminated (NOTE: Setting this state may not be reverted). */
#define KTASK2_STATE_WAITING       0x11 /*< Task is queued to wake on a locking primitive. */
#define KTASK2_STATE_WAITINGTMO    0x10 /*< Task is queued to wake on a locking primitive, or time out after a given amount of time has passed. (NOTE: Task is still scheduled) */
#define KTASK2_STATE_ISACTIVE(state)   ((state)==KTASK_STATE_RUNNING)
#define KTASK2_STATE_ISWAITING(state) (((state)&0x10)!=0)
#define KTASK2_STATE_HASCPU(state)    (((state)&0x01)==0) /*< Task is associated with an explicit CPU */
 __atomic __u8                     t_state;    /*< The task's current state (One of 'KTASK_STATE_*') */
#define KTASK2_FLAG_NONE           0x0000
#define KTASK2_FLAG_USERTASK       0x0001 /*< [const] This task is running in ring 3. */
#define KTASK2_FLAG_OWNSUSTACK     0x0002 /*< [const] t_ustackvp is free'd upon task destruction (Requires the 'KTASK_FLAG_USERTASK' flag). */
#define KTASK2_FLAG_MALLKSTACK     0x0004 /*< [const] The kernel stack is allocated through malloc, instead of mmap (NOTE: Only allowed for kernel tasks; aka. when 'KTASK_FLAG_USERTASK' isn't set). */
#define KTASK2_FLAG_CRITICAL       0x0008 /*< [atomic|w:ktask_self()] This task is currently running critical code, and may not be terminated directly. */
#define KTASK2_FLAG_TIMEDOUT       0x0010 /*< [atomic] The task timed out. */
#define KTASK2_FLAG_ALARMED        0x0020 /*< [lock(KTASK2_LOCK_RESCHED)] The task may be running, but a 't_abstime' describes a user-defined alarm. */
#define KTASK2_FLAG_WASINTR        0x0040 /*< [lock(KTASK2_LOCK_RESCHED)] An interrupt signal was send to a critical task (While set, the signal is still pending). */
#define KTASK2_FLAG_NOINTR         0x0080 /*< [atomic|w:ktask_self()] Don't send KE_INTR errors to this task when it gets terminated while inside a critical block. */
 __u16                             t_flags;     /*< A set of 'KTASK2_FLAG_*' */
 kcpuset_t                         t_cpuset;    /*< [lock(KTASK2_LOCK_CPU)] Set of CPUs this task is allowed to run on. */
 __s16                             t_suspended; /*< [lock(KTASK2_LOCK_RESCHED)] Recursive suspension counter (When non-zero, t_state is 'KTASK2_STATE_SUSPENDED'). */
 __atomic __u32                    t_refcnt;    /*< The task's reference counter. */
 __ref struct ktask2              *t_prev;      /*< [0..1|1..1|?..?] Previous task (meaning depends on 't_state' and the associated CPU). */
 __ref struct ktask2              *t_next;      /*< [0..1|1..1|?..?] Next task (meaning depends on 't_state' and the associated CPU). */
 struct timespec                   t_timeout;   /*< [lock(KTASK2_LOCK_RESCHED|t_cpu:KCPU2_FLAG_INUSE)] Absolute point in time when the task gets rescheduled. */
 struct ksignal                    t_joinsig;   /*< Signal closed when the task terminates (Can be received to join the task). */
 void                             *t_exitcode;  /*< The exitcode of this task upon termination. */
};

__local KOBJECT_DEFINE_INCREF(ktask2_incref,struct ktask2,t_refcnt,kassert_ktask2);
__local KOBJECT_DEFINE_DECREF(ktask2_decref,struct ktask2,t_refcnt,kassert_ktask2,ktask2_destroy);

#define ktask2_islocked(self,lock)    ((katomic_load((self)->t_locks)&(lock))!=0)
#define ktask2_trylock(self,lock)     ((katomic_fetchor((self)->t_locks,lock)&(lock))==0)
#define ktask2_lock(self,lock)          KTASK_SPIN(ktask2_trylock(self,lock))
#define ktask2_unlock(self,lock)        assertef((katomic_fetchand((self)->t_locks,~(lock))&(lock))!=0,"Lock not held")
#define ktask2_iscpuallowed(self,cpu) ((self)->t_cpuset&(1 << (cpu)->c_id))

#define ktask2_iscurrcpu(task) (katomic_load((task)->t_cpu) == kcpu2_self())

//////////////////////////////////////////////////////////////////////////
// Called just before the task is rescheduled due to a timeout.
extern void __SCHED_CALL ktask2_ontimeout(struct ktask2 *__restrict self);

//////////////////////////////////////////////////////////////////////////
// Returns the control structure for the calling task.
// NOTE: Unless the calling thread is re-scheduled on a different CPU, this value
//       will not change, as IRQ preemption won't ever swap tasks to different CPUs.
#ifdef __INTELLISENSE__
extern __wunused __retnonnull struct ktask2 *ktask2_self(void);
#else
__forcelocal __wunused __constcall
__retnonnull struct ktask2 *ktask2_self(void) {
 register struct ktask2 *result;
 __asm__("mov %%" KCPU2_SELF_SEGMENT_S ":"
         __PP_STR(KCPU2_OFFSETOF_CURRENT)
         ", %0\n" : "=g" (result));
 return result;
}
#endif

//////////////////////////////////////////////////////////////////////////
// Yield execution, preferring the hinted task as new target if non-NULL.
// If NULL or if the hint-ed task isn't running, or part of a different CPU,
// simply yield execution to the next task in like for preemptive IRQ.
#ifdef __INTELLISENSE__
extern kerrno_t ktask2_yield(struct ktask2 *hint);
#else
#define ktask2_yield(hint) \
 ((__builtin_constant_p(hint) && !(hint))\
  ? ktask2_yield_nohint() : ktask2_yield_hint(hint))
extern kerrno_t __SCHED_CALL ktask2_yield_hint(struct ktask2 *hint);
extern kerrno_t __SCHED_CALL ktask2_yield_nohint(void);
#endif


struct ksigset;

//////////////////////////////////////////////////////////////////////////
// Unschedule a given task to wait for a set of signals.
extern __crit __nonnull((1)) kerrno_t __SCHED_CALL
ktask2_unschedule_waiting(struct ktask2 *__restrict self,
                          struct ksigset const *signals);
extern __crit __nonnull((1)) kerrno_t __SCHED_CALL
ktask2_unschedule_waiting_timeout(struct ktask2 *__restrict self,
                                  struct ksigset const *signals,
                                  struct timespec const *__restrict timeout);
extern __crit __nonnull((1)) kerrno_t __SCHED_CALL
ktask2_unschedule_terminate(struct ktask2 *__restrict self,
                            void *exitcode);
extern __crit __nonnull((1)) kerrno_t __SCHED_CALL
ktask2_unschedule_suspend(struct ktask2 *__restrict self);

/* Applies any scheduling changes after the last critical region is left. */
extern __nonnull((1)) void __SCHED_CALL
ktask2_unschedule_aftercrit(struct ktask *__restrict self);


__local __crit __wunused __nonnull((1)) kerrno_t __SCHED_CALL
ktask2_unschedule_ex(struct ktask2 *__restrict self, __u8 newstate,
                     void *__restrict arg, struct ksigset const *signals) {
 switch (newstate) {
  case KTASK2_STATE_SUSPENDED : return ktask2_unschedule_suspend(self);
  case KTASK2_STATE_TERMINATED: return ktask2_unschedule_terminate(self,arg);
  case KTASK2_STATE_WAITING   : return ktask2_unschedule_waiting(self,signals);
  case KTASK2_STATE_WAITINGTMO: return ktask2_unschedule_waiting_timeout(self,signals,(struct timespec *)arg);
  default: __builtin_unreachable();
 }
}





#endif /* !__ASSEMBLY__ */
__DECL_END

#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_TASK2_H__ */

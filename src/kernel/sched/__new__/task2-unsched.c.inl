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
#ifndef __KOS_KERNEL_TASK2_UNSCHED_C_INL__
#define __KOS_KERNEL_TASK2_UNSCHED_C_INL__ 1

#include <kos/config.h>
#include <kos/compiler.h>
#include <kos/kernel/task2.h>
#include <kos/kernel/debug.h>
#include <kos/kernel/sigset.h>
#include <kos/atomic.h>
#include <kos/kernel/task2-cpu.h>
#include <kos/syslog.h>

__DECL_BEGIN

// Acquires a lock to 'self:KTASK2_LOCK_CPU',
// as well as to 'self->t_cpu:KCPU2_LOCK_TASKS'
__local struct kcpu2 *
ktask2_lockcpu_and_tasks(struct ktask2 *__restrict self) {
 struct kcpu2 *result;
 kassert_ktask2(self);
 ktask2_lock(self,KTASK2_LOCK_CPU);
cpu_again:
 result = self->t_cpu;
 kassert_kcpu2(result);
 if __likely(kcpu2_trylock(result,KCPU2_LOCK_TASKS)) return result;
 ktask2_unlock(self,KTASK2_LOCK_CPU);
 kcpu2_lock(result,KCPU2_LOCK_TASKS);
 ktask2_lock(self,KTASK2_LOCK_CPU);
 if __likely(self->t_cpu == result) return result;
 kcpu2_unlock(result,KCPU2_LOCK_TASKS);
 goto cpu_again;
}

__local kerrno_t __SCHED_CALL
ktask2_do_unschedule_self(struct ktask2 *__restrict self,
                          int register_timeout, int was_terminated) {
 assert(self == ktask2_self());

}


__local kerrno_t __SCHED_CALL ktask2_do_unschedule_unlocked_term(struct ktask2 *__restrict self);
__local kerrno_t __SCHED_CALL ktask2_do_unschedule_unlocked_time(struct ktask2 *__restrict self);
__local kerrno_t __SCHED_CALL ktask2_do_unschedule_unlocked(struct ktask2 *__restrict self);


__crit kerrno_t __SCHED_CALL
ktask2_unschedule_waiting(struct ktask2 *__restrict self,
                          struct ksigset const *signals) {
 __u8 oldstate; kerrno_t error;
 KTASK_CRIT_MARK
 kassert_ktask2(self);
 kassertobjnull(signals);
 for (;;) {
  oldstate = katomic_load(self->t_state);
  if (oldstate == KTASK2_STATE_TERMINATED) {
   /* TODO: Unlock 'signals'. */
   return KE_DESTROYED;
  }
  ktask2_lock(self,KTASK2_LOCK_RESCHED);
  if (katomic_cmpxch(self->t_state,oldstate,KTASK2_STATE_WAITING)) break;
  ktask2_unlock(self,KTASK2_LOCK_RESCHED);
 }
 /* TODO: Registers 'signals'. */
 switch (oldstate) {
  case KTASK2_STATE_RUNNING:
   error = ktask2_do_unschedule_unlocked(self);
   break;
  case KTASK2_STATE_SUSPENDED:
  case KTASK2_STATE_WAITING:
  case KTASK2_STATE_WAITINGTMO:
   /* Simple case: No need to re-schedule. - The task already is. */
   assert(self != ktask2_self());
   error = KS_UNCHANGED;
   ktask2_unlock(self,KTASK2_LOCK_RESCHED);
   break;
  default: __compiler_unreachable();
 }
 return error;
}

__crit kerrno_t __SCHED_CALL
ktask2_unschedule_waiting_timeout(struct ktask2 *__restrict self,
                                  struct ksigset const *signals,
                                  struct timespec const *__restrict timeout) {
 __u8 oldstate; kerrno_t error;
 KTASK_CRIT_MARK
 kassert_ktask2(self);
 kassertobjnull(signals);
 kassertobj(timeout);
 for (;;) {
  oldstate = katomic_load(self->t_state);
  if (oldstate == KTASK2_STATE_TERMINATED) {
   return KE_DESTROYED;
  }
  ktask2_lock(self,KTASK2_LOCK_RESCHED);
  if (katomic_cmpxch(self->t_state,oldstate,KTASK2_STATE_WAITINGTMO)) break;
  ktask2_unlock(self,KTASK2_LOCK_RESCHED);
 }
 switch (oldstate) {
  case KTASK2_STATE_RUNNING:
   error = ktask2_do_unschedule_unlocked(self);
   break;
  case KTASK2_STATE_SUSPENDED:
  case KTASK2_STATE_WAITING:
  case KTASK2_STATE_WAITINGTMO:
   assert(self != ktask2_self());
   assert(0); /* TODO: Scheduled/Set/Update a timeout. */
   ktask2_unlock(self,KTASK2_LOCK_RESCHED);
   break;
  default: __compiler_unreachable();
 }
 return error;
}

__crit kerrno_t __SCHED_CALL
ktask2_unschedule_terminate(struct ktask2 *__restrict self,
                            void *exitcode) {
 __u8 oldstate; kerrno_t error;
 KTASK_CRIT_MARK
 kassert_ktask2(self);
 for (;;) {
  oldstate = katomic_load(self->t_state);
  if (oldstate == KTASK2_STATE_TERMINATED) return KE_DESTROYED;
  ktask2_lock(self,KTASK2_LOCK_RESCHED);
  if (self->t_flags&KTASK2_FLAG_CRITICAL) {
   if (self->t_flags&KTASK2_FLAG_WASINTR) {
    error = KE_DESTROYED;
   } else {
    self->t_flags |= KTASK2_FLAG_WASINTR;
    error = KE_OK;
   }
   /* Special case: Critical task. */
  }
  if (katomic_cmpxch(self->t_state,oldstate,KTASK2_STATE_TERMINATED)) break;
  ktask2_unlock(self,KTASK2_LOCK_RESCHED);
 }
 /* TODO: Registers 'signals'. */
 switch (oldstate) {
  case KTASK2_STATE_RUNNING:
   error = ktask2_do_unschedule_unlocked(self);
   break;
  case KTASK2_STATE_SUSPENDED:
  case KTASK2_STATE_WAITING:
  case KTASK2_STATE_WAITINGTMO:
   assert(self != ktask2_self());
   assert(0); /* TODO: Scheduled/Set/Update a timeout. */
   ktask2_unlock(self,KTASK2_LOCK_RESCHED);
   break;
  default: __compiler_unreachable();
 }
 return error;
}

__crit kerrno_t __SCHED_CALL
ktask2_unschedule_suspend(struct ktask2 *__restrict self) {
 KTASK_CRIT_MARK
 kassert_ktask2(self);

 return KE_NOSYS;
}


/* Applies any scheduling changes after the last critical region is left. */
void __SCHED_CALL
ktask2_unschedule_aftercrit(struct ktask *__restrict self) {
 kassert_ktask2(self);
}

__DECL_END

#ifndef __INTELLISENSE__
#define TERMINATE
#include "task2-unsched-impl.c.inl"
#define TIMEOUT
#include "task2-unsched-impl.c.inl"
#include "task2-unsched-impl.c.inl"
#endif

#endif /* !__KOS_KERNEL_TASK2_UNSCHED_C_INL__ */

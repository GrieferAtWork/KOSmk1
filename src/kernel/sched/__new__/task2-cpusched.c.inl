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
#ifndef __KOS_KERNEL_TASK2_CPUSCHED_C_INL__
#define __KOS_KERNEL_TASK2_CPUSCHED_C_INL__ 1

#include <kos/compiler.h>
#include <kos/config.h>
#include <kos/kernel/debug.h>
#include <kos/kernel/task2-cpu.h>
#include <kos/kernel/task2.h>
#include <kos/timespec.h>

__DECL_BEGIN


__crit __size_t __SCHED_CALL
kcpu2_wakesleepers_unlocked(struct kcpu2 *__restrict self,
                            struct timespec const *__restrict tmnow) {
 __size_t result = 0;
 struct ktask2 *task,*first_task,*last_task;
 kassert_kcpu2(self);
 assert(kcpu2_islocked(self,KCPU2_LOCK_TASKS) ||
        kcpu2_inuse_notrunning(self));
 kassertobj(tmnow);
 assert((self->c_nsleeping == 0) == (self->c_sleeping == NULL));
 assert((self->c_nrunning == 0) == (self->c_current == &self->c_idle));
 kassert_ktask2(self->c_current);
 task = self->c_sleeping;
 while (task && __timespec_cmple(&task->t_timeout,tmnow)) {
  ktask2_ontimeout(task);
  ++result,task = task->t_next;
 }
 if (result) {
  /* Reschedule these tasks. */
  first_task = self->c_sleeping;
  kassert_ktask2(first_task);
  if ((self->c_sleeping = task) != NULL) {
   last_task = task->t_prev;
  } else {
   last_task = task;
   while (last_task->t_next) last_task = last_task->t_next;
  }
  kassert_ktask2(last_task);
  assert(result <= self->c_nsleeping);
  self->c_nsleeping -= result;
  self->c_nrunning  += result;
  if (self->c_current == &self->c_idle) {
   /* Replace the IDLE task. */
   first_task->t_prev = last_task;
   last_task->t_next = first_task;
  } else {
   /* Insert before the current task. */
   first_task->t_prev = self->c_current->t_prev;
   kassert_ktask2(first_task->t_prev);
   first_task->t_prev->t_next = first_task;
   last_task->t_next = self->c_current;
   last_task->t_next->t_prev = last_task;
  }
  /* Overwrite the ring position with the first task. */
  self->c_current = first_task;
 }
 return result;
}

__crit void __SCHED_CALL
kcpu2_rotaterunning_unlocked(struct kcpu2 *__restrict self) {
 kassert_kcpu2(self);
 assert(kcpu2_islocked(self,KCPU2_LOCK_TASKS) ||
        kcpu2_inuse_notrunning(self));
 kassert_ktask2(self->c_current);
 /* TODO: Priorities. */
 self->c_current = self->c_current->t_next;
 kassert_ktask2(self->c_current);
}


__DECL_END

#endif /* !__KOS_KERNEL_TASK2_CPUSCHED_C_INL__ */

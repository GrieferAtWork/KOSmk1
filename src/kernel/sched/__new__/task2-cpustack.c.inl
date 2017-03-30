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
#ifndef __KOS_KERNEL_TASK2_CPUSTACK_C_INL__
#define __KOS_KERNEL_TASK2_CPUSTACK_C_INL__ 1

#include <assert.h>
#include <kos/compiler.h>
#include <kos/config.h>
#include <kos/kernel/debug.h>
#include <kos/kernel/task2-cpu.h>
#include <kos/kernel/task2.h>
#include <stddef.h>

__DECL_BEGIN

#define YES      1
#define NO       0
#define MAYBE  (-1)
#ifdef __DEBUG__
static int
kcpu2_debug_has_scheduled_task(struct kcpu2 *self,
                               struct ktask2 *task) {
 struct ktask2 *iter = self->c_current;
 for (;;) {
  kassert_ktask2(iter);
  if (iter == task) return YES;
  if (iter == iter->t_next) break;
  iter = iter->t_next;
 }
 return NO;
}
static int
kcpu2_debug_has_sleeping_task(struct kcpu2 *self,
                              struct ktask2 *task) {
 struct ktask2 *iter = self->c_sleeping;
 while (iter) {
  kassert_ktask2(iter);
  if (iter == task) return YES;
  iter = iter->t_next;
 }
 return NO;
}
#define HAS_SCHEDULED_TASK(task) kcpu2_debug_has_scheduled_task(self,task)
#define HAS_SLEEPING_TASK(task)  kcpu2_debug_has_sleeping_task(self,task)
#else
#define HAS_SCHEDULED_TASK(task) MAYBE
#define HAS_SLEEPING_TASK(task)  MAYBE
#endif

__crit void __SCHED_CALL
kcpu2_addcurrent_front_unlocked(struct kcpu2 *__restrict self,
                                __ref struct ktask2 *__restrict task) {
 KTASK_CRIT_MARK
 kassert_kcpu2(self);
 kassert_ktask2(task);
 assert(kcpu2_inuse_notrunning(self) ||
       (self == kcpu2_self() &&
        kcpu2_islocked(self,KCPU2_LOCK_TASKS)));
 assert(task->t_cpu == self);
 assert(HAS_SCHEDULED_TASK(task) != YES);
 assert(HAS_SLEEPING_TASK(task) != YES);
 assert((self->c_nrunning != 0) || (self->c_current == &self->c_idle));
 /* Insert the given task at the front. */
 if (self->c_current == &self->c_idle) {
  task->t_next = task;
  task->t_prev = task;
 } else {
  task->t_prev = self->c_current->t_prev;
  task->t_next = self->c_current;
  kassert_ktask2(task->t_prev);
  kassert_ktask2(task->t_next);
  task->t_prev->t_next = task;
  task->t_next->t_prev = task;
 }
 self->c_current = task;
 ++self->c_nrunning;
}

__crit void __SCHED_CALL
kcpu2_addcurrent_next_unlocked(struct kcpu2 *__restrict self,
                               __ref struct ktask2 *__restrict task) {
 KTASK_CRIT_MARK
 kassert_kcpu2(self);
 kassert_ktask2(task);
 assert(kcpu2_inuse_notrunning(self) ||
        kcpu2_islocked(self,KCPU2_LOCK_TASKS));
 assert(task->t_cpu == self);
 assert(HAS_SCHEDULED_TASK(task) != YES);
 assert(HAS_SLEEPING_TASK(task) != YES);
 assert((self->c_nrunning != 0) || (self->c_current == &self->c_idle));
 /* Insert the given task after the current. */
 task->t_prev = self->c_current;
 kassert_ktask2(task->t_prev);
 task->t_next = self->c_current->t_next;
 kassert_ktask2(task->t_next);
 task->t_prev->t_next = task;
 task->t_next->t_prev = task;
 ++self->c_nrunning;
 /* NOTE: If 'self == kcpu2_self()', we could technically
  *       check for c_current to match c_idle and unlink
  *       it if they are. - But we don't have to... */
}

__crit void __SCHED_CALL
kcpu2_addcurrent_back_unlocked(struct kcpu2 *__restrict self,
                               __ref struct ktask2 *__restrict task) {
 KTASK_CRIT_MARK
 kassert_kcpu2(self);
 kassert_ktask2(task);
 assert(kcpu2_inuse_notrunning(self) ||
        kcpu2_islocked(self,KCPU2_LOCK_TASKS));
 assert(task->t_cpu == self);
 assert(HAS_SCHEDULED_TASK(task) != YES);
 assert(HAS_SLEEPING_TASK(task) != YES);
 assert((self->c_nrunning != 0) || (self->c_current == &self->c_idle));
 /* Insert the given task at the end (aka. before the first without moving it). */
 task->t_next = self->c_current;
 kassert_ktask2(task->t_next);
 task->t_prev = self->c_current->t_prev;
 kassert_ktask2(task->t_prev);
 task->t_next->t_prev = task;
 task->t_prev->t_next = task;

 ++self->c_nrunning;
}

__crit void __SCHED_CALL
kcpu2_addcurrent_idle_unlocked(struct kcpu2 *__restrict self) {
 kassert_kcpu2(self);
 assert(kcpu2_inuse_notrunning(self) ||
        kcpu2_islocked(self,KCPU2_LOCK_TASKS));
 assert(self->c_current != &self->c_idle);
 self->c_idle.t_prev = self->c_current;
 self->c_idle.t_next = self->c_current->t_next;
 self->c_idle.t_next->t_prev = &self->c_idle;
 self->c_current    ->t_next = &self->c_idle;
}

__crit void __SCHED_CALL
kcpu2_delcurrent_unlocked(struct kcpu2 *__restrict self,
                          __ref struct ktask2 *__restrict task) {
 KTASK_CRIT_MARK
 kassert_kcpu2(self);
 kassert_ktask2(task);
 assert(kcpu2_inuse_notrunning(self) ||
       (kcpu2_islocked(self,KCPU2_LOCK_TASKS) &&
       (task != self->c_current || self == kcpu2_self())));
 assert(task->t_cpu == self);
 assert(task->t_next != task || task == self->c_current);
 kassert_ktask2(task->t_prev);
 kassert_ktask2(task->t_next);
 assert(task->t_prev->t_next == task);
 assert(task->t_next->t_prev == task);
 assert(HAS_SCHEDULED_TASK(task) != NO);
 assert(HAS_SLEEPING_TASK(task) != YES);
 assert(self->c_nrunning != 0);
 if (task == self->c_current) {
  /* Special case: First task (Switch to the next task).
   * NOTE: In this case, 'self' must be 'kcpu2_self()' */
  self->c_current = task->t_next;
  if (self->c_current == task) {
   /* Even more special case: Only task (Must switch to IDLE task). */
   self->c_idle.t_prev = &self->c_idle;
   self->c_idle.t_next = &self->c_idle;
   self->c_current = &self->c_idle;
  } else {
   /* Proceed by doing a regular unlink. */
   goto unlink_normal;
  }
 } else {
unlink_normal:
  assert(task->t_prev != task);
  assert(task->t_next != task);
  /* Simple case: The task is located somewhere deep within the task chain. */
  task->t_prev->t_next = task->t_next;
  task->t_next->t_prev = task->t_prev;
 }
 --self->c_nrunning;
}


__crit void __SCHED_CALL
kcpu2_addsleeping_unlocked(struct kcpu2 *__restrict self,
                           __ref struct ktask2 *__restrict task) {
 struct ktask2 **pdst,*task_before;
 KTASK_CRIT_MARK
 kassert_kcpu2(self);
 kassert_ktask2(task);
 assert(kcpu2_inuse_notrunning(self) ||
       (kcpu2_islocked(self,KCPU2_LOCK_TASKS) &&
       (task != self->c_current || self == kcpu2_self())));
 assert(task->t_cpu == self);
 assert(HAS_SCHEDULED_TASK(task) != YES);
 assert(HAS_SLEEPING_TASK(task) != YES);
 assert((self->c_nsleeping != 0) == (self->c_sleeping != NULL));
 assert(!self->c_sleeping || !self->c_sleeping->t_prev);
 pdst = &self->c_sleeping;
 /* Search for an appropriate insert position. */
 while ((task_before = *pdst) != NULL &&
        __timespec_cmplo(&task->t_timeout,&task_before->t_timeout)
        ) pdst = &task_before->t_next;
 if ((task->t_prev = task_before) != NULL) {
  task->t_next = task_before->t_next;
  if (task->t_next) task->t_next->t_prev = task;
  task_before->t_next = task;
 } else {
  /* Append at the end */
  task->t_next = NULL;
 }
 *pdst = task;
 ++self->c_nsleeping;
}

__crit void __SCHED_CALL
kcpu2_addsleeping_all_unlocked(struct kcpu2 *__restrict self,
                               __ref struct ktask2 *task) {
 struct ktask2 *next;
 KTASK_CRIT_MARK
 kassert_kcpu2(self);
 /* TODO: The given chain of tasks is already sorted.
  *     - This can be optimized the way mergesort works! */
 while (task) {
  next = task->t_next;
  ktask2_lock(task,KTASK2_LOCK_CPU);
  task->t_cpu = self;
  ktask2_unlock(task,KTASK2_LOCK_CPU);
  kcpu2_addsleeping_unlocked(self,task);
  task = next;
 }
}


__crit void __SCHED_CALL
kcpu2_delsleeping_unlocked(struct kcpu2 *__restrict self,
                           __ref struct ktask2 *__restrict task) {
 KTASK_CRIT_MARK
 kassert_kcpu2(self);
 kassert_ktask2(task);
 assert(kcpu2_inuse_notrunning(self) ||
        kcpu2_islocked(self,KCPU2_LOCK_TASKS));
 assert(task->t_cpu == self);
 assert((task->t_prev == NULL) == (task == self->c_sleeping));
 assert(!task->t_prev || (kassert_ktask2(task->t_prev),task->t_prev->t_next == task));
 assert(!task->t_next || (kassert_ktask2(task->t_next),task->t_next->t_prev == task));
 assert(HAS_SCHEDULED_TASK(task) != YES);
 assert(HAS_SLEEPING_TASK(task) != NO);
 assert((self->c_nsleeping != 0) == (self->c_sleeping != NULL));
 assert(!self->c_sleeping || !self->c_sleeping->t_prev);
 /* Unlink the given task from the sleeper list. */
 if (task == self->c_sleeping) self->c_sleeping = task->t_next;
 if (task->t_prev) task->t_prev->t_next = task->t_next;
 if (task->t_next) task->t_next->t_prev = task->t_prev;
 --self->c_nsleeping;
 assert((self->c_nsleeping != 0) == (self->c_sleeping != NULL));
}



__crit void __SCHED_CALL
kcpu2_inherit_tasks(struct kcpu2 *__restrict source) {
 struct kcpu2 *self = kcpu2_self();
 KTASK_CRIT_MARK
 kassert_kcpu2(self);
 kassert_kcpu2(source);
 kcpu2_lock(self,KCPU2_LOCK_TASKS);
 kcpu2_inherit_tasks_unlocked(self,source);
 kcpu2_unlock(self,KCPU2_LOCK_TASKS);
}
__crit void __SCHED_CALL
kcpu2_inherit_tasks_unlocked(struct kcpu2 *__restrict self,
                             struct kcpu2 *__restrict source) {
 struct ktask2 *first_task,*last_task,*iter;
 KTASK_CRIT_MARK
 kassert_kcpu2(self);
 kassert_kcpu2(source);
 assert(self != source);
 assert(kcpu2_inuse_notrunning(source) ||
        kcpu2_islocked(self,KCPU2_LOCK_TASKS));
 assert(self == kcpu2_self() &&
        kcpu2_islocked(self,KCPU2_LOCK_TASKS));
 assert((self->c_nrunning != 0) || (self->c_current == &self->c_idle));
 assert((self->c_nsleeping != 0) == (self->c_sleeping != NULL));
 assert((source->c_nrunning != 0) || (source->c_current == &source->c_idle));
 assert((source->c_nsleeping != 0) == (source->c_sleeping != NULL));
 iter = first_task = source->c_current;
 /* Make sure that the source's IDLE task isn't linked! */
 do {
  kassert_ktask2(iter);
  if (iter == &source->c_idle) {
   assert((iter->t_prev == iter) == (iter->t_next == iter));
   assert(!source->c_nrunning);
   /* Check for special case: The given source CPU _only_ has its IDLE task scheduled. */
   if (iter->t_prev == iter) goto inherit_sleeping;
   iter->t_prev->t_next = iter->t_next;
   iter->t_next->t_prev = iter->t_prev;
   if (iter == first_task) first_task = iter->t_next;
  }
  /* Update the CPU pointers of all tasks. */
  assert(iter->t_cpu == source);
  iter->t_cpu = self;
  iter = iter->t_next;
 } while (iter != first_task);
 kassert_ktask2(first_task);
 assert(first_task != &source->c_idle);
 assert(self->c_current->t_prev->t_next == self->c_current);
 last_task = first_task->t_prev;
 /* Append at the end (Insert before the current task) */
 self->c_current->t_prev->t_next = first_task;
 first_task->t_prev = self->c_current->t_prev;
 last_task->t_next = self->c_current;
 self->c_current->t_prev = last_task;
 self->c_nrunning += source->c_nrunning;

inherit_sleeping:
 /* Inherit sleeping tasks.
  * >> This one's a bit more complicated because we need to merge-sort them. */
 kcpu2_addsleeping_all_unlocked(self,source->c_sleeping);
 self->c_nsleeping += source->c_nsleeping;

 /* Reset the source task lists. */
 source->c_nrunning = 0;
 source->c_nsleeping = 0;
 source->c_idle.t_prev = &source->c_idle;
 source->c_idle.t_next = &source->c_idle;
 source->c_current = &source->c_idle;
 source->c_sleeping = NULL;
}

#undef HAS_SCHEDULED_TASK
#undef HAS_SLEEPING_TASK
#undef MAYBE
#undef NO
#undef YES

__DECL_END

#endif /* !__KOS_KERNEL_TASK2_CPUSTACK_C_INL__ */

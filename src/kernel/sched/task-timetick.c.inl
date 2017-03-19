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
#ifdef __INTELLISENSE__
#include "task.c"
#define UNLOCKED2
#endif

__DECL_BEGIN

/* TODO: Only unlocked2 is ever used; Remove the other one! */
#ifdef UNLOCKED2
void kcpu_ticktime_unlocked2
#else
void kcpu_ticktime_unlocked
#endif
(struct kcpu *__restrict self,
 struct timespec const *__restrict now) {
 struct ktask *iter;
 kassert_kcpu(self);
 kassertobj(now);
 assert(!karch_irq_enabled());
 assert(kcpu_islocked(self,KCPU_LOCK_SLEEP));
#ifdef UNLOCKED2
 assert(kcpu_islocked(self,KCPU_LOCK_TASKS));
#endif
 while ((iter = self->c_sleeping) != NULL) {
  kassert_ktask(iter);
  assert(!iter->t_prev);
  if (!ktask_trylock(iter,KTASK_LOCK_STATE)) break;
  assertf(iter->t_state == KTASK_STATE_WAITINGTMO,
          "Only waiting tasks should be in a CPU's sleeper list");
  assertf(iter->t_newstate == KTASK_STATE_RUNNING,
          "A task not supposed to continue running should not be sleeping");
  if (__timespec_cmpge(now,&iter->t_abstime)) {
   KTASK_ONLEAVESLEEP_TM(iter,self,now);
   /* Re-schedule this task by transferring its reference. */
   iter->t_flags |= KTASK_FLAG_TIMEDOUT;
   katomic_store(iter->t_state,KTASK_STATE_RUNNING);
   if ((self->c_sleeping = iter->t_next) != NULL) {
    kassert_ktask(self->c_sleeping);
    self->c_sleeping->t_prev = NULL;
   }
#ifndef UNLOCKED2
   kcpu_unlock(self,KCPU_LOCK_SLEEP);
   /* NOTE: 'iter' is still held alive by the
    *       reference we inherited from the sleeper list. */
   kcpu_lock(self,KCPU_LOCK_TASKS);
#endif
   assert(!iter->t_prev);
   if ((iter->t_next = self->c_current) != NULL) {
    iter->t_prev = self->c_current->t_prev;
    assert(iter->t_prev);
    iter->t_prev->t_next = iter;
    iter->t_next->t_prev = iter;
    assert(self->c_current->t_next != self->c_current);
   } else {
    iter->t_prev = iter;
    iter->t_next = iter;
    self->c_current = iter;
   }
   KTASK_ONRESCHEDULE_TM(iter,self,now);
#ifndef UNLOCKED2
   kcpu_unlock(self,KCPU_LOCK_TASKS);
   ktask_unlock(iter,KTASK_LOCK_STATE);
   kcpu_lock(self,KCPU_LOCK_SLEEP);
#else
   ktask_unlock(iter,KTASK_LOCK_STATE);
#endif
  } else {
   ktask_unlock(iter,KTASK_LOCK_STATE);
   break;
  }
 }
}
#ifdef UNLOCKED2
void kcpu_ticknow_unlocked2
#else
void kcpu_ticknow_unlocked
#endif
(struct kcpu *__restrict self) {
 struct timespec now;
 ktime_getnow(&now);
#ifdef UNLOCKED2
 kcpu_ticktime_unlocked2(self,&now);
#else
 kcpu_ticktime_unlocked(self,&now);
#endif
}

#ifdef UNLOCKED2
#undef UNLOCKED2
#endif

__DECL_END

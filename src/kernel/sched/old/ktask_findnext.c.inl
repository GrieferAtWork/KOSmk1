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
#ifndef __GOS_KERNEL_KTASK_FINDNEXT_C_INL__
#define __GOS_KERNEL_KTASK_FINDNEXT_C_INL__ 1

#include <gos/compiler.h>
#include <gos/kernel/katomic.h>
#include <gos/kernel/ktask.h>

__DECL_BEGIN

ktask_t *__ktask_chain_unlock_and_findnext_and_lock(ktask_t *self) {
 ktask_priority_t old_priority,new_priority;
 ktask_t *result = self,*next;
 do { // Check for high-priority/realtime tasks
  old_priority = katomic_load(result->t_prio);
  if (old_priority <= 0) goto no_highprio;
  new_priority = old_priority == KTASK_PRIORITY_REALTIME
   ? KTASK_PRIORITY_REALTIME : old_priority-1;
 } while (!katomic_cmpxch(result->t_prio,old_priority,new_priority));
 return result; // High-priority task
no_highprio:
 for (;;) { // Search for the next task in line (considering priorities)
  next = result->t_sched.ts_next;
  _ktask_unlock(result);
  if __unlikely(!next) break;
  result = next;
  _ktask_lock(result);
  do {
   old_priority = katomic_load(result->t_prio);
   new_priority = old_priority < 0 ? old_priority+1 : katomic_load(result->t_setprio);
  } while (!katomic_cmpxch(result->t_prio,old_priority,new_priority));
  if (new_priority != 0) continue; // Low-priority task
  return result;
 }
 return NULL;
}

ktask_t *__ktask_chain_unlock_and_findfirst_and_lock(ktask_t *self) {
 ktask_priority_t old_priority,new_priority;
 ktask_t *result = self,*next;
 for (;;) { // Search for the next task in line (considering priorities)
  do {
   old_priority = katomic_load(result->t_prio);
   new_priority = old_priority < 0 ? old_priority+1 : katomic_load(result->t_setprio);
  } while (!katomic_cmpxch(result->t_prio,old_priority,new_priority));
  if (new_priority == 0) return result;
  // Low-priority task
  next = result->t_sched.ts_next;
  _ktask_unlock(result);
  if __unlikely(!next) break;
  result = next;
  _ktask_lock(result);
 }
 return NULL;
}

__DECL_END

#endif /* !__GOS_KERNEL_KTASK_FINDNEXT_C_INL__ */

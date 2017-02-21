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
#ifndef __GOS_KERNEL_KTASK_SUSPEND_C_INL__
#define __GOS_KERNEL_KTASK_SUSPEND_C_INL__ 1

#include <assert.h>
#include <gos/compiler.h>
#include <gos/kernel/katomic.h>
#include <gos/kernel/errno.h>
#include <gos/kernel/ktask.h>
#include <gos/kernel/pageframe.h>

__DECL_BEGIN

kerrno_t ktask_resume_and_unlock(ktask_t *self) {
 assert(self);
 if (--self->t_suspended != 0) {
  _ktask_unlock(self);
  return KE_UNCHANGED;
 }
 assertf(self != __ktask_self,"But you're already running...");
 if ((self->t_flags&KTASK_FLAG_SUSPENDED)==0) {
  self->t_flags |= KTASK_FLAG_SUSPENDED;
  interrupt_disable(); // Register this task
  __ktask_chain_add_and_unlock(self);
 } else {
  _ktask_unlock(self);
 }
 return KE_OK;
}
kerrno_t ktask_suspend_and_unlock(ktask_t *self) {
 kerrno_t result;
 assert(self);
 if (self->t_suspended++ == 0) {
  // Someone else already suspended this thread
  _ktask_unlock(self);
  return KE_UNCHANGED;
 }
 if ((self->t_flags&KTASK_FLAG_SUSPENDED)==0) {
  self->t_flags |= KTASK_FLAG_SUSPENDED;
  interrupt_disable();
  if (self == ktask_self()) {
   ktask_t *yield_next;
   yield_next = self->t_sched.ts_next;
   if (yield_next == self) {
    // Make sure no deadlock will occur
    interrupt_enable();
    --self->t_suspended;
    self->t_flags &= ~(KTASK_FLAG_SUSPENDED);
    _ktask_unlock(self);
    return KE_WOULDBLOCK;
   }
   _ktask_lock(yield_next);
   _ktask_unlock(self);
   yield_next = __ktask_chain_unlock_and_findfirst_and_lock(yield_next);
   __ktask_self = yield_next;
   irq_registers_xch(&self->t_regs,
                     &yield_next->t_regs);
  } else {
   interrupt_enable();
  }
 }
 return result;
}

__DECL_END

#endif /* !__GOS_KERNEL_KTASK_SUSPEND_C_INL__ */

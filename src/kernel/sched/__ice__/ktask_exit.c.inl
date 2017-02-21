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
#ifndef __GOS_KERNEL_KTASK_EXIT_C_INL__
#define __GOS_KERNEL_KTASK_EXIT_C_INL__ 1

#include <assert.h>
#include <gos/compiler.h>
#include <gos/kernel/katomic.h>
#include <gos/kernel/ktask.h>
#include <gos/kernel/pageframe.h>

__DECL_BEGIN

static inline __noreturn void ktask_exit_and_unlock(ktask_t *self, void *retval) {
 assertf(self != ktask_cpu0(),"ktask_exit() executed by cpu0");
 assertf(ktask_issuspended(self),"ktask_exit() executed by a suspended task? (how did it get here?)");
 assertf(ktask_isterminated(self),"ktask_exit() executed by a terminated task? (how did it get here?)");
 interrupt_disable();
 self->t_sched.ts_prev->t_sched.ts_next = self->t_sched.ts_next;
 self->t_sched.ts_next->t_sched.ts_prev = self->t_sched.ts_prev;
 __ktask_self = __ktask_self->t_sched.ts_next;
 self->t_flags |= KTASK_FLAG_TERMINATED;
#ifdef __DEBUG__
 self->t_sched.ts_prev = NULL;
 self->t_sched.ts_next = NULL;
#endif
 _ktask_unlock(self);
 self->t_regs.eax = (__u32)retval;
 // This switch does not return
 irq_registers_set(&__ktask_self->t_regs);
 __builtin_unreachable();
}

void ktask_exit(void *retval) {
 ktask_t *self = ktask_self();
 _ktask_lock(self);
 ktask_exit_and_unlock(self,retval);
}

kerrno_t ktask_terminate_and_unlock(ktask_t *self, void *retval) {
 kerrno_t error;
 kassert_obj_rw(self);
 interrupt_disable();
 assert(!__ktask_isdetached(self) || self == ktask_self());
 if (self == ktask_self()) {
  ktask_exit_and_unlock(self,retval);
  __builtin_unreachable();
 }
 if (ktask_isterminated(self)) {
  // Terminated task
  error = KE_UNCHANGED;
  goto end;
 } else if (ktask_issuspended(self)) {
  // Suspended task
  self->t_flags |= KTASK_FLAG_TERMINATED;
 } else {
  self->t_sched.ts_next->t_sched.ts_prev = self->t_sched.ts_next;
  self->t_sched.ts_prev->t_sched.ts_next = self->t_sched.ts_prev;
#ifdef __DEBUG__
  self->t_sched.ts_prev = NULL;
  self->t_sched.ts_next = NULL;
#endif
  self->t_flags |= KTASK_FLAG_TERMINATED;
  assert(self != ktask_self());
 }
 self->t_regs.eax = (__u32)retval;
 error = KE_OK;
end:
 _ktask_unlock(self);
 interrupt_enable();
 return error;
}


__DECL_END

#endif /* !__GOS_KERNEL_KTASK_EXIT_C_INL__ */

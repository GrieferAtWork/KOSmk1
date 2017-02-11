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
#include "ktask_lockstate.c.inl"
#define TIMEOUT
__DECL_BEGIN
#endif

#ifdef TIMEOUT
kerrno_t __ktask_enter_lockstate_and_unlock_timed(
 ktask_t *task_self, struct timespec const *timeout, struct __klock *lock)
#else
kerrno_t __ktask_enter_lockstate_and_unlock(
 ktask_t *task_self, struct __klock *lock)
#endif
{
 ktask_t *yield_next; kerrno_t error;
 int irq_return_error;
 kassert_obj_rw(task_self);
 kassert_obj_rw(lock);
#ifdef TIMEOUT
 kassert_obj_ro(timeout);
#endif
 assert(task_self == ktask_self());
 yield_next = task_self->t_sched.ts_next;
 assert(yield_next);
 if (yield_next == task_self) {
  _ktask_unlock(task_self);
  __klock_internal_unlock(lock);
  interrupt_enable();
  return KE_WOULDBLOCK;
 }
 if ((task_self->t_wait.tw_prev = lock->l_wait_last) != NULL) {
  task_self->t_wait.tw_prev->t_wait.tw_next = task_self;
 } else {
  assert(!lock->l_wait_first);
  lock->l_wait_first = task_self;
 }
 task_self->t_wait.tw_next = NULL;
 lock->l_wait_last = task_self;
 task_self->t_wait.tw_wait = lock;
 error = _ktask_trylock(yield_next);
 __klock_internal_unlock(lock);
 if __unlikely(!error) {
  _ktask_unlock(task_self);
  return KE_UNCHANGED; // Try again (failed to acquire lock to the next task)
 }
 irq_return_error = irq_registers_get(&task_self->t_regs);
 if (irq_return_error == IRQ_REGISTERS_XGET_FIRST) {
  task_self->t_flags |= KTASK_FLAG_WAITING;
  // Find the first task applicable for the switch
  yield_next = __ktask_chain_unlock_and_findfirst_and_lock(yield_next);
  __ktask_self = yield_next;
#ifdef TIMEOUT
  // TODO: Register 'task_self' to be resumed after timeout has passed
  // TODO: If timeout has passed, 't_regs.eax' must be set to KE_TIMEDOUT!
#endif
  _ktask_unlock(task_self);
  _ktask_unlock(yield_next);
  // Perform the context switch into the next task
  irq_registers_set(&yield_next->t_regs);
 }
#ifdef TIMEOUT
 return (kerrno_t)irq_return_error;
#else
 return KE_OK;
#endif
}

#ifdef TIMEOUT
#undef TIMEOUT
#endif

#ifdef __INTELLISENSE__
__DECL_END
#endif

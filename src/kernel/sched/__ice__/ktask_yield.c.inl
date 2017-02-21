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
#ifndef __GOS_KERNEL_KTASK_YIELD_C_INL__
#define __GOS_KERNEL_KTASK_YIELD_C_INL__ 1

#include <gos/compiler.h>
#include <gos/kernel/katomic.h>
#include <gos/kernel/ktask.h>

__DECL_BEGIN

void ktask_yield(void) {
 ktask_t *newtask,*oldtask;
 interrupt_disable();
 oldtask = __ktask_self;
 _ktask_lock(oldtask);
 newtask = __ktask_chain_unlock_and_findnext_and_lock(oldtask);
 _ktask_unlock(newtask);
 if __likely(newtask != oldtask) {
  __ktask_self = newtask;
  irq_registers_xch(&oldtask->t_regs,
                    &newtask->t_regs);
 } else {
  interrupt_enable();
 }
}

kerrno_t ktask_tryyield(void) {
 ktask_t *newtask,*oldtask;
 interrupt_disable();
 oldtask = __ktask_self;
 _ktask_lock(oldtask);
 newtask = __ktask_chain_unlock_and_findnext_and_lock(oldtask);
 _ktask_unlock(newtask);
 if __likely(newtask != oldtask) {
  __ktask_self = newtask;
  irq_registers_xch(&oldtask->t_regs,
                    &newtask->t_regs);
  return KE_OK;
 } else {
  interrupt_enable();
  return KE_UNCHANGED;
 }
}

kerrno_t ktask_tryyield_nointerrupt(void) {
 ktask_t *newtask,*oldtask;
 oldtask = __ktask_self;
 _ktask_lock(oldtask);
 newtask = __ktask_chain_unlock_and_findnext_and_lock(oldtask);
 _ktask_unlock(newtask);
 if __likely(newtask != oldtask) {
  __ktask_self = newtask;
  irq_registers_xch(&oldtask->t_regs,
                    &newtask->t_regs);
  interrupt_disable();
  return KE_OK;
 } else {
  return KE_UNCHANGED;
 }
}


__DECL_END

#endif /* !__GOS_KERNEL_KTASK_YIELD_C_INL__ */

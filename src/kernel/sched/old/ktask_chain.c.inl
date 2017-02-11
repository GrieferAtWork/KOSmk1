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
#ifndef __GOS_KERNEL_KTASK_CHAIN_C_INL__
#define __GOS_KERNEL_KTASK_CHAIN_C_INL__ 1

#include <assert.h>
#include <gos/compiler.h>
#include <gos/kernel/katomic.h>
#include <gos/kernel/ktask.h>
#include <gos/kernel/pageframe.h>


__DECL_BEGIN

ktask_t *__ktask_self; //< TODO: This needs to be a per-cpu variable!

void __ktask_chain_add_and_unlock(ktask_t *self) {
 ktask_t *prev;
again:
 assert(self);
 _ktask_lock(__ktask_self);
 prev = __ktask_self->t_sched.ts_prev;
 if __likely(prev != __ktask_self) {
  // NOTE: Don't cause a deadlock here!
  if (!_ktask_trylock(prev)) {
   _ktask_unlock(__ktask_self);
   ktask_tryyield_nointerrupt();
   goto again;
  }
 }
 prev->t_sched.ts_next = self;
 self->t_sched.ts_prev = prev;
 self->t_sched.ts_next = __ktask_self;
 __ktask_self->t_sched.ts_prev = self;
 _ktask_unlock(prev);
 _ktask_unlock(__ktask_self);
 _ktask_unlock(self);
 interrupt_enable();
}


// Returns the task next in line to replace 'self'
void __irq_switchtask(irq_registers_t *regs) {
 ktask_t *nexttask;
 _ktask_lock(__ktask_self);
 nexttask = __ktask_chain_unlock_and_findnext_and_lock(__ktask_self);
 _ktask_unlock(nexttask);
 memcpy(&__ktask_self->t_regs,regs,sizeof(irq_registers_t));
 memcpy(regs,&nexttask->t_regs,sizeof(irq_registers_t));
 __arch_setpagedirectory(nexttask->t_pd);
 __ktask_self = nexttask;
 assertf(regs->eip == __ktask_self->t_regs.eip,
         "This is bad: %p != %p",
         regs->eip,__ktask_self->t_regs.eip);
 assertf(regs->eip >= 0x00100000 &&
         regs->eip < (__u32)arch_kernel_end,
         "Invalid EIP address: %p (regs->eip)\n"
         "regs->eip            = %p\n"
         "nexttask->t_regs.eip = %p\n"
         ,regs->eip
         ,regs->eip
         ,nexttask->t_regs.eip);
}



__DECL_END

#endif /* !__GOS_KERNEL_KTASK_CHAIN_C_INL__ */

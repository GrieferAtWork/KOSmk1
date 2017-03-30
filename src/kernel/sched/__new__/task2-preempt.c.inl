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
#ifndef __KOS_KERNEL_TASK2_PREEMPT_C_INL__
#define __KOS_KERNEL_TASK2_PREEMPT_C_INL__ 1

#include <assert.h>
#include <kos/atomic.h>
#include <kos/compiler.h>
#include <kos/config.h>
#include <kos/kernel/task2-cpu.h>
#include <kos/kernel/task2.h>
#include <kos/kernel/time.h>

__DECL_BEGIN

void kcpu2_preempt(struct kirq_registers *__restrict regs) {
 register struct kcpu2 *self = kcpu2_self();
 register struct ktask2 *oldtask,*newtask;
 struct timespec tmnow;
 __u8 flags;
again:
 flags = katomic_load(self->c_flags);
 /* TODO: What if this is a sporadic IRQ (Then these flags may no longer hold up...) */
 assertf((flags&(KCPU2_FLAG_PRESENT|KCPU2_FLAG_STARTED|KCPU2_FLAG_RUNNING)) ==
                (KCPU2_FLAG_PRESENT|KCPU2_FLAG_STARTED|KCPU2_FLAG_RUNNING),
         "Invalid flags for running CPU: %#.2I8x",flags);
 if (flags&(KCPU2_FLAG_TURNOFF|KCPU2_FLAG_NOIRQ)) {
  /* IPI command. */
  if (flags&KCPU2_FLAG_TURNOFF) {
   /* Acknowledge a CPU turn-off command. */
   if (!katomic_cmpxch(self->c_flags,flags,flags&
                     ~(KCPU2_FLAG_STARTED|
                       KCPU2_FLAG_RUNNING|
                       KCPU2_FLAG_TURNOFF)
       )) goto again;
   katomic_fetchinc(kcpu2_offline);
   katomic_fetchdec(kcpu2_online);
   kcpu2_apic_offline();
  }
  ktime_irqtick();
  return; /* KCPU2_FLAG_NOIRQ: Don't do preemption now. */
 }
 ktime_irqtick();
 ktime_getnow(&tmnow);
 /* Attempt to acquire a lock to the cpu's task list. */
 if (!kcpu2_trylock(self,KCPU2_LOCK_TASKS)) return;
 oldtask = self->c_current;
 if (!kcpu2_wakesleepers_unlocked(self,&tmnow))
      kcpu2_rotaterunning_unlocked(self);
 newtask = self->c_current;
 if __likely(oldtask != newtask) {
  assert(oldtask->t_prev != oldtask);
  assert(oldtask->t_next != oldtask);
  kassert_ktask2(oldtask->t_prev);
  kassert_ktask2(oldtask->t_next);
  /* Exchange the registers state. */
  oldtask->t_userregs = regs->userregs;
  oldtask->t_usercr3 = regs->usercr3;
  regs->userregs = newtask->t_userregs;
  regs->usercr3 = newtask->t_usercr3;
  if (oldtask == &self->c_idle) {
   /* Unlink the idle task after another task was added.
    * NOTE: The idle task doesn't count towards the running task counter. */
   oldtask->t_prev->t_next = oldtask->t_next;
   oldtask->t_next->t_prev = oldtask->t_prev;
  }
 }
 kcpu2_unlock(self,KCPU2_LOCK_TASKS);
}


__DECL_END

#endif /* !__KOS_KERNEL_TASK2_PREEMPT_C_INL__ */

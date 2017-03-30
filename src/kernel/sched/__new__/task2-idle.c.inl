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
#ifndef __KOS_KERNEL_TASK2_IDLE_C_INL__
#define __KOS_KERNEL_TASK2_IDLE_C_INL__ 1

#include <kos/atomic.h>
#include <kos/config.h>
#include <kos/compiler.h>
#include <kos/kernel/task2.h>
#include <kos/kernel/task2-cpu.h>

__DECL_BEGIN

__asm__(".global kcpu2_idlemain\n"
        "kcpu2_idlemain:\n"
        "    mov %" KCPU2_SELF_SEGMENT_S ":0, %eax\n" /* Load 'kcpu2_self()' */
        "1:  \n"
        /* TODO */
        "    \n"
        ".size kcpu2_idlemain, . - kcpu2_idlemain\n"
);
__noreturn void kcpu2_idlemain_(void) {
 register struct kcpu2 *self = kcpu2_self();
 register __u8 flags;
 /* Either do nothing, or turn off the CPU if it's allowed to. */
 for (;;) {
  assert(self->c_current == &self->c_idle);
  do {
   flags = katomic_load(self->c_flags);
   if ((flags&(KCPU2_FLAG_TURNOFF|KCPU2_FLAG_KEEPON)) !=
              (KCPU2_FLAG_TURNOFF)) {
    /* Don't turn off if another CPU is holding a lock to our task-chain. */
    if (kcpu2_trylock(self,KCPU2_LOCK_TASKS)) {
     int is_only_idle = self->c_idle.t_next == &self->c_idle;
     /* Only in the event of only the IDLE thread
      * remaining may we turn off this CPU.
      * >> If there are more threads, this is likely just another
      *    CPU that needed us to switch to another task so it
      *    could get a hold of the task we were executing then. */
     kcpu2_unlock(self,KCPU2_LOCK_TASKS);
     if (is_only_idle) goto continue_turnoff;
    }
    __asm_volatile__("sti\n" : : : "memory");
    goto idle; /* CPU Must be kept online (Just do idle stuff). */
   }
continue_turnoff:;
  } while (!katomic_cmpxch(self->c_flags,flags,flags&~
          (KCPU2_FLAG_STARTED|KCPU2_FLAG_RUNNING|KCPU2_FLAG_TURNOFF)));
  /* Turn the CPU off */
  katomic_fetchinc(kcpu2_offline);
  katomic_fetchdec(kcpu2_online);
  __asm_volatile__("cli\n" : : : "memory");
idle:
  __asm_volatile__("hlt\n" : : : "memory");
 }
}

__DECL_END

#endif /* !__KOS_KERNEL_TASK2_IDLE_C_INL__ */

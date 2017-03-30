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
#ifndef __KOS_KERNEL_TASK2_CPUTOGGLE_C_INL__
#define __KOS_KERNEL_TASK2_CPUTOGGLE_C_INL__ 1

#include <assert.h>
#include <kos/atomic.h>
#include <kos/compiler.h>
#include <kos/config.h>
#include <kos/errno.h>
#include <kos/kernel/debug.h>
#include <kos/kernel/interrupts.h>
#include <kos/kernel/task2-cpu.h>
#include <kos/kernel/task2.h>

__DECL_BEGIN

__crit kerrno_t __SCHED_CALL
kcpu2_disable(struct kcpu2 *__restrict self) {
 __u8 oldflags,newflags;
 __size_t oldenabled;
 kerrno_t error = KS_UNCHANGED;
 KTASK_CRIT_MARK
 kassert_kcpu2(self);
 do {
  oldenabled = katomic_load(kcpu2_enabled);
  if (oldenabled <= 1) {
   oldflags = katomic_load(self->c_flags);
   assert(oldflags&KCPU2_FLAG_PRESENT);
        if (!(oldflags&KCPU2_FLAG_ENABLED)) error = KS_UNCHANGED;
   else if (oldflags&KCPU2_FLAG_KEEPON) error = KE_PERM;
   else error = KE_DEADLK;
   return error;
  }
 } while (!katomic_cmpxch(kcpu2_enabled,oldenabled,oldenabled-1));
 if (self == kcpu2_self()) {
  /* Special case: Disable the current CPU. */

 }
 do {
  oldflags = katomic_load(self->c_flags);
  assert(oldflags&KCPU2_FLAG_PRESENT);
  if (!(oldflags&KCPU2_FLAG_ENABLED)) goto err; /* Already disabled (KS_UNCHANGED). */
  if (oldflags&KCPU2_FLAG_KEEPON) { error = KE_PERM; goto err; /* CPU cannot be turned off. */}
  newflags = oldflags|KCPU2_FLAG_INUSE|KCPU2_FLAG_TURNOFF;
 } while (!katomic_cmpxch(self->c_flags,oldflags,newflags));
 if (oldflags&KCPU2_FLAG_RUNNING) {
  /* Wait for the cpu to acknowledge the shutdown. */
  for (;;) {
   newflags = katomic_load(self->c_flags);
   if (!(newflags&KCPU2_FLAG_RUNNING)) break;
   if (!(newflags&KCPU2_FLAG_TURNOFF)) {
    /* Shutdown was aborted (State didn't change) --> KS_UNCHANGED */
    goto err;
   }
   ktask_yield();
  }
 }
 /* If we were the ones who actually turned off the CPU, we must inhert its tasks. */
 if (oldflags&KCPU2_FLAG_TURNOFF) goto err;
 /* Inherit all tasks from this CPU. */
 kcpu2_inherit_tasks(self);
 /* Drop the in-use flag inherited from the CPU. */
 kcpu2_turnon_abort(self);
 return KE_OK;
err:
 katomic_fetchinc(kcpu2_enabled);
 return error;
}

__crit kerrno_t __SCHED_CALL
kcpu2_enable(struct kcpu2 *__restrict self) {
 __u8 oldflags;
 KTASK_CRIT_MARK
 kassert_kcpu2(self);
 /* Set the CPU-enabled flag. */
 oldflags = katomic_fetchor(self->c_flags,KCPU2_FLAG_ENABLED);
 assert(oldflags&KCPU2_FLAG_PRESENT);
 if (oldflags&KCPU2_FLAG_ENABLED) return KS_UNCHANGED;
 asserte(katomic_incfetch(kcpu2_enabled) >= 2);
 return KE_OK;
}


__DECL_END

#endif /* !__KOS_KERNEL_TASK2_CPUTOGGLE_C_INL__ */

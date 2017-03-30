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
#ifndef __KOS_KERNEL_TASK2_CPUPOWER_C_INL__
#define __KOS_KERNEL_TASK2_CPUPOWER_C_INL__ 1

#include <kos/compiler.h>
#include <kos/config.h>
#include <kos/kernel/signal.h>
#include <kos/kernel/task2-cpu.h>
#include <kos/kernel/task2.h>

__DECL_BEGIN

__crit kerrno_t __SCHED_CALL
kcpu2_turnon_begin(struct kcpu2 *__restrict self) {
 kassert_kcpu2(self);
 assert(kcpu2_ispresent(self));
 return (katomic_fetchor(self->c_flags,KCPU2_FLAG_INUSE)&KCPU2_FLAG_INUSE) ? KE_BUSY : KE_OK;
}
__crit struct kcpu2 *__SCHED_CALL
kcpu2_turnon_begin_firstoffline(void) {
 struct kcpu2 *result; __u8 flags;
 KCPU2_FOREACH_ALL(result) {
  do {
   flags = katomic_load(result->c_flags);
   if ((flags&(KCPU2_FLAG_PRESENT|KCPU2_FLAG_INUSE)) !=
              (KCPU2_FLAG_PRESENT)) goto next;
  } while (!katomic_cmpxch(result->c_flags,flags,flags|KCPU2_FLAG_INUSE));
  return result;
next:;
 }
 return NULL;
}

__crit kerrno_t __SCHED_CALL
kcpu2_turnon_end(struct kcpu2 *__restrict self) {
 kassert_kcpu2(self);
 assert(kcpu2_inuse_notrunning(self));
 return kcpu2_apic_start(self);
}

__crit void __SCHED_CALL
kcpu2_turnon_abort(struct kcpu2 *__restrict self) {
 kassert_kcpu2(self);
 assert(kcpu2_inuse_notrunning(self));
 katomic_fetchand(self->c_flags,~(KCPU2_FLAG_INUSE));
}



__crit kerrno_t __SCHED_CALL
kcpu2_softoff_idle(struct kcpu2 *__restrict self) {
 kerrno_t error;
 kassert_kcpu2(self);
again:
 error = kcpu2_softoff_async(self);
 switch (error) {
  case KE_WOULDBLOCK: goto again;
  case KE_OK:
   /* Wait for the turnoff to be acknowledged. */
   while ((katomic_load(self->c_flags)&(KCPU2_FLAG_TURNOFF|KCPU2_FLAG_RUNNING)) ==
                                       (KCPU2_FLAG_TURNOFF|KCPU2_FLAG_RUNNING)
          ) __asm_volatile__("pause\n");
   break;
  default: break;
 }
 return error;
}
__crit kerrno_t __SCHED_CALL
kcpu2_softoff_yield(struct kcpu2 *__restrict self) {
 kerrno_t error;
 kassert_kcpu2(self);
again:
 error = kcpu2_softoff_async(self);
 switch (error) {
  case KE_WOULDBLOCK:
   ktask2_yield(NULL);
   goto again;
  case KE_OK:
   /* Wait for the turnoff to be acknowledged. */
   while ((katomic_load(self->c_flags)&(KCPU2_FLAG_TURNOFF|KCPU2_FLAG_RUNNING)) ==
                                       (KCPU2_FLAG_TURNOFF|KCPU2_FLAG_RUNNING)
          ) ktask2_yield(NULL);
   break;
  default: break;
 }
 return error;
}
__crit kerrno_t __SCHED_CALL
kcpu2_softoff_signal(struct kcpu2 *__restrict self) {
 kerrno_t error;
 kassert_kcpu2(self);
again:
 error = kcpu2_softoff_async(self);
 switch (error) {
  case KE_WOULDBLOCK:
   ktask2_yield(NULL);
   goto again;
  case KE_OK:
   /* Wait for the turnoff to be acknowledged. */
   ksignal_lock_c(&self->c_offline,KSIGNAL_LOCK_WAIT);
   if ((katomic_load(self->c_flags)&(KCPU2_FLAG_TURNOFF|KCPU2_FLAG_RUNNING)) ==
                                    (KCPU2_FLAG_TURNOFF|KCPU2_FLAG_RUNNING)) {
    error = _ksignal_recv_andunlock_c(&self->c_offline);
    assert(KE_ISERR(error) ||
         !(katomic_load(self->c_flags)&(KCPU2_FLAG_TURNOFF|KCPU2_FLAG_RUNNING)));
   } else {
    ksignal_unlock_c(&self->c_offline,KSIGNAL_LOCK_WAIT);
   }
   break;
  default: break;
 }
 return error;
}

__crit kerrno_t __SCHED_CALL
kcpu2_softoff_async(struct kcpu2 *__restrict self) {
 __u8 oldflags,newflags;
 kassert_kcpu2(self);
 assert(self != kcpu2_self());
 do {
  oldflags = katomic_load(self->c_flags);
  assert(oldflags&KCPU2_FLAG_PRESENT);
  if ((oldflags&(KCPU2_FLAG_STARTED|KCPU2_FLAG_RUNNING)) !=
                (KCPU2_FLAG_STARTED|KCPU2_FLAG_RUNNING)) {
   /* This CPU is either currently starting, or not running at all. */
   if (oldflags&KCPU2_FLAG_STARTED) return KE_WOULDBLOCK;
   return KS_UNCHANGED;
  }
  if (oldflags&KCPU2_FLAG_KEEPON) return KE_PERM;
  newflags = oldflags|KCPU2_FLAG_TURNOFF;
 } while (!katomic_cmpxch(self->c_flags,oldflags,newflags));
 return KE_OK;
}

__DECL_END

#endif /* !__KOS_KERNEL_TASK2_CPUPOWER_C_INL__ */

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
#ifndef __KOS_KERNEL_SCHED_SYNC_SEM_C_INL__
#define __KOS_KERNEL_SCHED_SYNC_SEM_C_INL__ 1

#include <kos/compiler.h>
#include <kos/atomic.h>
#include <kos/kernel/debug.h>
#include <kos/kernel/sem.h>
#include <kos/kernel/task.h>
#include <kos/kernel/time.h>
#include <kos/timespec.h>
#include <alloca.h>
#include <stddef.h>

__DECL_BEGIN

kerrno_t ksem_post(struct ksem *__restrict self,
                   ksemcount_t  new_tickets,
                   ksemcount_t *old_tickets) {
 ksemcount_t oldcount,newcount;
 kassert_ksem(self);
 kassertobjnull(old_tickets);
 do {
  oldcount = katomic_load(self->s_tck);
  newcount = oldcount+new_tickets;
  if __unlikely(newcount < oldcount) {
   // Overflow
   if (old_tickets) *old_tickets = oldcount;
   return KE_OVERFLOW;
  }
 } while (!katomic_cmpxch(self->s_tck,oldcount,newcount));
 if (old_tickets) *old_tickets = oldcount;
 return ksignal_sendn(&self->s_sig,new_tickets,NULL);
}

kerrno_t ksem_trywait(struct ksem *__restrict self) {
 ksemcount_t oldcount;
 kassert_ksem(self);
 do {
  oldcount = katomic_load(self->s_tck);
  if (!oldcount) return KE_WOULDBLOCK;
 } while (!katomic_cmpxch(self->s_tck,oldcount,oldcount-1));
 return KE_OK;
}
kerrno_t ksem_wait(struct ksem *__restrict self) {
 kerrno_t error;
 kassert_ksem(self);
 do {
  ksignal_lock(&self->s_sig,KSIGNAL_LOCK_WAIT);
  if ((error = ksem_trywait(self)) != KE_WOULDBLOCK) {
   ksignal_breakunlock(&self->s_sig,KSIGNAL_LOCK_WAIT);
   return error;
  }
  error = _ksignal_recv_andunlock_c(&self->s_sig);
  ksignal_endlock();
 } while (error == KE_OK);
 return error;
}
kerrno_t ksem_timedwait(struct ksem *__restrict self,
                        struct timespec const *__restrict abstime) {
 kerrno_t error;
 kassert_ksem(self);
 kassertobj(abstime);
 do {
  ksignal_lock(&self->s_sig,KSIGNAL_LOCK_WAIT);
  if ((error = ksem_trywait(self)) != KE_WOULDBLOCK) {
   ksignal_breakunlock(&self->s_sig,KSIGNAL_LOCK_WAIT);
   return error;
  }
  error = _ksignal_timedrecv_andunlock_c(&self->s_sig,abstime);
  ksignal_endlock();
 } while (error == KE_OK);
 return error;
}
kerrno_t ksem_timoutwait(struct ksem *__restrict self,
                         struct timespec const *__restrict timeout) {
 struct timespec abstime;
 kassert_ksem(self);
 kassertobj(timeout);
 ktime_getnoworcpu(&abstime);
 __timespec_add(&abstime,timeout);
 return ksem_timedwait(self,&abstime);
}

__DECL_END

#endif /* !__KOS_KERNEL_SCHED_SYNC_SEM_C_INL__ */

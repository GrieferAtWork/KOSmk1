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
#ifndef __KOS_KERNEL_SCHED_SYNC_CLOSELOCK_C_INL__
#define __KOS_KERNEL_SCHED_SYNC_CLOSELOCK_C_INL__ 1

#include <kos/compiler.h>
#include <kos/kernel/signal.h>
#include <kos/kernel/closelock.h>
#include <kos/kernel/time.h>
#include <kos/timespec.h>
#include <kos/kernel/debug.h>
#include <kos/errno.h>

__DECL_BEGIN

__crit kerrno_t kcloselock_beginop(struct kcloselock *__restrict self) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kcloselock(self);
 ksignal_lock_c(&self->cl_opsig,KSIGNAL_LOCK_WAIT);
 if (self->cl_opsig.s_flags&KCLOSELOCK_FLAG_CLOSEING) {
  error = KE_DESTROYED;
 } else if __unlikely(self->cl_opcount == (__u32)-1) {
  error = KE_OVERFLOW;
 } else {
  ++self->cl_opcount;
  error = KE_OK;
 }
 ksignal_unlock_c(&self->cl_opsig,KSIGNAL_LOCK_WAIT);
 return error;
}
__crit void kcloselock_endop(struct kcloselock *__restrict self) {
 KTASK_CRIT_MARK
 kassert_kcloselock(self);
 ksignal_lock_c(&self->cl_opsig,KSIGNAL_LOCK_WAIT);
 assertf(self->cl_opcount != 0,"Operation lock not held");
 if (!--self->cl_opcount) {
  // Signal all waiting tasks
  // NOTE: It doesn't matter if no one is waiting.
  _ksignal_sendall_andunlock_c(&self->cl_opsig);
 } else {
  ksignal_unlock_c(&self->cl_opsig,KSIGNAL_LOCK_WAIT);
 }
}
__crit kerrno_t kcloselock_reset(struct kcloselock *__restrict self) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kcloselock(self);
again:
 ksignal_lock_c(&self->cl_opsig,KSIGNAL_LOCK_WAIT);
 if (!(self->cl_opsig.s_flags&KCLOSELOCK_FLAG_CLOSEING)) {
  error = KS_UNCHANGED;
  goto end;
 }
 if __unlikely(self->cl_opcount) {
  // The closelock is current closing
  // >> Let it finish, then try again to reset it.
  __evalexpr(_ksignal_recv_andunlock_c(&self->cl_opsig));
  goto again;
 }
 self->cl_opsig.s_flags &= ~(KCLOSELOCK_FLAG_CLOSEING);
 error = KE_OK;
end:
 ksignal_unlock_c(&self->cl_opsig,KSIGNAL_LOCK_WAIT);
 return error;
}
__crit kerrno_t kcloselock_close(struct kcloselock *__restrict self) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kcloselock(self);
 ksignal_lock_c(&self->cl_opsig,KSIGNAL_LOCK_WAIT);
 if (self->cl_opsig.s_flags&KCLOSELOCK_FLAG_CLOSEING) {
  // Close lock was already destroyed
  error = KE_DESTROYED;
  goto end;
 }
 // Set the closing flag
 self->cl_opsig.s_flags |= KCLOSELOCK_FLAG_CLOSEING;
 if (self->cl_opcount) {
  // Operations are being performed. (wait for them)
  error = _ksignal_recv_andunlock_c(&self->cl_opsig);
  // If the signal was destroyed, the close lock still fired
  if (error == KE_DESTROYED) error = KE_OK;
  assertf(self->cl_opcount == 0,"Signal was send, but operations are still being performed");
 } else {
  // Nothing to wait on. -> We're already done
  error = KE_OK;
end:
  ksignal_unlock_c(&self->cl_opsig,KSIGNAL_LOCK_WAIT);
 }
 return error;
}
__crit kerrno_t kcloselock_tryclose(struct kcloselock *__restrict self) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kcloselock(self);
 ksignal_lock_c(&self->cl_opsig,KSIGNAL_LOCK_WAIT);
 if (self->cl_opsig.s_flags&KCLOSELOCK_FLAG_CLOSEING) {
  // Close lock was already destroyed
  error = KE_DESTROYED;
  goto end;
 }
 if (!self->cl_opcount) {
  self->cl_opsig.s_flags |= KCLOSELOCK_FLAG_CLOSEING;
  error = KE_OK;
 } else {
  error = KE_WOULDBLOCK;
 }
end:
 ksignal_unlock_c(&self->cl_opsig,KSIGNAL_LOCK_WAIT);
 return error;
}

__crit kerrno_t
kcloselock_timedclose(struct kcloselock *__restrict self,
                      struct timespec const *__restrict abstime) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kcloselock(self);
 ksignal_lock_c(&self->cl_opsig,KSIGNAL_LOCK_WAIT);
 if (self->cl_opsig.s_flags&KCLOSELOCK_FLAG_CLOSEING) {
  // Close lock was already destroyed
  error = KE_DESTROYED;
  goto end;
 }
 // Set the close signal
 self->cl_opsig.s_flags |= KCLOSELOCK_FLAG_CLOSEING;
 for (;;) {
  if (!self->cl_opcount) { error = KE_OK; break; }
  // Wait for a signal to be send, then re-lock the closelock
  error = _ksignal_timedrecv_andunlock_c(&self->cl_opsig,abstime);
  if (error == KE_TIMEDOUT) {
   // Remove the closing flag if we timed out
   ksignal_lock_c(&self->cl_opsig,KSIGNAL_LOCK_WAIT);
   self->cl_opsig.s_flags &= ~(KCLOSELOCK_FLAG_CLOSEING);
   break;
  }
  if (KE_ISERR(error)) return error;
  // Check the opcount again
  ksignal_lock_c(&self->cl_opsig,KSIGNAL_LOCK_WAIT);
 }
end:
 ksignal_unlock_c(&self->cl_opsig,KSIGNAL_LOCK_WAIT);
 return error;
}
__crit kerrno_t
kcloselock_timeoutclose(struct kcloselock *__restrict self,
                        struct timespec const *__restrict timeout) {
 struct timespec abstime;
 KTASK_CRIT_MARK
 kassert_kcloselock(self);
 kassertobj(timeout);
 ktime_getnoworcpu(&abstime);
 __timespec_add(&abstime,timeout);
 return kcloselock_timedclose(self,&abstime);
}

__DECL_END

#endif /* !__KOS_KERNEL_SCHED_SYNC_CLOSELOCK_C_INL__ */

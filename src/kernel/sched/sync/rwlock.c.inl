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
#ifndef __KOS_KERNEL_SCHED_SYNC_RWLOCK_C_INL__
#define __KOS_KERNEL_SCHED_SYNC_RWLOCK_C_INL__ 1

#include <kos/compiler.h>
#include <kos/kernel/debug.h>
#include <kos/kernel/rwlock.h>
#include <kos/kernel/signal.h>
#include <kos/kernel/time.h>
#include <kos/timespec.h>

__DECL_BEGIN

kerrno_t krwlock_reset(struct krwlock *__restrict self) {
 kassert_krwlock(self);
 ksignal_lock(&self->rw_sig,KSIGNAL_LOCK_WAIT);
 if (!(self->rw_sig.s_flags&KSIGNAL_FLAG_DEAD)) {
  ksignal_breakunlock(&self->rw_sig,KSIGNAL_LOCK_WAIT);
  return KS_UNCHANGED;
 }
 assertf(self->rw_readc == 0,"Closed R/W locks mustn't have any readers");
 assertf(self->rw_sig.s_flags&KRWLOCK_FLAG_WRITEMODE,
         "Closed R/W locks must be in write-mode");
 self->rw_sig.s_flags &= ~(KSIGNAL_FLAG_DEAD);
 ksignal_unlock(&self->rw_sig,KSIGNAL_LOCK_WAIT);
 return KE_OK;
}


__crit kerrno_t
krwlock_beginread(struct krwlock *__restrict self) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_krwlock(self);
again:
 ksignal_lock_c(&self->rw_sig,KSIGNAL_LOCK_WAIT);
 if (self->rw_sig.s_flags&KRWLOCK_FLAG_WRITEMODE) {
  // Wait for other tasks to finish writing
  // NOTE: When we're here, the R/W lock may have also been closed.
  error = _ksignal_recv_andunlock_c(&self->rw_sig);
  if __unlikely(KE_ISERR(error)) return error;
  // The R/W lock was released because the write task is finished.
  // -> Try again.
  goto again;
 }
 // Lock is not in write-mode. --> begin reading
 ++self->rw_readc;
 ksignal_unlock_c(&self->rw_sig,KSIGNAL_LOCK_WAIT);
 return KE_OK;
}
__crit kerrno_t
krwlock_trybeginread(struct krwlock *__restrict self) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_krwlock(self);
 ksignal_lock_c(&self->rw_sig,KSIGNAL_LOCK_WAIT);
 if (self->rw_sig.s_flags&KRWLOCK_FLAG_WRITEMODE) {
  // NOTE: Since we're not going to wait for the
  //       signal, we must manually check if we're
  //       about to fail because someone is writing,
  //       or because the R/W lock was closed
  //      (and therefor the signal killed).
  //      Otherwise, the lock is in write-mode,
  //      so we need to fail with a timeout.
  error = (self->rw_sig.s_flags&KSIGNAL_FLAG_DEAD)
   ? KE_DESTROYED : KE_WOULDBLOCK;
 } else {
  // Lock is not in write-mode. --> begin reading
  ++self->rw_readc;
  error = KE_OK;
 }
 ksignal_unlock_c(&self->rw_sig,KSIGNAL_LOCK_WAIT);
 return error;
}
__crit kerrno_t
krwlock_timedbeginread(struct krwlock *__restrict self,
                       struct timespec const *__restrict abstime) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_krwlock(self);
again:
 ksignal_lock_c(&self->rw_sig,KSIGNAL_LOCK_WAIT);
 if (self->rw_sig.s_flags&KRWLOCK_FLAG_WRITEMODE) {
  // Wait for other tasks to finish writing
  // NOTE: When we're here, the R/W lock may have also been closed.
  error = _ksignal_timedrecv_andunlock_c(&self->rw_sig,abstime);
  if __unlikely(KE_ISERR(error)) return error;
  // The R/W lock was released because the write task is finished.
  // -> Try again.
  goto again;
 }
 // Lock is not in write-mode. --> begin reading
 ++self->rw_readc;
 ksignal_unlock_c(&self->rw_sig,KSIGNAL_LOCK_WAIT);
 return KE_OK;
}
__crit kerrno_t
krwlock_timeoutbeginread(struct krwlock *__restrict self,
                         struct timespec const *__restrict timeout) {
 struct timespec abstime;
 KTASK_CRIT_MARK
 kassert_krwlock(self);
 kassertobj(timeout);
 ktime_getnoworcpu(&abstime);
 __timespec_add(&abstime,timeout);
 return krwlock_timedbeginread(self,&abstime);
}


__crit kerrno_t
krwlock_beginwrite(struct krwlock *__restrict self) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_krwlock(self);
again:
 ksignal_lock_c(&self->rw_sig,KSIGNAL_LOCK_WAIT);
 if (self->rw_sig.s_flags&KRWLOCK_FLAG_WRITEMODE ||
     self->rw_readc != 0) {
  // Lock is already in write-mode, or other tasks are reading.
  // >> Wait for the other tasks to finish.
  error = _ksignal_recv_andunlock_c(&self->rw_sig);
  if __unlikely(KE_ISERR(error)) return error;
  goto again;
 }
 // Enter write-mode
 self->rw_sig.s_flags |= KRWLOCK_FLAG_WRITEMODE;
 ksignal_unlock_c(&self->rw_sig,KSIGNAL_LOCK_WAIT);
 return KE_OK;
}
__crit kerrno_t
krwlock_trybeginwrite(struct krwlock *__restrict self) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_krwlock(self);
 ksignal_lock_c(&self->rw_sig,KSIGNAL_LOCK_WAIT);
 if (self->rw_sig.s_flags&KRWLOCK_FLAG_WRITEMODE ||
     self->rw_readc != 0) {
  // Lock is already in write-mode, or other tasks are reading.
  // >> Wait for the other tasks to finish.
  // NOTE: Just as in trybeginread, we must manually
  //       check if the signal was destroyed.
  error = (self->rw_sig.s_flags&KSIGNAL_FLAG_DEAD)
   ? KE_DESTROYED : KE_WOULDBLOCK;
 } else {
  // Enter write-mode
  self->rw_sig.s_flags |= KRWLOCK_FLAG_WRITEMODE;
  error = KE_OK;
 }
 ksignal_unlock_c(&self->rw_sig,KSIGNAL_LOCK_WAIT);
 return error;
}
__crit kerrno_t
krwlock_timedbeginwrite(struct krwlock *__restrict self,
                        struct timespec const *__restrict abstime) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_krwlock(self);
again:
 ksignal_lock_c(&self->rw_sig,KSIGNAL_LOCK_WAIT);
 if (self->rw_sig.s_flags&KRWLOCK_FLAG_WRITEMODE ||
     self->rw_readc != 0) {
  // Lock is already in write-mode, or other tasks are reading.
  // >> Wait for the other tasks to finish.
  error = _ksignal_timedrecv_andunlock_c(&self->rw_sig,abstime);
  if __unlikely(KE_ISERR(error)) return error;
  goto again;
 }
 // Enter write-mode
 self->rw_sig.s_flags |= KRWLOCK_FLAG_WRITEMODE;
 ksignal_unlock_c(&self->rw_sig,KSIGNAL_LOCK_WAIT);
 return KE_OK;
}
__crit kerrno_t
krwlock_timeoutbeginwrite(struct krwlock *__restrict self,
                          struct timespec const *__restrict timeout) {
 struct timespec abstime;
 KTASK_CRIT_MARK
 kassert_krwlock(self);
 kassertobj(timeout);
 ktime_getnoworcpu(&abstime);
 __timespec_add(&abstime,timeout);
 return krwlock_timedbeginwrite(self,&abstime);
}


__crit kerrno_t
krwlock_endread(struct krwlock *self) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_krwlock(self);
 ksignal_lock_c(&self->rw_sig,KSIGNAL_LOCK_WAIT);
 assertf(self->rw_readc != 0,"No read ticket held");
 if (!--self->rw_readc) {
  /* Signal a waiting write task
   * NOTE: While it wouldn't break the rules of R/W locks
   *       forcing a task attempting to start writing to
   *       employ a try-until-success strategy, it is
   *       better for the scheduler if we only wake one task.
   * NOTE: Keep trying if we failed to wake one due to the
   *       unlikely case of a race-condition failure, where
   *       the task is being terminated, yet we locked the
   *       signal before it could be unscheduled from it. */
  while ((error = _ksignal_sendone_andunlock_c(&self->rw_sig)) == KS_UNCHANGED)
   ksignal_lock_c(&self->rw_sig,KSIGNAL_LOCK_WAIT);
  assertf(error != KE_DESTROYED,"How did a non-critical task manage to receive an R/W-signal?");
  return (error != KS_EMPTY) ? KE_OK : KS_EMPTY;
 }
 ksignal_unlock_c(&self->rw_sig,KSIGNAL_LOCK_WAIT);
 return KS_UNCHANGED;
}
__crit kerrno_t
krwlock_endwrite(struct krwlock *__restrict self) {
 KTASK_CRIT_MARK
 kassert_krwlock(self);
 ksignal_lock_c(&self->rw_sig,KSIGNAL_LOCK_WAIT);
 assertf(self->rw_sig.s_flags&KRWLOCK_FLAG_WRITEMODE,
         "The lock is not in write-mode (No write ticket held)");
 self->rw_sig.s_flags &= ~(KRWLOCK_FLAG_WRITEMODE);
 /* Signal waiting read tasks */
 return _ksignal_sendall_andunlock_c(&self->rw_sig) ? KE_OK : KS_EMPTY;
}

__crit kerrno_t krwlock_downgrade(struct krwlock *__restrict self) {
 KTASK_CRIT_MARK
 kassert_krwlock(self);
 ksignal_lock_c(&self->rw_sig,KSIGNAL_LOCK_WAIT);
 assertf(self->rw_sig.s_flags&KRWLOCK_FLAG_WRITEMODE,
         "The lock is not in write-mode (No write ticket held)");
 assertf(self->rw_readc == 0,"There are readers while in write-mode?");
 self->rw_sig.s_flags &= ~(KRWLOCK_FLAG_WRITEMODE);
 self->rw_readc = 1;
 return _ksignal_sendall_andunlock_c(&self->rw_sig) ? KE_OK : KS_EMPTY;
}



__crit kerrno_t
krwlock_atomic_upgrade(struct krwlock *__restrict self) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_krwlock(self);
 assert(krwlock_isreadlocked(self));
 ksignal_lock_c(&self->rw_sig,KSIGNAL_LOCK_WAIT);
 if (self->rw_readc == 1) {
  /* Simple case: One reader (the caller), so we can just switch to write-mode. */
  self->rw_readc        = 0;
  self->rw_sig.s_flags |= KRWLOCK_FLAG_WRITEMODE;
  error                 = KE_OK;
  goto unlock_and_end;
 }
 /* Difficult case with the potential of failure:
  * We must release our read-lock and wait until all other readers
  * are done, then hope that we're fastest to re-lock the signal
  * before anyone else and try to enter write-mode as the first
  * one to do so.
  * If we fail to, we'll still have lost the read-lock, meaning that
  * we can't even try again and must inform the caller of our failure. */
 --self->rw_readc;
 error = _ksignal_recv_andunlock_c(&self->rw_sig);
 if __unlikely(KE_ISERR(error)) return error;
 /* We've received a signal which means that at one point
  * the R/W lock had left read-mode, leaving its current, new
  * state undefined and hopefully being neither (aka. unlocked). */
 ksignal_lock_c(&self->rw_sig,KSIGNAL_LOCK_WAIT);
 /* Check if the lock was closed in the mean time. */
 if __unlikely(self->rw_sig.s_flags&KSIGNAL_FLAG_DEAD) {
  error = KE_DESTROYED;
  goto unlock_and_end;
 }
 /* Check if we can claim this lock for us. */
 if (!self->rw_readc &&
     !(self->rw_sig.s_flags&KRWLOCK_FLAG_WRITEMODE)) {
  /* Yes, we can enter write-mode. */
  self->rw_sig.s_flags |= KRWLOCK_FLAG_WRITEMODE;
  /* Tell the caller that we had to block to reach out goal. */
  error = KS_BLOCKING;
  goto unlock_and_end;
 }
 /* Damn! Someone else was faster, and now we don't even have
  * out read-lock anymore. (We've failed...)
  * >> Tell the caller that the operation wasn't
  *    permitted, which is technically true. */
 error = KE_PERM;
unlock_and_end:
 ksignal_unlock_c(&self->rw_sig,KSIGNAL_LOCK_WAIT);
 return error;
}

__crit kerrno_t
krwlock_atomic_timedupgrade(struct krwlock *__restrict self,
                            struct timespec const *__restrict abstime) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_krwlock(self);
 assert(krwlock_isreadlocked(self));
 ksignal_lock_c(&self->rw_sig,KSIGNAL_LOCK_WAIT);
 if (self->rw_readc == 1) {
  /* Simple case: One reader (the caller), so we can just switch to write-mode. */
  self->rw_readc        = 0;
  self->rw_sig.s_flags |= KRWLOCK_FLAG_WRITEMODE;
  error                 = KE_OK;
  goto unlock_and_end;
 }
 /* Difficult case with the potential of failure:
  * We must release our read-lock and wait until all other readers
  * are done, then hope that we're fastest to re-lock the signal
  * before anyone else and try to enter write-mode as the first
  * one to do so.
  * If we fail to, we'll still have lost the read-lock, meaning that
  * we can't even try again and must inform the caller of our failure. */
 --self->rw_readc;
 error = _ksignal_timedrecv_andunlock_c(&self->rw_sig,abstime);
 if __unlikely(KE_ISERR(error)) return error;
 /* We've received a signal which means that at one point
  * the R/W lock had left read-mode, leaving its current, new
  * state undefined and hopefully being neither (aka. unlocked). */
 ksignal_lock_c(&self->rw_sig,KSIGNAL_LOCK_WAIT);
 /* Check if the lock was closed in the mean time. */
 if __unlikely(self->rw_sig.s_flags&KSIGNAL_FLAG_DEAD) {
  error = KE_DESTROYED;
  goto unlock_and_end;
 }
 /* Check if we can claim this lock for us. */
 if (!self->rw_readc &&
     !(self->rw_sig.s_flags&KRWLOCK_FLAG_WRITEMODE)) {
  /* Yes, we can enter write-mode. */
  self->rw_sig.s_flags |= KRWLOCK_FLAG_WRITEMODE;
  /* Tell the caller that we had to block to reach out goal. */
  error = KS_BLOCKING;
  goto unlock_and_end;
 }
 /* Damn! Someone else was faster, and now we don't even have
  * out read-lock anymore. (We've failed...)
  * >> Tell the caller that the operation wasn't
  *    permitted, which is technically true. */
 error = KE_PERM;
unlock_and_end:
 ksignal_unlock_c(&self->rw_sig,KSIGNAL_LOCK_WAIT);
 return error;
}

__crit kerrno_t
krwlock_atomic_timeoutupgrade(struct krwlock *__restrict self,
                              struct timespec const *__restrict timeout) {
 struct timespec abstime;
 KTASK_CRIT_MARK
 kassert_krwlock(self);
 ktime_getnoworcpu(&abstime);
 __timespec_add(&abstime,timeout);
 return krwlock_atomic_timedupgrade(self,&abstime);
}



__crit kerrno_t
krwlock_tryupgrade(struct krwlock *__restrict self) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_krwlock(self);
 assert(krwlock_isreadlocked(self));
 ksignal_lock_c(&self->rw_sig,KSIGNAL_LOCK_WAIT);
 if (self->rw_readc == 1) {
  /* The caller is the only reader (the caller). -> Simply enter write-mode. */
  self->rw_readc        = 0;
  self->rw_sig.s_flags |= KRWLOCK_FLAG_WRITEMODE;
  error = KE_OK;
 } else {
  /* There are more readers. (Blocking upgrades would now receive the signal). */
  error = KE_WOULDBLOCK;
 }
 ksignal_unlock_c(&self->rw_sig,KSIGNAL_LOCK_WAIT);
 return error;
}

__crit kerrno_t
krwlock_upgrade(struct krwlock *__restrict self) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_krwlock(self);
 if __likely(KE_ISOK(error = krwlock_atomic_upgrade(self))) return error;
 if __unlikely(error != KE_PERM) return error;
 /* The read-lock was lost and atomicity failed due to some other task.
  * >> Try the manual way and acquire a regular write-lock. */
 error = krwlock_beginwrite(self);
 return KE_ISERR(error) ? error : KS_UNLOCKED;
}
__crit kerrno_t
krwlock_timedupgrade(struct krwlock *__restrict self,
                     struct timespec const *__restrict abstime) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_krwlock(self);
 if __likely(KE_ISOK(error = krwlock_atomic_timedupgrade(self,abstime))) return error;
 if __unlikely(error != KE_PERM) return error;
 /* The read-lock was lost and atomicity failed due to some other task.
  * >> Try the manual way and acquire a regular write-lock. */
 error = krwlock_timedbeginwrite(self,abstime);
 return KE_ISERR(error) ? error : KS_UNLOCKED;
}
__crit kerrno_t
krwlock_timeoutupgrade(struct krwlock *__restrict self,
                       struct timespec const *__restrict timeout) {
 struct timespec abstime;
 KTASK_CRIT_MARK
 kassert_krwlock(self);
 ktime_getnoworcpu(&abstime);
 __timespec_add(&abstime,timeout);
 return krwlock_timedupgrade(self,&abstime);
}


__DECL_END

#endif /* !__KOS_KERNEL_SCHED_SYNC_RWLOCK_C_INL__ */

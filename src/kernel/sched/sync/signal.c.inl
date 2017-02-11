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
#ifndef __KOS_KERNEL_SCHED_SYNC_SIGNAL_C_INL__
#define __KOS_KERNEL_SCHED_SYNC_SIGNAL_C_INL__ 1

#include <kos/compiler.h>
#include <kos/atomic.h>
#include <kos/kernel/debug.h>
#include <kos/syslog.h>
#include <kos/kernel/pageframe.h>
#include <kos/kernel/signal.h>
#include <kos/kernel/task.h>
#include <kos/kernel/time.h>
#include <stddef.h>
#include <stdio.h>

__DECL_BEGIN

__crit int ksignal_trylocks_c(size_t sigc,
                              struct ksignal *const *__restrict sigv,
                              __u8 lock) {
 struct ksignal *const *iter,*const *end;
 KTASK_CRIT_MARK
 kassert_ksignals(sigc,sigv);
 end = (iter = sigv)+sigc;
 for (; iter != end; ++iter) {
  kassert_ksignal(*iter);
  if __unlikely(!ksignal_trylock_c(*iter,lock)) {
   // Unlock what we've already managed to lock.
   ksignal_unlocks_c((size_t)(iter-sigv),sigv,lock);
   return 0;
  }
 }
 return 1;
}


__crit __ref struct ktask *
_ksignal_popone_andunlock_c(struct ksignal *__restrict self) {
 struct ktask *result;
 struct ktasksigslot *slot;
 KTASK_CRIT_MARK
again:
 kassert_ksignal(self);
 assertf(ksignal_islocked(self,KSIGNAL_LOCK_WAIT),
         "The caller is not locking the signal");
 slot = self->s_wakefirst;
 if __unlikely(!slot) {
err_signal:
  ksignal_unlock_c(self,KSIGNAL_LOCK_WAIT);
  return NULL;
 }
 assert(slot->tss_prev == NULL);
 assert(!slot->tss_next || slot->tss_next->tss_prev == slot);
 assert((slot->tss_next == NULL) == (slot == self->s_wakelast));
 assert(slot->tss_sig == self);
 result = slot->tss_self;
 kassert_ktask(result);
 if __unlikely(!ktask_trylock(result,KTASK_LOCK_SIGCHAIN)) {
  // Create a new reference to the task
  if __unlikely(KE_ISERR(ktask_incref(result))) {
   /* TODO: This is some bad error handling right here... */
   goto err_signal;
  }
  ksignal_unlock_c(self,KSIGNAL_LOCK_WAIT);
  ktask_lock(result,KTASK_LOCK_SIGCHAIN);
  ksignal_lock_c(self,KSIGNAL_LOCK_WAIT);
  if __unlikely(slot != self->s_wakefirst) {
   // Slot was consumed by another task
   ktask_unlock(result,KTASK_LOCK_SIGCHAIN);
   ktask_decref(result);
   goto again;
  }
  // Drop the temporary reference
  // NOTE: This is optimized because we know that the task
  //       already has at least two stable references:
  //       #1: The one we created after we failed to acquire 'KTASK_LOCK_SIGCHAIN'
  //       #2: The one still lingering inside of the signal chain (which has
  //           been checked and is now protected by 'KTASK_LOCK_SIGCHAIN')
  asserte(katomic_decfetch(result->t_refcnt) >= 1);
 }
 assert(slot->tss_self == result);
 assert(slot->tss_sig == self);
 assert(!slot->tss_prev);
 // Manually unlink the signal chain.
 if ((self->s_wakefirst = slot->tss_next) != NULL) {
  self->s_wakefirst->tss_prev = NULL;
 } else {
  assert(self->s_wakelast == slot);
  self->s_wakelast = NULL;
 }
#ifdef __DEBUG__
 slot->tss_next = NULL;
#endif
 slot->tss_sig = NULL;
 // Delete the signal slot, thereby inheriting
 // a reference to the task from the chain.
 ktasksig_delslot(&result->t_sigchain,slot);
 // And we're done!
 // >> Now simply return the reference from the signal chain
 ksignal_unlock_c(self,KSIGNAL_LOCK_WAIT);
 ktask_unlock(result,KTASK_LOCK_SIGCHAIN);
 return result;
}

kerrno_t ksignal_sendn(struct ksignal *__restrict self,
                       size_t n_tasks,
                       size_t *__restrict woken_tasks) {
 size_t i; kerrno_t error;
 kassert_ksignal(self);
 for (i = 0; i != n_tasks; ++i) {
  error = ksignal_sendone(self);
  if __unlikely(KE_ISERR(error)) goto end;
 }
 error = KE_OK;
end:
 if (woken_tasks) *woken_tasks = i;
 return error;
}

__crit size_t _ksignal_sendall_andunlock_c(struct ksignal *__restrict self) {
 size_t count = 0;
 KTASK_CRIT_MARK
 kassert_ksignal(self);
 ksignal_unlock_c(self,KSIGNAL_LOCK_WAIT);
 // TODO: This only works for the case where
 //       this function is called from 'ksignal_close'
 //       When adding new tasks to the signal is still
 //       allowed, this is unsafe by waking tasks
 //       that may have been added after the initial
 //       call to this function.
 // TODO: To do this correct, we'd have to collect
 //       all tasks that we're supposed to wake,
 //       then unlink them, and then, finally
 //       wake them one-by-one.
 //       NOTE: Though if that doesn't work, due to
 //             a failure in allocating the necessary
 //             buffer, we should count how many
 //             are within our signal, and then
 //             call 'ksignal_sendone' that many times.
 //          >> That should still be OK for almost
 //             all situations.
 while (ksignal_sendone(self) != KS_EMPTY) ++count;
 return count;
}

__DECL_END

#ifndef __INTELLISENSE__
#define SINGLE
#include "signal-impl.c.inl"
#include "signal-impl.c.inl"
#include "signal_v.c.inl"
#endif

#endif /* !__KOS_KERNEL_SCHED_SYNC_SIGNAL_C_INL__ */

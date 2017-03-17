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
#ifndef __KOS_KERNEL_SCHED_TASK_SIGCHAIN_C_INL__
#define __KOS_KERNEL_SCHED_TASK_SIGCHAIN_C_INL__ 1

#include <kos/compiler.h>
#include <kos/kernel/features.h>
#include <kos/kernel/task.h>
#include <kos/kernel/sigset.h>
#include <malloc.h>

__DECL_BEGIN

__local void ktasksig_reset(struct ktasksig *__restrict self) {
 struct ktasksigslot *iter,*end;
 kassertobj(self);
 end = (iter = self->ts_sigs)+KTASK_SIGBUFSIZE;
 self->ts_next = NULL;
 for (; iter != end; ++iter) {
  iter->tss_prev = NULL;
  iter->tss_next = NULL;
  iter->tss_sig  = NULL;
 }
}

void ktasksig_init(struct ktasksig *__restrict self,
                   struct ktask *__restrict task) {
 struct ktasksigslot *iter,*end;
 kassertobj(self);
 kassert_ktask(task);
 self->ts_next = NULL;
 end = (iter = self->ts_sigs)+KTASK_SIGBUFSIZE;
 for (; iter != end; ++iter) {
#ifdef __DEBUG__
  iter->tss_prev = NULL;
  iter->tss_next = NULL;
#endif
  iter->tss_sig  = NULL;
  iter->tss_self = task; /* Weakly referenced (for now) */
 }
}

__local void ktasksig_quit(struct ktasksig *__restrict self) {
 struct ktasksigslot *iter,*end;
 struct ksignal *sig; kassertobj(self);
 end = (iter = self->ts_sigs)+KTASK_SIGBUFSIZE;
 for (; iter != end; ++iter) {
  assert(iter->tss_self == ktasksig_taskself(self));
  if ((sig = iter->tss_sig) != NULL) {
   kassert_ksignal(sig);
   ksignal_lock_c(sig,KSIGNAL_LOCK_WAIT);
   assert(!iter->tss_prev || iter->tss_prev->tss_next == iter);
   assert(!iter->tss_next || iter->tss_next->tss_prev == iter);
   assert((iter->tss_prev == NULL) == (iter == sig->s_wakefirst));
   assert((iter->tss_next == NULL) == (iter == sig->s_wakelast));
   if (iter->tss_prev) iter->tss_prev->tss_next = iter->tss_next;
   else       sig->s_wakefirst = iter->tss_next;
   if (iter->tss_next) iter->tss_next->tss_prev = iter->tss_prev;
   else        sig->s_wakelast = iter->tss_prev;
   iter->tss_sig = NULL;
#ifdef __DEBUG__
   iter->tss_next = NULL;
   iter->tss_prev = NULL;
#endif
   assertef(katomic_decfetch(iter->tss_self->t_refcnt) >= 1
            ,"The caller is not holding a reference to the task");
   ksignal_unlock_c(sig,KSIGNAL_LOCK_WAIT);
  }
 }
}

void ktasksig_clear(struct ktasksig *__restrict self) {
 struct ktasksig *iter,*next;
 kassertobj(self);
 ktasksig_quit(self);
 iter = self->ts_next;
 ktasksig_reset(self);
 while (iter) {
  next = iter->ts_next;
  ktasksig_quit(iter);
  free(iter);
  iter = next;
 }
}

struct ktasksigslot *ktasksig_newslot(struct ktasksig *__restrict self) {
 struct ktasksig *bufiter,*newbuf;
 struct ktasksigslot *iter,*end;
 kassertobj(self);
 bufiter = self;
 for (;;) {
  end = (iter = bufiter->ts_sigs)+KTASK_SIGBUFSIZE;
  for (; iter != end; ++iter) if (!iter->tss_sig) {
   return iter; /* Found a free slot. */
  }
  if (!bufiter->ts_next) break;
  bufiter = bufiter->ts_next;
 }
 /* Must allocate a new chain buffer. */
 newbuf = omalloc(struct ktasksig);
 if __unlikely(!newbuf) return NULL;
 bufiter->ts_next = newbuf;
 ktasksig_init(newbuf,ktasksig_taskself(self));
 return &newbuf->ts_sigs[0];
}

void ktasksig_delslot(struct ktasksig *__restrict self,
                      struct ktasksigslot *__restrict slot) {
 struct ktasksig *lastslot = NULL,*bufslot;
 kassertobj(self);
 kassertobj(slot);
 assert(!slot->tss_sig);
 bufslot = self; do {
  assert((lastslot != NULL) == (bufslot != self));
  if (slot >= self->ts_sigs &&
      slot < self->ts_sigs+KTASK_SIGBUFSIZE) {
   struct ktasksigslot *iter,*end;
   /* Found the slot. */
   if (bufslot == self) return;
   assert(lastslot != NULL);
   /* Check if the signal buffer is empty.
    * >> And if it is, free it. */
   end = (iter = bufslot->ts_sigs)+KTASK_SIGBUFSIZE;
   while (iter != end) if ((iter++)->tss_sig) return;
   /* Slot is empty >> Free it. */
   lastslot->ts_next = bufslot->ts_next;
   free(bufslot);
   return;
  }
  lastslot = bufslot;
  assert(bufslot->ts_next != bufslot &&
         bufslot->ts_next != self);
 }
#ifdef __DEBUG__
 while ((bufslot = bufslot->ts_next) != NULL);
 assertf(0,"The given slot %p is not part of the %p signal chain",slot,self);
#else
 while (1);
#endif
}

kerrno_t
ktasksig_addsig_locked(struct ktasksig *__restrict self,
                       struct ksignal *__restrict sig) {
 kerrno_t error; struct ktasksigslot *slot;
 kassertobj(self);
 kassert_ksignal(sig);
 assert(ksignal_islocked(sig,KSIGNAL_LOCK_WAIT));
 slot = ktasksig_newslot(self);
 if __unlikely(!slot) return KE_NOMEM;
 assert(!slot->tss_sig);
 kassert_ktask(slot->tss_self);
 /* Turn the weak self-reference into a strong one. */
 if __unlikely(KE_ISERR(error = ktask_incref(slot->tss_self))) {
  ktasksig_delslot(self,slot);
  return error;
 }
 assert(slot->tss_self == ktasksig_taskself(self));
 slot->tss_sig  = sig;
 slot->tss_next = NULL;
 if ((slot->tss_prev = sig->s_wakelast) != NULL) {
  slot->tss_prev->tss_next = slot;
 } else sig->s_wakefirst = slot;
 sig->s_wakelast = slot;
 return KE_OK;
}


__wunused kerrno_t
ktasksig_addsigs_andunlock(struct ktasksig *__restrict self,
                           struct ksigset const *signals) {
 struct ksignal *sig;
 kerrno_t error = KE_OK;
 /* TODO: Make this faster by knowing how may signals there are beforehand. */
 /* TODO: Is this function even called if 'signals' is empty? (can we make it non-NULL+non-EMPTY?) */
 KSIGSET_FOREACH(sig,signals) {
  if __likely(KE_ISOK(error)) {
   error = ktasksig_addsig_locked(self,sig);
  }
  ksignal_unlock_c(sig,KSIGNAL_LOCK_WAIT);
 }
 return error;
}


__DECL_END

#endif /* !__KOS_KERNEL_SCHED_TASK_SIGCHAIN_C_INL__ */

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
#ifndef __KOS_KERNEL_SCHED_SYNC_SIGSET_C_INL__
#define __KOS_KERNEL_SCHED_SYNC_SIGSET_C_INL__ 1

#include <kos/compiler.h>
#include <kos/syslog.h>
#include <kos/kernel/sigset.h>
#include <kos/kernel/time.h>
#include <malloc.h>

__DECL_BEGIN

#define SIGOF(p) (struct ksignal *)((uintptr_t)*(p)+self->ss_elemoff)

__crit __nomp kerrno_t
ksigset_dyn_insert(struct ksigset *__restrict self,
                   struct ksignal *__restrict sig) {
 struct ksignal **iter,**end;
 KTASK_CRIT_MARK
 kassert_ksigset(self);
 kassert_ksignal(sig);
 assert(!self->ss_next);
 assert(!self->ss_elemsiz);
 assert(!self->ss_elemoff);

 /* Make sure the given signal isn't already part of the set. */
 end = (iter = (struct ksignal **)self->ss_elemvec)+self->ss_elemcnt;
 for (; iter != end; ++iter) if (*iter == sig) return KE_EXISTS;
 iter = (struct ksignal **)realloc(self->ss_elemvec,
                                  (self->ss_elemcnt+1)*
                                   sizeof(struct ksignal *));
 if __unlikely(!iter) return KE_NOMEM;
 self->ss_elemvec = (void **)iter;
 iter[self->ss_elemcnt++] = sig;
 return KE_OK;
}
__crit __nomp kerrno_t
ksigset_dyn_remove(struct ksigset *__restrict self,
                   struct ksignal *__restrict sig) {
 struct ksignal **iter,**end;
 KTASK_CRIT_MARK
 kassert_ksigset(self);
 kassert_ksignal(sig);
 assert(!self->ss_next);
 assert(!self->ss_elemsiz);
 assert(!self->ss_elemoff);
 end = (iter = (struct ksignal **)self->ss_elemvec)+self->ss_elemcnt;
 for (; iter != end; ++iter) if (*iter == sig) {
  /* Remove the signal. */
  memmove(iter,iter+1,((end-iter)-1)*sizeof(struct ksignal));
  /* Update the count value & free some memory. */
  if (!--self->ss_elemcnt) {
   free(self->ss_elemvec);
   self->ss_elemvec = NULL;
  } else {
   iter = (struct ksignal **)realloc(self->ss_elemvec,self->ss_elemcnt*
                                     sizeof(struct ksignal *));
   if __likely(iter) self->ss_elemvec = (void **)iter;
  }
  return KE_OK;
 }
 return KS_UNCHANGED;
}


__local void
ksigset_unlockone(struct ksigset *__restrict self,
                  ksiglock_t lock) {
 uintptr_t elemsiz;
 if ((elemsiz = self->ss_elemsiz) != 0) {
  struct ksignal *iter,*begin;
  begin = self->ss_eleminl;
  iter = (struct ksignal *)((uintptr_t)begin+elemsiz*self->ss_elemcnt);
  while (iter != begin) {
   *(uintptr_t *)&iter -= elemsiz;
   ksignal_unlock_c(iter,lock);
  }
 } else {
  void **begin,**iter;
  iter = (begin = self->ss_elemvec)+self->ss_elemcnt;
  while (iter != begin) {
   --iter;
   ksignal_unlock_c(SIGOF(iter),lock);;
  }
 }
}

__crit int
ksigset_trylocks_c(struct ksigset *__restrict self,
                   ksiglock_t lock) {
 uintptr_t elemsiz;
 struct ksigset *start = self;
 KTASK_CRIT_MARK
 do {
  if ((elemsiz = self->ss_elemsiz) != 0) {
   struct ksignal *iter,*end;
   iter = self->ss_eleminl;
   end = (struct ksignal *)((uintptr_t)iter+elemsiz*self->ss_elemcnt);
   for (; iter != end; *(uintptr_t *)&iter += elemsiz) {
    if __unlikely(!ksignal_trylock_c(iter,lock)) {
     while (iter != self->ss_eleminl) {
      *(uintptr_t *)&iter -= elemsiz;
      ksignal_unlock_c(iter,lock);
     }
     goto err;
    }
   }
  } else {
   void **iter,**end;
   end = (iter = self->ss_elemvec)+self->ss_elemcnt;
   for (; iter != end; ++iter) {
    if __unlikely(!ksignal_trylock_c(SIGOF(iter),lock)) {
     while (iter-- != self->ss_elemvec) ksignal_unlock_c(SIGOF(iter),lock);
     goto err;
    }
   }
  }
 } while ((self = self->ss_next) != NULL);
 return 1;
err:
 while (self-- != start) {
  ksigset_unlockone(self,lock);
 }
 return 0;
}

__crit kerrno_t
_ksigset_recvs_andunlock_c(struct ksigset const *__restrict self) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_ksigset(self);
 assert(ksigset_islockeds(self,KSIGNAL_LOCK_WAIT));
 if __unlikely(ksigset_isdeads_unlocked(self)) {
  ksigset_unlocks_c(self,KSIGNAL_LOCK_WAIT);
  return KE_DESTROYED;
 }
 error = ktask_unschedule_ex(ktask_self(),KTASK_STATE_WAITING,NULL,self);
 assertf(error != KS_UNCHANGED,"Should never happen for ktask_self()");
 return error;
}
__crit kerrno_t
_ksigset_timedrecvs_andunlock_c(struct ksigset const *__restrict self,
                                struct timespec const *__restrict abstime) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_ksigset(self);
 assert(ksigset_islockeds(self,KSIGNAL_LOCK_WAIT));
 kassertobj(abstime);
 if __unlikely(ksigset_isdeads_unlocked(self)) {
  ksigset_unlocks_c(self,KSIGNAL_LOCK_WAIT);
  return KE_DESTROYED;
 }
 error = ktask_unschedule_ex(ktask_self(),KTASK_STATE_WAITINGTMO,(void *)abstime,self);
 assertf(error != KS_UNCHANGED,"Should never happen for ktask_self()");
 return error;
}
__crit kerrno_t
_ksigset_timeoutrecvs_andunlock_c(struct ksigset const *__restrict self,
                                  struct timespec const *__restrict timeout) {
 struct timespec abstime;
 KTASK_CRIT_MARK
 kassertobj(timeout);
 ktime_getnow(&abstime);
 __timespec_add(&abstime,timeout);
 return _ksigset_timedrecvs_andunlock_c(self,&abstime);
}


#undef SIGOF

__DECL_END

#endif /* !__KOS_KERNEL_SCHED_SYNC_SIGSET_C_INL__ */

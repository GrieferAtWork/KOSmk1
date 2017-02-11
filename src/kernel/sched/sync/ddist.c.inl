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
#ifndef __KOS_KERNEL_SCHED_SYNC_DDIST_C_INL__
#define __KOS_KERNEL_SCHED_SYNC_DDIST_C_INL__ 1

#include <kos/compiler.h>
#include <kos/config.h>
#include <kos/kernel/ddist.h>
#include <kos/kernel/proc.h>
#include <kos/kernel/signal.h>
#include <kos/kernel/signal_v.h>
#include <kos/kernel/task.h>
#include <kos/kernel/time.h>
#include <kos/timespec.h>
#include <sys/types.h>
#if KDEBUG_HAVE_TRACKEDDDIST
#include <stdio.h>
#include <stdlib.h>
#endif

__DECL_BEGIN

#define YES     1
#define NO      0
#define MAYBE (-1)

#if KDEBUG_HAVE_TRACKEDDDIST
__local int
kddist_task_is_registered(struct kddist const *self,
                          struct ktask const *task) {
 struct kddist_debug_slot *iter,*end;
 assert(ksignal_islocked(&self->dd_sigpoll,KSIGNAL_LOCK_WAIT));
 if (self->dd_sigpoll.s_flags&KSIGNAL_FLAG_DEAD)
  return MAYBE;
 iter = (end = self->dd_debug.ddd_regv)+self->dd_debug.ddd_regc;
 while (iter-- != end) if (iter->ds_task == task) return YES;
 return __unlikely(self->dd_debug.ddd_flags&KDDIST_DEBUG_FLAG_NOMEM)
  ? MAYBE
  : NO;
}
#else
#define kddist_task_is_registered(self,task) MAYBE
#endif


//////////////////////////////////////////////////////////////////////////
// Add/Delete the calling task as a registered user from the data distributor.
// @return: KE_OK:        The caller was successfully registered as a receiver.
// @return: KE_DESTROYED: The given ddist was destroyed.
__crit kerrno_t kddist_adduser(struct kddist *self) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kddist(self);
 ksignal_lock_c(&self->dd_sigpoll,KSIGNAL_LOCK_WAIT);
 if (self->dd_sigpoll.s_flags&KSIGNAL_FLAG_DEAD) {
  // ddist was already destroyed.
  error = KE_DESTROYED;
 } else if __unlikely(self->dd_users == (__un(KDDIST_USERBITS))-1) {
  // Too many registered users
  error = KE_OVERFLOW;
 } else {
#if KDEBUG_HAVE_TRACKEDDDIST
  struct kddist_debug_slot *iter,*end;
  struct ktask *caller = ktask_self();
  if (!(self->dd_debug.ddd_flags&KDDIST_DEBUG_FLAG_NOMEM) || !self->dd_debug.ddd_regc) {
   // Make sure the caller hadn't already been registered as a user
   end = (iter = self->dd_debug.ddd_regv)+self->dd_debug.ddd_regc;
   for (; iter != end; ++iter) if (iter->ds_task == caller) {
    printf("kddist_adduser() : Calling task %I32d:%Iu:%s was already registered\n"
           "--- See reference to last registration:\n",
           caller->t_proc->p_pid,caller->t_tid,ktask_getname(caller));
    dtraceback_print(iter->ds_tb);
    printf("--- See reference to new registration:\n");
    _printtracebackex_d(1);
    abort();
   }
   iter = (struct kddist_debug_slot *)realloc(self->dd_debug.ddd_regv,
                                             (self->dd_debug.ddd_regc)+1*
                                              sizeof(struct kddist_debug_slot));
   if (iter) {
    self->dd_debug.ddd_flags &= ~(KDDIST_DEBUG_FLAG_NOMEM);
    self->dd_debug.ddd_regv = iter;
    iter += self->dd_debug.ddd_regc;
    iter->ds_task = caller;
    iter->ds_tb = dtraceback_captureex(1);
   } else {
    self->dd_debug.ddd_flags |= (KDDIST_DEBUG_FLAG_NOMEM);
   }
  }
#endif /* KDEBUG_HAVE_TRACKEDDDIST */
  // Increment the users-counter
  ++self->dd_users;
 }
 ksignal_unlock_c(&self->dd_sigpoll,KSIGNAL_LOCK_WAIT);
 return error;
}

__crit void kddist_deluser(struct kddist *self) {
 KTASK_CRIT_MARK
 kassert_kddist(self);
 ksignal_lock_c(&self->dd_sigpoll,KSIGNAL_LOCK_WAIT);
 if (!(self->dd_sigpoll.s_flags&KSIGNAL_FLAG_DEAD)) {
#if KDEBUG_HAVE_TRACKEDDDIST
  struct kddist_debug_slot *iter,*end;
  struct ktask *caller = ktask_self();
  iter = (end = self->dd_debug.ddd_regv)+self->dd_debug.ddd_regc;
  while (--iter != end) if (iter->ds_task == caller) {
   // Found it!
   dtraceback_free(iter->ds_tb);
   memmove(iter,iter+1,(self->dd_debug.ddd_regc-1)-(iter-end)*
           sizeof(struct kddist_debug_slot));
   assert(end == self->dd_debug.ddd_regv);
   if (self->dd_debug.ddd_regc == 1) {
    free(end);
    self->dd_debug.ddd_regv = NULL;
    self->dd_debug.ddd_flags &= ~(KDDIST_DEBUG_FLAG_NOMEM);
   } else {
    iter = (struct kddist_debug_slot *)realloc(end,(self->dd_debug.ddd_regc-1)*
                                               sizeof(struct kddist_debug_slot));
    if (iter) self->dd_debug.ddd_regv = iter;
   }
   goto found_it;
  }
  if (!(self->dd_debug.ddd_flags&KDDIST_DEBUG_FLAG_NOMEM)) {
   // USAGE ERROR: Caller tried to unregister themselves, without prior registration
   printf("kddist_deluser() : Calling task %I32d:%Iu:%s hadn't been registered\n"
          "--- See reference to un-registration attempt:\n",
          caller->t_proc->p_pid,caller->t_tid,ktask_getname(caller));
   _printtracebackex_d(1);
   abort();
  }
found_it:
#endif /* KDEBUG_HAVE_TRACKEDDDIST */
  assertf(self->dd_users,"No more registered users");
  if (--self->dd_users == self->dd_ready) {
   // Must signal baker thread
   ksignal_sendall(&self->dd_sigpost);
  }
 }
 ksignal_unlock_c(&self->dd_sigpoll,KSIGNAL_LOCK_WAIT);
}


kerrno_t kddist_vrecv(struct kddist *__restrict self,
                      void *__restrict buf) {
 kerrno_t error;
 kassert_kddist(self);
 ksignal_lock(&self->dd_sigpoll,KSIGNAL_LOCK_WAIT);
 assertf(kddist_task_is_registered(self,ktask_self()) != NO,
         "The caller is not a registered consumer thread "
         "(call 'kddist_vrecv_unregistered' instead)");
 if __unlikely(self->dd_sigpoll.s_flags&KSIGNAL_FLAG_DEAD) {
  error = KE_DESTROYED;
  ksignal_unlock_c(&self->dd_sigpoll,KSIGNAL_LOCK_WAIT);
 } else {
  if (++self->dd_ready == self->dd_users) {
   // Wake the baker if all tasks are ready
   ksignal_sendall(&self->dd_sigpost);
  }
  error = _ksignal_vrecv_andunlock_c(&self->dd_sigpoll,buf);
 }
 ksignal_endlock();
 return error;
}
kerrno_t kddist_vtimedrecv(struct kddist *__restrict self,
                           struct timespec const *__restrict abstime,
                           void *__restrict buf) {
 kerrno_t error;
 kassert_kddist(self);
 ksignal_lock(&self->dd_sigpoll,KSIGNAL_LOCK_WAIT);
 assertf(kddist_task_is_registered(self,ktask_self()) != NO,
         "The caller is not a registered consumer thread "
         "(call 'kddist_vtimedrecv_unregistered' instead)");
 if __unlikely(self->dd_sigpoll.s_flags&KSIGNAL_FLAG_DEAD) {
  error = KE_DESTROYED;
  ksignal_unlock_c(&self->dd_sigpoll,KSIGNAL_LOCK_WAIT);
 } else {
  if (++self->dd_ready == self->dd_users) {
   // Wake the baker if all tasks are ready
   ksignal_sendall(&self->dd_sigpost);
  }
  error = _ksignal_vtimedrecv_andunlock_c(&self->dd_sigpoll,abstime,buf);
  // NOTE: At this point, the ready-counter was reset by vsend
 }
 ksignal_endlock();
 return error;
}
kerrno_t kddist_vtimeoutrecv(struct kddist *__restrict self,
                             struct timespec const *__restrict timeout,
                             void *__restrict buf) {
 struct timespec abstime;
 ktime_getnoworcpu(&abstime);
 __timespec_add(&abstime,timeout);
 return kddist_vtimedrecv(self,&abstime,buf);
}

#if KDEBUG_HAVE_TRACKEDDDIST
kerrno_t kddist_vrecv_unregistered(struct kddist *__restrict self,
                                   void *__restrict buf) {
 kerrno_t error;
 ksignal_lock(&self->dd_sigpoll,KSIGNAL_LOCK_WAIT);
 assertf(!(self->dd_debug.ddd_flags&KDDIST_DEBUG_FLAG_REGONLY),
         "This ddist is configured not to allow unregistered data receivers.");
 assertf(kddist_task_is_registered(self,ktask_self()) != YES,
         "The caller is a registered consumer thread "
         "(call 'kddist_vrecv' instead)");
 error = _ksignal_vrecv_andunlock_c(&self->dd_sigpoll,buf);
 ksignal_endlock();
 return error;
}
kerrno_t kddist_vtimedrecv_unregistered(struct kddist *__restrict self,
                                        struct timespec const *__restrict abstime,
                                        void *__restrict buf) {
 kerrno_t error;
 ksignal_lock(&self->dd_sigpoll,KSIGNAL_LOCK_WAIT);
 assertf(!(self->dd_debug.ddd_flags&KDDIST_DEBUG_FLAG_REGONLY),
         "This ddist is configured not to allow unregistered data receivers.");
 assertf(kddist_task_is_registered(self,ktask_self()) != YES,
         "The caller is a registered consumer thread "
         "(call 'kddist_timedvrecv' instead)");
 error = _ksignal_vtimedrecv_andunlock_c(&self->dd_sigpoll,abstime,buf);
 ksignal_endlock();
 return error;
}
kerrno_t kddist_vtimeoutrecv_unregistered(struct kddist *__restrict self,
                                          struct timespec const *__restrict timeout,
                                          void *__restrict buf) {
 struct timespec abstime;
 ktime_getnoworcpu(&abstime);
 __timespec_add(&abstime,timeout);
 return kddist_vtimedrecv_unregistered(self,&abstime,buf);
}
#endif /* KDEBUG_HAVE_TRACKEDDDIST */


ssize_t kddist_vsend(struct kddist *__restrict self,
                     void *__restrict buf, size_t bufsize) {
 ssize_t error; size_t result;
 kassert_kddist(self);
 ksignal_lock(&self->dd_sigpoll,KSIGNAL_LOCK_WAIT);
 error = _kddist_beginsend_c_maybeunlock(self);
 if __unlikely(KE_ISERR(error)) return error;
 result = kddist_commitsend_c(self,(size_t)error,buf,bufsize);
 if (result > SSIZE_MAX) result = SSIZE_MAX;
 error = (ssize_t)result;
 ksignal_endlock();
 return error;
}

ssize_t kddist_vtrysend(struct kddist *__restrict self,
                        void *__restrict buf, size_t bufsize) {
 ssize_t error; size_t result;
 kassert_kddist(self);
 ksignal_lock(&self->dd_sigpoll,KSIGNAL_LOCK_WAIT);
 error = _kddist_trybeginsend_c_maybeunlock(self);
 if __unlikely(KE_ISERR(error)) return error;
 result = kddist_commitsend_c(self,(size_t)error,buf,bufsize);
 if (result > SSIZE_MAX) result = SSIZE_MAX;
 error = (ssize_t)result;
 ksignal_endlock();
 return error;
}

ssize_t kddist_vtimedsend(struct kddist *__restrict self,
                          struct timespec const *__restrict abstime,
                          void *__restrict buf, size_t bufsize) {
 ssize_t error; size_t result;
 kassert_kddist(self);
 ksignal_lock(&self->dd_sigpoll,KSIGNAL_LOCK_WAIT);
 error = _kddist_timedbeginsend_c_maybeunlock(self,abstime);
 if __unlikely(KE_ISERR(error)) return error;
 result = kddist_commitsend_c(self,(size_t)error,buf,bufsize);
 if (result > SSIZE_MAX) result = SSIZE_MAX;
 error = (ssize_t)result;
 ksignal_endlock();
 return error;
}

ssize_t kddist_vtimeoutsend(struct kddist *__restrict self,
                            struct timespec const *__restrict timeout,
                            void *__restrict buf, size_t bufsize) {
 struct timespec abstime;
 ktime_getnoworcpu(&abstime);
 __timespec_add(&abstime,timeout);
 return kddist_vtimedsend(self,&abstime,buf,bufsize);
}

ssize_t kddist_vasyncsend(struct kddist *__restrict self,
                          void *__restrict buf, size_t bufsize) {
 ssize_t error; size_t result;
 kassert_kddist(self);
 ksignal_lock(&self->dd_sigpoll,KSIGNAL_LOCK_WAIT);
 error = _kddist_asyncbeginsend_c_maybeunlock(self);
 if __unlikely(KE_ISERR(error)) return error;
 result = kddist_commitsend_c(self,(size_t)error,buf,bufsize);
 if (result > SSIZE_MAX) result = SSIZE_MAX;
 error = (ssize_t)result;
 ksignal_endlock();
 return error;
}



__crit ssize_t
_kddist_beginsend_c_maybeunlock(struct kddist *__restrict self) {
 ssize_t error;
 KTASK_CRIT_MARK
 kassert_kddist(self);
again:
 assert(ksignal_islocked(&self->dd_sigpoll,KSIGNAL_LOCK_WAIT));
 if (self->dd_sigpoll.s_flags&KSIGNAL_FLAG_DEAD) {
  ksignal_unlock_c(&self->dd_sigpoll,KSIGNAL_LOCK_WAIT);
  return KE_DESTROYED;
 }
 assert(self->dd_ready <= self->dd_users);
 if (self->dd_ready != self->dd_users) {
  // Some threads are not yet ready
  // NOTE: Must lock the post signal before unlocking the poll
  //       signal, as to prevent post from being send before
  //       we have been unscheduled and are waiting for it.
  ksignal_lock_c(&self->dd_sigpost,KSIGNAL_LOCK_WAIT);
  ksignal_unlock_c(&self->dd_sigpoll,KSIGNAL_LOCK_WAIT);
  printf("Waiting for all tasks to become ready: %I32d:%Iu:%s\n",
         ktask_self()->t_proc->p_pid,
         ktask_self()->t_tid,
         ktask_getname(ktask_self()));
  _printtraceback_d();
  error = _ksignal_recv_andunlock_c(&self->dd_sigpost);
  if __unlikely(KE_ISERR(error)) return error;
  // The signal was received.
  // WARNING: Since we are not holding a lock on the post-signal,
  //          it is possible that some other baker task already
  //          consumed all ready task tickets.
  // Currently we aren't holding any locks, and in order to safely
  // handle this situation, we have to start over to see if we're
  // (most likely) not meant to consume all ready tasks.
  // >> __likely(self->dd_ready == self->dd_users)
  ksignal_lock_c(&self->dd_sigpoll,KSIGNAL_LOCK_WAIT);
  goto again;
 }
 // Consume all ready task tickets
 // >> It is required that we do this, because it the
 //    tasks had to un-ready themselves, it would create
 //    a race condition that would make it appear as
 //    though ready tasks were still available, when
 //    in fact they've just been handed data.
 error = self->dd_ready;
 self->dd_ready = 0;
 return error;
}

__crit ssize_t
_kddist_trybeginsend_c_maybeunlock(struct kddist *__restrict self) {
 ssize_t error;
 KTASK_CRIT_MARK
 kassert_kddist(self);
 assert(ksignal_islocked(&self->dd_sigpoll,KSIGNAL_LOCK_WAIT));
 if (self->dd_sigpoll.s_flags&KSIGNAL_FLAG_DEAD) {
  error = KE_DESTROYED;
end_unlock:
  ksignal_unlock_c(&self->dd_sigpoll,KSIGNAL_LOCK_WAIT);
  return error;
 }
 if (self->dd_ready != self->dd_users) {
  error = 0;
  goto end_unlock;
 }
 // Consume all ready task tickets
 // >> It is required that we do this, because it the
 //    tasks had to un-ready themselves, it would create
 //    a race condition that would make it appear as
 //    though ready tasks were still available, when
 //    in fact they've just been handed data.
 error = self->dd_ready;
 self->dd_ready = 0;
 return error;
}
__crit ssize_t
_kddist_timedbeginsend_c_maybeunlock(struct kddist *__restrict self,
                                     struct timespec const *__restrict abstime) {
 ssize_t error;
 KTASK_CRIT_MARK
 kassert_kddist(self);
again:
 assert(ksignal_islocked(&self->dd_sigpoll,KSIGNAL_LOCK_WAIT));
 if (self->dd_sigpoll.s_flags&KSIGNAL_FLAG_DEAD) {
  error = KE_DESTROYED;
  ksignal_unlock_c(&self->dd_sigpoll,KSIGNAL_LOCK_WAIT);
  return error;
 }
 if (self->dd_ready != self->dd_users) {
  // Some threads are not yet ready
  // NOTE: Must lock the post signal before unlocking the poll
  //       signal, as to prevent post from being send before
  //       we have been unscheduled and are waiting for it.
  ksignal_lock_c(&self->dd_sigpost,KSIGNAL_LOCK_WAIT);
  ksignal_unlock_c(&self->dd_sigpoll,KSIGNAL_LOCK_WAIT);
  error = _ksignal_timedrecv_andunlock_c(&self->dd_sigpost,abstime);
  if __unlikely(KE_ISERR(error)) return error;
  // The signal was received.
  // WARNING: Since we are not holding a lock on the post-signal,
  //          it is possible that some other baker task already
  //          consumed all ready task tickets.
  // Currently we aren't holding any locks, and in order to safely
  // handle this situation, we have to start over to see if we're
  // (most likely) not meant to consume all ready tasks.
  // >> __likely(self->dd_ready == self->dd_users)
  ksignal_lock_c(&self->dd_sigpoll,KSIGNAL_LOCK_WAIT);
  goto again;
 }
 // Consume all ready task tickets
 // >> It is required that we do this, because it the
 //    tasks had to un-ready themselves, it would create
 //    a race condition that would make it appear as
 //    though ready tasks were still available, when
 //    in fact they've just been handed data.
 error = self->dd_ready;
 self->dd_ready = 0;
 return error;
}
__crit ssize_t
_kddist_timeoutbeginsend_c_maybeunlock(struct kddist *__restrict self,
                                       struct timespec const *__restrict timeout) {
 struct timespec abstime;
 KTASK_CRIT_MARK
 ktime_getnoworcpu(&abstime);
 __timespec_add(&abstime,timeout);
 return _kddist_timedbeginsend_c_maybeunlock(self,&abstime);
}
__crit ssize_t _kddist_asyncbeginsend_c_maybeunlock(struct kddist *__restrict self) {
 ssize_t error;
 KTASK_CRIT_MARK
 kassert_kddist(self);
 assert(ksignal_islocked(&self->dd_sigpoll,KSIGNAL_LOCK_WAIT));
 if (self->dd_sigpoll.s_flags&KSIGNAL_FLAG_DEAD) {
  error = KE_DESTROYED;
  ksignal_unlock_c(&self->dd_sigpoll,KSIGNAL_LOCK_WAIT);
  return error;
 }
 // Consume all ready task tickets
 // >> It is required that we do this, because it the
 //    tasks had to un-ready themselves, it would create
 //    a race condition that would make it appear as
 //    though ready tasks were still available, when
 //    in fact they've just been handed data.
 error = self->dd_ready;
 self->dd_ready = 0;
 return error;
}

#if KDEBUG_HAVE_TRACKEDDDIST
__crit __size_t
kddist_commitsend_c(struct kddist *__restrict self, __size_t n_tasks,
                    void const *__restrict buf, __size_t bufsize) {
 __size_t result;
 KTASK_CRIT_MARK
 kassert_kddist(self);
 assertf(ksignal_islocked(&self->dd_sigpoll,KSIGNAL_LOCK_WAIT),
         "No send operation was started within this ddist!");
 assert(!self->dd_ready);
 result = _ksignal_vsendall_andunlock_c(&self->dd_sigpoll,buf,bufsize);
 if (self->dd_debug.ddd_flags&KDDIST_DEBUG_FLAG_REGONLY) {
  assertf(n_tasks >= result
         ,"Incorrect amount of tasks have received data in "
          "REGONLY ddist (Expected at most %Iu, got %Iu)."
         ,n_tasks,result);
 }
 return result;
}
#endif


#undef kddist_task_is_registered
#undef MAYBE
#undef NO
#undef YES

__DECL_END

#endif /* !__KOS_KERNEL_SCHED_SYNC_DDIST_C_INL__ */

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
#ifndef __KOS_KERNEL_SCHED_SYNC_MUTEX_C_INL__
#define __KOS_KERNEL_SCHED_SYNC_MUTEX_C_INL__ 1

#include <kos/compiler.h>
#include <kos/atomic.h>
#include <kos/kernel/debug.h>
#include <kos/kernel/mutex.h>
#include <kos/kernel/signal.h>
#include <kos/kernel/time.h>
#include <kos/timespec.h>
#if KDEBUG_HAVE_TRACKEDMUTEX
#include <kos/kernel/proc.h>
#include <kos/kernel/task.h>
#include <stdio.h>
#include <stdlib.h>
#endif

__DECL_BEGIN

#if KDEBUG_HAVE_TRACKEDMUTEX
#define KMUTEX_ONACQUIRE(self) \
do{\
 /* Capture a traceback and save the current task as holder. */\
 assert(!self->m_locktb);\
 self->m_holder = ktask_self();\
 self->m_locktb = dtraceback_captureex(1);\
}while(0)

#define KMUTEX_ONRELEASE(self) \
do{\
 if (self->m_holder != ktask_self()) {\
  /* Non-recursive lock held twice. */\
  printf("ERROR: Non-recursive lock %p not held by %I32d:%Iu:%s.\n"\
         "See reference to acquisition by %p:\n"\
        ,self,ktask_self()->t_proc->p_pid\
        ,ktask_self()->t_tid\
        ,ktask_getname(ktask_self())\
        ,self->m_holder);\
  dtraceback_print(self->m_locktb);\
  printf("See reference to attempted release:\n");\
  _printtracebackex_d(1);\
  abort();\
 }\
 dtraceback_free(self->m_locktb);\
 self->m_locktb = NULL;\
 self->m_holder = NULL;\
}while(0)

#define KMUTEX_ONLOCKINUSE(self) \
do{\
 if (self->m_holder == ktask_self()) {\
  /* Non-recursive lock held twice. */\
  printf("ERROR: Non-recursive lock %p held twice by %I32d:%Iu:%s.\n"\
         "See reference to previous acquisition:\n"\
        ,self,ktask_self()->t_proc->p_pid\
        ,ktask_self()->t_tid\
        ,ktask_getname(ktask_self()));\
  dtraceback_print(self->m_locktb);\
  printf("See reference to new acquisition:\n");\
  _printtracebackex_d(1);\
  abort();\
 }\
}while(0)

// struct ktask      *m_holder; /*< [0..1][lock(this)] Current holder of the mutex lock. */
// struct dtraceback *m_locktb; /*< [0..1][lock(this)] Traceback of where the lock was acquired. */
#else
#define KMUTEX_ONACQUIRE(self)   (void)0
#define KMUTEX_ONRELEASE(self)   (void)0
#define KMUTEX_ONLOCKINUSE(self) (void)0
#endif

__crit kerrno_t kmutex_lock(struct kmutex *__restrict self) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kmutex(self);
 do {
  if __likely(katomic_cmpxch(self->m_locked,0,1)) {
   KMUTEX_ONACQUIRE(self);
   return KE_OK;
  }
  KMUTEX_ONLOCKINUSE(self);
  error = ksignal_recv(&self->m_sig);
 } while (KE_ISOK(error));
 return error;
}
__crit kerrno_t kmutex_trylock(struct kmutex *__restrict self) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kmutex(self);
 if __likely(katomic_cmpxch(self->m_locked,0,1)) {
  KMUTEX_ONACQUIRE(self);
  return KE_OK;
 }
 // Failed to acquire a lock
 // >> Must list check if the mutex might have been destroyed.
 ksignal_lock(&self->m_sig,KSIGNAL_LOCK_WAIT);
 if __unlikely(self->m_sig.s_flags&KSIGNAL_FLAG_DEAD) {
  error = KE_DESTROYED;
 } else {
  KMUTEX_ONLOCKINUSE(self);
  error = KE_WOULDBLOCK;
 }
 ksignal_unlock(&self->m_sig,KSIGNAL_LOCK_WAIT);
 return error;
}
__crit kerrno_t kmutex_timedlock(struct kmutex *__restrict self,
                                 struct timespec const *__restrict abstime) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kmutex(self);
 do {
  if __likely(katomic_cmpxch(self->m_locked,0,1)) {
   KMUTEX_ONACQUIRE(self);
   return KE_OK;
  }
  KMUTEX_ONLOCKINUSE(self);
  error = ksignal_timedrecv(&self->m_sig,abstime);
 } while (KE_ISOK(error));
 return error;
}
__crit kerrno_t
kmutex_timeoutlock(struct kmutex *__restrict self,
                   struct timespec const *__restrict timeout) {
 struct timespec abstime;
 KTASK_CRIT_MARK
 kassert_kmutex(self);
 kassertobj(timeout);
 ktime_getnoworcpu(&abstime);
 __timespec_add(&abstime,timeout);
 return kmutex_timedlock(self,&abstime);
}
__crit kerrno_t kmutex_unlock(struct kmutex *__restrict self) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kmutex(self);
 KMUTEX_ONRELEASE(self);
 assertf(kmutex_islocked(self),"Lock not held");
 katomic_store(self->m_locked,0);
 // Signal a single task (Ignoring tasks that were re-schedued due to race conditions)
 while ((error = ksignal_sendone(&self->m_sig)) == KS_UNCHANGED);
 assertf(error != KE_DESTROYED,"How did a non-critical task manage to wait for a mutex?");
 return error;
}

#ifndef __INTELLISENSE__
#undef KMUTEX_ONACQUIRE
#undef KMUTEX_ONRELEASE
#undef KMUTEX_ONLOCKINUSE
#endif

__DECL_END

#endif /* !__KOS_KERNEL_SCHED_SYNC_MUTEX_C_INL__ */

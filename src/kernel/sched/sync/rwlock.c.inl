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
#if KCONFIG_HAVE_DEBUG_TRACKEDRWLOCK
#include <malloc.h>
#include <traceback.h>
#include <kos/kernel/panic.h>
#if KCONFIG_HAVE_TASK_DEADLOCK_CHECK
#include <kos/kernel/task.h>
#include <kos/kernel/proc.h>
#include <kos/kernel/serial.h>
#endif /* KCONFIG_HAVE_TASK_DEADLOCK_CHECK */
#endif /* !KCONFIG_HAVE_DEBUG_TRACKEDRWLOCK */

__DECL_BEGIN

/* Quickly disable tracked R/W locks without
 * having to recompile the entire kernel. */
//#define KCONFIG_QUICKDISABLE_DEBUG_TRACKEDRWLOCK 1

#ifndef KCONFIG_QUICKDISABLE_DEBUG_TRACKEDRWLOCK
#define KCONFIG_QUICKDISABLE_DEBUG_TRACKEDRWLOCK 0
#endif


#if KCONFIG_HAVE_DEBUG_TRACKEDRWLOCK
#define RWLOCK_DEBUG_NOINLINE __noinline
#else
#define RWLOCK_DEBUG_NOINLINE
#endif


#if KCONFIG_HAVE_DEBUG_TRACKEDRWLOCK && \
   !KCONFIG_QUICKDISABLE_DEBUG_TRACKEDRWLOCK
//////////////////////////////////////////////////////////////////////////
// Add/Remove/Set the calling thread as a/the read/write lock holder of the given R/W lock.
// NOTE: Recursively adding the same reader is allowed.
static void krwlock_debug_addreader(struct krwlock *__restrict self, size_t skip);
static void krwlock_debug_delreader(struct krwlock *__restrict self, size_t skip);
static void krwlock_debug_notwriter(struct krwlock *__restrict self, size_t skip);
static void krwlock_debug_setwriter(struct krwlock *__restrict self, size_t skip);
static void krwlock_debug_delwriter(struct krwlock *__restrict self, size_t skip);
#else /* KCONFIG_HAVE_DEBUG_TRACKEDRWLOCK */
#define krwlock_debug_addreader(self,skip) (void)(++(self)->rw_readc)
#define krwlock_debug_delreader(self,skip) (void)(--(self)->rw_readc)
#define krwlock_debug_notwriter(self,skip) (void)0
#if KCONFIG_HAVE_DEBUG_TRACKEDRWLOCK && \
    KCONFIG_QUICKDISABLE_DEBUG_TRACKEDRWLOCK
#define krwlock_debug_setwriter(self,skip) \
 (void)((self)->rw_debug.d_write.de_holder = ktask_self(),\
        (self)->rw_sig.s_flags |= KRWLOCK_FLAG_WRITEMODE)
#else
#define krwlock_debug_setwriter(self,skip) (void)((self)->rw_sig.s_flags |=   KRWLOCK_FLAG_WRITEMODE)
#endif
#define krwlock_debug_delwriter(self,skip) (void)((self)->rw_sig.s_flags &= ~(KRWLOCK_FLAG_WRITEMODE))
#endif /* !KCONFIG_HAVE_DEBUG_TRACKEDRWLOCK */
#if 1
#define krwlock_debug_notwriter_upgrade  krwlock_debug_notwriter
#else
#define krwlock_debug_notwriter_upgrade(self,skip) (void)0
#endif




#if KCONFIG_HAVE_DEBUG_TRACKEDRWLOCK && \
   !KCONFIG_QUICKDISABLE_DEBUG_TRACKEDRWLOCK
RWLOCK_DEBUG_NOINLINE void
krwlock_debug_addreader(struct krwlock *__restrict self, size_t skip) {
 struct krwlock_debugentry *entry;
 kassert_krwlock(self);
 assertf(!krwlock_iswritelocked(self),
         "The lock is in write-mode (Can't add read ticket)");
 assert(self->rw_readc <= self->rw_debug.d_read.r_locka);
 if (self->rw_readc == self->rw_debug.d_read.r_locka) {
  size_t old_alloc = self->rw_debug.d_read.r_locka;
  size_t new_alloc = old_alloc;
  if (!new_alloc) new_alloc = 2; else new_alloc *= 2;
  /* Must allocate more debug entries. */
  entry = (struct krwlock_debugentry *)realloc(self->rw_debug.d_read.r_lockv,
                                               new_alloc*sizeof(struct krwlock_debugentry));
  if __unlikely(!entry) { self->rw_sig.s_flags |= KRWLOCK_FLAG_BROKENREAD; goto done; }
  /* Fill new memory with ZERO(0) to allow for save deallocation in case of a broken vector. */
  memset(entry+old_alloc,0,(new_alloc-old_alloc)*
         sizeof(struct krwlock_debugentry));
  self->rw_debug.d_read.r_locka = new_alloc;
  self->rw_debug.d_read.r_lockv = entry;
 } else {
  entry = self->rw_debug.d_read.r_lockv;
 }
 entry += self->rw_readc;
 entry->de_holder = ktask_self();
 entry->de_locktb = tbtrace_captureex(skip);
done:
 ++self->rw_readc;
}

RWLOCK_DEBUG_NOINLINE void
krwlock_debug_delreader(struct krwlock *__restrict self, size_t skip) {
 struct ktask *caller;
 struct krwlock_debugentry *iter,*end;
 kassert_krwlock(self);
 assertf(self->rw_readc != 0,"No read ticket held");
 assertf(!krwlock_iswritelocked(self),
         "The lock is in write-mode (Can't add read ticket)");
 assert(self->rw_debug.d_read.r_locka >= self->rw_readc);
 caller = ktask_self();
 /* Search for the debug entry associated with the caller. */
 end = (iter = self->rw_debug.d_read.r_lockv)+self->rw_readc;
 for (; iter != end; ++iter) if (iter->de_holder == caller) {
  /* Delete the entry. */
  free(iter->de_locktb);
  memmove(iter,iter+1,((size_t)(end-iter)-1)*sizeof(struct krwlock_debugentry));
  end[-1].de_holder = NULL;
  end[-1].de_locktb = NULL;
  goto found;
 }
 /* Unless the read-vector is broken, this is an invalid access. */
 if (!(self->rw_sig.s_flags&KRWLOCK_FLAG_BROKENREAD)) {
  serial_printf(SERIAL_01,
                "Read ticket to R/W lock at %p not held by caller\n"
                "caller: %I32d:%Iu:%q (%p)\n",self,
                (caller && caller->t_proc) ? caller->t_proc->p_pid : 0,
                (caller) ? caller->t_tid : 0,
                (caller) ? ktask_getname(caller) : NULL,
                (caller));
  iter = self->rw_debug.d_read.r_lockv;
  for (; iter != end; ++iter) {
   caller = iter->de_holder;
   serial_printf(SERIAL_01,
                 "[TICKET %Iu] See reference to known ticket held by %I32d:%Iu:%q (%p):\n",
                (size_t)(iter-self->rw_debug.d_read.r_lockv),
                (caller && caller->t_proc) ? caller->t_proc->p_pid : 0,
                (caller) ? caller->t_tid : 0,
                (caller) ? ktask_getname(caller) : NULL,
                (caller));
   tbtrace_print(iter->de_locktb);
  }
  PANIC("R/W lock read ticket holder violation");
 }
found:
 if (!--self->rw_readc) {
  if (self->rw_sig.s_flags&KRWLOCK_FLAG_BROKENREAD) {
   /* Manually cleanup the contents of a broken vector. */
   end = (iter = self->rw_debug.d_read.r_lockv)+self->rw_debug.d_read.r_locka;
   for (; iter != end; ++iter) free(iter->de_locktb);
   self->rw_sig.s_flags &= ~(KRWLOCK_FLAG_BROKENREAD);
  }
  self->rw_debug.d_read.r_locka = 0;
  free(self->rw_debug.d_read.r_lockv);
  self->rw_debug.d_read.r_lockv = NULL;
 }
}
RWLOCK_DEBUG_NOINLINE void
krwlock_debug_notwriter(struct krwlock *__restrict self, size_t skip) {
 kassert_krwlock(self);
 if ((self->rw_sig.s_flags&(KSIGNAL_FLAG_DEAD|KRWLOCK_FLAG_WRITEMODE)) ==
                           (                  KRWLOCK_FLAG_WRITEMODE)) {
  struct ktask *caller = ktask_self();
  assertf(self->rw_readc == 0,"There are readers while in write-mode?");
  if (self->rw_debug.d_write.de_holder == caller) {
   serial_printf(SERIAL_01,
                 "Write ticket to R/W lock at %p is already held by caller (recursion is not supported)\n"
                 "thread: %I32d:%Iu:%q (%p)\n"
                 "See reference to original write ticket acquisition:\n",self,
                (caller && caller->t_proc) ? caller->t_proc->p_pid : 0,
                (caller) ? caller->t_tid : 0,
                (caller) ? ktask_getname(caller) : NULL,
                (caller));
   tbtrace_print(self->rw_debug.d_write.de_locktb);
   serial_printf(SERIAL_01,
                 "See reference to attempted second acquisition:\n");
   tb_printex(skip);
   PANIC("R/W lock recursive write ticket");
  }
 }
}
RWLOCK_DEBUG_NOINLINE void
krwlock_debug_setwriter(struct krwlock *__restrict self, size_t skip) {
 kassert_krwlock(self);
 assertf(!krwlock_iswritelocked(self),
         "The lock is already in write-mode (Can't add write ticket)");
 assertf(self->rw_readc == 0,"Can't switch to write-mode with readers set?");
 self->rw_sig.s_flags |= KRWLOCK_FLAG_WRITEMODE;
 self->rw_debug.d_write.de_holder = ktask_self();
 self->rw_debug.d_write.de_locktb = tbtrace_captureex(skip+1);
}
RWLOCK_DEBUG_NOINLINE void
krwlock_debug_delwriter(struct krwlock *__restrict self, size_t skip) {
 struct ktask *holder,*caller;
 kassert_krwlock(self);
 assertf(krwlock_iswritelocked(self),
         "The lock is not in write-mode (No write ticket held)");
 assertf(self->rw_readc == 0,"There are readers while in write-mode?");
 /* Make sure the caller is actually the write-ticket holder. */
 caller = ktask_self();
 holder = self->rw_debug.d_write.de_holder;
 if (holder != caller) {
  serial_printf(SERIAL_01,
                "Write ticket to R/W lock at %p not held by caller\n"
                "caller: %I32d:%Iu:%q (%p)\n"
                "holder: %I32d:%Iu:%q (%p)\n"
                "See reference to write ticket acquisition:\n",self,
               (caller && caller->t_proc) ? caller->t_proc->p_pid : 0,
               (caller) ? caller->t_tid : 0,
               (caller) ? ktask_getname(caller) : NULL,
               (caller),
               (holder && holder->t_proc) ? holder->t_proc->p_pid : 0,
               (holder) ? holder->t_tid : 0,
               (holder) ? ktask_getname(holder) : NULL,
               (holder));
  tbtrace_print(self->rw_debug.d_write.de_locktb);
  PANIC("R/W lock write ticket holder violation");
 }
 /* Free and NULL'ify old debug information */
 free(self->rw_debug.d_write.de_locktb);
 self->rw_debug.d_write.de_holder = NULL;
 self->rw_debug.d_write.de_locktb = NULL;
 /* Actually remove the write-mode flag. */
 self->rw_sig.s_flags &= ~(KRWLOCK_FLAG_WRITEMODE);
}

#if KCONFIG_HAVE_TASK_DEADLOCK_CHECK
static void
krwlock_deadlock_help(struct krwlock *__restrict self) {
 struct ktask *holder;
 assert(krwlock_iswritelocked(self) ||
        krwlock_isreadlocked(self));
 /* Deadlock help information for R/W locks. */
 if (krwlock_iswritelocked(self)) {
  holder = self->rw_debug.d_write.de_holder;
  serial_printf(SERIAL_01,
                "[W] Write mode R/W-lock at %p\n"
                "See reference to write ticket acquisition by thread %I32d:%Iu:%q (%p):\n",
                self,
               (holder && holder->t_proc) ? holder->t_proc->p_pid : 0,
               (holder) ? holder->t_tid : 0,
               (holder) ? ktask_getname(holder) : NULL,
               (holder));
  tbtrace_print(self->rw_debug.d_write.de_locktb);
 } else {
  struct krwlock_debugentry *iter,*end;
  serial_printf(SERIAL_01,"[R] Read mode R/W-lock at %p (%I" __PP_STR(KCONFIG_RWLOCK_READERBITS) "u tickets)\n",
                self,self->rw_readc);
  end = (iter = self->rw_debug.d_read.r_lockv)+self->rw_readc;
  for (; iter != end; ++iter) {
   holder = iter->de_holder;
   serial_printf(SERIAL_01,
                 "[TICKET %Iu] See reference to read ticket acquisition by thread %I32d:%Iu:%q (%p):\n",
                (size_t)(iter-self->rw_debug.d_read.r_lockv),
                (holder && holder->t_proc) ? holder->t_proc->p_pid : 0,
                (holder) ? holder->t_tid : 0,
                (holder) ? ktask_getname(holder) : NULL,
                (holder));
   tbtrace_print(iter->de_locktb);
  }
  if (self->rw_sig.s_flags&KRWLOCK_FLAG_BROKENREAD) {
   serial_printf(SERIAL_01,"WARNING: Some tickets may be missing from this list!\n");
  }
 }
}
#define KRWLOCK_DEADLOCK_HELP_BEGIN(self) \
 KTASK_DEADLOCK_HELP_BEGIN((void(*)(void*))&krwlock_deadlock_help,self)
#define KRWLOCK_DEADLOCK_HELP_END         \
 KTASK_DEADLOCK_HELP_END
#endif /* KCONFIG_HAVE_TASK_DEADLOCK_CHECK */
#endif /* KCONFIG_HAVE_DEBUG_TRACKEDRWLOCK */

#ifndef KRWLOCK_DEADLOCK_HELP_BEGIN
#define KRWLOCK_DEADLOCK_HELP_BEGIN(self) do
#define KRWLOCK_DEADLOCK_HELP_END         while(0)
#endif

kerrno_t krwlock_reset(struct krwlock *__restrict self) {
 kassert_krwlock(self);
 ksignal_lock(&self->rw_sig,KSIGNAL_LOCK_WAIT);
 if (!(self->rw_sig.s_flags&KSIGNAL_FLAG_DEAD)) {
  ksignal_breakunlock(&self->rw_sig,KSIGNAL_LOCK_WAIT);
  return KS_UNCHANGED;
 }
 assertf(self->rw_readc == 0,"Closed R/W locks mustn't have any readers");
 assertf(krwlock_iswritelocked(self),
         "Closed R/W locks must be in write-mode");
 self->rw_sig.s_flags &= ~(KSIGNAL_FLAG_DEAD);
 ksignal_unlock(&self->rw_sig,KSIGNAL_LOCK_WAIT);
 return KE_OK;
}

__crit RWLOCK_DEBUG_NOINLINE kerrno_t
krwlock_beginread(struct krwlock *__restrict self) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_krwlock(self);
again:
 ksignal_lock_c(&self->rw_sig,KSIGNAL_LOCK_WAIT);
 if (krwlock_iswritelocked(self)) {
  /* Wait for other tasks to finish writing
   * NOTE: When we're here, the R/W lock may have also been closed. */
  KRWLOCK_DEADLOCK_HELP_BEGIN(self) {
   error = _ksignal_recv_andunlock_c(&self->rw_sig);
  } KRWLOCK_DEADLOCK_HELP_END;
  if __unlikely(KE_ISERR(error)) return error;
  /* The R/W lock was released because the write task is finished.
   * -> Try again. */
  goto again;
 }
 /* Lock is not in write-mode. --> begin reading */
 krwlock_debug_addreader(self,1);
 ksignal_unlock_c(&self->rw_sig,KSIGNAL_LOCK_WAIT);
 return KE_OK;
}
__crit RWLOCK_DEBUG_NOINLINE kerrno_t
krwlock_trybeginread(struct krwlock *__restrict self) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_krwlock(self);
 ksignal_lock_c(&self->rw_sig,KSIGNAL_LOCK_WAIT);
 if (krwlock_iswritelocked(self)) {
  /* NOTE: Since we're not going to wait for the
   *       signal, we must manually check if we're
   *       about to fail because someone is writing,
   *       or because the R/W lock was closed
   *       (and therefor the signal killed).
   *       Otherwise, the lock is in write-mode,
   *       so we need to fail with would-block. */
  error = (self->rw_sig.s_flags&KSIGNAL_FLAG_DEAD)
   ? KE_DESTROYED : KE_WOULDBLOCK;
 } else {
  /* Lock is not in write-mode. --> begin reading */
  krwlock_debug_addreader(self,1);
  error = KE_OK;
 }
 ksignal_unlock_c(&self->rw_sig,KSIGNAL_LOCK_WAIT);
 return error;
}

#if KCONFIG_HAVE_DEBUG_TRACKEDRWLOCK
static __crit RWLOCK_DEBUG_NOINLINE kerrno_t
__krwlock_timedbeginread_d(struct krwlock *__restrict self,
                           struct timespec const *__restrict abstime,
                           size_t skip)
#else
__crit RWLOCK_DEBUG_NOINLINE kerrno_t
krwlock_timedbeginread(struct krwlock *__restrict self,
                       struct timespec const *__restrict abstime)
#endif
{
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_krwlock(self);
again:
 ksignal_lock_c(&self->rw_sig,KSIGNAL_LOCK_WAIT);
 if (krwlock_iswritelocked(self)) {
  /* Wait for other tasks to finish writing
   * NOTE: When we're here, the R/W lock may have also been closed. */
  KRWLOCK_DEADLOCK_HELP_BEGIN(self) {
   error = _ksignal_timedrecv_andunlock_c(&self->rw_sig,abstime);
  } KRWLOCK_DEADLOCK_HELP_END;
  if __unlikely(KE_ISERR(error)) return error;
  /* The R/W lock was released because the write task is finished.
   * -> Try again. */
  goto again;
 }
 /* Lock is not in write-mode. --> begin reading */
 krwlock_debug_addreader(self,skip+1);
 ksignal_unlock_c(&self->rw_sig,KSIGNAL_LOCK_WAIT);
 return KE_OK;
}
__crit RWLOCK_DEBUG_NOINLINE kerrno_t
krwlock_timeoutbeginread(struct krwlock *__restrict self,
                         struct timespec const *__restrict timeout) {
 struct timespec abstime;
 KTASK_CRIT_MARK
 kassert_krwlock(self);
 kassertobj(timeout);
 ktime_getnow(&abstime);
 __timespec_add(&abstime,timeout);
#if KCONFIG_HAVE_DEBUG_TRACKEDRWLOCK
 return __krwlock_timedbeginread_d(self,&abstime,1);
#else
 return krwlock_timedbeginread(self,&abstime);
#endif
}


#if KCONFIG_HAVE_DEBUG_TRACKEDRWLOCK
__crit RWLOCK_DEBUG_NOINLINE kerrno_t
__krwlock_beginwrite_d(struct krwlock *__restrict self,
                       size_t skip)
#else
__crit RWLOCK_DEBUG_NOINLINE kerrno_t
krwlock_beginwrite(struct krwlock *__restrict self)
#endif
{
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_krwlock(self);
again:
 ksignal_lock_c(&self->rw_sig,KSIGNAL_LOCK_WAIT);
 krwlock_debug_notwriter(self,skip+1);
 if (krwlock_iswritelocked(self) || self->rw_readc != 0) {
  /* Lock is already in write-mode, or other tasks are reading.
   * >> Wait for the other tasks to finish. */
  KRWLOCK_DEADLOCK_HELP_BEGIN(self) {
   error = _ksignal_recv_andunlock_c(&self->rw_sig);
  } KRWLOCK_DEADLOCK_HELP_END;
  if __unlikely(KE_ISERR(error)) return error;
  goto again;
 }
 /* Enter write-mode */
 krwlock_debug_setwriter(self,skip+1);
 ksignal_unlock_c(&self->rw_sig,KSIGNAL_LOCK_WAIT);
 return KE_OK;
}
__crit RWLOCK_DEBUG_NOINLINE kerrno_t
krwlock_trybeginwrite(struct krwlock *__restrict self) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_krwlock(self);
 ksignal_lock_c(&self->rw_sig,KSIGNAL_LOCK_WAIT);
 krwlock_debug_notwriter(self,1);
 if (krwlock_iswritelocked(self) || self->rw_readc != 0) {
  /* Lock is already in write-mode, or other tasks are reading.
   * >> Wait for the other tasks to finish.
   * NOTE: Just as in trybeginread, we must manually
   *       check if the signal was destroyed. */
  error = (self->rw_sig.s_flags&KSIGNAL_FLAG_DEAD)
   ? KE_DESTROYED : KE_WOULDBLOCK;
 } else {
  /* Enter write-mode */
  krwlock_debug_setwriter(self,1);
  error = KE_OK;
 }
 ksignal_unlock_c(&self->rw_sig,KSIGNAL_LOCK_WAIT);
 return error;
}

#if KCONFIG_HAVE_DEBUG_TRACKEDRWLOCK
static RWLOCK_DEBUG_NOINLINE __crit kerrno_t
__krwlock_timedbeginwrite_d(struct krwlock *__restrict self,
                            struct timespec const *__restrict abstime,
                            size_t skip)
#else
__crit RWLOCK_DEBUG_NOINLINE kerrno_t
krwlock_timedbeginwrite(struct krwlock *__restrict self,
                        struct timespec const *__restrict abstime)
#endif
{
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_krwlock(self);
again:
 ksignal_lock_c(&self->rw_sig,KSIGNAL_LOCK_WAIT);
 krwlock_debug_notwriter(self,skip+1);
 if (krwlock_iswritelocked(self) || self->rw_readc != 0) {
  /* Lock is already in write-mode, or other tasks are reading.
   * >> Wait for the other tasks to finish. */
  KRWLOCK_DEADLOCK_HELP_BEGIN(self) {
   error = _ksignal_timedrecv_andunlock_c(&self->rw_sig,abstime);
  } KRWLOCK_DEADLOCK_HELP_END;
  if __unlikely(KE_ISERR(error)) return error;
  goto again;
 }
 /* Enter write-mode */
 krwlock_debug_setwriter(self,skip+1);
 ksignal_unlock_c(&self->rw_sig,KSIGNAL_LOCK_WAIT);
 return KE_OK;
}
__crit RWLOCK_DEBUG_NOINLINE kerrno_t
krwlock_timeoutbeginwrite(struct krwlock *__restrict self,
                          struct timespec const *__restrict timeout) {
 struct timespec abstime;
 KTASK_CRIT_MARK
 kassert_krwlock(self);
 kassertobj(timeout);
 ktime_getnow(&abstime);
 __timespec_add(&abstime,timeout);
#if KCONFIG_HAVE_DEBUG_TRACKEDRWLOCK
 return __krwlock_timedbeginwrite_d(self,&abstime,1);
#else
 return krwlock_timedbeginwrite(self,&abstime);
#endif
}


__crit kerrno_t
krwlock_endread(struct krwlock *self) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_krwlock(self);
 ksignal_lock_c(&self->rw_sig,KSIGNAL_LOCK_WAIT);
 krwlock_debug_delreader(self,1);
 if (!self->rw_readc) {
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
#if !KCONFIG_HAVE_DEBUG_TRACKEDRWLOCK
 assert(krwlock_iswritelocked(self));
#endif
 ksignal_lock_c(&self->rw_sig,KSIGNAL_LOCK_WAIT);
 krwlock_debug_delwriter(self,1);
 /* Signal waiting read tasks */
 return _ksignal_sendall_andunlock_c(&self->rw_sig) ? KE_OK : KS_EMPTY;
}

__crit RWLOCK_DEBUG_NOINLINE kerrno_t
krwlock_downgrade(struct krwlock *__restrict self) {
 KTASK_CRIT_MARK
 kassert_krwlock(self);
 ksignal_lock_c(&self->rw_sig,KSIGNAL_LOCK_WAIT);
 krwlock_debug_delwriter(self,1);
#if KCONFIG_HAVE_DEBUG_TRACKEDRWLOCK
 krwlock_debug_addreader(self,1);
 assert(self->rw_readc == 1);
#else
 self->rw_readc = 1;
#endif
 return _ksignal_sendall_andunlock_c(&self->rw_sig) ? KE_OK : KS_EMPTY;
}


#if KCONFIG_HAVE_DEBUG_TRACKEDRWLOCK
static __crit RWLOCK_DEBUG_NOINLINE kerrno_t
__krwlock_atomic_upgrade_d(struct krwlock *__restrict self,
                           size_t skip)
#else
__crit RWLOCK_DEBUG_NOINLINE kerrno_t
krwlock_atomic_upgrade(struct krwlock *__restrict self)
#endif
{
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_krwlock(self);
 assert(krwlock_isreadlocked(self));
 ksignal_lock_c(&self->rw_sig,KSIGNAL_LOCK_WAIT);
 krwlock_debug_notwriter_upgrade(self,skip+1);
 if (self->rw_readc == 1) {
  /* Simple case: One reader (the caller), so we can just switch to write-mode. */
#if KCONFIG_HAVE_DEBUG_TRACKEDRWLOCK
  krwlock_debug_delreader(self,skip+1);
  assert(self->rw_readc == 0);
#else
  self->rw_readc = 0;
#endif
  krwlock_debug_setwriter(self,skip+1);
  error = KE_OK;
  goto unlock_and_end;
 }
 /* Difficult case with the potential of failure:
  * We must release our read-lock and wait until all other readers
  * are done, then hope that we're fastest to re-lock the signal
  * before anyone else and try to enter write-mode as the first
  * one to do so.
  * If we fail to, we'll still have lost the read-lock, meaning that
  * we can't even try again and must inform the caller of our failure. */
 krwlock_debug_delreader(self,skip+1);
 KRWLOCK_DEADLOCK_HELP_BEGIN(self) {
  error = _ksignal_recv_andunlock_c(&self->rw_sig);
 } KRWLOCK_DEADLOCK_HELP_END;
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
 if (!self->rw_readc && !krwlock_iswritelocked(self)) {
  /* Yes, we can enter write-mode. */
  krwlock_debug_setwriter(self,skip+1);
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

#if KCONFIG_HAVE_DEBUG_TRACKEDRWLOCK
static __crit RWLOCK_DEBUG_NOINLINE kerrno_t
__krwlock_atomic_timedupgrade_d(struct krwlock *__restrict self,
                                struct timespec const *__restrict abstime,
                                size_t skip)
#else
__crit RWLOCK_DEBUG_NOINLINE kerrno_t
krwlock_atomic_timedupgrade(struct krwlock *__restrict self,
                            struct timespec const *__restrict abstime)
#endif
{
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_krwlock(self);
 assert(krwlock_isreadlocked(self));
 ksignal_lock_c(&self->rw_sig,KSIGNAL_LOCK_WAIT);
 krwlock_debug_notwriter_upgrade(self,skip+1);
 if (self->rw_readc == 1) {
  /* Simple case: One reader (the caller), so we can just switch to write-mode. */
#if KCONFIG_HAVE_DEBUG_TRACKEDRWLOCK
  krwlock_debug_delreader(self,skip+1);
  assert(self->rw_readc == 0);
#else
  self->rw_readc = 0;
#endif
  krwlock_debug_setwriter(self,skip+1);
  error = KE_OK;
  goto unlock_and_end;
 }
 /* Difficult case with the potential of failure:
  * We must release our read-lock and wait until all other readers
  * are done, then hope that we're fastest to re-lock the signal
  * before anyone else and try to enter write-mode as the first
  * one to do so.
  * If we fail to, we'll still have lost the read-lock, meaning that
  * we can't even try again and must inform the caller of our failure. */
 krwlock_debug_delreader(self,skip+1);
 KRWLOCK_DEADLOCK_HELP_BEGIN(self) {
  error = _ksignal_timedrecv_andunlock_c(&self->rw_sig,abstime);
 } KRWLOCK_DEADLOCK_HELP_END;
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
 if (!self->rw_readc && !krwlock_iswritelocked(self)) {
  /* Yes, we can enter write-mode. */
  krwlock_debug_setwriter(self,skip+1);
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

__crit RWLOCK_DEBUG_NOINLINE kerrno_t
krwlock_atomic_timeoutupgrade(struct krwlock *__restrict self,
                              struct timespec const *__restrict timeout) {
 struct timespec abstime;
 KTASK_CRIT_MARK
 kassert_krwlock(self);
 ktime_getnow(&abstime);
 __timespec_add(&abstime,timeout);
#if KCONFIG_HAVE_DEBUG_TRACKEDRWLOCK
 return __krwlock_atomic_timedupgrade_d(self,&abstime,1);
#else
 return krwlock_atomic_timedupgrade(self,&abstime);
#endif
}



__crit RWLOCK_DEBUG_NOINLINE kerrno_t
krwlock_tryupgrade(struct krwlock *__restrict self) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_krwlock(self);
 assert(krwlock_isreadlocked(self));
 ksignal_lock_c(&self->rw_sig,KSIGNAL_LOCK_WAIT);
 krwlock_debug_notwriter_upgrade(self,1);
 if (self->rw_readc == 1) {
  /* The caller is the only reader (the caller). -> Simply enter write-mode. */
#if KCONFIG_HAVE_DEBUG_TRACKEDRWLOCK
  krwlock_debug_delreader(self,1);
  assert(self->rw_readc == 0);
#else
  self->rw_readc = 0;
#endif
  krwlock_debug_setwriter(self,1);
  error = KE_OK;
 } else {
  /* There are more readers. (Blocking upgrades would now receive the signal). */
  error = KE_WOULDBLOCK;
 }
 ksignal_unlock_c(&self->rw_sig,KSIGNAL_LOCK_WAIT);
 return error;
}

__crit RWLOCK_DEBUG_NOINLINE kerrno_t
krwlock_upgrade(struct krwlock *__restrict self) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_krwlock(self);
#if KCONFIG_HAVE_DEBUG_TRACKEDRWLOCK
 error = __krwlock_atomic_upgrade_d(self,1);
#else
 error = krwlock_atomic_upgrade(self);
#endif
 if __likely(KE_ISOK(error)) return error;
 if __unlikely(error != KE_PERM) return error;
 /* The read-lock was lost and atomicity failed due to some other task.
  * >> Try the manual way and acquire a regular write-lock. */
#if KCONFIG_HAVE_DEBUG_TRACKEDRWLOCK
 error = __krwlock_beginwrite_d(self,1);
#else
 error = krwlock_beginwrite(self);
#endif
 return KE_ISERR(error) ? error : KS_UNLOCKED;
}

#if KCONFIG_HAVE_DEBUG_TRACKEDRWLOCK
static __crit RWLOCK_DEBUG_NOINLINE kerrno_t
__krwlock_timedupgrade_d(struct krwlock *__restrict self,
                         struct timespec const *__restrict abstime,
                         size_t skip)
#else
__crit RWLOCK_DEBUG_NOINLINE kerrno_t
krwlock_timedupgrade(struct krwlock *__restrict self,
                     struct timespec const *__restrict abstime)
#endif
{
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_krwlock(self);
#if KCONFIG_HAVE_DEBUG_TRACKEDRWLOCK
 error = __krwlock_atomic_timedupgrade_d(self,abstime,skip+1);
#else
 error = krwlock_atomic_timedupgrade(self,abstime);
#endif
 if __likely(KE_ISOK(error)) return error;
 if __unlikely(error != KE_PERM) return error;
 /* The read-lock was lost and atomicity failed due to some other task.
  * >> Try the manual way and acquire a regular write-lock. */
#if KCONFIG_HAVE_DEBUG_TRACKEDRWLOCK
 error = __krwlock_timedbeginwrite_d(self,abstime,skip+1);
#else
 error = krwlock_timedbeginwrite(self,abstime);
#endif
 return KE_ISERR(error) ? error : KS_UNLOCKED;
}
__crit RWLOCK_DEBUG_NOINLINE kerrno_t
krwlock_timeoutupgrade(struct krwlock *__restrict self,
                       struct timespec const *__restrict timeout) {
 struct timespec abstime;
 KTASK_CRIT_MARK
 kassert_krwlock(self);
 ktime_getnow(&abstime);
 __timespec_add(&abstime,timeout);
#if KCONFIG_HAVE_DEBUG_TRACKEDRWLOCK
 return __krwlock_timedupgrade_d(self,&abstime,1);
#else
 return krwlock_timedupgrade(self,&abstime);
#endif
}






#if KCONFIG_HAVE_DEBUG_TRACKEDRWLOCK
__crit RWLOCK_DEBUG_NOINLINE kerrno_t
krwlock_timedbeginread(struct krwlock *__restrict self,
                       struct timespec const *__restrict abstime) {
 return __krwlock_timedbeginread_d(self,abstime,1);
}
__crit RWLOCK_DEBUG_NOINLINE kerrno_t
krwlock_beginwrite(struct krwlock *__restrict self) {
 return __krwlock_beginwrite_d(self,1);
}
__crit RWLOCK_DEBUG_NOINLINE kerrno_t
krwlock_timedbeginwrite(struct krwlock *__restrict self,
                        struct timespec const *__restrict abstime) {
 return __krwlock_timedbeginwrite_d(self,abstime,1);
}
__crit RWLOCK_DEBUG_NOINLINE kerrno_t
krwlock_atomic_upgrade(struct krwlock *__restrict self) {
 return __krwlock_atomic_upgrade_d(self,1);
}
__crit RWLOCK_DEBUG_NOINLINE kerrno_t
krwlock_atomic_timedupgrade(struct krwlock *__restrict self,
                            struct timespec const *__restrict abstime) {
 return __krwlock_atomic_timedupgrade_d(self,abstime,1);
}
__crit RWLOCK_DEBUG_NOINLINE kerrno_t
krwlock_timedupgrade(struct krwlock *__restrict self,
                     struct timespec const *__restrict abstime) {
 return __krwlock_timedupgrade_d(self,abstime,1);
}
#endif /* KCONFIG_HAVE_DEBUG_TRACKEDRWLOCK */

#undef KRWLOCK_DEADLOCK_HELP_BEGIN
#undef KRWLOCK_DEADLOCK_HELP_END
#undef RWLOCK_DEBUG_NOINLINE

__DECL_END

#endif /* !__KOS_KERNEL_SCHED_SYNC_RWLOCK_C_INL__ */

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
#ifndef __KOS_KERNEL_RWLOCK_H__
#define __KOS_KERNEL_RWLOCK_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/kernel/object.h>
#include <kos/compiler.h>
#include <kos/atomic.h>
#include <kos/kernel/signal.h>
#include <kos/kernel/features.h>
#if KCONFIG_HAVE_DEBUG_TRACKEDRWLOCK
#include <kos/atomic.h>
#include <kos/kernel/sched_yield.h>
#endif /* KCONFIG_HAVE_DEBUG_TRACKEDRWLOCK */

__DECL_BEGIN

#define KOBJECT_MAGIC_RWLOCK 0x5770C4 /*< RWLOCK */
#define kassert_krwlock(self) kassert_object(self,KOBJECT_MAGIC_RWLOCK)

#ifndef __ASSEMBLY__
struct timespec;
struct krwlock;
#endif /* __ASSEMBLY__ */

//////////////////////////////////////////////////////////////////////////
// A R/W lock (long: Read-write lock) is a special kind of lock
// for synchronizing read/write access of an unspecific resource.
//  - Where a regular mutex-style lock has the obvious dis-advantage
//    of only ever allowing a single task to be holding the lock,
//    a R/W lock steps in to split apart the difference between
//    holding a read-only (aka. read/shared lock), or holding
//    a read-write (aka. write/exclusive lock).
//  - Synchronization is performed in both directions, meaning
//    that in order to start reading, your task may be suspended
//    until another task that was previously writing is done.
//    And if you attempt to start writing, you will be suspended
//    until no more read operations are being performed.
// NOTE: The obvious undefined behavior here is what happens
//       when you attempt to start writing ('krwlock_beginwrite'),
//       but another task is already reading, yet before that
//       that finishes, yet another one starts reading as well:
//    >> Unlike a close-lock, a R/W lock does not impose any
//       restrictions for allowing multiple shared locks while
//       some other task is currently waiting for all of those
//       locks to go away, meaning that your write-task will
//       keep of being suspended until the (potentially) rare
//       chance of all reading tasks being done.
// TODO: Add a second r/w lock that does the opposite (doesn't allow new
//       tasks to start reading when yet another attempts to start writing).


#if __SIZEOF_POINTER__ >= __SIZEOF_SIZE_T__
#define __KRWLOCK_DEBUG_SIZEOF     (__SIZEOF_POINTER__*2)
#else
#define __KRWLOCK_DEBUG_SIZEOF     (__SIZEOF_SIZE_T__+__SIZEOF_POINTER__)
#endif

#ifndef __ASSEMBLY__
#if KCONFIG_HAVE_DEBUG_TRACKEDRWLOCK
struct krwlock_debugentry {
 struct ktask   *de_holder; /*< [1..1][lock(:r_lock)] Current holder of the lock. */
 struct tbtrace *de_locktb; /*< [1..1][lock(:r_lock)] Traceback of where the lock was acquired. */
};
union krwlock_debug {
 struct {
  __size_t                   r_locka; /*< Allocated size of the vector (NOTE: When 'rw_readc' is ZERO(0), this must be too). */
  struct krwlock_debugentry *r_lockv; /*< [0..:rw_readc][owned] Vector of lock entries. */
 } d_read; /*< [!(:rw_sig.s_flags&KRWLOCK_FLAG_WRITEMODE)] */
 struct krwlock_debugentry   d_write; /*< [:rw_sig.s_flags&KRWLOCK_FLAG_WRITEMODE] */
};

#define __KRWLOCK_DEBUG_INIT       ,{{0,NULL}}
#define __krwlock_debug_init(self) memset(self,0,sizeof(union krwlock_debug))
#else /* KCONFIG_HAVE_DEBUG_TRACKEDRWLOCK */
#define __KRWLOCK_DEBUG_INIT       /* nothing */
#define __krwlock_debug_init(self) /* nothing */
#endif /* !KCONFIG_HAVE_DEBUG_TRACKEDRWLOCK */
#endif /* !__ASSEMBLY__ */

#define KRWLOCK_OFFSETOF_SIG      (KOBJECT_SIZEOFHEAD)
#if KCONFIG_RWLOCK_READERBITS <= KSIGNAL_USERBITS
#define KRWLOCK_OFFSETOF_READC    (KOBJECT_SIZEOFHEAD+KSIGNAL_OFFSETOF_USER)
#define __KRWLOCK_OFFSETOF_DEBUG  (KOBJECT_SIZEOFHEAD+KSIGNAL_SIZEOF)
#else
#define KRWLOCK_OFFSETOF_READC  (KOBJECT_SIZEOFHEAD+KSIGNAL_SIZEOF)
#if KCONFIG_RWLOCK_READERBITS == 32
#define __KRWLOCK_OFFSETOF_DEBUG  (KOBJECT_SIZEOFHEAD+KSIGNAL_SIZEOF+4)
#elif KCONFIG_RWLOCK_READERBITS == 16
#define __KRWLOCK_OFFSETOF_DEBUG  (KOBJECT_SIZEOFHEAD+KSIGNAL_SIZEOF+2)
#elif KCONFIG_RWLOCK_READERBITS == 8
#define __KRWLOCK_OFFSETOF_DEBUG  (KOBJECT_SIZEOFHEAD+KSIGNAL_SIZEOF+1)
#else
#define __KRWLOCK_OFFSETOF_DEBUG  (KOBJECT_SIZEOFHEAD+KSIGNAL_SIZEOF+(KCONFIG_RWLOCK_READERBITS/8))
#endif
#endif
#if KCONFIG_HAVE_DEBUG_TRACKEDRWLOCK
#define KRWLOCK_OFFSETOF_DEBUG  __KRWLOCK_OFFSETOF_DEBUG
#define KRWLOCK_SIZEOF         (__KRWLOCK_OFFSETOF_DEBUG+__KRWLOCK_DEBUG_SIZEOF)
#else
#define KRWLOCK_SIZEOF          __KRWLOCK_OFFSETOF_DEBUG
#endif

#ifndef __ASSEMBLY__
struct krwlock {
 KOBJECT_HEAD
 /* v [lock(KSIGNAL_LOCK_WAIT)] Set when the lock is in write-mode. */
#define KRWLOCK_FLAG_WRITEMODE   KSIGNAL_FLAG_USER(0)
#if KCONFIG_HAVE_DEBUG_TRACKEDRWLOCK
#define KRWLOCK_FLAG_BROKENREAD  KSIGNAL_FLAG_USER(1) /*< Read-mode debug informations are broken. */
#endif /* KCONFIG_HAVE_DEBUG_TRACKEDRWLOCK */
 struct ksignal          rw_sig;   /*< Underlying signal. */
#if KCONFIG_RWLOCK_READERBITS <= KSIGNAL_USERBITS
#define rw_readc         rw_sig.s_useru
#else
 __un(KCONFIG_RWLOCK_READERBITS) rw_readc; /*< [lock(KSIGNAL_LOCK_WAIT)] Amount of tasks currently performing read operations. */
#endif
#if KCONFIG_HAVE_DEBUG_TRACKEDRWLOCK
 union krwlock_debug     rw_debug; /*< [lock(KSIGNAL_LOCK_WAIT)] Debug information used for tracking locks. */
#endif /* KCONFIG_HAVE_DEBUG_TRACKEDRWLOCK */
};
#ifdef __INTELLISENSE__
extern __wunused __nonnull((1)) bool krwlock_iswritelocked(struct krwlock const *__restrict self);
extern __wunused __nonnull((1)) bool krwlock_isreadlocked(struct krwlock const *__restrict self);
#else
#define krwlock_iswritelocked(self)  ((self)->rw_sig.s_flags&KRWLOCK_FLAG_WRITEMODE)
#define krwlock_isreadlocked(self)   ((self)->rw_readc!=0)
#endif

#if defined(KOBJECT_INIT_IS_MEMSET_ZERO) && \
    defined(KSIGNAL_INIT_IS_MEMSET_ZERO)
#define KRWLOCK_INIT_IS_MEMSET_ZERO 1
#endif

#if KCONFIG_RWLOCK_READERBITS <= KSIGNAL_USERBITS
#define KRWLOCK_INIT  {KOBJECT_INIT(KOBJECT_MAGIC_RWLOCK) KSIGNAL_INIT __KRWLOCK_DEBUG_INIT}
#else
#define KRWLOCK_INIT  {KOBJECT_INIT(KOBJECT_MAGIC_RWLOCK) KSIGNAL_INIT,0 __KRWLOCK_DEBUG_INIT}
#endif

#ifdef __INTELLISENSE__
extern __nonnull((1)) void krwlock_init(struct krwlock *__restrict self);
#elif defined(KSIGNAL_INIT_IS_MEMSET_ZERO)
#define krwlock_init(self) kobject_initzero(self,KOBJECT_MAGIC_RWLOCK)
#elif KCONFIG_RWLOCK_READERBITS <= KSIGNAL_USERBITS
#define krwlock_init(self) \
 __xblock({ struct krwlock *const __rwself = (self);\
            kobject_init(__rwself,KOBJECT_MAGIC_RWLOCK);\
            ksignal_init(&__rwself->rw_sig);\
            __krwlock_debug_init(&__rwself->rw_debug);\
            (void)0;\
 })
#else
#define krwlock_init(self) \
 __xblock({ struct krwlock *const __rwself = (self);\
            kobject_init(__rwself,KOBJECT_MAGIC_RWLOCK);\
            __rwself->rw_readc = 0;\
            ksignal_init(&__rwself->rw_sig);\
            __krwlock_debug_init(&__rwself->rw_debug);\
            (void)0;\
 })
#endif

//////////////////////////////////////////////////////////////////////////
// Close a given R/W lock.
// >> 'krwlock_close' can be called to close a r/w lock,
//    causing any successive attempts to start reading
//    or writing to fail.
// >> In order to close the R/W lock, an exclusive lock
//    must be acquired, leaving this function to share
//    its synchronization characteristics with 'krwlock_beginwrite'
// @return: KE_OK:         The lock was successfully closed.
// @return: KS_UNCHANGED:  The given lock was already closed.
// @return: KE_WOULDBLOCK: [krwlock_tryclose] Failed to acquire an exclusive lock immediately.
// @return: KE_TIMEDOUT:   [krwlock_(timed|timeout)close] The given timeout has expired.
// @return: KE_INTR:      [!krwlock_tryclose] The calling task was interrupted.
__local __wunused __nonnull((1))   kerrno_t krwlock_close(struct krwlock *__restrict self);
__local __wunused __nonnull((1))   kerrno_t krwlock_tryclose(struct krwlock *__restrict self);
__local __wunused __nonnull((1,2)) kerrno_t krwlock_timedclose(struct krwlock *__restrict self, struct timespec const *__restrict abstime);
__local __wunused __nonnull((1,2)) kerrno_t krwlock_timeoutclose(struct krwlock *__restrict self, struct timespec const *__restrict timeout);

//////////////////////////////////////////////////////////////////////////
// Reset a given R/W lock after it has been closed.
// @return: KE_OK:        The lock was successfully reset.
// @return: KS_UNCHANGED: The given lock was not closed.
extern __nonnull((1)) kerrno_t krwlock_reset(struct krwlock *__restrict self);

//////////////////////////////////////////////////////////////////////////
// Acquire read-only access from a given R/W lock.
// >> Calling this function grants the calling task shared,
//    but read-only access to the associated resource.
// @return: KE_OK:         The caller is now holding a shared ticket to the
//                         given resource and must call 'krwlock_endread'
//                         once they are ready to release it.
// @return: KE_DESTROYED:  The given lock was closed.
// @return: KE_WOULDBLOCK: [krwlock_trybeginread] Failed to acquire a shared lock immediately.
// @return: KE_TIMEDOUT:   [krwlock_(timed|timeout)beginread] The given timeout has expired.
// @return: KE_INTR:      [!krwlock_trybeginread] The calling task was interrupted.
extern __crit __wunused __nonnull((1))   kerrno_t krwlock_beginread(struct krwlock *__restrict self);
extern __crit __wunused __nonnull((1))   kerrno_t krwlock_trybeginread(struct krwlock *__restrict self);
extern __crit __wunused __nonnull((1,2)) kerrno_t krwlock_timedbeginread(struct krwlock *__restrict self, struct timespec const *__restrict abstime);
extern __crit __wunused __nonnull((1,2)) kerrno_t krwlock_timeoutbeginread(struct krwlock *__restrict self, struct timespec const *__restrict timeout);

//////////////////////////////////////////////////////////////////////////
// Acquire read-write access from a given R/W lock.
// >> Calling this function grants the calling task exclusive,
//    and read-write access to the associated resource.
// @return: KE_OK:         The caller is now holding an exclusive ticket to the
//                         given resource and must call 'krwlock_endwrite'
//                         once they are ready to release it.
// @return: KE_DESTROYED:  The given lock was closed.
// @return: KE_WOULDBLOCK: [krwlock_trybeginwrite] Failed to acquire an exclusive lock immediately.
// @return: KE_TIMEDOUT:   [krwlock_(timed|timeout)beginwrite] The given timeout has passed.
// @return: KE_INTR:      [!krwlock_trybeginwrite] The calling task was interrupted.
extern __crit __wunused __nonnull((1))   kerrno_t krwlock_beginwrite(struct krwlock *__restrict self);
extern __crit __wunused __nonnull((1))   kerrno_t krwlock_trybeginwrite(struct krwlock *__restrict self);
extern __crit __wunused __nonnull((1,2)) kerrno_t krwlock_timedbeginwrite(struct krwlock *__restrict self, struct timespec const *__restrict abstime);
extern __crit __wunused __nonnull((1,2)) kerrno_t krwlock_timeoutbeginwrite(struct krwlock *__restrict self, struct timespec const *__restrict timeout);

//////////////////////////////////////////////////////////////////////////
// Release a shared (read-only) or exclusive (read-write) lock,
// after it has been acquired during a previous call to
// 'krwlock_beginread' or 'krwlock_beginwrite'
// NOTE: 'krwlock_end' can be called to either release a read-
//       or a write-lock, Though a call to this function is not
//       recommended, as it may lead to half-a$$ed code.
// @return: KE_OK:        The lock was released, and waiting tasks were signaled.
// @return: KS_UNCHANGED: [krwlock_endread] More tasks holding read-locks
//                        are preventing the read-done signal being send.
//                        This is not an error, but simply means no writers were woken.
// @return: KS_EMPTY:     All shared/exclusive locks have been released,
//                        but no tasks were waiting, and therefor none
//                        were signaled.
//                        NOTE: This is NOT an error, but simply additional
//                              information to the caller.
extern __crit __nonnull((1)) kerrno_t krwlock_endread(struct krwlock *__restrict self);
extern __crit __nonnull((1)) kerrno_t krwlock_endwrite(struct krwlock *__restrict self);
extern __crit __nonnull((1)) kerrno_t krwlock_end(struct krwlock *__restrict self);

//////////////////////////////////////////////////////////////////////////
// Downgrade a read-write (exclusive) lock to a read-only (shared) lock.
// - Unlike the obvious alternate code,
//   >> krwlock_endwrite(self);
//   >> krwlock_beginread(self);
//   This operation is non-blocking and guaranties that no other writer
//   can take a hold of the lock in some minuscule amount of time during
//   which a write-lock would have otherwise been available.
// - It also prevents a possible KE_DESTROYED error that may
//   have occurred during a call to 'krwlock_beginread'.
// @return: KE_OK:    The write-lock was released, and waiting tasks were signaled.
// @return: KS_EMPTY: The exclusive lock has been released, but no tasks
//                    were waiting, and therefor none were signaled.
//                    NOTE: This is NOT an error, but simply
//                          additional information for the caller.
extern __crit __nonnull((1)) kerrno_t krwlock_downgrade(struct krwlock *__restrict self);


//////////////////////////////////////////////////////////////////////////
// Upgrade a read-lock into a write lock, potentially while ensuring
// no other task had a chance to begin writing before the caller
// was able to begin doing so them self (aka. atomic upgrade).
// WARNING: In all KE_ISERR(return) situations, the read-lock
//          previously held by the caller will be lost.
// @return: KE_OK:          The read-lock held by the caller was upgraded into a write-lock.
// @return: KS_BLOCKING:   [krwlock_{atomic_}{timed|timeout}upgrade]
//                          Similar to KE_OK, but the calling task was blocked for a moment,
//                          meaning that a call to 'krwlock_*tryupgrade' would have
//                          failed with 'KE_WOULDBLOCK'.
// @return: KE_TIMEDOUT:   [krwlock_*(timed|timeout)upgrade] The given timeout has expired.
// @return: KE_INTR:       [!krwlock_*tryupgrade] The calling task was interrupted.
// @return: KE_WOULDBLOCK: [krwlock_*tryupgrade] Failed to upgrade the lock immediately.
// @return: KE_WOULDBLOCK: [krwlock_atomic_upgrade|krwlock_atomic_(timed|timeout)upgrade]
//                          The read lock held by the caller was released,
//                          but because another task attempted to upgrade
//                          its lock at the same time, it was impossible
//                          to uphold the guaranty of no other task acquiring
//                          a write-lock before the caller would get their's.
//                          NOTE: 'krwlock_upgrade' does not technically solve this problem,
//                                simply handling this situation by acquiring a normal
//                                write-lock with the same semantics as calling:
//                             >> krwlock_endread(self);
//                             >> krwlock_beginwrite(self);
//                                Though using 'krwlock_upgrade' will be faster in most
//                                situations when this special case does not arise.
//                          WARNING: The caller must _NOT_ call 'krwlock_endread'
//                          HINT: This error is not returned by 'krwlock_atomic_tryupgrade'
// @return: KE_DESTROYED:  [krwlock_*{timed|timeout}upgrade]
//                          Due to how upgrades handle failure, a situation in which
//                          the R/W lock can be destroyed before the caller was able
//                          to acquire their write-lock can arise, with handling of the
//                          R/W lock's destruction being performed the same way it would
//                          have when 'krwlock_beginwrite' returned the same error.
// @return: KS_UNLOCKED:   [!*atomic*] Successfully managed to upgrade the lock,
//                          but in order to do so, the calling task momentarily held
//                          no lock at all. (*atomic* would have returned 'KE_WOULDBLOCK')
//                          The caller should handle this signal by reloading cached
//                          variables affected by the lock, as their previous values
//                          may no longer be up to date.
extern __crit __wunused __nonnull((1))   kerrno_t krwlock_upgrade(struct krwlock *__restrict self);
extern __crit __wunused __nonnull((1))   kerrno_t krwlock_tryupgrade(struct krwlock *__restrict self);
extern __crit __wunused __nonnull((1,2)) kerrno_t krwlock_timedupgrade(struct krwlock *__restrict self, struct timespec const *__restrict abstime);
extern __crit __wunused __nonnull((1,2)) kerrno_t krwlock_timeoutupgrade(struct krwlock *__restrict self, struct timespec const *__restrict timeout);
extern __crit __wunused __nonnull((1))   kerrno_t krwlock_atomic_upgrade(struct krwlock *__restrict self);
extern __crit __wunused __nonnull((1,2)) kerrno_t krwlock_atomic_timedupgrade(struct krwlock *__restrict self, struct timespec const *__restrict abstime);
extern __crit __wunused __nonnull((1,2)) kerrno_t krwlock_atomic_timeoutupgrade(struct krwlock *__restrict self, struct timespec const *__restrict timeout);
#ifdef __INTELLISENSE__
extern __crit __wunused __nonnull((1))   kerrno_t krwlock_atomic_tryupgrade(struct krwlock *__restrict self);
#else /* try-upgrade is already atomic by nature. */
#define krwlock_atomic_tryupgrade        krwlock_tryupgrade
#endif


/* Since a closed R/W-lock must be in write-mode and because
 * in order to close a R/W-lock, you must first acquire a write-lock,
 * semantically speaking this is the simplest form of closing such a lock. */
#if KCONFIG_HAVE_DEBUG_TRACKEDRWLOCK
#define _krwlock_endwrite_andclose(self) \
 (assert((self)->rw_debug.d_write.de_holder == ktask_self()),\
    free((self)->rw_debug.d_write.de_locktb),\
    ksignal_close(&(self)->rw_sig))
#else /* KCONFIG_HAVE_DEBUG_TRACKEDRWLOCK */
#define _krwlock_endwrite_andclose(self) \
    ksignal_close(&(self)->rw_sig)
#endif /* !KCONFIG_HAVE_DEBUG_TRACKEDRWLOCK */

#ifndef __INTELLISENSE__
__local kerrno_t
krwlock_close(struct krwlock *__restrict self) {
 kerrno_t error; kassert_krwlock(self);
 if ((error = krwlock_beginwrite(self)) == KE_DESTROYED) return KS_UNCHANGED;
 else if __unlikely(KE_ISERR(error)) return error;
 _krwlock_endwrite_andclose(self);
 return KE_OK;
}
__local kerrno_t
krwlock_tryclose(struct krwlock *__restrict self) {
 kerrno_t error; kassert_krwlock(self);
 if ((error = krwlock_trybeginwrite(self)) == KE_DESTROYED) return KS_UNCHANGED;
 else if __unlikely(KE_ISERR(error)) return error;
 _krwlock_endwrite_andclose(self);
 return KE_OK;
}
__local kerrno_t
krwlock_timedclose(struct krwlock *__restrict self,
                   struct timespec const *__restrict abstime) {
 kerrno_t error; kassert_krwlock(self);
 if ((error = krwlock_timedbeginwrite(self,abstime)) == KE_DESTROYED) return KS_UNCHANGED;
 else if __unlikely(KE_ISERR(error)) return error;
 _krwlock_endwrite_andclose(self);
 return KE_OK;
}
__local kerrno_t
krwlock_timeoutclose(struct krwlock *__restrict self,
                     struct timespec const *__restrict timeout) {
 kerrno_t error; kassert_krwlock(self);
 if ((error = krwlock_timeoutbeginwrite(self,timeout)) == KE_DESTROYED) return KS_UNCHANGED;
 else if __unlikely(KE_ISERR(error)) return error;
 _krwlock_endwrite_andclose(self);
 return KE_OK;
}
#endif
#endif /* !__ASSEMBLY__ */

__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_RWLOCK_H__ */

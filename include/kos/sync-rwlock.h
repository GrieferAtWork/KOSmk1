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
#ifndef __KOS_SYNC_RWLOCK_H__
#define __KOS_SYNC_RWLOCK_H__ 1

#include <kos/config.h>
#include <kos/compiler.h>

#ifndef __ASSEMBLY__
#ifndef __KERNEL__
#include <kos/atomic.h>
#include <kos/errno.h>
#include <kos/futex.h>
#ifndef __INTELLISENSE__
#ifdef __DEBUG__
#include <assert.h>
#endif
#endif

/* Userspace synchronization primitives. */
__DECL_BEGIN

struct timespec;

struct krwlock { kfutex_t rw_futex; /*< Futex/write-mode/read-count. */ };
#define _KRWLOCK_WRITEMODE 0x80000000 /* Flag: R/W lock is in write-mode. */
#define _KRWLOCK_READCOUNT 0x7fffffff /* Mask: R/W lock read counter. */

#define KRWLOCK_INIT  {0}

#ifdef __INTELLISENSE__
extern __wunused __nonnull((1)) bool krwlock_iswritelocked(struct krwlock const *__restrict self);
extern __wunused __nonnull((1)) bool krwlock_isreadlocked(struct krwlock const *__restrict self);
#else
#define krwlock_iswritelocked(self)  (((self)->rw_futex&_KRWLOCK_WRITEMODE)!=0)
#define krwlock_isreadlocked(self)   (((self)->rw_futex&_KRWLOCK_READCOUNT)!=0)
#endif

//////////////////////////////////////////////////////////////////////////
// Acquire read-only access from a given R/W lock.
// >> Calling this function grants the calling task shared,
//    but read-only access to the associated resource.
// @return: KE_OK:          The caller is now holding a shared ticket to the
//                          given resource and must call 'krwlock_endread'
//                          once they are ready to release it.
// @return: KE_FAULT:      [krwlock_timedbeginread] The given 'abstime' pointer is faulty and non-NULL.
// @return: KE_NOMEM:      [!krwlock_trybeginread] Failed to allocate a missing kernel futex.
// @return: KE_TIMEDOUT:   [krwlock_{timed}beginread] The given 'abstime' or a previously set timeout has expired.
// @return: KE_WOULDBLOCK: [krwlock_trybeginread] Failed to acquire a shared lock immediately.
// @return: KE_OVERFLOW:    Too many other threads are already holding a read-lock.
__local __wunused __nonnull((1)) kerrno_t krwlock_beginread(struct krwlock *__restrict self);
__local __wunused __nonnull((1)) kerrno_t krwlock_trybeginread(struct krwlock *__restrict self);
__local __wunused __nonnull((1)) kerrno_t krwlock_timedbeginread(struct krwlock *__restrict self, struct timespec const *__restrict abstime);

//////////////////////////////////////////////////////////////////////////
// Acquire read-write access from a given R/W lock.
// >> Calling this function grants the calling task exclusive,
//    and read-write access to the associated resource.
// @return: KE_OK:          The caller is now holding an exclusive ticket to the
//                          given resource and must call 'krwlock_endwrite'
//                          once they are ready to release it.
// @return: KE_FAULT:      [krwlock_timedbeginwrite] The given 'abstime' pointer is faulty and non-NULL.
// @return: KE_NOMEM:      [!krwlock_trybeginwrite] Failed to allocate a missing kernel futex.
// @return: KE_TIMEDOUT:   [krwlock_{timed}beginwrite] The given 'abstime' or a previously set timeout has expired.
// @return: KE_WOULDBLOCK: [krwlock_trybeginwrite] Failed to acquire an exclusive lock immediately.
__local __wunused __nonnull((1)) kerrno_t krwlock_beginwrite __P((struct krwlock *__restrict __self));
__local __wunused __nonnull((1)) kerrno_t krwlock_trybeginwrite __P((struct krwlock *__restrict __self));
__local __wunused __nonnull((1)) kerrno_t krwlock_timedbeginwrite __P((struct krwlock *__restrict __self, struct timespec const *__restrict __abstime));

//////////////////////////////////////////////////////////////////////////
// Release a shared (read-only) or exclusive (read-write) lock,
// after it has been acquired during a previous call to
// 'krwlock_beginread' or 'krwlock_beginwrite'.
// NOTE: 'krwlock_end' can be called to either release a read-
//       or a write-lock, Though a call to this function is not
//       recommended, as it may lead to half-a$$ed code.
// @return: KE_OK:         The lock was released, and waiting tasks were signaled.
// @return: KS_UNCHANGED: [krwlock_end{read}] More tasks holding read-locks
//                         are preventing the read-done signal being send.
//                         This is not an error, but simply means no writers were woken.
// @return: KS_EMPTY:      All shared/exclusive locks have been released,
//                         but no tasks were waiting, and therefor none
//                         were signaled.
//                         NOTE: This is NOT an error, but simply additional
//                               information to the caller.
// @return: KE_PERM:      [*_safe] The given RW-lock was not locked properly
__local __nonnull((1)) kerrno_t krwlock_endread __P((struct krwlock *__restrict __self));
__local __nonnull((1)) kerrno_t krwlock_endwrite __P((struct krwlock *__restrict __self));
__local __nonnull((1)) kerrno_t krwlock_end __P((struct krwlock *__restrict __self));
__local __nonnull((1)) kerrno_t krwlock_endread_safe __P((struct krwlock *__restrict __self));
__local __nonnull((1)) kerrno_t krwlock_endwrite_safe __P((struct krwlock *__restrict __self));
__local __nonnull((1)) kerrno_t krwlock_end_safe __P((struct krwlock *__restrict __self));



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
// @return: KE_PERM: [krwlock_downgrade_safe] The rw-lock wasn't in write-mode before.
__local __nonnull((1)) kerrno_t krwlock_downgrade __P((struct krwlock *__restrict __self));
__local __nonnull((1)) kerrno_t krwlock_downgrade_safe __P((struct krwlock *__restrict __self));


//////////////////////////////////////////////////////////////////////////
// Upgrade a read-lock into a write lock, potentially while ensuring
// no other task had a chance to begin writing before the caller
// was able to begin doing so them self (aka. atomic upgrade).
// WARNING: In all KE_ISERR(return) situations, the read-lock
//          previously held by the caller will be lost.
// @return: KE_OK:          The read-lock held by the caller was upgraded into a write-lock.
// @return: KS_UNLOCKED:    Successfully managed to upgrade the lock, but in order
//                          to do so, the calling task momentarily held no lock at all.
//                          The caller should handle this signal by reloading cached
//                          variables affected by the lock, as their previous values
//                          may no longer be up to date.
// @return: KE_FAULT:      [krwlock_timedupgrade*] The given 'abstime' pointer is faulty and non-NULL.
// @return: KE_TIMEDOUT:   [krwlock_{timed}upgrade*] The given 'abstime' or a previously set timeout has expired.
// @return: KE_WOULDBLOCK: [krwlock_tryupgrade*] Failed to upgrade the lock immediately.
// @return: KE_FAULT:      [krwlock_{timed}upgrade*] The given 'abstime' pointer is faulty and non-NULL.
// @return: KE_NOMEM:      [krwlock_{timed}upgrade*] Failed to allocate a missing kernel futex.
// @return: KE_TIMEDOUT:   [krwlock_{timed}upgrade*] The given 'abstime' or a previously set timeout has expired.
// @return: KE_PERM:       [*_safe] The rw-lock wasn't in read-mode before.
__local __wunused __nonnull((1)) kerrno_t krwlock_upgrade __P((struct krwlock *__restrict __self));
__local __wunused __nonnull((1)) kerrno_t krwlock_tryupgrade __P((struct krwlock *__restrict __self));
__local __wunused __nonnull((1)) kerrno_t krwlock_timedupgrade __P((struct krwlock *__restrict __self, struct timespec const *__restrict __abstime));
__local __wunused __nonnull((1)) kerrno_t krwlock_upgrade_safe __P((struct krwlock *__restrict __self));
__local __wunused __nonnull((1)) kerrno_t krwlock_tryupgrade_safe __P((struct krwlock *__restrict __self));
__local __wunused __nonnull((1)) kerrno_t krwlock_timedupgrade_safe __P((struct krwlock *__restrict __self, struct timespec const *__restrict __abstime));



#ifndef __INTELLISENSE__
__local kerrno_t krwlock_trybeginread __D1(struct krwlock *__restrict,__self) {
 unsigned int __oldval;
 do {
  __oldval = katomic_load(*(unsigned int *)&__self->rw_futex);
  if (__oldval&_KRWLOCK_WRITEMODE) return KE_WOULDBLOCK;
  if ((__oldval&_KRWLOCK_READCOUNT) == _KRWLOCK_READCOUNT) return KE_OVERFLOW;
 } while (!katomic_cmpxch(*(unsigned int *)&__self->rw_futex,__oldval,__oldval+1));
 return KE_OK;
}
__local kerrno_t krwlock_timedbeginread
__D2(struct krwlock *__restrict,__self,struct timespec const *__restrict,__abstime) {
 kerrno_t __error;
 while (((__error = krwlock_trybeginread(__self)) == KE_WOULDBLOCK) &&
         /* atomic: if (__self->rw_futex & _KRWLOCK_WRITEMODE) { recv(__self->rw_futex); }  */
        (__error = kfutex_cmd(&__self->rw_futex,KFUTEX_RECVIF(KFUTEX_AND),
                              _KRWLOCK_WRITEMODE,NULL,__abstime),
         KE_ISOK(__error) || __error == KE_AGAIN));
 return __error;
}
__local kerrno_t krwlock_beginread __D1(struct krwlock *__restrict,__self) {
 return krwlock_timedbeginread(__self,NULL);
}
__local kerrno_t krwlock_trybeginwrite __D1(struct krwlock *__restrict,__self) {
 unsigned int __oldval;
 do {
  __oldval = katomic_load(*(unsigned int *)&__self->rw_futex);
  if (__oldval) return KE_WOULDBLOCK; /* _KRWLOCK_READCOUNT|_KRWLOCK_WRITEMODE */
 } while (!katomic_cmpxch(*(unsigned int *)&__self->rw_futex,__oldval,__oldval|_KRWLOCK_WRITEMODE));
 return KE_OK;
}
__local kerrno_t krwlock_timedbeginwrite
__D2(struct krwlock *__restrict,__self,struct timespec const *__restrict,__abstime) {
 kerrno_t __error;
 while (((__error = krwlock_trybeginwrite(__self)) == KE_WOULDBLOCK) &&
         /* atomic: if (__self->rw_futex != 0) { recv(__self->rw_futex); }  */
        (__error = kfutex_cmd(&__self->rw_futex,KFUTEX_RECVIF(KFUTEX_NOT_EQUAL),0,NULL,__abstime),
         KE_ISOK(__error) || __error == KE_AGAIN));
 return __error;
}
__local kerrno_t krwlock_beginwrite __D1(struct krwlock *__restrict,__self) {
 return krwlock_timedbeginwrite(__self,NULL);
}


__local kerrno_t krwlock_end __D1(struct krwlock *__restrict,__self) {
 unsigned int __oldval,__newval;
 do {
  __oldval = katomic_load(*(unsigned int *)&__self->rw_futex);
  if (__oldval&_KRWLOCK_WRITEMODE) __newval = 0;
  else {
   assertf(__oldval != 0,"Neither in read, nor in write-mode");
   __newval = __oldval-1;
  }
 } while (!katomic_cmpxch(*(unsigned int *)&__self->rw_futex,__oldval,__newval));
 if (__newval) return KS_UNCHANGED; /* Left read-mode with non-zero new count. */
 return kfutex_sendall(&__self->rw_futex);
}
__local kerrno_t krwlock_endread __D1(struct krwlock *__restrict,__self) {
#ifdef __DEBUG__
 unsigned int __oldval;
 do {
  __oldval = katomic_load(*(unsigned int *)&__self->rw_futex);
  assertf(__oldval&_KRWLOCK_READCOUNT,"Not in read-mode");
 } while (!katomic_cmpxch(*(unsigned int *)&__self->rw_futex,__oldval,__oldval-1));
 if (__oldval != 1) return KS_UNCHANGED;
#else
 /* Can simply decrement one if we don't need to assert read-mode. */
 if (katomic_decfetch(*(unsigned int *)&__self->rw_futex)) return KS_UNCHANGED;
#endif
 return kfutex_sendall(&__self->rw_futex);
}
__local kerrno_t krwlock_endwrite __D1(struct krwlock *__restrict,__self) {
 assertef(katomic_fetchand(*(unsigned int *)&__self->rw_futex,
        ~(_KRWLOCK_WRITEMODE))&_KRWLOCK_WRITEMODE,"Not in write-mode");
 return kfutex_sendall(&__self->rw_futex);
}
__local kerrno_t krwlock_end_safe __D1(struct krwlock *__restrict,__self) {
 unsigned int __oldval,__newval;
 do {
  __oldval = katomic_load(*(unsigned int *)&__self->rw_futex);
  if (__oldval&_KRWLOCK_WRITEMODE) __newval = 0;
  else {
   if __unlikely(!__oldval) return KE_PERM; /* Neither write-, nor read-mode. */
   __newval = __oldval-1;
  }
 } while (!katomic_cmpxch(*(unsigned int *)&__self->rw_futex,__oldval,__newval));
 if (__newval) return KS_UNCHANGED; /* Left read-mode with non-zero new count. */
 return kfutex_sendall(&__self->rw_futex);
}
__local kerrno_t krwlock_endread_safe __D1(struct krwlock *__restrict,__self) {
 unsigned int __oldval;
 do {
  __oldval = katomic_load(*(unsigned int *)&__self->rw_futex);
  if __unlikely(!(__oldval&_KRWLOCK_READCOUNT)) return KE_PERM; /* Not in read-mode. */
 } while (!katomic_cmpxch(*(unsigned int *)&__self->rw_futex,__oldval,__oldval-1));
 if (__oldval != 1) return KS_UNCHANGED;
 return kfutex_sendall(&__self->rw_futex);
}
__local kerrno_t krwlock_endwrite_safe __D1(struct krwlock *__restrict,__self) {
 if (!(katomic_fetchand(*(unsigned int *)&__self->rw_futex,
     ~(_KRWLOCK_WRITEMODE))&_KRWLOCK_WRITEMODE)) return KE_PERM; /* Not in write-mode. */
 return kfutex_sendall(&__self->rw_futex);
}

__local kerrno_t krwlock_downgrade __D1(struct krwlock *__restrict,__self) {
 /* Switch from write-mode to 1-reader mode. */
 asserte(katomic_cmpxch(*(unsigned int *)&__self->rw_futex,_KRWLOCK_WRITEMODE,1));
 return kfutex_sendall(&__self->rw_futex);
}
__local kerrno_t krwlock_downgrade_safe __D1(struct krwlock *__restrict,__self) {
 /* Switch from write-mode to 1-reader mode. */
 if __unlikely(!katomic_cmpxch(*(unsigned int *)&__self->rw_futex,_KRWLOCK_WRITEMODE,1)) return KE_PERM;
 return kfutex_sendall(&__self->rw_futex);
}

__local kerrno_t krwlock_tryupgrade __D1(struct krwlock *__restrict,__self) {
 unsigned int __oldval;
 do {
  __oldval = katomic_load(*(unsigned int *)&__self->rw_futex);
  assertf(__oldval&_KRWLOCK_READCOUNT,"Not in read-mode");
  if (__oldval != 1) return KE_WOULDBLOCK; /* To upgrade, the caller must be the only reader. */
 } while (!katomic_cmpxch(*(unsigned int *)&__self->rw_futex,__oldval,_KRWLOCK_WRITEMODE));
 return KE_OK;
}
__local kerrno_t krwlock_timedupgrade
__D2(struct krwlock *__restrict,__self,struct timespec const *__restrict,__abstime) {
 kerrno_t __error;
 unsigned int __oldval,__newval;
 do {
  /* Enter write-mode, or release read-lock. */
  __oldval = katomic_load(*(unsigned int *)&__self->rw_futex);
  assertf(__oldval&_KRWLOCK_READCOUNT,"Not in read-mode");
  if (__oldval == 1) __newval = _KRWLOCK_WRITEMODE;
  else __newval = __oldval-1;
 } while (!katomic_cmpxch(*(unsigned int *)&__self->rw_futex,__oldval,__newval));
 if (__newval == _KRWLOCK_WRITEMODE) return KE_OK; /* Atomic switch to write-mode. */
 /* Difficult case: We lost our read-lock and must acquire a regular write-lock.
  * >> For this case we return KS_UNLOCKED upon success because the caller must reload protected variables. */
 __error = krwlock_timedbeginwrite(__self,__abstime);
 return __error == KE_OK ? KS_UNLOCKED : __error;
}
__local kerrno_t krwlock_upgrade __D1(struct krwlock *__restrict,__self) {
 return krwlock_timedupgrade(__self,NULL);
}
__local kerrno_t krwlock_tryupgrade_safe __D1(struct krwlock *__restrict,__self) {
 unsigned int __oldval;
 do {
  __oldval = katomic_load(*(unsigned int *)&__self->rw_futex);
  if (!(__oldval&_KRWLOCK_READCOUNT)) return KE_PERM; /* Not in read-mode. */
  if (__oldval != 1) return KE_WOULDBLOCK; /* To upgrade, the caller must be the only reader. */
 } while (!katomic_cmpxch(*(unsigned int *)&__self->rw_futex,__oldval,_KRWLOCK_WRITEMODE));
 return KE_OK;
}
__local kerrno_t krwlock_timedupgrade_safe
__D2(struct krwlock *__restrict,__self,struct timespec const *__restrict,__abstime) {
 kerrno_t __error;
 unsigned int __oldval,__newval;
 do {
  /* Enter write-mode, or release read-lock. */
  __oldval = katomic_load(*(unsigned int *)&__self->rw_futex);
  if (!(__oldval&_KRWLOCK_READCOUNT)) return KE_PERM; /* Not in read-mode. */
  if (__oldval == 1) __newval = _KRWLOCK_WRITEMODE;
  else __newval = __oldval-1;
 } while (!katomic_cmpxch(*(unsigned int *)&__self->rw_futex,__oldval,__newval));
 if (__newval == _KRWLOCK_WRITEMODE) return KE_OK; /* Atomic switch to write-mode. */
 /* Difficult case: We lost our read-lock and must acquire a regular write-lock.
  * >> For this case we return KS_UNLOCKED upon success because the caller must reload protected variables. */
 __error = krwlock_timedbeginwrite(__self,__abstime);
 return __error == KE_OK ? KS_UNLOCKED : __error;
}
__local kerrno_t krwlock_upgrade_safe __D1(struct krwlock *__restrict,__self) {
 return krwlock_timedupgrade_safe(__self,NULL);
}

#endif


__DECL_END
#else /* !__KERNEL__ */
#include <kos/kernel/rwlock.h>
#endif /* __KERNEL__ */
#endif /* !__ASSEMBLY__ */

#endif /* !__KOS_SYNC_RWLOCK_H__ */

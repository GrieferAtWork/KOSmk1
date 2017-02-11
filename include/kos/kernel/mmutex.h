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
#ifndef __KOS_KERNEL_MMUTEX_H__
#define __KOS_KERNEL_MMUTEX_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/compiler.h>
#include <kos/errno.h>
#include <kos/kernel/signal.h>
#include <strings.h>
#if KDEBUG_HAVE_TRACKEDMMUTEX
#include <traceback.h>
#endif /* KDEBUG_HAVE_TRACKEDMMUTEX */

__DECL_BEGIN

#define KOBJECT_MAGIC_MMUTEX  0x7707E3E /*< MMUTEX */
#define kassert_kmmutex(self) kassert_object(self,KOBJECT_MAGIC_MMUTEX)

#if KDEBUG_HAVE_TRACKEDMMUTEX
struct kmmutex_debug {
 struct ktask      *md_holder; /*< [0..1][lock(this)] Current holder of the mutex lock. */
 struct dtraceback *md_locktb; /*< [0..1][lock(this)] Traceback of where the lock was acquired. */
};
#endif /* KDEBUG_HAVE_TRACKEDMMUTEX */

//////////////////////////////////////////////////////////////////////////
// KMMUTEX_LOCKC:   Max amount of available multi-mutex locks (usually 16)
// KMMUTEX_LOCK(n): Used to create the mask of a given lock by its id (0..KMMUTEX_LOCKC-1)
#define KMMUTEX_LOCKC            KSIGNAL_USERBITS
#define KMMUTEX_LOCK(id)        (1 << (id))
#define KMMUTEX_LOCKID(lock)    (ffs(lock)-1)
#define KMMUTEX_ISONELOCK(lock) ((lock) == KMMUTEX_LOCK(KMMUTEX_LOCKID(lock)))
#define KMMUTEX_LOCKMASK        ((1 << KMMUTEX_LOCKC)-1)

//////////////////////////////////////////////////////////////////////////
// A non-recursive multi-lock mutex.
struct kmmutex {
 KOBJECT_HEAD
 struct ksignal       mmx_sig;   /*< Signal used for locking. */
#define mmx_locks     mmx_sig.s_useru
#if KDEBUG_HAVE_TRACKEDMMUTEX
 struct kmmutex_debug mmx_debug[KMMUTEX_LOCKC]; /*< Debug information. */
#endif
};
#if KDEBUG_HAVE_TRACKEDMMUTEX
__local __crit void __kmmutex_free_debug(struct kmmutex *self) {
 struct kmmutex_debug *iter,*end;
 end = (iter = self->mmx_debug)+KMMUTEX_LOCKC;
 for (; iter != end; ++iter) {
  dtraceback_free(iter->md_locktb);
  iter->md_locktb = NULL;
 }
}

#define __kmmutex_init_debug(self) memset((self)->mmx_debug,0,sizeof((self)->mmx_debug))
#define __KMMUTEX_INIT_DEBUG ,{{NULL,NULL},}
#else
#define __kmmutex_init_debug(self) (void)0
#define __KMMUTEX_INIT_DEBUG
#endif
#define KMMUTEX_INIT               {KOBJECT_INIT(KOBJECT_MAGIC_MMUTEX) KSIGNAL_INIT __KMMUTEX_INIT_DEBUG}
#define KMMUTEX_INIT_LOCKED(locks) {KOBJECT_INIT(KOBJECT_MAGIC_MMUTEX) KSIGNAL_INIT_U(locks) __KMMUTEX_INIT_DEBUG}

typedef __un(KMMUTEX_LOCKC) kmmutex_lock_t;

#ifdef __INTELLISENSE__
extern void kmmutex_init(struct kmmutex *__restrict self);
extern void kmmutex_init_locked(struct kmmutex *__restrict self, kmmutex_lock_t locks);

//////////////////////////////////////////////////////////////////////////
// Returns true if at least one(kmmutex_islocked) or
// all(kmmutex_islockeds) of the specified locks are held.
extern bool kmmutex_islocked(struct kmmutex const *__restrict self, kmmutex_lock_t lock);
extern bool kmmutex_islockeds(struct kmmutex const *__restrict self, kmmutex_lock_t lock);

//////////////////////////////////////////////////////////////////////////
// Closes a given multi-mutex, causing any successive calls to 'kmmutex_lock' to fail
// NOTE: This function can safely be called when any lock is held
// WARNING: Though no checking is done to ensure that the caller is holding a lock.
// @return: KE_OK:        The multi-mutex was destroyed.
// @return: KS_UNCHANGED: The multi-mutex was already destroyed.
// @return: KS_EMPTY:     The multi-mutex was destroyed, but no task was signaled. (NOT AN ERROR)
extern __crit __nonnull((1)) kerrno_t kmmutex_close(struct kmmutex *__restrict self);

//////////////////////////////////////////////////////////////////////////
// Reset a given multi-mutex after it had previously been closed.
// @return: KE_OK:        The multi-mutex has been reset.
// @return: KS_UNCHANGED: The multi-mutex was not destroyed.
extern __crit __nonnull((1)) kerrno_t kmmutex_reset(struct kmmutex *__restrict self);
#else
#define kmmutex_islocked(self,lock)   ((katomic_load((self)->mmx_locks)&(lock))!=0)
#define kmmutex_islockeds(self,locks) ((katomic_load((self)->mmx_locks)&(locks))==(locks))
#define kmmutex_init(self) \
 __xblock({ struct kmmutex *const __kmiself = (self);\
            kobject_init(__kmiself,KOBJECT_MAGIC_MMUTEX);\
            ksignal_init(&__kmiself->mmx_sig);\
            __kmmutex_init_debug(__kmiself);\
            (void)0;\
 })
#define kmmutex_init_locked(self,locks) \
 __xblock({ struct kmmutex *const __kmiself = (self);\
            kobject_init(__kmiself,KOBJECT_MAGIC_MMUTEX);\
            ksignal_init_u(&__kmiself->mmx_sig,locks);\
            __kmmutex_init_debug(__kmiself);\
            (void)0;\
 })
#if KDEBUG_HAVE_TRACKEDMMUTEX
#define /*__crit*/ kmmutex_close(self) \
 __xblock({ struct kmmutex *const __kmmxcself = (self);\
            kerrno_t __kmmxcerr = ksignal_close(&__kmmxcself->mmx_sig,__sigself->s_useru=KMMUTEX_LOCKMASK);\
            if (__kmmxcerr != KS_UNCHANGED) __kmmutex_free_debug(__kmmxcself);\
            __xreturn __kmmxcerr;\
 })
#else
#define /*__crit*/ kmmutex_close(self) \
 ksignal_close(&(self)->mmx_sig,__sigself->s_useru=KMMUTEX_LOCKMASK)
#endif
#define /*__crit*/ kmmutex_reset(self) \
 ksignal_reset(&(self)->mmx_sig,__sigself->s_useru=0)
#endif

//////////////////////////////////////////////////////////////////////////
// Acquire an exclusive lock on the given multi-mutex
// WARNING: Only a single lock may be acquired at once using this function
//          If multiple locks should be acquired at once, 'kmmutex_locks' must be used.
// @return: KE_OK:         The caller is now holding the specified exclusive lock on the multi-mutex.
// @return: KE_DESTROYED:  The given multi-mutex was destroyed.
// @return: KE_TIMEDOUT:   [kmmutex_(timed|timeout)lock{s}] The given timeout has passed.
// @return: KE_WOULDBLOCK: [kmmutex_trylock{s}] Failed to acquire a(ll) lock(s) immediately.
// @return: KE_INTR:      [!kmmutex_trylock{s}] The calling task was interrupted.
extern __crit __wunused __nonnull((1))   kerrno_t kmmutex_lock(struct kmmutex *self, kmmutex_lock_t lock);
extern __crit __wunused __nonnull((1))   kerrno_t kmmutex_locks(struct kmmutex *self, kmmutex_lock_t locks);
extern __crit __wunused __nonnull((1))   kerrno_t kmmutex_trylock(struct kmmutex *__restrict self, kmmutex_lock_t lock);
extern __crit __wunused __nonnull((1))   kerrno_t kmmutex_trylocks(struct kmmutex *__restrict self, kmmutex_lock_t locks);
extern __crit __wunused __nonnull((1,2)) kerrno_t kmmutex_timedlock(struct kmmutex *__restrict self, struct timespec const *__restrict abstime, kmmutex_lock_t lock);
extern __crit __wunused __nonnull((1,2)) kerrno_t kmmutex_timedlocks(struct kmmutex *__restrict self, struct timespec const *__restrict abstime, kmmutex_lock_t locks);
extern __crit __wunused __nonnull((1,2)) kerrno_t kmmutex_timeoutlock(struct kmmutex *__restrict self, struct timespec const *__restrict timeout, kmmutex_lock_t lock);
extern __crit __wunused __nonnull((1,2)) kerrno_t kmmutex_timeoutlocks(struct kmmutex *__restrict self, struct timespec const *__restrict timeout, kmmutex_lock_t locks);

//////////////////////////////////////////////////////////////////////////
// Unlocks a given multi-mutex after an exclusive lock was obtained
// @return: KE_OK:        The multi-mutex was unlocked.
// @return: KS_UNCHANGED: The multi-mutex was unlocked, but with no waiting
//                        tasks, no signal was send. (NOT AN ERROR)
extern __crit __nonnull((1)) kerrno_t kmmutex_unlock(struct kmmutex *__restrict self, kmmutex_lock_t lock);
extern __crit __nonnull((1)) kerrno_t kmmutex_unlocks(struct kmmutex *__restrict self, kmmutex_lock_t locks);

__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_MMUTEX_H__ */

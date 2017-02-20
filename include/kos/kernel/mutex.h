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
#ifndef __KOS_KERNEL_MUTEX_H__
#define __KOS_KERNEL_MUTEX_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/compiler.h>
#include <kos/errno.h>
#include <kos/kernel/signal.h>
#if KCONFIG_HAVE_DEBUG_TRACKEDMUTEX
#include <traceback.h>
#endif /* KCONFIG_HAVE_DEBUG_TRACKEDMUTEX */

__DECL_BEGIN

__struct_fwd(kmutex);

#define KOBJECT_MAGIC_MUTEX  0x707E3E /*< MUTEX */
#define kassert_kmutex(self) kassert_object(self,KOBJECT_MAGIC_MUTEX)

//////////////////////////////////////////////////////////////////////////
// A non-recursive bi-state mutex.


#define KMUTEX_OFFSETOF_SIG    (KOBJECT_SIZEOFHEAD)
#define KMUTEX_OFFSETOF_LOCKED (KOBJECT_SIZEOFHEAD+KSIGNAL_OFFSETOF_USER)
#if KCONFIG_HAVE_DEBUG_TRACKEDMUTEX
#define KMUTEX_OFFSETOF_HOLDER (KOBJECT_SIZEOFHEAD+KSIGNAL_SIZEOF)
#define KMUTEX_OFFSETOF_LOCKTB (KOBJECT_SIZEOFHEAD+KSIGNAL_SIZEOF+__SIZEOF_POINTER__)
#define KMUTEX_SIZEOF          (KOBJECT_SIZEOFHEAD+KSIGNAL_SIZEOF+__SIZEOF_POINTER__*2)
#else /* KCONFIG_HAVE_DEBUG_TRACKEDMUTEX */
#define KMUTEX_SIZEOF          (KOBJECT_SIZEOFHEAD+KSIGNAL_SIZEOF)
#endif /* !KCONFIG_HAVE_DEBUG_TRACKEDMUTEX */

#ifndef __ASSEMBLY__
struct kmutex {
 KOBJECT_HEAD
 struct ksignal     m_sig; /*< Signal used for locking. */
#define m_locked    m_sig.s_useru /*< [atomic] 0: unlocked; 1: locked */
#if KCONFIG_HAVE_DEBUG_TRACKEDMUTEX
 struct ktask      *m_holder; /*< [0..1][lock(this)] Current holder of the mutex lock. */
 struct dtraceback *m_locktb; /*< [0..1][lock(this)] Traceback of where the lock was acquired. */
#endif /* KCONFIG_HAVE_DEBUG_TRACKEDMUTEX */
};
#if KCONFIG_HAVE_DEBUG_TRACKEDMUTEX
#define /*__crit*/ __kmutex_free_debug(self)   dtraceback_free((self)->m_locktb)
#define /*__crit*/ __kmutex_init_debug(self) ((self)->m_holder = NULL,(self)->m_locktb = NULL)
#define            __KMUTEX_INIT_DEBUG ,NULL,NULL
#else
#define /*__crit*/ __kmutex_init_debug(self) (void)0
#define            __KMUTEX_INIT_DEBUG
#endif

#define KMUTEX_INIT         {KOBJECT_INIT(KOBJECT_MAGIC_MUTEX) KSIGNAL_INIT __KMUTEX_INIT_DEBUG}
#define KMUTEX_INIT_LOCKED  {KOBJECT_INIT(KOBJECT_MAGIC_MUTEX) KSIGNAL_INIT_U(1) __KMUTEX_INIT_DEBUG}


#ifdef __INTELLISENSE__
extern bool kmutex_islocked(struct kmutex const *__restrict self);
extern void kmutex_init(struct kmutex *__restrict self);
extern void kmutex_init_locked(struct kmutex *__restrict self);

//////////////////////////////////////////////////////////////////////////
// Closes a given mutex, causing any successive calls to 'kmutex_lock' to fail
// NOTE: This function can safely be called when the mutex is locked
// WARNING: Though no checking is done to ensure that the caller is who had locked the mutex.
// @return: KE_OK:        The mutex was destroyed.
// @return: KS_UNCHANGED: The mutex was already destroyed.
// @return: KS_EMPTY:     The mutex was destroyed, but no task was signaled. (NOT AN ERROR)
extern __crit __nonnull((1)) kerrno_t kmutex_close(struct kmutex *__restrict self);

//////////////////////////////////////////////////////////////////////////
// Reset a given mutex after it had previously been closed.
// @return: KE_OK:        The mutex has been reset.
// @return: KS_UNCHANGED: The mutex was not destroyed.
extern __crit __nonnull((1)) kerrno_t kmutex_reset(struct kmutex *__restrict self);
#else
#define kmutex_islocked(self) (katomic_load((self)->m_locked)!=0)
#define kmutex_init(self)   \
 __xblock({ struct kmutex *const __kmiself = (self);\
            kobject_init(__kmiself,KOBJECT_MAGIC_MUTEX);\
            ksignal_init(&__kmiself->m_sig);\
            __kmutex_init_debug(__kmiself);\
            (void)0;\
 })
#define kmutex_init_locked(self)   \
 __xblock({ struct kmutex *const __kmiself = (self);\
            kobject_init(__kmiself,KOBJECT_MAGIC_MUTEX);\
            ksignal_init_u(&__kmiself->m_sig,1);\
            __kmutex_init_debug(__kmiself);\
            (void)0;\
 })
#if KCONFIG_HAVE_DEBUG_TRACKEDMUTEX
#define /*__crit*/ kmutex_close(self) \
 __xblock({ struct kmutex *const __kmxcself = (self);\
            kerrno_t __kmxcerr = ksignal_close(&__kmxcself->m_sig,__sigself->s_useru=1);\
            if (__kmxcerr != KS_UNCHANGED) __kmutex_free_debug(__kmxcself);\
            __xreturn __kmxcerr;\
 })
#else
#define /*__crit*/ kmutex_close(self) \
 ksignal_close(&(self)->m_sig,__sigself->s_useru=1)
#endif
#define /*__crit*/ kmutex_reset(self) \
 ksignal_reset(&(self)->m_sig,__sigself->s_useru=0)
#endif

//////////////////////////////////////////////////////////////////////////
// Acquires an exclusive, non-recursive lock on a given mutex.
// @return: KE_OK:         The caller is now holding an exclusive lock on the mutex.
// @return: KE_DESTROYED:  The given mutex was destroyed.
// @return: KE_WOULDBLOCK: [kmutex_trylock] failed to acquire a lock immediately.
// @return: KE_TIMEDOUT:   [kmutex_(timed|timeout)lock] The given timeout has expired.
// @return: KE_INTR:      [!kmutex_trylock] The calling task was interrupted.
extern __crit __wunused __nonnull((1))   kerrno_t kmutex_lock(struct kmutex *__restrict self);
extern __crit __wunused __nonnull((1))   kerrno_t kmutex_trylock(struct kmutex *__restrict self);
extern __crit __wunused __nonnull((1,2)) kerrno_t kmutex_timedlock(struct kmutex *__restrict self, struct timespec const *__restrict abstime);
extern __crit __wunused __nonnull((1,2)) kerrno_t kmutex_timeoutlock(struct kmutex *__restrict self, struct timespec const *__restrict timeout);

//////////////////////////////////////////////////////////////////////////
// Unlocks a given mutex after an exclusive lock was obtained
// @return: KE_OK:        The mutex was unlocked.
// @return: KS_UNCHANGED: The mutex was unlocked, but with no waiting
//                        tasks, no signal was send. (NOT AN ERROR)
extern __crit __nonnull((1)) kerrno_t kmutex_unlock(struct kmutex *__restrict self);
#endif /* !__ASSEMBLY__ */

__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_MUTEX_H__ */

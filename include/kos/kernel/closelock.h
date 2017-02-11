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
#ifndef __KOS_KERNEL_CLOSELOCK_H__
#define __KOS_KERNEL_CLOSELOCK_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/compiler.h>
#include <kos/kernel/features.h>
#include <kos/atomic.h>
#include <kos/kernel/signal.h>
#include <kos/kernel/object.h>

__DECL_BEGIN
#define KOBJECT_MAGIC_CLOSELOCK 0xC705E70C /*< CLOSELOC */

struct timespec;
struct kcloselock;

#define kassert_kcloselock(self) kassert_object(self,KOBJECT_MAGIC_CLOSELOCK)


//////////////////////////////////////////////////////////////////////////
// Close Locks
//////////////////////////////////////////////////////////////////////////
//  - A close lock is a special kind of lock that is designed to
//    synchronize safe operations on a closeable object.
//  - The semantics of this lock are:
//    - An infinite amount of tasks may operate on a non-closed object
//    - Only one task is allowed to close the object, that being
//      an atomic operation that prevents more tasks from starting
//      operations.
//    - If other tasks are operating while the object is being closed,
//      the closing task waits until all operating tasks finish,
//      though no new tasks can start operating.

struct kcloselock {
 KOBJECT_HEAD
#define KCLOSELOCK_FLAG_CLOSEING  \
 KSIGNAL_FLAG_USER(0) /*< [lock(KSIGNAL_LOCK_WAIT)] Set while attempting to close the object. */
 struct ksignal cl_opsig;   /*< The signal a closing task waits for. */
 // [lock(KSIGNAL_LOCK_WAIT)] Amount of tasks currently performing operations.
#if KCLOSELOCK_OPCOUNTBITS <= KSIGNAL_USERBITS
#define cl_opcount            cl_opsig.s_useru
#else
 __un(KCLOSELOCK_OPCOUNTBITS) cl_opcount;
#endif
};
#if KCLOSELOCK_OPCOUNTBITS <= KSIGNAL_USERBITS
#define KCLOSELOCK_INIT  {KOBJECT_INIT(KOBJECT_MAGIC_CLOSELOCK) KSIGNAL_INIT}
#else
#define KCLOSELOCK_INIT  {KOBJECT_INIT(KOBJECT_MAGIC_CLOSELOCK) KSIGNAL_INIT,0}
#endif

#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Initialize/Finalize a given close lock.
extern        __nonnull((1)) void kcloselock_init(struct kcloselock *__restrict self);
extern __crit __nonnull((1)) void kcloselock_quit(struct kcloselock *__restrict self);
#else
#if KCLOSELOCK_OPCOUNTBITS <= KSIGNAL_USERBITS
#define kcloselock_init(self) \
 __xblock({ struct kcloselock *__clself = (self);\
            kobject_init(__clself,KOBJECT_MAGIC_CLOSELOCK);\
            ksignal_init(&__clself->cl_opsig);\
            (void)0;\
 })
#else
#define kcloselock_init(self) \
 __xblock({ struct kcloselock *__clself = (self);\
            __clself->cl_opcount = 0;\
            kobject_init(__clself,KOBJECT_MAGIC_CLOSELOCK);\
            ksignal_init(&__clself->cl_opsig);\
            (void)0;\
 })
#endif
#define kcloselock_quit(self) \
 (void)ksignal_close(&(self)->cl_opsig)
#endif
 


//////////////////////////////////////////////////////////////////////////
// begin/end a closelock operation
// @return: KE_OVERFLOW:  Too many concurrent operations
// @return: KE_DESTROYED: The associated object was closed/is currently closing.
//                        >> New tasks are not allowed to being operations
extern __crit __wunused kerrno_t kcloselock_beginop(struct kcloselock *__restrict self);
extern __crit               void kcloselock_endop(struct kcloselock *__restrict self);

//////////////////////////////////////////////////////////////////////////
// Set the lock to its closing state, waiting for all
// other tasks still performing closing operations.
// >> Once this function returns, it is
//    safe to close the associated object.
// @return: KE_OK:         The closelock was successfully closed.
// @return: KE_DESTROYED:  This isn't the first time the lock is closed
// @return: KE_TIMEDOUT:   The given timeout has passed, or 'kcloselock_tryclose'
//                         failed to close the lock immediately.
// @return: KE_WOULDBLOCK: [kcloselock_tryclose] Failed to close the lock immediately.
// @return: KE_INTR:      [!kcloselock_tryclose] The calling task was interrupted.
extern __crit           __nonnull((1))   kerrno_t kcloselock_close(struct kcloselock *__restrict self);
extern __crit __wunused __nonnull((1))   kerrno_t kcloselock_tryclose(struct kcloselock *__restrict self);
extern __crit __wunused __nonnull((1,2)) kerrno_t kcloselock_timedclose(struct kcloselock *__restrict self, struct timespec const *__restrict abstime);
extern __crit __wunused __nonnull((1,2)) kerrno_t kcloselock_timeoutclose(struct kcloselock *__restrict self, struct timespec const *__restrict timeout);

//////////////////////////////////////////////////////////////////////////
// Resets a close lock after it has already been closed.
// @return: KS_UNCHANGED: The lock wasn't closed to being with
extern __crit kerrno_t kcloselock_reset(struct kcloselock *__restrict self);

__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_CLOSELOCK_H__ */

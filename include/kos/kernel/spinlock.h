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
#ifndef __KOS_KERNEL_SPINLOCK_H__
#define __KOS_KERNEL_SPINLOCK_H__ 1

#include <kos/config.h>
#include <kos/atomic.h>
#include <kos/compiler.h>
#include <kos/types.h>
#include <kos/errno.h>
#include <kos/kernel/sched_yield.h>

__DECL_BEGIN

struct kspinlock;

struct kspinlock { __u8 s_spinner; };
#define KSPINLOCK_INIT       {0}

#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Initialize a given spinlock for future use.
extern __nonnull((1)) void kspinlock_init(struct kspinlock *__restrict self);

//////////////////////////////////////////////////////////////////////////
// Attempt to acquire a lock on the given spinlock.
// @return: KE_OK:         Successfully acquired a lock.
// @return: KE_WOULDBLOCK: Failed to acquire a lock as the operation would have blocked.
extern __crit __nonnull((1)) __wunused kerrno_t kspinlock_trylock(struct kspinlock *__restrict self);

//////////////////////////////////////////////////////////////////////////
// Acquire/release a given spinlock, idly waiting and actively
// performing scheduler preemption while the lock remains unavailable.
// WARNING: This kind of lock should only be used in places when
//          the section of code guarded is very small and must
//          therefor only remain locked for very short periods of time.
// >> Remember that you're the kernel, meaning that as far as
//    memory consumption goes, a regular mutex doesn't really
//    but too much of a strain of resources, but might be much
//    more efficient in the long run.
extern __crit __nonnull((1)) void kspinlock_lock(struct kspinlock *__restrict self);
extern __crit __nonnull((1)) void kspinlock_unlock(struct kspinlock *__restrict self);
#else
#define kspinlock_init(self) ((self)->s_spinner = 0)
#define kspinlock_trylock(lock) (katomic_cmpxch((lock)->s_spinner,0,1) ? KE_OK : KE_WOULDBLOCK)
#define kspinlock_lock(lock)     KTASK_SPIN(KE_ISOK(kspinlock_trylock(lock)))
#define kspinlock_unlock(lock)   katomic_store((lock)->s_spinner,0)
#endif


__DECL_END

#endif /* !__KOS_KERNEL_SPINLOCK_H__ */

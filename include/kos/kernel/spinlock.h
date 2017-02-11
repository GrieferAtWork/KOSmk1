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
#ifdef __KERNEL__
#include <kos/atomic.h>
#include <kos/compiler.h>
#include <kos/kernel/sched_yield.h>

__DECL_BEGIN

struct kspinlock;

struct kspinlock { int s_spinner; };
#define KSPINLOCK_INIT       {0}
#define kspinlock_init(self) ((self)->s_spinner = 0)

#define kspinlock_trylock(lock)            katomic_cmpxch((lock)->s_spinner,0,1)
#define kspinlock_lock(lock)    __xblock({ KTASK_SPIN(kspinlock_trylock(lock)); (void)0; })
#define kspinlock_unlock(lock)             katomic_store((lock)->s_spinner,0)

__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_SPINLOCK_H__ */

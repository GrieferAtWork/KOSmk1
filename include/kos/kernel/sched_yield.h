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
#ifndef __KOS_KERNEL_SCHED_YIELD_H__
#define __KOS_KERNEL_SCHED_YIELD_H__ 1

#include <kos/config.h>
#ifndef __ASSEMBLY__
#include <kos/compiler.h>

#ifdef __KERNEL__
#include <kos/errno.h>

__DECL_BEGIN

// Scheduler-level preemption
//////////////////////////////////////////////////////////////////////////
// Willingly Yields execution to a different task
// @return: KE_OK:        Execution has returned after other tasks were executed.
// @return: KS_UNCHANGED: No task to switch to was available.
// @return: KE_OVERFLOW:  A reference overflow occurred.
// WARNING: Upon return, interrupts have the same state as they did upon entry,
//          though in the before the function returns, they have be re-enabled,
//          or disabled multiple times.
extern kerrno_t ktask_tryyield(void);
extern void ktask_yield(void);

#ifndef __INTELLISENSE__
/* ktask_yield can be overwritten by re-defining this macro! */
#define ktask_yield (void)ktask_tryyield
#endif

__DECL_END

#else /* __KERNEL__ */
#include <kos/task.h>
#endif /* !__KERNEL__ */


#if 1 /* DEFAULT: yield, while operation fails. */
#define KTASK_SPIN(try_operation)   __xblock({ while (!(try_operation)) ktask_yield(); (void)0; })
#elif 1 /* Test case: Full preemption has no unwanted side-effects. */
#define KTASK_SPIN(try_operation)   __xblock({ do ktask_yield(); while (!(try_operation)); (void)0; })
#else /* Unused case: Don't use preemption (Don't even know why this is an option...) */
#define KTASK_SPIN(try_operation)   __xblock({ while (!(try_operation)); (void)0; })
#endif

#endif /* !__ASSEMBLY__ */


#endif /* !__KOS_KERNEL_SCHED_YIELD_H__ */

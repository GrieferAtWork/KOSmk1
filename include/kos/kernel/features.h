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
#ifndef __KOS_KERNEL_FEATURES_H__
#define __KOS_KERNEL_FEATURES_H__ 1

#include <kos/config.h>

#ifdef __KERNEL__
#define KCONFIG_HAVE_TASKNAMES 1
#define KCONFIG_HAVE_INTERRUPTS 1
//#define KCONFIG_HAVE_SINGLECORE
#if KCONFIG_HAVE_TASKNAMES
#define KCONFIG_TASKZERO_NAME   "KERNEL"
#endif /* KCONFIG_HAVE_TASKNAMES */

//////////////////////////////////////////////////////////////////////////
// Secure user (ring-3) task bootstrapping:
//  - When defined, all registers that may still contain
//    kernel (physical) pointers are set to NULL before
//    execution is switching to a ring-3 (user) program.
// NOTE: Even when disabled, this does not pose a security
//       risk, as paging prevents ring-3 applications from
//       accessing kernel-space memory.
//    >> Though some hacky applications might (ab-)use those
//       physical pointers (from EAX,EBX,ECX and EDX), not
//       necessarily to do bad things, but good (like randomization
//       seeds), keeping this hidden from user-apps improves
//       sandboxing as a task is not able to get this information.
#define KCONFIG_SECUREUSERBOOTSTRAP 1


// Save all segment registers individually during a task switch
// When not defined, only '%es' is saved.
// Otherwise '%es', '%ds', '%fs' and '%gs' are saved.
#define KTASK_I386_SAVE_SEGMENT_REGISTERS 1

#define KTASK_HAVE_CRITICAL_TASK           1
#define KTASK_HAVE_CRITICAL_TASK_INTERRUPT 1
//#define KCONFIG_HAVE_SINGLECORE 1

#define KTASK_HAVE_STATS_START     0x01
#define KTASK_HAVE_STATS_QUANTUM   0x02
#define KTASK_HAVE_STATS_NOSCHED   0x04
#define KTASK_HAVE_STATS_SLEEP     0x08
#define KTASK_HAVE_STATS_SWITCHOUT 0x10
#define KTASK_HAVE_STATS_SWITCHIN  0x20

// Task stat features supported by the kernel
#define KTASK_HAVE_STATS \
 (KTASK_HAVE_STATS_START|KTASK_HAVE_STATS_QUANTUM|\
  KTASK_HAVE_STATS_NOSCHED|KTASK_HAVE_STATS_SLEEP|\
  KTASK_HAVE_STATS_SWITCHOUT|KTASK_HAVE_STATS_SWITCHIN)
#define KTASK_HAVE_STATS_FEATURE(f) ((KTASK_HAVE_STATS&(f))==(f))


// NOTE: Special size optimizations may be performed for bit sizes fitting
//       inside of 'KSIGNAL_USERBITS' (16) (s.a.: <kos/kernel/signal.h>)
#define KCLOSELOCK_OPCOUNTBITS  32 /*< Amount of bits used for recursion in close locks. */
#define KSEMAPHORE_TICKETBITS   32 /*< Amount of bits used to represent semaphore tickets. */
#define KRWLOCK_READCBITS       32 /*< Amount of bits used for recursive reading in R/W locks. */
#define KDDIST_USERBITS         32 /*< Amount of bits available tracking the amount of registered users. */

#ifdef __DEBUG__
// Track the holder of non-recursive mutex locks and assert that any
// given task doesn't attempt to acquire a single mutex more than once.
// NOTE: Due to the fact that the traceback of acquired locks is stored
//       as well, non-released locks will be dumped as memory leaks.
#define KDEBUG_HAVE_TRACKEDMUTEX 1
#define KDEBUG_HAVE_TRACKEDMMUTEX 1
#define KDEBUG_HAVE_TRACKEDDDIST 1
#endif

#ifndef KSECURITY_MAX_LOGIN_ATTEMPTS_PER_SESSION
// Maximum amount of loging attempts per session,
// before forkas will no longer allow a task to
// escape its sandbox.
#define KSECURITY_MAX_LOGIN_ATTEMPTS_PER_SESSION 32
#endif

// Not fully implemented yet: copy-on-write fork() paging
//#define KCONFIG_HAVE_SHM_COPY_ON_WRITE

#ifdef __INTELLISENSE__
#undef KDEBUG_HAVE_TRACKEDMUTEX
#undef KDEBUG_HAVE_TRACKEDMMUTEX
//#undef KDEBUG_HAVE_TRACKEDDDIST
#endif


#ifndef KCONFIG_HAVE_TASKNAMES
#define KCONFIG_HAVE_TASKNAMES 0
#endif
#ifndef KCONFIG_HAVE_INTERRUPTS
#define KCONFIG_HAVE_INTERRUPTS 1
#endif
#ifndef KCONFIG_SECUREUSERBOOTSTRAP
#define KCONFIG_SECUREUSERBOOTSTRAP 0
#endif
#ifndef KTASK_I386_SAVE_SEGMENT_REGISTERS
#define KTASK_I386_SAVE_SEGMENT_REGISTERS 0
#endif
#ifndef KDEBUG_HAVE_TRACKEDMUTEX
#define KDEBUG_HAVE_TRACKEDMUTEX 0
#endif
#ifndef KDEBUG_HAVE_TRACKEDMMUTEX
#define KDEBUG_HAVE_TRACKEDMMUTEX 0
#endif
#ifndef KDEBUG_HAVE_TRACKEDDDIST
#define KDEBUG_HAVE_TRACKEDDDIST 0
#endif
#ifndef KTASK_HAVE_CRITICAL_TASK
#define KTASK_HAVE_CRITICAL_TASK 1
#endif
#ifndef KTASK_HAVE_CRITICAL_TASK_INTERRUPT
#define KTASK_HAVE_CRITICAL_TASK_INTERRUPT 1
#endif
#ifndef KTASK_HAVE_STATS
#define KTASK_HAVE_STATS 0
#endif
#if !KTASK_HAVE_CRITICAL_TASK
#undef KTASK_HAVE_CRITICAL_TASK_INTERRUPT
#define KTASK_HAVE_CRITICAL_TASK_INTERRUPT 0
#endif



#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_FEATURES_H__ */

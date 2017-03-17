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
#define KCONFIG_HAVE_IRQ 1

/* Count allocated pages within the pageframe allocator.
 * >> Required for x/y-style memory usage statistics. */
#define KCONFIG_HAVE_PAGEFRAME_COUNT_ALLOCATED 1

/* Save all segment registers individually during a task switch
 * When not defined, only '%es' is saved.
 * Otherwise '%es', '%ds', '%fs' and '%gs' are saved. */
#define KCONFIG_HAVE_I386_SAVE_SEGMENT_REGISTERS 1

//#define KCONFIG_HAVE_SINGLECORE 1
#define KCONFIG_HAVE_TASK_NAMES   1
#define KCONFIG_TASKNAME_KERNEL   "KERNEL"

/* Task stat features supported by the kernel */
#define KCONFIG_HAVE_TASK_STATS_START     1
#define KCONFIG_HAVE_TASK_STATS_QUANTUM   1
#define KCONFIG_HAVE_TASK_STATS_NOSCHED   1
#define KCONFIG_HAVE_TASK_STATS_SLEEP     1
#define KCONFIG_HAVE_TASK_STATS_SWITCHOUT 1
#define KCONFIG_HAVE_TASK_STATS_SWITCHIN  1

/* Include functionality to trace system calls,
 * as well information on their arguments and names. */
#define KCONFIG_HAVE_SYSCALL_TRACE  defined(__DEBUG__)

/* NOTE: Special size optimizations may be performed for bit sizes fitting
 *       inside of 'KSIGNAL_USERBITS' (16) (s.a.: <kos/kernel/signal.h>) */
#define KCONFIG_CLOSELOCK_OPCOUNTBITS  32 /*< Amount of bits used for recursion in close locks. */
#define KCONFIG_SEMAPHORE_TICKETBITS   32 /*< Amount of bits used to represent semaphore tickets. */
#define KCONFIG_RWLOCK_READERBITS      32 /*< Amount of bits used for recursive reading in R/W locks. */
#define KCONFIG_DDIST_USERBITS         32 /*< Amount of bits available tracking the amount of registered users. */

#ifdef __DEBUG__
/* Track the holder of non-recursive mutex locks and assert that any
 * given task doesn't attempt to acquire a single mutex more than once.
 * NOTE: Due to the fact that the traceback of acquired locks is stored
 *       as well, non-released locks will be dumped as memory leaks. */
#define KCONFIG_HAVE_DEBUG_TRACKEDMUTEX     1
#define KCONFIG_HAVE_DEBUG_TRACKEDMMUTEX    1
#define KCONFIG_HAVE_DEBUG_TRACKEDDDIST     1
#define KCONFIG_HAVE_DEBUG_TRACKEDRWLOCK    0 /*< Too expensive, considering its use by copy_*_user. */
#define KCONFIG_HAVE_DEBUG_MEMCHECKS        1
#define KCONFIG_HAVE_TASK_DEADLOCK_CHECK    1
#define KCONFIG_HAVE_DEBUG_PAGEFRAME_MEMSET 1
#endif


/* Size for the global cache of recently loaded shared libraries
 * & executables. Define as ZERO(0) to disable this cache. */
#define KCONFIG_SHLIB_RECENT_CACHE_SIZE 32



#ifdef __INTELLISENSE__
#undef KCONFIG_HAVE_DEBUG_TRACKEDMUTEX
#undef KCONFIG_HAVE_DEBUG_TRACKEDMMUTEX
#undef KCONFIG_HAVE_DEBUG_TRACKEDDDIST
#endif


//
// ---== End of configurable section ==---
//


#ifndef KCONFIG_HAVE_TASK_STATS_START
#define KCONFIG_HAVE_TASK_STATS_START     0
#endif
#ifndef KCONFIG_HAVE_TASK_STATS_QUANTUM
#define KCONFIG_HAVE_TASK_STATS_QUANTUM   0
#endif
#ifndef KCONFIG_HAVE_TASK_STATS_NOSCHED
#define KCONFIG_HAVE_TASK_STATS_NOSCHED 0
#endif
#ifndef KCONFIG_HAVE_TASK_STATS_SLEEP
#define KCONFIG_HAVE_TASK_STATS_SLEEP 0
#endif
#ifndef KCONFIG_HAVE_TASK_STATS_SWITCHOUT
#define KCONFIG_HAVE_TASK_STATS_SWITCHOUT 0
#endif
#ifndef KCONFIG_HAVE_TASK_STATS_SWITCHIN
#define KCONFIG_HAVE_TASK_STATS_SWITCHIN 0
#endif
#ifndef KCONFIG_HAVE_TASK_NAMES
#define KCONFIG_HAVE_TASK_NAMES 0
#endif
#ifndef KCONFIG_HAVE_IRQ
#define KCONFIG_HAVE_IRQ 1
#endif
#ifndef KCONFIG_HAVE_I386_SAVE_SEGMENT_REGISTERS
#define KCONFIG_HAVE_I386_SAVE_SEGMENT_REGISTERS 0
#endif
#ifndef KCONFIG_HAVE_DEBUG_TRACKEDMUTEX
#define KCONFIG_HAVE_DEBUG_TRACKEDMUTEX 0
#endif
#ifndef KCONFIG_HAVE_DEBUG_TRACKEDMMUTEX
#define KCONFIG_HAVE_DEBUG_TRACKEDMMUTEX 0
#endif
#ifndef KCONFIG_HAVE_DEBUG_TRACKEDDDIST
#define KCONFIG_HAVE_DEBUG_TRACKEDDDIST 0
#endif
#ifndef KCONFIG_HAVE_DEBUG_TRACKEDRWLOCK
#define KCONFIG_HAVE_DEBUG_TRACKEDRWLOCK 0
#endif
#ifndef KCONFIG_SHLIB_RECENT_CACHE_SIZE
#define KCONFIG_SHLIB_RECENT_CACHE_SIZE 0
#endif
#ifndef KCONFIG_HAVE_DEBUG_MEMCHECKS
#define KCONFIG_HAVE_DEBUG_MEMCHECKS 0
#endif
#ifndef KCONFIG_HAVE_DEBUG_PAGEFRAME_MEMSET
#define KCONFIG_HAVE_DEBUG_PAGEFRAME_MEMSET 0
#endif
#ifndef KCONFIG_HAVE_TASK_STATS_START
#define KCONFIG_HAVE_TASK_STATS_START 0
#endif
#ifndef KCONFIG_HAVE_TASK_STATS_QUANTUM
#define KCONFIG_HAVE_TASK_STATS_QUANTUM 0
#endif
#ifndef KCONFIG_HAVE_TASK_STATS_NOSCHED
#define KCONFIG_HAVE_TASK_STATS_NOSCHED 0
#endif
#ifndef KCONFIG_HAVE_TASK_STATS_SLEEP
#define KCONFIG_HAVE_TASK_STATS_SLEEP 0
#endif
#ifndef KCONFIG_HAVE_TASK_STATS_SWITCHOUT
#define KCONFIG_HAVE_TASK_STATS_SWITCHOUT 0
#endif
#ifndef KCONFIG_HAVE_TASK_STATS_SWITCHIN
#define KCONFIG_HAVE_TASK_STATS_SWITCHIN 0
#endif
#ifndef KCONFIG_HAVE_TASK_DEADLOCK_CHECK
#define KCONFIG_HAVE_TASK_DEADLOCK_CHECK 0
#endif

#undef KCONFIG_HAVE_TASK_STATS
#if KCONFIG_HAVE_TASK_STATS_START \
 || KCONFIG_HAVE_TASK_STATS_QUANTUM \
 || KCONFIG_HAVE_TASK_STATS_NOSCHED \
 || KCONFIG_HAVE_TASK_STATS_SLEEP \
 || KCONFIG_HAVE_TASK_STATS_SWITCHOUT \
 || KCONFIG_HAVE_TASK_STATS_SWITCHIN
#define KCONFIG_HAVE_TASK_STATS  1
#else
#define KCONFIG_HAVE_TASK_STATS  0
#endif



#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_FEATURES_H__ */

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
#ifndef __KOS_KERNEL_SCHED_SYNC_MMUTEX_C_INL__
#define __KOS_KERNEL_SCHED_SYNC_MMUTEX_C_INL__ 1

#include <kos/compiler.h>
#include <kos/atomic.h>
#include <kos/kernel/debug.h>
#include <kos/kernel/mmutex.h>
#include <kos/kernel/signal.h>
#include <kos/kernel/time.h>
#include <kos/timespec.h>
#if KCONFIG_HAVE_DEBUG_TRACKEDMMUTEX
#include <kos/kernel/proc.h>
#include <kos/kernel/task.h>
#include <stdio.h>
#include <stdlib.h>
#include <traceback.h>
#endif

__DECL_BEGIN


#if KCONFIG_HAVE_DEBUG_TRACKEDMUTEX
#define DBGSLOT(self,id) self->mmx_debug[id]
#define KMMUTEX_ONACQUIRE(self,id) \
do{\
 int lid = (id); assert(lid < KMMUTEX_LOCKC);\
 /* Capture a traceback and save the current task as holder. */\
 assert(!DBGSLOT(self,lid).md_locktb);\
 DBGSLOT(self,lid).md_holder = ktask_self();\
 DBGSLOT(self,lid).md_locktb = tbtrace_captureex(1);\
}while(0)

#define KMMUTEX_ONRELEASE(self,id) \
do{\
 int lid = (id); assert(lid < KMMUTEX_LOCKC);\
 if (DBGSLOT(self,lid).md_holder != ktask_self()) {\
  /* Non-recursive lock held twice. */\
  printf("ERROR: Non-recursive multi-lock %d in %p not held by %I32d:%Iu:%s.\n"\
         "See reference to acquisition by %p:\n"\
        ,lid,self\
        ,ktask_self()->t_proc->p_pid\
        ,ktask_self()->t_tid\
        ,ktask_getname(ktask_self())\
        ,DBGSLOT(self,lid).md_holder);\
  tbtrace_print(DBGSLOT(self,lid).md_locktb);\
  printf("See reference to attempted release:\n");\
  tb_printex(1);\
  abort();\
 }\
 free(DBGSLOT(self,lid).md_locktb);\
 DBGSLOT(self,lid).md_locktb = NULL;\
 DBGSLOT(self,lid).md_holder = NULL;\
}while(0)

#define KMMUTEX_ONLOCKINUSE(self,id) \
do{\
 int lid = (id); assert(lid < KMMUTEX_LOCKC);\
 if (DBGSLOT(self,lid).md_holder == ktask_self()) {\
  /* Non-recursive lock held twice. */\
  printf("ERROR: Non-recursive multi-lock %d %p held twice by %I32d:%Iu:%s.\n"\
         "See reference to previous acquisition:\n"\
        ,lid,self\
        ,ktask_self()->t_proc->p_pid\
        ,ktask_self()->t_tid\
        ,ktask_getname(ktask_self()));\
  tbtrace_print(DBGSLOT(self,lid).md_locktb);\
  printf("See reference to new acquisition:\n");\
  tb_printex(1);\
  abort();\
 }\
}while(0)

#define KMMUTEX_ONACQUIRES(self,locks) \
 for (int i = 0; i < KMMUTEX_LOCKC; ++i) {\
  if (KMMUTEX_LOCK(i)&locks) {\
   KMMUTEX_ONACQUIRE(self,i);\
  }\
 }
#define KMMUTEX_ONRELEASES(self,locks) \
 for (int i = 0; i < KMMUTEX_LOCKC; ++i) {\
  if (KMMUTEX_LOCK(i)&locks) {\
   KMMUTEX_ONRELEASE(self,i);\
  }\
 }
#define KMMUTEX_ONLOCKINUSES(self,locks) \
 for (int i = 0; i < KMMUTEX_LOCKC; ++i) {\
  if (KMMUTEX_LOCK(i)&locks) {\
   KMMUTEX_ONLOCKINUSE(self,i);\
  }\
 }

#else
#define KMMUTEX_ONACQUIRE(self,id)        (void)0
#define KMMUTEX_ONRELEASE(self,id)        (void)0
#define KMMUTEX_ONLOCKINUSE(self,id)      (void)0
#define KMMUTEX_ONACQUIRES(self,locks)   (void)0
#define KMMUTEX_ONRELEASES(self,locks)   (void)0
#define KMMUTEX_ONLOCKINUSES(self,locks) (void)0
#endif

#ifndef __INTELLISENSE__
#define LOCKALL
#include "mmutex-impl.c.inl"
#include "mmutex-impl.c.inl"
#endif

#ifndef __INTELLISENSE__
#undef KMMUTEX_ONACQUIRE
#undef KMMUTEX_ONRELEASE
#undef KMMUTEX_ONLOCKINUSE
#undef KMMUTEX_ONACQUIRES
#undef KMMUTEX_ONRELEASES
#undef KMMUTEX_ONLOCKINUSES
#endif

__DECL_END

#endif /* !__KOS_KERNEL_SCHED_SYNC_MMUTEX_C_INL__ */

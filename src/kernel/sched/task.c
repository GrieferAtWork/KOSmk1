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
#ifndef __KOS_KERNEL_SCHED_TASK_C__
#define __KOS_KERNEL_SCHED_TASK_C__ 1

#include <kos/compiler.h>
#include <kos/kernel/debug.h>
#include <kos/kernel/features.h>
#include <kos/kernel/interrupts.h>
#include <kos/atomic.h>
#include <kos/kernel/task.h>
#include <kos/kernel/proc.h>
#include <kos/kernel/time.h>
#include <kos/kernel/sigset.h>
#include <kos/syslog.h>
#include <kos/kernel/paging.h>
#include <kos/timespec.h>
#include <math.h>
#include <stdio.h>
#include <sys/types.h>
#include <kos/kernel/pageframe.h>
#include <kos/kernel/serial.h>
#include <stddef.h>

__DECL_BEGIN

#if defined(__DEBUG__) && 1
#define KTASK_ONSWITCH(reason,oldtask,newtask) \
 k_syslogf(KLOG_TRACE,"SCHED-SWITCH [%s]: %p:%I32d:%Iu:%s(%p|%p) --> %p:%I32d:%Iu:%s(%p|%p)\n",reason,\
      oldtask,(oldtask) ? ((struct ktask *)(oldtask))->t_proc->p_pid : -1,(oldtask) ? ((struct ktask *)(oldtask))->t_tid : 0,(oldtask) ? ktask_getname((struct ktask *)(oldtask)) : NULL,(oldtask) ? ((struct ktask *)(oldtask))->t_esp : NULL,(oldtask) ? ((struct ktask *)(oldtask))->t_userpd : NULL,\
      newtask,(newtask) ? ((struct ktask *)(newtask))->t_proc->p_pid : -1,(newtask) ? ((struct ktask *)(newtask))->t_tid : 0,(newtask) ? ktask_getname((struct ktask *)(newtask)) : NULL,(newtask) ? ((struct ktask *)(newtask))->t_esp : NULL,(newtask) ? ((struct ktask *)(newtask))->t_userpd : NULL)
#define KTASK_ONTERMINATE(task) \
 k_syslogf(KLOG_TRACE,"ONTERMINATE(%p): %I32d:%Iu:%s (exitcode: %p)\n",\
       task,(task)->t_proc->p_pid,(task)->t_tid,ktask_getname(task),task->t_exitcode)
#else
#define KTASK_ONSWITCH(reason,oldtask,newtask) (void)0
#define KTASK_ONTERMINATE(task)                (void)0
#endif

/* Pick some random members to assert our contant offsets of. */
__STATIC_ASSERT(offsetof(struct ktask,t_cpu)       == KTASK_OFFSETOF_CPU);
__STATIC_ASSERT(offsetof(struct ktask,t_abstime)   == KTASK_OFFSETOF_ABSTIME);
__STATIC_ASSERT(offsetof(struct ktask,t_suspended) == KTASK_OFFSETOF_SUSPENDED);
__STATIC_ASSERT(offsetof(struct ktask,t_sigval)    == KTASK_OFFSETOF_SIGVAL);
__STATIC_ASSERT(offsetof(struct ktask,t_joinsig)   == KTASK_OFFSETOF_JOINSIG);
__STATIC_ASSERT(offsetof(struct ktask,t_proc)      == KTASK_OFFSETOF_PROC);
__STATIC_ASSERT(offsetof(struct ktask,t_children)  == KTASK_OFFSETOF_CHILDREN);
__STATIC_ASSERT(offsetof(struct ktask,t_ustackvp)  == KTASK_OFFSETOF_USTACKVP);
__STATIC_ASSERT(offsetof(struct ktask,t_ustacksz)  == KTASK_OFFSETOF_USTACKSZ);
__STATIC_ASSERT(offsetof(struct ktask,t_kstackvp)  == KTASK_OFFSETOF_KSTACKVP);
__STATIC_ASSERT(offsetof(struct ktask,t_kstackend) == KTASK_OFFSETOF_KSTACKEND);
__STATIC_ASSERT(offsetof(struct ktask,t_tls)       == KTASK_OFFSETOF_TLS);
#if KCONFIG_HAVE_TASK_STATS
__STATIC_ASSERT(offsetof(struct ktask,t_stats)     == KTASK_OFFSETOF_STAT);
__STATIC_ASSERT(sizeof(struct ktaskstat)           == KTASKSTAT_SIZEOF);
#endif /* KCONFIG_HAVE_TASK_STATS */
__STATIC_ASSERT(sizeof(struct ktask)               == KTASK_SIZEOF);


extern struct kcpu  __kcpu_zero;
extern struct ktask __ktask_zero;
extern byte_t stack_top[];
extern byte_t stack_bottom[];

struct kcpu __kcpu_zero = {
 KOBJECT_INIT(KOBJECT_MAGIC_CPU)
 /* c_locks              */0,
 /* c_flags              */KCPU_FLAG_NONE,
 /* c_padding            */{0,0},
 /* c_current            */&__ktask_zero,
 /* c_sleeping           */NULL,
 /* c_tss                */{0,},
};

struct ktask __ktask_zero = {
 KOBJECT_INIT(KOBJECT_MAGIC_TASK)
 /* t_esp         */stack_top,
 /* t_userpd      */kpagedir_kernel(),
 /* t_esp0        */NULL,
 /* t_epd         */kpagedir_kernel(),
 /* t_refcnt      */0xffff,
 /* t_locks       */0,
 /* t_state       */KTASK_STATE_RUNNING,
 /* t_newstate    */KTASK_STATE_RUNNING,
 /* t_flags       */KTASK_FLAG_CRITICAL,
 /* t_exitcode    */NULL,
 /* t_cpu         */kcpu_zero(),
 /* t_prev        */ktask_zero(),
 /* t_next        */ktask_zero(),
 /* t_abstime     */{0,0},
 /* t_suspended   */0,
 /* t_setpriority */KTASK_PRIORITY_DEF,
 /* t_curpriority */KTASK_PRIORITY_DEF,
 /* t_sigval      */NULL,
 /* t_sigchain    */KTASKSIG_INIT(ktask_zero()),
 /* t_joinsig     */KSIGNAL_INIT,
 /* t_proc        */kproc_kernel(),
 /* t_parent      */ktask_zero(),
 /* t_parid       */0,
 /* t_tid         */0,
 /* t_children    */KTASKLIST_INIT,
 /* t_ustackvp    */NULL,
 /* t_ustacksz    */0,
 /* t_kstackvp    */stack_bottom,
 /* t_kstack      */stack_bottom,
 /* t_kstackend   */stack_top,
#if KCONFIG_HAVE_TASK_NAMES
 /* t_name        */(char *)KCONFIG_TASKNAME_KERNEL,
#endif /* KCONFIG_HAVE_TASK_NAMES */
 /* t_tls         */KTLSPT_INIT,
#if KCONFIG_HAVE_TASK_STATS
 /* t_stats       */KTASKSTAT_INIT,
#endif /* KCONFIG_HAVE_TASK_STATS */
#if !KCONFIG_HAVE_IRQ
 /* t_preempt     */KTASK_PREEMT_COUNTDOWN,
#endif /* !KCONFIG_HAVE_IRQ */
#if KCONFIG_HAVE_TASK_DEADLOCK_CHECK
 /* t_deadlock_help     */NULL,
 /* t_deadlock_closure  */NULL,
#endif
};


__COMPILER_PACK_PUSH(1)
struct __packed scheddata {
 __user   void            *esp;
 __kernel struct kpagedir *pd;
};
__COMPILER_PACK_POP


size_t kcpu_taskcount(struct kcpu const *__restrict self) {
 size_t result = 0;
 struct ktask *iter,*start;
 kassert_kcpu(self);
 NOIRQ_BEGINLOCK(kcpu_trylock((struct kcpu *)self,KCPU_LOCK_TASKS));
 iter = start = self->c_current;
 if (iter) do ++result; while ((iter = iter->t_next) != start);
 NOIRQ_ENDUNLOCK(kcpu_unlock((struct kcpu *)self,KCPU_LOCK_TASKS));
 return result;
}
size_t kcpu_sleepcount(struct kcpu const *__restrict self) {
 size_t result = 0;
 struct ktask *iter,*start;
 kassert_kcpu(self);
 NOIRQ_BEGINLOCK(kcpu_trylock((struct kcpu *)self,KCPU_LOCK_SLEEP));
 iter = start = self->c_sleeping;
 if (iter) do ++result; while ((iter = iter->t_next) != start);
 NOIRQ_ENDUNLOCK(kcpu_unlock((struct kcpu *)self,KCPU_LOCK_SLEEP));
 return result;
}

int kcpu_global_onetask(void) {
 struct kcpu *ccheck; int result;
 /* TODO: foreach-cpu */
 ccheck = kcpu_zero();
 NOIRQ_BEGIN;
 for (;;) {
  if (kcpu_trylock(ccheck,KCPU_LOCK_TASKS)) {
   if (kcpu_trylock(ccheck,KCPU_LOCK_SLEEP)) break;
   kcpu_unlock(ccheck,KCPU_LOCK_TASKS);
  } else if (kcpu_trylock(ccheck,KCPU_LOCK_SLEEP)) {
   if (kcpu_trylock(ccheck,KCPU_LOCK_TASKS)) break;
   kcpu_unlock(ccheck,KCPU_LOCK_SLEEP);
  }
 }
 result = !ccheck->c_sleeping && (!ccheck->c_current ||
           ccheck->c_current->t_next == ccheck->c_current);
 kcpu_unlock(ccheck,KCPU_LOCK_TASKS);
 kcpu_unlock(ccheck,KCPU_LOCK_SLEEP);
 NOIRQ_END
 return result;
}

#if KCONFIG_HAVE_TASK_STATS_QUANTUM
#define KTASK_ONENTERQUANTUM_NEEDTM 1
#define KTASK_ONLEAVEQUANTUM_NEEDTM 1
#if KCONFIG_HAVE_TASK_STATS_SWITCHIN
#define KTASK_ONENTERQUANTUM_TM(task,cpu,tmnow) (++(task)->t_stats.ts_irq_switchin,ktaskstat_on_enterquantum_tm(&(task)->t_stats,tmnow))
#define KTASK_ONENTERQUANTUM(task,cpu)          (++(task)->t_stats.ts_irq_switchin,ktaskstat_on_enterquantum(&(task)->t_stats))
#else /* KCONFIG_HAVE_TASK_STATS_SWITCHIN */
#define KTASK_ONENTERQUANTUM_TM(task,cpu,tmnow) ktaskstat_on_enterquantum_tm(&(task)->t_stats,tmnow)
#define KTASK_ONENTERQUANTUM(task,cpu)          ktaskstat_on_enterquantum(&(task)->t_stats)
#endif /* !KCONFIG_HAVE_TASK_STATS_SWITCHIN */
#if KCONFIG_HAVE_TASK_STATS_SWITCHOUT
#define KTASK_ONLEAVEQUANTUM_TM(task,cpu,tmnow) (++(task)->t_stats.ts_irq_switchout,ktaskstat_on_leavequantum_tm(&(task)->t_stats,tmnow))
#define KTASK_ONLEAVEQUANTUM(task,cpu)          (++(task)->t_stats.ts_irq_switchout,ktaskstat_on_leavequantum(&(task)->t_stats))
#else /* KCONFIG_HAVE_TASK_STATS_SWITCHOUT */
#define KTASK_ONLEAVEQUANTUM_TM(task,cpu,tmnow) ktaskstat_on_leavequantum_tm(&(task)->t_stats,tmnow)
#define KTASK_ONLEAVEQUANTUM(task,cpu)          ktaskstat_on_leavequantum(&(task)->t_stats)
#endif /* !KCONFIG_HAVE_TASK_STATS_SWITCHOUT */
#else /* KCONFIG_HAVE_TASK_STATS_QUANTUM */
#if KCONFIG_HAVE_TASK_STATS_SWITCHIN
#define KTASK_ONENTERQUANTUM_TM(task,cpu,tmnow) (++(task)->t_stats.ts_irq_switchin,++(task)->t_stats.ts_version)
#define KTASK_ONENTERQUANTUM(task,cpu)          (++(task)->t_stats.ts_irq_switchin,++(task)->t_stats.ts_version)
#endif /* KCONFIG_HAVE_TASK_STATS_SWITCHIN */
#if KCONFIG_HAVE_TASK_STATS_SWITCHOUT
#define KTASK_ONLEAVEQUANTUM_TM(task,cpu,tmnow) (++(task)->t_stats.ts_irq_switchout,++(task)->t_stats.ts_version)
#define KTASK_ONLEAVEQUANTUM(task,cpu)          (++(task)->t_stats.ts_irq_switchout,++(task)->t_stats.ts_version)
#endif /* KCONFIG_HAVE_TASK_STATS_SWITCHOUT */
#endif /* !KCONFIG_HAVE_TASK_STATS_QUANTUM */

#if KCONFIG_HAVE_TASK_STATS_NOSCHED
#define KTASK_ONRESCHEDULE_NEEDTM 1
#define KTASK_ONUNSCHEDULE_NEEDTM 1
#define KTASK_ONRESCHEDULE_TM(task,cpu,tmnow)   ktaskstat_on_reschedule_tm(&(task)->t_stats,tmnow)
#define KTASK_ONUNSCHEDULE_TM(task,cpu,tmnow)   ktaskstat_on_unschedule_tm(&(task)->t_stats,tmnow)
#define KTASK_ONRESCHEDULE(task,cpu)            ktaskstat_on_reschedule(&(task)->t_stats)
#define KTASK_ONUNSCHEDULE(task,cpu)            ktaskstat_on_unschedule(&(task)->t_stats)
#endif /* KCONFIG_HAVE_TASK_STATS_NOSCHED */

#if KCONFIG_HAVE_TASK_STATS_SLEEP
#define KTASK_ONENTERSLEEP_NEEDTM 1
#define KTASK_ONLEAVESLEEP_NEEDTM 1
#define KTASK_ONENTERSLEEP_TM(task,cpu,tmnow)   ktaskstat_on_entersleep_tm(&(task)->t_stats,tmnow)
#define KTASK_ONLEAVESLEEP_TM(task,cpu,tmnow)   ktaskstat_on_leavesleep_tm(&(task)->t_stats,tmnow)
#define KTASK_ONENTERSLEEP(task,cpu)            ktaskstat_on_entersleep(&(task)->t_stats)
#define KTASK_ONLEAVESLEEP(task,cpu)            ktaskstat_on_leavesleep(&(task)->t_stats)
#endif

#ifndef KTASK_ONENTERQUANTUM
#define KTASK_ONENTERQUANTUM_TM(task,cpu,tmnow) (void)0
#define KTASK_ONLEAVEQUANTUM_TM(task,cpu,tmnow) (void)0
#define KTASK_ONENTERQUANTUM(task,cpu)          (void)0
#define KTASK_ONLEAVEQUANTUM(task,cpu)          (void)0
#endif

#ifndef KTASK_ONRESCHEDULE
#define KTASK_ONRESCHEDULE_TM(task,cpu,tmnow)   (void)0
#define KTASK_ONUNSCHEDULE_TM(task,cpu,tmnow)   (void)0
#define KTASK_ONRESCHEDULE(task,cpu)            (void)0
#define KTASK_ONUNSCHEDULE(task,cpu)            (void)0
#endif

#ifndef KTASK_ONENTERSLEEP
#define KTASK_ONENTERSLEEP_TM(task,cpu,tmnow)   (void)0
#define KTASK_ONLEAVESLEEP_TM(task,cpu,tmnow)   (void)0
#define KTASK_ONENTERSLEEP(task,cpu)            (void)0
#define KTASK_ONLEAVESLEEP(task,cpu)            (void)0
#endif


extern void kcpu_setesp0(void *esp0);
void kcpu_setesp0(void *esp0) {
 kcpu_self()->c_tss.esp0 = (uintptr_t)esp0;
}





__crit __ref struct ktask *
ktask_newkernel(struct ktask *__restrict parent,
                struct kproc *__restrict proc,
                __size_t kstacksize, __u16 flags) {
 struct ktask *result;
 KTASK_CRIT_MARK
 kassert_ktask(parent); kassert_kproc(proc);
 assertf(krwlock_iswritelocked(&proc->p_shm.s_lock),
         "The caller must lock the pagedir of the associated task context!");
 assertf(!(flags&KTASK_FLAG_USERTASK),"Kernel tasks can't be user-level tasks. (DUH!)");
 assertf(!(flags&KTASK_FLAG_OWNSUSTACK),"Kernel tasks can't own user-level stacks");
 assertf(kproc_getpagedir(proc) == kpagedir_kernel(),"Kernel tasks require a context using the kernel page directory");
 if __unlikely(KE_ISERR(ktask_incref(parent))) return NULL;
 if __unlikely(KE_ISERR(kproc_incref(proc))) goto err_par;
 if __unlikely((result = omalloc(struct ktask)) == NULL) goto err_ctx;
 kobject_init(result,KOBJECT_MAGIC_TASK);
 /* Allocate a stack for the tasks. */
 result->t_kstack = (flags&KTASK_FLAG_MALLKSTACK)
  ? malloc(kstacksize)
  : kpageframe_alloc(ceildiv(kstacksize,PAGESIZE));
 if __unlikely(!result->t_kstack) goto err_r;
 result->t_kstackvp    = result->t_kstack;
 result->t_flags       = flags;
 result->t_esp0        = NULL; /*< The TSS isn't used by kernel tasks. */
 result->t_ustackvp    = NULL;
 result->t_ustacksz    = 0;
 result->t_epd         =
 result->t_userpd      = kpagedir_kernel();
 result->t_kstackend   = (void *)((uintptr_t)result->t_kstack+kstacksize);
 result->t_esp         = result->t_kstackend;
 result->t_proc        = proc; /* Inherit reference. */
 result->t_refcnt      = 1;
 result->t_locks       = 0;
 result->t_state       = KTASK_STATE_SUSPENDED;
 result->t_newstate    = KTASK_STATE_RUNNING;
 result->t_suspended   = 1;
 result->t_cpu         = NULL;
 result->t_setpriority = KTASK_PRIORITY_DEF;
 result->t_curpriority = KTASK_PRIORITY_DEF;
#ifdef __DEBUG__
 /* These are undefined for suspended tasks. */
 result->t_prev        = NULL;
 result->t_next        = NULL;
#endif
 result->t_parent      = parent; /* Inherit reference. */
 result->t_sigval      = NULL;
 ktasklist_init(&result->t_children);
 ktlspt_init(&result->t_tls);
#if KCONFIG_HAVE_TASK_STATS
 ktaskstat_init(&result->t_stats);
#endif /* KCONFIG_HAVE_TASK_STATS */
#if !KCONFIG_HAVE_IRQ
 result->t_preempt = KTASK_PREEMT_COUNTDOWN;
#endif /* !KCONFIG_HAVE_IRQ */
#if KCONFIG_HAVE_TASK_DEADLOCK_CHECK
 result->t_deadlock_help     = NULL;
 result->t_deadlock_closure  = NULL;
#endif
 ksignal_init(&result->t_joinsig);
 ktasksig_init(&result->t_sigchain,result);
#if KCONFIG_HAVE_TASK_NAMES
 result->t_name = NULL;
#endif /* KCONFIG_HAVE_TASK_NAMES */
 /* Initialize the task's TID and setup its parent hooks. */
 if __unlikely(KE_ISERR(kproc_addtask(proc,result))) goto err_map;
 ktask_lock(parent,KTASK_LOCK_CHILDREN);
 if __unlikely(ktask_isterminated(parent)) {
  /* TODO: Handle terminate parent task (Fail with
   *       something that doesn't indicate no memory...). */
  ktask_unlock(parent,KTASK_LOCK_CHILDREN);
  goto err_ctx2;
 }
 result->t_parid = ktasklist_insert(&parent->t_children,result);
 ktask_unlock(parent,KTASK_LOCK_CHILDREN);
 if __unlikely(result->t_parid == (size_t)-1) goto err_ctx2;
 return result;
err_ctx2: kproc_deltask(proc,result);
err_map:  if (flags&KTASK_FLAG_MALLKSTACK) free(result->t_kstack);
          else kpageframe_free((struct kpageframe *)result->t_kstack,
                               ceildiv(kstacksize,PAGESIZE));
err_r:    free(result);
err_ctx:  kproc_decref(proc);
err_par:  ktask_decref(parent);
 return NULL;
}

__crit __ref struct ktask *
ktask_newuser(struct ktask *__restrict parent, struct kproc *__restrict proc,
              __user void **__restrict useresp, __size_t ustacksize, __size_t kstacksize) {
 void *userstack; struct ktask *result;
 KTASK_CRIT_MARK
 kassertobj(useresp);
 assertf(krwlock_iswritelocked(&proc->p_shm.s_lock),
         "The caller must lock the pagedir of the associated task context!");
 if __unlikely(KE_ISERR(kshm_mapram_unlocked(kproc_getshm(proc),&userstack,
                                             ceildiv(ustacksize,PAGESIZE),
                                             KPAGEDIR_MAPANY_HINT_USTACK,
                                             KSHMREGION_FLAG_READ|KSHMREGION_FLAG_WRITE
               ))) return NULL;
 result = ktask_newuserex(parent,proc,userstack,ustacksize,kstacksize,
                          KTASK_FLAG_USERTASK|KTASK_FLAG_OWNSUSTACK);
 if __unlikely(!result) kshm_unmap_unlocked(kproc_getshm(proc),userstack,
                                            ceildiv(ustacksize,PAGESIZE),
                                            KSHMUNMAP_FLAG_NONE);
 else *useresp = (__user void *)((uintptr_t)userstack+ustacksize);
 return result;
}

__crit __ref struct ktask *
ktask_newuserex(struct ktask *__restrict parent, struct kproc *__restrict proc,
                __pagealigned __user void *ustackaddr, __size_t ustacksize,
                __size_t kstacksize, __u16 flags) {
 struct ktask *result;
 KTASK_CRIT_MARK
 kassert_ktask(parent); kassert_kproc(proc);
 assertf(krwlock_iswritelocked(&proc->p_shm.s_lock),
         "The caller must lock the pagedir of the associated task context!");
 assertf(isaligned((uintptr_t)ustackaddr,PAGEALIGN),
         "User stack address %p is not page-aligned",ustackaddr);
 assertf(!(flags&KTASK_FLAG_OWNSUSTACK) || kpagedir_ismapped
         (kproc_getpagedir(proc),ustackaddr,ceildiv(ustacksize,PAGESIZE)),
         "Kernel-managed user-stack is not properly allocated");
 assertf(flags&KTASK_FLAG_USERTASK,"The 'KTASK_FLAG_USERTASK' flag is not set in %I16x",flags);
 if __unlikely(KE_ISERR(ktask_incref(parent))) return NULL;
 if __unlikely(KE_ISERR(kproc_incref(proc))) goto err_par;
 if __unlikely((result = omalloc(struct ktask)) == NULL) goto err_ctx;
 kobject_init(result,KOBJECT_MAGIC_TASK);
 /* Allocate and map a linear kernel stack for user tasks. */
 k_syslogf(KLOG_TRACE,"[TASK] Mapping Linear RAM for kernel stack of user-thread\n");
 if __unlikely(KE_ISERR(kshm_mapram_linear_unlocked(kproc_getshm(proc),
                                                   &result->t_kstack,
                                                   &result->t_kstackvp,
                                                    ceildiv(kstacksize,PAGESIZE),
                                                    KPAGEDIR_MAPANY_HINT_KSTACK,
                                                    KSHMREGION_FLAG_LOSEONFORK|
                                                    KSHMREGION_FLAG_RESTRICTED
               ))) goto err_r;
 result->t_flags       = flags;
 result->t_ustackvp    = ustackaddr;
 result->t_ustacksz    = ustacksize;
 result->t_esp         = (__user void *)((uintptr_t)result->t_kstackvp+kstacksize);
 result->t_esp0        = result->t_esp;
 result->t_epd         =
 result->t_userpd      = kproc_getpagedir(proc);
 result->t_kstackend   = (void *)((uintptr_t)result->t_kstack+kstacksize);
 result->t_proc        = proc; /* Inherit reference. */
 result->t_refcnt      = 1;
 result->t_locks       = 0;
 result->t_state       = KTASK_STATE_SUSPENDED;
 result->t_newstate    = KTASK_STATE_RUNNING;
 result->t_suspended   = 1;
 result->t_cpu         = NULL;
 result->t_setpriority = KTASK_PRIORITY_DEF;
 result->t_curpriority = KTASK_PRIORITY_DEF;
#ifdef __DEBUG__
 // These are undefined for suspended tasks
 result->t_prev        = NULL;
 result->t_next        = NULL;
#endif
 result->t_parent      = parent; /* Inherit reference. */
 result->t_sigval      = NULL;
 ktasklist_init(&result->t_children);
 ktlspt_init(&result->t_tls);
#if KCONFIG_HAVE_TASK_STATS
 ktaskstat_init(&result->t_stats);
#endif /* KCONFIG_HAVE_TASK_STATS */
#if !KCONFIG_HAVE_IRQ
 result->t_preempt = KTASK_PREEMT_COUNTDOWN;
#endif /* !KCONFIG_HAVE_IRQ */
#if KCONFIG_HAVE_TASK_DEADLOCK_CHECK
 result->t_deadlock_help     = NULL;
 result->t_deadlock_closure  = NULL;
#endif
 ksignal_init(&result->t_joinsig);
 ktasksig_init(&result->t_sigchain,result);
#if KCONFIG_HAVE_TASK_NAMES
 result->t_name = NULL;
#endif /* KCONFIG_HAVE_TASK_NAMES */
 /* Initialize the task's TID and setup its parent hooks. */
 if __unlikely(KE_ISERR(kproc_addtask(proc,result))) goto err_map;
 ktask_lock(parent,KTASK_LOCK_CHILDREN);
 if __unlikely(ktask_isterminated(parent)) {
  /* TODO: Handle terminate parent task (Fail with
   *       something that doesn't imply no memory...). */
  ktask_unlock(parent,KTASK_LOCK_CHILDREN);
  goto err_ctx2;
 }
 result->t_parid = ktasklist_insert(&parent->t_children,result);
 ktask_unlock(parent,KTASK_LOCK_CHILDREN);
 if __unlikely(result->t_parid == (size_t)-1) goto err_ctx2;
 return result;
err_ctx2: kproc_deltask(proc,result);
err_map:  kshm_unmap_unlocked(kproc_getshm(proc),result->t_kstackvp,
                              ceildiv(ktask_getkstacksize(result),PAGESIZE),
                              KSHMUNMAP_FLAG_RESTRICTED);
err_r:    free(result);
err_ctx:  kproc_decref(proc);
err_par:  ktask_decref(parent);
 return NULL;
}

#ifndef KCONFIG_HAVE_SINGLECORE
struct kcpu *kcpu_self(void) {
 /* TODO: Per-CPU (SMP) */
 return &__kcpu_zero;
}
struct kcpu *kcpu_leastload(void) {
 /* TODO: Per-CPU (SMP) */
 return &__kcpu_zero;
}
#endif /* KCONFIG_HAVE_SINGLECORE */











// =====================
// BEGIN OF SCHEDULE-IMPLEMENTATION
// =====================

#ifndef __INTELLISENSE__
/* Prevent recursion within the scheduler,
 * as caused by yield-calles from spinlocks. */
#undef ktask_tryyield
#undef ktask_yield
#if 1
#   define ktask_tryyield() KE_OK
#else
#   define ktask_tryyield() KS_UNCHANGED
#endif
#   define ktask_yield()    (void)0
#endif /* !__INTELLISENSE__ */


#define LOAD_LDT(x) \
 __asm_volatile__("lldt %0\n" : : "g" (x))


__local kerrno_t kcpu_rotate_unlocked(struct kcpu *__restrict self);
extern void ktask_schedule(struct scheddata *state) {
 /* IRQ Task scheduling callback.
  * WARNING: This function is called without
  *          an up-to-date ktask_self() value! */
 struct kcpu *cpuself = kcpu_self();
 struct ktask *prevtask,*currtask;
 /*serial_printf(SERIAL_01,".");*/
#if 0
 {
  static int curr = 0;
  static struct timespec lasttime;
  struct timespec tm;
  ktime_getnoworcpu(&tm);
  ++curr;
  if (tm.tv_sec != lasttime.tv_sec) {
   k_syslogf(KLOG_TRACE,"QUANTUM after %d\n",curr);
   curr = 0;
   lasttime = tm;
  }
 }
#endif

 if (!kcpu_trylock(cpuself,KCPU_LOCK_TASKS)) return;
 if (cpuself->c_flags&KCPU_FLAG_NOIRQ) {
  kcpu_unlock(cpuself,KCPU_LOCK_TASKS);
  return;
 }
 prevtask = cpuself->c_current;
 if (kcpu_trylock(cpuself,KCPU_LOCK_SLEEP)) {
  kcpu_ticknow_unlocked2(cpuself);
  kcpu_unlock(cpuself,KCPU_LOCK_SLEEP);
  if __unlikely(!prevtask) {
   /* Special handling: Without any tasks before, we must re-checked to see
    *                   if we just managed to wake at least one, and if we
    *                   did, we must switch to that task without storing
    *                   the state of an existing task! */
   currtask = cpuself->c_current;
   kcpu_unlock(cpuself,KCPU_LOCK_TASKS);
   if __unlikely(!currtask) return;
   KTASK_ONSWITCH("IRQ",NULL,currtask);
   /* Load local descriptor table of the new process. */
   LOAD_LDT(kproc_getshm(ktask_getproc(currtask))->s_ldt.ldt_gdtid);
   goto setcurrtask;
  }
 } else if (!prevtask) {
  kcpu_unlock(cpuself,KCPU_LOCK_TASKS);
  return;
 }
 kassert_ktask(prevtask);
#ifdef KCONFIG_HAVE_SINGLECORE
 assertf(KTASK_STATE_ISACTIVE(prevtask->t_newstate) ||
        (prevtask->t_newstate == KTASK_STATE_TERMINATED &&
        (prevtask->t_flags&KTASK_FLAG_CRITICAL))
        ,"Remote-CPU state-switches are not allowed when configured for single-core");
#else
 if (!KTASK_STATE_ISACTIVE(prevtask->t_newstate) &&
    (prevtask->t_newstate != KTASK_STATE_TERMINATED ||
   !(prevtask->t_flags&KTASK_FLAG_CRITICAL))) {
  /* There must be at least one currently scheduled
   * task, which we are being called from. */
  kcpu_unlock(cpuself,KCPU_LOCK_TASKS);
  ktask_lock(prevtask,KTASK_LOCK_STATE);
  kcpu_lock(cpuself,KCPU_LOCK_TASKS);
  assertf(prevtask == cpuself->c_current,
          "Other CPUs should not have been allowed to "
          "modify the current task of a different CPU");
  /* Since our prior check wasn't locked, check again. */
  if (KTASK_STATE_ISACTIVE(prevtask->t_newstate) ||
     (prevtask->t_newstate == KTASK_STATE_TERMINATED &&
     (prevtask->t_flags&KTASK_FLAG_CRITICAL))) {
   ktask_unlock(prevtask,KTASK_LOCK_STATE);
   goto rotate_normal;
  }
  /* New state was requested
   * >> Recognize the changed state. */
  if (prevtask->t_next == prevtask) {
   /* There's nothing we can do here... (we can't approve the state change) */
   ktask_unlock(prevtask,KTASK_LOCK_STATE);
   kcpu_unlock(cpuself,KCPU_LOCK_TASKS);
   return;
  }
  cpuself->c_current = prevtask->t_next;
  prevtask->t_next->t_prev = prevtask->t_prev;
  prevtask->t_prev->t_next = prevtask->t_next;
  if (prevtask->t_newstate == KTASK_STATE_WAITINGTMO) {
   /* Special handling for remote-cpu request
    * to re-schedule with a local timeout. */
   while (!kcpu_trylock(cpuself,KCPU_LOCK_SLEEP)) {
    kcpu_unlock(cpuself,KCPU_LOCK_TASKS);
    kcpu_lock(cpuself,KCPU_LOCK_SLEEP);
    if (kcpu_trylock(cpuself,KCPU_LOCK_TASKS)) break;
    kcpu_unlock(cpuself,KCPU_LOCK_SLEEP);
    kcpu_lock(cpuself,KCPU_LOCK_TASKS);
   }
   assertf(cpuself->c_current == prevtask->t_next,
           "No other CPU should have been allowed to modify this");
   kcpu_schedulesleep_unlocked(cpuself,prevtask); /* Inherit reference. */
   katomic_store(prevtask->t_state,prevtask->t_newstate);
   ktask_unlock(prevtask,KTASK_LOCK_STATE);
   kcpu_unlock(cpuself,KCPU_LOCK_SLEEP);
  } else {
   prevtask->t_cpu = NULL;
#ifdef __DEBUG__
   prevtask->t_prev = NULL;
   prevtask->t_next = NULL;
#endif
   katomic_store(prevtask->t_state,prevtask->t_newstate);
   ktask_unlock(prevtask,KTASK_LOCK_STATE);
  }
  KTASK_ONUNSCHEDULE(prevtask,cpuself);
 } else
#endif
 {
rotate_normal:
  if (kcpu_rotate_unlocked(cpuself) == KS_UNCHANGED) {
   kcpu_unlock(cpuself,KCPU_LOCK_TASKS);
   return;
  }
 }
 currtask = cpuself->c_current;
 assert(prevtask != currtask);
 KTASK_ONSWITCH("IRQ",prevtask,currtask);
 prevtask->t_userpd = state->pd;
 prevtask->t_esp    = state->esp;
 KTASK_ONLEAVEQUANTUM(prevtask,cpuself);
 if (prevtask->t_proc != currtask->t_proc) {
  /* Load local descriptor table of the new process. */
  LOAD_LDT(kproc_getshm(ktask_getproc(currtask))->s_ldt.ldt_gdtid);
 }
 kcpu_unlock(cpuself,KCPU_LOCK_TASKS);
setcurrtask:
 assertf(kpagedir_translate(currtask->t_userpd
                           ,currtask->t_esp
         ) != NULL,"Cannot translate: %p|%p"
         ,currtask->t_esp,currtask->t_userpd);
 state->pd           = currtask->t_userpd;
 state->esp          = currtask->t_esp;
 cpuself->c_tss.esp0 = (uintptr_t)currtask->t_esp0;
 KTASK_ONENTERQUANTUM(currtask,cpuself);
}

#ifndef __INTELLISENSE__
#define UNLOCKED2
#include "task-timetick.c.inl"
#if KCPU_LOCK_SLEEP != KCPU_LOCK_TASKS
#include "task-timetick.c.inl"
#endif
#endif

//////////////////////////////////////////////////////////////////////////
// Rotate the cpu's current task
// @return: KS_EMPTY:     No further tasks are scheduled on the CPU
// @return: KS_UNCHANGED: The selected task didn't change
__local kerrno_t kcpu_rotate_unlocked(struct kcpu *__restrict self) {
 struct ktask *current;
 kassert_kcpu(self);
 assert(kcpu_islocked(self,KCPU_LOCK_TASKS));
 if __unlikely((current = self->c_current) == NULL) return KS_EMPTY;
 assert(current->t_cpu == self);
 kassert_ktask(current->t_prev);
 kassert_ktask(current->t_next);
 /* Special case: Don't do priority scheduling
  *               if only one task is available! */
 if (current == current->t_next) return KS_UNCHANGED;
#if 0
 self->c_current = current->t_next;
#else
 /* High/realtime priority tasks. */
 if (current->t_curpriority > 0) {
  /* Do not manually switch out a realtime task (allowing an RT
   * task to remain scheduled as the top task, until it manually
   * performs an operation that results in a forced unscheduling;
   * aka. causes the task to receive a signal). */
  if (current->t_curpriority == KTASK_PRIORITY_REALTIME) return KE_OK;
  /* Keep high-priority tasks around for a while. */
  if (--current->t_curpriority != 0) return KE_OK;
  /* Re-set the current priority. */
  current->t_curpriority = katomic_load(current->t_setpriority);
 }
 /* Scan for tasks until a zero, or greater priority task is found. */
 for (;;) {
  current = current->t_next;
  if (current->t_curpriority >= 0) break;
  /* Skip priority-suspended tasks. */
  if (current->t_curpriority == KTASK_PRIORITY_SUSPENDED) continue;
  /* Upgrade low-priority tasks. */
  if (++current->t_curpriority == 0) {
   /* Schedule low-priority task. */
   current->t_curpriority = katomic_load(current->t_setpriority);
   break;
  }
 }
 self->c_current = current;
#endif
 assert(ktask_getstate(self->c_current) != KTASK_STATE_TERMINATED);
 assert(self->c_current->t_kstack);
 return KE_OK;
}
void kcpu_schedulesleep_unlocked(struct kcpu *__restrict self,
                           __ref struct ktask *__restrict task) {
 struct ktask *insert_before,*next;
 kassert_kcpu(self);
 kassert_ktask(task);
 assert(ktask_islocked(task,KTASK_LOCK_STATE));
 assert(kcpu_islocked(self,KCPU_LOCK_SLEEP));
 assert(task->t_cpu == self);
 KTASK_ONENTERSLEEP(task,self);
 insert_before = self->c_sleeping;
 if (!insert_before) {
  /* Insert the first task. */
  task->t_prev = NULL;
  task->t_next = NULL;
  self->c_sleeping = task;
  return;
 }
 /* Find the appropriate position to schedule the task at. */
#if 1
 /* For fairness, schedule a timeout task after other
  * tasks with the same timeout, but that came before. */
 while (__timespec_cmple(&insert_before->t_abstime,
                                  &task->t_abstime))
#else
 while (__timespec_cmplo(&insert_before->t_abstime,
                                  &task->t_abstime))
#endif
 {
  next = insert_before->t_next;
  if (!next) {
   /* Insert at the end. */
   task->t_prev = insert_before;
   task->t_next = NULL;
   insert_before->t_next = task;
   return;
  }
  insert_before = next;
 }
 kassertobj(insert_before);
 task->t_next = insert_before;
 task->t_prev = insert_before->t_prev;
 insert_before->t_prev = task;
 if (!task->t_prev) {
  /* Insert at the front. */
  assert(self->c_sleeping == insert_before);
  self->c_sleeping = task;
 } else {
  assert(self->c_sleeping != insert_before);
  task->t_prev->t_next = task;
 }
}
void kcpu_unschedulesleep_unlocked(struct kcpu *__restrict self,
                                   struct ktask *__restrict task) {
 kassert_kcpu(self);
 kassert_ktask(task);
 assert(ktask_islocked(task,KTASK_LOCK_STATE));
 assert(kcpu_islocked(self,KCPU_LOCK_SLEEP));
 assert(self->c_sleeping != NULL);
 assert((self->c_sleeping == task) == (task->t_prev == NULL));
 assert(task->t_cpu == self);
 KTASK_ONLEAVESLEEP(task,self);
 if (task->t_prev) task->t_prev->t_next = task->t_next;
 else self->c_sleeping = task->t_next;
 if (task->t_next) task->t_next->t_prev = task->t_prev;
#ifdef __DEBUG__
 task->t_prev = NULL;
 task->t_next = NULL;
#endif
}


__local __crit void ktask_releasedata(struct ktask *__restrict self);
__local void ktask_onterminate(struct ktask *__restrict self);
__local void ktask_onterminateother(struct ktask *__restrict self);

extern __crit void ktask_decref_f(struct ktask *__restrict self);
__crit void ktask_decref_f(struct ktask *__restrict self) {
 KTASK_CRIT_MARK
 kassert_ktask(self);
 if (katomic_load(self->t_state) == KTASK_STATE_TERMINATED) {
  assert(!(self->t_flags&KTASK_FLAG_CRITICAL));
  assert(!(self->t_flags&KTASK_FLAG_NOINTR));
  /* Release no longer needed task data after switching away. */
  ktask_releasedata(self);
 }
 ktask_decref(self);
}

__local __crit void ktask_releasedata(struct ktask *__restrict self) {
 KTASK_CRIT_MARK
 kassert_object(self,KOBJECT_MAGIC_TASK);
 assert(ktask_isterminated(self) || !self->t_refcnt);
 kassert_kproc(self->t_proc);
#ifdef KTASK_LOCK_RELEASEDATA
 assertef(ktask_trylock(self,KTASK_LOCK_RELEASEDATA)
         ,"ktask_releasedata() Is not properly synchronized!");
#endif
 if (self->t_flags&KTASK_FLAG_USERTASK) {
  /* Unmap the kernel stack of the tasks. */
  assert(!(self->t_flags&KTASK_FLAG_MALLKSTACK));
  if (self->t_kstackvp ||
     (self->t_ustackvp && self->t_flags&KTASK_FLAG_OWNSUSTACK)) {
   kerrno_t lockerror = krwlock_beginwrite(&self->t_proc->p_shm.s_lock);
   if (self->t_kstackvp) {
    assert(isaligned((uintptr_t)self->t_kstackvp,PAGEALIGN));
    assert(isaligned((uintptr_t)self->t_kstack,PAGEALIGN));
    if __likely(KE_ISOK(lockerror)) {
     assert(self->t_kstack == kpagedir_translate(kproc_getshm(ktask_getproc(self))->s_pd,self->t_kstackvp));
     kshm_unmap_unlocked(kproc_getshm(ktask_getproc(self)),self->t_kstackvp,
                         ceildiv(ktask_getkstacksize(self),PAGESIZE),
                         KSHMUNMAP_FLAG_RESTRICTED);
    }
    self->t_kstackvp = self->t_kstack = self->t_kstackend = NULL;
   }
   /* Unmap the user stack of the tasks. */
   if (self->t_ustackvp && self->t_flags&KTASK_FLAG_OWNSUSTACK) {
    if __likely(KE_ISOK(lockerror)) {
     kshm_unmap_unlocked(kproc_getshm(ktask_getproc(self)),self->t_ustackvp,
                         ceildiv(self->t_ustacksz,PAGESIZE),
                         KSHMUNMAP_FLAG_RESTRICTED);
    }
#ifdef __DEBUG__
    self->t_flags &= ~(KTASK_FLAG_OWNSUSTACK);
#endif
    self->t_ustacksz = 0;
    self->t_ustackvp = NULL;
   }
   if __likely(KE_ISOK(lockerror)) {
    krwlock_endwrite(&self->t_proc->p_shm.s_lock);
   }
  }
 } else {
  assert(self->t_ustackvp == NULL);
  assert(self->t_ustacksz == 0);
  assert(self->t_kstackvp == self->t_kstack);
  assert(self->t_userpd == kpagedir_kernel());
  assert(kproc_getshm(ktask_getproc(self))->s_pd == kpagedir_kernel());
  if (self->t_kstack) {
   /* In kernel mode, task stacks are usually allocated through pageframes. */
   if (self->t_flags&KTASK_FLAG_MALLKSTACK) {
    free(self->t_kstack);
   } else {
    assert(isaligned((uintptr_t)self->t_kstack,PAGEALIGN));
    kpageframe_free((struct kpageframe *)self->t_kstack,
                    ceildiv(ktask_getkstacksize(self),PAGESIZE));
   }
   self->t_kstackend = self->t_kstackvp = self->t_kstack = NULL;
  }
 }
 ktlspt_clear(&self->t_tls);
 kproc_deltask(self->t_proc,self);
#ifdef KTASK_LOCK_RELEASEDATA
 ktask_unlock(self,KTASK_LOCK_RELEASEDATA);
#endif
}
__local __crit void ktask_onterminateother(struct ktask *__restrict self) {
 KTASK_CRIT_MARK
 ktask_onterminate(self);
 ktask_releasedata(self);
}

__crit void ktask_destroy(struct ktask *__restrict self) {
 struct ktask *parent;
 KTASK_CRIT_MARK
 if (self->t_state != KTASK_STATE_TERMINATED) {
  assert(self->t_state == KTASK_STATE_SUSPENDED);
  assert(self->t_suspended != 0);
  /* This can happen if a suspended task is detached. */
  if (self->t_flags&KTASK_FLAG_CRITICAL) {
   struct ktask *caller = ktask_self();
   /* NOTE: In any case, it's OK if a kernel task does this... */
   k_syslogf(ktask_isusertask(caller) ? KLOG_WARN : KLOG_MSG
        ,"Reviving+resuming critical, suspended task %I32d:%Iu:%s after all references were dropped\n"
         "Last reference way dropped by%s task %I32d:%Iu:%s\n"
        ,self->t_proc->p_pid,self->t_tid,ktask_getname(self)
        ,ktask_isusertask(caller) ? "(possibly malicious)" : ""
        ,caller->t_proc->p_pid,caller->t_tid,ktask_getname(caller));
   /* _very_ rare (and probably intentional) case:
    *  - The task was suspended while inside a critical block
    *  - Afterwards all references were dropped, causing
    *    us to get here, where we are trying to destroy it.
    * The only (kind-of questionable) thing we can do now,
    * is to forceably reset the suspension counter and manually
    * re-schedule the task, thus leaving it to once again
    * be referenced within the associated CPU.
    *  - Though we can't easily prevent this from happening
    *    again, we can fight such a potential crit-suspend
    *    attacker with the fact that to get here, they had
    *    to drop all references they had to the task, forcing
    *    them to re-open a new reference through (e.g.) its TID.
    *  - Until then, we re-schedule the task, hinting the
    *    scheduler to immediately switch to it, thus allowing
    *    it to do at least ~some~ work before our ~Attacker~
    *    has another chance to suspend it again, and drop
    *    all references another time.
    * WARNING: If this attacker has the ability to keep the task they
    *          are attacking from leaving kernel-space indefinitely
    *          while that other task is locking within a critical block,
    *          whatever they are doing to achieve that won't be our fault! */
   self->t_suspended = 0;
   self->t_refcnt    = 1; /* Magic reference (Will be inherited below). */
   self->t_newstate  = KTASK_STATE_TERMINATED;
   self->t_exitcode  = NULL;
   self->t_flags    |= KTASK_FLAG_WASINTR; 
   /* Re-schedule the task on our own CPU, thus ensuring an immediate task switch. */
   __evalexpr(ktask_reschedule_ex(self,NULL,KTASK_RESCHEDULE_HINT_CURRENT|
                                  KTASK_RESCHEDULE_HINTFLAG_UNDOSUSPEND|
                                  KTASK_RESCHEDULE_HINTFLAG_INHERITREF));
   /* The task already had a chance to run and may, or may not have finished.
    * >> There is no guaranty of us not being back here
    *    again in a moment, but for now, the day is saved... */
   return;
  }
  k_syslogf(KLOG_INFO,"Task %I32d:%Iu:%s was detached while suspended\n",
            self->t_proc->p_pid,self->t_tid,ktask_getname(self));
  ktask_onterminate(self);
 } else {
  assert(!(self->t_flags&KTASK_FLAG_CRITICAL));
  assert(!(self->t_flags&KTASK_FLAG_NOINTR));
 }
 ktask_releasedata(self);
 kproc_decref(self->t_proc);
 parent = self->t_parent;
 kassert_ktask(parent);
 ktask_lock(parent,KTASK_LOCK_CHILDREN);
 asserte(ktasklist_erase(&parent->t_children,self->t_parid) == self);
 ktask_unlock(parent,KTASK_LOCK_CHILDREN);
 ktasklist_quit(&self->t_children);
 ktask_decref(parent);
#if KCONFIG_HAVE_TASK_NAMES
 free(self->t_name);
#endif
 free(self);
}


extern void ktask_switchimpl(__kernel struct ktask *__restrict newtask,
                       __ref __kernel struct ktask *__restrict oldtask);

#ifdef __DEBUG__
/* Switch to a given new task and unlock 'cpuself:KCPU_LOCK_TASKS'
 * NOTE: Also re-enable interrupts, which were disabled by the caller
 * NOTE: Besides doing some assertions, this function also generates
 *       a stackframe entry, as 'ktask_switchimpl' will not have one. */
__noinline void ktask_switchdecref(struct ktask *__restrict newtask,
                             __ref struct ktask *__restrict oldtask) {
 assert(newtask != oldtask);
 kassert_ktask(newtask);
 kassert_ktask(oldtask);
 assert(!karch_irq_enabled());
 assert(kcpu_self()->c_current == newtask);
 kassert_kproc(oldtask->t_proc);
 kassert_kproc(newtask->t_proc);
 KTASK_ONSWITCH("YIELD",oldtask,newtask);
#if 0
 void *esp,*pd;
 __asm_volatile__("movl %%esp, %0\n"
                  "movl %%cr3, %1\n"
                  : "=g" (esp), "=g" (pd));
 printf("SCHED-SWITCH [YIELD]: ESP(%p) PD(%p)\n",esp,pd);
#endif
 kassertobj(newtask->t_userpd);
 assertf(kpagedir_ismappedp(newtask->t_userpd,newtask->t_esp)
        ,"User ESP %p is not mapped in %p"
        ,newtask->t_esp,newtask->t_userpd);
#if 0
 if (newtask->t_userpd == kpagedir_kernel()) {
  printf("newtask-before %p:%I32d:%Iu:%s:\n",
         newtask,newtask->t_proc->p_pid,newtask->t_tid,ktask_getname(newtask));
  printregs(newtask->t_esp);
 }
#endif
 ktask_switchimpl(newtask,oldtask);
#if 0
 if (newtask->t_userpd == kpagedir_kernel()) {
  printf("newtask-after %p:%I32d:%Iu:%s:\n",
         newtask,newtask->t_proc->p_pid,newtask->t_tid,ktask_getname(newtask));
  printregs(newtask->t_esp);
  tb_print();
 }
#endif
}
#else
#define ktask_switchdecref ktask_switchimpl
#endif


#if KCONFIG_HAVE_TASK_DEADLOCK_CHECK
static __atomic int _ktask_deadlock_intended = 0;
__crit void ktask_deadlock_intended_begin(void) { katomic_incfetch(_ktask_deadlock_intended); }
__crit void ktask_deadlock_intended_end(void) { katomic_decfetch(_ktask_deadlock_intended); }
#endif


/* Idly wait for tasks to show up, occasionally spinning the tick timer
 * >> Upon return, at least one valid task is stored in 'cpuself->c_current'. */
__local void
kcpu_idlewaitfortasks_unlocked(struct kcpu *cpuself,
                               struct ktask *oldtask) {
#if KCONFIG_HAVE_TASK_DEADLOCK_CHECK
 int deadlock_printed = 0;
#endif
 kassert_kcpu(cpuself);
 assert(!karch_irq_enabled());
 assert(kcpu_islocked(cpuself,KCPU_LOCK_TASKS));
 while (!cpuself->c_current) {
  if (kcpu_trylock(cpuself,KCPU_LOCK_SLEEP)) {
   kcpu_ticknow_unlocked2(cpuself);
   kcpu_unlock(cpuself,KCPU_LOCK_SLEEP);
  }
  if (!ktask_isterminated(oldtask) ||
      KE_ISERR(ksignal_close(&oldtask->t_joinsig))) {
   /* We can't allow the IRQ to perform the switch,
    * so we can later decref the old task. */
   cpuself->c_flags |= KCPU_FLAG_NOIRQ;
   kcpu_unlock(cpuself,KCPU_LOCK_TASKS);
   karch_irq_enable();
#if KCONFIG_HAVE_TASK_DEADLOCK_CHECK
   if (!deadlock_printed) {
    deadlock_printed = 1;
    if (!katomic_load(_ktask_deadlock_intended)) {
     serial_printf(SERIAL_01,":::ERROR::: Deadlock:\n");
     tb_print();
     if (oldtask->t_deadlock_help) {
      (*oldtask->t_deadlock_help)(oldtask->t_deadlock_closure);
     }
    }
   }
#endif /* KCONFIG_HAVE_TASK_DEADLOCK_CHECK */
   karch_irq_idle();
   karch_irq_disable();
   kcpu_lock(cpuself,KCPU_LOCK_TASKS);
   cpuself->c_flags &= ~(KCPU_FLAG_NOIRQ);
  }
  assert(kcpu_islocked(cpuself,KCPU_LOCK_TASKS));
 }
}


__local void ktask_selectcurrunlock_anddecref_ni(struct kcpu *__restrict cpuself,
                                                 struct ktask *__restrict oldtask) {
 assert(!karch_irq_enabled());
 assert(kcpu_islocked(cpuself,KCPU_LOCK_TASKS));
 assert(cpuself == kcpu_self());
 KTASK_ONLEAVEQUANTUM(oldtask,cpuself);
 kcpu_idlewaitfortasks_unlocked(cpuself,oldtask);
 kcpu_unlock(cpuself,KCPU_LOCK_TASKS);
 /* NOTE: Since we're cpuself, we have exclusive write access to
  *       'cpuself->c_current', meaning that we can assume that
  *       'cpuself->c_current' will continue to contain a valid
  *       task pointer. */
 kassert_ktask(cpuself->c_current);
 assert(!karch_irq_enabled());
 KTASK_ONENTERQUANTUM(cpuself->c_current,cpuself);
 if (cpuself->c_current != oldtask) {
  ktask_switchdecref(cpuself->c_current,oldtask);
 } else {
  assertef(katomic_decfetch(oldtask->t_refcnt) >= 1,
           "You're the one calling & who just got re-scheduled!");
 }
}
__local void ktask_selectcurr_anddecref_ni(struct ktask *__restrict oldtask) {
 struct kcpu *cpuself = kcpu_self();
 assert(!karch_irq_enabled());
 kcpu_lock(cpuself,KCPU_LOCK_TASKS);
 ktask_selectcurrunlock_anddecref_ni(cpuself,oldtask);
}

__local void ktask_selectnext_anddecref_ni(struct ktask *__restrict oldtask) {
 struct kcpu *cpuself;
 assert(!karch_irq_enabled());
 cpuself = kcpu_self();
 kcpu_lock(cpuself,KCPU_LOCK_TASKS);
 if (kcpu_rotate_unlocked(cpuself) == KS_EMPTY) {
  kcpu_idlewaitfortasks_unlocked(cpuself,oldtask);
 }
 kcpu_unlock(cpuself,KCPU_LOCK_TASKS);
 if (cpuself->c_current != oldtask) {
  KTASK_ONENTERQUANTUM(cpuself->c_current,cpuself);
  ktask_switchdecref(cpuself->c_current,oldtask);
 } else {
  assertef(katomic_decfetch(oldtask->t_refcnt) >= 1,
           "You're the one calling & who just got re-scheduled!");
 }
}

struct kcpu *ktask_getcpu(struct ktask *__restrict self) {
 struct kcpu *result;
 kassert_ktask(self);
 NOIRQ_BEGINLOCK(ktask_trylock(self,KTASK_LOCK_STATE))
 result = self->t_cpu;
 NOIRQ_ENDUNLOCK(ktask_unlock(self,KTASK_LOCK_STATE))
#ifdef __DEBUG__
 if (result) kassert_kcpu(result);
#endif
 return result;
}


/* Called when a task terminates. */
__local void ktask_onterminate(struct ktask *__restrict self) {
 kassert_object(self,KOBJECT_MAGIC_TASK);
 assertf(ktask_islocked(self,KTASK_LOCK_STATE) ||
         !self->t_refcnt,"Task state is not locked");
 KTASK_ONTERMINATE(self);
 /* Can't delete the task from its proc here:
  * If we're that task and is the last,
  * we would free our own stack. */
 /*kproc_deltask(self->t_proc,self);*/
 ksignal_close(&self->t_joinsig);
}

__local void ktask_unschedulecurrent(struct ktask *__restrict self) {
 /* Unschedule the task from the associated CPU's current list. */
 struct kcpu *task_cpu;
 kassert_ktask(self);
 assertf(ktask_islocked(self,KTASK_LOCK_STATE),"Task state is not locked");
 assert(KTASK_STATE_ISACTIVE(self->t_state));
 task_cpu = self->t_cpu; kassert_kcpu(task_cpu);
 kcpu_lock(task_cpu,KCPU_LOCK_TASKS);
 if (self == task_cpu->c_current) {
  if (self == self->t_next) {
   task_cpu->c_current = NULL;
  } else {
   task_cpu->c_current = self->t_next;
  }
 }
 self->t_prev->t_next = self->t_next;
 self->t_next->t_prev = self->t_prev;
 self->t_cpu = NULL;
#ifdef __DEBUG__
 self->t_prev = NULL;
 self->t_next = NULL;
#endif
 KTASK_ONUNSCHEDULE(self,task_cpu);
 kcpu_unlock(task_cpu,KCPU_LOCK_TASKS);
}
__local void ktask_scheduletimeout(__ref struct ktask *__restrict self) {
 struct kcpu *cpuself; kassert_ktask(self);
 assertf(ktask_islocked(self,KTASK_LOCK_STATE),"Task state is not locked");
 cpuself = self->t_cpu; kassert_kcpu(cpuself);
 kcpu_lock(cpuself,KCPU_LOCK_SLEEP);
 /* Schedule the task in the associated CPU's sleeper list. */
 kcpu_schedulesleep_unlocked(cpuself,self);
 kcpu_unlock(cpuself,KCPU_LOCK_SLEEP);
}
__local void ktask_upgradetimeout(struct ktask *__restrict self) {
 struct kcpu *cpuself; kassert_ktask(self);
 assertf(ktask_islocked(self,KTASK_LOCK_STATE),"Task state is not locked");
 cpuself = self->t_cpu; kassert_kcpu(cpuself);
 kcpu_lock(cpuself,KCPU_LOCK_SLEEP);
 /* Move the task ahead in associated CPU's sleeper list
  * TODO: This can be optimized: We know the task must be moved forward,
  *       so we could compare its timeout with that of its predecessor. */
 kcpu_unschedulesleep_unlocked(cpuself,self);
 kcpu_schedulesleep_unlocked(cpuself,self);
 kcpu_unlock(cpuself,KCPU_LOCK_SLEEP);
}
__local void ktask_unscheduletimeout(struct ktask *__restrict self) {
 struct kcpu *cpuself; kassert_ktask(self);
 assertf(ktask_islocked(self,KTASK_LOCK_STATE),"Task state is not locked");
 cpuself = self->t_cpu; kassert_kcpu(cpuself);
 kcpu_lock(cpuself,KCPU_LOCK_SLEEP);
 /* Unschedule the task from the associated CPU's sleeper list,
  * The caller will drop the reference previously held by the sleeper list. */
 kcpu_unschedulesleep_unlocked(cpuself,self);
 kcpu_unlock(cpuself,KCPU_LOCK_SLEEP);
}
#if 1
#define ktask_scheduleandunlocksigs \
        ktask_scheduleandunlockmoresigs
#else
__local kerrno_t
ktask_scheduleandunlocksigs(struct ktask *self,
                            struct ksigset const *signals) {
 kerrno_t error;
 kassert_ktask(self);
 assertf(ktask_islocked(self,KTASK_LOCK_STATE),"Task state is not locked");
 /* Create a sigchain entry for every signal, then unlock them. */
 ktask_lock(self,KTASK_LOCK_SIGCHAIN);
 error = ktasksig_addsigs_andunlock(&self->t_sigchain,signals);
 ktask_unlock(self,KTASK_LOCK_SIGCHAIN);
 return KE_OK;
}
#endif
__local kerrno_t
ktask_scheduleandunlockmoresigs(struct ktask *__restrict self,
                                struct ksigset const *signals) {
 kerrno_t error;
 kassert_ktask(self);
 assertf(ktask_islocked(self,KTASK_LOCK_STATE),"Task state is not locked");
 /* Create a sigchain entry for every signal, then unlock them. */
 ktask_lock(self,KTASK_LOCK_SIGCHAIN);
 error = ktasksig_addsigs_andunlock(&self->t_sigchain,signals);
 ktask_unlock(self,KTASK_LOCK_SIGCHAIN);
 return error;
}
__local void ktask_unscheduleallsignals(struct ktask *__restrict self) {
 kassert_ktask(self);
 assertf(ktask_islocked(self,KTASK_LOCK_STATE),"Task state is not locked");
 //assertf(KTASK_STATE_ISWAITING(self->t_state),"Task isn't in a waitable state");
 ktask_lock(self,KTASK_LOCK_SIGCHAIN);
 /* Remove all signals the task is currently receiving. */
 ktasksig_clear(&self->t_sigchain);
 ktask_unlock(self,KTASK_LOCK_SIGCHAIN);
}


/* Unschedule a given task without any associated CPU after
 * the special state change cases have already been handled. */
__local kerrno_t
__ktask_unschedule_nocpu(struct ktask *__restrict self, ktask_state_t newstate,
                         void *__restrict arg, struct ksigset const *signals) {
 kerrno_t error;
 kassert_ktask(self);
 assertf(ktask_islocked(self,KTASK_LOCK_STATE),"Task state is not locked");
 assertf(!KTASK_STATE_HASCPU(self->t_state),"Task does have a CPU");
 assertf(self->t_state != KTASK_STATE_WAITINGTMO,
         "No timeout can be set without a CPU");
 assert(self->t_state != newstate);
 if (self->t_newstate == KTASK_STATE_TERMINATED) {
  /* This can happen when a critical task was re-scheduled
   * as terminated, then re-scheduled again as suspended/waiting. */
  return KE_DESTROYED;
 }
 /* Task isn't scheduled on any CPU.
  * >> We must setup a pending newstate to switch
  *    from e.g.: 'suspended --> terminated' or
  *         e.g.: 'waiting --> waitingtmo'.
  * NOTE: Signals that the task is meant to receive
  *       will already be scheduled. */
 switch (newstate) {
  case KTASK_STATE_SUSPENDED:
   assert(self->t_state == KTASK_STATE_WAITING);
   /* waiting --> suspended (Schedule as pending waiting)
    *  - Can't be KTASK_STATE_SUSPENDED (Handled by KS_UNCHANGED)
    *  - Can't be KTASK_STATE_WAITINGTMO (Would have required a CPU)
    *  - Can't be KTASK_STATE_TERMINATED (Handled by KE_DESTROYED)
    * NOTE: No need to unschedule anything, as a
    *       waiting task already isn't scheduled. */
   katomic_store(self->t_state,KTASK_STATE_SUSPENDED);
   self->t_newstate = KTASK_STATE_WAITING;
   return KE_OK;
  case KTASK_STATE_WAITING:
  case KTASK_STATE_WAITINGTMO:
   /* suspended --> waiting (Schedule as pending waiting)
    * Unschedule to waiting.
    * >> The old state must be 'suspended':
    *  - Can't be KTASK_STATE_WAITING (Handled by KS_UNCHANGED)
    *  - Can't be KTASK_STATE_WAITINGTMO (Also handled by KS_UNCHANGED; special case: waiting --> waiting; Also would have required a CPU)
    *  - Can't be KTASK_STATE_TERMINATED (Handled by KE_DESTROYED). */
   assert(self->t_state == KTASK_STATE_SUSPENDED);
   /* >> Only once the task is resumed should it enter the wait state.
    * Simply schedule the task to wait on our own CPU. */
   if (newstate == KTASK_STATE_WAITINGTMO &&
      (self->t_newstate != KTASK_STATE_WAITINGTMO ||
       __timespec_cmplo((struct timespec *)arg,&self->t_abstime))) {
    /* Update/Set the timeout to be applied once the task is resumed. */
    self->t_abstime = *(struct timespec *)arg;
   }
   /* Schedule the additional signals.
    * NOTE: Additional, because since the task was already suspended,
    *       this may not be the first time that we've got here. */
   error = ktask_scheduleandunlockmoresigs(self,signals);
   if __unlikely(KE_ISERR(error)) return error;
   /* NOTE: Since the task is currently suspended, we cannot actually
    *       overwrite its current state, meaning we have to schedule
    *       the new state to become active once the task becomes
    *       unsuspended. */
   self->t_newstate = newstate;
   return KE_OK;
  case KTASK_STATE_TERMINATED:
   /* waiting   --> terminated (Unschedule task from all receiving signals)
    * suspended --> terminated (Just switch to terminated)
    * Task is being terminated
    *  - Can't be KTASK_STATE_TERMINATED (Handled by KE_DESTROYED).
    *  - Can't be KTASK_STATE_WAITINGTMO (Would have required a CPU). */
   assert(self->t_state == KTASK_STATE_WAITING ||
          self->t_state == KTASK_STATE_SUSPENDED);
   assert(!(self->t_flags&KTASK_FLAG_CRITICAL));
   assert(!(self->t_flags&KTASK_FLAG_NOINTR));
   if (self->t_state == KTASK_STATE_WAITING) {
    /* Unschedule signals. */
    ktask_unscheduleallsignals(self);
   }
   /* Set the task's exit code. */
   self->t_exitcode = arg;
   katomic_store(self->t_state,newstate);
   return KE_OK;
  default: __compiler_unreachable();
 }
}

/* Perform post unscheduling operations, or
 * re-schedule the task if an error occurred. */
kerrno_t __SCHED_CALL
ktask_dopostunscheduling(struct kcpu *__restrict cputask,
                         struct ktask *__restrict self,
                         ktask_state_t newstate, void *__restrict arg,
                         struct ksigset const *signals) {
 kerrno_t error;
 /* Schedule signals & timeouts
  * NOTE: At this point, we've inherited the
  *       reference the task's old CPU held. */
 kassert_kcpu(cputask);
 assert(!self->t_cpu);
 assert(newstate == KTASK_STATE_SUSPENDED ||
        newstate == KTASK_STATE_WAITING ||
        newstate == KTASK_STATE_WAITINGTMO ||
        newstate == KTASK_STATE_TERMINATED);
 if (self->t_state == KTASK_STATE_SUSPENDED) {
  error = ktask_scheduleandunlocksigs(self,signals);
  if __unlikely(KE_ISERR(error)) return error;
  /* Schedule a pending state change for a suspended task. */
  switch (newstate) {
   case KTASK_STATE_WAITINGTMO: self->t_abstime = *(struct timespec *)arg; break;
   case KTASK_STATE_TERMINATED: self->t_exitcode = arg; break;
   default: break;
  }
  self->t_newstate = newstate;
  return KS_BLOCKING;
 }
 switch (newstate) {
  case KTASK_STATE_SUSPENDED:
   /* Nothing to do here (no re-scheduling necessary). */
   error = KE_OK;
   break;
  case KTASK_STATE_WAITINGTMO:
   /* Old state can't be 'KTASK_STATE_WAITINGTMO' (same state). */
   assert(self->t_state == KTASK_STATE_RUNNING);
   if __unlikely(KE_ISERR(error = ktask_incref(self))) {
reschedule_running:
    /* Revert the changes and re-schedule the task. */
    kcpu_lock(cputask,KCPU_LOCK_TASKS);
    self->t_cpu = cputask;
    if (cputask->c_current) {
     self->t_next = cputask->c_current;
     self->t_prev = cputask->c_current->t_prev;
     self->t_next->t_prev = self;
     self->t_prev->t_next = self;
    } else {
     self->t_prev = self;
     self->t_next = self;
    }
    cputask->c_current = self;
    KTASK_ONRESCHEDULE(self,cputask);
    katomic_store(self->t_state,KTASK_STATE_RUNNING);
    kcpu_unlock(cputask,KCPU_LOCK_TASKS);
    return error;
   }
   /* Schedule the timeout. */
   self->t_abstime = *(struct timespec *)arg;
   self->t_cpu = cputask;
   ktask_scheduletimeout(self); /* Inherit reference. */
  case KTASK_STATE_WAITING:
   /* Old state can't be 'KTASK_STATE_WAITINGTMO'
    * -> Already handled by: waiting --> waiting. */
   assert(self->t_state == KTASK_STATE_RUNNING);
   /* Schedule signals. */
   error = ktask_scheduleandunlocksigs(self,signals);
   if __unlikely(KE_ISERR(error)) {
    if (newstate == KTASK_STATE_WAITINGTMO) {
     ktask_unscheduletimeout(self);
     assert(katomic_load(self->t_refcnt) >= 2);
     katomic_decfetch(self->t_refcnt);
    }
    goto reschedule_running;
   }
   break;
  case KTASK_STATE_TERMINATED:
   /* Simply set the task's exitcode. */
   self->t_exitcode = arg;
   error = KE_OK;
   break;
  default: __compiler_unreachable();
 }
 /* At this point, we still own a reference to the task 'self'
  * >> Unlock the task, decref it, then re-enable interrupt and exit with KE_OK. */
 katomic_store(self->t_state,newstate);
 return error;
}

kerrno_t __SCHED_CALL
ktask_unschedule_ex(struct ktask *__restrict self, ktask_state_t newstate,
                    void *__restrict arg, struct ksigset const *signals) {
 struct kcpu *cpuself,*cputask;
 kerrno_t error;
 kassert_ktask(self);
 assertf(!KTASK_STATE_ISACTIVE(newstate),"Expected an inactive newstate");
 assertf(newstate == KTASK_STATE_SUSPENDED ||
         newstate == KTASK_STATE_TERMINATED ||
         newstate == KTASK_STATE_WAITING ||
         newstate == KTASK_STATE_WAITINGTMO
        ,"Invalid new state: %I8u(%I8x)"
        ,newstate,newstate);
 assertf(!(signals && signals->ss_elemcnt) || KTASK_STATE_ISWAITING(newstate)
        ,"Signals may only be specified on waiting states");
#ifdef __DEBUG__
 if (newstate == KTASK_STATE_WAITINGTMO) {
  /* 'KTASK_STATE_WAITINGTMO' expects a timespec as argument. */
  kassertobj((struct timespec *)arg);
 }
#endif
 NOIRQ_BEGINLOCK(ktask_trylock(self,KTASK_LOCK_STATE));
 assertf((self->t_cpu != NULL) == KTASK_STATE_HASCPU(self->t_state)
         ,"CPU pointer and state of task %p contradict each other (t_cpu != NULL: %d | state: %d)"
         ,self,(int)(self->t_cpu != NULL),(int)self->t_state);
 /* Check for special case: Task was terminated. */
 if (self->t_state == KTASK_STATE_TERMINATED) {
  /* Task was already destroyed. */
  error = KE_DESTROYED;
  goto end;
 }
 if __unlikely(self->t_flags&KTASK_FLAG_ALARMED) {
  if (newstate == KTASK_STATE_WAITING) {
   struct timespec tmnow;
   self->t_flags &= ~(KTASK_FLAG_ALARMED);
   ktime_getnoworcpu(&tmnow);
   /* Check for special case: The given timeout has already passed. */
   if (__timespec_cmpge(&tmnow,&self->t_abstime)) {
    error = KE_TIMEDOUT;
    goto end;
   }
   newstate = KTASK_STATE_WAITINGTMO;
   arg = &self->t_abstime;
  } else if (newstate == KTASK_STATE_WAITINGTMO) {
   self->t_flags &= ~(KTASK_FLAG_ALARMED);
  }
 }
 /* Check for special case: State didn't change/waiting --> waiting. */
 if (self->t_state == newstate || (
  KTASK_STATE_ISWAITING(self->t_state) &&
  KTASK_STATE_ISWAITING(newstate))) {
  /* State didn't change
   * NOTE: If the new state is a waiting state and signals
   *       were specified or a timeout was set, we need
   *       to update the task's timeout and signals.
   *    >> For timeouts, the new timeout is min-ed together with the old. */
  assert(!KTASK_STATE_ISACTIVE(self->t_state));
  error = ktask_scheduleandunlockmoresigs(self,signals);
  if __unlikely(KE_ISERR(error)) goto end;
  /* Update a possible timeout. */
  if (newstate == KTASK_STATE_WAITINGTMO) {
   assert(!KTASK_STATE_ISACTIVE(self->t_state));
   if (self->t_state != KTASK_STATE_WAITINGTMO) {
    assert(self->t_state == KTASK_STATE_WAITING);
    assertf(!self->t_cpu,"No CPU should have been set for KTASK_STATE_WAITING");
    /* Timeout was set. */
    error = ktask_incref(self);
    if __unlikely(KE_ISERR(error)) goto end;
    self->t_cpu = kcpu_leastload();
    self->t_abstime = *((struct timespec *)arg);
    ktask_scheduletimeout(self);
   } else if (__timespec_cmplo((struct timespec *)arg,&self->t_abstime)) {
    /* Timeout was lowered. */
    self->t_abstime = *((struct timespec *)arg);
    /* For upgrading, a CPU must have already been present. */
    kassert_kcpu(self->t_cpu);
    ktask_upgradetimeout(self);
   }
  } else if (self->t_state == KTASK_STATE_WAITINGTMO) {
   /* Cancel an existing timeout. */
   ktask_unscheduletimeout(self);
   self->t_cpu = NULL;
   katomic_store(self->t_state,newstate);
   error = KS_UNCHANGED;
   goto end_decref;
  }
  katomic_store(self->t_state,newstate);
  error = KS_UNCHANGED;
  goto end;
 }
 /* Check for special case: Terminating a critical task. */
 if __unlikely(newstate == KTASK_STATE_TERMINATED &&
              (self->t_flags&KTASK_FLAG_CRITICAL)) {
  k_syslogf(KLOG_TRACE,"Scheduling future termination of critical task %p: %I32d:%Iu:%s\n",
            self,self->t_proc->p_pid,self->t_tid,ktask_getname(self));
  /* Task is currently performing a critical operation
   * >> We must schedule its terminating through 't_newstate'. */
  if (self->t_newstate == KTASK_STATE_TERMINATED) {
   /* Task is already scheduled to be terminated. */
   error = KE_DESTROYED;
   goto end;
  }
  /* Set the task's new state to be terminated (also set the exit code). */
  self->t_newstate = KTASK_STATE_TERMINATED;
  self->t_exitcode = arg;
  if __likely(!(self->t_flags&KTASK_FLAG_NOINTR)) {
   if (self == ktask_self()) error = KE_INTR;
   else {
    /* Reschedule the task to continue running, but return KE_INTR
     * >> This is the most likely case to happen:
     *  - Some task1 is marked as critical and currently waiting for some signal.
     *  - Some other task2 tries to terminate
     *  - task1 should stop receiving the signal and return KE_INTR
     * >> If task1 wasn't critical, it would just be terminated without warning. */
    if (self->t_state != KTASK_STATE_RUNNING) {
     struct kcpu *newcpu;
     if (self->t_state == KTASK_STATE_WAITINGTMO) {
      ktask_unscheduletimeout(self);
      /* We re-use the reference from the timeout. */
      newcpu = self->t_cpu;
     } else {
      assert(!KTASK_STATE_HASCPU(self->t_state));
      if __unlikely(KE_ISERR(error = ktask_incref(self))) goto end;
      self->t_cpu = newcpu = kcpu_leastload();
     }
     kassert_kcpu(newcpu);
     assert(newcpu == self->t_cpu);
     ktask_unscheduleallsignals(self);
     kcpu_lock(newcpu,KCPU_LOCK_TASKS);
     if ((self->t_prev = newcpu->c_current) != NULL) {
      self->t_next = newcpu->c_current->t_next;
      self->t_prev->t_next = self;
      self->t_next->t_prev = self;
     } else {
      self->t_prev = self;
      self->t_next = self;
      newcpu->c_current = self;
     }
     KTASK_ONRESCHEDULE(self,newcpu);
     katomic_store(self->t_state,KTASK_STATE_RUNNING);
     kcpu_unlock(newcpu,KCPU_LOCK_TASKS);
    }
    self->t_flags |= KTASK_FLAG_WASINTR;
    error = KS_BLOCKING;
   }
  } else {
   self->t_flags |= KTASK_FLAG_WASINTR;
   error = KS_BLOCKING;
  }
  goto end;
 }
 /* Check for special case: Unscheduling a waiting task. */
 if __unlikely(KTASK_STATE_ISWAITING(self->t_state)) {
  assert(self != ktask_self());
  /* - Can't be KTASK_STATE_WAITING (handled by special case waiting --> waiting)
   * - Can't be KTASK_STATE_WAITINGTMO (*ditto*). */
  assert(newstate == KTASK_STATE_TERMINATED ||
         newstate == KTASK_STATE_SUSPENDED);
  if (self->t_state == KTASK_STATE_WAITINGTMO) {
   /* Unschedule the task from its cpu's sleeper list. */
   ktask_unscheduletimeout(self);
   /* The task is no longer scheduled on any CPU (update t_cpu to be NULL). */
   self->t_cpu = NULL;
  }
  if (newstate == KTASK_STATE_TERMINATED) {
   ktask_unscheduleallsignals(self);
   self->t_exitcode = arg;
   katomic_store(self->t_state,KTASK_STATE_TERMINATED);
   ktask_onterminateother(self);
  } else {
   self->t_newstate = self->t_state;
   katomic_store(self->t_state,newstate);
  }
  error = KE_OK;
  goto end_decref;
 }
 /* Special cases are done. --> Let's get to the meat! */

 /* HINT: Inside of the nointerrupt block, 'kcpu_self()' will never change. */
 if ((cputask = self->t_cpu) != NULL) {
  struct ktask *nexttask;
  cpuself = kcpu_self();
restart_cputask:
  assertf(self->t_state != KTASK_STATE_WAITINGTMO,
          "Should have been handled by 'KTASK_STATE_ISWAITING(self->t_state)'");
  kcpu_lock(cputask,KCPU_LOCK_TASKS);
  kassert_ktask(self->t_prev);
  kassert_ktask(self->t_next);
  assert((self->t_next == self) ==
         (self->t_prev == self));
  assert(KTASK_STATE_HASCPU(self->t_state));
  nexttask = self->t_next;
  assert(nexttask != self || self == cputask->c_current);
  if (self == cputask->c_current) {
   /* Unscheduling the current task of a CPU. */
#ifndef KCONFIG_HAVE_SINGLECORE
   if (cputask == cpuself)
#endif /* !KCONFIG_HAVE_SINGLECORE */
   {
    /* Must check for an interrupt before starting. */
    if ((self->t_flags&(KTASK_FLAG_NOINTR|KTASK_FLAG_WASINTR)) == KTASK_FLAG_WASINTR) {
     self->t_flags &= ~(KTASK_FLAG_WASINTR);
     error = KE_INTR;
     goto end;
    }
    /* Unschedule, then switch to a different task
     * WARNING: We're about to unschedule ourselves */
    cpuself->c_current = self->t_next;
    if __unlikely(cpuself->c_current == self) {
     assert(self->t_prev == self &&
            self->t_next == self);
     cpuself->c_current = NULL;
    } else {
     self->t_prev->t_next = self->t_next;
     self->t_next->t_prev = self->t_prev;
    }
    self->t_cpu = NULL;
#ifdef __DEBUG__
    self->t_prev = NULL;
    self->t_next = NULL;
#endif
    KTASK_ONUNSCHEDULE(self,cpuself);
    error = ktask_dopostunscheduling(cputask,self,newstate,arg,signals);
    if __unlikely(KE_ISERR(error)) goto end;
    assert(self->t_state == newstate);
#if 1
    if (newstate == KTASK_STATE_TERMINATED) {
     kcpu_unlock(cputask,KCPU_LOCK_TASKS);
     self->t_exitcode = arg;
     ktask_onterminate(self);
     ktask_unlock(self,KTASK_LOCK_STATE);
     /* The following call will not return
      * because the new state is termination */
     ktask_selectcurr_anddecref_ni(self);
     __compiler_unreachable();
    } else {
     ktask_unlock(self,KTASK_LOCK_STATE);
     ktask_selectcurrunlock_anddecref_ni(cpuself,self);
    }
#else
    kcpu_unlock(cputask,KCPU_LOCK_TASKS);
    if (newstate == KTASK_STATE_TERMINATED) {
     self->t_exitcode = arg;
     ktask_onterminate(self);
    }
    ktask_unlock(self,KTASK_LOCK_STATE);
    /* The following call may not return if the new state was terminated. */
    ktask_selectcurr_anddecref_ni(self);
#endif
    ktask_lock(self,KTASK_LOCK_STATE);
    if ((self->t_flags&(KTASK_FLAG_NOINTR|KTASK_FLAG_WASINTR)) == KTASK_FLAG_WASINTR) {
     self->t_flags &= ~(KTASK_FLAG_WASINTR);
     error = KE_INTR;
    } else if (newstate == KTASK_STATE_WAITINGTMO) {
     /* Check if the timed-out flag was set. */
     if (self->t_flags&KTASK_FLAG_TIMEDOUT) {
      self->t_flags &= ~(KTASK_FLAG_TIMEDOUT);
      error = KE_TIMEDOUT;
     }
    }
    ktask_unlock(self,KTASK_LOCK_STATE);
    goto break_end;
   }
#ifndef KCONFIG_HAVE_SINGLECORE
restart_reqstate:
   /* Attempting to unschedule the current task of a different CPU
    * >> This is kind-of complicated, because we need to tell the
    *    other CPU to do the unscheduling for us or switch to a
    *    different task. */
   self->t_newstate = newstate;
   for (;;) {
    int volatile spinner = 0xffff;
    kcpu_unlock(cputask,KCPU_LOCK_TASKS);
    ktask_unlock(self,KTASK_LOCK_STATE);
    while (spinner--);
    ktask_lock(self,KTASK_LOCK_STATE);
    /* If the task switched CPUs, restart the whole operation. */
    if (!self->t_cpu) break; /* The task was unscheduled from the CPU. */
    if (self->t_cpu != cputask) goto restart_cputask;
    kcpu_lock(cputask,KCPU_LOCK_TASKS);
    if (self != cputask->c_current) {
     /* The CPU has switched to a different task. */
     goto unschedule_noncurrent;
    }
    if (self->t_newstate != newstate) goto restart_reqstate;
   }
   /* The task was unscheduled. */
   assert(!self->t_cpu);
#endif /* !KCONFIG_HAVE_SINGLECORE */
  } else {
unschedule_noncurrent:
   assert(self->t_prev != self && self->t_next != self);
   /* Not the current task
    * -> It's not the only task.
    * -> We can simply remove it from
    *    whatever CPU it's currently on. */
   self->t_prev->t_next = self->t_next;
   self->t_next->t_prev = self->t_prev;
#ifdef __DEBUG__
   self->t_prev = NULL;
   self->t_next = NULL;
#endif
   self->t_cpu = NULL;
  }
  kcpu_unlock(cputask,KCPU_LOCK_TASKS);
 } else {
  /* Task isn't scheduled on any CPU. */
  error = __ktask_unschedule_nocpu(self,newstate,arg,signals);
  if (newstate == KTASK_STATE_TERMINATED &&
      error == KE_OK) ktask_onterminateother(self);
  goto end;
 }
 /* NOTE: At this point, we've inherited the reference the task's old CPU held. */
 error = ktask_dopostunscheduling(cputask,self,newstate,arg,signals);
 if (newstate == KTASK_STATE_TERMINATED &&
     error == KE_OK) ktask_onterminateother(self);
end_decref:
 ktask_unlock(self,KTASK_LOCK_STATE);
 ktask_decref(self);
break_end:
 {
  NOIRQ_BREAK
  /*if (error == KE_INTR) tb_print();*/
  return error;
 }
end:
 NOIRQ_ENDUNLOCK(ktask_unlock(self,KTASK_LOCK_STATE));
 /*if (error == KE_INTR) tb_print();*/
 return error;
}


void __SCHED_CALL
ktask_unschedule_aftercrit(struct ktask *__restrict self) {
 assertf(self == ktask_self(),
         "ktask_unschedule_aftercrit() must be called with ktask_self()");
 NOIRQ_BEGINLOCK(ktask_trylock(self,KTASK_LOCK_STATE));
 assertf(self->t_flags&KTASK_FLAG_CRITICAL,
         "The 'KTASK_FLAG_CRITICAL' flag was not set. - You're not a critical task!");
 self->t_flags &= ~(KTASK_FLAG_CRITICAL);
 switch (self->t_newstate) {
  case KTASK_STATE_RUNNING:
   /* Nothing happened (most likely case). */
   break;
  {
   struct timespec now;
  case KTASK_STATE_WAITINGTMO:
   /* Check if the timeout has already passed
    * >> This way we can skip having to unschedule the task. */
   ktime_getnoworcpu(&now);
   if (__timespec_cmpge(&now,&self->t_abstime)) {
    self->t_flags |= KTASK_FLAG_TIMEDOUT;
    break;
   }
   /* Re-schedule the timeout, to have it run its course the normal way. */
   ktask_unschedulecurrent(self);
   ktask_scheduletimeout(self);
   self->t_newstate = KTASK_STATE_RUNNING;
   break;
  }
  case KTASK_STATE_SUSPENDED:
  case KTASK_STATE_WAITING:
  case KTASK_STATE_TERMINATED:
   ktask_unschedulecurrent(self);
   self->t_cpu = NULL;
   katomic_store(self->t_state,self->t_newstate);
   if (self->t_newstate != KTASK_STATE_TERMINATED) {
    self->t_newstate = KTASK_STATE_RUNNING;
   } else {
    ktask_onterminate(self);
   }
   ktask_unlock(self,KTASK_LOCK_STATE);
   ktask_selectcurr_anddecref_ni(self);
   {
    NOIRQ_BREAK
    return;
   }
  default: __compiler_unreachable();
 }
 NOIRQ_ENDUNLOCK(ktask_unlock(self,KTASK_LOCK_STATE));
}

#define KTASK_RESCHEDULE_HINT_MASK \
 (KTASK_RESCHEDULE_HINT_CURRENT\
 |KTASK_RESCHEDULE_HINT_NEXT\
 |KTASK_RESCHEDULE_HINT_LAST)

#ifndef KCONFIG_HAVE_SINGLECORE
kerrno_t __SCHED_CALL
ktask_reschedule_ex(struct ktask *__restrict self,
                    struct kcpu *__restrict newcpu,
                    int hint)
#else /* !KCONFIG_HAVE_SINGLECORE */
kerrno_t __SCHED_CALL
ktask_reschedule_ex_impl(struct ktask *__restrict self, int hint)
#define newcpu   kcpu_zero()
#endif /* KCONFIG_HAVE_SINGLECORE */
{
 kerrno_t error;
 ktask_lock_t unlock_locks;
#if KTASK_RESCHEDULE_HINTFLAG_UNLOCKSIGVAL == KTASK_LOCK_SIGVAL
 unlock_locks = KTASK_LOCK_STATE|(hint&KTASK_RESCHEDULE_HINTFLAG_UNLOCKSIGVAL);
#else
 unlock_locks = KTASK_LOCK_STATE|((hint&KTASK_RESCHEDULE_HINTFLAG_UNLOCKSIGVAL)
                                  ? KTASK_LOCK_SIGVAL : 0);
#endif
 NOIRQ_BEGINLOCK(ktask_trylock(self,KTASK_LOCK_STATE));
 if (!(hint&KTASK_RESCHEDULE_HINTFLAG_INHERITREF)) {
  error = ktask_incref(self);
  if __unlikely(KE_ISERR(error)) goto end;
 }
 switch (self->t_state) {
  case KTASK_STATE_SUSPENDED:
   if (hint&KTASK_RESCHEDULE_HINTFLAG_UNDOSUSPEND &&
       self->t_newstate != KTASK_STATE_SUSPENDED) {
    assertf(self != ktask_self(),
            "You can't undo your own suspension. "
            "How does that even work?");
    /* Undo the suspended task state (switch to t_newstate). */
    if (self->t_newstate == KTASK_STATE_RUNNING) break;
    if (self->t_newstate == KTASK_STATE_TERMINATED &&
       (self->t_flags&KTASK_FLAG_CRITICAL)) {
     /* Special case: We can't terminate this task now. - It's critical.
      *  #1: task1 enters critical block
      *  #2: task1 gets suspended
      *  #3: task2 terminates task1 (gets KS_BLOCKING error due to critical block)
      *  #4: task1 gets resumed <<-- This is us
      * >> Instead, we have to re-schedule it as running, but
      *    keep the 't_newstate' of terminated intact. */
     goto do_resched_running;
    }
    katomic_store(self->t_state,self->t_newstate);
    self->t_newstate = KTASK_STATE_RUNNING;
    error = KE_OK;
    switch (self->t_state) {
     case KTASK_STATE_TERMINATED:
      assert(!(self->t_flags&KTASK_FLAG_CRITICAL));
      /* Task was terminated while suspended. */
      self->t_newstate = KTASK_STATE_TERMINATED;
      ktask_onterminateother(self);
      goto wasterm;
     { struct timespec now;
     case KTASK_STATE_WAITINGTMO:
      ktime_getnoworcpu(&now);
      if (__timespec_cmpge(&now,&self->t_abstime)) {
       self->t_flags |= KTASK_FLAG_TIMEDOUT;
      } else {
       /* Inherit the reference here. */
       ktask_scheduletimeout(self);
       goto end;
      }
     } break;
     default: break;
    }
    goto end_decref;
   } else {
    /* Change the task's new state after suspension ends. */
    if (KTASK_STATE_ISWAITING(self->t_newstate)) {
     /* Unschedule all pending signals.
      * >> This is called if a suspended task receives a signal. */
     ktask_unscheduleallsignals(self);
    }
    self->t_newstate = KTASK_STATE_RUNNING;
    error = KS_BLOCKING;
    goto end;
   }
   break;
  case KTASK_STATE_TERMINATED: wasterm: error = KE_DESTROYED; goto end_decref;
  case KTASK_STATE_RUNNING:    error = KS_UNCHANGED; goto end_decref;
  case KTASK_STATE_WAITINGTMO:
   ktask_unscheduletimeout(self); /* Fallthrough. */
   ktask_decref(self); /* TODO: Use this reference instead of creating a new one above. */
  case KTASK_STATE_WAITING:
   ktask_unscheduleallsignals(self);
   /* Must change state to running if we were previously waiting:
    *  #1: task1 enters critical block
    *  #2: task1 starts receiving a signal1 (switches to wait state)
    *  #3: task2 terminates task1 (gets KS_BLOCKING error due to critical block)
    *  #4: task2 sends signal1 <<-- This is us
    *  >> t_newstate == KTASK_STATE_TERMINATED
    *  >> t_state == KTASK_STATE_WAITING/KTASK_STATE_WAITINGTMO. */
do_resched_running:
   katomic_store(self->t_state,KTASK_STATE_RUNNING);
   goto do_resched;
   break;
  default: break;
 }
 katomic_store(self->t_state,self->t_newstate);
do_resched:
#ifndef KCONFIG_HAVE_SINGLECORE
 if __unlikely(!newcpu) {
  assertf(!karch_irq_enabled(),
          "Without interrupts, kcpu_self() isn't volatile");
  newcpu = kcpu_self();
 }
#endif /* !KCONFIG_HAVE_SINGLECORE */
 kcpu_lock(newcpu,KCPU_LOCK_TASKS);
 self->t_cpu = newcpu; // Set the new CPU
 switch (hint&KTASK_RESCHEDULE_HINT_MASK) {
  case KTASK_RESCHEDULE_HINT_CURRENT:
#ifndef KCONFIG_HAVE_SINGLECORE
   if (newcpu == kcpu_self())
#endif /* !KCONFIG_HAVE_SINGLECORE */
   {
    struct ktask *caller = newcpu->c_current;
    if __unlikely(KE_ISERR(ktask_incref(caller))) {
     /* If we can't get a reference, we can still
      * manage to schedule it as the next task! */
     goto schedule_next;
    }
    /* Directly switch to the new task. */
    if (newcpu->c_current) {
     self->t_prev = newcpu->c_current->t_prev;
     self->t_next = newcpu->c_current;
     self->t_prev->t_next = self;
     self->t_next->t_prev = self;
    } else {
     self->t_prev = self;
     self->t_next = self;
    }
    newcpu->c_current = self;
    ktask_unlock(self,unlock_locks);
#if (defined(KTASK_ONRESCHEDULE_NEEDTM)+\
     defined(KTASK_ONENTERQUANTUM_NEEDTM)+\
     defined(KTASK_ONLEAVEQUANTUM_NEEDTM)) > 1
    {
     struct timespec tmnow;
     ktime_getnoworcpu(&tmnow);
     KTASK_ONRESCHEDULE_TM(self,newcpu,&tmnow);
     KTASK_ONLEAVEQUANTUM_TM(caller,newcpu,&tmnow);
     KTASK_ONENTERQUANTUM_TM(self,newcpu,&tmnow);
    }
#else
    KTASK_ONRESCHEDULE(self,newcpu);
    KTASK_ONENTERQUANTUM(self,newcpu);
    KTASK_ONLEAVEQUANTUM(caller,newcpu);
#endif
    kcpu_unlock(newcpu,KCPU_LOCK_TASKS);
    ktask_switchdecref(self,caller);
    {
     NOIRQ_BREAK
     return KE_OK;
    }
   }
   /* Fallthrough. */
  case KTASK_RESCHEDULE_HINT_NEXT:
schedule_next:
   /* Schedule as next task. */
   if (newcpu->c_current) {
    self->t_prev = newcpu->c_current;
    self->t_next = newcpu->c_current->t_next;
    self->t_prev->t_next = self;
    self->t_next->t_prev = self;
   } else {
reschedule_first:
    self->t_prev = self;
    self->t_next = self;
    newcpu->c_current = self;
   }
   break;
  case KTASK_RESCHEDULE_HINT_LAST:
   /* Schedule as last task. */
   if (newcpu->c_current) {
    self->t_next = newcpu->c_current;
    self->t_prev = newcpu->c_current->t_prev;
    self->t_next->t_prev = self;
    self->t_prev->t_next = self;
   } else {
    goto reschedule_first;
   }
   break;
  default: __compiler_unreachable();
 }
/*endok:*/
 error = KE_OK;
 KTASK_ONRESCHEDULE(self,newcpu);
 kcpu_unlock(newcpu,KCPU_LOCK_TASKS);
end:
 {
  NOIRQ_BREAKUNLOCK(ktask_unlock(self,unlock_locks));
  return error;
 }
end_decref:
 ktask_unlock(self,unlock_locks);
 ktask_decref(self);
 NOIRQ_END
 return error;
#ifdef KCONFIG_HAVE_SINGLECORE
#undef newcpu
#endif /* !KCONFIG_HAVE_SINGLECORE */
}

/* ============================== */
/* END OF SCHEDULE-IMPLEMENTATION */
/* ============================== */


#ifndef __INTELLISENSE__
#undef ktask_yield
void ktask_yield(void) { (void)ktask_tryyield(); }
#endif

#undef ktask_tryyield
kerrno_t ktask_tryyield(void) {
 kerrno_t error; struct ktask *oldtask;
 struct kcpu *cpuself;
 NOIRQ_BEGINLOCK_VOLATILE(cpuself,kcpu_self(),
                          kcpu_trylock(cpuself,KCPU_LOCK_TASKS),
                          kcpu_unlock(cpuself,KCPU_LOCK_TASKS))
#if KCPU_LOCK_SLEEP != KCPU_LOCK_TASKS
 if (kcpu_trylock(cpuself,KCPU_LOCK_SLEEP))
#endif
 {
  kcpu_ticknow_unlocked2(cpuself);
#if KCPU_LOCK_SLEEP != KCPU_LOCK_TASKS
  kcpu_unlock(cpuself,KCPU_LOCK_SLEEP);
#endif
 }
 oldtask = cpuself->c_current;
 if __unlikely(KE_ISERR(error = ktask_incref(oldtask))) goto end;
 error = kcpu_rotate_unlocked(cpuself);
 assertf(error != KS_EMPTY,
         "If no tasks are scheduled, and you're calling this, "
         "you must also own a reference to your task, meaning "
         "that in order to not get a reference leak, you have "
         "'to call ktask_selectnext_anddecref_ni'");
 kassert_ktask(oldtask);
 if __likely(error != KS_UNCHANGED) {
  kcpu_unlock(cpuself,KCPU_LOCK_TASKS);
#if defined(KTASK_ONLEAVEQUANTUM_NEEDTM) &&\
    defined(KTASK_ONENTERQUANTUM_NEEDTM)
  {
   struct timespec tmnow;
   ktime_getnoworcpu(&tmnow);
   KTASK_ONLEAVEQUANTUM_TM(oldtask,cpuself,&tmnow);
   KTASK_ONENTERQUANTUM_TM(cpuself->c_current,cpuself,&tmnow);
  }
#else
  KTASK_ONLEAVEQUANTUM(oldtask,cpuself);
  KTASK_ONENTERQUANTUM(cpuself->c_current,cpuself);
#endif
  ktask_switchdecref(cpuself->c_current,oldtask);
  /* Disable interrupts after returning
   * from a yield that re-enabled them. */
  {
   NOIRQ_BREAK
   return error;
  }
 } else {
  ktask_decref(oldtask);
 }
end:
 NOIRQ_ENDUNLOCK(kcpu_unlock(cpuself,KCPU_LOCK_TASKS))
 return error;
}



kerrno_t
ktask_setalarm_k(struct ktask *__restrict self,
                 struct timespec const *__restrict abstime,
                 struct timespec *__restrict oldabstime) {
 struct kcpu *timeoutcpu; kerrno_t error;
 kassert_ktask(self);
 kassertobjnull(abstime);
 kassertobjnull(oldabstime);
 NOIRQ_BEGINLOCK(ktask_trylock(self,KTASK_LOCK_STATE));
 switch (self->t_state) {
  case KTASK_STATE_TERMINATED: error = KE_DESTROYED; break;
  case KTASK_STATE_WAITING:
   if (abstime) {
    assert(!(self->t_flags&KTASK_FLAG_ALARMED));
    assert(!self->t_cpu);
    /* Run a waiting task into a timeout-waiting one
     * NOTE: For simplicity, we just use the
     *       least-load CPU for timeout scheduling. */
    timeoutcpu = kcpu_leastload();
    kassert_kcpu(timeoutcpu);
    self->t_cpu = timeoutcpu;
    self->t_abstime = *abstime;
    self->t_state = KTASK_STATE_WAITINGTMO;
    ktask_scheduletimeout(self);
   }
   error = KE_OK;
   break;
  case KTASK_STATE_WAITINGTMO:
   assert(!(self->t_flags&KTASK_FLAG_ALARMED));
   assert(self->t_cpu);
   if (oldabstime) *oldabstime = self->t_abstime;
   ktask_unscheduletimeout(self);
   if (abstime) {
    /* Set a new timeout. */
    self->t_abstime = *abstime;
    ktask_scheduletimeout(self);
   } else {
    /* Delete a set timeout. */
    self->t_state = KTASK_STATE_WAITING;
   }
   error = KS_UNCHANGED;
   break;
  default:
   /* suspended or running. */
   if (self->t_flags&KTASK_FLAG_ALARMED) {
    /* Alarm was already set. */
    if (oldabstime) *oldabstime = self->t_abstime;
    error = KS_UNCHANGED;
    if (abstime) self->t_abstime = *abstime;
    else self->t_flags &= ~(KTASK_FLAG_ALARMED);
   } else {
    if (abstime) {
     self->t_flags |= KTASK_FLAG_ALARMED;
     self->t_abstime = *abstime;
    }
    error = KE_OK;
   }
   break;
 }
 NOIRQ_ENDUNLOCK(ktask_unlock(self,KTASK_LOCK_STATE));
 return error;
}

kerrno_t ktask_getalarm_k(struct ktask const *__restrict self,
                          struct timespec *__restrict abstime) {
 kerrno_t error;
 kassert_ktask(self);
 kassertobj(abstime);
 NOIRQ_BEGINLOCK(ktask_trylock((struct ktask *)self,KTASK_LOCK_STATE));
 switch (self->t_state) {
  case KTASK_STATE_TERMINATED: error = KE_DESTROYED; break;
  case KTASK_STATE_WAITING:    error = KS_BLOCKING; break;
  case KTASK_STATE_WAITINGTMO: eok: *abstime = self->t_abstime; error = KE_OK; break;
  default:
   if (self->t_flags&KTASK_FLAG_ALARMED) goto eok;
   error = KS_EMPTY;
   break;
 }
 NOIRQ_ENDUNLOCK(ktask_unlock((struct ktask *)self,KTASK_LOCK_STATE));
 return error;
}



int ktask_ischildof(struct ktask const *self,
                    struct ktask const *parent) {
 struct ktask *next;
 /* kassert_ktask(parent); Not necessarily! */
 for (;;) {
  kassert_ktask(self);
  next = ktask_parent_k(self);
  if (next == parent) return 1;
  if (next == self) break;
  self = next;
 }
 return 0;
}

__DECL_END

#ifndef __INTELLISENSE__
#include "task-attr.c.inl"
#include "task-recursive.c.inl"
#include "task-sigchain.c.inl"
#include "task-syscall.c.inl"
#include "task-usercheck.c.inl"
#include "task-util.c.inl"
#include "tls.c.inl"

__DECL_BEGIN
#undef ktask_iscrit
#undef ktask_isnointr
int ktask_iscrit(void) { return KTASK_ISCRIT; }
int ktask_isnointr(void) { return KTASK_ISNOINTR; }

__DECL_END
#endif

#endif /* !__KOS_KERNEL_SCHED_TASK_C__ */

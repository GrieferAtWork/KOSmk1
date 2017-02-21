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
#ifndef __KOS_KERNEL_TASK_H__
#define __KOS_KERNEL_TASK_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/attr.h>
#include <kos/compiler.h>
#include <kos/kernel/features.h>
#include <kos/kernel/object.h>
#include <kos/kernel/paging.h>
#include <kos/kernel/sched_yield.h>
#include <kos/kernel/tls.h>
#include <kos/kernel/tss.h>
#include <kos/kernel/types.h>
#include <kos/kernel/interrupts.h>
#include <kos/timespec.h>
#include <limits.h>
#if KCONFIG_HAVE_TASK_CRITICAL
#include <kos/kernel/region.h>
#endif /* KCONFIG_HAVE_TASK_CRITICAL */
#if KCONFIG_HAVE_TASK_STATS
#ifndef __INTELLISENSE__
#include <kos/kernel/time.h>
#endif
#endif /* KCONFIG_HAVE_TASK_STATS */
#ifndef __KOS_KERNEL_SIGNAL_H__
#include <kos/kernel/signal.h>
#endif /* !__KOS_KERNEL_SIGNAL_H__ */
#ifdef __ASSEMBLY__
#include <kos/symbol.h>
#endif /* __ASSEMBLY__ */

__DECL_BEGIN

#if defined(__i386__)
/* Try to reduce stack overhead during scheduler calls. */
#define __SCHED_CALL __fastcall
#else
#define __SCHED_CALL
#endif

#define KOBJECT_MAGIC_CPU      0xC9D      // CPU
#define KOBJECT_MAGIC_TASK     0x7A5F     // TASK
#define KOBJECT_MAGIC_TASKLIST 0x7A5F7157 // TASKLIST

#define KTASK_STACK_SIZE_DEF   0x4000

#ifndef __ASSEMBLY__
struct ktask;
struct kcpu;
struct kproc;

#define kassert_kcpu(self)      kassert_object(self,KOBJECT_MAGIC_CPU)
#define kassert_ktask(self)     kassert_refobject(self,t_refcnt,KOBJECT_MAGIC_TASK)
#define kassert_ktasklist(self) kassert_object(self,KOBJECT_MAGIC_TASKLIST)
#endif /* !__ASSEMBLY__ */


#define KCPU_OFFSETOF_CURRENT   (KOBJECT_SIZEOFHEAD+4)
#define KCPU_OFFSETOF_SLEEPING  (KOBJECT_SIZEOFHEAD+4+__SIZEOF_POINTER__)
#define KCPU_OFFSETOF_TSS       (KOBJECT_SIZEOFHEAD+4+__SIZEOF_POINTER__*2)

#ifndef __ASSEMBLY__
struct kcpu {
 KOBJECT_HEAD
#define KCPU_LOCK_TASKS 0x01 /*< Lock for pending tasks.
                                 When used in conjunction with 'KTASK_LOCK_STATE',
                                 this lock must be acquire second. */
#define KCPU_LOCK_SLEEP 0x02 /*< Lock for sleeping tasks.
                                 When used in conjunction with 'KTASK_LOCK_STATE',
                                 this lock must be acquire second. */
 __atomic __u8       c_locks;
#define KCPU_FLAG_NONE  0x00
#define KCPU_FLAG_NOIRQ 0x01 /*< [lock(KCPU_LOCK_TASKS)] Don't perform IRQ switching. */
          __u8       c_flags;
          __u8       c_padding[2];
 // NOTE: The following two describe a linked list of tasks, using 't_prev' and 't_next'.
 //       Note though, that tasks listed in 'c_current' wrap around, forming a
 //       ring buffer, while tasks listed in 'c_sleeping' don't, causing the
 //       list of sleeping tasks to end abruptly.
 // NOTE: References in these lists are exclusive, meaning
 //       that a task can only be referenced by through this.
 __ref struct ktask *c_current;  /*< [0..1][lock(KCPU_LOCK_TASKS)] Task scheduled to switch to next. */
 __ref struct ktask *c_sleeping; /*< [0..1][lock(KCPU_LOCK_SLEEP)] Task who's abstime will pass next. */
 struct ktss         c_tss;      /*< [lock(kcpu_self())] Per-CPU task state segment (required for syscalls). */
};


#ifdef __INTELLISENSE__
extern bool kcpu_islocked(struct kcpu *self, __u8 lock);
extern bool kcpu_trylock(struct kcpu *self, __u8 lock);
extern void kcpu_lock(struct kcpu *self, __u8 lock);
extern void kcpu_unlock(struct kcpu *self, __u8 lock);
#else
#define kcpu_islocked(self,lock)  ((katomic_load((self)->c_locks)&(lock))!=0)
#define kcpu_trylock(self,lock)   ((katomic_fetchor((self)->c_locks,lock)&(lock))==0)
#define kcpu_lock(self,lock)      KTASK_SPIN(kcpu_trylock(self,lock))
#define kcpu_unlock(self,lock)    assertef((katomic_fetchand((self)->c_locks,~(lock))&(lock))!=0,"Lock not held")
#endif

#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Returns the control structure associated with the current CPU
extern __wunused __retnonnull struct kcpu *kcpu_self(void);

//////////////////////////////////////////////////////////////////////////
// Returns the control structure associated with the CPU
// current managing the least amount of load (subjectively)
extern __wunused __retnonnull struct kcpu *kcpu_leastload(void);
#elif defined(KCONFIG_HAVE_SINGLECORE)
#define kcpu_self        kcpu_zero
#define kcpu_leastload   kcpu_zero
#else
extern __wunused __retnonnull struct kcpu *kcpu_self(void);
extern __wunused __retnonnull struct kcpu *kcpu_leastload(void);
#endif

//////////////////////////////////////////////////////////////////////////
// Returns the amount of active/sleeping tasks on a given CPU
extern __wunused __size_t kcpu_taskcount(struct kcpu const *__restrict self);
extern __wunused __size_t kcpu_sleepcount(struct kcpu const *__restrict self);

//////////////////////////////////////////////////////////////////////////
// Returns TRUE(1) if system-wide, only one task exists.
// NOTE: Returns FALSE(0) if sleeping tasks exist
extern __wunused int kcpu_global_onetask(void);

#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Returns the control structure describing the original
// CPU initialized by the BIOS and/or Bootloader.
extern __wunused __constcall __retnonnull struct kcpu *kcpu_zero(void);
#else
extern struct kcpu  __kcpu_zero;
#define kcpu_zero() (&__kcpu_zero)
#endif

// NOTE: 'unlocked2' expects the caller to hold 'KCPU_LOCK_TASKS' and 'KCPU_LOCK_SLEEP',
//       whereas 'unlocked' only expects them to be holding 'KCPU_LOCK_SLEEP'
// WARNING: 'unlocked' may temporarily release the 'KCPU_LOCK_SLEEP' lock.
extern __nonnull((1,2)) void kcpu_ticktime_unlocked2(struct kcpu *__restrict self, struct timespec const *__restrict now);
extern __nonnull((1)) void kcpu_ticknow_unlocked2(struct kcpu *__restrict self);
#if KCPU_LOCK_SLEEP != KCPU_LOCK_TASKS
extern __nonnull((1,2)) void kcpu_ticktime_unlocked(struct kcpu *__restrict self, struct timespec const *__restrict now);
extern __nonnull((1)) void kcpu_ticknow_unlocked(struct kcpu *__restrict self);
#else
#define kcpu_ticktime_unlocked kcpu_ticktime_unlocked2
#define kcpu_ticknow_unlocked  kcpu_ticknow_unlocked2
#endif


extern __nonnull((1,2)) void
kcpu_schedulesleep_unlocked(struct kcpu *__restrict self,
                            struct ktask *__restrict task);
extern __nonnull((1,2)) void
kcpu_unschedulesleep_unlocked(struct kcpu *__restrict self,
                              struct ktask *__restrict task);
#endif /* !__ASSEMBLY__ */




#define KTASKREGISTERS_OFFSETOF_DS       (0)
#if KCONFIG_HAVE_I386_SAVE_SEGMENT_REGISTERS
#define KTASKREGISTERS_OFFSETOF_ES       (2)
#define KTASKREGISTERS_OFFSETOF_FS       (4)
#define KTASKREGISTERS_OFFSETOF_GS       (6)
#define KTASKREGISTERS_OFFSETOF_EDI      (8)
#else
#define KTASKREGISTERS_OFFSETOF_EDI      (4)
#endif
#define KTASKREGISTERS_OFFSETOF_ESI      (KTASKREGISTERS_OFFSETOF_EDI+__SIZEOF_POINTER__)
#define KTASKREGISTERS_OFFSETOF_EBP      (KTASKREGISTERS_OFFSETOF_EDI+__SIZEOF_POINTER__*2)
#define KTASKREGISTERS_OFFSETOF_EBX      (KTASKREGISTERS_OFFSETOF_EDI+__SIZEOF_POINTER__*3)
#define KTASKREGISTERS_OFFSETOF_EDX      (KTASKREGISTERS_OFFSETOF_EDI+__SIZEOF_POINTER__*4)
#define KTASKREGISTERS_OFFSETOF_ECX      (KTASKREGISTERS_OFFSETOF_EDI+__SIZEOF_POINTER__*5)
#define KTASKREGISTERS_OFFSETOF_EAX      (KTASKREGISTERS_OFFSETOF_EDI+__SIZEOF_POINTER__*6)
#define KTASKREGISTERS_OFFSETOF_EIP      (KTASKREGISTERS_OFFSETOF_EDI+__SIZEOF_POINTER__*7)
#define KTASKREGISTERS_OFFSETOF_CS       (KTASKREGISTERS_OFFSETOF_EDI+__SIZEOF_POINTER__*8)
#define KTASKREGISTERS_OFFSETOF_EFLAGS   (KTASKREGISTERS_OFFSETOF_EDI+__SIZEOF_POINTER__*9)
#define KTASKREGISTERS3_OFFSETOF_USERESP (KTASKREGISTERS_OFFSETOF_EDI+__SIZEOF_POINTER__*10)
#define KTASKREGISTERS3_OFFSETOF_SS      (KTASKREGISTERS_OFFSETOF_EDI+__SIZEOF_POINTER__*11)

#ifndef __ASSEMBLY__
__COMPILER_PACK_PUSH(1)
struct __packed ktaskregisters {
 // The structure located at the top of any suspended/pending task
 // >> After creating a new task using one of the 'ktask_new*' functions,
 //    one instance of this structure must be pushed on the task's stack
 //    using the 'ktask_stackpush_sp_unlocked' function before it may be launched by
 //    calling 'ktask_resume_k'.
#if KCONFIG_HAVE_I386_SAVE_SEGMENT_REGISTERS
 __u16 ds,es,fs,gs;
#else
 __u16 ds,__padding;
#endif
 __uintptr_t edi,esi,ebp,ebx,edx,ecx,eax;
 // NOTE: The layout of everything below this point
 //       is defined by the CPU (s.a.: 'iret')
union __packed {
 __uintptr_t eip;
 void       *returnaddr;
 void      (*main)();
 void     *(*callback)(void *);
};
 __uintptr_t cs,eflags;
};
struct __packed ktaskregisters3 {
 struct ktaskregisters base;
 // ring-3 only:
 __uintptr_t useresp,ss;
};
__COMPILER_PACK_POP
#endif /* !__ASSEMBLY__ */


#define KTASKSIGSLOT_SIZEOF (4*__SIZEOF_POINTER__)
#ifndef __ASSEMBLY__
struct ktasksig;
struct ktasksigslot {
 struct ktasksigslot *tss_prev; /*< [0..1][lock(tss_sig:KSIGNAL_LOCK_WAIT)] Previous task in this signal chain. */
 struct ktasksigslot *tss_next; /*< [0..1][lock(tss_sig:KSIGNAL_LOCK_WAIT)] Next task in this signal chain. */
 struct ksignal      *tss_sig;  /*< [0..1] Signal associated with this chain (NULL indicates unused slot). */
 __ref struct ktask  *tss_self; /*< [1..1][const] The task associated with this slot (needed for dynamic sub-referencing).
                                     NOTE: This one is only a reference when 'tss_sig != NULL'. */
};
#define KTASKSIGSLOT_INIT(task)  {NULL,NULL,NULL,task}
#endif /* !__ASSEMBLY__ */

#define KTASK_SIGBUFSIZE 2
#define KTASKSIG_SIZEOF  (__SIZEOF_POINTER__+KTASK_SIGBUFSIZE*KTASKSIGSLOT_SIZEOF)

#ifndef __ASSEMBLY__
struct ktasksig {
 // NOTE: When both are needed, 'KTASK_LOCK_SIGCHAIN' must be locked before 'KSIGNAL_LOCK_WAIT'
 struct ktasksig    *ts_next; /*< [0..1][owned] Next set of signal slots. */
 struct ktasksigslot ts_sigs[KTASK_SIGBUFSIZE];
};
#define ktasksig_taskself(self) ((self)->ts_sigs->tss_self)
#define KTASKSIG_INIT(task) {NULL,{KTASKSIGSLOT_INIT(task),KTASKSIGSLOT_INIT(task)}}
extern __nonnull((1,2)) void ktasksig_init(struct ktasksig *__restrict self,
                                           struct ktask *__restrict task);

//////////////////////////////////////////////////////////////////////////
// Clear all signal chain slots, safely removing the
// associated task from all signals it was waiting on before.
// WARNING: This function should only be called on the root signal chain node.
extern __nonnull((1)) void ktasksig_clear(struct ktasksig *__restrict self);

//////////////////////////////////////////////////////////////////////////
// Deletes a given slot from the signal chain.
// WARNING: The associated signal is not notified of this, potentially
//          creating a dangling pointer of a signal slot.
// >> The caller must unlink tss_prev/tss_next and set tss_sig to NULL (BEFOREHAND!)
// NOTE: 'tss_sig == NULL' is even asserted by this function...
extern __nonnull((1,2)) void
ktasksig_delslot(struct ktasksig *__restrict self,
                 struct ktasksigslot *__restrict slot);
extern __wunused __nonnull((1)) __malloccall struct ktasksigslot *
ktasksig_newslot(struct ktasksig *__restrict self);

//////////////////////////////////////////////////////////////////////////
// Adds a given signal to the signal chain.
// NOTE: The caller must be holding a lock to the signal.
// @return: KE_OK:       The signal was successfully chained.
// @return: KE_NOMEM:    Not enough available memory.
// @return: KE_OVERFLOW: Failed to create a new reference to the associated task
extern __wunused __nonnull((1,2)) kerrno_t
ktasksig_addsig_locked(struct ktasksig *__restrict self,
                       struct ksignal *__restrict sig);
#ifdef __INTELLISENSE__
extern __wunused __nonnull((1)) kerrno_t
ktasksig_addsigs_andunlock(struct ktasksig *__restrict self, __size_t sigc,
                           struct ksignal *const *__restrict sigv);
#else
#define ktasksig_addsigs_andunlock(self,sigc,sigv) \
 __xblock({ kerrno_t __error = KE_OK;\
            struct ktasksig *const __self = (self);\
            struct ksignal *const *__iter,*const *__end;\
            __end = (__iter = (sigv))+(sigc);\
            for (; __iter != __end; ++__iter) {\
             if __likely(KE_ISOK(__error)) __error = ktasksig_addsig_locked(__self,*__iter);\
             ksignal_unlock_c(*__iter,KSIGNAL_LOCK_WAIT);\
            }\
            __xreturn __error;\
 })

#endif
#endif /* !__ASSEMBLY__ */


#define KTASKLIST_SIZEOF          (KOBJECT_SIZEOFHEAD+__SIZEOF_SIZE_T__*2+__SIZEOF_POINTER__)
#define KTASKLIST_OFFSETOF_FREEP  (KOBJECT_SIZEOFHEAD)
#define KTASKLIST_OFFSETOF_TASKA  (KOBJECT_SIZEOFHEAD+__SIZEOF_SIZE_T__)
#define KTASKLIST_OFFSETOF_TASKV  (KOBJECT_SIZEOFHEAD+__SIZEOF_SIZE_T__*2)

#ifndef __ASSEMBLY__
struct ktasklist {
 KOBJECT_HEAD // Unreferenced list of tasks.
 __size_t       t_freep; /*< Index hinting at a possible free slot. */
 __size_t       t_taska; /*< Allocated vector size. */
 struct ktask **t_taskv; /*< [0..1][0..t_childa][owned] Vector of tasks. (Index if a task is stored somewhere in said task) */
};
#define KTASKLIST_INIT   {KOBJECT_INIT(KOBJECT_MAGIC_TASKLIST) 0,0,NULL}

#ifdef __INTELLISENSE__
extern void ktasklist_init(struct ktasklist *self);
extern void ktasklist_quit(struct ktasklist *self);
extern struct ktask *__ktasklist_get(struct ktasklist const *self, __size_t i);
extern struct ktask *ktasklist_get(struct ktasklist const *self, __size_t i);
#else
#define ktasklist_init(self) \
 __xblock({ struct ktasklist *const __ktlself = (self);\
            kobject_init(__ktlself,KOBJECT_MAGIC_TASKLIST);\
            __ktlself->t_freep = 0;\
            __ktlself->t_taska = 0;\
            __ktlself->t_taskv = NULL;\
            (void)0;\
 })
#define ktasklist_quit(self) \
 __xblock({ struct ktasklist *const __ktlself = (self);\
            kassert_ktasklist(__ktlself);\
            free(__ktlself->t_taskv);\
            (void)0;\
 })
#define __ktasklist_get(self,i) ((i) >= (self)->t_taska ? NULL : (self)->t_taskv[i])
#define ktasklist_get(self,i) \
 __xblock({ struct ktasklist const *const __ktlself = (self);\
            __size_t const __ktli = (i);\
            __xreturn __ktasklist_get(__ktlself,__ktli);\
 })
#endif

//////////////////////////////////////////////////////////////////////////
// Inserts the given task into the given task list.
// @return: (__size_t)-1: Not enough memory
// @return: * : Index of the given task
extern __size_t ktasklist_insert(struct ktasklist *self, struct ktask *task);

//////////////////////////////////////////////////////////////////////////
// Remove a given task by index from the list, returning that task
// @return: NULL: The given index was invalid.
extern struct ktask *ktasklist_erase(struct ktasklist *self, __size_t index);
#endif /* !__ASSEMBLY__ */


#ifndef __ASSEMBLY__
#if KCONFIG_HAVE_TASK_STATS
struct ktaskstat {
#if KCONFIG_HAVE_TASK_STATS_START
#define __KTASKSTAT_INIT_START {0,0},
 struct timespec ts_abstime_start;   /*< Point in time when a task got created. */
#else
#define __KTASKSTAT_INIT_START
#endif
#if KCONFIG_HAVE_TASK_STATS_QUANTUM
#define __KTASKSTAT_INIT_QUANTUM {0,0},{0,0},
 struct timespec ts_reltime_run;     /*< Amount of time the task spent running. */
 struct timespec ts_abstime_quantum_start; /*< Point in time when a task's quantum started. */
#else
#define __KTASKSTAT_INIT_QUANTUM
#endif
#if KCONFIG_HAVE_TASK_STATS_NOSCHED
#define __KTASKSTAT_INIT_NOSCHED {0,0},{0,0},
 struct timespec ts_reltime_nosched; /*< Amount of time the task spent not being scheduled. */
 struct timespec ts_abstime_nosched; /*< Point in time when a suspended task got unscheduled. */
#else
#define __KTASKSTAT_INIT_NOSCHED
#endif
#if KCONFIG_HAVE_TASK_STATS_SLEEP
#define __KTASKSTAT_INIT_SLEEP {0,0},{0,0},
 struct timespec ts_reltime_sleep;   /*< Amount of time the task spent as a sleeper. */
 struct timespec ts_abstime_sleep;   /*< Point in time when a sleeping task started sleeping. */
#else
#define __KTASKSTAT_INIT_SLEEP
#endif
#if KCONFIG_HAVE_TASK_STATS_SWITCHIN
#define __KTASKSTAT_INIT_SWITCHIN 0,
 __u32           ts_irq_switchin;   /*< Amount of this times this task was the winner at the end of an IRQ. */
#else
#define __KTASKSTAT_INIT_SWITCHIN
#endif
#if KCONFIG_HAVE_TASK_STATS_SWITCHOUT
#define __KTASKSTAT_INIT_SWITCHOUT 0,
 __u32           ts_irq_switchout;  /*< Amount of this times this task was switched away from within the IRQ. */
#else
#define __KTASKSTAT_INIT_SWITCHOUT
#endif
 __u32           ts_version;         /*< Stat IRQ number (incremented with every change to the stats below). */
};
#define KTASKSTAT_INIT  \
 {__KTASKSTAT_INIT_START __KTASKSTAT_INIT_QUANTUM\
  __KTASKSTAT_INIT_NOSCHED __KTASKSTAT_INIT_SLEEP\
  __KTASKSTAT_INIT_SWITCHIN __KTASKSTAT_INIT_SWITCHOUT 0}
#ifdef __INTELLISENSE__
extern void ktaskstat_init(struct ktaskstat *self);
#elif KCONFIG_HAVE_TASK_STATS_START
#define ktaskstat_init(self) \
 __xblock({ struct ktaskstat *const __ktsself = (self);\
            memset(__ktsself,0,sizeof(struct ktaskstat));\
            ktime_getnoworcpu(&__ktsself->ts_abstime_start);\
            (void)0;\
 })
#else
#define ktaskstat_init(self) memset(self,0,sizeof(struct ktaskstat))
#endif
#ifdef __INTELLISENSE__
#if KCONFIG_HAVE_TASK_STATS_QUANTUM
extern void ktaskstat_on_enterquantum_tm(struct ktaskstat *self, struct timespec const *tmnow);
extern void ktaskstat_on_leavequantum_tm(struct ktaskstat *self, struct timespec const *tmnow);
extern void ktaskstat_on_enterquantum(struct ktaskstat *self);
extern void ktaskstat_on_leavequantum(struct ktaskstat *self);
#endif
#if KCONFIG_HAVE_TASK_STATS_NOSCHED
extern void ktaskstat_on_unschedule_tm(struct ktaskstat *self, struct timespec const *tmnow);
extern void ktaskstat_on_reschedule_tm(struct ktaskstat *self, struct timespec const *tmnow);
extern void ktaskstat_on_unschedule(struct ktaskstat *self);
extern void ktaskstat_on_reschedule(struct ktaskstat *self);
#endif
#if KCONFIG_HAVE_TASK_STATS_SLEEP
extern void ktaskstat_on_entersleep_tm(struct ktaskstat *self, struct timespec const *tmnow);
extern void ktaskstat_on_leavesleep_tm(struct ktaskstat *self, struct timespec const *tmnow);
extern void ktaskstat_on_entersleep(struct ktaskstat *self);
extern void ktaskstat_on_leavesleep(struct ktaskstat *self);
#endif
#else
#define __ktaskstat_on_entertm(rel,abs) \
 self->abs = *tmnow; ++self->ts_version;
#define __ktaskstat_on_leavetm(rel,abs) \
 struct timespec __mtime = *tmnow;\
 __timespec_sub(&__mtime,&self->abs);\
 __timespec_add(&self->rel,&__mtime);\
 ++self->ts_version;
#if KCONFIG_HAVE_TASK_STATS_QUANTUM
__local void ktaskstat_on_enterquantum_tm(struct ktaskstat *self, struct timespec const *tmnow) { __ktaskstat_on_entertm(ts_reltime_run,ts_abstime_quantum_start) }
__local void ktaskstat_on_leavequantum_tm(struct ktaskstat *self, struct timespec const *tmnow) { __ktaskstat_on_leavetm(ts_reltime_run,ts_abstime_quantum_start) }
__local void ktaskstat_on_enterquantum(struct ktaskstat *self) { struct timespec tmnow; ktime_getnoworcpu(&tmnow); ktaskstat_on_enterquantum_tm(self,&tmnow); }
__local void ktaskstat_on_leavequantum(struct ktaskstat *self) { struct timespec tmnow; ktime_getnoworcpu(&tmnow); ktaskstat_on_leavequantum_tm(self,&tmnow); }
#endif
#if KCONFIG_HAVE_TASK_STATS_NOSCHED
__local void ktaskstat_on_reschedule_tm(struct ktaskstat *self, struct timespec const *tmnow) { __ktaskstat_on_entertm(ts_reltime_nosched,ts_abstime_nosched) }
__local void ktaskstat_on_unschedule_tm(struct ktaskstat *self, struct timespec const *tmnow) { __ktaskstat_on_leavetm(ts_reltime_nosched,ts_abstime_nosched) }
__local void ktaskstat_on_reschedule(struct ktaskstat *self) { struct timespec tmnow; ktime_getnoworcpu(&tmnow); ktaskstat_on_reschedule_tm(self,&tmnow); }
__local void ktaskstat_on_unschedule(struct ktaskstat *self) { struct timespec tmnow; ktime_getnoworcpu(&tmnow); ktaskstat_on_unschedule_tm(self,&tmnow); }
#endif
#if KCONFIG_HAVE_TASK_STATS_SLEEP
__local void ktaskstat_on_entersleep_tm(struct ktaskstat *self, struct timespec const *tmnow) { __ktaskstat_on_entertm(ts_reltime_sleep,ts_abstime_sleep) }
__local void ktaskstat_on_leavesleep_tm(struct ktaskstat *self, struct timespec const *tmnow) { __ktaskstat_on_leavetm(ts_reltime_sleep,ts_abstime_sleep) }
__local void ktaskstat_on_entersleep(struct ktaskstat *self) { struct timespec tmnow; ktime_getnoworcpu(&tmnow); ktaskstat_on_entersleep_tm(self,&tmnow); }
__local void ktaskstat_on_leavesleep(struct ktaskstat *self) { struct timespec tmnow; ktime_getnoworcpu(&tmnow); ktaskstat_on_leavesleep_tm(self,&tmnow); }
#endif
#undef __ktaskstat_on_entertm
#undef __ktaskstat_on_leavetm
#endif
__local void ktaskstat_get(struct ktaskstat const *self, struct ktaskstat *result) {
 do memcpy(result,self,sizeof(struct ktaskstat));
 while (self->ts_version != result->ts_version);
}

#endif /* KCONFIG_HAVE_TASK_STATS */

//////////////////////////////////////////////////////////////////////////
// The point in time when the kernel was booted (boot time)
// >> This can be used as a reference pointer to figure out stuff like uptime.
#if KCONFIG_HAVE_TASK_STATS_START
#define KERNEL_BOOT_TIME  (*(struct timespec const *)&ktask_zero()->t_stats.ts_abstime_start)
#else /* KCONFIG_HAVE_TASK_STATS_START */
extern struct timespec __kernel_boot_time;
#define KERNEL_BOOT_TIME  (*(struct timespec const *)&__kernel_boot_time)
#endif /* !KCONFIG_HAVE_TASK_STATS_START */

#endif /* !__ASSEMBLY__ */


// NOTE: The 'KTASK_PRIORITY_SUSPENDED' priority differs
//       from how 'ktask_suspend_k/ktask_resume_k' operate.
//       Unlike ~actually~ suspending a task, setting
//       its priority to suspended will not unschedule the
//       task, and the return value of ktask_issuspended
//       will remain unaffected.
//    -> The advantage of this is, that a priority-suspended
//       task will remain visible in any kind of process listing,
//       acting as though it was still running.
//    -> The disadvantage is, that this kind of suspension is
//       not recursive, once set doesn't allow reverting to
//       any former value, as well as it not being suitable
//       for suspending a task in order to perform operations
//       such as manipulating its stack.
//       Also note, that since the task will remain scheduled,
//       a minuscule portion of CPU cycles are spend skipping
//       this task during scheduling.
// WARNING: If the only task of any cup has its priority set to
//          'KTASK_PRIORITY_SUSPENDED', it will actually be
//          re-scheduling with whatever code running inside
//          being executed.
#define KTASK_PRIORITY_SUSPENDED (-32767-1)
#define KTASK_PRIORITY_DEF          0
#define KTASK_PRIORITY_REALTIME    32767



#define KTASK_OFFSETOF_ESP         (KOBJECT_SIZEOFHEAD)
#define KTASK_OFFSETOF_PD          (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__)
#define KTASK_OFFSETOF_ESP0        (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__*2)
#define KTASK_OFFSETOF_EPD         (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__*3)
#define KTASK_OFFSETOF_REFCNT      (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__*4)
#define KTASK_OFFSETOF_LOCKS       (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__*4+4)
#define KTASK_OFFSETOF_STATE       (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__*4+5)
#define KTASK_OFFSETOF_NEWSTATE    (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__*4+6)
#define KTASK_OFFSETOF_FLAGS       (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__*4+7)
#define KTASK_OFFSETOF_EXITCODE    (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__*4+8)
#define KTASK_OFFSETOF_CPU         (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__*5+8)
#define KTASK_OFFSETOF_PREV        (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__*6+8)
#define KTASK_OFFSETOF_NEXT        (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__*7+8)
#define KTASK_OFFSETOF_ABSTIME     (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__*8+8)
#define KTASK_OFFSETOF_SUSPENDED   (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__*8+8+__TIMESPEC_SIZEOF)
#define KTASK_OFFSETOF_SETPRIORITY (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__*8+12+__TIMESPEC_SIZEOF)
#define KTASK_OFFSETOF_CURPRIORITY (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__*8+14+__TIMESPEC_SIZEOF)
#define KTASK_OFFSETOF_SIGVAL      (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__*8+16+__TIMESPEC_SIZEOF)
#define KTASK_OFFSETOF_SIGCHAIN    (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__*9+16+__TIMESPEC_SIZEOF)
#define KTASK_OFFSETOF_JOINSIG     (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__*9+16+__TIMESPEC_SIZEOF+KTASKSIG_SIZEOF)
#define KTASK_OFFSETOF_PROC        (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__*9+16+__TIMESPEC_SIZEOF+KTASKSIG_SIZEOF+KSIGNAL_SIZEOF)
#define KTASK_OFFSETOF_PARENT      (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__*10+16+__TIMESPEC_SIZEOF+KTASKSIG_SIZEOF+KSIGNAL_SIZEOF)
#define KTASK_OFFSETOF_PARID       (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__*11+16+__TIMESPEC_SIZEOF+KTASKSIG_SIZEOF+KSIGNAL_SIZEOF)
#define KTASK_OFFSETOF_TID         (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__*11+__SIZEOF_SIZE_T__+16+__TIMESPEC_SIZEOF+KTASKSIG_SIZEOF+KSIGNAL_SIZEOF)
#define KTASK_OFFSETOF_CHILDREN    (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__*11+__SIZEOF_SIZE_T__*2+16+__TIMESPEC_SIZEOF+KTASKSIG_SIZEOF+KSIGNAL_SIZEOF)
#define KTASK_OFFSETOF_USTACKVP    (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__*11+__SIZEOF_SIZE_T__*2+16+__TIMESPEC_SIZEOF+KTASKSIG_SIZEOF+KSIGNAL_SIZEOF+KTASKLIST_SIZEOF)
#define KTASK_OFFSETOF_USTACKSZ    (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__*12+__SIZEOF_SIZE_T__*2+16+__TIMESPEC_SIZEOF+KTASKSIG_SIZEOF+KSIGNAL_SIZEOF+KTASKLIST_SIZEOF)
#define KTASK_OFFSETOF_KSTACKVP    (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__*12+__SIZEOF_SIZE_T__*3+16+__TIMESPEC_SIZEOF+KTASKSIG_SIZEOF+KSIGNAL_SIZEOF+KTASKLIST_SIZEOF)
#define KTASK_OFFSETOF_KSTACK      (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__*13+__SIZEOF_SIZE_T__*3+16+__TIMESPEC_SIZEOF+KTASKSIG_SIZEOF+KSIGNAL_SIZEOF+KTASKLIST_SIZEOF)
#define KTASK_OFFSETOF_KSTACKEND   (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__*14+__SIZEOF_SIZE_T__*3+16+__TIMESPEC_SIZEOF+KTASKSIG_SIZEOF+KSIGNAL_SIZEOF+KTASKLIST_SIZEOF)
#if KCONFIG_HAVE_TASK_NAMES
#define KTASK_OFFSETOF_NAME        (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__*15+__SIZEOF_SIZE_T__*3+16+__TIMESPEC_SIZEOF+KTASKSIG_SIZEOF+KSIGNAL_SIZEOF+KTASKLIST_SIZEOF)
#define KTASK_OFFSETOF_TLS         (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__*16+__SIZEOF_SIZE_T__*3+16+__TIMESPEC_SIZEOF+KTASKSIG_SIZEOF+KSIGNAL_SIZEOF+KTASKLIST_SIZEOF)
#else /* KCONFIG_HAVE_TASK_NAMES */
#define KTASK_OFFSETOF_TLS         (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__*15+__SIZEOF_SIZE_T__*3+16+__TIMESPEC_SIZEOF+KTASKSIG_SIZEOF+KSIGNAL_SIZEOF+KTASKLIST_SIZEOF)
#endif /* !KCONFIG_HAVE_TASK_NAMES */
#if KCONFIG_HAVE_TASK_STATS
#define KTASK_OFFSETOF_STAT        (KTASK_OFFSETOF_TLS+KTLSPT_SIZEOF)
#endif /* KCONFIG_HAVE_TASK_STATS */

#ifndef __ASSEMBLY__
struct ktask {
 KOBJECT_HEAD
 /* Address of the task's stack end.
  *  - read/write exclusive to the running thread.
  *  - [lock(KTASK_LOCK_MEMORY)] in a suspended thread.
  * NOTES
  *  - This is a user-level address
  *  - In a suspended task, this is the address to return to
  *  - 't_userpd' describes the last active page directory of the task,
  *    and usually is either kpagedir_kernel(), or kproc_getpagedir(t_proc).
  *  - 't_esp' must be mapped according to 't_userpd' */
 __user void                 *t_esp;
 struct kpagedir             *t_userpd;
 __user void                 *t_esp0;   /*< [1..1][const] This task's ESP0 mapped pointer.
                                             NOTE: This property has no effect in kernel tasks.
                                             NOTE: This pointer is __always__ mapped with 'kproc_getpagedir(t_proc)' */
 struct kpagedir const       *t_epd;    /*< [1..1][lock(this == ktask_self())] The effective page directory used user-kernel address translation. */
 __u32                        t_refcnt; /*< Reference counter. */
 __u8                         t_locks;  /*< Task locks (Set of 'KTASK_LOCK_*'). */
#define KTASK_LOCK_STATE    0x01
#define KTASK_LOCK_SIGCHAIN 0x02
#define KTASK_LOCK_CHILDREN 0x04
#define KTASK_LOCK_TLS      0x08
#define KTASK_LOCK_SIGVAL   0x10
#ifdef __DEBUG__
#define KTASK_LOCK_RELEASEDATA 0x80 /*< Debug-only safeguard to make sure releasing data is synchronized. */
#endif
 /* NOTE: When the task is not scheduled (t_cpu == NULL), its state must be any of
  *       'KTASK_STATE_SUSPENDED', 'KTASK_STATE_TERMINATED' or 'KTASK_STATE_WAITING'
  *       This condition can quickly be checked using 'KTASK_STATE_HASCPU()'
  *       Note though, that 'KTASK_STATE_WAITINGTMO' does not fall under this category
  *       as it will remain scheduled on a CPU that will be responsible for waking it
  *       at a later point in time. */
 __atomic __u8                t_state;    /*< [lock(KTASK_LOCK_STATE)] State of the task (atomic read). */
#define KTASK_STATE_RUNNING    0x00 /*< Task is currently running. */
#define KTASK_STATE_SUSPENDED  0x01 /*< Task is suspended by use of ktask_suspend_k(), or was created suspended. */
#define KTASK_STATE_TERMINATED 0x03 /*< Task has finished, or was terminated (NOTE: Setting this state may not be reverted). */
#define KTASK_STATE_WAITING    0x11 /*< Task is queued to wake on a locking primitive. */
#define KTASK_STATE_WAITINGTMO 0x10 /*< Task is queued to wake on a locking primitive, or time out after a given amount of time has passed. (NOTE: Task is still scheduled) */
#define KTASK_STATE_ISACTIVE(state)   ((state)==KTASK_STATE_RUNNING)
#define KTASK_STATE_ISWAITING(state) (((state)&0x10)!=0)
#define KTASK_STATE_HASCPU(state)    (((state)&0x01)==0) /*< Task is associated with an explicit CPU */
 __u8                         t_newstate; /*< [lock(KTASK_LOCK_STATE)] New state to switch to at the end of a critical block/after resuming from suspension. */
 __u8                         t_flags;    /*< [const] Task flags (Set of 'KTASK_FLAG_*'). */
#define KTASK_FLAG_NONE       0x00
#define KTASK_FLAG_USERTASK   0x01 /*< [const] This task is running in ring 3. */
#define KTASK_FLAG_OWNSUSTACK 0x02 /*< [const] t_ustackvp is free'd upon task destruction (Requires the 'KTASK_FLAG_USERTASK' flag). */
#define KTASK_FLAG_MALLKSTACK 0x04 /*< [const] The kernel stack is allocated through malloc, instead of mmap (NOTE: Only allowed for kernel tasks; aka. when 'KTASK_FLAG_USERTASK' isn't set). */
#if KCONFIG_HAVE_TASK_CRITICAL
#define KTASK_FLAG_CRITICAL   0x08 /*< [lock(r:KTASK_LOCK_STATE|ktask_self();w:KTASK_LOCK_STATE)] This task is currently running critical code, and may not be terminated directly. */
#endif /* KCONFIG_HAVE_TASK_CRITICAL */
#define KTASK_FLAG_TIMEDOUT   0x10 /*< [lock(KTASK_LOCK_STATE)] The task timed out. */
#define KTASK_FLAG_ALARMED    0x20 /*< [lock(KTASK_LOCK_STATE)] The task may be running, but a 't_abstime' describes a user-defined alarm. */
#if KCONFIG_HAVE_TASK_CRITICAL_INTERRUPT
#define KTASK_FLAG_WASINTR    0x40 /*< [lock(KTASK_LOCK_STATE)] An interrupt signal was send to a critical task (While set, the signal is still pending). */
#define KTASK_FLAG_NOINTR     0x80 /*< [lock(r:KTASK_LOCK_STATE|ktask_self();w:KTASK_LOCK_STATE)] Don't send KE_INTR errors to this task when it gets terminated while inside a critical block. */
#endif /* KCONFIG_HAVE_TASK_CRITICAL_INTERRUPT */
 void                        *t_exitcode; /*< [?..?][lock(KTASK_LOCK_STATE)] Exitcode of the task (non-locked & read-only after termination). NOTE: This must not necessarily be a pointer! */
 struct kcpu                 *t_cpu;      /*< [0..1][lock(KTASK_LOCK_STATE)] CPU Hosting the task (Only non-NULL if the state implies 'KTASK_STATE_HASCPU'). */
 __ref struct ktask          *t_prev;     /*< [?..1][lock(t_cpu:KCPU_LOCK_TASKS|KCPU_LOCK_SLEEP)] Previous sleeping/pending task (Only used if a cpu is present). */
 __ref struct ktask          *t_next;     /*< [?..1][lock(t_cpu:KCPU_LOCK_TASKS|KCPU_LOCK_SLEEP)] Next sleeping/pending task (Only used if a cpu is present). */
 struct timespec              t_abstime;  /*< [lock(KTASK_LOCK_STATE)] Absolute point in time used when the task is in a waiting-tmo state. */
#define KTASK_SUSPEND_MIN INT32_MIN
#define KTASK_SUSPEND_MAX INT32_MAX
 __atomic __s32               t_suspended;/*< Recursive suspension counter. (Must be ZERO(0) if 't_state != KTASK_STATE_SUSPENDED') */
 __atomic ktaskprio_t         t_setpriority; /*< Set task priority level. */
          ktaskprio_t         t_curpriority; /*< [lock(t_cpu:KCPU_LOCK_TASKS)] Current priority level. */
 __kernel void               *t_sigval;   /*< [0..1][running:ktask_self()|*:lock(KTASK_LOCK_SIGVAL)] Pointer to some buffer used during vsignal-transmissions. */
 struct ktasksig              t_sigchain; /*< [lock(KTASK_LOCK_SIGCHAIN)] List of signal chains used by waiting tasks. */
 struct ksignal               t_joinsig;  /*< One-time signal closed (and therefor send) when the task is terminated. - Used for joining of a task. */
 __ref struct kproc          *t_proc;     /*< [1..1][const] Shared context for stuff like file descriptors, et al (aka. process). */
 __ref struct ktask          *t_parent;   /*< [1..1][const] Parent of this task (== self for 'ktask_zero()'). */
 __size_t                     t_parid;    /*< [const] This child's index within its parent task's (t_parent) child vector (t_children). */
 __size_t                     t_tid;      /*< [const] This task's thread-id; aka. its index in the process's task list ('t_proc->p_threads'). */
 struct ktasklist             t_children; /*< [lock(KTASK_LOCK_CHILDREN)] List of this tasks children (Index == child->t_parid). */
 /* NOTE: The stack and its mappings are freed upon termination. */
 __user                 void *t_ustackvp; /*< [0..1][const] Virtual address of the user stack (NULL for kernel tasks). */
 __size_t                     t_ustacksz; /*< [const] Size of the user stack (ZERO(0) for kernel tasks). */
 /* NOTE: The following two must only be page-aligned when 'KTASK_FLAG_MALLKSTACK' isn't set. */
 __user   __pagealigned void *t_kstackvp; /*< [1..1][const] Virtual address of the kernel stack ('tanslated() == t_kstack'; '== t_kstack' for kernel tasks) (NOTE: Potentially unaligned). */
 __kernel __pagealigned void *t_kstack;   /*< [1..1][const][owned(KTASK_FLAG_OWNSKSTACK)] Physical address of the kernel stack (NOTE: Potentially unaligned). */
 __kernel void               *t_kstackend;/*< [1..1][const] Physical address of the kernel stack end. */
#if KCONFIG_HAVE_TASK_NAMES
 __atomic char               *t_name;     /*< [0..1][write_once(!NULL)] Name for this task (May only be set once). */
#endif /* KCONFIG_HAVE_TASK_NAMES */
 struct ktlspt                t_tls;      /*< [lock(KTASK_LOCK_TLS)] Per-task TLS data. */
#if KCONFIG_HAVE_TASK_STATS
 struct ktaskstat             t_stats;    /*< Task statistics. */
#endif /* KCONFIG_HAVE_TASK_STATS */
};

#ifdef __INTELLISENSE__
extern __wunused __nonnull((1)) __u8 ktask_getstate(struct ktask const *__restrict self);
extern __wunused __nonnull((1)) bool ktask_isusertask(struct ktask const *__restrict self);
extern __wunused __nonnull((1)) bool ktask_issuspended(struct ktask const *__restrict self);
extern __wunused __nonnull((1)) bool ktask_isterminated(struct ktask const *__restrict self);
extern __wunused __nonnull((1)) bool ktask_iswaiting(struct ktask const *__restrict self);
extern __wunused __nonnull((1)) bool ktask_isactive(struct ktask const *__restrict self);
extern __wunused __nonnull((1)) bool ktask_islocked(struct ktask const *__restrict self, __u8 lock);
extern __wunused __nonnull((1)) bool ktask_trylock(struct ktask *__restrict self, __u8 lock);
extern           __nonnull((1)) void ktask_lock(struct ktask *__restrict self, __u8 lock);
extern           __nonnull((1)) void ktask_unlock(struct ktask *__restrict self, __u8 lock);
extern __wunused __nonnull((1)) struct kproc *ktask_getproc(struct ktask const *__restrict self);
extern __wunused __nonnull((1)) __size_t ktask_getustacksize(struct ktask const *__restrict self);
extern __wunused __nonnull((1)) __size_t ktask_getkstacksize(struct ktask const *__restrict self);
#else
#define ktask_getstate(self)        katomic_load((self)->t_state)
#define ktask_isusertask(self)   (((self)->t_flags&KTASK_FLAG_USERTASK)!=0)
#define ktask_issuspended(self)    (ktask_getstate(self)==KTASK_STATE_SUSPENDED)
#define ktask_isterminated(self)   (ktask_getstate(self)==KTASK_STATE_TERMINATED)
#define ktask_iswaiting(self)       KTASK_STATE_ISWAITING(ktask_getstate(self))
#define ktask_isactive(self)        KTASK_STATE_ISACTIVE(ktask_getstate(self))
#define ktask_islocked(self,lock) ((katomic_load((self)->t_locks)&(lock))!=0)
#define ktask_trylock(self,lock)  ((katomic_fetchor((self)->t_locks,lock)&(lock))==0)
#define ktask_lock(self,lock)       KTASK_SPIN(ktask_trylock(self,lock))
#define ktask_unlock(self,lock)     assertef((katomic_fetchand((self)->t_locks,~(lock))&(lock))!=0,"Lock not held")
#define ktask_getproc(self)       ((self)->t_proc)
#define ktask_getustacksize(self) ((self)->t_ustacksz)
#define ktask_getkstacksize(self)   __xblock({ struct ktask const *const __ktsself = (self); __xreturn (uintptr_t)__ktsself->t_kstackend-(uintptr_t)__ktsself->t_kstack; })
#endif

#define __ktryassert_ktask(self) kassert_object(self,KOBJECT_MAGIC_TASK)
__local KOBJECT_DEFINE_INCREF(ktask_incref,struct ktask,t_refcnt,kassert_ktask);
__local KOBJECT_DEFINE_TRYINCREF(ktask_tryincref,struct ktask,t_refcnt,__ktryassert_ktask);
__local KOBJECT_DEFINE_DECREF(ktask_decref,struct ktask,t_refcnt,kassert_ktask,ktask_destroy);
#undef __ktryassert_ktask
#endif /* !__ASSEMBLY__ */


// Immediately switch to the scheduled task
//  - Interpreted as 'KTASK_RESCHEDULE_HINT_NEXT' if 'newcpu' isn't the host CPU
//  - Note, that the scheduler may choose to switch CPUs at any point,
//    meaning that calling 'ktask_reschedule(...,kcpu_self(),KTASK_RESCHEDULE_HINT_CURRENT)'
//    does not guaranty that the task will be switched to immediately.
#define KTASK_RESCHEDULE_HINT_CURRENT 0
// Schedule the task to be executed early (after the next context switch)
#define KTASK_RESCHEDULE_HINT_NEXT    1
// Schedule the task to be executed late (after all other tasks have already run)
#define KTASK_RESCHEDULE_HINT_LAST    2
// The default behavior is to schedule the task for execution last,
// as this way usercode that constantly suspends/resumes a task
// doesn't mess to badly with the scheduler by only with data
// relatively far away from what will be executed next.
#define KTASK_RESCHEDULE_HINT_DEFAULT KTASK_RESCHEDULE_HINT_LAST

// === Extended rescheduling flags stored in the hint argument ===
// WARNING: These flags may not be included when sending a signal.

// Undo the state backup generated when suspending a task.
// If this flag isn't set, re-scheduling of 'KTASK_STATE_SUSPENDED' tasks
// is not allowed, and attempting to do so will (kind-of) fail with 'KS_BLOCKING'
// >> This flag is required to handle the (not-really) re-scheduling
//    or a suspended task receiving a signal, such that once it is no
//    longer suspended, it will realize that the signal was send, causing
//    the task to really be re-scheduled.
#define KTASK_RESCHEDULE_HINTFLAG_UNDOSUSPEND 0x1000

// Inherit the given reference
// NOTE: It will be consumed in all cases!
#define KTASK_RESCHEDULE_HINTFLAG_INHERITREF  0x2000

// Unlock 'KTASK_LOCK_SIGVAL' after the task was re-scheduled,
// at the same time the task's 'KTASK_LOCK_STATE' lock is released.
// NOTE: The sigval lock is unlocked in all situations, even
//       when the rescheduling call would have otherwise failed.
// HINT: This flag can be defined as anything, but defining it
//       as 'KTASK_LOCK_SIGVAL' both makes sense and allows
//       for some minor optimizations down the line.
#define KTASK_RESCHEDULE_HINTFLAG_UNLOCKSIGVAL KTASK_LOCK_SIGVAL


#ifndef __ASSEMBLY__
//////////////////////////////////////////////////////////////////////////
// Unschedule a given task, settings its state to one of
//  - KTASK_STATE_SUSPENDED : suspended
//  - KTASK_STATE_TERMINATED: terminated (irreversible)
//  - KTASK_STATE_WAITING   : waiting (signals may be provided)
//  - KTASK_STATE_WAITINGTMO: waiting w/ '(struct timespec *)arg' timeout (signals may be provided)
//////////////////////////////////////////////////////////////////////////
// Unschedules a given task for execution
// >> The task's old state must be that of an active task
// >> The task's state will be updated
// NOTES:
//  - When unscheduling, newstate must not be an active state ('KTASK_STATE_ISACTIVE')
//  - When 'newstate' is 'KTASK_STATE_TERMINATED' a reference will be dropped.
//  - 'ktask_reschedule' will set the task's state to 'KTASK_STATE_RUNNING'
//  - If 'newstate' is 'KTASK_STATE_WAITINGTMO', a timeout must be specified as instance of 'struct timespec' in 'arg'
//  - If 'signal' is non-NULL, the task is added to it and it will be be unlocked after re-scheduling
//    Also note, that in this case, the caller must disable interrupts before locking the signal.
//  - When unscheduling a task for termination, its exitcode is set to the integral
//    value passed through the 'arg' argument (which then has nothing to do with a timeout)
//    >> ktask_unschedule(ktask_self(),KTASK_STATE_TERMINATED,(struct timespec const *)42);
//    >> // Same as:
//    >> ktask_exit((void *)42);
//  - If a waiting task gets suspended and the signal
//    its been waiting for get sent, the task must
//    still be notified of the send signal, allowing
//    it to resume execution once 'ktask_resume_k' is called
//////////////////////////////////////////////////////////////////////////
// @return: KE_DESTROYED: The task, or signal has terminated.
// @return: KE_TIMEDOUT:  'self == ktask_self()', 'newstate == KTASK_STATE_WAITINGTMO',
//                        'arg != NULL': The timeout has passed without the task
//                        being re-scheduled for execution by another task.
//                        NOTE: In this situation, the signal will have been re-locked to remove the given task.
//                        NOTE: If an alarm is set, this error may also be returned if
//                              the given 'newstate' was simply 'KTASK_STATE_WAITING'
// @return: KS_UNCHANGED: The task's state doesn't allow the operation
//                        NOTE: This value is never returned for 'ktask_self()'
//                        NOTE: Due to race-conditions, this should not be considered an error
// @return: KE_OVERFLOW:  Failed to acquire a new reference to 'self' (needed for timeouts)
//#if KCONFIG_HAVE_TASK_CRITICAL_INTERRUPT
// @return: KE_INTR:      After a waiting/suspended unscheduling, a critical
//                        task was re-awakened due to an attempt at termination.
//                        >> Not returned if the task is within a task-level nointr block
//                        Only returned once, but can be reset.
//#endif /* KCONFIG_HAVE_TASK_CRITICAL_INTERRUPT */
#define ktask_unschedule(self,newstate,arg) ktask_unschedule_ex(self,newstate,arg,0,NULL)
extern __wunused __nonnull((1)) kerrno_t __SCHED_CALL
ktask_unschedule_ex(struct ktask *__restrict self, __u8 newstate, void *__restrict arg,
                    __size_t sigc, struct ksignal *const *__restrict sigv);
extern __nonnull((1)) void __SCHED_CALL ktask_unschedule_aftercrit(struct ktask *__restrict self);

//////////////////////////////////////////////////////////////////////////
// Re-schedules a given task for execution
// >> The task's old state must be the opposite of it's new state
// >> The task's state will be updated
// If 'newcpu' is NULL, it will always refer to the current CPU.
//////////////////////////////////////////////////////////////////////////
// @return: KE_DESTROYED: The task has been terminated
// @return: KE_OVERFLOW:  There are too many references to the given task
//                        NOTE: Never returned if the 'KTASK_RESCHEDULE_HINTFLAG_INHERITREF' flag is set
// @return: KS_UNCHANGED: The task has already been re-scheduled
//                       (Due to race-conditions, this should not be considered an error)
// @return: KS_BLOCKING:  #1 The 'KTASK_RESCHEDULE_HINTFLAG_UNDOSUSPEND' flag is set, and
//                           the task cannot be re-scheduled because it was unscheduled
//                           for a reason other than suspension.
//                           NOTE: In this situation, the old task state is still restored,
//                                 including the point in time when the task may have been
//                                 scheduled to time out from a potential signal.
//                              >> The abstime does technically continue to
//                                 run, even when the task is suspended.
//                        #2 The 'KTASK_RESCHEDULE_HINTFLAG_UNDOSUSPEND' flag is not set, and
//                           the task cannot be re-scheduled because it is currently marked
//                           as suspended.
#ifndef KCONFIG_HAVE_SINGLECORE
extern __wunused __nonnull((1)) kerrno_t __SCHED_CALL
ktask_reschedule_ex(struct ktask *__restrict self,
                    struct kcpu *__restrict newcpu,
                    int hint);
#else /* !KCONFIG_HAVE_SINGLECORE */
extern __wunused __nonnull((1)) kerrno_t __SCHED_CALL
ktask_reschedule_ex_impl(struct ktask *__restrict self, int hint);
#define ktask_reschedule_ex(self,newcpu,hint) ktask_reschedule_ex(self,hint)
#endif /* KCONFIG_HAVE_SINGLECORE */
#define ktask_reschedule(self) \
 ktask_reschedule_ex(self,kcpu_leastload(),KTASK_RESCHEDULE_HINT_DEFAULT)



//////////////////////////////////////////////////////////////////////////
// Allocates a new suspended task.
// NOTE: The caller must fill the stack using 'ktask_stackpush_sp_unlocked'
//       before calling 'ktask_resume_k' to start the task.
// NOTE: The newly created task does not own the given PD
// @param: flags: A set of 'KTASK_FLAG_*' flags.
// @return: NULL: Not enough memory to allocate the task/stack

extern __crit __wunused __nonnull((1,2)) __ref struct ktask *
ktask_newkernel(struct ktask *__restrict parent, struct kproc *__restrict proc,
                __size_t kstacksize, __u16 flags);
extern __crit __wunused __nonnull((1,2)) __ref struct ktask *
ktask_newuser(struct ktask *__restrict parent, struct kproc *__restrict ctx,
              __user void **__restrict useresp, __size_t ustacksize, __size_t kstacksize);
extern __crit __wunused __nonnull((1,2)) __ref struct ktask *
ktask_newuserex(struct ktask *__restrict parent, struct kproc *__restrict ctx,
                __pagealigned __user void *ustackaddr,
                __size_t ustacksize, __size_t kstacksize, __u16 flags);

#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Returns the parent of a given task (Or the task itself for 'ktask_zero()')
// NOTE: [!*_k] The task itself is returned if it is the current visibility barrier.
extern __wunused __retnonnull __nonnull((1)) struct ktask *ktask_parent_k(struct ktask const *__restrict self);
extern __wunused __retnonnull __nonnull((1)) struct ktask *ktask_parent(struct ktask const *__restrict self);
extern __wunused              __nonnull((1)) __size_t ktask_getparid_k(struct ktask const *__restrict self);
extern __wunused              __nonnull((1)) __size_t ktask_getparid(struct ktask const *__restrict self);
extern __wunused              __nonnull((1)) __size_t ktask_gettid(struct ktask const *__restrict self);
#else
#define ktask_parent_k(self)   ((self)->t_parent)
#define ktask_getparid_k(self) ((self)->t_parid)
#define ktask_gettid(self)     ((self)->t_tid)
extern __wunused __retnonnull __nonnull((1)) struct ktask *ktask_parent(struct ktask const *__restrict self);
extern __wunused __nonnull((1)) __size_t ktask_getparid(struct ktask const *__restrict self);
#endif

//////////////////////////////////////////////////////////////////////////
// Returns a new reference to the child of a given task (by use of its parid)
// @return: NULL: The given parid is invalid, or the given child no longer exists
extern __crit __wunused __nonnull((1)) __ref struct ktask *
ktask_getchild(struct ktask const *self, __size_t parid);



//////////////////////////////////////////////////////////////////////////
// Internal helper functions
extern __wunused __nonnull((1)) __kernel void *ktask_getkernelesp_n(struct ktask const *self);
#ifdef __INTELLISENSE__
extern __wunused __nonnull((1)) __kernel void *ktask_getkernelesp(struct ktask const *self);
extern __wunused __nonnull((1)) __user   void *ktask_getuseresp(struct ktask const *self);
extern __wunused __nonnull((1)) struct ktaskregisters  *ktask_getregisters(struct ktask *self);
extern __wunused __nonnull((1)) struct ktaskregisters3 *ktask_getregisters3(struct ktask *self);
#else
#define ktask_getkernelesp(self) \
 __xblock({ struct ktask const *const __ktgkeself = (self);\
            __xreturn kpagedir_translate(__ktgkeself->t_userpd,__ktgkeself->t_esp);\
 })
#define ktask_getuseresp(self)      (self)->t_esp
#define ktask_getregisters(self)   ((struct ktaskregisters *)ktask_getkernelesp(self))
#define ktask_getregisters3(self) ((struct ktaskregisters3 *)ktask_getkernelesp(self))
#endif



//////////////////////////////////////////////////////////////////////////
// Setup the task stack for its initial jump into ring 3.
// >> Called after the task was created using one of the 'ktask_newuser*' functions,
//    the implementor should call this function to setup the task's stack for
//    the initial jump into user space.
extern __nonnull((1,2,3,4)) void ktask_setupuserex(struct ktask *self, __user void *useresp,
                                                   __user void *eip, __kernel struct kpagedir *userdir);
#if defined(__INTELLISENSE__) || 1
extern __nonnull((1,2,3)) void ktask_setupuser(struct ktask *self,
                                               __user void *useresp, __user void *eip);
#else
#define ktask_setupuser(self,useresp,eip) \
 __xblock({ struct ktask *const __ktsuself = (self);\
            ktask_setupuserex(__ktsuself,useresp,eip\
                             ,kproc_getpagedir(__ktsuself->t_proc));\
            (void)0;\
 })
#endif


//////////////////////////////////////////////////////////////////////////
// Fast-pass for creating and starting a new kernel task
#define ktask_new_suspended(main,closure)                               ktask_newex_suspended(main,closure,KTASK_STACK_SIZE_DEF,KTASK_FLAG_NONE)
#define ktask_new(main,closure)                                                   ktask_newex(main,closure,KTASK_STACK_SIZE_DEF,KTASK_FLAG_NONE)
#define ktask_newctx_suspended(parent,proc,main,closure) ktask_newctxex_suspended(parent,proc,main,closure,KTASK_STACK_SIZE_DEF,KTASK_FLAG_NONE)
#define ktask_newctx(parent,proc,main,closure)                     ktask_newctxex(parent,proc,main,closure,KTASK_STACK_SIZE_DEF,KTASK_FLAG_NONE)

extern __crit __wunused __nonnull((1)) __ref struct ktask *
ktask_newex_suspended(void *(*main)(void *), void *closure, __size_t stacksize, __u32 flags);
extern __crit __wunused __nonnull((1)) __ref struct ktask *
ktask_newex(void *(*main)(void *), void *closure, __size_t stacksize, __u32 flags);
extern __crit __wunused __nonnull((1,2,3)) __ref struct ktask *
ktask_newctxex_suspended(struct ktask *parent, struct kproc *__restrict proc,
                         void *(*main)(void *), void *closure, __size_t stacksize, __u32 flags);
extern __crit __wunused __nonnull((1,2,3)) __ref struct ktask *
ktask_newctxex(struct ktask *parent, struct kproc *__restrict proc,
               void *(*main)(void *), void *closure, __size_t stacksize, __u32 flags);

#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Get/Set the priority of a given task.
// NOTE: When setting, the given priority 'v' should be
// @return: KE_RANGE: The given priority level lies outside the active limits.
// @return: KE_ACCES: The given priority level isn't allowed by the sandbox,
//                    or the caller does not have permissions to modify the
//                    given task.
extern __wunused __nonnull((1)) ktaskprio_t ktask_getpriority(struct ktask const *self);
extern __wunused __nonnull((1))    kerrno_t ktask_setpriority(struct ktask *self, ktaskprio_t v);
extern __wunused __nonnull((1)) ktaskprio_t ktask_getpriority_k(struct ktask const *self);
extern           __nonnull((1))        void ktask_setpriority_k(struct ktask *self, ktaskprio_t v);
#else
#define ktask_getpriority  ktask_getpriority_k
extern __wunused __nonnull((1))   kerrno_t ktask_setpriority(struct ktask *self, ktaskprio_t v);
#define ktask_getpriority_k(self)   katomic_load((self)->t_setpriority)
#define ktask_setpriority_k(self,v) katomic_store((self)->t_setpriority,v)
#endif

//////////////////////////////////////////////////////////////////////////
// Returns non-ZERO of the given task 'self' is a child of 'parent'.
// NOTE: 'Parent' must not necessary be a valid pointer
extern __wunused __nonnull((1,2)) int ktask_ischildof(struct ktask const *self,
                                                      struct ktask const *parent);
#ifdef __INTELLISENSE__
extern __wunused __nonnull((1,2)) int ktask_issameorchildof(struct ktask const *self,
                                                            struct ktask const *parent);
#else
#define ktask_issameorchildof(self,parent) \
 __xblock({ struct ktask const *const __kicoself = (self);\
            struct ktask const *const __kicoparent = (parent);\
            __xreturn __kicoself == __kicoparent ||\
              ktask_ischildof(__kicoself,__kicoparent);\
 })

#endif

//////////////////////////////////////////////////////////////////////////
// Returns the CPU associated with a given task, or NULL
// if the given task is neither running, nor waiting.
// @return: NULL: The task's state is one of 'KTASK_STATE_SUSPENDED',
//                'KTASK_STATE_TERMINATED' or 'KTASK_STATE_WAITING'.
extern __wunused __nonnull((1)) struct kcpu *ktask_getcpu(struct ktask *__restrict self);

//////////////////////////////////////////////////////////////////////////
// Push/Pop values off the stack of a (suspended) task.
// NOTE: These functions are used to initialize the stack of a newly created task.
extern __nonnull((1,2)) void ktask_stackpush_sp_unlocked(struct ktask *__restrict self, void const *__restrict p, __size_t s);
extern __nonnull((1,2)) void ktask_stackpop_sp_unlocked(struct ktask *__restrict self, void *__restrict p, __size_t s);

/* Do an unsafe read of the current task from the
   volatile 'c_current' field of the associated CPU.
   NOTE: The 'c_current' field is only volatile when interrupts are enabled. */
#define __ktask_cpuself_volatile() (kcpu_self()->c_current)

#ifdef KCONFIG_HAVE_SINGLECORE
/* Exception: On a single-core machine, the possibility of being swapped over
 *            to a different CPU isn't there (as there's only one).
 *            So with this in mind, we can always use volatile reads,
 *            with the operation not even failing when an IRQ fires
 *            just before, even while we are reading the c_current field.
 *            Because once (if at all) we get executed again, we'll be
 *            just where we left off, resulting in a seamless transition.
 */
#define __ktask_cpuself   __ktask_cpuself_volatile
#elif defined(CONFIG_COMPILETIME_NOINTERRUPT_OPTIMIZATIONS)

/* A somewhat different version of ktask_self,
 * that uses the read-until-same result trick. */
__forcelocal __wunused __constcall __retnonnull
struct ktask *__ktask_cpuself_spin(void) {
 register struct kcpu *c;
 register struct ktask *result;
 do result = (c = kcpu_self())->c_current;
 while (c != kcpu_self());
 return result;
}
/* NOTE: 'NOIRQ_P' always evaluates at compile-time,
         with the optimization potential being that while
         inside a no-interrupt block, we know that our
         current CPU won't change, meaning that we can
         safely read our volatile c_current pointer, knowing
         that no IRQ will fire to unschedule us, after which
         some other task might re-schedule us on a different
         CPU, thereby resulting in kcpu_self() no longer
         pointing to what is really the current CPU. */
/* NOTE: Must use a macro for this to take
         advantage of compile-time nointerrupt. */
#define __ktask_cpuself() \
 (NOIRQ_P \
   ? __ktask_cpuself_volatile() \
   : __ktask_cpuself_spin())
#else
/* Interrupt-only ktask_self()-version.
   We use this if we can't make predictions about
   the nointerrupt state, so we'll just  */
#define __ktask_cpuself() \
 __xblock({ register struct ktask *__tsr;\
            NOIRQ_BEGIN\
            __tsr = kcpu_self()->c_current;\
            NOIRQ_END\
            __xreturn __tsr;\
 })
#endif


#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Returns the task structure associated with the calling task.
// NOTE: This function never fails and always returns non-NULL.
// @return: * : The task controller for the calling task (aka. thread).
extern __wunused __constcall __retnonnull struct ktask *ktask_self(void);
#else
#define ktask_self   __ktask_cpuself
#endif

#ifdef __INTELLISENSE__
extern __wunused __constcall __retnonnull struct kproc *kproc_self(void);
extern __wunused __constcall __retnonnull struct ktask *ktask_proc(void);
#else
#define kproc_self()  (ktask_self()->t_proc)
#define ktask_proc()  ktask_procof(ktask_self())
#endif

//////////////////////////////////////////////////////////////////////////
// Returns the process task of a given task.
// NOTE: The process task is the top-most parent task within the same context.
extern __wunused __constcall __retnonnull __nonnull((1))
struct ktask *ktask_procof(struct ktask *__restrict self);


//////////////////////////////////////////////////////////////////////////
// Sets an alarm timeout of a given task, or change/set the timeout
// of a task already in the process of blocking.
//  - If 'abstime' is NULL, a set alarm is deleted.
//  - The alarm will act as a timeout the next time the given task
//    will be unscheduled for the purposes of waiting on a signal.
//    If in that call, an explicit timeout is specified, the 'abstime'
//    specified in this call will have no effect and be dropped.
//  - This function can be used to specify
//  - If the task is already waiting, this function may be used to specify,
//    or even override the timeout of the already waiting task.
// @return: KE_DESTROYED: The given task was destroyed.
// @return: KE_OK:        The task is ready to wait on the next blocking signal-wait.
//                        NOTE: '*oldabstime' is left untouched.
//                        NOTE: If 'abstime' is NULL, the task is left unchanged.
// @return: KS_UNCHANGED: A timeout had already been set, and its value was stored in
//                        '*oldabstime' if non-NULL. Though the timeout has now been
//                        overwritten by 'abstime'.
//                        The task may have also already entered a blocking call
//                        with another timeout, and setting the alarm overwrote that
//                        previous timeout. ('*oldabstime' is filled with that old timeout)
// @return: KE_ACCES:     [!*_k] The call does not have SETPROP access to the given task
extern __nonnull((1)) kerrno_t
ktask_setalarm_k(struct ktask *__restrict self,
                 struct timespec const *__restrict abstime,
                 struct timespec *__restrict oldabstime);
extern __nonnull((1)) kerrno_t
ktask_setalarm(struct ktask *__restrict self,
               struct timespec const *__restrict abstime,
               struct timespec *__restrict oldabstime);

//////////////////////////////////////////////////////////////////////////
// Retrieves the timeout of a given waiting task and stores it in '*abstime'.
// @return: KE_OK:        The given task is waiting or has an alarm set, with
//                        its timeout now being stored in '*abstime'.
// @return: KE_DESTROYED: The given task has terminated.
// @return: KS_EMPTY:     The given task is neither waiting, nor has an alarm set.
// @return: KS_BLOCKING:  The given task is waiting, but not with a timeout.
extern __nonnull((1,2)) kerrno_t
ktask_getalarm_k(struct ktask const *__restrict self,
                 struct timespec *__restrict abstime);
#define ktask_getalarm ktask_getalarm_k

//////////////////////////////////////////////////////////////////////////
// Put a given task to sleep until a given time passes.
// NOTE: 'ktask_pause' can be used to wait indefinitely, or until an
//       alarm previously set through a call to 'ktask_setalarm_k' is triggered.
// @return: KE_OK:        The task's state was successfully updated.
// @return: KS_UNCHANGED: The given task was already sleeping.
//                        NOTE: In this situation, a given timeout may only lower an existing one.
// @return: KE_TIMEDOUT:  You are the given task, and you have simply timed out (This is normal).
// @return: KE_DESTROYED: The given task has already been terminated.
// @return: KE_OVERFLOW:  A reference overflow prevented the given task from entering sleep mode.
// @return: KE_INTR:      The calling task was interrupted.
extern __nonnull((1))   kerrno_t ktask_pause(struct ktask *__restrict self);
extern __nonnull((1,2)) kerrno_t ktask_sleep(struct ktask *__restrict self, struct timespec const *__restrict timeout);
extern __nonnull((1,2)) kerrno_t ktask_abssleep(struct ktask *__restrict self, struct timespec const *__restrict abstime);


#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Returns the task struct controlling the kernels ZERO-task.
// (That is the original task spawned by the BIOS and/or Bootloader)
extern __wunused __constcall __retnonnull struct ktask *ktask_zero(void);
#else
#define ktask_zero()   (&__ktask_zero)
#define ktask_hwirq()  (&__ktask_hwirq)
extern struct ktask __ktask_zero;
extern struct ktask __ktask_hwirq;
#endif

// ========================
//  Task statistics access
// ========================

#if KCONFIG_HAVE_TASK_STATS
#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Read the statistics of a given task and stores them in '*stats'
extern __nonnull((1,2)) void
ktask_getstat(struct ktask const *__restrict self,
              struct ktaskstat *__restrict stats);
#else
#define ktask_getstat(self,stats) ktaskstat_get(&(self)->t_stats,stats)
#endif
#endif /* KCONFIG_HAVE_TASK_STATS */

#ifdef KTASK_FLAG_CRITICAL
#define __ktask_region_crit_tryenter(did_not_enter) \
 do{ register struct ktask *const __ktself = ktask_self();\
     NOIRQ_BEGINLOCK(ktask_trylock(__ktself,KTASK_LOCK_STATE));\
     assert(__ktself->t_state != KTASK_STATE_TERMINATED);\
     if __likely(__ktself->t_flags&KTASK_FLAG_CRITICAL) {\
      (did_not_enter) = 1;\
     } else {\
      __ktself->t_flags |= KTASK_FLAG_CRITICAL;\
      (did_not_enter) = 0;\
     }\
     NOIRQ_ENDUNLOCK(ktask_unlock(__ktself,KTASK_LOCK_STATE));\
 }while(0)
#define __ktask_region_crit_enter() \
 do{ register struct ktask *const __ktself = ktask_self();\
     NOIRQ_BEGINLOCK(ktask_trylock(__ktself,KTASK_LOCK_STATE));\
     assert(__ktself->t_state != KTASK_STATE_TERMINATED);\
     assert(!(__ktself->t_flags&KTASK_FLAG_CRITICAL));\
     __ktself->t_flags |= KTASK_FLAG_CRITICAL;\
     NOIRQ_ENDUNLOCK(ktask_unlock(__ktself,KTASK_LOCK_STATE));\
 }while(0)
#define __ktask_region_crit_is_real()  ((ktask_self()->t_flags&KTASK_FLAG_CRITICAL)!=0)
#define __ktask_region_crit_is()       (NOIRQ_P || !karch_irq_enabled() || __ktask_region_crit_is_real() || kcpu_global_onetask())
#define __ktask_region_crit_leave()     ktask_unschedule_aftercrit(ktask_self())
#define __ktask_region_nointr_is()     ((ktask_self()->t_flags&KTASK_FLAG_NOINTR)!=0)
#ifdef __DEBUG__
#define __ktask_region_nointr_enter() \
 do{ struct ktask *const __ktself = ktask_self();\
     assertf(__ktself->t_flags&KTASK_FLAG_CRITICAL,\
             "Only a critical task may enter a no-intr region.");\
     __ktself->t_flags|=KTASK_FLAG_NOINTR;\
     (void)0;\
 }while(0);
#else
#define __ktask_region_nointr_enter() (ktask_self()->t_flags|=KTASK_FLAG_NOINTR)
#endif
#define __ktask_region_nointr_leave() (ktask_self()->t_flags&=~(KTASK_FLAG_NOINTR))

//////////////////////////////////////////////////////////////////////////
// Begin/end a critical operation within the calling task.
// While inside a critical block, the calling task cannot be terminated.
// NOTE: 'KTASK_CRIT_MARK' and 'KTASK_CRIT_TRUEMARK' differ from
//       each other, in that 'KTASK_CRIT_TRUEMARK' ensures a true
//       critical block, instead of allowing use of noirq as substitution.
//       Even though this might sound like what should be the default,
//       in most situations a noirq region completely suffices.
#define KTASK_CRIT_BEGIN        REGION_TRYENTER(ISCRIT,__ktask_region_crit_is_real,__ktask_region_crit_tryenter)
#define KTASK_CRIT_BEGIN_FIRST  REGION_ENTER_FIRST(ISCRIT,__ktask_region_crit_is_real,__ktask_region_crit_enter)
#define KTASK_CRIT_BREAK        REGION_BREAK(ISCRIT,__ktask_region_crit_leave)
#define KTASK_CRIT_END          REGION_LEAVE(ISCRIT,__ktask_region_crit_leave)
#define KTASK_CRIT_MARK         REGION_MARK(ISCRIT,__ktask_region_crit_is)
#define KTASK_CRIT_TRUEMARK     REGION_MARK(ISCRIT,__ktask_region_crit_is_real)
#define KTASK_CRIT_P            REGION_P(ISCRIT)
#define KTASK_CRIT_FIRST        REGION_FIRST(ISCRIT)
REGION_DEFINE(ISCRIT);


//////////////////////////////////////////////////////////////////////////
// Begin/end a critical operation within the calling task.
// While inside a critical block, the calling task cannot be terminated.
// NOTE: In addition, the nointerrupt counter is incremented,
//       thus suppressing an KE_INTR from being received by the
//       calling thread, ensuring an uninterrupted work-flow.
// >> KE_INTR is normally used to inform a critical task of some other
//    task's wish of terminating it. This is done by generating a sporadic
//    wake-up within the critical task that either causes the next
//    blocking operation involving a signal to fail with KE_INTR, or
//    a current blocking operation to be interrupted with the same error.
// >> Since this behavior may not always be wished, such an interrupt
//    can only happen once, when the original termination request returns
//    KS_BLOCKING to the caller, indicating that the task was scheduled
//    for termination, but is critical, preventing its immediate shutdown.
//    When not inside a no-interrupt block, the critical task is then woken,
//    with its current blocking operation failing with KE_INTR.
#define KTASK_NOINTR_BEGIN           REGION_ENTER(ISNOINTR,__ktask_region_nointr_is,__ktask_region_nointr_enter)
#define KTASK_NOINTR_BEGIN_FIRST     REGION_ENTER_FIRST(ISNOINTR,__ktask_region_nointr_is,__ktask_region_nointr_enter)
#define KTASK_NOINTR_BREAK           REGION_BREAK(ISNOINTR,__ktask_region_nointr_leave)
#define KTASK_NOINTR_END             REGION_LEAVE(ISNOINTR,__ktask_region_nointr_leave)
#define KTASK_NOINTR_MARK            REGION_MARK(ISNOINTR,__ktask_region_nointr_is)
#define KTASK_NOINTR_P               REGION_P(ISNOINTR)
#define KTASK_NOINTR_FIRST_P         REGION_FIRST_P(ISNOINTR)
#define KTASK_NOINTR_FIRST           REGION_FIRST(ISNOINTR)
REGION_DEFINE(ISNOINTR);

//////////////////////////////////////////////////////////////////////////
// Returns TRUE(1) if the calling task is protected from sporadic termination.
// Returns FALSE(0) otherwise.
// NOTE: 'KTASK_ISCRIT_P' is the compile-time-only version and can
//        be used to generate optimized code when used in macros.
#define KTASK_ISCRIT_P     (KTASK_CRIT_P || NOIRQ_P)
#define KTASK_ISCRIT       (KTASK_ISCRIT_P || __ktask_region_crit_is())
#define KTASK_ISTRUECRIT_P (KTASK_CRIT_P)
#define KTASK_ISTRUECRIT   (KTASK_ISCRIT_P || __ktask_region_crit_is_real())


//////////////////////////////////////////////////////////////////////////
// Similar to 'ktask_iscrit', but only returns TRUE(1)
// if the calling task isn't allowed to be interrupted
// by KE_INTR-style wake-ups.
// NOTE: 'KTASK_ISNOINTR_P' is the compile-time-only version and can
//        be used to generate optimized code when used in macros.
#define KTASK_ISNOINTR_P  (KTASK_NOINTR_P)
#define KTASK_ISNOINTR    (KTASK_ISNOINTR_P || __ktask_region_nointr_is())

#else
#define KTASK_ISCRIT_P      (NOIRQ_P)
#define KTASK_ISCRIT        (KTASK_ISCRIT_P || !karch_irq_enabled() || kcpu_global_onetask())
#define KTASK_ISTRUECRIT_P  (0)
#define KTASK_ISTRUECRIT    (kcpu_global_onetask())
#endif

#define __ktask_region_kepd_is()    (ktask_self()->t_epd == kpagedir_kernel())
#define __ktask_region_kepd_tryenter(did_not_enter) \
 do{ register struct ktask *const __ktself = ktask_self();\
     if __likely(__ktself->t_epd == kpagedir_kernel()) {\
      (did_not_enter) = 1;\
     } else {\
      __ktself->t_epd = kpagedir_kernel();\
      (did_not_enter) = 0;\
     }\
 }while(0)
#define __ktask_region_kepd_enter() (ktask_self()->t_epd = kpagedir_kernel())
#define __ktask_region_kepd_leave() \
 { register struct ktask *const __ktepdself = ktask_self();\
   __ktepdself->t_epd = kproc_getpagedir(ktask_getproc(__ktepdself)); }

#define KTASK_KEPD_BEGIN       REGION_TRYENTER(ISKEPD,__ktask_region_kepd_is,__ktask_region_kepd_tryenter)
#define KTASK_KEPD_BEGIN_FIRST REGION_ENTER_FIRST(ISKEPD,__ktask_region_kepd_is,__ktask_region_kepd_enter)
#define KTASK_KEPD_BREAK       REGION_BREAK(ISKEPD,__ktask_region_kepd_leave)
#define KTASK_KEPD_END         REGION_LEAVE(ISKEPD,__ktask_region_kepd_leave)
#define KTASK_KEPD_MARK        REGION_MARK(ISKEPD,__ktask_region_kepd_is)
#define KTASK_KEPD_P           REGION_P(ISKEPD)
#define KTASK_KEPD_FIRST_P     REGION_FIRST_P(ISKEPD)
#define KTASK_KEPD_FIRST       REGION_FIRST(ISKEPD)
REGION_DEFINE(ISKEPD);

//////////////////////////////////////////////////////////////////////////
// Returns true if the calling task has overwritten its
// effective page directory to that of the kernel.
#define KTASK_ISKEPD_P  (KTASK_KEPD_P)
#define KTASK_ISKEPD    (KTASK_ISKEPD_P || __ktask_region_kepd_is())


//////////////////////////////////////////////////////////////////////////
// Returns TRUE(1) if the calling thread (currently protected
// by a critical block through a prior call to 'KTASK_CRIT_BEGIN'),
// has been scheduled for termination and is merely kept alive
// by means of said critical block.
// Returns FALSE(0) otherwise.
// >> This function can be used to check for termination within
//    permanently critical tasks before entering a timeout-based
//    signal receiver for some fairly long time.
#define KTASK_SHOULDTERMINATE   (ktask_self()->t_newstate == KTASK_STATE_TERMINATED)

#ifndef __INTELLISENSE__
#undef ktask_iscrit
#undef ktask_isnointr
extern int ktask_iscrit(void);
extern int ktask_isnointr(void);
#endif

#define ktask_iscrit()    (KTASK_ISCRIT || kcpu_global_onetask())
#define ktask_isnointr()   KTASK_ISNOINTR

#define KTASK_CRIT_V(expr) \
 __xblock({ KTASK_CRIT_BEGIN\
            (expr);\
            KTASK_CRIT_END\
            (void)0;\
 })
#define KTASK_CRIT(expr) \
 __xblock({ __typeof__(expr) __ktcres;\
            KTASK_CRIT_BEGIN\
            __ktcres = (expr);\
            KTASK_CRIT_END\
            __xreturn __ktcres;\
 })

#define KTASK_KEPD(expr) \
 __xblock({ __typeof__(expr) __ktcres;\
            KTASK_KEPD_BEGIN\
            __ktcres = (expr);\
            KTASK_KEPD_END\
            __xreturn __ktcres;\
 })




//////////////////////////////////////////////////////////////////////////
// Terminates a given task with a given exit code.
// WARNING: Terminating a task without tracking all of its resources other
//          than those living on its heap will probably create leaks.
//          Also note, that a task that was meant to emit a signal at
//          some point meant for other tasks obviously won't not be able
//          to do so once it has been terminated.
// NOTES:
//   - It is technically possible to terminate 'ktask_zero()', but
//     doing so really isn't something you should ever attempt to do.
//   - When attempting to terminate a critical task, terminate will
//     mark the task as to-be terminated once it no longer executes
//     critical code. (NOTE: Use 'ktask_join' to wait until it is done)
//     HINT: 'ktask_terminate_k' does the same as '_ktask_terminate_k', but
//           will already perform the waiting in the KS_BLOCKING case.
//   - You are able to terminate yourself using this function,
//     making 'ktask_exit()' really just an a correctly denoted as
//     __noreturn alias for '_ktask_terminate_k(ktask_self(),...)'
// @return: KE_OK:        The task was destroyed.
// @return: KE_DESTROYED: [!*r] The task had already exited normally, or was already terminated.
// @return: KS_BLOCKING:  [_ktask_terminate*] The task is currently performing critical
//                        operations and can only be terminated afterwards. Its interrupt
//                        flag was set, and a blocking operation _may_ have been interrupted.
//                        >> Use 'ktask_terminate*' instead or call 'ktask_join'
//                           to wait for termination of commence.
// @return: KE_ACCES:     [!*_k] The caller does not have the required access to modify this task.
extern __nonnull((1)) kerrno_t _ktask_terminate_k(struct ktask *__restrict self, void *exitcode);
extern __nonnull((1)) kerrno_t  ktask_terminate_k(struct ktask *__restrict self, void *exitcode);
extern __nonnull((1)) kerrno_t _ktask_terminate(struct ktask *__restrict self, void *exitcode);
extern __nonnull((1)) kerrno_t  ktask_terminate(struct ktask *__restrict self, void *exitcode);

extern __nonnull((1)) kerrno_t ktask_terminateex_k(struct ktask *__restrict self, void *exitcode, __ktaskopflag_t mode);
extern __nonnull((1)) kerrno_t ktask_terminateex(struct ktask *__restrict self, void *exitcode, __ktaskopflag_t mode);

extern __noreturn void ktask_exit(void *exitcode);

#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Detaches a given task.
// WARNING: If you look at what this does, you should know that you're not
//          allowed to do anything with your task pointer after this returns!
extern __nonnull((1)) void ktask_detach(struct ktask *self);
#else
#define ktask_detach    ktask_decref
#endif

//////////////////////////////////////////////////////////////////////////
// Joins a given task.
//  - Using a real signal, the calling task may be unscheduled until
//    the given task 'self' has run its course, or was terminated.
//  - If 'exitcode' is non-NULL, the task's return value is stored in '*exitcode'
//  - Note, that these functions may be called any amount of times, even when
//    the task has already been joined by the same, or a different task.
//  - If the given task 'self' has already terminated/exited,
//    all of these functions return immediately.
// @return: KE_OK:         The given task has been terminated, or has exited.
//                         When given, its return value has been stored in '*exitcode'.
// @return: KE_WOULDBLOCK: [ktask_tryjoin] Failed to join the task without blocking.
// @return: KE_TIMEDOUT:   [ktask_(timed|timeout)join] The given abstime has passed.
// @return: KE_INTR:      [!ktask_tryjoin] The calling task was interrupted.
extern           __nonnull((1)) kerrno_t ktask_join(struct ktask *__restrict self, void **__restrict exitcode);
extern __wunused __nonnull((1)) kerrno_t ktask_tryjoin(struct ktask *__restrict self, void **__restrict exitcode);
extern __wunused __nonnull((1)) kerrno_t ktask_timedjoin(struct ktask *__restrict self, struct timespec const *__restrict abstime, void **__restrict exitcode);
extern __wunused __nonnull((1)) kerrno_t ktask_timeoutjoin(struct ktask *__restrict self, struct timespec const *__restrict timeout, void **__restrict exitcode);

//////////////////////////////////////////////////////////////////////////
// Returns the exitcode of a terminated (zombie) task.
// NOTE: This function is required for windows's GetExitCodeProcess/GetExitCodeThread functions.
// NOTE: This function is non-blocking.
// @return: KE_OK:   The return code has been successfully retrieved.
// @return: KE_BUSY: The given task has not been terminated.
extern __wunused __nonnull((1,2)) kerrno_t
ktask_getexitcode(struct ktask *__restrict self,
                  void **__restrict exitcode);


//////////////////////////////////////////////////////////////////////////
// Suspend/Resume a given task.
// NOTE: When resuming, the task will not be switched to immediately
// @return: KE_OK:        The task was suspended/resumed
// @return: KE_OVERFLOW:  The suspend/resume counter has reached its limit.
//                        The task's state was unchanged and the change should not be undone.
// @return: KE_DESTROYED: [ktask_suspend_k] Task has already terminated
// @return: KS_UNCHANGED: Due to recursion, the task's state was unchanged
//                        NOTE: This should not NEVER considered as an error.
// @return: KS_BLOCKING:  [ktask_resume_k] The suspension counter was modified, and the task's
//                        state would have changed, if it wasn't being blocked by a signal.
// @return: KE_ACCES:     [!*_k] The caller does not have the necessary permissions.
extern __wunused __nonnull((1)) kerrno_t ktask_suspend_k(struct ktask *__restrict self);
extern           __nonnull((1)) kerrno_t ktask_resume_k(struct ktask *__restrict self);
extern __wunused __nonnull((1)) kerrno_t ktask_suspend(struct ktask *__restrict self);
extern           __nonnull((1)) kerrno_t ktask_resume(struct ktask *__restrict self);
extern __wunused __nonnull((1)) kerrno_t ktask_suspend_kr(struct ktask *__restrict self);
extern           __nonnull((1)) kerrno_t ktask_resume_kr(struct ktask *__restrict self);
extern __wunused __nonnull((1)) kerrno_t ktask_suspend_r(struct ktask *__restrict self);
extern           __nonnull((1)) kerrno_t ktask_resume_r(struct ktask *__restrict self);

#if KCONFIG_HAVE_TASK_NAMES
//////////////////////////////////////////////////////////////////////////
// Get/Set the name of a task
// @return: KE_EXISTS: A name was already set (task names cannot be changed once set).
// @return: KE_NOMEM:  No enough memory to allocate the given name.
// @return: KE_ACCES:  The caller does not have the required access to modify this task.
#ifdef __INTELLISENSE__
extern __wunused __nonnull((1)) char const *
ktask_getname(struct ktask const *__restrict self);
extern __wunused __nonnull((1,2)) kerrno_t
ktask_setname(struct ktask *__restrict self,
              char const *__restrict name);
extern __wunused __nonnull((1,2)) kerrno_t
ktask_setnameex_k(struct ktask *__restrict self,
                  char const *__restrict name, __size_t maxlen);
extern __wunused __nonnull((1,2)) kerrno_t
ktask_setnameex(struct ktask *__restrict self,
                char const *__restrict name, __size_t maxlen);
#else
#define ktask_getname(self)               ((char const *)katomic_load((self)->t_name))
#define ktask_setname(self,name)            ktask_setnameex(self,name,(size_t)-1)
#define ktask_setnameex_k(self,name,maxlen) KTASK_CRIT(ktask_setnameex_ck(self,name,maxlen))
#define ktask_setnameex(self,name,maxlen) \
 __xblock({ struct ktask *const __tsncal = ktask_self();\
            __xreturn ktask_accesssp_ex(self,__tsncal)\
              ? ktask_setnameex_k(self,name,min(maxlen,__tsncal->t_proc->p_sand.ts_namemax))\
              : KE_ACCES;\
 })
#endif
extern __crit __wunused __nonnull((1,2)) kerrno_t
ktask_setnameex_ck(struct ktask *__restrict self,
                   char const *__restrict name, __size_t maxlen);
#endif

//////////////////////////////////////////////////////////////////////////
// Get the value of a TLS slot in the given task.
// @return: NULL: Invalid/Uninitialized TLS slot
#ifdef __INTELLISENSE__
extern __wunused __nonnull((1)) void *
ktask_gettls(struct ktask const *__restrict self,
             __ktls_t tlsid);
#else
#define ktask_gettls(self,tlsid) \
 __xblock({ void *__tgtres; struct ktask const *const __tgtself = (self);\
            kassert_ktask(__tgtself);\
            NOIRQ_BEGINLOCK(ktask_trylock((struct ktask *)__tgtself,KTASK_LOCK_TLS));\
            __tgtres = ktlspt_get(&__tgtself->t_tls,tlsid);\
            NOIRQ_ENDUNLOCK(ktask_unlock((struct ktask *)__tgtself,KTASK_LOCK_TLS));\
            __xreturn __tgtres;\
 })
#endif

//////////////////////////////////////////////////////////////////////////
// Set the value of a TLS slot in the given task.
// >> An optional 'oldvalue' will be filled with the old TLS value when non-NULL.
// @return: KE_OK:    The TLS value was successfully set.
// @return: KE_INVAL: The given TLS identifier is invalid/free/unused.
// @return: KS_FOUND: The given 'slot' was already allocated and a
//                    potential previous value was overwritten.
//                    NOTE: This does not mean that this was
//                          the first time 'slot' was set.
// @return: KE_NOMEM: The given 'slot' had yet to be set, and when attempting
//                    to set it just now, the kernel failed to reallocate its
//                    vector stored TLS values.
extern __crit __wunused __nonnull((1)) kerrno_t
ktask_settls_c(struct ktask *__restrict self, __ktls_t tlsid,
               void *value, void **oldvalue);
#ifdef __INTELLISENSE__
extern __wunused __nonnull((1)) kerrno_t
ktask_settls(struct ktask *__restrict self, __ktls_t tlsid,
             void *value, void **oldvalue);
#else
#define ktask_settls(self,tlsid,value,oldvalue) KTASK_CRIT(ktask_settls_c(self,tlsid,value,oldvalue))
#endif


//////////////////////////////////////////////////////////////////////////
// Get/Set task attributes.
// NOTE: The !*_k versions require the caller to have
//       GETPROP/SETPROP access respectively.
extern __wunused __nonnull((1,3)) kerrno_t
ktask_getattr_k(struct ktask *__restrict self, kattr_t attr,
                void *__restrict buf, __size_t bufsize, __size_t *__restrict reqsize);
extern __wunused __nonnull((1,3)) kerrno_t
ktask_setattr_k(struct ktask *__restrict self, kattr_t attr,
                void const *__restrict buf, __size_t bufsize);
extern __wunused __nonnull((1,3)) kerrno_t
ktask_getattr(struct ktask *__restrict self, kattr_t attr,
              void *__restrict buf, __size_t bufsize, __size_t *__restrict reqsize);
extern __wunused __nonnull((1,3)) kerrno_t
ktask_setattr(struct ktask *__restrict self, kattr_t attr,
              void const *__restrict buf, __size_t bufsize);
#endif /* !__ASSEMBLY__ */

__DECL_END

#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_TASK_H__ */

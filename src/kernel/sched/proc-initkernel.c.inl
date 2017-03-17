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
#ifndef __KOS_KERNEL_SCHED_PROC_INITKERNEL_C_INL__
#define __KOS_KERNEL_SCHED_PROC_INITKERNEL_C_INL__ 1

#include <kos/kernel/fs/fs-dirfile.h>
#include <kos/kernel/linker.h>
#include <kos/kernel/paging.h>
#include <kos/kernel/proc.h>
#include <kos/kernel/task.h>
#include <kos/syslog.h>
#include <malloc.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#if KCONFIG_HAVE_TASK_STATS_START
#include <kos/kernel/time.h>
#endif
#include <kos/kernel/panic.h>

__DECL_BEGIN

extern struct kdirfile __kproc_kernel_root;
struct kdirfile __kproc_kernel_root = KDIRFILE_INIT(&__kfs_root,NULL);
struct kproc __kproc_kernel = {
 KOBJECT_INIT(KOBJECT_MAGIC_PROC)
 /* p_refcnt               */0xffff,
 /* p_pid                  */0,
 /* p_lock                 */KMMUTEX_INIT,
 /* p_shm                  */{KOBJECT_INIT(KOBJECT_MAGIC_SHM)
 /* p_shm.s_lock           */KRWLOCK_INIT,
 /* p_shm.s_pd             */kpagedir_kernel(),
 /* p_shm.s_map            */{
 /* p_shm.s_map.m_root     */NULL},
 /* p_shm.s_ldt            */{
 /* p_shm.s_ldt.ldt_gdtid  */KSEG_KERNELLDT,
 /* p_shm.s_ldt.ldt_limit  */0,
 /* p_shm.s_ldt.ldt_base   */NULL,
 /* p_shm.s_ldt.ldt_vector */NULL}},
 /* p_regs                 */{
 /* p_regs.pr_cs           */KSEG_KERNEL_CODE,
 /* p_regs.pr_ds           */KSEG_KERNEL_DATA},
 /* p_modules              */{KOBJECT_INIT(KOBJECT_MAGIC_PROCMODULES)
 /* p_modules.pms_moda     */0,
 /* p_modules.pms_modv     */NULL},
 /* p_fdman                */{KOBJECT_INIT(KOBJECT_MAGIC_FDMAN)
 /* p_fdman.fdm_cnt        */0,
 /* p_fdman.fdm_max        */KFDMAN_FDMAX_TECHNICAL_MAXIMUM,
 /* p_fdman.fdm_fre        */0,
 /* p_fdman.fdm_fda        */0,
 /* p_fdman.fdm_fdv        */NULL,
 /* p_fdman.fdm_root       */KFDENTRY_INIT_FILE((struct kfile *)&__kproc_kernel_root),
 /* p_fdman.fdm_cwd        */KFDENTRY_INIT_FILE((struct kfile *)&__kproc_kernel_root)},
 /* p_perm                 */{KOBJECT_INIT(KOBJECT_MAGIC_PROCPERM){{
 /* p_perm.pp_gpbarrier    */ktask_zero(),
 /* p_perm.pp_spbarrier    */ktask_zero(),
 /* p_perm.pp_gmbarrier    */ktask_zero(),
 /* p_perm.pp_smbarrier    */ktask_zero()}},
 /* p_perm.pp_priomin      */KTASKPRIO_MIN,
 /* p_perm.pp_priomax      */KTASKPRIO_MAX,
#if KCONFIG_HAVE_TASK_NAMES
 /* p_perm.pp_namemax      */(size_t)-1,
#endif
 /* p_perm.pp_pipemax      */(size_t)-1,
 /* p_perm.pp_thrdmax      */(size_t)-1,
#if KPERM_FLAG_GROUPCOUNT != 4
#error FIXME: Add more permission masks
#endif
 /* p_perm.pp_flags        */{0xffff,0xffff,0xffff,0xffff},
 /* p_perm.pp_state        */KPROCSTATE_FLAG_NONE},
 /* p_tlsman               */KTLSMAN_INITROOT,
 /* p_threads              */KTASKLIST_INIT,
 /* p_environ              */KPROCENV_INIT_ROOT,
};

#if !KCONFIG_HAVE_TASK_STATS_START
extern struct timespec __kernel_boot_time;
struct timespec __kernel_boot_time = {0,0};
#endif

struct kproclist __kproclist_global = KPROCLIST_INIT;

void kernel_initialize_process(void) {
 k_syslogf(KLOG_INFO,"[init] Process...\n");
 __kproc_kernel.p_threads.t_taskv = (struct ktask **)malloc(1*sizeof(struct ktask *));
 if __unlikely(!__kproc_kernel.p_threads.t_taskv) {
  PANIC("Failed to initialize kernel task context: Not enough memory\n");
 }
 __kproc_kernel.p_threads.t_taska = 1;
 __kproc_kernel.p_threads.t_taskv[0] = ktask_zero();
#if KCONFIG_HAVE_TASK_STATS_START
 ktime_getnoworcpu(&ktask_zero()->t_stats.ts_abstime_start);
#else
 ktime_getnoworcpu(&__kernel_boot_time);
#endif
 __kproc_kernel.p_threads.t_freep = 1;
 assert(__kproc_kernel.p_pid == 0);
 __kproc_kernel.p_modules.pms_moda = 1;
 __kproc_kernel.p_modules.pms_modv = (struct kprocmodule *)malloc(sizeof(struct kprocmodule));
 if __unlikely(!__kproc_kernel.p_modules.pms_modv) {
  PANIC("Failed to initialize kernel process modules: Not enough memory\n");
 }
 kobject_init(&__kproc_kernel.p_modules.pms_modv[0],KOBJECT_MAGIC_PROCMODULE);
 __kproc_kernel.p_modules.pms_modv[0].pm_loadc  = 1;
 __kproc_kernel.p_modules.pms_modv[0].pm_flags  = KPROCMODULE_FLAG_RELOC;
 __kproc_kernel.p_modules.pms_modv[0].pm_lib    = kshlib_kernel();
 __kproc_kernel.p_modules.pms_modv[0].pm_base   = NULL;
 __kproc_kernel.p_modules.pms_modv[0].pm_depids = NULL;

 __kproclist_global.pl_procv = (struct kproc **)malloc(1*sizeof(struct kproc *));
 if __unlikely(!__kproclist_global.pl_procv) {
  PANIC("Failed to initialize kernel process list: Not enough memory\n");
 }
 __kproclist_global.pl_proca = 1;
 __kproclist_global.pl_procv[0] = &__kproc_kernel;
}

void kernel_finalize_process(void) {
 kproclist_close();
 assertf(__kproc_kernel.p_threads.t_taska == 1,
         "There are still tasks other than 'ktask_zero()' running in 'kproc_kernel()'");
 assertf(__kproc_kernel.p_threads.t_taskv[0] == ktask_zero(),
         "The last remaining task %p (%Iu|%Iu|%s) isn't ktask_zero()",
         __kproc_kernel.p_threads.t_taskv[0],
         __kproc_kernel.p_threads.t_taskv[0]->t_tid,
         __kproc_kernel.p_threads.t_taskv[0]->t_parid,
         ktask_getname(__kproc_kernel.p_threads.t_taskv[0]));
#ifdef __LIBC_HAVE_DEBUG_MALLOC
 // When allocations are tracked, satisfy the leak
 // detector by freeing this chunk of memory.
 free(__kproc_kernel.p_threads.t_taskv);
#endif
}


__DECL_END

#endif /* !__KOS_KERNEL_SCHED_PROC_INITKERNEL_C_INL__ */

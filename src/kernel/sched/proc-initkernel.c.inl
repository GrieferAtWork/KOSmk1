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
#include <malloc.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#if KTASK_HAVE_STATS_FEATURE(KTASK_HAVE_STATS_START)
#include <kos/kernel/time.h>
#endif

__DECL_BEGIN

struct kdirfile __kproc_kernel_root = KDIRFILE_INIT(&__kfs_root,NULL);
struct kproc __kproc_kernel = {
 KOBJECT_INIT(KOBJECT_MAGIC_PROC)
 0xffff,
 0,
 KMMUTEX_INIT,
 KSHM_INITROOT((struct kpagedir *)kpagedir_kernel(),KSEG_KERNELLDT),
 KPROCREGS_INIT(KSEG_KERNEL_CODE,KSEG_KERNEL_DATA),
 KPROCMODULES_INIT,
 KFDMAN_INITROOT((struct kfile *)&__kproc_kernel_root),
 KPROCSAND_INITROOT,
 KTLSMAN_INITROOT,
 KTASKLIST_INIT,
 KPROCENV_INIT_ROOT
};

#if !KTASK_HAVE_STATS_FEATURE(KTASK_HAVE_STATS_START)
extern struct timespec __kernel_boot_time;
struct timespec __kernel_boot_time = {0,0};
#endif

struct kproclist __kproclist_global = KPROCLIST_INIT;

void kernel_initialize_process(void) {
 __kproc_kernel.p_threads.t_taskv = (struct ktask **)malloc(1*sizeof(struct ktask *));
 if __unlikely(!__kproc_kernel.p_threads.t_taskv) {
  printf("Failed to initialize kernel task context: KE_NOMEM\n");
  abort();
 }
 __kproc_kernel.p_threads.t_taska = 1;
 __kproc_kernel.p_threads.t_taskv[0] = ktask_zero();
#if KTASK_HAVE_STATS_FEATURE(KTASK_HAVE_STATS_START)
 ktime_getnoworcpu(&ktask_zero()->t_stats.ts_abstime_start);
#else
 ktime_getnoworcpu(&__kernel_boot_time);
#endif
 __kproc_kernel.p_threads.t_freep = 1;
 assert(__kproc_kernel.p_pid == 0);
 __kproc_kernel.p_modules.pms_moda = 1;
 __kproc_kernel.p_modules.pms_modv = (struct kprocmodule *)malloc(sizeof(struct kprocmodule));
 if __unlikely(!__kproc_kernel.p_modules.pms_modv) {
  printf("Failed to initialize kernel process modules: KE_NOMEM\n");
  abort();
 }
 kobject_init(&__kproc_kernel.p_modules.pms_modv[0],KOBJECT_MAGIC_PROCMODULE);
 __kproc_kernel.p_modules.pms_modv[0].pm_loadc = 1;
 __kproc_kernel.p_modules.pms_modv[0].pm_flags = KPROCMODULE_FLAG_RELOC;
 __kproc_kernel.p_modules.pms_modv[0].pm_lib = kshlib_kernel();
 __kproc_kernel.p_modules.pms_modv[0].pm_base = NULL;

 __kproclist_global.pl_procv = (struct kproc **)malloc(1*sizeof(struct kproc *));
 if __unlikely(!__kproclist_global.pl_procv) {
  printf("Failed to initialize kernel process list: KE_NOMEM\n");
  abort();
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

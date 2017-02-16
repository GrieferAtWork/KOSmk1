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
#ifndef __KOS_KERNEL_SCHED_PROC_C__
#define __KOS_KERNEL_SCHED_PROC_C__ 1

#include <kos/config.h>
#include <kos/compiler.h>
#include <kos/kernel/fd.h>
#include <kos/kernel/fs/fs-dirfile.h>
#include <kos/kernel/fs/fs.h>
#include <kos/kernel/interrupts.h>
#include <kos/kernel/linker.h>
#include <kos/kernel/proc.h>
#include <malloc.h>
#include <stddef.h>

__DECL_BEGIN

__STATIC_ASSERT(offsetof(struct kproc,p_shm)     == KPROC_OFFSETOF_SHM);
__STATIC_ASSERT(offsetof(struct kproc,p_regs)    == KPROC_OFFSETOF_REGS);
__STATIC_ASSERT(offsetof(struct kproc,p_modules) == KPROC_OFFSETOF_MODULES);

__local void kprocsand_quit(struct kprocsand *__restrict self) {
 struct ktask *oldtask;
 oldtask = katomic_xch(self->ts_gpbarrier,NULL); assert(oldtask); ktask_decref(oldtask);
 oldtask = katomic_xch(self->ts_spbarrier,NULL); assert(oldtask); ktask_decref(oldtask);
 oldtask = katomic_xch(self->ts_gmbarrier,NULL); assert(oldtask); ktask_decref(oldtask);
 oldtask = katomic_xch(self->ts_smbarrier,NULL); assert(oldtask); ktask_decref(oldtask);
}
extern void kproc_destroy(struct kproc *__restrict self);
void kproc_destroy(struct kproc *__restrict self) {
 kassert_object(self,KOBJECT_MAGIC_PROC);
 assert(self != kproc_kernel());
 kproclist_delproc(self);
 kproc_close(self);
 /* Must destroy the SHM here, because the kernel process
  * may not destroy its statically allocated page directory! */
 kshm_quit(&self->p_shm);
 free(self);
}

kerrno_t kproc_close(struct kproc *__restrict self) {
 kerrno_t error;
 kassert_object(self,KOBJECT_MAGIC_PROC);
 error = kmmutex_close(&self->p_lock);
 if __likely(error != KS_UNCHANGED) {
  kfdman_quit(kproc_fdman(self));
  kprocsand_quit(&self->p_sand);
  ktlsman_quit(&self->p_tlsman);
  kprocmodules_quit(&self->p_modules);
  kprocenv_quit(&self->p_environ);
 }
 return error;
}

__crit kerrno_t kprocsand_initroot(struct kprocsand *self) {
 __u32 refcnt;
 struct ktask *taskgroup;
 KTASK_CRIT_MARK
 taskgroup = ktask_zero(); do {
  refcnt = katomic_load(taskgroup->t_refcnt);
  if __unlikely(refcnt > ((__u32)-1)-KSANDBOX_BARRIER_COUNT) return KE_OVERFLOW;
 } while (!katomic_cmpxch(taskgroup->t_refcnt,refcnt,refcnt+KSANDBOX_BARRIER_COUNT));
 self->ts_gpbarrier = taskgroup; // Inherit reference
 self->ts_spbarrier = taskgroup; // Inherit reference
 self->ts_gmbarrier = taskgroup; // Inherit reference
 self->ts_smbarrier = taskgroup; // Inherit reference
 self->ts_priomin   = KTASKPRIO_MIN;
 self->ts_priomax   = KTASKPRIO_MAX;
 self->ts_namemax   = (size_t)-1;
 self->ts_pipemax   = (size_t)-1;
 self->ts_flags     = 0xffffffff;
 self->ts_state     = KPROCSTATE_FLAG_NONE;
 return KE_OK;
}

static kerrno_t kproc_initregs(struct kproc *self) {
 static struct ksegment const defseg_cs = KSEGMENT_INIT(0,SEG_LIMIT_MAX,SEG_CODE_PL3);
 static struct ksegment const defseg_ds = KSEGMENT_INIT(0,SEG_LIMIT_MAX,SEG_DATA_PL3);
 /* TODO: Using this, we can restrict execute access within the process. */
 /* TODO: The LDT vector must be paged if we want to use it in ring-#3! */
 if __unlikely((self->p_regs.pr_cs = kshm_ldtalloc(&self->p_shm,&defseg_cs)) == KSEG_NULL) return KE_NOMEM;
 if __unlikely((self->p_regs.pr_ds = kshm_ldtalloc(&self->p_shm,&defseg_ds)) == KSEG_NULL) return KE_NOMEM;
 return KE_OK;
}


__crit __ref struct kproc *kproc_newroot(void) {
 struct kproc *result;
 KTASK_CRIT_MARK
 if __unlikely((result = omalloc(struct kproc)) == NULL) return NULL;
 if __unlikely(KE_ISERR(kprocsand_initroot(&result->p_sand))) goto err_r;
 if __unlikely(KE_ISERR(kshm_init(&result->p_shm,2))) goto err_sand;
 if __unlikely(KE_ISERR(kproc_initregs(result))) goto err_shm;
 kobject_init(result,KOBJECT_MAGIC_PROC);
 kobject_init(&result->p_fdman,KOBJECT_MAGIC_FDMAN);
 kmmutex_init(&result->p_lock);
 kprocmodules_init(&result->p_modules);
 result->p_refcnt            = 1;
 result->p_fdman.fdm_cnt     = 0;
 result->p_fdman.fdm_max     = KFDMAN_FDMAX_TECHNICAL_MAXIMUM;
 result->p_fdman.fdm_fre     = 0;
 result->p_fdman.fdm_fda     = 0;
 result->p_fdman.fdm_fdv     = NULL;
 if __unlikely(KE_ISERR(kproclist_addproc(result))) goto decref_result;
 result->p_fdman.fdm_root.fd_file = (struct kfile *)kdirfile_newroot();
 kprocenv_initroot(&result->p_environ);
 ktlsman_initroot(&result->p_tlsman);
 ktasklist_init(&result->p_threads);
 if __unlikely(!result->p_fdman.fdm_root.fd_file) {
  result->p_fdman.fdm_root.fd_type = KFDTYPE_NONE;
  result->p_fdman.fdm_cwd.fd_type = KFDTYPE_NONE;
decref_result:
  kproc_decref(result);
  result = NULL;
 } else {
  assert(result->p_fdman.fdm_root.fd_file->f_refcnt == 1);
  ++result->p_fdman.fdm_root.fd_file->f_refcnt;
  result->p_fdman.fdm_root.fd_type = KFDTYPE_FILE;
  result->p_fdman.fdm_cwd = result->p_fdman.fdm_root;
 }
 return result;
err_shm: kshm_quit(&result->p_shm);
err_sand: kprocsand_quit(&result->p_sand);
err_r: free(result);
 return NULL;
}

__crit kerrno_t kproc_barrier(struct kproc *__restrict self,
                                 struct ktask *__restrict barrier,
                                 ksandbarrier_t mode) {
 struct ktask *oldtasks[KSANDBOX_BARRIER_COUNT];
 struct ktask **olditer = oldtasks;
 kerrno_t error; __u32 refcnt; int nrefs = 0;
 KTASK_CRIT_MARK
 kassert_kproc(self);
 kassert_ktask(barrier);
 if __unlikely(KE_ISERR(error = kproc_lock(self,KPROC_LOCK_SAND))) return error;
 if (mode&KSANDBOX_BARRIER_NOGETPROP) ++nrefs;
 if (mode&KSANDBOX_BARRIER_NOSETPROP) ++nrefs;
 if (mode&KSANDBOX_BARRIER_NOGETMEM)  ++nrefs;
 if (mode&KSANDBOX_BARRIER_NOSETMEM)  ++nrefs;
 do { // Increment barrier references
  refcnt = katomic_load(barrier->t_refcnt);
  if __unlikely(refcnt > ((__u32)-1)-nrefs) return KE_OVERFLOW;
 } while (!katomic_cmpxch(barrier->t_refcnt,refcnt,refcnt+nrefs));
#define BARRIER(flag,field) \
 if (mode&flag) *olditer++ = katomic_xch(field,barrier) /* NOTE: Atomic read is unnecessary. */
 BARRIER(KSANDBOX_BARRIER_NOGETPROP,self->p_sand.ts_gpbarrier);
 BARRIER(KSANDBOX_BARRIER_NOSETPROP,self->p_sand.ts_spbarrier);
 BARRIER(KSANDBOX_BARRIER_NOGETMEM, self->p_sand.ts_gmbarrier);
 BARRIER(KSANDBOX_BARRIER_NOSETMEM, self->p_sand.ts_smbarrier);
#undef BARRIER
 kproc_unlock(self,KPROC_LOCK_SAND);
 // Drop references from all old barriers
 while (olditer != oldtasks) ktask_decref(*--olditer);
 return KE_OK;
}

__crit __ref struct kshlib *
kproc_getrootexe(struct kproc const *__restrict self) {
 __ref struct kshlib *result = NULL;
 struct kprocmodule *iter,*end;
 KTASK_CRIT_MARK
 kassert_kproc(self);
 if __unlikely(KE_ISERR(kproc_lock((struct kproc *)self,KPROC_LOCK_MODS))) return NULL;
 end = (iter = self->p_modules.pms_modv)+self->p_modules.pms_moda;
 for (; iter != end; ++iter) if ((result = iter->pm_lib) != NULL) {
  if (result->sh_callbacks.slc_start != KSYM_INVALID) goto found;
 }
 result = NULL;
 if (0) {found: if __unlikely(KE_ISERR(kshlib_incref(result))) result = NULL; }
 kproc_unlock((struct kproc *)self,KPROC_LOCK_MODS);
 return result;
}


__DECL_END

#ifndef __INTELLISENSE__
#define GETREF
#include "proc-getbarrier.c.inl"
#include "proc-getbarrier.c.inl"
#endif

#ifndef __INTELLISENSE__
#include "proc-attr.c.inl"
#include "proc-environ.c.inl"
#include "proc-exec.c.inl"
#include "proc-fdman.c.inl"
#include "proc-fork.c.inl"
#include "proc-initkernel.c.inl"
#include "proc-insmod.c.inl"
#include "proc-pid.c.inl"
#include "proc-tasks.c.inl"
#endif

#endif /* !__KOS_KERNEL_SCHED_PROC_C__ */

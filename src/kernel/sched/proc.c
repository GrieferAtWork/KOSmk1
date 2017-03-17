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
#include <kos/syslog.h>

__DECL_BEGIN

__STATIC_ASSERT(offsetof(struct kproc,p_shm)     == KPROC_OFFSETOF_SHM);
__STATIC_ASSERT(offsetof(struct kproc,p_regs)    == KPROC_OFFSETOF_REGS);
__STATIC_ASSERT(offsetof(struct kproc,p_modules) == KPROC_OFFSETOF_MODULES);
__STATIC_ASSERT(offsetof(struct kproc,p_shm.s_ldt.ldt_gdtid) ==
               (KPROC_OFFSETOF_SHM+KSHM_OFFSETOF_LDT+KLDT_OFFSETOF_GDTID));


extern void kproc_destroy(struct kproc *__restrict self);
void kproc_destroy(struct kproc *__restrict self) {
 kassert_object(self,KOBJECT_MAGIC_PROC);
 assert(self != kproc_kernel());
 kproclist_delproc(self);
 kproc_close(self);
 free(self);
}

kerrno_t kproc_close(struct kproc *__restrict self) {
 kerrno_t error;
 kassert_object(self,KOBJECT_MAGIC_PROC);
 error = kmmutex_close(&self->p_lock);
 if __likely(error != KS_UNCHANGED) {
  kfdman_quit(&self->p_fdman);
  kprocperm_quit(&self->p_perm);
  ktlsman_quit(&self->p_tlsman);
  kprocmodules_quit(&self->p_modules);
  kprocenv_quit(&self->p_environ);
  kshm_quit(&self->p_shm);
 }
 return error;
}

static kerrno_t kproc_initregs(struct kproc *self) {
 static struct ksegment const defseg_cs = KSEGMENT_INIT(0,SEG_LIMIT_MAX,SEG_CODE_PL3);
 static struct ksegment const defseg_ds = KSEGMENT_INIT(0,SEG_LIMIT_MAX,SEG_DATA_PL3);
 /* TODO: Using this, we can restrict execute access within the process. */
 if __unlikely((self->p_regs.pr_cs = kshm_ldtalloc(kproc_getshm(self),&defseg_cs)) == KSEG_NULL) return KE_NOMEM;
 if __unlikely((self->p_regs.pr_ds = kshm_ldtalloc(kproc_getshm(self),&defseg_ds)) == KSEG_NULL) return KE_NOMEM;
 k_syslogf(KLOG_DEBUG,"Initialized LDT at GDT offset %#I16x\n",kproc_getshm(self)->s_ldt.ldt_gdtid);
 return KE_OK;
}


__crit __ref struct kproc *kproc_newroot(void) {
 struct kproc *result;
 KTASK_CRIT_MARK
 if __unlikely((result = omalloc(struct kproc)) == NULL) return NULL;
 if __unlikely(KE_ISERR(kprocperm_initroot(&result->p_perm))) goto err_r;
 if __unlikely(KE_ISERR(kshm_init(&result->p_shm,2))) goto err_sand;
 if __unlikely(KE_ISERR(kproc_initregs(result))) goto err_shm;
 kobject_init(result,KOBJECT_MAGIC_PROC);
 kobject_init(&result->p_fdman,KOBJECT_MAGIC_FDMAN);
 kmmutex_init(&result->p_lock);
 kprocmodules_init(&result->p_modules);
 result->p_refcnt        = 1;
 result->p_fdman.fdm_cnt = 0;
 result->p_fdman.fdm_max = KFDMAN_FDMAX_TECHNICAL_MAXIMUM;
 result->p_fdman.fdm_fre = 0;
 result->p_fdman.fdm_fda = 0;
 result->p_fdman.fdm_fdv = NULL;
 if __unlikely(KE_ISERR(kproclist_addproc(result))) goto decref_result;
 result->p_fdman.fdm_root.fd_file = (struct kfile *)kdirfile_newroot();
 kprocenv_initroot(&result->p_environ);
 ktlsman_initroot(&result->p_tlsman);
 ktasklist_init(&result->p_threads);
 if __unlikely(!result->p_fdman.fdm_root.fd_file) {
  /* NOTE: We may have failed to open the root directory because there is no filesystem.
   *    >> If that's the case, it's not our job to handle it! */
  result->p_fdman.fdm_root.fd_type =
  result->p_fdman.fdm_cwd.fd_type = KFD_ATTR(KFDTYPE_FILE,KFD_FLAG_NONE);
  k_syslogf(KLOG_ERROR,"Failed to allocate KFD_ROOT/KFD_CWD descriptors\n");
 } else {
  assert(result->p_fdman.fdm_root.fd_file->f_refcnt == 1);
  ++result->p_fdman.fdm_root.fd_file->f_refcnt;
  result->p_fdman.fdm_root.fd_attr = KFD_ATTR(KFDTYPE_FILE,KFD_FLAG_NONE);
  result->p_fdman.fdm_cwd = result->p_fdman.fdm_root;
 }
 return result;
err_shm: kshm_quit(&result->p_shm);
err_sand: kprocperm_quit(&result->p_perm);
err_r: free(result);
 if (0) {decref_result: kproc_decref(result); result = NULL; }
 return NULL;
}

__crit kerrno_t
kproc_barrier(struct kproc *__restrict self,
              struct ktask *__restrict barrier,
              ksandbarrier_t mode) {
 struct ktask *oldtasks[KSANDBOX_BARRIER_COUNT];
 struct ktask **olditer = oldtasks;
 kerrno_t error; __u32 old_refs,max_refs,more_refs = 0;
 KTASK_CRIT_MARK
 kassert_kproc(self);
 kassert_ktask(barrier);
 if __unlikely(KE_ISERR(error = kproc_lock(self,KPROC_LOCK_PERM))) return error;
 if (mode&KSANDBOX_BARRIER_NOGETPROP) ++more_refs;
 if (mode&KSANDBOX_BARRIER_NOSETPROP) ++more_refs;
 if (mode&KSANDBOX_BARRIER_NOGETMEM)  ++more_refs;
 if (mode&KSANDBOX_BARRIER_NOSETMEM)  ++more_refs;
 max_refs = ((__u32)-1)-more_refs;
 do { /* Add the new references to the counter. */
  old_refs = katomic_load(barrier->t_refcnt);
  if (old_refs > max_refs) { error = KE_OVERFLOW; goto end; }
 } while (!katomic_cmpxch(barrier->t_refcnt,old_refs,old_refs+more_refs));
 if (mode&KSANDBOX_BARRIER_NOSETMEM)  *olditer++ = katomic_xch(self->p_perm.pp_smbarrier,barrier);
 if (mode&KSANDBOX_BARRIER_NOGETMEM)  *olditer++ = katomic_xch(self->p_perm.pp_gmbarrier,barrier);
 if (mode&KSANDBOX_BARRIER_NOSETPROP) *olditer++ = katomic_xch(self->p_perm.pp_spbarrier,barrier);
 if (mode&KSANDBOX_BARRIER_NOGETPROP) *olditer++ = katomic_xch(self->p_perm.pp_gpbarrier,barrier);
end:
 kproc_unlock(self,KPROC_LOCK_PERM);
 /* Drop references from all old barriers */
 while (olditer != oldtasks) ktask_decref(*--olditer);
 return KE_OK;
}

__crit __ref struct ktask *
kproc_getbarrier_r(struct kproc const *__restrict self,
                   ksandbarrier_t mode) {
 __ref struct ktask *result;
 if __unlikely(KE_ISERR(kproc_lock((struct kproc *)self,KPROC_LOCK_PERM))) return NULL;
 result = kprocperm_getbarrier(&self->p_perm,mode);
 if __unlikely(KE_ISERR(ktask_incref(result))) result = NULL;
 kproc_unlock((struct kproc *)self,KPROC_LOCK_PERM);
 return result;
}


__crit __ref struct kshlib *
kproc_getrootexe(struct kproc const *__restrict self) {
 __ref struct kshlib *result = NULL;
 struct kprocmodule *module;
 KTASK_CRIT_MARK
 kassert_kproc(self);
 if __unlikely(KE_ISERR(kproc_lock((struct kproc *)self,KPROC_LOCK_MODS))) return NULL;
 if ((module = kproc_getrootmodule_unlocked(self)) != NULL) {
  if __unlikely(KE_ISERR(kshlib_incref(result = module->pm_lib))) result = NULL; 
 } else result = NULL;
 kproc_unlock((struct kproc *)self,KPROC_LOCK_MODS);
 return result;
}
__nomp struct kprocmodule *
kproc_getrootmodule_unlocked(struct kproc const *__restrict self) {
 struct kprocmodule *iter,*end;
 kassert_kproc(self);
 end = (iter = self->p_modules.pms_modv)+self->p_modules.pms_moda;
 for (; iter != end; ++iter) if (iter->pm_lib) {
  if (iter->pm_lib->sh_flags&KMODFLAG_CANEXEC) return iter;
 }
 return NULL;
}


__DECL_END

#ifndef __INTELLISENSE__
#include "proc-attr.c.inl"
#include "proc-environ.c.inl"
#include "proc-exec.c.inl"
#include "proc-fdman.c.inl"
#include "proc-fork.c.inl"
#include "proc-initkernel.c.inl"
#include "proc-insmod.c.inl"
#include "proc-mod-syscall.c.inl"
#include "proc-pid.c.inl"
#include "proc-syscall.c.inl"
#include "proc-tasks.c.inl"
#endif

#endif /* !__KOS_KERNEL_SCHED_PROC_C__ */

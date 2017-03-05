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
#ifndef __KOS_KERNEL_SCHED_PROC_FORK_C_INL__
#define __KOS_KERNEL_SCHED_PROC_FORK_C_INL__ 1

#include <assert.h>
#include <kos/arch/x86/eflags.h>
#include <kos/compiler.h>
#include <kos/config.h>
#include <kos/kernel/debug.h>
#include <kos/kernel/gdt.h>
#include <kos/kernel/proc.h>
#include <kos/kernel/task.h>
#include <kos/kernel/pageframe.h>
#include <kos/kernel/linker.h>
#include <kos/kernel/shm.h>
#include <kos/kernel/fs/file.h>
#include <kos/errno.h>
#include <malloc.h>
#include <kos/syslog.h>
#include <sys/stat.h>

__DECL_BEGIN

__crit kerrno_t
kproc_copy4fork(__u32 flags, struct kproc *__restrict proc,
                __user void *eip, __ref struct kproc **presult) {
 __ref struct kproc *result; kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kproc(proc);
 if __unlikely((result = omalloc(struct kproc)) == NULL) return KE_NOMEM;
 kobject_init(result,KOBJECT_MAGIC_PROC);
 result->p_refcnt = 1;

 /* TODO: Initialize the LDT later in case we won't be able to do a root-fork. */
 error = kshm_initfork(&result->p_shm,&proc->p_shm);
 if __unlikely(KE_ISERR(error)) goto err_free;

 /* Check for root-fork permissions _AFTER_ we've already
  * created the SHM, just so we can ensure that there
  * are no race-conditions that would allow modification
  * of code allowed to perform a root-fork after we would
  * have otherwise checked for that permission.
  * >> Now that we've copied all memory (including the text memory),
  *    we can check if the calling code was modified, which would
  *    indicate that it was either modified before, or after we
  *    copied it, thus fixing all possible race conditions.
  */
 if ((flags&KTASK_NEW_FLAG_ROOTFORK) &&
     KE_ISERR(error = kproc_canrootfork(proc,eip))) goto err_shm;

 if __unlikely(KE_ISERR(error = kproc_lock(proc,KPROC_LOCK_MODS))) goto err_shm;
 error = kprocmodules_initcopy(&result->p_modules,&proc->p_modules);
 kproc_unlock(proc,KPROC_LOCK_MODS);
 if __unlikely(KE_ISERR(error)) goto err_shm;

 if __unlikely(KE_ISERR(error = kproc_lock(proc,KPROC_LOCK_FDMAN))) goto err_mod;
 error = kfdman_initcopy(&result->p_fdman,&proc->p_fdman);
 kproc_unlock(proc,KPROC_LOCK_FDMAN);
 if __unlikely(KE_ISERR(error)) goto err_mod;
 assert(result->p_fdman.fdm_cnt <= result->p_fdman.fdm_fda);

 if (flags&KTASK_NEW_FLAG_ROOTFORK) {
  /* Perform a root-fork (grant all permissions to new process). */
  error = kprocperm_initroot(&result->p_perm);
 } else {
  if __unlikely(KE_ISERR(error = kproc_lock(proc,KPROC_LOCK_PERM))) goto err_fdman;
  error = kprocperm_initcopy(&result->p_perm,&proc->p_perm);
  kproc_unlock(proc,KPROC_LOCK_PERM);
 }
 if __unlikely(KE_ISERR(error)) goto err_fdman;

 if __unlikely(KE_ISERR(error = kproc_lock(proc,KPROC_LOCK_TLSMAN))) goto err_sand;
 error = ktlsman_initcopy(&result->p_tlsman,&proc->p_tlsman);
 kproc_unlock(proc,KPROC_LOCK_TLSMAN);
 if __unlikely(KE_ISERR(error)) goto err_sand;

 if __unlikely(KE_ISERR(error = kproc_lock(proc,KPROC_LOCK_ENVIRON))) goto err_sand;
 error = kprocenv_initcopy(&result->p_environ,&proc->p_environ);
 kproc_unlock(proc,KPROC_LOCK_ENVIRON);
 if __unlikely(KE_ISERR(error)) goto err_tls;

 /* Initialize misc noexcept parts. */
 memcpy(&result->p_regs,&proc->p_regs,sizeof(struct kprocregs));
 ktasklist_init(&result->p_threads);
 kmmutex_init(&result->p_lock);

 if __unlikely(KE_ISERR(kproclist_addproc(result))) goto err_env;
 *presult = result;
 return error;
err_env:   kprocenv_quit(&result->p_environ);
err_tls:   ktlsman_quit(&result->p_tlsman);
err_sand:  kprocperm_quit(&result->p_perm);
err_fdman: kfdman_quit(&result->p_fdman);
err_mod:   kprocmodules_quit(&result->p_modules);
err_shm:   kshm_quit(&result->p_shm);
err_free:  free(result);
 return error;
}

__local __crit __ref struct ktask *
ktask_copy4fork(struct ktask *__restrict self,
                struct ktask *__restrict parent,
                struct kproc *__restrict proc,
                struct kirq_userregisters const *__restrict userregs) {
 __ref struct ktask *result;
 size_t kstacksize;
 KTASK_CRIT_MARK
 kassert_ktask(self); kassert_kproc(proc); kassertobj(userregs);
 assert(self->t_flags&KTASK_FLAG_USERTASK);
 assert(!(self->t_flags&KTASK_FLAG_MALLKSTACK));
 assert(krwlock_iswritelocked(&proc->p_shm.s_lock));
 if __unlikely(KE_ISERR(ktask_incref(parent))) return NULL;
 if __unlikely(KE_ISERR(kproc_incref(proc))) goto err_taskref;
 if __unlikely((result = omalloc(struct ktask)) == NULL) goto err_procref;
 if __unlikely(KE_ISERR(ktlspt_initcopy(&result->t_tls,&self->t_tls))) goto err_free;
 kobject_init(result,KOBJECT_MAGIC_TASK);
 result->t_epd         =
 result->t_userpd      = kproc_getpagedir(proc);
 result->t_refcnt      = 1;
 result->t_locks       = 0;
 result->t_newstate    = KTASK_STATE_RUNNING;
 result->t_state       = KTASK_STATE_SUSPENDED;
#if defined(KTASK_FLAG_CRITICAL) && defined(KTASK_FLAG_NOINTR)
 result->t_flags       = self->t_flags&~(KTASK_FLAG_CRITICAL|KTASK_FLAG_NOINTR);
#elif defined(KTASK_FLAG_CRITICAL)
 result->t_flags       = self->t_flags&~(KTASK_FLAG_CRITICAL);
#elif defined(KTASK_FLAG_NOINTR)
 result->t_flags       = self->t_flags&~(KTASK_FLAG_NOINTR);
#else
 result->t_flags       = self->t_flags;
#endif
 result->t_cpu         = NULL;
#ifdef __DEBUG__
 result->t_prev        = NULL;
 result->t_next        = NULL;
#endif
 result->t_suspended   = 1;
 result->t_sigval      = NULL;
 result->t_setpriority = katomic_load(self->t_setpriority);
 result->t_curpriority = result->t_setpriority;
 result->t_proc        = proc; /* Inherit reference. */
 result->t_parent      = parent; /* Inherit reference. */
 /* The user-stack was already copied when forking the SHM. */
 result->t_ustackvp    = self->t_ustackvp;
 result->t_ustacksz    = self->t_ustacksz;
 /* Since the kernel stack isn't addressable from ring-3, it
  * wasn't copied during the forking of the page directory.
  * >> That is good! So now lets manually allocate a new stack. */
 kstacksize = ktask_getkstacksize(self);
 /* Map linear memory for the kernel stack. */
 if __unlikely(KE_ISERR(kshm_mapram_linear_unlocked(kproc_getshm(proc),
                                                   &result->t_kstack,
                                                   &result->t_kstackvp,
                                                    ceildiv(kstacksize,PAGESIZE),
                                                    KPAGEDIR_MAPANY_HINT_KSTACK,
                                                    KSHMREGION_FLAG_LOSEONFORK|
                                                    KSHMREGION_FLAG_RESTRICTED
               ))) goto err_tls;
 result->t_kstackend  = (__kernel void *)((uintptr_t)result->t_kstack+kstacksize);
 // Make sure the linear memory was allocated correctly
 assert(kstacksize == ktask_getkstacksize(result));
 kassertmem(result->t_kstack,kstacksize);

#if KCONFIG_HAVE_TASK_NAMES
 result->t_name = NULL;
#endif /* KCONFIG_HAVE_TASK_NAMES */

 /* todo: minor: Shouldn't the resulting task wait for the same signals? */
 ktasksig_init(&result->t_sigchain,result);
 ksignal_init(&result->t_joinsig);
 ktasklist_init(&result->t_children);
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

 /* Setup the stack pointer to point to the new kernel stack */
 result->t_esp = (__user void *)((uintptr_t)result->t_kstackvp+kstacksize);
 result->t_esp0 = result->t_esp;
 assertf(kpagedir_ismappedp(result->t_userpd,
         (void *)alignd((uintptr_t)result->t_esp0-1,PAGEALIGN))
        ,"Kernel stack address %p is not mapped",result->t_esp0);

 {
  struct ktaskregisters3 regs;
  regs.base.ds     = userregs->ds;
#if KCONFIG_HAVE_I386_SAVE_SEGMENT_REGISTERS
  regs.base.es     = userregs->es;
  regs.base.fs     = userregs->fs;
  regs.base.gs     = userregs->gs;
#endif
  regs.base.edi    = userregs->edi;
  regs.base.esi    = userregs->esi;
  regs.base.ebp    = userregs->ebp;
  regs.base.ebx    = userregs->ebx;
  regs.base.edx    = userregs->edx;
  regs.base.ecx    = userregs->ecx;
  regs.base.eax    = KE_OK; /* Return 0 in EAX for child task. */
  regs.base.eip    = userregs->eip;
  regs.base.cs     = userregs->cs;
#ifdef KARCH_X86_EFLAGS_IF
  regs.base.eflags = userregs->eflags|KARCH_X86_EFLAGS_IF;
#else
  regs.base.eflags = userregs->eflags;
#endif
  /* Use the same ESP address as originally set when 'fork()' was called. */
  regs.useresp     = userregs->useresp;
  regs.ss          = userregs->ss;
  ktask_stackpush_sp_unlocked(result,&regs,sizeof(regs));
 }

 ktask_lock(self,KTASK_LOCK_CHILDREN);
 result->t_parid = ktasklist_insert(&self->t_children,result);
 ktask_unlock(self,KTASK_LOCK_CHILDREN);
 if __unlikely(result->t_parid == (size_t)-1) goto err_kstack;
 if __unlikely(KE_ISERR(kproc_addtask(proc,result))) goto err_parid;
 return result;
err_parid:
 ktask_lock(self,KTASK_LOCK_CHILDREN);
 asserte(ktasklist_erase(&self->t_children,result->t_parid) == result);
 ktask_unlock(self,KTASK_LOCK_CHILDREN);
err_kstack:  kshm_unmap(&proc->p_shm,result->t_kstackvp,
                        ceildiv(kstacksize,PAGESIZE),
                        KSHMUNMAP_FLAG_RESTRICTED);
err_tls:     ktlspt_quit(&result->t_tls);
err_free:    free(result);
err_procref: kproc_decref(proc);
err_taskref: ktask_decref(self);
 return NULL;
}
__crit kerrno_t
ktask_fork(__u32 flags, struct kirq_userregisters const *__restrict userregs,
           __ref struct ktask **presult) {
 kerrno_t error; __ref struct kproc *newproc;
 struct ktask *newtask,*task_self = ktask_self();
 KTASK_CRIT_MARK
 error = kproc_copy4fork(flags,ktask_getproc(task_self),
                        (__user void *)userregs->eip,&newproc);
 if __unlikely(KE_ISERR(error)) return error;
 error = krwlock_beginwrite(&newproc->p_shm.s_lock);
 if __unlikely(KE_ISERR(error)) goto err_proc;
 if (flags&KTASK_NEW_FLAG_UNREACHABLE) {
  /* Spawn the task as unreachable */
  if (flags&KTASK_NEW_FLAG_ROOTFORK) {
   /* Spawn it off of the kernel zero-task. */
   newtask = ktask_copy4fork(task_self,ktask_zero(),newproc,userregs);
  } else {
   /* Spawn it off of the setmem barrier. */
   struct ktask *parent = kproc_getbarrier_r(ktask_getproc(task_self),
                                             KSANDBOX_BARRIER_NOSETMEM);
   if __unlikely(!parent) { error = KE_DESTROYED; goto err_proc_unlock; }
   newtask = ktask_copy4fork(task_self,parent,newproc,userregs);
   ktask_decref(parent);
  }
 } else {
  newtask = ktask_copy4fork(task_self,task_self,newproc,userregs);
 }
 krwlock_endwrite(&newproc->p_shm.s_lock);
 if __unlikely(!newtask) { error = KE_NOMEM; goto err_proc; }
 assert(newtask != ktask_self());
 kproc_decref(newproc);
 *presult = newtask;
 return error;
err_proc_unlock: krwlock_endwrite(&newproc->p_shm.s_lock);
err_proc: kproc_decref(newproc);
 return error;
}


__crit kerrno_t kproc_canrootfork_c(struct kproc *__restrict self, __user void *eip) {
 kerrno_t error = KE_NOENT;
 struct kprocmodule *iter,*end;
 struct kshlibsection *sec_iter,*sec_end;
 kassert_kproc(self);
 error = kproc_locks(self,KPROC_LOCK_MODS);
 if __unlikely(KE_ISERR(error)) return error;
 error = krwlock_beginwrite(&self->p_shm.s_lock);
 if __unlikely(KE_ISERR(error)) goto end_mods;
 /* Figure out which module the given EIP belongs to (if any). */
 end = (iter = self->p_modules.pms_modv)+self->p_modules.pms_moda;
 for (; iter != end; ++iter) if (iter->pm_lib && (uintptr_t)eip >= (uintptr_t)iter->pm_base) {
  uintptr_t offset_eip = (uintptr_t)eip-(uintptr_t)iter->pm_base;
  sec_end = (sec_iter = iter->pm_lib->sh_data.ed_secv)+iter->pm_lib->sh_data.ed_secc;
  for (; sec_iter != sec_end; ++sec_iter) {
   if (offset_eip >= sec_iter->sls_albase &&
       offset_eip < sec_iter->sls_base+sec_iter->sls_size) {
    /* Found the section the given EIP. */
    /* #1: Make sure that the memory tab was declared as executable and read-only. */
    if ((kshmregion_getflags(sec_iter->sls_region)&
        (KSHMREGION_FLAG_WRITE|KSHMREGION_FLAG_EXEC)) !=
                              (KSHMREGION_FLAG_EXEC)) {
     k_syslogf_prefixfile(KLOG_WARN,iter->pm_lib->sh_file
                         ,"[ROOT-FORK] Denying root-fork request from non-executable "
                          "address %p in memory tab %p..%p (%Iu bytes)\n"
                         ,eip
                         ,(uintptr_t)iter->pm_base+sec_iter->sls_albase
                         ,(uintptr_t)iter->pm_base+sec_iter->sls_base+sec_iter->sls_size
                         ,(sec_iter->sls_base-sec_iter->sls_albase)+sec_iter->sls_size);
     error = KE_NOEXEC;
     goto end;
    }
    
    /* Make sure that the memory wasn't modified. */
    {
     /* TODO: Just to be sure, we should check all pages of all RO SHM sections for modifications.
      *       While there should really be no way of user-code modifying memory once its been
      *       mapped as read-only (in the spirit of the KOS design of permissions only be losable),
      *       we can't exclude the possibility that once there are kernel modules, some ~genius~
      *       might write a backdoor that allows usercode to write memory using the same utilities
      *       relocations use for modifying read-only (aka. text) memory during dynamic library loading.
      */
     x86_pde used_pde; x86_pte used_pte;
     kassertobj(self);
     used_pde = kproc_getpagedir(self)->d_entries[X86_VPTR_GET_PDID(eip)];
     if __unlikely(!used_pde.present) goto cont; /* Page table not present */
     used_pte = x86_pde_getpte(&used_pde)[X86_VPTR_GET_PTID(eip)];
     if __unlikely(!used_pte.present) goto cont; /* Page table entry not present */
     if (used_pte.dirty) {
      k_syslogf_prefixfile(KLOG_WARN,iter->pm_lib->sh_file,
                           "[ROOT-FORK] Denying root-fork request by modified address %p\n",eip);
      error = KE_CHANGED;
      goto end;
     }
    }

    /* Make sure that the memory associated with EIP isn't writable,
     * as otherwise we can't guaranty that the code we've granting
     * root-access to will remain unmodified. */
    {
     struct kshmbranch *branch;
     branch = kshmmap_locate(&self->p_shm.s_map,(uintptr_t)eip);
     if (!branch) {
      /* If we don't know this branch. */
      k_syslogf_prefixfile(KLOG_WARN,iter->pm_lib->sh_file
                          ,"[ROOT-FORK] Denying root-fork request at %p "
                           "(No associated SHM region)\n"
                          ,eip,branch->sb_map_min,branch->sb_map_max);
      error = KE_EXISTS;
      goto end;
     }
     kassertobj(branch);
     kassert_kshmregion(branch->sb_region);
     if ((kshmregion_getflags(branch->sb_region) &
         (KSHMREGION_FLAG_WRITE)) == (KSHMREGION_FLAG_WRITE)) {
      /* The SHM mapping of the calling EIP is writable. */
      k_syslogf_prefixfile(KLOG_WARN,iter->pm_lib->sh_file
                          ,"[ROOT-FORK] Denying root-fork request at %p "
                          "(shared-writable SHM mapping %p..%p)\n"
                          ,eip,branch->sb_map_min,branch->sb_map_max);
      error = KE_WRITABLE;
      goto end;
     }
    }

    /* Make sure that the module has the SETUID bit set. */
    {
     mode_t module_bits;
     error = kfile_kernel_getattr(iter->pm_lib->sh_file,KATTR_FS_PERM,&module_bits,sizeof(__mode_t),NULL);
     if __unlikely(KE_ISERR(error)) goto end;
     if (!(module_bits&S_ISUID)) {
      k_syslogf_prefixfile(KLOG_WARN,iter->pm_lib->sh_file
                          ,"[ROOT-FORK] Denying root-fork request by non-SETUID module at %p\n",eip);
      error = KE_ACCES;
      goto end;
     }
    }

    /* Yes. Everything checks out and you're allowed to root-fork()! */
    error = KE_OK;
    goto end;
   }
  }
cont:;
 }
end:      krwlock_endwrite(&self->p_shm.s_lock);
end_mods: kproc_unlocks(self,KPROC_LOCK_MODS);
 return error;
}


__DECL_END

#endif /* !__KOS_KERNEL_SCHED_PROC_FORK_C_INL__ */

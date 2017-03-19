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
#ifndef __KOS_KERNEL_SCHED_PROC_EXEC_C_INL__
#define __KOS_KERNEL_SCHED_PROC_EXEC_C_INL__ 1

#include <kos/config.h>
#include <kos/compiler.h>
#include <kos/kernel/proc.h>
#include <kos/kernel/linker.h>
#include <kos/syslog.h>
#include <kos/task.h>

__DECL_BEGIN

__local void kproc_delmods(struct kproc *self) {
 kmodid_t i;
 // TODO: Must actually check dependencies to only unload
 //       modules that other modules don't depend on,
 //       thus creating a safe unload order.
 for (i = 0; i < self->p_modules.pms_moda; ++i) {
  kproc_delmod_unlocked(self,i);
 }
}

#ifdef __DEBUG__
#define CLOSE_IF_CLOEXEC(x) \
 if ((x)->fd_flag&KFD_FLAG_CLOEXEC) {\
  kfdentry_quit(x);\
  (x)->fd_type = KFDTYPE_NONE;\
  (x)->fd_data = NULL;\
 }
#else
#define CLOSE_IF_CLOEXEC(x) \
 if ((x)->fd_flag&KFD_FLAG_CLOEXEC) {\
  kfdentry_quit(x);\
  (x)->fd_type = KFDTYPE_NONE;\
 }
#endif
__local void kfdman_close_cloexec(struct kfdman *self) {
 struct kfdentry *iter,*end;
 unsigned int new_alloc;
 KFDMAN_FOREACH_SPECIAL(self,CLOSE_IF_CLOEXEC);
 end = (iter = self->fdm_fdv)+self->fdm_fda;
 for (; iter != end; ++iter) {
  if (iter->fd_type != KFDTYPE_NONE &&
      iter->fd_flag&KFD_FLAG_CLOEXEC) {
   assert(self->fdm_cnt);
   kfdentry_quit(iter);
   iter->fd_type = KFDTYPE_NONE;
#ifdef __DEBUG__
   iter->fd_data = NULL;
#endif
   --self->fdm_cnt;
  }
 }
 new_alloc = self->fdm_fda;
 while (new_alloc && self->fdm_fdv[new_alloc-1].fd_type == KFDTYPE_NONE) --new_alloc;
 if (new_alloc != self->fdm_fda) {
  assert((self->fdm_cnt != 0) == (new_alloc != 0));
  if (!new_alloc) {
   free(self->fdm_fdv);
   self->fdm_fdv = NULL;
  } else {
   iter = (struct kfdentry *)realloc(self->fdm_fdv,new_alloc*
                                     sizeof(struct kfdentry));
   if __likely(iter) self->fdm_fdv = iter;
  }
  self->fdm_fda = new_alloc;
  assertf(new_alloc >= self->fdm_cnt
         ,"new_alloc     = %u\n"
          "self->fdm_cnt = %u\n"
         ,new_alloc,self->fdm_cnt);
 }
}

__local kerrno_t
kprocenv_init_from_execargs(struct kprocenv *self, size_t max_mem,
                            struct kexecargs *args, int args_members_are_kernel) {
 kerrno_t error;
 kobject_initzero(self,KOBJECT_MAGIC_PROCENV);
 kassertobj(args);
 self->pe_memmax = max_mem;
 error = args_members_are_kernel
  ? kprocenv_setargv  (self,args->ea_argc,args->ea_argv,args->ea_arglenv)
  : kprocenv_setargv_u(self,args->ea_argc,args->ea_argv,args->ea_arglenv);
 if __unlikely(KE_ISERR(error)) return error;
 args->ea_environ = NULL;
 // TODO: Initialize environ
 return error;
}


__crit kerrno_t
kproc_exec(struct kshlib *__restrict exec_main,
           struct kirq_userregisters *__restrict userregs,
           struct kexecargs *args, int args_members_are_kernel) {
 struct ktask *root; kerrno_t error; kmodid_t modid;
 struct ktask *caller_thread = ktask_self();
 struct kproc *self = caller_thread->t_proc;
 struct kprocenv newenv;
 KTASK_CRIT_MARK
 assertf(ktask_isnointr(),
         "exec() can't handle KE_INTR-style interrupts.\n"
         "Call this function from within a ktask_crit_begin_nointr()-block.");
 assertf(self != kproc_kernel(),"Can't exec in kernel process.");
 if __unlikely(!(exec_main->sh_flags&KMODFLAG_CANEXEC)) {
  return KE_NOEXEC; /* No entry point found */
 }

 /* Create an environment, based on what is described by the given arguments. */
 error = kprocenv_init_from_execargs(&newenv,self->p_environ.pe_memmax,
                                     args,args_members_are_kernel);
 if __unlikely(KE_ISERR(error)) return error;

 /* Handle shebang binary:
  * >> Recursively prepend additional shebang
  *    arguments and search for the actual binary. */
 while (KMODKIND_KIND(exec_main->sh_flags) == KMODKIND_SHEBANG) {
  error = kprocenv_prepend_argv(&newenv,            exec_main->sh_sb_args.sb_argc,
                               (char const *const *)exec_main->sh_sb_args.sb_argv);
  if __unlikely(KE_ISERR(error)) { kprocenv_quit(&newenv); return error; }
  exec_main = exec_main->sh_sb_ref;
 }

 if ((root = kproc_getroottask(self)) != NULL) {
  /* Terminate all threads in the current process, except for the calling one. */
  ktask_terminateex_k(root,NULL,KTASKOPFLAG_RECURSIVE|
                      KTASKOPFLAG_ASYNC|KTASKOPFLAG_NOCALLER);
  ktask_decref(root);
 }
 /* NOTE: The process should not be a zombie at this point,
  *       as we (being a critical task) are still alive. */
 error = kproc_locks(self,KPROC_LOCK_MODS|KPROC_LOCK_TLSMAN|KPROC_LOCK_ENVIRON);
 assertf(KE_ISOK(error),"Why did this fail? %d",error);
 error = krwlock_beginwrite(&self->p_shm.s_lock);
 assertf(KE_ISOK(error),"Why did this fail? %d",error);

 kproc_delmods(self);
 assert(!self->p_modules.pms_moda);

 /* Delete all user-level memory task, but exclude the tab
  * associated with the user-stack of the calling thread.
  * >> This way, we can simply re-use its stack and don't
  *    have to worry about failing to allocate a new one below.
  * >> This also gives user-level code more control by allowing them
  *    to specify a custom stack to be used for exec-ed processes. */
 kshm_unmap_unlocked(kproc_getshm(self),NULL,
                    ((size_t)caller_thread->t_ustackvp)/PAGESIZE,
                     KSHMUNMAP_FLAG_NONE);
 kshm_unmap_unlocked(kproc_getshm(self),
                    (void *)((uintptr_t)caller_thread->t_ustackvp+caller_thread->t_ustacksz),
                    ((uintptr_t)0-((uintptr_t)caller_thread->t_ustackvp+caller_thread->t_ustacksz))/PAGESIZE,
                     KSHMUNMAP_FLAG_NONE);
 assertf(kpagedir_ismapped(kproc_getpagedir(self),caller_thread->t_ustackvp,
                           ceildiv(ktask_getustacksize(caller_thread),PAGESIZE)),
         "Be we explicitly excluded the user-stack...");
 assertf(kpagedir_ismapped(kproc_getpagedir(self),caller_thread->t_kstackvp,
                           ceildiv(ktask_getkstacksize(caller_thread),PAGESIZE)),
         "But the kernel stack should have been restricted...");

 // Clear TLS Variables (I don't know if we really need to do this, but it feels right...)
 ktlsman_clear(&self->p_tlsman);

 // Close all file descriptors marked with FD_CLOEXEC
 error = kproc_lock(self,KPROC_LOCK_FDMAN);
 assertf(KE_ISOK(error),"Why did this fail? %d",error);
 kfdman_close_cloexec(&self->p_fdman);
 kproc_unlock(self,KPROC_LOCK_FDMAN);

 /* Make sure we're not loading a shebang script. */
 assert(KMODKIND_KIND(exec_main->sh_flags) != KMODKIND_SHEBANG);

 // Load the given module into the current process.
 // TODO: Everything that could potentially fail about this
 //       _MUST_ be done before we start killing off existing
 //       data.
 // NOTE: Though we can't just load the module before unloading
 //       the existing modules in order to prevent the addresses
 //       from clashing.
 k_syslogf_prefixfile(KLOG_DEBUG,exec_main->sh_file,"[exec] Installing main module\n");

 error = kproc_insmod_unlocked(self,exec_main,&modid);
 assertf(KE_ISOK(error)
        ,"TODO: We must (fail to) do this before destroying the existing process... %d"
        ,error);

 k_syslogf_prefixfile(KLOG_DEBUG,exec_main->sh_file,"[exec] Installing new environment\n");
 /* Install the new environment */
 kprocenv_install_after_exec(&self->p_environ,&newenv,
                             args->ea_environ == NULL);

 // Overwrite register values.
 // todo: Should we reset general purpose registers here, or
 //       just follow semantics of leaving them up to the calling
 //       process?
 //       Doing it like we are now does allow some cool hacking,
 //       like implementing secret handshakes when launching
 //       applications.
 // e.g.: Unless ESI is 0xDEADBEEF upon start of your app, exit immediately.
 userregs->useresp = (uintptr_t)caller_thread->t_ustackvp+caller_thread->t_ustacksz;
 userregs->eip     = (uintptr_t)self->p_modules.pms_modv[modid].pm_base+
                     (uintptr_t)exec_main->sh_callbacks.slc_start;
 // Unlock all locks that we're still holding
 krwlock_endwrite(&self->p_shm.s_lock);
 kproc_unlocks(self,KPROC_LOCK_MODS|KPROC_LOCK_TLSMAN|KPROC_LOCK_ENVIRON);
 return error;
}


__DECL_END

#endif /* !__KOS_KERNEL_SCHED_PROC_EXEC_C_INL__ */

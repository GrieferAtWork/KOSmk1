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
#ifndef __KOS_KERNEL_SYSCALL_SYSCALL_MOD_C_INL__
#define __KOS_KERNEL_SYSCALL_SYSCALL_MOD_C_INL__ 1

#include "syscall-common.h"
#include <kos/kernel/linker.h>
#include <kos/kernel/proc.h>
#include <kos/kernel/fs/file.h>
#include <kos/kernel/util/string.h>

__DECL_BEGIN

/* _syscall4(kerrno_t,kmod_open,char const *,name,size_t,namemax,kmodid_t *,modid,__u32,flags); */
SYSCALL(sys_kmod_open) {
 LOAD4(char const *,U(name),
       size_t      ,K(namemax),
       kmodid_t   *,K(modid),
       __u32       ,K(flags));
 kerrno_t error; struct kshlib *lib;
 kmodid_t resid; struct kproc *proc_self = kproc_self();
 KTASK_CRIT_BEGIN_FIRST
 (void)flags; // TODO
 error = kshlib_openlib(name,namemax,&lib);
 if __unlikely(KE_ISERR(error)) goto end;
 error = kproc_insmod(proc_self,lib,&resid);
 kshlib_decref(lib);
 if (__likely(KE_ISOK(error)) &&
     __unlikely(copy_to_user(modid,&resid,sizeof(resid)))) {
  kproc_delmod(proc_self,resid);
  error = KE_FAULT;
 }
end:
 KTASK_CRIT_END
 RETURN(error);
}

/* _syscall3(kerrno_t,kmod_fopen,int,fd,kmodid_t *,modid,__u32,flags); */
SYSCALL(sys_kmod_fopen) {
 LOAD3(int         ,K(fd),
       kmodid_t   *,K(modid),
       __u32       ,K(flags));
 kerrno_t error; struct kshlib *lib; struct kfile *fp;
 kmodid_t resid; struct kproc *proc_self = kproc_self();
 KTASK_CRIT_BEGIN_FIRST
 fp = kproc_getfdfile(proc_self,fd);
 if __unlikely(!fp) { error = KE_BADF; goto end; }
 (void)flags; /* As of now unused. */
 error = kshlib_fopenfile(fp,&lib);
 kfile_decref(fp);
 if __unlikely(KE_ISERR(error)) goto end;
 error = kproc_insmod(proc_self,lib,&resid);
 kshlib_decref(lib);
 if (__likely(KE_ISOK(error)) &&
     __unlikely(copy_to_user(modid,&resid,sizeof(resid)))) {
  kproc_delmod(proc_self,resid);
  error = KE_FAULT;
 }
end:
 KTASK_CRIT_END
 RETURN(error);
}

/* _syscall1(kerrno_t,kmod_close,kmodid_t,modid); */
SYSCALL(sys_kmod_close) {
 LOAD1(kmodid_t,K(modid));
 kerrno_t error;
 KTASK_CRIT_BEGIN_FIRST
 error = kproc_delmod2(kproc_self(),modid,
                      (__user void *)regs->regs.eip);
 KTASK_CRIT_END
 RETURN(error);
}

/* _syscall3(void *,kmod_sym,kmodid_t,modid,char const *,name,size_t,namemax); */
SYSCALL(sys_kmod_sym) {
 LOAD3(kmodid_t    ,K(modid),
       char const *,U(name),
       size_t      ,K(namemax));
 void *result; namemax = strnlen(name,namemax);
 result = kproc_dlsymex2(kproc_self(),modid,name,namemax,
                         ksymhash_of(name,namemax),
                        (__user void *)regs->regs.eip);
 RETURN(result);
}

/* _syscall6(kerrno_t,kmod_info,int,proc,kmodid_t,modid,struct kmodinfo *,buf,size_t,bufsize,size_t *,reqsize,__u32,flags); */
SYSCALL(sys_kmod_info) {
 LOAD6(int              ,K(procfd),
       kmodid_t         ,K(modid),
       struct kmodinfo *,K(buf),
       size_t           ,K(bufsize),
       size_t          *,K(preqsize),
       __u32            ,K(flags));
 kerrno_t error; size_t reqsize;
 struct kproc *proc,*caller = kproc_self();
 struct kprocmodule *module;
 struct kfdentry fd;
 if (!buf) bufsize = 0;
 KTASK_CRIT_BEGIN_FIRST
 if __unlikely(KE_ISERR(error = kproc_getfd(caller,procfd,&fd))) goto end;
      if (fd.fd_type == KFDTYPE_PROC) proc = fd.fd_proc;
 else if (fd.fd_type == KFDTYPE_TASK) proc = ktask_getproc(fd.fd_task);
 else { error = KE_BADF; goto end_fd; }
 if __unlikely(KE_ISERR(error = kproc_lock(proc,KPROC_LOCK_MODS))) goto end_fd;
 if (modid == KMODID_SELF) {
  /* symbolic module ID self is only allowed if procfd also described self. */
  if __unlikely(caller != proc) { error = KE_INVAL; goto end_fd; }
  error = kproc_getmodat_unlocked(proc,&modid,(void *)regs->regs.eip);
  if __unlikely(KE_ISERR(error)) goto end_unlock;
  module = kproc_getrootmodule_unlocked(proc);
  if __unlikely(!module) { error = KE_DESTROYED; goto end_fd; }
  goto parse_module;
 }
 if __unlikely(modid >= proc->p_modules.pms_moda) error = KE_INVAL;
 else if __unlikely(!(module = &proc->p_modules.pms_modv[modid])->pm_lib) error = KE_INVAL;
 else {
parse_module:
  error = kshlib_user_getinfo(module->pm_lib,module->pm_base,
                             (kmodid_t)(module-proc->p_modules.pms_modv),
                              buf,bufsize,preqsize ? &reqsize : NULL,flags);
 }
end_unlock:
 kproc_unlock(proc,KPROC_LOCK_MODS);
 if (__likely(KE_ISOK(error)) && preqsize &&
     __unlikely(copy_to_user(preqsize,&reqsize,sizeof(reqsize)))
     ) error = KE_FAULT;
end_fd: kfdentry_quit(&fd);
end: KTASK_CRIT_END
 RETURN(error);
}



__DECL_END

#endif /* !__KOS_KERNEL_SYSCALL_SYSCALL_MOD_C_INL__ */

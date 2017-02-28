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
#include <kos/kernel/fs/fs.h>
#include <kos/kernel/fs/file.h>
#include <kos/kernel/util/string.h>
#include <sys/types.h>
#include <stddef.h>

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

/* _syscall6(kerrno_t,kmod_info,int,procfd,kmodid_t,modid,struct kmodinfo *,buf,size_t,bufsize,size_t *,reqsize,__u32,flags); */
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
 if __unlikely(modid >= proc->p_modules.pms_moda) {
  if (modid == KMODID_SELF) {
   /* symbolic module ID self is only allowed if procfd also described self. */
   if __unlikely(caller != proc) { error = KE_INVAL; goto end_fd; }
   error = kproc_getmodat_unlocked(proc,&modid,(void *)regs->regs.eip);
   if __unlikely(KE_ISERR(error)) goto end_unlock;
   module = kproc_getrootmodule_unlocked(proc);
   if __unlikely(!module) { error = KE_DESTROYED; goto end_fd; }
   goto parse_module;
  }
  error = KE_INVAL;
 } else if __unlikely(!(module = &proc->p_modules.pms_modv[modid])->pm_lib) error = KE_INVAL;
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





static __crit struct ksymbol const *
proc_find_symbol_by_addr(struct kproc *self,
                         void const *sym_address,
                         struct kprocmodule **module,
                         struct kprocmodule *exclude_module,
                         __u32 *flags) {
 struct kprocmodule *iter,*end; ksymaddr_t reladdr;
 struct ksymbol const *result; struct kshlib *lib;
 KTASK_CRIT_MARK
 assert(kproc_islocked(self,KPROC_LOCK_MODS));
 end = (iter = self->p_modules.pms_modv)+self->p_modules.pms_moda;
 for (; iter != end; ++iter) if ((lib = iter->pm_lib) != NULL && iter != exclude_module &&
                                (uintptr_t)sym_address >= (uintptr_t)iter->pm_base) {
  reladdr = (ksymaddr_t)sym_address-(ksymaddr_t)iter->pm_base;
  if (reladdr >= lib->sh_data.ed_begin &&
      reladdr <  lib->sh_data.ed_end) {
   result = kshlib_get_closest_symbol(lib,reladdr,flags);
   if (result) { *module = iter; return result; }
  }
 }
 return NULL;
}

static __crit struct ksymbol const *
proc_find_symbol_by_name(struct kproc *self, char const *name,
                         size_t name_size, size_t name_hash,
                         struct kprocmodule **module,
                         struct kprocmodule *exclude_module,
                         __u32 *flags) {
 struct kprocmodule *iter,*end;
 struct ksymbol const *result;
 KTASK_CRIT_MARK
 assert(kproc_islocked(self,KPROC_LOCK_MODS));
 end = (iter = self->p_modules.pms_modv)+self->p_modules.pms_moda;
 for (; iter != end; ++iter) if (iter->pm_lib && iter != exclude_module) {
  result = kshlib_get_any_symbol(iter->pm_lib,name,name_size,name_hash,flags);
  if (result) { *module = iter; return result; }
 }
 return NULL;
}

static __crit kerrno_t
ksymname_copyfromuser(struct ksymname *dst,
                      struct ksymname const *src) {
 KTASK_CRIT_MARK
 if __unlikely(copy_from_user(dst,src,sizeof(struct ksymname))) return KE_FAULT;
 dst->sn_size = min(dst->sn_size,4096);
 dst->sn_name = user_strndup(dst->sn_name,dst->sn_size);
 if __unlikely(!dst->sn_name) return KE_NOMEM;
 dst->sn_size = strnlen(dst->sn_name,dst->sn_size);
 return KE_OK;
}

static __crit kerrno_t
ksymbol_fillinfo(struct kproc *__restrict proc,
                 struct kprocmodule *__restrict module,
                 struct ksymbol const *__restrict symbol,
                 ksymaddr_t effective_addr,
                 __user struct ksyminfo *buf,
                 size_t bufsize, size_t *__restrict reqsize,
                 __u32 flags, __u32 symbol_flags) {
 struct ksyminfo info; byte_t *buf_end; kerrno_t error;
 size_t copy_size,bufavail,bufreq,info_size;
 KTASK_CRIT_MARK
      if (flags&KMOD_SYMINFO_FLAG_WANTFILE) copy_size = offsetafter(struct ksyminfo,si_flsz);
 else if (flags&KMOD_SYMINFO_FLAG_WANTNAME) copy_size = offsetafter(struct ksyminfo,si_nmsz);
 else goto dont_copy_input;
 if __unlikely(copy_from_user(&info,buf,copy_size)) return KE_FAULT;
dont_copy_input:
 /* Fill in generic information, such as base and size. */
 info.si_flags = symbol_flags;
 info.si_modid = (kmodid_t)(module-proc->p_modules.pms_modv);
 if (symbol) {
  info.si_base   = (void *)((uintptr_t)module->pm_base+symbol->s_addr);
  info.si_flags |= KSYMINFO_FLAG_BASE;
  if ((info.si_size = symbol->s_size) != 0) info.si_flags |= KSYMINFO_FLAG_SIZE;
 } else {
  info.si_base = NULL;
  info.si_size = 0;
 }

 /* Figure out where the buffer of what the user requested ends. */
      if (flags&KMOD_SYMINFO_FLAG_WANTLINE) info_size = offsetafter(struct ksyminfo,si_line);
 else if (flags&KMOD_SYMINFO_FLAG_WANTFILE) info_size = offsetafter(struct ksyminfo,si_flsz);
 else if (flags&KMOD_SYMINFO_FLAG_WANTNAME) info_size = offsetafter(struct ksyminfo,si_nmsz);
 else                                       info_size = offsetafter(struct ksyminfo,si_size);
 /* Figure out how much memory is available after the buffer (used for inline strings). */
 *reqsize = info_size;
 if (bufsize < info_size) {
  info_size = bufsize;
  bufavail  = 0;
 } else {
  bufavail  = bufsize-info_size;
 }
 buf_end = (byte_t *)buf+info_size;
 /* Extract the symbol name. */
 if (flags&KMOD_SYMINFO_FLAG_WANTNAME) {
  if __likely(symbol && symbol->s_nmsz) {
   info.si_flsz |= KSYMINFO_FLAG_NAME;
   if (!info.si_name || !info.si_nmsz) {
    info.si_name = (char *)buf_end;
    info.si_nmsz = (__size_t)bufavail;
   }
   bufreq = (symbol->s_nmsz+1)*sizeof(char);
   copy_size = min(info.si_nmsz,bufreq);
   if __unlikely(copy_to_user(info.si_name,symbol->s_name,copy_size)) return KE_FAULT;
   if (info.si_name == (char *)buf_end) {
    if (!info.si_nmsz) info.si_name = NULL;
    *reqsize += bufreq;
    buf_end  += copy_size;
    bufavail -= copy_size;
   }
   info.si_nmsz = bufreq;
  } else {
   info.si_nmsz = 0;
  }
 }

 if (flags&(KMOD_SYMINFO_FLAG_WANTFILE|KMOD_SYMINFO_FLAG_WANTLINE)) {
  struct kfileandline fal; ssize_t print_error;
  error = kaddr2linelist_exec(&module->pm_lib->sh_addr2line,
                              module->pm_lib,effective_addr,&fal);
  if __unlikely(KE_ISERR(error)) {
   if (flags&KMOD_SYMINFO_FLAG_WANTFILE) info.si_flsz = 0;
   if (flags&KMOD_SYMINFO_FLAG_WANTLINE) info.si_line = 0;
  } else {
   if (flags&KMOD_SYMINFO_FLAG_WANTFILE) {
    char const *s1,*s2,*s3;
    info.si_flsz |= KSYMINFO_FLAG_FILE;
    if (!info.si_file || !info.si_flsz) {
     info.si_file = (char *)buf_end;
     info.si_flsz = (__size_t)bufavail;
    }
    if ((s1 = fal.fal_path) == NULL) s1 = "",s2 = "";
    else { char ch = *s1 ? strend(s1)[-1] : '\0'; s2 = KFS_ISSEP(ch) ? "" : "/"; }
    if ((s3 = fal.fal_file) == NULL) s3 = "";
    print_error = user_snprintf(info.si_file,info.si_flsz,
                                &bufreq,"%s%s%s",s1,s2,s3);
    if (print_error < 0) { kfileandline_quit(&fal); return KE_FAULT; }
    copy_size = bufreq-(size_t)print_error;
    if (info.si_file == (char *)buf_end) {
     if (!info.si_flsz) info.si_file = NULL;
     *reqsize += bufreq;
     buf_end  += copy_size;
     bufavail -= copy_size;
    }
    info.si_flsz = bufreq;
   } else {
    info.si_flsz = 0;
   }
   if (flags&KMOD_SYMINFO_FLAG_WANTLINE) info.si_line = fal.fal_line;
   kfileandline_quit(&fal);
  }
 }

 /* Copy the info data block to user-space. */
 if __unlikely(copy_to_user(buf,&info,info_size)) return KE_FAULT;
 return KE_OK;
}



/* _syscall7(kerrno_t,kmod_syminfo,int,procfd,kmodid_t,modid,void const *,addr_or_name,struct ksyminfo *,buf,size_t,bufsize,size_t *,reqsize,__u32,flags); */
SYSCALL(sys_kmod_syminfo) {
 LOAD7(int                    ,K(procfd),
       kmodid_t               ,K(modid),
       struct ksymname const *,K(addr_or_name),
       struct ksyminfo       *,K(buf),
       size_t                 ,K(bufsize),
       size_t                *,K(preqsize),
       __u32                  ,K(flags));
 kerrno_t error; size_t reqsize;
 struct kproc *proc,*caller = kproc_self();
 struct kprocmodule *module; size_t sym_name_hash;
 struct ksymname sym_name; __u32 symbol_flags;
 struct kfdentry fd; struct ksymbol const *symbol;
 if (!buf) bufsize = 0;
 KTASK_CRIT_BEGIN_FIRST
 if __unlikely(KE_ISERR(error = kproc_getfd(caller,procfd,&fd))) goto end;
      if (fd.fd_type == KFDTYPE_PROC) proc = fd.fd_proc;
 else if (fd.fd_type == KFDTYPE_TASK) proc = ktask_getproc(fd.fd_task);
 else { error = KE_BADF; goto end_fd; }
 if __unlikely(KE_ISERR(error = kproc_lock(proc,KPROC_LOCK_MODS))) goto end_fd;
 if (!(flags&KMOD_SYMINFO_FLAG_LOOKUPADDR)) {
  error = ksymname_copyfromuser(&sym_name,addr_or_name);
  if __unlikely(KE_ISERR(error)) goto end_unlock;
  sym_name_hash = ksymhash_of(sym_name.sn_name,sym_name.sn_size);
 }
 if __unlikely(modid >= proc->p_modules.pms_moda) {
  if (modid == KMODID_SELF) {
   /* symbolic module ID self is only allowed if procfd also described self. */
   if __unlikely(caller != proc) { error = KE_INVAL; goto end_fd; }
   error = kproc_getmodat_unlocked(proc,&modid,(void *)regs->regs.eip);
   if __unlikely(KE_ISERR(error)) goto end_unlock;
   module = kproc_getrootmodule_unlocked(proc);
   if __unlikely(!module) { error = KE_DESTROYED; goto end_fd; }
   goto parse_module;
  }
  if (modid == KMODID_ALL) {
   module = NULL;
search_all_modules:
   /* Search all modules for a viable symbol. */
   if (flags&KMOD_SYMINFO_FLAG_LOOKUPADDR) {
    symbol = proc_find_symbol_by_addr(proc,(__user void const *)addr_or_name,
                                      &module,module,&symbol_flags);
   } else {
    symbol = proc_find_symbol_by_name(proc,sym_name.sn_name,sym_name.sn_size,
                                      sym_name_hash,&module,module,&symbol_flags);
   }
   goto found_symbol;
  }
  if (modid == KMODID_NEXT) {
   /* Search all modules, excluding that of the calling process. */
   error = kproc_getmodat_unlocked(proc,&modid,(void *)regs->regs.eip);
   if __unlikely(KE_ISERR(error)) goto end_unlock;
   module = proc->p_modules.pms_modv+modid;
   goto search_all_modules;
  }
  error = KE_INVAL;
 } else if __unlikely(!(module = &proc->p_modules.pms_modv[modid])->pm_lib) error = KE_INVAL;
 else {
parse_module:
  if (flags&KMOD_SYMINFO_FLAG_LOOKUPADDR) {
   if ((uintptr_t)addr_or_name < (uintptr_t)module->pm_base) {
    symbol = NULL;
   } else {
    symbol = kshlib_get_closest_symbol(module->pm_lib,
                                      (uintptr_t)addr_or_name-
                                      (uintptr_t)module->pm_base,
                                       &symbol_flags);
   }
  } else {
   symbol = kshlib_get_any_symbol(module->pm_lib,sym_name.sn_name,
                                  sym_name.sn_size,sym_name_hash,
                                  &symbol_flags);
  }
found_symbol:
  /* If we're not looking up an address and didn't find the symbol,
   * don't continue because we don't even have an address to go on. */
  if __unlikely(!symbol && !(flags&KMOD_SYMINFO_FLAG_LOOKUPADDR))
  { error = KE_NOENT; goto end_freename; }
  assert(module);
  error = ksymbol_fillinfo(proc,module,symbol,(flags&KMOD_SYMINFO_FLAG_LOOKUPADDR)
                           ? ((uintptr_t)addr_or_name-(uintptr_t)module->pm_base)
                           : symbol->s_addr,buf,bufsize,preqsize
                           ? &reqsize : NULL,flags,symbol_flags);
 }
end_freename:
 if (!(flags&KMOD_SYMINFO_FLAG_LOOKUPADDR)) {
  /* The symbol name was allocated dynamically. */
  free((char *)sym_name.sn_name);
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

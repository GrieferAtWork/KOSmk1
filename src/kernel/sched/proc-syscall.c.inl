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
#ifndef __KOS_KERNEL_SCHED_PROC_SYSCALL_C_INL__
#define __KOS_KERNEL_SCHED_PROC_SYSCALL_C_INL__ 1

#include <kos/compiler.h>
#include <kos/config.h>
#include <stddef.h>
#include <kos/task.h>
#include <kos/kernel/proc.h>
#include <kos/kernel/syscall.h>

__DECL_BEGIN

KSYSCALL_DEFINE_EX4(c,kerrno_t,kproc_enumfd,int,procfd,
                    __user int *__restrict,fdv,size_t,fdc,
                    __user size_t *,reqfdc) {
 __ref struct kproc *ctx; kerrno_t error;
 int *fditer,*fdend;
 struct kfdentry *entry_iter,*entry_end,*entry_begin;
 KTASK_CRIT_MARK
 fdend = (fditer = fdv)+fdc;
 ctx = kproc_getfdproc(kproc_self(),procfd);
 if __unlikely(!ctx) return KE_BADF;
 error = kmmutex_lock(&ctx->p_lock,KPROC_LOCK_FDMAN);
 if __unlikely(KE_ISERR(error)) { kproc_decref(ctx); return error; }
 entry_end = (entry_iter = entry_begin = ctx->p_fdman.fdm_fdv)+ctx->p_fdman.fdm_fda;
 for (; entry_iter != entry_end; ++entry_iter) if (entry_iter->fd_type != KFDTYPE_NONE) {
  if (fditer < fdend) *fditer = (int)(entry_iter-entry_begin);
  ++fditer;
 }
 kmmutex_unlock(&ctx->p_lock,KPROC_LOCK_FDMAN);
 kproc_decref(ctx);
 if (reqfdc) *reqfdc = (size_t)(fditer-fdv);
 return KE_OK;
}

KSYSCALL_DEFINE_EX3(c,int,kproc_openfd,int,procfd,int,fd,int,flags) {
 __ref struct kproc *ctx,*caller = kproc_self();
 struct kfdentry fdentry; int resfd; kerrno_t error;
 KTASK_CRIT_MARK
 ctx = kproc_getfdproc(caller,procfd);
 if __unlikely(!ctx) return KE_BADF;
 error = kproc_getfd(ctx,fd,&fdentry);
 kproc_decref(ctx);
 if __unlikely(KE_ISERR(error)) return error;
 fdentry.fd_flag = (kfdflag_t)flags;
 error = kproc_insfd_inherited(caller,&resfd,&fdentry);
 if __likely(KE_ISOK(error)) error = resfd;
 else ktask_decref(fdentry.fd_task);
 return error;
}

KSYSCALL_DEFINE_EX4(c,int,kproc_openfd2,int,procfd,int,fd,int,newfd,int,flags) {
 __ref struct kproc *ctx,*caller = kproc_self();
 struct kfdentry fdentry; kerrno_t error;
 KTASK_CRIT_MARK
 ctx = kproc_getfdproc(caller,procfd);
 if __unlikely(!ctx) return KE_BADF;
 error = kproc_getfd(ctx,fd,&fdentry);
 kproc_decref(ctx);
 if __unlikely(KE_ISERR(error)) return error;
 fdentry.fd_flag = (kfdflag_t)flags;
 error = kproc_insfdat_inherited(caller,newfd,&fdentry);
 if __likely(KE_ISOK(error)) error = newfd;
 else ktask_decref(fdentry.fd_task);
 return error;
}

KSYSCALL_DEFINE_EX1(c,kerrno_t,kproc_barrier,ksandbarrier_t,level) {
 kerrno_t error;
 struct ktask *caller = ktask_self();
 struct kproc *caller_proc = ktask_getproc(caller);
 if __unlikely(!kproc_hasflag(caller_proc,KPERM_FLAG_SETBARRIER)) error = KE_ACCES;
 else error = kproc_barrier(caller_proc,caller,level);
 return error;
}

KSYSCALL_DEFINE_EX2(c,int,kproc_openbarrier,int,procfd,ksandbarrier_t,level) {
 struct kfdentry fdentry; kerrno_t error; int fd;
 struct kproc *proc,*caller = kproc_self();
 KTASK_CRIT_MARK
 if (!kproc_hasflag(caller,KPERM_FLAG_SETBARRIER)) return KE_ACCES;
 proc = kproc_getfdproc(caller,procfd);
 if __unlikely(!proc) return KE_BADF;
 fdentry.fd_task = kproc_getbarrier_r(proc,level);
 kproc_decref(proc);
 if __unlikely(!fdentry.fd_task) return KE_OVERFLOW;
 fdentry.fd_attr = KFD_ATTR(KFDTYPE_TASK,KFD_FLAG_NONE);
 error = kproc_insfd_inherited(caller,&fd,&fdentry);
 if __likely(KE_ISOK(error)) error = fd;
 else ktask_decref(fdentry.fd_task);
 return error;
}

KSYSCALL_DEFINE_EX2(cr,kerrno_t,kproc_tlsalloc,
                    __user void const *,template_,
                    size_t,template_size) {
 struct ktranslator trans; kerrno_t error;
 struct kshmregion *region;
 struct ktask *caller = ktask_self();
 size_t template_pages;
 ktls_addr_t resoffset;
 KTASK_CRIT_MARK
 template_pages = ceildiv(template_size,PAGESIZE);
 region = kshmregion_newram(template_pages,
                            KSHMREGION_FLAG_READ|
                            KSHMREGION_FLAG_WRITE|
                            KSHMREGION_FLAG_LOSEONFORK);
 if __unlikely(!region) return KE_NOMEM;
 /* Initialize the TLS region with the given template, or fill it with ZEROes. */
 if (template_) {
  size_t max_dst,max_src;
  kshmregion_addr_t region_address = 0;
  __kernel void *kdst,*ksrc;
  error = ktranslator_init(&trans,caller);
  if __unlikely(KE_ISERR(error)) goto err_region;
  while ((kdst = kshmregion_translate_fast(region,region_address,&max_dst)) != NULL &&
         (ksrc = ktranslator_exec(&trans,template_,max_dst,&max_src,0)) != NULL) {
   memcpy(kdst,ksrc,max_src);
   if ((template_size -= max_src) == 0) break;
   region_address           += max_src;
   *(uintptr_t *)&template_ += max_src;
  }
  ktranslator_quit(&trans);
  /* Make sure the entire template got copied. */
  if __unlikely(template_size) { error = KE_FAULT; goto err_region; }
 } else {
  kshmregion_memset(region,0);
 }
 /* Allocate a new TLS block using the new region as template. */
 error = kproc_tls_alloc_inherited(ktask_getproc(caller),
                                   region,&resoffset);
 if __unlikely(KE_ISERR(error)) goto err_region;
 regs->regs.ecx = (uintptr_t)resoffset;
 return error;
err_region:
 kshmregion_decref_full(region);
 return error;
}
KSYSCALL_DEFINE_EX1(c,kerrno_t,kproc_tlsfree,
                    ptrdiff_t,tls_offset) {
 KTASK_CRIT_MARK
 return kproc_tls_free_offset(kproc_self(),tls_offset);
}


KSYSCALL_DEFINE_EX3(c,kerrno_t,kproc_enumpid,__user __pid_t *,pidv,
                    size_t,pidc,__user size_t *,reqpidc) {
 kerrno_t error;
 KTASK_CRIT_MARK
 pidv = (__pid_t *)kpagedir_translate(ktask_self()->t_epd,pidv); /* TODO: Unsafe. */
 reqpidc = (size_t *)kpagedir_translate(ktask_self()->t_epd,reqpidc); /* TODO: Unsafe. */
 if (!pidv) pidc = 0;
 error = kproclist_enumpid(pidv,pidc,reqpidc);
 return error;
}

KSYSCALL_DEFINE_EX1(c,int,kproc_openpid,__pid_t,pid) {
 struct kfdentry fdentry; kerrno_t error; int fd;
 KTASK_CRIT_MARK
 fdentry.fd_proc = kproclist_getproc(pid);
 if __unlikely(!fdentry.fd_proc) return KE_INVAL;
 fdentry.fd_attr = KFD_ATTR(KFDTYPE_PROC,KFD_FLAG_NONE);
 error = kproc_insfd_inherited(kproc_self(),&fd,&fdentry);
 if __likely(KE_ISOK(error)) error = fd;
 else kproc_decref(fdentry.fd_proc);
 return error;
}

KSYSCALL_DEFINE1(__pid_t,kproc_getpid,int,procfd) {
 struct kproc *proc; __pid_t result;
 switch (procfd) {
  case KFD_TASKSELF:
  case KFD_PROCSELF:
  case KFD_TASKROOT:
   return kproc_getpid(kproc_self());
  default: break;
 }
 KTASK_CRIT_BEGIN_FIRST
 proc = kproc_getfdproc(kproc_self(),procfd);
 if __unlikely(!proc) { result = (__pid_t)-1; goto end; }
 result = kproc_getpid(proc);
 kproc_decref(proc);
end:
 KTASK_CRIT_END
 return result;
}

KSYSCALL_DEFINE6(kerrno_t,kproc_getenv,int,procfd,
                 __user char const *,name,size_t,namemax,
                 __user char *,buf,size_t,bufsize,
                 __user size_t *,reqsize) {
 kerrno_t error;
 struct kproc *proc;
 name = (char const *)kpagedir_translate(ktask_self()->t_epd,name); /* TODO: Unsafe. */
 buf = (char *)kpagedir_translate(ktask_self()->t_epd,buf); /* TODO: Unsafe. */
 reqsize = (size_t *)kpagedir_translate(ktask_self()->t_epd,reqsize); /* TODO: Unsafe. */
 switch (procfd) {
  case KFD_TASKSELF:
  case KFD_PROCSELF:
  case KFD_TASKROOT:
   return kproc_getenv_k(kproc_self(),name,namemax,buf,bufsize,reqsize);
  default: break;
 }
 KTASK_CRIT_BEGIN_FIRST
 proc = kproc_getfdproc(kproc_self(),procfd);
 if __unlikely(!proc) { error = KE_BADF; goto end; }
 error = kproc_getenv_c(proc,name,namemax,buf,bufsize,reqsize);
 kproc_decref(proc);
end:
 KTASK_CRIT_END
 return error;
}

KSYSCALL_DEFINE6(kerrno_t,kproc_setenv,int,procfd,
                 __user char const *,name,size_t,namemax,
                 __user char const *,value,size_t,valuemax,
                 int,override) {
 kerrno_t error;
 struct kproc *proc;
 name = (char const *)kpagedir_translate(ktask_self()->t_epd,name); /* TODO: Unsafe. */
 value = (char const *)kpagedir_translate(ktask_self()->t_epd,value); /* TODO: Unsafe. */
 switch (procfd) {
  case KFD_TASKSELF:
  case KFD_PROCSELF:
  case KFD_TASKROOT:
   return kproc_setenv_k(kproc_self(),name,namemax,value,valuemax,override);
  default: break;
 }
 KTASK_CRIT_BEGIN_FIRST
 proc = kproc_getfdproc(kproc_self(),procfd);
 if __unlikely(!proc) { error = KE_BADF; goto end; }
 error = kproc_setenv_c(proc,name,namemax,value,valuemax,override);
 kproc_decref(proc);
end:
 KTASK_CRIT_END
 return error;
}

KSYSCALL_DEFINE3(kerrno_t,kproc_delenv,int,procfd,
                 __user char const *,name,size_t,namemax) {
 kerrno_t error; struct kproc *proc;
 name = (char const *)kpagedir_translate(ktask_self()->t_epd,name); /* TODO: Unsafe. */
 switch (procfd) {
  case KFD_TASKSELF:
  case KFD_PROCSELF:
  case KFD_TASKROOT:
   return kproc_delenv_k(kproc_self(),name,namemax);
  default: break;
 }
 KTASK_CRIT_BEGIN_FIRST
 proc = kproc_getfdproc(kproc_self(),procfd);
 if __unlikely(!proc) { error = KE_BADF; goto end; }
 error = kproc_delenv_c(proc,name,namemax);
 kproc_decref(proc);
end:
 KTASK_CRIT_END
 return error;
}

struct enumenv_data {
 __user char *iter;
 __user char *end;
};
static __crit kerrno_t
enumenv_callback(__kernel char const *name, size_t name_size,
                 __kernel char const *value, size_t value_size,
                 struct enumenv_data *data) {
 static char const text[2] = {'=','\0'};
 KTASK_CRIT_MARK
 __user char *buf; size_t bufsize,copysize;
 if (data->iter >= data->end) goto end;
 buf = data->iter;
 bufsize = (size_t)(data->end-buf);
 copysize = min(bufsize,name_size);
 if __unlikely(copy_to_user(buf,name,copysize)) return KE_FAULT;
 bufsize -= copysize,buf += copysize;
 if __likely(bufsize) {
  if __unlikely(copy_to_user(buf,text,sizeof(char))) return KE_FAULT;
  ++buf,--bufsize;
 }
 copysize = min(bufsize,value_size);
 if __unlikely(copy_to_user(buf,value,copysize)) return KE_FAULT;
 bufsize -= copysize,buf += copysize;
 if __likely(bufsize) {
  if __unlikely(copy_to_user(buf,text+1,sizeof(char))) return KE_FAULT;
  ++buf,--bufsize;
 }
end:
 data->iter += name_size+value_size+2;
 return KE_OK;
}

KSYSCALL_DEFINE4(kerrno_t,kproc_enumenv,int,self,
                 __user char *,buf,size_t,bufsize,
                 __user size_t *,reqsize) {
 struct enumenv_data data;
 kerrno_t error; struct kproc *proc;
 data.end = (data.iter = buf)+bufsize;
 switch (self) {
  case KFD_TASKSELF:
  case KFD_PROCSELF:
  case KFD_TASKROOT:
   error = kproc_enumenv_k(kproc_self(),(penumenv)&enumenv_callback,&data);
   goto done;
  default: break;
 }
 KTASK_CRIT_BEGIN_FIRST
 proc = kproc_getfdproc(kproc_self(),self);
 if __unlikely(!proc) {
  KTASK_CRIT_BREAK
  return KE_BADF;
 }
 error = kproc_enumenv_c(kproc_self(),(penumenv)&enumenv_callback,&data);
 kproc_decref(proc);
 KTASK_CRIT_END
done:
 if __likely(KE_ISOK(error) && data.iter < data.end) {
  if __unlikely(copy_to_user(data.iter,"\0",sizeof(char))) error = KE_FAULT;
 }
 if (reqsize && KE_ISOK(error)) {
  size_t sz = (size_t)((data.iter-buf)+1);
  if __unlikely(copy_to_user(reqsize,&sz,sizeof(sz))) error = KE_FAULT;
 }
 return error;
}

struct enumcmd_data {
 __user char *iter;
 __user char *end;
};
static kerrno_t
enumcmd_callback(__kernel char const *arg, struct enumcmd_data *data) {
 size_t bufsize,argsize = strlen(arg);
 if (data->iter >= data->end) goto end;
 bufsize = (size_t)((data->end-data->iter)*sizeof(char));
 bufsize = min(bufsize,(argsize+1)*sizeof(char));
 if __unlikely(copy_to_user(data->iter,arg,bufsize)) return KE_FAULT;
end:
 data->iter += argsize+1;
 return KE_OK;
}


KSYSCALL_DEFINE4(kerrno_t,kproc_getcmd,int,procfd,
                 __user char *,buf,size_t,bufsize,
                 __user size_t *,reqsize) {
 struct enumcmd_data data;
 kerrno_t error; struct kproc *proc;
 data.end = (data.iter = buf)+bufsize;
 switch (procfd) {
  case KFD_TASKSELF:
  case KFD_PROCSELF:
  case KFD_TASKROOT:
   error = kproc_enumcmd_k(kproc_self(),
                          (penumcmd)&enumcmd_callback,&data);
   goto done;
  default: break;
 }
 KTASK_CRIT_BEGIN_FIRST
 proc = kproc_getfdproc(kproc_self(),procfd);
 if __unlikely(!proc) {
  KTASK_CRIT_BREAK
  return KE_BADF;
 }
 error = kproc_enumcmd_c(kproc_self(),(penumcmd)&enumcmd_callback,&data);
 kproc_decref(proc);
 KTASK_CRIT_END
done:
 if __likely(KE_ISOK(error) && data.iter < data.end) {
  if __unlikely(copy_to_user(data.iter,"\0",sizeof(char))) error = KE_FAULT;
 }
 if (reqsize && KE_ISOK(error)) {
  size_t sz = (size_t)((data.iter-buf)+1);
  if __unlikely(copy_to_user(reqsize,&sz,sizeof(sz))) error = KE_FAULT;
 }
 return error;
}

KSYSCALL_DEFINE_EX4(c,kerrno_t,kproc_perm,int,procfd,
                    __user struct kperm *,buf,
                    size_t,elem_count,int,mode) {
 char buffer[offsetof(struct kperm,p_data)+KPERM_MAXDATASIZE];
 __ref struct kproc *proc,*caller = kproc_self();
 kerrno_t error = KE_OK; int basic_mode = mode&3;
#define perm  (*(struct kperm *)buffer)
 KTASK_CRIT_MARK
 proc = kproc_getfdproc(caller,procfd);
 if __unlikely(!proc) return KE_BADF;
 if (basic_mode == KPROC_PERM_MODE_SET ||
     basic_mode == KPROC_PERM_MODE_XCH) {
  /* Check if write permissions are allowed. */
  if (proc != caller || !kproc_hasflag(caller,KPERM_FLAG_SETPERM)
      ) { error = KE_ACCES; goto end_proc; }
  if (basic_mode == KPROC_PERM_MODE_XCH) goto check_getperm;
 } else if (basic_mode == KPROC_PERM_MODE_GET) {
  __STATIC_ASSERT(KPERM_FLAG_GETGROUP(KPERM_FLAG_GETPERM|KPERM_FLAG_GETPERM_OTHER) ==
                  KPERM_FLAG_GETGROUP(KPERM_FLAG_GETPERM));
check_getperm:
  /* Check if read permissions are allowed. */
  if (!kproc_hasflag(caller,(proc == caller ? KPERM_FLAG_GETPERM :
                    (KPERM_FLAG_GETPERM|KPERM_FLAG_GETPERM_OTHER))
      )) { error = KE_ACCES; goto end_proc; }
 } else {
  /* Invalid mode. */
  error = KE_INVAL;
  goto end_proc;
 }
 /* Everything's checking out. - We can start! */
 while (elem_count--) {
  size_t partsize;
  if __unlikely(copy_from_user(&perm,buf,offsetof(struct kperm,p_data))) goto err_fault;
  partsize = (size_t)perm.p_size;
  assert(partsize <= KPERM_MAXDATASIZE); /*< There should ~really~ be no way for this to fail... */
  if (basic_mode != KPROC_PERM_MODE_GET) {
   /* We need input data. */
   if __unlikely(copy_from_user(&perm.p_data,&buf->p_data,partsize)) goto err_fault;
  } else if (perm.p_id == KPERM_GETID(KPERM_NAME_FLAG)) {
   /* We need to know which flag group we're talking about. */
   if __unlikely(copy_from_user(&perm.p_data.d_flag_group,
                                &buf->p_data.d_flag_group,
                                 sizeof(perm.p_data.d_flag_group)
                 )) goto err_fault;
  }
  partsize += offsetof(struct kperm,p_data);
  switch (basic_mode) {
   case KPROC_PERM_MODE_GET:
    error = kproc_perm_get(proc,&perm);
    if (error == KE_INVAL) perm.p_id = 0,error = KE_OK;
    goto normal_handler;
   case KPROC_PERM_MODE_SET: error = kproc_perm_set(proc,&perm); break;
   case KPROC_PERM_MODE_XCH: error = kproc_perm_xch(proc,&perm); break;
   default: __compiler_unreachable();
  }
       if (error == KE_INVAL && mode&KPROC_PERM_MODE_UNKNOWN) error = KE_OK;
  else if (error == KE_ACCES && mode&KPROC_PERM_MODE_IGNORE)  error = KE_OK;
  else normal_handler: if __unlikely(KE_ISERR(error)) break;
  if (basic_mode != KPROC_PERM_MODE_SET) {
   /* Copy our buffer back into userspace. */
   if __unlikely(copy_to_user(buf,&perm,partsize)) goto err_fault;
  }
  *(uintptr_t *)&buf += partsize;
 }
end_proc: kproc_decref(proc);
#undef perm
 return error;
err_fault: error = KE_FAULT; goto end_proc;
}

__DECL_END

#endif /* !__KOS_KERNEL_SCHED_PROC_SYSCALL_C_INL__ */

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
#ifndef __KOS_KERNEL_SYSCALL_SYSCALL_KPROC_C_INL__
#define __KOS_KERNEL_SYSCALL_SYSCALL_KPROC_C_INL__ 1

#include "syscall-common.h"
#include <kos/kernel/proc.h>
#include <kos/errno.h>
#include <kos/syscallno.h>
#include <kos/fd.h>
#include <kos/kernel/util/string.h>

__DECL_BEGIN

/*< _syscall4(kerrno_t,kproc_enumfd,kproc_t,self,int *__restrict,fdv,__size_t,fdc,__size_t *,reqfdc); */
SYSCALL(sys_kproc_enumfd) {
 LOAD4(int     ,K (procfd),
       int    *,U0(fdv),
       size_t  ,K (fdc),
       size_t *,U0(reqfdc));
 __ref struct kproc *ctx; kerrno_t error;
 int *fditer,*fdend;
 struct kfdentry *entry_iter,*entry_end,*entry_begin;
 if __unlikely(!fdv) fdc = 0;
 fdend = (fditer = fdv)+fdc;
 KTASK_CRIT_BEGIN_FIRST
 ctx = kproc_getfdproc(kproc_self(),procfd);
 if __unlikely(!ctx) { error = KE_BADF; goto end; }
 error = kmmutex_lock(&ctx->p_lock,KPROC_LOCK_FDMAN);
 if __unlikely(KE_ISERR(error)) { kproc_decref(ctx); goto end; }
 entry_end = (entry_iter = entry_begin = ctx->p_fdman.fdm_fdv)+ctx->p_fdman.fdm_fda;
 for (; entry_iter != entry_end; ++entry_iter) if (entry_iter->fd_type != KFDTYPE_NONE) {
  if (fditer < fdend) *fditer = (int)(entry_iter-entry_begin);
  ++fditer;
 }
 kmmutex_unlock(&ctx->p_lock,KPROC_LOCK_FDMAN);
 kproc_decref(ctx);
 if (reqfdc) *reqfdc = (size_t)(fditer-fdv);
 error = KE_OK;
end:
 KTASK_CRIT_END
 RETURN(error);
}

/*< _syscall3(int,kproc_openfd,kproc_t,self,int,fd,int,flags); */
SYSCALL(sys_kproc_openfd) {
 LOAD3(int,K(procfd),
       int,K(fd),
       int,K(flags));
 __ref struct kproc *ctx,*caller = kproc_self();
 struct kfdentry fdentry; int resfd; kerrno_t error;
 KTASK_CRIT_BEGIN_FIRST
 ctx = kproc_getfdproc(caller,procfd);
 if __unlikely(!ctx) { error = KE_BADF; goto end; }
 error = kproc_getfd(ctx,fd,&fdentry);
 kproc_decref(ctx);
 if __unlikely(KE_ISERR(error)) goto end;
 fdentry.fd_flag = (kfdflag_t)flags;
 error = kproc_insfd_inherited(caller,&resfd,&fdentry);
 if __likely(KE_ISOK(error)) error = resfd;
 else ktask_decref(fdentry.fd_task);
end:
 KTASK_CRIT_END
 RETURN(error);
}

/*< _syscall4(int,kproc_openfd2,kproc_t,self,int,fd,int,newfd,int,flags); */
SYSCALL(sys_kproc_openfd2) {
 LOAD4(int,K(procfd),
       int,K(fd),
       int,K(newfd),
       int,K(flags));
 __ref struct kproc *ctx,*caller = kproc_self();
 struct kfdentry fdentry; kerrno_t error;
 KTASK_CRIT_BEGIN_FIRST
 ctx = kproc_getfdproc(caller,procfd);
 if __unlikely(!ctx) { error = KE_BADF; goto end; }
 error = kproc_getfd(ctx,fd,&fdentry);
 kproc_decref(ctx);
 if __unlikely(KE_ISERR(error)) goto end;
 fdentry.fd_flag = (kfdflag_t)flags;
 error = kproc_insfdat_inherited(caller,newfd,&fdentry);
 if __likely(KE_ISOK(error)) error = newfd;
 else ktask_decref(fdentry.fd_task);
end:
 KTASK_CRIT_END
 RETURN(error);
}

/* _syscall1(kerrno_t,kproc_barrier,ksandbarrier_t,level); */
SYSCALL(sys_kproc_barrier) {
 LOAD1(ksandbarrier_t,K(mode));
 kerrno_t error; struct ktask *caller = ktask_self();
 KTASK_CRIT_BEGIN_FIRST
 error = kproc_barrier(ktask_getproc(caller),caller,mode);
 KTASK_CRIT_END
 RETURN(error);
}

/* _syscall2(int,kproc_openbarrier,int,procfd,ksandbarrier_t,level); */
SYSCALL(sys_kproc_openbarrier) {
 LOAD2(int           ,K(procfd),
       ksandbarrier_t,K(level));
 struct kfdentry fdentry; kerrno_t error; int fd;
 struct kproc *proc,*caller = kproc_self();
 KTASK_CRIT_BEGIN_FIRST
 proc = kproc_getfdproc(caller,procfd);
 if __unlikely(!proc) { error = KE_BADF; goto end; }
 fdentry.fd_task = kproc_getbarrier_r(proc,level);
 kproc_decref(proc);
 if __unlikely(!fdentry.fd_task) { error = KE_OVERFLOW; goto end; }
 fdentry.fd_attr = KFD_ATTR(KFDTYPE_TASK,KFD_FLAG_NONE);
 error = kproc_insfd_inherited(caller,&fd,&fdentry);
 if __likely(KE_ISOK(error)) error = fd;
 else ktask_decref(fdentry.fd_task);
end:
 KTASK_CRIT_END
 RETURN(error);
}

/*< _syscall1(kerrno_t,kproc_alloctls_c,ktls_t *,result); */
SYSCALL(sys_kproc_alloctls) {
 LOAD1(__ktls_t *,U(result));
 kerrno_t error;
 KTASK_CRIT_BEGIN_FIRST
 error = kproc_alloctls_c(kproc_self(),result);
 KTASK_CRIT_END
 RETURN(error);
}

/*< _syscall1(kerrno_t,kproc_freetls_c,ktls_t,slot); */
SYSCALL(sys_kproc_freetls) {
 LOAD1(__ktls_t,K(slot));
 KTASK_CRIT_BEGIN_FIRST
 kproc_freetls_c(kproc_self(),slot);
 KTASK_CRIT_END
 RETURN(KE_OK);
}

/*< _syscall4(kerrno_t,kproc_enumtls,int,self,ktls_t *__restrict,tlsv,size_t,tlsc,size_t *,reqtlsc); */
SYSCALL(sys_kproc_enumtls) {
 LOAD4(int       ,K (procfd),
       __ktls_t *,U0(tlsv),
       size_t    ,K (tlsc),
       size_t   *,U0(reqtlsc));
 struct kproc *proc; struct ktask *roottask; kerrno_t error;
 if (!tlsv) tlsc = 0;
 KTASK_CRIT_BEGIN_FIRST
 proc = kproc_getfdproc(kproc_self(),procfd);
 if __unlikely(!proc) error = KE_BADF;
 else {
  roottask = kproc_getroottask(proc);
  if __unlikely(!roottask) {
   error = KE_DESTROYED;
   kproc_decref(proc);
  } else if __unlikely(!ktask_accessgm(roottask)) {
   ktask_decref(roottask);
   kproc_decref(proc);
   error = KE_ACCES;
  } else {
   ktask_decref(roottask);
   error = kproc_lock(proc,KPROC_LOCK_TLSMAN);
   if __likely(KE_ISOK(error)) {
#define GETBIT(i) (proc->p_tlsman.tls_usedv[(i)/8] & (1 << ((i)%8)))
    size_t i,size,*iter,*end;
    end = (iter = tlsv)+tlsc;
    size = proc->p_tlsman.tls_usedc;
    for (i = 0; i < size; ++i) if (GETBIT(i)) {
     if (iter < end) *iter = i;
     ++iter;
    }
    assert((size_t)(iter-tlsv) == proc->p_tlsman.tls_cnt);
    if (reqtlsc) *reqtlsc = (size_t)(iter-tlsv);
#undef GETBIT
    kproc_unlock(proc,KPROC_LOCK_TLSMAN);
   }
   kproc_decref(proc);
  }
 }
 KTASK_CRIT_END
 RETURN(error);
}

/*< _syscall3(kerrno_t,kproc_enumpid,__pid_t *__restrict,pidv,size_t,pidc,size_t *,reqpidc); */
SYSCALL(sys_kproc_enumpid) {
 LOAD3(__pid_t *,U0(pidv),
       size_t   ,K (pidc),
       size_t  *,U0(reqpidc));
 kerrno_t error; if (!pidv) pidc = 0;
 KTASK_CRIT_BEGIN_FIRST
 error = kproclist_enumpid(pidv,pidc,reqpidc);
 KTASK_CRIT_END
 RETURN(error);
}

/*< _syscall1(kproc_t,kproc_openpid,__pid_t,pid); */
SYSCALL(sys_kproc_openpid) {
 LOAD1(__pid_t,K(pid));
 struct kfdentry fdentry; kerrno_t error; int fd;
 KTASK_CRIT_BEGIN_FIRST
 fdentry.fd_proc = kproclist_getproc(pid);
 if __unlikely(!fdentry.fd_proc) { error = KE_INVAL; goto end; }
 fdentry.fd_attr = KFD_ATTR(KFDTYPE_PROC,KFD_FLAG_NONE);
 error = kproc_insfd_inherited(kproc_self(),&fd,&fdentry);
 if __likely(KE_ISOK(error)) error = fd;
 else kproc_decref(fdentry.fd_proc);
end:
 KTASK_CRIT_END
 RETURN(error);
}

/*< _syscall1(__pid_t,kproc_getpid,int,self); */
SYSCALL(sys_kproc_getpid) {
 LOAD1(int,K(taskfd));
 struct kproc *proc; __pid_t result;
 switch (taskfd) {
  case KFD_TASKSELF:
  case KFD_PROCSELF:
  case KFD_TASKROOT: RETURN(kproc_self()->p_pid);
  default: break;
 }
 KTASK_CRIT_BEGIN_FIRST
 proc = kproc_getfdproc(kproc_self(),taskfd);
 if __unlikely(!proc) { result = (__pid_t)-1; goto end; }
 result = proc->p_pid;
 kproc_decref(proc);
end:
 KTASK_CRIT_END
 RETURN(result);
}

/*< _syscall6(kerrno_t,kproc_getenv,int,self,char const *,name,__size_t,namemax,char *,buf,__size_t,bufsize,__size_t *,reqsize); */
SYSCALL(sys_kproc_getenv) {
 LOAD6(int         ,K (self),
       char const *,U0(name),
       __size_t    ,K (namemax),
       char       *,U0(buf),
       __size_t    ,K (bufsize),
       __size_t   *,U0(reqsize));
 kerrno_t error;
 struct kproc *proc;
 if (!name) namemax = 0;
 if (!buf) bufsize = 0;
 switch (self) {
  case KFD_TASKSELF:
  case KFD_PROCSELF:
  case KFD_TASKROOT:
   RETURN(kproc_getenv_k(kproc_self(),name,namemax,buf,bufsize,reqsize));
  default: break;
 }
 KTASK_CRIT_BEGIN_FIRST
 proc = kproc_getfdproc(kproc_self(),self);
 if __unlikely(!proc) { error = KE_BADF; goto end; }
 error = kproc_getenv_c(proc,name,namemax,buf,bufsize,reqsize);
 kproc_decref(proc);
end:
 KTASK_CRIT_END
 RETURN(error);
}

/*< _syscall6(kerrno_t,kproc_setenv,int,self,char const *,name,__size_t,namemax,char const *,value,__size_t,valuemax,int,override); */
SYSCALL(sys_kproc_setenv) {
 LOAD6(int         ,K (self),
       char const *,U0(name),
       __size_t    ,K (namemax),
       char const *,U0(value),
       __size_t    ,K (valuemax),
       int         ,K (override));
 kerrno_t error;
 struct kproc *proc;
 if (!name) namemax = 0;
 if (!value) valuemax = 0;
 switch (self) {
  case KFD_TASKSELF:
  case KFD_PROCSELF:
  case KFD_TASKROOT:
   RETURN(kproc_setenv_k(kproc_self(),name,namemax,value,valuemax,override));
  default: break;
 }
 KTASK_CRIT_BEGIN_FIRST
 proc = kproc_getfdproc(kproc_self(),self);
 if __unlikely(!proc) { error = KE_BADF; goto end; }
 error = kproc_setenv_c(proc,name,namemax,value,valuemax,override);
 kproc_decref(proc);
end:
 KTASK_CRIT_END
 RETURN(error);
}

/*< _syscall3(kerrno_t,kproc_delenv,int,self,char const *,name,__size_t,namemax); */
SYSCALL(sys_kproc_delenv) {
 LOAD3(int         ,K (self),
       char const *,U0(name),
       __size_t    ,K (namemax));
 kerrno_t error;
 struct kproc *proc;
 if (!name) namemax = 0;
 switch (self) {
  case KFD_TASKSELF:
  case KFD_PROCSELF:
  case KFD_TASKROOT:
   RETURN(kproc_delenv_k(kproc_self(),name,namemax));
  default: break;
 }
 KTASK_CRIT_BEGIN_FIRST
 proc = kproc_getfdproc(kproc_self(),self);
 if __unlikely(!proc) { error = KE_BADF; goto end; }
 error = kproc_delenv_c(proc,name,namemax);
 kproc_decref(proc);
end:
 KTASK_CRIT_END
 RETURN(error);
}

struct enumenv_data {
 char *iter;
 char *end;
};
static __crit kerrno_t
enumenv_callback(char const *name, size_t name_size,
                 char const *value, size_t value_size,
                 struct enumenv_data *data) {
 static char const text[2] = {'=','\0'};
 KTASK_CRIT_MARK
 char *buf; size_t bufsize,copysize;
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

/*< _syscall4(kerrno_t,kproc_enumenv,int,self,char *,buf,__size_t,bufsize,__size_t *,reqsize); */
SYSCALL(sys_kproc_enumenv) {
 LOAD4(int       ,K(self),
       char     *,K(buf),
       __size_t  ,K(bufsize),
       __size_t *,K(reqsize));
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
  RETURN(KE_BADF);
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
 RETURN(error);
}

struct enumcmd_data {
 char *iter;
 char *end;
};
static kerrno_t
enumcmd_callback(char const *arg, struct enumcmd_data *data) {
 size_t bufsize,argsize = strlen(arg);
 if (data->iter >= data->end) goto end;
 bufsize = (size_t)((data->end-data->iter)*sizeof(char));
 bufsize = min(bufsize,(argsize+1)*sizeof(char));
 if __unlikely(copy_to_user(data->iter,arg,bufsize)) return KE_FAULT;
end:
 data->iter += argsize+1;
 return KE_OK;
}


/*< _syscall4(kerrno_t,kproc_enumenv,int,self,char *,buf,__size_t,bufsize,__size_t *,reqsize); */
SYSCALL(sys_kproc_getcmd) {
 LOAD4(int       ,K(self),
       char     *,K(buf),
       __size_t  ,K(bufsize),
       __size_t *,K(reqsize));
 struct enumcmd_data data;
 kerrno_t error; struct kproc *proc;
 data.end = (data.iter = buf)+bufsize;
 switch (self) {
  case KFD_TASKSELF:
  case KFD_PROCSELF:
  case KFD_TASKROOT:
   error = kproc_enumcmd_k(kproc_self(),(penumcmd)&enumcmd_callback,&data);
   goto done;
  default: break;
 }
 KTASK_CRIT_BEGIN_FIRST
 proc = kproc_getfdproc(kproc_self(),self);
 if __unlikely(!proc) {
  KTASK_CRIT_BREAK
  RETURN(KE_BADF);
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
 RETURN(error);
}



__DECL_END

#endif /* !__KOS_KERNEL_SYSCALL_SYSCALL_KPROC_C_INL__ */

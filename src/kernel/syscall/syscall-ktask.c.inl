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
#ifndef __KOS_KERNEL_SYSCALL_SYSCALL_KTASK_C_INL__
#define __KOS_KERNEL_SYSCALL_SYSCALL_KTASK_C_INL__ 1

#include "syscall-common.h"
#include <kos/kernel/proc.h>
#include <kos/kernel/task.h>
#include <kos/kernel/tls.h>
#include <kos/errno.h>
#include <kos/fd.h>
#include <kos/syscallno.h>
#include <kos/types.h>
#include <stddef.h>
#include <kos/task.h>
#include <kos/kernel/linker.h>
#include <kos/kernel/util/string.h>

__DECL_BEGIN

/* _syscall0(kerrno_t,ktask_yield); */
SYSCALL(sys_ktask_yield) {
 LOAD0();
 RETURN(ktask_tryyield());
}

/*< _syscall3(kerrno_t,ktask_setalarm,int,task,struct timespec const *__restrict,abstime,struct timespec *__restrict,oldabstime); */
SYSCALL(sys_ktask_setalarm) {
 LOAD3(int              ,K (taskfd),
       struct timespec *,U0(abstime),
       struct timespec *,U0(oldabstime));
 struct ktask *task; kerrno_t error;
 if (taskfd == KFD_TASKSELF) RETURN(ktask_setalarm(ktask_self(),abstime,oldabstime));
 KTASK_CRIT_BEGIN_FIRST
 task = kproc_getfdtask(kproc_self(),taskfd);
 if __unlikely(!task) { error = KE_BADF; goto end; }
 error = ktask_setalarm(task,abstime,oldabstime);
 ktask_decref(task);
end:
 KTASK_CRIT_END
 RETURN(error);
}

/*< _syscall2(kerrno_t,ktask_getalarm,int,task,struct timespec *__restrict,abstime); */
SYSCALL(sys_ktask_getalarm) {
 LOAD2(int              ,K(taskfd),
       struct timespec *,K(abstime));
 struct ktask *task; kerrno_t error;
 struct timespec res_abstime;
 KTASK_CRIT_BEGIN_FIRST
 task = kproc_getfdtask(kproc_self(),taskfd);
 if __unlikely(!task) { error = KE_BADF; goto end; }
 error = ktask_getalarm(task,&res_abstime);
 ktask_decref(task);
 if (__likely(KE_ISOK(error)) &&
     __unlikely(copy_to_user(abstime,&res_abstime,sizeof(res_abstime)))
     ) error = KE_FAULT;
end:
 KTASK_CRIT_END
 RETURN(error);
}

/*< _syscall2(kerrno_t,ktask_abssleep,int,task,struct timespec const *__restrict,timeout); */
SYSCALL(sys_ktask_abssleep) {
 LOAD2(int                    ,K (taskfd),
       struct timespec const *,U0(abstime));
 struct ktask *task; kerrno_t error;
 if (taskfd == KFD_TASKSELF) {
  task = ktask_self();
sleepself:
  error = abstime ? ktask_abssleep(task,abstime)
                  : ktask_pause(task);
  RETURN(error);
 }
 KTASK_CRIT_BEGIN_FIRST
 task = kproc_getfdtask(kproc_self(),taskfd);
 if __unlikely(!task) { error = KE_BADF; goto end; }
 if (task == ktask_self()) {
  ktask_decref(task);
  KTASK_CRIT_BREAK
  goto sleepself;
 }
 error = abstime ? ktask_abssleep(task,abstime)
                 : ktask_pause(task);;
 ktask_decref(task);
end:
 KTASK_CRIT_END
 RETURN(error);
}

/*< _syscall3(kerrno_t,ktask_terminate,int,task,void *,exitcode,ktaskopflag_t,flags); */
SYSCALL(sys_ktask_terminate) {
 LOAD3(int          ,K(taskfd),
       void        *,K(exitcode),
       ktaskopflag_t,K(flags));
 struct kfdentry fdentry; kerrno_t error;
 KTASK_CRIT_BEGIN_FIRST
 error = kproc_getfd(kproc_self(),taskfd,&fdentry);
 if __unlikely(KE_ISERR(error)) goto end;
 error = kfdentry_terminate(&fdentry,exitcode,flags);
 kfdentry_quit(&fdentry);
end:
 KTASK_CRIT_END
 assert(fdentry.fd_data != ktask_self());
 RETURN(error);
}

/*< _syscall2(kerrno_t,ktask_suspend,int,task,ktaskopflag_t,flags); */
SYSCALL(sys_ktask_suspend) {
 LOAD2(int          ,K(taskfd),
       ktaskopflag_t,K(flags));
 struct kfdentry fdentry; kerrno_t error;
 KTASK_CRIT_BEGIN_FIRST
 error = kproc_getfd(kproc_self(),taskfd,&fdentry);
 if __unlikely(KE_ISERR(error)) goto end;
 error = kfdentry_suspend(&fdentry,flags);
 kfdentry_quit(&fdentry);
end:
 KTASK_CRIT_END
 RETURN(error);
}

/* _syscall2(kerrno_t,ktask_resume,int,task,ktaskopflag_t,flags); */
SYSCALL(sys_ktask_resume) {
 LOAD2(int          ,K(taskfd),
       ktaskopflag_t,K(flags));
 struct kfdentry fdentry; kerrno_t error;
 KTASK_CRIT_BEGIN_FIRST
 error = kproc_getfd(kproc_self(),taskfd,&fdentry);
 if __unlikely(KE_ISERR(error)) goto end;
 error = kfdentry_resume(&fdentry,flags);
 kfdentry_quit(&fdentry);
end:
 KTASK_CRIT_END
 RETURN(error);
}

/* _syscall1(int,ktask_openroot,int,task); */
SYSCALL(sys_ktask_openroot) {
 LOAD1(int,K(taskfd));
 struct kfdentry fdentry; kerrno_t error; int fd;
 struct kproc *ctx = kproc_self(); struct ktask *result;
 KTASK_CRIT_BEGIN_FIRST
 error = kproc_getfd(ctx,taskfd,&fdentry);
 if __unlikely(KE_ISERR(error)) goto end;
 error = kfdentry_openprocof(&fdentry,&result);
 kfdentry_quit(&fdentry);
 if __unlikely(KE_ISERR(error)) goto end;
 fdentry.fd_task = result; // Inherit reference
 fdentry.fd_attr = KFD_ATTR(KFDTYPE_TASK,KFD_FLAG_NONE);
 error = kproc_insfd_inherited(ctx,&fd,&fdentry);
 if __likely(KE_ISOK(error)) error = fd;
 else ktask_decref(fdentry.fd_task);
end:
 KTASK_CRIT_END
 RETURN(error);
}

/* _syscall1(int,ktask_openparent,int,self); */
SYSCALL(sys_ktask_openparent) {
 LOAD1(int,K(taskfd));
 struct kfdentry fdentry; kerrno_t error; int fd;
 struct kproc *ctx = kproc_self(); struct ktask *result;
 KTASK_CRIT_BEGIN_FIRST
 error = kproc_getfd(ctx,taskfd,&fdentry);
 if __unlikely(KE_ISERR(error)) goto end;
 error = kfdentry_openparent(&fdentry,&result);
 kfdentry_quit(&fdentry);
 if __unlikely(KE_ISERR(error)) goto end;
 fdentry.fd_task = result; // Inherit reference
 fdentry.fd_attr = KFD_ATTR(KFDTYPE_TASK,KFD_FLAG_NONE);
 error = kproc_insfd_inherited(ctx,&fd,&fdentry);
 if __likely(KE_ISOK(error)) error = fd;
 else ktask_decref(fdentry.fd_task);
end:
 KTASK_CRIT_END
 RETURN(error);
}

/* _syscall1(int,ktask_openproc,int,self); */
SYSCALL(sys_ktask_openproc) {
 LOAD1(int,K(taskfd));
 struct kfdentry fdentry; kerrno_t error; int fd;
 struct kproc *ctx = kproc_self(); struct kproc *result;
 KTASK_CRIT_BEGIN_FIRST
 error = kproc_getfd(ctx,taskfd,&fdentry);
 if __unlikely(KE_ISERR(error)) goto end;
 error = kfdentry_openctx(&fdentry,&result);
 kfdentry_quit(&fdentry);
 if __unlikely(KE_ISERR(error)) goto end;
 fdentry.fd_proc = result; // Inherit reference
 fdentry.fd_attr = KFD_ATTR(KFDTYPE_PROC,KFD_FLAG_NONE);
 error = kproc_insfd_inherited(ctx,&fd,&fdentry);
 if __likely(KE_ISOK(error)) error = fd;
 else kproc_decref(fdentry.fd_proc);
end:
 KTASK_CRIT_END
 RETURN(error);
}

/* _syscall1(size_t,ktask_getparid,int,self); */
SYSCALL(sys_ktask_getparid) {
 LOAD1(int,K(taskfd));
 struct kfdentry fdentry; size_t result;
 switch (taskfd) {
  case KFD_TASKSELF: RETURN(ktask_getparid(ktask_self()));
  case KFD_PROCSELF:
  case KFD_TASKROOT: RETURN(ktask_getparid(ktask_proc()));
  default: break;
 }
 KTASK_CRIT_BEGIN_FIRST
 if __unlikely(KE_ISERR(kproc_getfd(kproc_self(),taskfd,&fdentry))) {
  result = (size_t)-1; goto end;
 }
 result = kfdentry_getparid(&fdentry);
 kfdentry_quit(&fdentry);
end:
 KTASK_CRIT_END
 RETURN(result);
}
/*< _syscall1(size_t,ktask_gettid,int,self); */
SYSCALL(sys_ktask_gettid) {
 LOAD1(int,K(taskfd));
 struct kfdentry fdentry; size_t result;
 switch (taskfd) {
  case KFD_TASKSELF: RETURN(ktask_gettid(ktask_self()));
  case KFD_PROCSELF:
  case KFD_TASKROOT: RETURN(ktask_gettid(ktask_proc()));
  default: break;
 }
 KTASK_CRIT_BEGIN_FIRST
 if __unlikely(KE_ISERR(kproc_getfd(kproc_self(),taskfd,
  &fdentry))) { result = (size_t)-1; goto end; }
 result = kfdentry_gettid(&fdentry);
 kfdentry_quit(&fdentry);
end:
 KTASK_CRIT_END
 RETURN(result);
}

/* _syscall2(int,ktask_openchild,int,self,size_t,parid); */
SYSCALL(sys_ktask_openchild) {
 LOAD2(int   ,K(taskfd),
       size_t,K(childid));
 struct kfdentry fdentry; kerrno_t error; int fd;
 struct kproc *ctx = kproc_self(); struct ktask *result;
 KTASK_CRIT_BEGIN_FIRST
 error = kproc_getfd(ctx,taskfd,&fdentry);
 if __unlikely(KE_ISERR(error)) goto end;
 error = kfdentry_opentask(&fdentry,childid,&result);
 kfdentry_quit(&fdentry);
 if __unlikely(KE_ISERR(error)) goto end;
 fdentry.fd_task = result; // Inherit reference
 fdentry.fd_attr = KFD_ATTR(KFDTYPE_TASK,KFD_FLAG_NONE);
 error = kproc_insfd_inherited(ctx,&fd,&fdentry);
 if __likely(KE_ISOK(error)) error = fd;
 else ktask_decref(fdentry.fd_task);
end:
 KTASK_CRIT_END
 RETURN(error);
}

/* _syscall4(kerrno_t,ktask_enumchildren,int,self,size_t *__restrict,idv,__size_t,idc,__size_t *,reqidc); */
SYSCALL(sys_ktask_enumchildren) {
 LOAD4(int     ,K (taskfd),
       size_t *,U0(idv),
       size_t  ,K (idc),
       size_t *,U0(reqidc));
 struct kfdentry fdentry; kerrno_t error;
 struct kproc *ctx = kproc_self();
 KTASK_CRIT_BEGIN_FIRST
 error = kproc_getfd(ctx,taskfd,&fdentry);
 if __unlikely(KE_ISERR(error)) goto end;
 error = kfdentry_enumtasks(&fdentry,idv,idc,reqidc);
 kfdentry_quit(&fdentry);
end:
 KTASK_CRIT_END
 RETURN(error);
}

/* _syscall2(kerrno_t,ktask_getpriority,int,self,ktaskprio_t *__restrict,result); */
SYSCALL(sys_ktask_getpriority) {
 LOAD2(int          ,K(taskfd),
       ktaskprio_t *,U(result));
 struct kfdentry fdentry; kerrno_t error;
 KTASK_CRIT_BEGIN_FIRST
 error = kproc_getfd(kproc_self(),taskfd,&fdentry);
 if __unlikely(KE_ISERR(error)) goto end;
 error = kfdentry_getpriority(&fdentry,result);
 kfdentry_quit(&fdentry);
end:
 KTASK_CRIT_END
 RETURN(error);
}

/* _syscall2(kerrno_t,ktask_setpriority,int,self,ktaskprio_t,value); */
SYSCALL(sys_ktask_setpriority) {
 LOAD2(int        ,K(taskfd),
       ktaskprio_t,K(value));
 struct kfdentry fdentry; kerrno_t error;
 KTASK_CRIT_BEGIN_FIRST
 error = kproc_getfd(kproc_self(),taskfd,&fdentry);
 if __unlikely(KE_ISERR(error)) goto end;
 error = kfdentry_setpriority(&fdentry,value);
 kfdentry_quit(&fdentry);
end:
 KTASK_CRIT_END
 RETURN(error);
}

/* _syscall2(kerrno_t,ktask_join,int,self,void **__restrict,exitcode) */
SYSCALL(sys_ktask_join) {
 LOAD2(int    ,K (taskfd),
       void **,U0(exitcode));
 kerrno_t error; struct kfdentry fdentry;
 KTASK_CRIT_BEGIN_FIRST
 error = kproc_getfd(kproc_self(),taskfd,&fdentry);
 if __unlikely(KE_ISERR(error)) goto end;
 if (fdentry.fd_type == KFDTYPE_PROC) {
  struct ktask *roottask = kproc_getroottask(fdentry.fd_proc);
  kproc_decref(fdentry.fd_proc);
  if __unlikely(!roottask) { error = KE_DESTROYED; goto end; }
  fdentry.fd_task = roottask;
  fdentry.fd_type = KFDTYPE_TASK;
 }
 if (fdentry.fd_type != KFDTYPE_TASK) { error = KE_BADF; goto end_entry; }
 if __unlikely(fdentry.fd_task == ktask_self()) {
  // You wan'na deadlock? Fine... But you won't screw over my kernel!
  // >> If a task would attempt to join itself while inside
  //    a critical block, a situation would arise that cannot
  //    be safely resolved.
  // >> The task cannot be terminated from the outside, as
  //    that is the main design feature of a critical block.
  //    Though the task can't exit normally either, as
  //    it is trying to wait for itself to finish.
  // EDIT: Such a situation can now be resolved through use of KE_INTR
  ktask_decref(fdentry.fd_task); // Drop our reference while inside the block
  KTASK_CRIT_BREAK // Don't join from inside a critical block
  // The following call should produce a deadlock, but just
  // in case it does manage to return, still handle that
  // case correctly.
  // NOTE: The call is still safe though, as the while we are still
  //       running, we own a reference to ktask_self(), which itself
  //       is immutable and known to be equal to 'task'.
  assert(fdentry.fd_task == ktask_self());
  RETURN(ktask_join(fdentry.fd_task,exitcode));
 }
 // ----: Somehow instruct the tasking system to drop a reference
 //       to the task once it starts joining, while also doing so
 //       outside of the critical block we build.
 // ----: Also update the code below to do this.
 // >> NOTE: Another option would be to store our reference to
 //          'task' somewhere in 'ktask_self()' and instruct
 //          whatever task might be trying to terminate us to
 //          decref that reference for us in case we'll never
 //          get around to doing so.
 //    NOTE: This is probably the better solution as it gives
 //          the kernel a safe way of holding references without
 //          being forced to stay inside a critical block.
 // EDIT: This problem was solved with the introduction of the KE_INTR error.
 error = ktask_join(fdentry.fd_task,exitcode);
end_entry:
 kfdentry_quit(&fdentry);
end:
 KTASK_CRIT_END
 RETURN(error);
}

/* _syscall2(kerrno_t,ktask_tryjoin,int,self,void **__restrict,exitcode) */
SYSCALL(sys_ktask_tryjoin) {
 LOAD2(int    ,K (taskfd),
       void **,U0(exitcode));
 kerrno_t error; struct ktask *task;
 KTASK_CRIT_BEGIN_FIRST
 task = kproc_getfdtask(kproc_self(),taskfd);
 if __unlikely(!task) { error = KE_BADF; goto end; }
 error = ktask_tryjoin(task,exitcode);
 ktask_decref(task);
end:
 KTASK_CRIT_END
 RETURN(error);
}

/* _syscall3(kerrno_t,ktask_timedjoin,int,self,struct timespec const *__restrict,abstime,void **__restrict,exitcode) */
SYSCALL(sys_ktask_timedjoin) {
 LOAD3(int              ,K (taskfd),
       struct timespec *,U (abstime),
       void           **,U0(exitcode));
 kerrno_t error; struct ktask *task;
 KTASK_CRIT_BEGIN_FIRST
 task = kproc_getfdtask(kproc_self(),taskfd);
 if __unlikely(!task) { error = KE_BADF; goto end; }
 error = ktask_timedjoin(task,abstime,exitcode);
 ktask_decref(task);
end:
 KTASK_CRIT_END
 RETURN(error);
}

/* _syscall4(int,ktask_newthread,ktask_threadfun_t,thread_main,void *,closure,__u32,flags,void **,arg); */
SYSCALL(sys_ktask_newthread) {
 LOAD4(ktask_threadfun_t,K(thread_main),
       void           *,K(closure),
       __u32           ,K(flags),
       void          **,K(arg));
 __COMPILER_PACK_PUSH(1)
 struct __packed stackframe {
  void *returnaddr;
  void *closure;
 };
 __COMPILER_PACK_POP
 void *useresp; struct kfdentry entry;
 int fd; kerrno_t error;
 struct stackframe *userstack;
 struct ktask *caller = ktask_self();
 struct kproc *callerctx = ktask_getproc(caller);
 KTASK_CRIT_BEGIN_FIRST
 if __unlikely(KE_ISERR(error = kproc_lock(callerctx,KPROC_LOCK_SHM))) goto end;
 if __unlikely(!kpagedir_ismappedp(kproc_pagedir(callerctx),
                                  *(void **)&thread_main)
               ) { error = KE_FAULT; goto err_pdlock; }
 if (flags&KTASK_NEW_FLAG_UNREACHABLE) {
  struct ktask *parent = kproc_getbarrier_r(callerctx,KSANDBOX_BARRIER_NOSETMEM);
  if __unlikely(!parent) { error = KE_DESTROYED; goto err_pdlock; }
  entry.fd_task = ktask_newuser(parent,callerctx,&useresp
                                ,KTASK_STACK_SIZE_DEF
                                ,KTASK_STACK_SIZE_DEF);
  ktask_decref(parent);
 } else {
  entry.fd_task = ktask_newuser(caller,callerctx,&useresp
                                ,KTASK_STACK_SIZE_DEF
                                ,KTASK_STACK_SIZE_DEF);
 }
 if __unlikely(!entry.fd_task) {
  error = KE_NOMEM;
err_pdlock:
  kproc_unlock(callerctx,KPROC_LOCK_SHM);
  goto end;
 }
 userstack = TRANSLATE((struct stackframe *)useresp-1);
 kassertobj(userstack);
 userstack->returnaddr   = NULL;
 userstack->closure      = closure;
 *(uintptr_t *)&useresp -= sizeof(struct stackframe);
 ktask_setupuser(entry.fd_task,useresp,*(void **)&thread_main);
 assertf(userstack->closure == closure,
         // Yes, I had troubles with this once...
         // But why not just leave it in? It just debug information.
         "%p, %p != %p\n"
         "userstack                       = %p\n"
         "entry.fd_task                   = %p\n"
         "entry.fd_task->t_esp            = %p\n"
         "TRANSLATE(entry.fd_task->t_esp) = %p\n"
         "entry.fd_task->t_kstackvp       = %p\n"
         "entry.fd_task->t_ustackvp       = %p\n"
         "entry.fd_task->t_ustacksz       = %Iu\n"
         "entry.fd_task->t_kstack         = %p\n"
         "entry.fd_task->t_kstackend      = %p\n"
         ,useresp,userstack->closure,closure
         ,userstack
         ,entry.fd_task
         ,entry.fd_task->t_esp
         ,TRANSLATE(entry.fd_task->t_esp)
         ,entry.fd_task->t_kstackvp
         ,entry.fd_task->t_ustackvp
         ,entry.fd_task->t_ustacksz
         ,entry.fd_task->t_kstack
         ,entry.fd_task->t_kstackend);
 kproc_unlock(callerctx,KPROC_LOCK_SHM);
 if (flags&KTASK_NEW_FLAG_JOIN) {
  void *exitcode;
  /* Immediatly join the task */
  if (flags&KTASK_NEW_FLAG_SUSPENDED) {
   error = ktask_resume_k(entry.fd_task);
   if __unlikely(KE_ISERR(error)) goto err_task;
  }
  error = ktask_join(entry.fd_task,&exitcode);
  ktask_decref(entry.fd_task);
  if (KE_ISOK(error) && arg &&
      copy_to_user(arg,&exitcode,sizeof(exitcode))
      ) error = KE_FAULT;
  goto end;
 }
 entry.fd_type = KFDTYPE_TASK;
 if (!(flags&KTASK_NEW_FLAG_SUSPENDED)) {
  asserte(KE_ISOK(ktask_incref(entry.fd_task)));
  error = kproc_insfd_inherited(callerctx,&fd,&entry);
  if __unlikely(KE_ISERR(error)) {
   ktask_decref(entry.fd_task);
err_task:
   ktask_decref(entry.fd_task);
   goto end;
  }
  __evalexpr(ktask_resume_k(entry.fd_task)); // Ignore errors in this
  ktask_decref(entry.fd_task);
 } else {
  error = kproc_insfd_inherited(callerctx,&fd,&entry);
  if __unlikely(KE_ISERR(error)) { ktask_decref(entry.fd_task); goto end; }
 }
 error = fd;
end:
 KTASK_CRIT_END
 RETURN(error);
}

/* _syscall5(int,ktask_newthreadi,ktask_threadfun_t,thread_main,void const *,buf,size_t,bufsize,__u32,flags,void **,arg); */
SYSCALL(sys_ktask_newthreadi) {
 LOAD5(ktask_threadfun_t,K(thread_main),
       void const     *,K(buf),
       size_t          ,K(bufsize),
       __u32           ,K(flags),
       void          **,K(arg));
 __COMPILER_PACK_PUSH(1)
 struct __packed stackframe {
  __user void *returnaddr;
  __user void *datap;
         __u8  data[1024];
 };
 __COMPILER_PACK_POP
 __user void *useresp,*dataesp; struct kfdentry entry;
 int fd; kerrno_t error; struct stackframe *userstack;
 struct ktask *caller    = ktask_self();
 struct kproc *callerctx = ktask_getproc(caller);
 if (!bufsize) buf = NULL;
 else if __unlikely(!kpagedir_ismappedex(kpagedir_user(),
                                         (void *)alignd((uintptr_t)buf,PAGEALIGN),
                                         ceildiv(bufsize,PAGESIZE),
                                         PAGEDIR_FLAG_USER,
                                         PAGEDIR_FLAG_USER)) RETURN(KE_FAULT);
 buf = TRANSLATE(buf); kassertmem(buf,bufsize);
 KTASK_CRIT_BEGIN_FIRST
 if __unlikely(KE_ISERR(error = kproc_lock(callerctx,KPROC_LOCK_SHM))) goto end;
 if __unlikely(!kpagedir_ismappedp(kproc_pagedir(callerctx),
                                  *(void **)&thread_main)
               ) { error = KE_FAULT; goto err_pdlock; }
 if (flags&KTASK_NEW_FLAG_UNREACHABLE) {
  struct ktask *parent = kproc_getbarrier_r(callerctx,KSANDBOX_BARRIER_NOSETMEM);
  if __unlikely(!parent) { error = KE_DESTROYED; goto err_pdlock; }
  entry.fd_task = ktask_newuser(parent,callerctx,&useresp
                                ,KTASK_STACK_SIZE_DEF
                                ,KTASK_STACK_SIZE_DEF);
  ktask_decref(parent);
 } else {
  entry.fd_task = ktask_newuser(caller,callerctx,&useresp
                                ,KTASK_STACK_SIZE_DEF
                                ,KTASK_STACK_SIZE_DEF);
 }
 if __unlikely(!entry.fd_task) {
  error = KE_NOMEM;
err_pdlock:
  kproc_unlock(callerctx,KPROC_LOCK_SHM);
  goto end;
 }
 // TODO: What if 'bufsize' is too big for the thread's stack?
 dataesp   = (void *)((uintptr_t)useresp-offsetof(struct stackframe,data));
 useresp   = (void *)((uintptr_t)dataesp-bufsize);
 userstack = (struct stackframe *)TRANSLATE(useresp);
 kassertmem(userstack,bufsize+offsetof(struct stackframe,data));
 // Copy the given buffer data on the new thread's stack
 memcpy(userstack->data,buf,bufsize);
 userstack->returnaddr   = NULL;
 userstack->datap        = dataesp;
 ktask_setupuser(entry.fd_task,useresp,*(void **)&thread_main);
 kproc_unlock(callerctx,KPROC_LOCK_SHM);
 if (flags&KTASK_NEW_FLAG_JOIN) {
  void *exitcode;
  /* Immediatly join the task */
  if (flags&KTASK_NEW_FLAG_SUSPENDED) {
   error = ktask_resume_k(entry.fd_task);
   if __unlikely(KE_ISERR(error)) goto err_task;
  }
  error = ktask_join(entry.fd_task,&exitcode);
  ktask_decref(entry.fd_task);
  if (KE_ISOK(error) && arg &&
      copy_to_user(arg,&exitcode,sizeof(exitcode))
      ) error = KE_FAULT;
  goto end;
 }
 entry.fd_type = KFDTYPE_TASK;
 if (!(flags&KTASK_NEW_FLAG_SUSPENDED)) {
  asserte(KE_ISOK(ktask_incref(entry.fd_task)));
  error = kproc_insfd_inherited(callerctx,&fd,&entry);
  if __unlikely(KE_ISERR(error)) {
   ktask_decref(entry.fd_task);
err_task:
   ktask_decref(entry.fd_task);
   goto end;
  }
  __evalexpr(ktask_resume_k(entry.fd_task)); // Ignore errors in this
  ktask_decref(entry.fd_task);
 } else {
  error = kproc_insfd_inherited(callerctx,&fd,&entry);
  if __unlikely(KE_ISERR(error)) { ktask_decref(entry.fd_task); goto end; }
 }
 error = fd;
end:
 KTASK_CRIT_END
 RETURN(error);
}

/* _syscall2(kerrno_t,ktask_fork,int *,childfd,__u32,flags); */
SYSCALL(sys_ktask_fork) {
 LOAD2(uintptr_t *,U0(childfd_or_exitcode),
       __u32      ,K (flags));
 struct kfdentry entry; kerrno_t error; int fd;
 struct kproc *caller = kproc_self();
 KTASK_CRIT_BEGIN_FIRST
 /* Make sure the caller is allowed to, if they want to root-fork. */
 /* TODO: SECURITY: RACECONDITION:
  * Must suspend all threads but the calling one
  * for the during of a root-fork being performed.
  * This is required to assure consistency of memory
  * after it has been checked that the page directory
  * entry associated with the calling EIP has not been
  * modified.
  * When we don't suspend anything, an application
  * could theoretically inject malicious code after
  * the unchanged check has been performed, but before
  * the memory has actually been marked as copy-on-write.
  * TODO: This will also required to prevent a race-condition
  *       where a process can still modify its memory, before
  *       it has been copied into a new process, but after
  *       fork() has already started doing work.
  * TODO: To prevent other processes from accessing
  *       the calling process's memory, we can simply
  *       leave KPROC_LOCK_SHM locked.
  */
 if (flags&KTASK_NEW_FLAG_ROOTFORK &&
     KE_ISERR(kproc_canrootfork(caller,(void *)regs->regs.eip))
     ) { error = KE_ACCES; goto end; }
 entry.fd_task = ktask_fork(flags,&regs->regs);
 if __unlikely(!entry.fd_task) { error = KE_NOMEM; goto end; }
 if (!(flags&KTASK_NEW_FLAG_SUSPENDED) || !childfd_or_exitcode) {
  error = ktask_resume_k(entry.fd_task);
  if __unlikely(KE_ISERR(error)) goto decentry;
 }
 if (childfd_or_exitcode) {
  entry.fd_attr = KFD_ATTR(KFDTYPE_TASK,KFD_FLAG_NONE);
  error = kproc_insfd_inherited(caller,&fd,&entry);
  if __unlikely(KE_ISERR(error)) goto decentry;
  *childfd_or_exitcode = fd;
  error = KS_UNCHANGED;
 } else {
  error = KS_UNCHANGED;
decentry:
  ktask_decref(entry.fd_task);
 }
end:
 KTASK_CRIT_END
 RETURN(error);
}

/*< _syscall4(kerrno_t,ktask_exec,char const *,path,__size_t,pathmax,struct kexecargs const *,args,__u32,flags); */
SYSCALL(sys_ktask_exec) {
 struct kexecargs args;
 LOAD4(char const       *,K(path),
       size_t            ,K(pathmax),
       struct kexecargs *,K(uargs),
       __u32             ,K(flags));
 kerrno_t error; struct kshlib *lib; char *given_path;
 char const *default_argv[1];
 KTASK_CRIT_BEGIN_FIRST
 /* TODO: Don't use strndup here. */
 given_path = user_strndup(path,pathmax);
 if __unlikely(!given_path) { error = KE_NOMEM; goto end; }
 if (uargs) {
  if __unlikely(copy_from_user(&args,uargs,sizeof(args))) {
   error = KE_FAULT;
   goto end_path2;
  }
 } else {
  default_argv[0] = given_path;
  args.ea_argc    = 1;
  args.ea_argv    = default_argv;
  args.ea_arglenv = NULL;
  args.ea_envc    = 0;
  args.ea_environ = NULL;
  args.ea_envlenv = NULL;
 }
 error = kshlib_openexe(given_path,(size_t)-1,&lib,
                       (flags&KTASK_EXEC_FLAG_SEARCHPATH)!=0);
 if (uargs) free(given_path);
 if __unlikely(KE_ISERR(error)) goto end_path;
 KTASK_NOINTR_BEGIN
 error = kproc_exec(lib,&regs->regs,&args,!uargs);
 KTASK_NOINTR_END
 kshlib_decref(lib);
end_path:  if (!uargs) end_path2: free(given_path);
end:
 KTASK_CRIT_END
 RETURN(error);
}

/*< _syscall5(kerrno_t,ktask_exec,int,fd,size_t,argc,char const *const *,argv,char const *const *,envp,__u32,flags); */
SYSCALL(sys_ktask_fexec) {
 struct kexecargs args;
 LOAD3(int               ,K(fd),
       struct kexecargs *,K(uargs),
       __u32             ,K(flags));
 kerrno_t error; struct kshlib *lib; struct kfile *fp;
 char *default_arg0; char const *default_argv[1];
 KTASK_CRIT_BEGIN_FIRST
 fp = kproc_getfdfile(kproc_self(),fd);
 if __unlikely(!fp) { error = KE_BADF; goto end; }
 if (uargs) {
  if __unlikely(copy_from_user(&args,uargs,sizeof(args))) {
   error = KE_FAULT;
end_fp:
   kfile_decref(fp);
   goto end;
  }
  default_arg0 = NULL;
 } else {
  default_arg0    = kfile_getmallname(fp);
  if __unlikely(!default_arg0) { error = KE_NOMEM; goto end_fp; }
  default_argv[0] = default_arg0;
  args.ea_argc    = 1;
  args.ea_argv    = default_argv;
  args.ea_arglenv = NULL;
  args.ea_envc    = 0;
  args.ea_environ = NULL;
  args.ea_envlenv = NULL;
 }
 (void)flags; // TODO
 error = kshlib_fopenfile(fp,&lib);
 kfile_decref(fp);
 if __unlikely(KE_ISERR(error)) goto end_path;
 KTASK_NOINTR_BEGIN
 error = kproc_exec(lib,&regs->regs,&args,!uargs);
 KTASK_NOINTR_END
 kshlib_decref(lib);
end_path:  if (default_arg0) free(default_arg0);
end:
 KTASK_CRIT_END
 RETURN(error);
}


/*< _syscall1(void *,ktask_gettls,__ktls_t,id); */
SYSCALL(sys_ktask_gettls) {
 LOAD1(__ktls_t,K(slot));
 RETURN(ktask_gettls(ktask_self(),slot));
}

/*< _syscall3(kerrno_t,ktask_settls,__ktls_t,id,void *,value,void **,oldvalue); */
SYSCALL(sys_ktask_settls) {
 LOAD3(__ktls_t ,K (slot),
       void    *,K (value),
       void   **,U0(oldvalue));
 kerrno_t error;
 KTASK_CRIT_BEGIN_FIRST
 error = ktask_settls(ktask_self(),slot,value,oldvalue);
 KTASK_CRIT_END
 RETURN(error);
}

/*< _syscall2(void *,ktask_gettls,int,self,ktls_t,slot); */
SYSCALL(sys_ktask_gettlsof) {
 LOAD2(int     ,K(taskfd),
       __ktls_t,K(slot));
 void *result; struct ktask *task;
 if (taskfd == KFD_TASKSELF) RETURN(ktask_gettls(ktask_self(),slot));
 KTASK_CRIT_BEGIN_FIRST
 task = kproc_getfdtask(kproc_self(),taskfd);
 if __unlikely(!task) result = NULL;
 else {
  result = ktask_accessgm(task)
   ? ktask_gettls(task,slot) : NULL;
  ktask_decref(task);
 }
 KTASK_CRIT_END
 RETURN(result);
}

/*< _syscall4(kerrno_t,ktask_settls,int,self,ktls_t,slot,void *,value,void **,oldvalue); */
SYSCALL(sys_ktask_settlsof) {
 LOAD4(int     ,K (taskfd),
       __ktls_t,K (slot),
       void   *,K (value),
       void  **,U0(oldvalue));
 kerrno_t error; struct ktask *task;
 KTASK_CRIT_BEGIN_FIRST
 task = kproc_getfdtask(kproc_self(),taskfd);
 if __unlikely(!task) error = KE_BADF;
 else {
  error = ktask_accesssm(task)
   ? ktask_settls(task,slot,value,oldvalue)
   : KE_ACCES;
  ktask_decref(task);
 }
 KTASK_CRIT_END
 RETURN(error);
}


__DECL_END

#endif /* !__KOS_KERNEL_SYSCALL_SYSCALL_KTASK_C_INL__ */

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
#ifndef __KOS_KERNEL_SCHED_TASK_SYSCALL_C_INL__
#define __KOS_KERNEL_SCHED_TASK_SYSCALL_C_INL__ 1

#include <kos/compiler.h>
#include <kos/config.h>
#include <kos/timespec.h>
#include <kos/kernel/syscall.h>
#include <kos/kernel/task.h>
#include <kos/kernel/sched_yield.h>
#include <kos/kernel/proc.h>
#include <kos/kernel/linker.h>
#include <kos/kernel/fs/file.h>
#include <stddef.h>
#include <kos/syslog.h>

__DECL_BEGIN

KSYSCALL_DEFINE0(kerrno_t,ktask_yield) {
 return ktask_tryyield();
}

KSYSCALL_DEFINE_EX3(c,kerrno_t,ktask_setalarm,int,taskfd,
                    __user struct timespec const *,abstime,
                    __user struct timespec *,oldabstime) {
 struct ktask *task; kerrno_t error;
 struct timespec kernel_abstime,kernel_oldabstime;
 KTASK_CRIT_MARK
 if (abstime && __unlikely(copy_from_user(&kernel_abstime,
     abstime,sizeof(struct timespec)))) return KE_FAULT;
 task = kproc_getfdtask(kproc_self(),taskfd);
 if __unlikely(!task) return KE_BADF;
 error = ktask_setalarm(task,
                        abstime ? &kernel_abstime : NULL,
                        oldabstime ? &kernel_oldabstime : NULL);
 ktask_decref(task);
 if (__likely  (KE_ISOK(error)) && oldabstime &&
     __unlikely(copy_to_user(oldabstime,&kernel_oldabstime,
                sizeof(struct timespec))
                )) error = KE_FAULT;;
 return error;
}

KSYSCALL_DEFINE_EX2(c,kerrno_t,ktask_getalarm,int,taskfd,
                    __user struct timespec *,abstime) {
 struct ktask *task; kerrno_t error;
 struct timespec kernel_abstime;
 KTASK_CRIT_MARK
 task = kproc_getfdtask(kproc_self(),taskfd);
 if __unlikely(!task) return KE_BADF;
 error = ktask_getalarm(task,&kernel_abstime);
 ktask_decref(task);
 if (__likely(KE_ISOK(error)) &&
     __unlikely(copy_to_user(abstime,&kernel_abstime,sizeof(struct timespec)))
     ) error = KE_FAULT;
 return error;
}

KSYSCALL_DEFINE_EX2(c,kerrno_t,ktask_abssleep,int,taskfd,
                    __user struct timespec const *,abstime) {
 struct ktask *task; kerrno_t error;
 struct timespec kernel_abstime;
 KTASK_CRIT_MARK
 task = kproc_getfdtask(kproc_self(),taskfd);
 if __unlikely(!task) return KE_BADF;
 if (abstime) {
  if __unlikely(copy_from_user(&kernel_abstime,abstime,sizeof(struct timespec))) error = KE_FAULT;
  else error = ktask_abssleep(task,&kernel_abstime);
 } else {
  error = ktask_pause(task);
 }
 ktask_decref(task);
 return error;
}

KSYSCALL_DEFINE_EX3(c,kerrno_t,ktask_terminate,int,taskfd,
                    void *,exitcode,ktaskopflag_t,flags) {
 struct kfdentry fdentry; kerrno_t error;
 KTASK_CRIT_MARK
 error = kproc_getfd(kproc_self(),taskfd,&fdentry);
 if __likely(KE_ISOK(error)) {
  error = kfdentry_terminate(&fdentry,exitcode,flags);
  kfdentry_quit(&fdentry);
 }
 return error;
}

KSYSCALL_DEFINE_EX2(c,kerrno_t,ktask_suspend,
                    int,taskfd,ktaskopflag_t,flags) {
 struct kfdentry fdentry; kerrno_t error;
 struct kproc *caller = kproc_self();
 KTASK_CRIT_MARK
 if (!kproc_hasflag(caller,KPERM_FLAG_SUSPEND_RESUME)) return KE_ACCES;
 error = kproc_getfd(caller,taskfd,&fdentry);
 if __unlikely(KE_ISERR(error)) return error;
 error = kfdentry_suspend(&fdentry,flags);
 kfdentry_quit(&fdentry);
 return error;
}

KSYSCALL_DEFINE_EX2(c,kerrno_t,ktask_resume,
                    int,taskfd,ktaskopflag_t,flags) {
 struct kfdentry fdentry; kerrno_t error;
 struct kproc *caller = kproc_self();
 KTASK_CRIT_MARK
 if (!kproc_hasflag(caller,KPERM_FLAG_SUSPEND_RESUME)) return KE_ACCES;
 error = kproc_getfd(caller,taskfd,&fdentry);
 if __unlikely(KE_ISERR(error)) return error;
 error = kfdentry_resume(&fdentry,flags);
 kfdentry_quit(&fdentry);
 return error;
}

KSYSCALL_DEFINE_EX1(c,int,ktask_openroot,int,taskfd) {
 struct kfdentry fdentry; kerrno_t error; int fd;
 struct kproc *ctx = kproc_self(); struct ktask *result;
 KTASK_CRIT_MARK
 error = kproc_getfd(ctx,taskfd,&fdentry);
 if __unlikely(KE_ISERR(error)) return error;
 error = kfdentry_openprocof(&fdentry,&result);
 kfdentry_quit(&fdentry);
 if __unlikely(KE_ISERR(error)) return error;
 fdentry.fd_task = result; /* Inherit reference. */
 fdentry.fd_attr = KFD_ATTR(KFDTYPE_TASK,KFD_FLAG_NONE);
 error = kproc_insfd_inherited(ctx,&fd,&fdentry);
 if __likely(KE_ISOK(error)) error = fd;
 else ktask_decref(fdentry.fd_task);
 return error;
}

KSYSCALL_DEFINE_EX1(c,int,ktask_openparent,int,taskfd) {
 struct kfdentry fdentry; kerrno_t error; int fd;
 struct kproc *ctx = kproc_self(); struct ktask *result;
 KTASK_CRIT_MARK
 error = kproc_getfd(ctx,taskfd,&fdentry);
 if __unlikely(KE_ISERR(error)) return error;
 error = kfdentry_openparent(&fdentry,&result);
 kfdentry_quit(&fdentry);
 if __unlikely(KE_ISERR(error)) return error;
 fdentry.fd_task = result; /* Inherit reference. */
 fdentry.fd_attr = KFD_ATTR(KFDTYPE_TASK,KFD_FLAG_NONE);
 error = kproc_insfd_inherited(ctx,&fd,&fdentry);
 if __likely(KE_ISOK(error)) error = fd;
 else ktask_decref(fdentry.fd_task);
 return error;
}

KSYSCALL_DEFINE_EX1(c,int,ktask_openproc,int,taskfd) {
 struct kfdentry fdentry; kerrno_t error; int fd;
 struct kproc *ctx = kproc_self(); struct kproc *result;
 KTASK_CRIT_MARK
 error = kproc_getfd(ctx,taskfd,&fdentry);
 if __unlikely(KE_ISERR(error)) return error;
 error = kfdentry_openctx(&fdentry,&result);
 kfdentry_quit(&fdentry);
 if __unlikely(KE_ISERR(error)) return error;
 fdentry.fd_proc = result; /* Inherit reference. */
 fdentry.fd_attr = KFD_ATTR(KFDTYPE_PROC,KFD_FLAG_NONE);
 error = kproc_insfd_inherited(ctx,&fd,&fdentry);
 if __likely(KE_ISOK(error)) error = fd;
 else kproc_decref(fdentry.fd_proc);
 return error;
}

KSYSCALL_DEFINE1(size_t,ktask_getparid,int,taskfd) {
 struct kfdentry fdentry; size_t result;
 switch (taskfd) {
  case KFD_TASKSELF: return ktask_getparid(ktask_self());
  case KFD_PROCSELF:
  case KFD_TASKROOT: return ktask_getparid(ktask_proc());
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
 return result;
}

KSYSCALL_DEFINE1(size_t,ktask_gettid,int,taskfd) {
 struct kfdentry fdentry; size_t result;
 switch (taskfd) {
  case KFD_TASKSELF: return ktask_gettid(ktask_self());
  case KFD_PROCSELF:
  case KFD_TASKROOT: return ktask_gettid(ktask_proc());
  default: break;
 }
 KTASK_CRIT_BEGIN_FIRST
 if __unlikely(KE_ISERR(kproc_getfd(kproc_self(),taskfd,
  &fdentry))) { result = (size_t)-1; goto end; }
 result = kfdentry_gettid(&fdentry);
 kfdentry_quit(&fdentry);
end:
 KTASK_CRIT_END
 return result;
}

KSYSCALL_DEFINE_EX2(c,int,ktask_openchild,int,taskfd,size_t,childid) {
 struct kfdentry fdentry; kerrno_t error; int fd;
 struct kproc *ctx = kproc_self(); struct ktask *result;
 KTASK_CRIT_MARK
 error = kproc_getfd(ctx,taskfd,&fdentry);
 if __unlikely(KE_ISERR(error)) return error;
 error = kfdentry_opentask(&fdentry,childid,&result);
 kfdentry_quit(&fdentry);
 if __unlikely(KE_ISERR(error)) return error;
 fdentry.fd_task = result; /* Inherit reference. */
 fdentry.fd_attr = KFD_ATTR(KFDTYPE_TASK,KFD_FLAG_NONE);
 error = kproc_insfd_inherited(ctx,&fd,&fdentry);
 if __likely(KE_ISOK(error)) error = fd;
 else ktask_decref(fdentry.fd_task);
 return error;
}

KSYSCALL_DEFINE_EX4(c,kerrno_t,ktask_enumchildren,int,taskfd,
                    __user size_t *,idv,__size_t,idc,
                    __user __size_t *,reqidc) {
 struct kfdentry fdentry; kerrno_t error;
 struct kproc *ctx = kproc_self();
 size_t kernel_reqidc;
 KTASK_CRIT_MARK
 error = kproc_getfd(ctx,taskfd,&fdentry);
 if __unlikely(KE_ISERR(error)) return error;
 error = kfdentry_user_enumtasks(&fdentry,idv,idc,
                                 reqidc ? &kernel_reqidc : NULL);
 kfdentry_quit(&fdentry);
 if (__likely(KE_ISOK(error)) && reqidc &&
     __unlikely(copy_to_user(reqidc,&kernel_reqidc,sizeof(kernel_reqidc)))
     ) error = KE_FAULT;
 return error;
}

KSYSCALL_DEFINE_EX2(c,kerrno_t,ktask_getpriority,int,taskfd,
                    __user ktaskprio_t *,result) {
 struct kfdentry fdentry; kerrno_t error;
 struct kproc *caller = kproc_self();
 ktaskprio_t kernel_result;
 KTASK_CRIT_MARK
 if (!kproc_hasflag(caller,KPERM_FLAG_GETPRIO)) return KE_ACCES;
 error = kproc_getfd(caller,taskfd,&fdentry);
 if __unlikely(KE_ISERR(error)) return error;
 error = kfdentry_getpriority(&fdentry,&kernel_result);
 kfdentry_quit(&fdentry);
 if (__likely(KE_ISOK(error)) && 
     __unlikely(copy_to_user(result,&kernel_result,sizeof(kernel_result)))
     ) error = KE_FAULT;
 return error;
}

KSYSCALL_DEFINE_EX2(c,kerrno_t,ktask_setpriority,int,taskfd,ktaskprio_t,value) {
 struct kfdentry fdentry; kerrno_t error;
 struct kproc *caller = kproc_self();
 KTASK_CRIT_MARK
 if (!kproc_hasflag(caller,KPERM_FLAG_SETPRIO)) return KE_ACCES;
 error = kproc_getfd(caller,taskfd,&fdentry);
 if __unlikely(KE_ISERR(error)) return error;
 error = kfdentry_setpriority(&fdentry,value);
 kfdentry_quit(&fdentry);
 return error;
}

KSYSCALL_DEFINE_EX2(rc,kerrno_t,ktask_join,int,taskfd,
                    ktaskopflag_t,flags) {
 kerrno_t error; struct kfdentry fdentry;
 void *kernel_exitcode;
 KTASK_CRIT_MARK
 (void)flags; /* TODO */
 error = kproc_getfd(kproc_self(),taskfd,&fdentry);
 if __unlikely(KE_ISERR(error)) return error;
 if (fdentry.fd_type == KFDTYPE_PROC) {
  struct ktask *roottask = kproc_getroottask(fdentry.fd_proc);
  kproc_decref(fdentry.fd_proc);
  if __unlikely(!roottask) return KE_DESTROYED;
  fdentry.fd_task = roottask;
  fdentry.fd_type = KFDTYPE_TASK;
 }
 if (fdentry.fd_type != KFDTYPE_TASK) { error = KE_BADF; goto end_entry; }
 if __unlikely(fdentry.fd_task == ktask_self()) {
  /* You wan'na deadlock? Fine... But you won't screw over my kernel!
   * >> If a task would attempt to join itself while inside
   *    a critical block, a situation would arise that cannot
   *    be safely resolved.
   * >> The task cannot be terminated from the outside, as
   *    that is the main design feature of a critical block.
   *    Though the task can't exit normally either, as
   *    it is trying to wait for itself to finish.
   * EDIT: Such a situation can now be resolved through use of KE_INTR */
  ktask_decref(fdentry.fd_task); // Drop our reference while inside the block
  KTASK_CRIT_BREAK // Don't join from inside a critical block
  /* The following call should produce a deadlock, but just
   * in case it does manage to return, still handle that
   * case correctly.
   * NOTE: The call is still safe though, as the while we are still
   *       running, we own a reference to ktask_self(), which itself
   *       is immutable and known to be equal to 'task'. */
  assert(fdentry.fd_task == ktask_self());
  /* This shouldn't return, but just in case it does... */
  error = ktask_join(fdentry.fd_task,&kernel_exitcode);
  regs->regs.ecx = (uintptr_t)kernel_exitcode;
  return error;
 }
 /* ----: Somehow instruct the tasking system to drop a reference
  *       to the task once it starts joining, while also doing so
  *       outside of the critical block we build.
  * ----: Also update the code below to do this.
  * >> NOTE: Another option would be to store our reference to
  *          'task' somewhere in 'ktask_self()' and instruct
  *          whatever task might be trying to terminate us to
  *          decref that reference for us in case we'll never
  *          get around to doing so.
  *    NOTE: This is probably the better solution as it gives
  *          the kernel a safe way of holding references without
  *          being forced to stay inside a critical block.
  * EDIT: This problem was solved with the introduction of the KE_INTR error. */
 error = ktask_join(fdentry.fd_task,&kernel_exitcode);
 regs->regs.ecx = (uintptr_t)kernel_exitcode;
end_entry:
 kfdentry_quit(&fdentry);
 return error;
}

KSYSCALL_DEFINE_EX2(rc,kerrno_t,ktask_tryjoin,int,taskfd,
                    ktaskopflag_t,flags) {
 kerrno_t error; struct ktask *task;
 void *kernel_exitcode;
 KTASK_CRIT_MARK
 (void)flags; /* TODO */
 task = kproc_getfdtask(kproc_self(),taskfd);
 if __unlikely(!task) return KE_BADF;
 error = ktask_tryjoin(task,&kernel_exitcode);
 regs->regs.ecx = (uintptr_t)kernel_exitcode;
 ktask_decref(task);
 return error;
}

KSYSCALL_DEFINE_EX3(rc,kerrno_t,ktask_timedjoin,int,taskfd,
                    __user struct timespec const *,abstime,
                    ktaskopflag_t,flags) {
 kerrno_t error; struct ktask *task;
 struct timespec kernel_abstime;
 void *kernel_exitcode;
 KTASK_CRIT_MARK
 (void)flags; /* TODO */
 if __unlikely(copy_from_user(&kernel_abstime,abstime,
                              sizeof(struct timespec))
               ) return KE_FAULT;
 task = kproc_getfdtask(kproc_self(),taskfd);
 if __unlikely(!task) return KE_BADF;
 error = ktask_timedjoin(task,&kernel_abstime,&kernel_exitcode);
 regs->regs.ecx = (uintptr_t)kernel_exitcode;
 ktask_decref(task);
 return error;
}

KSYSCALL_DEFINE_EX4(c,int,ktask_newthread,__user ktask_threadfun_t,thread_main,
                    void *,closure,__u32,flags,__user void **,arg) {
 COMPILER_PACK_PUSH(1)
 struct __packed stackframe {
  void *returnaddr;
  void *closure;
 };
 COMPILER_PACK_POP
 void *useresp; struct kfdentry entry;
 int fd; kerrno_t error;
 struct stackframe *userstack;
 struct ktask *caller = ktask_self();
 struct kproc *proc = ktask_getproc(caller);
 KTASK_CRIT_MARK
 error = krwlock_beginwrite(&proc->p_shm.s_lock);
 if __unlikely(KE_ISERR(error)) return error;
 if __unlikely(!kpagedir_ismappedp(kproc_getpagedir(proc),
                                  *(void **)&thread_main)
               ) { error = KE_FAULT; goto err_pdlock; }
 if (flags&KTASK_NEW_FLAG_UNREACHABLE) {
  struct ktask *parent = kproc_getbarrier_r(proc,KSANDBOX_BARRIER_NOSETMEM);
  if __unlikely(!parent) { error = KE_DESTROYED; goto err_pdlock; }
  entry.fd_task = ktask_newuser(parent,proc,&useresp
                                ,KTASK_STACK_SIZE_DEF
                                ,KTASK_STACK_SIZE_DEF);
  ktask_decref(parent);
 } else {
  entry.fd_task = ktask_newuser(caller,proc,&useresp
                                ,KTASK_STACK_SIZE_DEF
                                ,KTASK_STACK_SIZE_DEF);
 }
 if __unlikely(!entry.fd_task) {
  error = KE_NOMEM;
err_pdlock:
  error = krwlock_endwrite(&proc->p_shm.s_lock);
  return error;
 }
 userstack = (struct stackframe *)kpagedir_translate(caller->t_epd,(struct stackframe *)useresp-1);
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
         ,kpagedir_translate(caller->t_epd,entry.fd_task->t_esp)
         ,entry.fd_task->t_kstackvp
         ,entry.fd_task->t_ustackvp
         ,entry.fd_task->t_ustacksz
         ,entry.fd_task->t_kstack
         ,entry.fd_task->t_kstackend);
 krwlock_endwrite(&proc->p_shm.s_lock);
 if (flags&KTASK_NEW_FLAG_JOIN) {
  void *exitcode;
  /* Immediately join the task */
  if (flags&KTASK_NEW_FLAG_SUSPENDED) {
   error = ktask_resume_k(entry.fd_task);
   if __unlikely(KE_ISERR(error)) goto err_task;
  }
  error = ktask_join(entry.fd_task,&exitcode);
  ktask_decref(entry.fd_task);
  if (KE_ISOK(error) && arg &&
      copy_to_user(arg,&exitcode,sizeof(exitcode))
      ) error = KE_FAULT;
  return error;
 }
 entry.fd_type = KFDTYPE_TASK;
 if (!(flags&KTASK_NEW_FLAG_SUSPENDED)) {
  asserte(KE_ISOK(ktask_incref(entry.fd_task)));
  error = kproc_insfd_inherited(proc,&fd,&entry);
  if __unlikely(KE_ISERR(error)) {
   ktask_decref(entry.fd_task);
err_task:
   ktask_decref(entry.fd_task);
   return error;
  }
  __evalexpr(ktask_resume_k(entry.fd_task)); // Ignore errors in this
  ktask_decref(entry.fd_task);
 } else {
  error = kproc_insfd_inherited(proc,&fd,&entry);
  if __unlikely(KE_ISERR(error)) {
   ktask_decref(entry.fd_task);
   return error;
  }
 }
 return fd;
}

KSYSCALL_DEFINE_EX5(c,int,ktask_newthreadi,__user ktask_threadfun_t,thread_main,
                    __user void const *,buf,size_t,bufsize,__u32,flags,
                    __user void **,arg) {
 COMPILER_PACK_PUSH(1)
 struct __packed stackframe {
  __user void *returnaddr;
  __user void *datap;
         __u8  data[1024];
 };
 COMPILER_PACK_POP
 __user void *useresp,*dataesp; struct kfdentry entry;
 int fd; kerrno_t error; struct stackframe *userstack;
 struct ktask *caller = ktask_self();
 struct kproc *proc   = ktask_getproc(caller);
 KTASK_CRIT_MARK
 buf = kpagedir_translate(caller->t_epd,buf); /* TODO: Unsafe. */
 if (!bufsize) buf = NULL;
 error = krwlock_beginwrite(&proc->p_shm.s_lock);
 if __unlikely(KE_ISERR(error)) return error;
 if __unlikely(!kpagedir_ismappedp(kproc_getpagedir(proc),
                                  *(void **)&thread_main)
               ) { error = KE_FAULT; goto err_pdlock; }
 if (flags&KTASK_NEW_FLAG_UNREACHABLE) {
  struct ktask *parent = kproc_getbarrier_r(proc,KSANDBOX_BARRIER_NOSETMEM);
  if __unlikely(!parent) { error = KE_DESTROYED; goto err_pdlock; }
  entry.fd_task = ktask_newuser(parent,proc,&useresp
                                ,KTASK_STACK_SIZE_DEF
                                ,KTASK_STACK_SIZE_DEF);
  ktask_decref(parent);
 } else {
  entry.fd_task = ktask_newuser(caller,proc,&useresp
                                ,KTASK_STACK_SIZE_DEF
                                ,KTASK_STACK_SIZE_DEF);
 }
 if __unlikely(!entry.fd_task) {
  error = KE_NOMEM;
err_pdlock:
  krwlock_endwrite(&proc->p_shm.s_lock);
  return error;
 }
 /* TODO: What if 'bufsize' is too big for the thread's stack? */
 dataesp   = (void *)((uintptr_t)useresp-offsetof(struct stackframe,data));
 useresp   = (void *)((uintptr_t)dataesp-bufsize);
 userstack = (struct stackframe *)kpagedir_translate(caller->t_epd,useresp);
 kassertmem(userstack,bufsize+offsetof(struct stackframe,data));
 /* Copy the given buffer data on the new thread's stack. */
 memcpy(userstack->data,buf,bufsize);
 userstack->returnaddr   = NULL;
 userstack->datap        = dataesp;
 ktask_setupuser(entry.fd_task,useresp,*(void **)&thread_main);
 krwlock_endwrite(&proc->p_shm.s_lock);
 if (flags&KTASK_NEW_FLAG_JOIN) {
  void *exitcode;
  /* Immediately join the task */
  if (flags&KTASK_NEW_FLAG_SUSPENDED) {
   error = ktask_resume_k(entry.fd_task);
   if __unlikely(KE_ISERR(error)) goto err_task;
  }
  error = ktask_join(entry.fd_task,&exitcode);
  ktask_decref(entry.fd_task);
  if (KE_ISOK(error) && arg &&
      copy_to_user(arg,&exitcode,sizeof(exitcode))
      ) error = KE_FAULT;
  return error;
 }
 entry.fd_type = KFDTYPE_TASK;
 if (!(flags&KTASK_NEW_FLAG_SUSPENDED)) {
  asserte(KE_ISOK(ktask_incref(entry.fd_task)));
  error = kproc_insfd_inherited(proc,&fd,&entry);
  if __unlikely(KE_ISERR(error)) {
   ktask_decref(entry.fd_task);
err_task:
   ktask_decref(entry.fd_task);
   return error;
  }
  __evalexpr(ktask_resume_k(entry.fd_task)); // Ignore errors in this
  ktask_decref(entry.fd_task);
 } else {
  error = kproc_insfd_inherited(proc,&fd,&entry);
  if __unlikely(KE_ISERR(error)) {
   ktask_decref(entry.fd_task);
   return error;
  }
 }
 return fd;
}


KSYSCALL_DEFINE_EX2(rc,kerrno_t,ktask_fork,
                    __user int *,childfd_or_exitcode,
                    __u32,flags) {
 struct kfdentry entry; kerrno_t error; int fd;
 struct kproc *caller = kproc_self();
 KTASK_CRIT_MARK
 STATIC_ASSERT(KPERM_FLAG_GETGROUP(KPERM_FLAG_CANFORK) ==
               KPERM_FLAG_GETGROUP(KPERM_FLAG_CANFORK|KPERM_FLAG_CANROOTFORK));
 /* Make sure the caller is allowed to fork(). */
 if (!kproc_hasflag(caller,
    ((flags&KTASK_NEW_FLAG_ROOTFORK) == KTASK_NEW_FLAG_ROOTFORK)
   ? (KPERM_FLAG_CANFORK|KPERM_FLAG_CANROOTFORK)
   : (KPERM_FLAG_CANFORK))) return KE_ACCES;
 error = ktask_fork(flags,&regs->regs,&entry.fd_task);
 if __unlikely(KE_ISERR(error)) return error;
 if (!(flags&KTASK_NEW_FLAG_SUSPENDED) || !childfd_or_exitcode) {
  error = ktask_resume_k(entry.fd_task);
  if __unlikely(KE_ISERR(error)) goto decentry;
 }
 if (childfd_or_exitcode) {
  uintptr_t fd_ptr;
  entry.fd_attr = KFD_ATTR(KFDTYPE_TASK,KFD_FLAG_NONE);
  error = kproc_insfd_inherited(caller,&fd,&entry);
  if __unlikely(KE_ISERR(error)) goto decentry;
  fd_ptr = fd;
  error = copy_to_user(childfd_or_exitcode,&fd_ptr,sizeof(uintptr_t)
                       ) ? KE_FAULT : KS_UNCHANGED;
 } else {
  error = KS_UNCHANGED;
decentry:
  ktask_decref(entry.fd_task);
 }
 return error;
}

/*< _syscall4(kerrno_t,ktask_exec,char const *,path,__size_t,pathmax,struct kexecargs const *,args,__u32,flags); */
KSYSCALL_DEFINE_EX4(rc,kerrno_t,ktask_exec,__user char const *,path,__size_t,pathmax,
                    __user struct kexecargs const *,args,__u32,flags) {
 struct kexecargs kernel_args;
 kerrno_t error; struct kshlib *lib; char *given_path;
 char const *default_argv[1];
 KTASK_CRIT_MARK
 /* TODO: Don't use strndup here. */
 given_path = user_strndup(path,pathmax);
 if __unlikely(!given_path) return KE_NOMEM;
 if (args) {
  if __unlikely(copy_from_user(&kernel_args,args,sizeof(kernel_args))) {
   error = KE_FAULT;
   goto end_path2;
  }
 } else {
  default_argv[0] = given_path;
  kernel_args.ea_argc    = 1;
  kernel_args.ea_argv    = default_argv;
  kernel_args.ea_arglenv = NULL;
  kernel_args.ea_envc    = 0;
  kernel_args.ea_environ = NULL;
  kernel_args.ea_envlenv = NULL;
 }
 error = kshlib_openexe(given_path,(size_t)-1,&lib,flags);
 if (args) free(given_path);
 if __unlikely(KE_ISERR(error)) goto end_path;
 KTASK_NOINTR_BEGIN
 error = kproc_exec(lib,&regs->regs,&kernel_args,!args);
 KTASK_NOINTR_END
 kshlib_decref(lib);
end_path:  if (!args) end_path2: free(given_path);
 return error;
}

KSYSCALL_DEFINE_EX3(rc,kerrno_t,ktask_fexec,int,fd,
                    __user struct kexecargs *,args,
                    __u32,flags) {
 struct kexecargs kernel_args;
 kerrno_t error; struct kshlib *lib; struct kfile *fp;
 char *default_arg0; char const *default_argv[1];
 KTASK_CRIT_MARK
 fp = kproc_getfdfile(kproc_self(),fd);
 if __unlikely(!fp) return KE_BADF;
 if (args) {
  if __unlikely(copy_from_user(&kernel_args,args,sizeof(args))) {
   error = KE_FAULT;
end_fp:
   kfile_decref(fp);
   return error;
  }
  default_arg0 = NULL;
 } else {
  default_arg0    = kfile_getmallname(fp);
  if __unlikely(!default_arg0) { error = KE_NOMEM; goto end_fp; }
  default_argv[0] = default_arg0;
  kernel_args.ea_argc    = 1;
  kernel_args.ea_argv    = default_argv;
  kernel_args.ea_arglenv = NULL;
  kernel_args.ea_envc    = 0;
  kernel_args.ea_environ = NULL;
  kernel_args.ea_envlenv = NULL;
 }
 (void)flags; // TODO
 error = kshlib_fopenfile(fp,&lib);
 kfile_decref(fp);
 if __unlikely(KE_ISERR(error)) goto end_path;
 KTASK_NOINTR_BEGIN
 error = kproc_exec(lib,&regs->regs,&kernel_args,!args);
 KTASK_NOINTR_END
 kshlib_decref(lib);
end_path:  if (default_arg0) free(default_arg0);
 return error;
}

__DECL_END

#endif /* !__KOS_KERNEL_SCHED_TASK_SYSCALL_C_INL__ */

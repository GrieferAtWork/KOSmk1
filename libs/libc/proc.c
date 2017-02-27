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
#ifndef __PROC_C__
#define __PROC_C__ 1

#include <kos/config.h>
#ifndef __KERNEL__
#include <assert.h>
#include <errno.h>
#include <features.h>
#include <kos/attr.h>
#include <kos/compiler.h>
#include <kos/kernel/debug.h>
#include <kos/task.h>
#include <kos/time.h>
#include <kos/timespec.h>
#include <malloc.h>
#include <proc.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

__DECL_BEGIN

__public void task_yield(void) { ktask_yield(); }
__public int task_pause(task_t task) { return task_abssleep(task,NULL); }
__public int task_sleep(task_t task, struct timespec *__restrict timeout) {
 struct timespec abstime; kerrno_t error;
 kassertobj(timeout);
 error = ktime_getnoworcpu(&abstime);
 if __likely(KE_ISOK(error)) {
  __timespec_add(&abstime,timeout);
  return task_abssleep(task,&abstime);
 }
 __set_errno(-error);
 return -1;
}
__public int task_abssleep(task_t task, struct timespec *__restrict abstime) {
 kerrno_t error = ktask_abssleep(task,abstime);
 if __likely(KE_ISOK(error)) return 0;
 __set_errno(-error);
 return -1;
}

__public int task_setalarm(task_t task, struct timespec const *__restrict timeout) {
 struct timespec abstime; int error;
 kassertobjnull(timeout);
 if (!timeout) return task_setabsalarm(task,NULL);
 error = ktime_getnoworcpu(&abstime);
 if __unlikely(KE_ISERR(error)) { __set_errno(-error); return -1; }
 __timespec_add(&abstime,timeout);
 return task_setabsalarm(task,&abstime);
}
__public int task_getalarm(task_t task, struct timespec *__restrict timeout) {
 struct timespec tmnow,abstime; int error;
 kassertobj(timeout);
 error = task_getabsalarm(task,&abstime);
 if __likely(error == 0) {
  error = ktime_getnoworcpu(&tmnow);
  if __unlikely(KE_ISERR(error)) { __set_errno(-error); return -1; }
  __timespec_sub(&abstime,&tmnow);
  *timeout = abstime;
 }
 return error;
}
__public int task_setabsalarm(task_t task, struct timespec const *__restrict abstime) {
 kerrno_t error;
 kassertobjnull(abstime);
 error = ktask_setalarm(task,abstime,NULL);
 if __likely(KE_ISOK(error)) return 0;
 __set_errno(-error);
 return -1;
}
__public int task_getabsalarm(task_t task, struct timespec *__restrict abstime) {
 kerrno_t error;
 kassertobjnull(abstime);
 error = ktask_getalarm(task,abstime);
 if __likely(KE_ISOK(error)) {
  if (error == KE_OK) return 0;
  error = ktime_getnoworcpu(abstime);
  if __likely(KE_ISOK(error)) return 0;
 }
 __set_errno(-error);
 return -1;
}


__public int task_terminate(task_t task, void *exitcode) {
 kerrno_t error = ktask_terminate(task,exitcode,KTASKOPFLAG_NONE);
 if __likely(KE_ISOK(error)) return 0;
 __set_errno(-error);
 return -1;
}
__public int task_suspend(task_t task) {
 kerrno_t error = ktask_suspend(task,KTASKOPFLAG_NONE);
 if __likely(KE_ISOK(error)) return 0;
 __set_errno(-error);
 return -1;
}
__public int task_resume(task_t task) {
 kerrno_t error = ktask_resume(task,KTASKOPFLAG_NONE);
 if __likely(KE_ISOK(error)) return 0;
 __set_errno(-error);
 return -1;
}

__public void task_exit(void *exitcode) { ktask_exit(exitcode); }
__public void proc_exit(uintptr_t exitcode) { kproc_exit((void *)exitcode); }

__public task_t task_openparent(task_t self) {
 ktask_t result = ktask_openparent(self);
 if __likely(KE_ISOK(result)) return result;
 __set_errno(-result);
 return -1;
}
__public task_t task_openroot(task_t self) {
 proc_t result = ktask_openroot(self);
 if __likely(KE_ISOK(result)) return result;
 __set_errno(-result);
 return -1;
}
__public proc_t task_openproc(task_t self) {
 proc_t result = ktask_openproc(self);
 if __likely(KE_ISOK(result)) return result;
 __set_errno(-result);
 return -1;
}
__public size_t task_getparid(task_t self) { return ktask_getparid(self); }
__public tid_t task_gettid(task_t self) { return ktask_gettid(self); }
__public task_t task_openchild(task_t self, size_t parid) {
 task_t result = ktask_openchild(self,parid);
 if __likely(KE_ISOK(result)) return result;
 __set_errno(-result);
 return -1;
}
__public ssize_t task_enumchildren(task_t self, size_t *__restrict idv, size_t idc) {
 size_t reqsize; kerrno_t error;
 error = ktask_enumchildren(self,idv,idc,&reqsize);
 if __unlikely(KE_ISERR(error)) { __set_errno(-error); return -1; }
 if (reqsize > idc) { __set_errno(EBUFSIZ); return -1; }
 return (ssize_t)reqsize;
}

__public int task_getpriority(task_t self, taskprio_t *__restrict result) {
 kerrno_t error = ktask_getpriority(self,result);
 if __likely(KE_ISOK(error)) return 0;
 __set_errno(-error);
 return -1;
}

__public int task_setpriority(task_t self, taskprio_t value) {
 kerrno_t error = ktask_setpriority(self,value);
 if __likely(KE_ISOK(error)) return 0;
 __set_errno(-error);
 return -1;
}
__public int task_join(task_t self, void **exitcode) {
 kerrno_t error = ktask_join(self,exitcode,KTASKOPFLAG_NONE);
 if __likely(KE_ISOK(error)) return 0;
 __set_errno(-error);
 return -1;
}
__public int task_tryjoin(task_t self, void **exitcode) {
 kerrno_t error = ktask_tryjoin(self,exitcode,KTASKOPFLAG_NONE);
 if __likely(KE_ISOK(error)) return 0;
 __set_errno(-error);
 return -1;
}
__public int task_timedjoin(task_t self, struct timespec const *__restrict abstime,
                            void **exitcode) {
 kerrno_t error = ktask_timedjoin(self,abstime,exitcode,KTASKOPFLAG_NONE);
 if __likely(KE_ISOK(error)) return 0;
 __set_errno(-error);
 return -1;
}
__public int task_timoutjoin(task_t self, struct timespec const *__restrict timeout,
                             void **exitcode) {
 struct timespec abstime; kerrno_t error;
 error = ktime_getnoworcpu(&abstime);
 if __likely(KE_ISOK(error)) {
  __timespec_add(&abstime,timeout);
  return task_timedjoin(self,&abstime,exitcode);
 }
 __set_errno(-error);
 return -1;
}
__local char *fd_mallgetattrstring(int fd, kattr_t attr) {
 ssize_t reqsize = PATH_MAX; size_t bufsize;
 char *newbuf,*buf = NULL; do {
  newbuf = (char *)realloc(buf,reqsize);
  if __unlikely(!newbuf) { free(buf); return NULL; }
  bufsize = (size_t)reqsize;
  reqsize = getattr(fd,attr,newbuf,bufsize);
  if (reqsize < 0) { free(newbuf); return NULL; }
  buf = newbuf;
 } while (reqsize > bufsize);
 if ((size_t)reqsize != bufsize) {
  newbuf = (char *)realloc(buf,(size_t)reqsize);
  if __unlikely(!newbuf) __set_errno(EOK);
  else buf = newbuf;
 }
 return buf;
}

__public char *task_getname(task_t self, char *__restrict buf, size_t bufsize) {
 ssize_t reqsize;
 if (!bufsize) return fd_mallgetattrstring(self,KATTR_GENERIC_NAME);
 reqsize = getattr(self,KATTR_GENERIC_NAME,buf,bufsize);
 if __unlikely(reqsize < 0) return NULL;
 if __unlikely(reqsize > bufsize) { __set_errno(ERANGE); return NULL; }
 return buf;
}
__public int task_setname(task_t self, char const *__restrict name) {
 return setattr(self,KATTR_GENERIC_NAME,name,strlen(name));
}

struct taskmain_data {
 task_threadfunc thread_main;
 void           *real_closure;
};

// Use ktask_newthreadi to prevent memory leaks
// resulting from not resuming suspended tasks, or
// terminating a task before it was able to free
// its closure data.
#define USE_NEWTHREADI


static __noreturn void taskmain(struct taskmain_data *data) {
 assertf((uintptr_t)data >= 0x40000000,"%p(%p) %d",
         data,&data,ktask_getparid(ktask_self()));
#ifdef USE_NEWTHREADI
 ktask_exit((*data->thread_main)(data->real_closure));
#else
 {
  struct taskmain_data used_data;
  used_data = *data; (free)(data);
  ktask_exit((*used_data.thread_main)(used_data.real_closure));
 }
#endif
}

__public task_t task_newthread(task_threadfunc thread_main,
                               void *closure, __u32 flags) {
#ifdef USE_NEWTHREADI
 struct taskmain_data closuredata; task_t result;
 closuredata.thread_main = thread_main;
 closuredata.real_closure = closure;
 // Use a wrapper function that allows the user-specified task to return normally
 result = ktask_newthreadi((ktask_threadfun_t)&taskmain,
                            &closuredata,sizeof(closuredata),
                            flags,NULL);
 if __likely(KE_ISOK(result)) return result;
 __set_errno(-result);
 return -1;
#else
 struct taskmain_data *proxy_closure; task_t result;
 proxy_closure = (struct taskmain_data *)(malloc)(sizeof(struct taskmain_data));
 if __unlikely(!proxy_closure) return -1;
 proxy_closure->thread_main = thread_main;
 proxy_closure->real_closure = closure;
 // Use a wrapper function that allows the user-specified task to return normally
 result = ktask_newthread((ktask_threadfun_t)&taskmain,proxy_closure,flags);
 if __likely(KE_ISOK(result)) return result;
 free(proxy_closure);
 __set_errno(-result);
 return -1;
#endif
}

__public task_t task_fork(void) { return task_forkex(TASK_NEWTHREAD_DEFAULT); }
__public task_t task_forkex(__u32 flags) {
 union {
  uintptr_t u;
  intptr_t  s;
 } result;
 kerrno_t error;
 error = ktask_fork(&result.u,KTASK_NEW_FLAG_SUSPENDED);
 if (error == KS_UNCHANGED) {
  // Parent process on success
  if __unlikely(result.u == 0) {
   /* Rare case: We must get a new descriptor.
    * >> We can't return '0' to the caller,
    *    or they'll think they're the child! */
   int newresult = kfd_dup(0,0);
   if __unlikely(KE_ISERR(newresult)) {
    ktask_terminate(0,NULL,KTASKOPFLAG_RECURSIVE);
    __set_errno(-newresult);
    result.s = -1;
   } else {
    result.s = newresult;
    kfd_close(0);
   }
  }
  if (!(flags&TASK_NEWTHREAD_SUSPENDED)) {
   error = ktask_resume((int)result.s,KTASKOPFLAG_NONE);
   if __unlikely(KE_ISERR(error)) {
    __set_errno(-error);
    kfd_close((int)result.s);
    result.s = -1;
   }
  }
  return (int)result.s;
 }
 /* Child process or error */
 if __unlikely(KE_ISERR(error)) {
  __set_errno(-error);
  return -1;
 }
 /* Child process */
 return 0;
}


__public task_t proc_openthread(proc_t proc, tid_t tid) {
 task_t result = kproc_openthread(proc,tid);
 if __likely(KE_ISOK(result)) return result;
 __set_errno(-result);
 return -1;
}
__public ssize_t proc_enumthreads(proc_t proc, tid_t *__restrict tidv, size_t tidc) {
 size_t reqsize; kerrno_t error;
 error = kproc_enumthreads(proc,tidv,tidc,&reqsize);
 if __unlikely(KE_ISERR(error)) { __set_errno(-error); return -1; }
 if (reqsize > tidc) { __set_errno(EBUFSIZ); return -1; }
 return (ssize_t)reqsize;
}
__public int proc_openfd(proc_t proc, int fd, int flags) {
 int result;
 result = kproc_openfd(proc,fd,flags);
 if __likely(KE_ISOK(result)) return result;
 __set_errno(-result);
 return result;
}
__public int proc_openfd2(proc_t proc, int fd, int newfd, int flags) {
 int result;
 result = kproc_openfd2(proc,fd,newfd,flags);
 if __likely(KE_ISOK(result)) return result;
 __set_errno(-result);
 return result;
}
__public ssize_t proc_enumfd(proc_t proc, int *__restrict fdv, size_t fdc) {
 size_t reqsize; kerrno_t error;
 error = kproc_enumfd(proc,fdv,fdc,&reqsize);
 if __unlikely(KE_ISERR(error)) { __set_errno(-error); return -1; }
 if __unlikely(reqsize > fdc) { __set_errno(EBUFSIZ); return -1; }
 return (ssize_t)reqsize;
}
__public proc_t proc_openpid(pid_t pid) {
 proc_t result = kproc_openpid(pid);
 if __likely(KE_ISOK(result)) return result;
 __set_errno(-result);
 return -1;
}
__public pid_t proc_getpid(proc_t proc) {
 pid_t result = kproc_getpid(proc);
 if __likely(KE_ISOK(result)) return result;
 __set_errno(-result);
 return -1;
}
__public ssize_t proc_enumpid(pid_t pidv[], size_t pidc) {
 size_t reqsize; kerrno_t error;
 error = kproc_enumpid(pidv,pidc,&reqsize);
 if __unlikely(KE_ISERR(error)) { __set_errno(-error); return -1; }
 if __unlikely(reqsize > pidc) { __set_errno(EBUFSIZ); return -1; }
 return (ssize_t)reqsize;
}
__public task_t proc_openroot(proc_t proc) {
 task_t result = kproc_openroot(proc);
 if __likely(KE_ISOK(result)) return result;
 __set_errno(-result);
 return -1;
}
__public int proc_barrier(sandbarrier_t level) {
 kerrno_t error = kproc_barrier(level);
 if __likely(KE_ISOK(error)) return 0;
 __set_errno(-error);
 return -1;
}
__public task_t proc_openbarrier(proc_t proc, sandbarrier_t level) {
 task_t result;
 result = kproc_openbarrier(proc,level);
 if __likely(KE_ISOK(result)) return result;
 __set_errno(-result);
 return -1;
}
__public int proc_suspend(proc_t proc) {
 kerrno_t result;
 result = ktask_suspend(proc,KTASKOPFLAG_RECURSIVE);
 if __likely(KE_ISOK(result)) return 0;
 __set_errno(-result);
 return -1;
}
__public int proc_resume(proc_t proc) {
 kerrno_t result;
 result = ktask_resume(proc,KTASKOPFLAG_RECURSIVE);
 if __likely(KE_ISOK(result)) return 0;
 __set_errno(-result);
 return -1;
}
__public int proc_terminate(proc_t proc, uintptr_t exitcode) {
 kerrno_t result;
 result = ktask_terminate(proc,(void *)exitcode,KTASKOPFLAG_RECURSIVE);
 if __likely(KE_ISOK(result)) return 0;
 __set_errno(-result);
 return -1;
}


// Single join the root thread of a processes, assuming recursive termination
__public int proc_join(proc_t proc, uintptr_t *exitcode) {
 kerrno_t error = ktask_join(proc,(void **)exitcode,
                             KTASKOPFLAG_RECURSIVE);
 if __likely(KE_ISOK(error)) return 0;
 __set_errno(-error);
 return -1;
}
__public int proc_tryjoin(proc_t proc, uintptr_t *exitcode) {
 kerrno_t error = ktask_tryjoin(proc,(void **)exitcode,
                                KTASKOPFLAG_RECURSIVE);
 if __likely(KE_ISOK(error)) return 0;
 __set_errno(-error);
 return -1;
}
__public int proc_timedjoin(proc_t proc, struct timespec const *__restrict abstime,
                            uintptr_t *exitcode) {
 kerrno_t error = ktask_timedjoin(proc,abstime,(void **)exitcode,
                                  KTASKOPFLAG_RECURSIVE);
 if __likely(KE_ISOK(error)) return 0;
 __set_errno(-error);
 return -1;
}
__public int proc_timoutjoin(proc_t proc, struct timespec const *__restrict timeout,
                             uintptr_t *exitcode) {
 struct timespec abstime; kerrno_t error;
 error = ktime_getnoworcpu(&abstime);
 if __likely(KE_ISOK(error)) {
  __timespec_add(&abstime,timeout);
  return proc_timedjoin(proc,&abstime,exitcode);
 }
 __set_errno(-error);
 return -1;
}


__public void *tls_get(tls_t slot) {
 return ktask_gettls(slot);
}
__public int tls_set(tls_t slot, void *value) {
 kerrno_t error = ktask_settls(slot,value,NULL);
 if __likely(KE_ISOK(error)) return 0;
 __set_errno(-error);
 return -1;
}
__public tls_t tls_alloc(void) {
 kerrno_t error; tls_t result;
 error = kproc_alloctls(&result);
 // We assume that no process allowed to will ever allocate 2**32 tls handles...
 if __likely(KE_ISOK(error)) return result;
 __set_errno(-error);
 return TLS_ERROR;
}
__public void tls_free(tls_t slot) {
 kproc_freetls(slot);
}

__DECL_END
#endif

#endif /* !__PROC_C__ */

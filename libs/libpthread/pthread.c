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
#ifndef __PTHREAD_C__
#define __PTHREAD_C__ 1

#include <assert.h>
#include <errno.h>
#include <kos/atomic.h>
#include <kos/errno.h>
#include <kos/fd.h>
#include <kos/task.h>
#include <pthread.h>
#include <sched.h>
#include <proc.h>
#include <stdio.h>
#include <stddef.h>

__DECL_BEGIN

struct pthread_maindata {
 void *(*start_routine)(void *);
 void   *arg;
};

static __noreturn void
pthread_main(struct pthread_maindata *data) {
 ktask_exit((*data->start_routine)(data->arg));
}


__public int
pthread_create(pthread_t *__restrict newthread, pthread_attr_t const *attr,
               void *(*start_routine)(void *), void *arg) {
 struct pthread_maindata closure;
 ktask_t result;
 (void)attr; // TODO
 closure.arg           = arg;
 closure.start_routine = start_routine;
 result = ktask_newthreadi((ktask_threadfun_t)&pthread_main,&closure,
                           sizeof(closure),KTASK_NEW_FLAG_NONE,NULL);
 if __unlikely(KE_ISERR(result)) return -result;
 *newthread = result;
 return 0;
}


__noreturn void pthread_exit(void *retval) {
 ktask_exit(retval);
}

__public int
pthread_join(pthread_t th, void **thread_return) {
 kerrno_t error = ktask_join(th,thread_return,0);
 if __unlikely(KE_ISERR(error)) return -error;
 kfd_close(th);
 return 0;
}
__public int
pthread_tryjoin_np(pthread_t th, void **thread_return) {
 kerrno_t error = ktask_tryjoin(th,thread_return,0);
 if __unlikely(KE_ISERR(error)) return -error;
 kfd_close(th);
 return 0;
}
__public int
pthread_timedjoin_np(pthread_t th, void **thread_return,
                     struct timespec const *__restrict abstime) {
 kerrno_t error = ktask_timedjoin(th,abstime,thread_return,0);
 if __unlikely(KE_ISERR(error)) return -error;
 kfd_close(th);
 return 0;
}
__public int pthread_detach(pthread_t th) { return -kfd_close(th); }
__public pthread_t pthread_self(void) { return ktask_self(); }
__public int pthread_equal(pthread_t thread1, pthread_t thread2) { return thread1 == thread2 || kfd_equals(thread1,thread2); }
__public int pthread_setschedprio(pthread_t target_thread, int prio) { return -ktask_setpriority(target_thread,prio); }
__public int pthread_getname_np(pthread_t target_thread, char *__restrict buf, size_t buflen) {
 size_t reqsize; kerrno_t error;
 error = ktask_getname(target_thread,buf,buflen,&reqsize);
 if __unlikely(KE_ISERR(error)) return -error;
 if __unlikely(reqsize > buflen) return ERANGE;
 return 0;
}
__public int
pthread_setname_np(pthread_t target_thread, char const *__restrict name) {
 int error = -ktask_setname(target_thread,name);
 return KE_ISOK(error) ? 0 : -error;
}


__public int pthread_yield(void) { ktask_yield(); return 0; }
__public __COMPILER_ALIAS(sched_yield,pthread_yield);

__public int
pthread_getunique_np(pthread_t *__restrict thread,
                     pthread_id_np_t *__restrict id) {
 return ((*id = ktask_gettid(*thread)) == (size_t)-1) ? EBADF : 0;
}
__public pthread_id_np_t
pthread_getthreadid_np(void) {
 return ktask_gettid(ktask_self());
}
__public int
pthread_setschedparam(pthread_t target_thread, int __unused(policy),
                      struct sched_param const *__restrict param) {
 int error;
 if __unlikely(!param) return EINVAL;
 error = ktask_setpriority(target_thread,(ktaskprio_t)param->sched_priority);
 return KE_ISOK(error) ? 0 : -error;
}
__public int
pthread_getschedparam(pthread_t target_thread, int *policy,
                      struct sched_param *__restrict param) {
 ktaskprio_t prio; int error;
 if __unlikely(!param) return EINVAL;
 *policy = SCHED_RR;
 error = ktask_getpriority(target_thread,&prio);
 param->sched_priority = (int)prio;
 return KE_ISOK(error) ? 0 : -error;
}

__public int
pthread_cancel(pthread_t th) {
 int error = ktask_terminate(th,NULL,KTASKOPFLAG_NONE);
 return KE_ISOK(error) ? 0 : -error;
}

static __atomic int clevel = 0;
__public int pthread_getconcurrency(void) { return katomic_load(clevel); }
__public int pthread_setconcurrency(int level) {
 if __unlikely(level < 0) return EINVAL;
 katomic_store(clevel,level);
 return 0;
}


/* pthread_attr_t */
__public int
pthread_attr_init(pthread_attr_t *__restrict attr) {
 if __unlikely(!attr) return EINVAL;
 /* TODO */
 return 0;
}
__public int
pthread_attr_destroy(pthread_attr_t *__restrict attr) {
 return attr ? 0 : EINVAL;
}
__public int
pthread_attr_getdetachstate(pthread_attr_t const *__restrict attr,
                            int *__restrict detachstate) {
 if __unlikely(!attr || !detachstate) return EINVAL;
 return ENOSYS; /* TODO */
}
__public int
pthread_attr_setdetachstate(pthread_attr_t *__restrict attr,
                            int detachstate) {
 if __unlikely(!attr) return EINVAL;
 return ENOSYS; /* TODO */
}
__public int
pthread_attr_getguardsize(pthread_attr_t const *__restrict attr,
                          size_t *__restrict guardsize) {
 if __unlikely(!attr || !guardsize) return EINVAL;
 return ENOSYS; /* TODO */
}
__public int
pthread_attr_setguardsize(pthread_attr_t *__restrict attr,
                          size_t guardsize) {
 if __unlikely(!attr) return EINVAL;
 return ENOSYS; /* TODO */
}
__public int
pthread_attr_getschedparam(pthread_attr_t const *__restrict attr,
                           struct sched_param *__restrict param) {
 if __unlikely(!attr || !param) return EINVAL;
 return ENOSYS; /* TODO */
}
__public int
pthread_attr_setschedparam(pthread_attr_t *__restrict attr,
                           struct sched_param const *__restrict param) {
 if __unlikely(!attr || !param) return EINVAL;
 return ENOSYS; /* TODO */
}
__public int
pthread_attr_getschedpolicy(pthread_attr_t const *__restrict attr,
                            int *__restrict policy) {
 if __unlikely(!attr || !policy) return EINVAL;
 return ENOSYS; /* TODO */
}
__public int
pthread_attr_setschedpolicy(pthread_attr_t *__restrict attr,
                            int policy) {
 if __unlikely(!attr) return EINVAL;
 return ENOSYS; /* TODO */
}
__public int
pthread_attr_getinheritsched(pthread_attr_t const *__restrict attr,
                             int *__restrict inherit) {
 if __unlikely(!attr || !inherit) return EINVAL;
 return ENOSYS; /* TODO */
}
__public int
pthread_attr_setinheritsched(pthread_attr_t *__restrict attr,
                             int inherit) {
 if __unlikely(!attr) return EINVAL;
 return ENOSYS; /* TODO */
}
__public int
pthread_attr_getscope(pthread_attr_t const *__restrict attr,
                      int *__restrict scope) {
 if __unlikely(!attr || !scope) return EINVAL;
 return ENOSYS; /* TODO */
}
__public int
pthread_attr_setscope(pthread_attr_t *__restrict attr,
                      int scope) {
 if __unlikely(!attr) return EINVAL;
 return ENOSYS; /* TODO */
}
__public int
pthread_attr_getstackaddr(pthread_attr_t const *__restrict attr,
                          void **__restrict stackaddr) {
 if __unlikely(!attr || !stackaddr) return EINVAL;
 return ENOSYS; /* TODO */
}
__public int
pthread_attr_setstackaddr(pthread_attr_t *__restrict attr,
                          void *stackaddr) {
 if __unlikely(!attr) return EINVAL;
 return ENOSYS; /* TODO */
}
__public int
pthread_attr_getstacksize(pthread_attr_t const *__restrict attr,
                          size_t *__restrict stacksize) {
 if __unlikely(!attr || !stacksize) return EINVAL;
 return ENOSYS; /* TODO */
}
__public int
pthread_attr_setstacksize(pthread_attr_t *__restrict attr,
                          size_t stacksize) {
 if __unlikely(!attr) return EINVAL;
 return ENOSYS; /* TODO */
}
__public int
pthread_attr_getstack(pthread_attr_t const *__restrict attr,
                      void **__restrict stackaddr,
                      size_t *__restrict stacksize) {
 if __unlikely(!attr || !stackaddr || !stacksize) return EINVAL;
 return ENOSYS; /* TODO */
}
__public int
pthread_attr_setstack(pthread_attr_t *__restrict attr,
                      void *stackaddr,
                      size_t stacksize) {
 if __unlikely(!attr) return EINVAL;
 return ENOSYS; /* TODO */
}
__public int
pthread_getattr_default_np(pthread_attr_t *__restrict attr) {
 if __unlikely(!attr) return EINVAL;
 return ENOSYS; /* TODO */
}
__public int
pthread_setattr_default_np(pthread_attr_t const *__restrict attr) {
 if __unlikely(!attr) return EINVAL;
 return ENOSYS; /* TODO */
}
__public int
pthread_getattr_np(pthread_t th, pthread_attr_t *__restrict attr) {
 if __unlikely(!attr) return EINVAL;
 return ENOSYS; /* TODO */
}

__DECL_END

#endif /* !__PTHREAD_C__ */

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
#ifndef __PTHREAD_H__
#ifndef _PTHREAD_H
#define __PTHREAD_H__ 1
#define _PTHREAD_H 1

#include <kos/compiler.h>
#include <kos/types.h>

// The standard wants these two headers #include-ed...
#include <sched.h>
#include <time.h>

__DECL_BEGIN

#ifndef __pthread_t_defined
#define __pthread_t_defined 1
typedef __pthread_t            pthread_t;
typedef __pthread_attr_t       pthread_attr_t;
typedef __pthread_cond_t       pthread_cond_t;
typedef __pthread_condattr_t   pthread_condattr_t;
typedef __pthread_key_t        pthread_key_t;
typedef __pthread_mutex_t      pthread_mutex_t;
typedef __pthread_mutexattr_t  pthread_mutexattr_t;
typedef __pthread_once_t       pthread_once_t;
typedef __pthread_rwlock_t     pthread_rwlock_t;
typedef __pthread_rwlockattr_t pthread_rwlockattr_t;
#endif

#ifndef __pthread_spinlock_t_defined
#define __pthread_spinlock_t_defined 1
typedef __pthread_spinlock_t    pthread_spinlock_t;
typedef __pthread_barrierattr_t pthread_barrierattr_t;
typedef __pthread_barrier_t     pthread_barrier_t;
#endif

struct __pthread_once {
    __atomic int __po_done; /*< 0: pending; -1: running now; 1: done; */
};
#define PTHREAD_ONCE_INIT  {0}

struct __pthread_spinlock {
    __atomic unsigned int __ps_locked; /*< 0: unlocked; 1: locked; */
    __atomic __ktid_t     __ps_holder; /*< Thread currently holding the lock. */
};

struct timespec;

extern int pthread_create(pthread_t *newthread, pthread_attr_t const *attr,
                          void *(*start_routine)(void *), void *arg);
extern __noreturn void pthread_exit(void *retval);
extern int pthread_join(pthread_t th, void **thread_return);
extern int pthread_tryjoin_np(pthread_t th, void **thread_return);
extern int pthread_timedjoin_np(pthread_t th, void **thread_return, struct timespec const *__restrict abstime);
extern int pthread_detach(pthread_t th);
extern __constcall pthread_t pthread_self(void);
extern __constcall int pthread_equal(pthread_t thread1, pthread_t thread2);

//extern int pthread_attr_init(pthread_attr_t *attr);
//extern int pthread_attr_destroy(pthread_attr_t *attr);
//extern int pthread_attr_getdetachstate(pthread_attr_t const *attr, int *detachstate);
//extern int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate);
//extern int pthread_attr_getguardsize(pthread_attr_t const *attr, __size_t *guardsize);
//extern int pthread_attr_setguardsize(pthread_attr_t *attr, __size_t guardsize);
//extern int pthread_attr_getschedparam(pthread_attr_t const *attr, struct sched_param *param);
//extern int pthread_attr_setschedparam(pthread_attr_t *attr, struct sched_param const *param);
//extern int pthread_attr_getschedpolicy(pthread_attr_t const *attr, int *policy);
//extern int pthread_attr_setschedpolicy(pthread_attr_t *attr, int policy);
//extern int pthread_attr_getinheritsched(pthread_attr_t const *attr, int *inherit);
//extern int pthread_attr_setinheritsched(pthread_attr_t *attr, int inherit);
//extern int pthread_attr_getscope(pthread_attr_t const *attr, int *scope);
//extern int pthread_attr_setscope(pthread_attr_t *attr, int scope);
//extern int pthread_attr_getstackaddr(pthread_attr_t const * attr, void **stackaddr);
//extern int pthread_attr_setstackaddr(pthread_attr_t *attr, void *stackaddr);
//extern int pthread_attr_getstacksize(pthread_attr_t const *attr, __size_t *stacksize);
//extern int pthread_attr_setstacksize(pthread_attr_t *attr, __size_t stacksize);
//extern int pthread_attr_getstack(pthread_attr_t const *attr, void **stackaddr, __size_t *stacksize);
//extern int pthread_attr_setstack(pthread_attr_t *attr, void *stackaddr, __size_t stacksize);
//extern int pthread_attr_setaffinity_np(pthread_attr_t *attr, __size_t cpusetsize, cpu_set_t const *cpuset);
//extern int pthread_attr_getaffinity_np(pthread_attr_t const *attr, __size_t cpusetsize, cpu_set_t *cpuset);

//extern int pthread_getattr_default_np(pthread_attr_t *attr);
//extern int pthread_setattr_default_np(pthread_attr_t const *attr);
//extern int pthread_getattr_np(pthread_t th, pthread_attr_t *attr);
extern int pthread_setschedparam(pthread_t target_thread, int policy, struct sched_param const *param);
extern int pthread_getschedparam(pthread_t target_thread, int *policy, struct sched_param *param);
extern int pthread_setschedprio(pthread_t target_thread, int prio);
extern int pthread_getname_np(pthread_t target_thread, char *__restrict buf, __size_t buflen);
extern int pthread_setname_np(pthread_t target_thread, char const *__restrict name);
extern int pthread_getconcurrency(void);
extern int pthread_setconcurrency(int level);
extern int pthread_yield(void);
//extern int pthread_setaffinity_np(pthread_t th, __size_t cpusetsize, cpu_set_t const *cpuset);
//extern int pthread_getaffinity_np(pthread_t th, __size_t cpusetsize, cpu_set_t *cpuset);
//extern int pthread_setcancelstate(int state, int *oldstate);
//extern int pthread_setcanceltype(int type, int *oldtype);
//extern int pthread_cancel(pthread_t th);
//extern void pthread_testcancel(void);

extern int pthread_once(pthread_once_t *once_control, void (*init_routine)(void));

//struct pthread_cleanup_frame { int __placeholder; };
//#define pthread_cleanup_push(routine,arg) do{
//extern void pthread_register_cancel(pthread_unwind_buf_t *buf);
//#define pthread_cleanup_pop(execute)    do{}while(0);}while(0)
//extern void pthread_unregister_cancel(pthread_unwind_buf_t *buf);
//#define pthread_cleanup_push_defer_np(routine,arg) do{
//extern void pthread_register_cancel_defer(pthread_unwind_buf_t *buf);
//#define pthread_cleanup_pop_restore_np(execute) do{}while(0);}while(0)
//extern void pthread_unregister_cancel_restore(pthread_unwind_buf_t *buf);
//extern __noreturn void pthread_unwind_next(pthread_unwind_buf_t *buf);

//extern int pthread_mutex_init(pthread_mutex_t *mutex, pthread_mutexattr_t const *mutexattr);
//extern int pthread_mutex_destroy(pthread_mutex_t *mutex);
//extern int pthread_mutex_trylock(pthread_mutex_t *mutex);
//extern int pthread_mutex_lock(pthread_mutex_t *mutex);
//extern int pthread_mutex_timedlock(pthread_mutex_t *mutex, struct timespec const *__restrict abstime);
//extern int pthread_mutex_unlock(pthread_mutex_t *mutex);
//extern int pthread_mutex_getprioceiling(pthread_mutex_t const *mutex, int *prioceiling);
//extern int pthread_mutex_setprioceiling(pthread_mutex_t *mutex, int prioceiling, int *old_ceiling);
//extern int pthread_mutex_consistent(pthread_mutex_t *mutex);
//extern int pthread_mutex_consistent_np(pthread_mutex_t *mutex);
//extern int pthread_mutexattr_init(pthread_mutexattr_t *attr);
//extern int pthread_mutexattr_destroy(pthread_mutexattr_t *attr);
//extern int pthread_mutexattr_getpshared(pthread_mutexattr_t const *attr, int *pshared);
//extern int pthread_mutexattr_setpshared(pthread_mutexattr_t *attr, int pshared);
//extern int pthread_mutexattr_gettype(pthread_mutexattr_t const *attr, int *kind);
//extern int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int kind);
//extern int pthread_mutexattr_getprotocol(pthread_mutexattr_t const *attr, int *protocol);
//extern int pthread_mutexattr_setprotocol(pthread_mutexattr_t *attr, int protocol);
//extern int pthread_mutexattr_getprioceiling(pthread_mutexattr_t const *attr, int *prioceiling);
//extern int pthread_mutexattr_setprioceiling(pthread_mutexattr_t *attr, int prioceiling);
//extern int pthread_mutexattr_getrobust(pthread_mutexattr_t const *attr, int *robustness);
//extern int pthread_mutexattr_getrobust_np(pthread_mutexattr_t const *attr, int *robustness);
//extern int pthread_mutexattr_setrobust(pthread_mutexattr_t *attr, int robustness);
//extern int pthread_mutexattr_setrobust_np(pthread_mutexattr_t *attr, int robustness);

//extern int pthread_rwlock_init(pthread_rwlock_t *rwlock, pthread_rwlockattr_t const *attr);
//extern int pthread_rwlock_destroy(pthread_rwlock_t *rwlock);
//extern int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock);
//extern int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock);
//extern int pthread_rwlock_timedrdlock(pthread_rwlock_t *rwlock, struct timespec const *__restrict abstime);
//extern int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock);
//extern int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock);
//extern int pthread_rwlock_timedwrlock(pthread_rwlock_t *rwlock, struct timespec const *__restrict abstime);
//extern int pthread_rwlock_unlock(pthread_rwlock_t *rwlock);
//extern int pthread_rwlockattr_init(pthread_rwlockattr_t *attr);
//extern int pthread_rwlockattr_destroy(pthread_rwlockattr_t *attr);
//extern int pthread_rwlockattr_getpshared(pthread_rwlockattr_t const *attr, int *pshared);
//extern int pthread_rwlockattr_setpshared(pthread_rwlockattr_t *attr, int pshared);
//extern int pthread_rwlockattr_getkind_np(pthread_rwlockattr_t const *attr, int *pref);
//extern int pthread_rwlockattr_setkind_np(pthread_rwlockattr_t *attr, int pref);

//extern int pthread_cond_init(pthread_cond_t *cond, pthread_condattr_t const *cond_attr);
//extern int pthread_cond_destroy(pthread_cond_t *cond);
//extern int pthread_cond_signal(pthread_cond_t *cond);
//extern int pthread_cond_broadcast(pthread_cond_t *cond);
//extern int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
//extern int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, struct timespec const *__restrict abstime);
//extern int pthread_condattr_init(pthread_condattr_t *attr);
//extern int pthread_condattr_destroy(pthread_condattr_t *attr);
//extern int pthread_condattr_getpshared(pthread_condattr_t const *attr, int *pshared);
//extern int pthread_condattr_setpshared(pthread_condattr_t *attr, int pshared);
//extern int pthread_condattr_getclock(pthread_condattr_t const *attr, clockid_t *clock_id);
//extern int pthread_condattr_setclock(pthread_condattr_t *attr, clockid_t clock_id);

extern int pthread_spin_init(pthread_spinlock_t *lock, int pshared);
extern int pthread_spin_destroy(pthread_spinlock_t *lock);
extern int pthread_spin_lock(pthread_spinlock_t *lock);
extern int pthread_spin_trylock(pthread_spinlock_t *lock);
extern int pthread_spin_unlock(pthread_spinlock_t *lock);

//extern int pthread_barrier_init(pthread_barrier_t *barrier, pthread_barrierattr_t const *attr, unsigned int count);
//extern int pthread_barrier_destroy(pthread_barrier_t *barrier);
//extern int pthread_barrier_wait(pthread_barrier_t *barrier);
//extern int pthread_barrierattr_init(pthread_barrierattr_t *attr);
//extern int pthread_barrierattr_destroy(pthread_barrierattr_t *attr);
//extern int pthread_barrierattr_getpshared(pthread_barrierattr_t const *attr, int *pshared);
//extern int pthread_barrierattr_setpshared(pthread_barrierattr_t *attr, int pshared);

//extern int pthread_getcpuclockid(pthread_t thread_id, clockid_t *clock_id);
//extern int pthread_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void));


typedef __ktid_t pthread_id_np_t;
// I have no idea why IBM decided to make 'thread' be a pointer...
// >> Like seriously! That thing shouldn't be modified by this.
// Ugh... Lets just look past this and realize that this is
// nothing but a way too complicated wrapper around 'ktask_gettid()'
// Oh and yeah: the pthread_id_np_t is just the tid of the thread.
extern int pthread_getunique_np(pthread_t *thread, pthread_id_np_t *id);
extern pthread_id_np_t pthread_getthreadid_np(void);



// WARNING: The implementation currently ignored 'destr_function'
// WARNING: Any future implementation that will actually call 'destr_function',
//          will be subject to the fact that task_terminate-ing a thread will
//          not give that thread a chance to ever execute these destructors.
extern int pthread_key_create(pthread_key_t *key, void (*destr_function)(void *));
extern int pthread_key_delete(pthread_key_t key);
extern void *pthread_getspecific(pthread_key_t key);
extern int pthread_setspecific(pthread_key_t key, void const *pointer);



__DECL_END

#endif /* !_PTHREAD_H */
#endif /* !__PTHREAD_H__ */

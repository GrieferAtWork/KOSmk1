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

struct __pthread_once { __atomic int __po_done; };
struct __pthread_spinlock {
 __atomic unsigned int __ps_locked;
 __atomic __uintptr_t  __ps_holder;
};

struct __key { int __placeholder; };
struct __pthread_attr { int __placeholder; };
struct __pthread_cond { int __placeholder; };
struct __pthread_condattr { int __placeholder; };
struct __pthread_barrierattr { int __placeholder; };
struct __pthread_barrier { int __placeholder; };


struct __pthread_mutex {
 unsigned int volatile __m_ftx;
 unsigned int          __m_kind;
 __uintptr_t  volatile __m_owner;
};
struct __pthread_mutexattr { unsigned int __m_kind; };

struct __pthread_rwlock { int __placeholder; };
struct __pthread_rwlockattr { int __rw_kind; };


/* Detach state. */
#define PTHREAD_CREATE_JOINABLE	0
#define PTHREAD_CREATE_DETACHED	1

/* Mutex types. */
#define PTHREAD_MUTEX_TIMED_NP      0
#define PTHREAD_MUTEX_RECURSIVE_NP  1
#define PTHREAD_MUTEX_ERRORCHECK_NP 2
#define PTHREAD_MUTEX_ADAPTIVE_NP   PTHREAD_MUTEX_TIMED_NP /*3*/
#define PTHREAD_MUTEX_NORMAL        PTHREAD_MUTEX_TIMED_NP
#define PTHREAD_MUTEX_RECURSIVE     PTHREAD_MUTEX_RECURSIVE_NP
#define PTHREAD_MUTEX_ERRORCHECK    PTHREAD_MUTEX_ERRORCHECK_NP
#define PTHREAD_MUTEX_DEFAULT       PTHREAD_MUTEX_NORMAL
#define PTHREAD_MUTEX_FAST_NP       PTHREAD_MUTEX_TIMED_NP

/* Robust mutex or not flags. */
#define PTHREAD_MUTEX_STALLED_NP    0
#define PTHREAD_MUTEX_STALLED       PTHREAD_MUTEX_STALLED_NP
#define PTHREAD_MUTEX_ROBUST_NP     1
#define PTHREAD_MUTEX_ROBUST        PTHREAD_MUTEX_ROBUST_NP

/* Mutex protocols. */
#define PTHREAD_PRIO_NONE    0
#define PTHREAD_PRIO_INHERIT 1
#define PTHREAD_PRIO_PROTECT 2

#define PTHREAD_MUTEX_INITIALIZER               {0,PTHREAD_MUTEX_TIMED_NP,0}
#define PTHREAD_RECURSIVE_MUTEX_INITIALIZER     {0,PTHREAD_MUTEX_RECURSIVE_NP,0}
#define PTHREAD_ERRORCHECK_MUTEX_INITIALIZER    {0,PTHREAD_MUTEX_ERRORCHECK_NP|PTHREAD_MUTEX_TIMED_NP,0}
#define PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP  {0,PTHREAD_MUTEX_RECURSIVE_NP,0}
#define PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP {0,PTHREAD_MUTEX_ERRORCHECK_NP|PTHREAD_MUTEX_TIMED_NP,0}

/* Read-write lock types. */
#define PTHREAD_RWLOCK_PREFER_READER_NP              0
#define PTHREAD_RWLOCK_PREFER_WRITER_NP              1
#define PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP 2
#define PTHREAD_RWLOCK_DEFAULT_NP                    PTHREAD_RWLOCK_PREFER_READER_NP

/* Read-write lock initializers. */
#define PTHREAD_RWLOCK_INITIALIZER                        {/*TODO*/}
#define PTHREAD_RWLOCK_WRITER_NONRECURSIVE_INITIALIZER_NP {/*TODO*/}

/* Scheduler inheritance. */
#define PTHREAD_INHERIT_SCHED   0
#define PTHREAD_EXPLICIT_SCHED  1

/* Scope handling. */
#define PTHREAD_SCOPE_SYSTEM    0
#define PTHREAD_SCOPE_PROCESS   1

/* Process shared or private flag. */
#define PTHREAD_PROCESS_PRIVATE 0
#define PTHREAD_PROCESS_SHARED  1

/* Conditional variable handling. */
#define PTHREAD_COND_INITIALIZER {/*TODO*/}

/* Single execution handling. */
#define PTHREAD_ONCE_INIT  {0}

struct sched_param;

extern __nonnull((1,3)) int pthread_create __P((pthread_t *__restrict __newthread, pthread_attr_t const *__attr, void *(*__start_routine) (void *), void *__restrict __arg));
extern __noreturn void pthread_exit __P((void *__retval));
extern int pthread_join __P((pthread_t __th, void **__thread_return));
extern int pthread_tryjoin_np __P((pthread_t __th, void **__thread_return));
extern int pthread_timedjoin_np __P((pthread_t __th, void **__thread_return, struct timespec const *__restrict __abstime));
extern int pthread_detach __P((pthread_t __th));
extern __constcall pthread_t pthread_self __P((void));
extern __constcall int pthread_equal __P((pthread_t __thread1, pthread_t __thread2));
extern __nonnull((1)) int pthread_attr_init __P((pthread_attr_t *__restrict __attr));
extern __nonnull((1)) int pthread_attr_destroy __P((pthread_attr_t *__restrict __attr));
extern __nonnull((1,2)) int pthread_attr_getdetachstate __P((pthread_attr_t const *__restrict __attr, int *__restrict __detachstate));
extern __nonnull((1)) int pthread_attr_setdetachstate __P((pthread_attr_t *__restrict __attr, int __detachstate));
extern __nonnull((1,2)) int pthread_attr_getguardsize __P((pthread_attr_t const *__restrict __attr, __size_t *__restrict __guardsize));
extern __nonnull((1)) int pthread_attr_setguardsize __P((pthread_attr_t *__restrict __attr, __size_t __guardsize));
extern __nonnull((1,2)) int pthread_attr_getschedparam __P((pthread_attr_t const *__restrict __attr, struct sched_param *__restrict __param));
extern __nonnull((1,2)) int pthread_attr_setschedparam __P((pthread_attr_t *__restrict __attr, struct sched_param const *__restrict __param));
extern __nonnull((1,2)) int pthread_attr_getschedpolicy __P((pthread_attr_t const *__restrict __attr, int *__restrict __policy));
extern __nonnull((1)) int pthread_attr_setschedpolicy __P((pthread_attr_t *__restrict __attr, int __policy));
extern __nonnull((1,2)) int pthread_attr_getinheritsched __P((pthread_attr_t const *__restrict __attr, int *__restrict __inherit));
extern __nonnull((1)) int pthread_attr_setinheritsched __P((pthread_attr_t *__restrict __attr, int __inherit));
extern __nonnull((1,2)) int pthread_attr_getscope __P((pthread_attr_t const *__restrict __attr, int *__restrict __scope));
extern __nonnull((1)) int pthread_attr_setscope __P((pthread_attr_t *__restrict __attr, int __scope));
extern __nonnull((1,2)) int pthread_attr_getstackaddr __P((pthread_attr_t const *__restrict __attr, void **__restrict __stackaddr));
extern __nonnull((1)) int pthread_attr_setstackaddr __P((pthread_attr_t *__restrict __attr, void *__stackaddr));
extern __nonnull((1,2)) int pthread_attr_getstacksize __P((pthread_attr_t const *__restrict __attr, __size_t *__restrict __stacksize));
extern __nonnull((1)) int pthread_attr_setstacksize __P((pthread_attr_t *__restrict __attr, __size_t __stacksize));
extern __nonnull((1,2,3)) int pthread_attr_getstack __P((pthread_attr_t const *__restrict __attr, void **__restrict __stackaddr, __size_t *__restrict __stacksize));
extern __nonnull((1)) int pthread_attr_setstack __P((pthread_attr_t *__restrict __attr, void *__stackaddr, __size_t __stacksize));
extern __nonnull((1)) int pthread_getattr_default_np __P((pthread_attr_t *__restrict __attr));
extern __nonnull((1)) int pthread_setattr_default_np __P((pthread_attr_t const *__restrict __attr));
extern __nonnull((2)) int pthread_getattr_np __P((pthread_t __th, pthread_attr_t *__restrict __attr));
extern __nonnull((2,3)) int pthread_getschedparam __P((pthread_t __target_thread, int *__restrict __policy, struct sched_param *__restrict __param));
extern __nonnull((3)) int pthread_setschedparam __P((pthread_t __target_thread, int __policy, struct sched_param const *__restrict __param));
extern int pthread_setschedprio __P((pthread_t __target_thread, int __prio));
extern __nonnull((2)) int pthread_getname_np __P((pthread_t __target_thread, char *__restrict __buf, __size_t __buflen));
extern __nonnull((2)) int pthread_setname_np __P((pthread_t __target_thread, const char *__restrict __name));
extern int pthread_getconcurrency __P((void));
extern int pthread_setconcurrency __P((int __level));
extern int pthread_yield __P((void));

extern __nonnull((1,2)) int pthread_once __P((pthread_once_t *__restrict __once_control, void (*__init_routine)(void)));

extern int pthread_cancel __P((pthread_t __th)); /* Terminate. */

/* Mutex handling. */
extern __nonnull((1)) int pthread_mutex_init __P((pthread_mutex_t *__restrict __mutex, pthread_mutexattr_t const *__mutexattr));
extern __nonnull((1)) int pthread_mutex_destroy __P((pthread_mutex_t *__restrict __mutex));
extern __nonnull((1)) int pthread_mutex_trylock __P((pthread_mutex_t *__restrict __mutex));
extern __nonnull((1)) int pthread_mutex_lock __P((pthread_mutex_t *__restrict __mutex));
extern __nonnull((1,2)) int pthread_mutex_timedlock __P((pthread_mutex_t *__restrict __mutex, struct timespec const *__restrict __abstime));
extern __nonnull((1)) int pthread_mutex_unlock __P((pthread_mutex_t *__restrict __mutex));

/* Functions for handling mutex attributes. */
extern __nonnull((1)) int pthread_mutexattr_init __P((pthread_mutexattr_t *__restrict __attr));
extern __nonnull((1)) int pthread_mutexattr_destroy __P((pthread_mutexattr_t *__restrict __attr));
extern __nonnull((1,2)) int pthread_mutexattr_getpshared __P((pthread_mutexattr_t const *__restrict __attr, int *__restrict __pshared));
extern __nonnull((1)) int pthread_mutexattr_setpshared __P((pthread_mutexattr_t *__restrict __attr, int __pshared));
extern __nonnull((1,2)) int pthread_mutexattr_gettype __P((pthread_mutexattr_t const *__restrict __attr, int *__restrict __kind));
extern __nonnull((1)) int pthread_mutexattr_settype __P((pthread_mutexattr_t *__restrict __attr, int __kind));
extern __nonnull((1,2)) int pthread_mutexattr_getprotocol __P((pthread_mutexattr_t const *__restrict __attr, int *__restrict __protocol));
extern __nonnull((1)) int pthread_mutexattr_setprotocol __P((pthread_mutexattr_t *__restrict __attr, int __protocol));

/* Functions for handling read-write locks. */
extern __nonnull((1)) int pthread_rwlock_init __P((pthread_rwlock_t *__restrict __rwlock, pthread_rwlockattr_t const *__attr));
extern __nonnull((1)) int pthread_rwlock_destroy __P((pthread_rwlock_t *__restrict __rwlock));
extern __nonnull((1)) int pthread_rwlock_rdlock __P((pthread_rwlock_t *__restrict __rwlock));
extern __nonnull((1)) int pthread_rwlock_tryrdlock __P((pthread_rwlock_t *__restrict __rwlock));
extern __nonnull((1,2)) int pthread_rwlock_timedrdlock __P((pthread_rwlock_t *__restrict __rwlock, struct timespec const *__restrict __abstime));
extern __nonnull((1)) int pthread_rwlock_wrlock __P((pthread_rwlock_t *__restrict __rwlock));
extern __nonnull((1)) int pthread_rwlock_trywrlock __P((pthread_rwlock_t *__restrict __rwlock));
extern __nonnull((1,2)) int pthread_rwlock_timedwrlock __P((pthread_rwlock_t *__restrict __rwlock, struct timespec const *__restrict __abstime));
extern __nonnull((1)) int pthread_rwlock_unlock __P((pthread_rwlock_t *__restrict __rwlock));

/* Functions for handling read-write lock attributes. */
extern __nonnull((1)) int pthread_rwlockattr_init __P((pthread_rwlockattr_t *__restrict __attr));
extern __nonnull((1)) int pthread_rwlockattr_destroy __P((pthread_rwlockattr_t *__restrict __attr));
extern __nonnull((1,2)) int pthread_rwlockattr_getpshared __P((pthread_rwlockattr_t const *__restrict __attr, int *__restrict __pshared));
extern __nonnull((1)) int pthread_rwlockattr_setpshared __P((pthread_rwlockattr_t *__restrict __attr, int __pshared));
extern __nonnull((1,2)) int pthread_rwlockattr_getkind_np __P((pthread_rwlockattr_t const *__restrict __attr, int *__restrict __pref));
extern __nonnull((1)) int pthread_rwlockattr_setkind_np __P((pthread_rwlockattr_t *__restrict __attr, int __pref));

/* Functions for handling conditional variables. */
extern __nonnull((1)) int pthread_cond_init __P((pthread_cond_t *__restrict __cond, pthread_condattr_t const *__restrict __cond_attr));
extern __nonnull((1)) int pthread_cond_destroy __P((pthread_cond_t *__restrict __cond));
extern __nonnull((1)) int pthread_cond_signal __P((pthread_cond_t *__restrict __cond));
extern __nonnull((1)) int pthread_cond_broadcast __P((pthread_cond_t *__restrict __cond));
extern __nonnull((1,2)) int pthread_cond_wait __P((pthread_cond_t *__restrict __cond, pthread_mutex_t *__restrict __mutex));
extern __nonnull((1,2,3)) int pthread_cond_timedwait __P((pthread_cond_t *__restrict __cond, pthread_mutex_t *__restrict __mutex, struct timespec const *__restrict __abstime));

/* Functions for handling condition variable attributes. */
extern __nonnull((1)) int pthread_condattr_init __P((pthread_condattr_t *__restrict __attr));
extern __nonnull((1)) int pthread_condattr_destroy __P((pthread_condattr_t *__restrict __attr));
extern __nonnull((1,2)) int pthread_condattr_getpshared __P((pthread_condattr_t const * __restrict __attr, int *__restrict __pshared));
extern __nonnull((1)) int pthread_condattr_setpshared __P((pthread_condattr_t *__restrict __attr, int __pshared));

/* Functions to handle spinlocks. */
extern __nonnull((1)) int pthread_spin_init __P((pthread_spinlock_t *__restrict __lock, int __pshared));
extern __nonnull((1)) int pthread_spin_destroy __P((pthread_spinlock_t *__restrict __lock));
extern __nonnull((1)) int pthread_spin_lock __P((pthread_spinlock_t *__restrict __lock));
extern __nonnull((1)) int pthread_spin_trylock __P((pthread_spinlock_t *__restrict __lock));
extern __nonnull((1)) int pthread_spin_unlock __P((pthread_spinlock_t *__restrict __lock));

/* Functions to handle barriers. */
extern __nonnull((1)) int pthread_barrier_init __P((pthread_barrier_t *__restrict __barrier, pthread_barrierattr_t const *__restrict __attr, unsigned int __count));
extern __nonnull((1)) int pthread_barrier_destroy __P((pthread_barrier_t *__restrict __barrier));
extern __nonnull((1)) int pthread_barrier_wait __P((pthread_barrier_t *__restrict __barrier));
extern __nonnull((1)) int pthread_barrierattr_init __P((pthread_barrierattr_t *__restrict __attr));
extern __nonnull((1)) int pthread_barrierattr_destroy __P((pthread_barrierattr_t *__restrict __attr));
extern __nonnull((1,2)) int pthread_barrierattr_getpshared __P((pthread_barrierattr_t const *__restrict __attr, int *__restrict __pshared));
extern __nonnull((1)) int pthread_barrierattr_setpshared __P((pthread_barrierattr_t *__restrict __attr, int __pshared));

/* Functions for handling thread-specific data. */
extern __nonnull((1)) int pthread_key_create __P((pthread_key_t *__restrict __key, void (*__destr_function) (void *)));
extern int pthread_key_delete __P((pthread_key_t __key));
extern void *pthread_getspecific __P((pthread_key_t __key));
extern int pthread_setspecific __P((pthread_key_t __key, void const *__pointer));




typedef __ktid_t pthread_id_np_t;
// I have no idea why IBM decided to make 'thread' be a pointer...
// >> Like seriously! That thing shouldn't be modified by this.
// Ugh... Lets just look past this and realize that this is
// nothing but a way too complicated wrapper around 'ktask_gettid()'
// Oh and yeah: the pthread_id_np_t is just the tid of the thread.
extern int pthread_getunique_np(pthread_t *__restrict thread, pthread_id_np_t *__restrict id);
extern pthread_id_np_t pthread_getthreadid_np(void);

__DECL_END

#endif /* !_PTHREAD_H */
#endif /* !__PTHREAD_H__ */

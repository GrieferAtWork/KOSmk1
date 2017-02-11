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
#include <stdio.h>

__DECL_BEGIN

#define gettid()  ktask_gettid(ktask_self())

struct pthread_maindata {
 void *(*start_routine)(void *);
 void   *arg;
};

static __noreturn void pthread_main(struct pthread_maindata *data) {
 ktask_exit((*data->start_routine)(data->arg));
}


int pthread_create(pthread_t *newthread, pthread_attr_t const *attr,
                   void *(*start_routine)(void *), void *arg) {
 struct pthread_maindata closure;
 ktask_t result;
 (void)attr; // TODO
 closure.arg           = arg;
 closure.start_routine = start_routine;
 result = ktask_newthreadi((ktask_threadfunc)&pthread_main,&closure,
                           sizeof(closure),KTASK_NEW_FLAG_NONE,NULL);
 if __unlikely(KE_ISERR(result)) return -result;
 *newthread = result;
 return 0;
}


__noreturn void pthread_exit(void *retval) {
 ktask_exit(retval);
}

int pthread_join(pthread_t th, void **thread_return) {
 kerrno_t error = ktask_join(th,thread_return,0);
 if __unlikely(KE_ISERR(error)) return -error;
 kfd_close(th);
 return 0;
}
int pthread_tryjoin_np(pthread_t th, void **thread_return) {
 kerrno_t error = ktask_tryjoin(th,thread_return,0);
 if __unlikely(KE_ISERR(error)) return -error;
 kfd_close(th);
 return 0;
}
int pthread_timedjoin_np(pthread_t th, void **thread_return, struct timespec const *abstime) {
 kerrno_t error = ktask_timedjoin(th,abstime,thread_return,0);
 if __unlikely(KE_ISERR(error)) return -error;
 kfd_close(th);
 return 0;
}
int pthread_detach(pthread_t th) {
 return -kfd_close(th);
}
pthread_t pthread_self(void) {
 return ktask_self();
}
int pthread_equal(pthread_t thread1, pthread_t thread2) {
 return thread1 == thread2 || kfd_equals(thread1,thread2);
}

int pthread_setschedprio(pthread_t target_thread, int prio) {
 return -ktask_setpriority(target_thread,prio);
}
int pthread_getname_np(pthread_t target_thread, char *__restrict buf, size_t buflen) {
 size_t reqsize; kerrno_t error;
 error = ktask_getname(target_thread,buf,buflen,&reqsize);
 if __unlikely(KE_ISERR(error)) return -error;
 if __unlikely(reqsize > buflen) return ERANGE;
 return 0;
}
int pthread_setname_np(pthread_t target_thread, char const *__restrict name) {
 int error = -ktask_setname(target_thread,name);
 return KE_ISOK(error) ? 0 : -error;
}


int pthread_yield(void) {
 ktask_yield();
 return 0;
}

int pthread_getunique_np(pthread_t *thread, pthread_id_np_t *id) {
 return ((*id = ktask_gettid(*thread)) == (size_t)-1) ? EBADF : 0;
}

pthread_id_np_t pthread_getthreadid_np(void) {
 return gettid();
}

int pthread_setschedparam(pthread_t target_thread, int __unused(policy),
                          struct sched_param const *param) {
 int error;
 error = ktask_setpriority(target_thread,(ktaskprio_t)param->sched_priority);
 return KE_ISOK(error) ? 0 : -error;
}
int pthread_getschedparam(pthread_t target_thread, int *policy,
                          struct sched_param *param) {
 ktaskprio_t prio; int error;
 *policy = SCHED_RR;
 error = ktask_getpriority(target_thread,&prio);
 param->sched_priority = (int)prio;
 return KE_ISOK(error) ? 0 : -error;
}



int pthread_key_create(pthread_key_t *key, void (*destr_function)(void *)) {
 if (destr_function != NULL) return -ENOSYS; // TODO
 return -kproc_alloctls(key);
}
int pthread_key_delete(pthread_key_t key) {
 return -kproc_freetls(key);
}
void *pthread_getspecific(pthread_key_t key) {
 return ktask_gettls(key);
}
int pthread_setspecific(pthread_key_t key, void const *pointer) {
 return ktask_settls(key,(void *)pointer,NULL);
}




static __atomic int clevel = 0;
int pthread_getconcurrency(void) {
 return katomic_load(clevel);
}
int pthread_setconcurrency(int level) {
 if __unlikely(level < 0) return EINVAL;
 katomic_store(clevel,level);
 return 0;
}



int pthread_once(pthread_once_t *once_control,
                 void (*init_routine)(void)) {
 int oldval;
 if (!once_control || !init_routine) return EINVAL;
 oldval = katomic_cmpxch_val(once_control->__po_done,0,-1);
 if (!oldval) {
  (*init_routine)();
  katomic_store(once_control->__po_done,1);
 } else while (oldval < 0) {
  pthread_yield();
  oldval = katomic_load(once_control->__po_done);
 }
 return 0;
}






__public int pthread_spin_init(pthread_spinlock_t *lock,
                               int pshared) {
 if __unlikely(!lock) return EINVAL;
 lock->__ps_holder = KTID_INVALID;
 lock->__ps_locked = 0;
 return 0;
}
int pthread_spin_destroy(pthread_spinlock_t *lock) {
 if __unlikely(!lock) return EINVAL;
 if __unlikely(lock->__ps_locked) return EBUSY;
 return 0;
}
int pthread_spin_lock(pthread_spinlock_t *lock) {
 __ktid_t tid = gettid();
 if __unlikely(!lock) return EINVAL;
 if (!katomic_cmpxch(lock->__ps_locked,0,1)) {
  if __unlikely(lock->__ps_holder == tid) return EDEADLK;
  for (;;) {
   int spin_counter = 10000;
   // Spin a bit...
   while (spin_counter--) {
    if (katomic_cmpxch(lock->__ps_locked,0,1)) goto end;
   }
  }
 }
end:
 lock->__ps_holder = tid;
 return 0;
}
int pthread_spin_trylock(pthread_spinlock_t *lock) {
 if __unlikely(!lock) return EINVAL;
 if (!katomic_cmpxch(lock->__ps_locked,0,1)) return EBUSY;
 lock->__ps_holder = gettid();
 return 0;
}
int pthread_spin_unlock(pthread_spinlock_t *lock) {
 if __unlikely(!lock) return EINVAL;
 if __unlikely(lock->__ps_holder != gettid()) return EPERM;
 assert(lock->__ps_locked);
 lock->__ps_holder = KTID_INVALID;
 lock->__ps_locked = 0; // No need to use atomics here...
 return 0;
}



__DECL_END

#endif /* !__PTHREAD_C__ */

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
#ifndef __PTHREAD_MUTEX_C__
#define __PTHREAD_MUTEX_C__ 1

#include <errno.h>
#include <kos/atomic.h>
#include <kos/futex.h>
#include <proc.h>
#include <pthread.h>
#include <stdint.h>

__DECL_BEGIN


__public int
pthread_mutex_init(pthread_mutex_t *__restrict mutex,
                   pthread_mutexattr_t const *mutexattr) {
 if __unlikely(!mutex) return EINVAL;
 mutex->__m_ftx = 0;
 mutex->__m_kind = mutexattr ? mutexattr->__m_kind : PTHREAD_MUTEX_TIMED_NP;
 mutex->__m_owner = 0;
 return 0;
}
__public int
pthread_mutex_destroy(pthread_mutex_t *__restrict mutex) {
 return __likely(mutex) ? 0 : EINVAL;
}


__local int
pthread_mutex_dotrylock(pthread_mutex_t *__restrict mutex) {
 uintptr_t unique;
 switch (mutex->__m_kind) {
  case PTHREAD_MUTEX_RECURSIVE_NP:
   unique = thread_unique();
   /* Check twice to prevent false readings. */
   if (mutex->__m_owner == unique &&
       mutex->__m_owner == unique) {
    katomic_fetchinc(*(unsigned int *)&mutex->__m_ftx);
    return 0;
   }
   if (katomic_cmpxch(*(unsigned int *)&mutex->__m_ftx,0,1)) {
    mutex->__m_owner = unique;
    return 0;
   }
   break;
  default:
   if (katomic_cmpxch(*(unsigned int *)&mutex->__m_ftx,0,1)) return 0;
   break;
 }
 return EWOULDBLOCK;
}

__local int
mutex_do_timedlock(pthread_mutex_t *__restrict mutex,
                   struct timespec const *__restrict abstime) {
 kerrno_t error;
 for (;;) {
  if (!pthread_mutex_dotrylock(mutex)) return 0;
  /* atomic: if (mutex->__m_ftx != 0) { recv(mutex->__m_ftx); } */
  error = kfutex_cmd(&mutex->__m_ftx,KFUTEX_RECVIF(KFUTEX_NOT_EQUAL),0,NULL,abstime);
  if __unlikely(KE_ISERR(error) && error != KE_AGAIN) break;
 }
 return -error;
}
__public int
pthread_mutex_unlock(pthread_mutex_t *__restrict mutex) {
 kerrno_t error;
 if __unlikely(!mutex) return EINVAL;
 switch (mutex->__m_kind) {
  {
   uintptr_t unique;
   unsigned int oldval;
  case PTHREAD_MUTEX_ERRORCHECK_NP|
       PTHREAD_MUTEX_RECURSIVE_NP:
   unique = thread_unique();
   if __unlikely(mutex->__m_owner != unique) return EPERM;
   do {
    oldval = katomic_load(*(unsigned int *)&mutex->__m_ftx);
    if __unlikely(!oldval) return EPERM;
   } while (!katomic_cmpxch(*(unsigned int *)&mutex->__m_ftx,oldval,oldval-1));
   if (oldval != 1) return 0;
   break;
  case PTHREAD_MUTEX_RECURSIVE_NP:
   unique = thread_unique();
   assertf(mutex->__m_owner == unique,"You're not the owner of this mutex");
   oldval = katomic_fetchdec(*(unsigned int *)&mutex->__m_ftx);
   assertf(oldval != 0,"Mutex was not locked");
   if (oldval != 1) return 0;
   break;
  }
  case PTHREAD_MUTEX_ERRORCHECK_NP|
       PTHREAD_MUTEX_TIMED_NP:
   if __unlikely(!katomic_cmpxch(*(unsigned int *)&mutex->__m_ftx,1,0)) return EPERM;
   break;
  default:
   assertef(katomic_cmpxch(*(unsigned int *)&mutex->__m_ftx,1,0),"Mutex was not locked");
   break;
 }
 error = kfutex_sendone(&mutex->__m_ftx);
 return KE_ISOK(error) ? 0 : -error;
}


__public int
pthread_mutex_trylock(pthread_mutex_t *__restrict mutex) {
 return __likely(mutex) ? pthread_mutex_dotrylock(mutex) : EINVAL;
}
__public int
pthread_mutex_lock(pthread_mutex_t *__restrict mutex) {
 return __likely(mutex) ? mutex_do_timedlock(mutex,NULL) : EINVAL;
}
__public int
pthread_mutex_timedlock(pthread_mutex_t *__restrict mutex,
                        struct timespec const *__restrict abstime) {
 return __likely(mutex) ? mutex_do_timedlock(mutex,abstime) : EINVAL;
}


__public int
pthread_mutexattr_init(pthread_mutexattr_t *__restrict attr) {
 if __unlikely(!attr) return EINVAL;
 attr->__m_kind = PTHREAD_MUTEX_TIMED_NP;
 return 0;
}
__public int pthread_mutexattr_destroy(pthread_mutexattr_t *__restrict attr) { return attr ? 0 : EINVAL; }
__public int pthread_mutexattr_getpshared(pthread_mutexattr_t const *__restrict attr, int *__restrict pshared) { if (!attr || !pshared) return EINVAL; *pshared = 1; return 0; }
__public int pthread_mutexattr_setpshared(pthread_mutexattr_t *__restrict attr, int pshared) { return attr ? (pshared ? 0 : ENOSYS) : EINVAL; }
__public int pthread_mutexattr_gettype(pthread_mutexattr_t const *__restrict attr, int *__restrict kind) { if (!attr || !kind) return EINVAL; *kind = attr->__m_kind; return 0; }
__public int pthread_mutexattr_settype(pthread_mutexattr_t *__restrict attr, int kind) { if (!attr) return EINVAL; attr->__m_kind = kind; return 0; }
__public int pthread_mutexattr_getprotocol(pthread_mutexattr_t const *__restrict attr, int *__restrict protocol) { if (!attr || !protocol) return EINVAL; *protocol = PTHREAD_PRIO_NONE; return 0; }
__public int pthread_mutexattr_setprotocol(pthread_mutexattr_t *__restrict attr, int protocol) { return attr ? (protocol == PTHREAD_PRIO_NONE ? 0 : ENOSYS) : EINVAL; }

__DECL_END

#endif /* !__PTHREAD_MUTEX_C__ */

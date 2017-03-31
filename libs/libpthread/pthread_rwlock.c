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
#ifndef __PTHREAD_RWLOCK_C__
#define __PTHREAD_RWLOCK_C__ 1

#include <errno.h>
#include <pthread.h>

__DECL_BEGIN

__public int
pthread_rwlock_init(pthread_rwlock_t *__restrict rwlock,
                    pthread_rwlockattr_t const *attr) {
 if __unlikely(!rwlock) return EINVAL;
 return ENOSYS; /* TODO */
}
__public int
pthread_rwlock_destroy(pthread_rwlock_t *__restrict rwlock) {
 if __unlikely(!rwlock) return EINVAL;
 return ENOSYS; /* TODO */
}
__public int
pthread_rwlock_rdlock(pthread_rwlock_t *__restrict rwlock) {
 if __unlikely(!rwlock) return EINVAL;
 return ENOSYS; /* TODO */
}
__public int
pthread_rwlock_tryrdlock(pthread_rwlock_t *__restrict rwlock) {
 if __unlikely(!rwlock) return EINVAL;
 return ENOSYS; /* TODO */
}
__public int
pthread_rwlock_timedrdlock(pthread_rwlock_t *__restrict rwlock,
                           struct timespec const *__restrict abstime) {
 if __unlikely(!rwlock || !abstime) return EINVAL;
 return ENOSYS; /* TODO */
}
__public int
pthread_rwlock_wrlock(pthread_rwlock_t *__restrict rwlock) {
 if __unlikely(!rwlock) return EINVAL;
 return ENOSYS; /* TODO */
}
__public int
pthread_rwlock_trywrlock(pthread_rwlock_t *__restrict rwlock) {
 if __unlikely(!rwlock) return EINVAL;
 return ENOSYS; /* TODO */
}
__public int
pthread_rwlock_timedwrlock(pthread_rwlock_t *__restrict rwlock, struct timespec const *__restrict abstime) {
 if __unlikely(!rwlock || !abstime) return EINVAL;
 return ENOSYS; /* TODO */
}
__public int
pthread_rwlock_unlock(pthread_rwlock_t *__restrict rwlock) {
 if __unlikely(!rwlock) return EINVAL;
 return ENOSYS; /* TODO */
}


__public int
pthread_rwlockattr_init(pthread_rwlockattr_t *__restrict attr) {
 if __unlikely(!attr) return EINVAL;
 attr->__rw_kind = PTHREAD_RWLOCK_DEFAULT_NP;
 return 0;
}
__public int
pthread_rwlockattr_destroy(pthread_rwlockattr_t *__restrict attr) {
 return attr ? 0 : EINVAL;
}
__public int
pthread_rwlockattr_getpshared(pthread_rwlockattr_t const *__restrict attr,
                              int *__restrict pshared) {
 return (attr && pshared) ? (*pshared = 1,0) : EINVAL;
}
__public int
pthread_rwlockattr_setpshared(pthread_rwlockattr_t *__restrict attr,
                              int pshared) {
 return attr ? (pshared ? 0 : ENOSYS) : EINVAL;
}
__public int
pthread_rwlockattr_getkind_np(pthread_rwlockattr_t const *__restrict attr,
                              int *__restrict pref) {
 if __unlikely(!attr || !pref) return EINVAL;
 *pref = attr->__rw_kind;
 return 0;
}
__public int
pthread_rwlockattr_setkind_np(pthread_rwlockattr_t *__restrict attr,
                              int pref) {
 if __unlikely(!attr) return EINVAL;
 attr->__rw_kind = pref;
 return 0;
}


__DECL_END

#endif /* !__PTHREAD_RWLOCK_C__ */

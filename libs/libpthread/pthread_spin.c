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
#ifndef __PTHREAD_SPIN_C__
#define __PTHREAD_SPIN_C__ 1

#include <errno.h>
#include <kos/atomic.h>
#include <proc.h>
#include <pthread.h>
#include <stdint.h>

__DECL_BEGIN

__public int
pthread_spin_init(pthread_spinlock_t *lock,
                  int pshared) {
 if __unlikely(!lock) return EINVAL;
 lock->__ps_holder = 0;
 lock->__ps_locked = 0;
 return 0;
}
__public int pthread_spin_destroy(pthread_spinlock_t *lock) {
 if __unlikely(!lock) return EINVAL;
 if __unlikely(lock->__ps_locked) return EBUSY;
 return 0;
}
__public int pthread_spin_lock(pthread_spinlock_t *lock) {
 uintptr_t unique = thread_unique();
 if __unlikely(!lock) return EINVAL;
 if (!katomic_cmpxch(lock->__ps_locked,0,1)) {
  if __unlikely(lock->__ps_holder == unique) return EDEADLK;
  for (;;) {
   int spin_counter = 10000;
   /* Spin a bit... */
   while (spin_counter--) {
    if (katomic_cmpxch(lock->__ps_locked,0,1)) goto end;
   }
   pthread_yield();
  }
 }
end:
 lock->__ps_holder = unique;
 return 0;
}
__public int pthread_spin_trylock(pthread_spinlock_t *lock) {
 if __unlikely(!lock) return EINVAL;
 if (!katomic_cmpxch(lock->__ps_locked,0,1)) return EBUSY;
 lock->__ps_holder = thread_unique();
 return 0;
}
__public int pthread_spin_unlock(pthread_spinlock_t *lock) {
 if __unlikely(!lock) return EINVAL;
 if __unlikely(lock->__ps_holder != thread_unique()) return EPERM;
 assert(lock->__ps_locked);
 lock->__ps_holder = KTID_INVALID;
 lock->__ps_locked = 0; /* No need to use atomics here... */
 return 0;
}

__DECL_END

#endif /* !__PTHREAD_SPIN_C__ */

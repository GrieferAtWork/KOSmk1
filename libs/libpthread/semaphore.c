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
#ifndef __SEMAPHORE_C__
#define __SEMAPHORE_C__ 1

#include <semaphore.h>
#include <errno.h>
#include <stddef.h>
#include <kos/atomic.h>
#include <kos/futex.h>

__DECL_BEGIN

__public int
sem_init(sem_t *__restrict sem,
         int pshared,
         unsigned int value) {
 if __unlikely(!sem) { __set_errno(EINVAL); return -1; }
 (void)pshared;
 sem->__s_cnt = value;
 return 0;
}
__public int
sem_destroy(sem_t *__restrict sem) {
 return sem ? 0 : (__set_errno(EINVAL),-1);
}
__public int
sem_dotimedwait(sem_t *__restrict sem,
                struct timespec const *abstime) {
 unsigned int oldcount;
 kerrno_t error;
again: do {
  oldcount = katomic_load(*(unsigned int *)&sem->__s_cnt);
  if __unlikely(!oldcount) {
   /* atomic: if (sem->__s_cnt == 0) { recv(&sem->__s_cnt); } */
   error = kfutex_cmd(&sem->__s_cnt,KFUTEX_RECVIF(KFUTEX_EQUAL),0,NULL,abstime);
   if __likely(KE_ISOK(error) || error == KE_AGAIN) goto again;
   __set_errno(-error);
   return -1;
  }
 } while (!katomic_cmpxch(*(unsigned int *)&sem->__s_cnt,oldcount,oldcount-1));
 return 0;
}
__public int
sem_wait(sem_t *__restrict sem) {
 if __unlikely(!sem) { __set_errno(EINVAL); return -1; }
 return sem_dotimedwait(sem,NULL);
}
__public int
sem_timedwait(sem_t *__restrict sem,
              struct timespec const *__restrict abstime) {
 if __unlikely(!sem || !abstime) { __set_errno(EINVAL); return -1; }
 return sem_dotimedwait(sem,abstime);
}
__public int
sem_trywait(sem_t *__restrict sem) {
 unsigned int oldcount;
 if __unlikely(!sem) { __set_errno(EINVAL); return -1; }
 do {
  oldcount = katomic_load(*(unsigned int *)&sem->__s_cnt);
  if __unlikely(!oldcount) { __set_errno(EWOULDBLOCK); return -1; }
 } while (!katomic_cmpxch(*(unsigned int *)&sem->__s_cnt,oldcount,oldcount-1));
 return 0;
}
__public int
sem_post(sem_t *__restrict sem) {
 kerrno_t error;
 if __unlikely(!sem) { __set_errno(EINVAL); return -1; }
 katomic_fetchinc(*(unsigned int *)&sem->__s_cnt);
 error = kfutex_sendone(&sem->__s_cnt);
 if __likely(KE_ISOK(error)) return 0;
 __set_errno(-error);
 return -1;
}
__public int
sem_getvalue(sem_t *__restrict sem,
             unsigned int *__restrict sval) {
 if __unlikely(!sem || !sval) { __set_errno(EINVAL); return -1; }
 *sval = katomic_load(sem->__s_cnt);
 return 0;
}

__public sem_t *
sem_open(char const *__restrict name,
         int oflag, ...) {
 (void)name,(void)oflag; /* TODO: mmap shared_memory file. */
 __set_errno(ENOSYS);
 return NULL;
}
__public int
sem_close(sem_t *__restrict sem) {
 if (!sem) { __set_errno(EINVAL); return -1; }
 (void)sem; /* TODO: munmap shared_memory block. */
 __set_errno(ENOSYS);
 return -1;
}
__public int
sem_unlink(char const *__restrict name) {
 (void)name; /* TODO: delete reference-counted shared_memory file. */
 __set_errno(ENOSYS);
 return -1;
}

__DECL_END

#endif /* !__SEMAPHORE_C__ */

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
#include <malloc.h>
#include <fcntl.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <kos/fd.h>

#if 0
__DECL_BEGIN

int sem_init(sem_t *__restrict sem, int pshared, unsigned int value) {
 kassertobj(sem);
 // TODO: This syscall is missing
 sem->s_sem = ksem_new(value,pshared ? FD_CLOEXEC ? 0);
 if __likely(KE_ISOK(sem->s_sem)) return 0;
 __set_errno(-sem->s_sem);
 return -1;
}
int sem_destroy(sem_t *__restrict sem) {
 kassertobj(sem);
 return close(sem->s_sem);
}
int sem_wait(sem_t *__restrict sem) {
 kerrno_t error; kassertobj(sem);
 error = kfd_wait(sem->s_sem);
 if __unlikely(KE_ISOK(error)) return 0;
 __set_errno(-error);
 return -1;
}
int sem_timedwait(sem_t *__restrict sem, struct timespec const *__restrict abstime) {
 kerrno_t error; kassertobj(sem);
 kassertobj(abstime);
 error = kfd_timedwait(sem->s_sem,abstime);
 if __unlikely(KE_ISOK(error)) return 0;
 __set_errno(-error);
 return -1;
}
int sem_trywait(sem_t *__restrict sem) {
 kerrno_t error; kassertobj(sem);
 error = kfd_wait(sem->s_sem);
 if __unlikely(KE_ISOK(error)) return 0;
 __set_errno(-error);
 return -1;
}
int sem_post(sem_t *__restrict sem) {
 kassertobj(sem);
}
int sem_getvalue(sem_t *__restrict sem, int *__restrict sval) {
 kassertobj(sem);
 kassertobj(sval);
}

sem_t *sem_open(char const *__restrict name, int oflag, ...) {
 sem_t *result; va_list args;
 if __unlikely((result = omalloc(sem_t)) == NULL) return NULL;
 va_start(args,oflag);
 result->s_sem = open(name,oflag,va_arg(args,int));
 va_end(args);
 if __unlikely(result->s_sem == -1) {
  free(result);
  result = NULL;
 }
 return result;
}
int sem_close(sem_t *__restrict sem) {
 int error; kassertobj(sem);
 error = sem_destroy(sem);
 free(sem);
 return error;
}
int sem_unlink(char const *__restrict name) {
 return unlink(name);
}

__DECL_END
#endif

#endif /* !__SEMAPHORE_C__ */

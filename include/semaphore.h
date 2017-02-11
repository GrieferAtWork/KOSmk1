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
#ifndef __SEMAPHORE_H__
#ifndef _SEMAPHORE_H
#define __SEMAPHORE_H__ 1
#define _SEMAPHORE_H 1

#include <kos/compiler.h>

#ifdef __KERNEL__
#include <kos/kernel/sem.h>
#endif

__DECL_BEGIN

#ifdef __KERNEL__
typedef struct { struct ksem s_sem; } sem_t;
#else
typedef struct { int s_sem; } sem_t;
#endif

struct timespec;

// NOTE: In kernel mode, these functions return kerrno_t
//       values (because the kernel doesn't have errno)
extern __wunused int sem_init(sem_t *__restrict sem, int pshared, unsigned int value);
extern           int sem_destroy(sem_t *__restrict sem);
extern           int sem_wait(sem_t *__restrict sem);
extern __wunused int sem_timedwait(sem_t *__restrict sem, struct timespec const *__restrict abstime);
extern __wunused int sem_trywait(sem_t *__restrict sem);
extern           int sem_post(sem_t *__restrict sem);
extern __wunused int sem_getvalue(sem_t *__restrict sem, int *__restrict sval);
extern __wunused sem_t *sem_open(char const *__restrict name, int oflag, ...);
extern           int sem_close(sem_t *__restrict sem);
extern           int sem_unlink(char const *__restrict name);

__DECL_END

#endif /* !_SEMAPHORE_H */
#endif /* !__SEMAPHORE_H__ */

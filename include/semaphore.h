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
typedef struct ksem sem_t;
#else
typedef struct { unsigned int volatile __s_cnt; } sem_t;
#endif

struct timespec;

// NOTE: In kernel mode, these functions return kerrno_t
//       values (because the kernel doesn't have errno)
extern __wunused __nonnull((1))   int sem_init __P((sem_t *__restrict __sem, int __pshared, unsigned int __value));
extern           __nonnull((1))   int sem_destroy __P((sem_t *__restrict __sem));
extern           __nonnull((1))   int sem_wait __P((sem_t *__restrict __sem));
extern __wunused __nonnull((1,2)) int sem_timedwait __P((sem_t *__restrict __sem, struct timespec const *__restrict __abstime));
extern __wunused __nonnull((1))   int sem_trywait __P((sem_t *__restrict __sem));
extern           __nonnull((1))   int sem_post __P((sem_t *__restrict __sem));
extern __wunused __nonnull((1,2)) int sem_getvalue __P((sem_t *__restrict __sem, unsigned int *__restrict __sval));
extern __wunused __nonnull((1))   sem_t *sem_open __P((char const *__restrict __name, int __oflag, ...));
extern           __nonnull((1))   int sem_close __P((sem_t *__restrict __sem));
extern           __nonnull((1))   int sem_unlink __P((char const *__restrict __name));

__DECL_END

#endif /* !_SEMAPHORE_H */
#endif /* !__SEMAPHORE_H__ */

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
#ifndef	__SCHED_H__
#ifndef	_SCHED_H
#define	__SCHED_H__	1
#define	_SCHED_H	1

#include <kos/compiler.h>
#ifndef __ASSEMBLY__
#include <kos/types.h>
#include <time.h> /*< The standard wants this... */

#ifndef __KERNEL__
__DECL_BEGIN

extern int sched_yield(void);

struct sched_param {
    int sched_priority; /*< process execution scheduling priority. */
};

// NOTE: These currently have no effect... (KOS currently is hard-coded to be 'SCHED_RR')
#define SCHED_FIFO  0 /*< First in-first out (FIFO) scheduling policy. */
#define SCHED_RR    1 /*< Round robin scheduling policy. */
#define SCHED_OTHER 2 /*< Another scheduling policy. */

extern int sched_get_priority_max(int policy);
extern int sched_get_priority_min(int policy);
extern int sched_getparam(__pid_t pid, struct sched_param *param);
extern int sched_getscheduler(__pid_t pid);
extern int sched_rr_get_interval(__pid_t pid, struct timespec *interval);
extern int sched_setparam(__pid_t pid, struct sched_param const *param);
extern int sched_setscheduler(__pid_t pid, int policy, struct sched_param const *param);

__DECL_END
#else /* !__KERNEL__ */
#include <kos/kernel/sched_yield.h>
#define sched_yield   ktask_tryyield
#endif /* __KERNEL__ */
#endif /* !__ASSEMBLY__ */

#endif /* !_SCHED_H */
#endif /* !__SCHED_H__ */

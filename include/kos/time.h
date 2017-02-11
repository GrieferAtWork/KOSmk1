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
#ifndef __KOS_TIME_H__
#define __KOS_TIME_H__ 1

#include <kos/config.h>

#ifndef __NO_PROTOTYPES
#include <kos/compiler.h>
#include <kos/syscall.h>
#include <kos/errno.h>

#ifdef __KERNEL__
#include <kos/kernel/time.h>
#endif

__DECL_BEGIN

struct timespec;

#ifndef __KERNEL__
__local _syscall1(kerrno_t,ktime_getnow,struct timespec *,ptm);
__local _syscall1(kerrno_t,ktime_setnow,struct timespec *,ptm);
__local _syscall1(kerrno_t,ktime_getcpu,struct timespec *,ptm);

__local kerrno_t ktime_getnoworcpu(struct timespec *ptm) {
 kerrno_t error = ktime_getnow(ptm);
 if __unlikely(KE_ISERR(error)) error = ktime_getcpu(ptm);
 return error;
}
#endif /* !__KERNEL__ */

__DECL_END

#endif

#endif /* !__KOS_TIME_H__ */

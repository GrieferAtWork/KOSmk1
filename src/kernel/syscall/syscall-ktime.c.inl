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
#ifndef __KOS_KERNEL_SYSCALL_SYSCALL_KTIME_C_INL__
#define __KOS_KERNEL_SYSCALL_SYSCALL_KTIME_C_INL__ 1

#include "syscall-common.h"
#include <kos/kernel/time.h>
#include <kos/errno.h>
#include <kos/syscallno.h>

__DECL_BEGIN

/* _syscall1(kerrno_t,ktime_getnow,struct timespec *,ptm); */
SYSCALL(sys_ktime_getnow) {
 LOAD1(struct timespec *,U(ptm));
 RETURN(ktime_getnow(ptm));
}

/* _syscall1(kerrno_t,ktime_setnow,struct timespec *,ptm); */
SYSCALL(sys_ktime_setnow) {
 LOAD1(struct timespec *,U(ptm));
 kerrno_t error;
 KTASK_CRIT_BEGIN_FIRST
 error = ktime_setnow(ptm);
 KTASK_CRIT_END
 RETURN(error);
}

/* _syscall1(kerrno_t,ktime_getcpu,struct timespec *,ptm); */
SYSCALL(sys_ktime_getcpu) {
 LOAD1(struct timespec *,U(ptm));
 ktime_getcpu(ptm);
 RETURN(KE_OK);
}

__DECL_END

#endif /* !__KOS_KERNEL_SYSCALL_SYSCALL_KTIME_C_INL__ */

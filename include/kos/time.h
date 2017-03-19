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
#else
#include <kos/types.h>
#endif

__DECL_BEGIN

struct timespec;

#ifndef __KERNEL__
__local _syscall1(kerrno_t,ktime_getnow,struct timespec *,ptm);
__local _syscall1(kerrno_t,ktime_setnow,struct timespec *,ptm);

#if __SIZEOF_POINTER__ >= 8
__local _syscall0(__u64,ktime_htick);
__local _syscall0(__u64,ktime_hfreq);
#elif !defined(__ASSEMBLY__)
__local __u64 ktime_htick(void);
__local __u64 ktime_hfreq(void);

__local __u64 ktime_htick(void) {
 __u32 lo,hi;
 __asm_volatile__("int $" __PP_STR(__SYSCALL_INTNO) "\n"
                  : "=a" (lo), "=c" (hi)
                  : "a" (SYS_ktime_htick)
                  : "memory");
 return (__u64)lo | ((__u64)hi << 32);
}
__local __u64 ktime_hfreq(void) {
 __u32 lo,hi;
 __asm_volatile__("int $" __PP_STR(__SYSCALL_INTNO) "\n"
                  : "=a" (lo), "=c" (hi)
                  : "a" (SYS_ktime_hfreq)
                  : "memory");
 return (__u64)lo | ((__u64)hi << 32);
}
#endif
#endif /* !__KERNEL__ */

__DECL_END

#endif

#endif /* !__KOS_TIME_H__ */

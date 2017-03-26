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

#ifndef __ASSEMBLY__
#ifndef __NO_PROTOTYPES
#ifdef __KERNEL__
#include <kos/kernel/time.h>
#else /* __KERNEL__ */
#include <kos/compiler.h>
#include <kos/errno.h>
#include <kos/syscall.h>
#include <kos/types.h>
#include <kos/timespec.h>

__DECL_BEGIN

struct timespec;

__local kerrno_t ktime_getnow __D1(struct timespec *,__ptm) {
 kerrno_t __error;
 __asm_volatile__(__ASMSYSCALL_DOINT
                  : "=a" (__error)
#if _TIME_T_BITS > __SIZEOF_POINTER__*8
                  , "=c" (((__uintptr_t *)&__ptm->tv_sec)[0])
                  , "=d" (((__uintptr_t *)&__ptm->tv_sec)[1])
                  , "=b" (__ptm->tv_nsec)
#else
                  , "=c" (__ptm->tv_sec)
                  , "=d" (__ptm->tv_nsec)
#endif
                  : "a" (SYS_ktime_getnow)
                  );
 return __error;
}
__local kerrno_t ktime_setnow __D1(struct timespec const *,__ptm) {
 kerrno_t __error;
 __asm_volatile__(__ASMSYSCALL_DOINT
                  : "=a" (__error)
                  : "a" (SYS_ktime_setnow)
#if _TIME_T_BITS == 64
                  , "c" (((__u32 *)&__ptm->tv_sec)[0])
                  , "d" (((__u32 *)&__ptm->tv_sec)[1])
                  , "b" (__ptm->tv_nsec)
#else
                  , "c" (__ptm->tv_sec)
                  , "d" (__ptm->tv_nsec)
#endif
                  );
 return __error;
}

#if __SIZEOF_POINTER__ >= 8
__local _syscall0(__u64,ktime_htick);
__local _syscall0(__u64,ktime_hfreq);
#elif !defined(__ASSEMBLY__)
__local __u64 ktime_htick(void);
__local __u64 ktime_hfreq(void);

__local __u64 ktime_htick(void) {
 __u32 lo,hi;
 __asm_volatile__(__ASMSYSCALL_DOINT
                  : "=a" (lo), "=c" (hi)
                  : "a" (SYS_ktime_htick)
                  : "memory");
 return (__u64)lo | ((__u64)hi << 32);
}
__local __u64 ktime_hfreq(void) {
 __u32 lo,hi;
 __asm_volatile__(__ASMSYSCALL_DOINT
                  : "=a" (lo), "=c" (hi)
                  : "a" (SYS_ktime_hfreq)
                  : "memory");
 return (__u64)lo | ((__u64)hi << 32);
}
#endif

__DECL_END

#endif /* !__KERNEL__ */
#endif /* !__NO_PROTOTYPES */
#endif /* !__ASSEMBLY__ */

#endif /* !__KOS_TIME_H__ */

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
#ifndef __KOS_KERNEL_TIME_H__
#define __KOS_KERNEL_TIME_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/compiler.h>
#ifndef __ASSEMBLY__
#include <kos/atomic.h>
#include <kos/errno.h>
#include <kos/kernel/types.h>

__DECL_BEGIN


struct timespec;

//////////////////////////////////////////////////////////////////////////
// Attempts to retrieve the current time, but fails with
// 'KE_NOSYS' if the host system does not have a RTC.
// @return: KE_OK:    The current time was set/written to '*tmnow'.
// @return: KE_NOSYS: The host system does not have a RTC.
// @return: KE_ACCES: [ktime_setnow] The calling task does not have the required permissions.
extern __wunused __nonnull((1)) kerrno_t ktime_getnow(struct timespec *__restrict tmnow);
extern __wunused __nonnull((1)) kerrno_t ktime_setnow(struct timespec const *__restrict tmnow);
extern __wunused __nonnull((1)) kerrno_t ktime_setnow_k(struct timespec const *__restrict tmnow);

//////////////////////////////////////////////////////////////////////////
// Returns a cpu-based type with some undefined offset in the past.
// This time can still be assumed to run constant, though it may
// or may not match the current RTC time.
extern __nonnull((1)) void ktime_getcpu(struct timespec *__restrict tmcpu);

//////////////////////////////////////////////////////////////////////////
// Returns some sort of constant time
extern __nonnull((1)) void ktime_getnoworcpu(struct timespec *__restrict tmval);


struct cmostime {
 __u8  ct_second; /*< [0..59] Seconds. */
 __u8  ct_minute; /*< [0..59] Minutes. */
 __u8  ct_hour;   /*< [0..23] Hours. */
 __u8  ct_day;    /*< [1..31] Day of month. */
 __u8  ct_month;  /*< [1..12] Month of year. */
 __u32 ct_year;   /*< [0..~~] Year. */
};

extern __wunused __nonnull((1)) __time_t cmostime_to_time(struct cmostime const *__restrict cmtime);
extern __nonnull((2)) void time_to_cmostime(__time_t t, struct cmostime *__restrict cmtime);

#define CMOS_IOPORT_CODE 0x70
#define CMOS_IOPORT_DATA 0x71
#define CMOS_REGISTER_SECOND      0x00
#define CMOS_REGISTER_MINUTE      0x02
#define CMOS_REGISTER_HOUR        0x04
#define CMOS_REGISTER_DAY         0x07
#define CMOS_REGISTER_MONTH       0x08
#define CMOS_REGISTER_YEAR        0x09
#define CMOS_REGISTER_B           0x0B
#define CMOS_REGISTERB_FLAG_NOBCD 0x04 /*< When set, CMOS time is in decimal format. */
#define CMOS_REGISTERB_FLAG_24H   0x02 /*< When set, CMOS hours uses a 24-hour format. */

extern __wunused __nonnull((1)) kerrno_t cmos_getnow(struct cmostime *__restrict tmnow);
extern __wunused __nonnull((1)) kerrno_t cmos_setnow(struct cmostime const *__restrict tmnow);

extern void kernel_initialize_cmos(void);


#define HZ  256 /*< Jiffies per second. */
extern __attribute__((__section__(".data"))) __u64 volatile __jiffies64;
#define jiffies64   katomic_load(__jiffies64)

__DECL_END
#endif /* !__ASSEMBLY__ */
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_TIME_H__ */

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

typedef __u64 hstamp_t;
struct hpet {
 /* Low-level high-precision timer. */
 hstamp_t  (*hp_next)(void);  /*< [1..1] Same as 'read', but for emulated timers, increment the counter. */
 hstamp_t  (*hp_read)(void);  /*< [1..1] Read and return the current timer value. */
 char const *hp_name;         /*< [1..1] Timer name. */
};

extern struct hpet const ktime_hpet_emulated; /*< Emulated (fallback) timer. */
#ifdef __x86__
extern struct hpet const ktime_hpet_x86rdtsc; /*< X86 cpu timer using 'rdtsc'. */
#endif


/* Current high-precision timer (Set once during boot; read-only post-initialization). */
extern struct hpet ktime_hpet;

#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Return the system's current high-performance time/frequency.
// WARNING: The high-performance frequency is quite volatile
//          and should not be relied upon to never change.
// @return: * : [ktime_htick] A constantly incrementing value that may be used to high-precision timings.
// @return: * : [ktime_hfreq] A constantly incrementing value that may be used to high-precision timings.
extern hstamp_t ktime_htick(void);
extern hstamp_t ktime_hfreq(void);
#else
extern hstamp_t timer_tickfreq;
#define ktime_htick()  (*ktime_hpet.hp_read)()
#define ktime_hfreq()    NOIRQ_EVAL(timer_tickfreq)
#endif

//////////////////////////////////////////////////////////////////////////
// Highly intelligent function to automatically sync & align the timer time with
// CMOS/RTC time, while adding additional precision to the current system time
// using a previously configured HPET through 'ktime_hpet'.
// WARNING: This function should not be called randomly, but instead should
//          itself be called periodically, such as during IRQ preemption.
extern void ktime_irqtick(void);


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

extern __nonnull((1)) kerrno_t cmos_getnow(struct cmostime *__restrict tmnow);
extern __nonnull((1)) kerrno_t cmos_setnow(struct cmostime const *__restrict tmnow);

extern void kernel_initialize_cmos(void);
extern void kernel_initialize_time(void);

__DECL_END
#endif /* !__ASSEMBLY__ */
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_TIME_H__ */

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
struct khpet {
 /* Low-level high-precision timer. */
 hstamp_t  (*hp_next)(void);  /*< [1..1] Same as 'read', but for emulated timers, increment the counter. */
 hstamp_t  (*hp_read)(void);  /*< [1..1] Read and return the current timer value. */
 char const *hp_name;         /*< [1..1] Timer name. */
};

/* Current high-precision timer (Set once during boot; read-only post-initialization). */
extern struct khpet khpet_current;
extern struct khpet const khpet_emulated; /*< Emulated (fallback) timer. */
#ifdef __x86__
extern struct khpet const khpet_x86rdtsc; /*< X86 cpu timer using 'rdtsc'. */
#endif



struct krtc {
 kerrno_t  (*rtc_gettime)(__time_t *__restrict result); /*< [1..1] Get current RTC time. */
 kerrno_t  (*rtc_settime)(__time_t value);              /*< [1..1] Set current RTC time. */
 char const *rtc_name;                                  /*< [1..1] RTC name. */
};

/* Current RTC clock (Set once during boot; read-only post-initialization). */
extern struct krtc krtc_current;
extern struct krtc const krtc_cmos; /*< CMOS clock. */



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
#define ktime_htick()  (*khpet_current.hp_read)()
#define ktime_hfreq()    NOIRQ_EVAL(timer_tickfreq)
#endif

//////////////////////////////////////////////////////////////////////////
// Highly intelligent function to automatically sync & align the timer time with
// CMOS/RTC time, while adding additional precision to the current system time
// using a previously configured HPET through 'khpet_current'.
// WARNING: This function should not be called randomly, but instead should
//          itself be called periodically, such as during IRQ preemption.
extern void ktime_irqtick(void);


struct timespec;

#ifdef __INTELLISENSE__
extern __nonnull((1)) void ktime_getnow(struct timespec *__restrict tmnow);
#else
extern /*noirq*/__nonnull((1)) void __ktime_getnow_ni(struct timespec *__restrict tmnow);
#define ktime_getnow(tmnow) NOIRQ_EVAL_V(__ktime_getnow_ni(tmnow))
#endif

//////////////////////////////////////////////////////////////////////////
// Attempts to retrieve the current time, but fails with
// 'KE_NOSYS' if the host system does not have a CMOS/RTC.
// @return: KE_OK:    The current time was set/written to '*tmnow'.
// @return: KE_NOSYS: The host system does not have a CMOS/RTC.
// @return: KE_ACCES: [ktime_setnow] The calling task does not have the required permissions.
#ifdef __INTELLISENSE__
extern __wunused __nonnull((1)) kerrno_t ktime_hw_getnow(__time_t *__restrict tmnow);
extern __wunused kerrno_t ktime_hw_setnow(__time_t tmnow);
#else
#define ktime_hw_getnow(tmnow) (*krtc_current.rtc_gettime)(tmnow)
#define ktime_hw_setnow(tmnow) (*krtc_current.rtc_settime)(tmnow)
#endif



extern void kernel_initialize_time(void);
extern void kernel_initialize_hpet(void);

__DECL_END
#endif /* !__ASSEMBLY__ */
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_TIME_H__ */

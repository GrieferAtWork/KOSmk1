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
#ifndef __KOS_KERNEL_TIME_C__
#define __KOS_KERNEL_TIME_C__ 1

#include <kos/compiler.h>
#include <kos/kernel/debug.h>
#include <kos/kernel/proc.h>
#include <kos/kernel/time.h>
#include <kos/syslog.h>
#include <kos/timespec.h>
#include <stdio.h>
#include <sys/io.h>
#include <time.h>
#include <hw/rtc/cmos.h>
#ifdef __x86__
#include <kos/arch/x86/cpuid.h>
#endif

__DECL_BEGIN


/* CMOS/RTC time when time was last synced.
 * This is the base value when calculating actual time. */
static time_t timer_time = 0;

/* Time tick when the last synced second started
 * This is the HPET base value describing its ticks when 'timer_time'. */
static hstamp_t timer_ticklast = 0;

/* Amount of ticks per second as read from the active HPET.
 * NOTE: Is ZERO(0) when no data has been collected yet. */
hstamp_t timer_tickfreq = 0;

/* Set the timer's tick frequency to the given value. */
#define SET_TICKFREQ(f) (timer_tickfreq = (f))

#define KTIME_DYNAMIC_SYNC 0 /* TODO: This doesn't work correctly. */

/* High precision timers & sub-second sleep function */
void ktime_irqtick(void) {
 /* In seconds: Stability threshold to autoconfigure 'timer_delay'
  *             against and try to align IRQ time with.
  * Defining this as something other than ONE(1) is possible, but may degrade precision. */
#define TIMER_FREQ_STABILITY 1
#if KTIME_DYNAMIC_SYNC
 /* Current resync delay:
  *  - In IRQ ticks
  *  - Tries to align itself to fire every 'TIMER_FREQ_STABILITY' seconds.
  * >> This could be considered as IRQ_TICKS_PER_SECOND. */
 static unsigned long timer_delay = 8;
 /* Monotonically increasing IRQ timer used to track how IRQ
  * ticks have passed since boot. This value is only used to
  * fire CMOS/RTC sync code every 'timer_delay' IRQ ticks,
  * meaning it doesn't matter when this rolls over. */
 static unsigned long timer_irq   = 7;
 /* Special synchronization variable used to align 'timer_time' with
  * full seconds, thus preventing seemingly random jumps through time
  * whenever the CMOS/RTC clock jumps, but 'timer_delay' is not ZERO.
  * With that in mind, this variable is also used to align get
  * 'timer_delay' being able to mod 'timer_irq' into ZERO(0) with the
  * event of the CMOS/RTC clock just having changed its time.
  * >> It automatically adjusts itself to reduce
  *    CMOS/RTC lookups to the minimum amount required. */
 static unsigned long timer_sync  = 20;
 /* When calculating a new frequency during clock sync,
  * compare the old frequency against the new.
  * If the difference is greater than this value, it is likely that
  * the timer clock isn't aligned to full seconds, which then is
  * handled by increasing 'timer_sync' to align itself to full
  * seconds for a duration of 3 seconds in IRQ ticks. */
 static hstamp_t timer_avgdiff     = 0;
 static hstamp_t timer_avgdiff_min = 0;
 static hstamp_t timer_avgdiff_max = 0;
#endif /* KTIME_DYNAMIC_SYNC */
 hstamp_t time = (*khpet_current.hp_next)();
 /* Called at the start of a preemption IRQ. */
#if KTIME_DYNAMIC_SYNC
 if (!(++timer_irq % timer_delay) ||
      (timer_sync && timer_sync--))
#endif /* KTIME_DYNAMIC_SYNC */
 {
  /* Resync the timer. */
  unsigned int passed_second; time_t newtime;
  if __unlikely(KE_ISERR(ktime_hw_getnow(&newtime))) return;
  /* Figure out how many seconds have passed since the last sync. */
  passed_second = newtime-timer_time;
  if (passed_second >= TIMER_FREQ_STABILITY) {
   hstamp_t new_freq;
#if KTIME_DYNAMIC_SYNC
   hstamp_t freq_diff;
   if (passed_second > TIMER_FREQ_STABILITY &&
       timer_delay > 1 &&
     !(timer_irq % timer_delay)) { --timer_delay; timer_irq = 0; }
#endif /* KTIME_DYNAMIC_SYNC */
   /* Make sure that at least some discernible time has passed */
   timer_time     = newtime;
   /* Calculate the new frequency, based on how much time has passed. */
   new_freq  = (time-timer_ticklast);
   new_freq /= passed_second;
   if __unlikely(!new_freq) new_freq = 1; /* Prevent illegal frequencies. */
#if KTIME_DYNAMIC_SYNC
   if (timer_tickfreq) {
    /* Automatically sync with seconds if the new frequency greatly differs from the old.
     * While all the other code is used to automatically adjust tick frequency to full seconds,
     * this code */
    freq_diff = new_freq < timer_tickfreq ? timer_tickfreq-new_freq : new_freq-timer_tickfreq;
    if (freq_diff > timer_avgdiff_max) {
     /* Sync with full seconds for the duration of three full ticks.
      * 3 was chosen to make it very likely that we'll be able to hit at
      * least one jump in seconds, even if the last one just occurred and the
      * IRQ frequency will increase before the. */
     timer_sync = timer_delay*TIMER_FREQ_STABILITY*3;
     //printf("[TIME] Begin syncing seconds for %lu IRQ ticks after "
     //                     "unacceptable freq diff of %I64u > %I64u\n",
     //          timer_sync,freq_diff,timer_avgdiff);
     /* Define a frequency differential threshold that lazily
      * adjusts itself to slow changes in frequencies. */
     if __unlikely(!timer_avgdiff) {
      timer_avgdiff = freq_diff;
      goto update_avgdiff2;
     } else {
      goto update_avgdiff;
     }
    } else if (freq_diff < timer_avgdiff_min) {
update_avgdiff:
     timer_avgdiff = (timer_avgdiff+freq_diff)/2;
update_avgdiff2:
     /* Take the average between the old frequency different and
      * the new one, then add a threshold of +1.5 to it. */
     timer_avgdiff_min = timer_avgdiff/2;
     timer_avgdiff_max = (timer_avgdiff*3)/2;
    }
   }
#endif /* KTIME_DYNAMIC_SYNC */
   /* Actually set the new frequency. */
#if 1
   SET_TICKFREQ(new_freq);
#else
   /* Make frequency changes be lazy. */
   if (timer_tickfreq) new_freq = (new_freq+timer_tickfreq)/2;
   SET_TICKFREQ(new_freq);
#endif
   /* Update the last  */
   timer_ticklast = time;
  }
#if KTIME_DYNAMIC_SYNC
  else if (!(timer_irq % timer_delay)) {
   ++timer_delay;
   timer_irq = 0;
  }
#endif /* KTIME_DYNAMIC_SYNC */
 }
}

/*noirq*/void __ktime_getnow_ni(struct timespec *__restrict tmnow) {
 hstamp_t ticks_passed;
 NOIRQ_MARK
 if __likely(timer_tickfreq) {
  tmnow->tv_sec  = timer_time;
  ticks_passed   = (*khpet_current.hp_read)()-timer_ticklast;
  /* Figure out the amount of nanoseconds that have passed. */
  ticks_passed  *= UINT64_C(1000000000);
  ticks_passed  /= timer_tickfreq;
  tmnow->tv_sec += ticks_passed / 1000000000l;
  tmnow->tv_nsec = ticks_passed % 1000000000l;
 } else {
  /* Special case: Timer isn't configured yet. */
  if __unlikely(KE_ISERR(ktime_hw_getnow(&tmnow->tv_sec))) tmnow->tv_sec = 0;
  tmnow->tv_nsec = 0;
#if 1
  if (tmnow->tv_sec > timer_time) {
   hstamp_t currtick = (*khpet_current.hp_read)();
   unsigned int passed_second = tmnow->tv_sec-timer_time;
   /* We can calculate initial frequency data. */
   ticks_passed  = (currtick-timer_ticklast);
   ticks_passed /= passed_second;
   if __unlikely(!ticks_passed) ticks_passed = 1; /* Prevent illegal frequencies. */
   SET_TICKFREQ(ticks_passed);
   timer_time     = tmnow->tv_sec;
   timer_ticklast = currtick;
  }
#endif
 }
}

struct khpet khpet_current = {NULL,NULL,NULL};
void kernel_initialize_hpet(void) {
 assert(krtc_current.rtc_gettime &&
        krtc_current.rtc_settime);
#ifdef __x86__
 if (x86_cpufeatures()&X86_CPUFEATURE_TSC) {
  /* Can use the X86 rdtsc counter. */
  memcpy(&khpet_current,&khpet_x86rdtsc,sizeof(struct khpet));
  goto has_hpet;
 }
#endif

 /* Fallback: Use an emulated HPET timer if there's no other option. */
 memcpy(&khpet_current,&khpet_emulated,sizeof(struct khpet));
 goto has_hpet;
has_hpet:
 timer_ticklast = (*khpet_current.hp_next)();
 if __unlikely(KE_ISERR(ktime_hw_getnow(&timer_time))) timer_time = 0;
 k_syslogf(KLOG_INFO,"[init] Time...\n");
}


struct krtc krtc_current = {NULL,NULL,NULL};
void kernel_initialize_time(void) {
 /* Fallback: Use CMOS time. */
 kernel_initialize_cmos();
 memcpy(&krtc_current,&krtc_cmos,sizeof(struct krtc));
}

__DECL_END


#ifndef __INTELLISENSE__
#include "time-hpet.c.inl"
#include "time-rtc.c.inl"
#endif


#include <kos/kernel/proc.h>
#include <kos/kernel/syscall.h>
#include <kos/kernel/util/string.h>

__DECL_BEGIN

KSYSCALL_DEFINE_EX0(r,kerrno_t,ktime_getnow) {
 struct timespec tmnow;
 ktime_getnow(&tmnow);
#if _TIME_T_BITS > __SIZEOF_POINTER__*8
 regs->regs.ecx = ((__uintptr_t *)&tmnow.tv_sec)[0];
 regs->regs.edx = ((__uintptr_t *)&tmnow.tv_sec)[1];
 regs->regs.ebx = tmnow.tv_nsec;
#else
 regs->regs.ecx = tmnow.tv_sec;
 regs->regs.edx = tmnow.tv_nsec;
#endif
 return KE_OK;
}

KSYSCALL_DEFINE_EX0(rc,kerrno_t,ktime_setnow) {
 struct timespec tmnow;
 if __unlikely(!kproc_hasflag(kproc_self(),KPERM_FLAG_CHTIME)) return KE_ACCES;
#if _TIME_T_BITS > __SIZEOF_POINTER__*8
 ((__uintptr_t *)&tmnow.tv_sec)[0] = regs->regs.ecx;
 ((__uintptr_t *)&tmnow.tv_sec)[1] = regs->regs.edx;
 tmnow.tv_nsec = regs->regs.ebx;
#else
 tmnow.tv_sec = regs->regs.ecx;
 tmnow.tv_nsec = regs->regs.edx;
#endif
 return ktime_hw_setnow(tmnow.tv_sec);
}

#if __SIZEOF_POINTER__ >= 8
KSYSCALL_DEFINE0(__u64,ktime_htick) { return ktime_htick(); }
KSYSCALL_DEFINE0(__u64,ktime_hfreq) { return ktime_hfreq(); }
#else
KSYSCALL_DEFINE_EX0(r,__u32,ktime_htick) {
 __u64 tick = ktime_htick();
 regs->regs.ecx = (__u32)(tick >> 32);
 return (__u32)tick;
}
KSYSCALL_DEFINE_EX0(r,__u32,ktime_hfreq) {
 __u64 freq = ktime_hfreq();
 regs->regs.ecx = (__u32)(freq >> 32);
 return (__u32)freq;
}
#endif

__DECL_END


#endif /* !__KOS_KERNEL_TIME_C__ */

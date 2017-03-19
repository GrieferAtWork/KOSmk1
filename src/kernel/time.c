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


struct hpet ktime_hpet = {NULL,NULL,NULL};

void kernel_initialize_time(void) {
 struct cmostime cmtime;
#ifdef __x86__
 if (x86_cpufeatures()&X86_CPUFEATURE_TSC) {
  /* Can use the X86 rdtsc counter. */
  memcpy(&ktime_hpet,&ktime_hpet_x86rdtsc,sizeof(struct hpet));
  goto has_hpet;
 }
#endif

 /* Fallback: Use an emulated HPET timer if there's no other option. */
 memcpy(&ktime_hpet,&ktime_hpet_emulated,sizeof(struct hpet));
 goto has_hpet;
has_hpet:
 cmos_getnow(&cmtime);
 timer_ticklast = (*ktime_hpet.hp_next)();
 timer_time     = cmostime_to_time(&cmtime);
}


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
 hstamp_t time = (*ktime_hpet.hp_next)();
 /* Called at the start of a preemption IRQ. */
#if KTIME_DYNAMIC_SYNC
 if (!(++timer_irq % timer_delay) ||
      (timer_sync && timer_sync--))
#endif /* KTIME_DYNAMIC_SYNC */
 {
  /* Resync the timer. */
  unsigned int passed_second;
  struct cmostime cmtime;
  time_t newtime;
  cmos_getnow(&cmtime);
  newtime = cmostime_to_time(&cmtime);
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

kerrno_t ktime_getnow(struct timespec *__restrict tmnow) {
 hstamp_t ticks_passed;
 NOIRQ_BEGIN
 if __likely(timer_tickfreq) {
  tmnow->tv_sec  = timer_time;
  ticks_passed   = (*ktime_hpet.hp_read)()-timer_ticklast;
  /* Figure out the amount of nanoseconds that have passed. */
  ticks_passed  *= UINT64_C(1000000000);
  ticks_passed  /= timer_tickfreq;
  tmnow->tv_sec += ticks_passed / 1000000000l;
  tmnow->tv_nsec = ticks_passed % 1000000000l;
 } else {
  /* Special case: Timer isn't configured yet. */
  struct cmostime cmtime;
  kassertobj(tmnow);
  tmnow->tv_sec  = KE_ISOK(cmos_getnow(&cmtime)) ? cmostime_to_time(&cmtime) : 0;
  tmnow->tv_nsec = 0;
#if 1
  if (tmnow->tv_sec > timer_time) {
   hstamp_t currtick = (*ktime_hpet.hp_read)();
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
 NOIRQ_END
 return KE_OK;
}
kerrno_t ktime_setnow_k(struct timespec const *__restrict tmnow) {
 struct cmostime cmtime;
 kassertobj(tmnow);
 time_to_cmostime(tmnow->tv_sec,&cmtime);
 return cmos_setnow(&cmtime);
}
kerrno_t ktime_setnow(struct timespec const *__restrict tmnow) {
 if __unlikely(!kproc_hasflag(kproc_self(),KPERM_FLAG_CHTIME)) return KE_ACCES;
 return ktime_setnow_k(tmnow);
}

void ktime_getnoworcpu(struct timespec *__restrict tmval) {
 __evalexpr(ktime_getnow(tmval));
}






static int century_register = 0x00; /* Set by ACPI table parsing code if possible */
static __u8 cmos_regb;

#if 0
#   define cmos_isupdating()        0
#   define cmos_getregister(reg)    0
#   define cmos_setregister(reg,v)  0
#else
#   define cmos_isupdating()       (outb(CMOS_IOPORT_CODE,0x0A),(inb(CMOS_IOPORT_DATA)&0x80)!=0)
#   define cmos_getregister(reg)   (outb(CMOS_IOPORT_CODE,reg),inb(CMOS_IOPORT_DATA))
#   define cmos_setregister(reg,v) (outb(CMOS_IOPORT_CODE,reg),outb(v,CMOS_IOPORT_DATA))
#endif

void kernel_initialize_cmos(void) {
 cmos_regb      = cmos_getregister(CMOS_REGISTER_B);
 kernel_initialize_time();
 k_syslogf(KLOG_INFO,"[init] CMOS...\n");
}

time_t cmostime_to_time(struct cmostime const *__restrict cmtime) {
 time_t result;
 kassertobj(cmtime);
 if __unlikely(cmtime->ct_year < __LINUX_TIME_START_YEAR) return 0;
 result  = (time_t)((__time_years2days((__u64)cmtime->ct_year)-__time_years2days(__LINUX_TIME_START_YEAR))*__SECS_PER_DAY);
 result += __time_monthstart_yday[__time_isleapyear(cmtime->ct_year)][(cmtime->ct_month-1) % 12]*__SECS_PER_DAY;
 result += (cmtime->ct_day-1)*__SECS_PER_DAY;
 result += cmtime->ct_hour*60*60;
 result += cmtime->ct_minute*60;
 result += cmtime->ct_second;
 return result;
}
void time_to_cmostime(time_t t, struct cmostime *__restrict cmtime) {
 __u64 days; __u8 i; time_t const *monthvec;
 kassertobj(cmtime);
 cmtime->ct_second = (__u8)(t % 60);
 cmtime->ct_minute = (__u8)((t/60) % 60);
 cmtime->ct_hour   = (__u8)((t/(60*60)) % 24);
 days = (__u64)(t/__SECS_PER_DAY)+__time_years2days(__LINUX_TIME_START_YEAR);
 cmtime->ct_year = (__u32)__time_days2years(days);
 days -= __time_years2days(cmtime->ct_year);
 monthvec = __time_monthstart_yday[__time_isleapyear(cmtime->ct_year)];
 /* Find the appropriate month */
 for (i = 1; i < 12; ++i) if (monthvec[i] >= days) break;
 cmtime->ct_month = i;
 days -= monthvec[i-1];
 cmtime->ct_day = days+1;
}

kerrno_t cmos_getnow(struct cmostime *__restrict tmnow) {
 __u8 newcent = 0,centnow = 0;
 kassertobj(tmnow);
 while (cmos_isupdating());
 tmnow->ct_second = cmos_getregister(CMOS_REGISTER_SECOND);
 tmnow->ct_minute = cmos_getregister(CMOS_REGISTER_MINUTE);
 tmnow->ct_hour   = cmos_getregister(CMOS_REGISTER_HOUR);
 tmnow->ct_day    = cmos_getregister(CMOS_REGISTER_DAY);
 tmnow->ct_month  = cmos_getregister(CMOS_REGISTER_MONTH);
 tmnow->ct_year   = cmos_getregister(CMOS_REGISTER_YEAR);
 if (century_register) centnow = cmos_getregister(century_register);
 for (;;) {
  struct cmostime newtm;
  while (cmos_isupdating());
  newtm.ct_second = cmos_getregister(CMOS_REGISTER_SECOND);
  newtm.ct_minute = cmos_getregister(CMOS_REGISTER_MINUTE);
  newtm.ct_hour   = cmos_getregister(CMOS_REGISTER_HOUR);
  newtm.ct_day    = cmos_getregister(CMOS_REGISTER_DAY);
  newtm.ct_month  = cmos_getregister(CMOS_REGISTER_MONTH);
  newtm.ct_year   = cmos_getregister(CMOS_REGISTER_YEAR);
  if (century_register) newcent = cmos_getregister(century_register);
  if ((tmnow->ct_second == newtm.ct_second)
   && (tmnow->ct_minute == newtm.ct_minute)
   && (tmnow->ct_hour == newtm.ct_hour)
   && (tmnow->ct_day == newtm.ct_day)
   && (tmnow->ct_month == newtm.ct_month)
   && (tmnow->ct_year == newtm.ct_year)
   && (centnow == newcent)) break;
  *tmnow = newtm;
  centnow = newcent;
 }
 if ((cmos_regb&CMOS_REGISTERB_FLAG_NOBCD) == 0) {
  tmnow->ct_second = (tmnow->ct_second&0x0F)+((tmnow->ct_second >> 4)*10);
  tmnow->ct_minute = (tmnow->ct_minute&0x0F)+((tmnow->ct_minute >> 4)*10);
  tmnow->ct_hour = ((tmnow->ct_hour&0x0F)+(((tmnow->ct_hour&0x70) >> 4)*10))|(tmnow->ct_hour&0x80);
  tmnow->ct_day = (tmnow->ct_day&0x0F)+((tmnow->ct_day >> 4)*10);
  tmnow->ct_month = (tmnow->ct_month&0x0F)+((tmnow->ct_month >> 4)*10);
  tmnow->ct_year = (tmnow->ct_year&0x0F)+((tmnow->ct_year >> 4)*10);
  if (century_register != 0) newcent = (newcent&0x0F)+((newcent >> 4)*10);
 }
 if ((cmos_regb&CMOS_REGISTERB_FLAG_24H) == 0 && (tmnow->ct_hour&0x80) != 0) {
  tmnow->ct_hour = ((tmnow->ct_hour&0x7F)+12)%24; /* Fix 12-hour time */
 }
 if (century_register != 0) tmnow->ct_year += newcent*100;
 else {
  /* NOTE: '__DATE_YEAR__' is technically a tpp extension, but
   *       using our custom 'magic.dee' script, we can simulate it.
   *       >> It always expands to an integral constant describing
   *          the current year (at the time of compilation). */
  tmnow->ct_year += (__DATE_YEAR__/100)*100;
  /* If the year register says you're living more than 10 years
   * before this code was compiled, I just assume you're simply
   * from the future and allow you to not recompile this for
   * up to 90 years.
   * - Anyone after that is just a sucker,
   *   'cause I won't be around to blame.
   *   Hehehehehe... */
  if (tmnow->ct_year < (__DATE_YEAR__-10)) tmnow->ct_year += 100;
 }
 return KE_OK;
}


kerrno_t cmos_setnow(struct cmostime const *__restrict tmnow) {
 struct cmostime settm; __u8 setcent = 0;
 kassertobj(tmnow);
 settm = *tmnow;
 settm.ct_year %= 100;
 setcent = tmnow->ct_year/100;
 if ((cmos_regb&CMOS_REGISTERB_FLAG_24H) == 0 && settm.ct_hour > 12) {
  settm.ct_hour = (tmnow->ct_hour%12)|0x80; /* Fix 12-hour time */
 }
 if ((cmos_regb&CMOS_REGISTERB_FLAG_NOBCD) == 0) {
  /* Fu%#ing BCD... */
  settm.ct_second = ((settm.ct_second/10) << 8)+(settm.ct_second%10);
  settm.ct_minute = ((settm.ct_minute/10) << 8)+(settm.ct_minute%10);
  settm.ct_hour   = ((((settm.ct_hour&0x0F)/10) << 8)+((settm.ct_hour&0x70)%10))|(settm.ct_hour&0x80);
  settm.ct_day    = ((settm.ct_day/10) << 8)+(settm.ct_day%10);
  settm.ct_month  = ((settm.ct_month/10) << 8)+(settm.ct_month%10);
  settm.ct_year   = ((settm.ct_year/10) << 8)+(settm.ct_year%10);
  setcent         = ((setcent/10) << 8)+(setcent%10);
 }

 for (;;) {
  while (cmos_isupdating());
  cmos_setregister(CMOS_REGISTER_SECOND,settm.ct_second);
  cmos_setregister(CMOS_REGISTER_MINUTE,settm.ct_minute);
  cmos_setregister(CMOS_REGISTER_HOUR,  settm.ct_hour);
  cmos_setregister(CMOS_REGISTER_DAY,   settm.ct_day);
  cmos_setregister(CMOS_REGISTER_MONTH, settm.ct_month);
  cmos_setregister(CMOS_REGISTER_YEAR,  settm.ct_year);
  if (century_register) cmos_setregister(century_register,setcent);
  struct cmostime updatedtm; __u8 updatedcent;
  /* Continue setting the time until what we read matches it */
  while (cmos_isupdating());
  updatedtm.ct_second = cmos_getregister(CMOS_REGISTER_SECOND);
  updatedtm.ct_minute = cmos_getregister(CMOS_REGISTER_MINUTE);
  updatedtm.ct_hour   = cmos_getregister(CMOS_REGISTER_HOUR);
  updatedtm.ct_day    = cmos_getregister(CMOS_REGISTER_DAY);
  updatedtm.ct_month  = cmos_getregister(CMOS_REGISTER_MONTH);
  updatedtm.ct_year   = cmos_getregister(CMOS_REGISTER_YEAR);
  updatedcent = century_register ? cmos_getregister(century_register) : setcent;
  if ((settm.ct_second == updatedtm.ct_second)
   && (settm.ct_minute == updatedtm.ct_minute)
   && (settm.ct_hour == updatedtm.ct_hour)
   && (settm.ct_day == updatedtm.ct_day)
   && (settm.ct_month == updatedtm.ct_month)
   && (settm.ct_year == updatedtm.ct_year)
   && (setcent == updatedcent)) break;
 }
 return KE_OK;
}

__DECL_END

#ifndef __INTELLISENSE__
#include "time-hpet.c.inl"
#endif


#include <kos/kernel/proc.h>
#include <kos/kernel/syscall.h>
#include <kos/kernel/util/string.h>

__DECL_BEGIN

KSYSCALL_DEFINE1(kerrno_t,ktime_getnow,__user struct timespec *,ptm) {
 struct timespec tmnow;
 kerrno_t error = ktime_getnow(&tmnow);
 if (__likely(KE_ISOK(error)) && 
     __unlikely(copy_to_user(ptm,&tmnow,sizeof(tmnow)))
     ) error = KE_FAULT;
 return error;
}

KSYSCALL_DEFINE1(kerrno_t,ktime_setnow,__user struct timespec *,ptm) {
 kerrno_t error;
 struct timespec tmnow;
 KTASK_CRIT_BEGIN_FIRST
 if __unlikely(copy_from_user(&tmnow,ptm,sizeof(tmnow))) error = KE_FAULT;
 else error = ktime_setnow(&tmnow);
 KTASK_CRIT_END
 return error;
}

__DECL_END


#endif /* !__KOS_KERNEL_TIME_C__ */

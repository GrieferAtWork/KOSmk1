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
#include <kos/kernel/time.h>
#include <kos/kernel/proc.h>
#include <kos/timespec.h>
#include <sys/io.h>
#include <time.h>
#include <stdio.h>

__DECL_BEGIN

kerrno_t ktime_getnow(struct timespec *__restrict tmnow) {
 struct cmostime cmtime;
 kerrno_t error;
 kassertobj(tmnow);
 error = cmos_getnow(&cmtime);
 if (error == KE_OK) {
  tmnow->tv_sec = cmostime_to_time(&cmtime);
  tmnow->tv_nsec = 0;
 }
 return error;
}
kerrno_t ktime_setnow_k(struct timespec const *__restrict tmnow) {
 struct cmostime cmtime;
 kassertobj(tmnow);
 time_to_cmostime(tmnow->tv_sec,&cmtime);
 return cmos_setnow(&cmtime);
}
kerrno_t ktime_setnow(struct timespec const *__restrict tmnow) {
 if __unlikely(!kproc_hassandflag(kproc_self(),KPROCSAND_FLAG_CHTIME)) return KE_ACCES;
 return ktime_setnow_k(tmnow);
}

void ktime_getcpu(struct timespec *__restrict tmcpu) {
 kassertobj(tmcpu);
 // TODO: Required for timeouts used by 'KTASK_STATE_WAITINGTMO'
 tmcpu->tv_nsec = 0;
 tmcpu->tv_sec = 0;
}


void ktime_getnoworcpu(struct timespec *__restrict tmval) {
 if (ktime_getnow(tmval) != KE_OK) ktime_getcpu(tmval);
}






int century_register = 0x00; // Set by ACPI table parsing code if possible
#define cmos_isupdating()       (outb(CMOS_IOPORT_CODE,0x0A),(inb(CMOS_IOPORT_DATA)&0x80)!=0)
#define cmos_getregister(reg)   (outb(CMOS_IOPORT_CODE,reg),inb(CMOS_IOPORT_DATA))
#define cmos_setregister(reg,v) (outb(CMOS_IOPORT_CODE,reg),outb(v,CMOS_IOPORT_DATA))

static __u8 cmos_regb;

void kernel_initialize_cmos(void) {
 cmos_regb = cmos_getregister(CMOS_REGISTER_B);
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
 // Find the appropriate month
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
  tmnow->ct_hour = ((tmnow->ct_hour&0x7F)+12)%24; // Fix 12-hour time
 }
 if (century_register != 0) tmnow->ct_year += newcent*100;
 else {
  // NOTE: '__DATE_YEAR__' is technically a tpp extension, but
  //       using our custom 'magic.dee' script, we can simulate it.
  //       >> It always expands to an integral constant describing
  //          the current year (at the time of compilation).
  tmnow->ct_year += (__DATE_YEAR__/100)*100;
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
  settm.ct_hour = (tmnow->ct_hour%12)|0x80; // Fix 12-hour time
 }
 if ((cmos_regb&CMOS_REGISTERB_FLAG_NOBCD) == 0) {
  // Fu%#ing BCD...
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
  // Continue setting the time until what we read matches it
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

#endif /* !__KOS_KERNEL_TIME_C__ */

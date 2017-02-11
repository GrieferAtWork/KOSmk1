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
#ifndef __TIME_C__
#define __TIME_C__ 1

#include <errno.h>
#include <kos/compiler.h>
#include <kos/task.h>
#include <kos/kernel/debug.h>
#include <kos/fd.h>
#include <kos/time.h>
#include <format-printer.h>
#include <limits.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <utime.h>
#include <unistd.h>

__DECL_BEGIN

__public time_t time(time_t *t) {
 struct timespec result;
 ktime_getnoworcpu(&result);
 if (t) *t = result.tv_sec;
 return result.tv_sec;
}

#ifndef __KERNEL__
static
#endif
time_t const __time_monthstart_yday[2][13] = {
 {0,31,59,90,120,151,181,212,243,273,304,334,365},
 {0,31,60,91,121,152,182,213,244,274,305,335,366}};

__private extern char const abbr_month_names[12][4];
__private extern char const abbr_wday_names[7][4];

__public char *asctime_r(struct tm const *tp,
                         char *__restrict buf) {
#ifdef __KERNEL__
 kassertobj(tp);
 kassertmem(buf,26*sizeof(char));
#else
 if __unlikely(!tp) { __set_errno(EINVAL); return NULL; }
 if __unlikely(tp->tm_year > INT_MAX-1900) { __set_errno(EOVERFLOW); return NULL; }
#endif
 snprintf(buf,26,"%.3s %.3s%3d %.2d:%.2d:%.2d %d\n",
         (tp->tm_wday < 0 || tp->tm_wday >= 7 ? "???" : abbr_wday_names[tp->tm_wday]),
         (tp->tm_mon < 0 || tp->tm_mon >= 12 ? "???" : abbr_month_names[tp->tm_mon]),
          tp->tm_mday,tp->tm_hour,tp->tm_min,tp->tm_sec,tp->tm_year+1900);
 return buf;
}
__public char *ctime_r(time_t const *timep,
                       char *__restrict buf) {
 struct tm ltm;
 return asctime_r(localtime_r(timep,&ltm),buf);
}

__local int tm_calc_isdst(struct tm const *self) {
 // found here: "http://stackoverflow.com/questions/5590429/calculating-daylight-savings-time-from-only-date"
 int previousSunday;
 //January, February, and December are out.
 if (self->tm_mon < 3 || self->tm_mon > 11) { return 0; }
 //April to October are in
 if (self->tm_mon > 3 && self->tm_mon < 11) { return 1; }
 previousSunday = self->tm_mday-self->tm_wday;
 //In march, we are DST if our previous Sunday was on or after the 8th.
 if (self->tm_mon == 3) { return previousSunday >= 8; }
 //In November we must be before the first Sunday to be dst.
 //That means the previous Sunday must be before the 1st.
 return previousSunday <= 0;
}

__public struct tm *gmtime_r(time_t const *timep,
                             struct tm *result) {
 time_t t; int i; time_t const *monthvec;
 kassertobj(timep);
 kassertobj(result);
 t = *timep;
 result->tm_sec  = (int)(t % 60);
 result->tm_min  = (int)((t/60) % 60);
 result->tm_hour = (int)((t/(60*60)) % 24);
 t /= __SECS_PER_DAY;
 t += __time_years2days(__LINUX_TIME_START_YEAR);
 result->tm_wday = (int)(t % 7);
 result->tm_year = (int)__time_days2years(t);
 t -= __time_years2days(result->tm_year);
 result->tm_yday = (int)t;
 monthvec = __time_monthstart_yday[__time_isleapyear(result->tm_year)];
 for (i = 1; i < 12; ++i) if (monthvec[i] >= t) break;
 result->tm_mon  = i-1;
 t -= monthvec[i-1];
 result->tm_mday = t+1;
 result->tm_isdst = tm_calc_isdst(result);
 result->tm_year -= 1900;
 return result;
}
__public struct tm *localtime_r(time_t const *timep,
                                struct tm *result) {
 return gmtime_r(timep,result); // TODO: Timezones 'n $hit.
}
__public extern time_t mktime(struct tm *tm) {
 time_t result;
 kassertobj(tm);
 result = __time_years2days(tm->tm_year>__LINUX_TIME_START_YEAR
                         ? tm->tm_year-__LINUX_TIME_START_YEAR : 0);
 result += tm->tm_yday;
 result *= __SECS_PER_DAY;
 result += tm->tm_hour*60*60;
 result += tm->tm_min*60;
 result += tm->tm_sec;
 return result;
}

__public int nanosleep(struct timespec const *req,
                       struct timespec *rem) {
 struct timespec abstime;
 kerrno_t error;
#ifdef __KERNEL__
 ktime_getnoworcpu(&abstime);
#else
 error = ktime_getnoworcpu(&abstime);
 if __likely(KE_ISOK(error))
#endif
 {
  __timespec_add(&abstime,req);
  error = ktask_abssleep(ktask_self(),&abstime);
 }
 if (error == KE_INTR) {
  *rem = abstime;
#ifdef __KERNEL__
  ktime_getnoworcpu(&abstime);
#else
  if __unlikely(KE_ISERR(ktime_getnoworcpu(&abstime))) goto norem;
#endif
  if __unlikely(__timespec_cmplo(rem,&abstime)) goto norem;
  __timespec_sub(rem,&abstime);
 } else {
norem:
  rem->tv_nsec = 0;
  rem->tv_sec = 0;
 }
 if __unlikely(KE_ISERR(error)) {
  __set_errno(-error);
  return -1;
 }
 return 0;
}


struct ftimebuf {
 char *iter;
 char *end;
};

static int time_printer(char const *s, size_t n, struct ftimebuf *data) {
 char *new_iter;
 n = strnlen(s,n);
 new_iter = data->iter+n;
 if (new_iter >= data->end) return -1;
 memcpy(data->iter,s,n*sizeof(char));
 data->iter = new_iter;
 return 0;
}

__public size_t
strftime(char *__restrict s, size_t max,
         char const *__restrict format,
         struct tm const *__restrict tm) {
 struct ftimebuf buf; buf.end = (buf.iter = s)+max;
 if (format_strftime((pformatprinter)&time_printer,&buf,format,tm)) return 0;
 if (buf.iter == buf.end) return 0;
 *buf.iter = '\0';
 return (size_t)(buf.iter-s);
}

__public double difftime(time_t end, time_t beginning) {
 return (double)end-(double)beginning; /* Eh... duh? */
}



#ifndef __KERNEL__
static char asctimebuffer[26];
static struct tm gmtimebuffer;

__public char *asctime(struct tm const *tm) { return asctime_r(tm,asctimebuffer); }
__public char *ctime(time_t const *timep) { return ctime_r(timep,asctimebuffer); }
__public struct tm *gmtime(time_t const *timep) { return gmtime_r(timep,&gmtimebuffer); }
__public struct tm *localtime(time_t const *timep) { return localtime_r(timep,&gmtimebuffer); }
#endif

#ifndef __KERNEL__
__public int utime(const char *file, struct utimbuf const *file_times) {
 union kinodeattr attrib[2];
 int error,fd  = open(file,O_RDWR);
 if __unlikely(fd == -1) return -1;
 kassertobj(st);
 attrib[0].ia_common.a_id = KATTR_FS_ATIME;
 attrib[1].ia_common.a_id = KATTR_FS_MTIME;
 if (file_times) {
  attrib[0].ia_time.tm_time.tv_sec = file_times->actime;
  attrib[0].ia_time.tm_time.tv_nsec = 0;
  attrib[1].ia_time.tm_time.tv_sec = file_times->modtime;
  attrib[1].ia_time.tm_time.tv_nsec = 0;
 } else {
  error = ktime_getnoworcpu(&attrib[0].ia_time.tm_time);
  if __unlikely(KE_ISERR(error)) {
   __set_errno(-error);
   return -1;
  }
  attrib[1].ia_time.tm_time = attrib[0].ia_time.tm_time;
 }
 error = setattr(fd,KATTR_FS_ATTRS,attrib,sizeof(attrib));
 kfd_close(fd);
 return error;
 return 0;
}
#endif /* !__KERNEL__ */

__DECL_END

#endif /* !__TIME_C__ */

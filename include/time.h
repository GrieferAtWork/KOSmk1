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
#ifndef __TIME_H__
#ifndef _TIME_H
#ifndef _INC_TIME
#define __TIME_H__ 1
#define _TIME_H 1
#define _INC_TIME 1

#include <__null.h>
#include <features.h>
#include <kos/compiler.h>
#ifndef __ASSEMBLY__
#include <kos/types.h>

__DECL_BEGIN

#ifndef __time_t_defined
#define __time_t_defined 1
typedef __time_t time_t;
#endif

#ifndef __timespec_defined
#define __timespec_defined 1
struct timespec {
    __time_t tv_sec;  /*< Seconds. */
    long     tv_nsec; /*< Nanoseconds. */
};
#endif

#ifndef __struct_tm_defined
#define __struct_tm_defined 1
struct tm {
    int tm_sec;   /*< seconds [0,61]. */
    int tm_min;   /*< minutes [0,59]. */
    int tm_hour;  /*< hour [0,23]. */
    int tm_mday;  /*< day of month [1,31]. */
    int tm_mon;   /*< month of year [0,11]. */
    int tm_year;  /*< years since 1900. */
    int tm_wday;  /*< day of week [0,6] (Sunday = 0). */
    int tm_yday;  /*< day of year [0,365]. */
    int tm_isdst; /*< daylight savings flag. */
};
#endif

extern time_t time(time_t *t);

#ifdef __HAVE_REENTRANT
extern __nonnull((1,2)) char *asctime_r __P((struct tm const *__tm, char *__restrict __buf));
extern __nonnull((1,2)) char *ctime_r __P((time_t const *__timep, char *__restrict __buf));
extern __nonnull((1,2)) struct tm *gmtime_r __P((time_t const *__timep, struct tm *__result));
extern __nonnull((1,2)) struct tm *localtime_r __P((time_t const *__timep, struct tm *__result));
#endif
extern __nonnull((1)) time_t mktime __P((struct tm *__tm));

#ifndef __KERNEL__
extern __nonnull((1)) char *asctime __P((struct tm const *__tm));
extern __nonnull((1)) char *ctime __P((time_t const *__timep));
extern __nonnull((1)) struct tm *gmtime __P((time_t const *__timep));
extern __nonnull((1)) struct tm *localtime __P((time_t const *__timep));
#endif


extern __nonnull((1,2)) int nanosleep __P((struct timespec const *__req, struct timespec *__rem));


extern __nonnull((1,3,4)) __attribute_vaformat(__strftime__,3,4)
__size_t strftime __P((char *__restrict __s, __size_t __max,
                       char const *__restrict __format,
                       struct tm const *__restrict __tm));
extern __constcall __wunused double difftime __P((time_t __end, time_t __beginning));


/*
clock_t    clock(void);
int        clock_getres(clockid_t, struct timespec *);
int        clock_gettime(clockid_t, struct timespec *);
int        clock_settime(clockid_t, const struct timespec *);
struct tm *getdate(char const *);
char      *strptime(char const *, char const *, struct tm *);
int        timer_create(clockid_t, struct sigevent *, timer_t *);
int        timer_delete(timer_t);
int        timer_gettime(timer_t, struct itimerspec *);
int        timer_getoverrun(timer_t);
int        timer_settime(timer_t, int, const struct itimerspec *, struct itimerspec *);
void       tzset(void);
*/



#if defined(__KERNEL__) || defined(__TIME_C__)
#define __LINUX_TIME_START_YEAR  1970
// Taken from deemon: <deemon/time.h>
#define __time_days2years(n_days)  ((400*((n_days)+1))/146097)
#define __time_years2days(n_years) (((146097*(n_years))/400)/*-1*/) // rounding error?
#define __time_isleapyear(year) \
 (__builtin_constant_p(year)\
  ? ((year)%400 == 0 || ((year)%100 != 0 && (year)%4 == 0))\
  : __xblock({ __typeof__(year) const __year = (year);\
               __xreturn __year%400 == 0 || (__year%100 != 0 && __year%4 == 0);\
  }))
#define __SECS_PER_DAY    86400
#ifdef __KERNEL__
extern time_t const __time_monthstart_yday[2][13];
#endif /* __KERNEL__ */
#endif

__DECL_END
#endif /* !__ASSEMBLY__ */

#endif /* !_INC_TIME */
#endif /* !_TIME_H */
#endif /* !__TIME_H__ */

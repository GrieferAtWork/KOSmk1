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
#ifndef __KOS_TIMESPEC_H__
#define __KOS_TIMESPEC_H__ 1

#include <kos/compiler.h>
#include <kos/kernel/types.h>
#include <kos/types.h>

__DECL_BEGIN

#ifndef __ASSEMBLY__
#ifndef __timespec_defined
#define __timespec_defined 1
struct timespec {
 __time_t tv_sec;  /*< Seconds. */
 long     tv_nsec; /*< Nanoseconds (0 ... 999.999.999). */
};
#endif /* !__timespec_defined */
#endif /* !__ASSEMBLY__ */

#define __timespec_add(a,b)\
 ((a)->tv_nsec += (b)->tv_nsec,\
  (a)->tv_sec += (a)->tv_nsec/1000000000l,\
  (a)->tv_nsec %= 1000000000l,\
  (a)->tv_sec += (b)->tv_sec)
#define __timespec_sub(a,b)\
 ((a)->tv_sec -= (b)->tv_sec,\
  ((a)->tv_nsec -= (b)->tv_nsec) < 0\
 ? (void)((a)->tv_sec -= (-(a)->tv_nsec)/1000000000l,\
          (a)->tv_nsec = (-(a)->tv_nsec)%1000000000l)\
 : (void)0)

#define __timespec_cmplo(a,b) ((a)->tv_sec < (b)->tv_sec || ((a)->tv_sec == (b)->tv_sec && (a)->tv_nsec < (b)->tv_nsec))
#define __timespec_cmple(a,b) ((a)->tv_sec < (b)->tv_sec || ((a)->tv_sec == (b)->tv_sec && (a)->tv_nsec <= (b)->tv_nsec))
#define __timespec_cmpeq(a,b) ((a)->tv_sec == (b)->tv_sec && (a)->tv_nsec == (b)->tv_nsec)
#define __timespec_cmpne(a,b) ((a)->tv_sec != (b)->tv_sec || (a)->tv_nsec != (b)->tv_nsec)
#define __timespec_cmpgr(a,b) ((a)->tv_sec > (b)->tv_sec || ((a)->tv_sec == (b)->tv_sec && (a)->tv_nsec > (b)->tv_nsec))
#define __timespec_cmpge(a,b) ((a)->tv_sec > (b)->tv_sec || ((a)->tv_sec == (b)->tv_sec && (a)->tv_nsec >= (b)->tv_nsec))


__DECL_END

#endif /* !__KOS_TIMESPEC_H__ */

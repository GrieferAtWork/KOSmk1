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
#ifndef __HW_TIME_CMOS_H__
#define __HW_TIME_CMOS_H__ 1

#include <kos/compiler.h>
#include <kos/types.h>

__DECL_BEGIN

struct cmostime {
 __u8  ct_second; /*< [0..59] Seconds. */
 __u8  ct_minute; /*< [0..59] Minutes. */
 __u8  ct_hour;   /*< [0..23] Hours. */
 __u8  ct_day;    /*< [1..31] Day of month. */
 __u8  ct_month;  /*< [1..12] Month of year. */
 __u32 ct_year;   /*< [0..~~] Year. */
};

#define CMOS_IOPORT_CODE          0x70
#define CMOS_IOPORT_DATA          0x71
#define CMOS_REGISTER_SECOND      0x00
#define CMOS_REGISTER_MINUTE      0x02
#define CMOS_REGISTER_HOUR        0x04
#define CMOS_REGISTER_DAY         0x07
#define CMOS_REGISTER_MONTH       0x08
#define CMOS_REGISTER_YEAR        0x09
#define CMOS_REGISTER_B           0x0B
#define CMOS_REGISTERB_FLAG_NOBCD 0x04 /*< When set, CMOS time is in decimal format. */
#define CMOS_REGISTERB_FLAG_24H   0x02 /*< When set, CMOS hours uses a 24-hour format. */


#ifdef __KERNEL__
extern void kernel_initialize_cmos(void);
extern __wunused __nonnull((1)) __time_t cmostime_to_time(struct cmostime const *__restrict cmtime);
extern __nonnull((2)) void time_to_cmostime(__time_t t, struct cmostime *__restrict cmtime);
extern __nonnull((1)) void cmos_getnow(struct cmostime *__restrict tmnow);
extern __nonnull((1)) void cmos_setnow(struct cmostime const *__restrict tmnow);
#endif

__DECL_END

#endif /* !__HW_TIME_CMOS_H__ */

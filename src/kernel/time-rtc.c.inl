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
#ifndef __KOS_KERNEL_TIME_RTC_C_INL__
#define __KOS_KERNEL_TIME_RTC_C_INL__ 1

#include <kos/compiler.h>
#include <kos/config.h>
#include <kos/kernel/time.h>
#include <kos/kernel/serial.h>
#include <hw/rtc/cmos.h>

__DECL_BEGIN

static kerrno_t rtc_cmos_gettime(__time_t *__restrict result) {
 struct cmostime cmtime;
 cmos_getnow(&cmtime);
 *result = cmostime_to_time(&cmtime);
 return KE_OK;
}
static kerrno_t rtc_cmos_settime(__time_t value) {
 struct cmostime cmtime;
 time_to_cmostime(value,&cmtime);
 cmos_setnow(&cmtime);
 return KE_OK;
}

struct krtc const krtc_cmos = {
 &rtc_cmos_gettime,
 &rtc_cmos_settime,
 "cmos",
};


__DECL_END

#endif /* !__KOS_KERNEL_TIME_RTC_C_INL__ */

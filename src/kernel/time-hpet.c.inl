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
#ifndef __KOS_KERNEL_TIME_HPET_C_INL__
#define __KOS_KERNEL_TIME_HPET_C_INL__ 1

#include <kos/compiler.h>
#include <kos/config.h>
#include <kos/kernel/time.h>
#ifdef __x86__
#include <kos/arch/x86/cpu.h>
#endif

__DECL_BEGIN


static hstamp_t emulated_hp_val = 0;
static hstamp_t emulated_hp_read(void) { return emulated_hp_val; }
static hstamp_t emulated_hp_next(void) { return emulated_hp_val++; }

struct khpet const khpet_emulated = {
 &emulated_hp_next,
 &emulated_hp_read,
 "emulated",
};


#ifdef __x86__
static hstamp_t x86rdtsc_hp_read(void) { return x86_rdtsc(); }
struct khpet const khpet_x86rdtsc = {
 &x86rdtsc_hp_read,
 &x86rdtsc_hp_read,
 "x86.rdtsc",
};
#endif


__DECL_END

#endif /* !__KOS_KERNEL_TIME_HPET_C_INL__ */

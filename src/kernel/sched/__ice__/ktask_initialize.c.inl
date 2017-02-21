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
#ifndef __GOS_KERNEL_KTASK_INITIALIZE_C_INL__
#define __GOS_KERNEL_KTASK_INITIALIZE_C_INL__ 1

#include <gos/compiler.h>
#include <gos/kernel/ktask.h>
#include <gos/kernel/interrupts.h>
#include <gos/kernel/paging.h>
#include <string.h>

__DECL_BEGIN

extern void __irq_switchtask(irq_registers_t *regs);

extern __u8 stack_bottom;
extern __u8 stack_top;

// Initial kernel task descriptor for cpu0 (bios CPU)
ktask_t  __ktask_cpu0;

void kernel_initialize_ktask(void) {
 memset(&__ktask_cpu0.t_regs,0,sizeof(irq_registers_t));
 __ktask_cpu0.t_flags         = KTASK_FLAG_NONE;
 __ktask_cpu0.t_suspended     = 0;
 __ktask_cpu0.t_sched.ts_prev = &__ktask_cpu0;
 __ktask_cpu0.t_sched.ts_next = &__ktask_cpu0;
 __ktask_cpu0.t_pd            = kpage_directory();
 __ktask_cpu0.t_setprio       = KTASK_PRIORITY_DEF;
 __ktask_cpu0.t_prio          = KTASK_PRIORITY_DEF;
 __ktask_cpu0.t_stackbegin    = &stack_bottom;
 __ktask_cpu0.t_stackend      = &stack_top;
 __ktask_cpu0.t_pid           = 0;
 memset(&__ktask_cpu0.t_perms,0,sizeof(kperms_t));
 __ktask_cpu0.t_parent        = 0;
 memset(&__ktask_cpu0.t_children,0,sizeof(struct __ktasklist));
 __ktask_self = &__ktask_cpu0;
 set_irq_handler(32,&__irq_switchtask);
}


__DECL_END

#endif /* !__GOS_KERNEL_KTASK_INITIALIZE_C_INL__ */

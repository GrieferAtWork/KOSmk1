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
#ifndef __KOS_KERNEL_SHM_COW_C_INL__
#define __KOS_KERNEL_SHM_COW_C_INL__ 1

/* Copy-on-write shared memory functionality. */

#include <kos/config.h>
#include <kos/kernel/features.h>
#include <kos/kernel/interrupts.h>
#include <kos/kernel/shm.h>

__DECL_BEGIN

#if KCONFIG_HAVE_SHM_COPY_ON_WRITE
void kshm_pf_handler(struct kirq_registers *__restrict regs) {


 /* Fallback: Call the default IRQ handler. */
 (*kirq_default_handler)(regs);
}

void kernel_initialize_copyonwrite(void) {
 kirq_sethandler(KIRQ_EXCEPTION_PF,&kshm_pf_handler);
}


#endif /* KCONFIG_HAVE_SHM_COPY_ON_WRITE */
              
__DECL_END

#endif /* !__KOS_KERNEL_SHM_COW_C_INL__ */

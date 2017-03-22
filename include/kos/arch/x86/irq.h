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
#ifndef __KOS_ARCH_X86_IRQ_H__
#define __KOS_ARCH_X86_IRQ_H__ 1

#include <kos/compiler.h>
#include <kos/kernel/types.h>
#include <kos/arch/x86/cpu-registers.h>

__DECL_BEGIN

#ifdef __KERNEL__
#ifdef __ASSEMBLY__
#define __karch_raw_irq_enable()  sti
#define __karch_raw_irq_disable() cli
#define __karch_raw_irq_idle()    hlt
#else
#define __karch_raw_irq_enable()    __asm__("sti" : : : "memory")
#define __karch_raw_irq_disable()   __asm__("cli" : : : "memory")
#define __karch_raw_irq_idle()      __asm__("hlt" : : : "memory")
#define __karch_raw_irq_enabled()  (x86_geteflags()&X86_EFLAGS_IF)
#endif

#define karch_irq_enable  __karch_raw_irq_enable
#define karch_irq_disable __karch_raw_irq_disable
#define karch_irq_idle    __karch_raw_irq_idle
#ifdef __karch_raw_irq_enabled
#define karch_irq_enabled __karch_raw_irq_enabled
#endif
#endif

#define KARCH_X64_IRQ_DE 0  /*< Divide-by-zero. */
#define KARCH_X64_IRQ_DB 1  /*< Debug. */
#define KARCH_X64_IRQ_BP 3  /*< Breakpoint. */
#define KARCH_X64_IRQ_OF 4  /*< Overflow. */
#define KARCH_X64_IRQ_BR 5  /*< Bound Range Exceeded. */
#define KARCH_X64_IRQ_UD 6  /*< Invalid Opcode. */
#define KARCH_X64_IRQ_NM 7  /*< Device Not Available. */
#define KARCH_X64_IRQ_DF 8  /*< Double Fault. */
#define KARCH_X64_IRQ_TS 10 /*< Invalid TSS. */
#define KARCH_X64_IRQ_NP 11 /*< Segment Not Present. */
#define KARCH_X64_IRQ_SS 12 /*< Stack-Segment Fault. */
#define KARCH_X64_IRQ_GP 13 /*< General Protection Fault. */
#define KARCH_X64_IRQ_PF 14 /*< Page Fault. */
#define KARCH_X64_IRQ_MF 16 /*< x87 Floating-Point Exception. */
#define KARCH_X64_IRQ_AC 17 /*< Alignment Check. */
#define KARCH_X64_IRQ_MC 18 /*< Machine Check. */
#define KARCH_X64_IRQ_XM 19 /*< SIMD Floating-Point Exception. */
#define KARCH_X64_IRQ_XF KARCH_X64_IRQ_XM
#define KARCH_X64_IRQ_VE 20 /*< Virtualization Exception. */
#define KARCH_X64_IRQ_SX 30 /*< Security Exception. */


__DECL_END

#endif /* !__KOS_ARCH_X86_IRQ_H__ */

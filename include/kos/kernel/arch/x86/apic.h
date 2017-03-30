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
#ifndef __KOS_KERNEL_ARCH_X86_APIC_H__
#define __KOS_KERNEL_ARCH_X86_APIC_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/compiler.h>
#include <kos/types.h>
#include "apicdef.h"

__DECL_BEGIN

/* From linux: "/arch/x86/include/asm/apic.h" */
#define TRAMPOLINE_PHYS_LOW  0x467
#define TRAMPOLINE_PHYS_HIGH 0x469
/* end... */

extern __u32 apic_id;
extern __u32 apic_base; /*< [0..1] Memory-mapped address of local APICs. */
#define APIC_BASE   (apic_base)

__local void apic_writeb(__u32 reg, __u8  v) { *(__u8  volatile *)(APIC_BASE+reg) = v; }
__local void apic_writew(__u32 reg, __u16 v) { *(__u16 volatile *)(APIC_BASE+reg) = v; }
__local void apic_writel(__u32 reg, __u32 v) { *(__u32 volatile *)(APIC_BASE+reg) = v; }
__local __u8  apic_readb(__u32 reg) { return *(__u8  volatile *)(APIC_BASE+reg); }
__local __u16 apic_readw(__u32 reg) { return *(__u16 volatile *)(APIC_BASE+reg); }
__local __u32 apic_readl(__u32 reg) { return *(__u32 volatile *)(APIC_BASE+reg); }

__local __u8 apic_get_icr2_dest_field(void)    { return GET_APIC_DEST_FIELD(apic_readl(APIC_ICR2)); }
__local void apic_set_icr2_dest_field(__u8 id) { apic_writel(APIC_ICR2,(apic_readl(APIC_ICR2)&0x00ffffff)|SET_APIC_DEST_FIELD(id)); }

__local __u32 apic_get_icr(void) { return apic_readl(APIC_ICR)&0xCDFFF; }
__local void apic_set_icr(__u32 bits) { apic_writel(APIC_ICR,(apic_readl(APIC_ICR)&~0xCDFFF)|bits); }



__DECL_END

#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_ARCH_X86_APIC_H__ */

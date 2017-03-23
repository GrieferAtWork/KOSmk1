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
#ifndef __KOS_ARCH_X86_CONTROL_REGISTERS_H__
#define __KOS_ARCH_X86_CONTROL_REGISTERS_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/compiler.h>
#include <kos/types.h>
#include <kos/kernel/features.h>

__DECL_BEGIN

#define X86_EFLAGS_CF           0x00000001 /*< [bit(0)]     Carry Flag Status. */
#define X86_EFLAGS_PF           0x00000004 /*< [bit(2)]     Parity Flag Status. */
#define X86_EFLAGS_AF           0x00000010 /*< [bit(4)]     Auxiliary Carry Flag Status. */
#define X86_EFLAGS_ZF           0x00000040 /*< [bit(6)]     Zero Flag Status. */
#define X86_EFLAGS_SF           0x00000080 /*< [bit(7)]     Sign Flag Status. */
#define X86_EFLAGS_TF           0x00000100 /*< [bit(8)]     Trap Flag (System). */
#if !defined(__KERNEL__) || KCONFIG_HAVE_IRQ
#define X86_EFLAGS_IF           0x00000200 /*< [bit(9)]     Interrupt Enable Flag (System). */
#endif /* KCONFIG_HAVE_IRQ */
#define X86_EFLAGS_DF           0x00000400 /*< [bit(10)]    Direction Flag (Control). */
#define X86_EFLAGS_OF           0x00000800 /*< [bit(11)]    Overflow Flag Status. */
#define X86_EFLAGS_IOPL(n) (((n)&3) << 12) /*< [bit(12,13)] I/O Privilege Level (System). */
#define X86_EFLAGS_NT           0x00004000 /*< [bit(14)]    Nested Task Flag (System). */
#define X86_EFLAGS_RF           0x00010000 /*< [bit(16)]    Resume Flag (System). */
#define X86_EFLAGS_VM           0x00020000 /*< [bit(17)]    Virtual 8086 Mode (System). */
#define X86_EFLAGS_AC           0x00040000 /*< [bit(18)]    Alignment Check (System). */
#define X86_EFLAGS_VIF          0x00080000 /*< [bit(19)]    Virtual Interrupt Flag (System). */
#define X86_EFLAGS_VIP          0x00100000 /*< [bit(20)]    Virtual Interrupt Pending (System). */
#define X86_EFLAGS_ID           0x00200000 /*<	[bit(21)]    ID Flag (System). */

#define X86_CR0_PE 0x00000001 /*< [bit(0)] Protected Mode Enable. */
#define X86_CR0_MP 0x00000002 /*< [bit(1)] Monitor CO-Processor. */
#define X86_CR0_EM 0x00000004 /*< [bit(2)] Emulation. */
#define X86_CR0_TS 0x00000008 /*< [bit(3)] Task Switched. */
#define X86_CR0_ET 0x00000010 /*< [bit(4)] Extension Type. */
#define X86_CR0_NE 0x00000020 /*< [bit(5)] Numeric Error. */
#define X86_CR0_WP 0x00010000 /*< [bit(16)] Write Protect. */
#define X86_CR0_AM 0x00040000 /*< [bit(18)] Alignment Mask. */
#define X86_CR0_NW 0x20000000 /*< [bit(29)] Not-write Through. */
#define X86_CR0_CD 0x40000000 /*< [bit(30)] Cache Disable. */
#define X86_CR0_PG 0x80000000 /*< [bit(31)] Paging. */

#define X86_CR4_VME 	       0x00000001 /*< [bit(0)] Virtual 8086 mode extensions. */
#define X86_CR4_PVI 	       0x00000002 /*< [bit(1)] Protected mode virtual interrupts. */
#define X86_CR4_TSD 	       0x00000004 /*< [bit(2)] Time stamp disable. */
#define X86_CR4_DE 	        0x00000008 /*< [bit(3)] Debugging extensions. */
#define X86_CR4_PSE 	       0x00000010 /*< [bit(4)] Page size extension. */
#define X86_CR4_PAE 	       0x00000020 /*< [bit(5)] Physical address extension. */
#define X86_CR4_MCE 	       0x00000040 /*< [bit(6)] Machine check exception. */
#define X86_CR4_PGE 	       0x00000080 /*< [bit(7)] Page global enable. */
#define X86_CR4_PCE 	       0x00000100 /*< [bit(8)] Performance Monitoring counter enable. */
#define X86_CR4_OSFXSR 	    0x00000200 /*< [bit(9)] OS support for FXSAVE and FXRSTOR Instructions. */
#define X86_CR4_OSXMMEXCPT 	0x00000400 /*< [bit(10)] OS support for unmasked SIMD floating point Exceptions. */
#define X86_CR4_VMXE 	      0x00002000 /*< [bit(13)] Virtual Machine extensions enable. */
#define X86_CR4_SMXE 	      0x00004000 /*< [bit(14)] Safer mode extensions enable. */
#define X86_CR4_PCIDE 	     0x00020000 /*< [bit(17)] PCID enable. */
#define X86_CR4_OSXSAVE 	   0x00040000 /*< [bit(18)] XSAVE and Processor extended states enable. */
#define X86_CR4_SMEP 	      0x00100000 /*< [bit(20)] Supervisor Mode executions Protection enable. */
#define X86_CR4_SMAP 	      0x00200000 /*< [bit(21)] Supervisor Mode access Protection enable. */

#define X86_DR7_L0 0x00000001 /*< [bit(0)] local DR0 breakpoint. */
#define X86_DR7_G0 0x00000002 /*< [bit(1)] global DR0 breakpoint. */
#define X86_DR7_L1 0x00000004 /*< [bit(2)] local DR1 breakpoint. */
#define X86_DR7_G1 0x00000008 /*< [bit(3)] global DR1 breakpoint. */
#define X86_DR7_L2 0x00000010 /*< [bit(4)] local DR2 breakpoint. */
#define X86_DR7_G2 0x00000020 /*< [bit(5)] global DR2 breakpoint. */
#define X86_DR7_L3 0x00000040 /*< [bit(6)] local DR3 breakpoint. */
#define X86_DR7_G3 0x00000080 /*< [bit(7)] global DR3 breakpoint. */
#define X86_DR7_C0 0x00030000 /*< [bit(16..17)] conditions for DR0. */
#define X86_DR7_S0 0x000C0000 /*< [bit(18..19)] size of DR0 breakpoint. */
#define X86_DR7_C1 0x00300000 /*< [bit(20..21)] conditions for DR1. */
#define X86_DR7_S1 0x00C00000 /*< [bit(22..23)] size of DR1 breakpoint. */
#define X86_DR7_C2 0x03000000 /*< [bit(24..25)] conditions for DR2. */
#define X86_DR7_S2 0x0C000000 /*< [bit(26..27)] size of DR2 breakpoint. */
#define X86_DR7_C3 0x30000000 /*< [bit(28..29)] conditions for DR3. */
#define X86_DR7_S3 0xC0000000 /*< [bit(30..31)] size of DR3 breakpoint . */

#ifndef __ASSEMBLY__
#define __GET_REGISTER(name,value) __asm__("mov %%" name ", %0" : "=r" (value))
#define __SET_REGISTER(name,value) __asm__("mov %0, %%" name "" : : "r" (value))

__forcelocal __u32 x86_geteflags(void);
__forcelocal void  x86_seteflags(__u32 __value);
__forcelocal __u32 x86_getcr0(void)        { register __u32 result; __GET_REGISTER("cr0",result); return result; }
__forcelocal void  x86_setcr0(__u32 value) {                        __SET_REGISTER("cr0",value);  }
__forcelocal void *x86_getcr2(void)        { register void *result; __GET_REGISTER("cr2",result); return result; }
__forcelocal void  x86_setcr2(void *value) {                        __SET_REGISTER("cr2",value);  }
__forcelocal void *x86_getcr3(void)        { register void *result; __GET_REGISTER("cr3",result); return result; }
__forcelocal void  x86_setcr3(void *value) {                        __SET_REGISTER("cr3",value);  }
__forcelocal __u32 x86_getcr4(void)        { register __u32 result; __GET_REGISTER("cr4",result); return result; }
__forcelocal void  x86_setcr4(__u32 value) {                        __SET_REGISTER("cr4",value);  }

__forcelocal void *x86_getdr0(void)        { register void *result; __GET_REGISTER("dr0",result); return result; }
__forcelocal void  x86_setdr0(void *value) {                        __SET_REGISTER("dr0",value);  }
__forcelocal void *x86_getdr1(void)        { register void *result; __GET_REGISTER("dr1",result); return result; }
__forcelocal void  x86_setdr1(void *value) {                        __SET_REGISTER("dr1",value);  }
__forcelocal void *x86_getdr2(void)        { register void *result; __GET_REGISTER("dr2",result); return result; }
__forcelocal void  x86_setdr2(void *value) {                        __SET_REGISTER("dr2",value);  }
__forcelocal void *x86_getdr3(void)        { register void *result; __GET_REGISTER("dr3",result); return result; }
__forcelocal void  x86_setdr3(void *value) {                        __SET_REGISTER("dr3",value);  }
__forcelocal __u32 x86_getdr6(void)        { register __u32 result; __GET_REGISTER("dr6",result); return result; }
__forcelocal void  x86_setdr6(__u32 value) {                        __SET_REGISTER("dr6",value);  }
__forcelocal __u32 x86_getdr7(void)        { register __u32 result; __GET_REGISTER("dr7",result); return result; }
__forcelocal void  x86_setdr7(__u32 value) {                        __SET_REGISTER("dr7",value);  }

#undef __SET_REGISTER
#undef __GET_REGISTER

#ifndef __INTELLISENSE__
__forcelocal __u32 x86_geteflags(void) {
 register __u32 __eflags;
 __asm__("pushfl\n"
         "popl %0\n"
         : "=g" (__eflags));
 return __eflags;
}
__forcelocal void x86_seteflags(__u32 __value) {
 __asm__("pushl %0\n"
         "popfl\n"
         : "=g" (__value));
}
#endif
#endif /* !__ASSEMBLY__ */

__DECL_END
#endif /* !__KERNEL__ */

#endif /* !__KOS_ARCH_X86_CONTROL_REGISTERS_H__ */

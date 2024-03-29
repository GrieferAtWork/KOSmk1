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
#ifndef __KOS_KERNEL_ARCH_X86_REALMODE_H__
#define __KOS_KERNEL_ARCH_X86_REALMODE_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/compiler.h>
#ifndef __INTELLISENSE__
#include <kos/kernel/interrupts.h>
#endif

__DECL_BEGIN

#define KCONFIG_HAVE_X86_REALMODE_SEGMENT_REGISTERS 1

#ifdef __ASSEMBLY__
#if KCONFIG_HAVE_X86_REALMODE_SEGMENT_REGISTERS
#   define REALMODE_REGS_SIZE   34
#else
#   define REALMODE_REGS_SIZE   24
#endif
#else
struct __packed realmode_regs {
#if KCONFIG_HAVE_X86_REALMODE_SEGMENT_REGISTERS
 __u16 ds,es,fs,gs,ss;
#endif /* KCONFIG_HAVE_X86_REALMODE_SEGMENT_REGISTERS */
 __u16 di,si,bp,sp;
 /* todo: Might we need eflags here? If so: they can be added... */
union __packed {
 struct __packed { __u32 ebx,edx,ecx,eax; };
 struct __packed { __u16 bx,__hbx;
                   __u16 dx,__hdx;
                   __u16 cx,__hcx;
                   __u16 ax,__hax; };
 struct __packed { __u8  bl,bh,__hbl,__hbh;
                   __u8  dl,dh,__hdl,__hdh;
                   __u8  cl,ch,__hcl,__hch;
                   __u8  al,ah,__hal,__hah; };
};};

#ifdef KCONFIG_HAVE_SINGLECORE
#   define __REALMODE_NOSMP /* nothing */
#else
#   define __REALMODE_NOSMP __nomp
#endif

#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Switch to 16-bit real mode and perform an interrupt
// before switching back to 32-bit protected mode.
// >> Implemented in "/src/kernel/arch/x86/realmode.S"
// @param: intnum: The interrupt number that shall be performed (as in 'int $<intnum>')
// @param: regs: The register state before the when the interrupt instruction is
//               being called on input, and the register state afterwards on output.
// NOTES:
//   - The interrupt-enabled state is preserved across a call to this function. 
//   - While executing, the interrupt vector is temporarily
//     overwritten on the calling CPU, though restore afterwards.
//   - The paging-enabled state is preserved across a call to this function. 
//    (As a matter of fact: all bits from %CR0 are preserved)
//   - The active page directory is _NOT_ 'preserved' and
//     will be set to that of 'kpagedir_kernel()' upon return.
//   - While multithreading-safe across the same CPU, this function
//     is _NOT_ SMP safe and should not invoked without a guard when
//     more than one CPU is online.
extern __REALMODE_NOSMP void realmode_interrupt(__u8 intnum, struct realmode_regs *regs);
#else
extern __REALMODE_NOSMP void __realmode_interrupt(__u8 intnum, struct realmode_regs *regs);
/* Regular interrupts (cli/sti) must be disabled before switching to real mode. */
#define realmode_interrupt(intnum,regs) \
 NOIRQ_EVAL_V(__realmode_interrupt(intnum,regs))
#endif

#undef __REALMODE_NOSMP
#endif /* !__ASSEMBLY__ */

// Real-mode memory configuration.
// These are used by memory initialization to
// make sure that all low-memory used by the real-mode
// interfacing code is marked as in-use and not handed
// out as dynamically available memory.
#define MODE16_RELOC_BASE   0x7C00
#ifdef __KOS_KERNEL_ARCH_X86_REALMODE_S__
#define MODE16_RELOC_SIZE  (mode16_reloc_end-mode16_reloc_begin)
#elif !defined(__ASSEMBLY__)
#define MODE16_RELOC_SIZE  \
 ((size_t)(mode16_reloc_end-mode16_reloc_begin)+\
   sizeof(struct realmode_regs))
extern __u8 const mode16_reloc_begin[];
extern __u8 const mode16_reloc_end[];
#ifdef __MAIN_C__
/* Relocate realmode bootstrap code into a low region of memory. */
extern void kernel_initialize_realmode(void);
#endif
#endif



__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_ARCH_X86_REALMODE_H__ */

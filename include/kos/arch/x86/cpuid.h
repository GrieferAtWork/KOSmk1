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
#ifndef __KOS_ARCH_X86_CPUID_H__
#define __KOS_ARCH_X86_CPUID_H__ 1

#include <kos/compiler.h>
#include <kos/kernel/types.h>
#ifndef __ASSEMBLY__
#include <string.h>
#endif

__DECL_BEGIN

#ifndef __ASSEMBLY__
__local void karch_x86_cpuname(char buf[12]) {
 __asm_volatile__("movl $0, %%eax\n"
                  "cpuid\n"
                  "movl %0, %%eax\n"
                  "movl %%ebx, 0(%%eax)\n"
                  "movl %%edx, 4(%%eax)\n"
                  "movl %%ecx, 8(%%eax)\n"
                  : : "g" (buf)
                  : "eax", "ebx", "ecx", "edx");
}
__local __u32 karch_x86_cpuidmax(void) {
 __u32 result;
 __asm_volatile__("movl $0x80000000, %%eax\n"
                  "cpuid\n"
                  "movl %%eax, %0\n"
                  : "=g" (result) :
                  : "eax", "ebx", "ecx", "edx");
 return result;
}
#endif


// Descriptions are taken from here: http://x86.renejeschke.de/html/file_module_x86_id_45.html
#define KARCH_X86_CPUFEATURE_FPU    0x00000001 /*< [bit(0)]  Floating Point Unit On-Chip. The processor contains an x87 FPU. */
#define KARCH_X86_CPUFEATURE_VME	   0x00000002 /*< [bit(1)]  Virtual 8086 Mode Enhancements. Virtual 8086 mode enhancements, including CR4.VME for controlling the feature, CR4.PVI for protected mode virtual interrupts, software interrupt indirection, expansion of the TSS with the software indirection bitmap, and EFLAGS.VIF and EFLAGS.VIP flags. */
#define KARCH_X86_CPUFEATURE_DE	    0x00000004 /*< [bit(2)]  Debugging Extensions. Support for I/O breakpoints, including CR4.DE for controlling the feature, and optional trapping of accesses to DR4 and DR5. */
#define KARCH_X86_CPUFEATURE_PSE	   0x00000008 /*< [bit(3)]  Page Size Extension. Large pages of size 4 MByte are supported, including CR4.PSE for controlling the feature, the defined dirty bit in PDE (Page Directory Entries), optional reserved bit trapping in CR3, PDEs, and PTEs. */
#define KARCH_X86_CPUFEATURE_TSC	   0x00000010 /*< [bit(4)]  Time Stamp Counter. The RDTSC instruction is supported, including CR4.TSD for controlling privilege. */
#define KARCH_X86_CPUFEATURE_MSR	   0x00000020 /*< [bit(5)]  Model Specific Registers RDMSR and WRMSR Instructions. The RDMSR and WRMSR instructions are supported. Some of the MSRs are implementation dependent. */
#define KARCH_X86_CPUFEATURE_PAE	   0x00000040 /*< [bit(6)]  Physical Address Extension. Physical addresses greater than 32 bits are supported: extended page table entry formats, an extra level in the page translation tables is defined, 2-MByte pages are supported instead of 4 Mbyte pages if PAE bit is 1. The actual number of address bits beyond 32 is not defined, and is implementation specific. */
#define KARCH_X86_CPUFEATURE_MCE	   0x00000080 /*< [bit(7)]  Machine Check Exception. Exception 18 is defined for Machine Checks, including CR4.MCE for controlling the feature. This feature does not define the model-specific implementations of machine-check error logging, reporting, and processor shutdowns. Machine Check exception handlers may have to depend on processor version to do model specific processing of the exception, or test for the presence of the Machine Check feature. */
#define KARCH_X86_CPUFEATURE_CX8	   0x00000100 /*< [bit(8)]  CMPXCHG8B Instruction. The compare-and-exchange 8 bytes (64 bits) instruction is supported (implicitly locked and atomic). */
#define KARCH_X86_CPUFEATURE_APIC	  0x00000200 /*< [bit(9)]  APIC On-Chip. The processor contains an Advanced Programmable Interrupt Controller (APIC), responding to memory mapped commands in the physical address range FFFE0000H to FFFE0FFFH (by default - some processors permit the APIC to be relocated). */
#define KARCH_X86_CPUFEATURE_SEP	   0x00000800 /*< [bit(11)]	SYSENTER and SYSEXIT Instructions. The SYSENTER and SYSEXIT and associated MSRs are supported. */
#define KARCH_X86_CPUFEATURE_MTRR   0x00001000 /*< [bit(12)] Memory Type Range Registers. MTRRs are supported. The MTRRcap MSR contains feature bits that describe what memory types are supported, how many variable MTRRs are supported, and whether fixed MTRRs are supported. */
#define KARCH_X86_CPUFEATURE_PGE	   0x00002000 /*< [bit(13)]	PTE Global Bit. The global bit in page directory entries (PDEs) and page table entries (PTEs) is supported, indicating TLB entries that are common to different processes and need not be flushed. The CR4.PGE bit controls this feature. */
#define KARCH_X86_CPUFEATURE_MCA	   0x00004000 /*< [bit(14)]	Machine Check Architecture. The Machine Check Architecture, which provides a compatible mechanism for error reporting in P6 family, Pentium 4, Intel Xeon processors, and future processors, is supported. The MCG_CAP MSR contains feature bits describing how many banks of error reporting MSRs are supported. */
#define KARCH_X86_CPUFEATURE_CMOV	  0x00008000 /*< [bit(15)]	Conditional Move Instructions. The conditional move instruction CMOV is supported. In addition, if x87 FPU is present as indicated by the CPUID.FPU feature bit, then the FCOMI and FCMOV instructions are supported */
#define KARCH_X86_CPUFEATURE_PAT	   0x00010000 /*< [bit(16)]	Page Attribute Table. Page Attribute Table is supported. This feature augments the Memory Type Range Registers (MTRRs), allowing an operating system to specify attributes of memory on a 4K granularity through a linear address. */
#define KARCH_X86_CPUFEATURE_PSE_36	0x00020000 /*< [bit(17)]	36-Bit Page Size Extension. Extended 4-MByte pages that are capable of addressing physical memory beyond 4 GBytes are supported. This feature indicates that the upper four bits of the physical address of the 4-MByte page is encoded by bits 13-16 of the page directory entry. */
#define KARCH_X86_CPUFEATURE_PSN	   0x00040000 /*< [bit(18)]	Processor Serial Number. The processor supports the 96-bit processor identification number feature and the feature is enabled. */
#define KARCH_X86_CPUFEATURE_CLFSH	 0x00080000 /*< [bit(19)]	CLFLUSH Instruction. CLFLUSH Instruction is supported. */
#define KARCH_X86_CPUFEATURE_DS	    0x00200000 /*< [bit(21)]	Debug Store. The processor supports the ability to write debug information into a memory resident buffer. This feature is used by the branch trace store (BTS) and precise event-based sampling (PEBS) facilities (see Chapter 15, Debugging and Performance Monitoring, in the IA-32 Intel Architecture Software Developer's Manual, Volume 3). */
#define KARCH_X86_CPUFEATURE_ACPI	  0x00400000 /*< [bit(22)]	Thermal Monitor and Software Controlled Clock Facilities. The processor implements internal MSRs that allow processor temperature to be monitored and processor performance to be modulated in predefined duty cycles under software control. */
#define KARCH_X86_CPUFEATURE_MMX	   0x00800000 /*< [bit(23)]	Intel MMX Technology. The processor supports the Intel MMX technology. */
#define KARCH_X86_CPUFEATURE_FXSR	  0x01000000 /*< [bit(24)]	FXSAVE and FXRSTOR Instructions. The FXSAVE and FXRSTOR instructions are supported for fast save and restore of the floating point context. Presence of this bit also indicates that CR4.OSFXSR is available for an operating system to indicate that it supports the FXSAVE and FXRSTOR instructions. */
#define KARCH_X86_CPUFEATURE_SSE	   0x02000000 /*< [bit(25)]	SSE. The processor supports the SSE extensions. */
#define KARCH_X86_CPUFEATURE_SSE2	  0x04000000 /*< [bit(26)]	SSE2. The processor supports the SSE2 extensions. */
#define KARCH_X86_CPUFEATURE_SS	    0x08000000 /*< [bit(27)]	Self Snoop. The processor supports the management of conflicting memory types by performing a snoop of its own cache structure for transactions issued to the bus. */
#define KARCH_X86_CPUFEATURE_HTT	   0x10000000 /*< [bit(28)]	Hyper-Threading Technology. The processor supports Hyper-Threading Technology. */
#define KARCH_X86_CPUFEATURE_TM	    0x20000000 /*< [bit(29)]	Thermal Monitor. The processor implements the thermal monitor automatic thermal control circuitry (TCC). */
#define KARCH_X86_CPUFEATURE_PBE	   0x80000000 /*< [bit(31)]	Pending Break Enable. The processor supports the use of the FERR#/PBE# pin when the processor is in the stop-clock state (STPCLK# is asserted) to signal the processor that an interrupt is pending and that the processor should return to normal operation to handle the interrupt. Bit 10 (PBE enable) in the IA32_MISC_ENABLE MSR enables this capability. */

#ifndef __ASSEMBLY__
__local __u32 karch_x86_cpufeatures(void) {
 __u32 result;
 __asm_volatile__("movl $1, %%eax\n"
                  "cpuid\n"
                  "movl %%edx, %0\n"
                  : "=g" (result) :
                  : "eax", "ebx", "ecx", "edx");
 return result;
}
#endif

#ifndef __ASSEMBLY__
__local void karch_x86_cpubrandname(char buf[48]) {
 if (karch_x86_cpuidmax() >= 0x80000004) {
  __asm_volatile__("pushl %%ebp\n"
                   "movl %0, %%ebp\n"
                   "\n"
                   "movl $0x80000002, %%eax\n"
                   "cpuid\n"
                   "movl %%eax, 0(%%ebp)\n"
                   "movl %%ebx, 4(%%ebp)\n"
                   "movl %%ecx, 8(%%ebp)\n"
                   "movl %%edx, 12(%%ebp)\n"
                   "\n"
                   "movl $0x80000003, %%eax\n"
                   "cpuid\n"
                   "movl %%eax, 16(%%ebp)\n"
                   "movl %%ebx, 20(%%ebp)\n"
                   "movl %%ecx, 24(%%ebp)\n"
                   "movl %%edx, 28(%%ebp)\n"
                   "\n"
                   "movl $0x80000004, %%eax\n"
                   "cpuid\n"
                   "movl %%eax, 32(%%ebp)\n"
                   "movl %%ebx, 36(%%ebp)\n"
                   "movl %%ecx, 40(%%ebp)\n"
                   "movl %%edx, 44(%%ebp)\n"
                   "\n"
                   "popl %%ebp\n"
                   : : "g" (buf)
                   : "eax", "ebx", "ecx", "edx");
 } else {
  strcpy(buf,"Brand String not supported");
 }
}
#endif


__DECL_END

#endif /* !__KOS_ARCH_X86_CPUID_H__ */

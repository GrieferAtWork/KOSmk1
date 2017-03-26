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
#ifndef __KOS_KERNEL_CPU_H__
#define __KOS_KERNEL_CPU_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/compiler.h>
#ifndef __ASSEMBLY__
#include <kos/types.h>
#include <kos/errno.h>

__DECL_BEGIN

/* From linux: "/arch/x86/include/asm/apicdef.h" */
#define IO_APIC_DEFAULT_PHYS_BASE 0xfec00000
#define APIC_DEFAULT_PHYS_BASE    0xfee00000
#define APIC_ID         0x20
#define APIC_LVR        0x30
#define APIC_TASKPRI    0x80
#define APIC_ARBPRI     0x90
#define APIC_PROCPRI    0xA0
#define APIC_EOI        0xB0
#define APIC_RRR        0xC0
#define APIC_LDR        0xD0
#define APIC_DFR        0xE0
#define APIC_SPIV       0xF0
#define APIC_ISR        0x100
#define APIC_ISR_NR     0x8     /* Number of 32 bit ISR registers. */
#define APIC_TMR        0x180
#define APIC_IRR        0x200
#define APIC_ESR        0x280
#define         APIC_ESR_SEND_CS        0x00001
#define         APIC_ESR_RECV_CS        0x00002
#define         APIC_ESR_SEND_ACC       0x00004
#define         APIC_ESR_RECV_ACC       0x00008
#define         APIC_ESR_SENDILL        0x00020
#define         APIC_ESR_RECVILL        0x00040
#define         APIC_ESR_ILLREGA        0x00080
#define         APIC_LVTCMCI    0x2f0
#define APIC_ICR        0x300
#define         APIC_DEST_SELF          0x40000
#define         APIC_DEST_ALLINC        0x80000
#define         APIC_DEST_ALLBUT        0xC0000
#define         APIC_ICR_RR_MASK        0x30000
#define         APIC_ICR_RR_INVALID     0x00000
#define         APIC_ICR_RR_INPROG      0x10000
#define         APIC_ICR_RR_VALID       0x20000
#define         APIC_INT_LEVELTRIG      0x08000
#define         APIC_INT_ASSERT         0x04000
#define         APIC_ICR_BUSY           0x01000
#define         APIC_DEST_LOGICAL       0x00800
#define         APIC_DEST_PHYSICAL      0x00000
#define         APIC_DM_FIXED           0x00000
#define         APIC_DM_FIXED_MASK      0x00700
#define         APIC_DM_LOWEST          0x00100
#define         APIC_DM_SMI             0x00200
#define         APIC_DM_REMRD           0x00300
#define         APIC_DM_NMI             0x00400
#define         APIC_DM_INIT            0x00500
#define         APIC_DM_STARTUP         0x00600
#define         APIC_DM_EXTINT          0x00700
#define         APIC_VECTOR_MASK        0x000FF
#define APIC_ICR2       0x310
#define         GET_APIC_DEST_FIELD(x)  (((x) >> 24) & 0xFF)
#define         SET_APIC_DEST_FIELD(x)  ((__u32)(x) << 24)
#define APIC_LVTT       0x320
#define APIC_LVTTHMR    0x330
#define APIC_LVTPC      0x340
#define APIC_LVT0       0x350
#define APIC_LVT1       0x360
#define APIC_LVTERR     0x370
#define APIC_TMICT      0x380
#define APIC_TMCCT      0x390
#define APIC_TDCR       0x3E0
#define APIC_SELF_IPI   0x3F0
#define APIC_EFEAT      0x400
#define APIC_ECTRL      0x410
#define APIC_EILVTn(n)  (0x500 + 0x10 * n)
/* From linux: "/arch/x86/include/asm/apic.h" */
#define TRAMPOLINE_PHYS_LOW  0x467
#define TRAMPOLINE_PHYS_HIGH 0x469
/* end... */

#define APICVER_82489DX    0x00
#define APICVER_INTEGRATED 0x10


struct __packed mpfps { /* mp_floating_pointer_structure */
 char  mp_sig[4];   /*< == "_MP_". */
 __u32 mp_cfgtab;   /*< [0..1] Physical address of the configuration table. */
 __u8  mp_length;   /*< Size of this structure (in 16-byte increments). */
 __u8  mp_specrev;  /*< MP version. */
 __u8  mp_chksum;   /*< memsum()-alignment to ZERO. */
 __u8  mp_defcfg;   /*< If this is not zero then configuration_table should be 
                     *  ignored and a default configuration should be loaded instead. */
 __u32 mp_features; /*< If bit 7 is then the IMCR is present and PIC mode is being used,
                     *  otherwise virtual wire mode is; all other bits are reserved. */
};

struct __packed mpcfgtab { /* mp_configuration_table */
 char  tab_sig[4];         /*< == "PCMP". */
 __u16 tab_length;         /*< Length of this header + following vector. */
 __u8  tab_specrev;        /*< Specifications revision. */
 __u8  tab_chksum;         /*< memsum()-alignment to ZERO. */
 char  tab_oemid[8];       /*< OEM ID (Padded with spaces). */
 char  tab_productid[12];  /*< Product ID (Padded with spaces). */
 __u32 tab_oemtab;         /*< [0..1] Address of the OEM table. */
 __u16 tab_oemtabsize;     /*< Size of the OEM table. */
 __u16 tab_entryc;         /*< Amount of entires following this configuration table. */
 __u32 tab_lapicaddr;      /*< Memory-mapped address of local APICs. */
 __u16 tab_extab_length;   /*< Extended table length. */
 __u8  tab_extab_checksum; /*< Extended table checksum. */
 __u8  tab_reserved;       /*< Reserved... */
 /* Inlined vector of MP entires, containing 'tab_entryc'
  * entires in 'tab_length-sizeof(struct mpcfgtab)' bytes. */
};

#define	MPCFG_PROCESSOR	0
#define	MPCFG_BUS		     1
#define	MPCFG_IOAPIC	   2
#define	MPCFG_INT_IO	   3
#define	MPCFG_INT_LOCAL	4
struct __packed mpcfg_common {
 __u8  cc_type;	/*< Entry Type (One of 'MPCFG_*'). */
};

struct __packed mpcfg_processor {
 __u8  cp_type;	    /*< [== MPCFG_LINT] Entry Type. */
 __u8  cp_lapicid;	 /*< Local APIC ID number. */
 __u8  cp_lapicver; /*< APIC versions (APICVER_*). */
#define MP_PROCESSOR_ENABLED		     0x01	/*< Processor is available. */
#define MP_PROCESSOR_BOOTPROCESSOR	0x02	/*< This is the boot processor (The one you're probably running on right now). */
 __u8  cp_cpuflag;	 /*< Set of 'MP_PROCESSOR_*'. */
 __u32 cp_cpusig;	  /*< Processor Type signature. */
 __u32 cp_features; /*< CPUID feature value. */
 __u32 cp_reserved[2];
};

struct __packed mpcfg_bus {
 __u8 cb_type;	      /*< [== MPCFG_BUS] Entry Type. */
 __u8 cb_busid;	     /*< ID number for this bus. */
 char cb_bustype[6];	/*< Identifier string. */
};

struct __packed mpcfg_ioapic {
 __u8  cio_type;	    /*< [== MPCFG_IOAPIC] Entry Type. */
 __u8  cio_apicid;	  /*< ID of this I/O APIC. */
 __u8  cio_apicver;	 /*< This I/O APIC's version number (APICVER_*). */
#define MP_IOAPIC_ENABLED	0x01
 __u8  cio_flags;	   /*< Set of 'MP_IOAPIC_*'. */
 __u32 cio_apicaddr;	/*< Physical address of this I/O APIC. */
};

struct __packed mpcfg_int {
 __u8  ci_type;	     /*< [== MPCFG_INT_IO|MPCFG_INT_LOCAL] Entry Type. */
 __u8  ci_irqtype;	  /*< One of 'MP_INT_IRQTYPE_*' */
 __u16 ci_irqflag;	  /*< Or'd together 'MP_INT_IRQPOL' and 'MP_INT_IRQTRIGER'. */
 __u8  ci_srcbus;	   /*< Source bus ID number. */
 __u8  ci_srcbusirq;	/*< Source bus IRQ signal number. */
 __u8  ci_dstapic;	  /*< ID number of the connected I/O APIC, or 0xff for all. */
 __u8  ci_dstirq;	   /*< IRQ pin number to which the signal is connected. */
};

#define MP_INT_IRQTYPE_INT        0
#define MP_INT_IRQTYPE_NMI        1
#define MP_INT_IRQTYPE_SMI        2
#define MP_INT_IRQTYPE_EXTINT     3

#define MP_INT_IRQPOL             0x03
#define MP_INT_IRQPOL_DEFAULT     0x00
#define MP_INT_IRQPOL_HIGH        0x01
#define MP_INT_IRQPOL_RESERVED    0x02
#define MP_INT_IRQPOL_LOW         0x03

#define MP_INT_IRQTRIGER          0x0C
#define MP_INT_IRQTRIGER_DEFAULT  0x00
#define MP_INT_IRQTRIGER_EDGE     0x04
#define MP_INT_IRQTRIGER_RESERVED 0x08
#define MP_INT_IRQTRIGER_LEVEL    0x0C


union mpcfg {
 struct mpcfg_common    mc_common;
 struct mpcfg_processor mc_processor;
 struct mpcfg_bus       mc_bus;
 struct mpcfg_ioapic    mc_ioapic;
 struct mpcfg_int       mc_int;
};

/* Max amount of CPUs that may be used. */
#define KCPU_MAXCOUNT   16

#define CPUID_INVALID  (KCPU_MAXCOUNT)
typedef __u8 cpuid_t;    /*< LAPIC/CPU ID number. */
typedef __u8 cpustate_t; /*< Set of 'CPUSTATE_*' */
#define CPUSTATE_NONE      0x00
#define CPUSTATE_AVAILABLE 0x01 /*< This CPU is available for use. */
#define CPUSTATE_STARTED   0x10 /*< Set by CPU init code (Used to confirm an intended startup) */
#define CPUSTATE_RUNNING   0x20 /*< Must be set by bootstrap code (Used to confirm a successful start sequence) */

struct kprocessor {
union __packed {
 cpuid_t    p_id;	           /*< Processor id (kprocessor_v vector index). */
 cpuid_t    p_lapic_id;	     /*< LAPIC id. */
};
 __u8       p_lapic_version; /*< LAPIC version (s.a.: 'APICVER_*'). */
 cpustate_t p_state;         /*< CPU State. */
 __u8       p_padding;       /*< Padding... */
#ifdef __x86__
 __u32      p_x86_features;  /*< Processor features (Set of 'X86_CPUFEATURE_*'). */
#endif
};

extern __size_t           kprocessor_c;                 /*< [const] Amount of available processors. */
extern struct kprocessor  kprocessor_v[KCPU_MAXCOUNT];  /*< [const] Global processor control vector. */
extern struct kprocessor *kprocessor_boot;              /*< [1..1][const] Control entry for the boot processor. */
extern __u32              lapic_addr;                   /*< [0..1] Memory-mapped address of local APICs. */



#define CPUINIT_IPI_ATTEMPTS             2    /*< Amount of times init should be attempted through IPI. */
#define CPUINIT_IPI_TIMEOUT              10   /*< Timeout when waiting for 'APIC_ICR_BUSY' to go away after IPI init. */
#define CPUINIT_ACKNOWLEDGE_SEND_TIMEOUT 1000 /*< Timeout in peer CPU during init to wait for startup intention acknowledgement. */
#define CPUINIT_ACKNOWLEDGE_RECV_TIMEOUT 100  /*< Timeout in host CPU during init to wait for startup confirmation. */

__local void apic_writeb(__u32 reg, __u8  v) { *(volatile __u8  *)(lapic_addr+reg) = v; }
__local void apic_writew(__u32 reg, __u16 v) { *(volatile __u16 *)(lapic_addr+reg) = v; }
__local void apic_writel(__u32 reg, __u32 v) { *(volatile __u32 *)(lapic_addr+reg) = v; }
__local __u8  apic_readb(__u32 reg) { return *(volatile __u8  *)(lapic_addr+reg); }
__local __u16 apic_readw(__u32 reg) { return *(volatile __u16 *)(lapic_addr+reg); }
__local __u32 apic_readl(__u32 reg) { return *(volatile __u32 *)(lapic_addr+reg); }

/* Get/Set the CPU that should receive interrupts. */
__local cpuid_t apic_getirqdst(void)    { return GET_APIC_DEST_FIELD(apic_readl(APIC_ICR2)); }
__local void apic_setirqdst(cpuid_t id) { apic_writel(APIC_ICR2,(apic_readl(APIC_ICR2)&0x00ffffff)|SET_APIC_DEST_FIELD(id)); }

__local __u32 apic_geticr(void)      { return apic_readl(APIC_ICR)&0xCDFFF; }
__local void apic_seticr(__u32 bits) { apic_writel(APIC_ICR,(apic_readl(APIC_ICR)&~0xCDFFF)|bits); }

//////////////////////////////////////////////////////////////////////////
// Search for additional CPUs the system might offer
extern void kernel_initialize_cpu(void);


//////////////////////////////////////////////////////////////////////////
// Start a given cpu at the specified EIP address.
// WARNING: All other registers are either undefined arch-specific, or undefined
// WARNING: If the given CPU was already running, it will be reset.
// NOTE: The caller is responsible to pass a valid, available CPU id.
// @return: KE_OK:       Successfully started the given CPU.
// @return: KE_DEVICE:   The device didn't respond to the INIT command.
// @return: KE_PERM:     The CPU didn't accept the INIT request.
// @return: KE_TIMEDOUT: Code at the given START_EIP was either
//                       not executed, or failed to acknowledge
//                       'CPUSTATE_STARTED' with 'CPUSTATE_RUNNING'.
extern kerrno_t cpu_start(cpuid_t id, __u32 start_eip);
#define CPU_START_EIPBITS  20

//////////////////////////////////////////////////////////////////////////
// Stop execution on the calling CPU (This function never returns).
extern __noreturn void cpu_stopme(void);

//////////////////////////////////////////////////////////////////////////
// Acknowledge a successful CPU startup.
extern void cpu_acknowledge(void);

//////////////////////////////////////////////////////////////////////////
// Returns the ID of a suspended CPU, or 'CPUID_INVALID' if none are
// @return: * :            ID of the first unused CPU.
// @return: CPUID_INVALID: No available CPU is unused.
extern __wunused cpuid_t cpu_unused(void);

//////////////////////////////////////////////////////////////////////////
// Returns the ID of the calling CPU
__local __wunused cpuid_t cpu_self(void) { return 1; /* TODO */ }

__DECL_END
#endif /* !__ASSEMBLY__ */
#endif /* __KERNEL__ */


#endif /* !__KOS_KERNEL_CPU_H__ */

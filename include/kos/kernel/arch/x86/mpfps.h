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
#ifndef __KOS_KERNEL_ARCH_X86_MPFPS_H__
#define __KOS_KERNEL_ARCH_X86_MPFPS_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/compiler.h>
#include <kos/types.h>

__DECL_BEGIN

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

__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_ARCH_X86_MPFPS_H__ */

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
#ifndef __KOS_KERNEL_CPU_C__
#define __KOS_KERNEL_CPU_C__ 1

#include <assert.h>
#include <hw/rtc/cmos.h>
#include <hw/rtc/i8253.h>
#include <kos/arch/x86/cpuid.h>
#include <kos/atomic.h>
#include <kos/compiler.h>
#include <kos/config.h>
#include <kos/kernel/arch/x86/apic.h>
#include <kos/kernel/arch/x86/apicdef.h>
#include <kos/kernel/arch/x86/mpfps.h>
#include <kos/kernel/cpu.h>
#include <kos/kernel/interrupts.h>
#include <kos/kernel/pageframe.h>
#include <kos/syslog.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <sys/io.h>
#include <sys/types.h>

__DECL_BEGIN

size_t        kcpu2_c                = 0;
struct kcpu2  kcpu2_v[KCPU_MAXCOUNT] = {{{0}},};
struct kcpu2 *kcpu2_boot             = NULL;
__u32         apic_base              = 0;
__u32         apic_id                = 0;


static void smpinit_onecore(void); /*< Fallback initialization if no SMP info was found. */
static void smpinit_default(__u8 cfg);
static void smpinit_tab(struct mpcfgtab const *__restrict cfgtab);



static struct mpfps *
mpfps_locate_at(void *base, size_t bytes) {
#define MP_ALIGN 16
 static char const sig[] = {'_','M','P','_'};
 struct mpfps *result = NULL;
 uintptr_t iter,end;
 end = (iter = (uintptr_t)base)+bytes;
 /* Clamp the search area to a 16-byte alignment. */
 iter = align(iter,MP_ALIGN),end = alignd(end,MP_ALIGN);
 if (iter < end) for (; iter != end; iter += MP_ALIGN) {
  /* Search for the mp_sig (this search is fairly cheap because it's 16-byte aligned). */
  result = (struct mpfps *)memidx((void *)iter,(size_t)(end-iter),
                                  sig,sizeof(sig),MP_ALIGN);
  if (!result) break; /* No more candidates. */
  k_syslogf(KLOG_DEBUG,"[SMP] MPFPS candidate found at %p\n",result);
  /* When found, check the length and checksum. */
  if (result->mp_length == sizeof(struct mpfps)/16 &&
     !memsum(result,sizeof(struct mpfps))) break;
  /* Continue searching past this candidate (MP_ALIGN will be added above) */
  iter = (uintptr_t)result;
 }
 return result;
}


static struct mpfps *mpfps_locate(void) {
 struct mpfps *result;
              result = mpfps_locate_at((void *)(uintptr_t)*(__u16 volatile *)0x40E,1024);
 if (!result) result = mpfps_locate_at((void *)(uintptr_t)((*(__u16 volatile *)0x413)*1024),1024);
 if (!result) result = mpfps_locate_at((void *)(uintptr_t)0x0F0000,64*1024);
 return result;
}



static void smpinit_onecore(void) {
 k_syslogf(KLOG_INFO,"[SMP] No special SMP information found (Configuring for 1 core)\n");
 kcpu2_c             = 1;
 kcpu2_boot          = kcpu2_v;
 kcpu2_v->p_lapic_id = cpu_self();
#ifdef __x86__
 kcpu2_v->p_x86_features = x86_cpufeatures();
#endif
 apic_base = 0;
}

static void smpinit_default(__u8 cfg) {
 __u8 boot_apic_id;
 struct kcpu2 *primary,*secondary;
 k_syslogf(KLOG_INFO,"[SMP] Default configuration: %I8x\n",cfg);
 apic_base    = APIC_DEFAULT_PHYS_BASE;
 boot_apic_id = cpu_self();
 kcpu2_c      = 2;
 primary      = kcpu2_v+(boot_apic_id);
 secondary    = kcpu2_v+(3-boot_apic_id);
 kcpu2_boot   = primary;

 primary->p_lapic_id   = boot_apic_id;
 secondary->p_lapic_id = 3-boot_apic_id;
#ifdef __x86__
 primary->p_x86_features =
 secondary->p_x86_features = x86_cpufeatures();
#endif

 /* TODO: This is incomplete!. */
 if (cfg > 4) {
  /* Bus #1 is a PCI bus. */
  primary->p_lapic_version = 
  secondary->p_lapic_version = APICVER_INTEGRATED;
 } else {
  primary->p_lapic_version = 
  secondary->p_lapic_version = APICVER_82489DX;
 }
}


static void smpinit_tab(struct mpcfgtab const *__restrict cfgtab) {
 union mpcfg *iter,*end; size_t entryc;
 k_syslogf(KLOG_INFO,"[SMP] CFG Table at %p (v1.%I8u) (%.?q %.?q)\n",
           cfgtab,cfgtab->tab_specrev,
           memlen(cfgtab->tab_oemid,' ',8),cfgtab->tab_oemid,
           memlen(cfgtab->tab_productid,' ',12),cfgtab->tab_productid);
 iter       = (union mpcfg *)(cfgtab+1);
 end        = (union mpcfg *)((uintptr_t)cfgtab+cfgtab->tab_length);
 entryc     = (size_t)cfgtab->tab_entryc;
 kcpu2_c    = 0;
 kcpu2_boot = NULL;
 apic_base  = cfgtab->tab_lapicaddr;
 for (; iter < end && entryc; --entryc) {
  switch (iter->mc_common.cc_type) {
   case MPCFG_PROCESSOR:
    if (iter->mc_processor.cp_cpuflag & MP_PROCESSOR_ENABLED) {
     if (iter->mc_processor.cp_lapicid >= KCPU_MAXCOUNT) {
      k_syslogf(KLOG_ERROR,"[SMP] Cannot use additional processor (LAPICID %I8u): "
                           "LAPICID is too great\n",
                iter->mc_processor.cp_lapicid);
     } else if (kcpu2_c < KCPU_MAXCOUNT) {
      struct kcpu2 *proc = kcpu2_v+iter->mc_processor.cp_lapicid;
      k_syslogf(KLOG_INFO,"[SMP] Found CPU (LAPICID %I8u)\n",
                iter->mc_processor.cp_lapicid);
      proc->p_lapic_id      = iter->mc_processor.cp_lapicid;
      proc->p_x86_features  = iter->mc_processor.cp_features;
      proc->p_lapic_version = iter->mc_processor.cp_lapicver;
      proc->p_state         = CPUSTATE_AVAILABLE;
      if (iter->mc_processor.cp_cpuflag & MP_PROCESSOR_BOOTPROCESSOR) {
       if (kcpu2_boot) {
        k_syslogf(KLOG_ERROR,"[SMP] Ignoring second boot processor specification (LAPICID %I8u)\n",
                  iter->mc_processor.cp_lapicid);
       } else {
        kcpu2_boot = proc;
       }
      }
      ++kcpu2_c;
     } else {
      k_syslogf(KLOG_ERROR,"[SMP] Cannot use additional processor (LAPICID %I8u): "
                           "Too many others already found\n",
                iter->mc_processor.cp_lapicid);
     }
    }
    *(uintptr_t *)&iter += sizeof(struct mpcfg_processor);
    break;
   case MPCFG_BUS:
    /* TODO: Do we need this? */
    *(uintptr_t *)&iter += sizeof(struct mpcfg_bus);
    break;
   case MPCFG_IOAPIC:
    /* TODO: Do we need this? */
    *(uintptr_t *)&iter += sizeof(struct mpcfg_ioapic);
    break;
   case MPCFG_INT_IO:
   case MPCFG_INT_LOCAL:
    /* TODO: Do we need this? */
    *(uintptr_t *)&iter += sizeof(struct mpcfg_int);
    break;
   default:
    k_syslogf(KLOG_ERROR,"[SMP] Unrecognized entry type %I8u (%I8x) in configuration table at index %Iu/%Iu, offset %Iu/%Iu\n",
              iter->mc_common.cc_type,iter->mc_common.cc_type,
             (size_t)cfgtab->tab_entryc-entryc,cfgtab->tab_entryc,
              iter-(union mpcfg *)(cfgtab+1),end-(union mpcfg *)(cfgtab+1));
    goto done;
  }
 }
done:;
 /* Fix broken configurations. */
 if (!kcpu2_c) {
  k_syslogf(KLOG_ERROR,"[SMP] Failed to find any processor entries in the MPCFG table (Reverting single-core)\n");
  kcpu2_c    = 1;
  kcpu2_boot = kcpu2_v;
 }
 if (!kcpu2_boot) {
  kcpu2_boot = &kcpu2_v[0];
  k_syslogf(KLOG_ERROR,"[SMP] Failed to locate the boot processor (Assuming ID 0; LAPICID %Iu)\n",
            kcpu2_boot->p_lapic_id);
 }
}


extern __u8 apic_cpu_self_begin[];
extern __u8 apic_cpu_self_end[];
extern __u8 onecore_cpu_self_begin[];
extern __u8 onecore_cpu_self_end[];

void kernel_initialize_cpu(void) {
 struct mpfps *f = mpfps_locate();
 memset(kcpu2_v,0,KCPU_MAXCOUNT*sizeof(struct kcpu2));
 kcpu2_boot = NULL;
 apic_base       = 0;
 kcpu2_c    = 0;
 if (f) {
  k_syslogf(KLOG_INFO,"[SMP] MPFPS structure at %p (v1.%I8u)\n",
            f,f->mp_specrev);
  if (f->mp_defcfg) {
   /* Default configuration. */
   smpinit_default(f->mp_defcfg);
  } else {
   struct mpcfgtab *cfgtab = (struct mpcfgtab *)(uintptr_t)f->mp_cfgtab;
   /* Validate the configuration table. */
   if (cfgtab && cfgtab->tab_length >= sizeof(struct mpcfgtab) &&
      !memsum(cfgtab,cfgtab->tab_length) && !memcmp(cfgtab->tab_sig,"PCMP",4)) {
    /* Since this function may allocate dynamic memory, we need
     * to protect the volatile cfgtab from be re-allocated. */
    struct kpageguard pg;
    kpageguard_init(&pg,cfgtab,cfgtab->tab_length);
    smpinit_tab(cfgtab);
    kpageguard_quit(&pg);
   } else {
    k_syslogf(KLOG_ERROR,"[SMP] MP Configuration table at %p is corrupted\n",cfgtab);
    goto nosmp;
   }
  }
  goto done;
 }
 /* Fallback: Configure as one core. */
nosmp:
 smpinit_onecore();
done:
 assert(kcpu2_c >= 1);
 assert(kcpu2_boot >= kcpu2_v &&
        kcpu2_boot <  kcpu2_v+KCPU_MAXCOUNT);
 kcpu2_boot->p_state = CPUSTATE_AVAILABLE|CPUSTATE_STARTED|CPUSTATE_RUNNING;
 if (apic_base) {
  apic_id = apic_base+APIC_ID;
 } else {
  assert((apic_cpu_self_end-apic_cpu_self_begin) >=
         (onecore_cpu_self_end-onecore_cpu_self_begin));
  /* Runtime-relocation: Use a stub function always returning 0 instead. */
  memcpy(apic_cpu_self_begin,onecore_cpu_self_begin,
        (size_t)(onecore_cpu_self_end-onecore_cpu_self_begin));
 }
}

__asm__(".global cpu_self\n"
        "cpu_self:\n"
        "apic_cpu_self_begin:\n"
        "    mov apic_id, %eax\n"
        "    mov (%eax),  %eax\n"
        "    shr $24,     %eax\n"
        "    and $0x0f,   %eax\n"
        "    ret\n"
        "apic_cpu_self_end:\n");
__asm__("onecore_cpu_self_begin:\n"
        "    xor %eax, %eax\n"
        "    ret\n"
        "onecore_cpu_self_end:\n");



cpuid_t cpu_unused(void) {
 struct kcpu2 *iter,*end;
 end = (iter = kcpu2_v)+KCPU_MAXCOUNT;
 for (; iter != end; ++iter) {
  assertf(iter->p_id == ((iter->p_state&CPUSTATE_AVAILABLE)
          ? (cpuid_t)(iter-kcpu2_v) : 0),
          "iter->p_id        = %I8u\n"
          "iter-kcpu2_v = %Iu\n"
         ,iter->p_id,iter-kcpu2_v);
  if ((iter->p_state&(CPUSTATE_AVAILABLE|CPUSTATE_STARTED)) ==
                     (CPUSTATE_AVAILABLE)
      ) return iter->p_id;
 }
 return CPUID_INVALID;
}


static __atomic int cpu_init_lock = 0;
kerrno_t cpu_start(cpuid_t id, __u32 start_eip) {
 kerrno_t error;
 NOIRQ_BEGIN
 while (katomic_xch(cpu_init_lock,1));
 error = cpu_start_unlocked(id,start_eip);
 cpu_init_lock = 0;
 __asm_volatile__("sfence\n" : : : "memory");
 NOIRQ_END
 return error;
}
kerrno_t cpu_start_unlocked(cpuid_t id, __u32 start_eip) {
 int need_startup_ipi; kerrno_t error; struct kcpu2 *cpu;
 assert(!karch_irq_enabled());
 assertf(start_eip < ((__u32)1 << CPU_START_EIPBITS),"The given EIP %p is too large",start_eip);
 assertf(id < KCPU_MAXCOUNT,"Invalid CPU id (ID: %I8u)",id);
 cpu = &kcpu2_v[id];
 assertf(cpu->p_state&CPUSTATE_AVAILABLE,"Inactive CPU id %I8u\n",id);

 /* Set BIOS shutdown code to warm start. */
 outb(CMOS_IOPORT_CODE,0xf);
 outb(CMOS_IOPORT_DATA,0xa);

 /* Set the reset vector address. */
 *(__u16 volatile *)TRAMPOLINE_PHYS_HIGH = (__u16)(start_eip >> 4);
 *(__u16 volatile *)TRAMPOLINE_PHYS_LOW  = (__u16)(start_eip & 0xf);

 need_startup_ipi = ((kcpu2_v[id].p_lapic_version & 0xf0) != APICVER_82489DX);

 if (need_startup_ipi) {
  /* Clear APIC errors. */
  apic_writel(APIC_ESR,0);
  apic_readl(APIC_ESR);
 }

 cpu->p_state = CPUSTATE_AVAILABLE;

 /* Turn INIT on. */
 apic_set_icr2_dest_field(id);
 apic_set_icr(APIC_INT_LEVELTRIG|APIC_INT_ASSERT|APIC_DM_INIT);

 i8253_delay10ms();
 i8253_delay10ms();

 /* De-assert INIT. */
 apic_set_icr2_dest_field(id);
 apic_set_icr(APIC_INT_LEVELTRIG|APIC_DM_INIT);

 if (need_startup_ipi) {
  __u32 acc_state = 0;
  int n = CPUINIT_IPI_ATTEMPTS,timeout;
send_again:
  do {
   k_syslogf(KLOG_DEBUG,"[SMP] Sending startup IPI #%d\n",3-n);
   apic_writel(APIC_ESR,0);
   apic_set_icr2_dest_field(id);
   apic_set_icr(APIC_DM_STARTUP|((start_eip >> 12) & APIC_VECTOR_MASK));
   k_syslogf(KLOG_DEBUG,"[SMP] Waiting for send to finish");
   timeout = CPUINIT_IPI_TIMEOUT+1;
   for (;;) {
    i8253_delay10ms();
    k_syslogf(KLOG_DEBUG,".");
    if (!(apic_readl(APIC_ICR)&APIC_ICR_BUSY)) break;
    if (!--timeout) { k_syslogf(KLOG_DEBUG," (Timed out)\n"); goto send_again; }
   }
   k_syslogf(KLOG_DEBUG," (OK)\n");
   /* Give the other CPU some time to accept the IPI. */
   i8253_delay10ms();
   acc_state = (apic_readl(APIC_ESR) & 0xEF);
   if __likely(!acc_state) goto ipi_initialized;
  } while (--n);
  /* An error occurred. */
  if (acc_state) {
   /* The command wasn't accepted. */
   k_syslogf(KLOG_ERROR,"CPU %I8u didn't accept startup command (ESR = %I8x)\n",id,acc_state);
   error = KE_PERM;
  } else {
   error = KE_DEVICE;
  }
  goto end;
 } else {
  /* No startup IPI required. */
 }
ipi_initialized:
 /* Wait for the INIT to be acknowledged.
  * NOTE: The race condition between here and when we actually
  *       started the CPU is solved by a timeout in cpu_acknowledge(). */
 cpu->p_state |= CPUSTATE_STARTED;
 {
  int timeout = CPUINIT_ACKNOWLEDGE_RECV_TIMEOUT+1;
  cpustate_t oldstate;
  while (--timeout) {
   if (cpu->p_state&CPUSTATE_RUNNING) goto endok; /* CPU is now running! */
   i8253_delay10ms();
  }
  do {
   oldstate = cpu->p_state;
   if (oldstate&CPUSTATE_RUNNING) goto endok; /* That was a close one... */
  } while (!katomic_cmpxch(cpu->p_state,oldstate,CPUSTATE_AVAILABLE));
  k_syslogf(KLOG_ERROR,"[SMP] CPU %I8u failed to acknowledge INIT\n",id);
 }
 error = KE_TIMEDOUT;
 goto end;
endok: error = KE_OK;
end:
 return error;
}

void cpu_stopme(void) {
 cpuid_t selfid = cpu_self();
 struct kcpu2 *cpu = &kcpu2_v[selfid];
 /* Reset the CPU state to ~just~ being available.
  * >> After this point, whenever someone needs a CPU for use,
  *    this one may be chosen randomly and be reset, causing
  *    its instruction pointer to jump elsewhere and escape
  *    the idle loop below. */
 cpu->p_state = CPUSTATE_AVAILABLE;
 /* End of the line... */
 __asm_volatile__("1:  cli\n"
                  "    hlt\n"
                  "    jmp 1b\n"
                  : : : "memory");
 __builtin_unreachable();
}


void cpu_acknowledge(void) {
 cpustate_t oldstate; cpuid_t selfid = cpu_self();
 struct kcpu2 *cpu = &kcpu2_v[selfid];
 int timeout = CPUINIT_ACKNOWLEDGE_SEND_TIMEOUT+1;
 while (--timeout) {
  oldstate = cpu->p_state;
  if (oldstate&CPUSTATE_STARTED) {
   /* Confirm running. */
   while (!katomic_cmpxch(cpu->p_state,oldstate,oldstate|CPUSTATE_RUNNING)) {
    oldstate = cpu->p_state;
    if (!(oldstate&CPUSTATE_STARTED)) goto end;
   }
   return;
  }
  i8253_delay10ms();
 }
end:
 /* Failed to receive startup confirmation (just shut ) */
 k_syslogf(KLOG_ERROR,"[SMP] CPU %I8u started without confirmation\n",selfid);
 cpu_stopme();
}


__DECL_END

#endif /* !__KOS_KERNEL_CPU_C__ */

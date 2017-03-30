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
#ifndef __KOS_KERNEL_TASK2_APIC_C_INL__
#define __KOS_KERNEL_TASK2_APIC_C_INL__ 1

#include <hw/rtc/cmos.h>
#include <hw/rtc/i8253.h>
#include <kos/compiler.h>
#include <kos/config.h>
#include <kos/kernel/arch/x86/apic.h>
#include <kos/kernel/arch/x86/mpfps.h>
#include <kos/kernel/gdt.h>
#include <kos/kernel/mutex.h>
#include <kos/kernel/task2-cpu.h>
#include <kos/kernel/task2.h>
#include <kos/syslog.h>
#include <sys/io.h>

__DECL_BEGIN

#define APIC_BOOTSTRAP_ADDR  0x7C00 /* Abuse realmode for now... */
#define _(x)  __PP_STR((((x)-apic_bootstrap_begin)+APIC_BOOTSTRAP_ADDR))
extern __u8 apic_bootstrap_begin[];
extern __u8 apic_bootstrap_end[];
extern __u8 apic_bootstrap_gdt[];
extern __u8 apic_bootstrap_idt[];


__crit void kcpu2_apic_initialize(void) {
 /* Relocate the APIC bootstrap code to a low memory address. */
 memcpy((void *)APIC_BOOTSTRAP_ADDR,apic_bootstrap_begin,
       (size_t)(apic_bootstrap_end-apic_bootstrap_begin));
 /* Save the GDT for use by the bootstrapping code. */
 // TODO: Per-cpu segments
 // TODO: Dynamic GDT handling
 __asm_volatile__("sgdt (" _(apic_bootstrap_gdt) ")\n");
 __asm_volatile__("sidt (" _(apic_bootstrap_idt) ")\n");
}


__crit kerrno_t
kcpu2_apic_int(struct kcpu2 *__restrict self, __u8 intno) {

 /* TODO */

 return KE_NOSYS;
}

#define S  __PP_STR
__asm__(".code16\n"
        "apic_bootstrap_begin:\n"
        /* This is the ~true~ entry point when warmbooting any additional CPU.
         * (Well... This is the code that'll be executed then anyways.
         *        - The true entrypoint is 'APIC_BOOTSTRAP_ADDR') */
        "    cli\n"
        "    lgdt (" _(apic_bootstrap_gdt) ")\n"
        "    lidt (" _(apic_bootstrap_idt) ")\n"
        /* Enter protected mode. */
        "    mov %cr0, %ebx\n"
        "    or  $0x00000001, %ebx\n"
        "    mov %ebx, %cr0\n"
        "    ljmpl $" S(KSEG_KERNEL_CODE) ", $apic_bootstrap32\n"
        "apic_bootstrap_gdt:\n"
        "    .word	0\n"
        "    .long	0\n"
        "apic_bootstrap_idt:\n"
        "    .word	0\n"
        "    .long	0\n"
        "apic_bootstrap_end:\n"
        ".code32\n"
);
#undef _


__asm__("apic_bootstrap32:\n"
        /* 32-bit mode bootstrap from here on. */
        "    mov $(" S(KSEG_KERNEL_DATA) "), %ax\n"
        "    mov %ax, %ds\n"
        "    mov %ax, %es\n"
      //"    mov %ax, %fs\n" /* CPU-self segment register. */
        "    mov %ax, %gs\n"
        "    mov %ax, %ss\n"
        /* Figure out our current CPU. */
        "    mov apic_id, %esi\n"
        "    mov (%esi),  %esi\n"
        "    shr $24,     %esi\n"
        "    and $0x0f,   %esi\n"
        /* The CPU's id is not stored in ESI. */
        "    add $kcpu2_vector, %esi\n"
        "    movw (" S(KCPU2_OFFSETOF_SEGID) ")(%esi), %bx\n"
        /* This CPU's segment ID is not stored in 'BX'. */
        "    mov %bx, %" KCPU2_SELF_SEGMENT_S "\n"
        /* The per-cpu memory segment is now set up!
         * >> Time to acknowledge the boot sequence
         *    and start running the current task. */
        "1:  mov (" S(KCPU2_OFFSETOF_FLAGS) ")(%esi), %cl\n"
        "    test $(" S(KCPU2_FLAG_STARTED) "), %cl\n"
        "    jz kcpu2_apic_offline\n" /* Handle sporadic bootups. */
        "    mov %cl, %al\n" /* Store old flags in AL */
        "    or $(" S(KCPU2_FLAG_RUNNING) "), %cl\n" /* Set the running flag. */
        "    lock cmpxchgb %cl, (" S(KCPU2_OFFSETOF_FLAGS) ")(%esi)\n" /* al = katomic_cmpxch_val(cpu->c_flags,al,cl); */
        "    jne 1b\n" /* if (al != cl) goto try_again; */
        /* Startup is confirmed to be intended. */
        /* Due to some race conditions, we must idle wait until the CPU's task list gets unlocked. */
        "1:  mov (" S(KCPU2_OFFSETOF_LOCKS) ")(%esi), %cl\n"
        "    test $(" S(KCPU2_LOCK_TASKS) "), %cl\n"
        "    jz 2f\n"
        "    pause\n"
        "    jmp 1b\n"
        "2:  \n"

        /* Time to actually move onto the scheduled tasks. */
        "    mov (" S(KCPU2_OFFSETOF_CURRENT) ")(%esi), %esi\n" /* Load ktask_self() into ESI. */
        "    mov (" S(KTASK2_OFFSETOF_USERREGS) ")(%esi), %esp\n"
        "    mov (" S(KTASK2_OFFSETOF_USERCR3) ")(%esi), %eax\n"
        "    mov %eax, %cr3\n"
        /* NOTE: 'EBX' is still filled with the CR0 value from the realmode bootstrap code. */
        "    or  $0x80000000, %ebx\n"
        "    mov %ebx, %cr0\n"
#if __SIZEOF_POINTER__ == 4
        "    lock decl kcpu2_offline\n"
        "    lock incl kcpu2_online\n"
#else
#error FIXME
#endif
/* TODO "    jmp ktask_irq_loadregs\n" */
        "1:  jmp 1b\n"
        ".size apic_bootstrap32, . - apic_bootstrap32\n"
        ".global kcpu2_apic_offline\n"
        "kcpu2_apic_offline:\n"
        "1:  cli\n"
        "    hlt\n"
        "    jmp 1b\n"
        ".size kcpu2_apic_offline, . - kcpu2_apic_offline\n"
);
#undef S

static struct kmutex apic_boot_lock = KMUTEX_INIT;

__crit kerrno_t
kcpu2_apic_start(struct kcpu2 *__restrict self) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kcpu2(self);
 error = kmutex_lock(&apic_boot_lock);
 if __unlikely(KE_ISERR(error)) return error;
 NOIRQ_BEGIN
 error = kcpu2_apic_start_unlocked(self,APIC_BOOTSTRAP_ADDR);
 NOIRQ_END
 kmutex_unlock(&apic_boot_lock);
 return error;
}

__crit __nomp kerrno_t
kcpu2_apic_start_unlocked(struct kcpu2 *__restrict self, __u32 start_eip) {
 int need_startup_ipi; kcpuid_t id;
 kassert_kcpu2(self);
 assert(!karch_irq_enabled());
 assertf(start_eip < ((__u32)1 << KCPU2_APIC_START_EIPBITS),"The given EIP %p is too large",start_eip);
 id = self->c_id;
 assertf(kcpu2_ispresent(self),"Inactive CPU id %I8u\n",id);

 /* NOTE: Since this function is the only place where the 'KCPU2_FLAG_STARTED' flag can be set,
  *       and since this function can only uninterrupted on one CPU at a time, we can safely
  *       check the 'KCPU2_FLAG_STARTED' flag here and can assume that it still won't be set
  *       at the bottom of this function! */
 if __unlikely(katomic_load(self->c_flags)&KCPU2_FLAG_STARTED) return KE_BUSY;
 assert(!(katomic_load(self->c_flags)&KCPU2_FLAG_RUNNING));

 /* Set BIOS shutdown code to warm start. */
 outb(CMOS_IOPORT_CODE,0xf);
 outb(CMOS_IOPORT_DATA,0xa);

 /* Set the reset vector address. */
 *(__u16 volatile *)TRAMPOLINE_PHYS_HIGH = (__u16)(start_eip >> 4);
 *(__u16 volatile *)TRAMPOLINE_PHYS_LOW  = (__u16)(start_eip & 0xf);

 need_startup_ipi = ((self->c_lapicver&0xf0) != APICVER_82489DX);

 if (need_startup_ipi) {
  /* Clear APIC errors. */
  apic_writel(APIC_ESR,0);
  apic_readl(APIC_ESR);
 }

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
  int n = KCPU2_APIC_IPI_ATTEMPTS,timeout;
send_again:
  do {
   k_syslogf(KLOG_DEBUG,"[SMP] Sending startup IPI #%d\n",3-n);
   apic_writel(APIC_ESR,0);
   apic_set_icr2_dest_field(id);
   apic_set_icr(APIC_DM_STARTUP|((start_eip >> 12) & APIC_VECTOR_MASK));
   k_syslogf(KLOG_DEBUG,"[SMP] Waiting for send to finish");
   timeout = KCPU2_APIC_IPI_TIMEOUT+1;
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
   return KE_PERM;
  }
  return KE_DEVICE;
 } else {
  /* No startup IPI required. */
 }
ipi_initialized:
 /* Wait for the INIT to be acknowledged.
  * NOTE: The race condition between here and when we actually
  *       started the CPU is solved by a timeout in cpu_acknowledge(). */
 katomic_fetchor(self->c_flags,KCPU2_FLAG_STARTED);
 {
  int timeout = KCPU2_APIC_ACKNOWLEDGE_RECV_TIMEOUT+1;
  __u8 oldstate;
  while (--timeout) {
   if (katomic_load(self->c_flags)&KCPU2_FLAG_RUNNING) return KE_OK; /* CPU is now running! */
   i8253_delay10ms();
  }
  do {
   oldstate = katomic_load(self->c_flags);
   if (oldstate&KCPU2_FLAG_RUNNING) return KE_OK; /* That was a close one... */
  } while (!katomic_cmpxch(self->c_flags,oldstate,oldstate&~(KCPU2_FLAG_STARTED)));
  k_syslogf(KLOG_ERROR,"[SMP] CPU %I8u failed to acknowledge INIT\n",id);
 }
 return KE_TIMEDOUT;
}


__DECL_END

#endif /* !__KOS_KERNEL_TASK2_APIC_C_INL__ */

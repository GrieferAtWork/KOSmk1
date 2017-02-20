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
#ifndef __KOS_KERNEL_INTERRUPTS_C__
#define __KOS_KERNEL_INTERRUPTS_C__ 1

#include <assert.h>
#include <kos/arch/x86/vga.h>
#include <kos/compiler.h>
#include <kos/kernel/debug.h>
#include <kos/kernel/interrupts.h>
#include <kos/atomic.h>
#include <kos/syslog.h>
#include <kos/kernel/serial.h>
#include <kos/kernel/tty.h>
#include <itos.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/io.h>
#include <kos/kernel/paging.h>
#include <kos/kernel/gdt.h>
#include <kos/kernel/task.h>
#include <kos/kernel/proc.h>
#include <disasm/disasm.h>
#include <kos/kernel/shm2.h>

__DECL_BEGIN

#define C(x) x(void)
void C(isr0), C(isr1), C(isr2), C(isr3), C(isr4), C(isr5), C(isr6), C(isr7)
    ,C(isr8), C(isr9), C(isr10),C(isr11),C(isr12),C(isr13),C(isr14),C(isr15)
    ,C(isr16),C(isr17),C(isr18),C(isr19),C(isr20),C(isr21),C(isr22),C(isr23)
    ,C(isr24),C(isr25),C(isr26),C(isr27),C(isr28),C(isr29),C(isr30),C(isr31)
/*
    ,C(irq0) ,C(irq1), C(irq2), C(irq3), C(irq4), C(irq5), C(irq6), C(irq7)
    ,C(irq8) ,C(irq9), C(irq10),C(irq11),C(irq12),C(irq13),C(irq14)
*/
;
#undef C

void lidt(uintptr_t base, __size_t limit){
 __asm_volatile__("subl $6, %%esp\n\t"
                  "movw %w0, 0(%%esp)\n\t"
                  "movl %1, 2(%%esp)\n\t"
                  "lidt (%%esp)\n\t"
                  "addl $6, %%esp"
                  : : "rN" (limit),"R" (base));
}

struct idt_entry {
 __u16 offset_1;
 __u16 selector;
 __u8  zero;
 __u8  type_attr;
 __u16 offset_2;
};
#define IDTFLAG_PRESENT                 0x80 /*< Set to 0 for unused interrupts. */
// Descriptor Privilege Level 	Gate call protection.
// Specifies which privilege Level the calling Descriptor minimum should have.
// So hardware and CPU interrupts can be protected from being called out of userspace.
#define IDTFLAG_DPL(n)          (((n)&3)<<5) /*< Mask: 0x60 */
#define IDTFLAG_STORAGE_SEGMENT         0x10 /*< Set to 0 for interrupt gates. */
#define IDTTYPE_80386_32_TASK_GATE      0x05
#define IDTTYPE_80286_16_INTERRUPT_GATE 0x06
#define IDTTYPE_80286_16_TRAP_GATE      0x07
#define IDTTYPE_80386_32_INTERRUPT_GATE 0x0E
#define IDTTYPE_80386_32_TRAP_GATE      0x0F

struct idt_entry idt[256];

void make_idt_entry(struct idt_entry *entry,
                    void(*handler)(void), uint16_t sel, uint8_t flags) {
 entry->offset_1  = ((uintptr_t)handler & 0xFFFF);
 entry->offset_2  = ((uintptr_t)handler >> 16) & 0xFFFF;
 entry->selector  = sel;
 entry->zero      = 0;
 entry->type_attr = flags;
}

#define IRQ(i) extern void irq##i(void)
/*[[[deemon
#include <util>
print "          ",;
for (i: util::range(1,224)) {
  print " IRQ({i.rjust(3)});".format({ .i = str i }),;
  if ((i % 16) == 15) print;
}
]]]*/
           IRQ(  1); IRQ(  2); IRQ(  3); IRQ(  4); IRQ(  5); IRQ(  6); IRQ(  7); IRQ(  8); IRQ(  9); IRQ( 10); IRQ( 11); IRQ( 12); IRQ( 13); IRQ( 14); IRQ( 15);
 IRQ( 16); IRQ( 17); IRQ( 18); IRQ( 19); IRQ( 20); IRQ( 21); IRQ( 22); IRQ( 23); IRQ( 24); IRQ( 25); IRQ( 26); IRQ( 27); IRQ( 28); IRQ( 29); IRQ( 30); IRQ( 31);
 IRQ( 32); IRQ( 33); IRQ( 34); IRQ( 35); IRQ( 36); IRQ( 37); IRQ( 38); IRQ( 39); IRQ( 40); IRQ( 41); IRQ( 42); IRQ( 43); IRQ( 44); IRQ( 45); IRQ( 46); IRQ( 47);
 IRQ( 48); IRQ( 49); IRQ( 50); IRQ( 51); IRQ( 52); IRQ( 53); IRQ( 54); IRQ( 55); IRQ( 56); IRQ( 57); IRQ( 58); IRQ( 59); IRQ( 60); IRQ( 61); IRQ( 62); IRQ( 63);
 IRQ( 64); IRQ( 65); IRQ( 66); IRQ( 67); IRQ( 68); IRQ( 69); IRQ( 70); IRQ( 71); IRQ( 72); IRQ( 73); IRQ( 74); IRQ( 75); IRQ( 76); IRQ( 77); IRQ( 78); IRQ( 79);
 IRQ( 80); IRQ( 81); IRQ( 82); IRQ( 83); IRQ( 84); IRQ( 85); IRQ( 86); IRQ( 87); IRQ( 88); IRQ( 89); IRQ( 90); IRQ( 91); IRQ( 92); IRQ( 93); IRQ( 94); IRQ( 95);
 IRQ( 96); IRQ( 97); IRQ( 98); IRQ( 99); IRQ(100); IRQ(101); IRQ(102); IRQ(103); IRQ(104); IRQ(105); IRQ(106); IRQ(107); IRQ(108); IRQ(109); IRQ(110); IRQ(111);
 IRQ(112); IRQ(113); IRQ(114); IRQ(115); IRQ(116); IRQ(117); IRQ(118); IRQ(119); IRQ(120); IRQ(121); IRQ(122); IRQ(123); IRQ(124); IRQ(125); IRQ(126); IRQ(127);
 IRQ(128); IRQ(129); IRQ(130); IRQ(131); IRQ(132); IRQ(133); IRQ(134); IRQ(135); IRQ(136); IRQ(137); IRQ(138); IRQ(139); IRQ(140); IRQ(141); IRQ(142); IRQ(143);
 IRQ(144); IRQ(145); IRQ(146); IRQ(147); IRQ(148); IRQ(149); IRQ(150); IRQ(151); IRQ(152); IRQ(153); IRQ(154); IRQ(155); IRQ(156); IRQ(157); IRQ(158); IRQ(159);
 IRQ(160); IRQ(161); IRQ(162); IRQ(163); IRQ(164); IRQ(165); IRQ(166); IRQ(167); IRQ(168); IRQ(169); IRQ(170); IRQ(171); IRQ(172); IRQ(173); IRQ(174); IRQ(175);
 IRQ(176); IRQ(177); IRQ(178); IRQ(179); IRQ(180); IRQ(181); IRQ(182); IRQ(183); IRQ(184); IRQ(185); IRQ(186); IRQ(187); IRQ(188); IRQ(189); IRQ(190); IRQ(191);
 IRQ(192); IRQ(193); IRQ(194); IRQ(195); IRQ(196); IRQ(197); IRQ(198); IRQ(199); IRQ(200); IRQ(201); IRQ(202); IRQ(203); IRQ(204); IRQ(205); IRQ(206); IRQ(207);
 IRQ(208); IRQ(209); IRQ(210); IRQ(211); IRQ(212); IRQ(213); IRQ(214); IRQ(215); IRQ(216); IRQ(217); IRQ(218); IRQ(219); IRQ(220); IRQ(221); IRQ(222); IRQ(223);
//[[[end]]]
#undef IRQ

extern void ktask_irq(void);
static void (*idt_functions[256])(void) = {
 &isr0,&isr1,&isr2,&isr3,&isr4,&isr5,&isr6,&isr7,&isr8,&isr9,&isr10,&isr11,&isr12,&isr13,&isr14,&isr15,
 &isr16,&isr17,&isr18,&isr19,&isr20,&isr21,&isr22,&isr23,&isr24,&isr25,&isr26,&isr27,&isr28,&isr29,&isr30,&isr31,
 &ktask_irq,
           &irq1,  &irq2,  &irq3,  &irq4,  &irq5,  &irq6,  &irq7,  &irq8,  &irq9, &irq10, &irq11, &irq12, &irq13, &irq14, &irq15,
 &irq16,  &irq17, &irq18, &irq19, &irq20, &irq21, &irq22, &irq23, &irq24, &irq25, &irq26, &irq27, &irq28, &irq29, &irq30, &irq31,
 &irq32,  &irq33, &irq34, &irq35, &irq36, &irq37, &irq38, &irq39, &irq40, &irq41, &irq42, &irq43, &irq44, &irq45, &irq46, &irq47,
 &irq48,  &irq49, &irq50, &irq51, &irq52, &irq53, &irq54, &irq55, &irq56, &irq57, &irq58, &irq59, &irq60, &irq61, &irq62, &irq63,
 &irq64,  &irq65, &irq66, &irq67, &irq68, &irq69, &irq70, &irq71, &irq72, &irq73, &irq74, &irq75, &irq76, &irq77, &irq78, &irq79,
 &irq80,  &irq81, &irq82, &irq83, &irq84, &irq85, &irq86, &irq87, &irq88, &irq89, &irq90, &irq91, &irq92, &irq93, &irq94, &irq95,
 &irq96,  &irq97, &irq98, &irq99,&irq100,&irq101,&irq102,&irq103,&irq104,&irq105,&irq106,&irq107,&irq108,&irq109,&irq110,&irq111,
 &irq112,&irq113,&irq114,&irq115,&irq116,&irq117,&irq118,&irq119,&irq120,&irq121,&irq122,&irq123,&irq124,&irq125,&irq126,&irq127,
 &irq128,&irq129,&irq130,&irq131,&irq132,&irq133,&irq134,&irq135,&irq136,&irq137,&irq138,&irq139,&irq140,&irq141,&irq142,&irq143,
 &irq144,&irq145,&irq146,&irq147,&irq148,&irq149,&irq150,&irq151,&irq152,&irq153,&irq154,&irq155,&irq156,&irq157,&irq158,&irq159,
 &irq160,&irq161,&irq162,&irq163,&irq164,&irq165,&irq166,&irq167,&irq168,&irq169,&irq170,&irq171,&irq172,&irq173,&irq174,&irq175,
 &irq176,&irq177,&irq178,&irq179,&irq180,&irq181,&irq182,&irq183,&irq184,&irq185,&irq186,&irq187,&irq188,&irq189,&irq190,&irq191,
 &irq192,&irq193,&irq194,&irq195,&irq196,&irq197,&irq198,&irq199,&irq200,&irq201,&irq202,&irq203,&irq204,&irq205,&irq206,&irq207,
 &irq208,&irq209,&irq210,&irq211,&irq212,&irq213,&irq214,&irq215,&irq216,&irq217,&irq218,&irq219,&irq220,&irq221,&irq222,&irq223,
};

static void set_default_irq_handlers(void);
void kernel_initialize_interrupts(void) {
 memset(&idt,0,sizeof(idt));
 set_default_irq_handlers();
#define ICW1        0x11
#define ICW2_MASTER 0x20
#define ICW2_SLAVE  0x28
#define ICW3_MASTER 0x04
#define ICW3_SLAVE  0x02
#define ICW4        0x01

 // Program the interrupt controller
 outb_p(0x20,ICW1       );
 outb_p(0xA0,ICW1       );
 outb_p(0x21,ICW2_MASTER);
 outb_p(0xA1,ICW2_SLAVE );
 outb_p(0x21,ICW3_MASTER);
 outb_p(0xA1,ICW3_SLAVE );
 outb_p(0x21,ICW4       );
 outb_p(0xA1,ICW4       );
 outb_p(0x21,0x00       );
 outb_p(0xA1,0x00       );
 for (unsigned int i = 0; i < __compiler_ARRAYSIZE(idt); ++i) {
  __u8 flags = IDTFLAG_PRESENT|IDTTYPE_80386_32_INTERRUPT_GATE|IDTFLAG_DPL(3);
  make_idt_entry(&idt[i],idt_functions[i],KSEG_KERNEL_CODE,flags);
 }
 lidt((uintptr_t)idt,sizeof(idt)-1);
}


static kirq_handler_t interrupt_handlers[KIRQ_SIGNAL_COUNT];
static void set_default_irq_handlers(void) {
 kirq_handler_t *iter,*end;
 end = (iter = interrupt_handlers)+KIRQ_SIGNAL_COUNT;
 while (iter != end) *iter++ = kirq_default_handler;
}


kirq_handler_t kirq_gethandler(kirq_t signum) {
 if (signum >= KIRQ_SIGNAL_COUNT) return NULL;
 return katomic_load(interrupt_handlers[signum]);
}
kirq_handler_t kirq_sethandler(kirq_t signum, kirq_handler_t new_handler) {
 assert(new_handler);
 if (signum >= KIRQ_SIGNAL_COUNT) return NULL;
 return katomic_xch(interrupt_handlers[signum],new_handler);
}

static __attribute_unused int
print_error(char const *__restrict data,
            size_t maxchars,
            void *__unused(closure)) {
 tty_printn(data,maxchars);
 serial_printn(SERIAL_01,data,maxchars);
 return 0;
}

void
kshmbranch_print(struct kshmbranch *__restrict branch,
             uintptr_t addr_semi, unsigned int level);


void __kirq_default_handler(struct kirq_registers *regs) {
 //assert(ktask_self() != NULL);
 //printf("INTERADDR = %p\n",regs->esp);
 if (regs->regs.intno < 32) {
  struct kirq_siginfo const *info;
  struct ktask *caller = ktask_self();
  info = kirq_getsiginfo(regs->regs.intno);
  if __unlikely(KE_ISERR(kmem_validateob(caller))) caller = NULL;
  k_syslogf(KLOG_ERROR,"\nTask: %I32u|%Iu: %s"
            "\nException code received %u|%u\n>> [%s] %s\n",
            caller ? caller->t_proc->p_pid : -1,
            caller ? caller->t_tid : -1,
            caller ? ktask_getname(caller) : NULL,
            regs->regs.intno,regs->regs.ecode,
            info ? info->isi_mnemonic : NULL,
            info ? info->isi_name : NULL);
  serial_print(SERIAL_01,"########### Registers:\n");
  kirq_print_registers(regs);
#define GETREG(name) __xblock({ void *temp; __asm_volatile__("movl %%" name ", %0" : "=r"(temp)); __xreturn temp; })
  printf("cr0     = % 8I32x\n",GETREG("cr0"));
//printf("cr1     = % 8I32x\n",GETREG("cr1"));
  printf("cr2     = % 8I32x\n",GETREG("cr2"));
  printf("cr3     = % 8I32x\n",GETREG("cr3"));
#undef GETREG
#ifdef __DEBUG__
  serial_printf(SERIAL_01,"########### Location (EIP):\n"
                "#!$ addr2line(%p) '{file}({line}) : {func} : %p'\n",
                regs->regs.eip,regs->regs.eip);
  serial_print(SERIAL_01,"########### Traceback (EBP):\n");
  _printtracebackebp_d((void *)(uintptr_t)regs->regs.ebp);
  serial_print(SERIAL_01,"########### Traceback (interrupt):\n");
  _printtraceback_d();
#endif
  {
   struct kpagedir *pd = NULL;
   if ((kmem_validateob(caller) == KE_OK) &&
       (kmem_validateob(caller->t_proc) == KE_OK)) {
    if ((pd = caller->t_proc->p_shm.sm_pd) != NULL &&
        (kmem_validateob(pd) == KE_OK)) {
     kpagedir_print(pd);
     kshmbranch_print(caller->t_proc->p_shm.s_map.m_root,
                  KSHMBRANCH_ADDRSEMI_INIT,
                  KSHMBRANCH_ADDRLEVEL_INIT);
    } else {
     printf("INVALID PAGEDIRECTORY (NULL POINTER)\n");
    }
   }
#if 0
   {
    void *physical_eip;
    size_t disasm_size = 120;
    physical_eip = (void *)regs->regs.eip;
    if (pd) physical_eip = kpagedir_translate(pd,physical_eip);
    disasm_x86((void *)((uintptr_t)physical_eip-(disasm_size/2)),
               disasm_size+1,&print_error,NULL,DISASM_FLAG_ADDR);
   }
#endif
  }
#undef abort
  abort();
 }
}

void x86_interrupt_handler(struct kirq_registers *regs) {
 kirq_handler_t handler;
 assertf(regs->regs.intno < KIRQ_SIGNAL_COUNT,
         "Invalid signal: %u|%u",regs->regs.intno,regs->regs.ecode);
 handler = katomic_load(interrupt_handlers[regs->regs.intno]);
#if 0
 printf("Calling handler %u (%u) %p\n",regs->regs.intno,
        regs->regs.intno >= 32 ? regs->regs.intno-32 : regs->regs.intno,
        handler);
 //kirq_print_registers(regs);
 //_printtraceback_d();
#endif
 assertf(handler,"NULL-handler for interrupt %u at %p",
         regs->regs.intno,&interrupt_handlers[regs->regs.intno]);
 (*handler)(regs);
}



static struct kirq_siginfo const __irq_siginfo_data[] = {
   {"#DE",    "Divide-by-zero"},
   {"#DB",    "Debug"},
   {"-",      "Non-maskable Interrupt"},
   {"#BP",    "Breakpoint"},
   {"#OF",    "Overflow"},
   {"#BR",    "Bound Range Exceeded"},
   {"#UD",    "Invalid Opcode"},
   {"#NM",    "Device Not Available"},
   {"#DF",    "Double Fault"},
// {"-",      "Coprocessor Segment Overrun"},
   {NULL,     NULL},
   {"#TS",    "Invalid TSS"},
   {"#NP",    "Segment Not Present"},
   {"#SS",    "Stack-Segment Fault"},
   {"#GP",    "General Protection Fault"},
   {"#PF",    "Page Fault"},
   {NULL,     NULL},
   {"#MF",    "x87 Floating-Point Exception"},
   {"#AC",    "Alignment Check"},
   {"#MC",    "Machine Check"},
   {"#XM/#XF","SIMD Floating-Point Exception"},
   {"#VE",    "Virtualization Exception"},
   {NULL,     NULL},
   {NULL,     NULL},
   {NULL,     NULL},
   {NULL,     NULL},
   {NULL,     NULL},
   {NULL,     NULL},
   {NULL,     NULL},
   {NULL,     NULL},
   {NULL,     NULL},
   {"#SX",    "Security Exception"},
   {NULL,     NULL},
   {"-",      "Triple Fault"},
};
enum{x=__compiler_ARRAYSIZE(__irq_siginfo_data)};

struct kirq_siginfo const *kirq_getsiginfo(kirq_t signum) {
 struct kirq_siginfo const *result;
 if (signum >= __compiler_ARRAYSIZE(__irq_siginfo_data)) return NULL;
 result = &__irq_siginfo_data[signum];
 if (!result->isi_mnemonic) result = NULL;
 return result;
}





void kirq_print_registers(struct kirq_registers const *regs) {
 printf("regs @ %p\n",regs);
 printf("ds      = % 8I32x"
#if KCONFIG_HAVE_I386_SAVE_SEGMENT_REGISTERS
                         " | es       = % 8I32x\n"
        "fs      = % 8I32x | gs       = % 8I32x"
#endif
        "\n"
        "edi     = % 8I32x | esi      = % 8I32x\n"
        "ebp     = % 8I32x | ebx      = % 8I32x\n"
        "edx     = % 8I32x | ecx      = % 8I32x\n"
        "eax     = % 8I32x\n"
        "intno   = % 8I32x | ecode    = % 8I32x\n"
        "eip     = % 8I32x | cs       = % 8I32x\n"
        "eflags  = % 8I32x | userregs = % 8I32x\n"
        "usercr3 = % 8I32x\n"
       ,regs->regs.ds
#if KCONFIG_HAVE_I386_SAVE_SEGMENT_REGISTERS
       ,regs->regs.es
       ,regs->regs.fs
       ,regs->regs.gs
#endif
       ,regs->regs.edi
       ,regs->regs.esi
       ,regs->regs.ebp
       ,regs->regs.ebx
       ,regs->regs.edx
       ,regs->regs.ecx
       ,regs->regs.eax
       ,regs->regs.intno
       ,regs->regs.ecode
       ,regs->regs.eip
       ,regs->regs.cs
       ,regs->regs.eflags
       ,regs->userregs
       ,regs->usercr3);
 if (kirq_registers_isuser(regs)) {
  printf("=============== USER ===============\n"
         "useresp = % 8I32x | ss       = % 8I32x\n"
        ,regs->regs.useresp
        ,regs->regs.ss);
 }
}

__DECL_END

#endif /* !__KOS_KERNEL_INTERRUPTS_C__ */

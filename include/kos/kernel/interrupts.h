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
#ifndef __KOS_KERNEL_INTERRUPTS_H__
#define __KOS_KERNEL_INTERRUPTS_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/compiler.h>
#include <kos/kernel/types.h>
#include <kos/kernel/features.h>
#include <kos/kernel/sched_yield.h>
#include <kos/kernel/region.h>
#include <kos/arch/irq.h>

__DECL_BEGIN

#define KIRQ_REGISTERS_OFFSETOF_REGS_USERREGS (__SIZEOF_POINTER__*0)
#define KIRQ_REGISTERS_OFFSETOF_REGS_USERCR3  (__SIZEOF_POINTER__*1)
#define KIRQ_REGISTERS_OFFSETOF_REGS_DS       (__SIZEOF_POINTER__*2)
#if KTASK_I386_SAVE_SEGMENT_REGISTERS
#define KIRQ_REGISTERS_OFFSETOF_REGS_ES       (__SIZEOF_POINTER__*3)
#define KIRQ_REGISTERS_OFFSETOF_REGS_FS       (__SIZEOF_POINTER__*4)
#define KIRQ_REGISTERS_OFFSETOF_REGS_GS       (__SIZEOF_POINTER__*5)
#define KIRQ_REGISTERS_OFFSETOF_REGS_EDI      (__SIZEOF_POINTER__*6)
#else
#define KIRQ_REGISTERS_OFFSETOF_REGS_EDI      (__SIZEOF_POINTER__*3)
#endif
#define KIRQ_REGISTERS_OFFSETOF_REGS_ESI      (KIRQ_REGISTERS_OFFSETOF_REGS_EDI+__SIZEOF_POINTER__*1)
#define KIRQ_REGISTERS_OFFSETOF_REGS_EBP      (KIRQ_REGISTERS_OFFSETOF_REGS_EDI+__SIZEOF_POINTER__*2)
#define KIRQ_REGISTERS_OFFSETOF_REGS_EBX      (KIRQ_REGISTERS_OFFSETOF_REGS_EDI+__SIZEOF_POINTER__*3)
#define KIRQ_REGISTERS_OFFSETOF_REGS_EDX      (KIRQ_REGISTERS_OFFSETOF_REGS_EDI+__SIZEOF_POINTER__*4)
#define KIRQ_REGISTERS_OFFSETOF_REGS_ECX      (KIRQ_REGISTERS_OFFSETOF_REGS_EDI+__SIZEOF_POINTER__*5)
#define KIRQ_REGISTERS_OFFSETOF_REGS_EAX      (KIRQ_REGISTERS_OFFSETOF_REGS_EDI+__SIZEOF_POINTER__*6)
#define KIRQ_REGISTERS_OFFSETOF_REGS_INTNO    (KIRQ_REGISTERS_OFFSETOF_REGS_EDI+__SIZEOF_POINTER__*6)
#define KIRQ_REGISTERS_OFFSETOF_REGS_ECODE    (KIRQ_REGISTERS_OFFSETOF_REGS_EDI+__SIZEOF_POINTER__*6+1*4)
#define KIRQ_REGISTERS_OFFSETOF_REGS_EIP      (KIRQ_REGISTERS_OFFSETOF_REGS_EDI+__SIZEOF_POINTER__*7+2*4)
#define KIRQ_REGISTERS_OFFSETOF_REGS_CS       (KIRQ_REGISTERS_OFFSETOF_REGS_EDI+__SIZEOF_POINTER__*8+2*4)
#define KIRQ_REGISTERS_OFFSETOF_REGS_EFLAGS   (KIRQ_REGISTERS_OFFSETOF_REGS_EDI+__SIZEOF_POINTER__*9+2*4)
#define KIRQ_REGISTERS_OFFSETOF_REGS_USERESP  (KIRQ_REGISTERS_OFFSETOF_REGS_EDI+__SIZEOF_POINTER__*10+2*4)
#define KIRQ_REGISTERS_OFFSETOF_REGS_SS       (KIRQ_REGISTERS_OFFSETOF_REGS_EDI+__SIZEOF_POINTER__*11+2*4)

#ifndef __ASSEMBLY__

__COMPILER_PACK_PUSH(1)
struct __packed kirq_userregisters {
#if KTASK_I386_SAVE_SEGMENT_REGISTERS
 /*3*/__uintptr_t ds,es,fs,gs;
#else
 /*1*/__uintptr_t ds;
#endif
 /*7*/__uintptr_t edi,esi,ebp,ebx,edx,ecx,eax;
 /*2*/__u32       intno,ecode;
 /*3*/__uintptr_t eip,cs,eflags;
 // WARNING: The following two fields are only
 //          valid for user (aka. ring-3) tasks.
 //       >> They may only be accessed or modified
 //          if the lower two bits in 'cs' are set:
 //       >> if (regs->regs.cs&0x3) {
 //       >>   regs->regs.useresp = 42;
 //       >> }
 /*2*/__uintptr_t useresp,ss;
};
struct __packed kirq_registers {
 // User-mapped registers address (usually part of kernel stack)
 __user struct kirq_userregisters *userregs;
 // User-set page directory (usually equal to 'kpagedir_user()')
 __kernel struct kpagedir         *usercr3;
 // NOTE: The following 'regs' object is what the
 //       above 'userregs' user-memory pointer maps to.
 //    >> The registers in 'regs' may be modified
 //       freely and upon IRQ return will be applied.
 // WARNING: 'userregs' may be changed, but if so, registers
 //          will be popped from its new location instead
 //          of that described by the 'regs' below.
 //       >> The kernel is therefor responsible to ensure
 //          that upon changing 'userregs', the new value describes
 //          a valid user pointer pointing at another instance
 //          of a 'kirq_userregisters' structure.
 // WARNING: While inside an interrupt handle, paging will be
 //          disabled, though the user-space page directory
 //          is still set in the CR3 register.
 //       >> Unless the kernel is attempting to perform a
 //          context switch (which should really just be left
 //          to the scheduler), the CR3 register must be
 //          set to the page directory of the new task as well.
 struct kirq_userregisters         regs;
};
__COMPILER_PACK_POP
#define kirq_registers_isuser(self) (((self)->regs.cs&0x3)==0x3)

extern __nonnull((1)) void irq_print_registers(struct kirq_registers const *__restrict regs);

typedef __u32 irq_t;
typedef void (*irq_handler_t)(struct kirq_registers *__restrict regs);

extern __nonnull((1)) void __irq_default_handler(struct kirq_registers *__restrict regs);
#define irq_default_handler  (&__irq_default_handler)
#endif /* !__ASSEMBLY__ */

//////////////////////////////////////////////////////////////////////////
// IRQ numbers of CPU-generated exceptions
#define KIRQ_EXCEPTION_MIN   0
#define KIRQ_EXCEPTION_MAX   31
#define KIRQ_EXCEPTION_COUNT 32
#ifdef __i386__
#define KIRQ_EXCEPTION_DE KARCH_X64_IRQ_DE /*< Divide-by-zero. */
#define KIRQ_EXCEPTION_DB KARCH_X64_IRQ_DB /*< Debug. */
#define KIRQ_EXCEPTION_BP KARCH_X64_IRQ_BP /*< Breakpoint. */
#define KIRQ_EXCEPTION_OF KARCH_X64_IRQ_OF /*< Overflow. */
#define KIRQ_EXCEPTION_BR KARCH_X64_IRQ_BR /*< Bound Range Exceeded. */
#define KIRQ_EXCEPTION_UD KARCH_X64_IRQ_UD /*< Invalid Opcode. */
#define KIRQ_EXCEPTION_NM KARCH_X64_IRQ_NM /*< Device Not Available. */
#define KIRQ_EXCEPTION_DF KARCH_X64_IRQ_DF /*< Double Fault. */
#define KIRQ_EXCEPTION_TS KARCH_X64_IRQ_TS /*< Invalid TSS. */
#define KIRQ_EXCEPTION_NP KARCH_X64_IRQ_NP /*< Segment Not Present. */
#define KIRQ_EXCEPTION_SS KARCH_X64_IRQ_SS /*< Stack-Segment Fault. */
#define KIRQ_EXCEPTION_GP KARCH_X64_IRQ_GP /*< General Protection Fault. */
#define KIRQ_EXCEPTION_PF KARCH_X64_IRQ_PF /*< Page Fault. */
#define KIRQ_EXCEPTION_MF KARCH_X64_IRQ_MF /*< x87 Floating-Point Exception. */
#define KIRQ_EXCEPTION_AC KARCH_X64_IRQ_AC /*< Alignment Check. */
#define KIRQ_EXCEPTION_MC KARCH_X64_IRQ_MC /*< Machine Check. */
#define KIRQ_EXCEPTION_XM KARCH_X64_IRQ_XM /*< SIMD Floating-Point Exception. */
#define KIRQ_EXCEPTION_XF KARCH_X64_IRQ_XF /*< *ditto*. */
#define KIRQ_EXCEPTION_VE KARCH_X64_IRQ_VE /*< Virtualization Exception. */
#define KIRQ_EXCEPTION_SX KARCH_X64_IRQ_SX /*< Security Exception. */
#endif


#define KIRQ_SIGNAL_COUNT   256

#ifndef __ASSEMBLY__
//////////////////////////////////////////////////////////////////////////
// Get/Set an irq interrupt handler for a given signal
// -> Returns 'NULL' if 'signum' is invalid
// -> Returns 'irq_default_handler' if no hander has been set
// The setter acts as an atomic exchange, returning the previous handler.
// -> Ids from 0..31   are the isr ids
// -> Ids from 32..256 are the irq ids
// e.g.: To register IRQ 0 (a programmable interrupt timer),
//       you should use id 32: set_irq_handler(32,...);
//       NOTE: IRQ 0 is used for scheduling
// A list of exception codes can be found here:
// >> http://wiki.osdev.org/Exceptions
// WARNING: IRQ Handlers may only perform non-blocking, asynchronous,
//          or otherwise marked as safe-to-call-from-irq functions.
//          This is required as the system falls apart if an IRQ
//          handler would attempt to do the wait for a signal that
//          would otherwise be send by the task itself (aka. deadlock).
//       >> NOTE: Locks that can only be acquired while interrupts are
//                disabled (aka. always entered with 'NOINTERRUPT_BEGINLOCK')
//                are safe to acquire from inside an IRQ handler, though.
//               (aka: You are allowed to send signals)
//       >> If you don't know if a call is preemptive (receives signals),
//          you can usually look at its return value documentation, if which
//          contains an entry for KE_INTR means that the call is _NOT_ safe.
extern __wunused      irq_handler_t get_irq_handler(irq_t signum);
extern __nonnull((2)) irq_handler_t set_irq_handler(irq_t signum, irq_handler_t new_handler);
#endif /* !__ASSEMBLY__ */

#ifndef __ASSEMBLY__
struct irq_siginfo {
 char const *isi_mnemonic; /*< [1..1] */
 char const *isi_name;     /*< [1..1] */
};

//////////////////////////////////////////////////////////////////////////
// Returns human-readable information about a given IRQ error number.
// Returns NULL if given given signum doesn't describe an error
extern __wunused __constcall struct irq_siginfo const *irq_getsiginfo(irq_t signum);
#endif /* !__ASSEMBLY__ */

#define CONFIG_COMPILETIME_NOINTERRUPT_OPTIMIZATIONS

#ifndef __ASSEMBLY__
#ifdef CONFIG_COMPILETIME_NOINTERRUPT_OPTIMIZATIONS
/* These symbol are overwritten locally whenever code is
   being compiled within a specially protected block.
   This way, we can detect such a block at compile-time
   and generate must more optimized code. */
REGION_DEFINE(NOINTERRUPT);

/* Returns true if in a no-interrupt block. */
#   define NOINTERRUPT_P         REGION_P(NOINTERRUPT)

/* Returns true if in a nested no-interrupt block. */
#   define NOINTERRUPT_NESTED_P  REGION_NESTED_P(NOINTERRUPT)
#else
RUNTIME_REGION_DEFINE(NOINTERRUPT);
#   define NOINTERRUPT_P         RUNTIME_REGION_P(NOINTERRUPT)
#   define NOINTERRUPT_NESTED_P  RUNTIME_REGION_NESTED_P(NOINTERRUPT)
#endif
#else /* !__ASSEMBLY__ */
#   define NOINTERRUPT_P         0
#   define NOINTERRUPT_NESTED_P  0
#endif /* __ASSEMBLY__ */


//////////////////////////////////////////////////////////////////////////
// Begin/End nointerrupt sections.
// NOTE: A nointerrupt section is considered an option for ensuring __crit compliance.
// WARNING: Some function calls may temporarily break a nointerrupt section, such
//          as blocking calls (anything that eventually calls 'ktask_unschedule_ex').
//       >> Such functions must not be called when critical resources, such as
//          references, or dynamic memory are owned by the calling task.
//          To safely call such functions, a secondary 'critical block' must be
//          established through a call to 'KTASK_CRIT_BEGIN', either as a
//          primary, or as a secondary guard:
//    -- Don't do this:
//       >> NOINTERRUPT_BEGIN;
//       >> // NO! NO! NO! NO! NO! THIS IS REALLY FUC$ING WRONG!
//       >> //  -> Even though this lock could safely be acquired, any lock
//       >> //     that could potentially be held somewhere further up the
//       >> //     stack is in danger of being left dangling!
//       >> // NOTE: In the future, the KOS kernel may provide a way for tasks
//       >> //       to track held locks and references safely without the need
//       >> //       of constantly being inside of a critical block.
//       >> kmutex_lock(&lock);
//       >> do_something();
//       >> kmutex_unlock(&lock);
//       >> NOINTERRUPT_END
//    -- Do this instead:
//       >> ktask_crt_begin();
//       >> // OK: The calling task will not be terminated while
//       >> //     still locking this, or any other mutex.
//       >> kmutex_lock(&lock);
//       >> do_something();
//       >> kmutex_unlock(&lock);
//       >> NOINTERRUPT_END
#if KCONFIG_HAVE_INTERRUPTS
#ifdef CONFIG_COMPILETIME_NOINTERRUPT_OPTIMIZATIONS
#define NOINTERRUPT_BEGIN                         REGION_ENTER(NOINTERRUPT,!karch_irq_enabled,karch_irq_disable)
#define NOINTERRUPT_END                           REGION_LEAVE(NOINTERRUPT,karch_irq_enable)
#define NOINTERRUPT_BREAK                         REGION_BREAK(NOINTERRUPT,karch_irq_enable)
#define NOINTERRUPT_BREAKUNLOCK(unlock) (unlock); REGION_BREAK(NOINTERRUPT,karch_irq_enable)
#define NOINTERRUPT_BEGINLOCK(trylock) \
 REGION_ENTER_LOCK(NOINTERRUPT,!karch_irq_enabled\
                  ,karch_irq_disable,karch_irq_enable,trylock)

// Begin a nointerrupt block by locking a volatile lock.
// While the lock is held, the volatile memory is unchangeable,
// NOTE: 'ob' should be a local variable
// NOTE: 'readvolatile' must be atomic and immutable when interrupts are disabled.
// NOTE: 'trylock' must return a boolean
// though while it isn't held, the location may change at any moment.
// @param: ob:           Cache for the currently loaded volatile instance
// @param: readvolatile: Reads the volatile memory that contains the real value
// @param: trylock:      Code used to lock the currently loaded cache of 
#define NOINTERRUPT_BEGINLOCK_VOLATILE(ob,readvolatile,trylock,unlock) \
 COMPILER_REGION_ENTER_LOCK_VOLATILE(NOINTERRUPT,!karch_irq_enabled,karch_irq_disable,\
                                     karch_irq_enable,ob,readvolatile,trylock,unlock)
#else
#define NOINTERRUPT_BEGIN                         RUNTIME_REGION_ENTER(NOINTERRUPT,!karch_irq_enabled,karch_irq_disable)
#define NOINTERRUPT_END                           RUNTIME_REGION_LEAVE(NOINTERRUPT,karch_irq_enable)
#define NOINTERRUPT_BREAK                         RUNTIME_REGION_BREAK(NOINTERRUPT,karch_irq_enable)
#define NOINTERRUPT_BREAKUNLOCK(unlock) (unlock); RUNTIME_REGION_BREAK(NOINTERRUPT,karch_irq_enable)
#define NOINTERRUPT_BEGINLOCK(trylock) \
 RUNTIME_REGION_ENTER_LOCK(NOINTERRUPT,!karch_irq_enabled,karch_irq_disable,karch_irq_enable,trylock)
#define NOINTERRUPT_BEGINLOCK_VOLATILE(ob,readvolatile,trylock,unlock) \
 RUNTIME_REGION_ENTER_LOCK_VOLATILE(NOINTERRUPT,!karch_irq_enabled,karch_irq_disable,\
                                    karch_irq_enable,ob,readvolatile,trylock,unlock)
#endif

#define /*__crit*/ NOINTERRUPT_ENDUNLOCK(unlock) \
 (unlock); NOINTERRUPT_END

#define NOINTERRUPT_EVAL(expr) \
 __xblock({ __typeof__(expr) __nires;\
            NOINTERRUPT_BEGIN\
            __nires = (expr);\
            NOINTERRUPT_END\
            __xreturn __nires;\
 })

#if 0 /* DELETE ME */
#define NOINTERRUPT_BEGINLOCK_VOLATILE(ob,readvolatile,trylock,unlock) \
 { __u32 __nistored = karch_irq_enabled();\
  (ob) = (readvolatile);\
  if (__nistored) for (;;) {\
   karch_irq_disable();\
   while (!(trylock)) {\
    volatile int __spinner = 10000;\
    karch_irq_enable();\
    if (ktask_tryyield() != KE_OK) while (__spinner--);\
    karch_irq_disable();\
   }\
   if __likely(ob == readvolatile) break;\
   /* Volatile memory has changed while we we're interrupting. */\
   unlock; karch_irq_enable(); ob = readvolatile;\
  } else {\
   /* TODO: This must succeed first-try on single-core config. */\
   while (!(trylock));\
  }
#if 0
} /* Fix side-bar highlighting in visual studio */
#endif
#endif

#else
#define NOINTERRUPT_BEGIN                 {
#define NOINTERRUPT_END                   }
#define NOINTERRUPT_BREAK                 ;
#define NOINTERRUPT_BEGINLOCK(trylock)  { { while(!(trylock)) ktask_yield(); }
#define NOINTERRUPT_BREAKUNLOCK(unlock)   (unlock)
#define NOINTERRUPT_ENDUNLOCK(unlock)     (unlock); }
#endif

#ifdef __MAIN_C__
//////////////////////////////////////////////////////////////////////////
// Initializes interrupts.
// NOTE: The caller must still enable them via 'karch_irq_enable()'
extern void kernel_initialize_interrupts(void);
#endif /* __MAIN_C__ */

__DECL_END

#ifdef __ASSEMBLY__
#include <kos/asm.h>
.macro PUSH_REGISTERS2_NOEAX
    push32 %ecx
    push32 %edx
    push32 %ebx
    push32 %ebp
    push32 %esi
    push32 %edi
#if 1
#if KTASK_I386_SAVE_SEGMENT_REGISTERS
    push32 %gs
    push32 %fs
    push32 %es
#endif
    push32 %ds
#else
#if KTASK_I386_SAVE_SEGMENT_REGISTERS
    mov %gs, %ebx
    push32 %ebx
    mov %fs, %ebx
    push32 %ebx
    mov %es, %ebx
    push32 %ebx
#endif
    mov %ds, %ebx
    push32 %ebx
#endif
.endm

.macro PUSH_REGISTERS
    push32 %eax
    PUSH_REGISTERS2_NOEAX
.endm

.macro POP_REGISTERS_NOEAX
#if KTASK_I386_SAVE_SEGMENT_REGISTERS
#if 1
    pop32 %ds
    pop32 %es
    pop32 %fs
    pop32 %gs
#else
    pop32 %ebx
    mov %ebx, %ds
    pop32 %ebx
    mov %ebx, %es
    pop32 %ebx
    mov %ebx, %fs
    pop32 %ebx
    mov %ebx, %gs
#endif
#else
    pop32 %ebx
    mov %ebx, %ds
    mov %ebx, %es
    mov %ebx, %fs
    mov %ebx, %gs
#endif
    pop32 %edi
    pop32 %esi
    pop32 %ebp
    pop32 %ebx
    pop32 %edx
    pop32 %ecx
.endm

.macro POP_REGISTERS
    POP_REGISTERS_NOEAX
    pop32 %eax
.endm

.macro DISABLE_PAGING
    mov32 %cr0, %eax
    and32 $0x7fffffff, %eax
    mov32 %eax, %cr0
.endm

.macro ENABLE_PAGING
    mov32 %cr0, %eax
    or32  $0x80000000, %eax
    mov32 %eax, %cr0
.endm

#endif
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_INTERRUPTS_H__ */

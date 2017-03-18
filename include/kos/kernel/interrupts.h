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
#if KCONFIG_HAVE_I386_SAVE_SEGMENT_REGISTERS
#define KIRQ_REGISTERS_OFFSETOF_REGS_ES       (__SIZEOF_POINTER__*2+2)
#define KIRQ_REGISTERS_OFFSETOF_REGS_FS       (__SIZEOF_POINTER__*2+4)
#define KIRQ_REGISTERS_OFFSETOF_REGS_GS       (__SIZEOF_POINTER__*2+6)
#define KIRQ_REGISTERS_OFFSETOF_REGS_EDI      (__SIZEOF_POINTER__*2+8)
#else
#define KIRQ_REGISTERS_OFFSETOF_REGS_EDI      (__SIZEOF_POINTER__*2+4)
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
#if KCONFIG_HAVE_I386_SAVE_SEGMENT_REGISTERS
 /*4*/__u16 ds,es,fs,gs;
#else
 /*2*/__u16 ds,__padding;
#endif
 /*7*/__uintptr_t edi,esi,ebp,ebx,edx,ecx,eax;
 /*2*/__u32       intno,ecode;
 /*3*/__uintptr_t eip,cs,eflags;
 /* WARNING: The following two fields are only
  *          valid for user (aka. ring-3) tasks.
  *       >> They may only be accessed or modified
  *          if the lower two bits in 'cs' are set:
  *       >> if (regs->regs.cs&0x3) {
  *       >>   regs->regs.useresp = 42;
  *       >> } */
 /*2*/__uintptr_t useresp,ss;
};
struct __packed kirq_registers {
 /* User-mapped registers address (usually part of kernel stack) */
 __user struct kirq_userregisters *userregs;
 /* User-set page directory (usually equal to the SHM pagedir of the calling process) */
 __kernel struct kpagedir         *usercr3;
 /* NOTE: The following 'regs' object is what the
  *       above 'userregs' user-memory pointer maps to.
  *    >> The registers in 'regs' may be modified freely
  *       and changes will be applied upon IRQ return.
  * WARNING: The 'userregs' pointer may be changed as well, but if
  *          so, registers will be popped from its new location
  *          instead of that described by the 'regs' below.
  *       >> The kernel is therefor responsible to ensure
  *          that upon changing 'userregs', the new value describes
  *          a valid user pointer pointing at another instance
  *          of a 'kirq_userregisters' structure. */
 struct kirq_userregisters         regs;
};
__COMPILER_PACK_POP
#define kirq_registers_isuser(self) (((self)->regs.cs&0x3)==0x3)

extern __nonnull((1)) void kirq_print_registers(struct kirq_registers const *__restrict regs);

typedef __u32 kirq_t;
typedef void (*kirq_handler_t)(struct kirq_registers *__restrict regs);

extern __nonnull((1)) void __kirq_default_handler(struct kirq_registers *__restrict regs);
#define kirq_default_handler  (&__kirq_default_handler)
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
// -> Returns 'kirq_default_handler' if no hander has been set
// The setter acts as an atomic exchange, returning the previous handler.
// -> Ids from 0..31   are the isr ids
// -> Ids from 32..256 are the irq ids
// e.g.: To register IRQ 0 (a programmable interrupt timer),
//       you should use id 32: kirq_sethandler(32,...);
//       NOTE: IRQ 0 is used for scheduling
// A list of exception codes can be found here:
// >> http://wiki.osdev.org/Exceptions
// WARNING: IRQ Handlers may only perform non-blocking, asynchronous,
//          or otherwise marked as safe-to-call-from-irq functions.
//          This is required as the system falls apart if an IRQ
//          handler would attempt to do the wait for a signal that
//          would otherwise be send by the task itself (aka. deadlock).
//       >> NOTE: Locks that can only be acquired while interrupts are
//                disabled (aka. always entered with 'NOIRQ_BEGINLOCK')
//                are safe to acquire from inside an IRQ handler, though.
//               (aka: You are allowed to send signals)
//       >> If you don't know if a call is preemptive (receives signals),
//          you can usually look at its return value documentation, if which
//          contains an entry for KE_INTR means that the call is _NOT_ safe.
extern __wunused      kirq_handler_t kirq_gethandler(kirq_t signum);
extern __nonnull((2)) kirq_handler_t kirq_sethandler(kirq_t signum, kirq_handler_t new_handler);
#endif /* !__ASSEMBLY__ */

#ifndef __ASSEMBLY__
struct kirq_siginfo {
 char const *isi_mnemonic; /*< [1..1] */
 char const *isi_name;     /*< [1..1] */
};

//////////////////////////////////////////////////////////////////////////
// Returns human-readable information about a given IRQ error number.
// Returns NULL if given given signum doesn't describe an error
extern __wunused __constcall struct kirq_siginfo const *kirq_getsiginfo(kirq_t signum);
#endif /* !__ASSEMBLY__ */

#define CONFIG_COMPILETIME_NOINTERRUPT_OPTIMIZATIONS

#ifndef __ASSEMBLY__
#ifdef CONFIG_COMPILETIME_NOINTERRUPT_OPTIMIZATIONS
/* These symbol are overwritten locally whenever code is
 * being compiled within a specially protected block.
 * This way, we can detect such a block at compile-time
 * and generate must more optimized code. */
REGION_DEFINE(NOIRQ);

/* Returns true if in a no-interrupt block. */
#   define NOIRQ_P         REGION_P(NOIRQ)

/* Returns true if in a nested no-interrupt block. */
#   define NOIRQ_NESTED_P  REGION_NESTED_P(NOIRQ)

/* Returns true if leaving the current noirq block will re-enable interrupts. */
#   define NOIRQ_FIRST     REGION_FIRST(NOIRQ)
#else
RUNTIME_REGION_DEFINE(NOIRQ);
#   define NOIRQ_P         RUNTIME_REGION_P(NOIRQ)
#   define NOIRQ_NESTED_P  RUNTIME_REGION_NESTED_P(NOIRQ)
#endif
#else /* !__ASSEMBLY__ */
#   define NOIRQ_P         0
#   define NOIRQ_NESTED_P  0
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
//       >> NOIRQ_BEGIN;
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
//       >> NOIRQ_END
//    -- Do this instead:
//       >> KTASK_CRIT_BEGIN
//       >> // OK: The calling task will not be terminated while
//       >> //     still locking this, or any other mutex.
//       >> kmutex_lock(&lock);
//       >> do_something();
//       >> kmutex_unlock(&lock);
//       >> KTASK_CRIT_END
#if KCONFIG_HAVE_IRQ
#ifdef CONFIG_COMPILETIME_NOINTERRUPT_OPTIMIZATIONS
#define NOIRQ_BEGIN                         REGION_ENTER(NOIRQ,!karch_irq_enabled,karch_irq_disable)
#define NOIRQ_END                           REGION_LEAVE(NOIRQ,karch_irq_enable)
#define NOIRQ_BREAK                         REGION_BREAK(NOIRQ,karch_irq_enable)
#define NOIRQ_BREAKUNLOCK(unlock) (unlock); REGION_BREAK(NOIRQ,karch_irq_enable)
#define NOIRQ_BEGINLOCK(trylock) \
 REGION_ENTER_LOCK(NOIRQ,!karch_irq_enabled\
                  ,karch_irq_disable,karch_irq_enable,trylock)

//////////////////////////////////////////////////////////////////////////
// Begin a nointerrupt block by locking a volatile lock.
// While the lock is held, the volatile memory is unchangeable,
// NOTE: 'ob' should be a local variable
// NOTE: 'readvolatile' must be atomic and immutable when interrupts are disabled.
// NOTE: 'trylock' must return a boolean
// though while it isn't held, the location may change at any moment.
// @param: ob:           Cache for the currently loaded volatile instance
// @param: readvolatile: Reads the volatile memory that contains the real value
// @param: trylock:      Code used to lock the currently loaded cache of 
#define NOIRQ_BEGINLOCK_VOLATILE(ob,readvolatile,trylock,unlock) \
 COMPILER_REGION_ENTER_LOCK_VOLATILE(NOIRQ,!karch_irq_enabled,karch_irq_disable,\
                                     karch_irq_enable,ob,readvolatile,trylock,unlock)
#else
#define NOIRQ_BEGIN                         RUNTIME_REGION_ENTER(NOIRQ,!karch_irq_enabled,karch_irq_disable)
#define NOIRQ_END                           RUNTIME_REGION_LEAVE(NOIRQ,karch_irq_enable)
#define NOIRQ_BREAK                         RUNTIME_REGION_BREAK(NOIRQ,karch_irq_enable)
#define NOIRQ_BREAKUNLOCK(unlock) (unlock); RUNTIME_REGION_BREAK(NOIRQ,karch_irq_enable)
#define NOIRQ_BEGINLOCK(trylock) \
 RUNTIME_REGION_ENTER_LOCK(NOIRQ,!karch_irq_enabled,karch_irq_disable,karch_irq_enable,trylock)
#define NOIRQ_BEGINLOCK_VOLATILE(ob,readvolatile,trylock,unlock) \
 RUNTIME_REGION_ENTER_LOCK_VOLATILE(NOIRQ,!karch_irq_enabled,karch_irq_disable,\
                                    karch_irq_enable,ob,readvolatile,trylock,unlock)
#endif

#define /*__crit*/ NOIRQ_ENDUNLOCK(unlock) \
 (unlock); NOIRQ_END

#define NOIRQ_EVAL(expr) \
 __xblock({ __typeof__(expr) __nires;\
            NOIRQ_BEGIN\
            __nires = (expr);\
            NOIRQ_END\
            __xreturn __nires;\
 })


#define NOIRQ_EVAL_V(expr) \
 __xblock({ NOIRQ_BEGIN\
            (expr);\
            NOIRQ_END\
            (void)0;\
 })

#else
#define NOIRQ_BEGIN                 {
#define NOIRQ_END                   ;}
#define NOIRQ_BREAK                 ;
#define NOIRQ_BEGINLOCK_VOLATILE(ob,readvolatile,trylock,unlock) \
 NOIRQ_BEGIN (ob) = (readvolatile); while (!(trylock));

#define NOIRQ_BEGINLOCK(trylock)  { { while(!(trylock)) ktask_yield(); }
#define NOIRQ_BREAKUNLOCK(unlock)   (unlock)
#define NOIRQ_ENDUNLOCK(unlock)     (unlock); }
#define NOIRQ_EVAL(expr)            (expr)
#define NOIRQ_EVAL_V(expr)          (expr)
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
#if KCONFIG_HAVE_I386_SAVE_SEGMENT_REGISTERS
    push16 %gs
    push16 %fs
    push16 %es
#else
    push16 $0
#endif
    push16 %ds
.endm

.macro PUSH_REGISTERS
    push32 %eax
    PUSH_REGISTERS2_NOEAX
.endm

.macro POP_REGISTERS_NOEAX
#if KCONFIG_HAVE_I386_SAVE_SEGMENT_REGISTERS
    pop16 %ds
    pop16 %es
    pop16 %fs
    pop16 %gs
#else
    pop16 %bx
    mov16 %bx, %ds
    mov16 %bx, %es
    mov16 %bx, %fs
    mov16 %bx, %gs
    addI $2, %esp
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

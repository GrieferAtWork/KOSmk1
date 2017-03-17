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
#ifndef __KOS_KERNEL_SYSCALL_SYSCALL_C__
#define __KOS_KERNEL_SYSCALL_SYSCALL_C__ 1

#include <kos/kernel/syscall.h>
#include <kos/atomic.h>
#include <kos/syscall.h>
#include <kos/syscallno.h>
#include <kos/syslog.h>
#ifdef __KOS_HAVE_NTSYSCALL
#include <windows/syscall.h>
#endif
#ifdef __KOS_HAVE_UNIXSYSCALL
#include <linux/syscall.h>
#endif

__DECL_BEGIN

void kernel_initialize_syscall(void) {
 k_syslogf(KLOG_INFO,"[init] Syscall...\n");
 kirq_sethandler(__SYSCALL_INTNO,&ksyscall_handler);
#ifdef __KOS_HAVE_NTSYSCALL
 kirq_sethandler(__NTSYSCALL_INTNO,&ksyscall_handler_nt);
#endif
#ifdef __KOS_HAVE_UNIXSYSCALL
 kirq_sethandler(__UNIXSYSCALL_INTNO,&ksyscall_handler_unix);
#endif
}

#ifdef __DEBUG__
void __ksyscall_enter_d(struct ksyscall_d *entry) {
 struct ktask *caller = ktask_self();
 assertf(!(caller->t_flags&KTASK_FLAG_NOINTR)
         ,"Nointerrupt task attempted to enter syscall %q (%Iu)"
         ,entry->sc_name,entry->sc_id);
 assertf(!(caller->t_flags&KTASK_FLAG_CRITICAL)
         ,"Critical task attempted to enter syscall %q (%Iu)"
         ,entry->sc_name,entry->sc_id);
 katomic_incfetch(entry->sc_inside);
 katomic_incfetch(entry->sc_total);
 if (entry->sc_id != SYS_kmem_validate /* Boooriiing! */
     ) {
  k_syslogf(KLOG_TRACE,"[syscall] Calling %q (%Iu)\n",
            entry->sc_name,entry->sc_id);
 }
}
void __ksyscall_leave_d(struct ksyscall_d *entry) {
 struct ktask *caller = ktask_self();
 katomic_decfetch(entry->sc_inside);
 assertf(!(caller->t_flags&KTASK_FLAG_NOINTR)
         ,"Nointerrupt usertask attempted to leave syscall %q (%Iu)"
         ,entry->sc_name,entry->sc_id);
 assertf(!(caller->t_flags&KTASK_FLAG_CRITICAL)
         ,"Critical usertask attempted to leave syscall %q (%Iu)"
         ,entry->sc_name,entry->sc_id);
}
#endif


#ifndef __INTELLISENSE__
#define SYSCALL_NULL  /* nothing */
#define SYSCALL(name) extern void syscall_##name(struct kirq_registers *regs);
#include <kos/kernel/syscall_list.h>
#undef SYSCALL
#undef SYSCALL_NULL
#endif
static void ksyscall_null(struct kirq_registers *regs) { regs->regs.eax = KE_NOSYS; }

static kirq_handler_t const global_syscalls[] = {
#define SYSCALL_NULL  &ksyscall_null,
#define SYSCALL_PAD   &ksyscall_null,
#define SYSCALL(name) &syscall_##name,
#include <kos/kernel/syscall_list.h>
#undef SYSCALL
#undef SYSCALL_NULL
#undef SYSCALL_PAD
};


void ksyscall_handler(struct kirq_registers *regs) {
 kirq_handler_t syscall;
 __STATIC_ASSERT(__compiler_ARRAYSIZE(global_syscalls) == SYSCALL_TOTAL);
 syscall = global_syscalls[regs->regs.eax&((1 << (SYSCALL_IDBITS+1))-1)];
 (*syscall)(regs);
}

#ifdef __KOS_HAVE_NTSYSCALL
void ksyscall_handler_nt(struct kirq_registers *regs) { (void)regs; /* TODO */ }
#endif
#ifdef __KOS_HAVE_UNIXSYSCALL
void ksyscall_handler_unix(struct kirq_registers *regs) { (void)regs; /* TODO */ }
#endif

__DECL_END

#endif /* !__KOS_KERNEL_SYSCALL_SYSCALL_C__ */

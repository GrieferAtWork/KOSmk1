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
#if KCONFIG_HAVE_SYSCALL_TRACE
#include <ctype.h>
#include <alloca.h>
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

#if KCONFIG_HAVE_SYSCALL_TRACE
#define SYSCALL_TRACE_LEVEL  KLOG_TRACE

/* Blacklist of syscalls not to log:
 * - 'kmem_validate' is called a lot when mall generates tracebacks.
 * - Logging 'k_syslog' would defeat the point, since it's already about logging. */
#define SYSCALL_SHOULDTRACE(callid) \
 (/*(callid) != SYS_kmem_validate && */\
  (callid) != SYS_k_syslog)

static void print_arg(char const *type, const char const *name, __uintptr_t value) {
 struct fmt_map { char const *type_name; char const *printf_name; };
 static struct fmt_map const map[] = {
  {"char const *","%~q"}, /*< Userspace string. */
  {"void const *","%p"},
  {"void *","%p"},
#if __SIZEOF_POINTER__ >= 8
  {"__u64","%#.16I64x"},
  {"__s64","%I64d"},
#endif
  {"__u32","%#.8I32x"},
  {"__u16","%#.4I16x"},
  {"__u8","%#.2I8x"},
  {"__s32","%I32d"},
  {"__s16","%I16d"},
  {"__s8","%I8d"},
  {"size_t","%Iu"},
  {"ssize_t","%Id"},
  {"unsigned int","%u"},
  {"unsigned","%u"},
  {"unsigned long","%lu"},
  {"long","%ld"},
  {"int","%d"},
#if __SIZEOF_POINTER__ == 4
  {NULL,"%#.8Ix"},
#elif __SIZEOF_POINTER__ == 8
  {NULL,"%#.16Ix"},
#elif __SIZEOF_POINTER__ == 2
  {NULL,"%#.4Ix"},
#else
#error FIXME
#endif
 };
 size_t typelen;
 struct fmt_map const *iter = map;
 while (*type && isspace(*type)) ++type;
 typelen = strlen(type);
 while (typelen && isspace(type[typelen-1])) --typelen;
 while (iter->type_name && strncmp(iter->type_name,type,typelen) != 0) ++iter;
 if (name) {
  typelen = strnlen(type,typelen);
  k_dosyslog(SYSCALL_TRACE_LEVEL,NULL,NULL,type,typelen);
  if (typelen && (type[typelen-1] != '*' &&
                  type[typelen-1] != '.')
      ) k_dosyslog(SYSCALL_TRACE_LEVEL,NULL,NULL," ",1);
  k_dosyslog(SYSCALL_TRACE_LEVEL,NULL,NULL,name,(size_t)-1);
  k_dosyslog(SYSCALL_TRACE_LEVEL,NULL,NULL," = ",(size_t)-1);
 }
 k_dosyslogf(SYSCALL_TRACE_LEVEL,NULL,NULL,iter->printf_name,value);
}

void __ksyscall_enter_d(struct kirq_registers *__restrict regs, struct ksyscall_d *entry) {
 struct ktask *caller = ktask_self();
 assertf(!(caller->t_flags&KTASK_FLAG_NOINTR)
         ,"Nointerrupt task attempted to enter syscall %q (%Iu)"
         ,entry->sc_name,entry->sc_id);
 assertf(!(caller->t_flags&KTASK_FLAG_CRITICAL)
         ,"Critical task attempted to enter syscall %q (%Iu)"
         ,entry->sc_name,entry->sc_id);
 katomic_incfetch(entry->sc_inside);
 katomic_incfetch(entry->sc_total);
 if (k_sysloglevel >= SYSCALL_TRACE_LEVEL &&
     SYSCALL_SHOULDTRACE(entry->sc_id)) {
  uintptr_t *args = (uintptr_t *)alloca(entry->sc_argc*sizeof(uintptr_t));
  uintptr_t *iter,*end;
  char const *const *name,*const *type;
  switch (entry->sc_argc) {
   default:
    if (copy_from_user(args+5,(__user void *)regs->regs.ebp,(entry->sc_argc-5)*sizeof(uintptr_t)))
                memset(args+5,0,(entry->sc_argc-5)*sizeof(uintptr_t));
   case 5: args[4] = regs->regs.edi;
   case 4: args[3] = regs->regs.esi;
   case 3: args[2] = regs->regs.ebx;
   case 2: args[1] = regs->regs.edx;
   case 1: args[0] = regs->regs.ecx;
  }
  k_dosyslogf(SYSCALL_TRACE_LEVEL,NULL,NULL,"[syscall] Calling %Iu:%s(",entry->sc_id,entry->sc_name);
  end = (iter = args)+entry->sc_argc;
  name = entry->sc_namev,type = entry->sc_typev;
  for (; iter != end; ++iter,++type,++name) {
   if (iter != args) k_dosyslog(SYSCALL_TRACE_LEVEL,NULL,NULL,", ",2);
   print_arg(*type,*name,*iter);
  }
  k_dosyslog(SYSCALL_TRACE_LEVEL,NULL,NULL,")\n",2);
 }
}
void __ksyscall_leave_d(struct kirq_registers *__restrict regs, struct ksyscall_d *entry) {
 struct ktask *caller = ktask_self();
 katomic_decfetch(entry->sc_inside);
 assertf(!(caller->t_flags&KTASK_FLAG_NOINTR)
         ,"Nointerrupt usertask attempted to leave syscall %q (%Iu)"
         ,entry->sc_name,entry->sc_id);
 assertf(!(caller->t_flags&KTASK_FLAG_CRITICAL)
         ,"Critical usertask attempted to leave syscall %q (%Iu)"
         ,entry->sc_name,entry->sc_id);
#if 0
 if (k_sysloglevel >= SYSCALL_TRACE_LEVEL &&
     SYSCALL_SHOULDTRACE(entry->sc_id)) {
  k_dosyslog(SYSCALL_TRACE_LEVEL,NULL,NULL," -> ",4);
  print_arg(entry->sc_type,NULL,regs->regs.eax);
  k_dosyslog(SYSCALL_TRACE_LEVEL,NULL,NULL,"\n",1);
 }
#endif
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
 STATIC_ASSERT(COMPILER_ARRAYSIZE(global_syscalls) == SYSCALL_TOTAL);
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

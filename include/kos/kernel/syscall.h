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
#ifndef __KOS_KERNEL_SYSCALL_H__
#define __KOS_KERNEL_SYSCALL_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/syscallno.h>
#include <kos/types.h>
#include <kos/kernel/features.h>
#include <kos/kernel/util/string.h>

__DECL_BEGIN

#ifndef KCONFIG_HAVE_SYSCALL_TRACE
#define KCONFIG_HAVE_SYSCALL_TRACE 0
#endif

#ifdef __INTELLISENSE__
/* Syntax highlighting & non-glitchy autocompletion. */
enum{r,c,cr,rc};
#endif

#if KCONFIG_HAVE_SYSCALL_TRACE
struct ksyscall_d {
 char const        *sc_name;   /*< System call name. */
 char const        *sc_type;   /*< System call return type string. */
 __uintptr_t        sc_id;     /*< System call ID. */
 __size_t           sc_argc;   /*< Amount of syscall arguments. */
 char const *const *sc_typev;  /*< [1..1][0..sc_argc] Vector of argument type strings. */
 char const *const *sc_namev;  /*< [1..1][0..sc_argc] Vector of argument name strings. */
 kirq_handler_t     sc_call;   /*< The IRQ-compatible handler used to wrap this syscall. */
 __atomic __size_t  sc_total;  /*< The total amount of calls to 'sc_call'. */
 __atomic __size_t  sc_inside; /*< The amount of tasks currently within 'sc_call'.
                                *  WARNING: If the syscall referenced by 'sc_call' is
                                *           noreturn, this value must not be trusted. */
};
#define __KSYSCALL_INITTAIL ,0,0
extern void __ksyscall_enter_d(struct ksyscall_d *entry);
extern void __ksyscall_leave_d(struct ksyscall_d *entry);
#define __ksyscall_enter_d __ksyscall_enter_d
#define __ksyscall_leave_d __ksyscall_leave_d
#endif /* KCONFIG_HAVE_SYSCALL_TRACE */

#if !KCONFIG_HAVE_IRQ
#define KSYSCALL_PREEMPT \
 {/* Allow for preemption on systems without IRQ interrupts. */\
  struct ktask *caller = ktask_self();\
  if (!caller->t_preempt--) {\
   caller->t_preempt = KTASK_PREEMT_COUNTDOWN;\
   ktask_yield();\
  }\
 }
#endif /* !KCONFIG_HAVE_IRQ */


#if defined(__ksyscall_enter_d) && defined(KSYSCALL_PREEMPT)
#define ksyscall_enter_d(x) KSYSCALL_PREEMPT __ksyscall_enter_d(x)
#elif defined(__ksyscall_enter_d)
#define ksyscall_enter_d    __ksyscall_enter_d
#elif defined(KSYSCALL_PREEMPT)
#define ksyscall_enter_d(x) KSYSCALL_PREEMPT
#else
#define ksyscall_enter_d(x) /* nothing */
#endif
#ifdef __ksyscall_leave_d
#define ksyscall_leave_d    __ksyscall_leave_d
#else
#define ksyscall_leave_d(x) /* nothing */
#endif


#ifdef __DEEMON__
#define MAX_ARGS     8
#endif


/* Syscall flags:
 * 'r': Pass a hidden 'regs' argument to the syscall function.
 * 'c': The syscall is executed inside of a KTASK_CRIT block.
 */
#define KSYSCALL_HASFLAG_R(flags) __KSYSCALL_HASFLAG_R_##flags
#define KSYSCALL_HASFLAG_C(flags) __KSYSCALL_HASFLAG_C_##flags

#define __KSYSCALL_HASFLAG_R_   0
#define __KSYSCALL_HASFLAG_R_0  0
#define __KSYSCALL_HASFLAG_R_r  1
#define __KSYSCALL_HASFLAG_R_c  0
#define __KSYSCALL_HASFLAG_R_rc 1
#define __KSYSCALL_HASFLAG_R_cr 1
#define __KSYSCALL_HASFLAG_C_   0
#define __KSYSCALL_HASFLAG_C_0  0
#define __KSYSCALL_HASFLAG_C_r  0
#define __KSYSCALL_HASFLAG_C_c  1
#define __KSYSCALL_HASFLAG_C_rc 1
#define __KSYSCALL_HASFLAG_C_cr 1

#define __KSYSCALL_REGISTER_PAR_0
#define __KSYSCALL_REGISTER_ARG_0
#define __KSYSCALL_REGISTER_PAR_1  __kernel struct kirq_registers *__restrict regs,
#define __KSYSCALL_REGISTER_ARG_1  regs,
#define __KSYSCALL_REGISTER_PAR0_0 void
#define __KSYSCALL_REGISTER_ARG0_0 /* nothing */
#define __KSYSCALL_REGISTER_PAR0_1 __kernel struct kirq_registers *__restrict regs
#define __KSYSCALL_REGISTER_ARG0_1 regs

#define __KSYSCALL_REGISTER_PAR(flags)  __PP_CAT_2(__KSYSCALL_REGISTER_PAR_,KSYSCALL_HASFLAG_R(flags))
#define __KSYSCALL_REGISTER_ARG(flags)  __PP_CAT_2(__KSYSCALL_REGISTER_ARG_,KSYSCALL_HASFLAG_R(flags))
#define __KSYSCALL_REGISTER_PAR0(flags) __PP_CAT_2(__KSYSCALL_REGISTER_PAR0_,KSYSCALL_HASFLAG_R(flags))
#define __KSYSCALL_REGISTER_ARG0(flags) __PP_CAT_2(__KSYSCALL_REGISTER_ARG0_,KSYSCALL_HASFLAG_R(flags))

#define __KSYSCALL_ENTER_C0  /* nothing */
#define __KSYSCALL_ENTER_C1  KTASK_CRIT_BEGIN_FIRST
#define __KSYSCALL_LEAVE_C0  /* nothing */
#define __KSYSCALL_LEAVE_C1  KTASK_CRIT_END
#define __KSYSCALL_ENTER(flags) __PP_CAT_2(__KSYSCALL_ENTER_C,KSYSCALL_HASFLAG_C(flags))
#define __KSYSCALL_LEAVE(flags) __PP_CAT_2(__KSYSCALL_LEAVE_C,KSYSCALL_HASFLAG_C(flags))



/*[[[deemon
#include <util>
for (local i: util::range(MAX_ARGS)) {
  print "#define KIDSYSCALL_WRAPPER"+i+"(flags,type,name,id",;
  for (local j: util::range(i)) print ",type"+(j+1)+",arg"+(j+1),;
  print ") \\";
  print " void syscall_##name(struct kirq_registers *__restrict regs) {\\";
  print "  __STATIC_ASSERT(sizeof(type)  <= __SIZEOF_POINTER__);\\";
  for (local j: util::range(i)) {
    print "  __STATIC_ASSERT(sizeof(type"+(j+1)+") <= __SIZEOF_POINTER__);\\";
  }
  print "  ksyscall_enter_d(&sysdbg_##name);\\";
  print "  __KSYSCALL_ENTER(flags)\\";
  const first_missing_arg = 3;
  local register_arguments = util::min(i,first_missing_arg);
  local missing_arguments = i-register_arguments;
  if (missing_arguments) {
  }
  print "  {\\";
  if (missing_arguments) {
    print "   __uintptr_t stackargs["+missing_arguments+"];\\";
    print "   if __unlikely(copy_from_user(stackargs,(__user void *)regs->regs.ebp,\\";
    print "                 sizeof(stackargs))) regs->regs.eax = KE_FAULT;\\";
    print "   else\\";
  }
  print "   regs->regs.eax = (__uintptr_t)sysimpl_##name("
        "__KSYSCALL_REGISTER_ARG"+(i ? "" : "0")+"(flags)",;
  for (local j: util::range(register_arguments)) {
    print "\\\n                                               "+(j ? "," : " "),;
    print "(type"+(j+1)+")regs->regs."+["ebx","ecx","edx"][j],;
  }
  for (local j: util::range(missing_arguments)) {
    print "\\\n                                               ,(type"+(first_missing_arg+j+1)+")stackargs["+j+"]",;
  }
  print ");\\";
  print "  }\\";
  print "  __KSYSCALL_LEAVE(flags)\\";
  print "  ksyscall_leave_d(&sysdbg_##name);\\";
  print " }";
}
]]]*/
#define KIDSYSCALL_WRAPPER0(flags,type,name,id) \
 void syscall_##name(struct kirq_registers *__restrict regs) {\
  __STATIC_ASSERT(sizeof(type)  <= __SIZEOF_POINTER__);\
  ksyscall_enter_d(&sysdbg_##name);\
  __KSYSCALL_ENTER(flags)\
  {\
   regs->regs.eax = (__uintptr_t)sysimpl_##name(__KSYSCALL_REGISTER_ARG0(flags));\
  }\
  __KSYSCALL_LEAVE(flags)\
  ksyscall_leave_d(&sysdbg_##name);\
 }
#define KIDSYSCALL_WRAPPER1(flags,type,name,id,type1,arg1) \
 void syscall_##name(struct kirq_registers *__restrict regs) {\
  __STATIC_ASSERT(sizeof(type)  <= __SIZEOF_POINTER__);\
  __STATIC_ASSERT(sizeof(type1) <= __SIZEOF_POINTER__);\
  ksyscall_enter_d(&sysdbg_##name);\
  __KSYSCALL_ENTER(flags)\
  {\
   regs->regs.eax = (__uintptr_t)sysimpl_##name(__KSYSCALL_REGISTER_ARG(flags)\
                                                (type1)regs->regs.ebx);\
  }\
  __KSYSCALL_LEAVE(flags)\
  ksyscall_leave_d(&sysdbg_##name);\
 }
#define KIDSYSCALL_WRAPPER2(flags,type,name,id,type1,arg1,type2,arg2) \
 void syscall_##name(struct kirq_registers *__restrict regs) {\
  __STATIC_ASSERT(sizeof(type)  <= __SIZEOF_POINTER__);\
  __STATIC_ASSERT(sizeof(type1) <= __SIZEOF_POINTER__);\
  __STATIC_ASSERT(sizeof(type2) <= __SIZEOF_POINTER__);\
  ksyscall_enter_d(&sysdbg_##name);\
  __KSYSCALL_ENTER(flags)\
  {\
   regs->regs.eax = (__uintptr_t)sysimpl_##name(__KSYSCALL_REGISTER_ARG(flags)\
                                                (type1)regs->regs.ebx\
                                               ,(type2)regs->regs.ecx);\
  }\
  __KSYSCALL_LEAVE(flags)\
  ksyscall_leave_d(&sysdbg_##name);\
 }
#define KIDSYSCALL_WRAPPER3(flags,type,name,id,type1,arg1,type2,arg2,type3,arg3) \
 void syscall_##name(struct kirq_registers *__restrict regs) {\
  __STATIC_ASSERT(sizeof(type)  <= __SIZEOF_POINTER__);\
  __STATIC_ASSERT(sizeof(type1) <= __SIZEOF_POINTER__);\
  __STATIC_ASSERT(sizeof(type2) <= __SIZEOF_POINTER__);\
  __STATIC_ASSERT(sizeof(type3) <= __SIZEOF_POINTER__);\
  ksyscall_enter_d(&sysdbg_##name);\
  __KSYSCALL_ENTER(flags)\
  {\
   regs->regs.eax = (__uintptr_t)sysimpl_##name(__KSYSCALL_REGISTER_ARG(flags)\
                                                (type1)regs->regs.ebx\
                                               ,(type2)regs->regs.ecx\
                                               ,(type3)regs->regs.edx);\
  }\
  __KSYSCALL_LEAVE(flags)\
  ksyscall_leave_d(&sysdbg_##name);\
 }
#define KIDSYSCALL_WRAPPER4(flags,type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4) \
 void syscall_##name(struct kirq_registers *__restrict regs) {\
  __STATIC_ASSERT(sizeof(type)  <= __SIZEOF_POINTER__);\
  __STATIC_ASSERT(sizeof(type1) <= __SIZEOF_POINTER__);\
  __STATIC_ASSERT(sizeof(type2) <= __SIZEOF_POINTER__);\
  __STATIC_ASSERT(sizeof(type3) <= __SIZEOF_POINTER__);\
  __STATIC_ASSERT(sizeof(type4) <= __SIZEOF_POINTER__);\
  ksyscall_enter_d(&sysdbg_##name);\
  __KSYSCALL_ENTER(flags)\
  {\
   __uintptr_t stackargs[1];\
   if __unlikely(copy_from_user(stackargs,(__user void *)regs->regs.ebp,\
                 sizeof(stackargs))) regs->regs.eax = KE_FAULT;\
   else\
   regs->regs.eax = (__uintptr_t)sysimpl_##name(__KSYSCALL_REGISTER_ARG(flags)\
                                                (type1)regs->regs.ebx\
                                               ,(type2)regs->regs.ecx\
                                               ,(type3)regs->regs.edx\
                                               ,(type4)stackargs[0]);\
  }\
  __KSYSCALL_LEAVE(flags)\
  ksyscall_leave_d(&sysdbg_##name);\
 }
#define KIDSYSCALL_WRAPPER5(flags,type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5) \
 void syscall_##name(struct kirq_registers *__restrict regs) {\
  __STATIC_ASSERT(sizeof(type)  <= __SIZEOF_POINTER__);\
  __STATIC_ASSERT(sizeof(type1) <= __SIZEOF_POINTER__);\
  __STATIC_ASSERT(sizeof(type2) <= __SIZEOF_POINTER__);\
  __STATIC_ASSERT(sizeof(type3) <= __SIZEOF_POINTER__);\
  __STATIC_ASSERT(sizeof(type4) <= __SIZEOF_POINTER__);\
  __STATIC_ASSERT(sizeof(type5) <= __SIZEOF_POINTER__);\
  ksyscall_enter_d(&sysdbg_##name);\
  __KSYSCALL_ENTER(flags)\
  {\
   __uintptr_t stackargs[2];\
   if __unlikely(copy_from_user(stackargs,(__user void *)regs->regs.ebp,\
                 sizeof(stackargs))) regs->regs.eax = KE_FAULT;\
   else\
   regs->regs.eax = (__uintptr_t)sysimpl_##name(__KSYSCALL_REGISTER_ARG(flags)\
                                                (type1)regs->regs.ebx\
                                               ,(type2)regs->regs.ecx\
                                               ,(type3)regs->regs.edx\
                                               ,(type4)stackargs[0]\
                                               ,(type5)stackargs[1]);\
  }\
  __KSYSCALL_LEAVE(flags)\
  ksyscall_leave_d(&sysdbg_##name);\
 }
#define KIDSYSCALL_WRAPPER6(flags,type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6) \
 void syscall_##name(struct kirq_registers *__restrict regs) {\
  __STATIC_ASSERT(sizeof(type)  <= __SIZEOF_POINTER__);\
  __STATIC_ASSERT(sizeof(type1) <= __SIZEOF_POINTER__);\
  __STATIC_ASSERT(sizeof(type2) <= __SIZEOF_POINTER__);\
  __STATIC_ASSERT(sizeof(type3) <= __SIZEOF_POINTER__);\
  __STATIC_ASSERT(sizeof(type4) <= __SIZEOF_POINTER__);\
  __STATIC_ASSERT(sizeof(type5) <= __SIZEOF_POINTER__);\
  __STATIC_ASSERT(sizeof(type6) <= __SIZEOF_POINTER__);\
  ksyscall_enter_d(&sysdbg_##name);\
  __KSYSCALL_ENTER(flags)\
  {\
   __uintptr_t stackargs[3];\
   if __unlikely(copy_from_user(stackargs,(__user void *)regs->regs.ebp,\
                 sizeof(stackargs))) regs->regs.eax = KE_FAULT;\
   else\
   regs->regs.eax = (__uintptr_t)sysimpl_##name(__KSYSCALL_REGISTER_ARG(flags)\
                                                (type1)regs->regs.ebx\
                                               ,(type2)regs->regs.ecx\
                                               ,(type3)regs->regs.edx\
                                               ,(type4)stackargs[0]\
                                               ,(type5)stackargs[1]\
                                               ,(type6)stackargs[2]);\
  }\
  __KSYSCALL_LEAVE(flags)\
  ksyscall_leave_d(&sysdbg_##name);\
 }
#define KIDSYSCALL_WRAPPER7(flags,type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6,type7,arg7) \
 void syscall_##name(struct kirq_registers *__restrict regs) {\
  __STATIC_ASSERT(sizeof(type)  <= __SIZEOF_POINTER__);\
  __STATIC_ASSERT(sizeof(type1) <= __SIZEOF_POINTER__);\
  __STATIC_ASSERT(sizeof(type2) <= __SIZEOF_POINTER__);\
  __STATIC_ASSERT(sizeof(type3) <= __SIZEOF_POINTER__);\
  __STATIC_ASSERT(sizeof(type4) <= __SIZEOF_POINTER__);\
  __STATIC_ASSERT(sizeof(type5) <= __SIZEOF_POINTER__);\
  __STATIC_ASSERT(sizeof(type6) <= __SIZEOF_POINTER__);\
  __STATIC_ASSERT(sizeof(type7) <= __SIZEOF_POINTER__);\
  ksyscall_enter_d(&sysdbg_##name);\
  __KSYSCALL_ENTER(flags)\
  {\
   __uintptr_t stackargs[4];\
   if __unlikely(copy_from_user(stackargs,(__user void *)regs->regs.ebp,\
                 sizeof(stackargs))) regs->regs.eax = KE_FAULT;\
   else\
   regs->regs.eax = (__uintptr_t)sysimpl_##name(__KSYSCALL_REGISTER_ARG(flags)\
                                                (type1)regs->regs.ebx\
                                               ,(type2)regs->regs.ecx\
                                               ,(type3)regs->regs.edx\
                                               ,(type4)stackargs[0]\
                                               ,(type5)stackargs[1]\
                                               ,(type6)stackargs[2]\
                                               ,(type7)stackargs[3]);\
  }\
  __KSYSCALL_LEAVE(flags)\
  ksyscall_leave_d(&sysdbg_##name);\
 }
//[[[end]]]



/*[[[deemon
#include <util>

function syscall_forward(i) {
  print "type sysimpl_##name(__KSYSCALL_REGISTER_PAR"+(i ? "" : "0")+"(flags)",;
  if (i) {
    print " "+(",".join(for (local j: util::range(i)) "type"+(j+1)+" arg"+(j+1))),;
  }
  print ")",;
}

print "#if KCONFIG_HAVE_SYSCALL_TRACE";
for (local i: util::range(MAX_ARGS)) {
  print "#define KIDSYSCALL_DEBUG_EX"+i+"(flags,type,name,id",;
  for (local j: util::range(i)) print ",type"+(j+1)+",arg"+(j+1),;
  print ") \\";
  if (i) {
    print " static char const *const sysdbg_typev_##name[] = {"+",".join(for (local j: util::range(i)) "#type"+(j+1))+"};\\";
    print " static char const *const sysdbg_namev_##name[] = {"+",".join(for (local j: util::range(i)) "#arg"+(j+1))+"};\\";
    print " static struct ksyscall_d sysdbg_##name = {#name,#type,id,"+i+",sysdbg_typev_##name,sysdbg_namev_##name,&syscall_##name __KSYSCALL_INITTAIL};";
  } else {
    print " static struct ksyscall_d sysdbg_##name = {#name,#type,id,0,NULL,NULL,&syscall_##name __KSYSCALL_INITTAIL};";
  }
}
print "#else /" "* KCONFIG_HAVE_SYSCALL_TRACE *" "/";
for (local i: util::range(MAX_ARGS)) {
  print "#define KIDSYSCALL_DEBUG_EX"+i+"(flags,type,name,id",;
  for (local j: util::range(i)) print ",type"+(j+1)+",arg"+(j+1),;
  print ") /" "* nothing *" "/";
}
print "#endif /" "* !KCONFIG_HAVE_SYSCALL_TRACE *" "/";
print;

for (local i: util::range(MAX_ARGS)) {
  print "#define KIDSYSCALL_DEFINE_EX"+i+"(flags,type,name,id",;
  for (local j: util::range(i)) print ",type"+(j+1)+",arg"+(j+1),;
  print ") \\";
  print " extern void syscall_##name(struct kirq_registers *__restrict regs); \\";
  print " __forcelocal ",;
  syscall_forward(i);
  print ";\\";
  print " KIDSYSCALL_DEBUG_EX"+i+"(flags,type,name,id",;
  for (local j: util::range(i)) print ",type"+(j+1)+",arg"+(j+1),;
  print ")\\";
  print " KIDSYSCALL_WRAPPER"+i+"(flags,type,name,id",;
  for (local j: util::range(i)) print ",type"+(j+1)+",arg"+(j+1),;
  print ")\\";
  print " __forcelocal ",;
  syscall_forward(i);
  print;
}
print;
for (local i: util::range(MAX_ARGS)) {
  print "#define KSYSCALL_DEFINE_EX"+i+"(flags,type,name",;
  for (local j: util::range(i)) print ",type"+(j+1)+",arg"+(j+1),;
  print ") KIDSYSCALL_DEFINE_EX"+i+"(flags,type,name,SYS_##name",;
  for (local j: util::range(i)) print ",type"+(j+1)+",arg"+(j+1),;
  print ")";
}
print;
for (local i: util::range(MAX_ARGS)) {
  print "#define KIDSYSCALL_DEFINE"+i+"(type,name,id",;
  for (local j: util::range(i)) print ",type"+(j+1)+",arg"+(j+1),;
  print ") KIDSYSCALL_DEFINE_EX"+i+"(,type,name,SYS_##name",;
  for (local j: util::range(i)) print ",type"+(j+1)+",arg"+(j+1),;
  print ")";
}
print;
for (local i: util::range(MAX_ARGS)) {
  print "#define KSYSCALL_DEFINE"+i+"(type,name",;
  for (local j: util::range(i)) print ",type"+(j+1)+",arg"+(j+1),;
  print ") KIDSYSCALL_DEFINE_EX"+i+"(,type,name,SYS_##name",;
  for (local j: util::range(i)) print ",type"+(j+1)+",arg"+(j+1),;
  print ")";
}
]]]*/
#if KCONFIG_HAVE_SYSCALL_TRACE
#define KIDSYSCALL_DEBUG_EX0(flags,type,name,id) \
 static struct ksyscall_d sysdbg_##name = {#name,#type,id,0,NULL,NULL,&syscall_##name __KSYSCALL_INITTAIL};
#define KIDSYSCALL_DEBUG_EX1(flags,type,name,id,type1,arg1) \
 static char const *const sysdbg_typev_##name[] = {#type1};\
 static char const *const sysdbg_namev_##name[] = {#arg1};\
 static struct ksyscall_d sysdbg_##name = {#name,#type,id,1,sysdbg_typev_##name,sysdbg_namev_##name,&syscall_##name __KSYSCALL_INITTAIL};
#define KIDSYSCALL_DEBUG_EX2(flags,type,name,id,type1,arg1,type2,arg2) \
 static char const *const sysdbg_typev_##name[] = {#type1,#type2};\
 static char const *const sysdbg_namev_##name[] = {#arg1,#arg2};\
 static struct ksyscall_d sysdbg_##name = {#name,#type,id,2,sysdbg_typev_##name,sysdbg_namev_##name,&syscall_##name __KSYSCALL_INITTAIL};
#define KIDSYSCALL_DEBUG_EX3(flags,type,name,id,type1,arg1,type2,arg2,type3,arg3) \
 static char const *const sysdbg_typev_##name[] = {#type1,#type2,#type3};\
 static char const *const sysdbg_namev_##name[] = {#arg1,#arg2,#arg3};\
 static struct ksyscall_d sysdbg_##name = {#name,#type,id,3,sysdbg_typev_##name,sysdbg_namev_##name,&syscall_##name __KSYSCALL_INITTAIL};
#define KIDSYSCALL_DEBUG_EX4(flags,type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4) \
 static char const *const sysdbg_typev_##name[] = {#type1,#type2,#type3,#type4};\
 static char const *const sysdbg_namev_##name[] = {#arg1,#arg2,#arg3,#arg4};\
 static struct ksyscall_d sysdbg_##name = {#name,#type,id,4,sysdbg_typev_##name,sysdbg_namev_##name,&syscall_##name __KSYSCALL_INITTAIL};
#define KIDSYSCALL_DEBUG_EX5(flags,type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5) \
 static char const *const sysdbg_typev_##name[] = {#type1,#type2,#type3,#type4,#type5};\
 static char const *const sysdbg_namev_##name[] = {#arg1,#arg2,#arg3,#arg4,#arg5};\
 static struct ksyscall_d sysdbg_##name = {#name,#type,id,5,sysdbg_typev_##name,sysdbg_namev_##name,&syscall_##name __KSYSCALL_INITTAIL};
#define KIDSYSCALL_DEBUG_EX6(flags,type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6) \
 static char const *const sysdbg_typev_##name[] = {#type1,#type2,#type3,#type4,#type5,#type6};\
 static char const *const sysdbg_namev_##name[] = {#arg1,#arg2,#arg3,#arg4,#arg5,#arg6};\
 static struct ksyscall_d sysdbg_##name = {#name,#type,id,6,sysdbg_typev_##name,sysdbg_namev_##name,&syscall_##name __KSYSCALL_INITTAIL};
#define KIDSYSCALL_DEBUG_EX7(flags,type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6,type7,arg7) \
 static char const *const sysdbg_typev_##name[] = {#type1,#type2,#type3,#type4,#type5,#type6,#type7};\
 static char const *const sysdbg_namev_##name[] = {#arg1,#arg2,#arg3,#arg4,#arg5,#arg6,#arg7};\
 static struct ksyscall_d sysdbg_##name = {#name,#type,id,7,sysdbg_typev_##name,sysdbg_namev_##name,&syscall_##name __KSYSCALL_INITTAIL};
#else /* KCONFIG_HAVE_SYSCALL_TRACE */
#define KIDSYSCALL_DEBUG_EX0(flags,type,name,id) /* nothing */
#define KIDSYSCALL_DEBUG_EX1(flags,type,name,id,type1,arg1) /* nothing */
#define KIDSYSCALL_DEBUG_EX2(flags,type,name,id,type1,arg1,type2,arg2) /* nothing */
#define KIDSYSCALL_DEBUG_EX3(flags,type,name,id,type1,arg1,type2,arg2,type3,arg3) /* nothing */
#define KIDSYSCALL_DEBUG_EX4(flags,type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4) /* nothing */
#define KIDSYSCALL_DEBUG_EX5(flags,type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5) /* nothing */
#define KIDSYSCALL_DEBUG_EX6(flags,type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6) /* nothing */
#define KIDSYSCALL_DEBUG_EX7(flags,type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6,type7,arg7) /* nothing */
#endif /* !KCONFIG_HAVE_SYSCALL_TRACE */

#define KIDSYSCALL_DEFINE_EX0(flags,type,name,id) \
 extern void syscall_##name(struct kirq_registers *__restrict regs); \
 __forcelocal type sysimpl_##name(__KSYSCALL_REGISTER_PAR0(flags));\
 KIDSYSCALL_DEBUG_EX0(flags,type,name,id)\
 KIDSYSCALL_WRAPPER0(flags,type,name,id)\
 __forcelocal type sysimpl_##name(__KSYSCALL_REGISTER_PAR0(flags))
#define KIDSYSCALL_DEFINE_EX1(flags,type,name,id,type1,arg1) \
 extern void syscall_##name(struct kirq_registers *__restrict regs); \
 __forcelocal type sysimpl_##name(__KSYSCALL_REGISTER_PAR(flags) type1 arg1);\
 KIDSYSCALL_DEBUG_EX1(flags,type,name,id,type1,arg1)\
 KIDSYSCALL_WRAPPER1(flags,type,name,id,type1,arg1)\
 __forcelocal type sysimpl_##name(__KSYSCALL_REGISTER_PAR(flags) type1 arg1)
#define KIDSYSCALL_DEFINE_EX2(flags,type,name,id,type1,arg1,type2,arg2) \
 extern void syscall_##name(struct kirq_registers *__restrict regs); \
 __forcelocal type sysimpl_##name(__KSYSCALL_REGISTER_PAR(flags) type1 arg1,type2 arg2);\
 KIDSYSCALL_DEBUG_EX2(flags,type,name,id,type1,arg1,type2,arg2)\
 KIDSYSCALL_WRAPPER2(flags,type,name,id,type1,arg1,type2,arg2)\
 __forcelocal type sysimpl_##name(__KSYSCALL_REGISTER_PAR(flags) type1 arg1,type2 arg2)
#define KIDSYSCALL_DEFINE_EX3(flags,type,name,id,type1,arg1,type2,arg2,type3,arg3) \
 extern void syscall_##name(struct kirq_registers *__restrict regs); \
 __forcelocal type sysimpl_##name(__KSYSCALL_REGISTER_PAR(flags) type1 arg1,type2 arg2,type3 arg3);\
 KIDSYSCALL_DEBUG_EX3(flags,type,name,id,type1,arg1,type2,arg2,type3,arg3)\
 KIDSYSCALL_WRAPPER3(flags,type,name,id,type1,arg1,type2,arg2,type3,arg3)\
 __forcelocal type sysimpl_##name(__KSYSCALL_REGISTER_PAR(flags) type1 arg1,type2 arg2,type3 arg3)
#define KIDSYSCALL_DEFINE_EX4(flags,type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4) \
 extern void syscall_##name(struct kirq_registers *__restrict regs); \
 __forcelocal type sysimpl_##name(__KSYSCALL_REGISTER_PAR(flags) type1 arg1,type2 arg2,type3 arg3,type4 arg4);\
 KIDSYSCALL_DEBUG_EX4(flags,type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4)\
 KIDSYSCALL_WRAPPER4(flags,type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4)\
 __forcelocal type sysimpl_##name(__KSYSCALL_REGISTER_PAR(flags) type1 arg1,type2 arg2,type3 arg3,type4 arg4)
#define KIDSYSCALL_DEFINE_EX5(flags,type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5) \
 extern void syscall_##name(struct kirq_registers *__restrict regs); \
 __forcelocal type sysimpl_##name(__KSYSCALL_REGISTER_PAR(flags) type1 arg1,type2 arg2,type3 arg3,type4 arg4,type5 arg5);\
 KIDSYSCALL_DEBUG_EX5(flags,type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5)\
 KIDSYSCALL_WRAPPER5(flags,type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5)\
 __forcelocal type sysimpl_##name(__KSYSCALL_REGISTER_PAR(flags) type1 arg1,type2 arg2,type3 arg3,type4 arg4,type5 arg5)
#define KIDSYSCALL_DEFINE_EX6(flags,type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6) \
 extern void syscall_##name(struct kirq_registers *__restrict regs); \
 __forcelocal type sysimpl_##name(__KSYSCALL_REGISTER_PAR(flags) type1 arg1,type2 arg2,type3 arg3,type4 arg4,type5 arg5,type6 arg6);\
 KIDSYSCALL_DEBUG_EX6(flags,type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6)\
 KIDSYSCALL_WRAPPER6(flags,type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6)\
 __forcelocal type sysimpl_##name(__KSYSCALL_REGISTER_PAR(flags) type1 arg1,type2 arg2,type3 arg3,type4 arg4,type5 arg5,type6 arg6)
#define KIDSYSCALL_DEFINE_EX7(flags,type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6,type7,arg7) \
 extern void syscall_##name(struct kirq_registers *__restrict regs); \
 __forcelocal type sysimpl_##name(__KSYSCALL_REGISTER_PAR(flags) type1 arg1,type2 arg2,type3 arg3,type4 arg4,type5 arg5,type6 arg6,type7 arg7);\
 KIDSYSCALL_DEBUG_EX7(flags,type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6,type7,arg7)\
 KIDSYSCALL_WRAPPER7(flags,type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6,type7,arg7)\
 __forcelocal type sysimpl_##name(__KSYSCALL_REGISTER_PAR(flags) type1 arg1,type2 arg2,type3 arg3,type4 arg4,type5 arg5,type6 arg6,type7 arg7)

#define KSYSCALL_DEFINE_EX0(flags,type,name) KIDSYSCALL_DEFINE_EX0(flags,type,name,SYS_##name)
#define KSYSCALL_DEFINE_EX1(flags,type,name,type1,arg1) KIDSYSCALL_DEFINE_EX1(flags,type,name,SYS_##name,type1,arg1)
#define KSYSCALL_DEFINE_EX2(flags,type,name,type1,arg1,type2,arg2) KIDSYSCALL_DEFINE_EX2(flags,type,name,SYS_##name,type1,arg1,type2,arg2)
#define KSYSCALL_DEFINE_EX3(flags,type,name,type1,arg1,type2,arg2,type3,arg3) KIDSYSCALL_DEFINE_EX3(flags,type,name,SYS_##name,type1,arg1,type2,arg2,type3,arg3)
#define KSYSCALL_DEFINE_EX4(flags,type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4) KIDSYSCALL_DEFINE_EX4(flags,type,name,SYS_##name,type1,arg1,type2,arg2,type3,arg3,type4,arg4)
#define KSYSCALL_DEFINE_EX5(flags,type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5) KIDSYSCALL_DEFINE_EX5(flags,type,name,SYS_##name,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5)
#define KSYSCALL_DEFINE_EX6(flags,type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6) KIDSYSCALL_DEFINE_EX6(flags,type,name,SYS_##name,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6)
#define KSYSCALL_DEFINE_EX7(flags,type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6,type7,arg7) KIDSYSCALL_DEFINE_EX7(flags,type,name,SYS_##name,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6,type7,arg7)

#define KIDSYSCALL_DEFINE0(type,name,id) KIDSYSCALL_DEFINE_EX0(,type,name,SYS_##name)
#define KIDSYSCALL_DEFINE1(type,name,id,type1,arg1) KIDSYSCALL_DEFINE_EX1(,type,name,SYS_##name,type1,arg1)
#define KIDSYSCALL_DEFINE2(type,name,id,type1,arg1,type2,arg2) KIDSYSCALL_DEFINE_EX2(,type,name,SYS_##name,type1,arg1,type2,arg2)
#define KIDSYSCALL_DEFINE3(type,name,id,type1,arg1,type2,arg2,type3,arg3) KIDSYSCALL_DEFINE_EX3(,type,name,SYS_##name,type1,arg1,type2,arg2,type3,arg3)
#define KIDSYSCALL_DEFINE4(type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4) KIDSYSCALL_DEFINE_EX4(,type,name,SYS_##name,type1,arg1,type2,arg2,type3,arg3,type4,arg4)
#define KIDSYSCALL_DEFINE5(type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5) KIDSYSCALL_DEFINE_EX5(,type,name,SYS_##name,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5)
#define KIDSYSCALL_DEFINE6(type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6) KIDSYSCALL_DEFINE_EX6(,type,name,SYS_##name,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6)
#define KIDSYSCALL_DEFINE7(type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6,type7,arg7) KIDSYSCALL_DEFINE_EX7(,type,name,SYS_##name,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6,type7,arg7)

#define KSYSCALL_DEFINE0(type,name) KIDSYSCALL_DEFINE_EX0(,type,name,SYS_##name)
#define KSYSCALL_DEFINE1(type,name,type1,arg1) KIDSYSCALL_DEFINE_EX1(,type,name,SYS_##name,type1,arg1)
#define KSYSCALL_DEFINE2(type,name,type1,arg1,type2,arg2) KIDSYSCALL_DEFINE_EX2(,type,name,SYS_##name,type1,arg1,type2,arg2)
#define KSYSCALL_DEFINE3(type,name,type1,arg1,type2,arg2,type3,arg3) KIDSYSCALL_DEFINE_EX3(,type,name,SYS_##name,type1,arg1,type2,arg2,type3,arg3)
#define KSYSCALL_DEFINE4(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4) KIDSYSCALL_DEFINE_EX4(,type,name,SYS_##name,type1,arg1,type2,arg2,type3,arg3,type4,arg4)
#define KSYSCALL_DEFINE5(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5) KIDSYSCALL_DEFINE_EX5(,type,name,SYS_##name,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5)
#define KSYSCALL_DEFINE6(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6) KIDSYSCALL_DEFINE_EX6(,type,name,SYS_##name,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6)
#define KSYSCALL_DEFINE7(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6,type7,arg7) KIDSYSCALL_DEFINE_EX7(,type,name,SYS_##name,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6,type7,arg7)
//[[[end]]]

__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_SYSCALL_H__ */

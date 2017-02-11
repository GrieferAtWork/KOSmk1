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
#ifndef __KOS_KERNEL_SYSCALL_SYSCALL_COMMON_H__
#define __KOS_KERNEL_SYSCALL_SYSCALL_COMMON_H__ 1
#ifndef __KERNEL__
#error Kernel-only source file
#endif
#define __NO_PROTOTYPES

#include <kos/compiler.h>
#include <kos/arch.h>
#include <kos/endian.h>
#include <kos/syscallno.h>
#include <kos/kernel/interrupts.h>
#include <kos/kernel/paging.h>
#include <kos/kernel/proc.h>

__DECL_BEGIN

// TODO: Security: Ensure that all required arguments are mapped (page borders...)

struct kirq_registers;

#ifdef __INTELLISENSE__
#define A(T,i) (i,*(T *)0)
#elif defined(__NT_SYSTEM_CALL)
#define A(T,i) \
 (T)(((uintptr_t *)kpagedir_translate(kpagedir_user(),\
           (void *)(uintptr_t)regs->regs.edx))[i])
#elif defined(__UNIX_SYSTEM_CALL)
#define A(T,i) \
 ((i) == 0 ? (T)regs->regs.ebx :\
  (i) == 1 ? (T)regs->regs.ecx :\
  (i) == 2 ? (T)regs->regs.edx :\
  (i) == 3 ? (T)regs->regs.esi :\
  (i) == 4 ? (T)regs->regs.edi :\
  (i) == 5 ? (T)regs->regs.ebp : (T)0)
#else
#define A(T,i) \
 ((i) == 0 ? (T)regs->regs.ebx :\
  (i) == 1 ? (T)regs->regs.ecx :\
  (i) == 2 ? (T)regs->regs.edx :\
  (T)(((uintptr_t *)kpagedir_translate(kpagedir_user(),\
           (void *)(uintptr_t)regs->regs.ebp))[(i)-3]))
#endif
#define __TRANSLATE(p) kpagedir_translate(kpagedir_user(),p)
#define TRANSLATE(p)   ((__typeof__(p))kpagedir_translate(kpagedir_user(),p))
#define RETURN(ret)    do{(regs->regs.eax = (__u32)(__uintptr_t)(ret));return;}while(0)
#define SYSID          regs->regs.eax
#define __LOADMODE_K(x)  K  /*< Kernel pointer/data. */
#define __LOADMODE_U(x)  U  /*< User pointer. */
#define __LOADMODE_U0(x) U0 /*< User pointer (May be NULL). */
#define __LOADMODE_D(x)  D  /*< Double argument (takes up two slots). */
#define __LOADMODE_N     N  /*< NULL argument (skip slot). */
#define __LOADMODE(x) __LOADMODE_##x
#define __LOADARG_K(x)  x
#define __LOADARG_U(x)  x
#define __LOADARG_U0(x) x
#define __LOADARG_D(x)  x
#define __LOADARG_N     
#define __LOADARG(x) __LOADARG_##x

#define __LOAD_K(T,x,i)  T x = A(T,i)
#define __LOAD_U(T,x,i)  T x = (T)__TRANSLATE(A(void *,i)); if __unlikely(!x) RETURN(KE_FAULT)
#define __LOAD_U0(T,x,i) T x = A(T,i); if (x && (x = (T)__TRANSLATE((void *)x)) == NULL) RETURN(KE_FAULT)
#if (__BYTE_ORDER == __LITTLE_ENDIAN) == defined(KOS_ARCH_STACK_GROWS_DOWNWARDS)
#define __LOAD_D(T,x,i)  T x = (T)(((__u64)A(__u32,i) << 32) | (__u64)A(__u32,i+1))
#else /* linear: down */
#define __LOAD_D(T,x,i)  T x = (T)((__u64)A(__u32,i) | ((__u64)A(__u32,i+1) << 32))
#endif /* linear: up */
#define __LOAD_N(T,x,i)  /* nothing */
#define __LOAD(T,x,i)    __PP_CAT_2(__LOAD_,__LOADMODE(x))(T,__LOADARG(x),i)
/*[[[deemon
#include <util>
for (local x: util::range(8)) {
  print "#define LOAD"+x+"(",;
  for (local y: util::range(x)) print (y ? "," : "")+"type"+(y+1)+",arg"+(y+1),;
  print ") ",;
  if (x) {
    if (x > 1) {
      print "LOAD"+(x-1)+"(",;
      for (local y: util::range(x-1)) print (y ? "," : "")+"type"+(y+1)+",arg"+(y+1),;
      print "); ",;
    }
    print "__LOAD(type"+x+",arg"+x+","+(x-1)+")",;
  } else print "do{}while(0)",;
  print;
}
]]]*/
#define LOAD0() do{}while(0)
#define LOAD1(type1,arg1) __LOAD(type1,arg1,0)
#define LOAD2(type1,arg1,type2,arg2) LOAD1(type1,arg1); __LOAD(type2,arg2,1)
#define LOAD3(type1,arg1,type2,arg2,type3,arg3) LOAD2(type1,arg1,type2,arg2); __LOAD(type3,arg3,2)
#define LOAD4(type1,arg1,type2,arg2,type3,arg3,type4,arg4) LOAD3(type1,arg1,type2,arg2,type3,arg3); __LOAD(type4,arg4,3)
#define LOAD5(type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5) LOAD4(type1,arg1,type2,arg2,type3,arg3,type4,arg4); __LOAD(type5,arg5,4)
#define LOAD6(type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6) LOAD5(type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5); __LOAD(type6,arg6,5)
#define LOAD7(type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6,type7,arg7) LOAD6(type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6); __LOAD(type7,arg7,6)
//[[[end]]]

#define SYSCALL(name) static void name(struct kirq_registers *regs)

typedef void (*ksyscall_t)(struct kirq_registers *regs);

#define GROUPBEGIN(name) /* nothing */
#define GROUPEND         /* nothing */
#define ENTRY(name)      SYSCALL(sys_##name);
#include "syscall-groups.h"
#undef ENTRY
#undef GROUPEND
#undef GROUPBEGIN


#define GROUPBEGIN(name) static ksyscall_t const name[] = {
#define GROUPEND         };
#ifdef __INTELLISENSE__
#define ENTRY(name)                              &sys_##name,
#else
#define ENTRY(name) [__SYSCALL_ID(SYS_##name)] = &sys_##name,
#endif
#include "syscall-groups.h"
#undef ENTRY
#undef GROUPEND
#undef GROUPBEGIN

__DECL_END

#endif /* !__KOS_KERNEL_SYSCALL_SYSCALL_COMMON_H__ */

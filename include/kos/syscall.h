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
#ifndef __KOS_SYSCALL_H__
#define __KOS_SYSCALL_H__ 1

#include <kos/compiler.h>
#include <kos/config.h>
#include <kos/syscallno.h>

__DECL_BEGIN

#define __SYSCALL_INTNO  0x69 /*< Huehuehue... */

#ifndef __ASSEMBLY__
#define __ASMSYSCALL0(id,more)                  more "movl $" __PP_STR(id) ", %%eax\nint $" __PP_STR(__SYSCALL_INTNO) "\n"
#define __ASMSYSCALL1(id,more) __ASMSYSCALL0(id,"movl %1, %%ebx\n" more)
#define __ASMSYSCALL2(id,more) __ASMSYSCALL1(id,"movl %2, %%ecx\n" more)
#define __ASMSYSCALL3(id,more) __ASMSYSCALL2(id,"movl %3, %%edx\n" more)
#define __ASMSYSCALL4(id,more) __ASMSYSCALL3(id,"pushl %%ebp\npushl %4\n"                               "movl %%esp, %%ebp\n" more) "addl $4,  %%esp\npopl %%ebp\n"
#define __ASMSYSCALL5(id,more) __ASMSYSCALL3(id,"pushl %%ebp\npushl %5\npushl %4\n"                     "movl %%esp, %%ebp\n" more) "addl $8,  %%esp\npopl %%ebp\n"
#define __ASMSYSCALL6(id,more) __ASMSYSCALL3(id,"pushl %%ebp\npushl %6\npushl %5\npushl %4\n"           "movl %%esp, %%ebp\n" more) "addl $12, %%esp\npopl %%ebp\n"
#define __ASMSYSCALL7(id,more) __ASMSYSCALL3(id,"pushl %%ebp\npushl %7\npushl %6\npushl %5\npushl %4\n" "movl %%esp, %%ebp\n" more) "addl $16, %%esp\npopl %%ebp\n"
#define __ASMSYSCALL0_CLOBBER                        "eax"
#define __ASMSYSCALL1_CLOBBER __ASMSYSCALL0_CLOBBER, "ebx"
#define __ASMSYSCALL2_CLOBBER __ASMSYSCALL1_CLOBBER, "ecx"
#define __ASMSYSCALL3_CLOBBER __ASMSYSCALL2_CLOBBER, "edx"
#define __ASMSYSCALL4_CLOBBER __ASMSYSCALL3_CLOBBER
#define __ASMSYSCALL5_CLOBBER __ASMSYSCALL4_CLOBBER
#define __ASMSYSCALL6_CLOBBER __ASMSYSCALL4_CLOBBER
#define __ASMSYSCALL7_CLOBBER __ASMSYSCALL4_CLOBBER
/*[[[deemon
#include <util>
const MAX = 8;
function print_block(x,paste) {
  print "{ type __r; (void)(id); __asm_volatile__(__ASMSYSCALL"+x+"(id,\"\")",;
  print " \"movl %%eax, %0\" : \"=g\" (__r) :",;
  for (local y: util::range(x)) print (y ? "," : "")+" \"g\" (__"+paste+"arg"+(y+1)+")",;
  print " : __ASMSYSCALL"+x+"_CLOBBER); return __r; }";
}
print "#ifdef __ANSI_COMPILER__";
for (local x: util::range(MAX)) {
  print "#define __IDsyscall"+x+"(type,name,id",;
  for (local y: util::range(x)) print ",type"+(y+1)+",arg"+(y+1),;
  print ") type name(",;
  if (!x) print "void",;
  else for (local y: util::range(x)) print (y ? ", " : "")+"type"+(y+1)+" __##arg"+(y+1),;
  print ") ",;
  print_block(x,"##");
}
print "#else /" "* __ANSI_COMPILER__ *" "/";
for (local x: util::range(MAX)) {
  print "#define __IDsyscall"+x+"(type,name,id",;
  for (local y: util::range(x)) print ",type"+(y+1)+",arg"+(y+1),;
  print ") type name(",;
  for (local y: util::range(x)) print (y ? "," : "")+"__/" "**" "/arg"+(y+1),;
  print ") ",;
  for (local y: util::range(x)) print "type"+(y+1)+" __/" "**" "/arg"+(y+1)+"; ",;
  print_block(x,"/" "**" "/");
}
print "#endif /" "* !__ANSI_COMPILER__ *" "/";
]]]*/
#ifdef __ANSI_COMPILER__
#define __IDsyscall0(type,name,id) type name(void) { type __r; (void)(id); __asm_volatile__(__ASMSYSCALL0(id,"") "movl %%eax, %0" : "=g" (__r) : : __ASMSYSCALL0_CLOBBER); return __r; }
#define __IDsyscall1(type,name,id,type1,arg1) type name(type1 __##arg1) { type __r; (void)(id); __asm_volatile__(__ASMSYSCALL1(id,"") "movl %%eax, %0" : "=g" (__r) : "g" (__##arg1) : __ASMSYSCALL1_CLOBBER); return __r; }
#define __IDsyscall2(type,name,id,type1,arg1,type2,arg2) type name(type1 __##arg1, type2 __##arg2) { type __r; (void)(id); __asm_volatile__(__ASMSYSCALL2(id,"") "movl %%eax, %0" : "=g" (__r) : "g" (__##arg1), "g" (__##arg2) : __ASMSYSCALL2_CLOBBER); return __r; }
#define __IDsyscall3(type,name,id,type1,arg1,type2,arg2,type3,arg3) type name(type1 __##arg1, type2 __##arg2, type3 __##arg3) { type __r; (void)(id); __asm_volatile__(__ASMSYSCALL3(id,"") "movl %%eax, %0" : "=g" (__r) : "g" (__##arg1), "g" (__##arg2), "g" (__##arg3) : __ASMSYSCALL3_CLOBBER); return __r; }
#define __IDsyscall4(type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4) type name(type1 __##arg1, type2 __##arg2, type3 __##arg3, type4 __##arg4) { type __r; (void)(id); __asm_volatile__(__ASMSYSCALL4(id,"") "movl %%eax, %0" : "=g" (__r) : "g" (__##arg1), "g" (__##arg2), "g" (__##arg3), "g" (__##arg4) : __ASMSYSCALL4_CLOBBER); return __r; }
#define __IDsyscall5(type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5) type name(type1 __##arg1, type2 __##arg2, type3 __##arg3, type4 __##arg4, type5 __##arg5) { type __r; (void)(id); __asm_volatile__(__ASMSYSCALL5(id,"") "movl %%eax, %0" : "=g" (__r) : "g" (__##arg1), "g" (__##arg2), "g" (__##arg3), "g" (__##arg4), "g" (__##arg5) : __ASMSYSCALL5_CLOBBER); return __r; }
#define __IDsyscall6(type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6) type name(type1 __##arg1, type2 __##arg2, type3 __##arg3, type4 __##arg4, type5 __##arg5, type6 __##arg6) { type __r; (void)(id); __asm_volatile__(__ASMSYSCALL6(id,"") "movl %%eax, %0" : "=g" (__r) : "g" (__##arg1), "g" (__##arg2), "g" (__##arg3), "g" (__##arg4), "g" (__##arg5), "g" (__##arg6) : __ASMSYSCALL6_CLOBBER); return __r; }
#define __IDsyscall7(type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6,type7,arg7) type name(type1 __##arg1, type2 __##arg2, type3 __##arg3, type4 __##arg4, type5 __##arg5, type6 __##arg6, type7 __##arg7) { type __r; (void)(id); __asm_volatile__(__ASMSYSCALL7(id,"") "movl %%eax, %0" : "=g" (__r) : "g" (__##arg1), "g" (__##arg2), "g" (__##arg3), "g" (__##arg4), "g" (__##arg5), "g" (__##arg6), "g" (__##arg7) : __ASMSYSCALL7_CLOBBER); return __r; }
#else /* __ANSI_COMPILER__ */
#define __IDsyscall0(type,name,id) type name() { type __r; (void)(id); __asm_volatile__(__ASMSYSCALL0(id,"") "movl %%eax, %0" : "=g" (__r) : : __ASMSYSCALL0_CLOBBER); return __r; }
#define __IDsyscall1(type,name,id,type1,arg1) type name(__/**/arg1) type1 __/**/arg1; { type __r; (void)(id); __asm_volatile__(__ASMSYSCALL1(id,"") "movl %%eax, %0" : "=g" (__r) : "g" (__/**/arg1) : __ASMSYSCALL1_CLOBBER); return __r; }
#define __IDsyscall2(type,name,id,type1,arg1,type2,arg2) type name(__/**/arg1,__/**/arg2) type1 __/**/arg1; type2 __/**/arg2; { type __r; (void)(id); __asm_volatile__(__ASMSYSCALL2(id,"") "movl %%eax, %0" : "=g" (__r) : "g" (__/**/arg1), "g" (__/**/arg2) : __ASMSYSCALL2_CLOBBER); return __r; }
#define __IDsyscall3(type,name,id,type1,arg1,type2,arg2,type3,arg3) type name(__/**/arg1,__/**/arg2,__/**/arg3) type1 __/**/arg1; type2 __/**/arg2; type3 __/**/arg3; { type __r; (void)(id); __asm_volatile__(__ASMSYSCALL3(id,"") "movl %%eax, %0" : "=g" (__r) : "g" (__/**/arg1), "g" (__/**/arg2), "g" (__/**/arg3) : __ASMSYSCALL3_CLOBBER); return __r; }
#define __IDsyscall4(type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4) type name(__/**/arg1,__/**/arg2,__/**/arg3,__/**/arg4) type1 __/**/arg1; type2 __/**/arg2; type3 __/**/arg3; type4 __/**/arg4; { type __r; (void)(id); __asm_volatile__(__ASMSYSCALL4(id,"") "movl %%eax, %0" : "=g" (__r) : "g" (__/**/arg1), "g" (__/**/arg2), "g" (__/**/arg3), "g" (__/**/arg4) : __ASMSYSCALL4_CLOBBER); return __r; }
#define __IDsyscall5(type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5) type name(__/**/arg1,__/**/arg2,__/**/arg3,__/**/arg4,__/**/arg5) type1 __/**/arg1; type2 __/**/arg2; type3 __/**/arg3; type4 __/**/arg4; type5 __/**/arg5; { type __r; (void)(id); __asm_volatile__(__ASMSYSCALL5(id,"") "movl %%eax, %0" : "=g" (__r) : "g" (__/**/arg1), "g" (__/**/arg2), "g" (__/**/arg3), "g" (__/**/arg4), "g" (__/**/arg5) : __ASMSYSCALL5_CLOBBER); return __r; }
#define __IDsyscall6(type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6) type name(__/**/arg1,__/**/arg2,__/**/arg3,__/**/arg4,__/**/arg5,__/**/arg6) type1 __/**/arg1; type2 __/**/arg2; type3 __/**/arg3; type4 __/**/arg4; type5 __/**/arg5; type6 __/**/arg6; { type __r; (void)(id); __asm_volatile__(__ASMSYSCALL6(id,"") "movl %%eax, %0" : "=g" (__r) : "g" (__/**/arg1), "g" (__/**/arg2), "g" (__/**/arg3), "g" (__/**/arg4), "g" (__/**/arg5), "g" (__/**/arg6) : __ASMSYSCALL6_CLOBBER); return __r; }
#define __IDsyscall7(type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6,type7,arg7) type name(__/**/arg1,__/**/arg2,__/**/arg3,__/**/arg4,__/**/arg5,__/**/arg6,__/**/arg7) type1 __/**/arg1; type2 __/**/arg2; type3 __/**/arg3; type4 __/**/arg4; type5 __/**/arg5; type6 __/**/arg6; type7 __/**/arg7; { type __r; (void)(id); __asm_volatile__(__ASMSYSCALL7(id,"") "movl %%eax, %0" : "=g" (__r) : "g" (__/**/arg1), "g" (__/**/arg2), "g" (__/**/arg3), "g" (__/**/arg4), "g" (__/**/arg5), "g" (__/**/arg6), "g" (__/**/arg7) : __ASMSYSCALL7_CLOBBER); return __r; }
#endif /* !__ANSI_COMPILER__ */
//[[[end]]]

#ifndef _syscall0
#ifdef __INTELLISENSE__
#define _syscall0(type,name)                                                                              type name(void) { (SYS_##name); }
#define _syscall1(type,name,type1,arg1)                                                                   type name(type1 arg1) { (SYS_##name); }
#define _syscall2(type,name,type1,arg1,type2,arg2)                                                        type name(type1 arg1, type2 arg2) { (SYS_##name); }
#define _syscall3(type,name,type1,arg1,type2,arg2,type3,arg3)                                             type name(type1 arg1, type2 arg2, type3 arg3) { (SYS_##name); }
#define _syscall4(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4)                                  type name(type1 arg1, type2 arg2, type3 arg3, type4 arg4) { (SYS_##name); }
#define _syscall5(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5)                       type name(type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5) { (SYS_##name); }
#define _syscall6(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6)            type name(type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6) { (SYS_##name); }
#define _syscall7(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6,type7,arg7) type name(type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6, type7 arg7) { (SYS_##name); }
#else
#define _syscall0(type,name)                                                                              __IDsyscall0(type,name,SYS_##name)
#define _syscall1(type,name,type1,arg1)                                                                   __IDsyscall1(type,name,SYS_##name,type1,arg1)
#define _syscall2(type,name,type1,arg1,type2,arg2)                                                        __IDsyscall2(type,name,SYS_##name,type1,arg1,type2,arg2)
#define _syscall3(type,name,type1,arg1,type2,arg2,type3,arg3)                                             __IDsyscall3(type,name,SYS_##name,type1,arg1,type2,arg2,type3,arg3)
#define _syscall4(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4)                                  __IDsyscall4(type,name,SYS_##name,type1,arg1,type2,arg2,type3,arg3,type4,arg4)
#define _syscall5(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5)                       __IDsyscall5(type,name,SYS_##name,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5)
#define _syscall6(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6)            __IDsyscall6(type,name,SYS_##name,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6)
#define _syscall7(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6,type7,arg7) __IDsyscall7(type,name,SYS_##name,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6,type7,arg7)
#endif
#endif /* !_syscall0 */

#ifdef __KERNEL__
struct kirq_registers;
extern void ksyscall_handler __P((struct kirq_registers *__regs));
#ifdef __KOS_HAVE_NTSYSCALL
extern void ksyscall_handler_nt __P((struct kirq_registers *__regs));
#endif
#ifdef __KOS_HAVE_NTSYSCALL
extern void ksyscall_handler_unix __P((struct kirq_registers *__regs));
#endif
#ifdef __MAIN_C__
extern void kernel_initialize_syscall __P((void));
#endif /* __MAIN_C__ */
#endif /* __KERNEL__ */
#endif /* !__ASSEMBLY__ */

__DECL_END

#endif /* !__KOS_SYSCALL_H__ */

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

//////////////////////////////////////////////////////////////////////////
// KOS System call conventions (__i386__):
// INTERRUPT NUMBER:       0x69 (69h)
// FUNCTION NUMBER:        %eax
// ARG #1:                 %ecx
// ARG #2:                 %edx
// ARG #3:                 %ebx
// ARG #4:                 %esi
// ARG #5:                 %edi
// ARG #6 .. ARG #*:     (([index]-6)*PS)(%esp)
// 1st RETURN REGISTER:    %eax
// 2nd RETURN REGISTER:    %ecx
// 3rd RETURN REGISTER:    %edx
// 4th RETURN REGISTER:    %ebx
// NOTES:
//   - When an unknown function is called, '%eax' (1st return register)
//     is set to 'KE_NOSYS', though it should be noted that this can
//     also happen if the system call does exist and simply chooses
//     to fail with that error.
#ifndef __ASSEMBLY__
#ifdef _MSC_VER
#define __ASMSYSCALL0(id,...)                                    __VA_ARGS__; mov eax, id; int __SYSCALL_INTNO;
#define __ASMSYSCALL1(id,arg1,...)                               __ASMSYSCALL0(id,                          mov ecx, arg1; __VA_ARGS__)
#define __ASMSYSCALL2(id,arg1,arg2,...)                          __ASMSYSCALL1(id,arg1,                     mov edx, arg2; __VA_ARGS__)
#define __ASMSYSCALL3(id,arg1,arg2,arg3,...)                     __ASMSYSCALL2(id,arg1,arg2,                mov ebx, arg3; __VA_ARGS__)
#define __ASMSYSCALL4(id,arg1,arg2,arg3,arg4,...)                __ASMSYSCALL3(id,arg1,arg2,arg3,           mov esi, arg4; __VA_ARGS__)
#define __ASMSYSCALL5(id,arg1,arg2,arg3,arg4,arg5,...)           __ASMSYSCALL4(id,arg1,arg2,arg3,arg4,      mov edi, arg5; __VA_ARGS__)
#define __ASMSYSCALL6(id,arg1,arg2,arg3,arg4,arg5,arg6,...)      __ASMSYSCALL5(id,arg1,arg2,arg3,arg4,arg5, pushl arg6;             __VA_ARGS__) addl esp, 4;
#define __ASMSYSCALL7(id,arg1,arg2,arg3,arg4,arg5,arg6,arg7,...) __ASMSYSCALL5(id,arg1,arg2,arg3,arg4,arg5, pushl arg7; pushl arg6; __VA_ARGS__) addl esp, 8;
#define __ASMSYSCALL0_DO(result,id)                                    __asm { __ASMSYSCALL0(id) movl result, eax; }
#define __ASMSYSCALL1_DO(result,id,arg1)                               __asm { __ASMSYSCALL1(id,arg1) movl result, eax; }
#define __ASMSYSCALL2_DO(result,id,arg1,arg2)                          __asm { __ASMSYSCALL2(id,arg1,arg2) movl result, eax; }
#define __ASMSYSCALL3_DO(result,id,arg1,arg2,arg3)                     __asm { __ASMSYSCALL3(id,arg1,arg2,arg3) movl result, eax; }
#define __ASMSYSCALL4_DO(result,id,arg1,arg2,arg3,arg4)                __asm { __ASMSYSCALL4(id,arg1,arg2,arg3,arg4) movl result, eax; }
#define __ASMSYSCALL5_DO(result,id,arg1,arg2,arg3,arg4,arg5)           __asm { __ASMSYSCALL5(id,arg1,arg2,arg3,arg4,arg5) movl result, eax; }
#define __ASMSYSCALL6_DO(result,id,arg1,arg2,arg3,arg4,arg5,arg6)      __asm { __ASMSYSCALL6(id,arg1,arg2,arg3,arg4,arg5,arg6) movl result, eax; }
#define __ASMSYSCALL7_DO(result,id,arg1,arg2,arg3,arg4,arg5,arg6,arg7) __asm { __ASMSYSCALL7(id,arg1,arg2,arg3,arg4,arg5,arg6,arg7) movl result, eax; }
#else
#define __ASMSYSCALL_DOINT    "int $" __PP_STR(__SYSCALL_INTNO) "\n"
#define __ASMSYSCALL0_DO(result,id)                                    __asm__(__ASMSYSCALL_DOINT : "=a" (result) : "a" (id))
#define __ASMSYSCALL1_DO(result,id,arg1)                               __asm__(__ASMSYSCALL_DOINT : "=a" (result) : "a" (id), "c" (arg1))
#define __ASMSYSCALL2_DO(result,id,arg1,arg2)                          __asm__(__ASMSYSCALL_DOINT : "=a" (result) : "a" (id), "c" (arg1), "d" (arg2))
#define __ASMSYSCALL3_DO(result,id,arg1,arg2,arg3)                     __asm__(__ASMSYSCALL_DOINT : "=a" (result) : "a" (id), "c" (arg1), "d" (arg2), "b" (arg3))
#define __ASMSYSCALL4_DO(result,id,arg1,arg2,arg3,arg4)                __asm__(__ASMSYSCALL_DOINT : "=a" (result) : "a" (id), "c" (arg1), "d" (arg2), "b" (arg3), "S" (arg4))
#define __ASMSYSCALL5_DO(result,id,arg1,arg2,arg3,arg4,arg5)           __asm__(__ASMSYSCALL_DOINT : "=a" (result) : "a" (id), "c" (arg1), "d" (arg2), "b" (arg3), "S" (arg4), "D" (arg5))
#define __ASMSYSCALL6_DO(result,id,arg1,arg2,arg3,arg4,arg5,arg6)      __asm__("pushl %7\n"           __ASMSYSCALL_DOINT "add $4, %%esp" : "=a" (result) : "a" (id), "c" (arg1), "d" (arg2), "b" (arg3), "S" (arg4), "D" (arg5), "" (arg6))
#define __ASMSYSCALL7_DO(result,id,arg1,arg2,arg3,arg4,arg5,arg6,arg7) __asm__("pushl %8\npushl %7\n" __ASMSYSCALL_DOINT "add $8, %%esp" : "=a" (result) : "a" (id), "c" (arg1), "d" (arg2), "b" (arg3), "S" (arg4), "D" (arg5), "" (arg6), "" (arg7))
#endif

/*[[[deemon
#include <util>
const MAX = 8;
function print_block(x,paste) {
  print "{ type __r; (void)(id); __ASMSYSCALL"+x+"_DO(__r,id",;
  for (local y: util::range(x)) print ",__"+paste+"arg"+(y+1),;
  print "); return __r; }";
  // print "{ type __r; (void)(id); __asm_volatile__(__ASMSYSCALL"+x+"(id,\"\")",;
  // print " \"movl %%eax, %0\" : \"=g\" (__r) :",;
  // for (local y: util::range(x)) print (y ? "," : "")+" \"g\" (__"+paste+"arg"+(y+1)+")",;
  // print " : __ASMSYSCALL"+x+"_CLOBBER); return __r; }";
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
#define __IDsyscall0(type,name,id) type name(void) { type __r; (void)(id); __ASMSYSCALL0_DO(__r,id); return __r; }
#define __IDsyscall1(type,name,id,type1,arg1) type name(type1 __##arg1) { type __r; (void)(id); __ASMSYSCALL1_DO(__r,id,__##arg1); return __r; }
#define __IDsyscall2(type,name,id,type1,arg1,type2,arg2) type name(type1 __##arg1, type2 __##arg2) { type __r; (void)(id); __ASMSYSCALL2_DO(__r,id,__##arg1,__##arg2); return __r; }
#define __IDsyscall3(type,name,id,type1,arg1,type2,arg2,type3,arg3) type name(type1 __##arg1, type2 __##arg2, type3 __##arg3) { type __r; (void)(id); __ASMSYSCALL3_DO(__r,id,__##arg1,__##arg2,__##arg3); return __r; }
#define __IDsyscall4(type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4) type name(type1 __##arg1, type2 __##arg2, type3 __##arg3, type4 __##arg4) { type __r; (void)(id); __ASMSYSCALL4_DO(__r,id,__##arg1,__##arg2,__##arg3,__##arg4); return __r; }
#define __IDsyscall5(type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5) type name(type1 __##arg1, type2 __##arg2, type3 __##arg3, type4 __##arg4, type5 __##arg5) { type __r; (void)(id); __ASMSYSCALL5_DO(__r,id,__##arg1,__##arg2,__##arg3,__##arg4,__##arg5); return __r; }
#define __IDsyscall6(type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6) type name(type1 __##arg1, type2 __##arg2, type3 __##arg3, type4 __##arg4, type5 __##arg5, type6 __##arg6) { type __r; (void)(id); __ASMSYSCALL6_DO(__r,id,__##arg1,__##arg2,__##arg3,__##arg4,__##arg5,__##arg6); return __r; }
#define __IDsyscall7(type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6,type7,arg7) type name(type1 __##arg1, type2 __##arg2, type3 __##arg3, type4 __##arg4, type5 __##arg5, type6 __##arg6, type7 __##arg7) { type __r; (void)(id); __ASMSYSCALL7_DO(__r,id,__##arg1,__##arg2,__##arg3,__##arg4,__##arg5,__##arg6,__##arg7); return __r; }
#else /* __ANSI_COMPILER__ */
#define __IDsyscall0(type,name,id) type name() { type __r; (void)(id); __ASMSYSCALL0_DO(__r,id); return __r; }
#define __IDsyscall1(type,name,id,type1,arg1) type name(__/**/arg1) type1 __/**/arg1; { type __r; (void)(id); __ASMSYSCALL1_DO(__r,id,__/**/arg1); return __r; }
#define __IDsyscall2(type,name,id,type1,arg1,type2,arg2) type name(__/**/arg1,__/**/arg2) type1 __/**/arg1; type2 __/**/arg2; { type __r; (void)(id); __ASMSYSCALL2_DO(__r,id,__/**/arg1,__/**/arg2); return __r; }
#define __IDsyscall3(type,name,id,type1,arg1,type2,arg2,type3,arg3) type name(__/**/arg1,__/**/arg2,__/**/arg3) type1 __/**/arg1; type2 __/**/arg2; type3 __/**/arg3; { type __r; (void)(id); __ASMSYSCALL3_DO(__r,id,__/**/arg1,__/**/arg2,__/**/arg3); return __r; }
#define __IDsyscall4(type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4) type name(__/**/arg1,__/**/arg2,__/**/arg3,__/**/arg4) type1 __/**/arg1; type2 __/**/arg2; type3 __/**/arg3; type4 __/**/arg4; { type __r; (void)(id); __ASMSYSCALL4_DO(__r,id,__/**/arg1,__/**/arg2,__/**/arg3,__/**/arg4); return __r; }
#define __IDsyscall5(type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5) type name(__/**/arg1,__/**/arg2,__/**/arg3,__/**/arg4,__/**/arg5) type1 __/**/arg1; type2 __/**/arg2; type3 __/**/arg3; type4 __/**/arg4; type5 __/**/arg5; { type __r; (void)(id); __ASMSYSCALL5_DO(__r,id,__/**/arg1,__/**/arg2,__/**/arg3,__/**/arg4,__/**/arg5); return __r; }
#define __IDsyscall6(type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6) type name(__/**/arg1,__/**/arg2,__/**/arg3,__/**/arg4,__/**/arg5,__/**/arg6) type1 __/**/arg1; type2 __/**/arg2; type3 __/**/arg3; type4 __/**/arg4; type5 __/**/arg5; type6 __/**/arg6; { type __r; (void)(id); __ASMSYSCALL6_DO(__r,id,__/**/arg1,__/**/arg2,__/**/arg3,__/**/arg4,__/**/arg5,__/**/arg6); return __r; }
#define __IDsyscall7(type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6,type7,arg7) type name(__/**/arg1,__/**/arg2,__/**/arg3,__/**/arg4,__/**/arg5,__/**/arg6,__/**/arg7) type1 __/**/arg1; type2 __/**/arg2; type3 __/**/arg3; type4 __/**/arg4; type5 __/**/arg5; type6 __/**/arg6; type7 __/**/arg7; { type __r; (void)(id); __ASMSYSCALL7_DO(__r,id,__/**/arg1,__/**/arg2,__/**/arg3,__/**/arg4,__/**/arg5,__/**/arg6,__/**/arg7); return __r; }
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
#else /* !__ASSEMBLY__ */
#define __IDsyscall0(type,name,id)                                                                              /* nothing */
#define __IDsyscall1(type,name,id,type1,arg1)                                                                   /* nothing */
#define __IDsyscall2(type,name,id,type1,arg1,type2,arg2)                                                        /* nothing */
#define __IDsyscall3(type,name,id,type1,arg1,type2,arg2,type3,arg3)                                             /* nothing */
#define __IDsyscall4(type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4)                                  /* nothing */
#define __IDsyscall5(type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5)                       /* nothing */
#define __IDsyscall6(type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6)            /* nothing */
#define __IDsyscall7(type,name,id,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6,type7,arg7) /* nothing */
#define _syscall0(type,name)                                                                              /* nothing */
#define _syscall1(type,name,type1,arg1)                                                                   /* nothing */
#define _syscall2(type,name,type1,arg1,type2,arg2)                                                        /* nothing */
#define _syscall3(type,name,type1,arg1,type2,arg2,type3,arg3)                                             /* nothing */
#define _syscall4(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4)                                  /* nothing */
#define _syscall5(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5)                       /* nothing */
#define _syscall6(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6)            /* nothing */
#define _syscall7(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6,type7,arg7) /* nothing */
#endif /* __ASSEMBLY__ */

__DECL_END

#endif /* !__KOS_SYSCALL_H__ */

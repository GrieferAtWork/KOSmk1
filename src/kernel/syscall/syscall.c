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

#include "syscall-common.h"
#include <assert.h>
#include <fcntl.h>
#include <kos/kernel/interrupts.h>
#include <kos/syslog.h>
#include <kos/kernel/pageframe.h>
#include <kos/kernel/proc.h>
#include <kos/kernel/serial.h>
#include <kos/kernel/task.h>
#include <kos/kernel/time.h>
#include <kos/kernel/tty.h>
#include <kos/syscall.h>
#include <kos/syscallno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#ifdef __KOS_HAVE_NTSYSCALL
#include <windows/syscall.h>
#endif
#ifdef __KOS_HAVE_UNIXSYSCALL
#include <linux/syscall.h>
#endif

__DECL_BEGIN

void kernel_initialize_syscall(void) {
 set_irq_handler(__SYSCALL_INTNO,&ksyscall_handler);
#ifdef __KOS_HAVE_NTSYSCALL
 set_irq_handler(__NTSYSCALL_INTNO,&ksyscall_handler_nt);
#endif
#ifdef __KOS_HAVE_UNIXSYSCALL
 set_irq_handler(__UNIXSYSCALL_INTNO,&ksyscall_handler_unix);
#endif
}

static struct ksysgroup {
 size_t            callc;
 ksyscall_t const *callv;
} const sysgroups[] = {
#ifdef __INTELLISENSE__
#define GROUP(id,symbol)        {__compiler_ARRAYSIZE(symbol),symbol}
#else
#define GROUP(id,symbol) [id] = {__compiler_ARRAYSIZE(symbol),symbol}
#endif
 GROUP(SYSCALL_GROUP_GENERIC,  sys_generic),
 GROUP(SYSCALL_GROUP_FILEDESCR,sys_kfd),
 GROUP(SYSCALL_GROUP_FILESYS,  sys_kfs),
 GROUP(SYSCALL_GROUP_TIME,     sys_ktm),
 GROUP(SYSCALL_GROUP_MEM,      sys_kmem),
 GROUP(SYSCALL_GROUP_TASK,     sys_ktask),
 GROUP(SYSCALL_GROUP_PROC,     sys_kproc),
 GROUP(SYSCALL_GROUP_MOD,      sys_mod),
#undef GROUP
};

#ifdef __DEBUG__
void ksyscall_trace(struct kirq_registers const *regs) {
 uintptr_t *uargvec = NULL; int argi = -1;
 uargvec = (uintptr_t *)kpagedir_translate(kpagedir_user(),
                                          (void *)(uintptr_t)regs->regs.ebp);
#define _A(i) \
 ((i) == 0 ? (uintptr_t)regs->regs.ebx :\
  (i) == 1 ? (uintptr_t)regs->regs.ecx :\
  (i) == 2 ? (uintptr_t)regs->regs.edx :\
   uargvec ? uargvec[(i)-3] : 0)
#define _I8   (++argi,(__u8)_A(argi))
#define _I16  (++argi,(__u16)_A(argi))
#define _I32  (++argi,(__u32)_A(argi))
#if __SIZEOF_POINTER__ <= 8
#if (__BYTE_ORDER == __LITTLE_ENDIAN) == defined(KOS_ARCH_STACK_GROWS_DOWNWARDS)
#define _I64  (argi += 2,((__u64)_A(argi-1) << 32) | (__u64)_A(argi))
#else /* linear: down */
#define _I64  (argi += 2,((__u64)_A(argi-1) | (__u64)_A(argi) << 32))
#endif /* linear: up */
#else
#define _I64  (++argi,(__u64)_A(argi))
#endif
#define __b(bits) __PRIVATE_PP_CAT_2(_I,bits)
#define __x(size) __b(__PP_MUL8(size))
#define _I    __x(__SIZEOF_POINTER__)
#define _d    __x(__SIZEOF_INT__)
#define _o    _d
#define _u    _d
#define _x    _d
#define _p    _I
#define _s    TRANSLATE((void *)_I)
 struct ktask *caller = ktask_self();
 printf("SYSCALL[%I32d:%Iu:%s] ",caller->t_proc->p_pid,
        caller->t_tid,ktask_getname(caller));
 switch (regs->regs.eax) {
#define CASE0(name,fmt)               case SYS_##name: { printf(#name fmt "\n"); } break
#define CASE1(name,fmt,a)             case SYS_##name: { __typeof__(a) a_ = (a); printf(#name fmt "\n",a_); } break
#define CASE2(name,fmt,a,b)           case SYS_##name: { __typeof__(a) a_ = (a); __typeof__(b) b_ = (b); printf(#name fmt "\n",a_,b_); } break
#define CASE3(name,fmt,a,b,c)         case SYS_##name: { __typeof__(a) a_ = (a); __typeof__(b) b_ = (b); __typeof__(c) c_ = (c); printf(#name fmt "\n",a_,b_,c_); } break
#define CASE4(name,fmt,a,b,c,d)       case SYS_##name: { __typeof__(a) a_ = (a); __typeof__(b) b_ = (b); __typeof__(c) c_ = (c); __typeof__(d) d_ = (d); printf(#name fmt "\n",a_,b_,c_,d_); } break
#define CASE5(name,fmt,a,b,c,d,e)     case SYS_##name: { __typeof__(a) a_ = (a); __typeof__(b) b_ = (b); __typeof__(c) c_ = (c); __typeof__(d) d_ = (d); __typeof__(e) e_ = (e); printf(#name fmt "\n",a_,b_,c_,d_,e_); } break
#define CASE6(name,fmt,a,b,c,d,e,f)   case SYS_##name: { __typeof__(a) a_ = (a); __typeof__(b) b_ = (b); __typeof__(c) c_ = (c); __typeof__(d) d_ = (d); __typeof__(e) e_ = (e); __typeof__(f) f_ = (f); printf(#name fmt "\n",a_,b_,c_,d_,e_,f_); } break
#define CASE7(name,fmt,a,b,c,d,e,f,g) case SYS_##name: { __typeof__(a) a_ = (a); __typeof__(b) b_ = (b); __typeof__(c) c_ = (c); __typeof__(d) d_ = (d); __typeof__(e) e_ = (e); __typeof__(f) f_ = (f); __typeof__(g) g_ = (g); printf(#name fmt "\n",a_,b_,c_,d_,e_,f_,g_); } break
#define ARGS0()              "()"
#define ARGS1(a)             "(" a ")"
#define ARGS2(a,b)           "(" a ", " b ")"
#define ARGS3(a,b,c)         "(" a ", " b ", " c ")"
#define ARGS4(a,b,c,d)       "(" a ", " b ", " c ", " d ")"
#define ARGS5(a,b,c,d,e)     "(" a ", " b ", " c ", " d ", " e ")"
#define ARGS6(a,b,c,d,e,f)   "(" a ", " b ", " c ", " d ", " e ", " f ")"
#define ARGS7(a,b,c,d,e,f,g) "(" a ", " b ", " c ", " d ", " e ", " f ", " g ")"
#define X(name,fmt) name " = " fmt
  CASE2(k_syslog,             ARGS2(X("s","'%s'"),X("maxlen","%Iu")),_s,_I);
  CASE1(k_sysconf,         ARGS1(X("name","%d")),_d);
  CASE5(kfd_open,          ARGS5(X("cwd","%d"),X("path","'%s'"),X("pathmax","%Iu"),X("mode","%x"),X("perms","%x")),_d,_s,_I,_x,_x);
  CASE6(kfd_open2,         ARGS6(X("dfd","%d"),X("cwd","%d"),X("path","'%s'"),X("pathmax","%Iu"),X("mode","%x"),X("perms","%x")),_d,_d,_s,_I,_x,_x);
  CASE1(kfd_close,         ARGS1(X("fd","%d")),_d);
  CASE4(kfd_seek,          ARGS4(X("fd","%d"),X("off","%I64d"),X("whence","%d"),X("newpos","%p")),_d,_I64,_d,_p);
  CASE4(kfd_read,          ARGS4(X("fd","%d"),X("buf","%p"),X("bufsize","%Iu"),X("rsize","%p")),_d,_p,_I,_p);
  CASE4(kfd_write,         ARGS4(X("fd","%d"),X("buf","%p"),X("bufsize","%Iu"),X("wsize","%p")),_d,_p,_I,_p);
  CASE5(kfd_pread,         ARGS5(X("fd","%d"),X("pos","%I64u"),X("buf","%p"),X("bufsize","%Iu"),X("rsize","%p")),_d,_I64,_p,_I,_p);
  CASE5(kfd_pwrite,        ARGS5(X("fd","%d"),X("pos","%I64u"),X("buf","%p"),X("bufsize","%Iu"),X("wsize","%p")),_d,_I64,_p,_I,_p);
  CASE1(kfd_flush,         ARGS1(X("fd","%d")),_d);
  CASE2(kfd_trunc,         ARGS2(X("fd","%d"),X("size","%I64u")),_d,_I64);
  CASE3(kfd_fcntl,         ARGS3(X("fd","%d"),X("cmd","%d"),X("arg","%p")),_d,_d,_p);
  CASE3(kfd_ioctl,         ARGS3(X("fd","%d"),X("attr","%d"),X("cmd","%p")),_d,_d,_p);
  CASE5(kfd_getattr,       ARGS5(X("fd","%d"),X("attr","%d"),X("buf","%p"),X("bufsize","%Iu"),X("reqsize","%p")),_d,_d,_p,_I,_p);
  CASE4(kfd_setattr,       ARGS4(X("fd","%d"),X("attr","%d"),X("buf","%p"),X("bufsize","%Iu")),_d,_d,_p,_I);
  CASE3(kfd_readdir,       ARGS3(X("fd","%d"),X("dent","%p"),X("flags","%I32u")),_d,_p,_I32);
  CASE2(kfd_dup,           ARGS2(X("fd","%d"),X("flags","%x")),_d,_x);
  CASE3(kfd_dup2,          ARGS3(X("fd","%d"),X("dfd","%d"),X("flags","%x")),_d,_d,_x);
  CASE2(kfd_pipe,          ARGS2(X("fd","%p"),X("flags","%x")),_p,_x);
  CASE2(kfd_equals,        ARGS2(X("fda","%d"),X("fdb","%d")),_d,_d);
  CASE4(kfs_mkdir,         ARGS4(X("dirfd","%d"),X("path","'%s'"),X("pathmax","%Iu"),X("perms","%o")),_d,_s,_I,_o);
  CASE3(kfs_rmdir,         ARGS3(X("dirfd","%d"),X("path","'%s'"),X("pathmax","%Iu")),_d,_s,_I);
  CASE3(kfs_unlink,        ARGS3(X("dirfd","%d"),X("path","'%s'"),X("pathmax","%Iu")),_d,_s,_I);
  CASE3(kfs_remove,        ARGS3(X("dirfd","%d"),X("path","'%s'"),X("pathmax","%Iu")),_d,_s,_I);
  CASE5(kfs_symlink,       ARGS5(X("dirfd","%d"),X("target","'%s'"),X("targetmax","%Iu"),X("lnk","'%s'"),X("lnkmax","%Iu")),_d,_s,_I,_s,_I);
  CASE5(kfs_hrdlink,       ARGS5(X("dirfd","%d"),X("target","'%s'"),X("targetmax","%Iu"),X("lnk","'%s'"),X("lnkmax","%Iu")),_d,_s,_I,_s,_I);
  CASE1(ktime_getnow,      ARGS1(X("ptm","%p")),_p);
  CASE1(ktime_setnow,      ARGS1(X("ptm","%p")),_p);
  CASE1(ktime_getcpu,      ARGS1(X("ptm","%p")),_p);
  CASE6(kmem_map,          ARGS6(X("hint","%p"),X("length","%Iu"),X("prot","%d"),X("flags","%d"),X("fd","%d"),X("offset","%I64u")),_p,_I,_d,_d,_d,_I64);
  CASE2(kmem_unmap,        ARGS2(X("addr","%p"),X("length","%Iu")),_p,_I);
  CASE2(kmem_validate,     ARGS2(X("addr","%p"),X("bytes","%Iu")),_p,_I);
  CASE0(ktask_yield,       ARGS0());
  CASE3(ktask_setalarm,    ARGS3(X("task","%d"),X("abstime","%p"),X("oldabstime","%p")),_d,_p,_p);
  CASE2(ktask_getalarm,    ARGS2(X("task","%d"),X("abstime","%p")),_d,_p);
  CASE2(ktask_abssleep,    ARGS2(X("task","%d"),X("abstime","%p")),_d,_p);
  CASE3(ktask_terminate,   ARGS3(X("task","%d"),X("exitcode","%p"),X("flags","%I32u")),_d,_p,_I32);
  CASE2(ktask_suspend,     ARGS2(X("task","%d"),X("flags","%I32u")),_d,_I32);
  CASE2(ktask_resume,      ARGS2(X("task","%d"),X("flags","%I32u")),_d,_I32);
  CASE1(ktask_openroot,    ARGS1(X("task","%d")),_d);
  CASE1(ktask_openparent,  ARGS1(X("task","%d")),_d);
  CASE1(ktask_openproc,    ARGS1(X("task","%d")),_d);
  CASE1(ktask_getparid,    ARGS1(X("task","%d")),_d);
  CASE1(ktask_gettid,      ARGS1(X("task","%d")),_d);
  CASE2(ktask_openchild,   ARGS2(X("task","%d"),X("parid","%Iu")),_d,_I);
  CASE4(ktask_enumchildren,ARGS4(X("task","%d"),X("idv","%p"),X("idc","%Iu"),X("reqidc","%p")),_d,_p,_I,_p);
  CASE2(ktask_getpriority, ARGS2(X("task","%d"),X("result","%p")),_d,_p);
  CASE2(ktask_setpriority, ARGS2(X("task","%d"),X("value","%I32d")),_d,_I32);
  CASE2(ktask_join,        ARGS2(X("task","%d"),X("exitcode","%p")),_d,_p);
  CASE2(ktask_tryjoin,     ARGS2(X("task","%d"),X("exitcode","%p")),_d,_p);
  CASE3(ktask_timedjoin,   ARGS3(X("task","%d"),X("abstime","%p"),X("exitcode","%p")),_d,_p,_p);
  CASE3(ktask_newthread,   ARGS3(X("thread_main","%p"),X("buf","%p"),X("flags","%I32u")),_p,_p,_I32);
  CASE4(ktask_newthreadi,  ARGS4(X("thread_main","%p"),X("buf","%p"),X("bufsize","%Iu"),X("flags","%I32u")),_p,_p,_I,_I32);
  CASE3(ktask_fork,      ARGS3(X("childfd","%p"),X("login","%p"),X("flags","%I32u")),_p,_p,_I32);
  CASE1(ktask_gettls,      ARGS1(X("slot","%Iu")),_I);
  CASE3(ktask_settls,      ARGS3(X("slot","%Iu"),X("value","%p"),X("oldvalue","%p")),_I,_p,_p);
  CASE2(ktask_gettlsof,    ARGS2(X("task","%d"),X("slot","%Iu")),_d,_I);
  CASE4(ktask_settlsof,    ARGS4(X("task","%d"),X("slot","%Iu"),X("value","%p"),X("oldvalue","%p")),_d,_I,_p,_p);
  CASE4(kproc_enumfd,      ARGS4(X("proc","%d"),X("fdv","%p"),X("fdc","%Iu"),X("reqfdc","%p")),_d,_p,_I,_p);
  CASE3(kproc_openfd,      ARGS3(X("proc","%d"),X("fd","%d"),X("flags","%x")),_d,_d,_x);
  CASE4(kproc_openfd2,     ARGS4(X("proc","%d"),X("fd","%d"),X("newfd","%d"),X("flags","%x")),_d,_d,_d,_x);
  CASE1(kproc_barrier,     ARGS1(X("level","%x")),_x);
  CASE2(kproc_openbarrier, ARGS2(X("proc","%d"),X("level","%x")),_d,_x);
  CASE1(kproc_alloctls,    ARGS1(X("result","%p")),_p);
  CASE1(kproc_freetls,     ARGS1(X("slot","%Iu")),_I);
  CASE4(kproc_enumtls,     ARGS4(X("proc","%d"),X("tlsv","%p"),X("tlsc","%Iu"),X("reqtlsc","%p")),_d,_p,_I,_p);
  CASE3(kproc_enumpid,     ARGS3(X("pidv","%p"),X("pidc","%Iu"),X("reqpidc","%p")),_p,_I,_p);
  CASE1(kproc_openpid,     ARGS1(X("pid","%I32d")),_I32);
  CASE1(kproc_getpid,      ARGS1(X("proc","%d")),_d);
  CASE4(kmod_open,         ARGS4(X("name","%p"),X("namemax","%Iu"),X("modid","%p"),X("flags","I32u")),_p,_I,_p,_I32);
  CASE1(kmod_close,        ARGS1(X("modid","%Iu")),_I);
  CASE3(kmod_sym,          ARGS3(X("modid","%Iu"),X("name","%p"),X("namemax","%Iu")),_I,_I,_p);
  CASE5(kmod_info,         ARGS5(X("modid","%Iu"),X("buf","%p"),X("bufsize","%Iu"),X("reqsize","%p"),X("flags","%I32u")),_I32,_p,_I,_p,_I32);
#undef ARGS7
#undef ARGS6
#undef ARGS5
#undef ARGS4
#undef ARGS3
#undef ARGS2
#undef ARGS1
#undef ARGS0
#undef CASE7
#undef CASE6
#undef CASE5
#undef CASE4
#undef CASE3
#undef CASE2
#undef CASE1
#undef CASE0
  default: printf("?:%I32x(...)\n",regs->regs.eax); break;
 }
#undef _s
#undef _p
#undef _x
#undef _u
#undef _o
#undef _d
#undef _I
#undef __x
#undef __b
#undef _I64
#undef _I32
#undef _I16
#undef _I8
#undef _A
}
#endif


void ksyscall_handler(struct kirq_registers *regs) {
 ksyscall_t call; __u32 id = SYSID;
 struct ksysgroup const *group;
#ifdef __DEBUG__
 __u32 original_id = id;
 if (k_sysloglevel >= KLOG_INSANE) ksyscall_trace(regs);
 assertf((ktask_self()->t_flags&(KTASK_FLAG_USERTASK|KTASK_FLAG_CRITICAL)) !=
                                (KTASK_FLAG_USERTASK|KTASK_FLAG_CRITICAL)
        ,"User-task still critical after syscall return (syscall = %I32x)"
        ,original_id);
#endif
 //printf("ksyscall_handler::begin(%I32x)\n",id);
 if __unlikely(__SYSCALL_GROUP(id) >= __compiler_ARRAYSIZE(sysgroups)) goto noavail;
 group = &sysgroups[__SYSCALL_GROUP(id)];
 if __unlikely((id = __SYSCALL_ID(id)) >= group->callc) goto noavail;
 if __unlikely((call = group->callv[id]) == NULL) goto noavail;
 (*call)(regs);
 //printf("ksyscall_handler::end()\n");
#ifdef __DEBUG__
 assertf((ktask_self()->t_flags&(KTASK_FLAG_USERTASK|KTASK_FLAG_NOINTR)) !=
                                (KTASK_FLAG_USERTASK|KTASK_FLAG_NOINTR)
        ,"User-task still nointr after syscall return (syscall = %I32x)"
        ,original_id);
 assertf((ktask_self()->t_flags&(KTASK_FLAG_USERTASK|KTASK_FLAG_CRITICAL)) !=
                                (KTASK_FLAG_USERTASK|KTASK_FLAG_CRITICAL)
        ,"User-task still critical after syscall return (syscall = %I32x)"
        ,original_id);
#endif
 return;
noavail:
// return;
//  switch (SYSID) {
//   case 100: {
//    LOAD4(int,K(a),
//          int,K(b),
//          int,K(c),
//          int,K(d));
//    printf("test() : a : %d\n",a);
//    printf("test() : b : %d\n",b);
//    printf("test() : c : %d\n",c);
//    printf("test() : d : %d\n",d);
//    RETURN(KE_OK);
//   } break;
//   default: break;
//  }
// printf("NOAVAIL()\n");
 RETURN(KE_NOSYS);
}


#ifndef __INTELLISENSE__
#include "syscall-generic.c.inl"
#include "syscall-kfd.c.inl"
#include "syscall-kfs.c.inl"
#include "syscall-kmem.c.inl"
#include "syscall-ktask.c.inl"
#include "syscall-kproc.c.inl"
#include "syscall-ktime.c.inl"
#include "syscall-mod.c.inl"

#include "syscall-nt.c.inl"
#include "syscall-unix.c.inl"
#endif

__DECL_END

#endif /* !__KOS_KERNEL_SYSCALL_SYSCALL_C__ */

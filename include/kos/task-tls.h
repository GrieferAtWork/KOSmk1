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
#ifndef __KOS_TASK_TLS_H__
#define __KOS_TASK_TLS_H__ 1

#include <kos/config.h>
#include <kos/compiler.h>
#include <kos/asm.h>
#include <kos/timespec.h>
#ifndef __KERNEL__
#ifndef __NO_PROTOTYPES
#include <kos/errno.h>
#include <kos/types.h>
#include <kos/syscall.h>
#endif /* !__NO_PROTOTYPES */
#endif /* !__KERNEL__ */

__DECL_BEGIN

#define KEXINFO_PTR_COUNT         4
#define KEXINFO_OFFSETOF_NO       0
#define KEXINFO_OFFSETOF_INFO     4
#define KEXINFO_OFFSETOF_PTR(i)  (8+(i)*__SIZEOF_POINTER__)
#define KEXINFO_SIZEOF            KEXINFO_OFFSETOF_PTR(KEXINFO_PTR_COUNT)
#ifndef __ASSEMBLY__
struct __packed kexinfo {
 __u32 ex_no;                     /*< [type(kexno_t)] Exception number. */
 __u32 ex_info;                   /*< Exception-specific information detail. */
 void *ex_ptr[KEXINFO_PTR_COUNT]; /*< Exception-specific additional pointers/numbers. */
};
#endif /* !__ASSEMBLY__ */

#ifdef __i386__
#define KEXSTATE_SIZEOF          40
#define KEXSTATE_OFFSETOF_EDI    0
#define KEXSTATE_OFFSETOF_ESI    4
#define KEXSTATE_OFFSETOF_EBP    8
#define KEXSTATE_OFFSETOF_ESP    12
#define KEXSTATE_OFFSETOF_EBX    16
#define KEXSTATE_OFFSETOF_EDX    20
#define KEXSTATE_OFFSETOF_ECX    24
#define KEXSTATE_OFFSETOF_EAX    28
#define KEXSTATE_OFFSETOF_EIP    32
#define KEXSTATE_OFFSETOF_EFLAGS 36
#ifndef __ASSEMBLY__
struct __packed kexstate { __u32 edi,esi,ebp,esp,ebx,edx,ecx,eax,eip,eflags; };
#endif /* !__ASSEMBLY__ */
#else /* __i386__ */
#error "FIXME"
#endif /* !__i386__ */

#ifndef __ASSEMBLY__
typedef void (*__noreturn kexhandler_t)();
struct kexrecord {
 /* Exception handler. */
 __user struct kexrecord *eh_prev;    /*< [0..1] Previous exception handler. */
 kexhandler_t             eh_handler; /*< [1..1] Exception handler. */
};
#endif /* !__ASSEMBLY__ */

#define KUTHREAD_NAMEMAX  47 /*< Amount of leading characters to provide to ring-#3 code through uthread. */

#define KUTHREAD_OFFSETOF_SELF           0
#define KUTHREAD_OFFSETOF_PARENT        (__SIZEOF_POINTER__)
#define KUTHREAD_OFFSETOF_EXH           (__SIZEOF_POINTER__*2)
#define KUTHREAD_OFFSETOF_STACKBEGIN    (__SIZEOF_POINTER__*3)
#define KUTHREAD_OFFSETOF_STACKEND      (__SIZEOF_POINTER__*4)
#define KUTHREAD_OFFSETOF_ERRNO         (__SIZEOF_POINTER__*16)
#define KUTHREAD_OFFSETOF_PID           (__SIZEOF_POINTER__*16+__SIZEOF_INT__)
#define KUTHREAD_OFFSETOF_TID           (__SIZEOF_POINTER__*16+__SIZEOF_INT__+4)
#define KUTHREAD_OFFSETOF_PARID         (__SIZEOF_POINTER__*16+__SIZEOF_INT__+4+__SIZEOF_SIZE_T__)
#define KUTHREAD_OFFSETOF_START         (__SIZEOF_POINTER__*16+__SIZEOF_INT__+4+__SIZEOF_SIZE_T__*2)
#define KUTHREAD_OFFSETOF_NAME          (__SIZEOF_POINTER__*32)
#define KUTHREAD_OFFSETOF_ZERO          (__SIZEOF_POINTER__*32+KUTHREAD_NAMEMAX)
#define KUTHREAD_OFFSETOF_EXINFO        (__SIZEOF_POINTER__*32+KUTHREAD_NAMEMAX+1)
#define KUTHREAD_OFFSETOF_EXSTATE       (__SIZEOF_POINTER__*32+KUTHREAD_NAMEMAX+1+KEXINFO_SIZEOF)
#ifndef __ASSEMBLY__
#ifndef __kuthread_defined
#define __kuthread_defined 1
struct __packed kuthread {
 /* This structure is located at TLS offset ZERO, and actual TLS variables as located below. */
 /* Per-thread user structure at positive offsets. */
 /* NOTE: Upon startup, the kernel will initialize any unused & future fields to ZERO(0). */
 __user struct kuthread   *u_self;         /*< [1..1] Structure self-pointer (NOTE: Required by i386/x86-64 compilers to take the address of TLS variables). */
 __user struct kuthread   *u_parent;       /*< [0..1] Userthread control block of this thread's parent (or NULL if it has no parent, or that parent is part of a different process). */
 __user struct kexrecord  *u_exh;          /*< [0..1] Stack of active exception handlers. */
 __user void              *u_stackbegin;   /*< [1..1] First valid memory address associated with the stack. */
 __user void              *u_stackend;     /*< [1..1] First invalid memory address no longer associated with the stack. */
 __user void              *u_padding1[11]; /*< Padding data. */
 /* OFFSET: 16*__SIZEOF_POINTER__ */
union __packed { struct __packed {
 int                       u_errno;        /*< Used by libc: the 'errno' variable (Placed here to prevent overhead from placing it in an ELF TLS block). */
 __pid_t                   u_pid;          /*< The ID of the process (unless changed: 'kproc_getpid(kproc_self())') */
 __ktid_t                  u_tid;          /*< The ID of the thread (unless changed: 'ktask_gettid(ktask_self())') */
 __size_t                  u_parid;        /*< The ID of the thread within its parent (unless changed: 'ktask_getparid(ktask_self())') */
 struct timespec           u_start;        /*< Timestamp for when the thread was launched. */
}; __user void            *u_padding2[16]; /*< Padding data. */};
 /* OFFSET: 32*__SIZEOF_POINTER__ */
 char                      u_name[KUTHREAD_NAMEMAX]; /*< First 'KUTHREAD_NAMEMAX' characters from the thread's name (padded with ZEROes).
                                                        (Included to provide a standardized, addressable thread name string. - Use this for debug output!) */
 char                      u_zero;         /*< Always zero */
 struct kexinfo            u_exinfo;       /*< Info about the last occurred exception. */
 struct kexstate           u_exstate;      /*< CPU register state when the last exception occurred. */
 /* More data can easily be here (The kernel internally aligns sizeof(kuthread) with PAGESIZE)... */
};
#endif /* !__kuthread_defined */

#ifdef __INTELLISENSE__
#ifdef __PROC_TLS_H__
extern struct kuthread *const _tls_self;
#endif
#else
#ifdef _tls_addr
#define _tls_self  ((struct kuthread *)_tls_addr)
#endif /* _tls_addr */
#endif

struct ktlsinfo {
 __ptrdiff_t ti_offset; /*< TLS offset. */
 __size_t    ti_size;   /*< TLS block size (in bytes). */
};



#ifndef __KERNEL__
#ifndef __NO_PROTOTYPES

//////////////////////////////////////////////////////////////////////////
// Allocate/Free dynamic, thread-local storage memory.
// Using 'kproc_tlsalloc', you can allocate a new block of thread-local storage
// that can be accessed in any thread using the arch-specific TLS register (%fs/%gs).
// The offset that must be added to access the TLS address is then stored in 'tls_offset'.
// A TLS block can later be freed using 'kproc_tlsfree', by specifying
// the same offset as previously returned in a call to 'kproc_tlsalloc'.
// NOTE: The returned offset is always page-aligned, meaning you
//       can only allocate with an alignment of (usually 4096)
// @param: TEMPLATE: A template data block that will be used to initialize TLS blocks.
//             NOTE: The data pointed to here will be copied during this call,
//                   meaning you won't have to keep the template around.
//             HINT: Passing NULL for 'TEMPLATE' causes the kernel to
//                   generate a template filled with nothing but ZEROes.
// @param: TEMPLATE_SIZE: The size of the given template, or to be more precise:
//                        the amount of TLS memory to allocate.
// @param: TLS_OFFSET: Output parameter to store the newly allocated TLS offset inside of.
// @return: KE_OK:    Successfully allocated a new TLS block.
// @return: KE_NOMEM: Not enough available memory to complete the operation.
__local _syscall1(kerrno_t,kproc_tlsfree,__ptrdiff_t,tls_offset);
__local kerrno_t
kproc_tlsalloc(void const *__template,
               __size_t __template_size,
               __ptrdiff_t *__restrict __tls_offset) {
 kerrno_t __error;
 __asm__("int $" __PP_STR(__SYSCALL_INTNO) "\n"
         : "=a" (__error), "=c" (*__tls_offset)
         : "a" (SYS_kproc_tlsalloc)
         , "c" (__template)
         , "d" (__template_size));
 return __error;
}

//////////////////////////////////////////////////////////////////////////
// Enumerate all TLS offsets allocated to the calling process.
// Following usual conventions, an optional output parameter 'reqoffc'
// may be specified to return the actual required amount of offsets.
// @return: KE_OK:        Successfully stored up to 'OFFC' offsets in 'OFFV'
// @return: KE_FAULT:     The given OFFV pointer is faulty.
// @return: KE_DESTROYED: The TLS sub-system of the calling process was closed.
__local kerrno_t
kproc_tlsenum(struct ktlsinfo *__restrict __infov,
              __size_t __infoc, __size_t *__restrict __reqinfoc) {
 kerrno_t __error;
 __size_t __val_reqinfoc;
 __asm__("int $" __PP_STR(__SYSCALL_INTNO) "\n"
         : "=a" (__error), "=c" (__val_reqinfoc)
         : "a" (SYS_kproc_tlsenum), "c" (__infov), "d" (__infoc));
 if (__reqinfoc) *__reqinfoc = __val_reqinfoc;
 return __error;
}

#endif /* !__ASSEMBLY__ */
#endif /* !__NO_PROTOTYPES */
#endif /* !__KERNEL__ */

__DECL_END

#endif /* !__KOS_TASK_TLS_H__ */

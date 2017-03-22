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
#include <kos/asm.h>
#include <kos/config.h>
#ifdef __KOS_HAVE_SYSCALL
#include <kos/syscall.h>
#endif
#ifdef __KOS_HAVE_NTSYSCALL
#include <windows/syscall.h>
#endif
#ifdef __KOS_HAVE_UNIXSYSCALL
#include <linux/syscall.h>
#endif

/* KOS System call.
 * >> extern long syscall(long sysno, ...); */
#ifdef __KOS_HAVE_SYSCALL
#ifndef __KOS_HAVE_UNIXSYSCALL
GLOBAL_FUNCTION(syscall):
#endif
GLOBAL_FUNCTION(__kos_syscall):
	/* ... And this big blob of code fixing registers is why I really
	 *     just wants you to use the macros from <kos/syscall.h> ...
	 *  >> With inlining and what not, those are just so much faster! */
	movI PS(%esp), %eax /*< sysno. */
	pushI %ecx
	pushI %edx
	pushI %ebx
	pushI %esi
	pushI %edi
	/* 12: 1*return_addr+1*sysno+5*old_regs+5*regargs */
	addI $(PS*12), %esp /*< Pointer to additional arguments (#5 + ...) */
	movI (-PS*5)(%esp), %ecx /*< ARG #0. */
	movI (-PS*4)(%esp), %edx /*< ARG #1. */
	movI (-PS*3)(%esp), %ebx /*< ARG #2. */
	movI (-PS*2)(%esp), %esi /*< ARG #3. */
	movI (-PS*1)(%esp), %edi /*< ARG #4. */

	int $__SYSCALL_INTNO

	subI $(PS*12), %esp
	popI %edi
	popI %esi
	popI %ebx
	popI %edx
	popI %ecx
	ret
SYMBOLEND(__kos_syscall)
SYMBOLEND(syscall)
#endif /* __KOS_HAVE_SYSCALL */

#ifdef __KOS_HAVE_NTSYSCALL
/* NT (windows) System call.
 * >> extern long __nt_syscall(long sysno, ...); */
GLOBAL_FUNCTION(__nt_syscall):
	pushI %edx
	movI (PS*1)(%esp), %eax /*< sysno. */
	leaI (PS*2)(%esp), %edx /*< argument vector. */
	int $__NTSYSCALL_INTNO	/*< NT System Call. */
	popI %edx
	ret
SYMBOLEND(__nt_syscall)
#endif /* __KOS_HAVE_NTSYSCALL */

#ifdef __KOS_HAVE_UNIXSYSCALL
/* Unix System call.
 * >> extern long __unix_syscall(long sysno, ...); */
GLOBAL_FUNCTION(syscall):
GLOBAL_FUNCTION(__unix_syscall):
	/* Documented here: http://man7.org/linux/man-pages/man2/syscall.2.html */
	pushI %ebp
	pushI %edi
	pushI %esi
	pushI %edx
	pushI %ecx
	pushI %ebx
	movI (PS*7)(%esp), %eax
	movI (PS*8)(%esp), %ebx
	movI (PS*9)(%esp), %ecx
	movI (PS*10)(%esp), %edx
	movI (PS*11)(%esp), %esi
	movI (PS*12)(%esp), %edi
	movI (PS*13)(%esp), %ebp
	int $__UNIXSYSCALL_INTNO
	popI %ebx
	popI %ecx
	popI %edx
	popI %esi
	popI %edi
	popI %ebp
SYMBOLEND(__unix_syscall)
#endif /* __KOS_HAVE_UNIXSYSCALL */


#include <kos/__alloca.h>
#if defined(__LIBC_MUST_IMPLEMENT_LIBC_ALLOCA) || !defined(__KERNEL__)
/* Provide a ~real~ version of alloca (don't just rely on gcc's __builtin_alloca) */
#ifndef __KERNEL__
#undef alloca
GLOBAL_FUNCTION(alloca):
#endif
GLOBAL_FUNCTION(__libc_alloca):
	/* void *alloca(size_t s);
	 * Clobbers: EAX, EBX */
	popI %ebx         /* Return address */
	movI %esp, %eax   /* Store old ESP in eax */
	subI (%esp), %eax /* Size to allocate (subtract from old ESP) */
	movI %eax, %esp   /* Store new ESP. */
	pushI %ebx        /* Push return address. */
	ret
SYMBOLEND(__libc_alloca)
#ifndef __KERNEL__
SYMBOLEND(alloca)
#endif
#endif

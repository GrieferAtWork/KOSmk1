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
#ifndef __SYS_IO_H__
#define __SYS_IO_H__ 1

#include <kos/compiler.h>
#include <kos/types.h>
#include <kos/arch.h>

// NOTE: The kernel does not export I/O functions.
//       Only libc is exporting these.
// NOTE: out(b|w|l)-parameter order:
//       - Libc is exporting functions using the unix-order (value,port)
//       - Using macros, the default parameter order is swapped to (port,value)
//         Libc also exports __out(b|w|l)-style functions that follow (port,value)
//      >> Linking existing applications is straight-forward and requires no modifications.
//      >> To get the regular out-parameter order, compile in STDC-PURE mode, or
//         define '__LEGACY_OUT_PARAMORDER__' before including this header.


#if KOS_ARCH_HAVE(IO_H) && !defined(__INTELLISENSE__)
#include <kos/arch/io.h>
// Pull in platform-specific, optimized versions
#ifdef karch_inb
#define inb      karch_inb
#define inw      karch_inw
#define inl      karch_inl
#endif
#ifdef karch_outb
#define __outb   karch_outb
#define __outw   karch_outw
#define __outl   karch_outl
#endif
#ifdef karch_inb_p
#define inb_p    karch_inb_p
#define inw_p    karch_inw_p
#define inl_p    karch_inl_p
#endif
#ifdef karch_outb_p
#define __outb_p karch_outb_p
#define __outw_p karch_outw_p
#define __outl_p karch_outl_p
#endif
#ifdef karch_insb
#define insb     karch_insb
#define insw     karch_insw
#define insl     karch_insl
#endif
#ifdef karch_outsb
#define outsb    karch_outsb
#define outsw    karch_outsw
#define outsl    karch_outsl
#endif
#endif /* KOS_ARCH_HAVE(IO_H) */

__DECL_BEGIN

#ifdef __INTELLISENSE__
/* Syntax highlighting... */
extern __wunused unsigned char inb(unsigned short port);
extern __wunused unsigned char inb_p(unsigned short port);
extern __wunused unsigned short inw(unsigned short port);
extern __wunused unsigned short inw_p(unsigned short port);
extern __wunused unsigned int inl(unsigned short port);
extern __wunused unsigned int inl_p(unsigned short port);
extern __nonnull((2)) void insb(unsigned short port, void *addr, __size_t count);
extern __nonnull((2)) void insw(unsigned short port, void *addr, __size_t count);
extern __nonnull((2)) void insl(unsigned short port, void *addr, __size_t count);
extern void __outb(unsigned short port, unsigned char value);
extern void __outb_p(unsigned short port, unsigned char value);
extern void __outw(unsigned short port, unsigned short value);
extern void __outw_p(unsigned short port, unsigned short value);
extern void __outl(unsigned short port, unsigned int value);
extern void __outl_p(unsigned short port, unsigned int value);
extern __nonnull((2)) void outsb(unsigned short port, const void *addr, __size_t count);
extern __nonnull((2)) void outsw(unsigned short port, const void *addr, __size_t count);
extern __nonnull((2)) void outsl(unsigned short port, const void *addr, __size_t count);
#if defined(__LEGACY_OUT_PARAMORDER__) || defined(__STDC_PURE__)
extern void outb(unsigned char value, unsigned short port);
extern void outb_p(unsigned char value, unsigned short port);
extern void outw(unsigned short value, unsigned short port);
extern void outw_p(unsigned short value, unsigned short port);
extern void outl(unsigned int value, unsigned short port);
extern void outl_p(unsigned int value, unsigned short port);
#else
extern void outb(unsigned short port, unsigned char value);
extern void outb_p(unsigned short port, unsigned char value);
extern void outw(unsigned short port, unsigned short value);
extern void outw_p(unsigned short port, unsigned short value);
extern void outl(unsigned short port, unsigned int value);
extern void outl_p(unsigned short port, unsigned int value);
#endif
#else

#ifndef __IOPAUSE
#define __IOPAUSE()  __asm_volatile__("jmp 1f\n1: jmp 2f\n2:");
#endif

#ifdef __KERNEL__
#if !KOS_ARCH_HAVE(IO_H)
#   define inb(port)          __builtin_unreachable()
#   define inw(port)          __builtin_unreachable()
#   define inl(port)          __builtin_unreachable()
#   define __outb(port,value) __builtin_unreachable()
#   define __outw(port,value) __builtin_unreachable()
#   define __outl(port,value) __builtin_unreachable()
#endif /* !KOS_ARCH_HAVE(IO_H) */
#ifndef inb_p
__forcelocal unsigned char  inb_p __D1(unsigned short,__port) { __u8  result = inb(__port); __IOPAUSE(); return result; }
__forcelocal unsigned short inw_p __D1(unsigned short,__port) { __u16 result = inw(__port); __IOPAUSE(); return result; }
__forcelocal unsigned int   inl_p __D1(unsigned short,__port) { __u32 result = inl(__port); __IOPAUSE(); return result; }
#endif
#ifndef __outb_p
__forcelocal void __outb_p __D2(unsigned short,__port,unsigned char, __value) { __outb(__port,__value); __IOPAUSE(); }
__forcelocal void __outw_p __D2(unsigned short,__port,unsigned short,__value) { __outw(__port,__value); __IOPAUSE(); }
__forcelocal void __outl_p __D2(unsigned short,__port,unsigned int,  __value) { __outl(__port,__value); __IOPAUSE(); }
#endif
#ifndef insb
__forcelocal void insb __D3(unsigned short,__port,void *,__addr,__size_t,__count) { unsigned char  *iter,*end; end = (iter = (unsigned char  *)__addr)+__count; while (iter != end) *iter++ = inb(__port); }
__forcelocal void insw __D3(unsigned short,__port,void *,__addr,__size_t,__count) { unsigned short *iter,*end; end = (iter = (unsigned short *)__addr)+__count; while (iter != end) *iter++ = inw(__port); }
__forcelocal void insl __D3(unsigned short,__port,void *,__addr,__size_t,__count) { unsigned int   *iter,*end; end = (iter = (unsigned int   *)__addr)+__count; while (iter != end) *iter++ = inl(__port); }
#endif
#ifndef outsb
__forcelocal void outsb __D3(unsigned short,__port,void const *,__addr,__size_t,__count) { unsigned char  *iter,*end; end = (iter = (unsigned char  *)__addr)+__count; while (iter != end) __outb(__port,*iter++); }
__forcelocal void outsw __D3(unsigned short,__port,void const *,__addr,__size_t,__count) { unsigned short *iter,*end; end = (iter = (unsigned short *)__addr)+__count; while (iter != end) __outw(__port,*iter++); }
__forcelocal void outsl __D3(unsigned short,__port,void const *,__addr,__size_t,__count) { unsigned int   *iter,*end; end = (iter = (unsigned int   *)__addr)+__count; while (iter != end) __outl(__port,*iter++); }
#endif
#else /* __KERNEL__ */
#ifndef inb
extern __wunused unsigned char inb __P((unsigned short __port));
extern __wunused unsigned short inw __P((unsigned short __port));
extern __wunused unsigned int inl __P((unsigned short __port));
#endif
#ifndef __outb
extern void __outb __P((unsigned short __port, unsigned char __value));
extern void __outw __P((unsigned short __port, unsigned short __value));
extern void __outl __P((unsigned short __port, unsigned int __value));
#endif
#ifndef inb_p
extern __wunused unsigned char inb_p __P((unsigned short __port));
extern __wunused unsigned short inw_p __P((unsigned short __port));
extern __wunused unsigned int inl_p __P((unsigned short __port));
#endif
#ifndef __outb_p
extern void __outb_p __P((unsigned short __port, unsigned char __value));
extern void __outw_p __P((unsigned short __port, unsigned short __value));
extern void __outl_p __P((unsigned short __port, unsigned int __value));
#endif
#ifndef insb
extern __nonnull((2)) void insb __P((unsigned short __port, void *__addr, __size_t __count));
extern __nonnull((2)) void insw __P((unsigned short __port, void *__addr, __size_t __count));
extern __nonnull((2)) void insl __P((unsigned short __port, void *__addr, __size_t __count));
#endif
#ifndef outsb
extern __nonnull((2)) void outsb __P((unsigned short __port, const void *__addr, __size_t __count));
extern __nonnull((2)) void outsw __P((unsigned short __port, const void *__addr, __size_t __count));
extern __nonnull((2)) void outsl __P((unsigned short __port, const void *__addr, __size_t __count));
#endif
#endif /* !__KERNEL__ */

#if defined(__LEGACY_OUT_PARAMORDER__) || defined(__STDC_PURE__)
/* Legacy-style parameter order is requested. */
#ifdef __KERNEL__
#   define outb(value,port)   __outb(port,value)
#   define outb_p(value,port) __outb_p(port,value)
#   define outw(value,port)   __outw(port,value)
#   define outw_p(value,port) __outw_p(port,value)
#   define outl(value,port)   __outl(port,value)
#   define outl_p(value,port) __outl_p(port,value)
#else
extern void outb __P((unsigned char __value, unsigned short __port));
extern void outb_p __P((unsigned char __value, unsigned short __port));
extern void outw __P((unsigned short __value, unsigned short __port));
extern void outw_p __P((unsigned short __value, unsigned short __port));
extern void outl __P((unsigned int __value, unsigned short __port));
extern void outl_p __P((unsigned int __value, unsigned short __port));
#endif
#else
#   define outb   __outb
#   define outb_p __outb_p
#   define outw   __outw
#   define outw_p __outw_p
#   define outl   __outl
#   define outl_p __outl_p
#endif
#endif

__DECL_END

#endif /* !__SYS_IO_H__ */

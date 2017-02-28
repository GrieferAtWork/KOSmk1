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
#ifndef __SYS_IO_C__
#define __SYS_IO_C__ 1

#include <kos/config.h>
#ifndef __KERNEL__
#include <kos/compiler.h>
#include <sys/io.h>
#if KOS_ARCH_HAVE(IO_H)
#include KOS_ARCH_INCLUDE(io.h)
#endif
__DECL_BEGIN

#undef outb
#undef outw
#undef outl
#undef outb_p
#undef outw_p
#undef outl_p
__public void outb(unsigned char  value, unsigned short port) { __outb(port,value); }
__public void outw(unsigned short value, unsigned short port) { __outw(port,value); }
__public void outl(unsigned int   value, unsigned short port) { __outl(port,value); }
__public void outb_p(unsigned char  value, unsigned short port) { __outb_p(port,value); }
__public void outw_p(unsigned short value, unsigned short port) { __outw_p(port,value); }
__public void outl_p(unsigned int   value, unsigned short port) { __outl_p(port,value); }

#undef inb
#undef inb_p
#undef inw
#undef inw_p
#undef inl
#undef inl_p
#undef __outb
#undef __outb_p
#undef __outw
#undef __outw_p
#undef __outl
#undef __outl_p
#undef insb
#undef insw
#undef insl
#undef outsb
#undef outsw
#undef outsl

#ifdef karch_inb
__public unsigned char  inb(unsigned short port) { return karch_inb(port); }
__public unsigned short inw(unsigned short port) { return karch_inw(port); }
__public unsigned int   inl(unsigned short port) { return karch_inl(port); }
#else
__public unsigned char  inb(unsigned short __unused(port)) { __compiler_unreachable(); }
__public unsigned short inw(unsigned short __unused(port)) { __compiler_unreachable(); }
__public unsigned int   inl(unsigned short __unused(port)) { __compiler_unreachable(); }
#endif
#ifdef karch_outb
__public void __outb(unsigned short port, unsigned char  value) { karch_outb(port,value); }
__public void __outw(unsigned short port, unsigned short value) { karch_outw(port,value); }
__public void __outl(unsigned short port, unsigned int   value) { karch_outl(port,value); }
#else
__public void __outb(unsigned short __unused(port), unsigned char  __unused(value)) { __compiler_unreachable(); }
__public void __outw(unsigned short __unused(port), unsigned short __unused(value)) { __compiler_unreachable(); }
__public void __outl(unsigned short __unused(port), unsigned int   __unused(value)) { __compiler_unreachable(); }
#endif
#ifdef karch_inb_p
__public unsigned char  inb_p(unsigned short port) { return karch_inb_p(port); }
__public unsigned short inw_p(unsigned short port) { return karch_inw_p(port); }
__public unsigned int   inl_p(unsigned short port) { return karch_inl_p(port); }
#else
__public unsigned char  inb_p(unsigned short port) { __u8  result = inb(port); __IOPAUSE(); return result; }
__public unsigned short inw_p(unsigned short port) { __u16 result = inw(port); __IOPAUSE(); return result; }
__public unsigned int   inl_p(unsigned short port) { __u32 result = inl(port); __IOPAUSE(); return result; }
#endif
#ifdef karch_outb_p
__public void __outb_p(unsigned short port, unsigned char  value) { karch_outb_p(port,value); }
__public void __outw_p(unsigned short port, unsigned short value) { karch_outw_p(port,value); }
__public void __outl_p(unsigned short port, unsigned int   value) { karch_outl_p(port,value); }
#else
__public void __outb_p(unsigned short port, unsigned char  value) { __outb(port,value); __IOPAUSE(); }
__public void __outw_p(unsigned short port, unsigned short value) { __outw(port,value); __IOPAUSE(); }
__public void __outl_p(unsigned short port, unsigned int   value) { __outl(port,value); __IOPAUSE(); }
#endif
#ifdef karch_insb
__public void insb(unsigned short port, void *addr, __size_t count) { karch_insb(port,addr,count); }
__public void insw(unsigned short port, void *addr, __size_t count) { karch_insw(port,addr,count); }
__public void insl(unsigned short port, void *addr, __size_t count) { karch_insl(port,addr,count); }
#else
__public void insb(unsigned short port, void *addr, __size_t count) { unsigned char  *iter,*end; end = (iter = (unsigned char  *)addr)+count; while (iter != end) *iter++ = inb(port); }
__public void insw(unsigned short port, void *addr, __size_t count) { unsigned short *iter,*end; end = (iter = (unsigned short *)addr)+count; while (iter != end) *iter++ = inw(port); }
__public void insl(unsigned short port, void *addr, __size_t count) { unsigned int   *iter,*end; end = (iter = (unsigned int   *)addr)+count; while (iter != end) *iter++ = inl(port); }
#endif
#ifdef karch_outsb
__public void outsb(unsigned short port, void const *addr, __size_t count) { karch_outsb(port,addr,count); }
__public void outsw(unsigned short port, void const *addr, __size_t count) { karch_outsw(port,addr,count); }
__public void outsl(unsigned short port, void const *addr, __size_t count) { karch_outsl(port,addr,count); }
#else
__public void outsb(unsigned short port, void const *addr, __size_t count) { unsigned char  *iter,*end; end = (iter = (unsigned char  *)addr)+count; while (iter != end) __outb(port,*iter++); }
__public void outsw(unsigned short port, void const *addr, __size_t count) { unsigned short *iter,*end; end = (iter = (unsigned short *)addr)+count; while (iter != end) __outw(port,*iter++); }
__public void outsl(unsigned short port, void const *addr, __size_t count) { unsigned int   *iter,*end; end = (iter = (unsigned int   *)addr)+count; while (iter != end) __outl(port,*iter++); }
#endif
__DECL_END
#endif

#endif /* !__SYS_IO_C__ */

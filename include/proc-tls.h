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
#ifndef __PROC_TLS_H__
#define __PROC_TLS_H__ 1

#include <kos/config.h>
#ifndef __KERNEL__
#include <kos/compiler.h>
#include <kos/types.h>

#ifdef __INTELLISENSE__
__DECL_BEGIN

/* The base address of the thread-local TLS block.
 * >> Add an offset to this and dereference to read thread-local data.
 * WARNING: The value of this variable may change sporadically during a
 *          call to 'tls_alloc', meaning that you must reload everything
 *          derived from it after allocating more TLS storage. */
extern __byte_t _tls_addr[];
#ifdef __KOS_TASK_TLS_H__
extern struct kuthread *const _tls_self;
#endif


/* Optimized getter/setter for TLS data.
 * NOTE: Using these is faster than using 'tls_addr'. */
extern __u8  _tls_getb(__ptrdiff_t offset);
extern __u16 _tls_getw(__ptrdiff_t offset);
extern __u32 _tls_getl(__ptrdiff_t offset);
extern __u64 _tls_getq(__ptrdiff_t offset);
extern void *_tls_getp(__ptrdiff_t offset);
extern void  _tls_putb(__ptrdiff_t offset, __u8  value);
extern void  _tls_putw(__ptrdiff_t offset, __u16 value);
extern void  _tls_putl(__ptrdiff_t offset, __u32 value);
extern void  _tls_putq(__ptrdiff_t offset, __u64 value);
extern void  _tls_putp(__ptrdiff_t offset, void *value);

__DECL_END
#else /* __INTELLISENSE__ */
#ifdef __i386__
#   define __TLS_REGISTER      gs
#   define __TLS_REGISTER_S   "gs"
#elif defined(__x86_64__)
#   define __TLS_REGISTER      fs
#   define __TLS_REGISTER_S   "fs"
#else
#   error "Not implemented"
#endif

#ifdef __TLS_REGISTER
#   define _tls_addr          __xblock({ register __byte_t *__tls_res; __asm__("movl %%" __TLS_REGISTER_S ":0, %0" : "=r" (__tls_res)); __xreturn __tls_res; })
#   define _tls_getb(off)     __xblock({ register __u8  __tls_res; __asm__("movb %%" __TLS_REGISTER_S ":(%1), %0" : "=r" (__tls_res) : "r" (off)); __xreturn __tls_res; })
#   define _tls_getw(off)     __xblock({ register __u16 __tls_res; __asm__("movw %%" __TLS_REGISTER_S ":(%1), %0" : "=r" (__tls_res) : "r" (off)); __xreturn __tls_res; })
#   define _tls_getl(off)     __xblock({ register __u32 __tls_res; __asm__("movl %%" __TLS_REGISTER_S ":(%1), %0" : "=r" (__tls_res) : "r" (off)); __xreturn __tls_res; })
#   define _tls_putb(off,val) __xblock({ __asm__("movb %1, %%" __TLS_REGISTER_S ":(%0)" : : "r" (off), "r" (val)); (void)0; })
#   define _tls_putw(off,val) __xblock({ __asm__("movw %1, %%" __TLS_REGISTER_S ":(%0)" : : "r" (off), "r" (val)); (void)0; })
#   define _tls_putl(off,val) __xblock({ __asm__("movl %1, %%" __TLS_REGISTER_S ":(%0)" : : "r" (off), "r" (val)); (void)0; })
#ifdef __x86_64__
#   define _tls_getq(off)     __xblock({ register __u64 __tls_res; __asm__("movq %%" __TLS_REGISTER_S ":(%1), %0" : "=r" (__tls_res) : "r" (off)); __xreturn __tls_res; })
#   define _tls_putq(off,val) __xblock({ __asm__("movq %1, %%" __TLS_REGISTER_S ":(%0)" : : "r" (off), "r" (val)); (void)0; })
#endif /* __x86_64__ */
#endif /* __TLS_REGISTER */

#ifndef _tls_addr
#   error "Missing TLS part: '_tls_addr'"
#endif

#if !defined(_tls_getw) && defined(_tls_getb)
#include <kos/endian.h>
#if __BYTE_ORDER == __LITTLE_ENDIAN
#   define __tls_getw(off)     ((__u16)_tls_getb(off) | ((__u16)_tls_getb((off)+1) << 8))
#   define __tls_putw(off,val) (_tls_putb(off,(__u8)(val)),_tls_putb((off)+1,(__u8)((__u16)(val) >> 8)))
#elif __BYTE_ORDER == __BIG_ENDIAN
#   define __tls_getw(off)     (((__u16)_tls_getb(off) << 8) | (__u16)_tls_getb((off)+1))
#   define __tls_putw(off,val) (_tls_putb(off,(__u8)((__u16)(val) >> 8)),_tls_putb((off)+1,(__u8)(val)))
#endif /* Endian... */
#ifdef __tls_getw
#   define _tls_getw(off)     __xblock({ __ptrdiff_t const __off = (off); __xreturn __tls_getw(__off); })
#endif /* __tls_getw */
#ifdef __tls_putw
#   define _tls_putw(off,val) __xblock({ __ptrdiff_t const __off = (off); __u16 const __val = (val); __tls_putw(__off,__val); })
#endif /* __tls_putw */
#endif /* !_tls_getw && _tls_getb */

#if !defined(_tls_getl) && defined(_tls_getw)
#include <kos/endian.h>
#if __BYTE_ORDER == __LITTLE_ENDIAN
#   define __tls_getl(off)     ((__u32)_tls_getw(off) | ((__u32)_tls_getw((off)+2) << 16))
#   define __tls_putl(off,val) (_tls_putw(off,(__u16)(val)),_tls_putw((off)+2,(__u16)((__u32)(val) >> 16)))
#elif __BYTE_ORDER == __BIG_ENDIAN
#   define __tls_getl(off)     (((__u32)_tls_getw(off) << 16) | (__u32)_tls_getw((off)+2))
#   define __tls_putl(off,val) (_tls_putw(off,(__u16)((__u32)(val) >> 16)),_tls_putw((off)+2,(__u16)(val)))
#endif /* Endian... */
#ifdef __tls_getl
#   define _tls_getl(off)     __xblock({ __ptrdiff_t const __off = (off); __xreturn __tls_getl(__off); })
#endif /* __tls_getl */
#ifdef __tls_putl
#   define _tls_putl(off,val) __xblock({ __ptrdiff_t const __off = (off); __u32 const __val = (val); __tls_putl(__off,__val); })
#endif /* __tls_putl */
#endif /* !_tls_getl && _tls_getw */

#if !defined(_tls_getq) && defined(_tls_getl)
#include <kos/endian.h>
#if __BYTE_ORDER == __LITTLE_ENDIAN
#   define __tls_getq(off)     ((__u64)_tls_getl(off) | ((__u64)_tls_getl((off)+4) << 32))
#   define __tls_putq(off,val) (_tls_putl(off,(__u32)(val)),_tls_putl((off)+4,(__u32)((__u64)(val) >> 32)))
#elif __BYTE_ORDER == __BIG_ENDIAN
#   define __tls_getq(off)     (((__u64)_tls_getl(off) << 32) | (__u64)_tls_getl((off)+4))
#   define __tls_putq(off,val) (_tls_putl(off,(__u32)((__u64)(val) >> 32)),_tls_putl((off)+4,(__u32)(val)))
#endif /* Endian... */
#ifdef __tls_getq
#   define _tls_getq(off)     __xblock({ __ptrdiff_t const __off = (off); __xreturn __tls_getq(__off); })
#endif /* __tls_getq */
#ifdef __tls_putq
#   define _tls_putq(off,val) __xblock({ __ptrdiff_t const __off = (off); __u64 const __val = (val); __tls_putq(__off,__val); })
#endif /* __tls_putq */
#endif /* !_tls_getq && _tls_getl */

/* Fallback: Implement everything missing with  */
#ifndef _tls_getb
#   define _tls_getb(off)            ((__u8 const *)_tls_addr)[off]
#   define _tls_putb(off,val) (void)(((__u8 *)_tls_addr)[off] = (val))
#endif
#ifndef _tls_getw
#   define _tls_getw(off)            ((__u16 const *)_tls_addr)[off]
#   define _tls_putw(off,val) (void)(((__u16 *)_tls_addr)[off] = (val))
#endif
#ifndef _tls_getl
#   define _tls_getl(off)            ((__u32 const *)_tls_addr)[off]
#   define _tls_putl(off,val) (void)(((__u32 *)_tls_addr)[off] = (val))
#endif
#ifndef _tls_getq
#   define _tls_getq(off)            ((__u64 const *)_tls_addr)[off]
#   define _tls_putq(off,val) (void)(((__u64 *)_tls_addr)[off] = (val))
#endif

#if __SIZEOF_POINTER__ == 4
#   define _tls_getp  (void *)_tls_getl
#   define _tls_putp(off,val) _tls_putl(off,(__u32)(val))
#elif __SIZEOF_POINTER__ == 8
#   define _tls_getp  (void *)_tls_getq
#   define _tls_putp(off,val) _tls_putq(off,(__u64)(val))
#elif __SIZEOF_POINTER__ == 2
#   define _tls_getp  (void *)_tls_getw
#   define _tls_putp(off,val) _tls_putw(off,(__u16)(val))
#elif __SIZEOF_POINTER__ == 1
#   define _tls_getp  (void *)_tls_getb
#   define _tls_putp(off,val) _tls_putb(off,(__u8)(val))
#else
#   error "FIXME"
#endif

#ifdef __kuthread_defined
#define _tls_self    ((struct kuthread *)_tls_addr)
#endif /* __kuthread_defined */

#endif /* __INTELLISENSE__ */

#endif /* !__KERNEL__ */

#endif /* !__PROC_TLS_H__ */

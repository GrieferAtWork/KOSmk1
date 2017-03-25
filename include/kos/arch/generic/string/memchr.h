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
#ifndef __KOS_ARCH_GENERIC_STRING_MEMCHR_H__
#define __KOS_ARCH_GENERIC_STRING_MEMCHR_H__ 1

#include <kos/compiler.h>

#ifndef __ASSEMBLY__
#include "common.h"
#include <kos/types.h>
#include <stddef.h>
#include <stdint.h>

__DECL_BEGIN

#ifndef __arch_memchr
#ifdef __arch_memchr_b
#   define __arch_memchr  __arch_memchr_b
#else
#   define __arch_generic_memchr memchr
#   define __arch_memchr         memchr
#ifndef __memchr_defined
#define __memchr_defined 1
#if defined(__cplusplus) && !defined(__NO_asmname) && !defined(__STRING_C__)
extern "C++" {
extern __wunused __purecall __nonnull((1)) void *memchr(void *__restrict __p, int __needle, size_t __bytes) __asmname("memchr");
extern __wunused __purecall __nonnull((1)) void const *memchr(void const *__restrict __p, int __needle, size_t __bytes) __asmname("memchr");
}
#else /* CPP */
extern __wunused __purecall __nonnull((1)) void *memchr __P((void const *__restrict __p, int __needle, size_t __bytes));
#endif /* !CPP */
#endif
#endif
#endif

extern __attribute_error("Size is too large for small memchr()")
void __arch_small_memchr_too_large(void);

__forcelocal __wunused void *
__arch_small_memchr __D3(void const *__restrict,__haystack,
                         int,__needle,__size_t,__bytes) {
#define __BYTE(i)  if (((__u8 *)__haystack)[i] == __needle) return (__u8 *)__haystack+(i)
 if (__bytes >= 1) __BYTE(0);
 if (__bytes >= 2) __BYTE(1);
 if (__bytes >= 3) __BYTE(2);
 if (__bytes >= 4) __BYTE(3);
 if (__bytes >= 5) __BYTE(4);
 if (__bytes >= 6) __BYTE(5);
 if (__bytes >= 7) __BYTE(6);
 if (__bytes >= 8) __BYTE(7);
 if (__bytes >= 9) __arch_small_memchr_too_large();
#undef __BYTE
 return NULL;
}

#define arch_memchr arch_memchr
__forcelocal __wunused void *
arch_memchr __D3(void const *__restrict,__haystack,
                 int,__needle,__size_t,__bytes) {
 if (__builtin_constant_p(__bytes) &&
     __bytes <= __ARCH_STRING_SMALL_LIMIT) {
  return __arch_small_memchr(__haystack,__needle,__bytes);
 }
 return (void *)__arch_memchr(__haystack,__needle,__bytes);
}




/* Optional extension: Search by word. */
#ifdef __arch_memchr_w
__forcelocal __wunused void *
__arch_small_memchr_w __D3(void const *__restrict,__haystack,
                           __u16,__needle,__size_t,__words) {
#define __WORD(i)  if (((__u16 *)__haystack)[i] == __needle) return (__u16 *)__haystack+(i)
 if (__words >= 1) __WORD(0);
 if (__words >= 2) __WORD(1);
 if (__words >= 3) __WORD(2);
 if (__words >= 4) __WORD(3);
 if (__words >= 5) __WORD(4);
 if (__words >= 6) __WORD(5);
 if (__words >= 7) __WORD(6);
 if (__words >= 8) __WORD(7);
 if (__words >= 9) __arch_small_memchr_too_large();
#undef __WORD
 return NULL;
}

#define arch_memchr_w arch_memchr_w
__forcelocal __wunused void *
arch_memchr_w __D3(void const *__restrict,__haystack,
                   __u16,__needle,__size_t,__words) {
 if (__builtin_constant_p(__words) &&
     __words <= __ARCH_STRING_SMALL_LIMIT) {
  return __arch_small_memchr_w(__haystack,__needle,__words);
 }
 return (void *)__arch_memchr_w(__haystack,__needle,__words);
}
#endif /* __arch_memchr_w */

/* Optional extension: Search by double-word. */
#ifdef __arch_memchr_l
__forcelocal __wunused void *
__arch_small_memchr_l __D3(void const *__restrict,__haystack,
                           __u32,__needle,__size_t,__dwords) {
#define __DWORD(i)  if (((__u32 *)__haystack)[i] == __needle) return (__u32 *)__haystack+(i)
 if (__dwords >= 1) __DWORD(0);
 if (__dwords >= 2) __DWORD(1);
 if (__dwords >= 3) __DWORD(2);
 if (__dwords >= 4) __DWORD(3);
 if (__dwords >= 5) __DWORD(4);
 if (__dwords >= 6) __DWORD(5);
 if (__dwords >= 7) __DWORD(6);
 if (__dwords >= 8) __DWORD(7);
 if (__dwords >= 9) __arch_small_memchr_too_large();
#undef __DWORD
 return NULL;
}

#define arch_memchr_l arch_memchr_l
__forcelocal __wunused void *
arch_memchr_l __D3(void const *__restrict,__haystack,
                   __u32,__needle,__size_t,__dwords) {
 if (__builtin_constant_p(__dwords) &&
     __dwords <= __ARCH_STRING_SMALL_LIMIT) {
  return __arch_small_memchr_l(__haystack,__needle,__dwords);
 }
 return (void *)__arch_memchr_l(__haystack,__needle,__dwords);
}
#endif /* __arch_memchr_l */

/* Optional extension: Search by quad-word. */
#ifdef __arch_memchr_q
__forcelocal __wunused void *
__arch_small_memchr_q __D3(void const *__restrict,__haystack,
                           __u64,__needle,__size_t,__qwords) {
#define __QWORD(i)  if (((__u64 *)__haystack)[i] == __needle) return (__u64 *)__haystack+(i)
 if (__qwords >= 1) __QWORD(0);
 if (__qwords >= 2) __QWORD(1);
 if (__qwords >= 3) __QWORD(2);
 if (__qwords >= 4) __QWORD(3);
 if (__qwords >= 5) __QWORD(4);
 if (__qwords >= 6) __QWORD(5);
 if (__qwords >= 7) __QWORD(6);
 if (__qwords >= 8) __QWORD(7);
 if (__qwords >= 9) __arch_small_memchr_too_large();
#undef __QWORD
 return NULL;
}

#define arch_memchr_q arch_memchr_q
__forcelocal __wunused void *
arch_memchr_q __D3(void const *__restrict,__haystack,
                   __u64,__needle,__size_t,__qwords) {
 if (__builtin_constant_p(__qwords) &&
     __qwords <= __ARCH_STRING_SMALL_LIMIT) {
  return __arch_small_memchr_q(__haystack,__needle,__qwords);
 }
 return (void *)__arch_memchr_q(__haystack,__needle,__qwords);
}
#endif /* __arch_memchr_q */


__DECL_END
#endif /* !__ASSEMBLY__ */

#endif /* !__KOS_ARCH_GENERIC_STRING_MEMCHR_H__ */

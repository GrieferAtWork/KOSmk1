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
#ifndef __KOS_ARCH_GENERIC_STRING_MEMCPY_H__
#define __KOS_ARCH_GENERIC_STRING_MEMCPY_H__ 1

#include <kos/compiler.h>

#ifndef __ASSEMBLY__
#include "common.h"
#include <kos/types.h>
#include <stdint.h>

__DECL_BEGIN

#ifndef __arch_memcpy
#ifdef __arch_memcpy_b
#   define __arch_memcpy  __arch_memcpy_b
#else
#   define __arch_generic_memcpy memcpy
#   define __arch_memcpy         memcpy
#ifndef __memcpy_defined
#define __memcpy_defined 1
extern __retnonnull __nonnull((1,2)) void *
memcpy __P((void *__restrict __dst,
            void const *__restrict __src,
            __size_t __bytes));
#endif
#endif
#endif


extern __attribute_error("Size is too large for small memcpy()")
void __arch_small_memcpy_too_large __P((void));

__forcelocal __retnonnull __nonnull((1,2)) void *__arch_small_memcpy
__D3(void *__restrict,__dst,void const *__restrict,__src,__size_t,__bytes) {
#define __X(T,i) ((T *)__dst)[i] = ((T const *)__src)[i]
 switch (__bytes) {
  case 0:                                        break;
  case 1: __X(__u8,0);                           break;
  case 2: __X(__u16,0);                          break;
  case 3: __X(__u16,0),__X(__u8,2);              break;
  case 4: __X(__u32,0);                          break;
  case 5: __X(__u32,0),__X(__u8,4);              break;
  case 6: __X(__u32,0),__X(__u16,2);             break;
  case 7: __X(__u32,0),__X(__u16,2),__X(__u8,6); break;
#if __SIZEOF_POINTER__ >= 8
  case 8: __X(__u64,0);                          break;
#else
  case 8: __X(__u32,0),__X(__u32,1);             break;
#endif
  default: __arch_small_memcpy_too_large();
 }
 return __dst;
#undef __X
}

__forcelocal __retnonnull __nonnull((1,2)) void *__arch_constant_memcpy
__D3(void *__restrict,__dst,void const *__restrict,__src,__size_t,__bytes) {
 /* Special optimization: Small memcpy. */
 if (__bytes <= __ARCH_STRING_SMALL_LIMIT) {
  return __arch_small_memcpy(__dst,__src,__bytes);
 }
#ifdef __arch_memcpy_q
 /* Special optimization: 8-byte aligned memcpy. */
 if (!(__bytes%8)) return __arch_memcpy_q(__dst,__src,__bytes/8);
#if __ARCH_STRING_SMALL_LIMIT >= 8
 if (__bytes >= __ARCH_STRING_LARGE_LIMIT_8) {
  /* Special optimization: Large memcpy. */
  __size_t __offset = __bytes % 8;
  __arch_small_memcpy(__dst,__src,__offset);
  return __arch_memcpy_q((void *)((__uintptr_t)__dst+__offset),
                              (void *)((__uintptr_t)__src+__offset),
                               __bytes/8);
 }
#endif /* __ARCH_STRING_SMALL_LIMIT >= 8 */
#endif /* __arch_memcpy_q */
#ifdef __arch_memcpy_l
 /* Special optimization: 4-byte aligned memcpy. */
 if (!(__bytes%4)) return __arch_memcpy_l(__dst,__src,__bytes/4);
#if __ARCH_STRING_SMALL_LIMIT >= 4
 if (__bytes >= __ARCH_STRING_LARGE_LIMIT_4) {
  /* Special optimization: Large memcpy. */
  __size_t __offset = __bytes % 4;
  __arch_small_memcpy(__dst,__src,__offset);
  return __arch_memcpy_l((void *)((__uintptr_t)__dst+__offset),
                              (void *)((__uintptr_t)__src+__offset),
                               __bytes/4);
 }
#endif /* __ARCH_STRING_SMALL_LIMIT >= 4 */
#endif /* __arch_memcpy_l */
#ifdef __arch_memcpy_w
 if (!(__bytes%2)) return __arch_memcpy_w(__dst,__src,__bytes/2);
#if __ARCH_STRING_SMALL_LIMIT >= 2
 if (__bytes >= __ARCH_STRING_LARGE_LIMIT_2) {
  /* Special optimization: Large memcpy. */
  __size_t __offset = __bytes % 2;
  __arch_small_memcpy(__dst,__src,__offset);
  return __arch_memcpy_w((void *)((__uintptr_t)__dst+__offset),
                              (void *)((__uintptr_t)__src+__offset),
                               __bytes/2);
 }
#endif /* __ARCH_STRING_SMALL_LIMIT >= 2 */
#endif /* __arch_memcpy_w */
 /* Fallback: Perform a regular, old memcpy(). */
 return __arch_memcpy(__dst,__src,__bytes);
}


#define arch_memcpy   arch_memcpy
__forcelocal __retnonnull __nonnull((1,2)) void *arch_memcpy
__D3(void *__restrict,__dst,void const *__restrict,__src,__size_t,__bytes) {
 if (__builtin_constant_p(__dst == __src) && (__dst == __src)) {
  /* Special case: DST and SRC are known to be equal. */
  return __dst;
 }
 return __builtin_constant_p(__bytes)
  ? __arch_constant_memcpy(__dst,__src,__bytes)
  :          __arch_memcpy(__dst,__src,__bytes);
}

__DECL_END
#endif /* !__ASSEMBLY__ */

#endif /* !__KOS_ARCH_GENERIC_STRING_MEMCPY_H__ */

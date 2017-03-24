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
#ifndef __KOS_ARCH_GENERIC_STRING_MEMSET_H__
#define __KOS_ARCH_GENERIC_STRING_MEMSET_H__ 1

#include <kos/compiler.h>

#ifndef __ASSEMBLY__
#include "common.h"
#include <kos/types.h>
#include <stdint.h>

__DECL_BEGIN

#ifndef __arch_memset
#ifdef __arch_memset_b
#   define __arch_memset  __arch_memset_b
#else
#   define __arch_generic_memset memset
#   define __arch_memset         memset
#ifndef __memset_defined
#define __memset_defined 1
extern __retnonnull __nonnull((1)) void *
memset __P((void *__restrict __dst,
            int __byte, __size_t __bytes));
#endif
#endif
#endif

extern __attribute_error("Size is too large for small memset()")
void __arch_small_memset_too_large(void);

__forcelocal __retnonnull __nonnull((1)) void *__arch_small_memset
__D3(void *__restrict,__dst,int,__c,__size_t,__bytes) {
#define __X(T,i,m) ((T *)__dst)[i] = m*__c
 switch (__bytes) {
  case 0:                                            break;
  case 1: __X(__u8, 0,UINT8_C (0x01));               break;
  case 2: __X(__u16,0,UINT16_C(0x0101));             break;
  case 3: __X(__u16,0,UINT16_C(0x0101)),
          __X(__u8, 2,UINT8_C (0x01));               break;
  case 4: __X(__u32,0,UINT32_C(0x01010101));         break;
  case 5: __X(__u32,0,UINT32_C(0x01010101)),
          __X(__u8, 4,UINT8_C (0x01));               break;
  case 6: __X(__u32,0,UINT32_C(0x01010101)),
          __X(__u16,2,UINT16_C(0x0101));             break;
  case 7: __X(__u32,0,UINT32_C(0x01010101)),
          __X(__u16,2,UINT16_C(0x0101)),
          __X(__u8, 6,UINT8_C (0x01));               break;
#if __SIZEOF_POINTER__ >= 8
  case 8: __X(__u64,0,UINT64_C(0x0101010101010101)); break;
#else
  case 8: __X(__u32,0,UINT32_C(0x01010101)),
          __X(__u32,1,UINT32_C(0x01010101));         break;
#endif
  default: __arch_small_memset_too_large();
 }
#undef __X
 return __dst;
}

__forcelocal __retnonnull __nonnull((1)) void *__arch_constant_memset
__D3(void *__restrict,__dst,int,__c,__size_t,__bytes) {
 if (__bytes <= __ARCH_STRING_SMALL_LIMIT) {
  return __arch_small_memset(__dst,__c,__bytes);
 }
#ifdef __arch_memset_q
 if (!(__bytes%8)) return __arch_memset_q(__dst,__c,__bytes/8);
#if __ARCH_STRING_SMALL_LIMIT >= 8
 if (__bytes >= __ARCH_STRING_LARGE_LIMIT_8) {
  /* Special optimization: Large memset. */
  __size_t __offset = __bytes % 8;
  __arch_small_memset(__dst,__c,__offset);
  return __arch_memset_q((void *)((__uintptr_t)__dst+__offset),__c,__bytes/8);
 }
#endif /* __ARCH_STRING_SMALL_LIMIT >= 8 */
#endif /* __arch_memset_q */
#ifdef __arch_memset_l
 if (!(__bytes%4)) return __arch_memset_l(__dst,__c,__bytes/4);
#if __ARCH_STRING_SMALL_LIMIT >= 4
 if (__bytes >= __ARCH_STRING_LARGE_LIMIT_4) {
  /* Special optimization: Large memset. */
  __size_t __offset = __bytes % 4;
  __arch_small_memset(__dst,__c,__offset);
  return __arch_memset_l((void *)((__uintptr_t)__dst+__offset),__c,__bytes/4);
 }
#endif /* __ARCH_STRING_SMALL_LIMIT >= 4 */
#endif /* __arch_memset_l */
#ifdef __arch_memset_w
 if (!(__bytes%2)) return __arch_memset_w(__dst,__c,__bytes/2);
#if __ARCH_STRING_SMALL_LIMIT >= 2
 if (__bytes >= __ARCH_STRING_LARGE_LIMIT_2) {
  /* Special optimization: Large memset. */
  __size_t __offset = __bytes % 2;
  __arch_small_memset(__dst,__c,__offset);
  return __arch_memset_w((void *)((__uintptr_t)__dst+__offset),__c,__bytes/2);
 }
#endif /* __ARCH_STRING_SMALL_LIMIT >= 2 */
#endif /* __arch_memset_w */
 return __arch_memset(__dst,__c,__bytes);
}


#define arch_memset  arch_memset
__forcelocal __retnonnull __nonnull((1)) void *arch_memset
__D3(void *__restrict,__dst,int,__c,__size_t,__bytes) {
 return __builtin_constant_p(__bytes)
  ? __arch_constant_memset(__dst,__c,__bytes)
  :          __arch_memset(__dst,__c,__bytes);
}

__DECL_END
#endif /* !__ASSEMBLY__ */

#endif /* !__KOS_ARCH_GENERIC_STRING_MEMSET_H__ */

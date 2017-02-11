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
#ifndef __KOS_ARCH_GENERIC_STRING_H__
#define __KOS_ARCH_GENERIC_STRING_H__ 1

#include <kos/compiler.h>


#ifdef __karch_raw_ffs
#include <kos/kernel/types.h>
__DECL_BEGIN

__forcelocal __wunused __constcall int __karch_constant_ffs(int __i) {
 if (__i & 0x01) return 1;
 if (__i & 0x02) return 2;
 if (__i & 0x04) return 3;
 if (__i & 0x08) return 4;
 if (__i & 0x10) return 5;
 if (__i & 0x20) return 6;
 if (__i & 0x40) return 7;
 if (__i & 0x80) return 8;
#if __SIZEOF_INT__ > 2
 if (__i & 0x0100) return 9;
 if (__i & 0x0200) return 10;
 if (__i & 0x0400) return 11;
 if (__i & 0x0800) return 12;
 if (__i & 0x1000) return 13;
 if (__i & 0x2000) return 14;
 if (__i & 0x4000) return 15;
 if (__i & 0x8000) return 16;
#endif
#if __SIZEOF_INT__ > 4
 if (__i & 0x00010000) return 17;
 if (__i & 0x00020000) return 18;
 if (__i & 0x00040000) return 19;
 if (__i & 0x00080000) return 20;
 if (__i & 0x00100000) return 21;
 if (__i & 0x00200000) return 22;
 if (__i & 0x00400000) return 23;
 if (__i & 0x08000000) return 24;
 if (__i & 0x01000000) return 25;
 if (__i & 0x02000000) return 26;
 if (__i & 0x04000000) return 27;
 if (__i & 0x08000000) return 28;
 if (__i & 0x10000000) return 29;
 if (__i & 0x20000000) return 30;
 if (__i & 0x40000000) return 31;
 if (__i & 0x80000000) return 32;
#endif
 return 0;
}

__DECL_END
#define karch_ffs(i) \
 (__builtin_constant_p(i)\
  ? __karch_constant_ffs(i)\
  :      __karch_raw_ffs(i))
#endif /* __karch_raw_ffs */

#ifdef __karch_raw_memcpy
#include <kos/kernel/types.h>
__DECL_BEGIN


__forcelocal void *__karch_constant_memcpy(void *__dst, void const *__src, __size_t __bytes) {
#define __X(T,i) ((T *)__dst)[i] = ((T const *)__src)[i]
 switch (__bytes) {
  case 1: __X(__u8,0);                           return __dst;
  case 2: __X(__u16,0);                          return __dst;
  case 3: __X(__u16,0),__X(__u8,2);              return __dst;
  case 4: __X(__u32,0);                          return __dst;
  case 5: __X(__u32,0),__X(__u8,4);              return __dst;
  case 6: __X(__u32,0),__X(__u16,2);             return __dst;
  case 7: __X(__u32,0),__X(__u16,2),__X(__u8,6); return __dst;
  case 8: __X(__u64,0);                          return __dst;
  default: break;
 }
#undef __X
 return __karch_raw_memcpy(__dst,__src,__bytes);
}
#define karch_memcpy(dst,src,bytes) \
 (__builtin_constant_p(bytes)\
  ? __karch_constant_memcpy(dst,src,bytes)\
  :      __karch_raw_memcpy(dst,src,bytes))

__DECL_END
#endif

#ifdef __karch_raw_memset
#include <kos/kernel/types.h>
#include <stdint.h>
__DECL_BEGIN

__forcelocal void *__karch_constant_memset(void *__dst, int __c, __size_t __bytes) {
#define __X(T,i,m) ((T *)__dst)[i] = m*__c
 switch (__bytes) {
  case 1: __X(__u8, 0,UINT8_C (0x01));               return __dst;
  case 2: __X(__u16,0,UINT16_C(0x0101));             return __dst;
  case 3: __X(__u16,0,UINT16_C(0x0101)),
          __X(__u8, 2,UINT8_C (0x01));               return __dst;
  case 4: __X(__u32,0,UINT32_C(0x01010101));         return __dst;
  case 5: __X(__u32,0,UINT32_C(0x01010101)),
          __X(__u8, 4,UINT8_C (0x01));               return __dst;
  case 6: __X(__u32,0,UINT32_C(0x01010101)),
          __X(__u16,2,UINT16_C(0x0101));             return __dst;
  case 7: __X(__u32,0,UINT32_C(0x01010101)),
          __X(__u16,2,UINT16_C(0x0101)),
          __X(__u8, 6,UINT8_C (0x01));               return __dst;
  case 8: __X(__u64,0,UINT64_C(0x0101010101010101)); return __dst;
  default: break;
 }
#undef __X
 return __karch_raw_memset(__dst,__c,__bytes);
}

#define karch_memset(dst,c,bytes) \
 (__builtin_constant_p(bytes)\
  ? __karch_constant_memset(dst,c,bytes)\
  :      __karch_raw_memset(dst,c,bytes))

__DECL_END
#endif

#ifdef __karch_raw_strend
#define karch_strend   __karch_raw_strend
#endif

#ifdef __karch_raw_strnend
#define karch_strnend  __karch_raw_strnend
#endif

#ifdef __karch_raw_strlen
#define karch_strlen   __karch_raw_strlen
#endif

#ifdef __karch_raw_strnlen
#define karch_strnlen  __karch_raw_strnlen
#endif


#endif /* !__KOS_ARCH_GENERIC_STRING_H__ */

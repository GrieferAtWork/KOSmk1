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

#ifndef __ASSEMBLY__
/* Using low-level assembly functions implemented
 * by the #include-er, generate high-level wrappers
 * for string utilities, featuring constant
 * optimizations for arguments known at compile time.
 * NOTE: The functions in this file assume and
 *       implement stdc-compliant semantics.
 */

#ifndef karch_ffs
#ifdef __karch_raw_ffs
#include <kos/kernel/types.h>
__DECL_BEGIN

__forcelocal __wunused __constcall
int __karch_constant_ffs(int __i) {
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
#endif /* !karch_ffs */


#ifndef karch_memcpy
#ifdef __karch_raw_memcpy
#include <kos/kernel/types.h>
__DECL_BEGIN

extern __attribute_error("Size is too large for small memcpy()")
void __karch_small_memcpy_too_large(void);

#define __KARCH_SMALL_MEMCPY_MAXSIZE  8
__forcelocal void *
__karch_small_memcpy(void *__dst, void const *__src, __size_t __bytes) {
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
  default: __karch_small_memcpy_too_large();
 }
 return __dst;
#undef __X
}

/* Size limits used to determine string-operations large
 * enough to qualify use of a w/l/q arch-operation, even
 * if that means having to produce additional alignment code. */
#define __KARCH_STRING_LARGE_LIMIT_8  (1 << 12)
#define __KARCH_STRING_LARGE_LIMIT_4  (1 << 10)
#define __KARCH_STRING_LARGE_LIMIT_2  (1 << 5)

__forcelocal void *
__karch_constant_memcpy(void *__dst, void const *__src, __size_t __bytes) {
 /* Special optimization: Small memcpy. */
 if (__bytes <= __KARCH_SMALL_MEMCPY_MAXSIZE) {
  return __karch_small_memcpy(__dst,__src,__bytes);
 }
#ifdef __karch_raw_memcpy_q
 /* Special optimization: 8-byte aligned memcpy. */
 if (!(__bytes%8)) return __karch_raw_memcpy_q(__dst,__src,__bytes/8);
#if __KARCH_SMALL_MEMCPY_MAXSIZE >= 8
 if (__bytes >= __KARCH_STRING_LARGE_LIMIT_8) {
  /* Special optimization: Large memcpy. */
  __size_t __offset = __bytes % 8;
  __karch_small_memcpy(__dst,__src,__offset);
  return __karch_raw_memcpy_q((void *)((__uintptr_t)__dst+__offset),
                              (void *)((__uintptr_t)__src+__offset),
                               __bytes/8);
 }
#endif /* __KARCH_SMALL_MEMCPY_MAXSIZE >= 8 */
#endif /* __karch_raw_memcpy_q */
#ifdef __karch_raw_memcpy_l
 /* Special optimization: 4-byte aligned memcpy. */
 if (!(__bytes%4)) return __karch_raw_memcpy_l(__dst,__src,__bytes/4);
#if __KARCH_SMALL_MEMCPY_MAXSIZE >= 4
 if (__bytes >= __KARCH_STRING_LARGE_LIMIT_4) {
  /* Special optimization: Large memcpy. */
  __size_t __offset = __bytes % 4;
  __karch_small_memcpy(__dst,__src,__offset);
  return __karch_raw_memcpy_l((void *)((__uintptr_t)__dst+__offset),
                              (void *)((__uintptr_t)__src+__offset),
                               __bytes/4);
 }
#endif /* __KARCH_SMALL_MEMCPY_MAXSIZE >= 4 */
#endif /* __karch_raw_memcpy_l */
#ifdef __karch_raw_memcpy_w
 if (!(__bytes%2)) return __karch_raw_memcpy_w(__dst,__src,__bytes/2);
#if __KARCH_SMALL_MEMCPY_MAXSIZE >= 2
 if (__bytes >= __KARCH_STRING_LARGE_LIMIT_2) {
  /* Special optimization: Large memcpy. */
  __size_t __offset = __bytes % 2;
  __karch_small_memcpy(__dst,__src,__offset);
  return __karch_raw_memcpy_w((void *)((__uintptr_t)__dst+__offset),
                              (void *)((__uintptr_t)__src+__offset),
                               __bytes/2);
 }
#endif /* __KARCH_SMALL_MEMCPY_MAXSIZE >= 2 */
#endif /* __karch_raw_memcpy_w */
 /* Fallback: Perform a regular, old memcpy(). */
#ifdef __karch_raw_memcpy_b
 return __karch_raw_memcpy_b(__dst,__src,__bytes);
#else
 return __karch_raw_memcpy(__dst,__src,__bytes);
#endif
}
#define karch_memcpy(dst,src,bytes) \
 (__builtin_constant_p(bytes)\
  ? __karch_constant_memcpy(dst,src,bytes)\
  :      __karch_raw_memcpy(dst,src,bytes))

__DECL_END
#endif
#endif /* !karch_memcpy */


#ifndef karch_memset
#ifdef __karch_raw_memset
#include <kos/kernel/types.h>
#include <stdint.h>
__DECL_BEGIN

extern __attribute_error("Size is too large for small memset()")
void __karch_small_memset_too_large(void);

#define __KARCH_SMALL_MEMSET_MAXSIZE  8
__forcelocal void *
__karch_small_memset(void *__dst, int __c, __size_t __bytes) {
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
  default: __karch_small_memset_too_large();
 }
#undef __X
 return __dst;
}

__forcelocal void *
__karch_constant_memset(void *__dst, int __c, __size_t __bytes) {
 if (__bytes <= __KARCH_SMALL_MEMSET_MAXSIZE) {
  return __karch_small_memset(__dst,__c,__bytes);
 }
#ifdef __karch_raw_memset_q
 if (!(__bytes%8)) return __karch_raw_memset_q(__dst,__c,__bytes/8);
#if __KARCH_SMALL_MEMSET_MAXSIZE >= 8
 if (__bytes >= __KARCH_STRING_LARGE_LIMIT_8) {
  /* Special optimization: Large memset. */
  __size_t __offset = __bytes % 8;
  __karch_small_memset(__dst,__c,__offset);
  return __karch_raw_memset_q((void *)((__uintptr_t)__dst+__offset),__c,__bytes/8);
 }
#endif /* __KARCH_SMALL_MEMSET_MAXSIZE >= 8 */
#endif /* __karch_raw_memset_q */
#ifdef __karch_raw_memset_l
 if (!(__bytes%4)) return __karch_raw_memset_l(__dst,__c,__bytes/4);
#if __KARCH_SMALL_MEMSET_MAXSIZE >= 4
 if (__bytes >= __KARCH_STRING_LARGE_LIMIT_4) {
  /* Special optimization: Large memset. */
  __size_t __offset = __bytes % 4;
  __karch_small_memset(__dst,__c,__offset);
  return __karch_raw_memset_l((void *)((__uintptr_t)__dst+__offset),__c,__bytes/4);
 }
#endif /* __KARCH_SMALL_MEMSET_MAXSIZE >= 4 */
#endif /* __karch_raw_memset_l */
#ifdef __karch_raw_memset_w
 if (!(__bytes%2)) return __karch_raw_memset_w(__dst,__c,__bytes/2);
#if __KARCH_SMALL_MEMSET_MAXSIZE >= 2
 if (__bytes >= __KARCH_STRING_LARGE_LIMIT_2) {
  /* Special optimization: Large memset. */
  __size_t __offset = __bytes % 2;
  __karch_small_memset(__dst,__c,__offset);
  return __karch_raw_memset_w((void *)((__uintptr_t)__dst+__offset),__c,__bytes/2);
 }
#endif /* __KARCH_SMALL_MEMSET_MAXSIZE >= 2 */
#endif /* __karch_raw_memset_w */
#ifdef __karch_raw_memset_b
 return __karch_raw_memset_b(__dst,__c,__bytes);
#else
 return __karch_raw_memset(__dst,__c,__bytes);
#endif
}

#define karch_memset(dst,c,bytes) \
 (__builtin_constant_p(bytes)\
  ? __karch_constant_memset(dst,c,bytes)\
  :      __karch_raw_memset(dst,c,bytes))


__DECL_END
#endif
#endif /* !karch_memset */


#ifndef karch_memcmp
#ifdef __karch_raw_memcmp
#include <kos/endian.h>
#include <stdint.h>
#if __BYTE_ORDER == __LITTLE_ENDIAN || \
    __BYTE_ORDER == __BIG_ENDIAN
__DECL_BEGIN

__forcelocal int
__karch_getfirstnzbyte_16(__s16 __b) {
#if __BYTE_ORDER == __LITTLE_ENDIAN
 if (!(__b&UINT16_C(0xff))) __b >>= 8;
 return (int)__b;
#elif __BYTE_ORDER == __BIG_ENDIAN
 if (!(__b&UINT16_C(0xff00))) __b <<= 8;
 return (int)(__b >> 8);
#endif
}

__forcelocal int
__karch_getfirstnzbyte_32(__s32 __b) {
#if __BYTE_ORDER == __LITTLE_ENDIAN
 if (!(__b&UINT32_C(0xff))) { __b >>= 8;
 if (!(__b&UINT32_C(0xff))) { __b >>= 8;
 if (!(__b&UINT32_C(0xff))) { __b >>= 8; }}}
 return (int)__b;
#elif __BYTE_ORDER == __BIG_ENDIAN
 if (!(__b&0xff000000)) { __b <<= 8;
 if (!(__b&0xff000000)) { __b <<= 8;
 if (!(__b&0xff000000)) { __b <<= 8; }}}
 return (int)(__b >> 24);
#endif
}

__forcelocal int
__karch_getfirstnzbyte_64(__s64 __b) {
#if __BYTE_ORDER == __LITTLE_ENDIAN
 if (!(__b&0xff)) __b >>= 8;
 if (!(__b&0xff)) __b >>= 8;
 if (!(__b&0xff)) __b >>= 8;
 if (!(__b&0xff)) __b >>= 8;
 if (!(__b&0xff)) __b >>= 8;
 if (!(__b&0xff)) __b >>= 8;
 if (!(__b&0xff)) __b >>= 8;
 return (int)__b;
#elif __BYTE_ORDER == __BIG_ENDIAN
 if (!(__b&UINT64_C(0xff00000000000000))) { __b >>= 8;
 if (!(__b&UINT64_C(0xff00000000000000))) { __b >>= 8;
 if (!(__b&UINT64_C(0xff00000000000000))) { __b >>= 8;
 if (!(__b&UINT64_C(0xff00000000000000))) { __b >>= 8;
 if (!(__b&UINT64_C(0xff00000000000000))) { __b >>= 8;
 if (!(__b&UINT64_C(0xff00000000000000))) { __b >>= 8;
 if (!(__b&UINT64_C(0xff00000000000000))) { __b >>= 8; }}}}}}}
 return (int)(__b >> 56);
#endif
}


extern __attribute_error("Size is too large for small memcmp()")
void __karch_small_memcmp_too_large(void);

#define __KARCH_SMALL_MEMCMP_MAXSIZE   8

__forcelocal int
__karch_small_memcmp(void const *__a, void const *__b, __size_t __bytes) {
#define __CMP(T,i) (((T *)__a)[i]-((T *)__b)[i])
 switch (__bytes) {
  case 0: return 0;
  case 1: return (int)__CMP(__s8,0);
  case 2: return __karch_getfirstnzbyte_16(__CMP(__s16,0));
  case 3: {
   __s16 __result = __CMP(__s16,0);
   if (__result) return __karch_getfirstnzbyte_16(__result);
   return (int)__CMP(__s8,2);
  }
  case 4: return __karch_getfirstnzbyte_32(__CMP(__s32,0));
  case 5: {
   __s32 __result = __CMP(__s32,0);
   if (__result) return __karch_getfirstnzbyte_32(__result);
   return (int)__CMP(__s8,4);
  }
  case 6: {
   __s32 __result = __CMP(__s32,0);
   if (__result) return __karch_getfirstnzbyte_32(__result);
   return __karch_getfirstnzbyte_16(__CMP(__s16,2));
  }
  case 7: {
   __s16 __result16;
   __s32 __result32 = __CMP(__s32,0);
   if (__result32) return __karch_getfirstnzbyte_32(__result32);
   __result16 = __CMP(__s16,2);
   if (__result16) return __karch_getfirstnzbyte_16(__result16);
   return (int)__CMP(__s8,6);
  }
#if __SIZEOF_POINTER__ >= 8
  case 8: return __karch_getfirstnzbyte_64(__CMP(__s64,0));
#else
  case 8: {
   __s32 __result32 = __CMP(__s32,0);
   if (__result32) return __karch_getfirstnzbyte_32(__result32);
   return __karch_getfirstnzbyte_32(__CMP(__s32,1));
  }
#endif
  default: __karch_small_memcmp_too_large();
 }
#undef __CMP
}


__forcelocal int
__karch_constant_memcmp(void const *__a, void const *__b, __size_t __bytes) {
 /* Optimizations for small sizes. */
 if (__bytes <= __KARCH_SMALL_MEMCMP_MAXSIZE) {
  return __karch_small_memcmp(__a,__b,__bytes);
 }
 /* Optimizations for qword/dword/word-aligned sizes. */
#ifdef __karch_raw_memcmp_q
 if (!(__bytes%8)) return __karch_getfirstnzbyte_64(__karch_raw_memcmp_q(__a,__b,__bytes/8));
#if __KARCH_SMALL_MEMSET_MAXSIZE >= 8
 if (__bytes >= __KARCH_STRING_LARGE_LIMIT_8) {
  /* Special optimization: Large memset. */
  __size_t __offset = __bytes % 8;
  int __result = __karch_small_memcmp(__a,__b,__offset);
  return __result ? __result : __karch_getfirstnzbyte_64(
   __karch_raw_memcmp_q((void *)((__uintptr_t)__a+__offset),
                        (void *)((__uintptr_t)__b+__offset),
                         __bytes/8));
 }
#endif /* __KARCH_SMALL_MEMSET_MAXSIZE >= 8 */
#endif /* __karch_raw_memcmp_q */
#ifdef __karch_raw_memcmp_l
 if (!(__bytes%4)) return __karch_getfirstnzbyte_32(__karch_raw_memcmp_l(__a,__b,__bytes/4));
#if __KARCH_SMALL_MEMSET_MAXSIZE >= 4
 if (__bytes >= __KARCH_STRING_LARGE_LIMIT_4) {
  /* Special optimization: Large memset. */
  __size_t __offset = __bytes % 4;
  int __result = __karch_small_memcmp(__a,__b,__offset);
  return __result ? __result : __karch_getfirstnzbyte_32(
   __karch_raw_memcmp_l((void *)((__uintptr_t)__a+__offset),
                        (void *)((__uintptr_t)__b+__offset),
                         __bytes/4));
 }
#endif /* __KARCH_SMALL_MEMSET_MAXSIZE >= 4 */
#endif /* __karch_raw_memcmp_l */
#ifdef __karch_raw_memcmp_w
 if (!(__bytes%2)) return __karch_getfirstnzbyte_16(__karch_raw_memcmp_w(__a,__b,__bytes/2));
#if __KARCH_SMALL_MEMSET_MAXSIZE >= 2
 if (__bytes >= __KARCH_STRING_LARGE_LIMIT_2) {
  /* Special optimization: Large memset. */
  __size_t __offset = __bytes % 2;
  int __result = __karch_small_memcmp(__a,__b,__offset);
  return __result ? __result : __karch_getfirstnzbyte_16(
   __karch_raw_memcmp_w((void *)((__uintptr_t)__a+__offset),
                        (void *)((__uintptr_t)__b+__offset),
                         __bytes/2));
 }
#endif /* __KARCH_SMALL_MEMSET_MAXSIZE >= 4 */
#endif /* __karch_raw_memcmp_w */
 /* Fallback. */
#ifdef __karch_raw_memcmp_b
 return __karch_raw_memcmp_b(__a,__b,__bytes);
#else
 return __karch_raw_memcmp(__a,__b,__bytes);
#endif
}
__DECL_END

#define karch_memcmp(a,b,bytes) \
 (__builtin_constant_p(bytes)\
  ? __karch_constant_memcmp(a,b,bytes)\
  :      __karch_raw_memcmp(a,b,bytes))
#else /* Endian... */
#define karch_memcmp(a,b,bytes) \
 ((__builtin_constant_p(bytes) && !(bytes))\
  ? 0 : __karch_raw_memcmp(a,b,bytes))
#endif /* Endian... */
#endif
#endif /* !karch_memcmp */


#ifdef __karch_raw_strend
#define karch_strend  __karch_raw_strend
#endif

#ifdef __karch_raw_strnend
#define karch_strnend __karch_raw_strnend
#endif

#ifdef __karch_raw_strlen
#define karch_strlen  __karch_raw_strlen
#endif

#ifdef __karch_raw_strnlen
#define karch_strnlen __karch_raw_strnlen
#endif

#ifdef __karch_raw_memchr_b
#define karch_memchr  __karch_raw_memchr_b
#elif defined(__karch_raw_memchr)
#define karch_memchr  __karch_raw_memchr
#endif

#ifdef __karch_raw_memrchr_b
#define karch_memrchr __karch_raw_memrchr_b
#elif defined(__karch_raw_memrchr)
#define karch_memrchr __karch_raw_memrchr
#endif




/* =============================================== */
/*  memmem(haystack,haystacklen,needle,needlelen)  */
/* =============================================== */
#ifndef karch_memmem
#ifdef karch_memchr
#if (defined(__karch_raw_memcmp) || defined(__karch_raw_memcmp_b)) &&\
    (defined(__karch_raw_memcmp_w) || defined(__karch_raw_memcmp_l) ||\
     defined(__karch_raw_memcmp_q))
#include <kos/kernel/types.h>
/* Implement a fast 'memmem' using 'memcmp' and 'memchr' */
__DECL_BEGIN

#define __MAKE_MEMMEM(mnemonic,length) \
/* Optimized version for 'NEEDLELEN%length == 0' */\
__forcelocal void *\
__karch_memmem_##mnemonic(void const *__haystack, __size_t __haystacklen,\
                          void const *__needle, __size_t __needlelen) {\
 void const *__candidate,*__haystackend; __u8 __key;\
 __haystackend = (void const *)((__uintptr_t)__haystack+(__haystacklen-__needlelen));\
 __key         = *(__u8 *)__needle;\
 __needlelen  /= length;\
 while ((__candidate = karch_memchr(__haystack,\
        (__uintptr_t)__haystackend-(__uintptr_t)__haystack,__key)) != NULL) {\
  /* Check the candidate for really being a match. */\
  if (!__karch_raw_memcmp_##mnemonic(__candidate,__needle,__needlelen)) return (void *)__candidate;\
  /* Continue searching 1 byte after the candidate. */\
  *(__uintptr_t *)&__haystack = (__uintptr_t)__candidate+1;\
 }\
 return NULL;\
}
#define __MAKE_MEMMEM1(mnemonic,length) \
/* Optimized version for 'NEEDLELEN%length == 1' */\
__forcelocal void *\
__karch_memmem_##mnemonic##1(void const *__haystack, __size_t __haystacklen,\
                             void const *__needle, __size_t __needlelen) {\
 void const *__candidate,*__haystackend; __u8 __key;\
 __haystackend = (void const *)((__uintptr_t)__haystack+(__haystacklen-__needlelen));\
 __key         = *(__u8 *)__needle;\
 ++*(__uintptr_t *)&__needle,--__needlelen;\
 __needlelen /= length;\
 while ((__candidate = karch_memchr(__haystack,\
        (__uintptr_t)__haystackend-(__uintptr_t)__haystack,__key)) != NULL) {\
  /* Check the candidate for really being a match. */\
  if (!__karch_raw_memcmp_##mnemonic((void const *)((__uintptr_t)__candidate+1),\
                                     __needle,__needlelen)) return (void *)__candidate;\
  /* Continue searching 1 byte after the candidate. */\
  *(__uintptr_t *)&__haystack = (__uintptr_t)__candidate+1;\
 }\
 return NULL;\
}

#ifdef __karch_raw_memcmp_w
__MAKE_MEMMEM(w,2)
__MAKE_MEMMEM1(w,2)
#else
#ifdef __karch_raw_memcmp_b
__MAKE_MEMMEM(b,1)
#endif
#endif
#ifdef __karch_raw_memcmp_l
__MAKE_MEMMEM(l,4)
__MAKE_MEMMEM1(l,4)
#endif
#ifdef __karch_raw_memcmp_q
__MAKE_MEMMEM(q,8)
__MAKE_MEMMEM1(q,8)
#endif
#undef __MAKE_MEMMEM1
#undef __MAKE_MEMMEM

/* Merging all memmem algorithms, select the most appropriate one. */
__forcelocal void *
__karch_memmem_impl(void const *__haystack, __size_t __haystacklen,
                    void const *__needle, __size_t __needlelen) {
#ifdef assert
 assert(__needlelen && __needlelen <= __haystacklen);
#endif
#ifdef __karch_raw_memcmp_q
 switch (__needlelen%8) {
  case 0: return __karch_memmem_q (__haystack,__haystacklen,__needle,__needlelen);
  case 1: return __karch_memmem_q1(__haystack,__haystacklen,__needle,__needlelen);
  default: break;
 }
#endif
#ifdef __karch_raw_memcmp_l
 switch (__needlelen%4) {
  case 0: return __karch_memmem_l (__haystack,__haystacklen,__needle,__needlelen);
  case 1: return __karch_memmem_l1(__haystack,__haystacklen,__needle,__needlelen);
  default: break;
 }
#endif
#ifdef __karch_raw_memcmp_w
 if (!(__needlelen%2)) return __karch_memmem_w(__haystack,__haystacklen,__needle,__needlelen);
 /* The last possibility now is that the needle length is uneven. */
 return __karch_memmem_w1(__haystack,__haystacklen,__needle,__needlelen);
#else
 return __karch_memmem_b(__haystack,__haystacklen,__needle,__needlelen);
#endif
}

__forcelocal void *
__karch_memmem(void const *__haystack, __size_t __haystacklen,
               void const *__needle, __size_t __needlelen) {
 /* Handle special cases: Big needle and empty needle. */
 if __unlikely(__needlelen > __haystacklen || !__needlelen) return NULL;
 return __karch_memmem_impl(__haystack,__haystacklen,__needle,__needlelen);
}

/* Special optimization for known haystack size. */
__forcelocal void *
__karch_constant_memmem(void const *__haystack, __size_t __haystacklen,
                        void const *__needle, __size_t __needlelen) {
 /* Handle special cases: Big needle and empty needle. */
 if __unlikely(__needlelen > __haystacklen || !__needlelen) return NULL;
 /* Special optimization for small haystacks. */
 switch (__haystacklen) {
  case 1: return (*(__u8 *)__haystack == *(__u8 *)__needle) ? (void *)__haystack : NULL;
  case 2:
   if (__needlelen == 2) {
    return (*(__u16 *)__haystack == *(__u16 *)__needle) ? (void *)__haystack : NULL;
   } else {
    __u8 __key = *(__u8 *)__needle;
    /* Needle must be 1 byte long. */
    if (((__u8 *)__haystack)[0] == __key) return (void *)((__uintptr_t)__haystack);
    if (((__u8 *)__haystack)[1] == __key) return (void *)((__uintptr_t)__haystack+1);
    return NULL;
   }
   break;
  default: break;
 }
 /* Special optimization: Needle size is known and matches the haystack size. */
 if (__builtin_constant_p(__needlelen) &&
     __needlelen == __haystacklen) {
  return karch_memcmp(__haystack,__needle,__needlelen) ? (void *)__haystack : NULL;
 }

 /* Fallback: Use regular memmem. */
 return __karch_memmem_impl(__haystack,__haystacklen,__needle,__needlelen);
}
#define karch_memmem(haystack,haystacklen,needle,needlelen) \
 (__builtin_constant_p(haystacklen)\
  ? __karch_constant_memmem(haystack,haystacklen,needle,needlelen)\
  :          __karch_memmem(haystack,haystacklen,needle,needlelen))

__DECL_END
#endif /* __karch_raw_memcmp... */
#endif /* karch_memchr */
#endif /* !karch_memmem */



/* ================================================ */
/*  memrmem(haystack,haystacklen,needle,needlelen)  */
/* ================================================ */
#ifndef karch_memrmem
#ifdef karch_memrchr
#if (defined(__karch_raw_memcmp) || defined(__karch_raw_memcmp_b)) &&\
    (defined(__karch_raw_memcmp_w) || defined(__karch_raw_memcmp_l) ||\
     defined(__karch_raw_memcmp_q))
#include <kos/kernel/types.h>
/* Implement a fast 'memrmem' using 'memcmp' and 'memrchr' */
__DECL_BEGIN

#define __MAKE_MEMRMEM(mnemonic,length) \
/* Optimized version for 'NEEDLELEN%length == 0' */\
__forcelocal void *\
__karch_memrmem_##mnemonic(void const *__haystack, __size_t __haystacklen,\
                           void const *__needle, __size_t __needlelen) {\
 void const *__candidate,*__haystackend; __u8 __key;\
 __haystackend = (void const *)((__uintptr_t)__haystack+(__haystacklen-__needlelen));\
 __key         = *(__u8 *)__needle;\
 __needlelen  /= length;\
 while ((__candidate = karch_memrchr(__haystack,\
        (__uintptr_t)__haystackend-(__uintptr_t)__haystack,__key)) != NULL) {\
  /* Check the candidate for really being a match. */\
  if (!__karch_raw_memcmp_##mnemonic(__candidate,__needle,__needlelen)) return (void *)__candidate;\
  /* Continue searching, but stop at this candidate. */\
  __haystackend = __candidate;\
 }\
 return NULL;\
}
#define __MAKE_MEMRMEM1(mnemonic,length) \
/* Optimized version for 'NEEDLELEN%length == 1' */\
__forcelocal void *\
__karch_memrmem_##mnemonic##1(void const *__haystack, __size_t __haystacklen,\
                              void const *__needle, __size_t __needlelen) {\
 void const *__candidate,*__haystackend; __u8 __key;\
 __haystackend = (void const *)((__uintptr_t)__haystack+(__haystacklen-__needlelen));\
 __key         = *(__u8 *)__needle;\
 ++*(__uintptr_t *)&__needle,--__needlelen;\
 __needlelen /= length;\
 while ((__candidate = karch_memrchr(__haystack,\
        (__uintptr_t)__haystackend-(__uintptr_t)__haystack,__key)) != NULL) {\
  /* Check the candidate for really being a match. */\
  if (!__karch_raw_memcmp_##mnemonic((void const *)((__uintptr_t)__candidate+1),\
                                     __needle,__needlelen)) return (void *)__candidate;\
  /* Continue searching, but stop at this candidate. */\
  __haystack = __candidate;\
 }\
 return NULL;\
}

#ifdef __karch_raw_memcmp_w
__MAKE_MEMRMEM(w,2)
__MAKE_MEMRMEM1(w,2)
#else
#ifdef __karch_raw_memcmp_b
__MAKE_MEMRMEM(b,1)
#endif
#endif
#ifdef __karch_raw_memcmp_l
__MAKE_MEMRMEM(l,4)
__MAKE_MEMRMEM1(l,4)
#endif
#ifdef __karch_raw_memcmp_q
__MAKE_MEMRMEM(q,8)
__MAKE_MEMRMEM1(q,8)
#endif
#undef __MAKE_MEMRMEM1
#undef __MAKE_MEMRMEM

/* Merging all memrmem algorithms, select the most appropriate one. */
__forcelocal void *
__karch_memrmem_impl(void const *__haystack, __size_t __haystacklen,
                     void const *__needle, __size_t __needlelen) {
#ifdef assert
 assert(__needlelen && __needlelen <= __haystacklen);
#endif
#ifdef __karch_raw_memcmp_q
 switch (__needlelen%8) {
  case 0: return __karch_memrmem_q (__haystack,__haystacklen,__needle,__needlelen);
  case 1: return __karch_memrmem_q1(__haystack,__haystacklen,__needle,__needlelen);
  default: break;
 }
#endif
#ifdef __karch_raw_memcmp_l
 switch (__needlelen%4) {
  case 0: return __karch_memrmem_l (__haystack,__haystacklen,__needle,__needlelen);
  case 1: return __karch_memrmem_l1(__haystack,__haystacklen,__needle,__needlelen);
  default: break;
 }
#endif
#ifdef __karch_raw_memcmp_w
 if (!(__needlelen%2)) return __karch_memrmem_w(__haystack,__haystacklen,__needle,__needlelen);
 /* The last possibility now is that the needle length is uneven. */
 return __karch_memrmem_w1(__haystack,__haystacklen,__needle,__needlelen);
#else
 return __karch_memrmem_b(__haystack,__haystacklen,__needle,__needlelen);
#endif
}

__forcelocal void *
__karch_memrmem(void const *__haystack, __size_t __haystacklen,
                void const *__needle, __size_t __needlelen) {
 /* Handle special cases: Big needle and empty needle. */
 if __unlikely(__needlelen > __haystacklen || !__needlelen) return NULL;
 return __karch_memrmem_impl(__haystack,__haystacklen,__needle,__needlelen);
}

/* Special optimization for known haystack size. */
__forcelocal void *
__karch_constant_memrmem(void const *__haystack, __size_t __haystacklen,
                         void const *__needle, __size_t __needlelen) {
 /* Handle special cases: Big needle and empty needle. */
 if __unlikely(__needlelen > __haystacklen || !__needlelen) return NULL;
 /* Special optimization for small haystacks. */
 switch (__haystacklen) {
  case 1: return (*(__u8 *)__haystack == *(__u8 *)__needle) ? (void *)__haystack : NULL;
  case 2:
   if (__needlelen == 2) {
    return (*(__u16 *)__haystack == *(__u16 *)__needle) ? (void *)__haystack : NULL;
   } else {
    __u8 __key = *(__u8 *)__needle;
    /* Needle must be 1 byte long. */
    if (((__u8 *)__haystack)[1] == __key) return (void *)((__uintptr_t)__haystack+1);
    if (((__u8 *)__haystack)[0] == __key) return (void *)((__uintptr_t)__haystack);
    return NULL;
   }
   break;
  default: break;
 }
 /* Special optimization: Needle size is known and matches the haystack size. */
 if (__builtin_constant_p(__needlelen) &&
     __needlelen == __haystacklen) {
  return karch_memcmp(__haystack,__needle,__needlelen) ? (void *)__haystack : NULL;
 }

 /* Fallback: Use regular memmem. */
 return __karch_memrmem_impl(__haystack,__haystacklen,__needle,__needlelen);
}
#define karch_memrmem(haystack,haystacklen,needle,needlelen) \
 (__builtin_constant_p(haystacklen)\
  ? __karch_constant_memrmem(haystack,haystacklen,needle,needlelen)\
  :          __karch_memrmem(haystack,haystacklen,needle,needlelen))

__DECL_END
#endif /* __karch_raw_memcmp... */
#endif /* karch_memrchr */
#endif /* !karch_memrmem */
#endif /* !__ASSEMBLY__ */

#endif /* !__KOS_ARCH_GENERIC_STRING_H__ */

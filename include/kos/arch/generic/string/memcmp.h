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
#ifndef __KOS_ARCH_GENERIC_STRING_MEMCMP_H__
#define __KOS_ARCH_GENERIC_STRING_MEMCMP_H__ 1

#include <kos/compiler.h>

#ifndef __ASSEMBLY__
#include "common.h"
#include <kos/endian.h>
#include <kos/types.h>
#include <stdint.h>

__DECL_BEGIN

#ifndef __arch_memcmp
#ifdef __arch_memcmp_b
#   define __arch_memcmp  __arch_memcmp_b
#else
#   define __arch_generic_memcmp memcmp
#   define __arch_memcmp         memcmp
#ifndef __memcmp_defined
#define __memcmp_defined 1
extern __wunused __purecall __nonnull((1,2)) int
memcmp __P((void const *__a, void const *__b, __size_t __bytes));
#endif /* !__memcmp_defined */
#endif
#endif


#if __BYTE_ORDER == __LITTLE_ENDIAN || \
    __BYTE_ORDER == __BIG_ENDIAN

#if __SIZEOF_POINTER__ >= 2
__forcelocal __constcall int
__arch_getfirstnzbyte_16 __D1(__s16,__b) {
#if __BYTE_ORDER == __LITTLE_ENDIAN
 if (!(__b&UINT16_C(0xff))) __b >>= 8;
 return (int)__b;
#elif __BYTE_ORDER == __BIG_ENDIAN
 if (!(__b&UINT16_C(0xff00))) __b <<= 8;
 return (int)(__b >> 8);
#endif
}
#endif

#if __SIZEOF_POINTER__ >= 4
__forcelocal __constcall int
__arch_getfirstnzbyte_32 __D1(__s32,__b) {
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
#endif

#if __SIZEOF_POINTER__ >= 8
__forcelocal __constcall int
__arch_getfirstnzbyte_64 __D1(__s64,__b) {
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
#endif

extern __attribute_error("Size is too large for small memcmp()")
void __arch_small_memcmp_too_large(void);

__forcelocal __wunused __purecall int __arch_small_memcmp
__D3(void const *,__a,void const *,__b,__size_t,__bytes) {
#define __CMP(T,i) (((T *)__a)[i]-((T *)__b)[i])
 switch (__bytes) {
  case 0: return 0;
  case 1: return (int)__CMP(__s8,0);
  case 2: return __arch_getfirstnzbyte_16(__CMP(__s16,0));
  case 3: {
   __s16 __result = __CMP(__s16,0);
   if (__result) return __arch_getfirstnzbyte_16(__result);
   return (int)__CMP(__s8,2);
  }
  case 4: return __arch_getfirstnzbyte_32(__CMP(__s32,0));
  case 5: {
   __s32 __result = __CMP(__s32,0);
   if (__result) return __arch_getfirstnzbyte_32(__result);
   return (int)__CMP(__s8,4);
  }
  case 6: {
   __s32 __result = __CMP(__s32,0);
   if (__result) return __arch_getfirstnzbyte_32(__result);
   return __arch_getfirstnzbyte_16(__CMP(__s16,2));
  }
  case 7: {
   __s16 __result16;
   __s32 __result32 = __CMP(__s32,0);
   if (__result32) return __arch_getfirstnzbyte_32(__result32);
   __result16 = __CMP(__s16,2);
   if (__result16) return __arch_getfirstnzbyte_16(__result16);
   return (int)__CMP(__s8,6);
  }
#if __SIZEOF_POINTER__ >= 8
  case 8: return __arch_getfirstnzbyte_64(__CMP(__s64,0));
#else
  case 8: {
   __s32 __result32 = __CMP(__s32,0);
   if (__result32) return __arch_getfirstnzbyte_32(__result32);
   return __arch_getfirstnzbyte_32(__CMP(__s32,1));
  }
#endif
  default: __arch_small_memcmp_too_large();
 }
 return 0;
#undef __CMP
}


__forcelocal __wunused __purecall int __arch_constant_memcmp
__D3(void const *,__a,void const *,__b,__size_t,__bytes) {
 /* Optimizations for small sizes. */
 if (__bytes <= __ARCH_STRING_SMALL_LIMIT) {
  return __arch_small_memcmp(__a,__b,__bytes);
 }
 /* Optimizations for qword/dword/word-aligned sizes. */
#ifdef __arch_memcmp_q
 if (!(__bytes%8)) return __arch_getfirstnzbyte_64(__arch_memcmp_q(__a,__b,__bytes/8));
#if __ARCH_STRING_SMALL_LIMIT >= 8
 if (__bytes >= __ARCH_STRING_LARGE_LIMIT_8) {
  /* Special optimization: Large memset. */
  __size_t __offset = __bytes % 8;
  int __result = __arch_small_memcmp(__a,__b,__offset);
  return __result ? __result : __arch_getfirstnzbyte_64(
   __arch_memcmp_q((void *)((__uintptr_t)__a+__offset),
                        (void *)((__uintptr_t)__b+__offset),
                         __bytes/8));
 }
#endif /* __ARCH_STRING_SMALL_LIMIT >= 8 */
#endif /* __arch_memcmp_q */
#ifdef __arch_memcmp_l
 if (!(__bytes%4)) return __arch_getfirstnzbyte_32(__arch_memcmp_l(__a,__b,__bytes/4));
#if __ARCH_STRING_SMALL_LIMIT >= 4
 if (__bytes >= __ARCH_STRING_LARGE_LIMIT_4) {
  /* Special optimization: Large memset. */
  __size_t __offset = __bytes % 4;
  int __result = __arch_small_memcmp(__a,__b,__offset);
  return __result ? __result : __arch_getfirstnzbyte_32(
   __arch_memcmp_l((void *)((__uintptr_t)__a+__offset),
                        (void *)((__uintptr_t)__b+__offset),
                         __bytes/4));
 }
#endif /* __ARCH_STRING_SMALL_LIMIT >= 4 */
#endif /* __arch_memcmp_l */
#ifdef __arch_memcmp_w
 if (!(__bytes%2)) return __arch_getfirstnzbyte_16(__arch_memcmp_w(__a,__b,__bytes/2));
#if __ARCH_STRING_SMALL_LIMIT >= 2
 if (__bytes >= __ARCH_STRING_LARGE_LIMIT_2) {
  /* Special optimization: Large memset. */
  __size_t __offset = __bytes % 2;
  int __result = __arch_small_memcmp(__a,__b,__offset);
  return __result ? __result : __arch_getfirstnzbyte_16(
   __arch_memcmp_w((void *)((__uintptr_t)__a+__offset),
                        (void *)((__uintptr_t)__b+__offset),
                         __bytes/2));
 }
#endif /* __ARCH_STRING_SMALL_LIMIT >= 4 */
#endif /* __arch_memcmp_w */
 /* Fallback. */
 return __arch_memcmp(__a,__b,__bytes);
}
#endif /* Endian... */


#define arch_memcmp  arch_memcmp
__forcelocal __wunused __purecall int arch_memcmp
__D3(void const *,__a,void const *,__b,__size_t,__bytes) {
 /* Check for special case: input pointer known to be equal. */
 if (__builtin_constant_p(__a == __b) && (__a == __b)) {
  return 0;
 }
 if (__builtin_constant_p(__bytes)) {
#if __BYTE_ORDER == __LITTLE_ENDIAN || \
    __BYTE_ORDER == __BIG_ENDIAN
  return __arch_constant_memcmp(__a,__b,__bytes);
#else
  if (!__bytes) return 0;
#endif
 }
 return __arch_memcmp(__a,__b,__bytes);
}


__DECL_END
#endif /* !__ASSEMBLY__ */

#endif /* !__KOS_ARCH_GENERIC_STRING_MEMCMP_H__ */

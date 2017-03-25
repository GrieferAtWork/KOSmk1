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
#ifndef __KOS_ARCH_GENERIC_STRING_MEMMOVE_H__
#define __KOS_ARCH_GENERIC_STRING_MEMMOVE_H__ 1

#include <kos/compiler.h>

#ifndef __ASSEMBLY__
#include "common.h"
#include "memcpy.h"
#include <kos/types.h>
#include <stdint.h>

__DECL_BEGIN

#ifndef __arch_memmove
#ifdef __arch_memmove_b
#   define __arch_memmove  __arch_memmove_b
#else
#   define __arch_generic_memmove memmove
#   define __arch_memmove         memmove
#ifndef __memmove_defined
#define __memmove_defined 1
extern __retnonnull __nonnull((1,2)) void *
memmove __P((void *__dst,
             void const *__src,
             __size_t __bytes));
#endif
#endif
#endif


extern __attribute_error("Size is too large for small memmove()")
void __arch_small_memmove_too_large __P((void));

#define __D(T,i) ((T *)__dst)[i]
#define __S(T,i) ((T *)__src)[i]
#define __X(T,i) __D(T,i) = __S(T,i)
__forcelocal __retnonnull __nonnull((1,2)) void *__arch_small_memmove
__D3(void *,__dst,void const *,__src,__size_t,__bytes) {
 switch (__bytes) {
  case 0:                                        break;
  case 1: __X(__u8,0);                           break;
  case 2: __X(__u16,0);                          break;
  case 4: __X(__u32,0);                          break;
#if __SIZEOF_POINTER__ >= 8
  case 8: __X(__u64,0);                          break;
#else
  case 8: {
   register __u32 __temp;
   __temp = __S(__u32,1);
   __X(__u32,0);
   __D(__u32,1) = __temp;
  } break;
#endif
  case 3: {
   register __u8 __temp;
   __temp = __S(__u8,2);
   __X(__u16,0);
   __D(__u8,2) = __temp;
  } break;
  case 5: {
   register __u8 __temp;
   __temp = __S(__u8,4);
   __X(__u32,0);
   __D(__u8,4) = __temp;
  } break;
  case 6: {
   register __u16 __temp;
   __temp = __S(__u16,2);
   __X(__u32,0);
   __D(__u16,2) = __temp;
  } break;
  case 7: {
   register __u16 __temp16;
   register __u8 __temp8;
   __temp8  = __S(__u8,6);
   __temp16 = __S(__u16,2);
   __X(__u32,0);
   __D(__u16,2) = __temp16;
   __D(__u8,6) = __temp8;
  } break;
  default: __arch_small_memmove_too_large();
 }
 return __dst;
}

__forcelocal __retnonnull __nonnull((1,2)) void *__arch_constant_memmove
__D3(void *,__dst,void const *,__src,__size_t,__bytes) {
 /* Special optimization: Small memmove. */
 if (__bytes <= __ARCH_STRING_SMALL_LIMIT) {
  return __arch_small_memmove(__dst,__src,__bytes);
 }
 /* Fallback: Perform a regular, old memmove(). */
 return __arch_memmove(__dst,__src,__bytes);
}
#undef __X
#undef __S
#undef __D

#define __CTIME(x) (__builtin_constant_p(x) && (x))
#define arch_memmove   arch_memmove
__forcelocal __retnonnull __nonnull((1,2)) void *arch_memmove
__D3(void *,__dst,void const *,__src,__size_t,__bytes) {
 /* Compile-time special case: DST and SRC are equal. */
 if (__CTIME(__dst == __src)) {
  return __dst;
 }
 /* Compile-time special case: DST and SRC are not overlapping. */
 if (__CTIME((__uintptr_t)__dst+__bytes < (__uintptr_t)__src ||
             (__uintptr_t)__src+__bytes < (__uintptr_t)__dst)) {
  return arch_memcpy(__dst,__src,__bytes);
 }
 return __builtin_constant_p(__bytes)
  ? __arch_constant_memmove(__dst,__src,__bytes)
  :          __arch_memmove(__dst,__src,__bytes);
#undef __CTIME
}

__DECL_END
#endif /* !__ASSEMBLY__ */

#endif /* !__KOS_ARCH_GENERIC_STRING_MEMMOVE_H__ */

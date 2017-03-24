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
#ifndef __KOS_ARCH_GENERIC_STRING_MEMLEN_H__
#define __KOS_ARCH_GENERIC_STRING_MEMLEN_H__ 1

#include <kos/compiler.h>

#ifndef __ASSEMBLY__
#include "common.h"
#include <kos/types.h>
#include <stdint.h>

__DECL_BEGIN

#ifndef __arch_umemlen
#ifdef __arch_umemlen_b
#   define __arch_umemlen  __arch_umemlen_b
#else
#   define __arch_generic_umemlen _umemlen
#   define __arch_umemlen         _umemlen
#ifndef ___umemlen_defined
#define ___umemlen_defined 1
extern __wunused __purecall __nonnull((1)) __size_t
_umemlen __P((void const *__restrict __haystack, int __needle));
#endif
#endif
#endif

#define arch_umemlen  arch_umemlen
__forcelocal __wunused __purecall __nonnull((1)) __size_t
arch_umemlen __D2(void const *__restrict,__haystack,int,__needle) {
#ifdef __arch_strlen
 if (__builtin_constant_p(needle) && !needle) {
  return (void *)__arch_strlen((char const *)__haystack);
 }
#endif
 return __arch_umemlen(__haystack,__needle);
}


#ifndef __arch_memlen
#ifdef __arch_memlen_b
#   define __arch_memlen  __arch_memlen_b
#else
#   define __arch_generic_memlen _memlen
#   define __arch_memlen         _memlen
#ifndef ___memlen_defined
#define ___memlen_defined 1
extern __wunused __purecall __nonnull((1)) __size_t
_memlen __P((void const *__restrict __haystack,
             int __needle, __size_t __bytes));
#endif
#endif
#endif


extern __attribute_error("Size is too large for small memlen()")
void __arch_small_memlen_too_large(void);

#define __KARCH_SMALL_MEMLEN_MAXSIZE   8
__forcelocal __wunused __purecall __nonnull((1)) __size_t __arch_small_memlen
__D3(void const *__restrict,__haystack,int,__needle,__size_t,__bytes) {
#define __BYTE(i)  if (((__u8 *)__haystack)[i] == __needle) return i
 if (!__bytes) return 0;
 if (__bytes >= 1) __BYTE(0);
 if (__bytes >= 2) __BYTE(1);
 if (__bytes >= 3) __BYTE(2);
 if (__bytes >= 4) __BYTE(3);
 if (__bytes >= 5) __BYTE(4);
 if (__bytes >= 6) __BYTE(5);
 if (__bytes >= 7) __BYTE(6);
 if (__bytes >= 8) __BYTE(7);
 if (__bytes >= 9) __arch_small_memlen_too_large();
#undef __BYTE
 return 8;
}

__forcelocal __wunused __purecall __nonnull((1)) __size_t __arch_constant_memlen
__D3(void const *,__haystack,int,__needle,__size_t,__bytes) {
 /* Special case: Small search. */
 if (__bytes < __KARCH_SMALL_MEMLEN_MAXSIZE) {
  return __arch_small_memlen(__haystack,__needle,__bytes);
 }
 /* Special case: Infinite-search. */
 if (__bytes == ~(__size_t)0) {
  return arch_umemlen(__haystack,__needle);
 }
#ifdef __arch_strnlen
 /* Special case: strnlen-style search. */
 if (__builtin_constant_p(__needle) && !__needle) {
  return __arch_strnlen((char *)haystack,bytes);
 }
#endif /* __arch_strnend */
 return __arch_memlen(__haystack,__needle,__bytes);
}


#define arch_memlen  arch_memlen
__forcelocal __wunused __purecall __nonnull((1)) __size_t arch_memlen
__D3(void const *,__haystack,int,__needle,__size_t,__bytes) {
 if (__builtin_constant_p(__bytes)) {
  return __arch_constant_memlen(__haystack,__needle,__bytes);
 }
#ifdef __arch_strnlen
 if (__builtin_constant_p(__needle) && !__needle) {
  return __arch_strnlen((char *)haystack,bytes);
 }
#endif
 return __arch_memlen(__haystack,__needle,__bytes);
}


__DECL_END
#endif /* !__ASSEMBLY__ */

#endif /* !__KOS_ARCH_GENERIC_STRING_MEMLEN_H__ */

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
#ifndef __KOS_ARCH_GENERIC_STRING_MEMEND_H__
#define __KOS_ARCH_GENERIC_STRING_MEMEND_H__ 1

#include <kos/compiler.h>

#ifndef __ASSEMBLY__
#include "common.h"
#include <kos/types.h>
#include <stdint.h>

__DECL_BEGIN

#ifndef __arch_umemend
#ifdef __arch_umemend_b
#   define __arch_umemend  __arch_umemend_b
#else
#   define __arch_generic_umemend _umemend
#   define __arch_umemend         _umemend
#ifndef ___umemend_defined
#define ___umemend_defined 1
#if defined(__cplusplus) && !defined(__NO_asmname) && !defined(__STRING_C__)
extern "C++" {
extern __wunused __purecall __nonnull((1)) void *_umemend(void *__restrict __haystack, int __needle) __asmname("_umemend");
extern __wunused __purecall __nonnull((1)) void const *_umemend(void const *__restrict __haystack, int __needle) __asmname("_umemend");
}
#else /* CPP */
extern __wunused __purecall __nonnull((1)) void *
_umemend __P((void const *__restrict __haystack, int __needle));
#endif /* !CPP */
#endif
#endif
#endif

#define arch_umemend  arch_umemend
__forcelocal __wunused __purecall __nonnull((1)) void *
arch_umemend __D2(void const *__restrict,__haystack,int,__needle) {
#ifdef __arch_strend
 if (__builtin_constant_p(needle) && !needle) {
  return (void *)__arch_strend((char const *)__haystack);
 }
#endif
 return __arch_umemend(__haystack,__needle);
}


#ifndef __arch_memend
#ifdef __arch_memend_b
#   define __arch_memend  __arch_memend_b
#else
#   define __arch_generic_memend _memend
#   define __arch_memend         _memend
#ifndef ___memend_defined
#define ___memend_defined 1
#if defined(__cplusplus) && !defined(__NO_asmname) && !defined(__STRING_C__)
extern "C++" {
extern __wunused __purecall __nonnull((1)) void *_memend(void *__restrict __haystack, int __needle, __size_t __bytes) __asmname("_memend");
extern __wunused __purecall __nonnull((1)) void const *_memend(void const *__restrict __haystack, int __needle, __size_t __bytes) __asmname("_memend");
}
#else /* CPP */
extern __wunused __purecall __nonnull((1)) void *
_memend __P((void const *__restrict __haystack,
             int __needle, __size_t __bytes));
#endif /* !CPP */
#endif
#endif
#endif


extern __attribute_error("Size is too large for small memend()")
void __arch_small_memend_too_large(void);

__forcelocal __wunused __purecall __nonnull((1)) void *
__arch_small_memend __D3(void const *,__haystack,int,__needle,__size_t,__bytes) {
#define __BYTE(i)  if (((__u8 *)__haystack)[i] == __needle) return (__u8 *)__haystack+(i)
 if (!__bytes) return (void *)__haystack;
 if (__bytes >= 1) __BYTE(0);
 if (__bytes >= 2) __BYTE(1);
 if (__bytes >= 3) __BYTE(2);
 if (__bytes >= 4) __BYTE(3);
 if (__bytes >= 5) __BYTE(4);
 if (__bytes >= 6) __BYTE(5);
 if (__bytes >= 7) __BYTE(6);
 if (__bytes >= 8) __BYTE(7);
 if (__bytes >= 9) __arch_small_memend_too_large();
#undef __BYTE
 return (__u8 *)__haystack+8;
}
__forcelocal __wunused __purecall __nonnull((1)) void *
__arch_constant_memend __D3(void const *,__haystack,
                            int,__needle,__size_t,__bytes) {
 if (__bytes < __ARCH_STRING_SMALL_LIMIT) {
  return __arch_small_memend(__haystack,__needle,__bytes);
 }
#ifdef __arch_strnend
 if (__builtin_constant_p(__needle) && !__needle) {
  return (void *)__arch_strnend((char *)haystack,bytes);
 }
#endif /* __arch_strnend */
 return (void *)__arch_memend(__haystack,__needle,__bytes);
}


#define arch_memend arch_memend
__forcelocal __wunused __purecall __nonnull((1)) void *
arch_memend __D3(void const *,__haystack,
                 int,__needle,__size_t,__bytes) {
 /* Special case: Small search. */
 if (__builtin_constant_p(__bytes)) {
  return __arch_constant_memend(__haystack,__needle,__bytes);
 }
 /* Special case: Infinite-search. */
 if (__bytes == ~(__size_t)0) {
  return arch_umemend(__haystack,__needle);
 }
#ifdef __arch_strnend
 /* Special case: strnend-style search. */
 if (__builtin_constant_p(__needle) && !__needle) {
  return (void *)__arch_strnend((char *)__haystack,__bytes);
 }
#endif
 return (void *)__arch_memend(__haystack,__needle,__bytes);
}


__DECL_END
#endif /* !__ASSEMBLY__ */

#endif /* !__KOS_ARCH_GENERIC_STRING_MEMEND_H__ */

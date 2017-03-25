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
#ifndef __KOS_ARCH_GENERIC_STRING_MEMRCHR_H__
#define __KOS_ARCH_GENERIC_STRING_MEMRCHR_H__ 1

#include <kos/compiler.h>

#ifndef __ASSEMBLY__
#include "common.h"
#include <kos/types.h>
#include <stddef.h>
#include <stdint.h>

__DECL_BEGIN

#ifndef __arch_memrchr
#ifdef __arch_memrchr_b
#   define __arch_memrchr  __arch_memrchr_b
#else
#   define __arch_generic_memrchr memrchr
#   define __arch_memrchr         memrchr
#ifndef __memrchr_defined
#define __memrchr_defined 1
#if defined(__cplusplus) && !defined(__NO_asmname) && !defined(__STRING_C__)
extern "C++" {
extern __wunused __purecall __nonnull((1)) void *memrchr(void *__restrict __p, int __needle, size_t __bytes) __asmname("memrchr");
extern __wunused __purecall __nonnull((1)) void const *memrchr(void const *__restrict __p, int __needle, size_t __bytes) __asmname("memrchr");
}
#else /* CPP */
extern __wunused __purecall __nonnull((1)) void *memrchr __P((void const *__restrict __p, int __needle, size_t __bytes));
#endif /* !CPP */
#endif
#endif
#endif

extern __attribute_error("Size is too large for small memrchr()")
void __arch_small_memrchr_too_large(void);

__forcelocal __wunused void *
__arch_small_memrchr __D3(void const *__restrict,__haystack,
                          int,__needle,__size_t,__bytes) {
#define __BYTE(i)  if (((__u8 *)__haystack)[i] == __needle) return (__u8 *)__haystack+(i)
 switch (__bytes) {
  case 0: return NULL;
  case 8: __BYTE(7); /* fallthrough */
  case 7: __BYTE(6); /* fallthrough */
  case 6: __BYTE(5); /* fallthrough */
  case 5: __BYTE(4); /* fallthrough */
  case 4: __BYTE(3); /* fallthrough */
  case 3: __BYTE(2); /* fallthrough */
  case 2: __BYTE(1); /* fallthrough */
  case 1: __BYTE(0); /* fallthrough */
  default: __arch_small_memrchr_too_large();
 }
#undef __BYTE
 return NULL;
}

#define arch_memrchr arch_memrchr
__forcelocal __wunused void *
arch_memrchr __D3(void const *__restrict,__haystack,
                  int,__needle,__size_t,__bytes) {
 if (__builtin_constant_p(__bytes) &&
     __bytes <= __ARCH_STRING_SMALL_LIMIT) {
  return __arch_small_memrchr(__haystack,__needle,__bytes);
 }
 return (void *)__arch_memrchr(__haystack,__needle,__bytes);
}


#ifdef __arch_memrchr_w
__forcelocal __wunused void *
__arch_small_memrchr_w __D3(void const *__restrict,__haystack,
                            __u16,__needle,__size_t,__bytes) {
#define __WORD(i)  if (((__u16 *)__haystack)[i] == __needle) return (__u16 *)__haystack+(i)
 switch (__bytes) {
  case 0: return NULL;
  case 8: __WORD(7); /* fallthrough */
  case 7: __WORD(6); /* fallthrough */
  case 6: __WORD(5); /* fallthrough */
  case 5: __WORD(4); /* fallthrough */
  case 4: __WORD(3); /* fallthrough */
  case 3: __WORD(2); /* fallthrough */
  case 2: __WORD(1); /* fallthrough */
  case 1: __WORD(0); /* fallthrough */
  default: __arch_small_memrchr_too_large();
 }
#undef __WORD
 return NULL;
}

#define arch_memrchr_w arch_memrchr_w
__forcelocal __wunused void *
arch_memrchr_w __D3(void const *__restrict,__haystack,
                    __u16,__needle,__size_t,__bytes) {
 if (__builtin_constant_p(__bytes) &&
     __bytes <= __ARCH_STRING_SMALL_LIMIT) {
  return __arch_small_memrchr_w(__haystack,__needle,__bytes);
 }
 return (void *)__arch_memrchr_w(__haystack,__needle,__bytes);
}
#endif /* __arch_memrchr_w */

#ifdef __arch_memrchr_l
__forcelocal __wunused void *
__arch_small_memrchr_l __D3(void const *__restrict,__haystack,
                            __u32,__needle,__size_t,__bytes) {
#define __DWORD(i)  if (((__u32 *)__haystack)[i] == __needle) return (__u32 *)__haystack+(i)
 switch (__bytes) {
  case 0: return NULL;
  case 8: __DWORD(7); /* fallthrough */
  case 7: __DWORD(6); /* fallthrough */
  case 6: __DWORD(5); /* fallthrough */
  case 5: __DWORD(4); /* fallthrough */
  case 4: __DWORD(3); /* fallthrough */
  case 3: __DWORD(2); /* fallthrough */
  case 2: __DWORD(1); /* fallthrough */
  case 1: __DWORD(0); /* fallthrough */
  default: __arch_small_memrchr_too_large();
 }
#undef __DWORD
 return NULL;
}

#define arch_memrchr_l arch_memrchr_l
__forcelocal __wunused void *
arch_memrchr_l __D3(void const *__restrict,__haystack,
                    __u32,__needle,__size_t,__bytes) {
 if (__builtin_constant_p(__bytes) &&
     __bytes <= __ARCH_STRING_SMALL_LIMIT) {
  return __arch_small_memrchr_l(__haystack,__needle,__bytes);
 }
 return (void *)__arch_memrchr_l(__haystack,__needle,__bytes);
}
#endif /* __arch_memrchr_l */

#ifdef __arch_memrchr_q
__forcelocal __wunused void *
__arch_small_memrchr_q __D3(void const *__restrict,__haystack,
                            __u64,__needle,__size_t,__bytes) {
#define __QWORD(i)  if (((__u64 *)__haystack)[i] == __needle) return (__u64 *)__haystack+(i)
 switch (__bytes) {
  case 0: return NULL;
  case 8: __QWORD(7); /* fallthrough */
  case 7: __QWORD(6); /* fallthrough */
  case 6: __QWORD(5); /* fallthrough */
  case 5: __QWORD(4); /* fallthrough */
  case 4: __QWORD(3); /* fallthrough */
  case 3: __QWORD(2); /* fallthrough */
  case 2: __QWORD(1); /* fallthrough */
  case 1: __QWORD(0); /* fallthrough */
  default: __arch_small_memrchr_too_qarge();
 }
#undef __QWORD
 return NULL;
}

#define arch_memrchr_q arch_memrchr_q
__forcelocal __wunused void *
arch_memrchr_q __D3(void const *__restrict,__haystack,
                    __u64,__needle,__size_t,__bytes) {
 if (__builtin_constant_p(__bytes) &&
     __bytes <= __ARCH_STRING_SMALL_LIMIT) {
  return __arch_small_memrchr_q(__haystack,__needle,__bytes);
 }
 return (void *)__arch_memrchr_q(__haystack,__needle,__bytes);
}
#endif /* __arch_memrchr_q */

__DECL_END
#endif /* !__ASSEMBLY__ */

#endif /* !__KOS_ARCH_GENERIC_STRING_MEMRCHR_H__ */

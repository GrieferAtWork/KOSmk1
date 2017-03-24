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
#ifndef __KOS_ARCH_GENERIC_STRING_MEMMEM_H__
#define __KOS_ARCH_GENERIC_STRING_MEMMEM_H__ 1

#include <kos/compiler.h>

#ifndef __ASSEMBLY__
#include "common.h"
#include "memcmp.h"
#include "memchr.h"
#include <kos/types.h>
#include <stddef.h>
#include <stdint.h>

__DECL_BEGIN

#ifndef __arch_memmem
#if defined(arch_memrchr) && \
  ((defined(__arch_memcmp) || defined(__arch_memcmp_b)) &&\
   (defined(__arch_memcmp_w) || defined(__arch_memcmp_l) ||\
    defined(__arch_memcmp_q)))

#define __MAKE_MEMMEM(mnemonic,length) \
/* Optimized version for 'NEEDLELEN%length == 0' */\
__forcelocal __wunused __purecall __nonnull((1,3)) void *__arch_memmem_##mnemonic \
__D4(void const *,__haystack,__size_t,__haystacklen,void const *,__needle,__size_t,__needlelen) {\
 void const *__candidate,*__haystackend; __u8 __key;\
 __haystackend = (void const *)((__uintptr_t)__haystack+(__haystacklen-__needlelen));\
 __key         = *(__u8 *)__needle;\
 __needlelen  /= length;\
 while ((__candidate = arch_memchr(__haystack,__key,\
        (__uintptr_t)__haystackend-(__uintptr_t)__haystack)) != NULL) {\
  /* Check the candidate for really being a match. */\
  if (!__arch_memcmp_##mnemonic(__candidate,__needle,__needlelen)) return (void *)__candidate;\
  /* Continue searching 1 byte after the candidate. */\
  *(__uintptr_t *)&__haystack = (__uintptr_t)__candidate+1;\
 }\
 return NULL;\
}
#define __MAKE_MEMMEM1(mnemonic,length) \
/* Optimized version for 'NEEDLELEN%length == 1' */\
__forcelocal __wunused __purecall __nonnull((1,3)) void *__arch_memmem_##mnemonic##1 \
__D4(void const *,__haystack,__size_t,__haystacklen,void const *,__needle,__size_t,__needlelen) {\
 void const *__candidate,*__haystackend; __u8 __key;\
 __haystackend = (void const *)((__uintptr_t)__haystack+(__haystacklen-__needlelen));\
 __key         = *(__u8 *)__needle;\
 ++*(__uintptr_t *)&__needle,--__needlelen;\
 __needlelen /= length;\
 while ((__candidate = arch_memchr(__haystack,__key,\
        (__uintptr_t)__haystackend-(__uintptr_t)__haystack)) != NULL) {\
  /* Check the candidate for really being a match. */\
  if (!__arch_memcmp_##mnemonic((void const *)((__uintptr_t)__candidate+1),\
                                     __needle,__needlelen)) return (void *)__candidate;\
  /* Continue searching 1 byte after the candidate. */\
  *(__uintptr_t *)&__haystack = (__uintptr_t)__candidate+1;\
 }\
 return NULL;\
}

#ifdef __arch_memcmp_w
__MAKE_MEMMEM(w,2)
__MAKE_MEMMEM1(w,2)
#else
#ifdef __arch_memcmp_b
__MAKE_MEMMEM(b,1)
#endif
#endif
#ifdef __arch_memcmp_l
__MAKE_MEMMEM(l,4)
__MAKE_MEMMEM1(l,4)
#endif
#ifdef __arch_memcmp_q
__MAKE_MEMMEM(q,8)
__MAKE_MEMMEM1(q,8)
#endif
#undef __MAKE_MEMMEM1
#undef __MAKE_MEMMEM

/* Merging all memmem algorithms, select the most appropriate one. */
__forcelocal __wunused __purecall __nonnull((1,3)) void *__arch_memmem_impl
__D4(void const *,__haystack,__size_t,__haystacklen,void const *,__needle,__size_t,__needlelen) {
#ifdef assert
 assert(__needlelen && __needlelen <= __haystacklen);
#endif
#ifdef __arch_memcmp_q
 switch (__needlelen%8) {
  case 0: return __arch_memmem_q (__haystack,__haystacklen,__needle,__needlelen);
  case 1: return __arch_memmem_q1(__haystack,__haystacklen,__needle,__needlelen);
  default: break;
 }
#endif
#ifdef __arch_memcmp_l
 switch (__needlelen%4) {
  case 0: return __arch_memmem_l (__haystack,__haystacklen,__needle,__needlelen);
  case 1: return __arch_memmem_l1(__haystack,__haystacklen,__needle,__needlelen);
  default: break;
 }
#endif
#ifdef __arch_memcmp_w
 if (!(__needlelen%2)) return __arch_memmem_w(__haystack,__haystacklen,__needle,__needlelen);
 /* The last possibility now is that the needle length is uneven. */
 return __arch_memmem_w1(__haystack,__haystacklen,__needle,__needlelen);
#else
 return __arch_memmem_b(__haystack,__haystacklen,__needle,__needlelen);
#endif
}

#define __arch_memmem   __arch_memmem
__forcelocal __wunused __purecall __nonnull((1,3)) void *__arch_memmem
__D4(void const *,__haystack,__size_t,__haystacklen,void const *,__needle,__size_t,__needlelen) {
 /* Handle special cases: Big needle and empty needle. */
 if __unlikely(__needlelen > __haystacklen || !__needlelen) return NULL;
 return __arch_memmem_impl(__haystack,__haystacklen,__needle,__needlelen);
}
#else
#   define __arch_generic_memmem memmem
#   define __arch_memmem         memmem
#ifndef __memmem_defined
#define __memmem_defined 1
#if defined(__cplusplus) && !defined(__NO_asmname) && !defined(__STRING_C__)
extern "C++" {
extern __wunused __purecall __nonnull((1,3)) void *memmem(void *__haystack, size_t __haystacklen, void const *__needle, size_t __needlelen) __asmname("memmem");
extern __wunused __purecall __nonnull((1,3)) void const *memmem(void const *__haystack, size_t __haystacklen, void const *__needle, size_t __needlelen) __asmname("memmem");
}
#else /* CPP */
extern __wunused __purecall __nonnull((1,3)) void *
memmem __P((void const *__haystack, size_t __haystacklen,
            void const *__needle, size_t __needlelen));
#endif /* !CPP */
#endif
#endif
#endif




#define arch_memmem    arch_memmem
__forcelocal __wunused __purecall __nonnull((1,3)) void *arch_memmem
__D4(void const *,__haystack,__size_t,__haystacklen,
     void const *,__needle,__size_t,__needlelen) {
 /* Compile-time special case: Haystack and needle are of equal size (just do a memcmp) */
 if (__builtin_constant_p(__haystacklen == __needlelen) && (__haystacklen == __needlelen)) {
  return !arch_memcmp(__haystack,__needle,__haystacklen) ? (void *)__haystack : NULL;
 }
 /* Compile-time special case: Haystack is smaller than the needle. */
 if (__builtin_constant_p(__haystacklen < __needlelen) && (__haystacklen < __needlelen)) {
  return NULL;
 }
 /* Compile-time special case: Needle is empty. */
 if (__builtin_constant_p(__haystacklen < __needlelen) && (__haystacklen < __needlelen)) {
  return NULL;
 }
 /* Compile-time special case: The haystack _is_ the needle. */
 if (__builtin_constant_p(__haystack == __needle) && (__haystack == __needle)) {
  return (__haystacklen >= __needlelen) ? (void *)__haystack : NULL;
 }
 /* Compile-time special case: Single-byte needle. */
 if (__builtin_constant_p(__needlelen) && __needlelen == 1) {
  return arch_memchr(__haystack,*(__u8 *)__needle,__haystacklen);
 }
 /* Compile-time special case: The haystack size is known. */
 if (__builtin_constant_p(__haystacklen)) {
  /* Special optimization for small haystacks. */
  switch (__haystacklen) {
   case 0: return NULL;
   case 1:
    if (!__needlelen) return NULL;
    return (*(__u8 *)__haystack == *(__u8 *)__needle) ? (void *)__haystack : NULL;
   case 2:
    if (__needlelen == 2) {
     return (*(__u16 *)__haystack == *(__u16 *)__needle) ? (void *)__haystack : NULL;
    } else if (__needlelen == 1) {
     __u8 __key = *(__u8 *)__needle;
     /* Needle must be 1 byte long. */
     if (((__u8 *)__haystack)[0] == __key) return (void *)((__uintptr_t)__haystack);
     if (((__u8 *)__haystack)[1] == __key) return (void *)((__uintptr_t)__haystack+1);
    }
    return NULL;
   default: break;
  }
 }
 return (void *)__arch_memmem(__haystack,__haystacklen,__needle,__needlelen);
}

__DECL_END
#endif /* !__ASSEMBLY__ */

#endif /* !__KOS_ARCH_GENERIC_STRING_MEMMEM_H__ */

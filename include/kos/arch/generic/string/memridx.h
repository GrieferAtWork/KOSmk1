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
#ifndef __KOS_ARCH_GENERIC_STRING_MEMRIDX_H__
#define __KOS_ARCH_GENERIC_STRING_MEMRIDX_H__ 1

#include <kos/compiler.h>

#ifndef __ASSEMBLY__
#include "common.h"
#include "memrchr.h"
#include <kos/types.h>
#include <stddef.h>
#include <stdint.h>

__DECL_BEGIN

#ifndef __arch_memridx
#   define __arch_generic_memridx _memridx
#   define __arch_memridx         _memridx
#ifndef ___memridx_defined
#define ___memridx_defined 1
#if defined(__cplusplus) && !defined(__NO_asmname) && !defined(__STRING_C__)
extern "C++" {
extern __wunused __purecall __nonnull((1,3)) void *_memridx(void *__vector, __size_t __elemcount, void const *__pattern, __size_t __elemsize, __size_t __elemalign) __asmname("_memridx");
extern __wunused __purecall __nonnull((1,3)) void const *_memridx(void const *__vector, __size_t __elemcount, void const *__pattern, __size_t __elemsize, __size_t __elemalign) __asmname("_memridx");
}
#else /* CPP */
extern __wunused __purecall __nonnull((1,3)) void *
_memridx __P((void const *__vector, __size_t __elemcount,
              void const *__pattern, __size_t __elemsize,
              __size_t __elemalign));
#endif /* !CPP */
#endif
#endif



#define arch_memridx    arch_memridx
__forcelocal __wunused __purecall __nonnull((1,3)) void *arch_memridx
__D5(void const *,__vector,__size_t,__elemcount,
     void const *,__pattern,__size_t,__elemsize,
     __size_t,__elemalign) {
 /* Compile-time special case: Empty vector. */
 if (__builtin_constant_p(__elemcount) && !__elemcount) return NULL;
 /* Compile-time special case: Known element size. */
 if (__builtin_constant_p(__elemsize)) {
  if (!__elemsize) return NULL;
  /* Compile-time special case: size-aligned vector with known element length.
   * NOTE: This optimization is used a _lot_ (e.g.: when scanning an array) */
  if (__builtin_constant_p(__elemalign) && __elemalign == __elemsize) {
   switch (__elemsize) {
    case 1: return arch_memrchr(__vector,*(__u8 *)__pattern,__elemcount);
#ifdef arch_memrchr_w
    case 2: return arch_memrchr_w(__vector,*(__u16 *)__pattern,__elemcount);
#endif
#ifdef arch_memrchr_l
    case 4: return arch_memrchr_l(__vector,*(__u32 *)__pattern,__elemcount);
#endif
#ifdef arch_memrchr_q
    case 8: return arch_memrchr_q(__vector,*(__u64 *)__pattern,__elemcount);
#endif
    default: break;
   }
  }
 }
 return (void *)__arch_memridx(__vector,__elemcount,__pattern,__elemsize,__elemalign);
}

__DECL_END
#endif /* !__ASSEMBLY__ */

#endif /* !__KOS_ARCH_GENERIC_STRING_MEMRIDX_H__ */

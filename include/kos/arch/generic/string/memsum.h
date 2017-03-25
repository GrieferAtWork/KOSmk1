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
#ifndef __KOS_ARCH_GENERIC_STRING_MEMSUM_H__
#define __KOS_ARCH_GENERIC_STRING_MEMSUM_H__ 1

#include <kos/compiler.h>

#ifndef __ASSEMBLY__
#include "common.h"
#include <kos/types.h>
#include <stdint.h>

__DECL_BEGIN

#ifndef __arch_memsum
#ifdef __arch_memsum_b
#   define __arch_memsum  __arch_memsum_b
#else
#   define __arch_generic_memsum _memsum
#   define __arch_memsum         _memsum
#ifndef __memsum_defined
#define __memsum_defined 1
extern __wunused __purecall __nonnull((1)) __byte_t
_memsum __P((void const *__restrict __p, __size_t __bytes));
#endif
#endif
#endif


__forcelocal __wunused __purecall __nonnull((1)) __byte_t
__arch_constant_memsum __D2(void const *__restrict,__p,__size_t,__bytes) {
 switch (__bytes) {
  case 0: return 0;
  case 1: return *(__byte_t *)__p;
  case 2: return *(__byte_t *)__p+((__byte_t *)__p)[1];
  default: break;
 }
 return __arch_memsum(__p,__bytes);
}

__forcelocal __wunused __purecall __nonnull((1)) __byte_t
arch_memsum __D2(void const *__restrict,__p,__size_t,__bytes) {
 return __builtin_constant_p(__bytes)
   ? __arch_constant_memsum(__p,__bytes)
   :          __arch_memsum(__p,__bytes);
}


__DECL_END
#endif /* !__ASSEMBLY__ */

#endif /* !__KOS_ARCH_GENERIC_STRING_MEMSUM_H__ */

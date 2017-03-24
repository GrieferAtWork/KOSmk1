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
#ifndef __STRING_C__
#define __STRING_C__ 1
#undef __STDC_PURE__

#include <assert.h>
#include <ctype.h>
#include <malloc.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>

#ifndef __KERNEL__
#include <errno.h>
#endif

#undef __LIBC_HAVE_DEBUG_MEMCHECKS

#include <kos/arch/string.h>
#include <kos/arch/generic/string.h>
#include <kos/config.h>
#ifdef __LIBC_HAVE_DEBUG_MEMCHECKS
#   include <kos/kernel/debug.h>
#   define __STRING_ASSERTBYTE(p)  kassertbyte(p)
#   define __STRING_ASSERTMEM(p,s) kassertmem(p,s)
#else
#   define __STRING_ASSERTBYTE(p)  (void)0
#   define __STRING_ASSERTMEM(p,s) assert(!(s) || (p))
#endif

/* Avoid repeated use of strchr.
 * Instead, cache the string's length and use memchr.
 * >> This mode is used if the arch doesn't provide
 *    special optimizations for strchr, but does
 *    offer some for memchr and strlen. */
#if (!defined(__arch_generic_memchr) && \
     !defined(__arch_generic_strlen)) && \
      defined(__arch_generic_strchr)
#define STRING_AVOID_STRCHR 1
#endif
#if (!defined(__arch_generic_memchr) && \
     !defined(__arch_generic_strnlen)) && \
      defined(__arch_generic_strnchr)
#define STRING_AVOID_STRNCHR 1
#endif

#ifndef __LIBC_HAVE_DEBUG_MEMCHECKS
#   define string_memchr  arch_memchr
#   define string_memcmp  arch_memcmp
#   define string_strlen  arch_strlen
#   define string_strnlen arch_strnlen
#else /* !__LIBC_HAVE_DEBUG_MEMCHECKS */
#   define string_memchr  memchr
#   define string_memcmp  memcmp
#   define string_strlen  strlen
#   define string_strnlen strnlen
#endif /* __LIBC_HAVE_DEBUG_MEMCHECKS */


__DECL_BEGIN

#if !defined(_ffs8) || \
     defined(__arch_generic_ffs8) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef _ffs8
__public int _ffs8(__u8 i) {
#if defined(__arch_ffs8) && !defined(__arch_generic_ffs8)
 return __arch_ffs8(i);
#elif defined(__arch_ffs16) && !defined(__arch_generic_ffs16)
 return __arch_ffs16((__u16)i);
#elif defined(__arch_ffs32) && !defined(__arch_generic_ffs32)
 return __arch_ffs32((__u32)i);
#elif defined(__arch_ffs64) && !defined(__arch_generic_ffs64)
 return __arch_ffs64((__u64)i);
#else
 int result;
 if (!i) return 0;
 for (result = 1; !(i&1); ++result) i >>= 1;
 return result;
#endif
}
#endif


#if !defined(_ffs16) || \
     defined(__arch_generic_ffs16) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef _ffs16
__public int _ffs16(__u16 i) {
#if defined(__arch_ffs16) && !defined(__arch_generic_ffs16)
 return __arch_ffs16(i);
#elif defined(__arch_ffs32) && !defined(__arch_generic_ffs32)
 return __arch_ffs32((__u32)i);
#elif defined(__arch_ffs64) && !defined(__arch_generic_ffs64)
 return __arch_ffs64((__u64)i);
#else
 int result;
 if (!i) return 0;
 for (result = 1; !(i&1); ++result) i >>= 1;
 return result;
#endif
}
#endif


#if !defined(_ffs32) || \
     defined(__arch_generic_ffs32) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef _ffs32
__public int _ffs32(__u32 i) {
#if defined(__arch_ffs32) && !defined(__arch_generic_ffs32)
 return __arch_ffs32(i);
#elif defined(__arch_ffs64) && !defined(__arch_generic_ffs64)
 return __arch_ffs64((__u64)i);
#else
 int result;
 if (!i) return 0;
 for (result = 1; !(i&1); ++result) i >>= 1;
 return result;
#endif
}
#endif


#if !defined(_ffs64) || \
     defined(__arch_generic_ffs64) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef _ffs64
__public int _ffs64(__u64 i) {
#if defined(__arch_ffs64) && \
   !defined(__arch_generic_ffs64)
 return __arch_ffs64(i);
#else
 int result;
 if (!i) return 0;
 for (result = 1; !(i&1); ++result) i >>= 1;
 return result;
#endif
}
#endif


#ifndef __CONFIG_MIN_LIBC__
#undef ffs
#undef ffsl
#undef ffsll
__public __COMPILER_ALIAS(ffs,__PP_CAT_2(_ffs,__PP_MUL8(__SIZEOF_INT__)));
__public __COMPILER_ALIAS(ffsl,__PP_CAT_2(_ffs,__PP_MUL8(__SIZEOF_LONG__)));
__public __COMPILER_ALIAS(ffsll,__PP_CAT_2(_ffs,__PP_MUL8(__SIZEOF_LONG_LONG__)));
#endif /* !__CONFIG_MIN_LIBC__ */


#if !defined(memcpy) || \
     defined(__arch_generic_memcpy) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef memcpy
__public void *
memcpy(void *__restrict dst,
       void const *__restrict src,
       size_t bytes) {
 __assert_heref("memcpy(dst,src,bytes)"
               ,!((uintptr_t)dst > (uintptr_t)src && (uintptr_t)dst < ((uintptr_t)src+bytes)),1
               ,"memcpy(%p,%p,%Iu) : dst has %Iu overlapping bytes with src (use 'memmove' instead)"
               ,dst,src,bytes,((uintptr_t)src+bytes)-(uintptr_t)dst);
 __assert_heref("memcpy(dst,src,bytes)"
               ,!((uintptr_t)src > (uintptr_t)dst && (uintptr_t)src < ((uintptr_t)dst+bytes)),1
               ,"memcpy(%p,%p,%Iu) : src has %Iu overlapping bytes with src (use 'memmove' instead)"
               ,dst,src,bytes,((uintptr_t)dst+bytes)-(uintptr_t)src);
 __STRING_ASSERTMEM(dst,bytes);
 __STRING_ASSERTMEM(src,bytes);
 {
#if defined(arch_memcpy) && \
   !defined(__arch_generic_memcpy)
  return arch_memcpy(dst,src,bytes);
#elif 0
  byte_t *iter,*end; byte_t const *siter;
  siter = (byte_t const *)src;
  end = (iter = (byte_t *)dst)+bytes;
  while (iter != end) *iter++ = *siter++;
#else
  int *iter,*end; int const *siter;
  siter = (int const *)src;
  end = (iter = (int *)dst)+(bytes/__SIZEOF_INT__);
  while (iter != end) *iter++ = *siter++;
#if __SIZEOF_INT__ > 1
  switch (bytes % __SIZEOF_INT__) {
#if __SIZEOF_INT__ > 4
   case 7: *(*(byte_t **)&iter)++ = *(*(byte_t **)&siter)++;
   case 6: *(*(byte_t **)&iter)++ = *(*(byte_t **)&siter)++;
   case 5: *(*(byte_t **)&iter)++ = *(*(byte_t **)&siter)++;
   case 4: *(*(byte_t **)&iter)++ = *(*(byte_t **)&siter)++;
#endif
#if __SIZEOF_INT__ > 2
   case 3: *(*(byte_t **)&iter)++ = *(*(byte_t **)&siter)++;
   case 2: *(*(byte_t **)&iter)++ = *(*(byte_t **)&siter)++;
#endif
   case 1: *(*(byte_t **)&iter)++ = *(*(byte_t **)&siter)++;
   default: break;
  }
#endif /* __SIZEOF_INT__ > 1 */
#endif
 }
 return (void *)dst;
}
#endif


#if !defined(memccpy) || \
     defined(__arch_generic_memccpy) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef memccpy
__public void *
memccpy(void *__restrict dst,
        void const *__restrict src,
        int c, size_t bytes) {
#if defined(arch_memccpy) && \
   !defined(__arch_generic_memccpy) && \
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return arch_memccpy(dst,src,c,bytes);
#else
 byte_t *dst_iter,*end;
 byte_t const *src_iter;
 end = (dst_iter = (byte_t *)dst)+bytes;
 src_iter = (byte_t const *)src;
 while (dst_iter != end) {
  __STRING_ASSERTBYTE(dst_iter);
  __STRING_ASSERTBYTE(src_iter);
  if ((*dst_iter++ = *src_iter++) == c) return dst_iter;
 }
 return NULL;
#endif
}
#endif


#if !defined(memmove) || \
     defined(__arch_generic_memmove) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef memmove
__public void *
memmove(void *dst,
        void const *src,
        size_t bytes) {
 __STRING_ASSERTMEM(dst,bytes);
 __STRING_ASSERTMEM(src,bytes);
 {
#if defined(arch_memmove) && \
   !defined(__arch_generic_memmove)
  return arch_memmove(dst,src,bytes);
#else
  byte_t *iter,*end; byte_t const *siter;
  if (dst < src) {
   siter = (byte_t const *)src;
   end = (iter = (byte_t *)dst)+bytes;
   while (iter != end) *iter++ = *siter++;
  } else {
   siter = (byte_t const *)src+bytes;
   iter = (end = (byte_t *)dst)+bytes;
   while (iter != end) *--iter = *--siter;
  }
  return dst;
#endif
 }
}
#endif


#if !defined(memset) || \
     defined(__arch_generic_memset) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef memset
__public void *
memset(void *__restrict dst,
       int byte, size_t bytes) {
#if defined(arch_memset) && \
   !defined(__arch_generic_memset)
 __STRING_ASSERTMEM(dst,bytes);
 return arch_memset(dst,byte,bytes);
#else
 byte_t *iter,*end;
 __STRING_ASSERTMEM(dst,bytes);
 end = (iter = (byte_t *)dst)+bytes;
 while (iter != end) *iter++ = (byte_t)byte;
 return dst;
#endif
}
#endif

#if !defined(memcmp) || \
     defined(__arch_generic_memcmp) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef memcmp
__public int
memcmp(void const *a,
       void const *b,
       size_t bytes) {
#if defined(arch_memcmp) && \
   !defined(__arch_generic_memcmp)
 __STRING_ASSERTMEM(a,bytes);
 __STRING_ASSERTMEM(b,bytes);
 return arch_memcmp(a,b,bytes);
#else
 byte_t const *aiter,*biter,*end; byte_t av = 0,bv = 0;
 __STRING_ASSERTMEM(a,bytes);
 __STRING_ASSERTMEM(b,bytes);
 end = (aiter = (byte_t const *)a)+bytes,biter = (byte_t const *)b;
 while (aiter != end && ((av = *aiter++) == (bv = *biter++)));
 return (int)(av-bv);
#endif
}
#endif

#if !defined(memchr) || \
     defined(__arch_generic_memchr) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef memchr
__public void *memchr(void const *p, int needle, size_t bytes) {
#if defined(arch_memchr) && \
   !defined(__arch_generic_memchr)
 __STRING_ASSERTMEM(p,bytes);
 return arch_memchr(p,needle,bytes);
#else
 byte_t *iter,*end;
 __STRING_ASSERTMEM(p,bytes);
 end = (iter = (byte_t *)p)+bytes;
 while (iter != end) {
  if (*iter == needle) return iter;
  ++iter;
 }
 return NULL;
#endif
}
#endif

#if !defined(memrchr) || \
     defined(__arch_generic_memrchr) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef memrchr
__public void *memrchr(void const *p, int needle, size_t bytes) {
#if defined(arch_memrchr) && \
   !defined(__arch_generic_memrchr)
 __STRING_ASSERTMEM(p,bytes);
 return arch_memrchr(p,needle,bytes);
#else
 byte_t *iter;
 __STRING_ASSERTMEM(p,bytes);
 iter = (byte_t *)p+bytes;
 while (iter-- != (byte_t *)p) {
  if (*iter == needle) return iter;
 }
 return NULL;
#endif
}
#endif

#if !defined(memmem) || \
     defined(__arch_generic_memmem) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef memmem
__public void *
memmem(void const *haystack, size_t haystacklen,
       void const *needle, size_t needlelen) {
#if defined(arch_memmem) && \
   !defined(__arch_generic_memmem)
 __STRING_ASSERTMEM(haystack,haystacklen);
 __STRING_ASSERTMEM(needle,needlelen);
 return arch_memmem(haystack,haystacklen,
                     needle,needlelen);
#else
 byte_t *iter,*end;
 __STRING_ASSERTMEM(haystack,haystacklen);
 if __unlikely(needlelen > haystacklen) return NULL;
 end = (iter = (byte_t *)haystack)+(haystacklen-needlelen);
 for (;;) {
  if (!string_memcmp(iter,needle,needlelen)) return iter;
  if (iter++ == end) break;
 }
 return NULL;
#endif
}
#endif


#if !defined(_memrmem) || \
     defined(__arch_generic_memrmem) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef _memrmem
__public void *
_memrmem(void const *haystack, size_t haystacklen,
         void const *needle, size_t needlelen) {
#if defined(arch_memrmem) && \
   !defined(__arch_generic_memrmem)
 __STRING_ASSERTMEM(haystack,haystacklen);
 __STRING_ASSERTMEM(needle,needlelen);
 return arch_memrmem(haystack,haystacklen,
                      needle,needlelen);
#else
 byte_t *iter,*end,*result = NULL;
 __STRING_ASSERTMEM(haystack,haystacklen);
 if __unlikely(needlelen > haystacklen) return NULL;
 end = (iter = (byte_t *)haystack)+(haystacklen-needlelen);
 for (;;) {
  if (!string_memcmp(iter,needle,needlelen)) result = iter;
  if (iter++ == end) break;
 }
 return (void *)result;
#endif
}
#endif


#if !defined(_memidx) || \
     defined(__arch_generic_memmem) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef _memidx
__public void *
_memidx(void const *vector, size_t elemcount,
        void const *pattern, size_t elemsize,
        size_t elemalign) {
 assertf(elemalign >= elemsize,
         "Invalid element properties: %Iu < %Iu",
         elemalign,elemsize);
 assertf(elemalign,"Invalid element alignment: ZERO(0)");
 __STRING_ASSERTMEM(vector,elemcount*elemalign);
 __STRING_ASSERTMEM(pattern,elemsize);
 {
#if defined(arch_memidx) && \
   !defined(__arch_generic_memidx)
  return arch_memidx(vector,elemcount,patter,elemsize,elemalign);
#else
  byte_t *iter,*end;
  end = (iter = (byte_t *)vector)+(elemcount*elemalign);
  for (; iter != end; iter += elemalign) {
   if (!string_memcmp(iter,pattern,elemsize)) return iter;
  }
  return NULL;
#endif
 }
}
#endif


#if !defined(_memridx) || \
     defined(__arch_generic_memmem) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef _memridx
__public void *
_memridx(void const *vector, size_t elemcount,
        void const *pattern, size_t elemsize,
        size_t elemalign) {
 assertf(elemalign >= elemsize,
         "Invalid element properties: %Iu < %Iu",
         elemalign,elemsize);
 assertf(elemalign,"Invalid element alignment: ZERO(0)");
 __STRING_ASSERTMEM(vector,elemcount*elemalign);
 __STRING_ASSERTMEM(pattern,elemsize);
 {
#if defined(arch_memridx) && \
   !defined(__arch_generic_memridx)
  return arch_memridx(vector,elemcount,patter,elemsize,elemalign);
#else
  byte_t *iter;
  iter = (byte_t *)vector+(elemcount*elemalign);
  while (iter != (byte_t *)vector) {
   iter -= elemalign;
   if (!string_memcmp(iter,pattern,elemsize)) return iter;
  }
  return NULL;
#endif
 }
}
#endif


#if !defined(strcat) || \
     defined(__arch_generic_strcat) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef strcat
__public char *strcat(char *dst, char const *src) {
 return strcpy(_strend(dst),src);
}
#endif

#if !defined(strncat) || \
     defined(__arch_generic_strncat) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef strncat
__public char *strncat(char *dst, char const *src,
                       size_t maxchars) {
 return strncpy(_strend(dst),src,maxchars);
}
#endif

#if !defined(strcpy) || \
     defined(__arch_generic_strcpy) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef strcpy
__public char *strcpy(char *dst, char const *src) {
#if defined(arch_strcpy) && \
   !defined(__arch_generic_strcpy) && \
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return arch_strcpy(dst,src);
#else
 char *iter = dst,ch; char const *siter = src;
 while ((__STRING_ASSERTBYTE(siter),ch = *siter++) != '\0')
  __STRING_ASSERTBYTE(iter),*iter++ = ch;
 __STRING_ASSERTBYTE(iter),*iter = '\0';
 return iter;
#endif
}
#endif

#if !defined(strncpy) || \
     defined(__arch_generic_strncpy) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef strncpy
__public char *strncpy(char *dst, char const *src, size_t maxchars) {
#if defined(arch_strncpy) && \
   !defined(__arch_generic_strncpy) && \
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return arch_strncpy(dst,src,maxchars);
#else
 char *iter = dst,ch; char const *siter,*max;
 assert(!maxchars || (dst && src));
 max = (siter = src)+maxchars;
 while (siter != max && (__STRING_ASSERTBYTE(siter),ch = *siter++) != '\0')
  __STRING_ASSERTBYTE(iter),*iter++ = ch;
 __STRING_ASSERTBYTE(iter),*iter = '\0';
 return iter;
#endif
}
#endif

#if !defined(_umemend) || \
     defined(__arch_generic_umemend) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef _umemend
__public void *
_umemend(void const *__restrict haystack, int needle) {
#if defined(arch_umemend) && \
   !defined(__arch_generic_umemend) && \
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return arch_umemend(haystack,needle);
#elif defined(arch_umemlen) && \
     !defined(__arch_generic_umemlen) && \
     !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return (void *)((uintptr_t)haystack+arch_umemlen(haystack,needle));
#else
 byte_t *result = (byte_t *)haystack;
 while ((__STRING_ASSERTBYTE(result),
        *result != needle)) ++result;
 return (void *)result;
#endif
}
#endif

#if !defined(_umemlen) || \
     defined(__arch_generic_umemlen) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef _umemlen
__public size_t
_umemlen(void const *__restrict haystack,
        int needle) {
#if defined(arch_umemlen) && \
   !defined(__arch_generic_umemlen) && \
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return arch_umemlen(haystack,needle);
#elif defined(arch_umemend) && \
     !defined(__arch_generic_umemend) && \
     !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return (size_t)((byte_t *)arch_umemend(haystack,needle)-
                 (byte_t *)haystack);
#else
 byte_t *result = (byte_t *)haystack;
 while ((__STRING_ASSERTBYTE(result),
        *result != needle)) ++result;
 return (size_t)(result-(byte_t *)haystack);
#endif
}
#endif

#if !defined(_memend) || \
     defined(__arch_generic_memend) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef _memend
__public void *
_memend(void const *__restrict haystack,
        int needle, size_t bytes) {
#if defined(arch_memend) && \
   !defined(__arch_generic_memend) && \
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return arch_memend(haystack,needle,bytes);
#elif defined(arch_memlen) && \
     !defined(__arch_generic_memlen) && \
     !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return (void *)((uintptr_t)haystack+arch_memlen(haystack,needle,bytes));
#else
 byte_t *result,*max;
 assert(!bytes || haystack);
 max = (result = (byte_t *)haystack)+bytes;
 while (result != max && (__STRING_ASSERTBYTE(result),
       *result != needle)) ++result;
 return (void *)result;
#endif
}
#endif

#if !defined(_memlen) || \
     defined(__arch_generic_memlen) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef _memlen
__public size_t
_memlen(void const *__restrict haystack,
        int needle, size_t bytes) {
#if defined(arch_memlen) && \
   !defined(__arch_generic_memlen) && \
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return arch_memlen(haystack,needle,bytes);
#elif defined(arch_memend) && \
     !defined(__arch_generic_memend) && \
     !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return (size_t)((byte_t *)arch_memend(haystack,needle,bytes)-
                 (byte_t *)haystack);
#else
 byte_t *end,*max;
 assert(!bytes || haystack);
 max = (end = (byte_t *)haystack)+bytes;
 while (end != max && (__STRING_ASSERTBYTE(end),
       *end != needle)) ++end;
 return (size_t)(end-haystack);
#endif
}
#endif

#if !defined(_strend) || \
     defined(__arch_generic_strend) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef _strend
__public char *_strend(char const *__restrict s) {
#if defined(arch_umemend) && \
   !defined(__arch_generic_umemend) && \
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return (char *)arch_umemend(s,'\0');
#elif defined(arch_umemlen) && \
     !defined(__arch_generic_umemlen) && \
     !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return (char *)s+arch_umemlen(s,'\0');
#else
 char *result = (char *)s;
 while ((__STRING_ASSERTBYTE(result),*result)) ++result;
 return result;
#endif
}
#endif

#if !defined(_strnend) || \
     defined(__arch_generic_strnend) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef _strnend
__public char *_strnend(char const *__restrict s, size_t maxchars) {
#if defined(arch_memend) && \
   !defined(__arch_generic_memend) && \
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return (char *)arch_memend(s,'\0',maxchars);
#elif defined(arch_memlen) && \
      defined(__arch_generic_memlen) && \
     !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return (char *)s+arch_memlen(s,'\0',maxchars);
#else
 char *result,*max;
 assert(!maxchars || s);
 max = (result = (char *)s)+maxchars;
 while (result != max &&
       (__STRING_ASSERTBYTE(result),*result)) ++result;
 return result;
#endif
}
#endif

#if !defined(strlen) || \
     defined(__arch_generic_strlen) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef strlen
__public size_t strlen(char const *__restrict s) {
#if defined(arch_umemlen) && \
   !defined(__arch_generic_umemlen) && \
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return arch_umemlen(s,'\0');
#elif defined(arch_umemend) && \
     !defined(__arch_generic_umemend) && \
     !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return (size_t)((char const *)arch_umemend(s)-s);
#else
 char const *end = s;
 while ((__STRING_ASSERTBYTE(end),*end)) ++end;
 return (size_t)(end-s);
#endif
}
#endif

#if !defined(strnlen) || \
     defined(__arch_generic_strnlen) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef strnlen
__public size_t strnlen(char const *__restrict s, size_t maxchars) {
#if defined(arch_memlen) && \
   !defined(__arch_generic_memlen) && \
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return arch_memlen(s,'\0',maxchars);
#elif defined(arch_memend) && \
     !defined(__arch_generic_memend) && \
     !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return (size_t)((char const *)arch_memend(s,maxchars)-s);
#else
 char const *end,*max;
 assert(!maxchars || s);
 max = (end = s)+maxchars;
 while (end != max && (__STRING_ASSERTBYTE(end),*end)) ++end;
 return (size_t)(end-s);
#endif
}
#endif

#if !defined(strspn) || \
     defined(__arch_generic_strspn) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef strspn
__public size_t strspn(char const *str, char const *spanset) {
#if defined(arch_strspn) && \
   !defined(__arch_generic_strspn) && \
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return arch_strspn(str,spanset);
#else
 char const *iter = str;
#ifdef STRING_AVOID_STRCHR
 size_t spanset_size = string_strlen(spanset)*sizeof(char);
 while ((__STRING_ASSERTBYTE(iter),
         string_memchr(spanset,*iter,spanset_size))) ++iter;
#else
 while ((__STRING_ASSERTBYTE(iter),strchr(spanset,*iter))) ++iter;
#endif
 return (size_t)(iter-str);
#endif
}
#endif

#if !defined(_strnspn) || \
     defined(__arch_generic_strnspn) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef _strnspn
__public size_t _strnspn(char const *str, size_t maxstr,
                         char const *spanset, size_t maxspanset) {
#if defined(arch_strnspn) && \
   !defined(__arch_generic_strnspn) && \
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return arch_strnspn(str,maxstr,spanset,maxspanset);
#else
 char const *iter,*end;
#ifdef STRING_AVOID_STRNCHR
 maxspanset = string_strnlen(spanset,maxspanset)*sizeof(char);
#endif
 end = (iter = str)+maxstr;
#ifdef STRING_AVOID_STRNCHR
 while (iter != end && (__STRING_ASSERTBYTE(iter),
        string_memchr(spanset,*iter,maxspanset))) ++iter;
#else
 while (iter != end && (__STRING_ASSERTBYTE(iter),
        strnchr(spanset,maxspanset,*iter))) ++iter;
#endif
 return (size_t)(iter-str);
#endif
}
#endif

#if !defined(strcspn) || \
     defined(__arch_generic_strcspn) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef strcspn
__public size_t strcspn(char const *str, char const *spanset) {
#if defined(arch_strcspn) && \
   !defined(__arch_generic_strcspn) && \
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return arch_strcspn(str,spanset);
#else
 char const *iter = str;
#ifdef STRING_AVOID_STRCHR
 size_t spanset_size = string_strlen(spanset)*sizeof(char);
 while ((__STRING_ASSERTBYTE(iter),*iter &&
        !string_memchr(spanset,*iter,spanset_size))) ++iter;
#else
 while ((__STRING_ASSERTBYTE(iter),*iter &&
        !strchr(spanset,*iter))) ++iter;
#endif
 return (size_t)(iter-str);
#endif
}
#endif

#if !defined(_strncspn) || \
     defined(__arch_generic_strncspn) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef _strncspn
__public size_t _strncspn(char const *str, size_t maxstr,
                          char const *spanset, size_t maxspanset) {
#if defined(arch_strncspn) && \
   !defined(__arch_generic_strncspn) && \
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return arch_strncspn(str,maxstr,spanset,maxspanset);
#else
 char const *iter,*end; end = (iter = str)+maxstr;
#ifdef STRING_AVOID_STRNCHR
 maxspanset = string_strnlen(spanset,maxspanset)*sizeof(char);
 while (iter != end && (__STRING_ASSERTBYTE(iter),*iter &&
                       !string_memchr(spanset,*iter,maxspanset))
        ) ++iter;
#else
 while (iter != end && (__STRING_ASSERTBYTE(iter),*iter &&
                       !strnchr(spanset,maxspanset,*iter))
        ) ++iter;
#endif
 return (size_t)(iter-str);
#endif
}
#endif


#if !defined(strcmp) || \
     defined(__arch_generic_strcmp) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef strcmp
__public int strcmp(char const *a, char const *b) {
#if defined(arch_strcmp) && \
   !defined(__arch_generic_strcmp) && \
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return arch_strcmp(a,b);
#else
 char const *aiter,*biter; int cha,chb;
 assert(a && b); aiter = a,biter = b;
 do __STRING_ASSERTBYTE(aiter),cha = *aiter++,
    __STRING_ASSERTBYTE(biter),chb = *biter++;
 while (cha && chb && cha == chb);
 return (int)((signed char)cha-(signed char)chb);
#endif
}
#endif

#if !defined(strncmp) || \
     defined(__arch_generic_strncmp) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef strncmp
__public int strncmp(char const *a, char const *b, size_t maxchars) {
#if defined(arch_strncmp) && \
   !defined(__arch_generic_strncmp) && \
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return arch_strncmp(a,b,maxchars);
#else
 char const *aiter,*biter,*max; int cha = 0,chb = 0;
 assert(!maxchars || (a && b));
 max = (aiter = a)+maxchars,biter = b;
 while (aiter != max &&
       (__STRING_ASSERTBYTE(aiter),cha = *aiter++) != '\0' &&
       (__STRING_ASSERTBYTE(biter),chb = *biter++) != '\0' &&
       (cha == chb));
 return (int)((signed char)cha-(signed char)chb);
#endif
}
#endif

#if !defined(strchr) || \
     defined(__arch_generic_strchr) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef strchr
__public char *strchr(char const *__restrict haystack, int needle) {
#if defined(arch_strchr) && \
   !defined(__arch_generic_strchr) && \
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return arch_strchr(haystack,needle);
#else
 char ch,*iter;
 for (iter = (char *)haystack;;) {
  __STRING_ASSERTBYTE(iter);
  if ((ch = *iter) == '\0') break;
  if (ch == needle) return iter;
  ++iter;
 }
 return NULL;
#endif
}
#endif

#if !defined(strrchr) || \
     defined(__arch_generic_strrchr) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef strrchr
__public char *strrchr(char const *__restrict haystack, int needle) {
#if defined(arch_strrchr) && \
   !defined(__arch_generic_strrchr) && \
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return arch_strrchr(haystack,needle);
#else
 char ch,*iter,*result = NULL;
 assert(haystack);
 for (iter = (char *)haystack;;) {
  __STRING_ASSERTBYTE(iter);
  if ((ch = *iter) == '\0') break;
  if (ch == needle) result = iter;
  ++iter;
 }
 return result;
#endif
}
#endif

#if !defined(_strnchr) || \
     defined(__arch_generic_strnchr) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef _strnchr
__public char *_strnchr(char const *__restrict haystack,
                        size_t max_haychars, int needle) {
#if defined(arch_strnchr) && \
   !defined(__arch_generic_strnchr) && \
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return arch_strnchr(haystack,max_haychars,needle);
#else
 char ch,*iter,*max_hay;
 assert(!max_haychars || haystack);
 max_hay = (iter = (char *)haystack)+max_haychars;
 for (;iter != max_hay;) {
  __STRING_ASSERTBYTE(iter);
  if ((ch = *iter) == '\0') break;
  if (ch == needle) return iter;
  ++iter;
 }
 return NULL;
#endif
}
#endif

#if !defined(_strnrchr) || \
     defined(__arch_generic_strnrchr) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef _strnrchr
__public char *_strnrchr(char const *__restrict haystack,
                         size_t max_haychars, int needle) {
#if defined(arch_strnrchr) && \
   !defined(__arch_generic_strnrchr) && \
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return arch_strnrchr(haystack,max_haychars,needle);
#else
 char ch,*iter,*max_hay,*result = NULL;
 assert(haystack);
 max_hay = (iter = (char *)haystack)+max_haychars;
 for (;iter != max_hay;) {
  __STRING_ASSERTBYTE(iter);
  if ((ch = *iter) == '\0') break;
  if (ch == needle) result = iter;
  ++iter;
 }
 return result;
#endif
}
#endif

#if !defined(strstr) || \
     defined(__arch_generic_strstr) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef strstr
__public char *strstr(char const *haystack, char const *needle) {
#if defined(arch_strstr) && \
   !defined(__arch_generic_strstr) && \
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return arch_strstr(haystack,needle);
#else
 char *hay_iter,*hay2; char const *ned_iter; char ch,needle_start;
 assert(haystack);
 assert(needle);
 hay_iter = (char *)haystack;
 needle_start = *needle++;
 while ((__STRING_ASSERTBYTE(hay_iter),ch = *hay_iter++) != '\0') {
  if (ch == needle_start) {
   hay2 = hay_iter,ned_iter = needle;
   while ((__STRING_ASSERTBYTE(ned_iter),ch = *ned_iter++) != '\0') {
    if (*hay2++ != ch) goto miss;
   }
   return hay_iter-1;
  }
miss:;
 }
 return NULL;
#endif
}
#endif

#if !defined(_strrstr) || \
     defined(__arch_generic_strrstr) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef _strrstr
__public char *_strrstr(char const *haystack, char const *needle) {
#if defined(arch_strrstr) && \
   !defined(__arch_generic_strrstr) && \
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return arch_strrstr(haystack,needle);
#else
 char *hay_iter,*hay2,*result = NULL;
 char const *ned_iter; char ch,needle_start;
 assert(haystack);
 assert(needle);
 hay_iter = (char *)haystack;
 needle_start = *needle++;
 while ((__STRING_ASSERTBYTE(hay_iter),ch = *hay_iter++) != '\0') {
  if (ch == needle_start) {
   hay2 = hay_iter,ned_iter = needle;
   while ((__STRING_ASSERTBYTE(ned_iter),ch = *ned_iter++) != '\0') {
    if (*hay2++ != ch) goto miss;
   }
   result = hay_iter-1;
  }
miss:;
 }
 return result;
#endif
}
#endif

#if !defined(strpbrk) || \
     defined(__arch_generic_strpbrk) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef strpbrk
__public char *strpbrk(char const *haystack, char const *needle_list) {
#if defined(arch_strpbrk) && \
   !defined(__arch_generic_strpbrk) && \
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return arch_strpbrk(haystack,needle_list);
#else
 char *hay_iter = (char *)haystack;
 char const *ned_iter; char haych,ch;
 assert(haystack);
 assert(needle_list);
 while ((__STRING_ASSERTBYTE(hay_iter),haych = *hay_iter++) != '\0') {
  ned_iter = needle_list;
  while ((__STRING_ASSERTBYTE(ned_iter),ch = *ned_iter++) != '\0') {
   if (haych == ch) return hay_iter-1;
  }
 }
 return NULL;
#endif
}
#endif

#if !defined(_strrpbrk) || \
     defined(__arch_generic_strrpbrk) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef _strrpbrk
__public char *_strrpbrk(char const *haystack, char const *needle_list) {
#if defined(arch_strpbrk) && \
   !defined(__arch_generic_strpbrk) && \
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return arch_strpbrk(haystack,needle_list);
#else
 char *result = NULL,*hay_iter = (char *)haystack;
 char const *ned_iter; char haych,ch;
 assert(haystack);
 assert(needle_list);
 while ((__STRING_ASSERTBYTE(hay_iter),haych = *hay_iter++) != '\0') {
  ned_iter = needle_list;
  while ((__STRING_ASSERTBYTE(ned_iter),ch = *ned_iter++) != '\0') {
   if (haych == ch) { result = hay_iter-1; break; }
  }
 }
 return result;
#endif
}
#endif

#if !defined(_strnpbrk) || \
     defined(__arch_generic_strnpbrk) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef _strnpbrk
__public char *_strnpbrk(char const *haystack, size_t max_haychars,
                         char const *needle_list, size_t max_needlelist) {
#if defined(arch_strnpbrk) && \
   !defined(__arch_generic_strnpbrk) && \
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return arch_strnpbrk(haystack,max_haychars,needle_list,max_needlelist);
#else
 char *hay_iter = (char *)haystack;
 char *hay_end = hay_iter+max_haychars;
 char const *ned_iter,*ned_end; char haych,ch;
 assert(!max_haychars || haystack);
 assert(!max_needlelist || needle_list);
 if __unlikely(!max_needlelist) return NULL;
 ned_end = needle_list+max_needlelist;
 while (hay_iter != hay_end  &&
       (__STRING_ASSERTBYTE(hay_iter),haych = *hay_iter++) != '\0') {
  ned_iter = needle_list;
  while ((__STRING_ASSERTBYTE(ned_iter),ch = *ned_iter++) != '\0') {
   if (haych == ch) return hay_iter-1;
   if (ned_iter == ned_end) break;
  }
 }
 return NULL;
#endif
}
#endif

#if !defined(_strnrpbrk) || \
     defined(__arch_generic_strnrpbrk) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef _strnrpbrk
__public char *_strnrpbrk(char const *haystack, size_t max_haychars,
                          char const *needle_list, size_t max_needlelist) {
#if defined(arch_strnrpbrk) && \
   !defined(__arch_generic_strnrpbrk) && \
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return arch_strnrpbrk(haystack,max_haychars,needle_list,max_needlelist);
#else
 char *hay_iter = (char *)haystack;
 char *hay_end = hay_iter+max_haychars;
 char *result = NULL;
 char const *ned_iter,*ned_end; char haych,ch;
 assert(!max_haychars || haystack);
 assert(!max_needlelist || needle_list);
 if __unlikely(!max_needlelist) return NULL;
 ned_end = needle_list+max_needlelist;
 while (hay_iter != hay_end  &&
       (__STRING_ASSERTBYTE(hay_iter),haych = *hay_iter++) != '\0') {
  ned_iter = needle_list;
  while ((__STRING_ASSERTBYTE(ned_iter),ch = *ned_iter++) != '\0') {
   if (haych == ch) { result = hay_iter-1; break; }
   if (ned_iter == ned_end) break;
  }
 }
 return result;
#endif
}
#endif

#if !defined(_strnstr) || \
     defined(__arch_generic_strnstr) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef _strnstr
__public char *_strnstr(char const *haystack, size_t max_haychars,
                        char const *needle, size_t max_needlechars) {
#if defined(arch_strnstr) && \
   !defined(__arch_generic_strnstr) && \
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return arch_strnstr(haystack,max_haychars,needle,max_needlechars);
#else
 char *hay_iter,*hay2,*max_hay;
 char const *ned_iter,*max_needle; char ch,needle_start;
 assert(!max_haychars || haystack);
 assert(!max_needlechars || needle);
 if __unlikely(!max_needlechars) return NULL;
 max_hay = (hay_iter = (char *)haystack)+max_haychars;
 __STRING_ASSERTBYTE(needle),needle_start = *needle;
 max_needle = (needle++)+max_needlechars;
 while (hay_iter != max_hay && (__STRING_ASSERTBYTE(hay_iter),ch = *hay_iter++) != '\0') {
  if (ch == needle_start) {
   hay2 = hay_iter,ned_iter = needle;
   while (ned_iter != max_needle && (__STRING_ASSERTBYTE(ned_iter),ch = *ned_iter++) != '\0') {
    if (__STRING_ASSERTBYTE(hay2),*hay2++ != ch) goto miss;
   }
   return hay_iter-1;
  }
miss:;
 }
 return NULL;
#endif
}
#endif

#if !defined(_strnrstr) || \
     defined(__arch_generic_strnrstr) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef _strnrstr
__public char *_strnrstr(char const *haystack, size_t max_haychars,
                         char const *needle, size_t max_needlechars) {
#if defined(arch_strnrstr) && \
   !defined(__arch_generic_strnrstr) && \
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return arch_strnrstr(haystack,max_haychars,needle,max_needlechars);
#else
 char *hay_iter,*hay2,*max_hay,*result = NULL;
 char const *ned_iter,*max_needle; char ch,needle_start;
 assert(!max_haychars || haystack);
 assert(!max_needlechars || needle);
 if __unlikely(!max_needlechars) return NULL;
 max_hay = (hay_iter = (char *)haystack)+max_haychars;
 __STRING_ASSERTBYTE(needle),needle_start = *needle;
 max_needle = (needle++)+max_needlechars;
 while (hay_iter != max_hay && (__STRING_ASSERTBYTE(hay_iter),ch = *hay_iter++) != '\0') {
  if (ch == needle_start) {
   hay2 = hay_iter,ned_iter = needle;
   while (ned_iter != max_needle && (__STRING_ASSERTBYTE(ned_iter),ch = *ned_iter++) != '\0') {
    if (__STRING_ASSERTBYTE(hay2),*hay2++ != ch) goto miss;
   }
   result = hay_iter-1;
  }
miss:;
 }
 return result;
#endif
}
#endif

#ifndef __CONFIG_MIN_LIBC__
/* Character casting used to unify characters in stri-style functions.
 * -> This should either be tolower or toupper. */
#define STRICAST(ch) tolower(ch)

#if !defined(_stricmp) || \
     defined(__arch_generic_stricmp) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef _stricmp
__public int _stricmp(char const *a, char const *b) {
#if defined(arch_stricmp) && \
   !defined(__arch_generic_stricmp) && \
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return arch_stricmp(a,b);
#else
 char const *aiter = a,*biter = b; int cha,chb; assert(a && b);
 do __STRING_ASSERTBYTE(aiter),cha = STRICAST(*aiter++),
    __STRING_ASSERTBYTE(biter),chb = STRICAST(*biter++);
 while (cha && chb && cha == chb);
 return (int)((signed char)cha-(signed char)chb);
#endif
}
#endif

#if !defined(_memicmp) || \
     defined(__arch_generic_memicmp) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef _memicmp
__public int _memicmp(void const *a,
                      void const *b,
                      size_t bytes) {
#if defined(arch_memicmp) && \
   !defined(__arch_generic_memicmp)
 __STRING_ASSERTMEM(a,bytes);
 __STRING_ASSERTMEM(b,bytes);
 return arch_memicmp(a,b,bytes);
#else
 char const *aiter,*biter,*end; char av = 0,bv = 0;
 __STRING_ASSERTMEM(a,bytes);
 __STRING_ASSERTMEM(b,bytes);
 end = (aiter = (char const *)a)+bytes,biter = (char const *)b;
 while (aiter != end && ((av = STRICAST(*aiter++)) == (bv = STRICAST(*biter++))));
 return (int)(av-bv);
#endif
}
#endif

#if !defined(_strincmp) || \
     defined(__arch_generic_strincmp) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef _strincmp
__public int _strincmp(char const *a, char const *b, size_t maxchars) {
#if defined(arch_strincmp) && \
   !defined(__arch_generic_) && \
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return arch_strincmp(a,b,maxchars);
#else
 char const *aiter,*biter,*max; int cha = 0,chb = 0;
 assert(!maxchars || (a && b));
 max = (aiter = a)+maxchars,biter = b;
 while (aiter != max &&
       (__STRING_ASSERTBYTE(aiter),cha = STRICAST(*aiter++)) != '\0' &&
       (__STRING_ASSERTBYTE(biter),chb = STRICAST(*biter++)) != '\0' &&
       (cha == chb));
 return (int)((signed char)cha-(signed char)chb);
#endif
}
#endif

#if !defined(_memichr) || \
     defined(__arch_generic_memichr) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef _memichr
__public void *_memichr(void const *__restrict p, int needle, size_t bytes) {
#if defined(arch_memichr) && \
   !defined(__arch_generic_memichr) && \
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return arch_memichr(p,needle,bytes);
#else
 char *iter,*end; needle = STRICAST(needle);
 end = (iter = (char *)p)+bytes;
 while (iter != end) {
  __STRING_ASSERTBYTE(iter);
  if (STRICAST(*iter) == needle) return (void *)iter;
 }
 return NULL;
#endif
}
#endif

#if !defined(_memirchr) || \
     defined(__arch_generic_memirchr) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef _memirchr
__public void *_memirchr(void const *__restrict p, int needle, size_t bytes) {
#if defined(arch_memirchr) && \
   !defined(__arch_generic_memirchr) && \
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return arch_memirchr(p,needle,bytes);
#else
 char *iter,*end,*result = NULL;
 needle = STRICAST(needle);
 end = (iter = (char *)p)+bytes;
 while (iter != end) {
  __STRING_ASSERTBYTE(iter);
  if (STRICAST(*iter) == needle) {
   result = iter;
  }
 }
 return (void *)result;
#endif
}
#endif

#if !defined(_strichr) || \
     defined(__arch_generic_strichr) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef _strichr
__public char *_strichr(char const *__restrict haystack, int needle) {
#if defined(arch_strichr) && \
   !defined(__arch_generic_strichr) && \
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return arch_strichr(haystack,needle);
#else
 char ch; needle = STRICAST(needle);
 for (;;) {
  __STRING_ASSERTBYTE(haystack);
  if ((ch = *haystack) == '\0') break;
  if (STRICAST(ch) == needle) return (char *)haystack;
 }
 return NULL;
#endif
}
#endif

#if !defined(_strirchr) || \
     defined(__arch_generic_strirchr) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef _strirchr
__public char *_strirchr(char const *__restrict haystack, int needle) {
#if defined(arch_strichr) && \
   !defined(__arch_generic_strichr) && \
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return arch_strichr(haystack,needle);
#else
 char *result = NULL;
 char ch; needle = STRICAST(needle);
 for (;;) {
  __STRING_ASSERTBYTE(haystack);
  if ((ch = *haystack) == '\0') break;
  if (STRICAST(ch) == needle) result = (char *)haystack;
 }
 return result;
#endif
}
#endif

#if !defined(_stristr) || \
     defined(__arch_generic_stristr) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef _stristr
__public char *_stristr(char const *haystack, char const *needle) {
#if defined(arch_stristr) && \
   !defined(__arch_generic_stristr) && \
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return arch_stristr(haystack,needle);
#else
 char *hay_iter,*hay2; char const *ned_iter; char ch,needle_start;
 assert(haystack);
 assert(needle);
 hay_iter = (char *)haystack;
 needle_start = STRICAST(*needle++);
 while ((__STRING_ASSERTBYTE(hay_iter),ch = *hay_iter++) != '\0') {
  if (STRICAST(ch) == needle_start) {
   hay2 = hay_iter,ned_iter = needle;
   while ((__STRING_ASSERTBYTE(ned_iter),ch = *ned_iter++) != '\0') {
    if (STRICAST(*hay2++) != STRICAST(ch)) goto miss;
   }
   return hay_iter-1;
  }
miss:;
 }
 return NULL;
#endif
}
#endif

#if !defined(_strirstr) || \
     defined(__arch_generic_strirstr) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef _strirstr
__public char *_strirstr(char const *haystack, char const *needle) {
#if defined(arch_strirstr) && \
   !defined(__arch_generic_strirstr) && \
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return arch_strirstr(haystack,needle);
#else
 char *hay_iter,*hay2,*result = NULL;
 char const *ned_iter; char ch,needle_start;
 assert(haystack);
 assert(needle);
 hay_iter = (char *)haystack;
 needle_start = STRICAST(*needle++);
 while ((__STRING_ASSERTBYTE(hay_iter),ch = *hay_iter++) != '\0') {
  if (STRICAST(ch) == needle_start) {
   hay2 = hay_iter,ned_iter = needle;
   while ((__STRING_ASSERTBYTE(ned_iter),ch = *ned_iter++) != '\0') {
    if (STRICAST(*hay2++) != STRICAST(ch)) goto miss;
   }
   result = hay_iter-1;
  }
miss:;
 }
 return result;
#endif
}
#endif

#if !defined(_strinchr) || \
     defined(__arch_generic_strinchr) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef _strinchr
__public char *_strinchr(char const *__restrict haystack, size_t max_haychars, int needle) {
#if defined(arch_strinchr) && \
   !defined(__arch_generic_strinchr) && \
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return arch_strinchr(haystack,max_haychars,needle);
#else
 char ch,*iter,*max_hay; needle = STRICAST(needle);
 assert(!max_haychars || haystack);
 max_hay = (iter = (char *)haystack)+max_haychars;
 for (;iter != max_hay;) {
  __STRING_ASSERTBYTE(iter);
  if ((ch = *iter) == '\0') return iter;
  if (STRICAST(ch) == needle) return iter;
  ++iter;
 }
 return NULL;
#endif
}
#endif

#if !defined(_strinrchr) || \
     defined(__arch_generic_strinrchr) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef _strinrchr
__public char *_strinrchr(char const *__restrict haystack, size_t max_haychars, int needle) {
#if defined(arch_strinrchr) && \
   !defined(__arch_generic_) && \
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return arch_strinrchr(haystack,max_haychars,needle);
#else
 char ch,*iter,*max_hay,*result = NULL;
 assert(haystack); needle = STRICAST(needle);
 max_hay = (iter = (char *)haystack)+max_haychars;
 for (;iter != max_hay;) {
  __STRING_ASSERTBYTE(iter);
  if ((ch = *iter) == '\0') break;
  if (STRICAST(ch) == needle) result = iter;
  ++iter;
 }
 return result;
#endif
}
#endif

#if !defined(_strinstr) || \
     defined(__arch_generic_strinstr) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef _strinstr
__public char *_strinstr(char const *haystack, size_t max_haychars,
                         char const *needle, size_t max_needlechars) {
#if defined(arch_strinstr) && \
   !defined(__arch_generic_strinstr) && \
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return arch_strinstr(haystack,max_haychars,needle,max_needlechars);
#else
 char *hay_iter,*hay2,*max_hay;
 char const *ned_iter,*max_needle; char ch,needle_start;
 assert(!max_haychars || haystack);
 assert(!max_needlechars || needle);
 if __unlikely(!max_needlechars) return NULL;
 max_hay = (hay_iter = (char *)haystack)+max_haychars;
 __STRING_ASSERTBYTE(needle),needle_start = STRICAST(*needle);
 max_needle = (needle++)+max_needlechars;
 while (hay_iter != max_hay && (__STRING_ASSERTBYTE(hay_iter),ch = *hay_iter++) != '\0') {
  if (STRICAST(ch) == needle_start) {
   hay2 = hay_iter,ned_iter = needle;
   while (ned_iter != max_needle && (__STRING_ASSERTBYTE(ned_iter),ch = *ned_iter++) != '\0') {
    if (__STRING_ASSERTBYTE(hay2),STRICAST(*hay2++) != STRICAST(ch)) goto miss;
   }
   return hay_iter-1;
  }
miss:;
 }
 return NULL;
#endif
}
#endif

#if !defined(_strinrstr) || \
     defined(__arch_generic_strinrstr) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef _strinrstr
__public char *_strinrstr(char const *haystack, size_t max_haychars,
                          char const *needle, size_t max_needlechars) {
#if defined(arch_strinrstr) && \
   !defined(__arch_generic_strinrstr) && \
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return arch_strinrstr(haystack,max_haychars,needle,max_needlechars);
#else
 char *hay_iter,*hay2,*max_hay,*result = NULL;
 char const *ned_iter,*max_needle; char ch,needle_start;
 assert(!max_haychars || haystack);
 assert(!max_needlechars || needle);
 if __unlikely(!max_needlechars) return NULL;
 max_hay = (hay_iter = (char *)haystack)+max_haychars;
 __STRING_ASSERTBYTE(needle),needle_start = STRICAST(*needle);
 max_needle = (needle++)+max_needlechars;
 while (hay_iter != max_hay && (__STRING_ASSERTBYTE(hay_iter),ch = *hay_iter++) != '\0') {
  if (STRICAST(ch) == needle_start) {
   hay2 = hay_iter,ned_iter = needle;
   while (ned_iter != max_needle && (__STRING_ASSERTBYTE(ned_iter),ch = *ned_iter++) != '\0') {
    if (__STRING_ASSERTBYTE(hay2),STRICAST(*hay2++) != STRICAST(ch)) goto miss;
   }
   result = hay_iter-1;
  }
miss:;
 }
 return result;
#endif
}
#endif

#if !defined(_stripbrk) || \
     defined(__arch_generic_stripbrk) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef _stripbrk
__public char *_stripbrk(char const *haystack, char const *needle_list) {
#if defined(arch_stripbrk) && \
   !defined(__arch_generic_stripbrk) && \
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return arch_stripbrk(haystack,needle_list);
#else
 char *hay_iter = (char *)haystack;
 char const *ned_iter; char haych,ch;
 assert(haystack);
 assert(needle_list);
 while ((__STRING_ASSERTBYTE(hay_iter),haych = *hay_iter++) != '\0') {
  haych = STRICAST(haych),ned_iter = needle_list;
  while ((__STRING_ASSERTBYTE(ned_iter),ch = *ned_iter++) != '\0') {
   if (haych == STRICAST(ch)) return hay_iter-1;
  }
 }
 return NULL;
#endif
}
#endif

#if !defined(_strirpbrk) || \
     defined(__arch_generic_strirpbrk) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef _strirpbrk
__public char *_strirpbrk(char const *haystack, char const *needle_list) {
#if defined(arch_stripbrk) && \
   !defined(__arch_generic_stripbrk) && \
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return arch_stripbrk(haystack,needle_list);
#else
 char *result = NULL,*hay_iter = (char *)haystack;
 char const *ned_iter; char haych,ch;
 assert(haystack);
 assert(needle_list);
 while ((__STRING_ASSERTBYTE(hay_iter),haych = *hay_iter++) != '\0') {
  haych = STRICAST(haych),ned_iter = needle_list;
  while ((__STRING_ASSERTBYTE(ned_iter),ch = *ned_iter++) != '\0') {
   if (haych == STRICAST(ch)) { result = hay_iter-1; break; }
  }
 }
 return result;
#endif
}
#endif

#if !defined(_strinpbrk) || \
     defined(__arch_generic_strinpbrk) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef _strinpbrk
__public char *_strinpbrk(char const *haystack, size_t max_haychars,
                          char const *needle_list, size_t max_needlelist) {
#if defined(arch_strinpbrk) && \
   !defined(__arch_generic_strinpbrk) && \
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return arch_strinpbrk(haystack,max_haychars,needle_list,max_needlelist);
#else
 char *hay_iter = (char *)haystack;
 char *hay_end = hay_iter+max_haychars;
 char const *ned_iter,*ned_end; char haych,ch;
 assert(!max_haychars || haystack);
 assert(!max_needlelist || needle_list);
 if __unlikely(!max_needlelist) return NULL;
 ned_end = needle_list+max_needlelist;
 while (hay_iter != hay_end  &&
       (__STRING_ASSERTBYTE(hay_iter),haych = *hay_iter++) != '\0') {
  haych = STRICAST(haych),ned_iter = needle_list;
  while ((__STRING_ASSERTBYTE(ned_iter),ch = *ned_iter++) != '\0') {
   if (haych == STRICAST(ch)) return hay_iter-1;
   if (ned_iter == ned_end) break;
  }
 }
 return NULL;
#endif
}
#endif

#if !defined(_strinrpbrk) || \
     defined(__arch_generic_strinrpbrk) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef _strinrpbrk
__public char *_strinrpbrk(char const *haystack, size_t max_haychars,
                           char const *needle_list, size_t max_needlelist) {
#if defined(arch_strinrpbrk) && \
   !defined(__arch_generic_strinrpbrk) && \
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return arch_strinrpbrk(haystack,max_haychars,needle_list,max_needlelist);
#else
 char *hay_iter = (char *)haystack;
 char *hay_end = hay_iter+max_haychars;
 char *result = NULL;
 char const *ned_iter,*ned_end; char haych,ch;
 assert(!max_haychars || haystack);
 assert(!max_needlelist || needle_list);
 if __unlikely(!max_needlelist) return NULL;
 ned_end = needle_list+max_needlelist;
 while (hay_iter != hay_end  &&
       (__STRING_ASSERTBYTE(hay_iter),haych = *hay_iter++) != '\0') {
  haych = STRICAST(haych),ned_iter = needle_list;
  while ((__STRING_ASSERTBYTE(ned_iter),ch = *ned_iter++) != '\0') {
   if (haych == STRICAST(ch)) { result = hay_iter-1; break; }
   if (ned_iter == ned_end) break;
  }
 }
 return result;
#endif
}
#endif
#endif /* !__CONFIG_MIN_LIBC__ */

#ifndef __CONFIG_MIN_LIBC__
#if !defined(_strset) || \
     defined(__arch_generic_strset) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef _strset
__public char *_strset(char *__restrict dst, int chr) {
#if defined(arch_strset) && \
   !defined(__arch_generic_strset) && !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return arch_strset(dst,chr);
#else
 char *iter = dst;
 while ((__STRING_ASSERTBYTE(iter),*iter)) *iter = (char)chr;
 return dst;
#endif
}
#endif

#if !defined(_strnset) || \
     defined(__arch_generic_strnset) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef _strnset
__public char *_strnset(char *__restrict dst, int chr, size_t maxlen) {
#if defined(arch_strnset) && \
   !defined(__arch_generic_strnset) && !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return arch_strnset(dst,chr,maxlen);
#elif defined(arch_memset)  && !defined(__arch_generic_memset) && \
      defined(arch_strnlen) && !defined(__arch_generic_strnlen)
 return (char *)arch_memset(dst,chr,arch_strnlen(dst,maxlen));
#else
 char *iter,*end; end = (iter = dst)+maxlen;
 while (iter != end && (__STRING_ASSERTBYTE(iter),*iter)) *iter = (char)chr;
 return dst;
#endif
}
#endif

#if !defined(_memrev) || \
     defined(__arch_generic_memrev) || \
    !defined(__CONFIG_MIN_LIBC__)
#undef _memrev
__public void *_memrev(void *p, size_t bytes) {
#if defined(arch_memrev) && \
   !defined(__arch_generic_memrev)
 return arch_memrev(p,bytes);
#else
 byte_t *iter,*end,*swap,temp;
 __STRING_ASSERTMEM(p,bytes);
 if (bytes > 1) {
  end = (iter = (byte_t *)p)+(bytes/2);
  swap = iter+bytes;
  assert(iter != end);
  do {
   --swap;
   temp = *iter;
   *iter = *swap;
   *swap = temp;
   ++iter;
  } while (iter != end);
 }
 return p;
#endif
}
#endif

#if !defined(_strlwr) || !defined(__CONFIG_MIN_LIBC__)
#undef _strlwr
__public char *_strlwr(char *str) {
 char *iter = str;
 if __likely(str) {
  while ((__STRING_ASSERTBYTE(iter),*iter))
   *iter = tolower(*iter),++iter;
 }
 return str;
}
#endif

#if !defined(_strupr) || !defined(__CONFIG_MIN_LIBC__)
#undef _strupr
__public char *_strupr(char *str) {
 char *iter = str;
 if __likely(str) {
  while ((__STRING_ASSERTBYTE(iter),*iter))
   *iter = toupper(*iter),++iter;
 }
 return str;
}
#endif

#if !defined(_strnlwr) || !defined(__CONFIG_MIN_LIBC__)
#undef _strnlwr
__public char *_strnlwr(char *str, size_t maxlen) {
 char *iter,*end; end = (iter = str)+maxlen;
 while (iter != end && (__STRING_ASSERTBYTE(iter),*iter))
  *iter = tolower(*iter),++iter;
 return str;
}
#endif

#if !defined(_strnupr) || !defined(__CONFIG_MIN_LIBC__)
#undef _strnupr
__public char *_strnupr(char *str, size_t maxlen) {
 char *iter,*end; end = (iter = str)+maxlen;
 while (iter != end && (__STRING_ASSERTBYTE(iter),*iter))
  *iter = toupper(*iter),++iter;
 return str;
}
#endif
#endif /* !__CONFIG_MIN_LIBC__ */

#if !defined(strtok_r) || !defined(__CONFIG_MIN_LIBC__)
#undef strtok_r
__public char *
strtok_r(char *__restrict str,
         const char *__restrict delim,
         char **__restrict saveptr) {
 char ch,*result;
#ifdef STRING_AVOID_STRCHR
 size_t delim_size = string_strlen(delim)*sizeof(char);
#endif
 assert(delim);
 assert(saveptr);
 if (!str && (str = *saveptr) == NULL) return NULL;
 /* Skip leading delimiters. */
#ifdef STRING_AVOID_STRCHR
 while ((ch = *str++) != '\0' && string_memchr(delim,ch,delim_size));
#else
 while ((ch = *str++) != '\0' && strchr(delim,ch));
#endif
 if __unlikely(ch == '\0') goto stop; /* End of string. */
 result = str-1; /* Will return the next non-empty segment. */
 for (;;) {
  ch = *str++;
  if __unlikely(ch == '\0') { str = NULL; break; }
#ifdef STRING_AVOID_STRCHR
  if (string_memchr(delim,ch,delim_size)) { str[-1] = '\0'; break; }
#else
  if (strchr(delim,ch)) { str[-1] = '\0'; break; }
#endif
 }
 *saveptr = str;
 return result;
stop:
 *saveptr = NULL;
 return NULL;
}
#endif

#if !defined(_strntok_r) || !defined(__CONFIG_MIN_LIBC__)
#undef _strntok_r
__public char *
_strntok_r(char *__restrict str, const char *__restrict delim,
           size_t delim_max, char **__restrict saveptr) {
 char ch,*result;
#ifdef STRING_AVOID_STRNCHR
 delim_max = string_strnlen(delim,delim_max)*sizeof(char);
#endif
 assert(delim);
 assert(saveptr);
 if (!str && (str = *saveptr) == NULL) return NULL;
 /* Skip leading delimiters. */
#ifdef STRING_AVOID_STRNCHR
 while ((ch = *str++) != '\0' && string_memchr(delim,ch,delim_max));
#else
 while ((ch = *str++) != '\0' && strnchr(delim,delim_max,ch));
#endif
 if __unlikely(ch == '\0') goto stop; /* End of string. */
 result = str-1; /* Will return the next non-empty segment. */
 for (;;) {
  ch = *str++;
  if __unlikely(ch == '\0') { str = NULL; break; }
#ifdef STRING_AVOID_STRNCHR
  if (string_memchr(delim,ch,delim_max)) { str[-1] = '\0'; break; }
#else
  if (strnchr(delim,delim_max,ch)) { str[-1] = '\0'; break; }
#endif
 }
 *saveptr = str;
 return result;
stop:
 *saveptr = NULL;
 return NULL;
}
#endif

#ifndef __CONFIG_MIN_LIBC__
#if !defined(_stritok_r) || !defined(__CONFIG_MIN_LIBC__)
#undef _stritok_r
__public char *
_stritok_r(char *__restrict str,
           const char *__restrict delim,
           char **__restrict saveptr) {
 char ch,*result;
 assert(delim);
 assert(saveptr);
 if (!str && (str = *saveptr) == NULL) return NULL;
 /* Skip leading delimiters. */
 while ((ch = *str++) != '\0' && _strichr(delim,ch));
 if __unlikely(ch == '\0') goto stop; /* End of string. */
 result = str-1; /* Will return the next non-empty segment. */
 for (;;) {
  ch = *str++;
  if __unlikely(ch == '\0') { str = NULL; break; }
  if (_strichr(delim,ch)) { str[-1] = '\0'; break; }
 }
 *saveptr = str;
 return result;
stop:
 *saveptr = NULL;
 return NULL;
}
#endif

#if !defined(_strintok_r) || !defined(__CONFIG_MIN_LIBC__)
#undef _strintok_r
__public char *
_strintok_r(char *__restrict str, const char *__restrict delim,
            size_t delim_max, char **__restrict saveptr) {
 char ch,*result;
 assert(delim);
 assert(saveptr);
 if (!str && (str = *saveptr) == NULL) return NULL;
 /* Skip leading delimiters. */
 while ((ch = *str++) != '\0' && strinchr(delim,delim_max,ch));
 if __unlikely(ch == '\0') goto stop; /* End of string. */
 result = str-1; /* Will return the next non-empty segment. */
 for (;;) {
  ch = *str++;
  if __unlikely(ch == '\0') { str = NULL; break; }
  if (strinchr(delim,delim_max,ch)) { str[-1] = '\0'; break; }
 }
 *saveptr = str;
 return result;
stop:
 *saveptr = NULL;
 return NULL;
}
#endif
#endif /* !__CONFIG_MIN_LIBC__ */

#ifndef __CONFIG_MIN_BSS__
#if !defined(strtok) || !defined(__CONFIG_MIN_LIBC__)
#undef strtok
__public char *strtok(char *__restrict str,
                      const char *__restrict delim) {
 static char *saveptr = NULL;
 return strtok_r(str,delim,&saveptr);
}
#endif
#endif /* !__CONFIG_MIN_BSS__ */


#ifndef __CONFIG_MIN_LIBC__
#ifndef __INTELLISENSE__
#if !defined(_strinwcmp) || !defined(__CONFIG_MIN_LIBC__)
#define STRN
#define STRI
#include "string-wmatch.c.inl"
#endif
#if !defined(_strnwcmp) || !defined(__CONFIG_MIN_LIBC__)
#define STRN
#include "string-wmatch.c.inl"
#endif
#if !defined(_striwcmp) || !defined(__CONFIG_MIN_LIBC__)
#define STRI
#include "string-wmatch.c.inl"
#endif
#if !defined(_strwcmp) || !defined(__CONFIG_MIN_LIBC__)
#include "string-wmatch.c.inl"
#endif
#endif /* !__INTELLISENSE__ */
#endif /* !__CONFIG_MIN_LIBC__ */



#ifndef __CONFIG_MIN_BSS__
__public char *strerror(int eno) {
 char const *result;
 switch (eno) {
  case ENOMEM    : result = "ENOMEM: Not enough dynamic memory available to complete this operation"; break;
  case EINVAL    : result = "EINVAL: A given argument is invalid"; break;
  case EDESTROYED: result = "EDESTROYED: A given object was destroyed/the object's state doesn't allow for this operation"; break;
  case ETIMEDOUT : result = "ETIMEDOUT: A given timeout has passed/a try-call would block"; break;
  case ERANGE    : result = "ERANGE: An integer/pointer lies out of its valid range"; break;
  case EOVERFLOW : result = "EOVERFLOW: An integer overflow/underflow would/did occur"; break;
  case EACCES    : result = "EACCES: Permissions were denied"; break;
  case ENOSYS    : result = "ENOSYS: A given feature is not available"; break;
  case ELOOP     : result = "ELOOP: The operation would create an illegal loop of sorts"; break;
  case EDEVICE   : result = "EDEVICE: A device responded unexpectedly, and is likely damaged, or broken"; break;
  case ENOENT    : result = "ENOENT: No such file or directory"; break;
  case ENODIR    : result = "ENODIR: Entity is not a directory"; break;
  case ENOSPC    : result = "ENOSPC: Not enough available space"; break;
  case EISDIR    : result = "EISDIR: Entity is a directory"; break;
  case EEXISTS   : result = "EEXISTS: Object is still in the possession of data"; break;
  case EBUFSIZ   : result = "EBUFSIZ: Buffer too small"; break;
  case EBADF     : result = "EBADF: Invalid (bad) file descriptor"; break;
  case EMFILE    : result = "EMFILE: Too many open file descriptors"; break;
  case EFAULT    : result = "EFAULT: A faulty pointer was given"; break;
  case EDOM      : result = "EDOM: Domain error"; break;
  default        : result = NULL; break;
 }
 return (char *)result;
}
__public char *strerror_r(int eno, char *__restrict buf, size_t buflen) {
 char *stext;
 if __unlikely(!buflen) return buf;
 if __unlikely((stext = strerror(eno)) == NULL) { *buf = '\0'; return buf; }
 strncpy(buf,stext,buflen-1);
 buf[buflen-1] = '\0';
 return buf;
}
#endif /* !__CONFIG_MIN_BSS__ */

#if !defined(basename) || !defined(__CONFIG_MIN_LIBC__)
#undef basename
__public char *basename(char *__restrict path) {
 char ch,*iter = path,*result = NULL;
 if (!path || !*path) return ".";
 do if ((ch = *iter++) == '/') result = iter;
 while (ch);
 if __unlikely(!result) return path; /* Path doesn't contain '/'. */
 if (*result) return result; /* Last character isn't a '/'. */
 iter = result;
 while (iter != path && iter[-1] == '/') --iter;
 if (iter == path) return result-1; /* Only '/'-characters. */
 *iter = '\0'; /* Trim all ending '/'-characters. */
 while (iter != path && iter[-1] != '/') --iter; /* Scan until the previous '/'. */
 return iter; /* Returns string after previous '/'. */
}
#endif

#if !defined(dirname) || !defined(__CONFIG_MIN_LIBC__)
#undef dirname
__public char *dirname(char *__restrict path) {
 char *iter;
 if (!path || !*path) ret_cwd: return ".";
 iter = strend(path)-1;
 while (*iter == '/') {
  if (iter == path) { iter[1] = '\0'; return path; }
  --iter;
 }
 while (iter >= path && *iter != '/') --iter;
 if (iter < path) goto ret_cwd;
 if (iter == path) ++iter;
 *iter = '\0';
 return path;
}
#endif

__DECL_END

#endif /* !__STRING_C__ */

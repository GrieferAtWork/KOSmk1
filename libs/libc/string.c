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

#ifndef __KERNEL__
#include <errno.h>
#endif

#include <kos/arch/string.h>
#include <kos/config.h>
#ifdef __LIBC_HAVE_DEBUG_MEMCHECKS
#   include <kos/kernel/debug.h>
#   define __STRING_ASSERTBYTE(p)  kassertbyte(p)
#   define __STRING_ASSERTMEM(p,s) kassertmem(p,s)
#else
#   define __STRING_ASSERTBYTE(p)  (void)0
#   define __STRING_ASSERTMEM(p,s) assert(!(s) || (p))
#endif

__DECL_BEGIN

#undef memcpy
__public void *memcpy(void *__restrict dst,
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
 {
#if defined(karch_memcpy)
  __STRING_ASSERTMEM(dst,bytes);
  __STRING_ASSERTMEM(src,bytes);
  return karch_memcpy(dst,src,bytes);
#elif 0
  __u8 *iter,*end; __u8 const *siter;
  __STRING_ASSERTMEM(dst,bytes);
  __STRING_ASSERTMEM(src,bytes);
  siter = (__u8 const *)src;
  end = (iter = (__u8 *)dst)+bytes;
  while (iter != end) *iter++ = *siter++;
#else
  int *iter,*end; int const *siter;
  __STRING_ASSERTMEM(dst,bytes);
  __STRING_ASSERTMEM(src,bytes);
  siter = (int const *)src;
  end = (iter = (int *)dst)+(bytes/__SIZEOF_INT__);
  while (iter != end) *iter++ = *siter++;
#if __SIZEOF_INT__ > 1
  switch (bytes % __SIZEOF_INT__) {
#if __SIZEOF_INT__ > 4
   case 7: *(*(__u8 **)&iter)++ = *(*(__u8 **)&siter)++;
   case 6: *(*(__u8 **)&iter)++ = *(*(__u8 **)&siter)++;
   case 5: *(*(__u8 **)&iter)++ = *(*(__u8 **)&siter)++;
   case 4: *(*(__u8 **)&iter)++ = *(*(__u8 **)&siter)++;
#endif
#if __SIZEOF_INT__ > 2
   case 3: *(*(__u8 **)&iter)++ = *(*(__u8 **)&siter)++;
   case 2: *(*(__u8 **)&iter)++ = *(*(__u8 **)&siter)++;
#endif
   case 1: *(*(__u8 **)&iter)++ = *(*(__u8 **)&siter)++;
   default: break;
  }
#endif /* __SIZEOF_INT__ > 1 */
#endif
 }
 return (void *)dst;
}

#undef memccpy
__public void *
memccpy(void *__restrict dst,
        void const *__restrict src,
        int c, size_t bytes) {
#if defined(karch_memccpy)
 return karch_memccpy(dst,src,c,bytes);
#else
 __u8 *iter,*end; __u8 const *src_iter;
 end = (iter = (__u8 *)dst)+bytes;
 src_iter = (__u8 const *)src;
 while (iter != end) {
  __STRING_ASSERTBYTE(iter);
  __STRING_ASSERTBYTE(src_iter);
  if ((*iter++ = *src_iter++) == c) return iter;
 }
 return NULL;
#endif
}


#undef memmove
__public void *memmove(void *dst,
                       void const *src,
                       size_t bytes) {
#ifdef karch_memmove
 __STRING_ASSERTMEM(dst,bytes);
 __STRING_ASSERTMEM(src,bytes);
 return karch_memmove(dst,src,bytes);
#else
 __u8 *iter,*end; __u8 const *siter;
 __STRING_ASSERTMEM(dst,bytes);
 __STRING_ASSERTMEM(src,bytes);
 if (dst < src) {
  siter = (__u8 const *)src;
  end = (iter = (__u8 *)dst)+bytes;
  while (iter != end) *iter++ = *siter++;
 } else {
  siter = (__u8 const *)src+bytes;
  iter = (end = (__u8 *)dst)+bytes;
  while (iter != end) *--iter = *--siter;
 }
 return dst;
#endif
}

#undef memset
__public void *memset(void *__restrict dst,
                      int byte, size_t bytes) {
#ifdef karch_memset
 __STRING_ASSERTMEM(dst,bytes);
 return karch_memset(dst,byte,bytes);
#else
 __u8 *iter,*end;
 __STRING_ASSERTMEM(dst,bytes);
 end = (iter = (__u8 *)dst)+bytes;
 while (iter != end) *iter++ = (__u8)byte;
 return dst;
#endif
}

#undef memcmp
__public int memcmp(void const *a,
                    void const *b,
                    size_t bytes) {
#ifdef karch_memcmp
 __STRING_ASSERTMEM(a,bytes);
 __STRING_ASSERTMEM(b,bytes);
 return karch_memcmp(a,b,bytes);
#else
 __u8 const *aiter,*biter,*end; __u8 av = 0,bv = 0;
 __STRING_ASSERTMEM(a,bytes);
 __STRING_ASSERTMEM(b,bytes);
 end = (aiter = (__u8 const *)a)+bytes,biter = (__u8 const *)b;
 while (aiter != end && ((av = *aiter++) == (bv = *biter++)));
 return (int)(av-bv);
#endif
}

#undef memchr
__public void *memchr(void const *p, int needle, size_t bytes) {
#ifdef karch_memchr
 __STRING_ASSERTMEM(p,bytes);
 return karch_memchr(p,needle,bytes);
#else
 __u8 *iter,*end;
 __STRING_ASSERTMEM(p,bytes);
 end = (iter = (__u8 *)p)+bytes;
 while (iter != end) {
  if (*iter == needle) return iter;
  ++iter;
 }
 return NULL;
#endif
}

#undef memrchr
__public void *memrchr(void const *p, int needle, size_t bytes) {
#ifdef karch_memrchr
 __STRING_ASSERTMEM(p,bytes);
 return karch_memrchr(p,needle,bytes);
#else
 __u8 *iter,*end,*result = NULL;
 __STRING_ASSERTMEM(p,bytes);
 end = (iter = (__u8 *)p)+bytes;
 while (iter != end) {
  if (*iter == needle) result = iter;
  ++iter;
 }
 return result;
#endif
}

#ifndef __KERNEL__
#undef strcat
__public char *strcat(char *dst, char const *src) {
 return strcpy(_strend(dst),src);
}
#undef strncat
__public char *strncat(char *dst, char const *src,
                       size_t maxchars) {
 return strncpy(_strend(dst),src,maxchars);
}
#endif /* !__KERNEL__ */

#undef strcpy
__public char *strcpy(char *dst, char const *src) {
#if defined(karch_strcpy) &&\
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return karch_strcpy(dst,src);
#else
 char *iter = dst,ch; char const *siter = src;
 while ((__STRING_ASSERTBYTE(siter),ch = *siter++) != '\0')
  __STRING_ASSERTBYTE(iter),*iter++ = ch;
 __STRING_ASSERTBYTE(iter),*iter = '\0';
 return iter;
#endif
}
#undef strncpy
__public char *strncpy(char *dst, char const *src, size_t maxchars) {
#if defined(karch_strncpy) &&\
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return karch_strncpy(dst,src,maxchars);
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

#undef _strend
__public char *_strend(char const *__restrict s) {
#if defined(karch_strend) &&\
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return karch_strend(s);
#elif defined(karch_strlen) &&\
     !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return s+karch_strlen(s);
#else
 char const *result = s;
 while ((__STRING_ASSERTBYTE(result),*result)) ++result;
 return (char *)result;
#endif
}

#undef _strnend
__public char *_strnend(char const *__restrict s, size_t maxchars) {
#if defined(karch_strnend) &&\
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return karch_strnend(s,maxchars);
#elif defined(karch_strnlen) &&\
     !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return s+karch_strnlen(s,maxchars);
#else
 char const *result,*max;
 assert(!maxchars || s);
 max = (result = s)+maxchars;
 while (result != max && (__STRING_ASSERTBYTE(result),*result)) ++result;
 return (char *)result;
#endif
}

#undef strlen
__public size_t strlen(char const *__restrict s) {
#if defined(karch_strlen) &&\
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return karch_strlen(s);
#elif defined(karch_strend) &&\
     !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return (size_t)(karch_strend(s)-s);
#else
 char const *end = s;
 while ((__STRING_ASSERTBYTE(end),*end)) ++end;
 return (size_t)(end-s);
#endif
}
#undef strnlen
__public size_t strnlen(char const *__restrict s, size_t maxchars) {
#if defined(karch_strnlen) &&\
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return karch_strnlen(s,maxchars);
#elif defined(karch_strnend) &&\
     !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return (size_t)(karch_strnend(s,maxchars)-s);
#else
 char const *end,*max;
 assert(!maxchars || s);
 max = (end = s)+maxchars;
 while (end != max && (__STRING_ASSERTBYTE(end),*end)) ++end;
 return (size_t)(end-s);
#endif
}

#undef strspn
__public size_t strspn(char const *str, char const *spanset) {
#if defined(karch_strspn) &&\
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return karch_strspn(str,spanset);
#else
 char const *iter = str;
 while ((__STRING_ASSERTBYTE(iter),strchr(spanset,*iter))) ++iter;
 return (size_t)(iter-str);
#endif
}

#undef _strnspn
__public size_t _strnspn(char const *str, size_t maxstr,
                         char const *spanset, size_t maxspanset) {
#if defined(karch_strnspn) &&\
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return karch_strnspn(str,maxstr,spanset,maxspanset);
#else
 char const *iter,*end; end = (iter = str)+maxstr;
 while (iter != end && (__STRING_ASSERTBYTE(iter),strnchr(spanset,maxspanset,*iter))) ++iter;
 return (size_t)(iter-str);
#endif
}

#undef strcspn
__public size_t strcspn(char const *str, char const *spanset) {
#if defined(karch_strcspn) &&\
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return karch_strcspn(str,spanset);
#else
 char const *iter = str;
 while ((__STRING_ASSERTBYTE(iter),*iter && !strchr(spanset,*iter))) ++iter;
 return (size_t)(iter-str);
#endif
}

#undef _strncspn
__public size_t _strncspn(char const *str, size_t maxstr,
                          char const *spanset, size_t maxspanset) {
#if defined(karch_strncspn) &&\
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return karch_strncspn(str,maxstr,spanset,maxspanset);
#else
 char const *iter,*end; end = (iter = str)+maxstr;
 while (iter != end && (__STRING_ASSERTBYTE(iter),*iter &&
                       !strnchr(spanset,maxspanset,*iter))
        ) ++iter;
 return (size_t)(iter-str);
#endif
}


#undef strcmp
__public int strcmp(char const *a, char const *b) {
#if defined(karch_strcmp) &&\
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return karch_strcmp(a,b);
#else
 char const *aiter,*biter; int cha,chb;
 assert(a && b); aiter = a,biter = b;
 do __STRING_ASSERTBYTE(aiter),cha = *aiter++,
    __STRING_ASSERTBYTE(biter),chb = *biter++;
 while (cha && chb && cha == chb);
 return (int)((signed char)cha-(signed char)chb);
#endif
}
#undef strncmp
__public int strncmp(char const *a, char const *b, size_t maxchars) {
#if defined(karch_strncmp) &&\
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return karch_strncmp(a,b,maxchars);
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

#undef strchr
__public char *strchr(char const *__restrict haystack, int needle) {
#if defined(karch_strchr) &&\
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return karch_strchr(haystack,needle);
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


#undef strrchr
__public char *strrchr(char const *__restrict haystack, int needle) {
#if defined(karch_strrchr) &&\
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return karch_strrchr(haystack,needle);
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

#undef _strnchr
__public char *_strnchr(char const *__restrict haystack,
                        size_t max_haychars, int needle) {
#if defined(karch_strnchr) &&\
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return karch_strnchr(haystack,max_haychars,needle);
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
#undef _strnrchr
__public char *_strnrchr(char const *__restrict haystack,
                         size_t max_haychars, int needle) {
#if defined(karch_strnrchr) &&\
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return karch_strnrchr(haystack,max_haychars,needle);
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


#undef strstr
__public char *strstr(char const *haystack, char const *needle) {
#if defined(karch_strstr) &&\
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return karch_strstr(haystack,needle);
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

#undef _strrstr
__public char *_strrstr(char const *haystack, char const *needle) {
#if defined(karch_strrstr) &&\
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return karch_strrstr(haystack,needle);
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

#undef strpbrk
__public char *strpbrk(char const *haystack, char const *needle_list) {
#if defined(karch_strpbrk) &&\
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return karch_strpbrk(haystack,needle_list);
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

#undef _strrpbrk
__public char *_strrpbrk(char const *haystack, char const *needle_list) {
#if defined(karch_strpbrk) &&\
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return karch_strpbrk(haystack,needle_list);
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

#undef _strnpbrk
__public char *_strnpbrk(char const *haystack, size_t max_haychars,
                         char const *needle_list, size_t max_needlelist) {
#if defined(karch_strnpbrk) &&\
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return karch_strnpbrk(haystack,max_haychars,needle_list,max_needlelist);
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

#undef _strnrpbrk
__public char *_strnrpbrk(char const *haystack, size_t max_haychars,
                          char const *needle_list, size_t max_needlelist) {
#if defined(karch_strnrpbrk) &&\
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return karch_strnrpbrk(haystack,max_haychars,needle_list,max_needlelist);
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



#undef _strnstr
__public char *_strnstr(char const *haystack, size_t max_haychars,
                        char const *needle, size_t max_needlechars) {
#if defined(karch_strnstr) &&\
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return karch_strnstr(haystack,max_haychars,needle,max_needlechars);
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

#undef _strnrstr
__public char *_strnrstr(char const *haystack, size_t max_haychars,
                         char const *needle, size_t max_needlechars) {
#if defined(karch_strnrstr) &&\
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return karch_strnrstr(haystack,max_haychars,needle,max_needlechars);
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

#ifndef __CONFIG_MIN_LIBC__
/* Character casting used to unify characters in stri-style functions.
 * -> This should either be tolower or toupper. */
#define STRICAST(ch) tolower(ch)

#undef _stricmp
__public int _stricmp(char const *a, char const *b) {
#if defined(karch_stricmp) &&\
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return karch_stricmp(a,b);
#else
 char const *aiter = a,*biter = b; int cha,chb; assert(a && b);
 do __STRING_ASSERTBYTE(aiter),cha = STRICAST(*aiter++),
    __STRING_ASSERTBYTE(biter),chb = STRICAST(*biter++);
 while (cha && chb && cha == chb);
 return (int)((signed char)cha-(signed char)chb);
#endif
}

#undef _memicmp
__public int _memicmp(void const *a,
                      void const *b,
                      size_t bytes) {
#ifdef karch_memicmp
 __STRING_ASSERTMEM(a,bytes);
 __STRING_ASSERTMEM(b,bytes);
 return karch_memicmp(a,b,bytes);
#else
 __u8 const *aiter,*biter,*end; __u8 av = 0,bv = 0;
 __STRING_ASSERTMEM(a,bytes);
 __STRING_ASSERTMEM(b,bytes);
 end = (aiter = (__u8 const *)a)+bytes,biter = (__u8 const *)b;
 while (aiter != end && ((av = STRICAST(*aiter++)) == (bv = STRICAST(*biter++))));
 return (int)(av-bv);
#endif
}

#undef _strincmp
__public int _strincmp(char const *a, char const *b, size_t maxchars) {
#if defined(karch_strincmp) &&\
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return karch_strincmp(a,b,maxchars);
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

#undef _memichr
__public void *_memichr(void const *__restrict p, int needle, size_t bytes) {
#if defined(karch_memichr) &&\
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return karch_memichr(p,needle,bytes);
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

#undef _memirchr
__public void *_memirchr(void const *__restrict p, int needle, size_t bytes) {
#if defined(karch_memirchr) &&\
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return karch_memirchr(p,needle,bytes);
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

#undef _strichr
__public char *_strichr(char const *__restrict haystack, int needle) {
#if defined(karch_strichr) &&\
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return karch_strichr(haystack,needle);
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

#undef _strirchr
__public char *_strirchr(char const *__restrict haystack, int needle) {
#if defined(karch_strichr) &&\
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return karch_strichr(haystack,needle);
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

#undef _stristr
__public char *_stristr(char const *haystack, char const *needle) {
#if defined(karch_stristr) &&\
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return karch_stristr(haystack,needle);
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

#undef _strirstr
__public char *_strirstr(char const *haystack, char const *needle) {
#if defined(karch_strirstr) &&\
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return karch_strirstr(haystack,needle);
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

#undef _strinchr
__public char *_strinchr(char const *__restrict haystack, size_t max_haychars, int needle) {
#if defined(karch_strinchr) &&\
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return karch_strinchr(haystack,max_haychars,needle);
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

#undef _strinrchr
__public char *_strinrchr(char const *__restrict haystack, size_t max_haychars, int needle) {
#if defined(karch_strinrchr) &&\
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return karch_strinrchr(haystack,max_haychars,needle);
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

#undef _strinstr
__public char *_strinstr(char const *haystack, size_t max_haychars,
                         char const *needle, size_t max_needlechars) {
#if defined(karch_strinstr) &&\
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return karch_strinstr(haystack,max_haychars,needle,max_needlechars);
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

#undef _strinrstr
__public char *_strinrstr(char const *haystack, size_t max_haychars,
                          char const *needle, size_t max_needlechars) {
#if defined(karch_strinrstr) &&\
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return karch_strinrstr(haystack,max_haychars,needle,max_needlechars);
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

#undef _stripbrk
__public char *_stripbrk(char const *haystack, char const *needle_list) {
#if defined(karch_stripbrk) &&\
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return karch_stripbrk(haystack,needle_list);
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

#undef _strirpbrk
__public char *_strirpbrk(char const *haystack, char const *needle_list) {
#if defined(karch_stripbrk) &&\
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return karch_stripbrk(haystack,needle_list);
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

#undef _strinpbrk
__public char *_strinpbrk(char const *haystack, size_t max_haychars,
                          char const *needle_list, size_t max_needlelist) {
#if defined(karch_strinpbrk) &&\
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return karch_strinpbrk(haystack,max_haychars,needle_list,max_needlelist);
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

#undef _strinrpbrk
__public char *_strinrpbrk(char const *haystack, size_t max_haychars,
                           char const *needle_list, size_t max_needlelist) {
#if defined(karch_strinrpbrk) &&\
   !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return karch_strinrpbrk(haystack,max_haychars,needle_list,max_needlelist);
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
#endif /* !__CONFIG_MIN_LIBC__ */

#ifndef __CONFIG_MIN_LIBC__
#undef _strset
__public char *_strset(char *__restrict dst, int chr) {
#if defined(karch_strset) && !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return karch_strset(dst,chr);
#elif defined(karch_memset) && defined(karch_strlen)
 return (char *)memset(dst,chr,strlen(dst));
#else
 char *iter = dst;
 while ((__STRING_ASSERTBYTE(iter),*iter)) *iter = (char)chr;
 return dst;
#endif
}

#undef _strnset
__public char *_strnset(char *__restrict dst, int chr, size_t maxlen) {
#if defined(karch_strnset) && !defined(__LIBC_HAVE_DEBUG_MEMCHECKS)
 return karch_strnset(dst,chr,maxlen);
#elif defined(karch_memset) && defined(karch_strnlen)
 return (char *)memset(dst,chr,strnlen(dst,maxlen));
#else
 char *iter,*end; end = (iter = dst)+maxlen;
 while (iter != end && (__STRING_ASSERTBYTE(iter),*iter)) *iter = (char)chr;
 return dst;
#endif
}

#undef _memrev
__public void *_memrev(void *p, size_t bytes) {
 __u8 *iter,*end,*swap,temp;
 __STRING_ASSERTMEM(p,bytes);
 if (bytes > 1) {
  end = (iter = (__u8 *)p)+(bytes/2);
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
}

#undef _strlwr
__public char *_strlwr(char *str) {
 char *iter = str;
 if __likely(str) {
  while ((__STRING_ASSERTBYTE(iter),*iter))
   *iter = tolower(*iter),++iter;
 }
 return str;
}
#undef _strupr
__public char *_strupr(char *str) {
 char *iter = str;
 if __likely(str) {
  while ((__STRING_ASSERTBYTE(iter),*iter))
   *iter = toupper(*iter),++iter;
 }
 return str;
}
#undef _strnlwr
__public char *_strnlwr(char *str, size_t maxlen) {
 char *iter,*end; end = (iter = str)+maxlen;
 while (iter != end && (__STRING_ASSERTBYTE(iter),*iter))
  *iter = tolower(*iter),++iter;
 return str;
}
#undef _strnupr
__public char *_strnupr(char *str, size_t maxlen) {
 char *iter,*end; end = (iter = str)+maxlen;
 while (iter != end && (__STRING_ASSERTBYTE(iter),*iter))
  *iter = toupper(*iter),++iter;
 return str;
}
#endif /* !__CONFIG_MIN_LIBC__ */

#undef strtok_r
__public char *
strtok_r(char *__restrict str,
         const char *__restrict delim,
         char **__restrict saveptr) {
 char ch,*result;
 assert(delim);
 assert(saveptr);
 if (!str && (str = *saveptr) == NULL) return NULL;
 /* Skip leading delimiters. */
 while ((ch = *str++) != '\0' && strchr(delim,ch));
 if __unlikely(ch == '\0') goto stop; /* End of string. */
 result = str-1; /* Will return the next non-empty segment. */
 for (;;) {
  ch = *str++;
  if __unlikely(ch == '\0') { str = NULL; break; }
  if (strchr(delim,ch)) { str[-1] = '\0'; break; }
 }
 *saveptr = str;
 return result;
stop:
 *saveptr = NULL;
 return NULL;
}

#undef _strntok_r
__public char *
_strntok_r(char *__restrict str, const char *__restrict delim,
           size_t delim_max, char **__restrict saveptr) {
 char ch,*result;
 assert(delim);
 assert(saveptr);
 if (!str && (str = *saveptr) == NULL) return NULL;
 /* Skip leading delimiters. */
 while ((ch = *str++) != '\0' && strnchr(delim,delim_max,ch));
 if __unlikely(ch == '\0') goto stop; /* End of string. */
 result = str-1; /* Will return the next non-empty segment. */
 for (;;) {
  ch = *str++;
  if __unlikely(ch == '\0') { str = NULL; break; }
  if (strnchr(delim,delim_max,ch)) { str[-1] = '\0'; break; }
 }
 *saveptr = str;
 return result;
stop:
 *saveptr = NULL;
 return NULL;
}

#ifndef __CONFIG_MIN_LIBC__
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
#endif /* !__CONFIG_MIN_LIBC__ */

#ifndef __CONFIG_MIN_BSS__
#undef strtok
__public char *strtok(char *__restrict str,
                      const char *__restrict delim) {
 static char *saveptr = NULL;
 return strtok_r(str,delim,&saveptr);
}
#endif /* !__CONFIG_MIN_BSS__ */


#undef ffs
__public int ffs(int i) {
#if defined(__GNUC__) || __has_builtin(__builtin_ffs)
 return __builtin_ffs(i);
#elif defined(karch_ffs)
 return karch_ffs(i);
#else
 int result;
 if (!i) return 0;
 for (result = 1; !(i&1); ++result) i >>= 1;
 return result;
#endif

}



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

__public char *basename(char *__restrict path) {
 char ch,*iter = path,*result = NULL;
 if (!path || !*path) return ".";
 do if ((ch = *iter++) == '/') result = iter;
 while (ch);
 if __unlikely(!result) return path; // Path doesn't contain '/'
 if (*result) return result; // Last character isn't a '/'
 iter = result;
 while (iter != path && iter[-1] == '/') --iter;
 if (iter == path) return result-1; // Only '/'-characters
 *iter = '\0'; // Trim all ending '/'-characters
 while (iter != path && iter[-1] != '/') --iter; // Scan until the previous '/'
 return iter; // Returns string after previous '/'
}

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

__DECL_END

#endif /* !__STRING_C__ */

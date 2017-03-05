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
#ifndef __ATOI_C__
#define __ATOI_C__ 1

#include <kos/compiler.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <stddef.h>

__DECL_BEGIN

#define CONSUMENUMSYS(s) \
 (*(s) == '0' ? (*++(s) == 'x' ? (++(s),16) : *(s) == 'b' ? (++(s),2) : 8) : 10)
#define CONSUMENUMSYSN(s,end) \
 ((*(s) == '0' && (++(s) != (end))) ?\
 ((*(s) == 'x' && (++(s) != (end))) ? 16 :\
  (*(s) == 'b' && (++(s) != (end))) ? 2 : 8) : 10)
#define CH2INT(c) \
 ((c) >= 'a' ? 10+((c)-'a') : \
  (c) >= 'A' ? 10+((c)-'A') : \
  (c) >= '0' ?    ((c)-'0') : (-1))


__public float strtof(char const *string, char **endptr) {
 (void)string,(void)endptr;
 return -1.0f; /* TODO. */
}
__public double strtod(char const *string, char **endptr) {
 (void)string,(void)endptr;
 return -1.0f; /* TODO. */
}
__public long double strtold(char const *string, char **endptr) {
 (void)string,(void)endptr;
 return -1.0f; // TODO
}


__public __u32 _strntou32(char const *string, size_t max_chars,
                          char **endptr, int base) {
 __u32 result = 0,newresult; int part; char ch;
 char const *send = string+max_chars;
 if __unlikely(!string || !max_chars) goto end;
 if (!base) base = CONSUMENUMSYSN(string,send);
 while (string != send &&
       (ch = *string,part = CH2INT(ch),
        part >= 0 && part < base)) {
  newresult = (result*base)+part;
  if __unlikely(newresult < result) { __set_errno(ERANGE); return (__u32)-1; }
  result = newresult,++string;
 }
end:
 if (endptr) *endptr = (char *)string;
 return result;
}
__public __u64 _strntou64(char const *string, size_t max_chars,
                          char **endptr, int base) {
 __u64 result = 0,newresult; int part; char ch;
 char const *send = string+max_chars;
 if __unlikely(!string || !max_chars) goto end;
 if (!base) base = CONSUMENUMSYSN(string,send);
 while (string != send &&
       (ch = *string,part = CH2INT(ch),
        part >= 0 && part < base)) {
  newresult = (result*base)+part;
  if __unlikely(newresult < result) { __set_errno(ERANGE); return (__u64)-1; }
  result = newresult,++string;
 }
end:
 if (endptr) *endptr = (char *)string;
 return result;
}

__public __s32 _strntos32(char const *string, size_t max_chars,
                          char **endptr, int base) {
 int sign = (string && max_chars && *string == '-') ? (++string,--max_chars,1) : 0;
 __u32 ures = _strntou32(string,max_chars,endptr,base);
 __s32 result;
 if (ures > (__u32)INT32_MAX) {
  __set_errno(ERANGE);
  result = sign ? INT32_MIN : INT32_MAX;
 } else {
  result = sign ? -(__s32)ures : (__s32)ures;
 }
 return result;
}
__public __s64 _strntos64(char const *string, size_t max_chars,
                          char **endptr, int base) {
 int sign = (string && max_chars && *string == '-') ? (++string,--max_chars,1) : 0;
 __u64 ures = _strntou64(string,max_chars,endptr,base);
 __s64 result;
 if (ures > (__u64)INT64_MAX) {
  __set_errno(ERANGE);
  result = sign ? INT64_MIN : INT64_MAX;
 } else {
  result = sign ? -(__s64)ures : (__s64)ures;
 }
 return result;
}


__public __u32 _strtou32(char const *string, char **endptr, int base) {
 __u32 result = 0,newresult; int part; char ch;
 if __unlikely(!string) goto end;
 if (!base) base = CONSUMENUMSYS(string);
 while (ch = *string,part = CH2INT(ch),
        part >= 0 && part < base) {
  newresult = (result*base)+part;
  if __unlikely(newresult < result) { __set_errno(ERANGE); return (__u32)-1; }
  result = newresult,++string;
 }
end:
 if (endptr) *endptr = (char *)string;
 return result;
}
__public __u64 _strtou64(char const *string, char **endptr, int base) {
 __u64 result = 0,newresult; int part; char ch;
 if __unlikely(!string) goto end;
 if (!base) base = CONSUMENUMSYS(string);
 while (ch = *string,part = CH2INT(ch),
        part >= 0 && part < base) {
  newresult = (result*base)+part;
  if __unlikely(newresult < result) { __set_errno(ERANGE); return (__u64)-1; }
  result = newresult,++string;
 }
end:
 if (endptr) *endptr = (char *)string;
 return result;
}

__public __s32 _strtos32(char const *string, char **endptr, int base) {
 int sign = (string && *string == '-') ? (++string,1) : 0;
 __u32 ures = _strtou32(string,endptr,base);
 __s32 result;
 if (ures > (__u32)INT32_MAX) {
  __set_errno(ERANGE);
  result = sign ? INT32_MIN : INT32_MAX;
 } else {
  result = sign ? -(__s32)ures : (__s32)ures;
 }
 return result;
}
__public __s64 _strtos64(char const *string, char **endptr, int base) {
 int sign = (string && *string == '-') ? (++string,1) : 0;
 __u64 ures = _strtou64(string,endptr,base);
 __s64 result;
 if (ures > (__u64)INT64_MAX) {
  __set_errno(ERANGE);
  result = sign ? INT64_MIN : INT64_MAX;
 } else {
  result = sign ? -(__s64)ures : (__s64)ures;
 }
 return result;
}

__public int atoi(char const *string) {
#if __SIZEOF_INT__ == 4
 return (int)_strtos32(string,NULL,0);
#elif __SIZEOF_INT__ == 8
 return (int)_strtos64(string,NULL,0);
#else
#error "Unsupported sizeof(int)"
#endif
}
__public long atol(char const *string) {
#if __SIZEOF_LONG__ == 4
 return (long)_strtos32(string,NULL,0);
#elif __SIZEOF_LONG__ == 8
 return (long)_strtos64(string,NULL,0);
#else
#error "Unsupported sizeof(long)"
#endif
}

#ifndef __NO_longlong
__public long long atoll(char const *string) {
#if __SIZEOF_LONG_LONG__ == 4
 return (long long)_strtos32(string,NULL,0);
#elif __SIZEOF_LONG_LONG__ == 8
 return (long long)_strtos64(string,NULL,0);
#else
#error "Unsupported sizeof(long long)"
#endif
}
#endif /* !__NO_longlong */


#if __SIZEOF_LONG__ == 4
__public __compiler_ALIAS(strtol,_strtos32);
__public __compiler_ALIAS(strtoul,_strtou32);
#elif __SIZEOF_LONG__ == 8
__public __compiler_ALIAS(strtol,_strtos64);
__public __compiler_ALIAS(strtoul,_strtou64);
#else
#error "Unsupported sizeof(long)"
#endif

#ifndef __NO_longlong
#if __SIZEOF_LONG_LONG__ == 4
__public __compiler_ALIAS(strtoll,_strtos32);
__public __compiler_ALIAS(strtoull,_strtou32);
#elif __SIZEOF_LONG_LONG__ == 8
__public __compiler_ALIAS(strtoll,_strtos64);
__public __compiler_ALIAS(strtoull,_strtou64);
#else
#error "Unsupported sizeof(long long)"
#endif
#endif /* !__NO_longlong */


__DECL_END

#endif /* !__ATOI_C__ */

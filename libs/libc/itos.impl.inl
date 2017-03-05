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
#ifdef __INTELLISENSE__
#define BITS 32
#endif

#ifndef BITS
#error "Must #define BITS before #including this file"
#endif

#include <assert.h>
#include <kos/compiler.h>
#include <kos/kernel/types.h>
#include <itos.h>
#include <stddef.h>
#ifdef __KERNEL__
#include <kos/kernel/pageframe.h>
#include <kos/kernel/debug.h>
#endif

__DECL_BEGIN

#ifndef __libc_digit_chars_defined
#define __libc_digit_chars_defined 1
#define MAX_NUMSYS 16
static char const *const digit_chars = "0123456789abcdef";
#endif

#define __s  __PP_CAT_2(__s,BITS)
#define __u  __PP_CAT_2(__u,BITS)

__public size_t __PP_CAT_3(_itos,BITS,_ns)(char *__restrict buf,
                                           size_t bufsize, __s i,
                                           int numsys) {
 __s used_i; size_t result;
#ifdef __KERNEL__
 /*kassertmem(buf,bufsize);*/
#else
 assert(!bufsize || buf);
#endif
 if (i < 0 && numsys <= MAX_NUMSYS) {
  if (bufsize) { *buf++ = '-'; --bufsize; }
  used_i = (i < 0) ? -i : i;
  result = 1;
 } else used_i = i,result = 0;
 return result+__PP_CAT_3(_utos,BITS,_ns)(buf,bufsize,(__u)used_i,numsys);
}

size_t __PP_CAT_3(_utos,BITS,_ns)(char *__restrict buf, size_t bufsize, __u i, int numsys) {
 __u used_i; char *buf_dst,*buf_end; size_t result;
#ifdef __KERNEL__
 /*kassertmem(buf,bufsize);*/
#else
 assert(!bufsize || buf);
#endif
 if (numsys > MAX_NUMSYS) return 0;
 buf_end = (buf_dst = buf)+bufsize;
 used_i = i;
 do ++buf_dst; while ((used_i /= numsys) != 0);
 result = (size_t)(buf_dst-buf);
 if (buf_dst < buf_end) *buf_dst = '\0';
 used_i = i; do {
  --buf_dst;
  if (buf_dst < buf_end)
   *buf_dst = digit_chars[used_i%numsys];
 } while ((used_i /= numsys) != 0);
 return result;
}

__DECL_END

#undef __u
#undef __s
#undef BITS

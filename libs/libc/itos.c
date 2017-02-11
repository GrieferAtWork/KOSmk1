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
#ifndef __ITOS_C__
#define __ITOS_C__ 1

#ifndef __INTELLISENSE__
#define BITS 32
#include "itos.impl.inl"
#define BITS 64
#include "itos.impl.inl"
#endif

#include <kos/compiler.h>
#include <itos.h>
#include <math.h>
#include <string.h>
#include <kos/types.h>

__DECL_BEGIN

__public __size_t _dtos(char *__restrict buf, __size_t bufsize, double d,
                        unsigned int prec, unsigned int prec2, int g) {
 union { double d; __u64 l; __s64 sl; } u = {d};
 __s32 e10,e = ((u.l>>52)&((1<<11)-1))-1023;
 unsigned int i;
 double tmp,backup = d;
 char *oldbuf = buf;
 //if ((i = isinf(d))) return strncpy(buf,i>0 ? "inf" : "-inf",bufsize);
 if (isnan(d)) { strncpy(buf,"nan",bufsize); return 3; }
 e10 = 1+(__s32)(e*0.30102999566398119802); /* log10(2) */
 if (d == 0.0) {
  prec2 = prec2 == 0 ? 1 : prec2+2;
  prec2 = prec2 > bufsize ? 8 : prec2;
  i = 0;
  if (prec2 && (__s64)u.l < 0) buf[0] = '-',++i;
  for (; i<prec2; ++i) buf[i] = '0';
  buf[buf[0]=='0' ? 1 : 2] = '.'; buf[i] = 0;
  return i;
 }
 if (d < 0.0) { d = -d; *buf = '-'; --bufsize; ++buf; }
 tmp = 0.5;
 for (i = 0; i < prec2; ++i) tmp *= 0.1;
 d += tmp;
 if (d < 1.0) *buf++ = '0',--bufsize;

 if (e10>0) {
  int first = 1;	/* are we about to write the first digit? */
  tmp = 10.0;
  i = e10;
  while (i > 10) tmp = tmp*1e10,i -= 10;
  while (i > 1) tmp = tmp*10,--i;
  while (tmp > 0.9) {
   char digit;
   double fraction = d/tmp;
   digit = (int)(fraction);		/* floor() */
   if (!first || digit) {
    first = 0;
    *buf = digit+'0'; ++buf;
    if (!bufsize) {
     /* use scientific notation */
     int len = _dtos(oldbuf,bufsize,backup/tmp,prec,prec2,0);
     int initial = 1;
     if (len == 0) return 0;
     bufsize -= len; buf += len;
     if (bufsize>0) *buf = 'e',++buf;
     --bufsize;
     for (len = 1000; len>0; len /= 10) {
      if (e10>=len || !initial) {
       if (bufsize > 0) {
        *buf = (e10/len)+'0';
        ++buf;
       }
       --bufsize;
       initial = 0;
       e10 = e10%len;
      }
     }
     if (bufsize > 0) goto fini;
     return 0;
    }
    d -= digit*tmp;
    --bufsize;
   }
   tmp /= 10.0;
  }
 } else {
  tmp = 0.1;
 }
 if (buf==oldbuf) {
  if (!bufsize) return 0;
  --bufsize;
  *buf = '0'; ++buf;
 }
 if (prec2 || prec > (unsigned int)(buf-oldbuf)+1) {	/* more digits wanted */
  if (!bufsize) return 0;
  --bufsize;
  *buf = '.'; ++buf;
  if (g) {
   if (prec2) prec = prec2;
   prec -= buf-oldbuf-1;
  } else {
   prec -= buf-oldbuf-1;
   if (prec2) prec = prec2;
  }
  if (prec>bufsize) return 0;
  while (prec>0) {
   char digit;
   double fraction = d/tmp;
   digit = (int)(fraction);		/* floor() */
   *buf = digit+'0'; ++buf;
   d -= digit*tmp;
   tmp /= 10.0;
   --prec;
  }
 }
fini:
 *buf = 0;
 return buf-oldbuf;
}

__DECL_END

#endif /* !__ITOS_C__ */

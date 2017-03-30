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

#include "helper.h"
#include <stddef.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <kos/arch/string.h>

/* Control string functions (known to always be correct for all arguments) */
static void *ok_memset(void *dst, int byte, size_t bytes) {
 byte_t *iter = (byte_t *)dst;
 while (bytes--) *iter++ = (byte_t)byte;
 return dst;
}
static int ok_memcmp(void const *a, void const *b, size_t bytes) {
 signed char *ai = (signed char *)a;
 signed char *bi = (signed char *)b;
 while (bytes--) {
  int temp = (*ai++)-(*bi++);
  if (temp) return temp;
 }
 return 0;
}


__forcelocal void test_strings(size_t s) {
 void *__restrict p,*__restrict x; int t,t2;
 p = malloc(s); assert(p);
 x = malloc(s); assert(x);
 ok_memset(p,0xaa,s);
 //outf("x = %p; p = %p; s = %Iu\n",x,p,s);
 arch_memcpy(x,p,s);
 t = ok_memcmp(p,x,s);   assertf(!t,"%Iu %d %.?q %.?q",s,t,s,p,s,x);
 t = arch_memcmp(p,x,s); assertf(!t,"%Iu %d",s,t);
 ok_memset(x,0x00,s),t = ok_memcmp(p,x,s);
 t2 = arch_memcmp(p,x,s);
 assertf(t == t2,"%Iu %d != %d",s,t,t2);
 ok_memset(x,0xab,s);
 arch_memset(x,0xaa,s);
 t = arch_memcmp(p,x,s);
 assertf(!t,"%Iu %d %.?q %.?q",s,t,s,x,s-2,(char *)x+2);
 free(x);
 free(p);
}


TEST(strings) {
 /* Test arch-specific string operations for various compile-time known sizes. */
#define T(s) test_strings(s)
 T(0x00),T(0x01),T(0x02),T(0x03),T(0x04),T(0x05),T(0x06),T(0x07);
 T(0x08),T(0x09),T(0x0A),T(0x0B),T(0x0C),T(0x0D),T(0x0E),T(0x0F);
 T(0x10),T(0x11),T(0x12),T(0x13),T(0x14),T(0x15),T(0x16),T(0x17);
 T(0x18),T(0x19),T(0x1A),T(0x1B),T(0x1C),T(0x1D),T(0x1E),T(0x1F);
 T(0x20),T(0x21),T(0x22),T(0x23),T(0x24),T(0x25),T(0x26),T(0x27);
 T(0x28),T(0x29),T(0x2A),T(0x2B),T(0x2C),T(0x2D),T(0x2E),T(0x2F);
#undef T
}

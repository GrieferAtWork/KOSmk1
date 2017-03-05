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
#include <kos/asm.h>
#include <kos/config.h>

/* extern int setjmp(jmp_buf buf); */
GLOBAL_FUNCTION(setjmp):
	movI (PS)(%esp), %eax
	movI %ecx,    (0*PS)(%eax)
	movI %edx,    (1*PS)(%eax)
	movI %ebx,    (2*PS)(%eax)
	movI %esp,    (3*PS)(%eax)
	movI %ebp,    (4*PS)(%eax)
	movI %esi,    (5*PS)(%eax)
	movI %edi,    (6*PS)(%eax)
	movI 0(%esp), %ebx
	movI %ebx,    (7*PS)(%eax)
	mov32 $0, %eax
	ret
SYMBOLEND(setjmp)

/* extern void longjmp(jmp_buf buf, int sig); */
GLOBAL_FUNCTION(longjmp):
	movI (1*PS)(%esp), %ecx
	movI (2*PS)(%esp), %eax

	movI (3*PS)(%ecx), %esp
	movI (7*PS)(%ecx), %ebx
	movI %ebx, 0(%esp)

	movI (6*PS)(%ecx), %edi
	movI (5*PS)(%ecx), %esi
	movI (4*PS)(%ecx), %ebp
	movI (2*PS)(%ecx), %ebx
	movI (1*PS)(%ecx), %edx
	movI (0*PS)(%ecx), %ecx
	ret
SYMBOLEND(longjmp)



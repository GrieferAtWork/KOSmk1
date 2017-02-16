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
#include <kos/kernel/gdt.h>


/* void gdt_flush(struct kidtpointer *p); */
GLOBAL_FUNCTION(gdt_flush):
	STACKBEGIN
	/* Load GDT */
	movI STACKARG(0), %eax
	lgdt (%eax)

	mov16 $KSEG_KERNEL_DATA, %ax
	mov16 %ax, %ds
	mov16 %ax, %es
	mov16 %ax, %fs
	mov16 %ax, %gs
	mov16 %ax, %ss

	ljmp $KSEG_KERNEL_CODE, $.flush
.flush:
	STACKEND
	ret
SYMBOLEND(gdt_flush)

/* void tss_flush(ksegid_t sel); */
GLOBAL_FUNCTION(tss_flush):
	STACKBEGIN
	mov16 STACKARG(0), %ax
	ltr   %ax
	STACKEND
	ret
SYMBOLEND(tss_flush)

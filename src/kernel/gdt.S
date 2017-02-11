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


GLOBAL_FUNCTION(gdt_flush):
	STACKBEGIN
	/* Load GDT */
	movI STACKARG(0), %eax
	lgdt (%eax)

#if 1
	mov $KSEGMENT_KERNEL_DATA, %ax
	mov %ax,   %ds
	mov %ax,   %es
	mov %ax,   %fs
	mov %ax,   %gs
	mov %ax,   %ss

	ljmp $KSEGMENT_KERNEL_CODE, $.flush
.flush:
#else
	jmp $KSEGMENT_KERNEL_CODE, $.reload_cs
.reload_cs:
	mov $KSEGMENT_KERNEL_DATA, %ax
	mov %ax,   %ds
	mov %ax,   %es
	mov %ax,   %fs
	mov %ax,   %gs
	mov %ax,   %ss
#endif

	STACKEND
	ret
SYMBOLEND(gdt_flush)


GLOBAL_FUNCTION(tss_flush):
	STACKBEGIN
	mov STACKARG(0), %ax
	ltr %ax
	STACKEND
	ret
SYMBOLEND(tss_flush)

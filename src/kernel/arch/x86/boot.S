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
#include <kos/symbol.h>
#include <kos/kernel/task.h>

.set ALIGN,    1<<0
.set MEMINFO,  1<<1
.set VIDMOD,   1<<2
.set FLAGS,    ALIGN|MEMINFO
.set MAGIC,    0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)

SECTION(.multiboot)
	.align 4
	.long MAGIC
	.long FLAGS
	.long CHECKSUM

SECTION(.bss)
	.align 16
GLOBAL_SYMBOL(stack_bottom):
	.skip KTASK_STACK_SIZE_DEF
GLOBAL_SYMBOL(stack_top):

SECTION(.text)
GLOBAL_FUNCTION(_start):
	/* Remember the grub magic. */
	mov %eax, __grub_magic
	mov %ebx, __grub_mbt

	/* Setup the stack. */
	mov $stack_top, %esp
	call _S(kernel_main)

	/* Jump into the shutdown code */
	jmp _S(hw_shutdown)
SYMBOLEND(_start)

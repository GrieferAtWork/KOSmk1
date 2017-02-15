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
#define __KOS_KERNEL_ARCH_X86_REALMODE_S__ 1

#include <kos/asm.h>
#include <kos/kernel/gdt.h>
#include <kos/kernel/arch/x86/realmode.h>

#define MODE16_SYM(name) (((name)-mode16_reloc_begin)+MODE16_RELOC_BASE)

.global mode16_reloc_begin
.global mode16_reloc_end

/* For those interested in what is actually happening here,
 * I suggest reading the list of bullet points here:
 * http://wiki.osdev.org/Real_Mode#Switching_from_Protected_Mode_to_Real_Mode
 */


.code16
mode16_reloc_begin:
	mov $KSEGMENT_KERNEL_DATA_16, %eax
	mov %eax, %ds
	mov %eax, %es
	mov %eax, %fs
	mov %eax, %gs
	mov %eax, %ss
	sidt (MODE16_SYM(mode16_oldidt)) /* save old 32-bit idt pointer */
	lidt (MODE16_SYM(mode16_newidt)) /* load new 16-bit idt pointer */
	
	mov32 %cr0, %eax
	mov32 %eax, MODE16_SYM(mode16_oldcr0)
	and32 $0x7FFFFFFe, %eax /* Disable paging bit & enable 16-bit pmode (NOTE: Paging should already be disabled, though...). */
	mov32 %eax, %cr0
	jmp16 $0x0000, $MODE16_SYM(mode16_startup)
mode16_startup:
	mov $0, %eax
	mov %eax, %ds
	mov %eax, %es
	mov %eax, %fs
	mov %eax, %gs
	mov %eax, %ss
	mov16 $MODE16_SYM(mode16_regs), %sp
	pop16 %di
	pop16 %si
	pop16 %bp
	pop16 %ax
	pop32 %ebx
	pop32 %edx
	pop32 %ecx
	mov16 %ax, %sp
	mov32 (MODE16_SYM(mode16_regs)+REALMODE_REGS_SIZE-4), %eax

//	hlt

	sti
	/* We should be in 16-bit real mode at this point!
	 * - Interrupts use custom handlers.
	 * - Paging is disabled.
	 * - All segment registers are ZERO. */

	.byte 0xCD /* opcode of INT instruction with immediate byte. */
move16_irqnum:
	.byte 0x00 /* Interrupt number. */
	cli

	/* Store the new register values for 32-bit code to return. */
	movw %sp, (MODE16_SYM(mode16_regs)+3*2)
	movw $(MODE16_SYM(mode16_regs)+REALMODE_REGS_SIZE), %sp
	push32 %eax
	push32 %ecx
	push32 %edx
	push32 %ebx
	sub16 $2, %sp /* SP itself (set before) */
	push16 %bp
	push16 %si
	push16 %di

	mov32 MODE16_SYM(mode16_oldcr0), %eax
	mov32 %eax, %cr0

	lidt (MODE16_SYM(mode16_oldidt)) /* save old 32-bit idt pointer */
	jmp32 $KSEGMENT_KERNEL_CODE, $mode16_leave
mode16_newidt:
	.word 0x03ff     /* 256 entries, 4b each = 1K */
	.long 0x00000000 /* Real Mode IVT @ 0x0000 */
mode16_oldidt:
	.word 0x0000
	.long 0x00000000
mode16_oldcr0:
	.long 0x00000000
mode16_reloc_end:
mode16_regs:
	/* 'REALMODE_REGS_SIZE' bytes of memory here! */
.code32
mode16_oldstack:
	.long 0x00000000










#define MEMCPY(dst,src,bytes) \
	src, %esi;\
	dst, %edi;\
	bytes, %ecx;\
	cld;\
	rep  movsb;

/* extern void realmode_interrupt(__u8 intnum, struct realmode_regs *regs); */
GLOBAL_FUNCTION(__realmode_interrupt):
//	cli
	movI %cr0, %eax
	pushI %eax
	andI $0x7FFFFFFF, %eax
	movI %eax, %cr0

	/* CR3 must be ZERO before we can switch to 16-bit mode. */
	movI $0, %eax
	movI %eax, %cr3

#define ARG(i) (PS*((i)+2))(%esp)
	/* Copy the given registers as they will appear when we fire the interrupt. */
	MEMCPY(
		movI $MODE16_SYM(mode16_regs),
		movI ARG(1),
		movI $REALMODE_REGS_SIZE)

	/* Set the corrent IRQ number. */
	mov8 ARG(0), %al
	mov8 %al, MODE16_SYM(move16_irqnum)
#undef ARG

	/* Safe old 32-bit registers on the stack. */
	pushfI
	pushI %esi
	pushI %ebp

	/* Save the old stack. */
	movI %esp, mode16_oldstack

	// Jump into 16-bit code
	jmp16 $KSEGMENT_KERNEL_CODE_16, $MODE16_SYM(mode16_reloc_begin)
mode16_leave:
	mov $KSEGMENT_KERNEL_DATA, %eax
	mov %eax, %ds
	mov %eax, %es
	mov %eax, %fs
	mov %eax, %gs
	mov %eax, %ss
	
	movI mode16_oldstack, %esp

	/* Restore old 32-bit registers on the stack. */
	popI %ebp
	popI %esi
	popfI

	leaI __kpagedir_kernel, %eax
	movI %eax, %cr3

	popI %eax
	mov %eax, %cr0
//	sti

	/* It is possible for an unhandled interrupt to have occurred
	 * while we were in real mode. While we can't know for sure,
	 * or even figure out what it was (if it even existed), we
	 * must inform the CPU that we are ready to handle more, as
	 * not to break scheduling or other interrupt-driven devices. */
	mov $0xA0, %al
	outb %al, $0x20
	mov $0x20, %al
	outb %al, $0x20

	/* Copy output register values from after the interrupt. */
	MEMCPY(
		movI (PS*2)(%esp),
		movI $MODE16_SYM(mode16_regs),
		movI $REALMODE_REGS_SIZE)

	ret
SYMBOLEND(__realmode_interrupt)




/* extern void kernel_initialize_realmode(void); */
GLOBAL_FUNCTION(kernel_initialize_realmode):
	/* Relocate mode-16 code... */
	MEMCPY(
		movI $MODE16_RELOC_BASE,
		leaI mode16_reloc_begin,
		movI $MODE16_RELOC_SIZE)
	ret
SYMBOLEND(kernel_initialize_realmode)




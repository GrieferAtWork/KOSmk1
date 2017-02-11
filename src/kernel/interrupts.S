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
#include <kos/kernel/debug.h>
#include <kos/kernel/gdt.h>
#include <kos/kernel/interrupts.h>
#include <kos/kernel/paging.h>
#include <kos/kernel/task.h>
#include <kos/symbol.h>

.extern x86_interrupt_handler

int_handler_prep:
	PUSH_REGISTERS
	SET_KERNEL_SEGMENTS

	movI %esp, %ecx
	movI %esp, %edx

	DISABLE_PAGING
	/* ESP is still part of the user's address space. - Let's change that! */
	/* Translate address in %ECX */
	TRANSLATE_VADDR %ecx, %ebx, %eax
	movI %ebx, %esp

	movI  %cr3, %eax
	pushI %eax
	pushI %edx // User-address for registers
	movI %esp, %ebx

	lea32 __kpagedir_kernel, %eax
	mov32 %eax, %cr3  /* Set the kernel's page directory. */
	ENABLE_PAGING

#ifdef __DEBUG__
	/* Create a fake stackframe entry */
	/* NOTE: We use the virtual return address here */
	pushI (KIRQ_REGISTERS_OFFSETOF_REGS_EIP)(%ebx)
	pushI %ebp
	movI %esp, %ebp
#endif
	/* The stack is saved and we're ready for some kernel-level fun! */
	pushI %ebx
	//__KTASK_UPDATE_ARCH_CPU_REGISTER
	call _S(x86_interrupt_handler)
	popI %ebx

	movI (PS*0)(%ebx), %esp
	movI (PS*1)(%ebx), %eax
	movI %eax, %cr3

	POP_REGISTERS_NOEAX

	/* Signal interrupt handling completion */
	mov $0xA0, %al
	outb %al, $0x20
	mov $0x20, %al
	outb %al, $0x20

	pop32 %eax

	/* Adjust the stack to skip 'intno' and 'ecode' */
	addI $8, %esp

	iret
SYMBOLEND(int_handler_prep)


// Used for interrupt handlers where the CPU doesn't push 'ecode' for us
.macro isr num
	GLOBAL_FUNCTION(isr\num):
		pushl $0    // Push 'ecode'
		pushl $\num // Push 'intno'
		jmp int_handler_prep
	SYMBOLEND(isr\num)
.endm

/* Used for interrupt handlers where the CPU pushes 'ecode' for us */
.macro isr_code num
	GLOBAL_FUNCTION(isr\num):
		cli
		pushl $\num // Push 'intno'
		jmp int_handler_prep
.endm

/* Used for actual interrupts (Not exception handlers.) */
.macro irq num
	GLOBAL_FUNCTION(irq\num):
		pushl $0
		pushl $(\num +32)
		jmp int_handler_prep
.endm

/* Enumerate handlers for all known exceptions
 * >> http://wiki.osdev.org/Exceptions
 */
isr 0
isr 1
isr 2
isr 3
isr 4
isr 5
isr 6
isr 7
isr_code 8
isr 9
isr_code 10
isr_code 11
isr_code 12
isr_code 13
isr_code 14
isr 15
isr 16
isr_code 17
isr 18
isr 19
isr 20
isr 21
isr 22
isr 23
isr 24
isr 25
isr 26
isr 27
isr 28
isr 29
isr_code 30
isr 31

/* Enumerate handlers for all known IRQ interrupts
 * >> http://wiki.osdev.org/Interrupts
 */
irq 0
irq 1
irq 2
irq 3
irq 4
irq 5
irq 6
irq 7
irq 8
irq 9
irq 10
irq 11
irq 12
irq 13
irq 14

/*[[[deemon
#include <util>
for (local x :util::range(15,225))
	print "irq",x;
]]]*/
irq 15
irq 16
irq 17
irq 18
irq 19
irq 20
irq 21
irq 22
irq 23
irq 24
irq 25
irq 26
irq 27
irq 28
irq 29
irq 30
irq 31
irq 32
irq 33
irq 34
irq 35
irq 36
irq 37
irq 38
irq 39
irq 40
irq 41
irq 42
irq 43
irq 44
irq 45
irq 46
irq 47
irq 48
irq 49
irq 50
irq 51
irq 52
irq 53
irq 54
irq 55
irq 56
irq 57
irq 58
irq 59
irq 60
irq 61
irq 62
irq 63
irq 64
irq 65
irq 66
irq 67
irq 68
irq 69
irq 70
irq 71
irq 72
irq 73
irq 74
irq 75
irq 76
irq 77
irq 78
irq 79
irq 80
irq 81
irq 82
irq 83
irq 84
irq 85
irq 86
irq 87
irq 88
irq 89
irq 90
irq 91
irq 92
irq 93
irq 94
irq 95
irq 96
irq 97
irq 98
irq 99
irq 100
irq 101
irq 102
irq 103
irq 104
irq 105
irq 106
irq 107
irq 108
irq 109
irq 110
irq 111
irq 112
irq 113
irq 114
irq 115
irq 116
irq 117
irq 118
irq 119
irq 120
irq 121
irq 122
irq 123
irq 124
irq 125
irq 126
irq 127
irq 128
irq 129
irq 130
irq 131
irq 132
irq 133
irq 134
irq 135
irq 136
irq 137
irq 138
irq 139
irq 140
irq 141
irq 142
irq 143
irq 144
irq 145
irq 146
irq 147
irq 148
irq 149
irq 150
irq 151
irq 152
irq 153
irq 154
irq 155
irq 156
irq 157
irq 158
irq 159
irq 160
irq 161
irq 162
irq 163
irq 164
irq 165
irq 166
irq 167
irq 168
irq 169
irq 170
irq 171
irq 172
irq 173
irq 174
irq 175
irq 176
irq 177
irq 178
irq 179
irq 180
irq 181
irq 182
irq 183
irq 184
irq 185
irq 186
irq 187
irq 188
irq 189
irq 190
irq 191
irq 192
irq 193
irq 194
irq 195
irq 196
irq 197
irq 198
irq 199
irq 200
irq 201
irq 202
irq 203
irq 204
irq 205
irq 206
irq 207
irq 208
irq 209
irq 210
irq 211
irq 212
irq 213
irq 214
irq 215
irq 216
irq 217
irq 218
irq 219
irq 220
irq 221
irq 222
irq 223
irq 224
//[[[end]]]

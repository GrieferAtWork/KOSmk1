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
#ifndef __KOS_ARCH_X86_EFLAGS_H__
#define __KOS_ARCH_X86_EFLAGS_H__ 1

#include <kos/compiler.h>
#include <kos/types.h>

__DECL_BEGIN

#define KARCH_X86_EFLAGS_CF           0x00000001 /*< [bit(0)]     Carry Flag Status. */
#define KARCH_X86_EFLAGS_PF           0x00000004 /*< [bit(2)]     Parity Flag Status. */
#define KARCH_X86_EFLAGS_AF           0x00000010 /*< [bit(4)]     Auxiliary Carry Flag Status. */
#define KARCH_X86_EFLAGS_ZF           0x00000040 /*< [bit(6)]     Zero Flag Status. */
#define KARCH_X86_EFLAGS_SF           0x00000080 /*< [bit(7)]     Sign Flag Status. */
#define KARCH_X86_EFLAGS_TF           0x00000100 /*< [bit(8)]     Trap Flag (System). */
#define KARCH_X86_EFLAGS_IF           0x00000200 /*< [bit(9)]     Interrupt Enable Flag (System). */
#define KARCH_X86_EFLAGS_DF           0x00000400 /*< [bit(10)]    Direction Flag (Control). */
#define KARCH_X86_EFLAGS_OF           0x00000800 /*< [bit(11)]    Overflow Flag Status. */
#define KARCH_X86_EFLAGS_IOPL(n) (((n)&3) << 12) /*< [bit(12,13)] I/O Privilege Level (System). */
#define KARCH_X86_EFLAGS_NT           0x00004000 /*< [bit(14)]    Nested Task Flag (System). */
#define KARCH_X86_EFLAGS_RF           0x00010000 /*< [bit(16)]    Resume Flag (System). */
#define KARCH_X86_EFLAGS_VM           0x00020000 /*< [bit(17)]    Virtual 8086 Mode (System). */
#define KARCH_X86_EFLAGS_AC           0x00040000 /*< [bit(18)]    Alignment Check (System). */
#define KARCH_X86_EFLAGS_VIF          0x00080000 /*< [bit(19)]    Virtual Interrupt Flag (System). */
#define KARCH_X86_EFLAGS_VIP          0x00100000 /*< [bit(20)]    Virtual Interrupt Pending (System). */
#define KARCH_X86_EFLAGS_ID           0x00200000 /*<	[bit(21)]    ID Flag (System). */

#ifdef __INTELLISENSE__
extern __u32 karch_x86_eflags(void);
#else
#define karch_x86_eflags() \
 __xblock({ __u32 __eflags;\
            __asm__("pushfl\npopl %0" : "=g" (__eflags));\
            __xreturn __eflags;\
 })
#endif

__DECL_END

#endif /* !__KOS_ARCH_X86_EFLAGS_H__ */

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
#ifndef __KOS_ARCH_X86_VGA_H__
#define __KOS_ARCH_X86_VGA_H__ 1

#include <kos/compiler.h>
#include <kos/kernel/types.h>

__DECL_BEGIN

#define VGA_ADDR ((void *)0xB8000)

#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define VGA_TABSIZE 4

enum vga_color {
	VGA_COLOR_BLACK         = 0,
	VGA_COLOR_BLUE          = 1,
	VGA_COLOR_GREEN         = 2,
	VGA_COLOR_CYAN          = 3,
	VGA_COLOR_RED           = 4,
	VGA_COLOR_MAGENTA       = 5,
	VGA_COLOR_BROWN         = 6,
	VGA_COLOR_LIGHT_GREY    = 7,
	VGA_COLOR_DARK_GREY     = 8,
	VGA_COLOR_LIGHT_BLUE    = 9,
	VGA_COLOR_LIGHT_GREEN   = 10,
	VGA_COLOR_LIGHT_CYAN    = 11,
	VGA_COLOR_LIGHT_RED     = 12,
	VGA_COLOR_LIGHT_MAGENTA = 13,
	VGA_COLOR_LIGHT_BROWN   = 14,
	VGA_COLOR_WHITE         = 15,
	VGA_COLORS              = 16,
};

#define vga_entry_color(fg,bg) ((fg)|(bg) << 4)
#define vga_entry(uc,color) ((__u16)(uc)|(__u16)(color) << 8)
#define VGA_DEFAULT_COLOR   vga_entry_color(VGA_COLOR_LIGHT_GREY,VGA_COLOR_BLACK)

__DECL_END

#endif /* !__KOS_ARCH_X86_VGA_H__ */

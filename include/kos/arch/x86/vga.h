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
/*
0x0 	0x0 	 0,0,0 	    0,0,0 	     #000000 	black
0x1 	0x1 	 0,0,42 	   0,0,170 	   #0000aa 	blue
0x2 	0x2 	 00,42,00 	 0,170,0 	   #00aa00 	green
0x3 	0x3 	 00,42,42 	 0,170,170 	 #00aaaa 	cyan
0x4 	0x4 	 42,00,00 	 170,0,0 	   #aa0000 	red
0x5 	0x5 	 42,00,42 	 170,0,170 	 #aa00aa 	magenta
0x6 	0x14 	42,21,00 	170,85,0 	  #aa5500 	brown
0x7 	0x7 	 42,42,42 	 170,170,170 #aaaaaa 	gray
0x8 	0x38 	21,21,21 	85,85,85 	  #555555 	dark gray
0x9 	0x39 	21,21,63 	85,85,255 	 #5555ff 	bright blue
0xA 	0x3A 	21,63,21 	85,255,85 	 #55ff55 	bright green
0xB 	0x3B 	21,63,63 	85,255,255 	#55ffff 	bright cyan
0xC 	0x3C 	63,21,21 	255,85,85 	 #ff5555 	bright red
0xD 	0X3D 	63,21,63 	255,85,255 	#ff55ff 	bright magenta
0xE 	0x3E 	63,63,21 	255,255,85 	#ffff55 	Yellow
0xF 	0x3F 	63,63,63 	255,255,255 #ffffff 	white
*/

#define vga_entry_color(fg,bg) ((fg)|(bg) << 4)
#define vga_entry(uc,color) ((__u16)(uc)|(__u16)(color) << 8)
#define VGA_DEFAULT_COLOR   vga_entry_color(VGA_COLOR_LIGHT_GREY,VGA_COLOR_BLACK)

__DECL_END

#endif /* !__KOS_ARCH_X86_VGA_H__ */

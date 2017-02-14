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
#ifndef __KOS_ARCH_X86_BIOS_H__
#define __KOS_ARCH_X86_BIOS_H__ 1

#include <kos/compiler.h>
#include <kos/kernel/types.h>

__DECL_BEGIN

#define BIOS_INTNO_VGA  0x10 /*< Screen interrupts. */


/* VGA BIOS interrupt function call. */

/* Set VGA mode (%AH == BIOS_VGA_SETMODE; %AL == BIOS_VGA_MODE_*) */
#define BIOS_VGA_SETMODE 0x00
#define BIOS_VGA_MODE_TEXT_40x25x16_GRAY  0x00 /*< 40 x 25 characters, 16-levels of gray. */
#define BIOS_VGA_MODE_TEXT_40x25x16_COLOR 0x01 /*< 40 x 25 characters, 16 different colors ("vga.h" - "enum vga_color"). */
#define BIOS_VGA_MODE_TEXT_80x25x16_GRAY  0x02 /*< 80 x 25 characters, 16-levels of gray. */
#define BIOS_VGA_MODE_TEXT_80x25x16_COLOR 0x03 /*< 80 x 25 characters, 16 different colors ("vga.h" - "enum vga_color"). */
#define BIOS_VGA_MODE_GFX_320x200x4_COLOR 0x04 /*< 320 x 200 pixels, 4 different colors. */
#define BIOS_VGA_MODE_GFX_320x200x4_GRAY  0x05 /*< 320 x 200 pixels, 4 levels of gray. */
#define BIOS_VGA_MODE_GFX_640x200         0x06 /*< 640 x 200 pixels, monochrome. */
#define BIOS_VGA_MODE_TEXT_80x25          0x07 /*< 80 x 25 characters, monochrome. */
#define BIOS_VGA_MODE_GFX_320x200x16      0x0d /*< 320 x 200 pixels, 16 different colors. */
#define BIOS_VGA_MODE_GFX_640x200x16      0x0e /*< 640 x 200 pixels, 16 different colors. */
#define BIOS_VGA_MODE_GFX_640x350         0x0f /*< 640 x 350 pixels, monochrome. */
#define BIOS_VGA_MODE_GFX_640x350x16      0x10 /*< 640 x 350 pixels, 16 different colors. */
#define BIOS_VGA_MODE_GFX_640x480         0x11 /*< 640 x 480 pixels, monochrome. */
#define BIOS_VGA_MODE_GFX_640x480x16      0x12 /*< 640 x 480 pixels, 16 different colors. */
#define BIOS_VGA_MODE_GFX_320x200x256     0x13 /*< 320 x 200 pixels, 256 different colors (every pixel is one byte). */



__DECL_END

#endif /* !__KOS_ARCH_X86_BIOS_H__ */

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
#ifndef __KOS_KERNEL_TTY_C__
#define __KOS_KERNEL_TTY_C__ 1

#include <assert.h>
#include <format-printer.h>
#include <kos/arch/x86/vga.h>
#include <kos/kernel/debug.h>
#include <kos/kernel/interrupts.h>
#include <kos/kernel/pageframe.h>
#include <kos/kernel/tty.h>
#include <kos/types.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <kos/atomic.h>

__DECL_BEGIN

static __u8   tty_color; /* atomic */
static __u16 *tty_buffer_pos;
static __u16 *tty_buffer_begin;
static __u16 *tty_buffer_end;
#define tty_buffer_set(addr) \
 (tty_buffer_end = (tty_buffer_begin = (uint16_t *)(addr))+(VGA_WIDTH*VGA_HEIGHT))

#ifdef __x86__
void tty_x86_setcolors(__u8 colors) {
 katomic_store(tty_color,colors);
}
#endif

__local __u8 tty_ansi_to_fg_m0(__u32 x) {
 switch (x) {
//case 30:
  default: return VGA_COLOR_BLACK;
  case 34: return VGA_COLOR_BLUE;
  case 32: return VGA_COLOR_GREEN;
  case 36: return VGA_COLOR_CYAN;
  case 31: return VGA_COLOR_RED;
  case 35: return VGA_COLOR_MAGENTA;
  case 33: return VGA_COLOR_BROWN;
  case 37: return VGA_COLOR_LIGHT_GREY;
 }
}
__local __u8 tty_ansi_to_fg_m1(__u32 x) {
 switch (x) {
  case 30: return VGA_COLOR_DARK_GREY;
  case 34: return VGA_COLOR_LIGHT_BLUE;
  case 32: return VGA_COLOR_LIGHT_GREEN;
  case 36: return VGA_COLOR_LIGHT_CYAN;
  case 31: return VGA_COLOR_LIGHT_RED;
  case 35: return VGA_COLOR_LIGHT_MAGENTA;
  case 33: return VGA_COLOR_LIGHT_BROWN;
//case 37:
  default: return VGA_COLOR_WHITE;
 }
}

__local __u8 tty_arg1(__u32 arg1) {
 __u8 oldcolors,newcolors; do {
  oldcolors = tty_color;
  newcolors = oldcolors&0x0f;
  newcolors |= tty_ansi_to_fg_m0(arg1-10) << 4;
 } while (!katomic_cmpxch(tty_color,oldcolors,newcolors));
 return newcolors;
}
__local __u8 tty_arg2(__u32 arg1, __u32 arg2) {
 __u8 oldcolors,newcolors; do {
  oldcolors = tty_color;
  newcolors = oldcolors&0xf0;
  newcolors |= arg1 ? tty_ansi_to_fg_m1(arg2) : tty_ansi_to_fg_m0(arg2);
 } while (!katomic_cmpxch(tty_color,oldcolors,newcolors));
 return newcolors;
}
__local __u8 tty_arg3(__u32 arg1, __u32 arg2, __u32 arg3) {
 __u8 newcolors;
 newcolors  = tty_ansi_to_fg_m0(arg1-10) << 4;
 newcolors |= arg2 ? tty_ansi_to_fg_m1(arg3) : tty_ansi_to_fg_m0(arg3);
 katomic_store(tty_color,newcolors);
 return newcolors;
}

__local __u8 tty_escape(char const *__restrict data, size_t data_max) {
 char const *end = data+data_max; __u32 args[3];
 assert(data_max);
 if (*data == '[') ++data;
 args[0] = _strntou32(data,(size_t)(end-data),(char **)&data,0);
 if (*data == ';') {
  ++data,args[1] = _strntou32(data,(size_t)(end-data),(char **)&data,0);
  if (*data == ';') {
   ++data,args[2] = _strntou32(data,(size_t)(end-data),(char **)&data,0);
   return tty_arg3(args[0],args[1],args[2]);
  } else {
   return tty_arg2(args[0],args[1]);
  }
 } else {
  return tty_arg1(args[0]);
 }
}


static void tty_clear_unlocked(void) {
 uint16_t *iter,*end,entry;
 iter = tty_buffer_pos = tty_buffer_begin;
 end = tty_buffer_end,entry = vga_entry(' ',tty_color);
 while (iter != end) *iter++ = entry;
}

void tty_clear(void) {
 tty_clear_unlocked();
}

void kernel_initialize_tty(void) {
 tty_color = VGA_DEFAULT_COLOR;
 tty_buffer_set(VGA_ADDR);
 debug_addknownramregion(VGA_ADDR,VGA_WIDTH*VGA_HEIGHT*2);
 tty_clear_unlocked();
}

void tty_printf(char const *__restrict format, ...) {
 va_list args;
 va_start(args,format);
 tty_vprintf(format,args);
 va_end(args);
}
static int
tty_vprintf_callback(char const *__restrict data,
                     size_t maxchars, void *__unused(closure)) {
 tty_printn(data,maxchars);
 return 0;
}
void tty_vprintf(char const *__restrict format, va_list args) {
 format_vprintf(&tty_vprintf_callback,NULL,format,args);
}

#ifndef __INTELLISENSE__
#define PRINTN
#include "tty-print.c.inl"
#include "tty-print.c.inl"
#endif

__DECL_END

#endif /* !__KOS_KERNEL_TTY_C__ */

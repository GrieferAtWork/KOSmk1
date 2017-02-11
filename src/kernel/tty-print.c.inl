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
#ifdef __INTELLISENSE__
#include "tty.c"
__DECL_BEGIN
#define PRINTN
#endif

#ifdef PRINTN
__size_t tty_printn(__kernel char const *__restrict data, size_t max_bytes)
#else
void tty_prints(__kernel char const *__restrict data, size_t max_bytes)
#endif
{
 char const *iter,*end; char ch;
 size_t i; __u8 color; __u16 *dst,*start;
 end = (iter = data)+max_bytes;
 dst = start = katomic_load(tty_buffer_pos);
 color = katomic_load(tty_color);
 while (iter != end) {
#ifdef PRINTN
  if ((ch = *iter) == '\0') break;
  ++iter;
#else
  ch = *iter++;
#endif
  switch (ch) {
   case '\v': ++dst; break;
   case '\e':
    if (end != iter) {
     color = tty_escape(iter,(size_t)(end-iter));
#ifdef PRINTN
     while (iter != end && *iter && *iter++ != 'm');
#else
     while (iter != end && *iter++ != 'm');
#endif
    }
    break;
   case '\n': dst += (VGA_WIDTH-((dst-tty_buffer_begin) % VGA_WIDTH)); break;
   case '\r': dst -= ((dst-tty_buffer_begin) % VGA_WIDTH); break;
   case '\b': if (dst != tty_buffer_begin) *--dst = vga_entry(' ',color); break;
#if !VGA_TABSIZE
   case '\t': break;
#elif VGA_TABSIZE != 1
   {
    size_t linepos;
   case '\t':
    linepos = ((dst-tty_buffer_begin) % VGA_WIDTH);
    dst = (dst-linepos)+ceildiv(linepos+1,VGA_TABSIZE)*VGA_TABSIZE;
    if __unlikely(dst > tty_buffer_end) dst = tty_buffer_end;
    break;
   }
#else
   case '\t':
    ch = ' ';
    // fallthrough
#endif
   default: *dst++ = vga_entry(ch,color); break;
  }
  if (dst == tty_buffer_end) {
#if 0
   dst == tty_buffer_begin;
#else
   // Move the log up by one line
   memmove(tty_buffer_begin,tty_buffer_begin+VGA_WIDTH,
          (VGA_WIDTH*(VGA_HEIGHT-1))*sizeof(__u16));
   dst -= VGA_WIDTH;
   for (i = 0; i < VGA_WIDTH; ++i) dst[i] = vga_entry(' ',color);
#endif
  }
 }
 katomic_cmpxch(tty_buffer_pos,start,dst);
#ifdef PRINTN
 return (size_t)(iter-data);
#endif
}


#ifdef PRINTN
#undef PRINTN
#endif

#ifdef __INTELLISENSE__
__DECL_END
#endif

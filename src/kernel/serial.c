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
#ifndef __KOS_KERNEL_SERIAL_C__
#define __KOS_KERNEL_SERIAL_C__ 1

#include <stddef.h>
#include <sys/io.h>
#include <kos/compiler.h>
#include <kos/kernel/serial.h>
#include <stdint.h>
#include <format-printer.h>

__DECL_BEGIN

void kernel_initialize_serial(void) {
 serial_enable(SERIAL_01);
}
void serial_enable(int device) {
 outb(0x00,device+1); // Disable all interrupts
 outb(0x80,device+3); // Enable DLAB (set baud rate divisor)
 outb(0x03,device+0); // Set divisor to 3 (lo byte) 38400 baud
 outb(0x00,device+1); //                  (hi byte)
 outb(0x03,device+3); // 8 bits, no parity, one stop bit
 outb(0xC7,device+2); // Enable FIFO, clear them, with 14-byte threshold
 outb(0x0B,device+4); // IRQs enabled, RTS/DSR set
}
#define is_transmit_empty(device) (inb((device)+5)&0x20)

size_t serial_printn(int device, char const *data, size_t maxchars) {
 char const *iter,*end; char ch;
 end = (iter = data)+maxchars;
 while (iter != end) {
  if ((ch = *iter++) == '\0') break;
  while (is_transmit_empty(device) == 0);
  outb(device,(unsigned char)ch); 
 }
 return (size_t)(iter-data);
}
void serial_prints(int device, char const *data, size_t maxchars) {
 char const *iter,*end; char ch;
 end = (iter = data)+maxchars;
 while (iter != end) {
  ch = *iter++;
  while (is_transmit_empty(device) == 0);
  outb(device,(unsigned char)ch); 
 }
}

void serial_printf(int device, char const *format, ...) {
 va_list args;
 va_start(args,format);
 serial_vprintf(device,format,args);
 va_end(args);
}
static int serial_vprintf_callback(char const *data, size_t maxchars, void *device) {
 serial_printn((int)(uintptr_t)device,data,maxchars);
 return 0;
}
void serial_vprintf(int device, char const *format, va_list args) {
 format_vprintf(&serial_vprintf_callback,(void *)(uintptr_t)device,format,args);
}


__DECL_END

#endif /* !__KOS_KERNEL_SERIAL_C__ */

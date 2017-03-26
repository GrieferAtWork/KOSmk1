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
#ifndef __HW_TIME_I8253_H__
#define __HW_TIME_I8253_H__ 1

#include <kos/compiler.h>
#include <kos/types.h>
#include <sys/io.h>

__DECL_BEGIN

#define I8253_IOPORT_CMD  0x43
#define I8253_IOPORT_DATA 0x40


#define I8253_CMD_READCOUNT 0

__local __u16 i8253_readcount(void) {
 /* TODO: Protect this function with a spinlock. */
 outb(I8253_IOPORT_CMD,I8253_CMD_READCOUNT);
 return (__u16)inb(I8253_IOPORT_DATA) |
       ((__u16)inb(I8253_IOPORT_DATA) << 8);
}

__local void i8253_delay10ms(void) {
 __u16 curr_count,prev_count;
 curr_count = i8253_readcount();
 do {
  prev_count = curr_count;
  curr_count = i8253_readcount();
 } while ((curr_count-prev_count) < 300);
}

__local void i8253_delay(unsigned int n10ms) {
 while (n10ms--) i8253_delay10ms();
}


__DECL_END

#endif /* !__HW_TIME_I8253_H__ */

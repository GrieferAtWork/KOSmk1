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
#ifndef __KOS_KERNEL_SERIAL_H__
#define __KOS_KERNEL_SERIAL_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <stdarg.h>
#include <kos/compiler.h>
#include <kos/kernel/types.h>

__DECL_BEGIN

#define SERIAL_01 0x3F8

//////////////////////////////////////////////////////////////////////////
// Initialize the serial driver
extern void kernel_initialize_serial(void);
extern void serial_enable(int device);

//////////////////////////////////////////////////////////////////////////
// Print data to a given serial device.
// @return: * : Amount of bytes actually transmitted.
extern __nonnull((2)) __size_t serial_printn(int device, char const *__restrict data, __size_t maxchars);
extern __nonnull((2)) __size_t serial_prints(int device, char const *__restrict data, __size_t maxchars);
extern __nonnull((2)) __attribute_vaformat(__printf__,2,3) __size_t serial_printf(int device, char const *__restrict format, ...);
extern __nonnull((2)) __attribute_vaformat(__printf__,2,0) __size_t serial_vprintf(int device, char const *__restrict format, va_list args);
#define serial_print(device,data)  serial_printn(device,data,(__size_t)-1)

__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_SERIAL_H__ */

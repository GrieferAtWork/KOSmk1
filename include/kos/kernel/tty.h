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
#ifndef __KOS_KERNEL_TTY_H__
#define __KOS_KERNEL_TTY_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <stdarg.h>
#include <kos/compiler.h>
#include <kos/kernel/types.h>

__DECL_BEGIN

extern void kernel_initialize_tty(void);

extern void tty_clear(void);

#define tty_print(s) tty_printn(s,(__size_t)-1)
extern __nonnull((1)) __size_t tty_printn(__kernel char const *__restrict data, __size_t max_bytes);
extern __nonnull((1)) void tty_prints(__kernel char const *__restrict data, __size_t max_bytes);
extern __nonnull((1)) __attribute_vaformat(__printf__,1,2) void tty_printf(char const *__restrict format, ...);
extern __nonnull((1)) __attribute_vaformat(__printf__,1,0) void tty_vprintf(char const *__restrict format, va_list args);

#ifdef __x86__
extern void tty_x86_setcolors(__u8 colors);
#endif

__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_TTY_H__ */

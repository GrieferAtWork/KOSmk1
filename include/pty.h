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
#ifndef	__PTY_H__
#ifndef	_PTY_H
#define	__PTY_H__	1
#define	_PTY_H	1

#include <kos/compiler.h>
#include <kos/types.h>

__DECL_BEGIN

struct termios;
struct winsize;

#ifndef __CONFIG_MIN_LIBC__
extern __nonnull((1,2)) int
openpty __P((int *__restrict amaster, int *__restrict aslave, char *name,
             struct termios const *termp, struct winsize const *winp));
#endif /* !__CONFIG_MIN_LIBC__ */

__DECL_END

#endif /* !_PTY_H */
#endif /* !__PTY_H__ */

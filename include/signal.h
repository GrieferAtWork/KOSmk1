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
#ifndef __SIGNAL_H__
#ifndef _SIGNAL_H
#define __SIGNAL_H__ 1
#define _SIGNAL_H 1

#include <kos/compiler.h>
#include <kos/config.h>
#include <kos/types.h>
#include <kos/timespec.h>

__DECL_BEGIN

#ifndef __pid_t_defined
#define __pid_t_defined 1
typedef __pid_t pid_t;
#endif
#ifndef __uid_t_defined
#define __uid_t_defined 1
typedef __uid_t uid_t;
#endif

typedef void (*__libc_sighandler_t)(int signum);


#if 0
extern __libc_sighandler_t signal(int sig, __libc_sighandler_t func);
extern int raise(int sig);
#endif


__DECL_END

#endif /* !_SIGNAL_H */
#endif /* !__SIGNAL_H__ */

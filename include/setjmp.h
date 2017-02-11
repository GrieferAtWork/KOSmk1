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
#ifndef __SETJMP_H__
#ifndef _SETJMP_H
#ifndef _INC_SETJMP
#define __SETJMP_H__ 1
#define _SETJMP_H 1
#define _INC_SETJMP 1

#include <kos/compiler.h>

__DECL_BEGIN

struct __jmp_buf { void *ptr[5]; };
typedef __jmp_buf jmp_buf[1];

#ifdef __INTELLISENSE__
extern             int setjmp(jmp_buf buf);
extern __noreturn void longjmp(jmp_buf buf, int sig);
#else
#define setjmp  __builtin_setjmp
#define longjmp __builtin_longjmp
#endif

__DECL_END

#endif /* !_INC_SETJMP */
#endif /* !_SETJMP_H */
#endif /* !__SETJMP_H__ */

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
#include <kos/types.h>

__DECL_BEGIN

struct __jmp_buf {
 /* NOTE: EAX is not saved, because it will be set by 'longjmp' */
 __uintptr_t __jb_ecx,__jb_edx,__jb_ebx,__jb_esp;
 __uintptr_t __jb_ebp,__jb_esi,__jb_edi,__jb_eip;
};
#define _JMP_BUF_STATIC_INIT  {{0,0,0,0,0,0,0,0}}
typedef struct __jmp_buf jmp_buf[1];

extern __attribute__((__returns_twice__)) int setjmp(jmp_buf buf);
extern __noreturn void longjmp(jmp_buf buf, int sig);

__DECL_END

#endif /* !_INC_SETJMP */
#endif /* !_SETJMP_H */
#endif /* !__SETJMP_H__ */

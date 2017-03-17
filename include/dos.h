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
#ifndef __DOS_H__
#define __DOS_H__ 1

#include <kos/compiler.h>

#ifndef __ASSEMBLY__
#ifdef __LEGACY_OUT_PARAMORDER__
#ifdef __SYS_IO_H__
#error "<sys/io.h> was already included with legacy out parameter ordering"
#endif /* __SYS_IO_H__ */
#undef __LEGACY_OUT_PARAMORDER__
#endif
#include <sys/io.h>
#include <kos/arch/irq.h>

__DECL_BEGIN

#ifndef SEEK_SET
#   define SEEK_SET 0
#endif
#ifndef SEEK_CUR
#   define SEEK_CUR 1
#endif
#ifndef SEEK_END
#   define SEEK_END 2
#endif


#ifndef __UNISTD_H__
/* Unistd defines 'sleep()' this as returning 'unsigned int'. */
#ifdef __NO_asmname
extern void sleep __P((unsigned int __seconds));
#else /* __NO_asmname */
extern unsigned int __libc_sleep __P((unsigned int __seconds)) __asmname("sleep");
__local void sleep __D1(unsigned int,__seconds) { __libc_sleep(__seconds); }
#endif /* !__NO_asmname */
#endif /* !__UNISTD_H__ */


extern char **environ;
extern int unlink __P((char const *__path));
__local void _disable __D0() { karch_irq_disable(); }
__local void _enable __D0() { karch_irq_enable(); }


#define disable _disable
#define enable  _enable

#define inp      inb
#define inportb  inb
#define inpw     inw
#define inport   inw
#define outp     __outb
#define outportb __outb
#define outpw    __outw
#define outport  __outw

/* Did these exist? */
#define inpb     inb
#define inpl     inl
#define inportw  inw
#define inportl  inl
#define outpb    __outb
#define outpl    __outl
#define outportw __outw
#define outportl __outl


__DECL_END
#endif /* !__ASSEMBLY__ */

#endif /* !__DOS_H__ */

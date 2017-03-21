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
#ifndef __TEST_C__
#define __TEST_C__ 1


#include <stdio.h>
#include <stdlib.h>
#include <proc.h>
#include <kos/syslog.h>
#include <exception.h>
#include <stdarg.h>
#include <traceback.h>

static void outf(char const *fmt, ...) {
 va_list args;
 va_start(args,fmt);
 k_vsyslogf(KLOG_INFO,fmt,args);
 va_end(args);
 va_start(args,fmt);
 vprintf(fmt,args);
 va_end(args);
}


#if 0
__attribute_thread int foo = 42;

static void *thread_test(void *closure) {
 k_syslogf(KLOG_ERROR,"thread #2: tls_addr = %p\n",tls_addr);
 k_syslogf(KLOG_ERROR,"thread #2: foo = %p:%d\n",&foo,foo);
 ++foo;
 k_syslogf(KLOG_ERROR,"thread #2: foo = %p:%d\n",&foo,foo);
 return closure;
}
#endif


static __noreturn void handler(void) {
 outf("error = %d\n",exc_code());
 outf("eip = %p\n",tls_self->u_exstate.eip);
 tb_printebp((void const *)tls_self->u_exstate.ebp);
 exc_rethrow();
}


int main(void) {
 /* Reminder: This file gets compiler with GCC. - Not VC/VC++! */
 register int x = 42;
#define get_esp() __xblock({ void *r; __asm__("mov %%esp, %0\n" : "=r" (r)); __xreturn r; })

 __try {
  KEXCEPT_TRY_H(&handler) { exc_raise(69); }
 } __except(1) {
  assert(exc_code() == 69);
 }
 assert(exc_code() == 0);

 outf("Before try %d (%p)\n",x,get_esp());
 __try {
  outf("Before raise %d (%p)\n",x,get_esp());
  exc_raise(KEXCEPTION_TEST);
  outf("After raise %d (%p)\n",x,get_esp());
 } __finally {
  outf("In finally %d (%p)\n",x,get_esp());
 }
 outf("After finally %d (%p)\n",x,get_esp());

#if 0
 ptrdiff_t off = tls_alloc(NULL,1);
 k_syslogf(KLOG_ERROR,"thread #1: tls_addr = %p\n",tls_addr);
 k_syslogf(KLOG_ERROR,"thread #1: foo = %p:%d\n",&foo,foo);
 ++foo;
 k_syslogf(KLOG_ERROR,"thread #1: foo = %p:%d\n",&foo,foo);
 t = task_newthread(&thread_test,NULL,TASK_NEWTHREAD_DEFAULT);
 if (t == -1) perror("task_newthread");
 else task_join(t,NULL),close(t);
 tls_free(off);
#endif
 return 0;
}

#endif /* !__TEST_C__ */

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

#include "helper.h"
#include <stddef.h>
#include <proc.h>
#include <unistd.h>

static __attribute_thread int threadlocal_foo = 42;

static void *thread_test(void *closure) {
 outf("thread #2: tls_addr = %p\n",tls_addr);
 outf("thread #2: foo = %p:%d\n",&threadlocal_foo,threadlocal_foo);
 ++threadlocal_foo;
 outf("thread #2: foo = %p:%d\n",&threadlocal_foo,threadlocal_foo);
 return closure;
}


TEST(tls) {
 ptrdiff_t off; int t;

 /* ELF TLS sections are not being parsed yet.
  * - But KOS's new TLS system is designed to feature
  *   binary compatibility with what ELF does, meaning
  *   that the following line currently allocated the
  *   storage used by 'threadlocal_foo'.
  */
 off = tls_alloc(NULL,1);
 if (!off) outf("tls_alloc() : %d : %s\n",errno,strerror(errno));

 outf("thread #1: tls_addr = %p\n",tls_addr);
 outf("thread #1: foo = %p:%d\n",&threadlocal_foo,threadlocal_foo);
 ++threadlocal_foo;
 outf("thread #1: foo = %p:%d\n",&threadlocal_foo,threadlocal_foo);
 t = task_newthread(&thread_test,NULL,TASK_NEWTHREAD_DEFAULT);
 if (t == -1) outf("task_newthread() : %d : %s\n",errno,strerror(errno));
 else task_join(t,NULL),close(t);

 /* Cleanup... */
 tls_free(off);
}

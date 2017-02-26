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
#ifndef __PE_TEST_C__
#define __PE_TEST_C__ 1

#include <stddef.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <kos/task.h>
#include <kos/syslog.h>

extern void __libc_init(void);

void _start(void) {
 int i;
 for (i = 0; i < 20; ++i) {
  k_syslog(KLOG_INFO,"System log entry from PE file!\n",(size_t)-1);
 }
 __libc_init();
 char *s = strdup("calling a function from a .so library!");
 printf("This is an .exe file %s\n",s);
 free(s);



 kproc_exit(0);
 /* We should never get here! */
 __builtin_unreachable();
}

#endif /* !__PE_TEST_C__ */

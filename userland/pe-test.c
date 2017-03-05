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
#include <proc.h>
#include <unistd.h>
#include <stdio.h>
#include <kos/task.h>
#include <kos/syslog.h>

__public void exported_function(void) {
 printf("very secret callback...\n");
}

int main(int argc, char *argv[]) {
 //int i;
 //for (i = 0; i < 20; ++i) {
 // k_syslog(KLOG_INFO,"System log entry from PE file!\n",(size_t)-1);
 //}

 char *s = strdup("calling a function from a .so library!");
 printf("This is an .exe file %s\n",s);
 free(s);

 /* Do some testing for copy-on-write. */
 task_t t;
 size_t memsize = 1024*1024*16;
 char *memory = (char *)malloc(memsize);
 if ((t = task_fork()) == 0) {
  k_syslogf(KLOG_INFO,"Begin memset() in child task...\n");
  memset(memory,0xa0,memsize);
  k_syslogf(KLOG_INFO,"End memset() in child task...\n");
  _exit(0);
 }
 k_syslogf(KLOG_INFO,"Begin memset() in parent task...\n");
 memset(memory,0x0a,memsize);
 k_syslogf(KLOG_INFO,"End memset() in parent task...\n");
 if (t != -1) { ktask_join(t,NULL,0); close(t); }
 free(memory);
 k_syslogf(KLOG_INFO,"DONE...\n");


 return 0;
}

#endif /* !__PE_TEST_C__ */

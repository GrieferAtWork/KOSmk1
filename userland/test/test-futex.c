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
#include <kos/futex.h>
#include <kos/sync-mutex.h>
#include <kos/sync-rwlock.h>
#include <malloc.h>
#include <proc.h>
#include <unistd.h>
#include <limits.h>

static unsigned int ready  = 0;
static unsigned int ftx[2] = {0,0};

static void *thread_main(void *p) {
 kerrno_t error;
 char buffer[256];
 /* atomic: if (true) { ready = 1; sendall(&ready); vrecv(&ftx,buffer); } */
#if 1
 struct kfutexset set;
 set.fs_next  = NULL;
 set.fs_op    = KFUTEX_RECVIF(KFUTEX_TRUE)|
                KFUTEX_CCMD_SENDALL(KFUTEX_ADD);
 set.fs_val   = 0;
 set.fs_count = 2;
 set.fs_align = sizeof(kfutex_t);
 set.fs_avec  = ftx;
 outf("[THREAD] Begin receiving data\n");
 error = kfutex_ccmds(&set,buffer,NULL,&ready,1);
#else
 error = kfutex_ccmd(&ftx,KFUTEX_RECVIF(KFUTEX_TRUE)|
                          KFUTEX_CCMD_SENDALL(KFUTEX_ADD),
                     0,buffer,NULL,&ready,1);
#endif
 assertf(error == KE_OK,"%d",error);
 outf("[THREAD] Data: %.256q\n",buffer);
 return p;
}


TEST(futex) {
 static char const data[] = "This is the data that is send through a signal";
 kerrno_t error;
 int t[] = {
  task_newthread(&thread_main,NULL,TASK_NEWTHREAD_DEFAULT),
  task_newthread(&thread_main,NULL,TASK_NEWTHREAD_DEFAULT),
 };

 /* atomic: if (ready == 0) { recv(&ready). }
  * NOTE: This may randomly return 'KE_AGAIN' when
  *       the thread is already receiving. */
 outf("[MAIN] Begin receiving...\n");
 while (ready != 2) {
  error = kfutex_cmd(&ready,KFUTEX_RECVIF(KFUTEX_NOT_EQUAL),2,NULL,NULL);
  outf("[MAIN] Wakeup for %d...\n",error);
 }

 outf("[MAIN] Sending data...\n");
 /* atomic: vsendall(&ftx,data,sizeof(data)); */
 error = kfutex_vsendall(&ftx[0],data,sizeof(data));
 error = kfutex_vsendall(&ftx[1],data,sizeof(data));
 outf("[MAIN] Joining thread after %d...\n",error);
 task_join(t[0],NULL);
 task_join(t[1],NULL);
 outf("[MAIN] Joined thread\n");
 close(t[0]);
 close(t[1]);
}

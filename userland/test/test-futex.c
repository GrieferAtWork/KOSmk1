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
#include <malloc.h>
#include <proc.h>
#include <unistd.h>
#include <limits.h>

static unsigned int ftx   = 0;
static unsigned int ready = 0;

static void *thread_main(void *p) {
 kerrno_t error;
 char buffer[256];
 /* atomic: if (true) { ready = 1; sendall(&ready); vrecv(&ftx,buffer); } */
 outf("[THREAD] Begin receiving data\n");
 error = kfutex_ccmd(&ftx,KFUTEX_CCMD_SENDALL(KFUTEX_STORE)|
                          KFUTEX_RECVIF(KFUTEX_TRUE),
                     0,buffer,NULL,&ready,1);
 assertf(error == KE_OK,"%d",error);
 outf("[THREAD] Data: %.256q\n",buffer);
 return p;
}


TEST(futex) {
 static char const data[] = "This is the data that is send through a signal";
 kerrno_t error;
 int t = task_newthread(&thread_main,NULL,TASK_NEWTHREAD_DEFAULT);

 /* atomic: if (ready == 0) { recv(&ready). }
  * NOTE: This may randomly return 'KE_AGAIN' when
  *       the thread is already receiving. */
 error = kfutex_cmd(&ready,KFUTEX_RECVIF(KFUTEX_EQUAL),0,NULL,NULL);
 outf("[MAIN] Sending data after %d...\n",error);

 /* atomic: vsend(&ftx,data,sizeof(data)); */
 error = kfutex_vsendall(&ftx,data,sizeof(data));

 outf("[MAIN] Joining thread after %d...\n",error);
 task_join(t,NULL);
 outf("[MAIN] Joined thread\n");
 close(t);
}

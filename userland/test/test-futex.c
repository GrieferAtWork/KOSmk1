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


static struct kmutex user_mutex = KMUTEX_INIT;

static void *thread_main(void *p) {
 kerrno_t error;
 outf("[THREAD] begin:lock()\n");
 error = kmutex_lock(&user_mutex);
 outf("[THREAD] end:lock(): %d\n",error);
 outf("[THREAD] begin:unlock()\n");
 error = kmutex_unlock(&user_mutex);
 outf("[THREAD] end:unlock(): %d\n",error);
 return p;
}


TEST(futex) {
 int t = task_newthread(&thread_main,NULL,TASK_NEWTHREAD_DEFAULT);

 kerrno_t error;
 outf("[MAIN] begin:lock()\n");
 error = kmutex_lock(&user_mutex);
 outf("[MAIN] end:lock(): %d\n",error);
 ktask_yield();
 outf("[MAIN] begin:unlock()\n");
 error = kmutex_unlock(&user_mutex);
 outf("[MAIN] end:unlock(): %d\n",error);

 task_join(t,NULL);
 outf("[MAIN] Joined thread\n");
 close(t);
 outf("[MAIN] Closed thread\n");
}

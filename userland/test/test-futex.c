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
#include <malloc.h>
#include <proc.h>
#include <unistd.h>
#include <limits.h>


static unsigned int *user_futex;

static void *thread_main(void *p) {
 outf("[THREAD] Begin waiting\n");
 kerrno_t wait_error = ktask_futex(user_futex,KTASK_FUTEX_WAIT,0,NULL);
 outf("[THREAD] Done waiting: %d (%d)\n",wait_error,*user_futex);
 return p;
}


TEST(futex) {
 user_futex = (unsigned int *)malloc(sizeof(unsigned int));
 *user_futex = 1;
 int t = task_newthread(&thread_main,NULL,TASK_NEWTHREAD_DEFAULT);
 outf("[MAIN] Thread spawned (Sleep a bit)\n");
 struct timespec tmo = {1,0};
 task_sleep(task_self(),&tmo);
 outf("[MAIN] Waking futex\n");
 kerrno_t error = ktask_futex(user_futex,KTASK_FUTEX_WAKE,UINT_MAX,NULL);
 outf("[MAIN] Wake error: %d\n",error);
 task_join(t,NULL);
 outf("[MAIN] Joined thread\n");
 close(t);
 outf("[MAIN] Closed thread\n");
 free(user_futex);
 outf("[MAIN] Free\n");
}

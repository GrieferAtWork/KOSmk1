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
#ifndef __KOS_KERNEL_TEST_C__
#define __KOS_KERNEL_TEST_C__ 1

#include <assert.h>
#include <kos/compiler.h>
#include <kos/errno.h>
#include <kos/kernel/proc.h>
#include <kos/kernel/sched_yield.h>
#include <kos/kernel/sem.h>
#include <kos/kernel/task.h>
#include <stdio.h>

#define __test  __attribute__((__constructor__))

__DECL_BEGIN

#define KE_ASSERT(expr,eno)\
 __xblock({ kerrno_t __err = (expr);\
            assertf(__err == eno,"Expected %d, but got %d for " #expr,eno,__err);\
 })


__test void test_interrupts(void) {
 karch_irq_disable();
 assert(!karch_irq_enabled());
 karch_irq_enable();
 assert(karch_irq_enabled());
}




static struct ksignal sig = KSIGNAL_INIT;
static void *signals_main(void *closure) {
 struct timespec timeout = {1,0};
 assert(closure == (void *)42);
 ktask_sleep(ktask_self(),&timeout);
 assert(closure == (void *)42);
 KE_ASSERT(ksignal_recv(&sig),KE_OK);
 return closure;
}

void test_signals(void) {
 struct ktask *ts; void *exitcode;
 ts = ktask_new_suspended(signals_main,(void *)42); assert(ts);
 assert(ktask_getstate(ts) == KTASK_STATE_SUSPENDED);
 KE_ASSERT(ktask_resume_k(ts),KE_OK);
 while (ktask_getstate(ts) != KTASK_STATE_WAITING) ktask_yield();
 assert(ksignal_hasrecv(&sig));
 assert(ktask_getstate(ts) == KTASK_STATE_WAITING);
 KE_ASSERT(ktask_suspend_k(ts),KE_OK);
 assert(ktask_getstate(ts) == KTASK_STATE_SUSPENDED);
 asserte(ksignal_sendall(&sig) == 1);
 assert(ktask_getstate(ts) == KTASK_STATE_SUSPENDED);
 KE_ASSERT(ktask_resume_k(ts),KE_OK);
 KE_ASSERT(ktask_join(ts,&exitcode),KE_OK);
 assert(ktask_getstate(ts) == KTASK_STATE_TERMINATED);
 assert(exitcode == (void *)42);
 assert(ts->t_refcnt == 1);
 ktask_decref(ts);
}


static struct ksem sem = KSEM_INIT(0);
static void *my_ktask_main(void *closure) {
 assert(closure == (void *)42);
 assertf(ktask_self()->t_suspended == 0,
         "%I32d",ktask_self()->t_suspended);
 KE_ASSERT(ktask_suspend_k(ktask_self()),KE_OK);
 assert(closure == (void *)42);
 KE_ASSERT(ksem_wait(&sem),KE_OK);
 return closure;
}

__test void test_tasks(void) {
 struct ktask *task;
 KTASK_CRIT_BEGIN
 task = ktask_new(&my_ktask_main,(void *)42);
 assert(task != NULL);
 KE_ASSERT(ktask_setname(task,"my_ktask_main"),KE_OK);
// KE_ASSERT(ktask_tryyield(),KE_OK);
 while (ktask_getstate(task) == KTASK_STATE_RUNNING) ktask_yield();
 KE_ASSERT(ktask_resume_k(task),KE_OK);
 KE_ASSERT(ksem_post(&sem,1,NULL),KE_OK);
 kassert_ktask(task);
 void *result;
 KE_ASSERT(ktask_join(task,&result),KE_OK);
 assert(result == (void *)42);
 assert(task->t_refcnt == 1);
 assert(ktask_getstate(task) == KTASK_STATE_TERMINATED);
 KE_ASSERT(ktask_suspend_k(task),KE_DESTROYED);
 ktask_decref(task);
 KTASK_CRIT_END
}

static void *sleep_task(void *c) {
 struct timespec to = {100,0};
 for (;;) ktask_sleep(ktask_self(),&to);
 return c;
}

__test void test_sleep_terminate(void) {
 int i;
 struct ktask *tasks[200];
 KTASK_CRIT_BEGIN
 // Create a whole bunch of tasks
 for (i = 0; i < __compiler_ARRAYSIZE(tasks); ++i) {
  tasks[i] = ktask_new(&sleep_task,(void *)i);
  assert(tasks[i] != NULL);
 }
 // Let all those tasks run a bit
 struct timespec to = {1,0};
 ktask_sleep(ktask_self(),&to);
 // Now terminate them all
 for (i = 0; i < __compiler_ARRAYSIZE(tasks); ++i) {
  KE_ASSERT(ktask_terminate_k(tasks[i],NULL),KE_OK);
  assert(ktask_isterminated(tasks[i]));
  // Make sure the reference counters are correct
  assertf(tasks[i]->t_refcnt == 1,"%d",tasks[i]->t_refcnt);
  ktask_decref(tasks[i]);
 }
 assert(!ktask_self()->t_children.t_taska);
 assert(!ktask_self()->t_children.t_taskv);
 KTASK_CRIT_END
}

static void *suspended_threadmain(void *closure) {
 assert(closure == (void *)0xdead);
 // Suspend ourselves. - It's like sleeping, only
 // this time around we won't wake up again... :(
 __evalexpr(ktask_suspend_k(ktask_self()));
 assertf(0,"Unreachable");
 return closure;
}
__test void test_detachsuspended(void) {
 struct ktask *t;
 t = ktask_new(&suspended_threadmain,(void *)0xdead); assert(t);
 // Wait for the task to be suspended.
 while (ktask_getstate(t) != KTASK_STATE_SUSPENDED) ktask_yield();
 assertf(t->t_refcnt == 1,"%d",t->t_refcnt);
 // A suspended task is implicitly terminated when all references are dropped.
 // (Though for obvious reasons, you can't ever find out about that,
 //  as to be able to, you'd need a reference, preventing the whole process)
 // - I should totally given this thing a kick-ass name, like ~quantum task~ ;)
 ktask_decref(t);
}


static int crit_suspended_after_resume_reached;

static void *crit_suspended_threadmain(void *arg) {
 KTASK_CRIT_BEGIN
 KE_ASSERT(ktask_suspend(ktask_self()),KE_INTR);
 crit_suspended_after_resume_reached = 1;
 KTASK_CRIT_END
 assertf(0,"We were supposed to die when we left the critical block!");
 return arg;
}

__test void test_terminate_suspended_critical(void) {
 // Test termination of a suspended, critical task
 struct ktask *task; void *exitcode;
 crit_suspended_after_resume_reached = 0;
 task = ktask_new(&crit_suspended_threadmain,NULL); assert(task);
 while (ktask_getstate(task) != KTASK_STATE_SUSPENDED) ktask_yield();
 // NOTE: Must use '_ktask_terminate' instead of 'ktask_terminate',
 //       because we won't be able to join the task before it has
 //       safely left its critical block.
 KE_ASSERT(_ktask_terminate(task,(void *)42),KS_BLOCKING);
 KE_ASSERT(ktask_resume(task),KE_DESTROYED);
 KE_ASSERT(ktask_join(task,&exitcode),KE_OK);
 assertf(exitcode == (void *)42,"The task terminated with an invalid exitcode");
 ktask_decref(task);
 assertf(crit_suspended_after_resume_reached,
         "The task didn't wake up after we resumed it post-termination");
}


__DECL_END

#endif /* !__KOS_KERNEL_TEST_C__ */

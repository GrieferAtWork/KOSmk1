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
#ifdef __INTELLISENSE__
#include "task2-unsched.c.inl"
__DECL_BEGIN
#define TIMEOUT
#endif

#ifdef TERMINATE
__local kerrno_t __SCHED_CALL
ktask2_do_unschedule_unlocked_term(struct ktask2 *__restrict self)
#elif defined(TIMEOUT)
__local kerrno_t __SCHED_CALL
ktask2_do_unschedule_unlocked_time(struct ktask2 *__restrict self)
#else
__local kerrno_t __SCHED_CALL
ktask2_do_unschedule_unlocked(struct ktask2 *__restrict self)
#endif
{
 kerrno_t error; struct kcpu2 *cpu;
 struct kcpu2 *caller_cpu = kcpu2_self();
relock_cpu:
 cpu = ktask2_lockcpu_and_tasks(self);
 assert(ktask2_iscpuallowed(self,cpu));
 if __unlikely(cpu->c_current != self) {
  /* Simple case: non-current task on any CPU. */
  assert(self != ktask2_self());
  kcpu2_delcurrent_unlocked(cpu,self);
#ifdef TIMEOUT
  kcpu2_addsleeping_unlocked(cpu,self);
#endif
  error = KE_OK;
  goto end_and_dropref;
 }

 assert(cpu->c_nrunning != 0);
 assert((self == ktask2_self()) == (cpu == caller_cpu));
 assert((self->t_prev == self) == (cpu->c_nrunning == 1));
 assert((self->t_next == self) == (cpu->c_nrunning == 1));

 /* Difficult case: Must unschedule the current task of a CPU. */
 if (cpu == caller_cpu) {
  /* Unschedule the calling thread.
   * >> This is where we'll end up most of the time due to blocking . */
  error = KE_NOSYS;
  goto end_and_dropref;
 }

 /* OK... This is where it gets complicated! */
 if (cpu->c_nrunning == 1) {
  /* Special case: Only one task is running. */
#ifdef TIMEOUT
  if __unlikely(!ktask2_iscpuallowed(self,caller_cpu)) goto trick2switch;
  /* We can turn off the CPU because we are allowed to run the TASK ourselves,
   * meaning that we won't be running into any problems in the event
   * that we'd have to reactivate the CPU once we're done, but fail to do so. */
  error = kcpu2_softoff_signal(cpu);
  if __unlikely(KE_ISERR(error)) {
   k_syslogf(KLOG_ERROR,"[SCHED] Failed to turn off CPU %I8u: %d\n",cpu->c_id,error);
   goto trick2switch;
  }
  kcpu2_delcurrent_unlocked(cpu,self);
  kcpu2_unlock(cpu,KCPU2_LOCK_TASKS);
  self->t_cpu = caller_cpu;
  ktask2_unlock(self,KTASK2_LOCK_CPU);
  kcpu2_lock(caller_cpu,KCPU2_LOCK_TASKS);
  kcpu2_addsleeping_unlocked(caller_cpu,self);
  kcpu2_unlock(caller_cpu,KCPU2_LOCK_TASKS);
  if __likely(error != KS_UNCHANGED) {
   /* We failed to turn off the CPU because it already is off.
    * >> This is a special race condition that can happen
    *    if someone else is currently turning off the CPU.
    * But that's OK because we got a lock on the CPU's task list,
    * and the fact that a CPU can't fully start up and begin
    * executing its task chain until that list is unlocked means
    * that we are still in full control over it. */
   kcpu2_turnon_abort(cpu);
  }
  return KE_OK;
#else
  error = kcpu2_softoff_signal(cpu);
  if __likely(KE_ISOK(error)) {
   kcpu2_turnon_abort(cpu);
  } else
#endif
  {
#ifdef TIMEOUT
trick2switch:
#endif
   /* Much more difficult case: We must trick the CPU into switching to a different
    *                           task, thus giving us a chance to unlink this one. */
   /* TODO */
   ;
  }
 } else {
  kcpu2_unlock(cpu,KCPU2_LOCK_TASKS);
  ktask2_unlock(self,KTASK2_LOCK_CPU);
  kcpu2_wakeipi(cpu); /* Send an IPI wakeup request. */
  /* Wait until the CPU has switched to another task. */
  while (cpu->c_current == self && /*< Intended behavior (self is no longer the current task). */
         self->t_cpu    == cpu &&  /*< Something may caused the CPU to change. */
         cpu->c_nrunning >= 2      /*< Dependency: Are there enough tasks running. */
         ) ktask2_yield(NULL);
  /* At this point 'self' should have been switched out. - so start over. */
  goto relock_cpu;
 }
end_and_dropref:
 ktask2_unlock(self,KTASK2_LOCK_CPU);
 kcpu2_unlock(cpu,KCPU2_LOCK_TASKS);
 ktask2_unlock(self,KTASK2_LOCK_RESCHED);
#ifndef TIMEOUT
 ktask2_decref(self);
#endif
 return error;
}

#undef TERMINATE
#undef TIMEOUT

#ifdef __INTELLISENSE__
__DECL_END
#endif

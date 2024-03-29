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
#include "signal_v.c.inl"
__DECL_BEGIN
#define USER
#endif

#ifdef USER
__crit kerrno_t
_ksignal_vusendoneex_andunlock_c(struct ksignal *__restrict self,
                                 struct kcpu *__restrict newcpu, int hint,
                                 __user void const *__restrict buf, size_t bufsize)
#else
__crit kerrno_t
_ksignal_vsendoneex_andunlock_c(struct ksignal *__restrict self,
                                struct kcpu *__restrict newcpu, int hint,
                                void const *__restrict buf, size_t bufsize)
#endif
{
 struct ktask *waketask; kerrno_t error; int did_copy;
 KTASK_CRIT_MARK
 assertf((hint&0xFFF0)==0,"ksignal_vsendoneex doesn't accept flag modifiers: %x",hint);
 if ((waketask = _ksignal_popone_andunlock_c(self)) == NULL) return KS_EMPTY;
 /* Make sure that we're not accidentally using the exitcode of an already terminated task! */
 ktask_lock(waketask,KTASK_LOCK_SIGVAL);
 if ((did_copy = (waketask->t_sigval != NULL))) {
#ifdef USER
  struct ktranslator src_translator,dst_translator;
  __user void *user_dst; __size_t kernel_maxsize;
  __kernel void *kernel_src,*kernel_dst;
  error = ktranslator_init(&src_translator,ktask_self());
  if __unlikely(KE_ISERR(error)) {
err_sigval:
   __evalexpr(ktask_reschedule_ex(waketask,newcpu,hint|
                                  KTASK_RESCHEDULE_HINTFLAG_INHERITREF|
                                  KTASK_RESCHEDULE_HINTFLAG_UNLOCKSIGVAL));
   return error;
  }
  error = ktranslator_init(&dst_translator,waketask);
  if __unlikely(KE_ISERR(error)) { ktranslator_quit(&src_translator); goto err_sigval; }
  user_dst = waketask->t_sigval;
  while ((kernel_src = ktranslator_exec(&src_translator,buf,bufsize,&kernel_maxsize,0)) != NULL &&
         (kernel_dst = ktranslator_exec(&dst_translator,user_dst,kernel_maxsize,&kernel_maxsize,1)) != NULL) {
   memcpy(kernel_dst,kernel_src,kernel_maxsize);
   if ((bufsize -= kernel_maxsize) == 0) break;
   *(uintptr_t *)&buf      += kernel_maxsize;
   *(uintptr_t *)&user_dst += kernel_maxsize;
  }
  ktranslator_quit(&dst_translator);
  ktranslator_quit(&src_translator);
  if __unlikely(bufsize) { error = KE_FAULT; goto err_sigval; }
#else
  memcpy(waketask->t_sigval,buf,bufsize);
#endif
  /* By setting the sigval to NULL, we make sure that only the first
   * signal is able to post a value (in situations where more the
   * receiving task is waiting for more than one signal)
   * NOTE: The fact that we set this to NULL is also detected
   *       by the task we're currently waking, allowing them
   *       to distinguish between wake-ups with an associated
   *       value, as well as those without. */
  waketask->t_sigval = NULL;
 }
 /* Have 'ktask_reschedule_ex' inherit the reference returned by 'ksignal_popone' */
 error = ktask_reschedule_ex(waketask,newcpu,hint|
                             KTASK_RESCHEDULE_HINTFLAG_INHERITREF|
                             KTASK_RESCHEDULE_HINTFLAG_UNLOCKSIGVAL);
 assert(!ktask_islocked(waketask,KTASK_LOCK_SIGVAL));
 /* Return KS_NODATA if we did manage to wake a task, but weren't able to post a value. */
 return (error == KE_OK && !did_copy) ? KS_NODATA : error;
}

#ifdef USER
kerrno_t
ksignal_vusendn(struct ksignal *__restrict self,
                size_t n_tasks, size_t *__restrict woken_tasks,
                __user void const *__restrict buf, size_t bufsize)
#else
kerrno_t
ksignal_vsendn(struct ksignal *__restrict self,
               size_t n_tasks, size_t *__restrict woken_tasks,
               void const *__restrict buf, size_t bufsize)
#endif
{
 size_t i; kerrno_t error;
 kassert_ksignal(self);
 for (i = 0; i != n_tasks; ++i) {
#ifdef USER
  error = ksignal_vusendone(self,buf,bufsize);
#else
  error = ksignal_vsendone(self,buf,bufsize);
#endif
  if __unlikely(KE_ISERR(error)) goto end;
 }
 error = KE_OK;
end:
 if (woken_tasks) *woken_tasks = i;
 return error;
}


#ifdef USER
__crit size_t
_ksignal_vusendall_andunlock_c(struct ksignal *__restrict self,
                               __user void const *__restrict buf, size_t bufsize)
#else
__crit size_t
_ksignal_vsendall_andunlock_c(struct ksignal *__restrict self,
                              void const *__restrict buf, size_t bufsize)
#endif
{
 size_t count = 0;
 KTASK_CRIT_MARK
 kassert_ksignal(self);
 ksignal_unlock_c(self,KSIGNAL_LOCK_WAIT);
 /* TODO: This only works for the case where
  *       this function is called from 'ksignal_close'
  *       When adding new tasks to the signal is still
  *       allowed, this is unsafe by waking tasks
  *       that may have been added after the initial
  *       call to this function.
  * TODO: To do this correct, we'd have to collect
  *       all tasks that we're supposed to wake,
  *       then unlink them, and then, finally
  *       wake them one-by-one.
  *       NOTE: Though if that doesn't work, due to
  *             a failure in allocating the necessary
  *             buffer, we should count how many
  *             are within our signal, and then
  *             call 'ksignal_sendone' that many times.
  *          >> That should still be OK for almost
  *             all situations. */
#ifdef USER
 while (ksignal_vusendone(self,buf,bufsize) != KS_EMPTY) ++count;
#else
 while (ksignal_vsendone(self,buf,bufsize) != KS_EMPTY) ++count;
#endif
 return count;
}

#ifdef USER
#undef USER
#endif

#ifdef __INTELLISENSE__
__DECL_END
#endif

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
#ifndef __KOS_KERNEL_SCHED_PROC_TASKS_C_INL__
#define __KOS_KERNEL_SCHED_PROC_TASKS_C_INL__ 1

#include <kos/kernel/proc.h>
#include <stddef.h>

__DECL_BEGIN

kerrno_t kproc_addtask(struct kproc *__restrict self,
                       struct ktask *__restrict task) {
 kerrno_t error; kassert_kproc(self); kassert_ktask(task);
 if __unlikely(KE_ISERR(error = kproc_lock(self,KPROC_LOCK_THREADS))) return error;
 task->t_tid = ktasklist_insert(&self->p_threads,task);
 kproc_unlock(self,KPROC_LOCK_THREADS);
 if __unlikely(task->t_tid == (size_t)-1) return KE_NOMEM;
 return KE_OK;
}
void kproc_deltask(struct kproc *__restrict self,
                   struct ktask *__restrict task) {
 kerrno_t error; kassert_kproc(self);
 kassert_object(task,KOBJECT_MAGIC_TASK);
 error = kproc_lock(self,KPROC_LOCK_THREADS);
 if (ktasklist_erase(&self->p_threads,task->t_tid)) {
  if (!self->p_threads.t_taska) kproc_close(self);
 }
 if __likely(KE_ISOK(error)) kproc_unlock(self,KPROC_LOCK_THREADS);
}

__crit __ref struct ktask *
kproc_getthread(struct kproc const *__restrict self,
                __ktid_t tid) {
 __ref struct ktask *result;
 KTASK_CRIT_MARK
 kassert_kproc(self);
 if __unlikely(KE_ISERR(kproc_lock((struct kproc *)self,KPROC_LOCK_THREADS))) return NULL;
 if __unlikely(tid >= self->p_threads.t_taska) result = NULL;
 else if __likely((result = self->p_threads.t_taskv[tid]) != NULL) {
  if __unlikely(KE_ISERR(ktask_tryincref(result))) result = NULL;
 }
 kproc_unlock((struct kproc *)self,KPROC_LOCK_THREADS);
 return result;
}
__crit int
kproc_enumthreads_c(struct kproc const *__restrict self,
                    int (*callback)(struct ktask *thread,
                                    void *closure),
                    void *closure) {
 __ref struct ktask *task; __ktid_t tid; int error;
 KTASK_CRIT_MARK
 kassert_kproc(self);
 if __unlikely(KE_ISERR(kproc_lock((struct kproc *)self,KPROC_LOCK_THREADS))) return 0;
 for (tid = 0; tid < self->p_threads.t_taska; ++tid) {
  if ((task = self->p_threads.t_taskv[tid]) != NULL &&
      KE_ISOK(ktask_tryincref(task))) {
   kproc_unlock((struct kproc *)self,KPROC_LOCK_THREADS);
   error = (*callback)(task,closure);
   ktask_decref(task);
   if __unlikely(error != 0) return error;
   if __unlikely(KE_ISERR(kproc_lock((struct kproc *)self,KPROC_LOCK_THREADS))) return 0;
  }
 }
 kproc_unlock((struct kproc *)self,KPROC_LOCK_THREADS);
 return 0;
}


__crit kerrno_t
kproc_alloctls_c(struct kproc *__restrict self,
                 __ktls_t *__restrict result) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kproc(self);
 error = kproc_lock(self,KPROC_LOCK_TLSMAN);
 if __unlikely(KE_ISERR(error)) return error;
 error = ktlsman_alloctls(&self->p_tlsman,result);
 kproc_unlock(self,KPROC_LOCK_TLSMAN);
 return error;
}
__crit void kproc_freetls_c(struct kproc *__restrict self,
                            __ktls_t slot) {
 struct ktask **iter,**end,*elem;
 kerrno_t lockerror;
 KTASK_CRIT_MARK
 kassert_kproc(self);
 lockerror = kmmutex_locks(&self->p_lock,KPROC_LOCK_TLSMAN|KPROC_LOCK_THREADS);
 // NOTE: A closed process should not have any TLS slots, but lets be safe...
 ktlsman_freetls(&self->p_tlsman,slot);
 end = (iter = self->p_threads.t_taskv)+self->p_threads.t_taska;
 for (; iter != end; ++iter) if ((elem = *iter) != NULL) {
  ktask_lock(elem,KTASK_LOCK_TLS);
  ktlspt_del(&elem->t_tls,slot);
  ktask_unlock(elem,KTASK_LOCK_TLS);
 }
 if __likely(KE_ISOK(lockerror)) {
  kmmutex_unlocks(&self->p_lock,KPROC_LOCK_TLSMAN|KPROC_LOCK_THREADS);
 }
}


__DECL_END

#endif /* !__KOS_KERNEL_SCHED_PROC_TASKS_C_INL__ */

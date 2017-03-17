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
#ifndef __KOS_KERNEL_SCHED_TASK_RECURSIVE_C_INL__
#define __KOS_KERNEL_SCHED_TASK_RECURSIVE_C_INL__ 1

#include <kos/compiler.h>
#include <kos/kernel/task.h>
#include <stdio.h>

__DECL_BEGIN

kerrno_t
__ktask_suspend_kr_impl(struct ktask *__restrict self) {
 struct ktask **iter,**end,*child;
 kerrno_t error = KE_OK;
 ktask_lock(self,KTASK_LOCK_CHILDREN);
 end = (iter = self->t_children.t_taskv)+self->t_children.t_taska;
 for (; iter != end; ++iter) {
  if ((child = *iter) != NULL) {
   error = __ktask_suspend_kr_impl(child);
   if __unlikely(KE_ISERR(error)) {
resumeall:
    while (iter != self->t_children.t_taskv) {
     __evalexpr(ktask_resume(*--iter));
    }
    ktask_unlock(self,KTASK_LOCK_CHILDREN);
    return error;
   }
  }
 }
 ktask_unlock(self,KTASK_LOCK_CHILDREN);
 if __unlikely(KE_ISERR(error = ktask_suspend_k(self))) {
  ktask_lock(self,KTASK_LOCK_CHILDREN);
  iter = self->t_children.t_taskv+self->t_children.t_taska;
  goto resumeall;
 }
 return KE_OK;
}
kerrno_t
__ktask_resume_kr_impl(struct ktask *__restrict self) {
 struct ktask **iter,**end,*child;
 kerrno_t error = KE_OK;
 ktask_lock(self,KTASK_LOCK_CHILDREN);
 end = (iter = self->t_children.t_taskv)+self->t_children.t_taska;
 for (; iter != end; ++iter) {
  if ((child = *iter) != NULL) {
   error = __ktask_resume_kr_impl(child);
   if __unlikely(KE_ISERR(error)) {
resumeall:
    while (iter != self->t_children.t_taskv) {
     __evalexpr(ktask_suspend_k(*--iter));
    }
    ktask_unlock(self,KTASK_LOCK_CHILDREN);
    return error;
   }
  }
 }
 ktask_unlock(self,KTASK_LOCK_CHILDREN);
 if __unlikely(KE_ISERR(error = ktask_resume_k(self))) {
  ktask_lock(self,KTASK_LOCK_CHILDREN);
  iter = self->t_children.t_taskv+self->t_children.t_taska;
  goto resumeall;
 }
 return KE_OK;
}

kerrno_t
ktask_suspend_kr(struct ktask *__restrict self) {
 kerrno_t error;
 NOIRQ_BEGIN;
 error = __ktask_suspend_kr_impl(self);
 NOIRQ_END
 return error;
}
kerrno_t
ktask_resume_kr(struct ktask *__restrict self) {
 kerrno_t error;
 NOIRQ_BEGIN;
 error = __ktask_resume_kr_impl(self);
 NOIRQ_END
 return error;
}

#if 0
static void
ktask_terminatechildren(struct ktask *__restrict self, void *exitcode) {
 struct ktask *child; size_t i; int found_running;
 ktask_lock(self,KTASK_LOCK_CHILDREN);
 while (self->t_children.t_taska) {
  found_running = 0;
  for (i = 0; i < self->t_children.t_taska; ++i) {
   if ((child = self->t_children.t_taskv[i]) != NULL &&
       __likely(KE_ISOK(ktask_incref(child)))) {
    kerrno_t error;
    // TODO: Error handling when can't incref child
    ktask_unlock(self,KTASK_LOCK_CHILDREN);
    error = _ktask_terminate_kr(child,exitcode);
    if (error == KS_BLOCKING) {
     // Join a child task currently executing a critical operation
     if (child != ktask_self()) {
      found_running = 1;
      ktask_join(child,NULL);
     }
    } else if (error != KE_DESTROYED) {
     assert(KE_ISOK(error));
     found_running = 1;
    }
    ktask_decref(child);
    ktask_lock(self,KTASK_LOCK_CHILDREN);
   }
  }
  if (!found_running) break;
 }
 /* TODO: We must not unlock 'KTASK_LOCK_CHILDREN'
  *       here to prevent new children from being added. */
 ktask_unlock(self,KTASK_LOCK_CHILDREN);
}

kerrno_t
_ktask_terminate_kr(struct ktask *__restrict self, void *exitcode) {
 ktask_terminatechildren(self,exitcode);
 return _ktask_terminate_k(self,exitcode);
}
kerrno_t
ktask_terminate_kr(struct ktask *__restrict self, void *exitcode) {
 ktask_terminatechildren(self,exitcode);
 return self == ktask_self()
  ? _ktask_terminate_k(self,exitcode)
  :  ktask_terminate_k(self,exitcode);
}
#endif

__DECL_END

#endif /* !__KOS_KERNEL_SCHED_TASK_RECURSIVE_C_INL__ */

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
#ifndef __KOS_KERNEL_SCHED_TASK_ATTR_C_INL__
#define __KOS_KERNEL_SCHED_TASK_ATTR_C_INL__ 1

#include <kos/compiler.h>
#include <kos/kernel/task.h>
#include <kos/kernel/proc.h>
#include <kos/kernel/util/string.h>
#include <sys/types.h>

__DECL_BEGIN

kerrno_t
ktask_user_getattr_k(struct ktask *__restrict self, kattr_t attr,
                     __user void *__restrict buf, size_t bufsize,
                     __kernel size_t *__restrict reqsize) {
 switch (attr) {
  {
   char const *name;
  case KATTR_GENERIC_NAME:
   name = ktask_getname(self);
   if __likely(name) {
    if (reqsize) *reqsize = (strlen(name)+1)*sizeof(char);
    if __unlikely(user_strncpy((char *)buf,name,bufsize)) return KE_FAULT;
   } else {
    ssize_t error;
    /* TODO: filename of executable? */
    error = user_snprintf((char *)buf,bufsize,reqsize,
                          "thread[%Iu|%Iu]",
                          ktask_getparid(self),
                          ktask_gettid(self));
    if (error < 0) return KE_FAULT;
   }
   return KE_OK;
  }
 }
 /* TODO: Other attributes, such as priorities, etc.? */
 return KE_NOSYS;
}
kerrno_t
ktask_user_setattr_k(struct ktask *__restrict self, kattr_t attr,
                     __user void const *__restrict buf, size_t bufsize) {
 /* TODO: Set task name. */
 /* TODO: Other attributes, such as priorities, etc.? */
 return KE_NOSYS;
}
kerrno_t
ktask_user_getattr(struct ktask *__restrict self, kattr_t attr,
                   __user void *__restrict buf, size_t bufsize,
                   __kernel size_t *__restrict reqsize) {
 if __unlikely(!ktask_accessgp(self)) return KE_ACCES;
 return ktask_user_getattr_k(self,attr,buf,bufsize,reqsize);
}
kerrno_t
ktask_user_setattr(struct ktask *__restrict self, kattr_t attr,
                   __user void const *__restrict buf, size_t bufsize) {
 if __unlikely(!ktask_accesssp(self)) return KE_ACCES;
 return ktask_user_setattr_k(self,attr,buf,bufsize);
}

__DECL_END

#endif /* !__KOS_KERNEL_SCHED_TASK_ATTR_C_INL__ */

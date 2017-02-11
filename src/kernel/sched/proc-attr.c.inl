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
#ifndef __KOS_KERNEL_SCHED_PROC_ATTR_C_INL__
#define __KOS_KERNEL_SCHED_PROC_ATTR_C_INL__ 1

#include <kos/kernel/proc.h>
#include <kos/kernel/task.h>
#include <kos/errno.h>

__DECL_BEGIN

__crit kerrno_t
kproc_getattr_c(struct kproc *__restrict self, kattr_t attr,
                void *__restrict buf, __size_t bufsize,
                __size_t *__restrict reqsize) {
 struct ktask *root_task; kerrno_t error;
 KTASK_CRIT_MARK
 if __unlikely((root_task = kproc_getroottask(self)) == NULL) return KE_DESTROYED;
 error = ktask_getattr(root_task,attr,buf,bufsize,reqsize);
 ktask_decref(root_task);
 return error;
}
__crit kerrno_t
kproc_setattr_c(struct kproc *__restrict self, kattr_t attr,
                void const *__restrict buf, __size_t bufsize) {
 struct ktask *root_task; kerrno_t error;
 KTASK_CRIT_MARK
 if __unlikely((root_task = kproc_getroottask(self)) == NULL) return KE_DESTROYED;
 error = ktask_setattr(root_task,attr,buf,bufsize);
 ktask_decref(root_task);
 return error;
}

__DECL_END

#endif /* !__KOS_KERNEL_SCHED_PROC_ATTR_C_INL__ */

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
#ifndef __GOS_KERNEL_KTASK_JOIN_C_INL__
#define __GOS_KERNEL_KTASK_JOIN_C_INL__ 1

#include <assert.h>
#include <gos/compiler.h>
#include <gos/kernel/errno.h>
#include <gos/kernel/ktask.h>
#include <gos/kernel/pageframe.h>

__DECL_BEGIN

kerrno_t ktask_join(ktask_t *self, void **retval) {
 kassert_obj_rw(self);
 // Really crude implementation
 // TODO
 while (!ktask_isterminated(self)) {
  if (ktask_tryyield() != KE_OK) return KE_WOULDBLOCK;
  ktask_yield();
 }
 if (retval) *retval = (void *)self->t_regs.eax;
 return KE_OK;
}
kerrno_t ktask_tryjoin(ktask_t *self, void **retval) {
 kassert_obj_rw(self);
 if (!ktask_isterminated(self)) return KE_TIMEDOUT;
 if (retval) *retval = (void *)self->t_regs.eax;
 return KE_OK;
}
kerrno_t ktask_timedjoin(
 ktask_t *self, struct timespec const *timeout, void **retval) {
 // TODO
 return ktask_join(self,retval);
}


__DECL_END

#endif /* !__GOS_KERNEL_KTASK_JOIN_C_INL__ */

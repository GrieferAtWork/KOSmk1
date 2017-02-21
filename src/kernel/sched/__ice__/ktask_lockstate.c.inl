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
#ifndef __GOS_KERNEL_KTASK_LOCKSTATE_C_INL__
#define __GOS_KERNEL_KTASK_LOCKSTATE_C_INL__ 1

#include <assert.h>
#include <gos/compiler.h>
#include <gos/kernel/klock.h>
#include <gos/kernel/ktask.h>
#include <gos/kernel/pageframe.h>

__DECL_BEGIN


#ifndef __INTELLISENSE__
#define TIMEOUT
#include "ktask_lockstate_enter.impl.c.inl"
#include "ktask_lockstate_enter.impl.c.inl"
#endif

void __ktask_leave_lockstate_and_unlock(ktask_t *task_self) {
 if (task_self->t_suspended != 0) {
  task_self->t_flags &= ~(0x0004);
  _ktask_unlock(task_self);
 } else {
  task_self->t_flags &= ~(KTASK_FLAG_WAITING);
  __ktask_chain_add_and_unlock(task_self);
 }
}

__DECL_END

#endif /* !__GOS_KERNEL_KTASK_LOCKSTATE_C_INL__ */

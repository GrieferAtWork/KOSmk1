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
#ifndef __GOS_KERNEL_KTASK_STATE_C_INL__
#define __GOS_KERNEL_KTASK_STATE_C_INL__ 1

#include <assert.h>
#include <gos/compiler.h>
#include <gos/kernel/katomic.h>
#include <gos/kernel/pageframe.h>
#include <gos/kernel/ktask.h>

__DECL_BEGIN

kerrno_t ktask_stackpush_and_unlock(ktask_t *self, void const *p, size_t s) {
 kerrno_t error; uintptr_t dest;
 kassert_obj_rw(self); kassert_mem_ro(p,s);
 if (!ktask_issuspended(self)) { error = KE_RUNNING; goto end; }
 if (ktask_isterminated(self)) { error = KE_EXITED; goto end; }
 dest = (uintptr_t)self->t_regs.esp;
 if ((s > dest)                                      // Moving ESP would underflow the stack
  || (dest > (uintptr_t)ktask_stackend(self))        // ESP is already out-of-bounds (above)
  || (dest -= s) < (uintptr_t)ktask_stackbegin(self) // The new ESP is out-of-bounds (below)
  ) { error = KE_RANGE; goto end; }
 memcpy((void *)dest,p,s);
 self->t_regs.esp = (__u32)dest;
 error = KE_OK;
end:
 _ktask_unlock(self);
 return error;
}

kerrno_t ktask_stackpop_and_unlock(ktask_t *self, void *p, size_t s) {
 kerrno_t error; uintptr_t dest,newdest;
 kassert_obj_rw(self); kassert_mem_rw(p,s);
 if (!ktask_issuspended(self)) { error = KE_RUNNING; goto end; }
 if (ktask_isterminated(self)) { error = KE_EXITED; goto end; }
 dest = (uintptr_t)self->t_regs.esp;
 if (dest < (uintptr_t)ktask_stackbegin(self)) {erange: error = KE_RANGE; goto end; }
 newdest = dest+s;
 if (newdest < dest || newdest > (uintptr_t)
     ktask_stackend(self)) goto erange;
 memcpy(p,(void *)dest,s);
 self->t_regs.esp = (__u32)newdest;
 error = KE_OK;
end:
 _ktask_unlock(self);
 return error;
}

kerrno_t ktask_getstate_and_unlock(ktask_t *self, irq_registers_t *state) {
 kerrno_t error; uintptr_t dest,newdest;
 kassert_obj_rw(self); kassert_obj_rw(state);
 if (!ktask_issuspended(self)) { error = KE_RUNNING; goto end; }
 if (ktask_isterminated(self)) { error = KE_EXITED; goto end; }
 memcpy(state,&self->t_regs,sizeof(self->t_regs));
 error = KE_OK;
end:
 _ktask_unlock(self);
 return error;
}
kerrno_t ktask_setstate_and_unlock(ktask_t *self, irq_registers_t const *state) {
 kerrno_t error; uintptr_t dest,newdest;
 kassert_obj_rw(self); kassert_obj_ro(state);
 if (!ktask_issuspended(self)) { error = KE_RUNNING; goto end; }
 if (ktask_isterminated(self)) { error = KE_EXITED; goto end; }
 memcpy(&self->t_regs,state,sizeof(self->t_regs));
 error = KE_OK;
end:
 _ktask_unlock(self);
 return error;
}


__DECL_END

#endif /* !__GOS_KERNEL_KTASK_STATE_C_INL__ */

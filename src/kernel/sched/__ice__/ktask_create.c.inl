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
#ifndef __GOS_KERNEL_KTASK_CREATE_C_INL__
#define __GOS_KERNEL_KTASK_CREATE_C_INL__ 1

#include <assert.h>
#include <gos/compiler.h>
#include <gos/kernel/katomic.h>
#include <gos/kernel/ktask.h>
#include <gos/kernel/pageframe.h>
#include <sys/types.h>

__DECL_BEGIN

// Stack header of the task (resembles __cdecl functions)
// http://unixwiz.net/techtips/win32-callconv-asm.html
struct cdecl_stack_header {
 // Local variables of the task main function will grow here
 void *caller_baseptr;
 void *return_address;
 void *closure_value;
 // More arguments would go here...
};

extern void __ktask_exitnormal(void); // Implemented in "ktask_exit_normal.S"

kerrno_t ktask_create_ex(
 ktask_t **newtask, ktask_t *parent, void *(*main)(void *), void *closure,
 ktask_flag_t flags, __size_t stack_align, __size_t stack_size,
 ktask_priority_t priority, kpage_directory_t *pd) {
 kerrno_t error; void *allocated_stack;
 allocated_stack = memalign(stack_align,stack_size);
 if __unlikely(!allocated_stack) return KE_NOMEM;
 error = ktask_create_with_allocated_stack(newtask,parent,main,closure,flags,
                                           allocated_stack,stack_size,priority,pd);
 if __unlikely(error != KE_OK) free(allocated_stack);
 return error;
}

kerrno_t ktask_create_with_allocated_stack(
 ktask_t **newtask, ktask_t *parent, void *(*main)(void *), void *closure,
 ktask_flag_t flags, void *stack, __size_t stack_size,
 ktask_priority_t priority, kpage_directory_t *pd) {
 kerrno_t error; struct cdecl_stack_header *callback_header;
 // Checked later:
 // assert(newtask); kassert_obj_rw(parent);
 // assert(main); kassert_obj_rw(pd);
 if (stack_size < sizeof(struct cdecl_stack_header)) return KE_RANGE;
 // Fill the stack with initial arguments to support a closure and normal thread exit
 callback_header = ((struct cdecl_stack_header *)((uintptr_t)stack+stack_size))-1;
 callback_header->caller_baseptr = NULL; // ? (I guess this is meant for debugging...)
 callback_header->return_address = (void *)&__ktask_exitnormal;
 callback_header->closure_value  = closure;
 return ktask_create_with_stack(newtask,parent,(void *(*)())main,flags,
                                stack,stack_size,(void *)callback_header,priority,pd);
}

kerrno_t ktask_create_with_stack(
 ktask_t **newtask, ktask_t *parent, void *(*main)(),
 ktask_flag_t flags, void *stack, __size_t stack_size,
 void *esp, ktask_priority_t priority, kpage_directory_t *pd) {
 ktask_t *resulttask; kerrno_t error;
 assert(newtask); kassert_obj_rw(parent);
 assert(main); kassert_obj_rw(pd);
 assertf((uintptr_t)esp >= (uintptr_t)stack &&
         (uintptr_t)esp <= (uintptr_t)stack+stack_size,
         "Initial ESP is outside the valid range");
 // Check parameters
 if (priority < KTASK_PRIORITY_MIN ||
    (priority > KTASK_PRIORITY_MAX &&
     priority != KTASK_PRIORITY_REALTIME)
     ) return KE_INVAL;
 if ((flags&KTASK_FLAG_SUSPENDED)!=0) interrupt_disable();
 _ktask_lock(parent);
 // Allocate the new child task
 resulttask = __ktask_child_alloc(parent);
 if __unlikely(!resulttask) return KE_NOMEM;
 assert(resulttask->t_parent == parent);
 error = kperms_initfrom(&resulttask->t_perms,&parent->t_perms);
 if __unlikely(error != KE_OK) {
  __ktask_child_free(parent,resulttask);
  return error;
 }
 resulttask->t_pd         = pd;
 resulttask->t_setprio    = priority;
 resulttask->t_prio       = priority;
 resulttask->t_regs.esp   = (__u32)esp;
 resulttask->t_regs.eip   = (__u32)main;
 resulttask->t_stackbegin = stack;
 resulttask->t_stackend   = (void *)((uintptr_t)stack+stack_size);

 if ((flags&KTASK_FLAG_SUSPENDED)!=0) {
  resulttask->t_flags = (flags&KTASK_FLAG_MASKCTOR)|__KTASK_FLAG_LOCKED;
  resulttask->t_suspended = 0;
  _ktask_unlock(parent);
  __ktask_chain_add_and_unlock(resulttask);
 } else {
  resulttask->t_flags = (flags&KTASK_FLAG_MASKCTOR);
  resulttask->t_suspended = 1;
  _ktask_unlock(parent);
 }
 return KE_OK;
}



__DECL_END

#endif /* !__GOS_KERNEL_KTASK_CREATE_C_INL__ */

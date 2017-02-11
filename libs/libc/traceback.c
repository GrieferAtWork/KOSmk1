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
#ifndef __TRACEBACK_C__
#define __TRACEBACK_C__ 1

#include <assert.h>
#include <kos/compiler.h>
#include <kos/config.h>
#include <kos/kernel/debug.h>
#include <kos/errno.h>
#include <malloc.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <traceback.h>
#ifdef __KERNEL__
#include <kos/kernel/task.h>
#include <kos/kernel/serial.h>
#else /* __KERNEL__ */
#include <kos/syscall.h>
#include <kos/syscallno.h>
#include <stdio.h>
#endif /* __KERNEL__ */

__DECL_BEGIN


#ifdef __KERNEL__
#define PRINT(...) serial_printf(SERIAL_01,__VA_ARGS__)
#else
#define PRINT(...) fprintf(stderr,__VA_ARGS__)
#endif

static int debug_print_traceback_entry(void const *addr, void const *frame_address,
                                       size_t index, void *__unused(closure)) {
 PRINT("#!$ addr2line(%p) '{file}({line}) : {func} : [%Ix][%p] : %p'\n",
       ((uintptr_t)addr)-1,index,frame_address,addr);
 return 0;
}

__public int _traceback_errorentry_d(int error, void const *arg,
                                     void *__unused(closure)) {
 char const *reason;
 switch (error) {
  case KDEBUG_STACKERROR_EIP:   reason = "Invalid instruction pointer"; error = 0; break;
  case KDEBUG_STACKERROR_LOOP:  reason = "Stackframes are looping"; break;
  case KDEBUG_STACKERROR_FRAME: reason = "A stackframe points to an invalid address"; break;
  default: return 0;
 }
 PRINT("!!! CORRUPTED STACK !!! - %s (%p)\n",reason,arg);
 return error;
}




// Derived from documentation found here:
// >> http://unixwiz.net/techtips/win32-callconv-asm.html
struct stackframe {
 // Local vars...
 struct stackframe *caller;
 void              *eip;
 // Parameters...
};

#define SAFE_USERSPACE_TRACEBACKS 1

#ifndef __KERNEL__
#ifndef __WANT_KERNEL_MEM_VALIDATE
#if SAFE_USERSPACE_TRACEBACKS
#undef kmem_validate
#undef kmem_validatebyte
__local _syscall2(kerrno_t,kmem_validate,void const *__restrict,addr,__size_t,bytes);
#define kmem_validatebyte(p) kmem_validate(p,1)
#endif /* SAFE_USERSPACE_TRACEBACKS */
#endif
#endif

static __noinline int
debug_dostackwalk(struct stackframe *start, _ptraceback_stackwalker_d callback,
                  _ptraceback_stackerror_d handle_error, void *closure, size_t skip) {
 struct stackframe *frame,*next,*check; int temp;
 size_t didskip = 0,index = 0; frame = start;
 while (frame) {
  kerrno_t error = kmem_validate(frame,sizeof(struct stackframe));
  // Validate the stack frame address
  if (KE_ISERR(error)) {
   if (!handle_error) return KDEBUG_STACKERROR_FRAME;
   temp = (*handle_error)(KDEBUG_STACKERROR_FRAME,(void const *)frame,closure);
#ifdef __KERNEL__
   if (temp != 0 && error != KE_INVAL) return temp;
#else
   if (temp != 0) return temp;
#endif
  }
  // Validate the instruction pointer
  if (KE_ISERR(kmem_validatebyte((byte_t const *)(uintptr_t)frame->eip))) {
   if (!handle_error) return KDEBUG_STACKERROR_EIP;
   temp = (*handle_error)(KDEBUG_STACKERROR_EIP,(void const *)(uintptr_t)frame->eip,closure);
   if (temp != 0) return temp;
  }
  // Call the stack entry enumerator
  if (didskip < skip) ++didskip;
  else if (callback) {
   temp = (*callback)(frame->eip,frame,index++,closure);
   if (temp != 0) return temp;
  }
  next = frame->caller;
  // Ensure that the frame hasn't already been enumerated (looping stack)
  check = start;
  for (;;) {
   if (check == next) {
    // The stackframes are recursive
    if (!handle_error) return KDEBUG_STACKERROR_LOOP;
    temp = (*handle_error)(KDEBUG_STACKERROR_LOOP,(void const *)next,closure);
    if (temp != 0) return temp;
    break;
   }
   if (check == frame) break;
   check = check->caller;
  }
  frame = next;
 }
 return 0;
}
__public int _walktraceback_d(_ptraceback_stackwalker_d callback,
                              _ptraceback_stackerror_d handle_error,
                              void *closure, size_t skip) {
 struct stackframe *stack;
 __asm_volatile__("movl %%ebp, %0" : "=r" (stack));
 return debug_dostackwalk(stack,callback,handle_error,closure,skip);
}
__public int _walktracebackebp_d(void const *ebp,
                                 _ptraceback_stackwalker_d callback,
                                 _ptraceback_stackerror_d handle_error,
                                 void *closure, size_t skip) {
 return debug_dostackwalk((struct stackframe *)ebp,callback,handle_error,closure,skip);
}

__public void _printtraceback_d(void) { _printtracebackex_d(1); }
__public void _printtracebackebp_d(void const *ebp) { _printtracebackebpex_d(ebp,1); }
__public void _printtracebackex_d(size_t skip) { _walktraceback_d(&debug_print_traceback_entry,&_traceback_errorentry_d,NULL,skip+1); }
__public void _printtracebackebpex_d(void const *ebp, size_t skip) { _walktracebackebp_d(ebp,&debug_print_traceback_entry,&_traceback_errorentry_d,NULL,skip); }
#ifdef __KERNEL__
int _walktracebacktask_d(struct ktask *task, _ptraceback_stackwalker_d callback,
                        _ptraceback_stackerror_d handle_error, void *closure, size_t skip) {
 struct stackframe *stack;
 if (task == ktask_self()) _walktraceback_d(callback,handle_error,closure,skip+1);
 stack = (struct stackframe *)ktask_getkernelesp(task);
 return debug_dostackwalk(stack,callback,handle_error,closure,skip);
}
void _printtracebacktask_d(struct ktask *task) {
 _printtracebacktaskex_d(task,1);
}
void _printtracebacktaskex_d(struct ktask *task, size_t skip) {
 if (task == ktask_self()) _printtracebackex_d(skip+1);
 _walktracebacktask_d(task,
                     &debug_print_traceback_entry,
                     &_traceback_errorentry_d,
                     NULL,skip+1);
}
#endif /* __KERNEL__ */

struct dtracebackentry {
 void const *tb_eip;
 void const *tb_esp;
};
struct dtraceback {
 size_t                 tb_entryc;
 struct dtracebackentry tb_entryv[1024];
};

static int tbcapture_walkcount(void const *__restrict __unused(instruction_pointer),
                               void const *__restrict __unused(frame_address),
                               size_t frame_index, size_t *closure) {
 if (frame_index >= *closure) *closure = frame_index+1;
 return 0;
}
static int tbcapture_walkcollect(void const *__restrict instruction_pointer,
                                 void const *__restrict frame_address,
                                 size_t frame_index, struct dtraceback *tb) {
 assertf(frame_index < tb->tb_entryc,"%Iu >= %Iu",frame_index,tb->tb_entryc);
 tb->tb_entryv[frame_index].tb_eip = instruction_pointer;
 tb->tb_entryv[frame_index].tb_esp = frame_address;
 return 0;
}

__public struct dtraceback *dtraceback_capture(void) {
 return dtraceback_captureex(1);
}
__public struct dtraceback *dtraceback_captureex(size_t skip) {
 struct dtraceback *result; size_t entrycount = 0;
 _walktraceback_d((_ptraceback_stackwalker_d)&tbcapture_walkcount,NULL,&entrycount,skip+1);
 result = (struct dtraceback *)malloc(offsetof(struct dtraceback,tb_entryv)+
                                      entrycount*sizeof(struct dtracebackentry));
 if __unlikely(!result) return NULL;
 result->tb_entryc = entrycount;
 _walktraceback_d((_ptraceback_stackwalker_d)&tbcapture_walkcollect,NULL,result,skip+1);
 return result;
}
__public void dtraceback_free(struct dtraceback *__restrict self) { free(self); }
__public int dtraceback_walk(struct dtraceback const *__restrict self,
                             _ptraceback_stackwalker_d callback,
                             void *closure) {
 struct dtracebackentry const *elemv; size_t i; int error;
 if __unlikely(!self) return 0; // Ingore NULL tracebacks
 kassertmem(self,offsetof(struct dtraceback,tb_entryv));
 kassertmem(self->tb_entryv,self->tb_entryc*sizeof(struct dtracebackentry));
 kassertbyte(callback); elemv = self->tb_entryv;
 for (i = 0; i < self->tb_entryc; ++i) {
  error = (*callback)(elemv[i].tb_eip,
                      elemv[i].tb_esp,
                      i,closure);
  if __unlikely(error != 0) return error;
 }
 return 0;
}
__public void dtraceback_print(struct dtraceback const *__restrict self) {
 if (self) dtraceback_walk(self,&debug_print_traceback_entry,NULL);
}


__DECL_END

#endif /* !__TRACEBACK_C__ */

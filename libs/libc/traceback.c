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
#include <string.h>
#include <traceback.h>
#include <format-printer.h>
#ifdef __KERNEL__
#include <kos/kernel/fs/file.h>
#include <kos/kernel/serial.h>
#include <kos/kernel/tty.h>
#else /* __KERNEL__ */
#include <kos/syslog.h>
#include <kos/syscall.h>
#include <kos/syscallno.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <mod.h>
#endif /* __KERNEL__ */

__DECL_BEGIN


#ifdef __KERNEL__
#define PRINT(...) serial_printf(SERIAL_01,__VA_ARGS__)
#else
#define PRINT(...) fprintf(stderr,__VA_ARGS__)
#endif

static int
tbprint_callback(char const *data,
                 size_t max_data,
                 void *closure) {
 if (closure) {
#ifdef __KERNEL__
  kerrno_t error = kfile_kernel_writeall((struct kfile *)closure,data,
                                         strnlen(data,max_data)*sizeof(char));
  return KE_ISERR(error) ? error : 0;
#else
  max_data = strnlen(data,max_data);
  __evalexpr(fwrite(data,sizeof(char),max_data,(FILE *)closure));
#endif
 } else {
#ifdef __KERNEL__
  tty_printn(data,max_data);
  serial_printn(SERIAL_01,data,max_data);
#else
  k_syslog(KLOG_ERROR,data,max_data);
  if (write(STDERR_FILENO,data,strnlen(data,max_data)*sizeof(char)) < 0) return -1;
#endif
 }
 return 0;
}

__public int
tbdef_print(void const *__restrict instruction_pointer,
            void const *__restrict frame_address,
            size_t frame_index, void *closure) {
#ifdef __KERNEL__
 /* Placeholder until the kernel can load its own DWARF .debug_line information. */
 return format_printf(&tbprint_callback,closure
                     ,"#!$ addr2line(%p) '{file}({line}) : {func} : [%Ix][%p] : %p'\n"
                     ,((uintptr_t)instruction_pointer)-1,frame_index,frame_address,instruction_pointer);
#else
 int error; syminfo_t *sym; char buffer[512];
           sym = mod_addrinfo(MOD_ALL,instruction_pointer,(syminfo_t *)buffer,sizeof(buffer),MOD_SYMINFO_ALL);
 if (!sym) sym = mod_addrinfo(MOD_ALL,instruction_pointer,NULL,0,MOD_SYMINFO_ALL);
 if (!sym) format_printf(&tbprint_callback,closure,
                         "??" "?(??" "?) : ??" "? : [%Ix][%p] : %p (Unknown: %s)\n",
                         strerror(errno));
 else {
  error = format_printf(&tbprint_callback,closure,
                        "%s(%u) : %s : [%Ix][%p] : %p\n",
                        sym->si_file ? sym->si_file : "??" "?",
                        sym->si_line+1,
                        sym->si_name ? sym->si_name : "??" "?",
                        frame_index,frame_address,
                        instruction_pointer);
  if (sym != (syminfo_t *)buffer) free(sym);
 }
 return error;
#endif
}
__public int
tbdef_error(int error, void const *arg, void *closure) {
 char const *reason;
 switch (error) {
  case TRACEBACK_STACKERROR_EIP  : reason = "Invalid instruction pointer"; error = 0; break;
  case TRACEBACK_STACKERROR_LOOP : reason = "Stackframes are looping"; break;
  case TRACEBACK_STACKERROR_FRAME: reason = "A stackframe points to an invalid address"; break;
  default                        : return 0;
 }
 if (format_printf(&tbprint_callback,closure,
                   "CORRUPTED STACK: %s (%p)\n",
                   reason,arg) != 0) return -1;
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

static int
debug_dostackwalk(struct stackframe *start, ptbwalker callback,
                  ptberrorhandler handle_error, void *closure, size_t skip) {
 struct stackframe *frame,*next,*check; int temp;
 size_t didskip = 0,index = 0; frame = start;
 while (frame) {
  kerrno_t error = kmem_validate(frame,sizeof(struct stackframe));
  /* Validate the stack frame address */
  if (KE_ISERR(error)) {
   if (!handle_error) return TRACEBACK_STACKERROR_FRAME;
   temp = (*handle_error)(TRACEBACK_STACKERROR_FRAME,(void const *)frame,closure);
#ifdef __KERNEL__
   if (temp != 0 && error != KE_INVAL) return temp;
#else
   if (temp != 0) return temp;
#endif
  }
  /* Validate the instruction pointer */
  if (KE_ISERR(kmem_validatebyte((byte_t const *)(uintptr_t)frame->eip))) {
   if (!handle_error) return TRACEBACK_STACKERROR_EIP;
   temp = (*handle_error)(TRACEBACK_STACKERROR_EIP,(void const *)(uintptr_t)frame->eip,closure);
   if (temp != 0) return temp;
  }
  if (didskip < skip) ++didskip;
  else if (callback) {
   /* Call the stack entry enumerator */
   temp = (*callback)(frame->eip,frame,index++,closure);
   if (temp != 0) return temp;
  }
  next = frame->caller;
  /* Ensure that the frame hasn't already been enumerated (looping stack) */
  check = start;
  for (;;) {
   if (check == next) {
    /* The stackframes are recursive */
    if (!handle_error) return TRACEBACK_STACKERROR_LOOP;
    temp = (*handle_error)(TRACEBACK_STACKERROR_LOOP,(void const *)next,closure);
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
__public __noinline int tb_walk(ptbwalker callback, ptberrorhandler handle_error, void *closure) { return tb_walkex(callback,handle_error,closure,1); }
__public __noinline int tb_walkex(ptbwalker callback, ptberrorhandler handle_error, void *closure, size_t skip) { struct stackframe *stack; __asm_volatile__("movl %%ebp, %0" : "=r" (stack)); return debug_dostackwalk(stack,callback,handle_error,closure,skip); }
__public            int tb_walkebp(void const *ebp, ptbwalker callback, ptberrorhandler handle_error, void *closure) { return tb_walkebpex(ebp,callback,handle_error,closure,0); }
__public            int tb_walkebpex(void const *ebp, ptbwalker callback, ptberrorhandler handle_error, void *closure, size_t skip) { return debug_dostackwalk((struct stackframe *)ebp,callback,handle_error,closure,skip); }
__public __noinline int tb_print(void)                              { return tb_walkex(&tbdef_print,&tbdef_error,NULL,1); }
__public __noinline int tb_printex(size_t skip)                     { return tb_walkex(&tbdef_print,&tbdef_error,NULL,skip+1); }
__public            int tb_printebp(void const *ebp)                { return tb_walkebpex(ebp,&tbdef_print,&tbdef_error,NULL,0); }
__public            int tb_printebpex(void const *ebp, size_t skip) { return tb_walkebpex(ebp,&tbdef_print,&tbdef_error,NULL,skip); }

struct dtracebackentry {
 void const *tb_eip;
 void const *tb_esp;
};
struct tbtrace {
 size_t                 tb_entryc;
 struct dtracebackentry tb_entryv[1024];
};

static __noinline int
tbcapture_walkcount(void const *__restrict __unused(instruction_pointer),
                    void const *__restrict __unused(frame_address),
                    size_t frame_index, size_t *closure) {
 if (frame_index >= *closure) *closure = frame_index+1;
 return 0;
}
static __noinline int
tbcapture_walkcollect(void const *__restrict instruction_pointer,
                      void const *__restrict frame_address,
                      size_t frame_index, struct tbtrace *tb) {
 assertf(frame_index < tb->tb_entryc,"%Iu >= %Iu",frame_index,tb->tb_entryc);
 tb->tb_entryv[frame_index].tb_eip = instruction_pointer;
 tb->tb_entryv[frame_index].tb_esp = frame_address;
 return 0;
}

__public __noinline struct tbtrace *tbtrace_capture(void) { return tbtrace_captureex(1); }
__public __noinline struct tbtrace *tbtrace_captureex(size_t skip) { void *ebp; __asm_volatile__("movl %%ebp, %0" : "=r" (ebp)); return tbtrace_captureebpex(ebp,skip); }
__public struct tbtrace *tbtrace_captureebp(void const *ebp) { return tbtrace_captureebpex(ebp,0); }
__public struct tbtrace *tbtrace_captureebpex(void const *ebp, size_t skip) {
 struct tbtrace *result; size_t entrycount = 0;
 tb_walkebpex(ebp,(ptbwalker)&tbcapture_walkcount,NULL,&entrycount,skip);
 result = (struct tbtrace *)malloc(offsetof(struct tbtrace,tb_entryv)+
                                   entrycount*sizeof(struct dtracebackentry));
 if __unlikely(!result) return NULL;
 result->tb_entryc = entrycount;
 tb_walkebpex(ebp,(ptbwalker)&tbcapture_walkcollect,NULL,result,skip);
 return result;
}

__public int
tbtrace_walk(struct tbtrace const *__restrict self,
             ptbwalker callback,
             void *closure) {
 struct dtracebackentry const *elemv; size_t i; int error;
 if __unlikely(!self) return 0; /* Ignore NULL tracebacks */
 kassertmem(self,offsetof(struct tbtrace,tb_entryv));
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
__public int tbtrace_print(struct tbtrace const *__restrict self) {
 return tbtrace_walk(self,&tbdef_print,NULL);
}


__DECL_END

#endif /* !__TRACEBACK_C__ */

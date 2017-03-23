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
#ifndef __EXCEPTION_C__
#define __EXCEPTION_C__ 1

#include <kos/config.h>
#ifndef __KERNEL__
#include <kos/compiler.h>
#include <kos/exception.h>
#include <exception.h>
#include <traceback.h>
#include <malloc.h>
#include <proc.h>
#include <stddef.h>
#include <unistd.h>
#include <stdlib.h>
#include <format-printer.h>
#include "traceback-internal.h"

__DECL_BEGIN

#undef exc_continue

/* As simple as it gets: These are literally just small library wrapper around system functions. */
__public __noreturn void exc_throw(exception_t const *__restrict exception) { for (;;) exc_throw_resumeable(exception); }
__public __noreturn void exc_raise(exno_t exception_number) { for (;;) exc_raise_resumeable(exception_number); }
__public void exc_throw_resumeable(exception_t const *__restrict exception) { kexcept_throw(exception); }
__public void exc_raise_resumeable(exno_t exception_number) { kexcept_raise(exception_number); }
__public __noreturn void exc_rethrow(void) { kexcept_rethrow(); }
__public __noreturn void exc_continue(void) { kexcept_continue(); }

struct exc_tpfip1_data { /* TracebackPrinterFrameIndexPlus1 */
 ptbwalker       real_walker;
 ptberrorhandler real_error;
 void           *real_closure;
};
static int exc_tpfip1_walker(void const *__restrict instruction_pointer,
                             void const *__restrict frame_address,
                             size_t frame_index, struct exc_tpfip1_data *closure) {
 return (*closure->real_walker)(instruction_pointer,frame_address,
                                frame_index+1,closure->real_closure);
}
static int exc_tpfip1_error(int error, void const *arg,
                            struct exc_tpfip1_data *closure) {
 return (*closure->real_error)(error,arg,closure->real_closure);
}

__public int
exc_tbwalk(ptbwalker callback,
           ptberrorhandler handle_error,
           void *closure) {
 int error; struct kexstate *state = &tls_self->u_exstate;
 struct exc_tpfip1_data tpfip1;
 error = tb_walkeip((void const *)state->eip,callback,handle_error,closure);
 if __unlikely(error != 0) return error;
 tpfip1.real_walker  = callback;
 tpfip1.real_error   = handle_error;
 tpfip1.real_closure = closure;
 return tb_walkebp((void const *)state->ebp,
                   callback ? (ptbwalker)&exc_tpfip1_walker : NULL,
                   handle_error ? (ptberrorhandler)&exc_tpfip1_error : NULL,
                   &tpfip1);
}

__public char const *
exc_getname(exno_t exception_number) {
 char const *result = NULL;
 switch (exception_number) {
  case KEXCEPTION_DIVIDE_BY_ZERO  : result = "DivideByZero"; break;
  case KEXCEPTION_OVERFLOW        : result = "Overflow"; break;
  case KEXCEPTION_BOUND_RANGE     : result = "BoundRange"; break;
  case KEXCEPTION_INVALID_OPCODE  : result = "InvalidOpcode"; break;
  case KEXCEPTION_PROTECTION_FAULT: result = "ProtectionFault"; break;
  case KEXCEPTION_SEGFAULT        : result = "Segfault"; break;
  default: break;
 }
 return result;
}

__public int
exc_tbprint(int tp_only) {
 return exc_tbprintex(tp_only,NULL);
}
__public int
exc_tbprintex(int tp_only, void *closure) {
 int error;
 if (!tp_only) {
  struct kuthread *uthread = tls_self;
  exno_t exno = uthread->u_exinfo.ex_no;
  char const *name = exc_getname(exno);
  if (!name) name = "Unknown";
  /* Print the exception name and number. */
  error = format_printf(&tbprint_callback,closure,
                        "Exception occurred: %s (%Iu)\n",
                        name,exno);
  if __unlikely(error != 0) return error;
  /* Print some additional informations. */
  switch (exno) {
   {
    static char const *yesno[] = {"Yes(1)","No(0)"};
   case KEXCEPTION_SEGFAULT:
    error = format_printf(&tbprint_callback,closure
                         ,"Address: %p\n"
                          "Present: %s\n"
                          "Writing: %s\n"
                          "Execute: %s\n"
                         ,uthread->u_exinfo.ex_ptr[0]
                         ,yesno[!!(uthread->u_exinfo.ex_info&KEXCEPTIONINFO_SEGFAULT_PRESENT)]
                         ,yesno[!!(uthread->u_exinfo.ex_info&KEXCEPTIONINFO_SEGFAULT_WRITE)]
                         ,yesno[!!(uthread->u_exinfo.ex_info&KEXCEPTIONINFO_SEGFAULT_INSTR_FETCH)]);
   } break;

   default:
#if KEXINFO_PTR_COUNT == 4
    error = format_printf(&tbprint_callback,closure,
                          "Info: %I32x (%p,%p,%p,%p)\n",
                          uthread->u_exinfo.ex_info,
                          uthread->u_exinfo.ex_ptr[0],
                          uthread->u_exinfo.ex_ptr[1],
                          uthread->u_exinfo.ex_ptr[2],
                          uthread->u_exinfo.ex_ptr[3]);
#else
#error FIXME
#endif
    break;
  }
  if __unlikely(error != 0) return error;

 }
 return exc_tbwalk(&tbdef_print,&tbdef_error,closure);
}

__public __noreturn void __kexcept_unhandled(void) {
 static char const text[] = "UNHANDLED EXCEPTION:\n";
 tbprint_callback(text,__COMPILER_STRINGSIZE(text),NULL);
 exc_tbprintex(0,NULL);
 _exit(EXIT_FAILURE);
}



__public struct tbtrace *
exc_tbcapture(void) {
 struct tbtrace *result; size_t entrycount = 0;
 struct kexstate *state = &tls_self->u_exstate;
 tb_walkebpex((void const *)state->ebp,(ptbwalker)&tbcapture_walkcount,NULL,&entrycount,0);
 ++entrycount;
 result = (struct tbtrace *)malloc(offsetof(struct tbtrace,tb_entryv)+
                                   entrycount*sizeof(struct dtracebackentry));
 if __unlikely(!result) return NULL;
 result->tb_entryc = entrycount;
 exc_tbwalk((ptbwalker)&tbcapture_walkcollect,NULL,result);
 return result;
}

__DECL_END
#endif /* !__KERNEL__ */

#endif /* !__EXCEPTION_C__ */

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
#include "traceback-internal.h"
#include <exception.h>
#include <format-printer.h>
#include <kos/compiler.h>
#include <kos/exception.h>
#include <kos/symbol.h>
#include <kos/syscall.h>
#include <kos/syscallno.h>
#include <malloc.h>
#include <proc-tls.h>
#include <proc.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <traceback.h>
#include <unistd.h>

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
#if __SIZEOF_POINTER__ == 4
#   define P  "%.8p"
#elif __SIZEOF_POINTER__ == 8
#   define P  "%.16p"
#elif __SIZEOF_POINTER__ == 2
#   define P  "%.4p"
#elif __SIZEOF_POINTER__ == 1
#   define P  "%.2p"
#else
#   error FIXME
#endif
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
   case KEXCEPTION_SEGFAULT:
    error = format_printf(&tbprint_callback,closure,"\tAttempted to %s%s memory at " P "\n",
                         (uthread->u_exinfo.ex_info&KEXCEPTIONINFO_SEGFAULT_INSTR_FETCH) ? "execute" :
                         (uthread->u_exinfo.ex_info&KEXCEPTIONINFO_SEGFAULT_WRITE) ? "write" : "read",
                         (uthread->u_exinfo.ex_info&KEXCEPTIONINFO_SEGFAULT_PRESENT) ? "" : " unmapped",
                          uthread->u_exinfo.ex_ptr[0]);
    break;

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
  error = format_printf(&tbprint_callback,closure
                       ,"EDI: %.8I32x ESI: %.8I32x EBP: %.8I32x ESP: %.8I32x\n"
                        "EBX: %.8I32x EDX: %.8I32x ECX: %.8I32x EAX: %.8I32x\n"
                        "EIP: %.8I32x EFLAGS: %.8I32x\n"
                       ,uthread->u_exstate.edi,uthread->u_exstate.esi
                       ,uthread->u_exstate.ebp,uthread->u_exstate.esp
                       ,uthread->u_exstate.ebx,uthread->u_exstate.edx
                       ,uthread->u_exstate.ecx,uthread->u_exstate.eax
                       ,uthread->u_exstate.eip,uthread->u_exstate.eflags);
 }
#undef P
 return exc_tbwalk(&tbdef_print,&tbdef_error,closure);
}

/* Minimum amount of stack space that should be reserved for UEH handler. */
#define UEH_STACKRESERVE  2048


/* This function has _very_ special meaning, because
 * it is what the kernel will execute by default when
 * searching for an unhandled exception handler.
 * NOTE: Searching for such a handler follows reverse KMODID_NEXT
 *       rules, meaning that each module could theoretically define
 *       its own unhandled exception handler, although when not
 *       defined, the kernel will simply choose the next handler.
 * WARNING: Once inside an unhandled exception handler, there is
 *          no way to recover and return to executing normal code
 *          within the same module.
 *          Note though, that it is technically possible to
 *          unload, then re-load the module who's unhandled
 *          exception handler was invoked to reset this restriction.
 */
#define TLS(offset) "%" __TLS_REGISTER_S ":(" __PP_STR(offset) ")"
__asm__("    .text\n"
        "    .globl	" __PP_STR(__SYM(__kos_unhandled_exception)) "\n"
        "    .type " __PP_STR(__SYM(__kos_unhandled_exception)) ", @function\n"
        "" __PP_STR(__SYM(__kos_unhandled_exception)) ":\n"
        /* If the exception occurred because of an out-of-bounds stack
         * pointer, reset the stack to ensure that the exception handler
         * will be able to function properly:
         * Note though, that in doing this we might end up breaking some data.
         * >> if (esp > tls_self->u_stackend) {
         * >>   // Overflow above (Very rare, but indicates some pretty
         * >>   // severe error; potentially originating in assembly).
         * >>reset_stack_above:
         * >>   void *stack_used = tls_self->u_stackbegin+UEH_STACKRESERVE;
         * >>   if (stack_use > tls_self->u_stackend) stack_use = tls_self->u_stackend;
         * >>   esp = stack_use;
         * >> } else if (esp <= tls_self->u_stackbegin+UEH_STACKRESERVE) {
         * >>reset_stack_below:
         * >>   // (Ab-)use the TLS control block as temporary stack after
         * >>   // stack overflow to prevent overwriting existing data.
         * >>   // But make sure that the stack reserve fits into the upper TCB.
         * >>   size_t pagesize = k_sysconf(_SC_PAGE_SIZE);
         * >>   if (pagesize < KUTHREAD_SIZEOF+UEH_STACKRESERVE) goto reset_stack_above;
         * >>   esp = (char *)tls_self+pagesize;
         * >> } else {
         * >>   // Stack is still in-bounds with an acceptable margin
         * >> }
         * >>validate_esp:
         * >> if (kmem_validate(esp,UEH_STACKRESERVE) != KE_OK) {
         * >>   // I though I repaired the stack, but I guess it's still broken...
         * >>   // >> Last (and unlikely to success) chance: Allocate a new stack now...
         * >>#ifdef KFD_HAVE_BIARG_POSITIONS
         * >>   void *newstack = kmem_map(NULL,UEH_STACKRESERVE,PROT_READ|PROT_WRITE,MAP_ANONYMOUS,-1,0,0);
         * >>#else
         * >>   void *newstack = kmem_map(NULL,UEH_STACKRESERVE,PROT_READ|PROT_WRITE,MAP_ANONYMOUS,-1,0);
         * >>#endif
         * >>   // No point in checking if it worked. - There's nothing we can do if it didn't...
         * >>   esp = newstack+UEH_STACKRESERVE;
         * >> }
         * >> ueh_handler();
         */
        "    mov " TLS(KUTHREAD_OFFSETOF_STACKBEGIN) ", %eax\n"
        "    mov " TLS(KUTHREAD_OFFSETOF_STACKEND)   ", %ecx\n"
        "    add $" __PP_STR(UEH_STACKRESERVE)       ", %eax\n"
        "    cmp %ecx, %esp\n"
        "    ja  reset_stack_above\n"
        "    cmp %eax, %esp\n"
        "    jbe reset_stack_below\n"
        "    jmp validate_esp\n"
        "reset_stack_above:\n"
        /* Try to run the handler in the lower (end) region of the stack,
         * thus reducing the chances that executing it will overwrite
         * some of the data it is trying to dump:
         * >> void *stack_use = tls_self->u_stackbegin+UEH_STACKRESERVE;
         * >> // Make sure not to place the ESP out-of-bounds now:
         * >> // >> If the stack is too small for the reserved area, just use the whole stack.
         * >> if (stack_use > tls_self->u_stackend) stack_use = tls_self->u_stackend;
         * >> esp = stack_use;
         */
        "    cmp %ecx, %eax\n"
        "    jna use_reserve\n" /* if (!(stack_use > tls_self->u_stackend)) goto use_reserve; */
        "    mov %ecx, %eax\n"  /* stack_use = tls_self->u_stackend; */
        "use_reserve:\n"
        "    mov %eax, %esp\n"  /* Set ESP to point into the reserved area of the stack. */
        "    jmp validate_esp\n"
        "reset_stack_below:\n"
        /* The stack pointer is located below the limit.
         * This situation will likely arise if a stack overflow happened.
         * >> To prevent the real stack from being damaged, attempt to
         *    use the upper part of the TCB as make-shift stack.
         * NOTE: But make sure it's large enough!
         */
        "    mov $" __PP_STR(SYS_k_sysconf) ", %eax\n" 
        "    mov $" __PP_STR(_SC_PAGE_SIZE) ", %ecx\n" 
        "    int $" __PP_STR(__SYSCALL_INTNO) "\n" 
        /* PAGESIZE should now be stored in %eax */
        "    cmp $(" __PP_STR(KUTHREAD_SIZEOF+UEH_STACKRESERVE) "), %eax\n" 
        "    jb  reset_stack_above\n" /* if (pagesize < KUTHREAD_SIZEOF+UEH_STACKRESERVE) goto use_lower_stack; */
        /* We can use the TCB block as stack. */
        "    mov " TLS(0) ", %esp\n"
        "    add %esp,       %eax\n"

        /* Sanity check: validate that the fixed ESP will actually work. */
        "validate_esp:\n"
        "    mov $" __PP_STR(SYS_kmem_validate) ", %eax\n" 
        "    mov %esp, %ecx\n" 
        "    mov $" __PP_STR(UEH_STACKRESERVE)  ", %edx\n" 
        "    int $" __PP_STR(__SYSCALL_INTNO)   "\n" 
        /* If the memory is valid, EAX now equals KE_OK */
        "    cmp $(" __PP_STR(KE_OK) "), %eax\n" 
        "    je ueh_handler\n" /* if (kmem_validate(esp,UEH_STACKRESERVE) == KE_OK) ueh_handler(); */
        /* Something went wrong (A lot)
         * >> But we've still got a shot by trying to allocate a completely new stack!
         * NOTE: Since calling 'kmem_map' requires a lot of arguments (7), we have to hack together a small stack here. */
#ifdef KFD_HAVE_BIARG_POSITIONS
        "    lea (mini_mmap_stack+" __PP_STR(__SIZEOF_POINTER__*2) "), %esp\n"
#else
        "    lea (mini_mmap_stack+" __PP_STR(__SIZEOF_POINTER__*1) "), %esp\n"
#endif
        "    mov $(" __PP_STR(SYS_kmem_map)         "), %eax\n"
        "    mov $(" "0"                            "), %ecx\n" /* hint */
        "    mov $(" __PP_STR(UEH_STACKRESERVE)     "), %edx\n" /* length */
        "    mov $(" __PP_STR(PROT_READ|PROT_WRITE) "), %ebx\n" /* prot */
        "    mov $(" __PP_STR(MAP_ANONYMOUS)        "), %esi\n" /* flags */
        "    mov $(" __PP_STR(-1)                   "), %edi\n" /* fd */
        "    pushl $0\n" /* offset */
#ifdef KFD_HAVE_BIARG_POSITIONS
        "    pushl $0\n" /* offset */
#endif
        "    int $" __PP_STR(__SYSCALL_INTNO)  "\n" 
        /* If the syscall worked, a pointer to the memory base is now stored in EAX.
         * Yet since there's no point in checking if it worked, we simply don't do so. */
        "    add $" __PP_STR(UEH_STACKRESERVE) ", %eax\n" 
        "    mov %eax, %esp\n" 
        /* Lets do this! */
        "    jmp " __PP_STR(__SYM(ueh_handler)) "\n"
        ".size " __PP_STR(__SYM(__kos_unhandled_exception)) ", . - " __PP_STR(__SYM(__kos_unhandled_exception)) "\n"
);
#ifdef KFD_HAVE_BIARG_POSITIONS
static __attribute_used __uintptr_t mini_mmap_stack[2];
#else
static __attribute_used __uintptr_t mini_mmap_stack[1];
#endif
#undef TLS

static __attribute_used __noreturn
void ueh_handler(void) {
 /* Called from the assembly above. */
 static char const tty_begin[] = "\033[31;107m"; /*< Dark-red on white. */
 static char const tty_end[]   = "\033[m";
 static char const text[]      = "UNHANDLED EXCEPTION:\n";
 int stderr_is_a_tty = 0;
 size_t wsize;
 /* Figure out if stderr is a tty.
  * - If it is, we can do some color highlighting.
  * NOTE: We wrap this is a try-block to prevent KOS from
  *       just terminating the app if it would crash again. */
 exc_try(itty) {
  stderr_is_a_tty = isatty(STDERR_FILENO);
  if (stderr_is_a_tty) {
   kfd_write(STDERR_FILENO,tty_begin,sizeof(tty_begin)-sizeof(char),&wsize);
  }
 } exc_except(itty,1) {
 }
 tbprint_callback(text,__COMPILER_STRINGSIZE(text),NULL);
 exc_tbprintex(0,NULL);
 if (stderr_is_a_tty) {
  exc_try(itty2) {
   kfd_write(STDERR_FILENO,tty_end,sizeof(tty_end)-sizeof(char),&wsize);
  } exc_except(itty2,1) {
  }
 }
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

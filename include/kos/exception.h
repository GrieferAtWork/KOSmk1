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
#ifndef __KOS_EXCEPTION_H__
#define __KOS_EXCEPTION_H__ 1

#include <kos/config.h>
#include <kos/compiler.h>
#ifndef __KERNEL__
#include <kos/task-tls.h>
#include <proc-tls.h>
#ifndef __INTELLISENSE__
#include <assert.h>
#include <stddef.h>
#include <string.h>
#endif
#ifdef __i386__
#include <kos/arch/x86/irq.h>
#endif
#endif

__DECL_BEGIN

#ifndef __KEXCEPT_SAFE_ALL_REGISTERS
#define __KEXCEPT_SAFE_ALL_REGISTERS 0
#endif

#define KEXCEPTION_NONE   0

#ifdef __i386__
#define KEXCEPTION_X86_IRQ(num) (16+(num))
#define KEXCEPTION_DIVIDE_BY_ZERO   KEXCEPTION_X86_IRQ(KARCH_X64_IRQ_DE)
#define KEXCEPTION_OVERFLOW         KEXCEPTION_X86_IRQ(KARCH_X64_IRQ_OF)
#define KEXCEPTION_BOUND_RANGE      KEXCEPTION_X86_IRQ(KARCH_X64_IRQ_BR)
#define KEXCEPTION_INVALID_OPCODE   KEXCEPTION_X86_IRQ(KARCH_X64_IRQ_UD)
#define KEXCEPTION_PROTECTION_FAULT KEXCEPTION_X86_IRQ(KARCH_X64_IRQ_GP)

//////////////////////////////////////////////////////////////////////////
// Segfault (aka. Access Violation)
// This exception is the most interesting one, because it is
// generated when trying to access an invalid memory location.
// Additional exception information is stored in the 'ex_info'
// and 'ex_ptr' fields of the accompanying exception record:
// ex_info:   Set of 'KEXCEPTIONINFO_SEGFAULT_*'
// ex_ptr[0]: Virtual address that you attempted to access.
#define KEXCEPTION_SEGFAULT         KEXCEPTION_X86_IRQ(KARCH_X64_IRQ_PF)
#define KEXCEPTIONINFO_SEGFAULT_NONE        0x00000000
#define KEXCEPTIONINFO_SEGFAULT_PRESENT     0x00000001 /*< When set, the associated page is present (NOTE: Not set if the page belongs to the kernel). */
#define KEXCEPTIONINFO_SEGFAULT_WRITE       0x00000002 /*< Set if the fault was caused by a write operation (Otherwise it was a read). */
#define KEXCEPTIONINFO_SEGFAULT_INSTR_FETCH 0x00000010 /*< Set if the fault happened during an instruction fetch (aka.: because you jumped to unmapped memory). */
#endif


#define KEXCEPTION_COUNT    256 /*< Number of exceptions reserved for internal use (When defining your own, only use indices >= this) */
#define KEXCEPTION_USER(i) (KEXCEPTION_COUNT+(i)) /*< Generate a user exception number, given an ID >= 0. */


#ifndef __KERNEL__
/* Continue execution where the exception occurred
 * WARNING: Don't use this if you don't know what you're doing!
 *          After doing work somewhere further down the stack from
 *          where the exception got thrown at, you are probably
 *          going to use the stack in some way.
 *          But by doing so, you probably have inadvertently modified
 *          part of the stack that will be necessary for properly
 *          resuming execution where the exception occurred.
 *       >> Obviously this isn't a problem if you're careful or
 *          have set up an exception handler to perform thread-local
 *          tasks, then return to where the task was requested at:
 *       >> #include <stdio.h>
 *       >> #include <stdlib.h>
 *       >> #include <kos/exception.h>
 *       >>
 *       >> #define CMD_PRINT  (KEXCEPTION_COUNT+0)
 *       >> #define CMD_EXIT   (KEXCEPTION_COUNT+1)
 *       >>
 *       >> static char *text;
 *       >>
 *       >> static void highlevel(void) {
 *       >>   text = "Hello World";
 *       >>   kexcept_raise(CMD_PRINT); // Print some text
 *       >>   kexcept_raise(CMD_EXIT);  // Terminate the application
 *       >> }
 *       >>
 *       >> static int handle_exception(int cmd) {
 *       >>   switch (cmd) {
 *       >>     case CMD_PRINT:
 *       >>       printf("%s",text);
 *       >>       break;
 *       >>     case CMD_EXIT:
 *       >>       exit(0);
 *       >>     default: return KEXCEPTION_CONTINUE_SEARCH;
 *       >>   }
 *       >>   return KEXCEPTION_CONTINUE_EXECUTION;
 *       >> }
 *       >> 
 *       >> int main(void) {
 *       >>   __try {
 *       >>     // Reserve some stack space for the exception handler to do work.
 *       >>     __asm_volatile__("sub $512, %%esp" : : : "esp");
 *       >>     highlevel();
 *       >>   } __except(handle_exception(kexcept_code())) {
 *       >>   }
 *       >>   return 0;
 *       >> }
 *          */
#define KEXCEPTION_CONTINUE_EXECUTION (-1)
#define KEXCEPTION_CONTINUE_SEARCH      0
#define KEXCEPTION_EXECUTE_HANDLER      1


#ifndef __ASSEMBLY__
/* TLS address of the exception handler chain. */
#if __SIZEOF_POINTER__ == 4
#   define __KEXCEPT_HANDLER_ADDR  __TLS_REGISTER_S ":8"
#elif __SIZEOF_POINTER__ == 8
#   define __KEXCEPT_HANDLER_ADDR  __TLS_REGISTER_S ":16"
#else
#   error "FIXME"
#endif

#if __KEXCEPT_SAFE_ALL_REGISTERS
#define __KEXCEPT_SAVEREGS_S "push %%eax\n"\
                             "push %%ecx\n"\
                             "push %%ebx\n"\
                             "push %%edx\n"\
                             "push %%esi\n"\
                             "push %%edi\n"\
                             "push %%ebp\n"
#define __KEXCEPT_LOADREGS_S "pop %%ebp\n"\
                             "pop %%edi\n"\
                             "pop %%esi\n"\
                             "pop %%edx\n"\
                             "pop %%ebx\n"\
                             "pop %%ecx\n"\
                             "pop %%eax\n"
#define __KEXCEPT_SIZEOFREGS  28
#define __KEXCEPT_DEADREGS    
#else
#define __KEXCEPT_SAVEREGS_S "push %%ebp\n"
#define __KEXCEPT_LOADREGS_S "pop  %%ebp\n"
#define __KEXCEPT_SIZEOFREGS  4
/* List all of these registers so GCC knows that all of these must be dead.
 * Without this part, we'd have to safe all registers (which is the alternative).
 * HINT: Listing all of these registers has an effect similar to what
 *       __attribute__((returns_twice)) tells the compiler.
 * PS: Note how we don't specify 'EBP' in this list, but instead safe it on the stack.
 *     By doing it this way, we essentially force the compiler to safe all
 *     register variables on the stack before entering the try-block. */
#define __KEXCEPT_DEADREGS    , "eax", "ecx", "ebx", "edx", "esi", "edi"
#define __KEXCEPT_DEADREGS_H         , "ecx", "ebx", "edx", "esi", "edi"
#endif


#define __KEXCEPT_HANDLER_NAME(name) __xh_##name
#define __KEXCEPT_TRY_BEGIN_H(handler) \
 __xblock({ __asm__(__KEXCEPT_SAVEREGS_S\
                    /* On error, stack is restored here. */\
                    "push %0\n"\
                    "push %%" __KEXCEPT_HANDLER_ADDR "\n"\
                    "mov %%esp, %%" __KEXCEPT_HANDLER_ADDR "\n"\
                    : : "" (handler)\
                    : "memory", "esp" __KEXCEPT_DEADREGS);\
            (void)0;\
 })
#define __KEXCEPT_TRY_BEGIN(name) \
 __xblock({ __asm_goto__(__KEXCEPT_SAVEREGS_S\
                         /* On error, stack is restored here. */\
                         "push $%l0\n"\
                         "push %%" __KEXCEPT_HANDLER_ADDR "\n"\
                         "mov %%esp, %%" __KEXCEPT_HANDLER_ADDR "\n"\
                         : : : "memory", "esp" __KEXCEPT_DEADREGS\
                         : __KEXCEPT_HANDLER_NAME(name));\
            (void)0;\
 })
#define __KEXCEPT_TRY_END \
 __xblock({ __asm__("pop %%" __KEXCEPT_HANDLER_ADDR "\n"\
                    "add $(" __PP_STR(__SIZEOF_POINTER__+__KEXCEPT_SIZEOFREGS) "), %%esp\n"\
                    : : : "memory", "esp");\
            (void)0;\
 })

/* Finally begin/end */
#define __KEXCEPT_FINALLY_BEGIN \
 __xblock({ if (kexcept_code() != KEXCEPTION_NONE) {\
             __asm__(__KEXCEPT_LOADREGS_S : : : "memory", "esp");\
            }\
            (void)0;\
 })
#define __KEXCEPT_FINALLY_END \
 (kexcept_code() ? kexcept_rethrow() : (void)0)

/* Except begin/end */
#define __KEXCEPT_EXCEPT_BEGIN_CONTINUE   kexcept_continue()
#define __KEXCEPT_EXCEPT_BEGIN_RETHROW    kexcept_rethrow()
#define __KEXCEPT_EXCEPT_BEGIN(expr) \
 __xblock({ __asm__(__KEXCEPT_LOADREGS_S : : : "memory", "esp");\
            switch ((int)(expr)) {\
             case KEXCEPTION_CONTINUE_EXECUTION: __KEXCEPT_EXCEPT_BEGIN_CONTINUE;\
             case KEXCEPTION_CONTINUE_SEARCH:    __KEXCEPT_EXCEPT_BEGIN_RETHROW;\
             default: break;\
            }\
            (void)0;\
 })
#define __KEXCEPT_EXCEPT_END \
 _tls_putl(KUTHREAD_OFFSETOF_EXINFO+KEXINFO_OFFSETOF_NO,KEXCEPTION_NONE)


//////////////////////////////////////////////////////////////////////////
// Define low-level, named exception blocks.
// >> /* Global exception handlers take no arguments and don't return. */
// >> static __noreturn void global_handler(void) {
// >>   k_syslogf(KLOG_ERROR,"This is some baaad $h1t: %d\n",kexcept_code());
// >>   /* WARNING: Don't try to call 'kexcept_continue()' here, because the hidden
// >>    *          preamble of this function will have already broken the stack... */
// >>   kexcept_rethrow();
// >> }
// >> static kerrno_t exception_handling(void) {
// >>   kerrno_t error;
// >>   /* When exception blocks are named, you can
// >>    * declare blocks and handler out-of-order. */
// >>   KEXCEPT_TRY(exc_a) { error = do_dangerous_call_a(); }
// >>   KEXCEPT_TRY(exc_b) { error = do_dangerous_call_c(); }
// >>   KEXCEPT_TRY(exc_c) { error = do_dangerous_call_d(); }
// >>   KEXCEPT_TRY_H(&global_handler) { error = do_last_dangerous_call(); }
// >>   
// >>   KEXCEPT_EXCEPT(exc_a,1) { error = KE_NOMEM; }
// >>   KEXCEPT_EXCEPT(exc_b,1) { error = KE_FAULT; }
// >>   KEXCEPT_EXCEPT(exc_c,1) { error = KE_INVAL; }
// >>   return error;
// >> }

#ifdef __INTELLISENSE__
/* Mark a code block during which occurring exceptions
 * are handled by an exception handler 'name'. */
#define KEXCEPT_TRY_H(name) if(0)(name)();else for(;;)
#define KEXCEPT_TRY(name)   if(0)goto __KEXCEPT_HANDLER_NAME(name);else for(;;)
#define KEXCEPT_LEAVE       break
#else
#define KEXCEPT_TRY_H(handler) \
 if __unlikely((__KEXCEPT_TRY_BEGIN_H(handler),0)); else\
 for (int __f_temp = 1; __f_temp; __KEXCEPT_TRY_END,__f_temp = 0)
#define KEXCEPT_TRY(name) \
 if __unlikely((__KEXCEPT_TRY_BEGIN(name),0)); else\
 for (int __f_temp = 1; __f_temp; __KEXCEPT_TRY_END,__f_temp = 0)
#define KEXCEPT_LEAVE       { __KEXCEPT_TRY_END; break; }
#endif

/* Define a local finally-handler 'name'. */
#define KEXCEPT_FINALLY(name) \
 if __unlikely(0); else __KEXCEPT_HANDLER_NAME(name): \
 if ((__KEXCEPT_FINALLY_BEGIN,0)); else\
 for (int __f_temp = 1; __f_temp; __KEXCEPT_FINALLY_END,__f_temp = 0)



/* Define a local except-handler 'name' used to handle 'exno' exceptions. */
#ifdef __INTELLISENSE__
#define KEXCEPT_EXCEPT(name,should_handle) \
 if (((should_handle),1)); else __KEXCEPT_HANDLER_NAME(name):
#else
#define KEXCEPT_EXCEPT(name,should_handle) \
 if __likely(1); else __KEXCEPT_HANDLER_NAME(name): \
 if ((__KEXCEPT_EXCEPT_BEGIN(should_handle),0)); else\
 for (int __f_temp = 1; __f_temp; __KEXCEPT_EXCEPT_END,__f_temp = 0)
#endif


//////////////////////////////////////////////////////////////////////////
// === Define some high-level & user friendly keywords. ===
// WARNING: The '__finally' keyword is __NOT__ executed if you try to
//          return from a __try-block, which in it self is already something
//          extremely bad!
//       >> Don't try to jump or return from a __try-block!
//          Use '__leave' instead, which will safely jump
//          over the handler and place EIP directly behind, at
//          which point you are once again allowed to do whatever...
// WARNING: These keywords do not function recursively!
//          __DONT__ do something like this:
//       >> __try {
//       >>   __try { // Using try-in-try is not allowed!
//       >>     volatile int x = 1;
//       >>     volatile int y = 0;
//       >>     printf("DIVZERO = %d\n",x/y);
//       >>   } __finally {
//       >>     printf("In finally!\n");
//       >>   }
//       >> } __except(1) {
//       >>   printf("In except!\n");
//       >> }
//          When recursion is required, you must explicitly choose names for the guard:
//       >> KEXCEPT_TRY(outer) {
//       >>   KEXCEPT_TRY(inner) {
//       >>     volatile int x = 1;
//       >>     volatile int y = 0;
//       >>     printf("DIVZERO = %d\n",x/y);
//       >>   } KEXCEPT_FINALLY(inner) {
//       >>     printf("In finally!\n");
//       >>   }
//       >> } KEXCEPT_EXCEPT(outer,1) {
//       >>   printf("In except!\n");
//       >> }
#ifdef __INTELLISENSE__
/* These keywords and how I implement their semantics are equivalent to SEH. */
#elif defined(__TPP_EVAL) && __has_extension(__tpp_pragma_tpp_exec__)
/* Best case: this preprocessor can emulate recursion:
 * Use tpp_exec + __TPP_EVAL to emulate recursion:
 * >> __try {
 * >>   __try {
 * >>     *(char *)0xdeadbeef = 42;
 * >>   } __finally {
 * >>     printf("In finally\n");
 * >>   }
 * >> } __except (1) {
 * >>   printf("In except\n");
 * >> }
 */

/* Avert your eyes! The following macros are capable of re-refining others.
 * So unless you really know what you're expecting to find here, don't try
 * too to find the exact path of what is happening here:
 * >> __EXC_LEVEL: The current level of indirection.
 *   - Incremented by __try
 *   - Decremented by __finally and __except
 * >> __EXC_UNIQUE[level]: An unique identifier for a given level.
 *   - Incremented by __try with 'level = __EXC_LEVEL'
 *   - There exists an infinite amount of these macros (e.g.: '__EXC_UNIQUE1' or '__EXC_UNIQUE12')
 *   - Dynamically allocated as indirection of __try-blocks increase.
 * >> __EXC_LABEL_NAME
 *   - Generates a label name as follows:
 *     EXPAND(__EXC_LEVEL)##_##EXPAND(__EXC_UNIQUE##EXPAND(__EXC_LEVEL))
 *     e.g.: '1_4' for first-level indirection; 4th time use.
 */
#define __EXC_LEVEL     0
#define __EXC_INCUNIQUE(level) \
__compiler_pragma(tpp_exec("#undef __EXC_UNIQUE" #level "\n\
#define __EXC_UNIQUE" #level " " __PP_STR(__TPP_EVAL(__EXC_UNIQUE##level+1)) "\n"))

#define __EXC_PP_BEGIN \
__compiler_pragma(tpp_exec("#undef __EXC_LEVEL\n\
#define __EXC_LEVEL " __PP_STR(__TPP_EVAL(__EXC_LEVEL+1)) "\n"))\
__compiler_pragma(tpp_exec("\
#ifdef __EXC_UNIQUE" __PP_STR(__EXC_LEVEL) "\n\
__EXC_INCUNIQUE(" __PP_STR(__EXC_LEVEL) ")\n\
#else\n\
#define __EXC_UNIQUE" __PP_STR(__EXC_LEVEL) " 0\n\
#endif\n"))

#define __EXC_PP_END \
__compiler_pragma(tpp_exec("\
#undef __EXC_LEVEL\n\
#define __EXC_LEVEL " __PP_STR(__TPP_EVAL(__EXC_LEVEL-1)) "\n"))

#define __EXC_LABEL_NAME_IMPL4(level,unique) level##_##unique
#define __EXC_LABEL_NAME_IMPL3(level,unique) __EXC_LABEL_NAME_IMPL4(level,unique)
#define __EXC_LABEL_NAME_IMPL2(level) __EXC_LABEL_NAME_IMPL3(__EXC_LEVEL,__EXC_UNIQUE##level)
#define __EXC_LABEL_NAME_IMPL(level) __EXC_LABEL_NAME_IMPL2(level)
#define __EXC_LABEL_NAME   __EXC_LABEL_NAME_IMPL(__EXC_LEVEL)
#define __try    __EXC_PP_BEGIN KEXCEPT_TRY(__EXC_LABEL_NAME)
#define __finally               KEXCEPT_FINALLY(__EXC_LABEL_NAME) __EXC_PP_END
#define __except(should_handle) KEXCEPT_EXCEPT(__EXC_LABEL_NAME,should_handle) __EXC_PP_END
#elif defined(__COUNTER__) || defined(__TPP_COUNTER)
/* Try to use some TPP extensions
 * >> If both __TPP_COUNTER and __TPP_EVAL are present,
 *    we can actually make this work 100% of the time! */
#ifdef __TPP_COUNTER
/* Make sure that our counter name isn't already being used... */
#if __TPP_COUNTER(__exnam) != 0
#error "Please don't use the '__exnam' TPP counter (the KOS exception system needs that one)"
#endif
#if __TPP_COUNTER(__exnam)
#endif
#ifdef __TPP_EVAL
#define __EXC_LABEL_NAME        __TPP_EVAL(__COUNTER__ % 2)
#else /* __TPP_EVAL */
/* This only works for a predefined number of times (1000). */
#include "exception-counter.h"
#define __EXC_LABEL_NAME        __PP_CAT_2(__EN_,__TPP_COUNTER(__exnam))
#endif /* !__TPP_EVAL */
#else /* __TPP_COUNTER */
/* Align the counter properly.
 * WARNING:  */
#if !(__COUNTER__ % 2)
#if __COUNTER__
#endif
#endif
#ifdef __TPP_EVAL
#define __EXC_LABEL_NAME        __TPP_EVAL(__COUNTER__ % 2)
#else /* __TPP_EVAL */
/* This only works for a predefined number of times (1000),
 * and requires no-one else using __COUNTER__. */
#include "exception-counter.h"
#define __EXC_LABEL_NAME        __PP_CAT_2(__EN_,__COUNTER__)
#endif /* !__TPP_EVAL */
#endif /* !__TPP_COUNTER */
#define __try                   KEXCEPT_TRY(__EXC_LABEL_NAME)
#define __finally               KEXCEPT_FINALLY(__EXC_LABEL_NAME)
#define __except(should_handle) KEXCEPT_EXCEPT(__EXC_LABEL_NAME,should_handle)
#elif defined(__GNUC__)
/* This only works if '__try' is only used once per scope! */
#define __try                   __label__ __KEXCEPT_HANDLER_NAME(exc); KEXCEPT_TRY(exc)
#define __finally               KEXCEPT_FINALLY(exc)
#define __except(should_handle) KEXCEPT_EXCEPT(exc,should_handle)
#else
/* This only works if '__try' is only used once per function! */
#define __try                   KEXCEPT_TRY(exc)
#define __finally               KEXCEPT_FINALLY(exc)
#define __except(should_handle) KEXCEPT_EXCEPT(exc,should_handle)
#endif
#ifndef __INTELLISENSE__
#define __leave                 KEXCEPT_LEAVE
#endif





#ifndef __kexno_t_defined
#define __kexno_t_defined 1
typedef __u32 kexno_t;
#endif

//////////////////////////////////////////////////////////////////////////
// Called internally when an exception cannot be handled.
// The user may implement this function them self, or use
// the default one that ships with libc.
// NOTE: This function is only invoked for unhandled
//       exceptions raised through 'ex_raise'.
//       Without a handler, exceptions raised by the kernel
//       will cause the process to be terminated and
//       various human-readable information about
extern __noreturn void __kos_unhandled_exception(void);


#ifdef __INTELLISENSE__
extern     struct kexinfo kexcept_current[];         /*< Thread-local storage for the current exception. */
extern            kexno_t kexcept_code(void);        /*< Returns the current exception code or ZERO(0) if none are being handled. (Useful for __except's 'should_handle' argument) */
extern                int kexcept_handling(void);    /*< Returns non-ZERO(0) if an unhandled exception is being handled in the calling thread. */
extern __noreturn    void kexcept_rethrow(void);     /*< Rethrow the currently set exception. */
extern __noreturn    void kexcept_continue(void);    /*< Continue execution where the exception occurred (WARNING: Be _very_ careful when calling this!). */
#define __kexcept_save(eip_lavel) __xblock({ goto eip_label; }) /*< Save all registers to the thread-local exception state, but set the label 'eip_label' as EIP. */
extern __noreturn    void __kexcept_throw(struct kexinfo const *__restrict exc); /*< Throw the given exception without saving registers. */
extern               void kexcept_throw(struct kexinfo const *__restrict exc); /*< Throw the given exception (WARNING: This may if 'kexcept_continue' is called). */
extern               void kexcept_raise(kexno_t no); /*< Small helper functions that simply wraps around 'kexcept_throw'. */
#else
#define kexcept_current    (&_tls_self->u_exinfo)
#define kexcept_code()       _tls_getl(KUTHREAD_OFFSETOF_EXINFO+KEXINFO_OFFSETOF_NO)
#define kexcept_handling() (kexcept_code() != KEXCEPTION_NONE)

__local __noreturn void kexcept_rethrow __D0() {
 struct kexrecord *handler;
 assertf(kexcept_code() != KEXCEPTION_NONE,
         "Not handling any exceptions right now");
 /* Load the exception handler. */
 handler = (struct kexrecord *)_tls_getl(8);
 if __unlikely(!handler) __kos_unhandled_exception();
 /* Invoke the exception handler. */
 __asm_volatile__("mov %0, %%esp\n"
                  "pop %%" __KEXCEPT_HANDLER_ADDR "\n"
                  "ret\n" : : "r" (handler));
 __compiler_unreachable();
}
__local __noreturn void __kexcept_throw __D1(struct kexinfo const *,__exc) {
 memcpy(kexcept_current,__exc,sizeof(struct kexinfo));
 kexcept_rethrow();
}

#ifdef __i386__
#define __KX_SM(x) "%%" __TLS_REGISTER_S ":(" __PP_STR(KUTHREAD_OFFSETOF_EXSTATE+x) ")" /* StateMember */
#define __kexcept_save(eip_label) \
 __xblock({ __asm_goto__("mov %%edi," __KX_SM(KEXSTATE_OFFSETOF_EDI) "\n"\
                         "mov %%esi," __KX_SM(KEXSTATE_OFFSETOF_ESI) "\n"\
                         "mov %%ebp," __KX_SM(KEXSTATE_OFFSETOF_EBP) "\n"\
                         "mov %%esp," __KX_SM(KEXSTATE_OFFSETOF_ESP) "\n"\
                         "mov %%ebx," __KX_SM(KEXSTATE_OFFSETOF_EBX) "\n"\
                         "mov %%edx," __KX_SM(KEXSTATE_OFFSETOF_EDX) "\n"\
                         "mov %%ecx," __KX_SM(KEXSTATE_OFFSETOF_ECX) "\n"\
                         "mov %%eax," __KX_SM(KEXSTATE_OFFSETOF_EAX) "\n"\
                         "movl $%l0," __KX_SM(KEXSTATE_OFFSETOF_EIP) "\n"\
                         "pushf"                                     "\n"\
                         "pop "    __KX_SM(KEXSTATE_OFFSETOF_EFLAGS) "\n"\
                         : : : "memory" : eip_label);\
            (void)0;\
 })
__forcelocal __noreturn void kexcept_continue __D0() {
 assertf(kexcept_code() != KEXCEPTION_NONE,
         "Not handling any exceptions right now");
 __asm_volatile__("pushl " __KX_SM(KEXSTATE_OFFSETOF_EFLAGS)  "\n"
                  "popf"                                      "\n"
                  "mov " __KX_SM(KEXSTATE_OFFSETOF_EAX) ",%%eax\n"
                  "mov " __KX_SM(KEXSTATE_OFFSETOF_ECX) ",%%ecx\n"
                  "mov " __KX_SM(KEXSTATE_OFFSETOF_EDX) ",%%edx\n"
                  "mov " __KX_SM(KEXSTATE_OFFSETOF_EBX) ",%%ebx\n"
                  "mov " __KX_SM(KEXSTATE_OFFSETOF_ESP) ",%%esp\n"
                  "mov " __KX_SM(KEXSTATE_OFFSETOF_EBP) ",%%ebp\n"
                  "mov " __KX_SM(KEXSTATE_OFFSETOF_ESI) ",%%esi\n"
                  "mov " __KX_SM(KEXSTATE_OFFSETOF_EDI) ",%%edi\n"
                  "jmp *" __KX_SM(KEXSTATE_OFFSETOF_EIP) "\n"
                  : : : "memory");
 __compiler_unreachable();
}
#else /* __i386__ */
#error "FIXME"
#endif /* arch... */
__local void kexcept_throw __D1(struct kexinfo const *,__exc) {
 assertf(__exc->ex_no != KEXCEPTION_NONE,
         "Can't throw exception code ZERO(0)");
 __kexcept_save(__done);
 __kexcept_throw(__exc);
__done:;
}
__local void kexcept_raise __D1(kexno_t,__no) {
 struct kexinfo __exc;
 memset(&__exc,0,sizeof(struct kexinfo));
 __exc.ex_no = __no;
 kexcept_throw(&__exc);
}
#endif
#endif /* !__ASSEMBLY__ */
#endif /* !__KERNEL__ */

__DECL_END

#endif /* !__KOS_EXCEPTION_H__ */

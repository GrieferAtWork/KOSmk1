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
#ifndef __EXCEPTION_H__
#define __EXCEPTION_H__ 1

#include <kos/config.h>
#ifndef __KERNEL__
#include <kos/compiler.h>
#ifndef __ASSEMBLY__
#include <kos/exception.h>

#ifdef __STDC_PURE__
#warning "This header is only available on KOS platforms."
#endif

__DECL_BEGIN

#ifndef __ptbwalker_defined
#define __ptbwalker_defined 1
typedef int (*ptbwalker) __P((void const *__restrict __instruction_pointer,
                              void const *__restrict __frame_address,
                              __size_t __frame_index, void *__closure));
#endif /* !__ptbwalker_defined */
#ifndef __ptberrorhandler_defined
#define __ptberrorhandler_defined 1
typedef int (*ptberrorhandler) __P((int __error, void const *__arg, void *__closure));
#endif /* !__ptberrorhandler_defined */

typedef kexno_t        exno_t;
typedef struct kexinfo exception_t;

/* Reminder: 'exc_raise' is just a wrapper around 'exc_throw'! */
extern __noreturn void exc_throw __P((exception_t const *__restrict __exception));
extern __noreturn void exc_raise __P((exno_t __exception_number));
extern void exc_throw_resumeable __P((exception_t const *__restrict __exception));
extern void exc_raise_resumeable __P((exno_t __exception_number));
extern __noreturn void exc_rethrow __P((void));
extern __noreturn void exc_continue __P((void));

//////////////////////////////////////////////////////////////////////////
// Helper functions for walking/printing/capturing
// tracebacks from finally/exception handlers:
// >> __try {
// >>   ((char *)0xdeadbeef) = 42;
// >> } __except (1) {
// >>   exc_tbprint(0);
// >> }
// @param: TP_ONLY: Only print the traceback; don't include error description/additional info.
extern int exc_tbwalk __P((ptbwalker __callback, ptberrorhandler __handle_error, void *__closure));
extern int exc_tbprint __P((int __tp_only));
extern int exc_tbprintex __P((int __tp_only, void *__closure));
extern __wunused __malloccall struct tbtrace *exc_tbcapture __P((void));
extern __wunused char const *exc_getname __P((exno_t __exception_number));

//////////////////////////////////////////////////////////////////////////
// Prints some additional information specified by the
// given format string before calling 'exc_tbprint(0)'
// NOTE: The given format string should contain human-readable
//       information concerning the thread and context that
//       the exception occurred under, such as local variable
//       and argument values.
extern int exc_printf __P((char const *__restrict __fmt, ...));


#define exc_try       KEXCEPT_TRY
#define exc_try_h     KEXCEPT_TRY_H
#define exc_leave     KEXCEPT_LEAVE
#define exc_finally   KEXCEPT_FINALLY
#define exc_except    KEXCEPT_EXCEPT

#undef __KEXCEPT_EXCEPT_BEGIN_RETHROW
#define __KEXCEPT_EXCEPT_BEGIN_RETHROW \
 exc_rethrow() /*< Reduce binary size by not inlining this. */

#if 0  /* Not really a good idea... */
#ifndef __STDC_PURE__
#define try      __try
#define finally  __finally
#define except   __except
#define leave    __leave
#endif
#endif

#ifdef __INTELLISENSE__
extern exception_t exc_current[];      /*< Thread-local storage for the current exception. */
extern     kexno_t exc_code(void);     /*< Returns the current exception code or ZERO(0) if none are being handled. (Useful for __except's 'should_handle' argument) */
extern         int exc_handling(void); /*< Returns non-ZERO(0) if an unhandled exception is being handled in the calling thread. */
#else
#define exc_current  kexcept_current
#define exc_code     kexcept_code
#define exc_handling kexcept_handling
#define exc_continue kexcept_continue  /*< Inline this one to prevent it from touching the stack too much. */
#endif

__DECL_END

#endif /* !__ASSEMBLY__ */
#endif /* !__KERNEL__ */

#endif /* !__EXCEPTION_H__ */

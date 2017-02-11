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
#ifndef __TRACEBACK_H__
#define __TRACEBACK_H__ 1

#include <kos/compiler.h>
#include <kos/types.h>

__DECL_BEGIN

#ifndef __ASSEMBLY__
#ifndef __pdebug_stackwalker_defined
#define __pdebug_stackwalker_defined 1
typedef int (*_ptraceback_stackwalker_d) __P((void const *__restrict __instruction_pointer,
                                              void const *__restrict __frame_address,
                                              __size_t __frame_index, void *__closure));
#endif
extern int _traceback_errorentry_d __P((int __error, void const *__arg, void *__closure));
#endif /* !__ASSEMBLY__ */

#define KDEBUG_STACKERROR_NONE    0
#define KDEBUG_STACKERROR_EIP   (-1) /*< Invalid instruction pointer (arg: eip). */
#define KDEBUG_STACKERROR_LOOP  (-2) /*< Stackframes are looping (arg: first looping frame). */
#define KDEBUG_STACKERROR_FRAME (-3) /*< A stackframe points to an invalid address (arg: addr). */

#ifndef __ASSEMBLY__
typedef int (*_ptraceback_stackerror_d) __P((int error, void const *arg, void *closure));

// NOTE: Printing a traceback sends specially formatted commands over the serial port.
//       In this situation, the kernel expects the 'magic.dee' script to sit at
//       the other end and parse those command to generate a human-readable (as well
//       as visual studio clickable) backtrace that is also dumped through OutputDebugString.
//       >> Essentially, you must run the kernel through the magic.dee script
//          to have the backtrace show up in the console and Visual Studio.
extern void _printtraceback_d __P((void));
extern void _printtracebackex_d __P((__size_t __skip));
extern __nonnull((1)) void _printtracebackebp_d __P((void const *__ebp));
extern __nonnull((1)) void _printtracebackebpex_d __P((void const *__ebp, __size_t __skip));
extern int _walktraceback_d __P((_ptraceback_stackwalker_d __callback,
                                 _ptraceback_stackerror_d __handle_error,
                                 void *__closure, __size_t __skip));
extern __nonnull((1)) int
_walktracebackebp_d __P((void const *__ebp, _ptraceback_stackwalker_d __callback,
                         _ptraceback_stackerror_d __handle_error, void *__closure,
                         __size_t __skip));

#ifdef __KERNEL__
struct ktask;
extern __nonnull((1)) void _printtracebacktask_d __P((struct ktask *__restrict __task));
extern __nonnull((1)) void _printtracebacktaskex_d __P((struct ktask *__restrict __task, __size_t __skip));
extern __nonnull((1)) int
_walktracebacktask_d __P((struct ktask *__restrict __task, _ptraceback_stackwalker_d __callback,
                          _ptraceback_stackerror_d __handle_error, void *__closure,
                          __size_t __skip));
#endif

//////////////////////////////////////////////////////////////////////////
// Capture, walk and print tracebacks manually.
// >> Can be used to track resources that must be released, such as locks.
struct dtraceback;
extern __malloccall struct dtraceback *dtraceback_capture __P((void));
extern __malloccall struct dtraceback *dtraceback_captureex __P((__size_t __skip));
extern void dtraceback_free __P((struct dtraceback *__restrict __self));
extern int dtraceback_walk __P((struct dtraceback const *__restrict __self,
                                _ptraceback_stackwalker_d __callback, void *__closure));
extern void dtraceback_print __P((struct dtraceback const *__restrict __self));

#endif /* !__ASSEMBLY__ */

__DECL_END

#endif /* !__TRACEBACK_H__ */

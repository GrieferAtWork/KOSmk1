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
typedef int (*ptbwalker) __P((void const *__restrict __instruction_pointer,
                              void const *__restrict __frame_address,
                              __size_t __frame_index, void *__closure));
#endif
typedef int (*ptberrorhandler) __P((int error, void const *arg, void *closure));
#endif /* !__ASSEMBLY__ */

#define TRACEBACK_STACKERROR_NONE    0
#define TRACEBACK_STACKERROR_EIP   (-2) /*< Invalid instruction pointer (arg: eip). */
#define TRACEBACK_STACKERROR_LOOP  (-3) /*< Stackframes are looping (arg: first looping frame). */
#define TRACEBACK_STACKERROR_FRAME (-4) /*< A stackframe points to an invalid address (arg: addr). */

#ifndef __ASSEMBLY__

//////////////////////////////////////////////////////////////////////////
// Default callbacks for print/error
// NOTE: Unless 'CLOSURE' is NULL, text will be printed to
//       the system log with KLOG_ERROR and stderr in usermode,
//       or to serial output 01 and the tty in kernel mode.
//       When not NULL, it is treated as a 'FILE *' to print into instead.
// HINT: The implementation uses the kernel to figure out debug information (mod_addrinfo).
//tbdef_print:
//   >> print "{file}({line}) : {func} : [{frame_index}][{frame_address}] : {eip}".format({ ... });
//tbdef_error:
//   >> print get_error_message(error);
//   >> return error;
extern int tbdef_print __P((void const *__restrict __instruction_pointer,
                            void const *__restrict __frame_address,
                            __size_t __frame_index, void *__closure));
extern int tbdef_error __P((int __error, void const *__arg, void *__closure));


//////////////////////////////////////////////////////////////////////////
// Print a traceback
extern                int tb_print      __P((void));
extern                int tb_printex    __P((__size_t __skip));
extern __nonnull((1)) int tb_printebp   __P((void const *__ebp));
extern __nonnull((1)) int tb_printebpex __P((void const *__ebp, __size_t __skip));


extern                int tb_walk      __P((ptbwalker __callback, ptberrorhandler __handle_error, void *__closure));
extern                int tb_walkex    __P((ptbwalker __callback, ptberrorhandler __handle_error, void *__closure, __size_t __skip));
extern __nonnull((1)) int tb_walkebp   __P((void const *__ebp, ptbwalker __callback, ptberrorhandler __handle_error, void *__closure));
extern __nonnull((1)) int tb_walkebpex __P((void const *__ebp, ptbwalker __callback, ptberrorhandler __handle_error, void *__closure, __size_t __skip));

//////////////////////////////////////////////////////////////////////////
// Capture, walk and print tracebacks manually.
// >> Can be used to track resources that must be released, such as locks.
struct tbtrace;
extern __malloccall struct tbtrace *tbtrace_capture __P((void));
extern __malloccall struct tbtrace *tbtrace_captureex __P((__size_t __skip));
extern __malloccall struct tbtrace *tbtrace_captureebp __P((void const *__ebp));
extern __malloccall struct tbtrace *tbtrace_captureebpex __P((void const *__ebp, __size_t __skip));
extern int tbtrace_walk __P((struct tbtrace const *__restrict __self, ptbwalker __callback, void *__closure));
extern int tbtrace_print __P((struct tbtrace const *__restrict __self));

#endif /* !__ASSEMBLY__ */

__DECL_END

#endif /* !__TRACEBACK_H__ */

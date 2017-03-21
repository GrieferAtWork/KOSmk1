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

__DECL_BEGIN

#undef exc_continue

/* As simple as it gets: These are literally just small library wrapper around system functions. */
__public __noreturn void exc_throw(exception_t const *__restrict exception) { for (;;) exc_throw_resumeable(exception); }
__public __noreturn void exc_raise(exno_t exception_number) { for (;;) exc_raise_resumeable(exception_number); }
__public void exc_throw_resumeable(exception_t const *__restrict exception) { kexcept_throw(exception); }
__public void exc_raise_resumeable(exno_t exception_number) { kexcept_raise(exception_number); }
__public __noreturn void exc_rethrow(void) { kexcept_rethrow(); }
__public __noreturn void exc_continue(void) { kexcept_continue(); }

__DECL_END
#endif /* !__KERNEL__ */

#endif /* !__EXCEPTION_C__ */

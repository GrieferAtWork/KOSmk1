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
#ifndef __KOS_ASSERT_H__
#define __KOS_ASSERT_H__ 1

#include <kos/compiler.h>
#include <kos/config.h>

__DECL_BEGIN

#ifndef __ASSEMBLY__
#if defined(__DEBUG__) && !defined(__NO_LIBC__)
extern __noinline __noclone __noreturn __coldcall
#if    __LIBC_HAVE_DEBUG_PARAMS == 3
       __attribute_vaformat(__printf__,6,7)
#elif  __LIBC_HAVE_DEBUG_PARAMS == 0
       __attribute_vaformat(__printf__,3,4)
#else
#      error "FIXME"
#endif
void __assertion_failedf __P((__LIBC_DEBUG_PARAMS_ char const *expr,
                              unsigned int skip, char const *fmt, ...));
extern __noinline __noclone __noreturn __coldcall __nonnull((1,3))
void __builtin_unreachable_d __P((__LIBC_DEBUG_PARAMS));
#ifndef __INTELLISENSE__
#   define __builtin_unreachable() __builtin_unreachable_d(__LIBC_DEBUG_ARGS)
#endif
#   define __assert(expr)                      (!(expr)?__assertion_failedf(__LIBC_DEBUG_ARGS_ #expr,0,(char const *)0):(void)0)
#   define __assertf(expr,...)                 (!(expr)?__assertion_failedf(__LIBC_DEBUG_ARGS_ #expr,0,__VA_ARGS__):(void)0)
#   define __assert_at(sexpr,skip,expr)        (!(expr)?__assertion_failedf(__LIBC_DEBUG_FWD_ sexpr,skip,(char const *)0):(void)0)
#   define __assert_atf(sexpr,skip,expr,...)   (!(expr)?__assertion_failedf(__LIBC_DEBUG_FWD_ sexpr,skip,__VA_ARGS__):(void)0)
#   define __assert_here(sexpr,skip,expr)      (!(expr)?__assertion_failedf(__LIBC_DEBUG_ARGS_ sexpr,skip,(char const *)0):(void)0)
#   define __assert_heref(sexpr,skip,expr,...) (!(expr)?__assertion_failedf(__LIBC_DEBUG_ARGS_ sexpr,skip,__VA_ARGS__):(void)0)
#elif defined(__OPTIMIZE__)
#   define __assert(expr)                 __compiler_assume(expr)
#   define __assertf(expr,...)            __compiler_assume(expr)
#   define __assert_at(sexpr,expr)        __compiler_assume(expr)
#   define __assert_atf(sexpr,expr,...)   __compiler_assume(expr)
#   define __assert_here(sexpr,expr)      __compiler_assume(expr)
#   define __assert_heref(sexpr,expr,...) __compiler_assume(expr)
#else
#   define __assert(expr)                 (void)0
#   define __assertf(expr,...)            (void)0
#   define __assert_at(sexpr,expr)        (void)0
#   define __assert_atf(sexpr,expr,...)   (void)0
#   define __assert_here(sexpr,expr)      (void)0
#   define __assert_heref(sexpr,expr,...) (void)0
#endif
#endif /* !__ASSEMBLY__ */

__DECL_END

#endif /* !__KOS_ASSERT_H__ */

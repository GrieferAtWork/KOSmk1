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


#ifndef __ASSEMBLY__
#if defined(__DEBUG__) && !defined(__NO_LIBC__)

__DECL_BEGIN

#ifdef __ASSERT_C__
/* Hack to prevent warnings about assert returning during recursion.
 * >> Used to prevent infinite recursion when an assertion fails ~really~ bad.
 * Note though, that we don't tell usercode about this, because it's
 * OK if everything breaks after everything's already broken. */
#define __ASSERT_NORETURN /* nothing */
#else /* __ASSERT_C__ */
#define __ASSERT_NORETURN __noreturn
#endif /* !__ASSERT_C__ */

extern __noinline __noclone __ASSERT_NORETURN __coldcall
#if    __LIBC_HAVE_DEBUG_PARAMS == 3
       __attribute_vaformat(__printf__,6,7)
#elif  __LIBC_HAVE_DEBUG_PARAMS == 0
       __attribute_vaformat(__printf__,3,4)
#else
#      error "FIXME"
#endif
void __assertion_failedf __P((__LIBC_DEBUG_PARAMS_ char const *__expr,
                              unsigned int __skip, char const *__fmt, ...));
extern __noinline __noclone __ASSERT_NORETURN __coldcall
#if    __LIBC_HAVE_DEBUG_X_PARAMS == 1
       __attribute_vaformat(__printf__,4,5)
#elif  __LIBC_HAVE_DEBUG_X_PARAMS == 0
       __attribute_vaformat(__printf__,3,4)
#else
#      error "FIXME"
#endif
void __assertion_failedxf __P((__LIBC_DEBUG_X_PARAMS_ char const *__expr,
                               unsigned int __skip, char const *__fmt, ...));
extern __noinline __noclone __ASSERT_NORETURN __coldcall
#if __LIBC_HAVE_DEBUG_PARAMS == 3
       __nonnull((1,3))
#endif
void __libc_unreachable_d __P((__LIBC_DEBUG_PARAMS));
#undef __ASSERT_NORETURN

__DECL_END

#ifndef __INTELLISENSE__
#   undef  __compiler_unreachable
#   define __compiler_unreachable() __libc_unreachable_d(__LIBC_DEBUG_ARGS)
#endif
#ifdef __LIBC_DEBUG_X_ARGS_
#   define __assert(expr)                      (!(expr)?__assertion_failedxf(__LIBC_DEBUG_X_ARGS_ #expr,0,(char const *)0):(void)0)
#   define __assertf(expr,...)                 (!(expr)?__assertion_failedxf(__LIBC_DEBUG_X_ARGS_ #expr,0,__VA_ARGS__):(void)0)
#else
#   define __assert(expr)                      (!(expr)?__assertion_failedf(__LIBC_DEBUG_ARGS_ #expr,0,(char const *)0):(void)0)
#   define __assertf(expr,...)                 (!(expr)?__assertion_failedf(__LIBC_DEBUG_ARGS_ #expr,0,__VA_ARGS__):(void)0)
#endif
#   define __assert_at(sexpr,skip,expr)        (!(expr)?__assertion_failedf(__LIBC_DEBUG_FWD_ sexpr,skip,(char const *)0):(void)0)
#   define __assert_atf(sexpr,skip,expr,...)   (!(expr)?__assertion_failedf(__LIBC_DEBUG_FWD_ sexpr,skip,__VA_ARGS__):(void)0)
#   define __assert_xat(sexpr,skip,expr)       (!(expr)?__assertion_failedxf(__LIBC_DEBUG_X_FWD_ sexpr,skip,(char const *)0):(void)0)
#   define __assert_xatf(sexpr,skip,expr,...)  (!(expr)?__assertion_failedxf(__LIBC_DEBUG_X_FWD_ sexpr,skip,__VA_ARGS__):(void)0)
#ifdef __LIBC_DEBUG_X_ARGS_
#   define __assert_here(sexpr,skip,expr)      (!(expr)?__assertion_failedxf(__LIBC_DEBUG_X_ARGS_ sexpr,skip,(char const *)0):(void)0)
#   define __assert_heref(sexpr,skip,expr,...) (!(expr)?__assertion_failedxf(__LIBC_DEBUG_X_ARGS_ sexpr,skip,__VA_ARGS__):(void)0)
#else
#   define __assert_here(sexpr,skip,expr)      (!(expr)?__assertion_failedf(__LIBC_DEBUG_ARGS_ sexpr,skip,(char const *)0):(void)0)
#   define __assert_heref(sexpr,skip,expr,...) (!(expr)?__assertion_failedf(__LIBC_DEBUG_ARGS_ sexpr,skip,__VA_ARGS__):(void)0)
#endif
#elif defined(__OPTIMIZE__)
#   define __assert(expr)                 __compiler_assume(expr)
#   define __assertf(expr,...)            __compiler_assume(expr)
#   define __assert_at(sexpr,expr)        __compiler_assume(expr)
#   define __assert_atf(sexpr,expr,...)   __compiler_assume(expr)
#   define __assert_xat(sexpr,expr)       __compiler_assume(expr)
#   define __assert_xatf(sexpr,expr,...)  __compiler_assume(expr)
#   define __assert_here(sexpr,expr)      __compiler_assume(expr)
#   define __assert_heref(sexpr,expr,...) __compiler_assume(expr)
#else
#   define __assert(expr)                 (void)0
#   define __assertf(expr,...)            (void)0
#   define __assert_at(sexpr,expr)        (void)0
#   define __assert_atf(sexpr,expr,...)   (void)0
#   define __assert_xat(sexpr,expr)       (void)0
#   define __assert_xatf(sexpr,expr,...)  (void)0
#   define __assert_here(sexpr,expr)      (void)0
#   define __assert_heref(sexpr,expr,...) (void)0
#endif
#endif /* !__ASSEMBLY__ */

#endif /* !__KOS_ASSERT_H__ */

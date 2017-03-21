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
#ifndef __GCC_VERSION
#ifdef __GNUC__
#ifndef __GNUC_MINOR__
#   define __GNUC_MINOR__ 0
#endif
#ifndef __GNUC_PATCH__
#   define __GNUC_PATCH__ 0
#endif
#   define __GCC_VERSION_NUM    (__GNUC__*10000+__GNUC_MINOR__*100+__GNUC_PATCH__)
#   define __GCC_VERSION(a,b,c) (__GCC_VERSION_NUM >= ((a)*10000+(b)*100+(c)))
#else /* __GNUC__ */
#   define __GCC_VERSION(a,b,c) 0
#endif /* !__GNUC__ */
#endif /* !__GCC_VERSION */

#ifndef __COMPILER_HAVE_PRAGMA_PUSH_MACRO
#if __GCC_VERSION(4,4,0)
#define __COMPILER_HAVE_PRAGMA_PUSH_MACRO
#endif
#endif

#if !__GCC_VERSION(2,8,0)
#define __extension__
#endif
#ifndef __compiler_FUNCTION
#if 1
#define __compiler_FUNCTION()     __func__
#else
#define __compiler_FUNCTION()     __builtin_FUNCTION()
#endif
#endif
#ifndef __public
#define __public        __attribute__((__visibility__("default")))
#endif
#ifndef __private
#define __private       __attribute__((__visibility__("hidden")))
#endif
#ifdef __PE__
#ifndef __export
#define __export        __attribute__((__dllexport__))
#endif
#ifndef __import
#define __import        __attribute__((__dllimport__))
#endif
#endif
#ifndef __noreturn
#define __noreturn      __attribute__((__noreturn__))
#endif
#ifndef __cdecl
#define __cdecl         __attribute__((__cdecl__))
#endif
#ifndef __nonnull
#define __nonnull(ids)  __attribute__((__nonnull__ ids))
#endif
#ifndef __noinline
#define __noinline      __attribute__((__noinline__))
#endif
#ifndef __noclone
#if __GCC_VERSION(4,5,0)
#define __noclone       __attribute__((__noclone__))
#endif
#endif
#ifndef __purecall
#define __purecall      __attribute__((__pure__))
#endif
#ifndef __constcall
#define __constcall     __attribute__((__const__))
#endif
#ifndef __malloccall
#define __malloccall    __attribute__((__malloc__))
#endif
#ifndef __coldcall
#if __GCC_VERSION(4,3,0)
#define __coldcall      __attribute__((__cold__))
#endif
#endif
#ifndef __hotcall
#if __GCC_VERSION(4,3,0)
#define __hotcall       __attribute__((__hot__))
#endif
#endif
#ifndef __sizemalloccall
#define __sizemalloccall(xy)  __attribute__((__malloc__,__alloc_size__ xy))
#endif
#ifndef __wunused
#if __GCC_VERSION(3,4,0)
#define __wunused       __attribute__((__warn_unused_result__))
#endif
#endif
#ifndef __retnonnull
#define __retnonnull    __attribute__((__returns_nonnull__))
#endif
#ifndef __packed
#define __packed        __attribute__((__packed__))
#endif
#ifndef __attribute_vaformat
#define __attribute_vaformat(func,fmt_idx,varargs_idx) __attribute__((__format__(func,fmt_idx,varargs_idx)))
#endif
#if __GCC_VERSION(3,3,0)
#ifndef __attribute_used
#define __attribute_used          __attribute__((__used__))
#endif
#ifndef __attribute_unused
#define __attribute_unused        __attribute__((__unused__))
#endif
#endif
#ifndef __attribute_forceinline
#define __attribute_forceinline   __attribute__((__always_inline__)) __inline__
#endif
#ifndef __attribute_sentinel
#if __has_attribute(sentinel) || __GCC_VERSION(3,5,0)
#define __attribute_sentinel      __attribute__((__sentinel__))
#endif
#endif
#ifndef __attribute_thread
#define __attribute_thread        __thread
#endif
#ifndef __attribute_section
#define __attribute_section(name) __attribute__((__section__(name)))
#endif
#ifndef __attribute_inline
#if __GCC_VERSION(4,0,0)
#define __attribute_inline        __inline__
#else
#define __attribute_inline        __attribute_forceinline
#endif
#endif
#ifndef __attribute_aligned_a
#if __GCC_VERSION(5,4,0)
/* Returns pointer aligned by at least the value of argument 'x' (1-based) */
#define __attribute_aligned_a(x) __attribute__((__alloc_align__(x)))
#endif
#endif /* !__attribute_aligned_a */
#ifndef __attribute_aligned_c
#if __GCC_VERSION(4,9,0)
/* Returns pointer aligned by at least 'x' */
#define __attribute_aligned_c(x) __attribute__((__assume_aligned__(x)))
#endif
#endif /* !__attribute_aligned_c */

#ifndef __asmname
#define __asmname(x) __asm__(x)
#endif

#ifndef __evalexpr
#ifdef __INTELLISENSE__
#define __evalexpr /* nothing */
#else
#define __evalexpr(x) \
 __xblock({ __attribute_unused __typeof__(x) __x = (x);\
            __xreturn __x;\
 })
#endif
#endif

#ifndef __likely
#define __likely(x)   (__builtin_expect(!!(x),1))
#endif
#ifndef __unlikely
#define __unlikely(x) (__builtin_expect(!!(x),0))
#endif

#ifndef __COMPILER_ALIAS
#define __COMPILER_ALIAS(new_name,old_name) \
 __typeof__(old_name) new_name __attribute__((/*__weak__,*/ __alias__(#old_name)));
#endif


#ifndef __COMPILER_UNIQUE
#ifdef __COUNTER__
#define __COMPILER_UNIQUE(group) __PP_CAT_3(__hidden_,group,__COUNTER__)
#endif
#endif /* !__COMPILER_UNIQUE */

#ifndef __STATIC_ASSERT
#define __STATIC_ASSERT(expr) \
 extern __attribute_unused int __COMPILER_UNIQUE(sassert)[!!(expr)?1:-1]
#endif

#ifndef __xreturn
#define __xreturn         /* nothing */
#endif
#ifndef __xblock
#define __xblock          __extension__
#endif
#ifndef __asm_goto__
#define __asm_goto__      __asm__ goto
#endif
#ifndef __asm_volatile__
#define __asm_volatile__  __asm__ __volatile__
#endif

#ifndef __compiler_assume
#if __has_builtin(__builtin_assume)
#define __compiler_assume   __builtin_assume
#endif
#endif

#ifndef __INTELLISENSE__
#ifndef __fastcall
#define __fastcall  __attribute__((__fastcall__))
#endif
#ifndef __STATIC_ASSERT_M
#define __STATIC_ASSERT_M(expr,msg) \
 __xblock({ extern __attribute__((__error__(msg))) void STATIC_ASSERTION_FAILED(void);\
            if (!(expr)) STATIC_ASSERTION_FAILED();\
            (void)0;\
 })
#endif
#endif

#ifndef __compiler_barrier
#define __compiler_barrier() __asm_volatile__("" : : : "memory")
#endif
#ifndef __compiler_unreachable
#define __compiler_unreachable  __builtin_unreachable
#endif

#define __COMPILER_HAVE_TYPEOF

#ifndef __compiler_pragma
#define __PRIVATE_COMPILER_PRAGMA(x) _Pragma(#x)
#define __compiler_pragma(x) __PRIVATE_COMPILER_PRAGMA(x)
#endif

#ifndef __COMPILER_PACK_PUSH
#define __COMPILER_PACK_PUSH(n) __compiler_pragma(pack(push,n))
#define __COMPILER_PACK_POP     _Pragma("pack(pop)")
#endif

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
#ifndef __KOS_COMPILER_H__
#define __KOS_COMPILER_H__ 1

#include <kos/config.h>

#ifndef __has_builtin
#define __has_builtin(x) 0
#endif
#ifndef __has_feature
#define __has_feature(x) 0
#endif
#ifndef __has_extension
#define __has_extension(x) 0
#endif
#ifndef __has_attribute
#define __has_attribute(x) 0
#endif

#if __has_feature(__tpp_pragma_push_macro__) || defined(_MSC_VER)\
 || (defined(__TPP_VERSION__) && __TPP_VERSION__ == 103) /* Before "__has_feature" */
#   define __COMPILER_HAVE_PRAGMA_PUSH_MACRO
#endif

#ifdef __TPP_EVAL /*< TPP FTW! */
#   define __PP_MUL8(x)  __TPP_EVAL((x)*8)
#endif


// IDE
#ifdef __INTELLISENSE__
#include "compiler-intellisense.h"
#endif /* __INTELLISENSE__ */

// Language
#ifdef __ASSEMBLY__
#include "compiler-asm.h"
#endif /* __ASSEMBLY__ */
#ifdef __cplusplus
#include "compiler-cxx.h"
#endif /* __cplusplus */

// Compiler
#ifdef __TPP_VERSION__
#include "compiler-tpp.h"
#endif /* __TPP_VERSION__ */
#ifdef __GNUC__
#include "compiler-gcc.h"
#endif /* __GNUC__ */
#ifdef _MSC_VER
#include "compiler-msvc.h"
#endif /* _MSC_VER */
#include "compiler-predef.h"

// Ansi vs. Non-Ansi
#if defined(__STDC__) || defined(__cplusplus) || \
    defined(__DEEMON__) || defined(__ANSI_COMPILER__) || \
    defined(__TCC__) || defined(__GNUC__) || \
    defined(__clang__) || defined(_MSC_VER)
#ifndef __ANSI_COMPILER__
#define __ANSI_COMPILER__ 1
#endif
#include "compiler-ansi.h"
#else /* Ansi... */
#include "compiler-nonansi.h"
#endif /* !Ansi... */

#ifdef __CHECKER__
#include "compiler-sparse.h"
#endif

#ifndef __i386__
#undef __fastcall
#endif

#define __NO_float128

#ifndef __DECL_BEGIN
#define __DECL_BEGIN /* nothing */
#define __DECL_END   /* nothing */
#endif

#ifndef __COMPILER_PACK_PUSH
#define __COMPILER_PACK_PUSH(n) /* nothing */
#define __COMPILER_PACK_POP     /* nothing */
#endif

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

#ifndef __compiler_FUNCTION
#   define __NO_compiler_FUNCTION
#   define __compiler_FUNCTION() ""
#endif
#ifndef __compiler_assume
#   define __NO_compiler_assume
#   define __compiler_assume(expr) (void)0
#endif
#ifndef __COMPILER_ARRAYSIZE
#   define __COMPILER_ARRAYSIZE(x)  (sizeof(x)/sizeof(*(x)))
#endif
#ifndef __COMPILER_STRINGSIZE
#   define __COMPILER_STRINGSIZE(x) (__COMPILER_ARRAYSIZE(x)-1)
#endif
#ifndef __COMPILER_ALIAS
#   define __NO_compiler_ALIAS
#   define __COMPILER_ALIAS(new_name,old_name) /* nothing... */
#endif
#ifndef __COMPILER_UNIQUE
#   define __NO_compiler_UNIQUE
#   define __COMPILER_UNIQUE(group) __PP_CAT_3(__hidden_,group,__LINE__)
#endif

#ifndef __compiler_unreachable
#ifdef __noreturn
#   define __compiler_unreachable __compiler_unreachable_impl
static __noreturn 
#ifdef __attribute_forceinline
__attribute_forceinline
#elif defined(__attribute_inline)
__attribute_inline
#endif
void __compiler_unreachable_impl __D0() { for (;;) {} }
#else /* __noreturn */
#   define __NO_compiler_unreachable
#   define __compiler_unreachable() (void)0
#endif /* !__noreturn */
#endif /* !__compiler_unreachable */

#ifndef __attribute_vaformat
#   define __NO_attribute_vaformat
#   define __attribute_vaformat(func,fmt_idx,varargs_idx) /* nothing */
#elif !defined(__STDC_PURE__)
/* KOS adds a few extensions to printf-style functions.
 * >> And since we actually want to use them, hide the
 *    fact that it's a printf-function from GCC. */
#   undef  __attribute_vaformat
#   define __attribute_vaformat(func,fmt_idx,varargs_idx) /* nothing */
#endif

#ifndef __asmname
#   define __NO_asmname
#   define __asmname(x) /* nothing */
#endif
#ifndef __packed
#   define __NO_packed
#   define __packed /* nothing */
#endif
#ifndef __noreturn
#   define __NO_noreturn
#   define __noreturn /* nothing */
#endif
#ifndef __nonnull
#   define __NO_nonnull
#   define __nonnull(ids) /* nothing */
#endif
#ifndef __noinline
#   define __NO_noinline
#   define __noinline /* nothing */
#endif
#ifndef __noclone
#   define __NO_noclone
#   define __noclone /* nothing */
#endif
#ifndef __purecall
#   define __NO_purecall
#   define __purecall /* nothing */
#endif
#ifndef __constcall
#ifndef __NO_purecall
#   define __constcall __purecall
#else
#   define __NO_constcall
#   define __constcall /* nothing */
#endif
#endif
#ifndef __malloccall
#   define __NO_malloccall
#   define __malloccall /* nothing */
#endif
#ifndef __coldcall
#   define __NO_coldcall
#   define __coldcall /* nothing */
#endif
#ifndef __cxx11_constexpr
#   define __NO_cxx11_constexpr
#   define __cxx11_constexpr /* nothing */
#endif
#ifndef __cxx_noexcept
#   define __NO_cxx_noexcept
#   define __NO_cxx_noexcept_is
#   define __NO_cxx_noexcept_if
#   define __cxx_noexcept          /* nothing */
#   define __cxx_noexcept_is(expr) 0
#   define __cxx_noexcept_if(expr) /* nothing */
#endif
#ifndef __cxx14_constexpr
#   define __NO_cxx14_constexpr
#   define __cxx14_constexpr /* nothing */
#endif
#ifndef __sizemalloccall
#ifndef __NO_malloccall
#   define __sizemalloccall(xy)  __malloccall
#else
#   define __NO_sizemalloccall
#   define __sizemalloccall(xy) /* nothing */
#endif
#endif
#ifndef __wunused
#   define __NO_wunused
#   define __wunused /* nothing */
#endif
#ifndef __retnonnull
#   define __NO_retnonnull
#   define __retnonnull /* nothing */
#endif
#ifndef __attribute_unused
#   define __NO_attribute_unused
#   define __attribute_unused /* nothing */
#endif
#ifndef __attribute_inline
#   define __NO_attribute_inline
#   define __attribute_inline /* nothing */
#endif
#ifndef __attribute_sentinel
#   define __NO_attribute_sentinel
#   define __attribute_sentinel /* nothing */
#endif
#ifndef __attribute_forceinline
#ifndef __NO_attribute_inline
#   define __attribute_forceinline __attribute_inline
#else
#   define __NO_attribute_forceinline
#   define __attribute_forceinline /* nothing */
#endif
#endif
#ifndef __attribute_thread
#   define __NO_attribute_thread
#   define __attribute_thread /* nothing */
#endif
#ifndef __attribute_warning
#   define __NO_attribute_warning
#   define __attribute_warning(text) /* nothing */
#endif
#ifndef __attribute_error
#   define __NO_attribute_error
#   define __attribute_error(text) /* nothing */
#endif
#ifndef __attribute_section
#   define __NO_attribute_section
#   define __attribute_section(name) /* nothing */
#endif
#ifndef __attribute_aligned_a
#define __NO_attribute_aligned_a
#define __attribute_aligned_a(x) /* nothing */
#endif /* !__attribute_aligned_a */
#ifndef __attribute_aligned_c
#define __NO_attribute_aligned_c
#define __attribute_aligned_c(x) /* nothing */
#endif /* !__attribute_aligned_c */

/* Visibility attributes */
#ifndef __PE__
#ifdef __public
#ifndef __export
#   define __export __public
#endif
#ifndef __import
#   define __import __public
#endif
#endif /* __public */
#else /* !__PE__ */
/* Assuming usage in source files. */
#   undef __public
#   define __public  __export
#ifndef __private
#   define __private /* nothing */
#endif
#endif /* __PE__ */

#ifndef __export
#   define __NO_export
#   define __export /* nothing */
#endif
#ifndef __import
#   define __NO_import
#   define __import /* nothing */
#endif
#ifndef __public
#   define __NO_public
#   define __public __export
#endif
#ifndef __private
#   define __NO_private
#   define __private /* nothing */
#endif


#ifndef __local
#if defined(__DEBUG__) && 0
#   define __local        static __attribute_unused
#elif 0
#   define __local        static __attribute_forceinline
#else
#   define __local        static __attribute_inline
#endif
#endif /* !__local */
#ifndef __forcelocal
#   define __forcelocal   static __attribute_forceinline
#endif /* !__forcelocal */

#ifndef __evalexpr
#   define __evalexpr /* nothing */
#endif

#ifndef __unused
#if defined(__cplusplus) || defined(__DEEMON__)
#   define __unused(name)   /* nothing */
#elif !defined(__NO_attribute_unused)
#   define __unused(name)   name __attribute_unused
#elif defined(__LCLINT__)
#   define __unused(name)   /*@unused@*/ name
#elif defined(_MSC_VER)
#   define __unused(name)   name
#   pragma warning(disable: 4100)
#else
#   define __unused(name)   name
#endif
#endif

#ifndef __likely
#   define __NO_likely
#   define __likely /* nothing */
#endif
#ifndef __unlikely
#   define __NO_unlikely
#   define __unlikely /* nothing */
#endif

#ifndef __STATIC_ASSERT
#   define __STATIC_ASSERT(expr)   typedef int __COMPILER_UNIQUE(sassert)[!!(expr)?1:-1]
#endif
#ifndef __STATIC_ASSERT_M
#   define __STATIC_ASSERT_M(expr,msg) __STATIC_ASSERT(expr)
#endif

#ifndef __xblock
#   define __NO_xblock
#   define __xreturn     break;
#   define __xblock(...) do __VA_ARGS__ while(0)
#endif

#ifndef __ensuretype
#ifdef __INTELLISENSE__
#define __ensuretype(T,x) \
 __xblock({ T __etx = (x);\
            __xreturn __etx;\
 })
#else
#define __ensuretype(T,x) ((T)(x))
#endif
#endif /* !__ensuretype */

#ifndef __breakpoint
#ifdef __KERNEL__
#if 1 /* The one downside of using QEMU... */
#   define __breakpoint() __asm_volatile__("214743: jmp 214743b")
#else /* For BOCHS. */
#   define __breakpoint() __asm_volatile__("xchgw %bx, %bx")
#endif
#else /* User-land. */
#   define __breakpoint() __asm_volatile__("int $3")
#endif
#endif /* !__breakpoint */

#ifndef __compiler_barrier
#   define __compiler_barrier() (void)0
#endif

/* Marks a region of code containing only, or partially code
 * neither written, nor claimed by me (GrieferAtWork).
 * NOTE: These should be read as recursive, with every region
 *       started with '__NOCLAIM_BEGIN(*)' ending at the associated
 *       '__NOCLAIM_END', where everything '__NOCLAIM_BEGIN' has
 *       exactly one, nearest (afterwards) '__NOCLAIM_END' associated
 *       with it. Note that every '__NOCLAIM_END' can only be used once. */
#define __NOCLAIM_BEGIN(who)
#define __NOCLAIM_END


#ifndef __aligned
#define __aligned(n)    /* Attribute for pointer/integral: Must be aligned by 'n' */
#endif
#ifndef __user
#define __user          /* Attribute for pointer: User-space virtual memory address. */
#endif
#ifndef __kernel
#define __kernel        /* Attribute for pointer: Physical memory address. */
#endif
#ifndef __percpu
#define __percpu        /* Attribute for pointer: Per-cpu variable. */
#endif
#ifndef __rcu
#define __rcu           /* Attribute for pointer: Something about networks... (ignore for now) */
#endif
#ifndef __ref
#define __ref           /* Attribute for a pointer: If non-NULL, this is a reference; indicates reference storage/transfer. */
#endif
#ifndef __atomic
#define __atomic        /* Attribute for member/global variables: Access is atomic. */
#endif
#ifndef __nomp
#define __nomp          /* Attribute for functions: (NO)(M)ulti(P)rocessing: Function is not thread-safe. */
#endif
#ifndef __crit
/* Attribute for function: The caller must ensure that 'ktask_iscrit()' protection is enabled.
 * >> Failure to do so may result in a memory/resource leak when a different task terminates the calling.
 * NOTE: User-level code has no ability to guard itself from being terminated,
 *       meaning that this attribute is only meant to self-document kernel usage-rules.
 * HINT: To ensure this protected, functions marked with this may assert this state
 *       by simply inserting 'assert(ktask_iscrit())' at the start of their block. */
#define __crit          /* Attribute for functions: Requires a critical context. */
#endif

#ifndef __force
#define __force         /* Attribute for types: Force casting between otherwise incompatible types. */
#endif
#ifndef __nocast
#define __nocast        /* Attribute for types: Don't allow instance of this type to be casted. */
#endif
#ifndef __iomem
#define __iomem         /* I/O-mapped memory. */
#endif
#ifndef __bitwise__
#define __bitwise__     /* Attribute for integral types: Don't allow mixing with other types. */
#endif
#ifndef __chk_user_ptr
#define __NO_chk_user_ptr 1
#define __chk_user_ptr(x) (void)0
#endif
#ifndef __chk_io_ptr
#define __NO_chk_io_ptr 1
#define __chk_io_ptr(x)   (void)0
#endif
#ifndef __must_hold
#define __NO_must_hold 1
#define __must_hold(x)    /* Attribute for function: Calling this function requires the caller to hold a lock to 'x' */
#define __acquires(x)     /* Attribute for function: Calling this function acquires a lock to 'x'. */
#define __releases(x)     /* Attribute for function: Calling this function releases a lock to 'x'. */
#define __acquire(x)      (void)0
#define __release(x)      (void)0
#define __cond_lock(x,c)  (c)
#endif


#ifndef __bitwise
#ifdef __CHECK_ENDIAN__
#   define __bitwise      __bitwise__
#else
#   define __bitwise
#endif
#endif /* !__bitwise */

#ifdef __KERNEL__
#ifndef __KOS_KERNEL_STDKERNEL_H__
#include "kernel/stdkernel.h"
#endif
#endif

#endif /* !__KOS_COMPILER_H__ */

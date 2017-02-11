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
#ifndef __KOS_KERNEL_REGION_H__
#define __KOS_KERNEL_REGION_H__ 1

/*
 * Notice how this header doesn't have a #ifndef __KERNEL__ guard?
 * That's because it doesn't do anything that wouldn't work
 * for user-code just how it does for kernel-code!
 */

#include <kos/config.h>
#include <stdbool.h>
#include <kos/compiler.h>
#include <kos/kernel/features.h>
#include <kos/kernel/sched_yield.h>
#ifdef KCONFIG_HAVE_SINGLECORE
#include <assert.h>
#endif

__DECL_BEGIN

/* Values for nested & normal compile-time states. */
#define __REGION_NORMAL_NO  0 /* NORMAL: 0: not in region. */
#define __REGION_NORMAL_YES 1 /* NORMAL: 1: is in region. */
#define __REGION_NESTED_NO  0 /* NESTED: 0: not in region. */
#define __REGION_NESTED_ONE 1 /* NESTED: 1: in region; leave must look at runtime-based value. */
#define __REGION_NESTED_YES 2 /* NESTED: 2: in region; leave is recursive; code was already in region when region was entered. */
#define __REGION_BFIRST_NO  0 /* BFIRST: 0: Not the first level. */
#define __REGION_BFIRST_YES 1 /* BFIRST: 0: First level. */

#ifdef __ANSI_COMPILER__
#   define __REGION_IN_NORMAL(name) __REGION_##name##_NORMAL /*< Compile-time in-region detection. */
#   define __REGION_IN_NESTED(name) __REGION_##name##_NESTED /*< Compile-time nested-region detection. */
#   define __REGION_IN_BFIRST(name) __REGION_##name##_BFIRST /*< Compile-time first-region detection. */
#   define __REGION_IN_RUNTIM(name) __REGION_##name##_RUNTIM /*< Old in-region state compiled at runtime . */
#else /* __ANSI_COMPILER__ */
#   define __REGION_IN_NORMAL(name) __REGION_/**/name/**/_NORMAL
#   define __REGION_IN_NESTED(name) __REGION_/**/name/**/_NESTED
#   define __REGION_IN_BFIRST(name) __REGION_/**/name/**/_BFIRST
#   define __REGION_IN_RUNTIM(name) __REGION_/**/name/**/_RUNTIM
#endif /* !__ANSI_COMPILER__ */


/* Try to use some form of static-if to force
 * compiler-optimizations even in debug-mode.
 */
#if defined(__DEEMON__) && __has_feature(__static_if__)
#   define COMPILER_STATIC_IF      __static_if
#   define COMPILER_STATIC_ELSE(x) __static_else
#elif (defined(_MSC_VER) || defined(__INTELLISENSE__)) && defined(__cplusplus)
/* Ab-use '__if_exists' to get static-if semantics.
 * Cool, Eh? I discovered this little hack myself! */
extern "C++" { namespace kos { namespace detail {
template<bool> struct static_if {};
template<> struct static_if<true> { bool __is_true__(); };
} } }
#   define COMPILER_STATIC_IF(x)       __if_exists(::kos::detail::static_if<(x)>::__is_true__)
#   define COMPILER_STATIC_ELSE(x) __if_not_exists(::kos::detail::static_if<(x)>::__is_true__)
#else
#   define COMPILER_STATIC_IF      if
#   define COMPILER_STATIC_ELSE(x) else
#endif


/*
 * Region-optimization: Allows for macros to recognize special
 *                      context that allow them to run faster.
 *                      Very useful to implement recursive locking
 *                      of a non-recursive resource, such a no-interrupt,
 *                      critical, or crit-nointr blocks:
 * >> extern bool __is_inside_myregion(void);
 * >> extern void __do_enter_myregion(void);
 * >> extern void __do_leave_myregion(void);
 * >> COMPILER_REGION_DEFINE(myregion);
 * >>
 * >> #define ENTER_MYREGION    COMPILER_REGION_ENTER(myregion,__is_inside_myregion,__do_enter_myregion)
 * >> #define LEAVE_MYREGION    COMPILER_REGION_LEAVE(myregion,__do_leave_myregion)
 * >> #define BREAK_MYREGION    COMPILER_REGION_BREAK(myregion,__do_leave_myregion)
 * >>
 * >> // Compile-time check if the caller is in 'myregion'
 * >> // If this returns true, '__is_inside_myregion' is guarantied to be true as well!
 * >> #define IN_MYREGION_P()   COMPILER_REGION_P(myregion)
 * >> 
 */


/* These global symbols will be overwritten locally,
 * whenever a code is within a compiler-region.
 */
#define COMPILER_REGION_DEFINE(name) \
 enum{__REGION_IN_NORMAL(name) = __REGION_NORMAL_NO\
     ,__REGION_IN_NESTED(name) = __REGION_NESTED_NO\
     ,__REGION_IN_BFIRST(name) = __REGION_BFIRST_NO\
     ,__REGION_IN_RUNTIM(name) = false}

#define COMPILER_REGION_P(name)        (__REGION_IN_NORMAL(name) == __REGION_NORMAL_YES)
#define COMPILER_REGION_NESTED_P(name) (__REGION_IN_NESTED(name) == __REGION_NESTED_YES)
#define COMPILER_REGION_FIRST_P(name)  (__REGION_IN_BFIRST(name) == __REGION_BFIRST_YES)
#define COMPILER_REGION_FIRST(name) \
 (COMPILER_REGION_FIRST_P(name) || \
 (__REGION_IN_NESTED(name) == __REGION_NESTED_ONE && \
 !__REGION_IN_RUNTIM(name)))

#if defined(__DEBUG__) && 1
/* Leave this on for a while, and if everything works, disable it! */
#   define COMPILER_REGION_ASSERT(name,is_inside) \
        assertf(is_inside(),"The compiler said we were supposed to be inside the region!")
#else
#   define COMPILER_REGION_ASSERT(name,is_inside)
#endif

/* Explicitly mark a region as entered.
 * Useful at the start of a function, when the function requires
 * the caller to already be inside the given region.
 * NOTE: In debug mode, that inside-state is asserted. */
#define COMPILER_REGION_MARK(name,is_inside) \
 enum{__REGION_IN_NESTED(name) = __REGION_IN_NORMAL(name)+1\
     ,__REGION_IN_NORMAL(name) = __REGION_NORMAL_YES};\
 assertf(is_inside(),"Region violation: No actually inside a " #name " region.");


#define COMPILER_REGION_ENTER(name,is_inside,do_enter) \
 { enum{__REGION_IN_NESTED(name) = __REGION_IN_NORMAL(name)+1\
       ,__REGION_IN_NORMAL(name) = __REGION_NORMAL_YES};\
 { __attribute_unused bool __REGION_IN_RUNTIM(name);\
   COMPILER_STATIC_IF (__REGION_IN_NESTED(name) == __REGION_NESTED_ONE) {\
    /* Not inside a nested region. - Must deduce region-state at runtime. */\
    __REGION_IN_RUNTIM(name) = is_inside();\
    if __unlikely(!__REGION_IN_RUNTIM(name)) { do_enter(); }\
   } COMPILER_REGION_ASSERT(name,is_inside);
/* void (*try_enter)(bool &did_not_enter); */
#define COMPILER_REGION_TRYENTER(name,is_inside,try_enter) \
 { enum{__REGION_IN_NESTED(name) = __REGION_IN_NORMAL(name)+1\
       ,__REGION_IN_NORMAL(name) = __REGION_NORMAL_YES};\
 { __attribute_unused bool __REGION_IN_RUNTIM(name);\
   COMPILER_STATIC_IF (__REGION_IN_NESTED(name) == __REGION_NESTED_ONE) {\
    /* Not inside a nested region. - Must deduce region-state at runtime. */\
    {try_enter(__REGION_IN_RUNTIM(name));}\
   } COMPILER_REGION_ASSERT(name,is_inside);

/* A special kind of region enter:
 *   This one can be used in-place of the regular enter,
 *   but will be faster if it is already known at compile-time
 *   that this will be the first time the region is entered.
 */
#define COMPILER_REGION_ENTER_FIRST(name,is_inside,do_enter) \
 { enum{__REGION_IN_NESTED(name) = __REGION_IN_NORMAL(name)+1\
       ,__REGION_IN_NORMAL(name) = __REGION_NORMAL_YES\
       ,__REGION_IN_BFIRST(name) = __REGION_BFIRST_YES};\
 { enum{__REGION_IN_RUNTIM(name) = false};\
   __STATIC_ASSERT_M(__REGION_IN_NESTED(name) == __REGION_NESTED_ONE,\
                     "You're not the first to enter the region! "\
                     "Even I, the compiler know that.");\
   assertf(!is_inside(),"You're not the first to enter the region!");\
   {do_enter();};


/* Break outside of a region-block (Useful when wanting to jump outside)
 * WARNING: When the region was entered recursively,
 *          breaking will only leave the inner-most region.
 * If following this call, the region is left normally, additional
 * compiler-optimizations will take place, whereas if the scope
 * that this break was called from is the same as that the region
 * exists within, no code will be generated for leaving the region.
 * If it lies within a different scope, passing by the associated
 * region leave will cause undefined behavior by leaving the region twice!
 */
#define COMPILER_REGION_BREAK(name,do_leave) ;\
   enum{__REGION_IN_NESTED(name) = __REGION_IN_NORMAL(name)\
       ,__REGION_IN_NORMAL(name) = __REGION_NORMAL_NO};\
   COMPILER_STATIC_IF (__REGION_IN_NESTED(name) == __REGION_NESTED_ONE) {\
    if __unlikely(!__REGION_IN_RUNTIM(name)) { do_leave(); }\
   };\
   /* Code here should try not to pass by the associated region leave. */

#define COMPILER_REGION_LEAVE(name,do_leave) ;\
   COMPILER_STATIC_IF (__REGION_IN_NESTED(name) == __REGION_NESTED_ONE) {\
    /* Not a nested region -> Must check runtime variable. */\
    if __unlikely(!__REGION_IN_RUNTIM(name)) { do_leave(); }\
   }\
 } };


/* A special way of entering a region, this function will try to
 * acquire a lock through 'trylock', only calling 'trylock' while
 * inside the region, but leaving the region momentarily to call
 * ktask_yield() before enter it again and re-trying.
 * The behavior is different if the region was already entered when
 * the called invoked this macro, in that the region will not be
 * left momentarily, with the given 'trylock' being called without
 * intermediate calls to ktask_yield(), and the kernel entering
 * panic() mode when being compiled for single-core and the trylock
 * failing after the first attempt.
 * This behavior is required to implement an efficient nointerrupt-and-trylock
 * mechanism, which is required for safe and efficient locking of
 * interrupt-related resources while simultaniously acquiring a nointerrupt lock.
 */
#ifdef KCONFIG_HAVE_SINGLECORE
#define COMPILER_REGION_ENTER_LOCK(name,is_inside,do_enter,do_leave,trylock) \
 { enum{__REGION_IN_NESTED(name) = __REGION_IN_NORMAL(name)+1\
       ,__REGION_IN_NORMAL(name) = __REGION_NORMAL_YES};\
 { __attribute_unused bool __REGION_IN_RUNTIM(name);\
   COMPILER_STATIC_IF (__REGION_IN_NESTED(name) != __REGION_NESTED_ONE) {\
    assertef(trylock,"Failed immediate acquisition of lock '" #trylock "'");\
   } COMPILER_STATIC_ELSE (__REGION_IN_NESTED(name) != __REGION_NESTED_ONE) {\
    __REGION_IN_RUNTIM(name) = is_inside();\
    if __unlikely(!__REGION_IN_RUNTIM(name)) {\
     {do_enter();}\
     while (!(trylock)) {\
      {do_leave();}\
      assertef(ktask_tryyield() == KE_OK,\
               "IRQ-related lock '" #trylock "' cannot be acquired, "\
               "but there is no-one else to switch to!");\
      {do_enter();}\
     }\
    } else {\
     assertef(trylock,"Failed immediate acquisition of lock '" #trylock "'");\
    }
   } COMPILER_REGION_ASSERT(name,is_inside);
#define COMPILER_REGION_ENTER_LOCK_VOLATILE(name,is_inside,do_enter,do_leave,ob,readvolatile,trylock,unlock) \
 { enum{__REGION_IN_NESTED(name) = __REGION_IN_NORMAL(name)+1\
       ,__REGION_IN_NORMAL(name) = __REGION_NORMAL_YES};\
 { __attribute_unused bool __REGION_IN_RUNTIM(name);\
   COMPILER_STATIC_IF (__REGION_IN_NESTED(name) != __REGION_NESTED_ONE) {\
    assertef(trylock,"Failed immediate acquisition of lock '" #trylock "'");\
   } COMPILER_STATIC_ELSE (__REGION_IN_NESTED(name) != __REGION_NESTED_ONE) {\
    __REGION_IN_RUNTIM(name) = is_inside();\
    (ob) = (readvolatile);\
    if (!__REGION_IN_RUNTIM(name)) for (;;) {\
     {do_enter();}\
     while (!(trylock)) {\
      volatile int __spinner = 10000;\
      {do_leave();}\
      if (ktask_tryyield() != KE_OK) while (__spinner--);\
      {do_enter();}\
     }\
     if __likely(ob == readvolatile) break;\
     /* Volatile memory has changed while we were interrupting. */\
     (unlock); {do_leave();} (ob) = (readvolatile);\
    } else {\
     assertef(trylock,"Failed immediate acquisition of lock '" #trylock "'");\
    }\
   } COMPILER_REGION_ASSERT(name,is_inside);
#else
#define COMPILER_REGION_ENTER_LOCK(name,is_inside,do_enter,do_leave,trylock) \
 { enum{__REGION_IN_NESTED(name) = __REGION_IN_NORMAL(name)+1\
       ,__REGION_IN_NORMAL(name) = __REGION_NORMAL_YES};\
 { __attribute_unused bool __REGION_IN_RUNTIM(name);\
   COMPILER_STATIC_IF (__REGION_IN_NESTED(name) != __REGION_NESTED_ONE) {\
    while (!(trylock));\
   } COMPILER_STATIC_ELSE (__REGION_IN_NESTED(name) != __REGION_NESTED_ONE) {\
    __REGION_IN_RUNTIM(name) = is_inside();\
    if __unlikely(!__REGION_IN_RUNTIM(name)) {\
     {do_enter();}\
     while (!(trylock)) {\
      volatile int __spinner = 10000;\
      {do_leave();}\
      if (ktask_tryyield() != KE_OK) while (__spinner--);\
      {do_enter();}\
     }\
    } else {\
     while (!(trylock));\
    }\
   } COMPILER_REGION_ASSERT(name,is_inside);
#define COMPILER_REGION_ENTER_LOCK_VOLATILE(name,is_inside,do_enter,do_leave,ob,readvolatile,trylock,unlock) \
 { enum{__REGION_IN_NESTED(name) = __REGION_IN_NORMAL(name)+1\
       ,__REGION_IN_NORMAL(name) = __REGION_NORMAL_YES};\
 { __attribute_unused bool __REGION_IN_RUNTIM(name);\
   COMPILER_STATIC_IF (__REGION_IN_NESTED(name) != __REGION_NESTED_ONE) {\
    while (!(trylock));\
   } COMPILER_STATIC_ELSE (__REGION_IN_NESTED(name) != __REGION_NESTED_ONE) {\
    __REGION_IN_RUNTIM(name) = is_inside();\
    (ob) = (readvolatile);\
    if (!__REGION_IN_RUNTIM(name)) for (;;) {\
     {do_enter();}\
     while (!(trylock)) {\
      volatile int __spinner = 10000;\
      {do_leave();}\
      if (ktask_tryyield() != KE_OK) while (__spinner--);\
      {do_enter();}\
     }\
     if __likely(ob == readvolatile) break;\
     /* Volatile memory has changed while we were interrupting. */\
     (unlock); {do_leave();} (ob) = (readvolatile);\
    } else {\
     while (!(trylock));\
    }\
   } COMPILER_REGION_ASSERT(name,is_inside);
#endif

#if 0
}}}}}}}}}}}} /* Fix side-bar highlighting in visual studio */
#endif






/* Runtime-only version. */
#define RUNTIME_REGION_DEFINE(name)     /* nothing */
#define RUNTIME_REGION_P(name)          0
#define RUNTIME_REGION_NESTED_P(name)   0
#define RUNTIME_REGION_FIRST_P(name)    0
#define RUNTIME_REGION_FIRST(name)    (!__REGION_IN_RUNTIM(name))
#define RUNTIME_REGION_MARK(name,is_inside) \
 assertf(is_inside(),"Region violation: No actually inside a " #name " region.");
#define RUNTIME_REGION_ENTER(name,is_inside,do_enter) \
 { __attribute_unused bool __REGION_IN_RUNTIM(name) = is_inside();\
   if __unlikely(!__REGION_IN_RUNTIM(name)) { do_enter(); };
/* void (*try_enter)(bool &did_not_enter); */
#define RUNTIME_REGION_TRYENTER(name,is_inside,try_enter) \
 { __attribute_unused bool __REGION_IN_RUNTIM(name); \
 { try_enter(__REGION_IN_RUNTIM(name)); };
#define RUNTIME_REGION_BREAK(name,do_leave) \
   if __unlikely(!__REGION_IN_RUNTIM(name)) { do_leave(); };\
   /* Code here should try not to pass by the associated region leave. */
#define RUNTIME_REGION_LEAVE(name,do_leave)\
   if __unlikely(!__REGION_IN_RUNTIM(name)) { do_leave(); };\
 }
#define RUNTIME_REGION_ENTER_FIRST(name,is_inside,do_enter) \
 { enum{__REGION_IN_RUNTIM(name) = true};\
   assertf(!is_inside(),"You're not the first to enter the region!");\
   {do_enter();};

#ifdef KCONFIG_HAVE_SINGLECORE
#define RUNTIME_REGION_ENTER_LOCK(name,is_inside,do_enter,do_leave,trylock) \
 { __attribute_unused bool __REGION_IN_RUNTIM(name) = is_inside();\
   if (!__REGION_IN_RUNTIM(name)) {\
    do_enter();\
    while (!(trylock)) {\
     do_leave();\
     assertef(ktask_tryyield() == KE_OK,\
              "IRQ-related lock '" #trylock "' cannot be acquired, "\
              "but there is no-one else to switch to!");\
     do_enter();\
    }\
   } else {\
    assertef(trylock,"Failed immediate acquisition of lock '" #trylock "'");\
   };
#define RUNTIME_REGION_ENTER_LOCK_VOLATILE(name,is_inside,do_enter,do_leave,ob,readvolatile,trylock,unlock) \
 { __attribute_unused bool __REGION_IN_RUNTIM(name);\
   __REGION_IN_RUNTIM(name) = is_inside();\
   (ob) = (readvolatile);\
   if (!__REGION_IN_RUNTIM(name)) for (;;) {\
    {do_enter();}\
    while (!(trylock)) {\
     {do_leave();}\
     assertef(ktask_tryyield() == KE_OK,\
              "IRQ-related lock '" #trylock "' cannot be acquired, "\
              "but there is no-one else to switch to!");\
     {do_enter();}\
    }\
    if __likely(ob == readvolatile) break;\
    /* Volatile memory has changed while we were interrupting. */\
    (unlock); {do_leave();} (ob) = (readvolatile);\
   } else {\
    assertef(trylock,"Failed immediate acquisition of lock '" #trylock "'");\
   };
#else
#define RUNTIME_REGION_ENTER_LOCK(name,is_inside,do_enter,do_leave,trylock) \
 { __attribute_unused bool __REGION_IN_RUNTIM(name) = is_inside();\
   if (__REGION_IN_RUNTIM(name)) {\
    do_leave();\
    while (!(trylock)) {\
     volatile int __spinner = 10000;\
     karch_irq_enable();\
     if (ktask_tryyield() != KE_OK) while (__spinner--);\
     do_leave();\
    }\
   } else {\
    while (!(trylock));\
   };
#define RUNTIME_REGION_ENTER_LOCK_VOLATILE(name,is_inside,do_enter,do_leave,ob,readvolatile,trylock,unlock) \
 { __attribute_unused bool __REGION_IN_RUNTIM(name);\
   __REGION_IN_RUNTIM(name) = is_inside();\
   (ob) = (readvolatile);\
   if (!__REGION_IN_RUNTIM(name)) for (;;) {\
    {do_enter();}\
    while (!(trylock)) {\
     volatile int __spinner = 10000;\
     {do_leave();}\
     if (ktask_tryyield() != KE_OK) while (__spinner--);\
     {do_enter();}\
    }\
    if __likely(ob == readvolatile) break;\
    /* Volatile memory has changed while we were interrupting. */\
    (unlock); {do_leave();} (ob) = (readvolatile);\
   } else {\
    while (!(trylock));\
   };
#endif
#if 0
}}}}}} /* Fix side-bar highlighting in visual studio */
#endif





/* Even though regions only use standard-C features,
   some compilers might not understand the constant
   symbol re-definition and (most importantly) scoping.
   Compilers that don't understand it, should be added to this list. */
#if 0
#   define REGION_DEFINE              RUNTIME_REGION_DEFINE
#   define REGION_P                   RUNTIME_REGION_P
#   define REGION_NESTED_P            RUNTIME_REGION_NESTED_P
#   define REGION_FIRST_P             RUNTIME_REGION_FIRST_P
#   define REGION_FIRST               RUNTIME_REGION_FIRST
#   define REGION_MARK                RUNTIME_REGION_MARK
#   define REGION_ENTER               RUNTIME_REGION_ENTER
#   define REGION_TRYENTER            RUNTIME_REGION_TRYENTER
#   define REGION_BREAK               RUNTIME_REGION_BREAK
#   define REGION_LEAVE               RUNTIME_REGION_LEAVE
#   define REGION_ENTER_FIRST         RUNTIME_REGION_ENTER_FIRST
#   define REGION_ENTER_LOCK          RUNTIME_REGION_ENTER_LOCK
#   define REGION_ENTER_LOCK_VOLATILE RUNTIME_REGION_ENTER_LOCK_VOLATILE
#else
#   define REGION_DEFINE              COMPILER_REGION_DEFINE
#   define REGION_P                   COMPILER_REGION_P
#   define REGION_NESTED_P            COMPILER_REGION_NESTED_P
#   define REGION_FIRST_P             COMPILER_REGION_FIRST_P
#   define REGION_FIRST               COMPILER_REGION_FIRST
#   define REGION_MARK                COMPILER_REGION_MARK
#   define REGION_ENTER               COMPILER_REGION_ENTER
#   define REGION_TRYENTER            COMPILER_REGION_TRYENTER
#   define REGION_BREAK               COMPILER_REGION_BREAK
#   define REGION_LEAVE               COMPILER_REGION_LEAVE
#   define REGION_ENTER_FIRST         COMPILER_REGION_ENTER_FIRST
#   define REGION_ENTER_LOCK          COMPILER_REGION_ENTER_LOCK
#   define REGION_ENTER_LOCK_VOLATILE COMPILER_REGION_ENTER_LOCK_VOLATILE
#endif


__DECL_END


#endif /* !__KOS_KERNEL_REGION_H__ */

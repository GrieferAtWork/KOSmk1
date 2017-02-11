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
#ifndef __STDATOMIC_H__
#ifndef _STDATOMIC_H
#ifndef _INC_STDATOMIC
#define __STDATOMIC_H__ 1
#define _STDATOMIC_H 1
#define _INC_STDATOMIC 1

#include <kos/compiler.h>
#include <kos/types.h>

__DECL_BEGIN

#ifndef __memory_order_defined
#define __memory_order_defined 1
typedef enum {
    memory_order_relaxed = __ATOMIC_RELAXED,
    memory_order_consume = __ATOMIC_CONSUME,
    memory_order_acquire = __ATOMIC_ACQUIRE,
    memory_order_release = __ATOMIC_RELEASE,
    memory_order_acq_rel = __ATOMIC_ACQ_REL,
    memory_order_seq_cst = __ATOMIC_SEQ_CST,
} memory_order;
#endif

#define __ATOMIC_T(T) _Atomic T

typedef __ATOMIC_T(__bool_t)         atomic_bool;
typedef __ATOMIC_T(__char_t)         atomic_char;
typedef __ATOMIC_T(__schar_t)        atomic_schar;
typedef __ATOMIC_T(__uchar_t)        atomic_uchar;
typedef __ATOMIC_T(__short_t)        atomic_short;
typedef __ATOMIC_T(__ushort_t)       atomic_ushort;
typedef __ATOMIC_T(__int_t)          atomic_int;
typedef __ATOMIC_T(__uint_t)         atomic_uint;
typedef __ATOMIC_T(__long_t)         atomic_long;
typedef __ATOMIC_T(__ulong_t)        atomic_ulong;
typedef __ATOMIC_T(__llong_t)        atomic_llong;
typedef __ATOMIC_T(__ullong_t)       atomic_ullong;
typedef __ATOMIC_T(__char16_t)       atomic_char16_t;
typedef __ATOMIC_T(__char32_t)       atomic_char32_t;
typedef __ATOMIC_T(__wchar_t)        atomic_wchar_t;
typedef __ATOMIC_T(__int_least8_t)   atomic_int_least8_t;
typedef __ATOMIC_T(__uint_least8_t)  atomic_uint_least8_t;
typedef __ATOMIC_T(__int_least16_t)  atomic_int_least16_t;
typedef __ATOMIC_T(__uint_least16_t) atomic_uint_least16_t;
typedef __ATOMIC_T(__int_least32_t)  atomic_int_least32_t;
typedef __ATOMIC_T(__uint_least32_t) atomic_uint_least32_t;
typedef __ATOMIC_T(__int_least64_t)  atomic_int_least64_t;
typedef __ATOMIC_T(__uint_least64_t) atomic_uint_least64_t;
typedef __ATOMIC_T(__int_fast8_t)    atomic_int_fast8_t;
typedef __ATOMIC_T(__uint_fast8_t)   atomic_uint_fast8_t;
typedef __ATOMIC_T(__int_fast16_t)   atomic_int_fast16_t;
typedef __ATOMIC_T(__uint_fast16_t)  atomic_uint_fast16_t;
typedef __ATOMIC_T(__int_fast32_t)   atomic_int_fast32_t;
typedef __ATOMIC_T(__uint_fast32_t)  atomic_uint_fast32_t;
typedef __ATOMIC_T(__int_fast64_t)   atomic_int_fast64_t;
typedef __ATOMIC_T(__uint_fast64_t)  atomic_uint_fast64_t;
typedef __ATOMIC_T(__intptr_t)       atomic_intptr_t;
typedef __ATOMIC_T(__uintptr_t)      atomic_uintptr_t;
typedef __ATOMIC_T(__size_t)         atomic_size_t;
typedef __ATOMIC_T(__ptrdiff_t)      atomic_ptrdiff_t;
typedef __ATOMIC_T(__intmax_t)       atomic_intmax_t;
typedef __ATOMIC_T(__uintmax_t)      atomic_uintmax_t;
#undef __ATOMIC_T


#define ATOMIC_VAR_INIT(x)	(x)
#define atomic_init(p,v) (void)(*(p)=(v))
#define kill_dependency(y)	__xblock({ __typeof__(y) __kd_y = (y); __xreturn __kd_y; })

#define atomic_thread_fence	     __atomic_thread_fence
#define atomic_signal_fence	     __atomic_signal_fence
#define atomic_is_lock_free(obj) __atomic_is_lock_free(sizeof(*(obj)),(obj))

#define ATOMIC_BOOL_LOCK_FREE		   __GCC_ATOMIC_BOOL_LOCK_FREE
#define ATOMIC_CHAR_LOCK_FREE		   __GCC_ATOMIC_CHAR_LOCK_FREE
#define ATOMIC_CHAR16_T_LOCK_FREE	__GCC_ATOMIC_CHAR16_T_LOCK_FREE
#define ATOMIC_CHAR32_T_LOCK_FREE	__GCC_ATOMIC_CHAR32_T_LOCK_FREE
#define ATOMIC_WCHAR_T_LOCK_FREE	 __GCC_ATOMIC_WCHAR_T_LOCK_FREE
#define ATOMIC_SHORT_LOCK_FREE		  __GCC_ATOMIC_SHORT_LOCK_FREE
#define ATOMIC_INT_LOCK_FREE		    __GCC_ATOMIC_INT_LOCK_FREE
#define ATOMIC_LONG_LOCK_FREE		   __GCC_ATOMIC_LONG_LOCK_FREE
#define ATOMIC_LLONG_LOCK_FREE		  __GCC_ATOMIC_LLONG_LOCK_FREE
#define ATOMIC_POINTER_LOCK_FREE	 __GCC_ATOMIC_POINTER_LOCK_FREE

#define atomic_store_explicit(obj,desired,order)                                __xblock({ __typeof__(*(obj)) const __ase_d = (desired); __atomic_store(obj,&__ase_d,order); })
#define atomic_load_explicit(obj,order)	                                        __xblock({	__typeof__(*(obj)) _ale_res;	__atomic_load(obj,&_ale_res,order);	__xreturn _ale_res; })
#define atomic_exchange_explicit(obj,desired,order)	                            __xblock({ __typeof__(*(obj)) __axe_d = (desired),__axe_res; __atomic_exchange(obj,&__axe_d,&__axe_res,(order)); __xreturn __axe_res; })
#define atomic_compare_exchange_strong_explicit(obj,expected,desired,succ,fail) __xblock({	__typeof__(*(obj)) __acese_d = (desired);	__xreturn __atomic_compare_exchange(obj,expected,&__acese_d,0,succ,fail);	})
#define atomic_compare_exchange_weak_explicit(obj,expected,desired,succ,fail)   __xblock({	__typeof__(*(obj)) __acese_d = (desired);	__xreturn __atomic_compare_exchange(obj,expected,&__acese_d,1,succ,fail);	})

#define atomic_store(obj,desired)	                           atomic_store_explicit(obj,desired,__ATOMIC_SEQ_CST)
#define atomic_load(obj)                                     atomic_load_explicit(obj,__ATOMIC_SEQ_CST)
#define atomic_exchange(obj,desired)                         atomic_exchange_explicit(obj,desired,__ATOMIC_SEQ_CST)
#define atomic_compare_exchange_strong(obj,expected,desired) atomic_compare_exchange_strong_explicit(obj,expected,desired,__ATOMIC_SEQ_CST,__ATOMIC_SEQ_CST)
#define atomic_compare_exchange_weak(obj,expected,desired)			atomic_compare_exchange_weak_explicit(obj,expected,desired,__ATOMIC_SEQ_CST,__ATOMIC_SEQ_CST)

#define atomic_fetch_add_explicit     __atomic_fetch_add
#define atomic_fetch_sub_explicit     __atomic_fetch_sub
#define atomic_fetch_or_explicit      __atomic_fetch_or
#define atomic_fetch_xor_explicit     __atomic_fetch_xor
#define atomic_fetch_and_explicit     __atomic_fetch_and
#define atomic_fetch_add(obj,desired) atomic_fetch_add_explicit(obj,desired,__ATOMIC_SEQ_CST)
#define atomic_fetch_sub(obj,desired) atomic_fetch_sub_explicit(obj,desired,__ATOMIC_SEQ_CST)
#define atomic_fetch_or(obj,desired)  atomic_fetch_or_explicit (obj,desired,__ATOMIC_SEQ_CST)
#define atomic_fetch_xor(obj,desired) atomic_fetch_xor_explicit(obj,desired,__ATOMIC_SEQ_CST)
#define atomic_fetch_and(obj,desired) atomic_fetch_and_explicit(obj,desired,__ATOMIC_SEQ_CST)


typedef struct { unsigned char __v; } atomic_flag;
#define ATOMIC_FLAG_INIT	{0}
#define atomic_flag_test_and_set_explicit	__atomic_test_and_set
#define atomic_flag_clear_explicit        __atomic_clear
#define atomic_flag_test_and_set(obj) atomic_flag_test_and_set_explicit(obj,__ATOMIC_SEQ_CST)
#define atomic_flag_clear(obj)	       atomic_flag_clear_explicit(obj,__ATOMIC_SEQ_CST)

__DECL_END

#endif /* !_INC_STDATOMIC */
#endif /* !_STDATOMIC_H */
#endif /* !__STDATOMIC_H__ */

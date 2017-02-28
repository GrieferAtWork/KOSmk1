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

/*
 * Always-include, wrapper header to re-configure Visual Studio's
 * Intellisense into something as much conforming to what the GCC
 * compiler does, as possible.
 * >> This header is never included by any production code,
 *    only ever included through the nmake forced include option,
 *    and functions defined here are never implemented.
 *   (So there's no need to go looking...)
 */


// Get rid of Visual Studio's defines
#undef WIN32
#undef WIN64
#undef _WIN32
#undef _WIN64
#undef _M_IX86
#undef _M_IX86_FP
#undef _MSC_VER
#undef _MSC_BUILD
#undef _MSC_EXTENSIONS
#undef _MT


#define __KOS__           1
#define __STDC__          1
#define __i386__          1
#define __i386            1
#define i386              1
#define __ELF__           1
#define _FILE_OFFSET_BITS 64
#define ____INTELLISENE_ENDIAN  1234

#define __DATE_DAY__   1
#define __DATE_WDAY__  0
#define __DATE_YDAY__  1
#define __DATE_MONTH__ 1
#define __DATE_YEAR__  2017

#define __TIME_HOUR__  13
#define __TIME_MIN__   37
#define __TIME_SEC__   42

#define __GNUC__       6
#define __GNUC_MINOR__ 2
#define __GNUC_PATCH__ 0
#ifdef __cplusplus
#define __GXX_EXPERIMENTAL_CXX0X__ 1
#endif

#define __CHAR_BIT__   8

#define __INT8_TYPE__           signed __int8
#define __INT16_TYPE__          signed __int16
#define __INT32_TYPE__          signed __int32
#define __INT64_TYPE__          signed __int64
#define __UINT8_TYPE__        unsigned __int8
#define __UINT16_TYPE__       unsigned __int16
#define __UINT32_TYPE__       unsigned __int32
#define __UINT64_TYPE__       unsigned __int64
#define __INT_LEAST8_TYPE__     signed __int8
#define __INT_LEAST16_TYPE__    signed __int16
#define __INT_LEAST32_TYPE__    signed __int32
#define __INT_LEAST64_TYPE__    signed __int64
#define __UINT_LEAST8_TYPE__  unsigned __int8
#define __UINT_LEAST16_TYPE__ unsigned __int16
#define __UINT_LEAST32_TYPE__ unsigned __int32
#define __UINT_LEAST64_TYPE__ unsigned __int64
#define __INT_FAST8_TYPE__      signed __int8
#define __INT_FAST16_TYPE__     signed __int16
#define __INT_FAST32_TYPE__     signed __int32
#define __INT_FAST64_TYPE__     signed __int64
#define __UINT_FAST8_TYPE__   unsigned __int8
#define __UINT_FAST16_TYPE__  unsigned __int16
#define __UINT_FAST32_TYPE__  unsigned __int32
#define __UINT_FAST64_TYPE__  unsigned __int64
#define __INTMAX_TYPE__         signed __int64
#define __UINTMAX_TYPE__      unsigned __int64

#if defined(__i386__)
# define __INTPTR_TYPE__    signed __int32
# define __UINTPTR_TYPE__   unsigned __int32
# define __SIZEOF_POINTER__ 4
#else
# define __INTPTR_TYPE__    signed __int64
# define __UINTPTR_TYPE__   unsigned __int64
# define __SIZEOF_POINTER__ 8
#endif
# define __SIZE_TYPE__     __UINTPTR_TYPE__
# define __PTRDIFF_TYPE__  __INTPTR_TYPE__

#define __SIZEOF_SHORT__     2
#define __SIZEOF_INT__       4
#define __SIZEOF_LONG__      __SIZEOF_POINTER__
#define __SIZEOF_LONG_LONG__ 8
#define __SIZEOF_SIZE_T__    __SIZEOF_POINTER__

#define __INT8_C(c)    c##i8
#define __INT16_C(c)   c##i16
#define __INT32_C(c)   c##i32
#define __INT64_C(c)   c##i64
#define __UINT8_C(c)   c##ui8
#define __UINT16_C(c)  c##ui16
#define __UINT32_C(c)  c##ui32
#define __UINT64_C(c)  c##ui64
#define __INTMAX_C(c)  c##i64
#define __UINTMAX_C(c) c##ui64


#define asm(...)           /* nothing */
#define __asm(...)         /* nothing */
#define __asm__(...)       /* nothing */
#define __attribute        __attribute__
#define __extension__      /* nothing */
#define __inline__         inline
#define __const            const
//#define __const__        const
#define __volatile         volatile
#define __volatile__       volatile
#define __signed           signed
#define __signed__         signed
#define __unsigned         unsigned
#define __unsigned__       unsigned
#define __restrict__       __restrict
#define __attribute__(...) ____INTELLISENE_ATTR __VA_ARGS__
#define _Atomic            __declspec(thread)
typedef bool _Bool;

#define __CHAR16_TYPE__    unsigned __int16
#define __CHAR32_TYPE__    unsigned __int32
#define __WCHAR_TYPE__     wchar_t


// Visual-studio specific VA_NARGS implementation
// NOTE: Capable of detecting 0 arguments!
// HINT: This implementation works for intellisense, vc and vc++
#define ____INTELLISENE_PP_FORCE_EXPAND(...) __VA_ARGS__
#define ____INTELLISENE_PP_PRIVATE_VA_NARGS_I(x,_1,_2,_3,_4,_5,_6,_7,N,...) N
#define ____INTELLISENE_PP_PRIVATE_VA_NARGS_X(...) (~,__VA_ARGS__,7,6,5,4,3,2,1,0)
#define ____INTELLISENE_PP_VA_NARGS(...) ____INTELLISENE_PP_FORCE_EXPAND(____INTELLISENE_PP_PRIVATE_VA_NARGS_I ____INTELLISENE_PP_PRIVATE_VA_NARGS_X(__VA_ARGS__))

#define ____INTELLISENE_ATTR_1(a)      ____INTELLISENE_attribute_##a
#define ____INTELLISENE_ATTR_2(a,b)    ____INTELLISENE_attribute_##a ____INTELLISENE_attribute_##b
#define ____INTELLISENE_ATTR_3(a,b,c)  ____INTELLISENE_attribute_##a ____INTELLISENE_attribute_##b ____INTELLISENE_attribute_##c
#define ____INTELLISENE_ATTR_N2(n,...) ____INTELLISENE_ATTR_##n __VA_ARGS__
#define ____INTELLISENE_ATTR_N(n,...)  ____INTELLISENE_ATTR_N2(n,(__VA_ARGS__))
#define ____INTELLISENE_ATTR(...) ____INTELLISENE_ATTR_N(____INTELLISENE_PP_VA_NARGS(__VA_ARGS__),__VA_ARGS__)


#define __thread                        __declspec(thread)
#define ____INTELLISENE_ATTR_FUNC       __declspec(noreturn)
#define ____INTELLISENE_ATTR_VAR        __declspec(allocate("~"))
#define ____INTELLISENE_ATTR_FUNCORVAR  /* ??? */
#define ____INTELLISENE_ATTR_TYPE       __declspec(align(4))
// NOTE: These attribute are only meant to signal incorrect use, not to actually work corrently...
#define ____INTELLISENE_attribute_noreturn                   __declspec(noreturn)
#define ____INTELLISENE_attribute___noreturn                 __declspec(noreturn)
#define ____INTELLISENE_attribute___noreturn__               __declspec(noreturn)
#define ____INTELLISENE_attribute_noinline                   __declspec(noinline)
#define ____INTELLISENE_attribute___noinline                 __declspec(noinline)
#define ____INTELLISENE_attribute___noinline__               __declspec(noinline)
#define ____INTELLISENE_attribute_noclone                    __declspec(noinline)
#define ____INTELLISENE_attribute___noclone                  __declspec(noinline)
#define ____INTELLISENE_attribute___noclone__                __declspec(noinline)
#define ____INTELLISENE_attribute_const                      __declspec(noalias)
#define ____INTELLISENE_attribute___const                    __declspec(noalias)
#define ____INTELLISENE_attribute___const__                  __declspec(noalias)
#define ____INTELLISENE_attribute_pure                       __declspec(noalias)
#define ____INTELLISENE_attribute___pure                     __declspec(noalias)
#define ____INTELLISENE_attribute___pure__                   __declspec(noalias)
#define ____INTELLISENE_attribute_nothrow                    __declspec(nothrow)
#define ____INTELLISENE_attribute___nothrow                  __declspec(nothrow)
#define ____INTELLISENE_attribute___nothrow__                __declspec(nothrow)
#define ____INTELLISENE_attribute_deprecated(reason)         __declspec(deprecated(reason))
#define ____INTELLISENE_attribute___deprecated(reason)       __declspec(deprecated(reason))
#define ____INTELLISENE_attribute___deprecated__(reason)     __declspec(deprecated(reason))
#define ____INTELLISENE_attribute_malloc                     __declspec(restrict)
#define ____INTELLISENE_attribute___malloc                   __declspec(restrict)
#define ____INTELLISENE_attribute___malloc__                 __declspec(restrict)
#define ____INTELLISENE_attribute_assume_aligned(...)        __declspec(restrict)
#define ____INTELLISENE_attribute___assume_aligned(...)      __declspec(restrict)
#define ____INTELLISENE_attribute___assume_aligned__(...)    __declspec(restrict)
#define ____INTELLISENE_attribute_alloc_align(...)           __declspec(restrict)
#define ____INTELLISENE_attribute___alloc_align(...)         __declspec(restrict)
#define ____INTELLISENE_attribute___alloc_align__(...)       __declspec(restrict)
#define ____INTELLISENE_attribute_alloc_size(...)            __declspec(restrict)
#define ____INTELLISENE_attribute___alloc_size(...)          __declspec(restrict)
#define ____INTELLISENE_attribute___alloc_size__(...)        __declspec(restrict)
#define ____INTELLISENE_attribute_cold                       ____INTELLISENE_ATTR_FUNC
#define ____INTELLISENE_attribute___cold                     ____INTELLISENE_ATTR_FUNC
#define ____INTELLISENE_attribute___cold__                   ____INTELLISENE_ATTR_FUNC
#define ____INTELLISENE_attribute_always_inline              ____INTELLISENE_ATTR_FUNC
#define ____INTELLISENE_attribute___always_inline            ____INTELLISENE_ATTR_FUNC
#define ____INTELLISENE_attribute___always_inline__          ____INTELLISENE_ATTR_FUNC
#define ____INTELLISENE_attribute_sentinel                   ____INTELLISENE_ATTR_FUNC
#define ____INTELLISENE_attribute___sentinel                 ____INTELLISENE_ATTR_FUNC
#define ____INTELLISENE_attribute___sentinel__               ____INTELLISENE_ATTR_FUNC
#define ____INTELLISENE_attribute_visibility(v)              ____INTELLISENE_ATTR_FUNCORVAR
#define ____INTELLISENE_attribute___visibility(v)            ____INTELLISENE_ATTR_FUNCORVAR
#define ____INTELLISENE_attribute___visibility__(v)          ____INTELLISENE_ATTR_FUNCORVAR
#define ____INTELLISENE_attribute_nonnull(...)               ____INTELLISENE_ATTR_FUNC
#define ____INTELLISENE_attribute___nonnull(...)             ____INTELLISENE_ATTR_FUNC
#define ____INTELLISENE_attribute___nonnull__(...)           ____INTELLISENE_ATTR_FUNC
#define ____INTELLISENE_attribute_used                       /* nothing */
#define ____INTELLISENE_attribute___used                     /* nothing */
#define ____INTELLISENE_attribute___used__                   /* nothing */
#define ____INTELLISENE_attribute_unused                     /* nothing */
#define ____INTELLISENE_attribute___unused                   /* nothing */
#define ____INTELLISENE_attribute___unused__                 /* nothing */
#define ____INTELLISENE_attribute_aligned(x)                 ____INTELLISENE_ATTR_VAR/* __declspec(align(x)) */
#define ____INTELLISENE_attribute___aligned(x)               ____INTELLISENE_ATTR_VAR/* __declspec(align(x)) */
#define ____INTELLISENE_attribute___aligned__(x)             ____INTELLISENE_ATTR_VAR/* __declspec(align(x)) */
#define ____INTELLISENE_attribute_packed                     /* nothing */
#define ____INTELLISENE_attribute___packed                   /* nothing */
#define ____INTELLISENE_attribute___packed__                 /* nothing */
#define ____INTELLISENE_ATTR_FORMAT_printf                   ____INTELLISENE_ATTR_FUNC
#define ____INTELLISENE_ATTR_FORMAT_scanf                    ____INTELLISENE_ATTR_FUNC
#define ____INTELLISENE_ATTR_FORMAT_strftime                 ____INTELLISENE_ATTR_FUNC
#define ____INTELLISENE_ATTR_FORMAT_strfmon                  ____INTELLISENE_ATTR_FUNC
#define ____INTELLISENE_ATTR_FORMAT___printf                 ____INTELLISENE_ATTR_FUNC
#define ____INTELLISENE_ATTR_FORMAT___scanf                  ____INTELLISENE_ATTR_FUNC
#define ____INTELLISENE_ATTR_FORMAT___strftime               ____INTELLISENE_ATTR_FUNC
#define ____INTELLISENE_ATTR_FORMAT___strfmon                ____INTELLISENE_ATTR_FUNC
#define ____INTELLISENE_ATTR_FORMAT___printf__               ____INTELLISENE_ATTR_FUNC
#define ____INTELLISENE_ATTR_FORMAT___scanf__                ____INTELLISENE_ATTR_FUNC
#define ____INTELLISENE_ATTR_FORMAT___strftime__             ____INTELLISENE_ATTR_FUNC
#define ____INTELLISENE_ATTR_FORMAT___strfmon__              ____INTELLISENE_ATTR_FUNC
#define ____INTELLISENE_attribute_format(name,fmt_idx,varargs_idx)     ____INTELLISENE_ATTR_FORMAT_##name
#define ____INTELLISENE_attribute___format(name,fmt_idx,varargs_idx)   ____INTELLISENE_ATTR_FORMAT_##name
#define ____INTELLISENE_attribute___format__(name,fmt_idx,varargs_idx) ____INTELLISENE_ATTR_FORMAT_##name
#define ____INTELLISENE_attribute_returns_nonnull            ____INTELLISENE_ATTR_FUNC
#define ____INTELLISENE_attribute___returns_nonnull          ____INTELLISENE_ATTR_FUNC
#define ____INTELLISENE_attribute___returns_nonnull__        ____INTELLISENE_ATTR_FUNC
#define ____INTELLISENE_attribute_warn_unused_result         ____INTELLISENE_ATTR_FUNC
#define ____INTELLISENE_attribute___warn_unused_result       ____INTELLISENE_ATTR_FUNC
#define ____INTELLISENE_attribute___warn_unused_result__     ____INTELLISENE_ATTR_FUNC
#define ____INTELLISENE_attribute_cdecl                      __cdecl
#define ____INTELLISENE_attribute___cdecl                    __cdecl
#define ____INTELLISENE_attribute___cdecl__                  __cdecl
#define ____INTELLISENE_attribute_fastcall                   __fastcall
#define ____INTELLISENE_attribute___fastcall                 __fastcall
#define ____INTELLISENE_attribute___fastcall__               __fastcall
#define ____INTELLISENE_attribute_stdcall                    __stdcall
#define ____INTELLISENE_attribute___stdcall                  __stdcall
#define ____INTELLISENE_attribute___stdcall__                __stdcall
#define ____INTELLISENE_attribute_section(name)              ____INTELLISENE_ATTR_FUNCORVAR
#define ____INTELLISENE_attribute___section(name)            ____INTELLISENE_ATTR_FUNCORVAR
#define ____INTELLISENE_attribute___section__(name)          ____INTELLISENE_ATTR_FUNCORVAR
#define ____INTELLISENE_attribute_constructor                ____INTELLISENE_ATTR_FUNC
#define ____INTELLISENE_attribute___constructor              ____INTELLISENE_ATTR_FUNC
#define ____INTELLISENE_attribute___constructor__            ____INTELLISENE_ATTR_FUNC
#define ____INTELLISENE_attribute_destructor                 ____INTELLISENE_ATTR_FUNC
#define ____INTELLISENE_attribute___destructor               ____INTELLISENE_ATTR_FUNC
#define ____INTELLISENE_attribute___destructor__             ____INTELLISENE_ATTR_FUNC
#define ____INTELLISENE_attribute_weak                       ____INTELLISENE_ATTR_FUNCORVAR
#define ____INTELLISENE_attribute___weak                     ____INTELLISENE_ATTR_FUNCORVAR
#define ____INTELLISENE_attribute___weak__                   ____INTELLISENE_ATTR_FUNCORVAR
#define ____INTELLISENE_attribute_alias(target)              ____INTELLISENE_ATTR_FUNCORVAR
#define ____INTELLISENE_attribute___alias(target)            ____INTELLISENE_ATTR_FUNCORVAR
#define ____INTELLISENE_attribute___alias__(target)          ____INTELLISENE_ATTR_FUNCORVAR
#define ____INTELLISENE_attribute_regparm(n)                 ____INTELLISENE_ATTR_FUNC
#define ____INTELLISENE_attribute___regparm(n)               ____INTELLISENE_ATTR_FUNC
#define ____INTELLISENE_attribute___regparm__(n)             ____INTELLISENE_ATTR_FUNC
#define ____INTELLISENE_attribute_returns_twice              ____INTELLISENE_ATTR_FUNC
#define ____INTELLISENE_attribute___returns_twice            ____INTELLISENE_ATTR_FUNC
#define ____INTELLISENE_attribute___returns_twice__          ____INTELLISENE_ATTR_FUNC

#if 1
enum{ /* Highlight attributes in a different color */
 __noreturn__,           /*< __attribute__((noreturn)). */
 __noinline__,           /*< __attribute__((noinline)). */
 __pure__,               /*< __attribute__((const)). */
 __const__,              /*< __attribute__((const)). */
 __deprecated__,         /*< __attribute__((deprecated(...))). */
 __malloc__,             /*< __attribute__((malloc)). */
 __alloc_size__,         /*< __attribute__((alloc_size(...))). */
 __always_inline__,      /*< __attribute__((always_inline)). */
 __sentinel__,           /*< __attribute__((sentinel)). */
 __visibility__,         /*< __attribute__((visibility(...))). */
 __nonnull__,            /*< __attribute__((nonnull(...))). */
 __used__,               /*< __attribute__((used)). */
 __unused__,             /*< __attribute__((unused)). */
 __aligned__,            /*< __attribute__((aligned(...))). */
 __packed__,             /*< __attribute__((packed)). */
 __format__,             /*< __attribute__((format(...))). */
 __returns_nonnull__,    /*< __attribute__((returns_nonnull)). */
 __warn_unused_result__, /*< __attribute__((warn_unused_result)). */
 __cdecl__,              /*< __attribute__((cdecl)). */
 __fastcall__,           /*< __attribute__((fastcall)). */
 __stdcall__,            /*< __attribute__((stdcall)). */
 __section__,            /*< __attribute__((section(...))). */
 __constructor__,        /*< __attribute__((constructor)). */
 __destructor__,         /*< __attribute__((destructor)). */
 __weak__,               /*< __attribute__((weak)). */
 __alias__,              /*< __attribute__((alias(...))). */
 __regparm__,            /*< __attribute__((regparm(...))). */
 __returns_twice__,      /*< __attribute__((returns_twice)). */
};
#endif


template<class T> struct ____INTELLISENE_remlval { typedef T __type; };
template<class T> struct ____INTELLISENE_remlval<T &> { typedef T __type; };
struct ____INTELLISENE_void_helper {};
template<class T> T operator , (T &&,____INTELLISENE_void_helper);
template<class T> T const operator , (T const &&,____INTELLISENE_void_helper);
void ____INTELLISENE_typeof_helper(____INTELLISENE_void_helper &&);
template<class T> typename ____INTELLISENE_remlval<T>::__type ____INTELLISENE_typeof_helper(T&&);
template<class T> typename ____INTELLISENE_remlval<T>::__type const ____INTELLISENE_typeof_helper(T const &&);
template<class T, class ...Args> T (____INTELLISENE_typeof_helper(T (&)(Args...)))(Args...);
template<class T, class ...Args> T (____INTELLISENE_typeof_helper(T (&)(Args...,...)))(Args...,...);
#define typeof(...)     decltype(::____INTELLISENE_typeof_helper(((__VA_ARGS__),::____INTELLISENE_void_helper())))
#define __typeof(...)   decltype(::____INTELLISENE_typeof_helper(((__VA_ARGS__),::____INTELLISENE_void_helper())))
#define __typeof__(...) decltype(::____INTELLISENE_typeof_helper(((__VA_ARGS__),::____INTELLISENE_void_helper())))
#define __auto_type     auto

template<class T> struct ____INTELLISENE_remcv {typedef T __type; };
template<class T> struct ____INTELLISENE_remcv<T const> {typedef T __type; };
template<class T> struct ____INTELLISENE_remcv<T volatile> {typedef T __type; };
template<class T> struct ____INTELLISENE_remcv<T const volatile> {typedef T __type; };
template<class T1, class T2> struct ____INTELLISENE_sametype_impl {enum{__val=false};};
template<class T1> struct ____INTELLISENE_sametype_impl<T1,T1> {enum{__val=true};};
template<class T1, class T2> struct ____INTELLISENE_sametype:
 ____INTELLISENE_sametype_impl<typename ____INTELLISENE_remcv<T1>::__type,
                               typename ____INTELLISENE_remcv<T2>::__type>{};
#define __builtin_types_compatible_p(...) ____INTELLISENE_sametype< __VA_ARGS__ >::__val

template<class T> struct ____INTELLISENE_classify {
 enum{__val = __is_enum(T) ? 3 :
              __is_class(T) ? 12 :
              __is_union(T) ? 13 : -1};
};
template<> struct ____INTELLISENE_classify<void> {enum{__val=0};};
template<> struct ____INTELLISENE_classify<signed char> {enum{__val=1};};
template<> struct ____INTELLISENE_classify<unsigned char> {enum{__val=1};};
template<> struct ____INTELLISENE_classify<short> {enum{__val=1};};
template<> struct ____INTELLISENE_classify<unsigned short> {enum{__val=1};};
template<> struct ____INTELLISENE_classify<int> {enum{__val=1};};
template<> struct ____INTELLISENE_classify<unsigned int> {enum{__val=1};};
template<> struct ____INTELLISENE_classify<long> {enum{__val=1};};
template<> struct ____INTELLISENE_classify<unsigned long> {enum{__val=1};};
template<> struct ____INTELLISENE_classify<long long> {enum{__val=1};};
template<> struct ____INTELLISENE_classify<unsigned long long> {enum{__val=1};};
template<> struct ____INTELLISENE_classify<char> {enum{__val=2};};
template<> struct ____INTELLISENE_classify<bool> {enum{__val=4};};
template<> struct ____INTELLISENE_classify<float> {enum{__val=8};};
template<> struct ____INTELLISENE_classify<double> {enum{__val=8};};
template<> struct ____INTELLISENE_classify<long double> {enum{__val=8};};
template<__SIZE_TYPE__ s> struct ____INTELLISENE_classify<char[s]> {enum{__val=15};};
template<class T, __SIZE_TYPE__ s> struct ____INTELLISENE_classify<T[s]> {enum{__val=14};};
template<class T> struct ____INTELLISENE_classify<T *> {enum{__val=5};};
template<class T> struct ____INTELLISENE_classify<T &> {enum{__val=6};};
template<class T> struct ____INTELLISENE_classify<T::*> {enum{__val=7};};
template<class T, class ...Args> struct ____INTELLISENE_classify<T(Args...)> {enum{__val=10};};
template<class T, class ...Args> struct ____INTELLISENE_classify<T(Args...,...)> {enum{__val=10};};
template<class T, class C, class ...Args> struct ____INTELLISENE_classify<T(C::*)(Args...)> {enum{__val=11};};
template<class T, class C, class ...Args> struct ____INTELLISENE_classify<T(C::*)(Args...,...)> {enum{__val=11};};
#define __builtin_classify_type(...) ____INTELLISENE_classify<__typeof__(__VA_ARGS__)>::__val

__declspec(noreturn) void __builtin_abort(void);
__declspec(noreturn) void __compiler_unreachable(void);
__declspec(noreturn) void __builtin_trap(void);
__declspec(noreturn) void __builtin_exit(void);
__declspec(noreturn) void __builtin__exit(void);
__declspec(noreturn) void __builtin__Exit(void);

void *__builtin_alloca(__SIZE_TYPE__ s);
long __builtin_expect(long val, long expect);
int __builtin_ffs(int x);
int __builtin_clz(unsigned int x);
int __builtin_ctz(unsigned int x);
int __builtin_clrsb(int x);
int __builtin_popcount(unsigned int x);
int __builtin_parity(unsigned int x);
int __builtin_ffsl(long);
int __builtin_clzl(unsigned long);
int __builtin_ctzl(unsigned long);
int __builtin_clrsbl(long);
int __builtin_popcountl(unsigned long);
int __builtin_parityl(unsigned long);
int __builtin_ffsll(long long);
int __builtin_clzll(unsigned long long);
int __builtin_ctzll(unsigned long long);
int __builtin_clrsbll(long long);
int __builtin_popcountll(unsigned long long);
int __builtin_parityll(unsigned long long);
unsigned __int16 __builtin_bswap16(unsigned __int16);
unsigned __int32 __builtin_bswap32(unsigned __int32);
unsigned __int64 __builtin_bswap64(unsigned __int64);
char const *__builtin_FUNCTION(void);
char const *__builtin_FILE(void);
int __builtin_LINE(void);

int __builtin_setjmp(void *);
__declspec(noreturn) void __builtin_longjmp(void *, int);


#define __builtin_offsetof(s,m) (__SIZE_TYPE__)(&((s*)0)->m)
#define __builtin_constant_p(x) 0

template<bool> struct ____INTELLISENE_static_if_helper {};
template<> struct ____INTELLISENE_static_if_helper<true> { bool __is_true__(); };
#define __builtin_choose_expr(c,tt,ff) \
     __if_exists(::____INTELLISENE_static_if_helper<((c))>::__is_true__){tt} \
 __if_not_exists(::____INTELLISENE_static_if_helper<((c))>::__is_true__){ff}

typedef char *__builtin_va_list,*__gnuc_va_list;
template<class T> void __builtin_va_start(__builtin_va_list &ap, T &before_start);
void __builtin_va_end(__builtin_va_list &ap);
void __builtin_va_copy(__builtin_va_list &dst_ap, __builtin_va_list &src_ap);
template<class T> T ____INTELLISENE_va_arg_heper(__builtin_va_list &);
#define __builtin_va_arg(ap,T)   ____INTELLISENE_va_arg_heper< T >(ap)


#define __ATOMIC_RELAXED 0
#define __ATOMIC_CONSUME 1
#define __ATOMIC_ACQUIRE 2
#define __ATOMIC_RELEASE 3
#define __ATOMIC_ACQ_REL 4
#define __ATOMIC_SEQ_CST 5

#define __GCC_ATOMIC_BOOL_LOCK_FREE     1
#define __GCC_ATOMIC_CHAR_LOCK_FREE     1
#define __GCC_ATOMIC_CHAR16_T_LOCK_FREE 1
#define __GCC_ATOMIC_CHAR32_T_LOCK_FREE 1
#define __GCC_ATOMIC_WCHAR_T_LOCK_FREE  1
#define __GCC_ATOMIC_SHORT_LOCK_FREE    1
#define __GCC_ATOMIC_INT_LOCK_FREE      1
#define __GCC_ATOMIC_LONG_LOCK_FREE     1
#define __GCC_ATOMIC_LLONG_LOCK_FREE    1
#define __GCC_ATOMIC_POINTER_LOCK_FREE  1

template<class T, class S> struct ____INTELLISENE_enableif_nc { typedef S __type; };
template<class T, class S> struct ____INTELLISENE_enableif_nc<T const,S> {};

template<class type>          typename ____INTELLISENE_enableif_nc<type,type>::__type __atomic_load_n(type const volatile *ptr, int memorder);
template<class type>          typename ____INTELLISENE_enableif_nc<type,void>::__type __atomic_load(type const volatile *ptr, type *ret, int memorder);
template<class type, class S> typename ____INTELLISENE_enableif_nc<type,void>::__type __atomic_store_n(type volatile *ptr, S val, int memorder);
template<class type>          typename ____INTELLISENE_enableif_nc<type,void>::__type __atomic_store(type volatile *ptr, type *val, int memorder);
template<class type, class S> typename ____INTELLISENE_enableif_nc<type,type>::__type __atomic_exchange_n(type volatile *ptr, S val, int memorder);
template<class type>          typename ____INTELLISENE_enableif_nc<type,void>::__type __atomic_exchange(type volatile *ptr, type *val, type *ret, int memorder);
template<class type, class S> typename ____INTELLISENE_enableif_nc<type,bool>::__type __atomic_compare_exchange_n(type volatile *ptr, type *expected, S desired, bool weak, int success_memorder, int failure_memorder);
template<class type>          typename ____INTELLISENE_enableif_nc<type,bool>::__type __atomic_compare_exchange(type volatile *ptr, type *expected, type *desired, bool weak, int success_memorder, int failure_memorder);
template<class type, class S> typename ____INTELLISENE_enableif_nc<type,type>::__type __atomic_add_fetch(type volatile *ptr, S val, int memorder);
template<class type, class S> typename ____INTELLISENE_enableif_nc<type,type>::__type __atomic_sub_fetch(type volatile *ptr, S val, int memorder);
template<class type, class S> typename ____INTELLISENE_enableif_nc<type,type>::__type __atomic_and_fetch(type volatile *ptr, S val, int memorder);
template<class type, class S> typename ____INTELLISENE_enableif_nc<type,type>::__type __atomic_xor_fetch(type volatile *ptr, S val, int memorder);
template<class type, class S> typename ____INTELLISENE_enableif_nc<type,type>::__type __atomic_or_fetch(type volatile *ptr, S val, int memorder);
template<class type, class S> typename ____INTELLISENE_enableif_nc<type,type>::__type __atomic_nand_fetch(type volatile *ptr, S val, int memorder);
template<class type, class S> typename ____INTELLISENE_enableif_nc<type,type>::__type __atomic_fetch_add(type volatile *ptr, S val, int memorder);
template<class type, class S> typename ____INTELLISENE_enableif_nc<type,type>::__type __atomic_fetch_sub(type volatile *ptr, S val, int memorder);
template<class type, class S> typename ____INTELLISENE_enableif_nc<type,type>::__type __atomic_fetch_and(type volatile *ptr, S val, int memorder);
template<class type, class S> typename ____INTELLISENE_enableif_nc<type,type>::__type __atomic_fetch_xor(type volatile *ptr, S val, int memorder);
template<class type, class S> typename ____INTELLISENE_enableif_nc<type,type>::__type __atomic_fetch_or(type volatile *ptr, S val, int memorder);
template<class type, class S> typename ____INTELLISENE_enableif_nc<type,type>::__type __atomic_fetch_nand(type volatile *ptr, S val, int memorder);
bool __atomic_test_and_set(void *ptr, int memorder);
void __atomic_clear(bool *ptr, int memorder);
void __atomic_thread_fence(int memorder);
void __atomic_signal_fence(int memorder);
bool __atomic_always_lock_free(size_t size, void const volatile *ptr);
bool __atomic_is_lock_free(size_t size, void const volatile *ptr);


template<class type> type __sync_fetch_and_add(type volatile *ptr, type value, ...);
template<class type> type __sync_fetch_and_sub(type volatile *ptr, type value, ...);
template<class type> type __sync_fetch_and_or(type volatile *ptr, type value, ...);
template<class type> type __sync_fetch_and_and(type volatile *ptr, type value, ...);
template<class type> type __sync_fetch_and_xor(type volatile *ptr, type value, ...);
template<class type> type __sync_fetch_and_nand(type volatile *ptr, type value, ...);
template<class type> type __sync_add_and_fetch(type volatile *ptr, type value, ...);
template<class type> type __sync_sub_and_fetch(type volatile *ptr, type value, ...);
template<class type> type __sync_or_and_fetch(type volatile *ptr, type value, ...);
template<class type> type __sync_and_and_fetch(type volatile *ptr, type value, ...);
template<class type> type __sync_xor_and_fetch(type volatile *ptr, type value, ...);
template<class type> type __sync_nand_and_fetch(type volatile *ptr, type value, ...);
template<class type> bool __sync_bool_compare_and_swap(type volatile *ptr, type oldval, type newval, ...);
template<class type> type __sync_val_compare_and_swap(type volatile *ptr, type oldval, type newval, ...);
void __sync_synchronize(...);
template<class type> type __sync_lock_test_and_set(type *ptr, type value, ...);
template<class type> void __sync_lock_release(type *ptr, ...);


class char16_t {
 unsigned __int16 __v;
public:
 operator unsigned __int16 & (void) throw();
 operator unsigned __int16 const & (void) const throw();
 char16_t(unsigned __int16);
};
class char32_t {
 unsigned __int32 __v;
public:
 operator unsigned __int32 & (void) throw();
 operator unsigned __int32 const & (void) const throw();
 char32_t(unsigned __int32);
};


// Used to implement endian-specific integers capable of
// emitting warnings when attempted to be used as
// regular integers without prior casting using the.
// (le|be)swap_(16|32|64) functions from <byteswap.h>
// >> Again: Only meant to highlight usage errors in visual studio.

#pragma pack(push,1)
template<int endian, class __T> class ____INTELLISENE_integer {
 __T __v;
public:
 //bool operator ! (void) const;
 explicit operator bool(void) const throw();
 explicit operator char(void) const throw();
 explicit operator short(void) const throw();
 explicit operator int(void) const throw();
 explicit operator long(void) const throw();
 explicit operator __int64(void) const throw();
 explicit operator signed char(void) const throw();
 explicit operator unsigned char(void) const throw();
 explicit operator unsigned short(void) const throw();
 explicit operator unsigned int(void) const throw();
 explicit operator unsigned long(void) const throw();
 explicit operator unsigned __int64(void) const throw();
 template<class __S> explicit bool operator <  (____INTELLISENE_integer<endian,__S>) const throw();
 template<class __S> explicit bool operator <= (____INTELLISENE_integer<endian,__S>) const throw();
 template<class __S> explicit bool operator == (____INTELLISENE_integer<endian,__S>) const throw();
 template<class __S> explicit bool operator != (____INTELLISENE_integer<endian,__S>) const throw();
 template<class __S> explicit bool operator >  (____INTELLISENE_integer<endian,__S>) const throw();
 template<class __S> explicit bool operator >= (____INTELLISENE_integer<endian,__S>) const throw();
 //explicit ____INTELLISENE_integer(__T) throw();
};
#pragma pack(pop)

unsigned short ____INTELLISENE_leswap16(____INTELLISENE_integer<1234,unsigned short>);
unsigned int ____INTELLISENE_leswap32(____INTELLISENE_integer<1234,unsigned int>);
unsigned long ____INTELLISENE_leswap32(____INTELLISENE_integer<1234,unsigned long>);
unsigned __int64 ____INTELLISENE_leswap64(____INTELLISENE_integer<1234,unsigned __int64>);
____INTELLISENE_integer<1234,unsigned short> ____INTELLISENE_leswap16(unsigned short);
____INTELLISENE_integer<1234,unsigned int> ____INTELLISENE_leswap32(unsigned int);
____INTELLISENE_integer<1234,unsigned long> ____INTELLISENE_leswap32(unsigned long);
____INTELLISENE_integer<1234,unsigned __int64> ____INTELLISENE_leswap64(unsigned __int64);

unsigned short ____INTELLISENE_beswap16(____INTELLISENE_integer<4321,unsigned short>);
unsigned int ____INTELLISENE_beswap32(____INTELLISENE_integer<4321,unsigned int>);
unsigned long ____INTELLISENE_beswap32(____INTELLISENE_integer<4321,unsigned long>);
unsigned __int64 ____INTELLISENE_beswap64(____INTELLISENE_integer<4321,unsigned __int64>);
____INTELLISENE_integer<4321,unsigned short> ____INTELLISENE_beswap16(unsigned short);
____INTELLISENE_integer<4321,unsigned int> ____INTELLISENE_beswap32(unsigned int);
____INTELLISENE_integer<4321,unsigned long> ____INTELLISENE_beswap32(unsigned long);
____INTELLISENE_integer<4321,unsigned __int64> ____INTELLISENE_beswap64(unsigned __int64);


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

#if defined(__GNUC__) && (defined(__GXX_EXPERIMENTAL_CXX0X__) || __cplusplus >= 201103L)
#   define __COMPILER_GCC_CXX11 1
#endif

#if defined(__BORLANDC__) && __BORLANDC__ >= 0x599
#   pragma defineonoption __COMPILER_CODEGEAR_0X_SUPPORT -Ax
#endif

#ifndef __COMPILER_INTEL_CXX_VERSION
#if defined(__INTEL_COMPILER)
#if __INTEL_COMPILER == 9999
# define __COMPILER_INTEL_CXX_VERSION 1200
#else
# define __COMPILER_INTEL_CXX_VERSION __INTEL_COMPILER
#endif
#elif defined(__ICL)
# define __COMPILER_INTEL_CXX_VERSION __ICL
#elif defined(__ICC)
# define __COMPILER_INTEL_CXX_VERSION __ICC
#elif defined(__ECC)
# define __COMPILER_INTEL_CXX_VERSION __ECC
#endif
#endif /* !__COMPILER_INTEL_CXX_VERSION */


#ifndef __attribute_inline
#define __attribute_inline  inline
#endif

#ifndef __STATIC_ASSERT
#define __STATIC_ASSERT(expr)       static_assert((expr),#expr)
#endif
#ifndef __STATIC_ASSERT_M
#define __STATIC_ASSERT_M(expr,msg) static_assert((expr),msg)
#endif

#ifndef __cxx14_constexpr
#if (defined(__clang__) && !(!__has_feature(cxx_generic_lambdas) || \
   !(__has_feature(cxx_relaxed_constexpr) || __has_extension(cxx_relaxed_constexpr)))) || \
    (defined(__cpp_constexpr) && __cpp_constexpr >= 201304 && !defined(__clang__))
#   define __cxx14_constexpr    constexpr
#endif
#endif /* !__cxx14_constexpr */

#ifndef __cxx11_constexpr
#ifdef __cxx14_constexpr
#   define __cxx11_constexpr    __cxx14_constexpr
#elif __has_feature(cxx_constexpr) || \
   (defined(__cpp_constexpr) && __cpp_constexpr >= 200704) || \
   (defined(__cplusplus) && defined(__IBMCPP__) && defined(__IBMCPP_CONSTEXPR) && __IBMCPP_CONSTEXPR) || \
   (defined(__cplusplus) && defined(__SUNPRO_CC) && (__SUNPRO_CC >= 0x5130)) || \
   (defined(__cplusplus) && __GCC_VERSION(4,6,0) && defined(__GNUC_CXX11__)) || \
   (defined(__cplusplus) && defined(_MSC_FULL_VER) && (_MSC_FULL_VER >= 190023026))
#   define __cxx11_constexpr    constexpr
#endif
#endif /* !__cxx11_constexpr */

#ifndef __cxx_noexcept
#if __has_feature(cxx_noexcept) || \
   (defined(__cplusplus) && defined(__GXX_EXPERIMENTAL_CXX0X__) && __GCC_VERSION(4,6,0)) || \
   (defined(__cplusplus) && defined(_MSC_FULL_VER) && _MSC_FULL_VER >= 190021730 /*190023026*/)
#   define __cxx_noexcept          noexcept
#   define __cxx_noexcept_is(expr) noexcept(expr)
#   define __cxx_noexcept_if(expr) noexcept(expr)
#else
#   define __cxx_noexcept          throw()
#   define __NO_cxx_noexcept_is
#   define __NO_cxx_noexcept_if
#   define __cxx_noexcept_is(expr) 0
#   define __cxx_noexcept_if(expr) /* nothing */
#endif
#endif

#if __has_feature(cxx_rvalue_references) || \
   (defined(__cpp_rvalue_references) && __cpp_rvalue_references >= 200610) || \
   (defined(__cplusplus) && defined(_MSC_VER) && _MSC_VER >= 1600) || \
   (defined(__cplusplus) && __GCC_VERSION(4,3,0) && defined(__COMPILER_GCC_CXX11)) || \
   (defined(__cplusplus) && defined(__BORLANDC__) && defined(__COMPILER_CODEGEAR_0X_SUPPORT) && __BORLANDC__ >= 0x610) || \
   (defined(__cplusplus) && defined(__IBMCPP_RVALUE_REFERENCES) && __IBMCPP_RVALUE_REFERENCES)
#   define __cxx11_rvalue_reference(T) T&&
#else
#   define __NO_cxx11_rvalue_reference
#   define __cxx11_rvalue_reference(T) T
#endif

#if __has_feature(cxx_variadic_templates) || \
   (defined(__cpp_variadic_templates) && __cpp_variadic_templates >= 200704) || \
   (defined(__cplusplus) && __GCC_VERSION(4,3,0) && defined(__COMPILER_GCC_CXX11)) || \
   (defined(__cplusplus) && defined(_MSC_VER) && _MSC_VER >= 1800) || \
   (defined(__cplusplus) && defined(__IBMCPP_VARIADIC_TEMPLATES) && __IBMCPP_VARIADIC_TEMPLATES)
#else
#   define __NO_cxx11_variadic_templates 0
#endif

#if defined(_NATIVE_CHAR16_T_DEFINED) || \
   (defined(__cpp_unicode_characters) && __cpp_unicode_characters >= 200704) || \
   (defined(_HAS_CHAR16_T_LANGUAGE_SUPPORT) && (_HAS_CHAR16_T_LANGUAGE_SUPPORT-0)) || \
   (defined(__cplusplus) && defined(_MSC_VER) && _MSC_VER >= 1900) || \
   (defined(__cplusplus) && defined(__clang__) && !defined(_MSC_VER) && (defined(__GXX_EXPERIMENTAL_CXX0X__) || __cplusplus >= 201103L)) || \
   (defined(__cplusplus) && defined(__COMPILER_GCC_CXX11) && __GCC_VERSION(4,4,0)) || \
   (defined(__cplusplus) && defined(__BORLANDC__) && defined(__COMPILER_CODEGEAR_0X_SUPPORT) && __BORLANDC__ >= 0x610) || \
   (defined(__cplusplus) && defined(__IBMCPP_UTF_LITERAL__) && __IBMCPP_UTF_LITERAL__)
#else
#   define __NO_char16_t
#endif


#define __CXX_DECL_BEGIN
#define __CXX_DECL_END

#ifndef __DECL_BEGIN
#define __DECL_BEGIN  extern "C" {
#define __DECL_END    }
#endif

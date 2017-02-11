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
#ifndef __MATH_H__
#ifndef _MATH_H
#ifndef _INC_MATH
#define __MATH_H__ 1
#define _MATH_H 1
#define _INC_MATH 1

#include <kos/compiler.h>
#include <minmax.h>

#ifndef __ASSEMBLY__
__DECL_BEGIN

#ifndef __STDC_PURE__
// non-conforming names aliases for extension KOS functions/macros.
#ifdef __INTELLISENSE__
#define _align     align
#define _alignd    alignd
#define _isaligned isaligned
#define _ispow2    ispow2
#define _ceildiv   ceildiv
#define _sqr       sqr
#else
#define align     _align
#define alignd    _alignd
#define isaligned _isaligned
#define ispow2    _ispow2
#define ceildiv   _ceildiv
#define sqr       _sqr
#endif
#endif

#if defined(__INTELLISENSE__) && defined(__cplusplus)
extern "C++" {

//////////////////////////////////////////////////////////////////////////
// Returns the integral 'x' aligned to the closest, greater value
// conforming to the alignment requirements described by 'alignment'.
// NOTE: Causes weak undefined behavior if 'alignment' isn't power_of(2).
template<class Tx, class Talignment> Tx _align(Tx x, Talignment alignment);
template<class Tx, class Talignment> Tx _alignd(Tx x, Talignment alignment);

//////////////////////////////////////////////////////////////////////////
// Returns TRUE(1) if the integral value 'x' is aligned by 'alignment'
// NOTE: Causes weak undefined behavior if 'alignment' isn't power_of(2).
template<class Tx, class Talignment> bool _isaligned(Tx x, Talignment alignment);

//////////////////////////////////////////////////////////////////////////
// Checks if a given value 'x' is a power-of-two value (x is in 2**x)
// @return: TRUE(1):  'x' is a power_of(2) integral value.
// @return: FALSE(0): ... otherwise.
template<class T> bool _ispow2(T x);

//////////////////////////////////////////////////////////////////////////
// Ceil-divide two given values 'x' and 'y'
//  - Invocation of this function does not make use of the FPU (if any)
// @return: * : The value of x/y, rounding upwards if the division cannot be resolved equally.
template<class Tx, class Ty> Tx _ceildiv(Tx x, Ty y);

//////////////////////////////////////////////////////////////////////////
// Returns the square of a given integral value.
// @return: * : The square of 'x'
template<class Tx> auto _sqr(Tx x) -> decltype(x*x);

}
#else
#define _align(x,alignment) \
 (__builtin_constant_p(alignment)\
  ? (((x)+((alignment)-1))&~((alignment)-1))\
  : __xblock({ __typeof__(alignment) const __alalignment = (alignment);\
               __xreturn ((x)+(__alalignment-1))&~(__alalignment-1);\
  }))
#define _alignd(x,alignment)    ((x)&~((alignment)-1))
#define _isaligned(x,alignment) (((x)&((alignment)-1))==0)
#define _ispow2(x) \
 (__builtin_constant_p(alignment)\
  ? (((x)&((x)-1))==0)\
  : __xblock({ __typeof__(x) const __isp2x = (x);\
               __xreturn (__isp2x&(__isp2x-1))==0;\
  }))
#define _ceildiv(x,y) \
 (__builtin_constant_p(y)\
  ? ((x)+((y)-1))/(y)\
  : __xblock({ __typeof__(y) const __cdy = (y);\
               __xreturn ((x)+(__cdy-1))/__cdy;\
  }))
#define _sqr(x) \
 __xblock({ __typeof__(x) const __x = (x);\
            __xreturn __x*__x;\
 })
#endif


struct exception {
    int    type;
    char  *name;
    double arg1;
    double arg2;
    double retval;
};

extern __wunused __constcall double acos __P((double x));
extern __wunused __constcall double asin __P((double x));
extern __wunused __constcall double atan __P((double x));
extern __wunused __constcall double atan2 __P((double y, double x));
extern __wunused __constcall double cos __P((double x));
extern __wunused __constcall double sin __P((double x));
extern __wunused __constcall double tan __P((double x));
extern __wunused __constcall double cosh __P((double x));
extern __wunused __constcall double sinh __P((double x));
extern __wunused __constcall double tanh __P((double x));
extern __wunused __constcall double exp __P((double x));
extern __wunused __constcall double frexp __P((double x, int *eptr));
extern __wunused __constcall double ldexp __P((double x, int exp));
extern __wunused __constcall double log __P((double x));
extern __wunused __constcall double log10 __P((double x));
extern __wunused __constcall __nonnull((2)) double modf __P((double x, double *iptr));
extern __wunused __constcall double pow __P((double x, double y));
extern __wunused __constcall double sqrt __P((double x));
extern __wunused __constcall double ceil __P((double x));
extern __wunused __constcall double fabs __P((double x));
extern __wunused __constcall double floor __P((double x));
extern __wunused __constcall double fmod __P((double x, double y));
extern __wunused __constcall double erf __P((double x));
extern __wunused __constcall double erfc __P((double x));
extern __wunused __constcall double gamma __P((double x));
extern __wunused __constcall double hypot __P((double x, double y));
extern __wunused __constcall int isnan __P((double x));
extern __wunused __constcall int finite __P((double x));
extern __wunused __constcall double j0 __P((double x));
extern __wunused __constcall double j1 __P((double x));
extern __wunused __constcall double jn __P((int n, double x));
extern __wunused __constcall double lgamma __P((double x));
extern __wunused __constcall double y0 __P((double x));
extern __wunused __constcall double y1 __P((double x));
extern __wunused __constcall double yn __P((int n, double x));

extern __wunused __constcall double acosh __P((double x));
extern __wunused __constcall double asinh __P((double x));
extern __wunused __constcall double atanh __P((double x));
extern __wunused __constcall double cbrt __P((double x));
extern __wunused __constcall double logb __P((double x));
extern __wunused __constcall double nextafter __P((double x, double y));
extern __wunused __constcall double remainder __P((double x, double y));
extern __wunused __constcall double scalb __P((double x, int fn));
extern __wunused __constcall __nonnull((1)) int matherr __P((struct exception *x));
extern __wunused __constcall double significand __P((double x));
extern __wunused __constcall double copysign __P((double x, double y));
extern __wunused __constcall int ilogb __P((double x));
extern __wunused __constcall double rint __P((double x));
extern __wunused __constcall double scalbn __P((double x, int fn));
extern __wunused __constcall double expm1 __P((double x));
extern __wunused __constcall double log1p __P((double x));


#  define isfinite            finite
#  define isinf(x)            0 /* TODO */
//#define isnan(x)            0 /* TODO */
#  define isnormal(x)         0 /* TODO */
#  define signbit(x)          0 /* TODO */
#  define isgreater(x,y)      0 /* TODO */
#  define isgreaterequal(x,y) 0 /* TODO */
#  define isless(x,y)         0 /* TODO */
#  define islessequal(x,y)    0 /* TODO */
#  define islessgreater(x,y)  0 /* TODO */
#  define isunordered(x,y)    0 /* TODO */

__DECL_END
#endif /* !__ASSEMBLY__ */

#endif /* !_INC_MATH */
#endif /* !_MATH_H */
#endif /* !__MATH_H__ */

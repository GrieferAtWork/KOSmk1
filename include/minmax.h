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
#ifndef __MINMAX_H__
#ifndef _INC_MINMAX
#define __MINMAX_H__ 1
#define _INC_MINMAX 1

#include <kos/compiler.h>

__DECL_BEGIN

#ifndef __STDC_PURE__
/* non-conforming names aliases for extension KOS functions/macros. */
#ifdef __INTELLISENSE__
#   define _min       min
#   define _max       max
#else
#   define min(x,y)  _min(x,y)
#   define max(x,y)  _max(x,y)
#endif
#endif

#if defined(__INTELLISENSE__) && defined(__cplusplus)
extern "C++" {

//////////////////////////////////////////////////////////////////////////
// Returns the minimum of two given integral values.
// @return: * : The lower value of 'x' and 'y'
template<class Tx, class Ty> auto _min(Tx x, Ty y) -> decltype(1 ? x : y);

//////////////////////////////////////////////////////////////////////////
// Returns the maximum of two given integral values.
// @return: * : The greater value of 'x' and 'y'
template<class Tx, class Ty> auto _max(Tx x, Ty y) -> decltype(1 ? x : y);

}
#elif defined(__NO_xblock)
#define _min(x,y) ((x) < (y) ? (x) : (y))
#define _max(x,y) ((x) > (y) ? (x) : (y))
#else
#define _min(x,y) \
 __xblock({ __typeof__(x) const __mx = (x);\
            __typeof__(y) const __my = (y);\
            __xreturn __mx < __my ? __mx : __my;\
 })
#define _max(x,y) \
 __xblock({ __typeof__(x) const __mx = (x);\
            __typeof__(y) const __my = (y);\
            __xreturn __mx > __my ? __mx : __my;\
 })
#endif

__DECL_END

#endif  /* !_INC_MINMAX */
#endif  /* !__MINMAX_H__ */

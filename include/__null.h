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
#ifndef ____NULL_H__
#define ____NULL_H__ 1

#ifndef NULL
#ifdef __ASSEMBLY__
#   define NULL  0
#elif defined(__cplusplus)
#ifdef __INTELLISENSE__
/* I know this isn't standard-conforming, but for new,
 * as well as existing code, this is what I think an IDE
 * should highlight as the only correct use.
 * >> Though legacy code doing some _very_ questionable
 *    stuff will still be possible to compile, as this
 *    is merely meant for intellisense (and other IDEs) */
#   define NULL  nullptr
#elif defined(__SIZEOF_LONG__) && __SIZEOF_LONG__ == __SIZEOF_POINTER__
#   define NULL  0l
#elif defined(__SIZEOF_LONG_LONG__) && __SIZEOF_LONG_LONG__ == __SIZEOF_POINTER__
#   define NULL  0ll
#else
#   define NULL  0
#endif
#else
#   define NULL  ((void *)0)
#endif
#endif

#endif /* !____NULL_H__ */

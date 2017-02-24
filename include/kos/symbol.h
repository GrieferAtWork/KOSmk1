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
#ifndef __KOS_SYMBOL_H__
#define __KOS_SYMBOL_H__ 1

#include <kos/config.h>
#include <kos/compiler.h>

#if defined(__SYMBOL_NEED_UNDERSCORE) ||\
    defined(NEED_UNDERSCORE)
#   define __SYM(sym) _##sym
#elif defined(__USER_LABEL_PREFIX__)
#   define __SYM3(x,sym) x##sym
#   define __SYM2(x,sym) __SYM3(x,sym)
#   define __SYM(sym)    __SYM2(__USER_LABEL_PREFIX__,sym)
#else
#   define __SYM(sym)    sym
#endif

#ifdef __ASSEMBLY__
#define _S   __SYM
#endif

#endif /* !__KOS_SYMBOL_H__ */

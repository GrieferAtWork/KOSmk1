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
#ifndef __ITOS_H__
#define __ITOS_H__ 1

#include <kos/compiler.h>
#include <kos/types.h>

#ifndef __ASSEMBLY__
__DECL_BEGIN

#ifndef __STDC_PURE__
#   define itos    _itos
#   define utos    _utos
#   define itox    _itox
#   define utox    _utox
#   define itos_ns _itos_ns
#   define utos_ns _utos_ns
#endif

#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Convert a given integer @i to a string, filling @buf
// @return: The used/needed buffersize excluding the terminating \0 character.
//          Returns 0 if an error occurred, such as use of an invalid numsys
// @param buf:     The location of the buffer to fill
// @param bufsize: The size of the given buffer @buf
// @param i:       The integer to convert
// >> itos: The integer is represented as decimal
// >> itox: The integer is represented as hex (in lowercase and with no prefix)
extern __nonnull((1)) __size_t _itos(char *__restrict buf, __size_t bufsize, /*any*/   signed i);
extern __nonnull((1)) __size_t _utos(char *__restrict buf, __size_t bufsize, /*any*/ unsigned i);
extern __nonnull((1)) __size_t _itox(char *__restrict buf, __size_t bufsize, /*any*/   signed i);
extern __nonnull((1)) __size_t _utox(char *__restrict buf, __size_t bufsize, /*any*/ unsigned i);
#else
#   define _itos(buf,bufsize,i) _itos_ns(buf,bufsize,i,10)
#   define _utos(buf,bufsize,i) _utos_ns(buf,bufsize,i,10)
#   define _itox(buf,bufsize,i) _itos_ns(buf,bufsize,i,16)
#   define _utox(buf,bufsize,i) _utos_ns(buf,bufsize,i,16)
#endif

#define _itos_ns(buf,bufsize,i,numsys) (sizeof(i) >= 8 ? _itos64_ns(buf,bufsize,(__s64)(i),numsys) : _itos32_ns(buf,bufsize,(__s32)(i),numsys))
#define _utos_ns(buf,bufsize,i,numsys) (sizeof(i) >= 8 ? _utos64_ns(buf,bufsize,(__u64)(i),numsys) : _utos32_ns(buf,bufsize,(__u32)(i),numsys))
extern __nonnull((1)) __size_t _itos32_ns __P((char *__restrict buf, __size_t bufsize, __s32 i, int numsys));
extern __nonnull((1)) __size_t _utos32_ns __P((char *__restrict buf, __size_t bufsize, __u32 i, int numsys));
extern __nonnull((1)) __size_t _itos64_ns __P((char *__restrict buf, __size_t bufsize, __s64 i, int numsys));
extern __nonnull((1)) __size_t _utos64_ns __P((char *__restrict buf, __size_t bufsize, __u64 i, int numsys));


extern __nonnull((1)) __size_t
_dtos __P((char *__restrict buf, __size_t bufsize, double d,
           unsigned int prec, unsigned int prec2, int g));

__DECL_END
#endif /* !__ASSEMBLY__ */

#endif /* !__ITOS_H__ */

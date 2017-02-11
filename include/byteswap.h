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
#ifndef __BYTESWAP_H__
#ifndef _BYTESWAP_H
#define __BYTESWAP_H__ 1
#define _BYTESWAP_H 1

#include <kos/compiler.h>
#ifndef __ASSEMBLY__
#include <kos/__byteswap.h>
#include <kos/endian.h>

__DECL_BEGIN

#define __bswap_16 __kos_bswap16
#define __bswap_32 __kos_bswap32
#define __bswap_64 __kos_bswap64
#define _bswap_16  __kos_bswap16
#define _bswap_32  __kos_bswap32
#define _bswap_64  __kos_bswap64
#define bswap_16   __kos_bswap16
#define bswap_32   __kos_bswap32
#define bswap_64   __kos_bswap64

#ifdef __INTELLISENSE__
#   define _leswap_16 ____INTELLISENE_leswap16
#   define _leswap_32 ____INTELLISENE_leswap32
#   define _leswap_64 ____INTELLISENE_leswap64
#   define _beswap_16 ____INTELLISENE_beswap16
#   define _beswap_32 ____INTELLISENE_beswap32
#   define _beswap_64 ____INTELLISENE_beswap64
#elif __BYTE_ORDER == __LITTLE_ENDIAN
#   define _leswap_16 /* nothing */
#   define _leswap_32 /* nothing */
#   define _leswap_64 /* nothing */
#   define _beswap_16 __kos_bswap16
#   define _beswap_32 __kos_bswap32
#   define _beswap_64 __kos_bswap64
#elif __BYTE_ORDER == __BIG_ENDIAN
#   define _leswap_16 __kos_bswap16
#   define _leswap_32 __kos_bswap32
#   define _leswap_64 __kos_bswap64
#   define _beswap_16 /* nothing */
#   define _beswap_32 /* nothing */
#   define _beswap_64 /* nothing */
#endif

#ifndef __STDC_PURE__
#   define leswap_16 _leswap_16
#   define leswap_32 _leswap_32
#   define leswap_64 _leswap_64
#   define beswap_16 _beswap_16
#   define beswap_32 _beswap_32
#   define beswap_64 _beswap_64
#endif

__DECL_END
#endif /* !__ASSEMBLY__ */

#endif /* !_BYTESWAP_H */
#endif /* !__BYTESWAP_H__ */

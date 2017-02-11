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
#ifndef __ENDIAN_H__
#ifndef _ENDIAN_H
#define __ENDIAN_H__ 1
#define _ENDIAN_H 1

#include <features.h>
#include <kos/endian.h>

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN __LITTLE_ENDIAN
#endif
#ifndef BIG_ENDIAN
#define BIG_ENDIAN    __BIG_ENDIAN
#endif
#ifndef PDP_ENDIAN
#define PDP_ENDIAN    __PDP_ENDIAN
#endif
#ifndef BYTE_ORDER
#define BYTE_ORDER    __BYTE_ORDER
#endif

#if __BYTE_ORDER == __LITTLE_ENDIAN
#   define __LONG_LONG_PAIR(HI, LO) LO, HI
#elif __BYTE_ORDER == __BIG_ENDIAN
#   define __LONG_LONG_PAIR(HI, LO) HI, LO
#endif

#ifndef __ASSEMBLY__
#if __BSD_VISIBLE
#include <kos/__byteswap.h>
#if __BYTE_ORDER == __LITTLE_ENDIAN
#   define htobe16 __kos_bswap16
#   define htobe32 __kos_bswap32
#   define htobe64 __kos_bswap64
#   define be16toh __kos_bswap16
#   define be32toh __kos_bswap32
#   define be64toh __kos_bswap64
#   define htole16 /* nothing */
#   define htole32 /* nothing */
#   define htole64 /* nothing */
#   define le16toh /* nothing */
#   define le32toh /* nothing */
#   define le64toh /* nothing */
#elif __BYTE_ORDER == __BIG_ENDIAN
#   define htobe16 /* nothing */
#   define htobe32 /* nothing */
#   define htobe64 /* nothing */
#   define be16toh /* nothing */
#   define be32toh /* nothing */
#   define be64toh /* nothing */
#   define htole16 __kos_bswap16
#   define htole32 __kos_bswap32
#   define htole64 __kos_bswap64
#   define le16toh __kos_bswap16
#   define le32toh __kos_bswap32
#   define le64toh __kos_bswap64
#endif /* ... */
#endif /* __BSD_VISIBLE */
#endif /* !__ASSEMBLY__ */

#endif /* !_ENDIAN_H */
#endif /* !__KOS_ENDIAN_H__ */

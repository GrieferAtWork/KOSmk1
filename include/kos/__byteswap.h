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
#ifndef ____KOS_BYTESWAP_H__
#define ____KOS_BYTESWAP_H__ 1

#include <kos/compiler.h>
#include <kos/arch/byteswap.h>

__DECL_BEGIN

#define __constant_bswap16(x) \
 ((__u16)(x) >> 8 | (__u16)(x) << 8)
#define __constant_bswap32(x) \
 ((__u32)(x) << 24 | ((__u32)(x) & 0xff00) << 8 |\
 (((__u32)(x) >> 8) & 0xff00) | ((__u32)(x) >> 24))
#define __constant_bswap64(x) \
 ((__u64)(x) << 56 |\
 ((__u64)(x) & __UINT64_C(0x000000000000FF00)) << 40 |\
 ((__u64)(x) & __UINT64_C(0x0000000000FF0000)) << 24 |\
 ((__u64)(x) & __UINT64_C(0x00000000FF000000)) << 8 |\
 ((__u64)(x) & __UINT64_C(0x000000FF00000000)) >> 8 |\
 ((__u64)(x) & __UINT64_C(0x0000FF0000000000)) >> 24 |\
 ((__u64)(x) & __UINT64_C(0x00FF000000000000)) >> 40 |\
 ((__u64)(x) >> 56))

#if __GCC_VERSION(4,4,0)
#   define __compiler_bswap32 __builtin_bswap32
#   define __compiler_bswap64 __builtin_bswap64
#endif
#if __GCC_VERSION(4,8,0) || (defined(__powerpc__) && __GCC_VERSION(4,6,0))
#   define __compiler_bswap16 __builtin_bswap16
#endif

#ifdef __compiler_bswap16
#   define __runtime_bswap16 __compiler_bswap16
#elif defined(karch_bswap16)
#   define __runtime_bswap16 karch_bswap16
#else
__local __constcall __u16 __runtime_bswap16(__u16 x) { return __constant_bswap16(x); }
#endif

#ifdef __compiler_bswap32
#   define __runtime_bswap32 __compiler_bswap32
#elif defined(karch_bswap32)
#   define __runtime_bswap32 karch_bswap32
#else
__local __constcall __u32 __runtime_bswap32(__u32 x) { return __constant_bswap32(x); }
#endif

#ifdef __compiler_bswap64
#   define __runtime_bswap64 __compiler_bswap64
#elif defined(karch_bswap64)
#   define __runtime_bswap64 karch_bswap64
#else
__local __constcall __u64 __runtime_bswap64(__u64 x) { return __constant_bswap64(x); }
#endif

#define __kos_bswap16(x) (__builtin_constant_p(x) ? __constant_bswap16(x) : __runtime_bswap16(x))
#define __kos_bswap32(x) (__builtin_constant_p(x) ? __constant_bswap32(x) : __runtime_bswap32(x))
#define __kos_bswap64(x) (__builtin_constant_p(x) ? __constant_bswap64(x) : __runtime_bswap64(x))

__DECL_END

#endif /* !____KOS_BYTESWAP_H__ */

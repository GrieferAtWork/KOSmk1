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
#ifndef __KOS_ARCH_GENERIC_STRING_FFS_H__
#define __KOS_ARCH_GENERIC_STRING_FFS_H__ 1

#include <kos/compiler.h>

#ifndef __ASSEMBLY__
#include <kos/types.h>
#include <stdint.h>

__DECL_BEGIN

#ifndef __ffsx_defined
#define __ffsx_defined 1
extern __wunused __constcall int _ffs8  __P((__u8 __i));
extern __wunused __constcall int _ffs16 __P((__u16 __i));
extern __wunused __constcall int _ffs32 __P((__u32 __i));
extern __wunused __constcall int _ffs64 __P((__u64 __i));
#endif

#ifndef __arch_ffs8
#   define __arch_generic_ffs8   _ffs8
#   define __arch_ffs8  __arch_generic_ffs8
#endif

#ifndef __arch_ffs16
#   define __arch_generic_ffs16  _ffs16
#   define __arch_ffs16 __arch_generic_ffs16
#endif

#ifndef __arch_ffs32
#if (defined(__GNUC__) || __has_builtin(__builtin_ffs)) && __SIZEOF_INT__ == 4
#   define __arch_ffs32(i)       __builtin_ffs((int)(i))
#elif (defined(__GNUC__) || __has_builtin(__builtin_ffs)) && __SIZEOF_LONG__ == 4
#   define __arch_ffs32(i)       __builtin_ffsl((long)(i))
#endif
#endif

#ifndef __arch_ffs64
#if (defined(__GNUC__) || __has_builtin(__builtin_ffsl)) && __SIZEOF_LONG__ == 8
#   define __arch_ffs64(i)       __builtin_ffsl((long)(i))
#elif !defined(__NO_longlong) && \
      (defined(__GNUC__) || __has_builtin(__builtin_ffsll)) && \
      (__SIZEOF_LONG_LONG__ == 8)
#   define __arch_ffs64(i)       __builtin_ffsll((long long)(i))
#else

#endif
#endif


__forcelocal __wunused __constcall
int __arch_constant_ffs8 __D1(__u8,__i) {
 if (__i & UINT8_C(0x01)) return 1;
 if (__i & UINT8_C(0x02)) return 2;
 if (__i & UINT8_C(0x04)) return 3;
 if (__i & UINT8_C(0x08)) return 4;
 if (__i & UINT8_C(0x10)) return 5;
 if (__i & UINT8_C(0x20)) return 6;
 if (__i & UINT8_C(0x40)) return 7;
 if (__i & UINT8_C(0x80)) return 8;
 return 0;
}
__forcelocal __wunused __constcall
int __arch_constant_ffs16 __D1(__u16,__i) {
 if (__i & UINT16_C(0x0001)) return 1;
 if (__i & UINT16_C(0x0002)) return 2;
 if (__i & UINT16_C(0x0004)) return 3;
 if (__i & UINT16_C(0x0008)) return 4;
 if (__i & UINT16_C(0x0010)) return 5;
 if (__i & UINT16_C(0x0020)) return 6;
 if (__i & UINT16_C(0x0040)) return 7;
 if (__i & UINT16_C(0x0080)) return 8;
 if (__i & UINT16_C(0x0100)) return 9;
 if (__i & UINT16_C(0x0200)) return 10;
 if (__i & UINT16_C(0x0400)) return 11;
 if (__i & UINT16_C(0x0800)) return 12;
 if (__i & UINT16_C(0x1000)) return 13;
 if (__i & UINT16_C(0x2000)) return 14;
 if (__i & UINT16_C(0x4000)) return 15;
 if (__i & UINT16_C(0x8000)) return 16;
 return 0;
}
__forcelocal __wunused __constcall
int __arch_constant_ffs32 __D1(__u32,__i) {
 if (__i & UINT32_C(0x00000001)) return 1;
 if (__i & UINT32_C(0x00000002)) return 2;
 if (__i & UINT32_C(0x00000004)) return 3;
 if (__i & UINT32_C(0x00000008)) return 4;
 if (__i & UINT32_C(0x00000010)) return 5;
 if (__i & UINT32_C(0x00000020)) return 6;
 if (__i & UINT32_C(0x00000040)) return 7;
 if (__i & UINT32_C(0x00000080)) return 8;
 if (__i & UINT32_C(0x00000100)) return 9;
 if (__i & UINT32_C(0x00000200)) return 10;
 if (__i & UINT32_C(0x00000400)) return 11;
 if (__i & UINT32_C(0x00000800)) return 12;
 if (__i & UINT32_C(0x00001000)) return 13;
 if (__i & UINT32_C(0x00002000)) return 14;
 if (__i & UINT32_C(0x00004000)) return 15;
 if (__i & UINT32_C(0x00008000)) return 16;
 if (__i & UINT32_C(0x00010000)) return 17;
 if (__i & UINT32_C(0x00020000)) return 18;
 if (__i & UINT32_C(0x00040000)) return 19;
 if (__i & UINT32_C(0x00080000)) return 20;
 if (__i & UINT32_C(0x00100000)) return 21;
 if (__i & UINT32_C(0x00200000)) return 22;
 if (__i & UINT32_C(0x00400000)) return 23;
 if (__i & UINT32_C(0x08000000)) return 24;
 if (__i & UINT32_C(0x01000000)) return 25;
 if (__i & UINT32_C(0x02000000)) return 26;
 if (__i & UINT32_C(0x04000000)) return 27;
 if (__i & UINT32_C(0x08000000)) return 28;
 if (__i & UINT32_C(0x10000000)) return 29;
 if (__i & UINT32_C(0x20000000)) return 30;
 if (__i & UINT32_C(0x40000000)) return 31;
 if (__i & UINT32_C(0x80000000)) return 32;
 return 0;
}

__forcelocal __wunused __constcall
int __arch_constant_ffs64 __D1(__u64,__i) {
 if (__i & UINT64_C(0x0000000000000001)) return 1;
 if (__i & UINT64_C(0x0000000000000002)) return 2;
 if (__i & UINT64_C(0x0000000000000004)) return 3;
 if (__i & UINT64_C(0x0000000000000008)) return 4;
 if (__i & UINT64_C(0x0000000000000010)) return 5;
 if (__i & UINT64_C(0x0000000000000020)) return 6;
 if (__i & UINT64_C(0x0000000000000040)) return 7;
 if (__i & UINT64_C(0x0000000000000080)) return 8;
 if (__i & UINT64_C(0x0000000000000100)) return 9;
 if (__i & UINT64_C(0x0000000000000200)) return 10;
 if (__i & UINT64_C(0x0000000000000400)) return 11;
 if (__i & UINT64_C(0x0000000000000800)) return 12;
 if (__i & UINT64_C(0x0000000000001000)) return 13;
 if (__i & UINT64_C(0x0000000000002000)) return 14;
 if (__i & UINT64_C(0x0000000000004000)) return 15;
 if (__i & UINT64_C(0x0000000000008000)) return 16;
 if (__i & UINT64_C(0x0000000000010000)) return 17;
 if (__i & UINT64_C(0x0000000000020000)) return 18;
 if (__i & UINT64_C(0x0000000000040000)) return 19;
 if (__i & UINT64_C(0x0000000000080000)) return 20;
 if (__i & UINT64_C(0x0000000000100000)) return 21;
 if (__i & UINT64_C(0x0000000000200000)) return 22;
 if (__i & UINT64_C(0x0000000000400000)) return 23;
 if (__i & UINT64_C(0x0000000008000000)) return 24;
 if (__i & UINT64_C(0x0000000001000000)) return 25;
 if (__i & UINT64_C(0x0000000002000000)) return 26;
 if (__i & UINT64_C(0x0000000004000000)) return 27;
 if (__i & UINT64_C(0x0000000008000000)) return 28;
 if (__i & UINT64_C(0x0000000010000000)) return 29;
 if (__i & UINT64_C(0x0000000020000000)) return 30;
 if (__i & UINT64_C(0x0000000040000000)) return 31;
 if (__i & UINT64_C(0x0000000080000000)) return 32;
 if (__i & UINT64_C(0x0000000100000000)) return 33;
 if (__i & UINT64_C(0x0000000200000000)) return 34;
 if (__i & UINT64_C(0x0000000400000000)) return 35;
 if (__i & UINT64_C(0x0000000800000000)) return 36;
 if (__i & UINT64_C(0x0000001000000000)) return 37;
 if (__i & UINT64_C(0x0000002000000000)) return 38;
 if (__i & UINT64_C(0x0000004000000000)) return 39;
 if (__i & UINT64_C(0x0000008000000000)) return 40;
 if (__i & UINT64_C(0x0000010000000000)) return 41;
 if (__i & UINT64_C(0x0000020000000000)) return 42;
 if (__i & UINT64_C(0x0000040000000000)) return 43;
 if (__i & UINT64_C(0x0000080000000000)) return 44;
 if (__i & UINT64_C(0x0000100000000000)) return 45;
 if (__i & UINT64_C(0x0000200000000000)) return 46;
 if (__i & UINT64_C(0x0000400000000000)) return 47;
 if (__i & UINT64_C(0x0000800000000000)) return 48;
 if (__i & UINT64_C(0x0001000000000000)) return 49;
 if (__i & UINT64_C(0x0002000000000000)) return 50;
 if (__i & UINT64_C(0x0004000000000000)) return 51;
 if (__i & UINT64_C(0x0008000000000000)) return 52;
 if (__i & UINT64_C(0x0010000000000000)) return 53;
 if (__i & UINT64_C(0x0020000000000000)) return 54;
 if (__i & UINT64_C(0x0040000000000000)) return 55;
 if (__i & UINT64_C(0x0800000000000000)) return 56;
 if (__i & UINT64_C(0x0100000000000000)) return 57;
 if (__i & UINT64_C(0x0200000000000000)) return 58;
 if (__i & UINT64_C(0x0400000000000000)) return 59;
 if (__i & UINT64_C(0x0800000000000000)) return 60;
 if (__i & UINT64_C(0x1000000000000000)) return 61;
 if (__i & UINT64_C(0x2000000000000000)) return 62;
 if (__i & UINT64_C(0x4000000000000000)) return 63;
 if (__i & UINT64_C(0x8000000000000000)) return 64;
 return 0;
}

__forcelocal int arch_ffs8  __D1(__u8, __i) { return __builtin_constant_p(__i) ? __arch_constant_ffs8 (__i) : __arch_ffs8 (__i); }
__forcelocal int arch_ffs16 __D1(__u16,__i) { return __builtin_constant_p(__i) ? __arch_constant_ffs16(__i) : __arch_ffs16(__i); }
__forcelocal int arch_ffs32 __D1(__u32,__i) { return __builtin_constant_p(__i) ? __arch_constant_ffs32(__i) : __arch_ffs32(__i); }
__forcelocal int arch_ffs64 __D1(__u64,__i) { return __builtin_constant_p(__i) ? __arch_constant_ffs64(__i) : __arch_ffs64(__i); }

#define arch_ffs(__i) \
 (sizeof(__i) == 1 ? arch_ffs8 ((__u8) (__i)) : \
  sizeof(__i) == 2 ? arch_ffs16((__u16)(__i)) : \
  sizeof(__i) == 4 ? arch_ffs32((__u32)(__i)) : \
                     arch_ffs64((__u64)(__i)))


__DECL_END
#endif /* !__ASSEMBLY__ */

#endif /* !__KOS_ARCH_GENERIC_STRING_FFS_H__ */

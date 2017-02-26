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
#ifndef __WINDOWNS_TYPES_H__
#define __WINDOWNS_TYPES_H__ 1

#include <kos/config.h>
#include <kos/compiler.h>
#include <kos/types.h>

__DECL_BEGIN

#ifdef _MSC_VER
#define UNALIGNED __unaligned
#else
#define UNALIGNED /* nothing */
#endif

/* NOTE: Always use fixed-length integral types to keep this consistent.
 *    >> Even if it says 'LONG', doesn't mean it'll equal sizeof(long)!
 */
typedef __u8  UCHAR,*PUCHAR,*LPUCHAR;
typedef __u16 USHORT,*PUSHORT,*LPUSHORT;
typedef __u32 ULONG,*PULONG,*LPULONG;
typedef __s32 LONG,*PLONG,*LPLONG;
typedef __s64 LONGLONG,*PLONGLONG,*LPLONGLONG;
typedef __u64 ULONGLONG,*PULONGLONG,*LPULONGLONG;

typedef __s32 BOOL,*PBOOL,*LPBOOL;

typedef __u8  BYTE,*PBYTE,*LPBYTE;
typedef __u16 WORD,*PWORD,*LPWORD;
typedef __u32 DWORD,*PDWORD,*LPDWORD;
typedef float FLOAT,*PFLOAT,*LPFLOAT;
typedef __s32 INT,*PINT,*LPINT;
typedef __u32 UINT,*PUINT,*LPUINT;
typedef void *LPVOID;
typedef void const *LPCVOID;

typedef __uintptr_t UINT_PTR;
#if __SIZEOF_LONG__ == __SIZEOF_POINTER__
typedef long        LONG_PTR;
#else
typedef __intptr_t  LONG_PTR;
#endif

typedef LPVOID HANDLE;


__DECL_END

#endif /* !__WINDOWNS_TYPES_H__ */

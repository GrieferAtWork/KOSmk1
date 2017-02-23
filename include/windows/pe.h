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
#ifndef __WINDOWNS_PE_H__
#define __WINDOWNS_PE_H__ 1

#include <kos/compiler.h>
#include <kos/types.h>
#include <windows/ms_pe.h>

__DECL_BEGIN

typedef IMAGE_DOS_HEADER   Pe32_DosHeader;
typedef IMAGE_DOS_HEADER   Pe64_DosHeader;
typedef IMAGE_NT_HEADERS32 Pe32_FileHeader;
typedef IMAGE_NT_HEADERS64 Pe64_FileHeader;

#define PE_FILE_SIGNATURE  0x00004550 /*< "PE\0\0" (In Pe*_FileHeader::Signature). */


__DECL_END

#endif /* !__WINDOWNS_PE_H__ */

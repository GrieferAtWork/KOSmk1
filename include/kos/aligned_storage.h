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
#ifndef __KOS_ALIGNED_STORAGE_H__
#define __KOS_ALIGNED_STORAGE_H__ 1

#include <kos/config.h>
#include <kos/compiler.h>
#include <kos/types.h>

__DECL_BEGIN

// Returns a type structure type suitable to hold 'count'
// element of 'T', properly aligned to 'alignment.'
// >> This is done through overallocating padded data
#define KOS_ALIGNED_STORAGE_PAD(T,count,alignment) \
 struct __packed { __u8 sp_align[alignment]; T sp_first; T sp_data[(count)-1]; }

#ifdef __COMPILER_HAVE_TYPEOF
// Returns a pointer to the aligned data of a pad-aligned storage
// NOTE: 'self' is only evaluated once
#define KOS_ALIGNED_STORAGE_PAD_GET(self) \
 ((__typeof__((self)->sp_first) *)(((__uintptr_t)&(self)->sp_first)&~(sizeof((self)->sp_align)-1)))
#else /* __COMPILER_HAVE_TYPEOF */
#define KOS_ALIGNED_STORAGE_PAD_GET(self) \
 ((void *)(((__uintptr_t)&(self)->sp_first)&~(sizeof((self)->sp_align)-1)))
#endif /* !__COMPILER_HAVE_TYPEOF */

__DECL_END

#endif /* !__KOS_ALIGNED_STORAGE_H__ */

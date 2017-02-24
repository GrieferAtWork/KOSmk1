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
#ifndef __KOS_KERNEL_LINKER_PE32_H__
#define __KOS_KERNEL_LINKER_PE32_H__ 1

#include <kos/config.h>
#include <kos/compiler.h>
#include <kos/errno.h>
#include <kos/types.h>
#include <windows/pe.h>

__DECL_BEGIN


//////////////////////////////////////////////////////////////////////////
// Load shared library data from a given file.
extern __crit __wunused __nonnull((1,2,4)) kerrno_t
ksecdata_pe32_init(struct ksecdata *__restrict self,
                   Pe32_SectionHeader const *__restrict headerv,
                   __size_t headerc, struct kfile *__restrict pe_file,
                   __uintptr_t image_base);


__DECL_END

#endif /* !__KOS_KERNEL_LINKER_PE32_H__ */

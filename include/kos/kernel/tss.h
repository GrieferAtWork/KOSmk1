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
#ifndef __KOS_KERNEL_TSS_H__
#define __KOS_KERNEL_TSS_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/compiler.h>
#include <kos/types.h>
#include <assert.h>

__DECL_BEGIN

#ifndef __ASSEMBLY__
__COMPILER_PACK_PUSH(1)

struct __packed ktss {
 __u16 link,__reserved1;
 __u32 esp0;
 __u16 ss0,__reserved2;
 __u32 esp1;
 __u16 ss1,__reserved3;
 __u32 esp2;
 __u16 ss2,__reserved4;
 __u32 cr3,eip,eflags;
 __u32 eax,ecx,edx,ebx;
 __u32 esp,ebp,esi,edi;
 __u16 es,__reserved5;
 __u16 cs,__reserved6;
 __u16 ss,__reserved7;
 __u16 ds,__reserved8;
 __u16 fs,__reserved9;
 __u16 gs,__reservedA;
 __u16 ldtr,__reservedB;
 __u16 __reservedC,iomap_base;
};

__COMPILER_PACK_POP
#endif /* !__ASSEMBLY__ */

__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_TSS_H__ */

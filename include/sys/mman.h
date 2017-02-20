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
#ifndef __SYS_MMAN_H__
#define __SYS_MMAN_H__ 1

#include <kos/compiler.h>
#include <kos/types.h>

__DECL_BEGIN

#define PROT_NONE     0x00 /*< Data cannot be accessed. */
#define PROT_EXEC     0x01 /*< [== KSHMREGION_FLAG_EXEC] Data can be executed. */
#define PROT_WRITE    0x02 /*< [== KSHMREGION_FLAG_WRITE] Data can be written. */
#define PROT_READ     0x04 /*< [== KSHMREGION_FLAG_READ] Data can be read. */

#define MAP_PRIVATE   0x00 /*< Changes are private. */
#define MAP_SHARED    0x01 /*< Changes are shared. */
#define MAP_FIXED     0x02 /*< Interpret addr exactly. */
#define MAP_ANONYMOUS 0x04 /*< Map anonymous memory. */
#define MAP_ANON      MAP_ANONYMOUS

/* KOS-mmap extensions. */
#define _MAP_LOOSE    0x10 /*< Don't keep the mapping after a fork(). */

#ifndef __CONFIG_MIN_LIBC__
#ifndef __KERNEL__
// These wouldn't work in the kernel, which uses physical addresses on a 1 to 1 mapping
extern void *mmap __P((void *__addr, size_t __length, int __prot,
                       int __flags, int __fd, __off_t __offset));
extern int munmap __P((void *__addr, size_t __length));

// KOS-extension: Request direct mmapping of physical memory
extern void *mmapdev __P((void *__addr, size_t __length, int __prot,
                          int __flags, __kernel void *__physical_address));
#endif /* !__KERNEL__ */
#endif /* !__CONFIG_MIN_LIBC__ */

__DECL_END

#endif /* !__SYS_MMAN_H__ */

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
#define PROT_EXEC     0x01 /*< Data can be executed (aka. 'KSHMREGION_FLAG_EXEC'). */
#define PROT_WRITE    0x02 /*< Data can be written (aka. 'KSHMREGION_FLAG_WRITE'). */
#define PROT_READ     0x04 /*< Data can be read (aka. 'KSHMREGION_FLAG_READ'). */

#define MAP_PRIVATE   0x00 /*< Changes are private. */
#define MAP_SHARED    0x01 /*< Changes are shared (aka. 'KSHMREGION_FLAG_SHARED'). */
#define MAP_FIXED     0x02 /*< Interpret addr exactly. */
#define MAP_ANONYMOUS 0x04 /*< Map anonymous memory. */
#define MAP_ANON      MAP_ANONYMOUS

/* KOS-mmap extensions. */
#define _MAP_LOOSE    0x10 /*< Don't keep the mapping after a fork() (aka. 'KSHMREGION_FLAG_LOSEONFORK'). */

#ifndef __STDC_PURE__
#define MAP_LOOSE     _MAP_LOOSE
#endif

#ifndef __KERNEL__
#ifndef __CONFIG_MIN_LIBC__

/* Map dynamic memory as specified by the given arguments.
 * NOTE: When non-NULL and 'MAP_FIXED' isn't set, KOS will interpret 'ADDR'
 *       as a hint for where to map the new memory in the virtual address
 *       space of the calling process, usually choosing the nearest unused
 *       chunk of virtual memory that is of sufficient size.
 *       When NULL is passed, KOS will instead choose a suitable builtin
 *       hint address to prefer when searching for available memory.
 * NOTE: KOS does not enforce any alignment of either 'ADDR' or 'LENGTH',
 *       both in calls to 'mmap', as well as 'munmap'. Instead, both
 *       arguments are rounded up/down to the nearest page borders.
 */
extern void *mmap __P((void *__addr, size_t __length, int __prot,
                       int __flags, int __fd, __off_t __offset));
extern int munmap __P((void *__addr, size_t __length));


/* KOS-extension: Request direct m-mapping of physical (aka. device) memory
 * NOTE: The kernel will imply the following flags:
 *   - KSHMREGION_FLAG_NOCOPY
 *   - KSHMREGION_FLAG_NOFREE
 * HINT: Consider specifying the 'MAP_LOOSE' if you don't
 *       want the memory to remain mapped in the child
 *       process after a call to 'fork()'.
 * NOTE: When attempting to map physical memory to a fixed address,
 *       both the fixed address 'ADDR' and 'PHYSICAL_ADDRESS' must
 *       share the same page-alignment offset. e.g.:
 *       Assuming PAGESIZE is 4096 (4k):
 *       ALLOWED:             ========---
 *         ADDR             = 0xBA6BAR123
 *         PHYSICAL_ADDRESS = 0xF00F00123
 *       ILLEGAL:             ========---
 *         ADDR             = 0xBA6BAR123
 *         PHYSICAL_ADDRESS = 0xF00F00321
 *       Or speaking in code:
 *         require(((uintptr_t)ADDR % PAGESIZE) == ((uintptr_t)PHYSICAL_ADDRESS % PAGESIZE));
 */
extern void *mmapdev __P((void *__addr, size_t __length, int __prot,
                          int __flags, __kernel void *__physical_address));

#endif /* !__CONFIG_MIN_LIBC__ */
#endif /* !__KERNEL__ */

__DECL_END

#endif /* !__SYS_MMAN_H__ */

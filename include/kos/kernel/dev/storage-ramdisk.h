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
#ifndef __KOS_KERNEL_DEV_STORAGE_RAMDISK_H__
#define __KOS_KERNEL_DEV_STORAGE_RAMDISK_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/compiler.h>
#include <kos/kernel/dev/storage.h>
#include <kos/kernel/types.h>

__DECL_BEGIN

typedef struct __krddev krddev_t; /*< Kernel Ram Disk Device. */
struct __krddev {
 KSDEV_HEAD
 void   *rd_begin; /*< [1..1] Ram disk start. */
 void   *rd_end;   /*< [1..1] Ram disk end. */
};

//////////////////////////////////////////////////////////////////////////
// Creates a new RAMDISK storage device.
// @param blocksize:  The size of a single block of memory
// @param blockcount: The total amount of blocks.
// @return: KE_NOMEM: Not enough memory to create this ramdisk.
extern __wunused __nonnull((1)) kerrno_t krddev_create(krddev_t *self, __size_t blocksize, __size_t blockcount);

__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_DEV_STORAGE_RAMDISK_H__ */

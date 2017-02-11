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
#ifndef __KOS_KERNEL_DEV_FSTYPE_H__
#define __KOS_KERNEL_DEV_FSTYPE_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/compiler.h>
#include <kos/kernel/types.h>

__DECL_BEGIN

typedef __u8 kfstype_t;

// More ids here:
// >> http://www.win.tue.nl/~aeb/partitions/partition_types-1.html
#define KFSTYPE_EMPTY    0x00

#define KFSTYPE_FAT12    0x01
#define KFSTYPE_FAT16ALT 0x04
#define KFSTYPE_FAT16    0x06
#define KFSTYPE_FAT32    0x0b


// Extended partition (these describe another MBR)
#define KFSTYPE_EXTPART1 0x05
#define KFSTYPE_EXTPART2 0x0F
#define KFSTYPE_ISEXTPART(sysid) \
 ((sysid) == KFSTYPE_EXTPART1 || (sysid) == KFSTYPE_EXTPART2)


__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_DEV_FSTYPE_H__ */

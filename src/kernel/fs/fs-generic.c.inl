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
#ifndef __KOS_KERNEL_FS_FS_GENERIC_C_INL__
#define __KOS_KERNEL_FS_FS_GENERIC_C_INL__ 1

#include <kos/compiler.h>
#include <kos/kernel/debug.h>
#include <kos/kernel/dev/device.h>
#include <kos/kernel/dev/storage.h>
#include <kos/errno.h>
#include <kos/kernel/fd.h>
#include <kos/kernel/fs/file.h>
#include <kos/kernel/fs/fs.h>
#include <kos/kernel/proc.h>
#include <kos/kernel/time.h>

__DECL_BEGIN

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// --- KFILE
kerrno_t
kfile_generic_ioctl(struct kfile *__restrict self,
                    kattr_t cmd,
                    __user void *arg) {
 (void)self,(void)cmd,(void)arg;
 kassert_kfile(self);
 return KE_NOSYS;
}

__DECL_END

#ifndef __INTELLISENSE__
#define GETATTR
#include "fs-generic-inode.c.inl"
#include "fs-generic-inode.c.inl"
#define GETATTR
#include "fs-generic-file.c.inl"
#include "fs-generic-file.c.inl"
#define GETATTR
#include "fs-generic-superblock.c.inl"
#include "fs-generic-superblock.c.inl"
#endif


#endif /* !__KOS_KERNEL_FS_FS_GENERIC_C_INL__ */

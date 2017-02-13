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
#ifndef __KOS_KERNEL_FS_VFS_PROC_H__
#define __KOS_KERNEL_FS_VFS_PROC_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/kernel/fs/fs.h>
#include <kos/kernel/fs/vfs.h>

__DECL_BEGIN

/* "/proc": The /proc filesystem superblock. */
extern struct kvsdirsuperblock kvfs_proc;
extern struct ksuperblocktype kvfsproc_type;


/* "/proc/self": A symbolic link expanding to the pid of the calling process. */
extern struct kinode kvinode_proc_self;
extern struct kinodetype kvinodetype_proc_self;


__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_FS_VFS_PROC_H__ */

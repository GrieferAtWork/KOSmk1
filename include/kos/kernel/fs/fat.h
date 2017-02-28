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
#ifndef __KOS_KERNEL_FS_FAT_H__
#define __KOS_KERNEL_FS_FAT_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/compiler.h>
#include <kos/kernel/fs/fs.h>
#include <kos/kernel/fs/fat-internal.h>
#include <kos/kernel/fs/fs-blockfile.h>
#include <kos/kernel/types.h>
#include <kos/kernel/dev/fstype.h>

__DECL_BEGIN

struct kfatinode {
 // NOTE: We use the ino_t as the file's cluster address
 struct kinode         fi_inode; /*< Underlying INode. */
 struct kfatfilepos    fi_pos;   /*< On-disk locations of file data. */
 struct kfatfileheader fi_file;  /*< On-disk header of the file represented by this inode. */
};

struct kfatsuperblock {
 KSUPERBLOCK_TYPE(struct kfatinode)
 struct kfatfs f_fs; /*< The FAT filesystem itself. */
};
#define kfatsuperblock_fromfatfs(fs)\
 ((struct kfatsuperblock *)(((__uintptr_t)(fs))-offsetof(struct kfatsuperblock,f_fs)))

struct kfatfile {
 struct kblockfile ff_blockfile; /*< Underlying block file. */
};


extern __wunused __nonnull((1,2,3)) __ref struct kfatinode *
kfatinode_new(struct kfatsuperblock *__restrict superblock,
              struct kfatfilepos const *__restrict fpos,
              struct kfatfileheader const *__restrict fheader);

extern struct ksuperblocktype kfatsuperblock_type;
extern struct kinodetype      kfatinode_file_type;
extern struct kinodetype      kfatinode_dir_type;
extern struct kblockfiletype  kfatfile_file_type;

extern __wunused __nonnull((1,2)) kerrno_t
kfatsuperblock_init(struct kfatsuperblock *__restrict self,
                    struct ksdev *__restrict dev);

__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_FS_FAT_H__ */

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
#ifndef __KOS_KERNEL_PIPE_H__
#define __KOS_KERNEL_PIPE_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#ifndef __ASSEMBLY__
#include <kos/types.h>
#include <kos/kernel/features.h>
#include <kos/kernel/object.h>
#include <kos/kernel/iobuf.h>
#include <kos/kernel/fs/fs.h>
#include <kos/kernel/fs/file.h>
#ifndef __INTELLISENSE__
#include <malloc.h>
#endif /* !__INTELLISENSE__ */

__DECL_BEGIN

struct kpipe {
 struct kinode p_node;  /*< Underlying INode. */
 struct kiobuf p_iobuf; /*< I/O buffer. */
};

/* Symbolic superblock for the pipe filesystem. */
extern struct ksuperblock kpipe_fs;
extern struct kinodetype kpipe_type;

//////////////////////////////////////////////////////////////////////////
// Allocates a new pipe with the given max_size
// @return: * :   A reference to a newly allocated pipe.
// @return: NULL: Not enough available memory.
extern __crit __ref struct kpipe *kpipe_new(__size_t max_size);

//////////////////////////////////////////////////////////////////////////
// Insert the given pipe into the filesystem
#define kpipe_insnod(self,path,pathmax,dent) \
 kuserfs_insnod(path,pathmax,(struct kinode *)(self),dent,KDIRENT_FLAG_VIRT)


struct kinode;
struct kdirent;
struct kpipefile {
 struct kfile        pr_file;   /*< Underlying file. */
 __ref struct kpipe *pr_pipe;   /*< [1..1] Associated pipe. */
 struct kdirent     *pr_dirent; /*< [0..1] Associated directory entry. */
};

//////////////////////////////////////////////////////////////////////////
// Create and return a new reader/writer for a given pipe.
// @return: * :   A reference to a newly allocated file.
// @return: NULL: Not enough available memory.
extern __crit __ref struct kpipefile *kpipefile_newreader(struct kpipe *__restrict pipe, struct kdirent *dent);
extern __crit __ref struct kpipefile *kpipefile_newwriter(struct kpipe *__restrict pipe, struct kdirent *dent);

/* File types for pipe readers/writers */
extern struct kfiletype kpipesuper_type;
extern struct kfiletype kpipereader_type;
extern struct kfiletype kpipewriter_type;

__DECL_END

#endif /* !__ASSEMBLY__ */
#endif /* !__KERNEL__ */

#endif /* !__KOS_KERNEL_PIPE_H__ */

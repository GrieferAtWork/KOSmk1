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

#define KOBJECT_MAGIC_PIPE 0x919E /*< PIPE. */
#define kassert_kpipe(self) kassert_refobject(self,p_refcnt,KOBJECT_MAGIC_PIPE)

// TODO: Named pipes

struct kpipe {
 KOBJECT_HEAD // Pipe (basically just a reference-counted I/O buffer)
 __atomic __u32 p_refcnt; /*< Reference counter. */
 struct kiobuf  p_iobuf;  /*< I/O buffer. */
};

#define KPIPE_INIT               {KOBJECT_INIT(KOBJECT_MAGIC_PIPE) 0xffff,KIOBUF_INIT}
#define KPIPE_INIT_EX(max_size)  {KOBJECT_INIT(KOBJECT_MAGIC_PIPE) 0xffff,KIOBUF_INIT_EX(max_size)}

__local KOBJECT_DEFINE_INCREF(kpipe_incref,struct kpipe,p_refcnt,kassert_kpipe);
__local KOBJECT_DEFINE_DECREF(kpipe_decref,struct kpipe,p_refcnt,kassert_kpipe,kpipe_destroy);

//////////////////////////////////////////////////////////////////////////
// Allocates a new pipe with the given max_size
// @return: * :   A reference to a newly allocated pipe.
// @return: NULL: Not enough available memory.
extern __crit __ref struct kpipe *kpipe_new(__size_t max_size);


struct kpipefile {
 struct kfile        pr_file; /*< Underlying file. */
 __ref struct kpipe *pr_pipe; /*< [1..1] Associated pipe. */
};

//////////////////////////////////////////////////////////////////////////
// Create and return a new reader/writer for a given pipe.
// @return: * :   A reference to a newly allocated file.
// @return: NULL: Not enough available memory.
extern __crit __ref struct kpipefile *kpipefile_newreader(struct kpipe *__restrict pipe);
extern __crit __ref struct kpipefile *kpipefile_newwriter(struct kpipe *__restrict pipe);

// File types for pipe readers/writers
extern struct kfiletype kpipereader_type;
extern struct kfiletype kpipewriter_type;

__DECL_END

#endif /* !__ASSEMBLY__ */
#endif /* !__KERNEL__ */

#endif /* !__KOS_KERNEL_PIPE_H__ */

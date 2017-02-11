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
#ifndef __KOS_FS_H__
#define __KOS_FS_H__ 1

#include <kos/config.h>

#ifndef __NO_PROTOTYPES
#include <kos/compiler.h>
#include <kos/syscall.h>
#include <kos/types.h>
#include <kos/errno.h>

__DECL_BEGIN

// TODO: DOC
__local _syscall4(kerrno_t,kfs_mkdir,int,dirfd,char const *,path,__size_t,pathmax,__mode_t,mode);
__local _syscall3(kerrno_t,kfs_rmdir,int,dirfd,char const *,path,__size_t,pathmax);
__local _syscall3(kerrno_t,kfs_unlink,int,dirfd,char const *,path,__size_t,pathmax);
__local _syscall3(kerrno_t,kfs_remove,int,dirfd,char const *,path,__size_t,pathmax);
__local _syscall5(kerrno_t,kfs_symlink,int,dirfd,char const *,target,__size_t,targetmax,char const *,lnk,__size_t,lnkmax);
__local _syscall5(kerrno_t,kfs_hrdlink,int,dirfd,char const *,target,__size_t,targetmax,char const *,lnk,__size_t,lnkmax);

__DECL_END
#endif

#endif /* !__KOS_FS_H__ */

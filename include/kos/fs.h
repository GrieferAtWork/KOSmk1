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

//////////////////////////////////////////////////////////////////////////
// Create a new on-disk directory/link as specified by the given 'path'.
// @param: dirfd:    The directory to use as CWD in relative paths (Usually set to KFD_CWD).
// @param: mode:     Permissions model to use for the newly created directory.
// @param: path|lnk: The path to the directory/link to-be created.
// @param: *max:     The strnlen-style maximum string length of 'path|link'.
// @param: target:   The target text of the link to-be created.
// @return: KE_OK:        The directory/link was created successfully.
// @return: KE_ACCES:     The calling process does not have write permissions to some part of the directory chain.
// @return: KE_NOCWD:     The KFD_CWD descriptor of the calling process does not refer to a valid directory.
// @return: KE_NOROOT:    The KFD_ROOT descriptor of the calling process does not refer to a valid directory.
// @return: KE_NOSYS:     An requested operation is not supported by the underlying filesystem.
// @return: KE_DESTROYED: A part of the given path was recently deleted and has become inaccessible.
// @return: KE_NOENT:     A part of the given path does not exist.
// @return: KE_NODIR:     The given path tried to dereference a node that isn't a directory.
// @return: KE_DEVICE:    An error in the underlying disk-system prevent the operation from succeeding.
// @return: KE_EXISTS:    An entity with the given name already exists.
// @return: KE_FAULT:     A faulty pointer or area of memory was provided.
// @return: KE_ISERR(*):  Some other filesystem/hardware-specific error has occurred.
__local _syscall4(kerrno_t,kfs_mkdir,int,dirfd,char const *,path,__size_t,pathmax,__mode_t,mode);
__local _syscall5(kerrno_t,kfs_symlink,int,dirfd,char const *,target,__size_t,targetmax,char const *,lnk,__size_t,lnkmax);
__local _syscall5(kerrno_t,kfs_hrdlink,int,dirfd,char const *,target,__size_t,targetmax,char const *,lnk,__size_t,lnkmax);

//////////////////////////////////////////////////////////////////////////
// Remove a file/directory/either at a given path.
// @param: dirfd:   The directory to use as CWD in relative paths (Usually set to KFD_CWD).
// @param: path:    The given path.
// @param: pathmax: The strnlen-style maximum string length of 'path'.
// @return: KE_OK:        A filesystem entity was successfully removed.
// @return: KE_ACCES:     The calling process does not have write permissions to some part of the directory chain.
// @return: KE_NOCWD:     The KFD_CWD descriptor of the calling process does not refer to a valid directory.
// @return: KE_NOROOT:    The KFD_ROOT descriptor of the calling process does not refer to a valid directory.
// @return: KE_NOSYS:     An requested operation is not supported by the underlying filesystem.
// @return: KE_DESTROYED: A part of the given path was recently deleted and has become inaccessible.
// @return: KE_NOENT:     A part of the given path does not exist.
// @return: KE_NODIR:     The given path tried to dereference a node that isn't a directory.
// @return: KE_DEVICE:    An error in the underlying disk-system prevent the operation from succeeding.
// @return: KE_ISDIR:     [kfs_unlink] The given path is a directory or the root of a superblock.
// @return: KE_NODIR:     [kfs_rmdir] The given path is not a directory or the root of a superblock.
// @return: KE_ISERR(*):  Some other filesystem/hardware-specific error has occurred.
__local _syscall3(kerrno_t,kfs_rmdir,int,dirfd,char const *,path,__size_t,pathmax);
__local _syscall3(kerrno_t,kfs_unlink,int,dirfd,char const *,path,__size_t,pathmax);
__local _syscall3(kerrno_t,kfs_remove,int,dirfd,char const *,path,__size_t,pathmax);

__DECL_END
#endif

#endif /* !__KOS_FS_H__ */

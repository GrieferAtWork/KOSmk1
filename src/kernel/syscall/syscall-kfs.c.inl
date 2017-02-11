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
#ifndef __KOS_KERNEL_SYSCALL_SYSCALL_KFS_C_INL__
#define __KOS_KERNEL_SYSCALL_SYSCALL_KFS_C_INL__ 1

#include "syscall-common.h"
#include <kos/kernel/fd.h>
#include <kos/kernel/fs/file.h>
#include <kos/kernel/fs/fs.h>
#include <kos/kernel/task.h>
#include <kos/kernel/proc.h>
#include <kos/errno.h>
#include <kos/syscallno.h>
#include <stddef.h>
#include <sys/stat.h>
#include <string.h>

__DECL_BEGIN



/*< _syscall4(kerrno_t,kfs_mkdir,int,dirfd,char const *,path,size_t,pathmax,mode_t,mode); */
SYSCALL(sys_kfs_mkdir) {
 LOAD4(int         ,K(dirfd),
       char const *,U(path),
       size_t      ,K(pathmax),
       mode_t      ,K(mode));
 struct kproc *ctx = kproc_self();
 kerrno_t error; struct kfspathenv env;
 env.env_cwd = kproc_getfddirent(ctx,dirfd);
 if __unlikely(!env.env_cwd) RETURN(KE_NOCWD);
 env.env_root = kproc_getfddirent(ctx,KFD_ROOT);
 if __unlikely(!env.env_root) { error = KE_NOROOT; goto err_cwd; }
 env.env_flags = 0;
 env.env_uid   = kproc_uid(ctx);
 env.env_gid   = kproc_gid(ctx);
 kfspathenv_initcommon(&env);
 error = kdirent_mkdirat(&env,path,pathmax,mode,NULL,NULL);
 kdirent_decref(env.env_root);
err_cwd:
 kdirent_decref(env.env_cwd);
 RETURN(error);
}

/*< _syscall3(kerrno_t,kfs_rmdir,int,dirfd,char const *,path,size_t,pathmax); */
SYSCALL(sys_kfs_rmdir) {
 LOAD3(int         ,K(dirfd),
       char const *,U(path),
       size_t      ,K(pathmax));
 struct kproc *ctx = kproc_self();
 kerrno_t error; struct kfspathenv env;
 env.env_cwd = kproc_getfddirent(ctx,dirfd);
 if __unlikely(!env.env_cwd) RETURN(KE_NOCWD);
 env.env_root = kproc_getfddirent(ctx,KFD_ROOT);
 if __unlikely(!env.env_root) { error = KE_NOROOT; goto err_cwd; }
 env.env_flags = 0;
 env.env_uid   = kproc_uid(ctx);
 env.env_gid   = kproc_gid(ctx);
 kfspathenv_initcommon(&env);
 error = kdirent_rmdirat(&env,path,pathmax);
 kdirent_decref(env.env_root);
err_cwd:
 kdirent_decref(env.env_cwd);
 RETURN(error);
}

/*< _syscall3(kerrno_t,kfs_unlink,int,dirfd,char const *,path,size_t,pathmax); */
SYSCALL(sys_kfs_unlink) {
 LOAD3(int         ,K(dirfd),
       char const *,U(path),
       size_t      ,K(pathmax));
 struct kproc *ctx = kproc_self();
 kerrno_t error; struct kfspathenv env;
 env.env_cwd = kproc_getfddirent(ctx,dirfd);
 if __unlikely(!env.env_cwd) RETURN(KE_NOCWD);
 env.env_root = kproc_getfddirent(ctx,KFD_ROOT);
 if __unlikely(!env.env_root) { error = KE_NOROOT; goto err_cwd; }
 env.env_flags = 0;
 env.env_uid   = kproc_uid(ctx);
 env.env_gid   = kproc_gid(ctx);
 kfspathenv_initcommon(&env);
 error = kdirent_unlinkat(&env,path,pathmax);
 kdirent_decref(env.env_root);
err_cwd:
 kdirent_decref(env.env_cwd);
 RETURN(error);
}

/*< _syscall3(kerrno_t,kfs_remove,int,dirfd,char const *,path,size_t,pathmax); */
SYSCALL(sys_kfs_remove) {
 LOAD3(int         ,K(dirfd),
       char const *,U(path),
       size_t      ,K(pathmax));
 struct kproc *ctx = kproc_self();
 kerrno_t error; struct kfspathenv env;
 env.env_cwd = kproc_getfddirent(ctx,dirfd);
 if __unlikely(!env.env_cwd) RETURN(KE_NOCWD);
 env.env_root = kproc_getfddirent(ctx,KFD_ROOT);
 if __unlikely(!env.env_root) { error = KE_NOROOT; goto err_cwd; }
 env.env_flags = 0;
 env.env_uid   = kproc_uid(ctx);
 env.env_gid   = kproc_gid(ctx);
 kfspathenv_initcommon(&env);
 error = kdirent_removeat(&env,path,pathmax);
 kdirent_decref(env.env_root);
err_cwd:
 kdirent_decref(env.env_cwd);
 RETURN(error);
}

/*< _syscall5(kerrno_t,kfs_symlink,int,dirfd,char const *,target,size_t,targetmax,char const *,lnk,size_t,lnkmax); */
SYSCALL(sys_kfs_symlink) {
 LOAD5(int         ,K(dirfd),
       char const *,U(target),
       size_t      ,K(targetmax),
       char const *,U(lnk),
       size_t      ,K(lnkmax));
 struct kdirentname targetname;
 struct kproc *ctx = kproc_self();
 kerrno_t error; struct kfspathenv env;
 env.env_cwd = kproc_getfddirent(ctx,dirfd);
 if __unlikely(!env.env_cwd) RETURN(KE_NOCWD);
 env.env_root = kproc_getfddirent(ctx,KFD_ROOT);
 if __unlikely(!env.env_root) { error = KE_NOROOT; goto err_cwd; }
 env.env_flags = 0;
 env.env_uid   = kproc_uid(ctx);
 env.env_gid   = kproc_gid(ctx);
 kfspathenv_initcommon(&env);
 targetname.dn_name = (char *)target;
 targetname.dn_size = strnlen(target,targetmax);
 kdirentname_refreshhash(&targetname);
 error = kdirent_mklnkat(&env,lnk,lnkmax,&targetname,NULL,NULL);
 kdirent_decref(env.env_root);
err_cwd:
 kdirent_decref(env.env_cwd);
 RETURN(error);
}

/*< _syscall5(kerrno_t,kfs_hrdlink,int,dirfd,char const *,target,size_t,targetmax,char const *,lnk,size_t,lnkmax); */
SYSCALL(sys_kfs_hrdlink) {
 LOAD5(int         ,K(dirfd),
       char const *,U(target),
       size_t      ,K(targetmax),
       char const *,U(lnk),
       size_t      ,K(lnkmax));
 (void)dirfd,(void)target,(void)targetmax,(void)lnk,(void)lnkmax;
 RETURN(KE_NOSYS); // TODO
}

__DECL_END

#endif /* !__KOS_KERNEL_SYSCALL_SYSCALL_KFS_C_INL__ */

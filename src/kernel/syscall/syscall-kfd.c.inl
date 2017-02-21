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
#ifndef __KOS_KERNEL_SYSCALL_SYSCALL_KFD_C_INL__
#define __KOS_KERNEL_SYSCALL_SYSCALL_KFD_C_INL__ 1

#include "syscall-common.h"
#include <kos/kernel/fs/file.h>
#include <kos/kernel/fs/fs.h>
#include <kos/kernel/proc.h>
#include <kos/kernel/pipe.h>
#include <kos/kernel/pty.h>
#include <kos/kernel/util/string.h>
#include <kos/errno.h>
#include <kos/syscallno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <kos/attr.h>
#include <kos/fd.h>
#include <pty.h>

__DECL_BEGIN

/*< _syscall5(kerrno_t,kfd_open,int,cwd,char const *,path,size_t,maxpath,openmode_t,mode,mode_t,perms); */
SYSCALL(sys_kfd_open) {
 LOAD5(int         ,K(cwd),
       char const *,U(path),
       size_t      ,K(maxpath),
       openmode_t  ,K(mode),
       mode_t      ,K(perms));
 struct kproc *ctx = kproc_self();
 struct kfspathenv env; int result;
 struct kfdentry entry; kerrno_t error;
 KTASK_CRIT_BEGIN_FIRST
 env.env_cwd = kproc_getfddirent(ctx,cwd);
 if __unlikely(!env.env_cwd) { error = KE_NOCWD; goto end; }
 env.env_root = kproc_getfddirent(ctx,KFD_ROOT);
 if __unlikely(!env.env_root) { kdirent_decref(env.env_cwd); error = KE_NOROOT; goto end; }
 env.env_flags = mode;
 env.env_uid   = kproc_getuid(ctx);
 env.env_gid   = kproc_getgid(ctx);
 kfspathenv_initcommon(&env);
 if (env.env_flags&O_CREAT) {
  union kinodeattr attr[3];
  attr[0].ia_owner.a_id    = KATTR_FS_OWNER;
  attr[0].ia_owner.o_owner = env.env_uid;
  attr[1].ia_group.a_id    = KATTR_FS_GROUP;
  attr[1].ia_group.g_group = env.env_gid;
  attr[2].ia_perm.a_id     = KATTR_FS_PERM;
  attr[2].ia_perm.p_perm   = perms;
  error = kdirent_openat(&env,path,maxpath,env.env_flags,
                         __compiler_ARRAYSIZE(attr),attr,&entry.fd_file);
 } else {
  error = kdirent_openat(&env,path,maxpath,env.env_flags,0,NULL,&entry.fd_file);
 }
 kdirent_decref(env.env_root);
 kdirent_decref(env.env_cwd);
 if __unlikely(KE_ISERR(error)) goto end;
 kassert_kfile(entry.fd_file);
 entry.fd_attr = KFD_ATTR(KFDTYPE_FILE,(mode&O_CLOEXEC) ? KFD_FLAG_CLOEXEC : KFD_FLAG_NONE);
 assert(entry.fd_type == KFDTYPE_FILE);
 assert(entry.fd_flag == KFD_FLAG_NONE);
 error = kproc_insfd_inherited(ctx,&result,&entry);
 if __likely(KE_ISOK(error)) error = result;
 else kfile_decref(entry.fd_file);
end:
 KTASK_CRIT_END
 RETURN(error);
}

/*< _syscall6(kerrno_t,kfd_open2,int,dfd,int,cwd,char const *,path,size_t,maxpath,openmode_t,mode,mode_t,perms); */
SYSCALL(sys_kfd_open2) {
 LOAD6(int         ,K(dfd),
       int         ,K(cwd),
       char const *,U(path),
       size_t      ,K(maxpath),
       openmode_t  ,K(mode),
       mode_t      ,K(perms));
 struct kproc *ctx = kproc_self();
 struct kfspathenv env;
 struct kfdentry entry; kerrno_t error;
 KTASK_CRIT_BEGIN_FIRST
 env.env_cwd = kproc_getfddirent(ctx,cwd);
 if __unlikely(!env.env_cwd) { error = KE_NOCWD; goto end; }
 env.env_root = kproc_getfddirent(ctx,KFD_ROOT);
 if __unlikely(!env.env_root) { kdirent_decref(env.env_cwd); error = KE_NOROOT; goto end; }
 env.env_flags = mode;
 env.env_uid   = kproc_getuid(ctx);
 env.env_gid   = kproc_getgid(ctx);
 kfspathenv_initcommon(&env);
 if (env.env_flags&O_CREAT) {
  union kinodeattr attr[3];
  attr[0].ia_owner.a_id    = KATTR_FS_OWNER;
  attr[0].ia_owner.o_owner = env.env_uid;
  attr[1].ia_group.a_id    = KATTR_FS_GROUP;
  attr[1].ia_group.g_group = env.env_gid;
  attr[2].ia_perm.a_id     = KATTR_FS_PERM;
  attr[2].ia_perm.p_perm   = perms;
  error = kdirent_openat(&env,path,maxpath,env.env_flags,
                         __compiler_ARRAYSIZE(attr),attr,&entry.fd_file);
 } else {
  error = kdirent_openat(&env,path,maxpath,env.env_flags,0,NULL,&entry.fd_file);
 }
 kdirent_decref(env.env_root);
 kdirent_decref(env.env_cwd);
 if __unlikely(KE_ISERR(error)) goto end;
 kassert_kfile(entry.fd_file);
 entry.fd_attr = KFD_ATTR(KFDTYPE_FILE,(mode&O_CLOEXEC) ? KFD_FLAG_CLOEXEC : KFD_FLAG_NONE);
 error = kproc_insfdat_inherited(ctx,dfd,&entry);
 if __likely(KE_ISOK(error)) error = dfd;
 else kfile_decref(entry.fd_file);
end:
 KTASK_CRIT_END
 RETURN(error);
}

/*< _syscall1(kerrno_t,kfd_close,int,fd); */
SYSCALL(sys_kfd_close) {
 LOAD1(int,K(fd));
 RETURN(KTASK_CRIT(kproc_closefd(kproc_self(),fd)));
}

/*< _syscall2(kerrno_t,kfd_close,int,fd); */
SYSCALL(sys_kfd_closeall) {
 LOAD2(int,K(low),
       int,K(high));
 RETURN(KTASK_CRIT(kproc_closeall(kproc_self(),low,high)));
}

/*< _syscall{4|5}(kerrno_t,kfd_seek,int,fd,{__s64,off|__s32,offhi,__s32,offlo},int,whence,__u64 *,newpos); */
SYSCALL(sys_kfd_seek) {
#ifdef KFD_HAVE_BIARG_POSITIONS
 LOAD5(int    ,K (fd),
       __s64  ,D (off),void,N,
       int    ,K (whence),
       __u64 *,U0(newpos));
#else
 LOAD4(int    ,K (fd),
       __s64  ,K (off),
       int    ,K (whence),
       __u64 *,U0(newpos));
#endif
 RETURN(KTASK_CRIT(kproc_seekfd(kproc_self(),fd,off,whence,newpos)));
}

/*< _syscall4(kerrno_t,kfd_read,int,fd,void *,buf,size_t,bufsize,size_t *,rsize); */
SYSCALL(sys_kfd_read) {
 LOAD4(int     ,K(fd),
       void   *,U(buf),
       size_t  ,K(bufsize),
       size_t *,U(rsize));
 kerrno_t error;
 KTASK_CRIT_BEGIN_FIRST
 error = kproc_kernel_readfd(kproc_self(),fd,buf,bufsize,rsize);
 KTASK_CRIT_END
 RETURN(error);
}

/*< _syscall4(kerrno_t,kfd_write,int,fd,void const *,buf,size_t,bufsize,size_t *,wsize); */
SYSCALL(sys_kfd_write) {
 LOAD4(int         ,K(fd),
       void const *,U(buf),
       size_t      ,K(bufsize),
       size_t     *,U(wsize));
 RETURN(KTASK_CRIT(kproc_kernel_writefd(kproc_self(),fd,buf,bufsize,wsize)));
}

/*< _syscall{5|6}(kerrno_t,kfd_pread,int,fd,{__u64,pos|__u32,poshi,__u32,poslo},void *,buf,size_t,bufsize,size_t *,rsize); */
SYSCALL(sys_kfd_pread) {
#ifdef KFD_HAVE_BIARG_POSITIONS
 LOAD6(int     ,K(fd),
       __u64   ,D(pos),void,N,
       void   *,U(buf),
       size_t  ,K(bufsize),
       size_t *,U(rsize));
#else
 LOAD5(int     ,K(fd),
       __u64   ,K(pos),
       void   *,U(buf),
       size_t  ,K(bufsize),
       size_t *,U(rsize));
#endif
 RETURN(KTASK_CRIT(kproc_kernel_preadfd(kproc_self(),fd,pos,buf,bufsize,rsize)));
}

/*< _syscall{5|6}(kerrno_t,kfd_pwrite,int,fd,{__u64,pos|__u32,poshi,__u32,poslo},void const *,buf,size_t,bufsize,size_t *,wsize); */
SYSCALL(sys_kfd_pwrite) {
#ifdef KFD_HAVE_BIARG_POSITIONS
 LOAD6(int         ,K(fd),
       __u64       ,D(pos),void,N,
       void const *,U(buf),
       size_t      ,K(bufsize),
       size_t     *,U(wsize));
#else
 LOAD5(int         ,K(fd),
       __u64       ,K(pos),
       void const *,U(buf),
       size_t      ,K(bufsize),
       size_t     *,U(wsize));
#endif
 RETURN(KTASK_CRIT(kproc_kernel_pwritefd(kproc_self(),fd,pos,buf,bufsize,wsize)));
}

/*< _syscall1(kerrno_t,kfd_flush,int,fd); */
SYSCALL(sys_kfd_flush) {
 LOAD1(int,K(fd));
 RETURN(KTASK_CRIT(kproc_flushfd(kproc_self(),fd)));
}

/*< _syscall{2|3}(kerrno_t,kfd_trunc,int,fd,{__u64,size|__u32,sizehi,__u32,sizelo}); */
SYSCALL(sys_kfd_trunc) {
#ifdef KFD_HAVE_BIARG_POSITIONS
 LOAD3(int     ,K(fd),
       __u64   ,D(size),void,N);
#else
 LOAD2(int     ,K(fd),
       __u64   ,K(size));
#endif
 RETURN(KTASK_CRIT(kproc_truncfd(kproc_self(),fd,size)));
}

/*< _syscall3(kerrno_t,kfd_fcntl,int,fd,int,cmd,void *,arg); */
SYSCALL(sys_kfd_fcntl) {
 LOAD3(int   ,K(fd),
       int   ,K(cmd),
       void *,K(arg));
 RETURN(KTASK_CRIT(kproc_user_fcntlfd(kproc_self(),fd,cmd,arg)));
}

/*< _syscall3(kerrno_t,kfd_ioctl,int,fd,kattr_t,attr,void *,cmd); */
SYSCALL(sys_kfd_ioctl) {
 LOAD3(int    ,K(fd),
       kattr_t,K(cmd),
       void  *,K(arg));
 RETURN(KTASK_CRIT(kproc_user_ioctlfd(kproc_self(),fd,cmd,arg)));
}

/*< _syscall5(kerrno_t,kfd_getattr,int,fd,kattr_t,attr,void *,buf,size_t,bufsize,size_t *,reqsize); */
SYSCALL(sys_kfd_getattr) {
 LOAD5(int     ,K (fd),
       kattr_t ,K (attr),
       void   *,U (buf),
       size_t  ,K (bufsize),
       size_t *,U0(reqsize));
 RETURN(KTASK_CRIT(kproc_kernel_getattrfd(kproc_self(),fd,attr,buf,bufsize,reqsize)));
}

/*< _syscall4(kerrno_t,kfd_setattr,int,fd,kattr_t,attr,void const *,buf,size_t,bufsize); */
SYSCALL(sys_kfd_setattr) {
 LOAD4(int     ,K(fd),
       kattr_t ,K(attr),
       void   *,U(buf),
       size_t  ,K(bufsize));
 RETURN(KTASK_CRIT(kproc_kernel_setattrfd(kproc_self(),fd,attr,buf,bufsize)));
}

/*< _syscall3(kerrno_t,kfd_readdir,int,fd,struct kfddirent *,dent,__u32,flags); */
SYSCALL(sys_kfd_readdir) {
 LOAD3(int               ,K(fd),
       struct kfddirent *,U(dent),
        __u32            ,K(flags));
 struct kinode *inode; struct kdirentname *name;
 kerrno_t error; struct kfdentry entry;
 char *userbuffer = TRANSLATE(dent->kd_namev);
 if __unlikely(!userbuffer) dent->kd_namec = 0;
 KTASK_CRIT_BEGIN_FIRST
 error = kproc_getfd(kproc_self(),fd,&entry);
 if __unlikely(KE_ISERR(error)) goto end;
 if (entry.fd_type != KFDTYPE_FILE) error = KE_NOFILE;
 else if __likely((error = kfile_readdir(entry.fd_file,&inode,&name,
                                         flags&KFD_READDIR_MASKINTERNAL)
                   ) == KE_OK) {
  union kinodeattr  attr[2];
  union kinodeattr *iter = attr;
  size_t reqsize;
  kassert_kinode(inode);
  kassertobj(name);
  reqsize = (name->dn_size+1)*sizeof(char);
  memcpy(userbuffer,name->dn_name,min(reqsize,dent->kd_namec));
  if (dent->kd_namec >= reqsize && (flags&KFD_READDIR_FLAG_PERFECT)) {
   // Advance the directory stream by one if the buffer was sufficient
   error = kfile_seek(entry.fd_file,1,SEEK_CUR,NULL);
   if __unlikely(KE_ISERR(error)) goto end_inode;
  }
  dent->kd_namec = reqsize;
  // Fill in additional information if requested to
  if (flags&KFD_READDIR_FLAG_INO)   (*iter++).ia_common.a_id = KATTR_FS_INO;
  if (flags&KFD_READDIR_FLAG_PERM)  (*iter++).ia_common.a_id = KATTR_FS_PERM;
  if (iter != attr) {
   error = kinode_getattr(inode,(size_t)(iter-attr),attr);
   if __likely(KE_ISOK(error)) {
    iter = attr;
    if (flags&KFD_READDIR_FLAG_INO) dent->kd_ino = (*iter++).ia_ino.i_ino;
    if (flags&KFD_READDIR_FLAG_KIND) {
     dent->kd_mode = inode->i_kind;
     if (flags&KFD_READDIR_FLAG_PERM) dent->kd_mode |= (*iter++).ia_perm.p_perm;
    } else if (flags&KFD_READDIR_FLAG_PERM) {
     dent->kd_mode = (*iter++).ia_perm.p_perm;
    }
   }
  }
end_inode:
  kinode_decref(inode);
 }
 kfdentry_quit(&entry);
end:
 KTASK_CRIT_END
 RETURN(error);
}

/*< _syscall4(kerrno_t,kfd_readlink,int,fd,char *,buf,__size_t,bufsize,__size_t *,reqsize); */
SYSCALL(sys_kfd_readlink) {
 LOAD4(int     ,K(fd),
       char   *,K(buf),
       size_t  ,K(bufsize),
       size_t *,K(reqsize));
 struct kproc *caller = kproc_self();
 struct kfdentry entry; kerrno_t error;
 __ref struct kinode *filenode;
 struct kdirentname link_name;
 size_t link_req;
 KTASK_CRIT_BEGIN_FIRST
 if __unlikely(KE_ISERR(error = kproc_getfd(caller,fd,&entry))) goto end;
 if __unlikely(entry.fd_type != KFDTYPE_FILE) { error = KE_NOFILE; kfdentry_quit(&entry); goto end; }
 filenode = kfile_getinode(entry.fd_file);
 kfile_decref(entry.fd_file);
 if __unlikely(!filenode) { error = KE_NOLNK; goto end; }
 error = kinode_readlink(filenode,&link_name);
 kinode_decref(filenode);
 if __unlikely(KE_ISERR(error)) goto end;
 link_req = (link_name.dn_size+1)*sizeof(char);
 if (reqsize && copy_to_user(reqsize,&link_req,sizeof(link_req))) goto err_fault;
 if (link_name.dn_name[link_name.dn_size] == '\0') {
  if (copy_to_user(buf,link_name.dn_name,min(bufsize,link_req))) goto err_fault;
 } else {
  size_t link_size = link_name.dn_size*sizeof(char);
  if (copy_to_user(buf,link_name.dn_name,min(bufsize,link_size))) goto err_fault;
  if (link_size < bufsize &&
      copy_to_user((void *)((uintptr_t)buf+link_size),
                   "\0",sizeof(char))) goto err_fault;
 }
end_name: kdirentname_quit(&link_name);
end: KTASK_CRIT_END
 RETURN(error);
err_fault: error = KE_FAULT; goto end_name;
}

/*< _syscall2(int,kfd_dup,int,fd,int,flags); */
SYSCALL(sys_kfd_dup) {
 LOAD2(int,K(fd),
       int,K(flags));
 kerrno_t error; int result;
 error = kproc_dupfd(kproc_self(),fd,&result,(kfdflag_t)flags);
 if __likely(KE_ISOK(error)) error = result;
 RETURN(error);
}

/*< _syscall3(int,kfd_dup2,int,fd,int,resfd,int,flags); */
SYSCALL(sys_kfd_dup2) {
 LOAD3(int,K(fd),
       int,K(resfd),
       int,K(flags));
 kerrno_t error;
 error = kproc_dupfd2(kproc_self(),fd,resfd,(kfdflag_t)flags);
 if __likely(KE_ISOK(error)) error = resfd;
 RETURN(error);
}

/*< _syscall2(kerrno_t,kfd_pipe,int *,pipefd,int,flags); */
SYSCALL(sys_kfd_pipe) {
 LOAD2(int *,U(pipefd),
       int  ,K(flags));
 kerrno_t error; struct kpipe *pipe;
 struct kproc *proc_self = kproc_self();
 struct kfdentry entry; int pipes[2];
 KTASK_CRIT_BEGIN_FIRST
 pipe = kpipe_new(0x1000); // TODO: Don't hard-code this!
 if __unlikely(!pipe) { error = KE_NOMEM; goto end; }
 entry.fd_file = (struct kfile *)kpipefile_newreader(pipe,NULL);
 if __unlikely(!entry.fd_file) { error = KE_NOMEM; goto end_pipe; }
 entry.fd_type = KFDTYPE_FILE;
 entry.fd_flag = flags;
 error = kproc_insfd_inherited(proc_self,&pipes[0],&entry);
 if __unlikely(KE_ISERR(error)) { kfdentry_quit(&entry); goto end_pipe; }
 entry.fd_file = (struct kfile *)kpipefile_newwriter(pipe,NULL);
 if __unlikely(!entry.fd_file) { error = KE_NOMEM; goto err_pipe0; }
 entry.fd_type = KFDTYPE_FILE;
 entry.fd_flag = flags;
 error = kproc_insfd_inherited(proc_self,&pipes[1],&entry);
 if __unlikely(KE_ISERR(error)) { kfdentry_quit(&entry); goto err_pipe0; }
 // File descriptors were created!
 memcpy(pipefd,pipes,sizeof(pipes));
 goto end_pipe;
err_pipe0: kproc_closefd(proc_self,pipes[0]);
end_pipe:  kinode_decref((struct kinode *)pipe);
end:
 KTASK_CRIT_END
 RETURN(error);
}

/*< _syscall2(kerrno_t,kfd_equals,int,fda,int,fdb); */
SYSCALL(sys_kfd_equals) {
 LOAD2(int,K(fda),
       int,K(fdb));
 RETURN(kproc_equalfd(kproc_self(),fda,fdb));
}

/*< _syscall5(kerrno_t,kfd_openpty,int *,amaster,int *,aslave,char *,name,struct termios const *,termp,struct winsize const *,winp); */
SYSCALL(sys_kfd_openpty) {
 LOAD5(int                  *,K(amaster),
       int                  *,K(aslave),
       char                 *,K(name),
       struct termios const *,K(termp),
       struct winsize const *,K(winp));
 struct termios ios; struct winsize size;
 kerrno_t error; struct kfspty *fs_pty;
 __ref struct kdirent *m_ent,*s_ent;
 struct kfdentry fd_master,fd_slave;
 struct kproc *proc_self = kproc_self();
 int fno_master,fno_slave;
 fd_master.fd_attr = KFD_ATTR(KFDTYPE_FILE,KFD_FLAG_NONE);
 fd_slave.fd_attr = KFD_ATTR(KFDTYPE_FILE,KFD_FLAG_NONE);
 KTASK_CRIT_BEGIN_FIRST
 if (termp && copy_from_user(&ios,termp,sizeof(ios))) RETURN(KE_FAULT);
 if (winp && copy_from_user(&size,winp,sizeof(size))) RETURN(KE_FAULT);
 /* Create the actual filesystem-compatible PTY device. */
 fs_pty = kfspty_new(termp ? &ios : NULL,winp ? &size : NULL);
 if __unlikely(!fs_pty) goto err_nomem;
 /* Insert that device into the filesystem. */
 error = kfspty_insnod(fs_pty,&m_ent,&s_ent,name);
 if __unlikely(KE_ISERR(error)) goto end_fspty;
 /* Create the files used to interface with the master & slave ports. */
 fd_master.fd_file = (struct kfile *)kptyfile_master_new(fs_pty,m_ent);
 kdirent_decref(m_ent);
 if __unlikely(!fd_master.fd_file) { kdirent_decref(s_ent); goto err_nomem2; }
 fd_slave.fd_file = (struct kfile *)kptyfile_slave_new(fs_pty,s_ent);
 kdirent_decref(s_ent);
 if __unlikely(!fd_slave.fd_file) goto err_nomem3;
 /* Register the Master and Slave files as valid descriptors in the calling process. */
 error = kproc_insfd_inherited(proc_self,&fno_slave,&fd_slave);
 if __unlikely(KE_ISERR(error)) goto err4;
 if __unlikely(copy_to_user(aslave,&fno_slave,sizeof(fno_slave))) { error = KE_FAULT; goto err3; }
 error = kproc_insfd_inherited(proc_self,&fno_master,&fd_master);
 if __unlikely(KE_ISERR(error)) goto err3;
 if __unlikely(copy_to_user(amaster,&fno_master,sizeof(fno_master))) error = KE_FAULT;
 /* Do some finalizing cleanup... */
end_fspty: kinode_decref((struct kinode *)fs_pty);
end:
 KTASK_CRIT_END
 RETURN(error);
err4: kfile_decref(fd_slave.fd_file);
err3: kfile_decref(fd_master.fd_file); goto end_fspty; 
err_nomem3: error = KE_NOMEM; goto err3;
err_nomem2: error = KE_NOMEM; goto end_fspty; 
err_nomem: error = KE_NOMEM; goto end; 
}


__DECL_END

#endif /* !__KOS_KERNEL_SYSCALL_SYSCALL_KFD_C_INL__ */

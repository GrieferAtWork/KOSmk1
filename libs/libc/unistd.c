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
#ifndef __UNISTD_C__
#define __UNISTD_C__ 1

#include <errno.h>
#ifndef __CONFIG_MIN_LIBC__
#include <assert.h>
#include <fcntl.h>
#include <kos/atomic.h>
#include <kos/compiler.h>
#include <kos/config.h>
#include <kos/errno.h>
#include <kos/fd.h>
#include <kos/fs.h>
#include <kos/kernel/debug.h>
#include <kos/mem.h>
#include <kos/syscall.h>
#include <kos/task.h>
#include <kos/time.h>
#include <limits.h>
#include <malloc.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <termios.h>
#ifdef __KERNEL__
#include <kos/kernel/proc.h>
#include <kos/timespec.h>
#else
#include <proc.h>
#include <sched.h>
#include <sys/mman.h>
#endif
#endif /* !__CONFIG_MIN_LIBC__ */

#ifdef __KERNEL__
#undef kfs_mkdir
#undef kfs_mkreg
#undef kfs_mklnk
#undef kfs_rmdir
#undef kfs_unlink
#undef kfs_remove
#undef kfs_insnod
#undef kfs_mount
#undef kfs_open
#endif

__DECL_BEGIN

#ifndef __CONFIG_MIN_BSS__

__public errno_t *__geterrno(void) {
 // TODO: Thread-local
 static errno_t eno = 0;
 return &eno;
}
#endif

#ifndef __CONFIG_MIN_LIBC__
__public int open(char const *path, int mode, ...) {
 va_list args; kerrno_t error;
 va_start(args,mode);
 error = kfd_open(KFD_CWD,path,(size_t)-1,mode,
                  mode&O_CREAT ? (__mode_t)va_arg(args,int) : 0);
 va_end(args);
#ifdef __CONFIG_MIN_BSS__
 return error;
#else
 if __likely(KE_ISOK(error)) return error;
 __set_errno(-error);
 return -1;
#endif
}
__public int open2(int fd, char const *path, int mode, ...) {
 va_list args; kerrno_t error;
 va_start(args,mode);
 error = kfd_open2(fd,KFD_CWD,path,(size_t)-1,mode,
                    mode&O_CREAT ? (__mode_t)va_arg(args,int) : 0);
 va_end(args);
#ifdef __CONFIG_MIN_BSS__
 return error;
#else
 if __likely(KE_ISOK(error) || KFD_ISSPECIAL(error)) return error;
 __set_errno(-error);
 return -1;
#endif
}
__public int openat(int dirfd, char const *path, int mode, ...) {
 va_list args; kerrno_t error;
 va_start(args,mode);
 error = kfd_open(dirfd,path,(size_t)-1,mode,
                  mode&O_CREAT ? (__mode_t)va_arg(args,int) : 0);
 va_end(args);
#ifdef __CONFIG_MIN_BSS__
 return error;
#else
 if __likely(KE_ISOK(error)) return error;
 __set_errno(-error);
 return -1;
#endif
}
__public int openat2(int fd, int dirfd, char const *path, int mode, ...) {
 va_list args; kerrno_t error;
 va_start(args,mode);
 error = kfd_open2(fd,dirfd,path,(size_t)-1,mode,
                    mode&O_CREAT ? (__mode_t)va_arg(args,int) : 0);
 va_end(args);
#ifdef __CONFIG_MIN_BSS__
 return error;
#else
 if __likely(KE_ISOK(error) || KFD_ISSPECIAL(error)) return error;
 __set_errno(-error);
 return -1;
#endif
}
__public int close(int fd) {
#ifdef __CONFIG_MIN_BSS__
 return kfd_close(fd);
#else
 kerrno_t error = kfd_close(fd);
 if __likely(KE_ISOK(error)) return 0;
 __set_errno(-error);
 return -1;
#endif
}
__public ssize_t read(int fd, void *__restrict buf, size_t bufsize) {
 kerrno_t error; size_t rsize;
 if __unlikely(bufsize > SSIZE_MAX) bufsize = (size_t)SSIZE_MAX;
 error = kfd_read(fd,buf,bufsize,&rsize);
 if __likely(KE_ISOK(error)) return (ssize_t)rsize;
#ifdef __CONFIG_MIN_BSS__
 return error;
#else
 __set_errno(-error);
 return -1;
#endif
}
__public ssize_t write(int fd, void const *__restrict buf, size_t bufsize) {
 kerrno_t error; size_t rsize;
 if __unlikely(bufsize > SSIZE_MAX) bufsize = (size_t)SSIZE_MAX;
 error = kfd_write(fd,buf,bufsize,&rsize);
 if __likely(KE_ISOK(error)) return (ssize_t)rsize;
#ifdef __CONFIG_MIN_BSS__
 return error;
#else
 __set_errno(-error);
 return -1;
#endif
}
__public off64_t lseek64(int fd, off64_t off, int whence) {
 kerrno_t error; __u64 newpos;
 error = kfd_seek(fd,KFD_POSITION(off),whence,&newpos);
 if __likely(KE_ISOK(error)) {
  if (newpos > (__u64)INT64_MAX) error = KE_OVERFLOW;
  else return (off64_t)newpos;
 }
#ifdef __CONFIG_MIN_BSS__
 return error;
#else
 __set_errno(-error);
 return -1;
#endif
}

__public __compiler_ALIAS(llseek,lseek64);
#ifdef __USE_FILE_OFFSET64
__public __compiler_ALIAS(lseek,lseek64);
#else
__public off_t lseek(int fd, off_t off, int whence) {
 return (off_t)lseek64(fd,off,whence);
}
#endif

__public int ioctl(int fd, int cmd, ...) {
 kerrno_t error; va_list args; va_start(args,cmd);
 error = kfd_ioctl(fd,cmd,va_arg(args,void *));
 if __likely(KE_ISOK(error)) return 0;
#ifdef __CONFIG_MIN_BSS__
 return error;
#else
 __set_errno(-error);
 return -1;
#endif
}
__public ssize_t _getattr(int fd, kattr_t attr, void *__restrict buf, size_t bufsize) {
 kerrno_t error; size_t reqsize;
 error = kfd_getattr(fd,attr,buf,bufsize,&reqsize);
 if __likely(KE_ISOK(error)) return (ssize_t)reqsize;
#ifdef __CONFIG_MIN_BSS__
 return error;
#else
 __set_errno(-error);
 return -1;
#endif
}
__public int _setattr(int fd, kattr_t attr, void const *__restrict buf, size_t bufsize) {
#ifdef __CONFIG_MIN_BSS__
 return kfd_setattr(fd,attr,buf,bufsize);
#else
 kerrno_t error = kfd_setattr(fd,attr,buf,bufsize);
 if __likely(KE_ISOK(error)) return 0;
 __set_errno(-error);
 return -1;
#endif
}


__public int dup(int fd) {
#ifdef __CONFIG_MIN_BSS__
 return kfd_dup(fd,0);
#else
 kerrno_t error = kfd_dup(fd,0);
 if __likely(KE_ISOK(error)) return error;
 __set_errno(-error);
 return -1;
#endif
}
__public int dup2(int fd, int newfd) {
#ifdef __CONFIG_MIN_BSS__
 return kfd_dup2(fd,newfd,0);
#else
 kerrno_t error = kfd_dup2(fd,newfd,0);
 if __likely(KE_ISOK(error)) return newfd;
 __set_errno(-error);
 return -1;
#endif
}
__public int dup3(int fd, int newfd, int flags) {
#ifdef __CONFIG_MIN_BSS__
 return kfd_dup2(fd,newfd,flags);
#else
 kerrno_t error = kfd_dup2(fd,newfd,flags);
 if __likely(KE_ISOK(error)) return newfd;
 __set_errno(-error);
 return -1;
#endif
}

__public int chroot(char const *path) {
 return open2(KFD_ROOT,path,O_RDONLY|O_DIRECTORY);
}


__public int fsync(int fd) {
#ifdef __CONFIG_MIN_BSS__
 return kfd_flush(fd);
#else
 kerrno_t error = kfd_flush(fd);
 if __likely(KE_ISOK(error)) return 0;
 __set_errno(-error);
 return -1;
#endif
}
__public __compiler_ALIAS(fdatasync,fsync)

__public ssize_t pread(int fd, void *__restrict buf, size_t bufsize, off_t pos) {
#ifdef __CONFIG_MIN_BSS__
 assert(pos >= 0);
#else
 if __unlikely(pos < 0) { __set_errno(EINVAL); return -1; }
#endif
 return _pread64(fd,buf,bufsize,(__pos64_t)pos);
}
__public ssize_t pwrite(int fd, const void *__restrict buf, size_t bufsize, off_t pos) {
#ifdef __CONFIG_MIN_BSS__
 assert(pos >= 0);
#else
 if __unlikely(pos < 0) { __set_errno(EINVAL); return -1; }
#endif
 return _pwrite64(fd,buf,bufsize,(__pos64_t)pos);
}

__public ssize_t _pread64(int fd, void *__restrict buf, size_t bufsize, __pos64_t pos) {
 kerrno_t error; size_t rsize;
 if __unlikely(bufsize > SSIZE_MAX) bufsize = (size_t)SSIZE_MAX;
 error = kfd_pread(fd,KFD_POSITION(pos),buf,bufsize,&rsize);
 if __likely(KE_ISOK(error)) return (ssize_t)rsize;
#ifdef __CONFIG_MIN_BSS__
 return error;
#else
 __set_errno(-error);
 return -1;
#endif
}
__public ssize_t _pwrite64(int fd, const void *__restrict buf, size_t bufsize, __pos64_t pos) {
 kerrno_t error; size_t wsize;
 if __unlikely(bufsize > SSIZE_MAX) bufsize = (size_t)SSIZE_MAX;
 error = kfd_pwrite(fd,KFD_POSITION(pos),buf,bufsize,&wsize);
 if __likely(KE_ISOK(error)) return (ssize_t)wsize;
#ifdef __CONFIG_MIN_BSS__
 return error;
#else
 __set_errno(-error);
 return -1;
#endif
}


__public int ftruncate(int fd, off_t newsize) {
 if __unlikely(newsize < 0) { __set_errno(EINVAL); return -1; }
 return _ftruncate64(fd,(__pos64_t)newsize);
}
__public int _ftruncate64(int fd, __pos64_t newsize) {
#ifdef __CONFIG_MIN_BSS__
 return kfd_trunc(fd,KFD_POSITION(newsize));
#else
 kerrno_t error = kfd_trunc(fd,KFD_POSITION(newsize));
 if __likely(KE_ISOK(error)) return 0;
 __set_errno(-error);
 return -1;
#endif
}

__public int pipe(int pipefd[2]) {
 return pipe2(pipefd,0);
}
__public int pipe2(int pipefd[2], int flags) {
#ifdef __CONFIG_MIN_BSS__
 return kfd_pipe(pipefd,flags);
#else
 kerrno_t error = kfd_pipe(pipefd,flags);
 if __likely(KE_ISOK(error)) return 0;
 __set_errno(-error);
 return -1;
#endif
}

__local char *fd_mallgetattrstring(int fd, kattr_t attr) {
 ssize_t reqsize = PATH_MAX; size_t bufsize;
 char *newbuf,*buf = NULL; do {
  newbuf = (char *)realloc(buf,reqsize);
  if __unlikely(!newbuf) { free(buf); return NULL; }
  bufsize = (size_t)reqsize;
  reqsize = _getattr(fd,attr,newbuf,bufsize);
  if (reqsize < 0) { free(newbuf); return NULL; }
  buf = newbuf;
 } while (reqsize > bufsize);
 if ((size_t)reqsize != bufsize) {
  newbuf = (char *)realloc(buf,(size_t)reqsize);
  if __unlikely(!newbuf) __set_errno(EOK);
  else buf = newbuf;
 }
 return buf;
}
__local char *fd_getattrstring(int fd, kattr_t attr, char *__restrict buf, size_t bufsize) {
 ssize_t reqsize;
 if (!bufsize) return fd_mallgetattrstring(fd,attr);
 reqsize = _getattr(fd,attr,buf,bufsize);
 if __unlikely(reqsize < 0) return NULL;
 if __unlikely(reqsize > bufsize) { __set_errno(ERANGE); return NULL; }
 return buf;
}

__public int access(char const *path, int mode) {
 return faccessat(AT_FDCWD,path,mode,0);
}
__public int faccessat(int dirfd, char const *path, int mode, int flags) {
 openmode_t om; int fd;
 switch (mode&(W_OK|R_OK)) {
  case W_OK|R_OK: om = O_RDWR; break;
  case W_OK:      om = O_WRONLY; break;
  default:        om = O_RDONLY; break;
 }
 if (flags&AT_SYMLINK_NOFOLLOW) om |= _O_SYMLINK;
 fd = openat(dirfd,path,om);
 if (fd == -1) return -1;
 kfd_close(fd);
 return 0;
}

__public int truncate(char const *path, off_t newsize) {
 int error; int fd = open(path,O_RDWR);
 if __unlikely(fd == -1) return -1;
 error = ftruncate(fd,newsize);
 kfd_close(fd);
 return error;
}

__public int chown(char const *path, uid_t owner, gid_t group) {
 int error,fd = open(path,O_RDWR);
 if __unlikely(fd == -1) return -1;
 error = fchown(fd,owner,group);
 kfd_close(fd);
 return error;
}
__public int lchown(char const *path, uid_t owner, gid_t group) {
 int error,fd = open(path,O_RDWR|_O_SYMLINK);
 if __unlikely(fd == -1) return -1;
 error = fchown(fd,owner,group);
 kfd_close(fd);
 return error;
}
__public int fchown(int fd, uid_t owner, gid_t group) {
 union kinodeattr attrib[2]; kerrno_t error;
 attrib[0].ia_owner.a_id = KATTR_FS_OWNER;
 attrib[0].ia_owner.o_owner = owner;
 attrib[1].ia_group.a_id = KATTR_FS_GROUP;
 attrib[1].ia_group.g_group = group;
 error = kfd_getattr(fd,KATTR_FS_ATTRS,attrib,sizeof(attrib),NULL);
#ifdef __CONFIG_MIN_BSS__
 return error;
#else
 if __likely(KE_ISOK(error)) return 0;
 __set_errno(-error);
 return -1;
#endif
}
__public int fchownat(int dirfd, char const *path,
                      uid_t owner, gid_t group, int flags) {
 int error,fd;
 fd = openat(dirfd,path,O_RDWR|_O_SYMLINK);
 if __unlikely(fd == -1) return -1;
 error = fchown(fd,owner,group);
 kfd_close(fd);
 return error;
}

static int __linkat_impl(char const *target_name, int link_dirfd,
                         char const *link_name, int flags) {
 kerrno_t error;
 if (flags&AT_SYMLINK_FOLLOW) {
  // TODO: Dereference 'target'
 }
 error = kfs_symlink(link_dirfd,target_name,(size_t)-1,link_name,(size_t)-1);
#ifdef __CONFIG_MIN_BSS__
 return error;
#else
 if __likely(KE_ISOK(error)) return 0;
 __set_errno(-error);
 return -1;
#endif
}

__public int link(char const *target_name, char const *link_name) {
#ifdef __KERNEL__
 return kfs_symlink(KFD_CWD,target_name,(size_t)-1,link_name,(size_t)-1);
#else
#ifdef __CONFIG_MIN_BSS__
 return kfs_symlink(KFD_CWD,target_name,(size_t)-1,link_name,(size_t)-1);
#else
 kerrno_t error = kfs_symlink(KFD_CWD,target_name,(size_t)-1,link_name,(size_t)-1);
 if __likely(KE_ISOK(error)) return 0;
 __set_errno(-error);
 return -1;
#endif
#endif
}

__public int linkat(int target_dirfd, char const *target_name,
                    int link_dirfd, char const *link_name, int flags) {
 char *target_dirname,*target_fullname;
 size_t target_dirsize,targetsize; int error;
 if (target_dirfd == link_dirfd) {
  return __linkat_impl(target_name,link_dirfd,link_name,flags);
 }
 target_dirname = fd_mallgetattrstring(target_dirfd,KATTR_FS_PATHNAME);
 if __unlikely(!target_dirname) return -1;
 target_dirsize = strlen(target_dirname);
 targetsize = strlen(target_name);
 target_fullname = (char *)malloc((target_dirsize+targetsize+1)*sizeof(char));
 if __unlikely(!target_fullname) { free(target_dirname); return -1; }
 memcpy(target_fullname,target_dirname,target_dirsize);
 free(target_dirname);
 strcpy(target_fullname+target_dirsize,target_name);
 error = __linkat_impl(target_fullname,link_dirfd,link_name,flags);
 free(target_fullname);
 return error;
}

__public ssize_t readlink(char const *__restrict path,
                          char *__restrict buf, size_t bufsize) {
 return readlinkat(KFD_CWD,path,buf,bufsize);
}
__public ssize_t readlinkat(int dirfd, char const *__restrict path,
                            char *__restrict buf, size_t bufsize) {
 ssize_t result; int lfd = open(path,O_RDONLY|_O_SYMLINK);
 if __unlikely(lfd == -1) return -1;
 result = _freadlink(lfd,buf,bufsize);
 kfd_close(lfd);
 return result;
}
__public ssize_t _freadlink(int fd, char *__restrict buf, size_t bufsize) {
 size_t result; kerrno_t error;
 error = kfd_readlink(fd,buf,bufsize,&result);
 if __likely(KE_ISOK(error)) return (ssize_t)result;
 __set_errno(-error);
 return -1;
}

__public int mkdir(const char *pathname, mode_t mode) {
 return mkdirat(KFD_CWD,pathname,mode);
}
__public int mkdirat(int dirfd, const char *pathname, mode_t mode) {
#ifdef __CONFIG_MIN_BSS__
 return kfs_mkdir(dirfd,pathname,(size_t)-1,mode);
#else
 kerrno_t error = kfs_mkdir(dirfd,pathname,(size_t)-1,mode);
 if __likely(KE_ISOK(error)) return 0;
 __set_errno(-error);
 return -1;
#endif
}
__public int rmdir(char const *path) {
#ifdef __CONFIG_MIN_BSS__
 return kfs_rmdir(KFD_CWD,path,(size_t)-1);
#else
 kerrno_t error = kfs_rmdir(KFD_CWD,path,(size_t)-1);
 if __likely(KE_ISOK(error)) return 0;
 __set_errno(-error);
 return -1;
#endif
}
__public int unlink(char const *path) {
#ifdef __CONFIG_MIN_BSS__
 return kfs_unlink(KFD_CWD,path,(size_t)-1);
#else
 kerrno_t error = kfs_unlink(KFD_CWD,path,(size_t)-1);
 if __likely(KE_ISOK(error)) return 0;
 __set_errno(-error);
 return -1;
#endif
}
__public int unlinkat(int dirfd, char const *path, int flags) {
#ifdef __CONFIG_MIN_BSS__
 return flags&AT_REMOVEDIR
  ? kfs_rmdir(KFD_CWD,path,(size_t)-1)
  : kfs_unlink(KFD_CWD,path,(size_t)-1);
#else
 kerrno_t error = flags&AT_REMOVEDIR
  ? kfs_rmdir(KFD_CWD,path,(size_t)-1)
  : kfs_unlink(KFD_CWD,path,(size_t)-1);
 if __likely(KE_ISOK(error)) return 0;
 __set_errno(-error);
 return -1;
#endif
}
__public int remove(char const *path) {
#ifdef __CONFIG_MIN_BSS__
 return kfs_remove(KFD_CWD,path,(size_t)-1);
#else
 kerrno_t error = kfs_remove(KFD_CWD,path,(size_t)-1);
 if __likely(KE_ISOK(error)) return 0;
 __set_errno(-error);
 return -1;
#endif
}
__public int removeat(int dirfd, char const *path) {
#ifdef __CONFIG_MIN_BSS__
 return kfs_remove(KFD_CWD,path,(size_t)-1);
#else
 kerrno_t error = kfs_remove(KFD_CWD,path,(size_t)-1);
 if __likely(KE_ISOK(error)) return 0;
 __set_errno(-error);
 return -1;
#endif
}

__public void sync(void) {
 // TODO: Call fsync on all open file descriptors
}

__public int chdir(char const *path) {
 return open2(KFD_CWD,path,O_RDONLY|O_DIRECTORY);
}
__public int fchdir(int fd) {
 return __unlikely(fd == KFD_CWD) ? 0 : dup2(fd,KFD_CWD);
}
__public char *getcwd(char *__restrict buf, size_t bufsize) {
 return fd_getattrstring(KFD_CWD,KATTR_FS_PATHNAME,buf,bufsize);
}
__public char *get_current_dir_name(void) {
 return fd_mallgetattrstring(KFD_CWD,KATTR_FS_PATHNAME);
}

__public int fcntl(int fd, int cmd, ...) {
 va_list args; void *arg;
 va_start(args,cmd);
#if __SIZEOF_POINTER__ <= __SIZEOF_INT__
 arg = va_arg(args,void *);
#else
 switch (cmd) {
  case F_DUPFD:
  case F_DUPFD_CLOEXEC:
  case F_SETFD:
  case F_SETFL:
  case F_SETOWN:
  case F_SETSIG:
  case F_SETLEASE:
#ifdef F_ADD_SEALS
  case F_ADD_SEALS:
#endif
   arg = (void *)(uintptr_t)va_arg(args,int);
   break;
  default: arg = va_arg(args,void *); break;
 }
#endif
 va_end(args);
#ifdef __KERNEL__
 return kfd_fcntl(fd,cmd,arg);
#else
 {
  kerrno_t error = kfd_fcntl(fd,cmd,arg);
  if __likely(error >= 0) return error;
  __set_errno(-error);
  return -1;
 }
#endif
}

__public int stat(char const *path, struct stat *st) {
#ifdef __USE_FILE_OFFSET64
 return stat64(path,(struct stat64 *)st);
#else
 return stat32(path,(struct stat32 *)st);
#endif
}
__public int lstat(char const *path, struct stat *st) {
#ifdef __USE_FILE_OFFSET64
 return lstat64(path,(struct stat64 *)st);
#else
 return lstat32(path,(struct stat32 *)st);
#endif
}
__public int fstat(int fd, struct stat *st) {
#ifdef __USE_FILE_OFFSET64
 return fstat64(fd,(struct stat64 *)st);
#else
 return fstat32(fd,(struct stat32 *)st);
#endif
}

__public int stat32(char const *path, struct stat32 *st) {
 int error,fd = open(path,O_RDONLY);
 if __unlikely(fd == -1) return -1;
 error = fstat32(fd,st);
 kfd_close(fd);
 return error;
}
__public int stat64(char const *path, struct stat64 *st) {
 int error,fd = open(path,O_RDONLY);
 if __unlikely(fd == -1) return -1;
 error = fstat64(fd,st);
 kfd_close(fd);
 return error;
}
__public int lstat32(char const *path, struct stat32 *st) {
 int error,fd = open(path,O_RDONLY|_O_SYMLINK);
 if __unlikely(fd == -1) return -1;
 error = fstat32(fd,st);
 kfd_close(fd);
 return error;
}
__public int lstat64(char const *path, struct stat64 *st) {
 int error,fd = open(path,O_RDONLY|_O_SYMLINK);
 if __unlikely(fd == -1) return -1;
 error = fstat64(fd,st);
 kfd_close(fd);
 return error;
}

__public int fstat32(int fd, struct stat32 *st) {
 union kinodeattr attrib[10];
 kassertobj(st);
 attrib[0].ia_common.a_id = KATTR_FS_ATIME;
 attrib[1].ia_common.a_id = KATTR_FS_CTIME;
 attrib[2].ia_common.a_id = KATTR_FS_MTIME;
 attrib[3].ia_common.a_id = KATTR_FS_PERM;
 attrib[4].ia_common.a_id = KATTR_FS_OWNER;
 attrib[5].ia_common.a_id = KATTR_FS_GROUP;
 attrib[6].ia_common.a_id = KATTR_FS_SIZE;
 attrib[7].ia_common.a_id = KATTR_FS_INO;
 attrib[8].ia_common.a_id = KATTR_FS_NLINK;
 attrib[9].ia_common.a_id = KATTR_FS_KIND;
 if __unlikely(_getattr(fd,KATTR_FS_ATTRS,attrib,sizeof(attrib)) == -1) return -1;
 st->st_dev = (__dev_t)0; /* TODO? */
 st->st_ino = attrib[7].ia_ino.i_ino;
 st->st_mode = attrib[9].ia_kind.k_kind|attrib[3].ia_perm.p_perm;
 st->st_nlink = attrib[8].ia_nlink.n_lnk;
 st->st_uid = attrib[4].ia_owner.o_owner;
 st->st_gid = attrib[5].ia_group.g_group;
 st->st_rdev = 0; /* TODO? */
 st->st_size = (__off32_t)attrib[6].ia_size.sz_size;
 st->st_blksize = 512; /* TODO? */
 st->st_blocks = (__blkcnt32_t)ceildiv(st->st_size,st->st_blksize);
 st->st_atim = attrib[0].ia_time.tm_time;
 st->st_mtim = attrib[1].ia_time.tm_time;
 st->st_ctim = attrib[2].ia_time.tm_time;
 return 0;
}
__public int fstat64(int fd, struct stat64 *st) {
 union kinodeattr attrib[10];
 kassertobj(st);
 attrib[0].ia_common.a_id = KATTR_FS_ATIME;
 attrib[1].ia_common.a_id = KATTR_FS_CTIME;
 attrib[2].ia_common.a_id = KATTR_FS_MTIME;
 attrib[3].ia_common.a_id = KATTR_FS_PERM;
 attrib[4].ia_common.a_id = KATTR_FS_OWNER;
 attrib[5].ia_common.a_id = KATTR_FS_GROUP;
 attrib[6].ia_common.a_id = KATTR_FS_SIZE;
 attrib[7].ia_common.a_id = KATTR_FS_INO;
 attrib[8].ia_common.a_id = KATTR_FS_NLINK;
 attrib[9].ia_common.a_id = KATTR_FS_KIND;
 if __unlikely(_getattr(fd,KATTR_FS_ATTRS,attrib,sizeof(attrib)) == -1) return -1;
 st->st_dev = (__dev_t)0; /* TODO? */
 st->st_ino = attrib[7].ia_ino.i_ino;
 st->st_mode = attrib[9].ia_kind.k_kind|attrib[3].ia_perm.p_perm;
 st->st_nlink = attrib[8].ia_nlink.n_lnk;
 st->st_uid = attrib[4].ia_owner.o_owner;
 st->st_gid = attrib[5].ia_group.g_group;
 st->st_rdev = 0; /* TODO? */
 st->st_size = (__off64_t)attrib[6].ia_size.sz_size;
 st->st_blksize = 512; /* TODO? */
 st->st_blocks = (__blkcnt64_t)ceildiv(st->st_size,st->st_blksize);
 st->st_atim = attrib[0].ia_time.tm_time;
 st->st_mtim = attrib[1].ia_time.tm_time;
 st->st_ctim = attrib[2].ia_time.tm_time;
 return 0;
}

__public long sysconf(int name) {
#ifdef __KERNEL__
 return k_sysconf(name);
#else
 long result = k_sysconf(name);
 if __likely(KE_ISOK(result)) return result;
 __set_errno(-result);
 return -1;
#endif
}

#ifdef _SC_PAGE_SIZE
__public int getpagesize(void) { return k_sysconf(_SC_PAGE_SIZE); }
#endif
#ifdef _SC_PHYS_PAGES
__public long get_phys_pages(void) { return k_sysconf(_SC_PHYS_PAGES); }
#endif
#ifdef _SC_AVPHYS_PAGES
__public long get_avphys_pages(void) { return k_sysconf(_SC_AVPHYS_PAGES); }
#endif


__public pid_t getpid(void) {
 return kproc_getpid(kproc_self());
}
#undef getpgid
#undef __getpgid


// In KOS, process security is somewhat different:
// >> While not 100% posix conforming, KOS defines:
//    - process groups as level-2 (restricted memory read/write)
//    - process sessions as level-3 (restricted memory read/write/setprop)
// When neither a group or session has been set, a level-0 barrier
// is in place, meaning that no restrictions are being performed.
#define __KOS_PGID_BARRIER  KSANDBOX_BARRIERLEVEL(2)
#define __KOS_SID_BARRIER   KSANDBOX_BARRIERLEVEL(3)

#ifndef __KERNEL__
static __pid_t __kos_getbarrierpid(int proc, ksandbarrier_t barrier) {
 __pid_t result;
 int barrier_fd = kproc_openbarrier(proc,barrier);
 if (KE_ISERR(barrier_fd)) {
  __set_errno(-barrier_fd);
  return -1;
 }
 result = kproc_getpid(barrier_fd);
 kfd_close(barrier_fd);
 return result;
}
static __pid_t __kos_getbarrierpid_p(__pid_t proc, ksandbarrier_t barrier) {
 __pid_t result; int proc_fd;
 if (!proc) {
  /* In all situations this function is called from, proc==0 means the current process. */
  return __kos_getbarrierpid(kproc_self(),barrier);
 }
 proc_fd = kproc_openpid(proc);
 if (KE_ISERR(proc_fd)) {
#ifdef __KERNEL__
  return proc_fd;
#else
  __set_errno(-proc_fd);
  return -1;
#endif
 }
 result = __kos_getbarrierpid(proc_fd,barrier);
 kfd_close(proc_fd);
 return result;
}

__public __compiler_ALIAS(__getpgid,getpgid);
__public __pid_t getpgid(__pid_t pid) {
 return __kos_getbarrierpid_p(pid,__KOS_PGID_BARRIER);
}
__public __pid_t getsid(__pid_t pid) {
 return __kos_getbarrierpid_p(pid,__KOS_SID_BARRIER);
}


__public int setpgid(__pid_t pid, __pid_t pgid) {
 if ((!pid || pgid == getpid()) &&
     (!pgid || pid == getpid())) {
  /* Set the current process to its own group. - This is allowed! */
  return setpgrp();
 }
 /* KOS doesn't allow you to set the barrier of a different process (ever!). */
 __set_errno(EACCES);
 return -1;
}

__public int setpgrp(void) {
#ifdef __KERNEL__
 return kproc_barrier(__KOS_PGID_BARRIER);
#else
 kerrno_t error = kproc_barrier(__KOS_PGID_BARRIER);
 if __likely(KE_ISOK(error)) return 0;
 __set_errno(-error);
 return -1;
#endif
}

__public __pid_t setsid(void) {
 kerrno_t error = kproc_barrier(__KOS_SID_BARRIER);
 if __likely(KE_ISOK(error)) return getpid();
#ifdef __KERNEL__
 return error;
#else
 __set_errno(-error);
 return -1;
#endif
}
#endif

__public int isatty(int fd) {
 struct winsize wsiz;
 if (ioctl(fd,TIOCGWINSZ,&wsiz) < 0) return 0;
 return 1;
}

__public int _fdtype(int fd) {
 kfdtype_t result;
 if (fcntl(fd,_F_GETYPE,&result) == -1) return -1;
 return (int)result;
}
__public int _isafile(int fd) {
 int ty = fdtype(fd);
 return ty == -1 ? 0 : (ty == KFDTYPE_FILE ? 1 : (__set_errno(EBADF),0));
}
__public unsigned int _closeall(int low, int high) {
 return kfd_closeall(low,high);
}


#ifndef __KERNEL__
__public char *ttyname(int fd) {
 int error; static char buf[32];
 error = ttyname_r(fd,buf,sizeof(buf));
 if __unlikely(error != 0) { __set_errno(error); return NULL; }
 return buf;
}
#endif /* !__KERNEL__ */

__public int ttyname_r(int fd, char *buf, size_t buflen) {
 kerrno_t error; size_t reqsize;
 error = kfd_getattr(fd,KATTR_FS_PATHNAME,buf,buflen,&reqsize);
 if __unlikely(KE_ISERR(error)) return -error;
 if __unlikely(reqsize > buflen) return ERANGE;
 return 0;
}


__public pid_t getppid(void) {
#ifdef __KERNEL__
 return kproc_getpid(ktask_getproc(ktask_parent_k(ktask_proc())));
#else
 ktask_t parfd; pid_t result;
 // Try to open a descriptor for the parent of
 // the current process's root thread (task)
 parfd = ktask_openparent(ktask_proc());
 if __unlikely(KE_ISERR(parfd)) {
  /* May not be 100% correct, but the best we can do... */
#if 1
  return 0;
#else
  __set_errno(-parfd);
  return -1;
#endif
 }
 // For comply with getppid()'s semansics, KOS defines
 // PID namespaces as what barriers are here.
 // (If we can't see the parent, we return '0')
 // NOTE: KOS doesn't have PID namespaces (PIDs are actually not really
 //       used for anything but compatibility with other platforms and
 //       implementing a way of preventing new processes from spawning).
 if (kfd_equals(parfd,ktask_proc())) {
  // We are the origin of a barrier
  result = 0;
 } else {
  result = kproc_getpid(parfd);
 }
 kfd_close(parfd);
 return result;
#endif
}

__public unsigned int alarm(unsigned int seconds) {
 struct timespec abstime;
 struct timespec newalarm;
 struct timespec oldalarm;
 kerrno_t error;
#ifdef __KERNEL__
 ktime_getnoworcpu(&abstime);
#else
 error = ktime_getnoworcpu(&abstime);
 if __unlikely(KE_ISERR(error)) goto err;
#endif
 newalarm = abstime;
 newalarm.tv_sec += seconds;
 error = ktask_setalarm(ktask_self(),&newalarm,&oldalarm);
 if __unlikely(KE_ISERR(error)) goto err;
 if (oldalarm.tv_sec > abstime.tv_sec)
  return oldalarm.tv_sec-abstime.tv_sec;
 return 0;
err:
 __set_errno(-error);
 return 0;
}

__public __useconds_t ualarm(__useconds_t value, __useconds_t interval) {
 struct timespec abstime;
 struct timespec newalarm;
 struct timespec oldalarm;
 kerrno_t error;
 if (interval) { __set_errno(ENOSYS); return 0; }
#ifdef __KERNEL__
 ktime_getnoworcpu(&abstime);
#else
 error = ktime_getnoworcpu(&abstime);
 if __unlikely(KE_ISERR(error)) goto err;
#endif
 newalarm = abstime;
 newalarm.tv_sec += value/1000000;
 newalarm.tv_nsec += (value%1000000)*1000;
 error = ktask_setalarm(ktask_self(),&newalarm,&oldalarm);
 if __unlikely(KE_ISERR(error)) goto err;
 if (__timespec_cmpgr(&oldalarm,&abstime)) {
  return ((oldalarm.tv_sec-abstime.tv_sec)*1000000)+
         ((oldalarm.tv_nsec-abstime.tv_nsec)/1000);
 }
 return 0;
err:
 __set_errno(-error);
 return 0;
}


__public unsigned int sleep(unsigned int seconds) {
 struct timespec abstime,newtime;
#ifdef __KERNEL__
 ktime_getnoworcpu(&abstime);
 abstime.tv_sec += seconds;
 ktask_abssleep(ktask_self(),&abstime);
 ktime_getnoworcpu(&newtime);
#else
 if __unlikely(KE_ISERR(ktime_getnoworcpu(&abstime))) return seconds;
 abstime.tv_sec += seconds;
 if __unlikely(KE_ISERR(ktask_abssleep(ktask_self(),&abstime))) return seconds;
 if __unlikely(KE_ISERR(ktime_getnoworcpu(&newtime))) return 0;
#endif
 return newtime.tv_sec-(abstime.tv_sec-seconds);
}

__public int usleep(__useconds_t useconds) {
 struct timespec abstime;
#ifdef __KERNEL__
 ktime_getnoworcpu(&abstime);
 abstime.tv_sec += useconds/1000000;
 abstime.tv_nsec += (useconds%1000000)*1000;
 return ktask_abssleep(ktask_self(),&abstime);
#else
 kerrno_t error = ktime_getnoworcpu(&abstime);
 if __unlikely(KE_ISERR(error)) goto err;
 abstime.tv_sec += useconds/1000000;
 abstime.tv_nsec += (useconds%1000000)*1000;
 error = ktask_abssleep(ktask_self(),&abstime);
 if __unlikely(KE_ISERR(error)) goto err;
 return 0;
err:
 __set_errno(-error);
 return -1;
#endif
}

__public int pause(void) {
#ifdef __KERNEL__
 return ktask_pause(ktask_self());
#else
 kerrno_t error = ktask_pause(ktask_self());
 if __unlikely(KE_ISERR(error)) goto err;
 return 0;
err: __set_errno(-error);
 return -1;
#endif
}



#ifndef __KERNEL__
__public pid_t fork(void) {
 pid_t result; task_t child;
 if ((child = task_fork()) == 0) return 0; /*< Returns ZERO for child. */
 if __unlikely(child == -1) return -1;
 // Parent task: Figure out child PID and close child descriptor
 result = proc_getpid(child);
 kfd_close(child);
 return result;
}
__public void *mmap(void *addr, size_t length, int prot,
                    int flags, int fd, __off_t offset) {
 void *result = kmem_map(addr,length,prot,flags,fd,KFD_POSITION(offset));
 if __unlikely(result == KMEM_MAP_FAIL) __set_errno(ENOMEM);
 return result;
}
__public int munmap(void *addr, size_t length) {
 kerrno_t error = kmem_unmap(addr,length);
 if __likely(KE_ISOK(error)) return 0;
 __set_errno(-error);
 return -1;
}
__public void *
mmapdev(void *addr, size_t length, int prot,
        int flags, __kernel void *physical_address) {
 kerrno_t error; void *result = addr;
 error = kmem_mapdev(&result,length,prot,flags,physical_address);
 if __likely(KE_ISOK(error)) return result;
 __set_errno(-error);
 return KMEM_MAP_FAIL;
}



#ifdef __LIBC_HAVE_SBRK
extern __u8 _end[]; /*< Automatically defined by the linker (end of the BSS). */
static __u8 *__brk_end = _end;

#ifdef __LIBC_HAVE_SBRK_THREADSAFE
static __atomic int __brk_lock = 0;
#define BRK_ACQUIRE  while(!katomic_cmpxch(__brk_lock,0,1))ktask_yield();
#define BRK_RELEASE  katomic_store(__brk_lock,0);
static int __do_brk(void *addr);
__public int brk(void *addr) {
 int result;
 BRK_ACQUIRE
 result = __do_brk(addr);
 BRK_RELEASE
 return result;
}
static int __do_brk(void *addr)
#else
__public int brk(void *addr)
#endif
{
 __u8 *real_oldbrk = (__u8 *)align((uintptr_t)__brk_end,4096);
 __u8 *real_newbrk = (__u8 *)align((uintptr_t)addr,4096);
 if (real_newbrk < real_oldbrk) {
  // Release memory
  if __unlikely(munmap(real_newbrk,real_oldbrk-real_newbrk) == -1) return -1;
 } else if (real_newbrk > real_oldbrk) {
  void *map_result;
  // Allocate more memory
  map_result = mmap(real_oldbrk,real_newbrk-real_oldbrk,
                    PROT_READ|PROT_WRITE,MAP_FIXED|MAP_ANONYMOUS,0,0);
  if (map_result == (void *)(uintptr_t)-1) return -1;
  assert(map_result == real_oldbrk);
 }
 __brk_end = (__u8 *)addr;
 return 0;
}


__public void *sbrk(intptr_t increment) {
 __u8 *result = __brk_end;
#ifdef __LIBC_HAVE_SBRK_THREADSAFE
 BRK_ACQUIRE
 if __unlikely(__do_brk(result+increment) == -1) {
  result = (__u8 *)(uintptr_t)-1;
 }
 BRK_RELEASE
#else /* __LIBC_HAVE_SBRK_THREADSAFE */
 if __unlikely(brk(result+increment) == -1) {
  result = (__u8 *)(uintptr_t)-1;
 }
#endif /* !__LIBC_HAVE_SBRK_THREADSAFE */
 return (void *)result;
}

#ifdef __LIBC_HAVE_SBRK_THREADSAFE
#undef BRK_ACQUIRE
#undef BRK_RELEASE
#endif /* __LIBC_HAVE_SBRK_THREADSAFE */
#endif /* __LIBC_HAVE_SBRK */
#endif /* !__KERNEL__ */


__public pid_t tcgetpgrp(int fd) {
 pid_t pgrp;
 if (ioctl(fd,TIOCGPGRP,&pgrp)) return -1;
 return pgrp;
}
__public int tcsetpgrp(int fd, pid_t pgrp) {
 return ioctl(fd,TIOCSPGRP,&pgrp);
}

__public int nice(int inc) {
 ktaskprio_t prio; kerrno_t error;
#ifdef __KERNEL__
 prio = ktask_getpriority(ktask_self());
 error = ktask_setpriority(ktask_self(),prio+inc);
 if __unlikely(KE_ISERR(error)) goto err;
#else
 error = ktask_getpriority(kproc_self(),&prio);
 if __unlikely(KE_ISERR(error)) goto err;
 error = ktask_setpriority(kproc_self(),prio+inc);
 if __unlikely(KE_ISERR(error)) goto err;
#endif
 return 0;
err:
#ifdef __KERNEL__
 return error;
#else
 __set_errno(-error);
 return -1;
#endif
}


#ifndef __KERNEL__
__local int
kexecargs_fromsentinel(struct kexecargs *self,
                       char const *arg,
                       va_list args, int has_envp) {
 char const **argv; va_list args_copy;
 size_t argc = 1;
 if __unlikely(!arg) {
  __set_errno(EINVAL);
  return -1;
 }
 va_copy(args_copy,args);
 while (va_arg(args_copy,char const *)) ++argc;
 va_end(args_copy);
 argv = (char const **)malloc((argc+1)*sizeof(char const *));
 if __unlikely(!argv) return -1;
 self->ea_argc = argc;
 self->ea_argv = (char const *const *)argv;
 *argv++ = arg;
 while ((*argv++ = va_arg(args,char const *)) != NULL);
 assert(argv == self->ea_argv+(argc+1));
 self->ea_arglenv = NULL;
 self->ea_envc    = has_envp ? (size_t)-1 : 0;
 self->ea_environ = has_envp ? va_arg(args,char const *const *) : NULL;
 self->ea_envlenv = NULL;
 return 0;
};

__public int _fexecl(int fd, char const *arg, ...) {
 struct kexecargs argvdat;
 va_list args; int error;
 va_start(args,arg);
 error = kexecargs_fromsentinel(&argvdat,arg,args,0);
 va_end(args);
 if __unlikely(error != 0) return error;
 error = ktask_fexec(fd,&argvdat,KTASK_EXEC_FLAG_NONE);
 assert(KE_ISERR(error));
 free((void *)argvdat.ea_argv);
 __set_errno(-error);
 return -1;
}
__public int _fexecle(int fd, char const *arg, .../*, char *const envp[]*/) {
 struct kexecargs argvdat;
 va_list args; int error;
 va_start(args,arg);
 error = kexecargs_fromsentinel(&argvdat,arg,args,1);
 va_end(args);
 if __unlikely(error != 0) return error;
 error = ktask_fexec(fd,&argvdat,KTASK_EXEC_FLAG_NONE);
 assert(KE_ISERR(error));
 free((void *)argvdat.ea_argv);
 __set_errno(-error);
 return -1;
}

__public int _fexecv(int fd, char *const argv[]) {
 kerrno_t error;
 struct kexecargs argvdat;
 memset(&argvdat,0,sizeof(argvdat));
 argvdat.ea_argc = (size_t)-1;
 argvdat.ea_argv = (char const *const *)argv;
 error = ktask_fexec(fd,&argvdat,KTASK_EXEC_FLAG_NONE);
 assert(KE_ISERR(error));
 __set_errno(-error);
 return -1;
}
__public int _fexecve(int fd, char *const argv[], char *const envp[]) {
 kerrno_t error;
 struct kexecargs argvdat;
 memset(&argvdat,0,sizeof(argvdat));
 argvdat.ea_argc = (size_t)-1;
 argvdat.ea_argv = (char const *const *)argv;
 argvdat.ea_envc = (size_t)-1;
 argvdat.ea_environ = (char const *const *)envp;
 error = ktask_fexec(fd,&argvdat,KTASK_EXEC_FLAG_NONE);
 assert(KE_ISERR(error));
 __set_errno(-error);
 return -1;
}


__public int execl(char const *path, char const *arg, ...) {
 struct kexecargs argvdat;
 va_list args; int error;
 va_start(args,arg);
 error = kexecargs_fromsentinel(&argvdat,arg,args,0);
 va_end(args);
 if __unlikely(error != 0) return error;
 error = ktask_exec(path,(size_t)-1,&argvdat,KTASK_EXEC_FLAG_NONE);
 assert(KE_ISERR(error));
 free((void *)argvdat.ea_argv);
 __set_errno(-error);
 return -1;
}
__public int execlp(char const *file, char const *arg, ...) {
 struct kexecargs argvdat;
 va_list args; int error;
 va_start(args,arg);
 error = kexecargs_fromsentinel(&argvdat,arg,args,0);
 va_end(args);
 if __unlikely(error != 0) return error;
 error = ktask_exec(file,(size_t)-1,&argvdat,KTASK_EXEC_FLAG_SEARCHPATH);
 assert(KE_ISERR(error));
 free((void *)argvdat.ea_argv);
 __set_errno(-error);
 return -1;
}

__public int execle(char const *path, char const *arg, .../*, char *const envp[]*/) {
 struct kexecargs argvdat;
 va_list args; int error;
 va_start(args,arg);
 error = kexecargs_fromsentinel(&argvdat,arg,args,1);
 va_end(args);
 if __unlikely(error != 0) return error;
 error = ktask_exec(path,(size_t)-1,&argvdat,KTASK_EXEC_FLAG_NONE);
 assert(KE_ISERR(error));
 free((void *)argvdat.ea_argv);
 __set_errno(-error);
 return -1;
}
__public int execlpe(char const *file, char const *arg, .../*, char *const envp[]*/) {
 struct kexecargs argvdat;
 va_list args; int error;
 va_start(args,arg);
 error = kexecargs_fromsentinel(&argvdat,arg,args,1);
 va_end(args);
 if __unlikely(error != 0) return error;
 error = ktask_exec(file,(size_t)-1,&argvdat,KTASK_EXEC_FLAG_SEARCHPATH);
 assert(KE_ISERR(error));
 free((void *)argvdat.ea_argv);
 __set_errno(-error);
 return -1;
}
__public int execv(char const *path, char *const argv[]) {
 kerrno_t error;
 struct kexecargs argvdat;
 memset(&argvdat,0,sizeof(argvdat));
 argvdat.ea_argc = (size_t)-1;
 argvdat.ea_argv = (char const *const *)argv;
 error = ktask_exec(path,(size_t)-1,&argvdat,KTASK_EXEC_FLAG_NONE);
 assert(KE_ISERR(error));
 __set_errno(-error);
 return -1;
}
__public int execvp(char const *file, char *const argv[]) {
 kerrno_t error;
 struct kexecargs argvdat;
 memset(&argvdat,0,sizeof(argvdat));
 argvdat.ea_argc = (size_t)-1;
 argvdat.ea_argv = (char const *const *)argv;
 error = ktask_exec(file,(size_t)-1,&argvdat,KTASK_EXEC_FLAG_SEARCHPATH);
 assert(KE_ISERR(error));
 __set_errno(-error);
 return -1;
}
__public int execve(char const *path, char *const argv[], char *const envp[]) {
 kerrno_t error;
 struct kexecargs argvdat;
 memset(&argvdat,0,sizeof(argvdat));
 argvdat.ea_argc = (size_t)-1;
 argvdat.ea_argv = (char const *const *)argv;
 argvdat.ea_envc = (size_t)-1;
 argvdat.ea_environ = (char const *const *)envp;
 error = ktask_exec(path,(size_t)-1,&argvdat,
                    KTASK_EXEC_FLAG_NONE);
 assert(KE_ISERR(error));
 __set_errno(-error);
 return -1;
}
__public int execvpe(char const *file, char *const argv[], char *const envp[]) {
 kerrno_t error;
 struct kexecargs argvdat;
 memset(&argvdat,0,sizeof(argvdat));
 argvdat.ea_argc = (size_t)-1;
 argvdat.ea_argv = (char const *const *)argv;
 argvdat.ea_envc = (size_t)-1;
 argvdat.ea_environ = (char const *const *)envp;
 error = ktask_exec(file,(size_t)-1,&argvdat,
                    KTASK_EXEC_FLAG_SEARCHPATH);
 assert(KE_ISERR(error));
 __set_errno(-error);
 return -1;
}

#endif
#endif /* !__CONFIG_MIN_LIBC__ */


__DECL_END

#endif /* !__UNISTD_C__ */

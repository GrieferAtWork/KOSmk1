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
#ifndef __DIRENT_C__
#define __DIRENT_C__ 1

#include <dirent.h>
#include <kos/compiler.h>
#include <kos/config.h>
#ifndef __CONFIG_MIN_LIBC__
#include <errno.h>
#include <fcntl.h>
#include <kos/fd.h>
#include <kos/kernel/debug.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#if __DIRENT_HAVE_LONGENT
#include <stddef.h>
#endif /* __DIRENT_HAVE_LONGENT */

__DECL_BEGIN

__public int closedir(DIR *dirp) {
 int error;
#ifndef __KERNEL__
 if __unlikely(!dirp) {
  __set_errno(EBADF);
  return -1;
 }
#endif
 kassertobj(dirp);
#if __DIRENT_HAVE_LONGENT
 if (dirp->__d_longent != &dirp->__d_ent) {
  free(dirp->__d_longent);
 }
#endif /* __DIRENT_HAVE_LONGENT */
 error = close(dirp->__d_fd);
 free(dirp);
 return error;
}
__public DIR *opendir(char const *dirname) {
 DIR *result; int fd;
 fd = open(dirname,O_DIRECTORY|O_RDONLY);
 if __unlikely(fd == -1) return NULL;
 result = fdopendir(fd);
 if __unlikely(!result) kfd_close(fd);
 return result;
}
__public DIR *fdopendir(int fd) {
 DIR *result;
#ifndef __KERNEL__
 if __unlikely(fd < 0 && !KFD_ISSPECIAL(fd)) {
  __set_errno(EBADF);
  return NULL;
 }
#endif
 if __unlikely((result = omalloc(DIR)) == NULL) return NULL;
 result->__d_fd = fd;
#if __DIRENT_HAVE_LONGENT
 result->__d_longent = &result->__d_ent;
 result->__d_longsz = sizeof(result->__d_ent);
#endif /* __DIRENT_HAVE_LONGENT */
 return result;
}
#if __DIRENT_HAVE_LONGENT
__public struct dirent *readdir(DIR *dirp) {
 struct kfddirent kdent; kerrno_t error;
 size_t bufsize; struct dirent *result;
#ifdef __KERNEL__
 kassertobj(dirp);
#else
 if __unlikely(!dirp) { __set_errno(EBADF); return NULL; }
#endif
 bufsize = offsetafter(struct dirent,d_namlen)-dirp->__d_longsz;
again:
 result = dirp->__d_longent;
 kdent.kd_namev = result->d_name;
 kdent.kd_namec = bufsize;
 /* Read in perfect peeking mode:
  *  - Only advance the directory if the given buffer had sufficient size
  * >> In any buffer-related case, the kernel will store the required
  *    size within the 'kdent.kd_namec' field, allowing us to allocate
  *    a bigger buffer when our current was too small.
  *    If it wasn't we don't need to do a second system-call, as
  *    the kernel has already auto-magically advanced the directory. */
 error = kfd_readdir(dirp->__d_fd,&kdent,
                     KFD_READDIR_FLAG_INO|KFD_READDIR_FLAG_KIND|
                     KFD_READDIR_FLAG_PEEK|KFD_READDIR_FLAG_PERFECT);
 if __unlikely(error == KS_EMPTY) return NULL; /* End of directory (NOTE: This doesn't set errno) */
 if __unlikely(KE_ISERR(error)) {end_err: __set_errno(-error); return NULL; }
 if (kdent.kd_namec > bufsize) {
  struct dirent *new_long_ent;
  /* Must (re-)allocate a bigger long directory entry. */
  bufsize = offsetafter(struct dirent,d_namlen)+kdent.kd_namec;
  dirp->__d_longsz = bufsize;
  /* Use the required buffersize, as indicated by the kernel. */
  new_long_ent = (result == &dirp->__d_ent)
   ? (struct dirent *)malloc(bufsize)
   : (struct dirent *)realloc(result,bufsize);
  if __unlikely(!new_long_ent) { error = KE_NOMEM; goto end_err; }
  dirp->__d_longent = new_long_ent;
  /* Try again with the bigger buffer. */
  goto again;
 }
 result->d_ino    = kdent.kd_ino;
 result->d_type   = IFTODT(kdent.kd_mode);
 result->d_namlen = (kdent.kd_namec/sizeof(char))-1;
 return result;
}
__public struct dirent *_readdir_short(DIR *dirp)
#else /* __DIRENT_HAVE_LONGENT */
__public struct dirent *readdir(DIR *dirp)
#endif /* !__DIRENT_HAVE_LONGENT */
{
 struct dirent *result; int error;
#ifdef __KERNEL__
 kassertobj(dirp);
#else
 if __unlikely(!dirp) {
  __set_errno(EBADF);
  return NULL;
 }
#endif
 /* Read a short directory entry (truncating long filenames). */
 error = readdir_r(dirp,&dirp->__d_ent,&result);
 if __unlikely(error != 0) {
  __set_errno(error);
  result = NULL;
 }
 return result;
}

__public int readdir_r(DIR *dirp, struct dirent *entry,
                       struct dirent **result) {
 struct kfddirent kdent; kerrno_t error;
#ifdef __KERNEL__
 kassertobj(dirp);
#else
 if __unlikely(!dirp) {
  __set_errno(EBADF);
  return -1;
 }
#endif
 kassertobj(entry);
 kassertobj(result);
 kdent.kd_namev = entry->d_name;
 kdent.kd_namec = sizeof(entry->d_name);
 error = kfd_readdir(dirp->__d_fd,&kdent,KFD_READDIR_FLAG_INO|KFD_READDIR_FLAG_KIND);
 if __unlikely(error == KS_EMPTY) { *result = NULL; return 0; } /* End of directory. */
 if __unlikely(KE_ISERR(error)) return -error;
 entry->d_ino = kdent.kd_ino;
 entry->d_type = IFTODT(kdent.kd_mode);
 /* Make sure it's always zero-terminated (possibly truncating the filename...). */
 entry->d_name[NAME_MAX] = '\0';
 entry->d_namlen = (kdent.kd_namec/sizeof(char))-1;
 *result = entry;
 return 0;
}
__public void rewinddir(DIR *dirp) {
#ifdef __KERNEL__
 kassertobj(dirp);
#else
 if __unlikely(!dirp) return;
#endif
 seekdir(dirp,0);
}
__public void seekdir(DIR *dirp, long loc) {
#ifdef __KERNEL__
 kassertobj(dirp);
#else
 if __unlikely(!dirp) return;
#endif
 kfd_seek(dirp->__d_fd,KFD_POSITION(loc),SEEK_SET,NULL);
}
__public long telldir(DIR *dirp) {
 __u64 result; kerrno_t error;
#ifdef __KERNEL__
 kassertobj(dirp);
#else
 if __unlikely(!dirp) {
  __set_errno(EBADF);
  return -1;
 }
#endif
 error = kfd_seek(dirp->__d_fd,KFD_POSITION(0),SEEK_CUR,&result);
 if __unlikely(KE_ISERR(error)) {
#ifdef __KERNEL__
  return (long)error;
#else
  __set_errno(-error);
  return -1;
#endif
 }
 return (long)result;
}
__public int dirfd(DIR *dirp) {
#ifdef __KERNEL__
 kassertobj(dirp);
#else
 if __unlikely(!dirp) return -1;
#endif
 return dirp->__d_fd;
}

__DECL_END
#endif /* !__CONFIG_MIN_LIBC__ */

#endif /* !__DIRENT_C__ */

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
#ifndef __SYS_STAT_H__
#define __SYS_STAT_H__ 1

#include <kos/compiler.h>
#include <kos/types.h>
#include <kos/timespec.h>

__DECL_BEGIN

#ifndef __dev_t_defined
#define __dev_t_defined 1
typedef __dev_t dev_t;
#endif

#ifndef __ino_t_defined
#define __ino_t_defined 1
typedef __ino_t ino_t;
#endif

#ifndef __mode_t_defined
#define __mode_t_defined 1
typedef __mode_t mode_t;
#endif

#ifndef __nlink_t_defined
#define __nlink_t_defined 1
typedef __nlink_t nlink_t;
#endif

#ifndef __uid_t_defined
#define __uid_t_defined 1
typedef __uid_t uid_t;
#endif

#ifndef __gid_t_defined
#define __gid_t_defined 1
typedef __gid_t gid_t;
#endif

#ifndef __off_t_defined
#define __off_t_defined 1
typedef __off_t off_t;
#endif

#ifndef __blksize_t_defined
#define __blksize_t_defined 1
typedef __blksize_t blksize_t;
#endif

#ifndef __blkcnt_t_defined
#define __blkcnt_t_defined 1
typedef __blkcnt_t blkcnt_t;
#endif

#ifndef __time_t_defined
#define __time_t_defined 1
typedef __time_t time_t;
#endif

#define __STAT_MEMBERS(bits) \
 __dev_t             st_dev;     /* ID of device containing file. */\
 __ino_t             st_ino;     /* inode number. */\
 __mode_t            st_mode;    /* protection. */\
 __nlink_t           st_nlink;   /* number of hard links. */\
 __uid_t             st_uid;     /* user ID of owner. */\
 __gid_t             st_gid;     /* group ID of owner. */\
 __dev_t             st_rdev;    /* device ID (if special file). */\
 __off##bits##_t     st_size;    /* total size, in bytes. */\
 __blksize##bits##_t st_blksize; /* blocksize for file system I/O. */\
 __blkcnt##bits##_t  st_blocks;  /* number of 512B blocks allocated. */\
 union{ __time_t     st_atime;   /* time of last access. */\
 struct timespec     st_atim;    /* *ditto* */};\
 union{ __time_t     st_mtime;   /* time of last modification. */\
 struct timespec     st_mtim;    /* *ditto* */};\
 union{ __time_t     st_ctime;   /* time of file creation. */\
 struct timespec     st_ctim;    /* *ditto* */};
struct stat32 { __STAT_MEMBERS(32) };
struct stat64 { __STAT_MEMBERS(64) };
struct stat   { __STAT_MEMBERS(  ) };
#undef __STAT_MEMBERS

extern int stat    __P((char const *__path, struct stat *__st));
extern int stat32  __P((char const *__path, struct stat32 *__st));
extern int stat64  __P((char const *__path, struct stat64 *__st));
extern int lstat   __P((char const *__path, struct stat *__st));
extern int lstat32 __P((char const *__path, struct stat32 *__st));
extern int lstat64 __P((char const *__path, struct stat64 *__st));
extern int fstat   __P((int __fd, struct stat *__st));
extern int fstat32 __P((int __fd, struct stat32 *__st));
extern int fstat64 __P((int __fd, struct stat64 *__st));

extern int mkdir __P((const char *__pathname, mode_t __mode));
extern int mkdirat __P((int __dirfd, const char *__pathname, mode_t __mode));

#define S_IFMT   0170000
#define S_IFDIR  0040000 /*< Directory. */
#define S_IFCHR  0020000 /*< Character device. */
#define S_IFBLK  0060000 /*< Block device. */
#define S_IFREG  0100000 /*< Regular file. */
#define S_IFIFO  0010000 /*< FIFO. */
#define S_IFLNK  0120000 /*< Symbolic link. */
#define S_IFSOCK 0140000 /*< Socket. */

#define S_ISREG(m)  (((m)&S_IFMT)==S_IFREG)  /*< Regular file. */
#define S_ISDIR(m)  (((m)&S_IFMT)==S_IFDIR)  /*< Directory. */
#define S_ISCHR(m)  (((m)&S_IFMT)==S_IFCHR)  /*< Character device. */
#define S_ISBLK(m)  (((m)&S_IFMT)==S_IFBLK)  /*< Block device. */
#define S_ISFIFO(m) (((m)&S_IFMT)==S_IFIFO)  /*< FIFO. */
#define S_ISLNK(m)  (((m)&S_IFMT)==S_IFLNK)  /*< Symbolic link. */
#define S_ISSOCK(m) (((m)&S_IFMT)==S_IFSOCK) /*< Socket. */

#define S_IRUSR  0000400 /*< Read by owner. */
#define S_IWUSR  0000200 /*< Write by owner. */
#define S_IXUSR  0000100 /*< Execute by owner. */
#define S_IRWXU (S_IRUSR|S_IWUSR|S_IXUSR)
#define S_IRGRP  0000040 /*< Read by group. */
#define S_IWGRP  0000020 /*< Write by group. */
#define S_IXGRP  0000010 /*< Execute by group. */
#define S_IRWXG (S_IRGRP|S_IWGRP|S_IXGRP)
#define S_IROTH  0000004 /*< Read by others. */
#define S_IWOTH  0000002 /*< Write by others. */
#define S_IXOTH  0000001 /*< Execute by others. */
#define S_IRWXO (S_IROTH|S_IWOTH|S_IXOTH)

#define S_ISUID  0004000 /*< Set-user-ID on execute bit. */
#define S_ISGID  0002000 /*< Set-group-ID on execute bit. */
#define S_ISVTX  0001000 /*< Sticky bit. */

#define S_IREAD  S_IRUSR
#define S_IWRITE S_IWUSR
#define S_IEXEC  S_IXUSR


#ifndef __STDC_PURE__
// Visual studio defines these kind of macros (for some reason...)
#define _S_IFMT   S_IFMT
#define _S_IFDIR  S_IFDIR
#define _S_IFCHR  S_IFCHR
#define _S_IFBLK  S_IFBLK
#define _S_IFREG  S_IFREG
#define _S_IFIFO  S_IFIFO
#define _S_IFLNK  S_IFLNK
#define _S_IFSOCK S_IFSOCK
#endif

__DECL_END

#endif /* !__SYS_STAT_H__ */

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
#ifndef __FCNTL_H__
#ifndef _FCNTL_H
#ifndef _INC_FCNTL
#define __FCNTL_H__ 1
#define _FCNTL_H 1
#define _INC_FCNTL 1

#include <kos/compiler.h>
#include <kos/types.h>
#include <kos/attr.h>
#include <features.h>

__DECL_BEGIN

#ifndef SEEK_SET
#   define SEEK_SET 0
#endif
#ifndef SEEK_CUR
#   define SEEK_CUR 1
#endif
#ifndef SEEK_END
#   define SEEK_END 2
#endif

#define FD_CLOEXEC 0x0001 /*< Close the fd during a call to 'exec' (s.a.: FD_CLOEXEC/O_CLOEXEC). */

#define O_ACCMODE   00000003 /* OK */
#define O_RDONLY    00000000 /* OK */
#define O_WRONLY    00000001 /* OK */
#define O_RDWR      00000002 /* OK */
#define O_CREAT     00000100 /* OK */
#define O_EXCL      00000200 /* OK */
#define O_NOCTTY    00000400
#define O_TRUNC     00001000 /* OK */
#define O_APPEND    00002000 /* OK */
#define O_NONBLOCK  00004000
#define O_SYNC      00010000
#define O_ASYNC     00020000
#define O_DIRECT    00040000
#define O_LARGEFILE 00100000 /* OK? (all files are in KOS...) */
#define O_DIRECTORY 00200000 /* OK */
#define O_NOFOLLOW  00400000
#define O_NOATIME   01000000 /* OK */
#define O_CLOEXEC   02000000 /* OK */
#define _O_SYMLINK  04000000 /*< Open a symbolic link instead of dereferencing it. */
#define O_NDELAY    O_NONBLOCK


#define AT_FDCWD  (-100) /*< For binary compatibility w/ linux (which has this set to -100 as well). */
#define AT_SYMLINK_NOFOLLOW 0x100
#define AT_REMOVEDIR        0x200
#define AT_SYMLINK_FOLLOW   0x400
#define AT_EACCESS          0x200



// NOTE: Constants and structures below are taken from, and
//       compatible with '/usr/include/i386-linux-gnu/bits/fcntl.h'
#define F_DUPFD            0 /* Duplicate file descriptor. */
#define F_GETFD            1 /* Get file descriptor flags. */
#define F_SETFD            2 /* Set file descriptor flags. */
#define F_GETFL            3 /* TODO: Get file status flags. */
#define F_SETFL            4 /* TODO: Set file status flags. */
#define F_GETLK32          5 /* TODO: Get record locking info.  */
#define F_SETLK32          6 /* TODO: Set record locking info (non-blocking).  */
#define F_SETLKW32         7 /* TODO: Set record locking info (blocking). */
#define F_SETOWN           8 /* TODO: Get owner (process receiving SIGIO).  */
#define F_GETOWN           9 /* TODO: Set owner (process receiving SIGIO).  */
#define F_SETSIG          10 /* TODO: Set number of signal to be sent.  */
#define F_GETSIG          11 /* TODO: Get number of signal to be sent.  */
#define F_GETLK64         12 /* TODO: Get record locking info.  */
#define F_SETLK64         13 /* TODO: Set record locking info (non-blocking).  */
#define F_SETLKW64        14 /* TODO: Set record locking info (blocking). */
#define F_GETOWN_EX       15 /* TODO: Get owner (thread receiving SIGIO). */
#define F_SETOWN_EX       16 /* TODO: Set owner (thread receiving SIGIO). */
#define F_SETLEASE      1024 /* TODO: Set a lease. */
#define F_GETLEASE      1025 /* TODO: Enquire what lease is active. */
#define F_NOTIFY        1026 /* TODO: Request notifications on a directory. */
#define F_SETPIPE_SZ    1031 /* TODO: Set pipe page size array. */
#define F_GETPIPE_SZ    1032 /* TODO: Set pipe page size array. */
#define F_DUPFD_CLOEXEC 1030 /* Duplicate file descriptor with close-on-exit set. */

#define _F_GETYPE      10000 /* [KOS] Returns the type of file descriptor (one of 'KFDTYPE_*'). */

#ifdef __USE_FILE_OFFSET64
#   define F_SETLK   F_GETLK64
#   define F_SETLKW  F_SETLK64
#   define F_GETLK   F_SETLKW64
#else
#   define F_SETLK   F_GETLK32
#   define F_SETLKW  F_SETLK32
#   define F_GETLK   F_SETLKW32
#endif

#ifndef __ASSEMBLY__
#define __FLOCK_MEMBERS(bits) \
    short int       l_type;   /* Type of lock: F_RDLCK, F_WRLCK, or F_UNLCK. */\
    short int       l_whence; /* Where `l_start' is relative to (like `lseek'). */\
    __off##bits##_t l_start;  /* Offset where the lock begins. */\
    __off##bits##_t l_len;    /* Size of the locked area; zero means until EOF. */\
    __pid_t         l_pid;    /* Process holding the lock. */
struct flock32 { __FLOCK_MEMBERS(32) };
struct flock64 { __FLOCK_MEMBERS(64) };
struct flock   { __FLOCK_MEMBERS(  ) };
#undef __FLOCK_MEMBERS
#endif /* !__ASSEMBLY__ */

#define F_OWNER_TID  0            /* Kernel thread. */
#define F_OWNER_PID  1            /* Process. */
#define F_OWNER_PGRP 2            /* Process group. */
#define F_OWNER_GID  F_OWNER_PGRP /* Alternative, obsolete name. */
#ifndef __ASSEMBLY__
struct f_owner_ex {
    int     type;
    __pid_t pid;
};
#endif /* !__ASSEMBLY__ */

/* For posix fcntl() and `l_type' field of a `struct flock' for lockf(). */
#define F_RDLCK 0 /* Read lock. */
#define F_WRLCK 1 /* Write lock.*/
#define F_UNLCK 2 /* Remove lock. */

/* Types of directory notifications that may be requested with F_NOTIFY. */
#define DN_ACCESS    0x00000001 /* File accessed. */
#define DN_MODIFY    0x00000002 /* File modified. */
#define DN_CREATE    0x00000004 /* File created. */
#define DN_DELETE    0x00000008 /* File removed. */
#define DN_RENAME    0x00000010 /* File renamed. */
#define DN_ATTRIB    0x00000020 /* File changed attributes. */
#define DN_MULTISHOT 0x80000000 /* Don't remove notifier. */

#ifndef __ASSEMBLY__
#ifndef __CONFIG_MIN_LIBC__
extern int fcntl __P((int __fd, int __cmd, ...));
extern __nonnull((1)) int creat __P((char const *__restrict __path, __mode_t __mode));
extern __nonnull((1)) int open __P((char const *__restrict __path, int __flags, ...));

//////////////////////////////////////////////////////////////////////////
// Same as open, but the caller may specify the descriptor to open the file inside of
// >> Returns 'fd' on success and -1 on error
// HINT: This can be used to easily implement re-opening of files.
// NOTES:
//   - This is also a shortcut for implementing chroot:
//     >> open2(KFD_ROOT,"/new/root/path",O_RDONLY|O_DIRECTORY);
//   - The name was chosen based on the dup-dup2 pair of functions.
extern __nonnull((2)) int open2 __P((int __fd, char const *__restrict __path, int __flags, ...));

#ifdef __USE_ATFILE
extern __nonnull((2)) int openat __P((int __dirfd, char const *__restrict __path, int __flags, ...));
extern __nonnull((3)) int openat2 __P((int __fd, int __dirfd, char const *__restrict __path, int __flags, ...));
#endif
#endif /* !__CONFIG_MIN_LIBC__ */
#endif /* !__ASSEMBLY__ */



#ifndef __KERNEL__

//////////////////////////////////////////////////////////////////////////
// Compatibility defines for MSVC's <fcntl.h> header
#define _O_RDONLY       O_RDONLY
#define _O_WRONLY       O_WRONLY
#define _O_RDWR         O_RDWR
#define _O_APPEND       O_APPEND
#define _O_CREAT        O_CREAT
#define _O_TRUNC        O_TRUNC
#define _O_EXCL         O_EXCL
#define _O_TEXT         0
#define _O_BINARY       0
#define _O_WTEXT        0
#define _O_U16TEXT      0
#define _O_U8TEXT       0
#define _O_RAW          0
#define _O_NOINHERIT    O_CLOEXEC
#define _O_TEMPORARY    0
#define _O_SHORT_LIVED  0
#define _O_OBTAIN_DIR   O_DIRECTORY
#define _O_SEQUENTIAL   0
#define _O_RANDOM       0

#endif /* !__KERNEL__ */

__DECL_END

#endif /* !_INC_FCNTL */
#endif /* !_FCNTL_H */
#endif /* !__FCNTL_H__ */

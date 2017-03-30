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
#ifndef __UNISTD_H__
#ifndef _UNISTD_H
#define __UNISTD_H__ 1
#define _UNISTD_H 1

#include <__null.h>
#include <kos/compiler.h>
#ifndef __ASSEMBLY__
#include <getopt.h>
#include <kos/types.h>
#include <kos/attr.h>
#include <kos/sysconf.h>

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
#ifndef SEEK_DATA
#   define SEEK_DATA	3
#endif
#ifndef SEEK_HOLE
#   define SEEK_HOLE	4
#endif

#ifndef L_SET
#   define L_SET  SEEK_SET
#endif
#ifndef L_INCR
#   define L_INCR SEEK_CUR
#endif
#ifndef L_XTND
#   define L_XTND SEEK_END
#endif


#ifndef __ssize_t_defined
#define __ssize_t_defined 1
typedef __ssize_t ssize_t;
#endif

#ifndef __size_t_defined
#define __size_t_defined 1
typedef __size_t size_t;
#endif

#ifndef __off_t_defined
#define __off_t_defined 1
typedef __off_t off_t;
#endif

#ifndef __loff_t_defined
#define __loff_t_defined 1
typedef __loff_t loff_t;
#endif

#ifndef __off64_t_defined
#define __off64_t_defined 1
typedef __off64_t off64_t;
#endif

#ifndef __pid_t_defined
#define __pid_t_defined 1
typedef __pid_t pid_t;
#endif

#ifndef __useconds_t_defined
#define __useconds_t_defined 1
typedef __useconds_t useconds_t;
#endif

#ifndef __intptr_t_defined
#define __intptr_t_defined 1
typedef __intptr_t  intptr_t;
#endif

// Standard file descriptors.
#define	STDIN_FILENO  0 /*< Standard input. */
#define	STDOUT_FILENO 1 /*< Standard output. */
#define	STDERR_FILENO 2 /*< Standard error output. */


#define R_OK 4 /*< Test for read permission. */
#define W_OK 2 /*< Test for write permission. */
#define X_OK 1 /*< Test for execute permission. */
#define F_OK 0 /*< Test for existence. */

#undef PATH_MAX
#define PATH_MAX 256 /*< Not ~really~ a limit, but used as buffer size hint in many places. */


#ifndef __CONFIG_MIN_LIBC__

#ifndef __STDC_PURE__
#if defined(__INTELLISENSE__) || !defined(__NO_asmname)
extern __ssize_t getattr __P((int __fd, kattr_t __attr, void *__restrict __buf, __size_t __bufsize)) __asmname("_getattr");
extern int setattr __P((int __fd, kattr_t __attr, void const *__restrict __buf, __size_t __bufsize)) __asmname("_setattr");
#else /* !__NO_asmname */
#   define getattr _getattr
#   define setattr _setattr
#endif /* __NO_asmname */
#endif /* !__STDC_PURE__ */


extern int chdir __P((char const *__path));
extern int fchdir __P((int __fd));
extern char *getcwd __P((char *__restrict __buf, size_t __bufsize));
extern char *get_current_dir_name __P((void));

#ifndef __KERNEL__
#ifndef __STDC_PURE__
#ifndef __NO_asmname
extern __attribute_sentinel int fexecl __P((int __fd, char const *__arg, ...)) __asmname("_fexecl");
extern int fexecle __P((int __fd, char const *__arg, .../*, char *const __envp[]*/)) __asmname("_fexecle");
extern int fexecv __P((int __fd, char *const __argv[])) __asmname("_fexecv");
extern int fexecve __P((int __fd, char *const __argv[], char *const __envp[])) __asmname("_fexecve");
#else /* !__NO_asmname */
#   define fexecl  _fexecl
#   define fexecle _fexecle
#   define fexecv  _fexecv
#   define fexecve _fexecve
#endif /* __NO_asmname */
#endif /* !__STDC_PURE__ */
extern __attribute_sentinel int _fexecl __P((int __fd, char const *__arg, ...));
extern int _fexecle __P((int __fd, char const *__arg, .../*, char *const __envp[]*/));
extern int _fexecv __P((int __fd, char *const __argv[]));
extern int _fexecve __P((int __fd, char *const __argv[], char *const __envp[]));

extern __attribute_sentinel int execl __P((char const *__path, char const *__arg, ...));
extern __attribute_sentinel int execlp __P((char const *file, char const *__arg, ...));
extern int execle __P((char const *__path, char const *__arg, .../*, char *const __envp[]*/));
extern int execlpe __P((char const *file, char const *__arg, .../*, char *const __envp[]*/));
extern int execv __P((char const *__path, char *const __argv[]));
extern int execvp __P((char const *file, char *const __argv[]));
extern int execve __P((char const *__path, char *const __argv[], char *const __envp[]));
extern int execvpe __P((char const *file, char *const __argv[], char *const __envp[]));
#endif

extern unsigned int alarm __P((unsigned int __seconds));
extern __useconds_t ualarm __P((__useconds_t __value, __useconds_t __interval));
#ifndef __DOS_H__
extern unsigned int sleep __P((unsigned int __seconds));
#endif
extern int usleep __P((__useconds_t __useconds));
extern int pause __P((void));

extern int access __P((char const *__path, int __mode));
extern int faccessat __P((int dirfd, char const *__path, int __mode, int __flags));

extern int chown __P((char const *__path, __uid_t __owner, __gid_t __group));
extern int fchown __P((int __fd, __uid_t __owner, __gid_t __group));
extern int fchownat __P((int dirfd, char const *__path, __uid_t __owner, __gid_t __group, int __flags));
extern int lchown __P((char const *__path, __uid_t __owner, __gid_t __group));
extern int link __P((char const *target_name, char const *link_name));
extern int linkat __P((int __target_dirfd, char const *__target_name,
                       int __link_dirfd, char const *__link_name, int __flags));
extern ssize_t readlink __P((char const *__restrict __path, char *__restrict __buf, size_t __bufsize));
extern ssize_t readlinkat __P((int dirfd, char const *__restrict __path, char *__restrict __buf, size_t __bufsize));

/* On KOS you can open links like files, and therefor also read them through descriptors. */
extern ssize_t _freadlink __P((int __fd, char *__restrict __buf, size_t __bufsize));
#ifndef __STDC_PURE__
#ifndef __NO_asmname
extern ssize_t freadlink __P((int __fd, char *__restrict __buf, size_t __bufsize)) __asmname("_freadlink");
#else
#   define freadlink  _freadlink
#endif
#endif

#ifndef ___exit_defined
#define ___exit_defined 1
extern __noreturn void _exit __P((int __exitcode));
#endif /* !___exit_defined */

extern void sync __P((void));
extern int truncate __P((char const *__path, __off_t __newsize));
extern int rmdir __P((char const *__path));
extern int unlink __P((char const *__path));
extern int unlinkat __P((int dirfd, char const *__path, int __flags));

extern int pipe __P((int __pipefd[2]));
extern int pipe2 __P((int __pipefd[2], int __flags));

extern int close __P((int __fd));
extern __ssize_t read __P((int __fd, void *__restrict __buf, __size_t __bufsize));
extern __ssize_t write __P((int __fd, void const *__restrict __buf, __size_t __bufsize));
#ifndef __UNISTD_C__
extern __off_t lseek __P((int __fd, __off_t __off, int __whence));
extern __loff_t llseek __P((int __fd, __loff_t __off, int __whence));
#endif
extern __off64_t lseek64 __P((int __fd, __off64_t __off, int __whence));
extern __ssize_t _getattr __P((int __fd, kattr_t __attr, void *__restrict __buf, __size_t __bufsize));
extern int _setattr __P((int __fd, kattr_t __attr, void const *__restrict __buf, __size_t __bufsize));
extern int dup __P((int __fd));
extern int dup2 __P((int __fd, int __newfd));
extern int dup3 __P((int __fd, int __newfd, int __flags));
extern int fsync __P((int __fd));
extern int fdatasync __P((int __fd));
extern int ftruncate __P((int __fd, __off_t __newsize));
extern ssize_t pread __P((int __fd, void *__restrict __buf, __size_t __bufsize, __off_t __pos));
extern ssize_t pwrite __P((int __fd, const void *__restrict __buf, __size_t __bufsize, __off_t __pos));
extern ssize_t pread64 __P((int __fd, void *__restrict __buf, __size_t __bufsize, __off64_t __pos));
extern ssize_t pwrite64 __P((int __fd, const void *__restrict __buf, __size_t __bufsize, __off64_t __pos));

// Re-opens the KFD_ROOT descriptor at the given __path,
// trapping the calling process within that __path forever,
// unless a directory outside was opened before the call.
extern int chroot __P((char const *__path));

// Functions with parameters that make sense... (Ugh! why you no unsigned?)
extern int _ftruncate64 __P((int __fd, __pos64_t __newsize));
extern ssize_t _pread64 __P((int __fd, void *__restrict __buf, size_t __bufsize, __pos64_t __pos));
extern ssize_t _pwrite64 __P((int __fd, const void *__restrict __buf, size_t __bufsize, __pos64_t __pos));

#if defined(__KOS_HAVE_SYSCALL) || defined(__KOS_HAVE_UNIXSYSCALL)
// Unix System call (NOTE: If Unix sys-calls aren't supported, this is a KOS system call).
extern long syscall __P((long sysno, ...));
#endif

extern int getpagesize __P((void));
extern long sysconf __P((int __name));

// Process IDS and such...
extern __pid_t getpid __P((void)); /* Set current process id. */
extern __pid_t getppid __P((void)); /* Get parent process id. */

#ifndef __KERNEL__
extern __pid_t getpgrp __P((void)); /* Get level-2 barrier. */
extern __pid_t __getpgid __P((__pid_t __pid)); /* Get level-2 barrier. */
extern __pid_t getpgid __P((__pid_t __pid)); /* Get level-2 barrier. */
extern int setpgid __P((__pid_t __pid, __pid_t __pgid)); /* Only allowed for special self-self case. */
extern int setpgrp __P((void)); /* Set level-2 barrier. */

extern __pid_t setsid __P((void)); /* Set level-3 barrier. */
extern __pid_t getsid __P((__pid_t __pid)); /* Get level-3 barrier. */
#endif /* !__KERNEL__ */


#ifndef __KERNEL__
extern __pid_t fork __P((void));
#endif

// TTY Stuff...
extern pid_t tcgetpgrp(int fd);
extern int tcsetpgrp(int fd, pid_t pgrp);
extern int isatty __P((int __fd));
#ifndef __KERNEL__
extern char *ttyname __P((int __fd));
#endif /* !__KERNEL__ */
extern int ttyname_r __P((int __fd, char *__buf, size_t __buflen));


/* Returns the type of file descriptor that 'FD' is.
 * NOTE: Yes, the return type should technically be 'kfdtype_t'.
 * @return: * : One of 'KFDTYPE_NONE*'.
 */
extern int _fdtype __P((int __fd));

/* isatty-style helper functions to figure out if a given
 * file descriptor is actually a file (and not some other handle).
 * This function is _very_ helpful for sanitizing post-fork
 * environments before exec-ing some suspicious looking executable.
 * This is equivalent to 'fdtype(FD) == KFDTYPE_FILE', setting 'errno' to
 * 'EBADF' if the comparison fails and 'fdtype' didn't already return -1.
 * NOTE: A file-file-descriptor is anything that was originally created using 'open' or 'open2'
 * >> if (fork() == 0) {
 * >>   // Only leave the standard descriptors if they are actually files.
 * >>   if (!isafile(0)) close(0);
 * >>   if (!isafile(1)) close(1);
 * >>   if (!isafile(2)) close(2);
 * >>   closeall(3,INT_MAX);
 * >>   execl("/totally/legit/program","not","suspicious","at","all",(char *)NULL);
 * >>   _exec(EXIT_FAILURE);
 * >> }
 */
extern int _isafile __P((int __fd));

/* Closes all descriptors in a given low-high range (inclusive)
   Returns the total number of successfully closed descriptors. */
extern unsigned int _closeall __P((int __low, int __high));

#ifndef __STDC_PURE__
#ifndef __NO_asmname
extern int fdtype __P((int __fd)) __asmname("_fdtype");
extern int isafile __P((int __fd)) __asmname("_isafile");
extern unsigned int closeall __P((int __low, int __high)) __asmname("_closeall");
#else
#   define fdtype   _fdtype
#   define isafile  _isafile
#   define closeall _closeall
#endif
#endif


extern int nice __P((int __inc));

#ifndef __KERNEL__
extern char **environ;
#endif /* !__KERNEL__ */
#else /* !__CONFIG_MIN_LIBC__ */
/* TODO: Implement a hand full of functions above as
         macros, directly executing the system calls. */
#endif /* __CONFIG_MIN_LIBC__ */

#ifdef __LIBC_HAVE_SBRK
extern int brk __P((void *__addr));
extern void *sbrk __P((__intptr_t __increment));
#endif /* __LIBC_HAVE_SBRK */

#ifdef __KOS_HAVE_SYSCALL
extern long __kos_syscall __P((long __sysno, ...)); /*< KOS System call. */
#endif /* __KOS_HAVE_SYSCALL */
#ifdef __KOS_HAVE_NTSYSCALL
extern long __nt_syscall __P((long __sysno, ...)); /*< NT (windows) System call. */
#endif /* __KOS_HAVE_NTSYSCALL */
#ifdef __KOS_HAVE_UNIXSYSCALL
extern long __unix_syscall __P((long __sysno, ...)); /*< Unix System call. */
#endif /* __KOS_HAVE_UNIXSYSCALL */

#if defined(__GNUC__) && defined(__PP_VA_NARGS)
#ifndef __SYSCALL_VCONTR
#ifdef __REGISTER_ALLOCATOR_NO_OFFSET_FROM_EBP__
#   define __SYSCALL_VCONTR      "m" /* Work around GCC bug. */
#else
#   define __SYSCALL_VCONTR      "g"
#endif
#endif
#define __COMMON_SYSCALL_ANY(text,intno,...) \
 __xblock({ register long __result;\
            __asm_volatile__(text:"=a"(__result):"a"(intno),__VA_ARGS__:"memory");\
            __xreturn __result;\
 })
#define __COMMON_SYSCALL_SMALL(intno,...) \
 __COMMON_SYSCALL_ANY("int $0x69\n",intno,__VA_ARGS__)
#define __COMMON_SYSCALL_LARGE(text,size,intno,arg1,arg2,arg3,arg4,arg5,...) \
 __COMMON_SYSCALL_ANY(text "\nint $0x69\nadd $" #size ",%%esp\n",intno,"c"(arg1),"d"(arg2),"b"(arg3),"S"(arg4),"D"(arg5),__VA_ARGS__)
#define __PP_syscall1(intno) \
 __xblock({ register long __result;\
            __asm_volatile__("int $0x69\n":"=a"(__result):"a"(intno):"memory");\
            __xreturn __result;\
 })
#define __PP_syscall2(intno,arg1) __COMMON_SYSCALL_SMALL(intno,"c"(arg1))
#define __PP_syscall3(intno,arg1,arg2) __COMMON_SYSCALL_SMALL(intno,"c"(arg1),"d"(arg2))
#define __PP_syscall4(intno,arg1,arg2,arg3) __COMMON_SYSCALL_SMALL(intno,"c"(arg1),"d"(arg2),"b"(arg3))
#define __PP_syscall5(intno,arg1,arg2,arg3,arg4) __COMMON_SYSCALL_SMALL(intno,"c"(arg1),"d"(arg2),"b"(arg3),"S"(arg4))
#define __PP_syscall6(intno,arg1,arg2,arg3,arg4,arg5) __COMMON_SYSCALL_SMALL(intno,"c"(arg1),"d"(arg2),"b"(arg3),"S"(arg4),"D"(arg5))
#define __PP_syscall7(intno,arg1,arg2,arg3,arg4,arg5,arg6) __COMMON_SYSCALL_LARGE("pushl %7",4,intno,arg1,arg2,arg3,arg4,arg5,__SYSCALL_VCONTR(arg6))
#define __PP_syscall8(intno,arg1,arg2,arg3,arg4,arg5,arg6,arg7) __COMMON_SYSCALL_LARGE("pushl %8\npushl %7",8,intno,arg1,arg2,arg3,arg4,arg5,__SYSCALL_VCONTR(arg6),__SYSCALL_VCONTR(arg7))
#define __PP_syscallM(n,...) __PP_syscall##n(__VA_ARGS__)
#define __PP_syscallN(n,...) __PP_syscallM(n,__VA_ARGS__)
#define __PP_syscall(...) __PP_syscallN(__PP_VA_NARGS(__VA_ARGS__),__VA_ARGS__)
#endif


#if !defined(__INTELLISENSE__) && \
     defined(__PP_syscall) &&\
    !defined(__NO_INLINE_SYSCALL__)
/* Compile-time optimizations for known argument counts.
 * >> This way, we don't have to save all registers, as
 *    well as support syscalls that look at your EIP.
 * NOTE: A syscall() function that behaves the same
 *       way is still always exported by libc, though. */
#define syscall   __PP_syscall
#endif /* ... */

__DECL_END
#endif /* !__ASSEMBLY__ */

#endif /* !_UNISTD_H */
#endif /* !__UNISTD_H__ */

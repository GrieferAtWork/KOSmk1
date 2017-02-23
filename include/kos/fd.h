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
#ifndef __KOS_FD_H__
#define __KOS_FD_H__ 1

#include <kos/config.h>

#define __KFD_INTERNAL(a,b,c,d) \
 (-(((a)<<21)|((b)<<13)|((c)<<7)|(d)))

//////////////////////////////////////////////////////////////////////////
// Special, internal file descriptor for the root directory
// NOTE: While these additional descriptors can always
//       be used, they should be excluded in enumerations,
//       as every process has them.
// >> The root descriptor is used to implement chroot
//    and specifies the top directory visible to the process.
// WARNING: This value should only be changed after a fork and
//          before exec-ing a new process, as the prison you
//          are able to build with this is irreversible.
// WARNING: If this descriptor is set to that of a file,
//          all successive operations including a absolute
//          file path will fail with EINVAL/KE_INVAL
// WARNING: The user is responsible for ensuring that
//          for a perfect sandbox, all descriptors that
//          should be kept are pointing inside of the
//          root-sandbox generated by this, as the exec-ed
//          process will otherwise have a chance to escape:
//          >> #1: Find a parent-given fd that points outside of its root-trap
//                 NOTE: This is easier said, than done if you consider the following commandline:
//                 :~$ sandbox --root /opt/sandbox dangerzone > log.txt
//                 'log.txt' is an open descriptor, visible to 'dangerzone'
//                 and pointing to a file in your home folder.
//             #2: Using that fd, re-open the directory its file is located within.
//          - To prevent this security hole, use of a sandbox library is recommended.
//          - Another way to prevent this, is to not use the 'sandbox' command,
//            but set up the sandbox manually:
//            :~$ cp dangerzone /opt/sandbox/
//            :~$ chroot /opt/sandbox/
//            :~$ cd /
//            :/opt/sandbox/$ ./dangerzone > log.txt
//            Now, there is now way of pointing a handle (such as stdout) outside the sandbox.
#define KFD_ROOT      __KFD_INTERNAL('R','0','0','T')
#define KFD_CWD      (-100) /*< For binary compatibility w/ linux (which has this set to -100 as well). */

#define KFD_ISSPECIAL(x) ((x) == KFD_ROOT || (x) == KFD_CWD)

// Symbolic descriptors for the current task and process.
// NOTE: These are not real descriptors and therefor cannot be closed
//       or even used in a context other than the obvious of using
//       them for the system calls in <kos/task.h>
#define KFD_TASKSELF  __KFD_INTERNAL('T','A','S','K') /*< ktask_self(); */
#define KFD_TASKROOT  __KFD_INTERNAL('T','R','O','T') /*< ktask_proc(); */
#define KFD_PROCSELF  __KFD_INTERNAL('P','R','O','C') /*< kproc_self(); */

// Linux-style file descriptors for stdin, stdout and stderr
#define KFD_STDIN  0 /*< STDIN_FILENO. */
#define KFD_STDOUT 1 /*< STDOUT_FILENO. */
#define KFD_STDERR 2 /*< STDERR_FILENO. */

#ifndef KFD_POSITION
#if __SIZEOF_POINTER__ <= 4
#   include <kos/endian.h>
#   include <kos/arch.h>
#   include <stdint.h>
#   define KFD_HAVE_BIARG_POSITIONS
#   define KFD_POSITION_HI(x) (__u32)(((x)&UINT64_C(0xffffffff00000000))>>32)
#   define KFD_POSITION_LO(x)  (__u32)((x)&UINT64_C(0x00000000ffffffff))
#if (__BYTE_ORDER == __LITTLE_ENDIAN) == defined(KOS_ARCH_STACK_GROWS_DOWNWARDS)
#   define KFD_POSITION(x)      KFD_POSITION_HI(x),KFD_POSITION_LO(x)
#else /* linear: down */
#   define KFD_POSITION(x)      KFD_POSITION_LO(x),KFD_POSITION_HI(x)
#endif /* linear: up */
#else
#   define KFD_POSITION(x)      x
#endif
#endif /* !KFD_POSITION */

#ifndef SEEK_SET
#   define SEEK_SET 0
#endif
#ifndef SEEK_CUR
#   define SEEK_CUR 1
#endif
#ifndef SEEK_END
#   define SEEK_END 2
#endif

#ifndef __NO_PROTOTYPES
#include <kos/compiler.h>
#include <kos/syscall.h>
#include <kos/types.h>
#include <kos/errno.h>
#include <kos/attr.h>
#endif

__DECL_BEGIN

#ifndef __NO_PROTOTYPES

//////////////////////////////////////////////////////////////////////////
// Opens a new file.
//  - When kfd_open2 is used, any resource that previously occupied 'dfd' will be closed.
// @return: KE_ISOK(*): The file was opened and its descriptor was returned.
//                     'kfd_open' always returns a positive descriptor number
//                      on success, which then must be closed using 'kfd_close',
//                      or by duplicating a different descriptor onto this one
//                      through use of 'kfd_dup2', 'kfd_open2' or 'kproc_openfd2'.
// @return: dfd:      [kfd_open2] The file was opened and the given descriptor was applied.
// @return: KE_ACCES: The caller is not allowed to access the specified
//                    file or some part of the path leading up to it.
// @return: KE_FAULT: 'path' does not describe an addressable pointer.
// @return: KE_MFILE: The task context is configured to not allow descriptors numbers as high as 'dfd'.
// @return: KE_NOMEM: Not enough available system memory.
// @return: KE_OVERFLOW:  Failed to acquire a reference to 'path', 'cwd', or some
//                        porition of the filesystem tree represented by them.
// @return: KE_DESTROYED: The specified 'cwd' describes a directory that was removed
//                        at one point, leaving the associated descriptor to weakly
//                        reference a dead filesystem branch.
// @return: KE_EXIST:   'O_CREAT' and 'O_EXCL' were used, but the specified
// @return: KE_ISERR(*): File-specific error occurred during opening.
//                       [kfd_open2] NOTE: Make sure to check if 'dfd' is returned before
//                                         checking for this generic error condition.
//                                      >> If 'kfd_open2' was used to re-open a descriptor
//                                         such as KFD_ROOT, a negative value might be returned.
__local _syscall5(int,kfd_open,int,cwd,char const *,path,
                  __size_t,maxpath,__openmode_t,mode,__mode_t,perms);
__local _syscall6(kerrno_t,kfd_open2,int,dfd,int,cwd,char const *,path,
                  __size_t,maxpath,__openmode_t,mode,__mode_t,perms);

//////////////////////////////////////////////////////////////////////////
// Compare the resources references by two file descriptors for equality.
// WARNING: Do not expect this to work if you attempt to open some file
//          more than once, then call this function to see if they are
//          really describing the same open file.
//          This functions returns true for descriptors that have been
//          copied through duplication (kfd_dup/kfd_dup2) and will even
//          work for descriptors that may have went through multiple
//          different processes.
// @return: 1: The given file descriptors 'lhs' and 'rhs' are referencing the same resource
// @return: 0: The given file descriptors 'lhs' and 'rhs' are referencing different resources
__local _syscall2(int,kfd_equals,int,fda,int,fdb);

//////////////////////////////////////////////////////////////////////////
// Closes a given file descriptor, freeing up
// space and countering KE_MFILE errors.
// @return: KE_OK:   The specified descriptor was successfully closed.
// @return: KE_BADF: The given file descriptor was invalid.
// @return: * :      A Descriptor-specific error occurred during closing.
__local _syscall1(kerrno_t,kfd_close,int,fd);

//////////////////////////////////////////////////////////////////////////
// Closes all file descriptors inside the specified range (inclusive).
//  - Behaves equivalent to (but is much faster than):
// >> unsigned int kfd_closeall(int lo, int hi) {
// >>   unsigned int result = 0;
// >>   for (int i = lo; i <= hi; ++i) {
// >>     if (KE_ISOK(kfd_close(i))) ++result;
// >>   }
// >>   return result;
// >> }
//  - This function is very useful to close all descriptors
//    potentially posing security risks after a fork:
// >> if (fork() == 0) {
// >>   if (!isafile(0)) close(0);
// >>   if (!isafile(1)) close(1);
// >>   if (!isafile(2)) close(2);
// >>   kfd_closeall(3,INT_MAX); // Close all other descriptors
// >> }
// @return: *: The amount of successfully closed descriptors.
__local _syscall2(unsigned int,kfd_closeall,int,low,int,high);

//////////////////////////////////////////////////////////////////////////
// Read data from a file descriptor, such as a file
// @param: fd:      The descriptor to read data from.
// @param: buf:     A user-provided output buffer for raw data.
// @param: bufsize: The size of the user-provided buffer.
// @param: rsize:   A mandatory output parameter for the kernel to
//                  store amount of successfully read bytes inside of.
//               >> If this parameter is set to ZERO(0), it usually
//                  means that a buffer is empty, or that more commonly
//                  that the end of a file has been reached.
// @return: KE_BADF:     Invalid file descriptor.
// @return: KE_FAULT:    Invalid buf/rsize (NOTE: 'rsize' must always be non-NULL)
// @return: KE_OVERFLOW: Failed to acquire a reference to the descriptor due to an overflow.
// @return: KE_NOSYS:    The given descriptor does not support stream-based reading.
// @return: *:           Some descriptor-specific error code.
__local _syscall4(kerrno_t,kfd_read,int,fd,void *,buf,
                  __size_t,bufsize,__size_t *,rsize);

//////////////////////////////////////////////////////////////////////////
// Write data to a file descriptor, such as a file.
// @param: fd:      The descriptor to write data to.
// @param: buf:     A user-provided input buffer for raw data.
// @param: bufsize: The size of the user-provided buffer.
// @param: wsize:   A mandatory output parameter for the kernel to
//                  store amount of successfully written bytes inside of.
//               >> If this parameter is set to ZERO(0), it usually
//                  means that a limited-size buffer has been filled, or
//                  that your hard-disk is literally filled to the brim.
// @return: KE_BADF:     Invalid file descriptor.
// @return: KE_FAULT:    Invalid buf/wsize (NOTE: 'wsize' must always be non-NULL)
// @return: KE_OVERFLOW: Failed to acquire a reference to the descriptor due to an overflow.
// @return: KE_NOSYS:    The given descriptor does not support stream-based writing.
// @return: *:           Some descriptor-specific error code.
__local _syscall4(kerrno_t,kfd_write,int,fd,void const *,buf,
                  __size_t,bufsize,__size_t *,wsize);

//////////////////////////////////////////////////////////////////////////
// Safely flush unwritten data to disk.
// @param: fd: The descriptor to flush the data of.
// @return: KE_BADF:     Invalid file descriptor.
// @return: KE_OVERFLOW: Failed to acquire a reference to the descriptor due to an overflow.
// @return: KE_NOSYS:    The given descriptor does not support stream-based writing.
// @return: *:           Some descriptor-specific error code.
__local _syscall1(kerrno_t,kfd_flush,int,fd);

//////////////////////////////////////////////////////////////////////////
// Perform a rare operation on a given file descriptor handle itself.
//  - Recognized arguments for 'cmd' can be found in '<fcntl.h>'
//  - The 'arg' parameter is only used for some descriptor operation.
//  - It is recommended to use the 'fcntl' command from '<fcntl.h>' instead.
// @param: fd:  The descriptor to perform the command on.
// @param: cmd: The command to execute.
// @param: arg: Possibly a pointer, but may also be an integral value casted to a pointer.
//              Some fcntl commands may simply ignore this parameter.
// @return: *:          'cmd'-specific return value (may be negative and overlap
//                       with some error code, depending on what 'cmd' was specified.
// @return: KE_BADF:     Invalid file descriptor.
// @return: KE_FAULT:    An Invalid pointer was given.
// @return: KE_OVERFLOW: Failed to acquire a reference to the descriptor due to an overflow.
// @return: KE_NOSYS:    The given 'cmd' was not recognized, or isn't supported.
// @return: *:           Some descriptor-specific error code.
__local _syscall3(kerrno_t,kfd_fcntl,int,fd,int,cmd,void *,arg);

//////////////////////////////////////////////////////////////////////////
// Perform an io-command not fitting into any of the much
// more common descriptor categories, such as read/write.
// @param: fd:   The descriptor to perform the command on.
// @param: attr: The attribute number.
// @param: arg:  A pointer to a block of data to be used as an argument value.
// @return: KE_BADF:     Invalid file descriptor.
// @return: KE_FAULT:    'arg' is pointing to an invalid block of memory.
// @return: KE_OVERFLOW: Failed to acquire a reference to the descriptor due to an overflow.
// @return: KE_NOSYS:    The given 'cmd' was not recognized, isn't supported,
//                       or the descriptor doesn't support ioctl all-together.
// @return: *:           Some descriptor/attr-specific error code.
__local _syscall3(kerrno_t,kfd_ioctl,int,fd,kattr_t,attr,void *,arg);

//////////////////////////////////////////////////////////////////////////
// Get/Set the value of a given attribute.
//  - 'reqsize' may be specified as non-NULL, and will be filled with minimum
//    the amount of bytes required yield all data associated with the attribute.
//    Based on what attribute was requested, this value can always be used
//    to figure out how much data you were able to retrieve, or are missing out on.
//  - The expected/returned type of data is usually encoded into the attr-number,
//    and further detailed documentation can be found in <kos/attr.h>.
//  - This system call is also used to implement what is commonly known as getsockopt/setsockopt
//  - A very widely supported attribute is 'KATTR_GENERIC_NAME', that can be used
//    to retrieve a human-readable name associated with the data represented by 'fd'.
//    Support for this attribute is not mandatory though, as isn't support for any at all.
// @param: fd:      The descriptor to get/set attributes on.
// @param: attr:    The attribute number.
// @param: buf:     A user-provided input/output buffer for raw data.
// @param: bufsize: The size of the user-provided buffer.
// @param: reqsize: An optional output-parameter for the kernel to store the required size in.
// @return: KE_BADF:     Invalid file descriptor.
// @return: KE_FAULT:    'buf' or 'reqsize' is pointing to an invalid block of memory.
// @return: KE_OVERFLOW: Failed to acquire a reference to the descriptor due to an overflow.
// @return: KE_NOSYS:    The given 'attr' was not recognized, isn't supported,
//                       or the descriptor doesn't support getattr all-together.
// @return: *:           Some descriptor-specific error code.
__local _syscall5(kerrno_t,kfd_getattr,int,fd,kattr_t,attr,void *,buf,
                  __size_t,bufsize,__size_t *,reqsize);
__local _syscall4(kerrno_t,kfd_setattr,int,fd,kattr_t,attr,
                  void const *,buf,__size_t,bufsize);

// TODO: DOC
__local _syscall4(kerrno_t,kfd_readlink,int,fd,char *,buf,
                  __size_t,bufsize,__size_t *,reqsize);


//////////////////////////////////////////////////////////////////////////
// Duplicate a given file descriptor, returning the number of
// the new descriptor that may be used from now forth.
// @return: dfd:        [kfd_dup2] The given descriptor was duplicated.
// @return: KE_BADF:     Invalid file descriptor.
// @return: KE_MFILE:    The task context is configured to not allow descriptors numbers as high as 'dfd'.
// @return: KE_NOMEM:    Not enough available memory.
// @return: KE_ISERR(*): Some other, undocumented error.
// @return: *:           The duplicated descriptor that must be closed in a call to 'kfd_close'.
//                       NOTE: The descriptor number returned by 'kfd_dup' is always positive (>= 0).
__local _syscall2(int,kfd_dup,int,fd,int,flags);
__local _syscall3(int,kfd_dup2,int,fd,int,resfd,int,flags);

__struct_fwd(termios);
__struct_fwd(winsize);

// TODO: DOC
// NOTE: pipefd[0] is reader; pipefd[1] is writer.
__local _syscall2(kerrno_t,kfd_pipe,int *,pipefd,int,flags);
__local _syscall5(kerrno_t,kfd_openpty,int *,amaster,int *,aslave,char *,name,
                  struct termios const *,termp,struct winsize const *,winp);


//////////////////////////////////////////////////////////////////////////
// Acquire some kind of lock/ticket associated with the descriptor,
// or simply wait for some descriptor-specific event to happen.
//  - KFDTYPE_SEM:  Acquire a ticket to the semaphore
//  - KFDTYPE_MTX:  Lock the mutex
//  - KFDTYPE_TASK: Join the task (drop return value)
//  - KFDTYPE_PROC: Join the process's root task (drop return value)
// NOTES:
//  - Closing a descriptor after another task started waiting for it
//    will _NOT_ wake another task prematurely (it will continue waiting)
/*
__local _syscall1(kerrno_t,kfd_wait,int,self);
__local _syscall1(kerrno_t,kfd_trywait,int,self);
__local _syscall2(kerrno_t,kfd_timedwait,int,self,struct timespec const *,abstime);
*/

// Wait for the first event to happen in any of the specified descriptors.
// Returns the descriptor that was signaled on success, or a negative error on failure.
//  - Invalid/closed descriptors are ignored
//  - If no, or only invalid descriptors are provided, immediately return 'KE_BADF'
// TODO: __local _syscall2(int,kfd_waits,int const *,fdv,__size_t,fdc);
// TODO: __local _syscall2(int,kfd_trywaits,int const *,fdv,__size_t,fdc);
// TODO: __local _syscall3(int,kfd_timedwaits,int const *,fdv,__size_t,fdc,struct timespec const *,abstime);


// HINT: Use the 'KFD_POSITION' macro to pass position/size arguments
#ifdef KFD_HAVE_BIARG_POSITIONS
#if (__BYTE_ORDER == __LITTLE_ENDIAN) == defined(KOS_ARCH_STACK_GROWS_DOWNWARDS)
__local _syscall5(kerrno_t,kfd_seek,int,fd,__s32,offhi,__s32,offlo,int,whence,__u64 *,newpos);
__local _syscall6(kerrno_t,kfd_pread,int,fd,__u32,poshi,__u32,poslo,void *,buf,__size_t,bufsize,__size_t *,rsize);
__local _syscall6(kerrno_t,kfd_pwrite,int,fd,__u32,poshi,__u32,poslo,void const *,buf,__size_t,bufsize,__size_t *,wsize);
__local _syscall3(kerrno_t,kfd_trunc,int,fd,__u32,sizehi,__u32,sizelo);
#else /* linear: down */
__local _syscall5(kerrno_t,kfd_seek,int,fd,__s32,offlo,__s32,offhi,int,whence,__u64 *,newpos);
__local _syscall6(kerrno_t,kfd_pread,int,fd,__u32,poslo,__u32,poshi,void *,buf,__size_t,bufsize,__size_t *,rsize);
__local _syscall6(kerrno_t,kfd_pwrite,int,fd,__u32,poslo,__u32,poshi,void const *,buf,__size_t,bufsize,__size_t *,wsize);
__local _syscall3(kerrno_t,kfd_trunc,int,fd,__u32,sizelo,__u32,sizehi);
#endif /* linear: up */
#else
__local _syscall4(kerrno_t,kfd_seek,int,fd,__s64,off,int,whence,__u64 *,newpos);
__local _syscall5(kerrno_t,kfd_pread,int,fd,__u64,pos,void *,buf,__size_t,bufsize,__size_t *,rsize);
__local _syscall5(kerrno_t,kfd_pwrite,int,fd,__u64,pos,void const *,buf,__size_t,bufsize,__size_t *,wsize);
__local _syscall2(kerrno_t,kfd_trunc,int,fd,__u64,size);
#endif
#endif /* !__NO_PROTOTYPES */

#ifndef __ASSEMBLY__
struct kfddirent {
 // NOTE: If the 'kd_namev' buffer is too small, it will be filled with
 //       as many characters as possible kd_namec, possibly leaving
 //       it as a string without a terminating \0 character.
 // >> To safely get the full filename, call kfd_readdir twice
 //    with the 'KFD_READDIR_FLAG_PEEK' flag set the first time
 //    to figure out how big this buffer has to be.
 char    *kd_namev; /*< [1..1] Pointer to user-buffer for storing the filename. */
 __size_t kd_namec; /*< IN: Size of user-buffer for filename; OUT: required entry size (including terminating \0 character). */
 __ino_t  kd_ino;   /*< File INode number. */
 __mode_t kd_mode;  /*< File type and permissions. */
};
#endif /* !__ASSEMBLY__ */

#define KFD_READDIR_FLAG_NONE    0x00000000
#define KFD_READDIR_FLAG_PEEK    0x00000001 /*< Only retrieve the current entry. - don't advance the directory file. */
#define KFD_READDIR_FLAG_INO     0x00000002 /*< Fill 'kd_ino'. */
#define KFD_READDIR_FLAG_KIND    0x00000004 /*< Fill 'kd_mode' with file type information. */
#define KFD_READDIR_FLAG_PERM    0x00000008 /*< Fill 'kd_mode' with file permissions. */
#define KFD_READDIR_FLAG_PERFECT 0x00000010 /*< Only advance if the size of the provided name buffer was sufficient
                                               (Must be used in conjunction with 'KFD_READDIR_FLAG_PEEK'). */
#define KFD_READDIR_FLAG_MODE   (KFD_READDIR_FLAG_KIND|KFD_READDIR_FLAG_PERM)

#ifndef __NO_PROTOTYPES
//////////////////////////////////////////////////////////////////////////
// Read a directory entry from an open directory file stream.
__local _syscall3(kerrno_t,kfd_readdir,int,fd,struct kfddirent *,dent,__u32,flags);
#endif /* !__NO_PROTOTYPES */

#ifdef __KERNEL__
// Mask of flags passed to the internal readdir file operator.
#define KFD_READDIR_MASKINTERNAL \
 (KFD_READDIR_FLAG_PEEK)
#endif

#ifndef __ASSEMBLY__
#ifndef __kfdtype_t_defined
#define __kfdtype_t_defined 1
typedef __kfdtype_t kfdtype_t;
#endif
#endif /* !__ASSEMBLY__ */

#define KFDTYPE_NONE   0 /*< Invalid/Empty/Unused file descriptor. */
#define KFDTYPE_FILE   1 /*< [struct kfile] File descriptor. */
#define KFDTYPE_TASK   2 /*< [struct ktask] Task descriptor. */
#define KFDTYPE_PROC   3 /*< [struct kproc] Process (Task context) descriptor. */
#define KFDTYPE_INODE  4 /*< [struct kinode] INode descriptor. */
#define KFDTYPE_DIRENT 5 /*< [struct kdirent] Dirent descriptor. */
#define KFDTYPE_DEVICE 6 /*< [struct kdev] Device descriptor. */


__DECL_END

#endif /* !__KOS_FD_H__ */

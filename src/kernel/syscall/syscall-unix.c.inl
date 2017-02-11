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
#ifndef __KOS_KERNEL_SYSCALL_SYSCALL_UNIX_C_INL__
#define __KOS_KERNEL_SYSCALL_SYSCALL_UNIX_C_INL__ 1
#define __UNIX_SYSTEM_CALL

#include "syscall-common.h"
#include <kos/config.h>
#include <kos/kernel/task.h>
#include <linux/syscall.h>
#include <unistd.h>
#include <sys/types.h>
#ifdef __KOS_HAVE_UNIXSYSCALL

__DECL_BEGIN

static pid_t __unix_fork(struct kirq_registers *regs) {
 kerrno_t error; struct ktask *task;
 KTASK_CRIT_BEGIN_FIRST
 task = ktask_fork(NULL,&regs->regs);
 if __unlikely(!task) { error = KE_NOMEM; goto end; }
 error = ktask_resume_k(task);
 if __unlikely(KE_ISERR(error)) goto decentry;
 error = task->t_proc->p_pid;
decentry: ktask_decref(task);
end:
 KTASK_CRIT_END
 return error;
}

static ssize_t __unix_read(int fd, __user void *__restrict buf, size_t count) {
 ssize_t error; size_t rsize;
 if (count > SSIZE_MAX) count = SSIZE_MAX;
 KTASK_CRIT_BEGIN_FIRST
 if __unlikely(!kpagedir_ismappedu_rw(buf,count)) { error = KE_FAULT; goto end; }
 error = kproc_readfd(kproc_self(),fd,buf,count,&rsize);
 if __likely(KE_ISOK(error)) error = (ssize_t)rsize;
end:
 KTASK_CRIT_END
 return error;
}

static ssize_t __unix_write(int fd, __user void const *__restrict buf, size_t count) {
 ssize_t error; size_t rsize;
 if (count > SSIZE_MAX) count = SSIZE_MAX;
 KTASK_CRIT_BEGIN_FIRST
 if __unlikely(!kpagedir_ismappedu_ro(buf,count)) { error = KE_FAULT; goto end; }
 error = kproc_writefd(kproc_self(),fd,buf,count,&rsize);
 if __likely(KE_ISOK(error)) error = (ssize_t)rsize;
end:
 KTASK_CRIT_END
 return error;
}


void ksyscall_handler_unix(struct kirq_registers *regs) {
 switch (SYSID) {
  case SYS_exit:  RETURN(ktask_terminateex_k(ktask_proc(),(void *)A(int,0),KTASKOPFLAG_RECURSIVE)); break;
  case SYS_fork:  RETURN(__unix_fork(regs)); break;
  case SYS_read:  RETURN(__unix_read(A(int,0),A(__user void *,1),A(size_t,2))); break;
  case SYS_write: RETURN(__unix_write(A(int,0),A(__user void const *,1),A(size_t,2))); break;

#define SYS_open 5
#define SYS_close 6
#define SYS_waitpid 7
//#define SYS_creat 8
//#define SYS_link 9
#define SYS_unlink 10
#define SYS_execve 11
#define SYS_chdir 12
#define SYS_time 13
//#define SYS_mknod 14
#define SYS_chmod 15
//#define SYS_lchown 16
#define SYS_lseek 19
#define SYS_getpid 20
//#define SYS_mount 21
//#define SYS_umount 22
#define SYS_setuid 23
#define SYS_getuid 24
#define SYS_alarm 27
#define SYS_oldfstat 28
#define SYS_pause 29
#define SYS_utime 30
#define SYS_access 33
#define SYS_ftime 35
#define SYS_sync 36
#define SYS_kill 37
#define SYS_rename 38
#define SYS_mkdir 39
#define SYS_rmdir 40
#define SYS_dup 41
#define SYS_pipe 42
#define SYS_times 43
#define SYS_ioctl 54
#define SYS_fcntl 55
#define SYS_umask 60
#define SYS_chroot 61
#define SYS_ustat 62
#define SYS_dup2 63
#define SYS_getppid 64
#define SYS_sethostname 74
#define SYS_gettimeofday 78
#define SYS_settimeofday 79
//#define SYS_select 82
#define SYS_symlink 83
#define SYS_readlink 85
#define SYS_readdir 89
#define SYS_mmap 90
#define SYS_munmap 91
#define SYS_truncate 92
#define SYS_ftruncate 93
#define SYS_fchmod 94
#define SYS_fchown 95
#define SYS_getpriority 96
#define SYS_setpriority 97
#define SYS_stat 106
#define SYS_lstat 107
#define SYS_fstat 108
#define SYS_fsync 118
#define SYS_clone 120
#define SYS_fchdir 133
#define SYS__llseek 140
#define SYS_getdents 141
#define SYS_fdatasync 148

#define SYS_sched_yield 158
#define SYS_nanosleep 162
//maybe: #define SYS_mremap 163
#define SYS_pread64 180
#define SYS_pwrite64 181
#define SYS_chown 182
#define SYS_getcwd 183
#define SYS_vfork 190
#define SYS_truncate64 193
#define SYS_ftruncate64 194
#define SYS_stat64 195
#define SYS_lstat64 196
#define SYS_fstat64 197
#define SYS_fchown32 207
#define SYS_chown32 212
#define SYS_getdents64 220
#define SYS_fcntl64 221
#define SYS_gettid 224
//maybe: #define SYS_setxattr 226
//maybe: #define SYS_lsetxattr 227
//maybe: #define SYS_fsetxattr 228
//maybe: #define SYS_getxattr 229
//maybe: #define SYS_lgetxattr 230
//maybe: #define SYS_fgetxattr 231
//maybe: #define SYS_listxattr 232
//maybe: #define SYS_llistxattr 233
//maybe: #define SYS_flistxattr 234
//maybe: #define SYS_removexattr 235
//maybe: #define SYS_lremovexattr 236
//maybe: #define SYS_fremovexattr 237
#define SYS_tkill 238
#define SYS_utimes 271
#define SYS_waitid 284
#define SYS_openat 295
#define SYS_mkdirat 296
#define SYS_mknodat 297
#define SYS_fchownat 298
#define SYS_futimesat 299
#define SYS_fstatat64 300
#define SYS_unlinkat 301
#define SYS_renameat 302
#define SYS_linkat 303
#define SYS_symlinkat 304
#define SYS_readlinkat 305
#define SYS_fchmodat 306
#define SYS_faccessat 307
#define SYS_utimensat 320
#define SYS_dup3 330
#define SYS_pipe2 331
  default: break;
 }
}

__DECL_END

#endif /* __KOS_HAVE_UNIXSYSCALL */
#undef __UNIX_SYSTEM_CALL
#endif /* !__KOS_KERNEL_SYSCALL_SYSCALL_UNIX_C_INL__ */

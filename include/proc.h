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
#ifndef __PROC_H__
#define __PROC_H__ 1

#include <kos/config.h>
#ifndef __KERNEL__
#include <assert.h>
#include <errno.h>
#include <features.h>
#include <kos/errno.h>
#include <kos/compiler.h>
#include <kos/task.h>
#include <kos/task-tls.h>
#include <kos/perm.h>
#include <kos/types.h>

#ifdef __STDC_PURE__
#warning "This header is only available on KOS platforms."
#endif

//////////////////////////////////////////////////////////////////////////
// GLibc - style wrapper functions for KOS scheduler system calls.
// >> Functions behave the same as their <kos/task.h> counterparts,
//    though they are integrated into the errno system and return
//    -1 on error, and 0 (or some other, meaningful value) on success.
// >> Detailed documentations can be found in <kos/task.h>

// NOTE: 'task_t' and 'proc_t' returned by *open* functions must be
//       closed when no longer needed by use of 'close' from <unistd.h>

__DECL_BEGIN

// These will always remain as alias for int, so
// you can safely use 'int' instead of these types.
// >> The aliases are merely used to make it more clear what
//    types of descriptors are accepted/returned by functions below.
//    And remembed: '-1' is returned on error.
typedef int              task_t;
typedef int              proc_t;

typedef __ktaskprio_t    taskprio_t;
typedef __ksandbarrier_t sandbarrier_t;
typedef __ktid_t         tid_t;
typedef struct kperm     perm_t;
typedef kperm_name_t     perm_name_t;
typedef kperm_flag_t     perm_flag_t;

#ifndef __pid_t_defined
#define __pid_t_defined 1
typedef __pid_t pid_t;
#endif

struct timespec;

#ifdef __INTELLISENSE__
extern __wunused __constcall task_t task_self(void);
#else
#define task_self ktask_self
#endif

//////////////////////////////////////////////////////////////////////////
// Utility functions only affecting the calling task (thread)
extern               void task_yield __P((void));
extern __noreturn    void task_exit __P((void *__exitcode));
extern                int task_terminate __P((task_t __self, void *__exitcode));
extern                int task_suspend __P((task_t __self));
extern                int task_resume __P((task_t __self));
extern                int task_pause __P((task_t __self));
extern                int task_sleep __P((task_t __self, struct timespec *__restrict __timeout));
extern                int task_abssleep __P((task_t __self, struct timespec *__restrict __abstime));
extern                int task_setalarm __P((task_t __self, struct timespec const *__restrict __timeout));
extern __nonnull((2)) int task_getalarm __P((task_t __self, struct timespec *__restrict __timeout));
extern                int task_setabsalarm __P((task_t __self, struct timespec const *__restrict __abstime));
extern __nonnull((2)) int task_getabsalarm __P((task_t __self, struct timespec *__restrict __abstime));

extern __wunused    task_t task_openparent __P((task_t __self));
extern __wunused    task_t task_openroot __P((task_t __self));
extern __wunused    proc_t task_openproc __P((task_t __self));
extern __wunused  __size_t task_getparid __P((task_t __self));
extern __wunused     tid_t task_gettid __P((task_t __self));
extern __wunused    task_t task_openchild __P((task_t __self, __size_t __parid));
extern __wunused __ssize_t task_enumchildren __P((task_t __self, __size_t *__restrict __idv, __size_t __idc));
extern                 int task_getpriority __P((task_t __self, taskprio_t *__restrict __result));
extern                 int task_setpriority __P((task_t __self, taskprio_t __value));
extern                 int task_join __P((task_t __self, void **__exitcode));
extern                 int task_tryjoin __P((task_t __self, void **__exitcode));
extern __nonnull((2))  int task_timedjoin __P((task_t __self, struct timespec const *__restrict __abstime, void **__exitcode));
extern __nonnull((2))  int task_timoutjoin __P((task_t __self, struct timespec const *__restrict __timeout, void **__exitcode));
extern __wunused     char *task_getname __P((task_t __self, char *__restrict __buf, __size_t __bufsize)); /*< getcwd-style semantics (pass NULL,0 to return an malloc-ed string). */
extern __nonnull((2))  int task_setname __P((task_t __self, char const *__restrict __name));

#define TASK_NEWTHREAD_DEFAULT   KTASK_NEW_FLAG_NONE
#define TASK_NEWTHREAD_SUSPENDED KTASK_NEW_FLAG_SUSPENDED
typedef void *(*task_threadfunc) __P((void *closure));

extern __nonnull((1)) task_t task_newthread __P((task_threadfunc __thread_main,
                                                 void *__closure, __u32 __flags));

/* Returns non-ZERO child thread descriptor in parent; 0 in child process; -1 on error. */
/* '__flags' is a set of 'TASK_NEWTHREAD_*' __flags. */
/* NOTE: The returned thread is part of a different process, that
         can then be accessed through use of 'task_openproc'. */
extern __wunused task_t task_fork __P((void));
extern __wunused task_t task_forkex __P((__u32 __flags));

/* Performs a root-fork, safely creating a new process, as created with
 * 'KTASK_NEW_FLAG_JOIN|KTASK_NEW_FLAG_UNREACHABLE|KTASK_NEW_FLAG_ROOTFORK',
 * meaning that this function doesn't return until the child process has
 * terminated; the child process cannot be reached unless the calling process
 * already had access to the root process barrier (kproc_zero), and obviously
 * that when successful, the child process has root-permissions.
 * WARNING: This function must be implemented statically, or inline
 *          due to the way rootfork permissions are handled in KOS.
 *          If this was an actual library function, libc itself would
 *          need to have the SETUID bit set, meaning that anyone and everything
 *          would instantly have root access (and what's the point of that?)
 * @return:  0: You're the child task and have successfully acquired root-privileges.
 * @return:  1: You're the parent task and your child has (been) terminated.
 * @return: -1: An error occurred (Don't try to fool me; I don't just give anyone root)
 *              See 'errno' and 'ktask_fork' for further details.
 */
__local __wunused int task_rootfork(__uintptr_t *__exitcode) {
 kerrno_t error;
 /* Get rid of any set alarm to make sure we don't wake up before the root task terminates. */
 ktask_setalarm(ktask_self(),NULL,NULL);
 /* Do the root-fork! */
 error = ktask_fork(__exitcode,
                    KTASK_NEW_FLAG_JOIN|
                    KTASK_NEW_FLAG_UNREACHABLE|
                    KTASK_NEW_FLAG_ROOTFORK);
 if (error == KS_UNCHANGED) return 1; /* The child has finished, and we are the parent! */
 if (error == KE_OK) return 0; /* You're the child task and everything has worked out! */
 /* Well... Something went wrong! */
 assert(KE_ISERR(error));
 __set_errno(-error);
 return -1;
}


#ifdef __INTELLISENSE__
extern __wunused __constcall proc_t proc_self __P((void));
#else
#define proc_self  kproc_self
#endif
extern __noreturn void proc_exit __P((__uintptr_t __exitcode));

// NOTE: Process routines also found in tasks are simply recursive:
//     >> task_terminate(task_self(),...); // Only terminate the current thread
//     >> proc_terminate(proc_self(),...); // Also terminate all spawned threads
// HINT: Since both 'proc_t' and 'task_t' are descriptors,
//       the system call used to terminate a task vs. that
//       used to terminate a process is actually identical!
extern __wunused    task_t proc_openthread __P((proc_t __proc, tid_t __tid));
extern __wunused __ssize_t proc_enumthreads __P((proc_t __proc, tid_t *__restrict __tidv, __size_t __tidc));
extern __wunused       int proc_openfd __P((proc_t __proc, int __fd, int __flags));
extern __wunused       int proc_openfd2 __P((proc_t __proc, int __fd, int __newfd, int __flags));
extern __wunused __ssize_t proc_enumfd __P((proc_t __proc, int *__restrict __fdv, __size_t __fdc));
extern __wunused    proc_t proc_openpid __P((__pid_t __pid));
extern __wunused __ssize_t proc_enumpid __P((__pid_t *__restrict __pidv, __size_t __pidc));
extern __wunused   __pid_t proc_getpid __P((proc_t __proc));
extern __wunused    task_t proc_openroot __P((proc_t __proc));
extern                 int proc_barrier __P((sandbarrier_t __level));
extern __wunused    task_t proc_openbarrier __P((proc_t __proc, sandbarrier_t __level));
extern                 int proc_suspend __P((proc_t __proc));
extern                 int proc_resume __P((proc_t __proc));
extern                 int proc_terminate __P((proc_t __proc, __uintptr_t __exitcode));
extern                 int proc_join __P((proc_t __proc, __uintptr_t *__exitcode));
extern                 int proc_tryjoin __P((proc_t __proc, __uintptr_t *__exitcode));
extern __nonnull((2))  int proc_timedjoin __P((proc_t __proc, struct timespec const *__restrict __abstime, __uintptr_t *__restrict __exitcode));
extern __nonnull((2))  int proc_timeoutjoin __P((proc_t __proc, struct timespec const *__restrict __timeout, __uintptr_t *__restrict __exitcode));

//////////////////////////////////////////////////////////////////////////
// Return the state of a given permission flag.
// @return:  0: The flag is not set.
// @return:  1: The flag is set.
// @return: -1: An error occurred (see 'errno' and 'kproc_perm')
extern int proc_getflag __P((proc_t __proc, kperm_flag_t __flag));

//////////////////////////////////////////////////////////////////////////
// Delete the given permission flag(s) from the calling process.
// NOTES:
//   - 'proc_delflags' will ignore unknown flags.
//   - 'proc_delflags' behaves strnlen-style, terminating the
//     flag-list at a ZERO(0)-flag or when MAXFLAGS reaches ZERO(0).
// @return:  0: The operation was successful.
// @return:  1: [proc_delflag] The flag was already deleted.
// @return: -1: Failed to delete some flags (see 'errno' and 'kproc_perm')
extern int proc_delflag __P((kperm_flag_t __flag));
extern __nonnull((1)) int proc_delflags __P((kperm_flag_t const *__restrict __flagv, __size_t __maxflags));

//////////////////////////////////////////////////////////////////////////
// Lookup the value of a given permission 'NAME' and fill 'BUF' with its value.
// @return: BUF:  Lookup was successful.
// @return: NULL: Lookup failed (see 'errno' and 'kproc_perm').
extern __nonnull((2)) perm_t *proc_getperm __P((proc_t __proc, perm_t *__restrict __buf, perm_name_t __name));

//////////////////////////////////////////////////////////////////////////
// Perform extended permission operations.
// @return: BUF:  Operation was successful.
// @return: NULL: Operation failed (see 'errno' and 'kproc_perm').
extern __nonnull((2)) perm_t       *proc_permex __P((proc_t __proc, perm_t *__restrict __buf, __size_t __permcount, int __mode));
extern __nonnull((2)) perm_t       *proc_getpermex __P((proc_t __proc, perm_t *__restrict __buf, __size_t __permcount));
extern __nonnull((1)) perm_t const *proc_setpermex __P((perm_t const *__restrict __buf, __size_t __permcount));
extern __nonnull((1)) perm_t       *proc_xchpermex __P((perm_t *__restrict __buf, __size_t __permcount));


typedef struct ktlsinfo tlsinfo_t;

//////////////////////////////////////////////////////////////////////////
// Allocate/Free TLS memory.
// @param: SIZE: The size of the template/TLS block (Actually allocated storage may be greater than this)
// @return: 0 : [tls_alloc] Failed to allocate a new TLS block (see 'errno' and 'kproc_tlsalloc').
// @return: * : [tls_alloc] The lower-most address for an offset that may be
//                          used with as offset for 'tls_addr', or any of the
//                          tls_(get|put)(b|w|l|q) functions.
// @return: -1: [tls_free] An error occurred (see 'errno' and 'kproc_tlsfree').
// @return: 0 : [tls_free] Successfully freed the TLS block at the given offset.
extern __ptrdiff_t  tls_alloc __P((void const *__template, __size_t __size));
extern int          tls_free  __P((__ptrdiff_t __offset));
extern tlsinfo_t   *tls_enum  __P((tlsinfo_t *__restrict __infov, __size_t __infoc));



#ifndef __uthread_t_defined
#define __uthread_t_defined 1
typedef struct kuthread uthread_t;
#endif

__DECL_END

#ifdef __INTELLISENSE__
__DECL_BEGIN

/* The base address of the thread-local TLS block.
 * >> Add an offset to this and dereference to read thread-local data.
 * WARNING: The value of this variable may change sporadically during a
 *          call to 'tls_alloc', meaning that you must reload everything
 *          derived from it after allocating more TLS storage, or perform
 *          your own synchronization whenever code may allocate more TLS
 *          memory.
 *          Also note, that when loading libraries, additional TLS storage
 *          may be allocated for use by '__thread' or '__declspec(thread)',
 *          implying the same potential for TLS relocations. */
extern __byte_t  *const tls_addr;
extern uthread_t *const tls_self;

/* Optimized getter/setter for TLS data.
 * NOTE: Using these is faster than using 'tls_addr'. */
extern __u8  tls_getb(__ptrdiff_t offset);
extern __u16 tls_getw(__ptrdiff_t offset);
extern __u32 tls_getl(__ptrdiff_t offset);
extern __u64 tls_getq(__ptrdiff_t offset);
extern void  tls_putb(__ptrdiff_t offset, __u8  value);
extern void  tls_putw(__ptrdiff_t offset, __u16 value);
extern void  tls_putl(__ptrdiff_t offset, __u32 value);
extern void  tls_putq(__ptrdiff_t offset, __u64 value);

__DECL_END
#else
#include <proc-tls.h>
#define tls_addr   _tls_addr
#define tls_self   _tls_self
#define tls_getb   _tls_getb
#define tls_getw   _tls_getw
#define tls_getl   _tls_getl
#define tls_putb   _tls_putb
#define tls_putw   _tls_putw
#define tls_putl   _tls_putl
#define tls_getq   _tls_getq
#define tls_putq   _tls_putq
#endif

#endif /* !__KERNEL__ */

#endif /* !__PROC_H__ */

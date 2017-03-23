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
#ifndef __KOS_TASK_H__
#define __KOS_TASK_H__ 1

#include <kos/config.h>
#include <kos/compiler.h>
#ifndef __NO_PROTOTYPES
#include <kos/errno.h>
#include <kos/types.h>
#ifdef __KERNEL__
#include <kos/kernel/task.h>
#else /* __KERNEL__ */
#include <kos/assert.h>
#include <kos/fd.h>
#include <kos/syscall.h>
#include <string.h>
#endif /* !__KERNEL__ */
#endif /* !__NO_PROTOTYPES */

__DECL_BEGIN

#ifndef __ASSEMBLY__
struct timespec;
#endif /* __ASSEMBLY__ */


//////////////////////////////////////////////////////////////////////////
// Sandbox barrier levels
// NOTE: All listed permissions only affect visible tasks,
//       where the sub-set of visible tasks is defined by
//       a level three (KSANDBOX_BARRIERLEVEL_THREE) barrier.
//       >> 
// Permissions (for interactions with old barrier):
//   - SETMEM:
//     - write("/proc/[PID]/mem"); // Can modify memory of any visible task (including registers)
//   - GETMEM:
//     - read("/proc/[PID]/mem");  // Can read memory of any visible task (including registers)
//   - SETPROP:
//     - ktask_setname(PID);       // Can set the name of any unnamed visible task
//     - ktask_setprio(PID);       // Can set the priority of any visible task (restricted by priority sanding)
//     - ktask_suspend(PID);       // Can suspend any visible task
//     - ktask_resume(PID);        // Can resume any visible task
//     - ktask_terminate(PID);     // Can terminate any visible task
//   - GETPROP/VISIBLE:
//     - ktask_getname(PID);       // Can query the name of any visible task
//     - ktask_getpriority(PID);   // Can query the priority of any visible task
//     - system("ps")              // Can see all tasks that were visible before
// 
// Level-four (4) sandbox:
//   - Unless special rights are (re-)acquired, such as after performing a root fork,
//     the task is not able to interact, or even see, anything beyond the newly
//     set barrier:
//   >> pid_t child;
//   >> if ((child = fork()) == 0) {
//   >>   // Set a level four barrier in the child task
//   >>   kproc_barrier(KSANDBOX_BARRIERLEVEL(4));
//   >>   // Change the root directory to trap the child task
//   >>   chroot("/maybe/a");
//   >>   chdir("/");
//   >>   // "/virus" could still escape by doing a root-fork
//   >>   // if it knows your root password, or has access to
//   >>   // a hash accepted by the kernel.
//   >>   exit(system("/virus"));
//   >> }
//   >> // We (the parent) are still able to see the child, though
//   >> // the child task is not able to see us, nor to interact.
//   >> // WARNING: Any open file descriptor will remain open.
//   >> //          >> And if just one lies outside chroot, the
//   >> //            '/maybe/a/virus' task will be able to
//   >> //             escape its filesystem trap.
//   >> //          Though there's no way it can escape its barrier.
// 
// SECURE:  Once a barrier has been put in place, it can never be revered.
// TRAPPED: A child task can't hide, or protect itself from its parent.
//          >> Barriers only block the way out of a sandbox, but not into!
// ESCAPE:  There is none. Until its own termination, a parent will always
//          have full control of what one of its child processes can do.
//          
// 
// Default barrier levels:
// |    | SETMEM | GETMEM | SETPROP | GETPROP/VISIBLE
// | #0 |    yes |    yes |     yes |     yes
// | #1 |     no |    yes |     yes |     yes
// | #2 |     no |     no |     yes |     yes
// | #3 |     no |     no |      no |     yes
// | #4 |     no |     no |      no |      no
// HINT: Barrier can be set individually (s.a.: 'KSANDBOX_BARRIER_NO*' flags)
#define KSANDBOX_BARRIERLEVEL(n) (0xf >> (4-(n)))

// Explicit barrier flags. Yes, you can move barriers individually,
// but unless used as described by 'KSANDBOX_BARRIERLEVEL', that
// wouldn't really make much sense. e.g.:
// >> KSANDBOX_BARRIER_NOGETPROP|KSANDBOX_BARRIER_NOVISIBLE:
//    - Your child can't see you, or find out who you are.
//      But it can punch you in your face (modify your memory) and
//      force you to be known as '__xX$h1ttyTA5KXx__' (set your name).
//    - But then again: Your child would never even know it hit someone...
// WARNING: Most GETPROP/VISIBLE system calls make the user responsible
//          to ensure that no open file descriptors point to supposedly
//          invisible data, assuming that when you managed to get a
//          descriptor on something, you must also have permissions
//          to see it (even when those are taken away AFTER you've
//          already been given the descriptor).
// >> Again: A simple solution before exec-ing a dangerous
//           program, is to check the standard descriptors
//           0..2 for containing suspicious objects (such as
//           not actually being files), and then just call
//          'kfd_closeall(3,INT_MAX);' to close all the rest.
#define KSANDBOX_BARRIER_NOSETMEM  0x8
#define KSANDBOX_BARRIER_NOGETMEM  0x4
#define KSANDBOX_BARRIER_NOSETPROP 0x2
#define KSANDBOX_BARRIER_NOGETPROP 0x1
#define KSANDBOX_BARRIER_NOVISIBLE KSANDBOX_BARRIER_NOGETPROP
#define KSANDBOX_BARRIER_NONE      0x0
#define KSANDBOX_BARRIER_COUNT     4

#ifndef __ASSEMBLY__
#ifndef __ksandbarrier_t_defined
#define __ksandbarrier_t_defined 1
typedef __ksandbarrier_t ksandbarrier_t;
#endif

#ifndef __ktaskprio_t_defined
#define __ktaskprio_t_defined 1
typedef __ktaskprio_t ktaskprio_t;
#endif
#endif /* !__ASSEMBLY__ */

#define KTASKOPFLAG_NONE      0x00000000
#define KTASKOPFLAG_RECURSIVE 0x00000001 /*< Perform the operation recursively on the given task and all child tasks within the same process. */
#define KTASKOPFLAG_TREE      0x00000003 /*< Perform the operation recursively on the given task and all child tasks. */
#define KTASKOPFLAG_ASYNC     0x00000004 /*< Don't wait until an asynchronous operation has finished (e.g.: join all task during termination;
                                             required if the to-be-terminated task must finish performing a critical operation, such as writing a file). */
#define KTASKOPFLAG_NOCALLER  0x00000008 /*< Always prevent the calling task from being affected. */

#ifndef __ASSEMBLY__
#ifndef __ktaskopflag_t_defined
#define __ktaskopflag_t_defined 1
typedef __ktaskopflag_t ktaskopflag_t;
#endif

#ifndef __ktid_t_defined
#define __ktid_t_defined 1
#define KTID_INVALID   __KTID_INVALID
typedef __ktid_t ktid_t;
#endif
#endif /* !__ASSEMBLY__ */

#ifndef __KERNEL__

#ifndef __ASSEMBLY__
typedef int ktask_t;
typedef int kproc_t;
#endif /* !__ASSEMBLY__ */

#ifndef __NO_PROTOTYPES

//////////////////////////////////////////////////////////////////////////
// Willingly yield execution to another waiting task
__local _syscall0(kerrno_t,ktask_yield);

//////////////////////////////////////////////////////////////////////////
// Sets an alarm timeout of a given task, or change/set the timeout
// of a task already in the process of blocking.
//  - If 'abstime' is NULL, a set alarm is deleted.
//  - The alarm will act as a timeout the next time the given task
//    will be unscheduled for the purposes of waiting on a signal.
//    If in that call, an explicit timeout is specified, the 'abstime'
//    specified in this call will have no effect and be dropped.
//  - This function can be used to specify
//  - If the task is already waiting, this function may be used to specify,
//    or even override the timeout of the already waiting task.
// @return: KE_DESTROYED: The given task was destroyed.
// @return: KE_OK:        The task is ready to wait on the next blocking signal-wait.
//                        NOTE: '*oldabstime' is left untouched.
//                        NOTE: If 'abstime' is NULL, the task is left unchanged.
// @return: KS_UNCHANGED: A timeout had already been set, and its value was stored in
//                        '*oldabstime' if non-NULL. Though the timeout has now been
//                        overwritten by 'abstime'.
//                        The task may have also already entered a blocking call
//                        with another timeout, and setting the alarm overwrote that
//                        previous timeout. ('*oldabstime' is filled with that old timeout)
// @return: KE_ACCES:     The call does not have SETPROP access to the given task
__local _syscall3(kerrno_t,ktask_setalarm,ktask_t,task,
                  struct timespec const *__restrict,abstime,
                  struct timespec *__restrict,oldabstime);

//////////////////////////////////////////////////////////////////////////
// Retrieves the timeout of a given waiting task and stores it in '*abstime'.
// @return: KE_OK:        The given task is waiting or has an alarm set, with
//                        its timeout now being stored in '*abstime'.
// @return: KE_DESTROYED: The given task has terminated.
// @return: KS_EMPTY:     The given task is neither waiting, nor has an alarm set.
// @return: KS_BLOCKING:  The given task is waiting, but not with a timeout.
__local __nonnull((2)) _syscall2(kerrno_t,ktask_getalarm,ktask_t,task,
                                 struct timespec *__restrict,abstime);


//////////////////////////////////////////////////////////////////////////
// Sleep, doing nothing for a given timeout has passed
// @param: timeout: If NULL, sleep indefinitely (aka. 'pause()'-style)
// @return: KE_OK:        The task's state was successfully updated.
// @return: KS_UNCHANGED: The given task was already sleeping.
//                        NOTE: In this situation, a given timeout may only lower an
//                              existing one, or set an initial one (e.g. after 'ktask_pause()').
// @return: KE_TIMEDOUT:  You're the given task, and you have simply timed out (This is normal).
// @return: KE_DESTROYED: The given task has already been terminated.
// @return: KE_OVERFLOW:  A reference overflow prevented the given task from entering sleep mode.
__local _syscall2(kerrno_t,ktask_abssleep,ktask_t,task,
                  struct timespec const *__restrict,abstime);
#ifdef __INTELLISENSE__
extern kerrno_t ktask_pause(ktask_t task);
#else
#define ktask_pause(task) ktask_abssleep(task,NULL)
#endif

//////////////////////////////////////////////////////////////////////////
// Terminate a given task (thread/process)
// PERMISSIONS: The caller must have SETPROP access to the task
__local _syscall3(kerrno_t,ktask_terminate,ktask_t,task,
                  void *,exitcode,ktaskopflag_t,flags);

//////////////////////////////////////////////////////////////////////////
// Suspend/Resume a given task (thread/process)
// NOTE: If the recursive flag is set, the call behaves as though each
//       child task was suspended/resumed the same way as their parent.
//       If during this process an error occurs, all tasks already
//       suspended/resumed will be resumed/suspended again.
// PERMISSIONS: The caller must have SETPROP access to the task
__local _syscall2(kerrno_t,ktask_suspend,ktask_t,task,ktaskopflag_t,flags);
__local _syscall2(kerrno_t,ktask_resume,ktask_t,task,ktaskopflag_t,flags);

#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Returns a placeholder descriptor for the current thread.
// NOTE: The returned file descriptor is not a real descriptor
//       and merely operates as a symbolic handle.
// NOTE: This function never fails (manly because it literally returns a constant)
extern __wunused __constcall ktask_t ktask_self(void);

//////////////////////////////////////////////////////////////////////////
// Returns a placeholder descriptor for the root task of the current process.
// NOTE: The returned file descriptor is not a real descriptor
//       and merely operates as a symbolic handle.
// NOTE: This function never fails (manly because it literally returns a constant)
extern __wunused __constcall ktask_t ktask_proc(void);

#else
#define ktask_self() KFD_TASKSELF
#define ktask_proc() KFD_TASKROOT
#endif

#ifndef __ASSEMBLY__
//////////////////////////////////////////////////////////////////////////
// Exit the calling thread with the given exitcode
__local __noreturn void ktask_exit(void *exitcode) {
 /* NOTE: Don't terminate child-threads here. */
 ktask_terminate(ktask_self(),exitcode,KTASKOPFLAG_NONE);
 __asm_volatile__("" : : : "memory");
 __compiler_unreachable();
}

//////////////////////////////////////////////////////////////////////////
// Exit the calling process with the given exitcode
__local __noreturn void kproc_exit(void *exitcode) {
 // NOTE: Since this call won't return, we don't care about
 //       the leaked file descriptor returned by 'ktask_proc()'
 ktask_terminate(ktask_proc(),exitcode,KTASKOPFLAG_RECURSIVE);
 __asm_volatile__("" : : : "memory");
 __compiler_unreachable();
}
#endif /* !__ASSEMBLY__ */

//////////////////////////////////////////////////////////////////////////
// Returns the process root task (thread) of a given task.
// e.g.: If 'task' was spawned as a thread, the
//       associated process root-thread is returned.
// NOTE: If 'task' already is the root task of a process, a
//       duplicate descriptor of 'task' will be returned again.
// >> Recursive operations on the returned descriptor will affect every
//    thread of a process, as well as all spawned sub-processes.
// @return: KE_ISERR(*): An error occurred.
// @return: KE_ISOK(*):  A new file descriptor that must be closed using 'kfd_close'
// @return: KE_MFILE:    Too many open file descriptors.
__local __constcall _syscall1(ktask_t,ktask_openroot,ktask_t,task);

//////////////////////////////////////////////////////////////////////////
// Returns the parent of a given task.
// NOTE: If the caller is lacking the 'KPERM_FLAG_GETPROCPARENT' permission
//       and the task's parent is part of a different process, or if the
//       given task has no parent, such as the system root-task, the same
//       task is opened again and returned.
// @return: KE_ISERR(*): An error occurred (e.g.: The given task does not have a parent)
// @return: KE_ISOK(*):  A new file descriptor that must be closed using 'kfd_close'
// @return: KE_MFILE:    Too many open file descriptors.
__local _syscall1(ktask_t,ktask_openparent,ktask_t,self);

//////////////////////////////////////////////////////////////////////////
// Returns a descriptor for the process (task context) of a given task.
// @return: KE_ISERR(*): An error occurred (e.g.: The given task does not have a parent)
// @return: KE_ISOK(*):  A new file descriptor that must be closed using 'kfd_close'
// @return: KE_MFILE:    Too many open file descriptors.
__local _syscall1(kproc_t,ktask_openproc,ktask_t,self);

//////////////////////////////////////////////////////////////////////////
// Returns the id of a given task within its parent.
// NOTE: Returns '0' if the given task does not have a parent, or the
//       caller is not allowed to know of that parent's existence.
__local _syscall1(__size_t,ktask_getparid,ktask_t,self);

//////////////////////////////////////////////////////////////////////////
// Returns the TID (that is the thread id) of a given task.
// >> The thread id is unique to any task for the duration of its lifetime.
// >> TIDs are always, but only unique within the same process (task context).
__local _syscall1(ktid_t,ktask_gettid,ktask_t,self);

//////////////////////////////////////////////////////////////////////////
// Returns the child of a given task based on its parent ID
// @return: KE_ISERR(*) : An error occurred
// @return: KE_ISOK(*)  : A new file descriptor that must be closed using 'kfd_close'
// @return: KE_MFILE:     Too many open file descriptors.
__local _syscall2(ktask_t,ktask_openchild,ktask_t,self,__size_t,parid);

//////////////////////////////////////////////////////////////////////////
// Enumerate all children of a given task.
// PERMISSIONS: Only tasks the caller has GETPROP/VISIBLE access to are enumerated.
__local _syscall4(kerrno_t,ktask_enumchildren,ktask_t,self,
                  __size_t *__restrict,idv,__size_t,idc,__size_t *,reqidc);

//////////////////////////////////////////////////////////////////////////
// Get/Set the priority of a given task.
// PERMISSIONS: The caller needs GETPROP permissions to call 'ktask_getpriority'
//              The caller needs SETPROP permissions to call 'ktask_setpriority'
__local __nonnull((2)) _syscall2(kerrno_t,ktask_getpriority,ktask_t,self,ktaskprio_t *__restrict,result);
__local                _syscall2(kerrno_t,ktask_setpriority,ktask_t,self,ktaskprio_t,value);

//////////////////////////////////////////////////////////////////////////
// Joins a given task.
//  - Using a real signal, the calling task may be unscheduled until
//    the given task 'self' has run its course, or was terminated.
//  - If 'exitcode' is non-NULL, the task's return value is stored in '*exitcode'
//  - Note, that these functions may be called any amount of times, even when
//    the task has already been joined by the same, or a different task.
//  - If the given task 'self' has already terminated/exited,
//    all of these functions return immediately.
// @return: KE_OK:       The given task has been terminated, or has exited.
//                       When given, its return value has been stored in '*exitcode'.
// @return: KE_TIMEDOUT: The given abstime has passed, or 'ktask_tryjoin' would have blocked
// PERMISSIONS: The caller must have GETPROP access to the task
__local                _syscall3(kerrno_t,ktask_join,ktask_t,self,void **__restrict,exitcode,__u32,pending_argument);
__local                _syscall3(kerrno_t,ktask_tryjoin,ktask_t,self,void **__restrict,exitcode,__u32,pending_argument);
__local __nonnull((2)) _syscall4(kerrno_t,ktask_timedjoin,ktask_t,self,struct timespec const *__restrict,abstime,void **__restrict,exitcode,__u32,pending_argument);

//////////////////////////////////////////////////////////////////////////
// Get/Set the name of a given task.
// PERMISSIONS: The caller needs GETPROP permissions to call 'ktask_getname'
//              The caller needs SETPROP permissions to call 'ktask_setname'
// NOTE: The name of a task can only be set once.
__local __nonnull((2)) kerrno_t ktask_getname(ktask_t self, char *buffer, __size_t bufsize, __size_t *__restrict reqsize) { return kfd_getattr(self,KATTR_GENERIC_NAME,buffer,bufsize*sizeof(char),reqsize); }
__local __nonnull((2)) kerrno_t ktask_setname_ex(ktask_t self, char const *__restrict name, size_t namesize) { return kfd_setattr(self,KATTR_GENERIC_NAME,name,namesize*sizeof(char)); }
__local __nonnull((2)) kerrno_t ktask_setname(ktask_t self, char const *__restrict name) { return ktask_setname_ex(self,name,strlen(name)); }

#endif /* !__NO_PROTOTYPES */
#else /* !__KERNEL__ */
#ifndef __ASSEMBLY__
typedef struct ktask *ktask_t;
typedef struct kproc *kproc_t;
#endif /* !__ASSEMBLY__ */
#endif /* __KERNEL__ */

#define KTASK_NEW_FLAG_NONE        0x00000000

/* Create the thread/processes as suspended.
 * NOTE: 'KTASK_NEW_FLAG_JOIN' or 'childfd_or_exitcode == NULL' implies this flag. */
#define KTASK_NEW_FLAG_SUSPENDED   0x00000001

/* Join for the newly spawned process (Overrides 'KTASK_NEW_FLAG_SUSPENDED').
 * When this flag is set in 'ktask_newthread' or 'ktask_newthreadi', the
 * function will not return until the thread has terminated, at which point
 * the return value of the thread will be stored in '*arg' when 'arg != NULL'
 * NOTE: Only the immediate thread is joined, meaning that any other thread
 *       spawned by the task created through the call in which this flag is used
 *       will continue to run (including any processes spawned from there on).
 */
#define KTASK_NEW_FLAG_JOIN        0x00000002

/* Instead of mapping the process root thread as a child of the calling thread,
 * map it as a child of the furthest process the new process will have SETMEM access to.
 * Useful when wanting to perform a root-fork from an untrusted environment,
 * preventing access to a higher-privilege process by a lower-privilege caller
 * that could potentially start messing with things, such as its memory.
 * HINT: As the ~true~ parent, you can still join the process and read
 *       its exit code by specifying the 'KTASK_NEW_FLAG_JOIN'.
 * WARNING: When this flag is set, the task is spawned as a child of
 *         'kproc_openbarrier(child_task,KSANDBOX_BARRIER_NOSETMEM)'. */
#define KTASK_NEW_FLAG_UNREACHABLE 0x00000004

/* Only accepted by fork: Spawn the new task in a top-privilege sandbox.
 * In order of any application or library being allowed to do this, another
 * process with root-permissions must set the SETUID bit on the associated binary.
 * NOTE: That is the binary/library from where the rootfork-command originated from,
 *       meaning that you can write some library that makes use of this, even
 *       going as far as to allow untrusted applications to load and use you
 *       without running the risk of granting rootfork-permissions to the
 *       potentially hostile host application.
 *    >> This is enfoced as follows:
 *       - If root-fork is allowed or not depends on the value of your EIP
 *         (instruction pointer), more specifically the EIP at the time of the system
 *         call IRQ, where it is only allowed when it points into the executable area of
 *         a binary loaded into your address-space who's file has the SETUID bit set.
 *       - Pages that have been modified since loading of such a library are no
 *         longer considered part of that libraries address-space in this context.
 *        (So don't worry: the attacker can re-write your code, but once having done
 *                         so, they've also destroyed their only way of getting to root.)
 *       - So in the end, security lies with you, creator of such a library.
 * WARNING: It is strongly recommended to only use this flag with 'KTASK_NEW_FLAG_UNREACHABLE',
 *          especially when doing a root-fork from an untrusted environment. */
#define KTASK_NEW_FLAG_ROOTFORK    0x00000008


#ifndef __ASSEMBLY__
// Prototype for the entry point of a user-level thread.
// NOTE: Threads exit their control flow by terminating
//       themselves (for simplicity, 'ktask_exit' may be called)
// HINT: Through use of 'ktask_newthreadi', the given 'void *'
//       may be set to point to a given buffer. Using this,
//       a real thread callback can be specified that is then
//       allowed to return through normal means.
#ifndef __ktask_threadfun_t_defined
#define __ktask_threadfun_t_defined 1
typedef void (*__noreturn ktask_threadfun_t) __P((void *));
#endif /* !__ktask_threadfun_t_defined */
#endif /* !__ASSEMBLY__ */

#ifndef __KERNEL__
#ifndef __NO_PROTOTYPES

//////////////////////////////////////////////////////////////////////////
// Spawns a new thread as a child task of the calling thread.
// NOTE: If the thread is spawned suspended, a call
//       to 'ktask_resume' is required to launch it.
// NOTE: The given thread function may not return through normal
//       means. Instead the user should implement their own thread
//       api that executes a custom callback possibly passed
//       through the 'closure' argument, and always terminate
//       all thread_main functions via a 'ktask_exit()'.
// @param: flags: a set of 'KTASK_NEWTHREAD_FLAG_*' flags.
// @return: KE_ISERR(*) : An error occurred.
// @return: KE_ISOK(*)  : A new file descriptor used to interact with the child task.
// @return: KE_MFILE:     Too many open file descriptors.
// @return: KE_ACCES:     The calling process is not allowed to spawn more threads.
__local _syscall4(kerrno_t,ktask_newthread,ktask_threadfun_t,thread_main,
                  void *,closure,__u32,flags,void **,arg);

//////////////////////////////////////////////////////////////////////////
// Same as 'ktask_newthread', but the given 'buf|bufsize' data is pushed 
// on the stack of the child thread, before a pointer to that data is
// then passed as the closure argument, as also found in 'ktask_newthread'
//  - While this function doesn't ~really~ bring something new to the
//    table, it is very useful when creating threads with custom closure
//    data, of which you don't know if they are ever going to be started at all.
//  - If you know that any given thread that is started suspended will eventually
//    be launched, you can simply 'malloc()' the closure, then 'free()'
//    it at the start of your thread function.
// >> struct thread_data { ... };
// >> void __noreturn threadmain(struct thread_data *data) {
// >>   // '*data' is stored further up this thread's stack,
// >>   // meaning you should not attempt to 'free()' it.
// >>   ...
// >>   ktask_exit(NULL);
// >> }
// >> 
// >> ktask_t spawn_thread(__u32 flags) {
// >>   struct thread_data tdata = { ... };
// >>   // A copy of 'tdata' will be stored on the stack of 'threadmain'
// >>   return ktask_newthreadi((ktask_threadfun_t)&threadmain,&tdata,sizeof(tdata),flags);
// >> }
// @return: KE_NOMEM: Not enough available memory.
// @return: KE_MFILE: Too many open file descriptors.
// @return: KE_ACCES: The calling process is not allowed to spawn more threads.
__local _syscall5(kerrno_t,ktask_newthreadi,ktask_threadfun_t,thread_main,
                  void const *,buf,__size_t,bufsize,__u32,flags,void **,arg);

#endif /* !__NO_PROTOTYPES */
#endif /* !__KERNEL__ */

#ifndef __KERNEL__
#ifndef __NO_PROTOTYPES
//////////////////////////////////////////////////////////////////////////
// Fork into a new task and attempt to configure the new task's
// sandbox based on saved permissions of the given user account.
// >> Dependent on privilege levels of that user, the new
//    task may be spawned with more permissions than the caller,
//    posing a potential security risk as not just the calling
//    process, but all of its parent processes may also reach
//    downwards and, though they can't easily take over the higher
//    privileged process, they can still mess with it, such as
//    modifying its memory, or terminating it at any time.
// NOTE: If 'childfd' is NULL, the new task cannot be spawned suspended
// @return: KS_UNCHANGED: The forkas succeeded and you are the parent task. (NOTE: 'childfd' is filled with a descriptor for the new task, exclusive to your process (context))
// @return: KE_OK:        The forkas succeeded and you are the child task. (NOTE: 'childfd' is left untouched; get you're own, own handle...)
// @return: KE_MFILE:     Too many open file descriptors. (Never returned when 'childfd_or_exitcode' is NULL)
// @return: KE_NOMEM:     Not enough available kernel memory.
// @return: KE_TIMEDOUT:  'KTASK_NEW_FLAG_JOIN' was specified and an alarm()-timeout has expired.
// @return: KE_ACCES:     The caller is not allowed to perform a fork(), or not allowed to rootfork().
//                        s.a.: 'KPERM_FLAG_CANFORK' / 'KPERM_FLAG_CANROOTFORK'
// @param: childfd_or_exitcode: A pointer to an integer to store a file descriptor number
//                              that can be used to access the child task through.
//                              NOTE: To get the descriptor, cast '(int)*childfd_or_exitcode'
//                              >> When set to NULL, no FD repesentation of the forked task
//                                 is created, giving the caller no immediate way of finding
//                                 our who the task is that he just created.
//                              NOTE: To prevent security holes in situations where some tasks
//                                    with possible access to your file descriptors cannot be
//                                    trusted, this parameter should always be NULL.
//                              If flags contain 'KTASK_NEW_FLAG_JOIN', this argument is
//                              filled with the exitcode of the process once it terminates.
//                              If 'KTASK_NEW_FLAG_JOIN' is not set, but 'KTASK_NEW_FLAG_UNREACHABLE'
//                              is set, this argument is left untouched.
// @param: flags: A set of 'KTASK_NEW_FLAG_*' flags.
__local _syscall2(kerrno_t,ktask_fork,__uintptr_t *,childfd_or_exitcode,__u32,flags);

#ifndef __ASSEMBLY__
struct kexecargs;
#endif /* __ASSEMBLY__ */

//////////////////////////////////////////////////////////////////////////
// Overwrite the address space of the calling process with the given executable.
// WARNING: This function does not return upon success!
// HINT: Call this after doing a fork().
// NOTE: Within the calling process, all threads except for the calling are
//       terminated, with the calling thread being re-used as the root thread
//       of the new process. It is therefor recommended to have the calling
//       thread use a kernel-allocated stack, as any user-allocated memory
//       will be freed before switching to the new process.
// @param: argc|argv:    argc-argv-vector, as passed to the main() function of the new process.
//                       NOTE: The caller is responsible for desciding of the
//                             first argument should be the name of the process.
// @param: args:         Optional set of additional arguments, such as argc|argv and environ.
//                       When not specified, default to the following:
//                       >> args = omalloc(struct kexecargs);
//                       >> args->ea_argc    = 1;
//                       >> char *argv[] = {strndup(path,pathmax)};
//                       >> args->ea_argv    = (char **)memdup(argv,sizeof(argv));
//                       >> args->ea_arglenv = NULL;
//                       >> args->ea_environ = NULL;
//                       NOTE: If a file-descriptor is used, pass its absolute path
//                             as retrieved through 'getattr(KATTR_FS_PATHNAME)' as
//                             first argument.
// @return: KE_NOENT:    [ktask_exec] No module matching the given name was found.
// @return: KE_ACCES:    [ktask_exec] The caller doesn't have access to the only matching executable/dependency.
// @return: KE_BADF:     [ktask_fexec] Bad file-descriptor.
// @return: KE_NOMEM:    Insufficient memory.
// @return: KE_RANGE:    The specified module is position-dependent,
//                       but the region of memory required to load
//                       the module to is already in use.
// @return: KE_NOSPC:    The specified module is position-independent,
//                       but no region of memory big enough to hold it
//                       could be found (Damn, yo address space cluttered...).
// @return: KE_NOEXEC:   A file matching the given name was found, but it, or one
//                       of its dependencies isn't an executable recognized by KOS.
// @return: KE_NODEP:    At least one dependencies required by the module could not be found.
// @return: KE_LOOP:     The library and its dependencies are creating an unresolvable loop
// @return: KE_OVERFLOW: A reference counter would have overflown (the library is loaded too often)
// @return: KE_ISERR(*): Some other, possibly file-specific error has occurred
__local _syscall4(kerrno_t,ktask_exec,char const *,path,__size_t,pathmax,
                  struct kexecargs const *,args,__u32,flags);
__local _syscall3(kerrno_t,ktask_fexec,int,fd,
                  struct kexecargs const *,args,__u32,flags);
#endif
#endif

#define KTASK_EXEC_FLAG_NONE       0x00000000
/* Search $PATH for the given executable (Ignored in fexec). */
#define KTASK_EXEC_FLAG_SEARCHPATH 0x00000001
/* Use ':' or ';'-separated list $PATHEXT or ".exe" to search for the
 * executable again if it hasn't been found carrying the specified name.
 * WARNING: Ignored when the given path is absolute, or
 *          'KTASK_EXEC_FLAG_SEARCHPATH' isn't set as well. */
#define KTASK_EXEC_FLAG_RESOLVEEXT 0x00000002

#ifndef __ASSEMBLY__
struct kexecargs {
 __size_t           ea_argc;    /*< Max amount of arguments (NOTE: Argument vector may also be terminated by NULL-entry). */
 char const *const *ea_argv;    /*< [1..1][0..(ea_argc|first(NULL))] Vector of arguments. */
 __size_t const    *ea_arglenv; /*< [0..ea_argc|NULL] Max length of arguments (optional; strnlen-style). */
 __size_t           ea_envc;    /*< Max amount of arguments (NOTE: Environ vector may also be terminated by NULL-entry). */
 char const *const *ea_environ; /*< [1..1][0..(ea_argc|first(NULL))] Vector of environment strings.
                                     NOTE: Individual string must be of the format: '{name}={value}'.
                                           Environment strings not containing a '=' are ignored.
                                     NOTE: If this pointer is NULL, the environment remains unmodified.
                                        >> To exec the process in an empty environment, pass a pointer to NULL. */
 __size_t const    *ea_envlenv; /*< [0..ea_argc|NULL] Max length of individual environment strings (optional; strnlen-style). */
};
#endif /* !__ASSEMBLY__ */


#ifndef __KERNEL__
#ifndef __NO_PROTOTYPES
//////////////////////////////////////////////////////////////////////////
// Process (Task context) APIS

#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Returns a placeholder descriptor for the current process (task context).
// >> A process is what glues threads sharing the same address space
//    together, managing file descriptors and more while essentially being
//    what one would consider a process.
// NOTE: The returned file descriptor is not a real descriptor
//       and merely operates as a symbolic handle.
// NOTE: This function never fails (manly because it literally returns a constant)
extern __wunused __constcall kproc_t kproc_self(void);
#else
#define kproc_self() KFD_PROCSELF
#endif

//////////////////////////////////////////////////////////////////////////
// Enumerate all file descriptors of a given task.
// NOTE: These are the actual numbers of those descriptors,
//       meaning that unless you're enumerating your own descriptors,
//       you will have to re-open them for yourself.
__local _syscall4(kerrno_t,kproc_enumfd,kproc_t,self,
                  int *__restrict,fdv,__size_t,fdc,__size_t *,reqfdc);

//////////////////////////////////////////////////////////////////////////
// Re-opens an open file descriptor of the given process within
// the process (task context) of the calling thread (task).
// NOTE: For this operation to succeed, the calling task must have
//       SETPROP access to the root thread of the given process (task context).
// @return: KE_MFILE:     Too many open file descriptors.
__local _syscall3(int,kproc_openfd,kproc_t,self,int,fd,int,flags);
__local _syscall4(int,kproc_openfd2,kproc_t,self,int,fd,int,newfd,int,flags);

#if SYS_kproc_openroot != SYS_ktask_openroot || defined(__INTELLISENSE__)
//////////////////////////////////////////////////////////////////////////
// Opens the root thread of a given task process (context).
// >> If the calling task is not allowed to view the root task
//    of the given process, the nearest visible task is opened instead.
// @return: KE_MFILE:     Too many open file descriptors.
__local _syscall1(ktask_t,kproc_openroot,kproc_t,self);
#else
#define kproc_openroot ktask_openroot
#endif

#if SYS_kproc_openthread != SYS_ktask_openchild || defined(__INTELLISENSE__)
//////////////////////////////////////////////////////////////////////////
// Returns a new file descriptor for the thread with the given TID
// @return: KE_ISOK(*)  : A new descriptor that must be closed with 'kfd_close'
// @return: KE_ISERR(*) : An error occurred
// @return: KE_MFILE:     Too many open file descriptors.
__local _syscall2(ktask_t,kproc_openthread,kproc_t,self,ktid_t,tid);
#else
#define kproc_openthread ktask_openchild
#endif

#if SYS_kproc_enumthreads != SYS_ktask_enumchildren || defined(__INTELLISENSE__)
//////////////////////////////////////////////////////////////////////////
// Enumerate all threads running within a given task context.
// >> The enumerated tids can then be re-used to open descriptors
_syscall4(kerrno_t,kproc_enumthreads,int,self,ktid_t *__restrict,idv,
          __size_t,idc,__size_t *,reqidc);
#else
#define kproc_enumthreads  ktask_enumchildren
#endif

#if defined(__INTELLISENSE__) ||\
  (!defined(__GNUC__) && !__has_builtin(__builtin_constant_p))
//////////////////////////////////////////////////////////////////////////
// Sets up a task sandbox barrier of the given level (see above)
// >> The barrier will affect the entirety of the calling (task context).
// @return: KE_OK:       Successfully set the new barrier.
// @return: KE_OVERFLOW: The new barrier could not be set due to an internal reference overflow.
// @return: KE_ACCES:    The calling process if lacking the 'KPERM_FLAG_SETBARRIER' permission.
__local _syscall1(kerrno_t,kproc_barrier,ksandbarrier_t,level);
#else
__local __IDsyscall1(kerrno_t,__system_kproc_barrier,SYS_kproc_barrier,ksandbarrier_t,level);
// Try to optimize the no-op value of 'kproc_barrier(KSANDBOX_BARRIERLEVEL(0))' away.
#define kproc_barrier(level) (__builtin_constant_p(level)\
  ? ((level) ? __system_kproc_barrier(level) : KE_OK)\
  : __system_kproc_barrier(level))
#endif

//////////////////////////////////////////////////////////////////////////
// Returns a file descriptor to the lowest-level barrier task, as described by 'level'.
// >> This function can e.g. be used to open the top visible process to
//    the one, and is used in the implementation of the 'ps' command:
//    >> // Open the barrier describing the no-visibility limit,
//    >> // that is the last task possibly visible to us.
//    >> int proctop = kproc_openbarrier(kproc_self(),
//    >>                                 KSANDBOX_BARRIER_NOVISIBLE);
//    >> lsproc(proctop);
//    >> close(proctop);
// @return: KE_ISOK(*):  A new file descriptor that must be closed using 'kfd_close'
// @return: KE_ISERR(*): An error occurred.
// @return: KE_MFILE:    Too many open file descriptors.
__local _syscall2(ktask_t,kproc_openbarrier,kproc_t,self,ksandbarrier_t,level);

//////////////////////////////////////////////////////////////////////////
// Global list of processes
// KOS could have done without this, only going as far as to allow enumeration
// of processes via the 'SYS_ktask_enumchildren'/'SYS_kproc_enumthreads' system
// calls, but since not just linux, but also windows depend on some kind of
// global process enumeration system, KOS is kind-of forced to comply...
// >> Every existing process (task context) has an entry in this global list,
//    with the user-api for enumerating them being set up to only allow
//    visibility of processes who's root tasks are visible to them.
//    While this does pose the problem of not giving any leeway for implementing
//    zombie processes, their entire concept is already flawed from the get-go.
//    I mean: There's this kernel object that you have no control over what-so-ever.
//            It can disappear at any time and there is nothing you can do about
//            it, or somehow get informed when that happens.
//            You can only interact with it and hope that it's still alive when you
//            attempt to do whatever it is you're trying to do.
//         >> What am I talking about? - PIDs
// >> While linux sadly doesn't implement some way of controlling how long, if at
//    all, a process remains a zombie, KOS follows what windows does in a sense,
//    by offering user-apis for acquiring file descriptors (aka. Window's HANDLE),
//    to then use that handle for interacting with the process.
//    While the handle exists, the process may still terminate and before a zombie,
//    but for the time you haven't closed that handle, that zombie will stay around.
// >> Though KOS doesn't force you to deal with OpenProcess's permission garbage...
//    I mean: If I have the permission to do so, why do I have to tell you that
//    I really want to use that right.
//    Security? There's nothing secure about it!
//    If I can't interact with a process because my handle isn't allowed to, I can
//    just re-open another handle to the same process with more rights.
//    Oh! Is it because of how you can steal another process's handle through use of
//    DuplicateHandle? Well I'm sorry, but while KOS also allows for ~stealing~ of
//    open handles, you can simply trap anything you spawn and don't want to harm
//    you though use of 'kproc_barrier'.
// OK. Now that that rant is over, back to the business at hand: PIDs
//  - Every process (task context) has them, and they can be enumerated globally,
//    while only those that are visible to you will actually show up.
// SECURITY:
//  - While this really hurts my feelings, there is a minor flaw that might
//    allow some processes to figure out if they are actually behind a barrier:
//    Ever heard me mention a task/proc named 'ZERO' (aka. '/proc/0') (actual name is 'KERNEL')
//    Well... That's the kernel's initial task, and is somewhat special, as its PID is '0'.
//  - While not really too big of a problem, as any non-root session isn't able to see
//    that task, when taken quite literally any task can figure out if any sort of
//    GETPROP/VISIBLE barrier stands between it and the kernel by calling 'kproc_openpid(0)'

//////////////////////////////////////////////////////////////////////////
// Enumerate all processes who's root task is visible to the caller.
// @return: KE_DESTROYED: The system is shutting down and no new processes can be spawned.
__local _syscall3(kerrno_t,kproc_enumpid,
                  __pid_t *__restrict,pidv,
                  size_t,pidc,size_t *,reqpidc);

//////////////////////////////////////////////////////////////////////////
// Opens a handle to the process associated with the given PID.
// @return: KE_INVAL:     Invalid PID or the caller has insufficient
//                        permissions to open access process, or the system
//                        is shutting down (in which case no PID is valid).
// @return: KE_MFILE:     Too many open file descriptors.
// @return: KE_DESTROYED: The given process has terminated (it still exists as
//                        a zombie held alive by a reference from somewhere else,
//                        but no new references may be created)
//                      - Another reason might be that the system is past the point of
//                        shutdown at which processes may communicate through PIDs,
//                        also meaning that no new processes may be spawned.
__local _syscall1(kproc_t,kproc_openpid,__pid_t,pid);

//////////////////////////////////////////////////////////////////////////
// Returns PID (process ID) of a given process (task context)
// NOTES:
//   - If 'self' is a task descriptor, return the PID of the associated process.
//   - Returns a negative error code on failure (see below).
// @return: KE_DESTROYED: The given process has terminated (it still exists as
//                        a zombie held alive by a reference from somewhere else,
//                        but no new references may be created)
//                      - Another reason might be that the system is past the point of
//                        shutdown at which processes may communicate through PIDs,
//                        also meaning that no new processes may be spawned.
// @return: KE_BADF:     'self' is either an invalid descriptor, or not a task, or process.
__local _syscall1(__pid_t,kproc_getpid,kproc_t,self);

// TODO: DOC
__local _syscall6(kerrno_t,kproc_getenv,int,self,
                  char const *,name,__size_t,namemax,
                  char *,buf,__size_t,bufsize,__size_t *,reqsize);
__local _syscall6(kerrno_t,kproc_setenv,int,self,
                  char const *,name,__size_t,namemax,
                  char const *,value,__size_t,valuemax,
                  int,override);
__local _syscall3(kerrno_t,kproc_delenv,int,self,
                  char const *,name,__size_t,namemax);
__local _syscall4(kerrno_t,kproc_enumenv,int,self,
                  char *,buf,__size_t,bufsize,__size_t *,reqsize);

// TODO: DOC
__local _syscall4(kerrno_t,kproc_getcmd,int,self,
                  char *,buf,__size_t,bufsize,__size_t *,reqsize);


#endif /* !__NO_PROTOTYPES */
#else /* !__KERNEL__ */
#define kproc_getpid(self)  ((self)->p_pid)
#endif /* __KERNEL__ */

__DECL_END

#endif /* !__KOS_TASK_H__ */

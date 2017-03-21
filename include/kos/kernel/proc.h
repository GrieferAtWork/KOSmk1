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
#ifndef __KOS_KERNEL_PROC_H__
#define __KOS_KERNEL_PROC_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/compiler.h>
#include <kos/kernel/fdman.h>
#include <kos/kernel/gdt.h>
#include <kos/kernel/mmutex.h>
#include <kos/kernel/mutex.h>
#include <kos/kernel/object.h>
#include <kos/kernel/procenv.h>
#include <kos/kernel/procperm.h>
#include <kos/kernel/procmodules.h>
#include <kos/kernel/types.h>
#include <kos/kernel/tls.h>
#ifndef __KOS_KERNEL_SHM_H__
#include <kos/kernel/shm.h>
#endif
#include <kos/task.h>
#include <stdint.h>
#ifndef __INTELLISENSE__
#include <string.h>
#endif

__DECL_BEGIN

/* NOTE: kproc (Processes) were originally called ktaskctx (task-contexts).
 *       At some point I realized that's a really $h177y name and started
 *       calling it by what is really just simply is.
 *       So if see me mention ktaskctx anywhere, I'm talking about processes. */

#ifndef __ASSEMBLY__
struct ktask;
struct kproc;
struct kshlib;
struct kdirentname;
#endif /* !__ASSEMBLY__ */

#define KOBJECT_MAGIC_PROC  0x960C /*< PROC. */
#define kassert_kproc(self) kassert_refobject(self,p_refcnt,KOBJECT_MAGIC_PROC)


#define KPROCREGS_SIZEOF       (4)
#define KPROCREGS_OFFSETOF_CS  (0)
#define KPROCREGS_OFFSETOF_DS  (2)
#ifndef __ASSEMBLY__
struct kprocregs {
 __u16 pr_cs;  /*< [const] Code-segment used for new threads. (Usually an index within the LDT) */
 __u16 pr_ds;  /*< [const] Data-segment used for new threads. (Usually an index within the LDT) */
};
#endif /* !__ASSEMBLY__ */


#define KPROC_OFFSETOF_REFCNT  (KOBJECT_SIZEOFHEAD)
#define KPROC_OFFSETOF_PID     (KOBJECT_SIZEOFHEAD+4)
#define KPROC_OFFSETOF_LOCK    (KOBJECT_SIZEOFHEAD+8)
#define KPROC_OFFSETOF_SHM     (KOBJECT_SIZEOFHEAD+8+KMMUTEX_SIZEOF)
#define KPROC_OFFSETOF_REGS    (KOBJECT_SIZEOFHEAD+8+KMMUTEX_SIZEOF+KSHM_SIZEOF)
#define KPROC_OFFSETOF_MODULES (KOBJECT_SIZEOFHEAD+8+KMMUTEX_SIZEOF+KSHM_SIZEOF+KPROCREGS_SIZEOF)
#ifndef __ASSEMBLY__
#define __kproc_defined
struct kproc {
 KOBJECT_HEAD
 __atomic __u32       p_refcnt;   /*< Reference counter. */
 __u32                p_pid;      /*< Process ID (unsigned to prevent negative numbers). */
#define KPROC_LOCK_MODS    KMMUTEX_LOCK(0)
#define KPROC_LOCK_FDMAN   KMMUTEX_LOCK(1)
#define KPROC_LOCK_PERM    KMMUTEX_LOCK(2)
#define KPROC_LOCK_THREADS KMMUTEX_LOCK(3)
#define KPROC_LOCK_ENVIRON KMMUTEX_LOCK(4)
 struct kmmutex       p_lock;     /*< Lock for the task context. */
 struct kshm          p_shm;      /*< Shared memory management & page directory. */
 struct kprocregs     p_regs;     /*< [lock(KPROC_LOCK_REGS)] Per-process register-configuration & related. */
 struct kprocmodules  p_modules;  /*< [lock(KPROC_LOCK_MODS)] Shared libraries associated with this process. */
 struct kfdman        p_fdman;    /*< [lock(KPROC_LOCK_FDMAN)] File descriptor manager. */
 struct kprocperm     p_perm;     /*< [lock(KPROC_LOCK_PERM)] Process related, sandy limits / permissions. */
 struct ktlsman       p_tls;      /*< TLS identifiers. */
 struct ktasklist     p_threads;  /*< [lock(KPROC_LOCK_THREADS)] List of tasks using this context. */
 struct kprocenv      p_environ;  /*< [lock(KPROC_LOCK_ENVIRON)] Process environment variables. */
};

#define kproc_islocked(self,lock)  kmmutex_islocked(&(self)->p_lock,lock)
#define kproc_trylock(self,lock)   kmmutex_trylock(&(self)->p_lock,lock)
#define kproc_lock(self,lock)      kmmutex_lock(&(self)->p_lock,lock)
#define kproc_unlock(self,lock)    kmmutex_unlock(&(self)->p_lock,lock)
#define kproc_islockeds(self,lock) kmmutex_islockeds(&(self)->p_lock,lock)
#define kproc_trylocks(self,lock)  kmmutex_trylocks(&(self)->p_lock,lock)
#define kproc_locks(self,lock)     kmmutex_locks(&(self)->p_lock,lock)
#define kproc_unlocks(self,lock)   kmmutex_unlocks(&(self)->p_lock,lock)

#define kproc_getfdman(self)   (&(self)->p_fdman)
#define kproc_getuid(self)        0 /*< TODO */
#define kproc_getgid(self)        0 /*< TODO */
#define kproc_getshm(self)     (&(self)->p_shm)
#define kproc_getperm(self)    (&(self)->p_perm)
#define kproc_getpagedir(self)  ((self)->p_shm.s_pd)

#define __kassert_kproc(self) kassert_object(self,KOBJECT_MAGIC_PROC)
__local KOBJECT_DEFINE_INCREF(kproc_incref,struct kproc,p_refcnt,kassert_kproc);
__local KOBJECT_DEFINE_TRYINCREF(kproc_tryincref,struct kproc,p_refcnt,__kassert_kproc);
__local KOBJECT_DEFINE_DECREF(kproc_decref,struct kproc,p_refcnt,kassert_kproc,kproc_destroy);
#undef __kassert_kproc

extern struct kproc __kproc_kernel;
#define kproc_kernel()  (&__kproc_kernel)

//////////////////////////////////////////////////////////////////////////
// Create a new root task context (with all permissions enabled)
extern __crit __ref struct kproc *kproc_newroot(void);

/* === Helper functions for implementing fork()-exec() through system-calls. */

//////////////////////////////////////////////////////////////////////////
// Copies a given task context, returning the new copy.
// NOTES:
//     marked with 'KFD_FLAG_CLOEXEC' will not be copied.
//   - It is possible to copy a closed (Zombie) process to create another closed one.
//   - The thread list of the returned process will
//     only contain a copy of the calling thread.
//   - The child process will immediately fork into
//     user-space with the given set of registers.
//   - 'ktask_fork' requires the calling task to not be a kernel-task.
//   - The returned task will be suspended at first
extern __crit kerrno_t
ktask_fork(__u32 flags, struct kirq_userregisters const *__restrict userregs,
           __ref struct ktask **presult);
extern __crit kerrno_t kproc_copy4fork(__u32 flags, struct kproc *__restrict proc,
                                       __user void *eip, __ref struct kproc **presult);

//////////////////////////////////////////////////////////////////////////
// @return: KE_OK:       The given process and EIP can perform a root fork.
// @return: KE_NOENT:    The given EIP is not associated with any module/memory tab.
// @return: KE_NOEXEC:   The given EIP is not part of an executable section.
// @return: KE_CHANGED:  Memory at the given EIP was modified since module loading.
// @return: KE_EXISTS:   No SHM region is associated with the given EIP.
// @return: KE_WRITABLE: The SHM mapping of the given EIP indicates writable.
// @return: KE_ACCES:    The associated module's file does not have the SETUID bit enabled.
extern __crit kerrno_t kproc_canrootfork_c(struct kproc *__restrict self, __user void *eip);
#define kproc_canrootfork(self,eip) KTASK_CRIT(kproc_canrootfork_c(self,eip))


struct kexecargs;

//////////////////////////////////////////////////////////////////////////
// #1: Terminate all threads (excluding the calling one)
// #2: Unload all currently loaded libraries/executables.
// #3: Free up all user-level shared memory tabs
//    (except for the calling thread's user-level stack, if any).
// #4: Delete all TLS variables.
// #5: Close all file descriptors marked with O_CLOEXEC.
// #6: Load the given 'exec_main' and all its dependencies.
// #7: Overwrite the given registers to point to the base
//     of the calling thread's user-stack, and set its EIP
//     to jump to the entry point of the given exec_main.
extern __crit kerrno_t
kproc_exec(struct kshlib *__restrict exec_main,
           struct kirq_userregisters *__restrict userregs,
           struct kexecargs *args, int args_members_are_kernel);


//////////////////////////////////////////////////////////////////////////
// Closes a given task context
// @return: KE_OK:        The context was successfully closed
// @return: KS_UNCHANGED: The context was already closed
extern __nonnull((1)) kerrno_t kproc_close(struct kproc *__restrict self);

//////////////////////////////////////////////////////////////////////////
// Set the barrier level of a given task
// NOTE: Userland tasks can only set the barrier to themselves (for now...)
// NOTE: The given level (0..4) describes the level of sandbox enclosure.
// @return: KE_DESTROYED: The given task context was closed.
// @return: KE_OVERFLOW:  Failed to acquire a new reference to the given new task
extern __crit __nonnull((1,2)) kerrno_t
kproc_barrier(struct kproc *__restrict self,
                 struct ktask *__restrict barrier,
                 ksandbarrier_t mode);

#ifdef __INTELLISENSE__
extern __wunused __nonnull((1)) struct ktask *kproc_getgpbarrier(struct kproc const *__restrict self);
extern __wunused __nonnull((1)) struct ktask *kproc_getspbarrier(struct kproc const *__restrict self);
extern __wunused __nonnull((1)) struct ktask *kproc_getgmbarrier(struct kproc const *__restrict self);
extern __wunused __nonnull((1)) struct ktask *kproc_getsmbarrier(struct kproc const *__restrict self);
extern __wunused __nonnull((1))          bool kproc_hasflag(struct kproc const *__restrict self, kperm_flag_t flag);
#else
#define kproc_getgpbarrier(self)   katomic_load((self)->p_perm.pp_gpbarrier)
#define kproc_getspbarrier(self)   katomic_load((self)->p_perm.pp_spbarrier)
#define kproc_getgmbarrier(self)   katomic_load((self)->p_perm.pp_gmbarrier)
#define kproc_getsmbarrier(self)   katomic_load((self)->p_perm.pp_smbarrier)
#define kproc_hasflag(self,flag)   kprocperm_hasflag(&(self)->p_perm,flag)
#endif

//////////////////////////////////////////////////////////////////////////
// Returns the barrier that is used to restrict access as described by 'mode'.
// @return: NULL: [*_r] Failed to acquire a new reference.
// @return: NULL:       The task context was closed.
#ifdef __INTELLISENSE__
extern        __wunused __retnonnull __nonnull((1))       struct ktask *
kproc_getbarrier(struct kproc const *__restrict self, ksandbarrier_t mode);
extern __crit __wunused __retnonnull __nonnull((1)) __ref struct ktask *
kproc_getbarrier_r(struct kproc const *__restrict self, ksandbarrier_t mode);
#else
#define kproc_getbarrier(self,mode)   kprocperm_getbarrier(&(self)->p_perm,mode)
extern __crit __wunused __nonnull((1)) __retnonnull __ref struct ktask *
kproc_getbarrier_r(struct kproc const *__restrict self, ksandbarrier_t mode);
#endif


//////////////////////////////////////////////////////////////////////////
// Checks permissions to ensure that a given task 'caller' is allowed to
// access the task 'self', where 'mode' specifies a set of required permissions.
__local int ktask_access_ex(struct ktask const *__restrict self, ksandbarrier_t mode,
                            struct ktask const *__restrict caller) {
 return !ktask_isusertask(caller) /*< Skip access checks for kernel tasks (Those guys could have just called *_k if they wanted...). */
      || ktask_issameorchildof(self,caller) /*< 'self' is a child of 'caller' (parents can do everything) */
      || ktask_issameorchildof(self,kproc_getbarrier(ktask_getproc(caller),mode)); /*< Check for barrier-restrictive access. */
}

#ifdef __INTELLISENSE__
extern __wunused int ktask_access(struct ktask const *__restrict self, ksandbarrier_t mode);
#else
#define ktask_access(self,mode) ktask_access_ex(self,mode,ktask_self())
#endif

#ifdef __INTELLISENSE__
extern __wunused __nonnull((1,2)) bool ktask_accessgp_ex(struct ktask const *__restrict self, struct ktask const *__restrict caller);
extern __wunused __nonnull((1,2)) bool ktask_accesssp_ex(struct ktask const *__restrict self, struct ktask const *__restrict caller);
extern __wunused __nonnull((1,2)) bool ktask_accessgm_ex(struct ktask const *__restrict self, struct ktask const *__restrict caller);
extern __wunused __nonnull((1,2)) bool ktask_accesssm_ex(struct ktask const *__restrict self, struct ktask const *__restrict caller);
extern __wunused __nonnull((1))   bool ktask_accessgp(struct ktask const *__restrict self);
extern __wunused __nonnull((1))   bool ktask_accesssp(struct ktask const *__restrict self);
extern __wunused __nonnull((1))   bool ktask_accessgm(struct ktask const *__restrict self);
extern __wunused __nonnull((1))   bool ktask_accesssm(struct ktask const *__restrict self);
#else
#define ktask_accessgp_ex(self,caller) ktask_access_ex(self,KSANDBOX_BARRIER_NOGETPROP,caller)
#define ktask_accesssp_ex(self,caller) ktask_access_ex(self,KSANDBOX_BARRIER_NOSETPROP,caller)
#define ktask_accessgm_ex(self,caller) ktask_access_ex(self,KSANDBOX_BARRIER_NOGETMEM,caller)
#define ktask_accesssm_ex(self,caller) ktask_access_ex(self,KSANDBOX_BARRIER_NOSETMEM,caller)
#define ktask_accessgp(self)           ktask_accessgp_ex(self,ktask_self())
#define ktask_accesssp(self)           ktask_accesssp_ex(self,ktask_self())
#define ktask_accessgm(self)           ktask_accessgm_ex(self,ktask_self())
#define ktask_accesssm(self)           ktask_accesssm_ex(self,ktask_self())
#endif

//////////////////////////////////////////////////////////////////////////
// Perform various operations on file descriptors
// NOTE: Upon success, 'kproc_closefd' will delete the fd entry associated with 'fd'
// @return: KE_OK:   The specified descriptor was successfully closed.
// @return: KE_BADF: The given file descriptor was invalid.
// @return: * :      A Descriptor-specific error occurred during closing.
extern __nonnull((1)) kerrno_t
kproc_closefd(struct kproc *__restrict self, int fd);

//////////////////////////////////////////////////////////////////////////
// Closes all file descriptors within a given range.
// @return: * : The amount of successfully closed descriptors.
extern __nonnull((1)) unsigned int
kproc_closeall(struct kproc *__restrict self, int low, int high);

//////////////////////////////////////////////////////////////////////////
// Returns a reference to a given file by its fd number.
// NOTE: The caller must destroy the returned context using 'kfdentry_quit'
// @return: KE_OK:       A valid 'struct kfd' is stored in '*result'
// @return: KE_BADF:     Invalid file descriptor.
// @return: KE_OVERFLOW: Too many references to the specified resource.
extern __crit __wunused __nonnull((1,3)) kerrno_t
kproc_getfd(struct kproc const *__restrict self, int fd,
            struct kfdentry *__restrict result);

#if 0
//////////////////////////////////////////////////////////////////////////
// Pops a given resource from the fd manager, returning it in '*result'
// @return: KE_OVERFLOW: Failed to acquire a reference to the associated file
// @return: KE_BADF:     The given file descriptor was invalid
extern __wunused __nonnull((1,2)) kerrno_t
kproc_popfd(struct kproc const *__restrict self,
            int fd, struct kfd *__restrict result);
#endif

//////////////////////////////////////////////////////////////////////////
// Inserts a given fd entry into the given task context.
// NOTE: 'kproc_insfdat_inherited' will also close an existing fd under the given number
// @return: KE_OK:    The given descriptor was inserted
// @return: KE_MFILE: The fdman is configured to not allow descriptors as high as 'fd' (Caller still owns the data reference)
// @return: KE_NOMEM: Not enough available memory (can happen when extending the allocated vector; Caller still owns the data reference)
// @return: KS_FOUND: [kproc_insfdat_inherited] An existing fd was closed (NOT AN ERROR)
extern __nonnull((1,2,3)) kerrno_t   kproc_insfd_inherited(struct kproc *__restrict self, int *__restrict fd, struct kfdentry const *__restrict entry);
extern __nonnull((1,3))   kerrno_t kproc_insfdat_inherited(struct kproc *__restrict self, int fd, struct kfdentry const *__restrict entry);
extern __nonnull((1,3))   kerrno_t  kproc_insfdh_inherited(struct kproc *__restrict self, int hint, int *__restrict fd, struct kfdentry const *__restrict entry);

//////////////////////////////////////////////////////////////////////////
// Duplicate a given file descriptor
// NOTE: 'kproc_dupfdh*' will choose the closest, free descriptor slot following 'hintfd'
// @return: KE_OK:       The given descriptor was inserted
// @return: KE_BADF:     The given 'oldfd' was invalid
// @return: KE_MFILE:    The new file descriptor was out-of-bounds.
// @return: KE_NOMEM:    Not enough memory available (can happen when extending the allocated vector)
// @return: KS_FOUND:    [kproc_dupfd2*] An existing fd was closed (NOT AN ERROR)
extern __crit __wunused __nonnull((1,3)) kerrno_t kproc_dupfd_c (struct kproc *__restrict self, int oldfd, int *__restrict newfd, kfdflag_t flags);
extern __crit __wunused __nonnull((1))   kerrno_t kproc_dupfd2_c(struct kproc *__restrict self, int oldfd, int newfd, kfdflag_t flags);
extern __crit __wunused __nonnull((1))   kerrno_t kproc_dupfdh_c(struct kproc *__restrict self, int oldfd, int hintfd, int *__restrict newfd, kfdflag_t flags);
#ifdef __INTELLISENSE__
extern        __wunused __nonnull((1,3)) kerrno_t kproc_dupfd (struct kproc *__restrict self, int oldfd, int *__restrict newfd, kfdflag_t flags);
extern        __wunused __nonnull((1))   kerrno_t kproc_dupfd2(struct kproc *__restrict self, int oldfd, int newfd, kfdflag_t flags);
extern        __wunused __nonnull((1))   kerrno_t kproc_dupfdh(struct kproc *__restrict self, int oldfd, int hintfd, int *__restrict newfd, kfdflag_t flags);
#else
#define kproc_dupfd(self,oldfd,newfd,flags)         KTASK_CRIT(kproc_dupfd_c(self,oldfd,newfd,flags))
#define kproc_dupfd2(self,oldfd,newfd,flags)        KTASK_CRIT(kproc_dupfd2_c(self,oldfd,newfd,flags))
#define kproc_dupfdh(self,oldfd,hintfd,newfd,flags) KTASK_CRIT(kproc_dupfdh_c(self,oldfd,hintfd,newfd,flags))
#endif

//////////////////////////////////////////////////////////////////////////
// Get varios types of objects from file descriptors.
// @return: NULL: Invalid descriptor/type, or reference overflow.
extern __crit __wunused __nonnull((1)) __ref struct kfile *kproc_getfdfile(struct kproc *__restrict self, int fd);
extern __crit __wunused __nonnull((1)) __ref struct ktask *kproc_getfdtask(struct kproc *__restrict self, int fd);
extern __crit __wunused __nonnull((1)) __ref struct kproc *kproc_getfdproc(struct kproc *__restrict self, int fd);
extern __crit __wunused __nonnull((1)) __ref struct kinode *kproc_getfdinode(struct kproc *__restrict self, int fd);
extern __crit __wunused __nonnull((1)) __ref struct kdirent *kproc_getfddirent(struct kproc *__restrict self, int fd);

//////////////////////////////////////////////////////////////////////////
// Returns the root task of a given process
// @return: NULL: The given process was closed, or no threads are running within.
extern __crit __wunused __nonnull((1)) __ref struct ktask *
kproc_getroottask(struct kproc const *__restrict self);

//////////////////////////////////////////////////////////////////////////
// Returns the root binary of a given process
// @return: NULL: The given process was closed.
extern __crit __wunused __nonnull((1)) __ref struct kshlib *
kproc_getrootexe(struct kproc const *__restrict self);

extern __nomp __wunused __nonnull((1)) struct kprocmodule *
kproc_getrootmodule_unlocked(struct kproc const *__restrict self);

//////////////////////////////////////////////////////////////////////////
// Returns TRUE (non-ZERO) if the given process's root task is visible to the caller.
// NOTE: If the process has no root task (i.e. is a Zombie), return FALSE (ZERO).
extern __crit __wunused __nonnull((1)) int kproc_isrootvisible_c(struct kproc const *__restrict self);
#ifdef __INTELLISENSE__
extern __wunused __nonnull((1)) int kproc_isrootvisible(struct kproc const *__restrict self);
#else
#define kproc_isrootvisible(self) KTASK_CRIT(kproc_isrootvisible_c(self))
#endif

//////////////////////////////////////////////////////////////////////////
// Returns the physical address of a resource described by 'fd'.
// WARNING: Do not attempt to dereference that resource.
//          This function is only meant to abstract comparing of file descriptors.
// @return: NULL: Invalid file descriptor, or the task context was closed.
extern        __wunused __nonnull((1)) void *kproc_getresourceaddr_nc(struct kproc const *__restrict self, int fd);
extern __crit __wunused __nonnull((1)) void *kproc_getresourceaddr_c(struct kproc const *__restrict self, int fd);
#ifdef __INTELLISENSE__
extern        __wunused __nonnull((1)) void *kproc_getresourceaddr(struct kproc const *__restrict self, int fd);
#else
#define kproc_getresourceaddr(self,fd) \
 (KTASK_ISCRIT_P ? kproc_getresourceaddr_c(self,fd)\
                 : kproc_getresourceaddr_nc(self,fd))
#endif

#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Returns true if two given file descriptors are equal to each other.
// WARNING: The caller is responsible to ensure that the descriptors
//          are not re-assigned before the equality information
//          is put to use (otherwise they might no longer be equal).
extern __wunused __nonnull((1)) int
kproc_equalfd(struct kproc const *__restrict self, int fda, int fdb);
#else
#define kproc_equalfd(self,fda,fdb) \
 __xblock({ struct kproc const *const __ktcqfself = (self);\
            __xreturn kproc_getresourceaddr(__ktcqfself,fda)\
                   == kproc_getresourceaddr(__ktcqfself,fdb);\
 })
#endif



extern __crit __wunused __nonnull((1,3,5))   kerrno_t kproc_user_readfd_c(struct kproc *__restrict self, int fd, __user void *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict rsize);
extern __crit __wunused __nonnull((1,3,5))   kerrno_t kproc_user_writefd_c(struct kproc *__restrict self, int fd, __user void const *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict wsize);
extern __crit __wunused __nonnull((1,4,6))   kerrno_t kproc_user_preadfd_c(struct kproc *__restrict self, int fd, __pos_t pos, __user void *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict rsize);
extern __crit __wunused __nonnull((1,4,6))   kerrno_t kproc_user_pwritefd_c(struct kproc *__restrict self, int fd, __pos_t pos, __user void const *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict wsize);
extern __crit __wunused __nonnull((1))       kerrno_t kproc_user_fcntlfd_c(struct kproc *__restrict self, int fd, int cmd, __user void *arg);
extern __crit __wunused __nonnull((1))       kerrno_t kproc_user_ioctlfd_c(struct kproc *__restrict self, int fd, kattr_t cmd, __user void *arg);
extern __crit __wunused __nonnull((1,4))     kerrno_t kproc_user_getattrfd_c(struct kproc *__restrict self, int fd, kattr_t attr, __user void *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict reqsize);
extern __crit __wunused __nonnull((1,4))     kerrno_t kproc_user_setattrfd_c(struct kproc *__restrict self, int fd, kattr_t attr, __user void const *__restrict buf, __size_t bufsize);

#ifdef __INTELLISENSE__
extern __crit __wunused __nonnull((1,3,5))   kerrno_t kproc_kernel_readfd_c(struct kproc *__restrict self, int fd, __kernel void *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict rsize);
extern __crit __wunused __nonnull((1,3,5))   kerrno_t kproc_kernel_writefd_c(struct kproc *__restrict self, int fd, __kernel void const *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict wsize);
extern __crit __wunused __nonnull((1,4,6))   kerrno_t kproc_kernel_preadfd_c(struct kproc *__restrict self, int fd, __pos_t pos, __kernel void *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict rsize);
extern __crit __wunused __nonnull((1,4,6))   kerrno_t kproc_kernel_pwritefd_c(struct kproc *__restrict self, int fd, __pos_t pos, __kernel void const *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict wsize);
extern __crit __wunused __nonnull((1))       kerrno_t kproc_kernel_fcntlfd_c(struct kproc *__restrict self, int fd, int cmd, __kernel void *arg);
extern __crit __wunused __nonnull((1))       kerrno_t kproc_kernel_ioctlfd_c(struct kproc *__restrict self, int fd, kattr_t cmd, __kernel void *arg);
extern __crit __wunused __nonnull((1,4))     kerrno_t kproc_kernel_getattrfd_c(struct kproc *__restrict self, int fd, kattr_t attr, __kernel void *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict reqsize);
extern __crit __wunused __nonnull((1,4))     kerrno_t kproc_kernel_setattrfd_c(struct kproc *__restrict self, int fd, kattr_t attr, __kernel void const *__restrict buf, __size_t bufsize);
#else
#define kproc_kernel_readfd_c(self,fd,buf,bufsize,rsize)           KTASK_KEPD(kproc_user_readfd_c(self,fd,buf,bufsize,rsize))
#define kproc_kernel_writefd_c(self,fd,buf,bufsize,wsize)          KTASK_KEPD(kproc_user_writefd_c(self,fd,buf,bufsize,wsize))
#define kproc_kernel_preadfd_c(self,fd,pos,buf,bufsize,rsize)      KTASK_KEPD(kproc_user_preadfd_c(self,fd,pos,buf,bufsize,rsize))
#define kproc_kernel_pwritefd_c(self,fd,pos,buf,bufsize,wsize)     KTASK_KEPD(kproc_user_pwritefd_c(self,fd,pos,buf,bufsize,wsize))
#define kproc_kernel_fcntlfd_c(self,fd,cmd,arg)                    KTASK_KEPD(kproc_user_fcntlfd_c(self,fd,cmd,arg))
#define kproc_kernel_ioctlfd_c(self,fd,cmd,arg)                    KTASK_KEPD(kproc_user_ioctlfd_c(self,fd,cmd,arg))
#define kproc_kernel_getattrfd_c(self,fd,attr,buf,bufsize,reqsize) KTASK_KEPD(kproc_user_getattrfd_c(self,fd,attr,buf,bufsize,reqsize))
#define kproc_kernel_setattrfd_c(self,fd,attr,buf,bufsize)         KTASK_KEPD(kproc_user_setattrfd_c(self,fd,attr,buf,bufsize))
#endif

extern __crit __wunused __nonnull((1))       kerrno_t kproc_seekfd_c(struct kproc *__restrict self, int fd, __off_t off, int whence, __kernel __pos_t *newpos);
extern __crit __wunused __nonnull((1))       kerrno_t kproc_truncfd_c(struct kproc *__restrict self, int fd, __pos_t size);
extern __crit __wunused __nonnull((1))       kerrno_t kproc_flushfd_c(struct kproc *__restrict self, int fd);
extern __crit __wunused __nonnull((1,3,4,5)) kerrno_t kproc_readdirfd_c(struct kproc *__restrict self, int fd, __ref struct kinode **__restrict inode,
                                                                        struct kdirentname **__restrict name, __ref struct kfile **__restrict fp, __u32 flags);
#ifdef __INTELLISENSE__
extern __wunused __nonnull((1,3,5))   kerrno_t kproc_user_readfd(struct kproc *__restrict self, int fd, __user void *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict rsize);
extern __wunused __nonnull((1,3,5))   kerrno_t kproc_user_writefd(struct kproc *__restrict self, int fd, __user void const *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict wsize);
extern __wunused __nonnull((1,4,6))   kerrno_t kproc_user_preadfd(struct kproc *__restrict self, int fd, __pos_t pos, __user void *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict rsize);
extern __wunused __nonnull((1,4,6))   kerrno_t kproc_user_pwritefd(struct kproc *__restrict self, int fd, __pos_t pos, __user void const *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict wsize);
extern __wunused __nonnull((1))       kerrno_t kproc_user_fcntlfd(struct kproc *__restrict self, int fd, int cmd, __user void *arg);
extern __wunused __nonnull((1))       kerrno_t kproc_user_ioctlfd(struct kproc *__restrict self, int fd, kattr_t cmd, __user void *arg);
extern __wunused __nonnull((1,4))     kerrno_t kproc_user_getattrfd(struct kproc *__restrict self, int fd, kattr_t attr, __user void *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict reqsize);
extern __wunused __nonnull((1,4))     kerrno_t kproc_user_setattrfd(struct kproc *__restrict self, int fd, kattr_t attr, __user void const *__restrict buf, __size_t bufsize);
extern __wunused __nonnull((1,3,5))   kerrno_t kproc_kernel_readfd(struct kproc *__restrict self, int fd, __kernel void *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict rsize);
extern __wunused __nonnull((1,3,5))   kerrno_t kproc_kernel_writefd(struct kproc *__restrict self, int fd, __kernel void const *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict wsize);
extern __wunused __nonnull((1,4,6))   kerrno_t kproc_kernel_preadfd(struct kproc *__restrict self, int fd, __pos_t pos, __kernel void *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict rsize);
extern __wunused __nonnull((1,4,6))   kerrno_t kproc_kernel_pwritefd(struct kproc *__restrict self, int fd, __pos_t pos, __kernel void const *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict wsize);
extern __wunused __nonnull((1))       kerrno_t kproc_kernel_fcntlfd(struct kproc *__restrict self, int fd, int cmd, __kernel void *arg);
extern __wunused __nonnull((1))       kerrno_t kproc_kernel_ioctlfd(struct kproc *__restrict self, int fd, kattr_t cmd, __kernel void *arg);
extern __wunused __nonnull((1,4))     kerrno_t kproc_kernel_getattrfd(struct kproc *__restrict self, int fd, kattr_t attr, __kernel void *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict reqsize);
extern __wunused __nonnull((1,4))     kerrno_t kproc_kernel_setattrfd(struct kproc *__restrict self, int fd, kattr_t attr, __kernel void const *__restrict buf, __size_t bufsize);
extern __wunused __nonnull((1))       kerrno_t kproc_seekfd(struct kproc *__restrict self, int fd, __off_t off, int whence, __pos_t *newpos);
extern __wunused __nonnull((1))       kerrno_t kproc_truncfd(struct kproc *__restrict self, int fd, __pos_t size);
extern __wunused __nonnull((1))       kerrno_t kproc_flushfd(struct kproc *__restrict self, int fd);
#else
#define kproc_user_readfd(self,fd,buf,bufsize,rsize)             KTASK_CRIT(kproc_user_readfd_c(self,fd,buf,bufsize,rsize))
#define kproc_user_writefd(self,fd,buf,bufsize,wsize)            KTASK_CRIT(kproc_user_writefd_c(self,fd,buf,bufsize,wsize))
#define kproc_user_preadfd(self,fd,pos,buf,bufsize,rsize)        KTASK_CRIT(kproc_user_preadfd_c(self,fd,pos,buf,bufsize,rsize))
#define kproc_user_pwritefd(self,fd,pos,buf,bufsize,wsize)       KTASK_CRIT(kproc_user_pwritefd_c(self,fd,pos,buf,bufsize,wsize))
#define kproc_user_fcntlfd(self,fd,cmd,arg)                      KTASK_CRIT(kproc_user_fcntlfd_c(self,fd,cmd,arg))
#define kproc_user_ioctlfd(self,fd,cmd,arg)                      KTASK_CRIT(kproc_user_ioctlfd_c(self,fd,cmd,arg))
#define kproc_user_getattrfd(self,fd,attr,buf,bufsize,reqsize)   KTASK_CRIT(kproc_user_getattrfd_c(self,fd,attr,buf,bufsize,reqsize))
#define kproc_user_setattrfd(self,fd,attr,buf,bufsize)           KTASK_CRIT(kproc_user_setattrfd_c(self,fd,attr,buf,bufsize))
#define kproc_kernel_readfd(self,fd,buf,bufsize,rsize)           KTASK_CRIT(kproc_kernel_readfd_c(self,fd,buf,bufsize,rsize))
#define kproc_kernel_writefd(self,fd,buf,bufsize,wsize)          KTASK_CRIT(kproc_kernel_writefd_c(self,fd,buf,bufsize,wsize))
#define kproc_kernel_preadfd(self,fd,pos,buf,bufsize,rsize)      KTASK_CRIT(kproc_kernel_preadfd_c(self,fd,pos,buf,bufsize,rsize))
#define kproc_kernel_pwritefd(self,fd,pos,buf,bufsize,wsize)     KTASK_CRIT(kproc_kernel_pwritefd_c(self,fd,pos,buf,bufsize,wsize))
#define kproc_kernel_fcntlfd(self,fd,cmd,arg)                    KTASK_CRIT(kproc_kernel_fcntlfd_c(self,fd,cmd,arg))
#define kproc_kernel_ioctlfd(self,fd,cmd,arg)                    KTASK_CRIT(kproc_kernel_ioctlfd_c(self,fd,cmd,arg))
#define kproc_kernel_getattrfd(self,fd,attr,buf,bufsize,reqsize) KTASK_CRIT(kproc_kernel_getattrfd_c(self,fd,attr,buf,bufsize,reqsize))
#define kproc_kernel_setattrfd(self,fd,attr,buf,bufsize)         KTASK_CRIT(kproc_kernel_setattrfd_c(self,fd,attr,buf,bufsize))
#define kproc_seekfd(self,fd,off,whence,newpos)                  KTASK_CRIT(kproc_seekfd_c(self,fd,off,whence,newpos))
#define kproc_truncfd(self,fd,size)                              KTASK_CRIT(kproc_truncfd_c(self,fd,size))
#define kproc_flushfd(self,fd)                                   KTASK_CRIT(kproc_flushfd_c(self,fd))
#endif

typedef kerrno_t (*penumenv)(char const *name, __size_t namesize, char const *value, __size_t valuesize, void *closure);
typedef kerrno_t (*penumcmd)(char const *arg, void *closure);

//////////////////////////////////////////////////////////////////////////
// Environment variables interface (mp-safe)
// WARNING: The callback given to for environ enumeration must be
//          self-contained and not invoke other env-related functions!
//         (e.g.: Don't attempt to delete an environment variable)
// @return: KE_ACCES:     GETMEM/SETMEM permissions for the root task are
//                        required for accessing environment variables.
// @return: KE_DESTROYED: The given process is a zombie.
// @return: KE_NOENT:    [kproc_getenv*] The given environment variable doesn't exist.
extern __crit __wunused __nonnull((1,2,4)) kerrno_t kproc_getenv_ck(struct kproc *__restrict self, char const *__restrict name, __size_t namemax, char *buf, __size_t bufsize, __size_t *reqsize);
extern __crit __wunused __nonnull((1,2,4)) kerrno_t kproc_setenv_ck(struct kproc *__restrict self, char const *__restrict name, __size_t namemax, char const *__restrict value, __size_t valuemax, int override);
extern __crit __wunused __nonnull((1,2))   kerrno_t kproc_delenv_ck(struct kproc *__restrict self, char const *__restrict name, __size_t namemax);
extern __crit __wunused __nonnull((1,2))   kerrno_t kproc_enumenv_ck(struct kproc *__restrict self, penumenv callback, void *closure);
extern __crit __wunused __nonnull((1,2,4)) kerrno_t kproc_getenv_c(struct kproc *__restrict self, char const *__restrict name, __size_t namemax, char *buf, __size_t bufsize, __size_t *reqsize);
extern __crit __wunused __nonnull((1,2,4)) kerrno_t kproc_setenv_c(struct kproc *__restrict self, char const *__restrict name, __size_t namemax, char const *__restrict value, __size_t valuemax, int override);
extern __crit __wunused __nonnull((1,2))   kerrno_t kproc_delenv_c(struct kproc *__restrict self, char const *__restrict name, __size_t namemax);
extern __crit __wunused __nonnull((1,2))   kerrno_t kproc_enumenv_c(struct kproc *__restrict self, penumenv callback, void *closure);
#ifdef __INTELLISENSE__
extern __wunused __nonnull((1,2,4)) kerrno_t kproc_getenv_k(struct kproc *__restrict self, char const *__restrict name, __size_t namemax, char *buf, __size_t bufsize, __size_t *reqsize);
extern __wunused __nonnull((1,2,4)) kerrno_t kproc_setenv_k(struct kproc *__restrict self, char const *__restrict name, __size_t namemax, char const *__restrict value, __size_t valuemax, int override);
extern __wunused __nonnull((1,2))   kerrno_t kproc_delenv_k(struct kproc *__restrict self, char const *__restrict name, __size_t namemax);
extern __wunused __nonnull((1,2))   kerrno_t kproc_enumenv_k(struct kproc *__restrict self, penumenv callback, void *closure);
extern __wunused __nonnull((1,2,4)) kerrno_t kproc_getenv(struct kproc *__restrict self, char const *__restrict name, __size_t namemax, char *buf, __size_t bufsize, __size_t *reqsize);
extern __wunused __nonnull((1,2,4)) kerrno_t kproc_setenv(struct kproc *__restrict self, char const *__restrict name, __size_t namemax, char const *__restrict value, __size_t valuemax, int override);
extern __wunused __nonnull((1,2))   kerrno_t kproc_delenv(struct kproc *__restrict self, char const *__restrict name, __size_t namemax);
extern __wunused __nonnull((1,2))   kerrno_t kproc_enumenv(struct kproc *__restrict self, penumenv callback, void *closure);
#else
#define kproc_getenv_k(self,name,namemax,buf,bufsize,reqsize)     KTASK_CRIT(kproc_getenv_ck(self,name,namemax,buf,bufsize,reqsize))
#define kproc_setenv_k(self,name,namemax,value,valuemax,override) KTASK_CRIT(kproc_setenv_ck(self,name,namemax,value,valuemax,override))
#define kproc_delenv_k(self,name,namemax)                         KTASK_CRIT(kproc_delenv_ck(self,name,namemax))
#define kproc_enumenv_k(self,callback,closure)                    KTASK_CRIT(kproc_enumenv_ck(self,callback,closure))
#define kproc_getenv(self,name,namemax,buf,bufsize,reqsize)       KTASK_CRIT(kproc_getenv_c(self,name,namemax,buf,bufsize,reqsize))
#define kproc_setenv(self,name,namemax,value,valuemax,override)   KTASK_CRIT(kproc_setenv_c(self,name,namemax,value,valuemax,override))
#define kproc_delenv(self,name,namemax)                           KTASK_CRIT(kproc_delenv_c(self,name,namemax))
#define kproc_enumenv(self,callback,closure)                      KTASK_CRIT(kproc_enumenv_c(self,callback,closure))
#endif


//////////////////////////////////////////////////////////////////////////
// Commandline interface (mp-safe)
// WARNING: The callback given to for environ enumeration must be
//          self-contained and not invoke other env-related functions!
//         (e.g.: Don't attempt to delete an environment variable)
// @return: KE_DESTROYED: The given process is a zombie.
extern __crit __wunused __nonnull((1,2)) kerrno_t kproc_enumcmd_ck(struct kproc *__restrict self, penumcmd callback, void *closure);
#ifdef __INTELLISENSE__
extern        __wunused __nonnull((1,2)) kerrno_t kproc_enumcmd_k(struct kproc *__restrict self, penumcmd callback, void *closure);
extern __crit __wunused __nonnull((1,2)) kerrno_t kproc_enumcmd_c(struct kproc *__restrict self, penumcmd callback, void *closure);
extern        __wunused __nonnull((1,2)) kerrno_t kproc_enumcmd(struct kproc *__restrict self, penumcmd callback, void *closure);
#else
#define kproc_enumcmd_k(self,callback,closure) KTASK_CRIT(kproc_enumcmd_ck(self,callback,closure))
#define kproc_enumcmd_c kproc_enumcmd_ck
#define kproc_enumcmd   kproc_enumcmd_k
#endif



/* ===============================================
 * Process --> task interface
 * >> Since a task context is basically a process,
 *    all of its tasks are considered its threads.
 * =============================================== */

//////////////////////////////////////////////////////////////////////////
// Return the a given task by its thread id (tid)
// @return: NULL: Invalid tid/dead task/task context was closed
extern __crit __wunused __nonnull((1)) __ref struct ktask *
kproc_getthread(struct kproc const *__restrict self, __ktid_t tid);
extern __crit __nonnull((1,2)) int
kproc_enumthreads_c(struct kproc const *__restrict self,
                    int (*callback)(struct ktask *__restrict thread,
                                    void *closure),
                    void *closure);
#ifdef __INTELLISENSE__
extern __crit __nonnull((1,2)) int
kproc_enumthreads(struct kproc const *__restrict self,
                  int (*callback)(struct ktask *__restrict thread,
                                  void *closure),
                  void *closure);
#else
#define kproc_enumthreads(self,callback,closure) \
 KTASK_CRIT(kproc_enumthreads_c(self,callback,closure))
#endif

//////////////////////////////////////////////////////////////////////////
// Add a given task to the context's list of tasks.
// @return: KE_NOMEM:     Not enough memory to allocate the task.
// @return: KE_DESTROYED: The given context was closed.
extern __wunused __nonnull((1,2)) kerrno_t kproc_addtask(struct kproc *__restrict self, struct ktask *__restrict task);
extern           __nonnull((1,2))     void kproc_deltask(struct kproc *__restrict self, struct ktask *__restrict task);

//////////////////////////////////////////////////////////////////////////
// Get/Set generic attributes of a given process
// NOTE: Unrecognized attributes are passed to the root task of the given process.
// @return: KE_DESTROYED: The given process is a zombie (or doesn't have a root task.)
extern __crit __wunused __nonnull((1,3)) kerrno_t kproc_user_getattr_c(struct kproc *__restrict self, kattr_t attr, __user void *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict reqsize);
extern __crit __wunused __nonnull((1,3)) kerrno_t kproc_user_setattr_c(struct kproc *__restrict self, kattr_t attr, __user void const *__restrict buf, __size_t bufsize);
#ifdef __INTELLISENSE__
extern __crit __wunused __nonnull((1,3)) kerrno_t kproc_kernel_getattr_c(struct kproc *__restrict self, kattr_t attr, __kernel void *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict reqsize);
extern __crit __wunused __nonnull((1,3)) kerrno_t kproc_kernel_setattr_c(struct kproc *__restrict self, kattr_t attr, __kernel void const *__restrict buf, __size_t bufsize);
extern __wunused __nonnull((1,3)) kerrno_t kproc_user_getattr(struct kproc *__restrict self, kattr_t attr, __user void *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict reqsize);
extern __wunused __nonnull((1,3)) kerrno_t kproc_user_setattr(struct kproc *__restrict self, kattr_t attr, __user void const *__restrict buf, __size_t bufsize);
extern __wunused __nonnull((1,3)) kerrno_t kproc_kernel_getattr(struct kproc *__restrict self, kattr_t attr, __kernel void *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict reqsize);
extern __wunused __nonnull((1,3)) kerrno_t kproc_kernel_setattr(struct kproc *__restrict self, kattr_t attr, __kernel void const *__restrict buf, __size_t bufsize);
#else
#define kproc_kernel_getattr_c(self,attr,buf,bufsize,reqsize) KTASK_KEPD(kproc_user_getattr_c(self,attr,buf,bufsize,reqsize))
#define kproc_kernel_setattr_c(self,attr,buf,bufsize)         KTASK_KEPD(kproc_user_setattr_c(self,attr,buf,bufsize))
#define kproc_user_getattr(self,attr,buf,bufsize,reqsize)     KTASK_CRIT(kproc_user_getattr_c(self,attr,buf,bufsize,reqsize))
#define kproc_user_setattr(self,attr,buf,bufsize)             KTASK_CRIT(kproc_user_setattr_c(self,attr,buf,bufsize))
#define kproc_kernel_getattr(self,attr,buf,bufsize,reqsize)   KTASK_KEPD(kproc_user_getattr(self,attr,buf,bufsize,reqsize))
#define kproc_kernel_setattr(self,attr,buf,bufsize)           KTASK_KEPD(kproc_user_setattr(self,attr,buf,bufsize))
#endif


//////////////////////////////////////////////////////////////////////////
// Load a given module into a process
//  - If possible, the module will be relocated to a free memory
//    location, unless it must be linked statically, in which case the load
//    process may fail when the requested area of memory is already in use.
//  - [*_unlocked] The caller must be holding a write-lock to
//                'self->p_shm.s_lock' and the 'KPROC_LOCK_MODS' lock.
//  - Modules not unloaded when the a process is closed
//    (aka. turns into a zombie) are unloaded automatically.
//  - kproc_insmod_single_unlocked: Upon success, the caller must not free() 'depids'
// NOTE: [!*_single*] will not just load the given module, but all of its dependencies as well.
// @return: KE_RANGE:     One or more sections with fixed addresses overlap with already
//                        mapped virtual memory. (The executable and its dependencies
//                        cannot run in this configuration)
// @return: KE_NOMEM:     Not enough available memory.
// @return: KE_NOSPC:     Failed to find an unused region of virtual memory
//                        big enough to fit a position-independent section.
// @return: KE_OVERFLOW:  Failed to acquire a new reference to the given module.
// @return: KS_UNCHANGED: The given module was already loaded (a recursive load-counter was incremented, though)
// @return: DE_DESTROYED: [!*_unlocked] The given process is a zombie.
extern __crit __wunused __nonnull((1,2,3,4)) kerrno_t
kproc_insmod_single_unlocked(struct kproc *__restrict self,
                             struct kshlib *__restrict module,
                             kmodid_t *__restrict module_id,
                             kmodid_t *__restrict depids);
extern __crit __wunused __nonnull((1,2,3)) kerrno_t
kproc_insmod_unlocked(struct kproc *__restrict self,
                      struct kshlib *__restrict module,
                      kmodid_t *__restrict module_id);
extern __crit __wunused __nonnull((1,2,3)) kerrno_t
kproc_insmod_c(struct kproc *__restrict self,
               struct kshlib *__restrict module,
               kmodid_t *__restrict module_id);
#define kproc_insmod(self,module,module_id) \
 KTASK_CRIT(kproc_insmod_c(self,module,module_id))

//////////////////////////////////////////////////////////////////////////
// Unload a given module from a process after it had previously been loaded.
//  - [!*_single*] will also unload all dependencies of the given module.
//  - kproc_delmod_single_unlocked: '*dep_idv' and '*dep_idc' is filled with a 
//    vector of dependency module ids that must be unloaded by the caller.
//    NOTE: The caller must also 'free(*dep_idv)' when they are done.
//  - [kproc_delmod2*] Similar to 'kproc_delmod*', but also supports special module ids.
// @return: KE_OK:        The module was unloaded successfully.
// @return: KE_INVAL:     The given module wasn't loaded in this process.
// @return: KS_UNCHANGED: The module was loaded more than once (its load
//                        counter was decremented, but didn't reach ZERO)
// @return: DE_DESTROYED: [!*_unlocked] The given process is a zombie.
extern __crit __nonnull((1,3,4)) kerrno_t kproc_delmod_single_unlocked(struct kproc *__restrict self, kmodid_t module_id, kmodid_t **dep_idv, __size_t *dep_idc);
extern __crit __nonnull((1)) kerrno_t kproc_delmod_unlocked(struct kproc *__restrict self, kmodid_t module_id);
extern __crit __nonnull((1)) kerrno_t kproc_delmod_c(struct kproc *__restrict self, kmodid_t module_id);
extern __crit __nonnull((1)) kerrno_t kproc_delmod2_c(struct kproc *__restrict self, kmodid_t module_id, __user void *caller_eip);

#define kproc_delmod(self,module_id)             KTASK_CRIT(kproc_delmod_c(self,module_id))
#define kproc_delmod2(self,module_id,caller_eip) KTASK_CRIT(kproc_delmod2_c(self,module_id,caller_eip))

//////////////////////////////////////////////////////////////////////////
// Retrieves the module ID of a given user-land EIP address.
// @return: KE_OK:    *module_id was filled with the correct module id.
// @return: KE_FAULT: The given EIP does not map to any module.
extern __crit __nonnull((1,2)) kerrno_t
kproc_getmodat_unlocked(struct kproc *__restrict self,
                        kmodid_t *__restrict module_id,
                        __user void *addr);

#ifndef __ksymhash_t_defined
#define __ksymhash_t_defined 1
typedef __size_t       ksymhash_t;
#endif

//////////////////////////////////////////////////////////////////////////
// Resolves a module symbol from the module with the given ID by its name.
// NOTE: [*_unlocked] requires the caller holding the 'KPROC_LOCK_MODS' lock.
// WARNING: The returned pointer, even when non-NULL, may not be mapped
// @return: NULL: No such symbol was found.
extern __crit __wunused __nonnull((1)) __user void *
kproc_dlsymex_unlocked(struct kproc *__restrict self,
                       kmodid_t module_id, char const *__restrict name,
                       __size_t name_size, ksymhash_t name_hash);
extern __crit __wunused __nonnull((1)) __user void *
kproc_dlsymex_c(struct kproc *__restrict self,
                kmodid_t module_id, char const *__restrict name,
                __size_t name_size, ksymhash_t name_hash);
extern __crit __wunused __nonnull((1)) __user void *
kproc_dlsymex2_c(struct kproc *__restrict self,
                 kmodid_t module_id, char const *__restrict name,
                 __size_t name_size, ksymhash_t name_hash,
                 __user void *caller_eip);
__local __crit __wunused __nonnull((1)) __user void *
kproc_dlsym_unlocked(struct kproc *__restrict self,
                     kmodid_t module_id, char const *__restrict name);
__local __crit __wunused __nonnull((1)) __user void *
kproc_dlsym_c(struct kproc *__restrict self,
              kmodid_t module_id, char const *__restrict name);
#ifdef __INTELLISENSE__
extern __wunused __nonnull((1)) __user void *
kproc_dlsymex(struct kproc *__restrict self,
              kmodid_t module_id, char const *__restrict name,
              __size_t name_size, ksymhash_t name_hash);
extern __wunused __nonnull((1)) __user void *
kproc_dlsymex2(struct kproc *__restrict self,
               kmodid_t module_id, char const *__restrict name,
               __size_t name_size, ksymhash_t name_hash,
               __user void *caller_eip);
extern __wunused __nonnull((1)) __user void *
kproc_dlsym(struct kproc *__restrict self,
            kmodid_t module_id, char const *__restrict name);
#else
#define kproc_dlsymex(self,module_id,name,name_size,name_hash) \
 KTASK_CRIT(kproc_dlsymex_c(self,module_id,name,name_size,name_hash))
#define kproc_dlsymex2(self,module_id,name,name_size,name_hash,caller_eip) \
 KTASK_CRIT(kproc_dlsymex2_c(self,module_id,name,name_size,name_hash,caller_eip))
#define kproc_dlsym(self,module_id,name) \
 KTASK_CRIT(kproc_dlsym_c(self,module_id,name))
#endif


/* Max allowed PID value.
 * >> Posix wants this to be a signed value, so
 *    there goes half of all possible values... */
#define PID_MAX    INT32_MAX

//////////////////////////////////////////////////////////////////////////
// Global list of processes
struct kproclist {
 struct kmutex  pl_lock;  /*< Lock for all members below (closed during shutdown to prevent new processes from spawning). */
 __u32          pl_free;  /*< [lock(pl_lock)] A hint towards a free slot (May be used, or out-of-bounds). */
 __u32          pl_proca; /*< [lock(pl_lock)] Allocated vector size. */
 struct kproc **pl_procv; /*< [0..1][0..pl_proca][owned][lock(pl_lock)] Vector of processes. */
};
#define KPROCLIST_INIT  {KMUTEX_INIT,0,0,NULL}


#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Returns the global process list
extern __retnonnull __constcall struct kproclist *kproclist_global(void);
#else
#define kproclist_global() (&__kproclist_global)
extern struct kproclist __kproclist_global;
#endif

//////////////////////////////////////////////////////////////////////////
// Adds a given process to the global process list,
// initializing its 'p_pid' member along the way.
// @return: KE_DESTROYED: The process list was destroyed (No new processes may be spawed).
// @return: KE_NOMEM:     Not enough memory to reallocate the global process vector.
// @return: KE_OVERFLOW:  The resulting PID would have been too great (> PID_MAX)
extern __crit __nonnull((1)) kerrno_t kproclist_addproc(struct kproc *__restrict proc);
extern __crit __nonnull((1))     void kproclist_delproc(struct kproc *__restrict proc);

//////////////////////////////////////////////////////////////////////////
// Closes the process list, thus preventing any new processes from spawning.
// @return: KS_UNCHANGED: The process list was already closed.
extern __crit kerrno_t kproclist_close(void);


//////////////////////////////////////////////////////////////////////////
// Returns a reference to the process associated with a given PID
// NOTE: Permissions are checked to ensure that the caller is
//       allowed to view the process associated with the given PID.
// @return: NULL: Invalid PID or the caller has insufficient
//                permissions to open access process, or the system
//                is shutting down (in which case no PID is valid).
extern __crit __wunused __ref struct kproc *kproclist_getproc(__u32 pid);
extern __crit __wunused __ref struct kproc *kproclist_getproc_k(__u32 pid);

//////////////////////////////////////////////////////////////////////////
// Enumerate all process ids visible to the caller.
// @return: KE_DESTROYED: The process list was destroyed (No new processes may be spawed).
extern __crit kerrno_t kproclist_enumpid(__pid_t pidv[], __size_t pidc,
                                         __size_t *__restrict reqpidc);

//////////////////////////////////////////////////////////////////////////
// Enumerate all processes, storing a reference to each in the provided buffer.
// NOTE: If the buffer is not big enough, nothing is stored.
// @return: *: The required amount of buffer entries.
//             Unless this value is <= 'procc', 'procv' does not contain references.
extern __crit __size_t
kproclist_enumproc(__ref struct kproc *procv[], __size_t procc);



#ifdef __MAIN_C__
extern void kernel_initialize_process(void);
extern void kernel_finalize_process(void);
#endif /* __MAIN_C__ */

#ifndef __INTELLISENSE__
#ifndef __ksymhash_of_defined
#define __ksymhash_of_defined 1
extern __wunused __nonnull((1)) ksymhash_t
ksymhash_of(char const *__restrict text, __size_t size);
#endif

__local __crit __user void *
kproc_dlsym_unlocked(struct kproc *__restrict self,
                     kmodid_t module_id, char const *__restrict name) {
 __size_t name_size = strlen(name);
 return kproc_dlsymex_unlocked(self,module_id,name,name_size,
                               ksymhash_of(name,name_size));
}
__local __crit __user void *
kproc_dlsym_c(struct kproc *__restrict self,
              kmodid_t module_id, char const *__restrict name) {
 __size_t name_size = strlen(name);
 return kproc_dlsymex_c(self,module_id,name,name_size,
                        ksymhash_of(name,name_size));
}
#ifdef __KOS_KERNEL_SHM_H__
__local __crit kerrno_t
__ktranslator_init_intr(struct ktranslator *__restrict self,
                        struct ktask *__restrict caller) {
 kerrno_t error = KE_OK;
 KTASK_CRIT_MARK
 kassertobj(self);
 kassert_ktask(caller);
 self->t_shm  = kproc_getshm(ktask_getproc(caller));
 kassert_kshm(self->t_shm);
 self->t_epd  = caller->t_epd;
 if (self->t_epd == kpagedir_kernel()) {
  self->t_flags = KTRANSLATOR_FLAG_KEPD;
 } else if (self->t_epd != self->t_shm->s_pd) {
  self->t_flags = KTRANSLATOR_FLAG_NONE;
 } else {
  self->t_flags = KTRANSLATOR_FLAG_LOCK|KTRANSLATOR_FLAG_SEPD;
  KTASK_NOINTR_BEGIN
  error = krwlock_beginread(&self->t_shm->s_lock);
  KTASK_NOINTR_END
 }
 return error;
}
__local __crit /*__nointr*/ kerrno_t
__ktranslator_init_nointr(struct ktranslator *__restrict self,
                          struct ktask *__restrict caller) {
 kerrno_t error = KE_OK;
 KTASK_CRIT_MARK
 KTASK_NOINTR_MARK
 kassertobj(self);
 kassert_ktask(caller);
 self->t_shm  = kproc_getshm(ktask_getproc(caller));
 kassert_kshm(self->t_shm);
 self->t_epd  = caller->t_epd;
 if (self->t_epd == kpagedir_kernel()) {
  self->t_flags = KTRANSLATOR_FLAG_KEPD;
 } else if (self->t_epd != self->t_shm->s_pd) {
  self->t_flags = KTRANSLATOR_FLAG_NONE;
 } else {
  self->t_flags = KTRANSLATOR_FLAG_LOCK|KTRANSLATOR_FLAG_SEPD;
  error = krwlock_beginread(&self->t_shm->s_lock);
 }
 return error;
}
#endif /* __KOS_KERNEL_SHM_H__ */
#endif /* !__INTELLISENSE__ */
#endif /* !__ASSEMBLY__ */

__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_PROC_H__ */

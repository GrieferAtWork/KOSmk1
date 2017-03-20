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
#ifndef __KOS_PERM_H__
#define __KOS_PERM_H__ 1

#include <kos/config.h>
#include <kos/compiler.h>
#include <kos/errno.h>
#include <kos/types.h>
#include <kos/endian.h>
#ifndef __NO_PROTOTYPES
#include <kos/syscall.h>
#include <kos/syscallno.h>
#endif /* !__NO_PROTOTYPES */

__DECL_BEGIN

#ifndef __ASSEMBLY__
#ifndef __ktaskprio_t_defined
#define __ktaskprio_t_defined 1
typedef __ktaskprio_t ktaskprio_t;
#endif
typedef __u32 kperm_name_t;
typedef __u32 kperm_flag_t;
#endif /* !__ASSEMBLY__ */

#ifdef __ASSEMBLY__
#define KPERM_NAME(name,T)      (name)
#define KPERM_GETID(perm)      ((perm)&0x00ffffff)
#else /* __ASSEMBLY__ */
#define KPERM_NAME(name,T)     ((sizeof(T) << 24)|(name))
#define KPERM_GETID(perm)      ((perm)&0x00ffffff)
#define KPERM_GETSIZE(perm)   (((perm)&0xff000000) >> 24)
#endif /* !__ASSEMBLY__ */
#define KPERM_FLAG(group,mask) ((group) << 16|(mask))

#define KPERM_NAME_NONE                      0
#define KPERM_NAME_FLAG           KPERM_NAME(1,kperm_flag_t) /*< Various permission flags (see below). */
#define KPERM_NAME_PRIO_MIN       KPERM_NAME(2,ktaskprio_t)  /*< Lowest allowed task priority to-be set (NOTE: 'KPERM_FLAG_PRIO_SUSPENDED' can still regulate 'KTASK_PRIORITY_SUSPENDED' individually). */
#define KPERM_NAME_PRIO_MAX       KPERM_NAME(3,ktaskprio_t)  /*< Highest allowed task priority to-be set (NOTE: 'KPERM_FLAG_PRIO_REALTIME' can still regulate 'KTASK_PRIORITY_REALTIME' individually). */
#define KPERM_NAME_TASKNAME_MAX   KPERM_NAME(4,__size_t)     /*< Max name length when setting a task name (Set to '0' to prevent task names from being set). */
#define KPERM_NAME_PIPE_MAX       KPERM_NAME(5,__size_t)     /*< Max max_size value when creating/configuring pipes (Attempts at setting greater values are truncated). */
#define KPERM_NAME_SYSLOG         KPERM_NAME(6,int)          /*< One of KLOG_*: Only log levels >= this are allowed. */
#define KPERM_NAME_FDMAX          KPERM_NAME(7,unsigned int) /*< Max amount of file descriptors; greatest allowed descriptor index+1. (NOTE: Excluding special descriptors such as KFD_ROOT and KFD_CWD) */
#define KPERM_NAME_ENV_MEMMAX     KPERM_NAME(8,__size_t)     /*< Max amount of memory allowed for argv+environ. */
#define KPERM_NAME_MAXTHREADS     KPERM_NAME(9,__size_t)     /*< Max amount of threads allowed to run (Can only prevent new threads from spawning; not kill existing ones; does not affect fork()). */
#define KPERM_NAME_MAXMEM         KPERM_NAME(10,__size_t)    /*< Max amount of bytes the process may allocate (Rounded up to pages; Counting all memory, including shared, device, read-only and .text; cannot free memory previously allocated). */
#define KPERM_NAME_MAXMEM_RW      KPERM_NAME(11,__size_t)    /*< Max amount of bytes the process may allocate (Rounded up to pages; Only counting writable memory, but still including writable shared and device memory; cannot free memory previously allocated). */

//////////////////////////////////////////////////////////////////////////
// === Permission flags ===
// Split into a a group and mask each, groups are used to extend
// the amount of possible flags far beyond what is normally possible when
// using regular masks (e.g.: 32-bit flags can only represent 32 different flags).
// 
// Yet due to this, reading flag permissions is a bit more complicated.
// Where all other permission names only require you set 'p_name',
// reading permission flags requires you to set 'p_name = KPERM_NAME_FLAG',
// as well as 'p_data.d_flag_group = *' to read permissions for that group.
// >> More on that below.
#define KPERM_FLAG_GETGROUP(flag) (((flag)&0xffff0000) >> 16)
#define KPERM_FLAG_GETMASK(flag)   ((flag)&0xffff)
#define KPERM_FLAG_GROUPCOUNT        4 /*< Amount of different groups currently defined. */
#define KPERM_FLAG_NONE              0

/* Permissions-related permission flags. */
#define KPERM_FLAG_GETPERM          KPERM_FLAG(0,0x0001) /*< Allow the process to read permissions (NOTE: When removed, will only affect the next call to 'kproc_perm'). */
#define KPERM_FLAG_SETPERM          KPERM_FLAG(0,0x0002) /*< Allow the process to set its own permissions (NOTE: When removed, will only affect the next call to 'kproc_perm'). */
#define KPERM_FLAG_GETPERM_OTHER    KPERM_FLAG(0,0x0004) /*< Allow the process to read permissions of other processes (Requires 'KPERM_FLAG_GETPERM') (NOTE: When removed, will only affect the next call to 'kproc_perm'). */
#define KPERM_FLAG_SETBARRIER       KPERM_FLAG(0,0x0010) /*< Allow the process to use 'kproc_barrier' for changing its task barrier. */

/* Tasking/scheduler-related permission flags. */
#define KPERM_FLAG_CANFORK          KPERM_FLAG(1,0x0001) /*< Allow calls to 'ktask_fork'. */
#define KPERM_FLAG_CANROOTFORK      KPERM_FLAG(1,0x0002) /*< Allow calls to 'ktask_fork' with 'KTASK_NEW_FLAG_ROOTFORK' set. (Ignored if 'KPERM_FLAG_CANFORK' isn't set).
                                                             NOTE: This flag only controls if a task may even attempt to rootfork(), it does not affect the additional
                                                                   permission checks performed, as described in the documentation of 'KTASK_NEW_FLAG_ROOTFORK'. */
#define KPERM_FLAG_PRIO_SUSPENDED   KPERM_FLAG(1,0x0004) /*< Allow 'KTASK_PRIORITY_SUSPENDED' even when prio_min..prio_max wouldn't allow it. */
#define KPERM_FLAG_PRIO_REALTIME    KPERM_FLAG(1,0x0008) /*< Allow 'KTASK_PRIORITY_REALTIME' even when prio_min..prio_max wouldn't allow it. */
#define KPERM_FLAG_SETPRIO          KPERM_FLAG(1,0x0010) /*< Allow the process to use 'ktask_setpriority' for setting the priority of a task. */
#define KPERM_FLAG_GETPRIO          KPERM_FLAG(1,0x0020) /*< Allow the process to use 'ktask_getpriority' for setting the priority of a task. */
#define KPERM_FLAG_SUSPEND_RESUME   KPERM_FLAG(0,0x0040) /*< Allow the process to use 'ktask_suspend'/'ktask_resume' to suspend/resume a given task. */

/* System/BIOS-related permission flags. */
#define KPERM_FLAG_CHTIME           KPERM_FLAG(2,0x0001) /*< Allow the process to change the system time. */

/* Filesystem/File descriptor-related permission flags. */
#define KPERM_FLAG_CHROOT           KPERM_FLAG(3,0x0001) /*< Allow closing/reopening the KFD_ROOT special file descriptor (without this, KFD_ROOT is immutable). */
#define KPERM_FLAG_CHDIR            KPERM_FLAG(3,0x0002) /*< Allow closing/reopening the KFD_CWD special file descriptor (without this, KFD_CWD is immutable). */


#define KPERM_MAXDATASIZE   (1 << 8) /*< Max size that the 'p_data' block of permissions can grow to. */

#ifndef __ASSEMBLY__
struct __packed kperm {
union __packed {
    kperm_name_t     p_name;       /*< Permission name. */
struct __packed {
#if __BYTE_ORDER == __LITTLE_ENDIAN
    unsigned int     p_id   : 24;  /*< ID of the permission. */
    unsigned int     p_size : 8;   /*< Size of the data block below (in bytes; only lower bytes are used). */
#else
    unsigned int     p_size : 8;   /*< Size of the data block below (in bytes; only lower bytes are used). */
    unsigned int     p_id   : 24;  /*< ID of the permission. */
#endif
};};
    union __packed {
struct __packed {
#if __BYTE_ORDER == __LITTLE_ENDIAN
        __u16        d_flag_mask;  /*< For convenience: Mask of permissions. */
        __u16        d_flag_group; /*< For convenience: Permissions group the mask belongs to. */
#else
        __u16        d_flag_group; /*< For convenience: Permissions group the mask belongs to. */
        __u16        d_flag_mask;  /*< For convenience: Mask of permissions. */
#endif
};
        kperm_flag_t d_flag;       /*< For convenience: Group+mask of a given permissions flag. */
        ktaskprio_t  d_prio;       /*< For convenience: a priority-field. */
        __size_t     d_size;       /*< For convenience: a size-field. */
        int          d_int;        /*< For convenience: an int-field. */
        unsigned int d_uint;       /*< For convenience: a uint-field. */
        __byte_t     d_bytes[1];   /*< [:r_size] Raw data in bytes (Max size is 'KPERM_MAXDATASIZE'). */
    } p_data;
};
#endif /* !__ASSEMBLY__ */


#ifndef __NO_PROTOTYPES
//////////////////////////////////////////////////////////////////////////
// Read/Modify permissions of a process.
// This function can be used in 3 different modes: get/set/exchange.
//  - To get permissions, you will need the GETPROP/VISIBLE permission
//    for the given process, or acquire its handle in some other way.
//  - To set permissions, you must be 'PROCFD', as KOS only
//    allows a process to modify its own permissions.
//    Also note that all permissions are designed only to be
//    taken away, with no way of being returned to the caller.
//  - Attempting to extend permissions normally aborts execution
//    of the permissions buffer, unless 'KPROC_PERM_MODE_IGNORE'
//    is set, in which case such attempts are silently ignored.
//    NOTE: If mode is 'KPROC_PERM_MODE_XCH', the old permissions
//          value is still written to the 'p_data' area of the buffer.
//  - The amount of permission elements in the given buffer is specified
//    with the 'elem_count' argument, allowing you to modify any number
//    of permissions with a single system call.
//  - Attempting to set permissions not known to the kernel will
//    either cause the function to terminate with 'KE_INVAL', or
//    when the 'KPROC_PERM_MODE_UNKNOWN' flag was set in 'mode',
//    ignore that permission and continue processing more requests.
//  - Attempting to get permissions/flags not known to the kernel will
//    cause the kernel to overwrite the 'p_id' field of that permission
//    with 'KPERM_GETID(KPERM_NAME_NONE)', but not stop or fail with an error.
//    The kernel will also fill the 'p_data' area with ZERO(0), but not
//    modify the value of 'p_size'.
//    
// WARNING: When setting permission flags, instead of specifying the
//          new set of flags after the call, you must specify the set
//          of flags that should be removed.
// Example for checking a process for a permission flag:
// >> // @return:  0: The process does not have the permission flag set.
// >> // @return:  1: The process does have the permission flag set.
// >> // @return: -1: An error occurred (see errno).
// >> static int has_permission(int proc, kperm_flag_t flag) {
// >>   kerrno_t error;
// >>   struct kperm p;
// >>   p.p_name              = KPERM_NAME_FLAG;
// >>   p.p_data.d_flag_group = KPERM_FLAG_GETGROUP(flag);
// >>   error = kproc_perm(proc,&p,1,KPROC_PERM_MODE_GET);
// >>   if (KE_ISERR(error)) { errno = -error; return -1; }
// >>   /* Extract the flag mask. */
// >>   flag = KPERM_FLAG_GETMASK(flag);
// >>   return (p.p_data.d_flag_mask & flag) == flag;
// >> }
// Example for removing a given permission:
// >> // @return:  0: The permission was already removed.
// >> // @return:  1: Successfully removed the permission.
// >> // @return: -1: An error occurred (see errno).
// >> static int unset_permission(kperm_flag_t flag) {
// >>   kerrno_t error;
// >>   struct kperm p;
// >>   p.p_name        = KPERM_NAME_FLAG;
// >>   p.p_data.d_flag = flag; /* Set the flags we want to remove, not those we want to keep! */
// >>   /* Exchange permissions, so the callback will
// >>    * return the old permissions of the group. */
// >>   error = kproc_perm(proc,&p,1,KPROC_PERM_MODE_XCH);
// >>   if (KE_ISERR(error)) { errno = -error; return -1; }
// >>   /* Extract the flag mask. */
// >>   flag = KPERM_FLAG_GETMASK(flag);
// >>   /* Check if the flag was previously set. */
// >>   return (p.p_data.d_flag_mask & flag) == flag;
// >> }
// Example for setting multiple permissions at once:
// >> // @return:  0: The permissions were successfully set.
// >> // @return: -1: An error occurred (see errno).
// >> static int restrict_prio(void) {
// >>   kerrno_t error;
// >>   struct kperm p[2];
// >>   p[0].p_name = KPERM_NAME_PRIO_MIN;
// >>   p[1].p_name = KPERM_NAME_PRIO_MAX;
// >>   /* Overwrite the size member to align the permissions buffer by our array size.
// >>    * This way we can tell the kernel where the next element starts. */
// >>   p[0].p_size = sizeof(p[0].p_data);
// >>   p[1].p_size = sizeof(p[1].p_data);
// >>   /* Set the new min/max priority levels. */
// >>   p[0].p_data.d_prio = -10;
// >>   p[1].p_data.d_prio = +10;
// >>   /* Set the new permissions (Two at once)
// >>    * Ignore failure if we're already more restricted than -10..+10).
// >>    * HINT: Use 'KPROC_PERM_MODE_XCH' to simultaniously extract the old permissions. */
// >>   error = kproc_perm(proc,&p[0],2,KPROC_PERM_MODE_SET|KPROC_PERM_MODE_IGNORE);
// >>   if (KE_ISERR(error)) { errno = -error; return -1; }
// >>   return 0;
// >> }
// NOTE: When using 'KPROC_PERM_MODE_GET' or 'KPROC_PERM_MODE_XCH' to
//       retrieve information about permissions, on input each entry of 'buf'
//       contains the 'd_size' of the associated data block and implicitly
//       inform the kernel where the next permission (if existing) can be
//       found at.
//       Yet upon return, the kernel will overwrite the 'd_size' fields
//       with the actually required (aka. minimum size) for that permission,
//       meaning that the caller will have to keep track of where in the
//       provided buffer different permissions are located at, or simply
//       define a static array of permissions using constant element sizes
//       to determine their locations.
// NOTE: Even when the specified data size is smaller than what would be
//       required, the 'd_size' field will still be set to what the kernel
//       needs upon exit after only reading/writing as many bytes from the
//       provided buffer as possible.
//      'KE_BUFSIZE' is returned when attempting to set/exchange a permission
//       with a data block too small to fit what would otherwise be required.
//       This way, applications can be written with forward-compatibility
//       by defining a way for userland applications to figure out the actual
//       data size of a permission at runtime, instead of just relying on
//       the data sizes implicitly defined by the various permission ids,
//       as well as allow for permissions to be added with potentially variadic
//       data blocks.
// @param: procfd: A file descriptor for a process or task. When for a task, the associated process is used instead.
// @return: KE_OK:        Successfully read/set/exchanged the given permissions.
//                        NOTE: If 'KPROC_PERM_MODE_IGNORE' was set, some permissions
//                              may not have been set properly due to prior, more
//                              restrictive permission modifications.
// @return: KE_ACCESS:   [!(mode&KPROC_PERM_MODE_IGNORE) || (mode&0xf) != KPROC_PERM_MODE_GET]
//                       [!caller_has_permission(KPERM_FLAG_GETPERM) && (mode&0xf) != KPROC_PERM_MODE_SET]
//                       [!caller_has_permission(KPERM_FLAG_SETPERM) && (mode&0xf) != KPROC_PERM_MODE_GET]
//                        You are either not permitted to read permissions, change your own permissions,
//                        or are attempting to modify the permissions of a process other than kproc_self().
//                        NOTE: Even when 'KPROC_PERM_MODE_IGNORE' is set, this error may still
//                              be returned if you are not allowed to get/set permissions at all.
// @return: KE_INVAL:    [!(mode&KPROC_PERM_MODE_UNKNOWN) && (mode&0xf) != KPROC_PERM_MODE_GET] Attempted to set an unknown permission.
// @return: KE_TIMEDOUT:  A blocking operation took place and timed out due to a prior call to ktask_setalarm()
// @return: KE_DESTROYED: Either the given, or the calling process was destroyed.
// @return: KE_BADF:      The given file descriptor is invalid or not a task/process.
// @return: KE_FAULT:     The given buf or a part of it describes a faulty/unmapped area of memory.
// @return: KE_BUFSIZE:  [(mode&0xf) != KPROC_PERM_MODE_GET] The 'p_size' field was of insufficient size.
__local _syscall4(kerrno_t,kproc_perm,int,procfd,
                  struct kperm /*const_if((mode&0xf) == KPROC_PERM_MODE_SET)*/ *,buf,
                  __size_t,elem_count,int,mode);
#endif /* !__NO_PROTOTYPES */
#define KPROC_PERM_MODE_GET        0 /*< Read restrictions of a process. */
#define KPROC_PERM_MODE_SET        1 /*< Write restrictions of a process (Only ever allowed if 'procfd' refers to 'kproc_self()'). */
#define KPROC_PERM_MODE_XCH        2 /*< Exchange restrictions */
#define KPROC_PERM_MODE_IGNORE  0x10 /*< FLAG: Ignore illegal attempts at extending permissions and continue parsing more requests. */
#define KPROC_PERM_MODE_UNKNOWN 0x20 /*< FLAG: Ignore attempts at setting unknown permissions. */


__DECL_END

#endif /* !__KOS_PERM_H__ */

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
#ifndef __KOS_KERNEL_TASK2_CPU_H__
#define __KOS_KERNEL_TASK2_CPU_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/attr.h>
#include <kos/errno.h>
#include <kos/compiler.h>
#include <kos/kernel/features.h>
#include <kos/kernel/object.h>
#include <kos/kernel/types.h>
#include <kos/kernel/tss.h>
#include <kos/kernel/signal.h>
#include <kos/kernel/task2.h>

__DECL_BEGIN

#if defined(__i386__)
/* Try to reduce stack overhead during scheduler calls. */
#define __SCHED_CALL __fastcall
#else
#define __SCHED_CALL
#endif

#define KOBJECT_MAGIC_CPU2      0xC9D      /*< CPU. */

/* Max amount of CPUs that may be used.
 * NOTE: If you look at the 0x0f mask required in the apic version of 'cpu_self',
 *       you will see that 1 << 4 (aka. 16) is actually the maximum possible _ever_.
 *       >> So setting this to anything higher won't actually do you any good...
 * NOTE: This also means that any ID greater than that may carry special meaning. */
#define KCPU2_MAXCOUNT 16

#define KCPU2_IRQNUM   32


#ifndef __ASSEMBLY__
struct kproc;
struct ktask2;
struct kcpu2;

#ifdef __DEBUG__
#define kassert_kcpu2(self) \
 __xblock({ struct kcpu2 *const __kcpuself = (self);\
            kassert_object(__kcpuself,KOBJECT_MAGIC_CPU2);\
            assert(__kcpuself >= kcpu2_vector);\
            assert(__kcpuself <  kcpu2_vector+KCPU2_MAXCOUNT);\
            assertf(__kcpuself->c_id == (__u8)(__kcpuself-kcpu2_vector),\
                    "%I8u != %Iu",__kcpuself->c_id,__kcpuself-kcpu2_vector);\
            (void)0;\
 })
#else
#define kassert_kcpu2(self)      kassert_object(self,KOBJECT_MAGIC_CPU2)
#endif


struct kcpu2 {
 KOBJECT_HEAD
 /* NOTE: When holding the 'KCPU2_FLAG_INUSE' flag, as returned by 'kcpu2_turnon_begin'
  *       opon success on a valid, stopped CPU, all of the members below may be accessed
  *       without additional locking until the CPU is actually started through 'kcpu2_turnon_end'.
  *       >> During that time, the caller should fill 'c_current' and 'c_sleeping'
  *          with their desired chain of tasks to-be executed on this CPU. */
 __ref struct ktask2 *c_current;  /*< [1..1][lock(KCPU2_LOCK_TASKS)][ring] Task currently running on this CPU (Keep this at offset ZERO(0)). */
 struct kcpu2        *c_self;     /*< Self-pointer (Used to quickly acquire a pointer to this structure from the segment register) */
#define KCPU2_LOCK_TASKS 0x01 /*< Lock for pending tasks.
                               *  When used in conjunction with 'KTASK_LOCK_STATE',
                               *  this lock must be acquire second. */
 __atomic __u8        c_locks;
#define KCPU2_FLAG_NONE      0x00
#define KCPU2_FLAG_PRESENT   0x01 /*< [const] This CPU is available for use (But may already be running something).
                                   *  WARNING: Unless this flag is set, all other members are in an undefined state.
                                   *  NOTE:    Though you can assume that it is set for your own CPU.
                                   *  NOTE2:   Even in debug mode, it should be noted that all 'KCPU2_MAXCOUNT'
                                   *           CPUs have their KOBJECT_HEAD initialized, meaning that a 'kassert_kcpu2()'
                                   *           check will succeed even for CPUs that are otherwise unavailable and invalid. */
#define KCPU2_FLAG_ENABLED   0x02 /*< [atomic] This CPU is enabled for use
                                   *  Unless set, this CPU should be considered as unavailable in most cases, meaning
                                   *  that it neither counts towards online, nor offline CPUs, but does count towards
                                   *  the total amount of available CPUs. */
#define KCPU2_FLAG_INUSE     0x04 /*< [atomic] Set at the start of CPU initialization.
                                   *           After being set, the caller has exclusive access to the
                                   *          'KCPU2_FLAG_STARTED' and 'KCPU2_FLAG_RUNNING' flags until
                                   *           they manage to bring the CPU online.
                                   *           After that point, the then running CPU owns this flag indefinitely,
                                   *           or until it decides to go offline (at which point the instruction
                                   *           stream of the CPU is responsible of removing this flag once again,
                                   *           alongside the 'KCPU2_FLAG_STARTED' and 'KCPU2_FLAG_RUNNING' ones).
                                   *        >> Basically, when trying to bring a CPU online, katomic_fetchor() this
                                   *           flag to the others to ensure that the CPU isn't already online, and
                                   *           that no other CPU is trying to bring it online either.
                                   *   NOTE: Performing a soft-off using the 'KCPU2_FLAG_TURNOFF' flag will not unset this flag,
                                   *         meaning that upon waiting for the CPU to really turn of, the caller will inherit
                                   *         this flag just as it would have been acquired by 'kcpu2_turnon_begin()'. */
#define KCPU2_FLAG_TURNOFF   0x08 /*< [atomic] Ask nicely to turn off the CPU at the next appropriate time (During the next IRQ switch).
                                   *   NOTE: The process of this shutdown will not remove the 'KCPU2_FLAG_INUSE' flags, though it
                                   *         will remove the 'KCPU2_FLAG_STARTED' and 'KCPU2_FLAG_RUNNING' flags, meaning that
                                   *         the caller inherits the 'KCPU2_FLAG_INUSE', allowing them to keep the CPU offline
                                   *         by acting as though it was already in use, until that caller decides to remove that
                                   *         flag, allowing the CPU once again to be brought online.
                                   *   HINT: After being turned off, the caller must either turn it back on, of move all running/sleeping
                                   *         tasks to another CPU in order to not sporadically, or accidentally terminate critical tasks.
                                   *   NOTE: The process of this soft-off is used during shutdown, allowing KOS to return to single-core
                                   *         operations, as well as safely terminate all tasks still running at that point.
                                   *   NOTE: Alongside the 'KCPU2_FLAG_ENABLED' flag, it can also be used to dynamically disable
                                   *         a certain CPU on-the-fly without the need of rebooting the entire operating system.
                                   *   NOTE: Upon acknowledgement, the cpu itself will update 'kcpu2_online'. */
#define KCPU2_FLAG_KEEPON    0x10 /*< [lock(KCPU2_FLAG_INUSE|ME)] When set, this CPU may not be turned off (Used in conjunction with ). */
#define KCPU2_FLAG_STARTED   0x20 /*< [lock(KCPU2_FLAG_INUSE|ME)] Set by CPU init code (Used to confirm an intended startup). */
#define KCPU2_FLAG_RUNNING   0x40 /*< [lock(KCPU2_FLAG_INUSE|ME)] Must be set by bootstrap code (Used to confirm a successful start sequence and indicates a fully online CPU). */
#define KCPU2_FLAG_NOIRQ     0x80 /*< [atomic] Don't perform IRQ preemption. */
 __u8                 c_flags;    /*< Set of 'KCPU2_FLAG_*'. */
 kcpuid_t             c_id;	      /*< [const] CPU/LAPIC ID ('kcpu2_vector' vector index). */
 __u8                 c_lapicver; /*< [const] LAPIC version (s.a.: 'APICVER_*'). */
 ksegid_t             c_segid;    /*< [const] Segment ID of this CPU's per-cpu GDT segment (This segment points to the base of this structure). */
 __u16                c_padding;  /*< Padding... */
 __ref struct ktask2 *c_sleeping; /*< [0..1][lock(KCPU2_LOCK_TASKS)][list] Task who's abstime will pass next. */
 __size_t             c_nrunning; /*< [lock(KCPU2_LOCK_TASKS)] Amount of running tasks (Not counting 'c_idle'). */
 __size_t             c_nsleeping;/*< [lock(KCPU2_LOCK_TASKS)] Amount of sleeping tasks. */
 struct ktss          c_tss;      /*< [lock(ME)] Per-CPU task state segment (required for syscalls). */
 struct ktask2        c_idle;     /*< [lock(ME)] Per-CPU IDLE task scheduled when no others are running
                                   *            (Used to prevent 'c_current' from ever dropping to ZERO(0)).
                                   *   NOTE: While doing nothing other than waiting for the next IRQ to happen,
                                   *         this IDLE task will usually  */
 struct ksignal       c_offline;  /*< This signal is closed when the CPU goes offline, and reset during a call to 'kcpu2_turnon_begin'.
                                   *  Using this signal, one can wait for a CPU to turn off without having to resort to a spinlock. */
#ifdef __x86__
 __u32 c_x86_features; /*< [const] X86-specific processor features (Set of 'X86_CPUFEATURE_*'). */
#endif
};
#define kcpu2_isenabled(self)                     ((self)->c_flags&KCPU2_FLAG_ENABLED)
#define kcpu2_ispresent(self)                     ((self)->c_flags&KCPU2_FLAG_PRESENT)
#define kcpu2_isonline(self)         (katomic_load((self)->c_flags)&KCPU2_FLAG_RUNNING)
#define kcpu2_inuse(self)            (katomic_load((self)->c_flags)&KCPU2_FLAG_INUSE)
#define kcpu2_inuse_notrunning(self) ((katomic_load((self)->c_flags)&(KCPU2_FLAG_STARTED|KCPU2_FLAG_RUNNING|KCPU2_FLAG_INUSE))==(KCPU2_FLAG_INUSE))

#define kcpu2_islocked(self,lock)  (katomic_load((self)->c_locks)&lock)
#define kcpu2_trylock(self,lock) (!(katomic_fetchor((self)->c_locks,lock)&(lock)))
#define kcpu2_lock(self,lock)       KTASK_SPIN(kcpu2_trylock(self,lock))
#define kcpu2_unlock(self,lock)     assertef((katomic_fetchand((self)->c_locks,~(lock))&(lock))!=0,"Lock not held")


#define KCPU2_FOREACH_ALL(x)     for ((x) = kcpu2_vector; (x) != kcpu2_vector+KCPU2_MAXCOUNT; ++(x))
#define KCPU2_FOREACH_PRESENT(x) for ((x) = kcpu2_vector; (x) != kcpu2_vector+KCPU2_MAXCOUNT; ++(x)) if (!kcpu2_ispresent(x));else


extern          __size_t      kcpu2_total;                  /*< [!0][const] Total amount of available CPUs (either online, or offline). */
extern __atomic __size_t      kcpu2_enabled;                /*< Amount of CPUs currently enabled. */
extern __atomic __size_t      kcpu2_online;                 /*< Amount of CPUs currently online. */
extern __atomic __size_t      kcpu2_offline;                /*< Amount of CPUs currently offline. */
extern          struct kcpu2  kcpu2_vector[KCPU2_MAXCOUNT]; /*< [const] Global vector of all CPUs (index is the CPU's id; aka. 'c_id'). */
extern          struct kcpu2 *kcpu2_bootcpu;                /*< [1..1][const] Control entry for the boot processor. */
extern __atomic struct kcpu2 *kcpu2_hwirq;                  /*< [1..1] The CPU currently set up to receive hardware interrupts
                                                             *   NOTE: This CPU must not go offline, but instead run
                                                             *         it's IDLE task if there are no other jobs.
                                                             *         This is achieved with the 'KCPU2_FLAG_KEEPON' flag,
                                                             *         meaning that this CPU _must_ have that flag set. */


//////////////////////////////////////////////////////////////////////////
// Called during early initialization to
// collect information about available CPUs.
extern void kernel_initialize_cpu(void);

//////////////////////////////////////////////////////////////////////////
// Called later during initialization to setup the control
// structures of all CPUs detected during 'kernel_initialize_cpu()'
// This is required to safely begin making use of additional CPUs during
// scheduling, meaning that prior to this call nothing that may require to
// make use of a traditional mutex, or any other kind of signal can safely be used.
// >> This is turning-point, after which everything workds as far as SMP goes.
extern void kernel_initialize_scheduler(void);

//////////////////////////////////////////////////////////////////////////
// Restore a system similar to that prior to SMP initialization:
//  - Terminate all tasks still running on _any_ CPU (except for the call task)
//  - Disable the preemption IRQ (This will also disable automatic real-time adjustments)
//  - Turn off and disable all CPUs but for the boot processor.
//  - Switching to the boot processor if execution
//    previously decided to move the caller to another one.
//  - Destroying the IDLE task structures of all CPUs (including the caller's).
// >> Upon return, SMP will no longer work, but the system can safely proceed
//    with other shutdown-related operations, knowing that they're the only
//    string of instructions still present within the entire operating system.
extern void kernel_finalize_scheduler(void);


//////////////////////////////////////////////////////////////////////////
// Returns a pointer to the first CPU that was offline at the time of this
// function being called (aka. didn't have the 'KCPU2_FLAG_INUSE' flag set).
// NOTE: This function is safe to call at all times, not performing any locking
//       and also making sure not to return a CPU that isn't available.
// HINT: 'kcpu2_getoffline_maybe_disabled' can be used to include
//        CPUs that were disabled at the time of this call.
// WARNING: By the time this function returns, things may have already changed,
//          meaning that the returned CPU may no longer be offline, thus forcing
//          the caller to either only use this function for performance hits, or
//          in conjunction with an katomic_fetchof(), passing the 'KCPU2_FLAG_INUSE' flag.
// @return: * :   A pointer to a CPU that was offline at the time of this call.
//                NOTE: The caller may assume that this CPU is available ('KCPU2_FLAG_PRESENT' is set)
// @return: NULL: No offline CPUs found.
extern __wunused struct kcpu2 *kcpu2_getoffline(void);
extern __wunused struct kcpu2 *kcpu2_getoffline_maybe_disabled(void);

//////////////////////////////////////////////////////////////////////////
// Returns the control structure for the calling CPU.
// NOTE: Unless the calling thread is re-scheduled on a different CPU, this value
//       will not change, as IRQ preemption won't ever swap tasks to different CPUs.
#ifdef __INTELLISENSE__
extern __wunused __retnonnull struct kcpu2 *kcpu2_self(void);
#else
#define kcpu2_self() \
 __xblock({ register struct kcpu2 *__cself;\
            __asm__("mov %%" KCPU2_SELF_SEGMENT_S ":" __PP_STR(KCPU2_OFFSETOF_SELF) ", %0\n" : "=g" (__cself));\
            __xreturn __cself;\
 })
#endif


//////////////////////////////////////////////////////////////////////////
// Attempt to wake the given running CPU by sending an IPI (Inter-Processor-Interrupt),
// thus allowing it to respond to a shutdown request faster that having to wait for
// the next IRQ preemption interrupt.
// @return: KE_OK:        Successfully send the IPI request.
// @return: KE_DEVICE:    The device didn't respond to the IPI request.
// @return: KE_DESTROYED: The given CPU is not running ('KCPU2_FLAG_RUNNING' is not set)
#define kcpu2_wakeipi(self) kcpu2_apic_int(self,KCPU2_IRQNUM)

//////////////////////////////////////////////////////////////////////////
// The main function for the CPU IDLE task.
extern __noreturn void kcpu2_idlemain(void);
extern void kcpu2_preempt(struct kirq_registers *__restrict regs);

//////////////////////////////////////////////////////////////////////////
// Enable/Disable a given CPU for use by scheduling.
// WARNING: The caller is responsible to pass a valid CPU (one having the 'KCPU2_FLAG_PRESENT' flag set)
// @return: KE_OK:        Successfully disabled/enabled a given CPU.
//                        WARNING: When enabling, this only means that the CPU may once again be booted.
// @return: KE_DEADLK:    [kcpu2_disable] Attempted to disable the last CPU still enabled.
// @return: KE_PERM:      [kcpu2_disable] The given CPU is configured not to be disabled.
// @return: KS_UNCHANGED: The given CPU was already enabled/disabled.
extern __crit __wunused kerrno_t __SCHED_CALL kcpu2_disable(struct kcpu2 *__restrict self);
extern __crit __wunused kerrno_t __SCHED_CALL kcpu2_enable(struct kcpu2 *__restrict self);


//////////////////////////////////////////////////////////////////////////
// Begin/End turning on a given CPU.
// >> After 'kcpu2_turnon_begin' is called, the caller holds
//    the exclusive lock provided by the 'KCPU2_FLAG_INUSE' flag,
//    allowing them to fill the 'c_current' and 'c_sleeping' task chains
//    their their liking before calling 'kcpu2_turnon_end' to try and
//    actually turn on the CPU in order for it to start executing
//    the actual code.
// NOTE: 'kcpu2_turnon_begin' will reset the join-signal of the CPU,
//        as closed when it goes offline, meaning that the caller
//        is responsible not to hold a lock to it for the duration
//        of that call.
// NOTE: If the caller chooses to join the cpu (in the sense of waiting for it to turn off),
//       they can easily achieve this by locking the 'c_offline' signal before
//       calling 'kcpu2_turnon_end', then do a _ksignal_recv_andunlock() call,
//       or by simply calling 'kcpu2_turnon_end_and_join', which does the same thing.
// NOTE: If before the associated call to 'kcpu2_turnon_end*' an error occurrs
//       that forces the caller to abort initialization, 'kcpu2_turnon_abort()'
//       can be called to delete flags set by 'kcpu2_turnon_begin'
// WARNING: The caller is responsible to pass a valid CPU (one having the 'KCPU2_FLAG_PRESENT' flag set)
// WARNING: The caller must 'kcpu2_turnon_abort()' after cleanup upon failure or 'kcpu2_turnon_end()'
// @return: KE_OK:       [kcpu2_turnon_begin] Successfully acquired the 'KCPU2_FLAG_INUSE' flag.
// @return: KE_BUSY:     [kcpu2_turnon_begin] The 'KCPU2_FLAG_INUSE' flag was already set, indicating that the CPU is already in use.
// @return: KE_OK:       [kcpu2_turnon_end*] Successfully started the given CPU.
// @return: KE_DEVICE:   [kcpu2_turnon_end*] The device didn't respond to the INIT command.
// @return: KE_PERM:     [kcpu2_turnon_end*] The CPU didn't accept the INIT request.
// @return: KE_TIMEDOUT: [kcpu2_turnon_end*] Code at the given START_EIP was either
//                                           not executed, or failed to acknowledge
//                                           'KCPU2_FLAG_STARTED' with 'KCPU2_FLAG_RUNNING'.
//                                           Another possibility is that a previously set
//                                           timeout has expired in the calling thread.
// @return: KE_INTR:     [kcpu2_turnon_end*] The calling thread was interrupted.
// @return: * :          [kcpu2_turnon_begin_firstoffline] The CPU that the caller may now initialize before turning it on.
// @return: NULL:        [kcpu2_turnon_begin_firstoffline] All existing CPUs are online (*found_cpu is left untouched).
extern __crit __wunused __nonnull((1)) kerrno_t __SCHED_CALL kcpu2_turnon_begin(struct kcpu2 *__restrict self);
extern __crit __wunused         struct kcpu2   *__SCHED_CALL kcpu2_turnon_begin_firstoffline(void);
extern __crit __wunused __nonnull((1)) kerrno_t __SCHED_CALL kcpu2_turnon_end(struct kcpu2 *__restrict self);
extern __crit           __nonnull((1))     void __SCHED_CALL kcpu2_turnon_abort(struct kcpu2 *__restrict self);

//////////////////////////////////////////////////////////////////////////
// Softly turn off the CPU by setting the 'KCPU2_FLAG_TURNOFF' flag.
// NOTE: Upon success, these functions (except for 'kcpu2_softoff_async')
//       behave identical to a successful call to 'kcpu2_turnon_begin()',
//       meaning that you can safely turn the CPU back on through a
//       call to either 'kcpu2_turnon_end*', or make it available for
//       being turned on again by calling 'kcpu2_turnon_abort'.
// WARNING: These functions way _NOT_ be used to turn off the calling CPU!
//          Attempting to do so will cause an assertion failure in debug mode,
//          or potentially a system-wide softlock when assertions are disabled.
// kcpu2_softoff_idle:   Idly wait for the CPU to acknowledge the shutdown.
// kcpu2_softoff_yield:  Similar to 'kcpu2_softoff_idle', but yield execution within the calling CPU while waiting.
// kcpu2_softoff_signal: Using the 'c_offline' signal, wait for the CPU to shut down without consuming any CPU time.
//              WARNING: Depending on how long until the next preemption in the given CPU, this may actually be more expensive.
//                 NOTE: To prevent the potential of dealing with situation without solution
//                       in the event of KE_INTR, or KE_TIMEDOUT, neither will be checked
//                       for when calling 'kcpu2_softoff_signal'.
// kcpu2_softoff_async:  Just set the soft_off flag, but don't perform any operation to wait until that is acknowledged.
// @return: KE_OK: Successfully set the 'KCPU2_FLAG_TURNOFF' flag ([!kcpu2_softoff_async] and waited until the CPU has acknowledged this)
//                 [!kcpu2_softoff_async] The 'KCPU2_FLAG_TURNOFF' flag has also been removed again, and
//                                        the caller has inherited the 'KCPU2_FLAG_INUSE' flag, allowing
//                                        them to re-setup the CPU to their own liking.
// @return: KS_UNCHANGED: The given CPU was not fully online when attempting to set the 'KCPU2_FLAG_TURNOFF' flag
//                        (aka. neither the 'KCPU2_FLAG_STARTED' nor 'KCPU2_FLAG_RUNNING' flags were set)
// @return: KE_BUSY: The 'KCPU2_FLAG_TURNOFF' flag was already set, meaning that
//                   the CPU is currently being turned off by another thread.
//                >> In most situations, this can be interpreted as equivalent to 'KE_DESTROYED'.
// @return: KE_PERM: The given CPU is configured to not be disabled ('KCPU2_FLAG_KEEPON').
// @return: KE_WOULDBLOCK: [kcpu2_softoff_async] The CPU is currently being started (try again in a few moments)
extern __crit __wunused __nonnull((1)) kerrno_t __SCHED_CALL kcpu2_softoff_idle(struct kcpu2 *__restrict self);
extern __crit __wunused __nonnull((1)) kerrno_t __SCHED_CALL kcpu2_softoff_yield(struct kcpu2 *__restrict self);
extern __crit __wunused __nonnull((1)) kerrno_t __SCHED_CALL kcpu2_softoff_async(struct kcpu2 *__restrict self);
extern __crit __wunused __nonnull((1)) kerrno_t __SCHED_CALL kcpu2_softoff_signal(struct kcpu2 *__restrict self);


//////////////////////////////////////////////////////////////////////////
// Add a given task to the chain of currently running ones.
// NOTE: The caller is expected to either hold the 'KCPU2_FLAG_INUSE' lock,
//       as provided by a call to 'kcpu2_turnon_begin' during setup, or
//       the 'KCPU2_LOCK_TASKS' lock at any other point in time.
// WARNING: 'kcpu2_addcurrent_front_unlocked' behaves somewhat differently,
//          as it requires the caller to either hold the 'KCPU2_FLAG_INUSE' lock
//          on a CPU currently being set up, or to be the the given cpu 'self',
//          in which case the caller is required to safely perform the switch
//          to that CPU once additional setup has been performed.
// HINT: In debug mode, all of the above are asserted to the best of our ability.
// NOTE: The 't_cpu' field of the given task will not be modified, but will be
//       asserted for equality with the given CPU 'self', meaning that the caller
//       is responsible for updating that pointer (if necessary)
// HINT: If the given task is the first (aka. 'self' only contains the IDLE task as current one),
//       these functions will automatically unlink the IDLE task.
// HINT: These functions will also update the 'c_nrunning' field.
// WARNING: These functions will modify 't_prev' and 't_next' of 'task'.
extern __crit void __SCHED_CALL kcpu2_addcurrent_front_unlocked(struct kcpu2 *__restrict self, __ref struct ktask2 *__restrict task);
extern __crit void __SCHED_CALL kcpu2_addcurrent_next_unlocked(struct kcpu2 *__restrict self, __ref struct ktask2 *__restrict task);
extern __crit void __SCHED_CALL kcpu2_addcurrent_back_unlocked(struct kcpu2 *__restrict self, __ref struct ktask2 *__restrict task);

//////////////////////////////////////////////////////////////////////////
// Add the IDLE task as the one that will be switched to next.
// >> This function is called if some other CPU requires 'SELF'
//    to switch out its current task for another.
// NOTE: If the IDLE task was already scheduled, it will be re-schedule
//       as the next TASK to switch to, unless the IDLE task _is_ the
//       current task, in which case an internal assertion will fail.
extern __crit void __SCHED_CALL kcpu2_addcurrent_idle_unlocked(struct kcpu2 *__restrict self);

//////////////////////////////////////////////////////////////////////////
// Remove a given task from the chain of those currently running on a given CPU.
// NOTE: Based on the position of that task inside the CPU 'self',
//       the caller must be holding different locks/flags, matching
//       those required by 'kcpu2_addcurrent_*_unlocked':
//       - If the given task is the first (aka. 'current') task, the caller
//         must either be the given CPU 'self', or hold the 'KCPU2_FLAG_INUSE'
//         flag (as available during setup).
//         HINT: This is asserted.
//         HINT: To safely remove the ~true~ current task of a CPU,
//               you must unlock 'KCPU2_LOCK_TASKS', wait for the
//               next IRQ switch for the task to be swapped out, then
//               proceed to re-acquire the 'KCPU2_LOCK_TASKS' lock
//               and retry until the task is no longer at the front.
//         NOTE: In the case that the given task is the only one running,
//               you may also choose to turn off the CPU using the
//               'KCPU2_FLAG_TURNOFF' flag, wait for it to actually turn off
//               by either idling until the 'KCPU2_FLAG_TURNOFF' and
//               'KCPU2_FLAG_RUNNING' flags have been unset, before proceeding
//               to remove the task safely, and re-start the CPU if there
//               are other tasks that the IDLE one remaining.
//       - For any other position, the 'KCPU2_LOCK_TASKS' lock is acceptable, too.
// WARNING: Do not attempt to delete the IDLE task (An assertion will fail)
// WARNING: Do not attempt to delete a task that's not scheduled on the CPU 'self' (An assertion will fail)
// WARNING: Do not attempt to delete a task scheduled on a different CPU (An assertion will fail)
// HINT: If the given task is the last, this function will automatically schedule the IDLE task.
// HINT: This function will also update the 'c_nrunning' field.
// WARNING: This function will modify 't_prev' and 't_next' of 'task'.
extern __crit void __SCHED_CALL kcpu2_delcurrent_unlocked(struct kcpu2 *__restrict self, __ref struct ktask2 *__restrict task);

//////////////////////////////////////////////////////////////////////////
// Add/Delete a given task to/from the chain of sleeping tasks.
// NOTE: 'kcpu2_addsleeping_unlocked' will read the 't_timeout' member of the given task
//       to figure out where to insert the task in the sorted list of sleepers.
// NOTE: In debug mode, assertions similar to those of 'kcpu2_(add|del)current_unlocked'
//       are performed (requiring a 'KCPU2_FLAG_INUSE' or 'KCPU2_LOCK_TASKS' lock), yet
//       no special handling for adding/deleting the first sleeping task is required.
//       This also entails checks for the given task actually having its t_cpu field
//       be equal to 'self'.
// HINT: These functions will update the 'c_nsleeping' field.
// WARNING: These functions will modify 't_prev' and 't_next' of 'task'.
extern __crit void __SCHED_CALL kcpu2_addsleeping_unlocked(struct kcpu2 *__restrict self, __ref struct ktask2 *__restrict task);
extern __crit void __SCHED_CALL kcpu2_delsleeping_unlocked(struct kcpu2 *__restrict self, __ref struct ktask2 *__restrict task);

//////////////////////////////////////////////////////////////////////////
// Similar to 'kcpu2_addsleeping_unlocked', but insert an already-sorted
// chain of tasks connected through 't_next->...', starting at the given task.
// NOTE: Unlike 'kcpu2_addsleeping_unlocked', the caller is _NOT_ responsible
//       for ensuring that the 't_cpu' pointers of the specified tasks are
//       properly set, but instead can rely on this function to update
//       them all into pointing towards the specified CPU 'self'.
extern __crit void __SCHED_CALL kcpu2_addsleeping_all_unlocked(struct kcpu2 *__restrict self, __ref struct ktask2 *task);

//////////////////////////////////////////////////////////////////////////
// Causes the calling CPU to inherit all tasks from the given CPU 'self'.
// NOTE: The caller must be holding the 'KCPU2_FLAG_INUSE' flag on the given CPU.
extern __crit void __SCHED_CALL kcpu2_inherit_tasks(struct kcpu2 *__restrict source);
extern __crit void
__SCHED_CALL kcpu2_inherit_tasks_unlocked(struct kcpu2 *__restrict self,
                                          struct kcpu2 *__restrict source);


//////////////////////////////////////////////////////////////////////////
// Using the given time 'tmnow', wake all tasks with a
// timeout lower that that described by 'tmnow'.
// NOTE: In the even multiple tasks are woken, the one with the
//       lowest timeout will be scheduled at the front of 'c_current'.
// NOTE: The caller must either be holding a lock to 'KCPU2_LOCK_TASKS',
//       or not be the given CPU and hold the 'KCPU2_FLAG_INUSE' flag.
// @return: 0 : No tasks were woken, meaning that 'c_current' and 'c_sleeping' remained unmodified.
// @return: * : The amount of woken tasks.
extern __crit __size_t
__SCHED_CALL kcpu2_wakesleepers_unlocked(struct kcpu2 *__restrict self,
                                         struct timespec const *__restrict tmnow);

//////////////////////////////////////////////////////////////////////////
// Rotate running tasks in the given CPU.
// NOTE: The caller must either be holding a lock to 'KCPU2_LOCK_TASKS',
//       or not be the given CPU and hold the 'KCPU2_FLAG_INUSE' flag.
extern __crit void __SCHED_CALL kcpu2_rotaterunning_unlocked(struct kcpu2 *__restrict self);



//////////////////////////////////////////////////////////////////////////
// Internal APIC functions.
extern __crit void kcpu2_apic_initialize(void);
extern __crit kerrno_t kcpu2_apic_int(struct kcpu2 *__restrict self, __u8 intno);
extern __crit kerrno_t kcpu2_apic_start(struct kcpu2 *__restrict self);
extern __crit __nomp kerrno_t kcpu2_apic_start_unlocked(struct kcpu2 *__restrict self, __u32 start_eip);
extern __noreturn void kcpu2_apic_offline(void);
#define KCPU2_APIC_START_EIPBITS  20
#define KCPU2_APIC_IPI_ATTEMPTS             2    /*< Amount of times init should be attempted through IPI. */
#define KCPU2_APIC_IPI_TIMEOUT              10   /*< Timeout when waiting for 'APIC_ICR_BUSY' to go away after IPI init. */
#define KCPU2_APIC_ACKNOWLEDGE_SEND_TIMEOUT 1000 /*< Timeout in peer CPU during init to wait for startup intention acknowledgement. */
#define KCPU2_APIC_ACKNOWLEDGE_RECV_TIMEOUT 100  /*< Timeout in host CPU during init to wait for startup confirmation. */

#endif /* !__ASSEMBLY__ */
__DECL_END

#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_TASK2_CPU_H__ */

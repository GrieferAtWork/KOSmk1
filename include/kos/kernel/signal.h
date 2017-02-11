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
#ifndef __KOS_KERNEL_SIGNAL_H__
#define __KOS_KERNEL_SIGNAL_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <__null.h>
#include <assert.h>
#include <kos/compiler.h>
#include <kos/errno.h>
#include <kos/atomic.h>
#include <kos/kernel/types.h>
#include <kos/kernel/object.h>
#include <kos/kernel/interrupts.h>
#include <kos/kernel/sched_yield.h>

__DECL_BEGIN

//
// === The Signal (Invented by Griefer@Work; Yes I claim this one for myself) ===
//
// In KOS, a signal is _the_ lowest-level existing synchronization primitive,
// in theory making it the only ~true~ primitive, as everything else is
// merely a derivative of it, usually adding some functionality or
// re-interpreting its existing features.
//
// In layman's terms, or using existing terminology, a signal is somewhat
// of a hybrid between a monitor (at least I think it is. - I've never
// actually used a monitor even once in my lifetime thus far), and a
// condition variable (though still allowing you to go further and run
// any kind of code at that critical point at which a regular condition
// variable would unlock the mutex it's been given, essentially allowing
// you to use any kind of other locking mechanism as the lock of a
// condition variable)
//
// A most detailed explanation on the semantics of how signals operate
// is, though truly primitive, somewhat hard to explain, with there not
// really being something that is exactly what me and KOS calls a signal.
// But the first thing I should mention, is that a signal, as versatile
// and powerful as it is, isn't suitable for exposure to anyone.
// 
// While the APIs provided within this header and others try to prevent
// and/or detect invalid signal states that would result in the scheduler
// deadlocking, or even worse: breaking all-together, it is very much
// possible to cause a lot of damage by locking a signal's KSIGNAL_LOCK_WAIT
// lock at the right (or rather wrong) time.
//
// With that in mind, you must never expose the true, raw nature of a
// pure signal to a user (unless your interface doesn't allow users
// to run arbitrary, or dangerous code while holding a signal lock).
//
// === Enough Theory. Explain how they work! ===
//
// The way a signal functions in quite basic. - Instead of defining some
// value (such as used by a semaphore) or a binary state (like a mutex has),
// a signal exposes no such thing. While KOS signals still allow you to
// lock them, those locks are only required for an internal insurance
// that can be used to keep a signal from being send while a thread
// is being scheduled for receiving said signal.
// 
// === Receive, Send? I though these were about multitasking, not networking! ===
// 
// You are absolutely correct. I think it is time for me to pull out
// the most important bits of what a signal does before getting back
// to the semantics of signal locking later:
// 
// >> void signal_recv(struct signal *s);
// >> void signal_send(struct signal *s);
// 
// This is a simplified version of a signal's most basic functionality.
// The behavior is as follows.
// signal_recv:
//   The calling thread is suspended (unscheduled) until it is
//   re-awakened by another thread that is sending a signal.
// signal_send:
//   Send a signal to some unspecified set of receiving threads
//   (e.g.: all of them), waking them and causing them to resume
//   execution by returning from their call to 'signal_recv'.
// 
// === OH! I get it. But I'm spotting a gaping design flaw.
//     You get a race condition because you can't synchronize
//     when a signal is send vs. when it is received. ===
//
// You are right again. When I originally came up with signals, I too
// though this was an obstacle impossible to overcome. To be honest,
// I came up with the idea of a primitive as simple as a signal back
// around 2010, but scrapped it because of this exact problem.
// The solution has to do with the aforementioned locking of signal,
// and is as follow (written in code to better illustrate the flow):
//
// >> void signal_recv(struct signal *s) {
// >>   // Lock the signal, implemented as a spin-lock, using atomics
// >>   LOCK_SIGNAL(s);
// >>   
// >>   // Do some synchronization stuff, like higher-level primitive
// >>   // checks. e.g.: If 's' is part of a mutex, can that mutex
// >>   // be locked, or must we wait for some other thread to
// >>   // unlock it first.
// >>   
// >>   // Unschedule the current thread and unlock the signal
// >>   // NOTE: The magical part of this is that the signal will only be
// >>   //       unlocked _AFTER_ the calling thread has already reached the
// >>   //       point at which is is also ready to receive the same signal.
// >>   MAGICALLY_UNSCHEDULE_CURRENT_THREAD_AND_UNLOCK_SIGNAL(s);
// >>   // Once we're here, the signal was send, at which point we can
// >>   // run more higher-level primitive-specific code.
// >>   // e.g.: If 's' is part of a mutex, start over and
// >>   //       check if we are able to lock it now.
// >> }
//
// >> void signal_send(struct signal *s) {
// >>   // Lock the signal again, this preventing any new
// >>   // threads from being added to the receiver queue
// >>   LOCK_SIGNAL(s);
// >>
// >>   // At this point do some more higher-level primitive task.
// >>   // e.g.: A mutex would mark itself as unlocked, allowing
// >>   //       other threads to acquire things like unique locks.
// >>   
// >>   // Reschedule a set of waiting tasks and unlock the signal.
// >>   RESCHEDULE_WAITING_TASKS_AND_UNLOCK_SIGNAL(s);
// >> }
// 
// === OK! That makes sense. You're system appears to work.
//     But what about fairness? From what I understand, the
//     way your mutex works still relies on try-and-failure. ===
// 
// First and foremost: I'd like to mention that when it comes to
// multi-threading I've got quite some experience under my belt.
// And let me promise you: fairness is completely over-rated and
// should be the least of your concerns when trying to implement
// an air-tight, multi-threaded system...
// 
// === So your saying that your system isn't fair? ===
// 
// Don't interrupt me... No that's not what I'm saying at all, but
// it's like almost midnight and I have no idea why I'm even doing
// this whole bi-polar dialog with myself within the documentation
// of this thing I came up with...
// 
// The system remains fair because you can control how many threads
// a signal should be send to when actually sending it.
// In the case of a mutex, you can easily achieve fairness by only
// sending (and therefor waking) up to one thread, where signals are
// intentionally designed to track the order in which threads originally
// started receiving them, thus allowing you to simply state:
// >> void mutex_unlock(struct mutex *m) {
// >>   LOCK_SIGNAL(&m->signal);
// >>   MARK_MUTEX_AS_NOT_ACQUIRED(m);
// >>   RESCHEDULE_THREAD_NEXT_IN_LINE_BASED_ON_ORDER_OF_UNSCHEDULING_AND_UNLOCK_SIGNAL(&m->signal);
// >> }
//
// === Awesome sauce! - And don't worry. You're not bi-polar.
//     I know I'm just a figment of your imagination, meant to
//     keep our deer reader interested while hopefully getting
//     them to learn something along the way ===
// 
// See you 'round!
//


//
// === Advanced Signal functionality ===
//
// Assuming you've already read and understood my 'The Signal'
// article above, the following will go further in-depth on
// advanced signal functionality. Most of this is already partially
// explained within the documentations of individual API function,
// though this article will try to explain 'Why Choose Signal?'
// 
// A signal, though fully packed with functionality that can
// be implemented without the need of extending its raw data
// structure, only requires two pointers and one additional bit.
// >> On 32-bit platforms this evaluates to <9 bytes>
// >> On 64-bit platforms this evaluates to <17 bytes>
//
// I should mention that of the two pointers only one is really
// required, with the second only being there to allow what has
// already been described as the ~receive~ operation to be 'O(1)'.
// >> Without the second pointer, it would
//    be 'O(number-of-already-receiving-threads)'.
// Due to alignment, and the ability of closing a signal added by
// KOS, the size of a 32-bit signal grows to 12 bytes in practice.
// 
// === Closing a signal? ===
//
// To allow for easy and intrinsic implementation of closeable
// resources, the best way of doing this is exactly what KOS does:
// By defining a way of closing its most basic of synchronization
// primitives, a standardized way of closing associated resources
// is defined as well.
// When a signal is closed (through a call to 'ksignal_close'), it
// can no longer be operated upon, meaning that any further attempts
// at receiving it will fail immediately, in KOS yielding a
// KE_DESTROYED error. Sending must not be protected by this,
// as a send operation by nature is already a no-op if on one's
// there to receive it.
//
// === I get it. Come to think of it, closeable resources and
//     synchronization are almost never seen apart. - Anything
//     that is closeable also has some kind of lock to it ===
// 
// We'll since you're me and I am you, I feel the urge to mention
// that I don't want to be corrected on this. But as far as I can
// think, there really isn't such a thing. - At least within a
// thread-safe application: Files, graphics, user-level I/O, ...
// Everything that is closeable has some kind of lock to it.
// And by simply defining that lock as closeable, you can ~really~
// easily prevent any further access to that resource by closing
// the lock, essentially rendering the resource inert.
//
// But lets put that aside, because I haven't yet talked about the one
// shortcoming of a signal: It is nothing but a meaningless Impulse.
// And while getting an Impulse usually is all you need, what
// if you wanted to include some special information within, to be
// told to who-ever is getting hit by it.
// 
// [continuation in <kos/kernel/signal_v.h>]
//


#define KOBJECT_MAGIC_SIGNAL 0x518AA7 // SIGNAL

#ifndef __ASSEMBLY__
struct ktask;
struct kcpu;
struct timespec;
struct ksignal;

#define kassert_ksignal(self) kassert_object(self,KOBJECT_MAGIC_SIGNAL)
#ifdef __INTELLISENSE__
extern void kassert_ksignals(__size_t sigc, struct ksignal const *const *sigv);
#elif defined(__DEBUG__)
#define kassert_ksignals(sigc,sigv) \
 __xblock({ struct ksignal const *const *__ksass_iter,*const *__ksass_end;\
            __ksass_end = (__ksass_iter = (struct ksignal const *const *)(sigv))+(sigc);\
            for (; __ksass_iter != __ksass_end; ++__ksass_iter) kassert_ksignal(*__ksass_iter);\
            (void)0;\
 })
#else
#define kassert_ksignals(sigc,sigv) (void)0
#endif
#endif /* !__ASSEMBLY__ */

//////////////////////////////////////////////////////////////////////////
// The most primitive possible kind of locking mechanism.
// >> This is what all other synchronization primitives use
//    for implementing fair & non-idling scheduling & unscheduling.
//////////////////////////////////////////////////////////////////////////
// NOTES:
//  - A signal is really just that: a signal that can be send
//    and received safely and effortlessly between multiple tasks.
//    (With the extensions of waiting with a given timeout)
//  - A signal on its own doesn't have any kind of additional
//    information, neither when send, nor when received.
//  - Signals are fair, meaning that the task that started receiving
//    it first will also be the one to be re-awakened first.
//    - An exception to this rule is a call to 'ksignal_sendall()',
//      which will signal all receiving tasks at once, and in an
//      undefined order.
//  - It is not possible to store additional data
//    for each receiving task within the signal.
//  - A signal can be closed by use of 'ksignal_close()'
//    - Once closed, any attempts to receive signals from a
//      closed signal will fail immediately, thus making the
//      implementation of a send-once signal as simple as:
//    >> static struct ksignal sig = KSIGNAL_INIT;
//    >> void task1(void) {
//    >>   if (ksignal_recv(&sig) == KE_DESTROYED)
//    >>     printf("Signal already dead");
//    >>   printf("Signal was send");
//    >> }
//    >> void task2(void) {
//    >>   if (ksignal_close(&sig) == KS_UNCHANGED)
//    >>     printf("Signal was already end");
//    >> }
//  - Signals are safe to use in conjunction with terminating tasks:
//    When a task that is currently receiving a signal is being
//    terminated, the signal will be informed, and the task is
//    removed from the signal's list of waiting tasks, the same
//    way as it also would if the receive had timed out.
//////////////////////////////////////////////////////////////////////////

#ifndef __ASSEMBLY__
struct ksignal {
 KOBJECT_HEAD
#define KSIGNAL_LOCK_WAIT    0x01
#define KSIGNAL_LOCK_USER_COUNT  7
#define KSIGNAL_LOCK_USER(n) (0x2 << (n)) /*< Returns up to 7 different signal locks (0..6) */
 __atomic __u8   s_locks; /*< Internal set available spinlocks. */
#define KSIGNAL_FLAG_NONE    0x00
#define KSIGNAL_FLAG_DEAD    0x01 /*< [lock(KSIGNAL_LOCK_WAIT)] Set to mark dead signals. */
#define KSIGNAL_FLAG_USER_COUNT  7
#define KSIGNAL_FLAG_USER(i) (0x2 << (i)) /*< Returns up to 7 different signal flags (0..6) */
          __u8   s_flags; /*< Signal flags. */
union{ /* 16 bits of Userdata (Used by extended implementations; set to 0 when not needed). */
#define KSIGNAL_USERBITS 16
          __u16  s_useru;
          __s16  s_users;
          __u8   s_useru8[2];
          __s8   s_users8[2];
};
 struct ktasksigslot *s_wakefirst; /*< [0..1][lock(KSIGNAL_LOCK_WAIT)] First task to wake on signal reception. */
 struct ktasksigslot *s_wakelast;  /*< [0..1][lock(KSIGNAL_LOCK_WAIT)] Last task to wake on signal reception. */
};
#define KSIGNAL_INIT_EXU(flags,user) \
 {KOBJECT_INIT(KOBJECT_MAGIC_SIGNAL) 0,flags,{user},NULL,NULL}
#define KSIGNAL_INIT_EX(flags)       KSIGNAL_INIT_EXU(flags,0)
#define KSIGNAL_INIT_U(user)         KSIGNAL_INIT_EXU(KSIGNAL_FLAG_NONE,user)
#define KSIGNAL_INIT                 KSIGNAL_INIT_EX(KSIGNAL_FLAG_NONE)

#ifdef KOBJECT_INIT_IS_MEMSET_ZERO
#define KSIGNAL_INIT_IS_MEMSET_ZERO 1
#endif

//////////////////////////////////////////////////////////////////////////
// Signal locking:
// >> kerrno_t ksignal_recv(struct ksignal *self) {
// >>   kerrno_t error;
// >>   ksignal_lock(self,KSIGNAL_LOCK_WAIT);
// >>   error = _ksignal_recv_andunlock_c(self);
// >>   ksignal_endlock();
// >>   return error;
// >> }


extern __crit __wunused __nonnull((2)) int ksignal_trylocks_c(__size_t sigc, struct ksignal *const *sigv, __u8 lock);
#ifdef __INTELLISENSE__
extern __crit __wunused __nonnull((1)) bool ksignal_trylock_c(struct ksignal *self, __u8 lock);
extern        __wunused __nonnull((1)) bool ksignal_isdead(struct ksignal const *self);
extern __crit __wunused __nonnull((1)) bool ksignal_isdead_c(struct ksignal const *self);
extern        __wunused __nonnull((1)) bool ksignal_isdead_unlocked(struct ksignal const *self);
extern        __wunused __nonnull((1)) bool ksignal_isdeads(__size_t sigc, struct ksignal const *const *sigv);
extern __crit __wunused __nonnull((1)) bool ksignal_isdeads_c(__size_t sigc, struct ksignal const *const *sigv);
extern        __wunused __nonnull((1)) bool ksignal_isdeads_unlocked(__size_t sigc, struct ksignal const *const *sigv);
extern        __wunused __nonnull((1)) bool ksignal_hasrecv(struct ksignal const *self);
extern __crit __wunused __nonnull((1)) bool ksignal_hasrecv_c(struct ksignal const *self);
extern        __wunused __nonnull((1)) bool ksignal_hasrecv_unlocked(struct ksignal const *self);
extern        __wunused __nonnull((1)) bool ksignal_hasrecvs(__size_t sigc, struct ksignal const *const *sigv);
extern __crit __wunused __nonnull((1)) bool ksignal_hasrecvs_c(__size_t sigc, struct ksignal const *const *sigv);
extern        __wunused __nonnull((1)) bool ksignal_hasrecvs_unlocked(__size_t sigc, struct ksignal const *const *sigv);
extern        __wunused __nonnull((1)) __size_t ksignal_cntrecv(struct ksignal const *self);
extern __crit __wunused __nonnull((1)) __size_t ksignal_cntrecv_c(struct ksignal const *self);
extern        __wunused __nonnull((1)) __size_t ksignal_cntrecv_unlocked(struct ksignal const *self);
extern        __wunused __nonnull((1)) __size_t ksignal_cntrecvs(__size_t sigc, struct ksignal const *const *sigv);
extern __crit __wunused __nonnull((1)) __size_t ksignal_cntrecvs_c(__size_t sigc, struct ksignal const *const *sigv);
extern        __wunused __nonnull((1)) __size_t ksignal_cntrecvs_unlocked(__size_t sigc, struct ksignal const *const *sigv);
extern        __wunused __nonnull((1)) bool ksignal_islocked(struct ksignal const *self, __u8 lock);
extern        __wunused __nonnull((2)) bool ksignal_islockeds(__size_t sigc, struct ksignal const *const *sigv, __u8 lock);

//////////////////////////////////////////////////////////////////////////
// Lock a given signal while simultaneously ensuring a critical block in the calling task.
#define ksignal_lock                         do{ksignal_lock_c
#define ksignal_unlock(self,lock)            do{ksignal_unlock_c(self,lock);}while(0);}while(0)
#define ksignal_breaklock()                  break
#define ksignal_breakunlock                  ksignal_unlock_c
#define ksignal_endlock()                    do{}while(0);}while(0)
#define ksignal_locks                        do{ksignal_locks_c
#define ksignal_unlocks(sigc,sigv,lock)      do{ksignal_unlocks_c(sigc,sigv,lock);}while(0);}while(0)
#define ksignal_breaklocks()                 break
#define ksignal_breakunlocks                 ksignal_unlocks_c
#define ksignal_endlocks()                   do{}while(0);}while(0)
extern __crit __nonnull((1)) void ksignal_lock_c(struct ksignal *self, __u8 lock);
extern __crit __nonnull((1)) void ksignal_unlock_c(struct ksignal *self, __u8 lock);
extern __crit __nonnull((2)) void ksignal_locks_c(__size_t sigc, struct ksignal *const *sigv, __u8 lock);
extern __crit __nonnull((2)) void ksignal_unlocks_c(__size_t sigc, struct ksignal *const *sigv, __u8 lock);

//////////////////////////////////////////////////////////////////////////
// Initialize a given signal
// >> An extended version can be called to specify additional
//    flags to be set before the signal is allowed to be send/received.
extern void ksignal_init(struct ksignal *self);
extern void ksignal_init_u(struct ksignal *self, __u16 userval);
extern void ksignal_init_ex(struct ksignal *self, __u8 flags);

//////////////////////////////////////////////////////////////////////////
// Close a given signal, broadcasting to all tasks still receiving.
// >> An extended version can be called to specify additional
//    flags to be removed in the return value of KE_OK.
// >> All funcitons allow additional code to be specified, that is
//    executed before the signal is unlocked in the case of KE_OK.
// @return: KE_OK:        At least one task has been re-scheduled.
// @return: KS_UNCHANGED: The signal was already closed
// @return: KS_EMPTY:     No tasks were waiting (and therefor signaled)
#define /*__crit*/ ksignal_close(self,callback,...)                __xblock({ struct ksignal *__sself = (self); {__VA_ARGS__;} __xreturn KE_OK; })
#define /*__crit*/ ksignal_closeex(self,addflags,...)              __xblock({ struct ksignal *__sself = (self); __u8 __sflags = addflags; {__VA_ARGS__;} __xreturn KE_OK; })
#define /*__crit*/ _ksignal_close_andunlock_c(self,...)            __xblock({ struct ksignal *__sself = (self); {__VA_ARGS__;} __xreturn KE_OK; })
#define /*__crit*/ _ksignal_closeex_andunlock_c(self,addflags,...) __xblock({ struct ksignal *__sself = (self); __u8 __sflags = addflags; {__VA_ARGS__;} __xreturn KE_OK; })

//////////////////////////////////////////////////////////////////////////
// Reset a given signal after it has been closed
// >> An extended version can be called to specify additional
//    flags to be added in the return value of KE_OK.
// >> All funcitons allow additional code to be specified, that is
//    executed before the signal is unlocked in the case of KE_OK.
// @return: KE_OK:        The signal has been reset and tasks
//                        can receive or send it once again.
// @return: KS_UNCHANGED: The signal was not closed.
#define /*__crit*/ ksignal_reset(self,...)                         __xblock({ struct ksignal *__sself = (self); {__VA_ARGS__;} __xreturn KE_OK; })
#define /*__crit*/ ksignal_resetex(self,delflags,...)              __xblock({ struct ksignal *__sself = (self); __u8 __sflags = delflags; {__VA_ARGS__;} __xreturn KE_OK; })
#define /*__crit*/ _ksignal_reset_andunlock_c(self,...)            __xblock({ struct ksignal *__sself = (self); {__VA_ARGS__;} __xreturn KE_OK; })
#define /*__crit*/ _ksignal_resetex_andunlock_c(self,delflags,...) __xblock({ struct ksignal *__sself = (self); __u8 __sflags = delflags; {__VA_ARGS__;} __xreturn KE_OK; })

#else
#define /*__crit*/ ksignal_lock_c(self,lock) \
 __xblock({ struct ksignal *const __kslself = (self);\
            __u8 const __ksllock = (lock);\
            KTASK_SPIN(ksignal_trylock_c(__kslself,__ksllock));\
            (void)0;\
 })
#define /*__crit*/ ksignal_trylock_c(self,lock) \
 (assert(ktask_iscrit()),!(katomic_fetchor((self)->s_locks,lock)&(lock)))
#define /*__crit*/ ksignal_unlock_c(self,lock)\
 (assert(ktask_iscrit()),_assertef((katomic_fetchand((self)->s_locks,~(lock))&(lock))!=0,"Lock not held"))
#define ksignal_lock(self,lock)        NOINTERRUPT_BEGINLOCK(ksignal_trylock_c(self,lock))
#define ksignal_unlock(self,lock)      NOINTERRUPT_ENDUNLOCK(ksignal_unlock_c(self,lock))
#define ksignal_breaklock()            NOINTERRUPT_BREAK
#define ksignal_breakunlock(self,lock) NOINTERRUPT_BREAKUNLOCK(ksignal_unlock_c(self,lock))
#define ksignal_endlock()              NOINTERRUPT_END

#define ksignal_breaklocks()                 NOINTERRUPT_BREAK
#define ksignal_breakunlocks(sigc,sigv,lock) NOINTERRUPT_BREAKUNLOCK(ksignal_unlocks_c(sigc,sigv,lock))
#define ksignal_endlocks()                   NOINTERRUPT_END
#define /*__crit*/ ksignal_locks_c(sigc,sigv,lock) \
 __xblock({ struct ksignal *const *const __kslssigv = (sigv);\
            __size_t const __kslssigc = (sigc); __u8 const __kslslock = (lock);\
            KTASK_SPIN(ksignal_trylocks_c(__kslssigc,__kslssigv,__kslslock));\
            (void)0;\
 })
#define /*__crit*/ ksignal_unlocks_c(sigc,sigv,lock) \
 __xblock({ struct ksignal *const *__ksul_iter,*const *__ksul_end;\
            __u8 const __ksul_lock = (lock);\
            __ksul_end = (__ksul_iter = (struct ksignal *const *)(sigv))+(sigc);\
            while (__ksul_end != __ksul_iter) { --__ksul_end;\
             ksignal_unlock_c(*__ksul_end,__ksul_lock);\
            }\
            (void)0;\
 })
#define ksignal_locks(sigc,sigv,lock)   NOINTERRUPT_BEGINLOCK(ksignal_trylocks_c(sigc,sigv,lock))
#define ksignal_unlocks(sigc,sigv,lock) NOINTERRUPT_ENDUNLOCK(ksignal_unlocks_c(sigc,sigv,lock))

#define __ksignal_getprop(T,self,lock,unlocked_getter) \
 __xblock({ struct ksignal const *const __ksgpself = (self); T __ksgpres;\
            ksignal_lock((struct ksignal *)__ksgpself,KSIGNAL_LOCK_WAIT);\
            __ksgpres = unlocked_getter(__ksgpself);\
            ksignal_unlock((struct ksignal *)__ksgpself,KSIGNAL_LOCK_WAIT);\
            __xreturn __ksgpres;\
 })
#define __ksignal_getprop_(T,self,lock,unlocked_getter,...) \
 __xblock({ struct ksignal const *const __ksgpself = (self); T __ksgpres;\
            ksignal_lock((struct ksignal *)__ksgpself,KSIGNAL_LOCK_WAIT);\
            __ksgpres = unlocked_getter(__ksgpself,__VA_ARGS__);\
            ksignal_unlock((struct ksignal *)__ksgpself,KSIGNAL_LOCK_WAIT);\
            __xreturn __ksgpres;\
 })
#define /*__crit*/ __ksignal_getprop_ni(T,self,lock,unlocked_getter) \
 __xblock({ struct ksignal const *const __ksgpself = (self); T __ksgpres;\
            ksignal_lock_c((struct ksignal *)__ksgpself,KSIGNAL_LOCK_WAIT);\
            __ksgpres = unlocked_getter(__ksgpself);\
            ksignal_unlock_c((struct ksignal *)__ksgpself,KSIGNAL_LOCK_WAIT);\
            __xreturn __ksgpres;\
 })
#define /*__crit*/ __ksignal_getprop_ni_(T,self,lock,unlocked_getter,...) \
 __xblock({ struct ksignal const *const __ksgpself = (self); T __ksgpres;\
            ksignal_lock_c((struct ksignal *)__ksgpself,KSIGNAL_LOCK_WAIT);\
            __ksgpres = unlocked_getter(__ksgpself,__VA_ARGS__);\
            ksignal_unlock_c((struct ksignal *)__ksgpself,KSIGNAL_LOCK_WAIT);\
            __xreturn __ksgpres;\
 })
#define __ksignal_getprops_boolany(sigc,sigv,single_getter) \
 __xblock({ struct ksignal *const *__ksgpsyiter,*const *__ksgpsyend; int __ksgpsyres = 0;\
            __ksgpsyend = (__ksgpsyiter = (struct ksignal *const *)(sigv))+(sigc);\
            for (;__ksgpsyiter != __ksgpsyend; ++__ksgpsyiter) {\
             if (single_getter(*__ksgpsyiter)) { __ksgpsyres = 1; break; }\
            }\
            __xreturn __ksgpsyres;\
 })
#define __ksignal_getprops_sizesum(sigc,sigv,single_getter) \
 __xblock({ struct ksignal *const *__ksgpsyiter,*const *__ksgpsyend; __size_t __ksgpsyres = 0;\
            __ksgpsyend = (__ksgpsyiter = (struct ksignal *const *)(sigv))+(sigc);\
            for (;__ksgpsyiter != __ksgpsyend; ++__ksgpsyiter)\
             __ksgpsyres += single_getter(*__ksgpsyiter);\
            __xreturn __ksgpsyres;\
 })
#define __ksignal_getprops_boolany_(sigc,sigv,single_getter,...) \
 __xblock({ struct ksignal *const *__ksgpsyiter,*const *__ksgpsyend; int __ksgpsyres = 0;\
            __ksgpsyend = (__ksgpsyiter = (struct ksignal *const *)(sigv))+(sigc);\
            for (;__ksgpsyiter != __ksgpsyend; ++__ksgpsyiter) {\
             if (single_getter(*__ksgpsyiter,__VA_ARGS__)) { __ksgpsyres = 1; break; }\
            }\
            __xreturn __ksgpsyres;\
 })
#define __ksignal_getprops_boolall(sigc,sigv,single_getter) \
 __xblock({ struct ksignal *const *__ksgpsaiter,*const *__ksgpsaend; int __ksgpsares = 1;\
            __ksgpsaend = (__ksgpsaiter = (struct ksignal *const *)(sigv))+(sigc);\
            for (;__ksgpsaiter != __ksgpsaend; ++__ksgpsaiter) {\
             if (!single_getter(*__ksgpsaiter)) { __ksgpsares = 0; break; }\
            }\
            __xreturn __ksgpsares;\
 })
#define __ksignal_getprops_boolall_(sigc,sigv,single_getter,...) \
 __xblock({ struct ksignal *const *__ksgpsaiter,*const *__ksgpsaend; int __ksgpsares = 1;\
            __ksgpsaend = (__ksgpsaiter = (struct ksignal *const *)(sigv))+(sigc);\
            for (;__ksgpsaiter != __ksgpsaend; ++__ksgpsaiter) {\
             if (!single_getter(*__ksgpsaiter,__VA_ARGS__)) { __ksgpsares = 0; break; }\
            }\
            __xreturn __ksgpsares;\
 })
#define            ksignal_cntrecv_unlocked(self)  \
 __xblock({ __size_t __kscrures = 0;\
            struct ktasksigslot *__kscruiter = (self)->s_wakefirst;\
            for (; __kscruiter; __kscruiter = __kscruiter->tss_next) ++__kscrures;\
            __xreturn __kscrures;\
 })

#define            ksignal_isdead(self)                 __ksignal_getprop(int,self,KSIGNAL_LOCK_WAIT,ksignal_isdead_unlocked)
#define /*__crit*/ ksignal_isdead_c(self)               __ksignal_getprop_ni(int,self,KSIGNAL_LOCK_WAIT,ksignal_isdead_unlocked)
#define            ksignal_isdead_unlocked(self)     (((self)->s_flags&KSIGNAL_FLAG_DEAD)!=0)
#define            ksignal_hasrecv(self)                __ksignal_getprop(int,self,KSIGNAL_LOCK_WAIT,ksignal_hasrecv_unlocked)
#define /*__crit*/ ksignal_hasrecv_c(self)              __ksignal_getprop_ni(int,self,KSIGNAL_LOCK_WAIT,ksignal_hasrecv_unlocked)
#define            ksignal_hasrecv_unlocked(self)     ((self)->s_wakefirst!=NULL)
#define            ksignal_cntrecv(self)                __ksignal_getprop(__size_t,self,KSIGNAL_LOCK_WAIT,ksignal_cntrecv_unlocked)
#define /*__crit*/ ksignal_cntrecv_c(self)              __ksignal_getprop_ni(__size_t,self,KSIGNAL_LOCK_WAIT,ksignal_cntrecv_unlocked)
#define            ksignal_isdeads(sigc,sigv)           __ksignal_getprops_boolany(sigc,sigv,ksignal_isdead)
#define /*__crit*/ ksignal_isdeads_c(sigc,sigv)         __ksignal_getprops_boolany(sigc,sigv,ksignal_isdead_c)
#define            ksignal_isdeads_unlocked(sigc,sigv)  __ksignal_getprops_boolany(sigc,sigv,ksignal_isdead_unlocked)
#define            ksignal_hasrecvs(sigc,sigv)          __ksignal_getprops_boolany(sigc,sigv,ksignal_hasrecv)
#define /*__crit*/ ksignal_hasrecvs_c(sigc,sigv)        __ksignal_getprops_boolany(sigc,sigv,ksignal_hasrecv_c)
#define            ksignal_hasrecvs_unlocked(sigc,sigv) __ksignal_getprops_boolany(sigc,sigv,ksignal_hasrecv_unlocked)
#define            ksignal_cntrecvs(sigc,sigv)          __ksignal_getprops_sizesum(sigc,sigv,ksignal_cntrecv)
#define /*__crit*/ ksignal_cntrecvs_c(sigc,sigv)        __ksignal_getprops_sizesum(sigc,sigv,ksignal_cntrecv_c)
#define            ksignal_cntrecvs_unlocked(sigc,sigv) __ksignal_getprops_sizesum(sigc,sigv,ksignal_cntrecv_unlocked)
#define            ksignal_islocked(self,lock)        ((katomic_load((self)->s_locks)&(lock))!=0)
#define            ksignal_islockeds(sigc,sigv,lock)    __ksignal_getprops_boolall_(sigc,sigv,ksignal_islocked,lock)

#define ksignal_init(self) \
 kobject_initzero(self,KOBJECT_MAGIC_SIGNAL)
#define ksignal_init_u(self,userval) \
 __xblock({ struct ksignal *const __ksiself = (self); \
            kobject_initzero(__ksiself,KOBJECT_MAGIC_SIGNAL);\
            __ksiself->s_useru = (userval);\
            (void)0;\
 })
#define ksignal_init_ex(self,flags) \
 __xblock({ struct ksignal *const __ksiself = (self); \
            kobject_initzero(__ksiself,KOBJECT_MAGIC_SIGNAL);\
            __ksiself->s_flags = (flags);\
            (void)0;\
 })
#define /*__crit*/ __KSIGNAL_CLOSEEX_ANDUNLOCK_IMPL(self,addflags,error,...) \
            if ((self->s_flags&KSIGNAL_FLAG_DEAD)!=0) {\
             error = KS_UNCHANGED;\
             ksignal_unlock_c(self,KSIGNAL_LOCK_WAIT);\
            } else {\
             self->s_flags |= (KSIGNAL_FLAG_DEAD|(addflags));\
             {__VA_ARGS__;}\
             error = _ksignal_sendall_andunlock_c(self) ? KE_OK : KS_EMPTY;\
            }
#define /*__crit*/ __KSIGNAL_RESETEX_IMPL_UNLOCKED(self,delflags,error,...) \
            if (self->s_flags&KSIGNAL_FLAG_DEAD) {\
             self->s_flags &= ~(KSIGNAL_FLAG_DEAD|(delflags));\
             {__VA_ARGS__;}\
             error = KE_OK;\
            } else {\
             error = KS_UNCHANGED;\
            }

#define /*__crit*/ ksignal_closeex(self,addflags,...) \
 __xblock({ kerrno_t __errid;\
            struct ksignal *const __sigself = (self);\
            kassert_ksignal(__sigself);\
            ksignal_lock_c(__sigself,KSIGNAL_LOCK_WAIT);\
            __KSIGNAL_CLOSEEX_ANDUNLOCK_IMPL(__sigself,addflags,__errid,__VA_ARGS__)\
            /*ksignal_endlock();*/\
            __xreturn __errid;\
 })
#define /*__crit*/ _ksignal_closeex_andunlock_c(self,addflags,...) \
 __xblock({ kerrno_t __errid;\
            struct ksignal *const __sigself = (self);\
            kassert_ksignal(__sigself);\
            __KSIGNAL_CLOSEEX_ANDUNLOCK_IMPL(__sigself,addflags,__errid,__VA_ARGS__)\
            __xreturn __errid;\
 })
#define /*__crit*/ ksignal_resetex(self,lock,unlock,delflags,...) \
 __xblock({ kerrno_t __errid;\
            struct ksignal *const __sigself = (self);\
            kassert_ksignal(__sigself);\
            ksignal_lock_c(__sigself,KSIGNAL_LOCK_WAIT);\
            __KSIGNAL_RESETEX_IMPL_UNLOCKED(__sigself,delflags,__errid,__VA_ARGS__)\
            ksignal_unlock_c(__sigself,KSIGNAL_LOCK_WAIT);\
            __xreturn __errid;\
 })
#define /*__crit*/ _ksignal_resetex_andunlock_c(self,unlock,delflags,...) \
 __xblock({ kerrno_t __errid;\
            struct ksignal *const __sigself = (self);\
            kassert_ksignal(__sigself);\
            __KSIGNAL_RESETEX_IMPL_UNLOCKED(__sigself,delflags,__errid,__VA_ARGS__)\
            ksignal_unlock_c(__sigself,KSIGNAL_LOCK_WAIT);\
            __xreturn __errid;\
 })
#define /*__crit*/  ksignal_close(self,...)              ksignal_closeex(self,0,__VA_ARGS__)
#define /*__crit*/ _ksignal_close_andunlock_c(self,...) _ksignal_closeex_andunlock_c(self,0,__VA_ARGS__)
#define /*__crit*/  ksignal_reset(self,...)              ksignal_resetex(self,0,__VA_ARGS__)
#define /*__crit*/ _ksignal_reset_andunlock_c(self,...) _ksignal_resetex_andunlock_c(self,0,__VA_ARGS__)
#endif

#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Waits for a given signal to be emitted.
// NOTE: In most situations, the use of a signal play out as follow:
//       >> // Acquire an exclusive lock to the signal's wait lock.
//       >> ksignal_lock(sig,KSIGNAL_LOCK_WAIT);
//       >> // Check if the signal in question has already been received.
//       >> if (get_intended_signal_already_received()) {
//       >>   // Signal data already send. - Don't wait for another signal.
//       >>   ksignal_unlock(sig,KSIGNAL_LOCK_WAIT);
//       >> } else {
//       >>   // Signal data not send yet. - Sleep until it is.
//       >>   _ksignal_recv_andunlock_c(sig);
//       >> }
//       >> // Signal data was sent. - Receive, and return it.
//       >> return get_received_signal();
//       The User is responsible for creating the data portion of the signal, or
//       use a higher-level synchronization primitive, such as a semaphore or mutex.
// NOTE: If you are wondering why there is no 'ksignal_trywait', think again,
//       because you obviously don't understand what signals are supposed to be.
//       >> They don't have a state. - You can't check if a signal was send.
//          The only thing you can do, is wait until the signal is send by
//          someone, or something, in which case you will be woken up.
//          (Or you can wait for that signal with an optional timeout)
// @return: KE_OK:         The signal was successfully received.
// @return: KE_DESTROYED:  The signal was/has-been killed.
// @return: KE_TIMEDOUT:   [ksignal_(timed|timeout)recv{s}] The given timeout has passed.
// @return: KE_INTR:       The calling task was interrupted.
extern           __nonnull((1))   kerrno_t ksignal_recv(struct ksignal *__restrict signal);
extern __wunused __nonnull((1,2)) kerrno_t ksignal_timedrecv(struct ksignal *__restrict signal, struct timespec const *__restrict abstime);
extern __wunused __nonnull((1,2)) kerrno_t ksignal_timeoutrecv(struct ksignal *__restrict signal, struct timespec const *__restrict timeout);
extern           __nonnull((2))   kerrno_t ksignal_recvs(__size_t sigc, struct ksignal *const *__restrict sigv);
extern __wunused __nonnull((2,3)) kerrno_t ksignal_timedrecvs(__size_t sigc, struct ksignal *const *__restrict sigv, struct timespec const *__restrict abstime);
extern __wunused __nonnull((2,3)) kerrno_t ksignal_timeoutrecvs(__size_t sigc, struct ksignal *const *__restrict sigv, struct timespec const *__restrict timeout);
#else
#define ksignal_recv(signal) \
 __xblock({ struct ksignal *const __ksrcvself = (signal);\
            kerrno_t __ksrcverr; kassert_ksignal(__ksrcvself);\
            ksignal_lock(__ksrcvself,KSIGNAL_LOCK_WAIT);\
            __ksrcverr = _ksignal_recv_andunlock_c(__ksrcvself);\
            ksignal_endlock();\
            __xreturn __ksrcverr;\
 })
#define ksignal_timedrecv(signal,abstime) \
 __xblock({ struct ksignal *const __kstrcvself = (signal);\
            kerrno_t __kstrcverr; kassert_ksignal(__kstrcvself);\
            ksignal_lock(__kstrcvself,KSIGNAL_LOCK_WAIT);\
            __kstrcverr = _ksignal_timedrecv_andunlock_c(__kstrcvself,abstime);\
            ksignal_endlock();\
            __xreturn __kstrcverr;\
 })
#define ksignal_timeoutrecv(signal,timeout) \
 __xblock({ struct ksignal *const __kstorcvself = (signal);\
            kerrno_t __kstorcverr; kassert_ksignal(__kstorcvself);\
            ksignal_lock(__kstorcvself,KSIGNAL_LOCK_WAIT);\
            __kstorcverr = _ksignal_timeoutrecv_andunlock_c(__kstorcvself,abstime);\
            ksignal_endlock();\
            __xreturn __kstorcverr;\
 })
#define ksignal_recvs(sigc,sigv) \
 __xblock({ kerrno_t __ksrcvserr; __size_t const __ksrcvssigc = (sigc);\
            struct ksignal *const *const __ksrcvssigv = (sigv);\
            kassert_ksignals(__ksrcvssigc,__ksrcvssigv);\
            ksignal_locks(__ksrcvssigc,__ksrcvssigv,KSIGNAL_LOCK_WAIT);\
            __ksrcvserr = _ksignal_recvs_andunlock_c(__ksrcvssigc,__ksrcvssigv);\
            ksignal_endlock();\
            __xreturn __ksrcvserr;\
 })
#define ksignal_timedrecvs(sigc,sigv,abstime) \
 __xblock({ kerrno_t __kstrcvserr; __size_t const __kstrcvssigc = (sigc);\
            struct ksignal *const *const __kstrcvssigv = (sigv);\
            kassert_ksignals(__kstrcvssigc,__kstrcvssigv);\
            ksignal_locks(__kstrcvssigc,__kstrcvssigv,KSIGNAL_LOCK_WAIT);\
            __kstrcvserr = _ksignal_timedrecvs_andunlock_c(__kstrcvssigc,__kstrcvssigv,abstime);\
            ksignal_endlock();\
            __xreturn __kstrcvserr;\
 })
#define ksignal_timeoutrecvs(sigc,sigv,timeout) \
 __xblock({ kerrno_t __kstorcvserr; __size_t const __kstorcvssigc = (sigc);\
            struct ksignal *const *const __kstorcvssigv = (sigv);\
            kassert_ksignals(__kstorcvssigc,__kstorcvssigv);\
            ksignal_locks(__kstorcvssigc,__kstorcvssigv,KSIGNAL_LOCK_WAIT);\
            __kstorcvserr = _ksignal_timeoutrecvs_andunlock_c(__kstorcvssigc,__kstorcvssigv,timeout);\
            ksignal_endlock();\
            __xreturn __kstorcvserr;\
 })
#endif
extern __crit           __nonnull((1))   kerrno_t _ksignal_recv_andunlock_c(struct ksignal *__restrict self);
extern __crit           __nonnull((2))   kerrno_t _ksignal_recvs_andunlock_c(__size_t sigc, struct ksignal *const *__restrict sigv);
extern __crit __wunused __nonnull((1,2)) kerrno_t _ksignal_timedrecv_andunlock_c(struct ksignal *self, struct timespec const *__restrict abstime);
extern __crit __wunused __nonnull((2,3)) kerrno_t _ksignal_timedrecvs_andunlock_c(__size_t sigc, struct ksignal *const *__restrict sigv, struct timespec const *__restrict abstime);
extern __crit __wunused __nonnull((1,2)) kerrno_t _ksignal_timeoutrecv_andunlock_c(struct ksignal *self, struct timespec const *timeout);
extern __crit __wunused __nonnull((2,3)) kerrno_t _ksignal_timeoutrecvs_andunlock_c(__size_t sigc, struct ksignal *const *__restrict sigv, struct timespec const *__restrict timeout);

//////////////////////////////////////////////////////////////////////////
// Returns the first task in line for being awoken.
// >> The caller should then call 'ktask_reschedule[_ex]' to re-schedule the task.
// @return: NULL: No task was waiting
#ifdef __INTELLISENSE__
extern __crit __wunused __nonnull((1)) __ref struct ktask *
ksignal_popone(struct ksignal *__restrict self);
#else
#define ksignal_popone(self) \
 __xblock({ struct ksignal *const __kspoself = (self); \
            kerrno_t __kspoerr; kassert_ksignal(__kspoself);\
            ksignal_lock(__kspoself,KSIGNAL_LOCK_WAIT);\
            __kspoerr = _ksignal_popone_andunlock_c(__kspoself);\
            ksignal_endlock();\
            __xreturn __kspoerr;\
 })
#endif
extern __crit __wunused __nonnull((1)) __ref struct ktask *
_ksignal_popone_andunlock_c(struct ksignal *self);

#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Send a given signal to one waiting task.
// @param: newcpu: If NULL, schedule any waiting task on the calling (aka. current) CPU
// @return: KE_DESTROYED: The task has been terminated
// @return: KS_EMPTY:     No task was waiting, or the signal is dead
// @return: KS_UNCHANGED: The task has already been re-scheduled
//                       (Due to race-conditions, this should not be considered an error)
extern        __nonnull((1)) kerrno_t ksignal_sendone(struct ksignal *__restrict self);
extern __crit __nonnull((1)) kerrno_t _ksignal_sendone_andunlock_c(struct ksignal *__restrict self);
extern __crit __nonnull((1)) kerrno_t ksignal_sendoneex(struct ksignal *__restrict self, struct kcpu *__restrict newcpu, int hint);
extern __crit __nonnull((1)) kerrno_t _ksignal_sendoneex_andunlock_c(struct ksignal *__restrict self, struct kcpu *__restrict newcpu, int hint);
#else
#define __KSIGNAL_SENDONEEX_ANDUNLOCK_C_IMPL(self,newcpu,hint,error) \
 { struct ktask *__soex_waketask; int const __soex_hint = (hint); \
  assertf((__soex_hint&0xFFF0)==0,"ksignal_sendoneex doesn't accept flag modifiers: %x",__soex_hint);\
  if ((__soex_waketask = _ksignal_popone_andunlock_c(self)) == NULL) error = KS_EMPTY;\
  else {\
   error = ktask_reschedule_ex(__soex_waketask,newcpu,__soex_hint|\
                               KTASK_RESCHEDULE_HINTFLAG_INHERITREF);\
  }\
 }\

#define ksignal_sendone(self) \
 ksignal_sendoneex(self,kcpu_leastload(),KTASK_RESCHEDULE_HINT_DEFAULT)
#define /*__crit*/ _ksignal_sendone_andunlock_c(self) \
 _ksignal_sendoneex_andunlock_c(self,kcpu_leastload(),KTASK_RESCHEDULE_HINT_DEFAULT)
#define ksignal_sendoneex(self,newcpu,hint) \
 __xblock({ struct ksignal *const __ksoeself = (self);\
            kerrno_t __ksoeerr; kassert_ksignal(__ksoeself);\
            ksignal_lock(__ksoeself,KSIGNAL_LOCK_WAIT);\
            __KSIGNAL_SENDONEEX_ANDUNLOCK_C_IMPL(__ksoeself,newcpu,hint,__ksoeerr);\
            ksignal_endlock();\
            __xreturn __ksoeerr;\
 })
#define _ksignal_sendoneex_andunlock_c(self,newcpu,hint) \
 __xblock({ struct ksignal *const __ksoecself = (self); kerrno_t __ksoecerr;\
            __KSIGNAL_SENDONEEX_ANDUNLOCK_C_IMPL(__ksoecself,newcpu,hint,__ksoecerr);\
            __xreturn __ksoecerr;\
 })
#endif

//////////////////////////////////////////////////////////////////////////
// Wake up to 'n_tasks' tasks.
//  - If non-NULL, '*woken_tasks' will be set to the amount of tasks woken.
// @return: * : Same as the first KE_ISERR of 'ksignal_sendone' 
extern __nonnull((1)) kerrno_t
ksignal_sendn(struct ksignal *__restrict self,
              __size_t n_tasks,
              __size_t *__restrict woken_tasks);

#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Wakes all waiting tasks.
// WARNING: Errorous wakeups are silently discarded,
//          but do not count to the total returned.
// @return: * : The amount of woken tasks.
extern __nonnull((1)) __size_t ksignal_sendall(struct ksignal *__restrict self);
#else
#define ksignal_sendall(self)\
 __xblock({ struct ksignal *const __kssaself = (self);\
            __size_t __kssares; kassert_ksignal(__kssaself);\
            ksignal_lock(__kssaself,KSIGNAL_LOCK_WAIT);\
            __kssares = _ksignal_sendall_andunlock_c(__kssaself);\
            ksignal_endlock();\
            __xreturn __kssares;\
 })
#endif
extern __crit __nonnull((1)) __size_t
_ksignal_sendall_andunlock_c(struct ksignal *__restrict self);
#endif /* !__ASSEMBLY__ */

#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Condition variables:
// >> Begin receiving a signal and run the given unlock code before
//    the calling task will be added to the queue of waiting tasks.
// WARNING: Any lock released will not be re-acquired after
//          the calling task is re-awakened (if ever).
// NOTE: The given code is executed in all situations (even when an error is returned)
// >> struct kmutex mutex = KMUTEX_INIT;
// >> struct ksignal condvar = KSIGNAL_INIT;
// >> 
// >> void waitfordata(void) {
// >>   for (;;) {
// >>     kmutex_lock(&mutex);
// >>     if (data_available()) {
// >>       parse_data();
// >>       kmutex_unlock(&mutex);
// >>     }
// >>     ksignal_recvc(&condvar,kmutex_unlock(&mutex));
// >>     // NOTE: 'ksignal_*recvc*' does not
// >>     //       re-acquire any kind of lock!
// >>   }
// >> }
// >>
// >> void postdata(int wakeall) {
// >>   kmutex_lock(&mutex);
// >>   do_postdata();
// >>   if (wakeall) {
// >>     ksignal_sendall(&condvar);
// >>   } else {
// >>     ksignal_sendone(&condvar);
// >>   }
// >>   kmutex_unlock(&mutex);
// >> }
// @return: KE_OK:         The signal was successfully received.
// @return: KE_DESTROYED:  The signal was/has-been killed.
// @return: KE_TIMEDOUT:   [ksignal_(timed|timeout)recv{s}] The given timeout has passed.
// @return: KE_INTR:       The calling task was interrupted.
#define ksignal_recvc(signal,...)                    __xblock({ struct ksignal *__ksrcvcself = (signal); (__VA_ARGS__); __xreturn KE_OK; })
#define ksignal_timedrecvc(signal,abstime,...)       __xblock({ struct ksignal *__kstrcvcself = (signal); struct timespec const *__kstrcvcabstime = (abstime); (__VA_ARGS__); __xreturn KE_OK; })
#define ksignal_timeoutrecvc(signal,timeout,...)     __xblock({ struct ksignal *__kstorcvcself = (signal); struct timespec const *__kstorcvctimeout = (timeout); (__VA_ARGS__); __xreturn KE_OK; })
#define ksignal_recvcs(sigc,sigv,...)                __xblock({ __size_t __ksrcvcssigc = (sigc); struct ksignal *const *__ksrcvcssigv = (sigv); (__VA_ARGS__); __xreturn KE_OK; })
#define ksignal_timedrecvcs(sigc,sigv,abstime,...)   __xblock({ __size_t __kstrcvcssigc = (sigc); struct ksignal *const *__kstrcvcssigv = (sigv); struct timespec const *__kstrcvcsabstime = (abstime); (__VA_ARGS__); __xreturn KE_OK; })
#define ksignal_timeoutrecvcs(sigc,sigv,timeout,...) __xblock({ __size_t __kstorcvcssigc = (sigc); struct ksignal *const *__kstorcvcssigv = (sigv); struct timespec const *__kstorcvcstimeout = (timeout); (__VA_ARGS__); __xreturn KE_OK; })
#else
#define ksignal_recvc(signal,...) \
 __xblock({ struct ksignal *const __ksrcvcself = (signal);\
            kassert_ksignal(__ksrcvcself); kerrno_t __ksrcvcerr;\
            ksignal_lock(__ksrcvcself,KSIGNAL_LOCK_WAIT);\
            (__VA_ARGS__);\
            __ksrcvcerr = _ksignal_recv_andunlock_c(__ksrcvcself);\
            ksignal_endlock();\
            __xreturn __ksrcvcerr;\
 })
#define ksignal_timedrecvc(signal,abstime,...) \
 __xblock({ struct ksignal *const __kstrcvcself = (signal);\
            kassert_ksignal(__kstrcvcself); kerrno_t __kstrcvcerr;\
            ksignal_lock(__kstrcvcself,KSIGNAL_LOCK_WAIT);\
            (__VA_ARGS__);\
            __kstrcvcerr = _ksignal_timedrecv_andunlock_c(__kstrcvcself,abstime);\
            ksignal_endlock();\
            __xreturn __kstrcvcerr;\
 })
#define ksignal_timeoutrecvc(signal,timeout,...) \
 __xblock({ struct ksignal *const __kstorcvcself = (signal);\
            kassert_ksignal(__kstorcvcself); kerrno_t __kstorcvcerr;\
            ksignal_lock(__kstorcvcself,KSIGNAL_LOCK_WAIT);\
            (__VA_ARGS__);\
            __kstorcvcerr = _ksignal_timeoutrecv_andunlock_c(__kstorcvcself,abstime);\
            ksignal_endlock();\
            __xreturn __kstorcvcerr;\
 })
#define ksignal_recvcs(sigc,sigv,...) \
 __xblock({ kerrno_t __ksrcvcserr; __size_t const __ksrcvcssigc = (sigc);\
            struct ksignal *const *const __ksrcvcssigv = (sigv);\
            kassert_ksignals(__ksrcvcssigc,__ksrcvcssigv);\
            ksignal_locks(__ksrcvcssigc,__ksrcvcssigv,KSIGNAL_LOCK_WAIT);\
            (__VA_ARGS__);\
            __ksrcvcserr = _ksignal_recvs_andunlock_c(__ksrcvcssigc,__ksrcvcssigv);\
            ksignal_endlock();\
            __xreturn __ksrcvcserr;\
 })
#define ksignal_timedrecvcs(sigc,sigv,abstime,...) \
 __xblock({ kerrno_t __kstrcvcserr; __size_t const __kstrcvcssigc = (sigc);\
            struct ksignal *const *const __kstrcvcssigv = (sigv);\
            kassert_ksignals(__kstrcvcssigc,__kstrcvcssigv);\
            ksignal_locks(__kstrcvcssigc,__kstrcvcssigv,KSIGNAL_LOCK_WAIT);\
            (__VA_ARGS__);\
            __kstrcvcserr = _ksignal_timedrecvs_andunlock_c(__kstrcvcssigc,__kstrcvcssigv,abstime);\
            ksignal_endlock();\
            __xreturn __kstrcvcserr;\
 })
#define ksignal_timeoutrecvcs(sigc,sigv,timeout,...) \
 __xblock({ kerrno_t __kstorcvcserr; __size_t const __kstorcvcssigc = (sigc);\
            struct ksignal *const *const __kstorcvcssigv = (sigv);\
            kassert_ksignals(__kstorcvcssigc,__kstorcvcssigv);\
            ksignal_locks(__kstorcvcssigc,__kstorcvcssigv,KSIGNAL_LOCK_WAIT);\
            (__VA_ARGS__);\
            __kstorcvcserr = _ksignal_timeoutrecvs_andunlock_c(__kstorcvcssigc,__kstorcvcssigv,timeout);\
            ksignal_endlock();\
            __xreturn __kstorcvcserr;\
 })
#endif

__DECL_END

#ifndef __KOS_KERNEL_TASK_H__
#include <kos/kernel/task.h>
#endif /* !__KOS_KERNEL_TASK_H__ */
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_SIGNAL_H__ */

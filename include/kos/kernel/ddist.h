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
#ifndef __KOS_KERNEL_DDIST_H__
#define __KOS_KERNEL_DDIST_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/compiler.h>
#include <kos/kernel/features.h>
#include <kos/kernel/signal.h>
#include <kos/kernel/object.h>
#if KCONFIG_HAVE_DEBUG_TRACKEDDDIST
#include <traceback.h>
#elif !defined(__INTELLISENSE__)
#include <kos/kernel/signal_v.h>
#endif

//
// === THIS IS NOT AN OFFICIAL SYNC-PRIMITIVE; I CAME UP WITH IT ===
// A _very_ special kind of synchronization primitive, a ddist
// (long: Data-Distributer) can be used for just that:
//  - Distributing data from one, or more (as they are often called) Baker Thread.
//  - Consuming data within one, or more (as they are also often called) Consumer Thread.
//
// This is the high-level variant of what the KOS kernel uses for distributing
// I/O data, such as key-strokes, or incoming network packets among all those
// in need of that information.
// NOTE: The keyboard actually uses kaddist, as found in <kos/kernel/addist.h>
//
// The 'kddist' sync-primitive is fairly versatile, allowing the baker
// thread to specify how many consumers are allowed to even receive data.
//
// You could argue, that everything that can be done with a datdist can already
// be done using a simple signal, alongside the vsend/vrecv functions, and in
// a basic sense, you would be right.
// The datdist is actually using exactly those functions to implement its
// lower-level functionality, but on top of that provides an intrinsic
// locking mechanism used to make sure that when data was ~baked~ and is
// ready to be v-send out, all threads registered to process said data have
// went back to waiting for data, thus providing loss-less, and direct
// transmissions between a ~baker~ and all of its consumers.
//
// It is noteworthy, that despite all of its features,
// a kddist still allows for fully unbuffered I/O, but doesn't
// fall victim to short-comings, arising within code executed
// within special contexts, such as from an interrupt handler.
//
//EXAMPLE (Misses error handling):
// >> static struct kddist dd = KDDIST_INIT;
// >>
// >> void *baker_thread(void *arg) {
// >>   for (;;) {
// >>     int data = rand();
// >>     // Post data to all Consumers
// >>     kddist_vsend(&dd,&data,sizeof(data));
// >>   }
// >> }
// >>
// >> void *consumer_thread(void *arg) {
// >>   // Register the calling task as a consumer,
// >>   // ensuring that beyond that point, this thread
// >>   // but be present before data can be send to it.
// >>   kddist_adduser(&cokkies);
// >>   while (running) {
// >>     int data;
// >>     kddist_vrecv(&dd,&data);
// >>     printf("Data received: %d\n",data);
// >>   }
// >>   kddist_deluser(&cokkies);
// >> }
//

__DECL_BEGIN

#define KOBJECT_MAGIC_DDIST  0xDD157 /*< DDIST. */
#define kassert_kddist(self) kassert_object(self,KOBJECT_MAGIC_DDIST)

struct timespec;

#if KCONFIG_HAVE_DEBUG_TRACKEDDDIST
struct tbtrace;
struct kddist_debug_slot {
 struct ktask      *ds_task; /*< [1..1] Task registered to this slot.
                                  NOTE: Registered pointer may not necessarily point to valid tasks.
                                  e.g.: When a task terminates without unregistering itself, it remains dangling. */
 struct tbtrace *ds_tb;   /*< [1..1] Traceback used for  */
};
struct kddist_debug {
 __un(KCONFIG_DDIST_USERBITS)     ddd_regc;  /*< Amount of registered users. */
 struct kddist_debug_slot *ddd_regv;  /*< [0..ddd_regc][owned] Vector registered user tasks. */
#define KDDIST_DEBUG_FLAG_NOMEM   0x1 /*< 'ddd_regv' may not be up-to-date due to a prior allocation failure. */
#define KDDIST_DEBUG_FLAG_REGONLY 0x2 /*< [const] The ddist is configured not to allow unregistered tasks receiving data. */
 int                       ddd_flags; /*< Flags describing the debug-state of the ddist. */
};
#define __KDDIST_DEBUG_INIT_REGONLY ,{0,NULL,KDDIST_DEBUG_FLAG_REGONLY}
#define __KDDIST_DEBUG_INIT         ,{0,NULL,0}
#define __kddist_debug_close(self) \
 {\
  struct kddist_debug_slot *__dsiter,*__dsend;\
  __dsend = (__dsiter = (self)->ddd_regv)+(self)->ddd_regc;\
  for (; __dsiter != __dsend; ++__dsiter) {\
   kassert_ktask(__dsiter->ds_task);\
   free(__dsiter->ds_tb);\
  }\
  free((self)->ddd_regv);\
  (self)->ddd_regv = NULL;\
 }
#define __kddist_debug_reset(self) ((self)->ddd_flags &= ~(KDDIST_DEBUG_FLAG_NOMEM))
#else
#define __kddist_debug_close(self) (void)0
#define __kddist_debug_reset(self) (void)0
#endif

struct kddist {
 KOBJECT_HEAD
 struct ksignal        dd_sigpoll; /*< Signal send when data can be polled (NOTE: Data is transferred through this signal). */
 struct ksignal        dd_sigpost; /*< Signal send when all registered users are ready (NOTE: This just is a secondary signal). */
 /* NOTE: When both signals must be locked, 'dd_sigpoll' must be locked first! */
 __un(KCONFIG_DDIST_USERBITS) dd_ready;   /*< [lock(dd_sigpoll:KSIGNAL_LOCK_WAIT)] Amount of registered+ready users. */
#if KCONFIG_HAVE_DEBUG_TRACKEDDDIST
 struct kddist_debug   dd_debug;   /*< [lock(dd_sigpoll:KSIGNAL_LOCK_WAIT)] Debug information for tracking registered users. */
#define dd_users       dd_debug.ddd_regc
#else
#define __KDDIST_DEBUG_INIT  ,0
 __un(KCONFIG_DDIST_USERBITS) dd_users;   /*< [lock(dd_sigpoll:KSIGNAL_LOCK_WAIT)] Amount of registered users. */
#endif
};

#if defined(KSIGNAL_INIT_IS_MEMSET_ZERO) && \
    defined(KOBJECT_INIT_IS_MEMSET_ZERO)
#define KDDIST_INIT_IS_MEMSET_ZERO 1
#endif
#define KDDIST_INIT          {KOBJECT_INIT(KOBJECT_MAGIC_DDIST) KSIGNAL_INIT,KSIGNAL_INIT,0 __KDDIST_DEBUG_INIT}
#define KDDIST_INIT_REGONLY  {KOBJECT_INIT(KOBJECT_MAGIC_DDIST) KSIGNAL_INIT,KSIGNAL_INIT,0 __KDDIST_DEBUG_INIT_REGONLY}

#ifdef __INTELLISENSE__
extern __nonnull((1)) void kddist_init(struct kddist *self);
extern __nonnull((1)) void kddist_init_regonly(struct kddist *self);

//////////////////////////////////////////////////////////////////////////
// Closes a given ddist, causing any successive API calls to to fail
// NOTE: This function can safely be called at any time.
// @return: KE_OK:        The ddist was destroyed.
// @return: KS_UNCHANGED: The ddist was already destroyed.
// @return: KS_EMPTY:     The ddist was destroyed, but no consumer were signaled. (NOT AN ERROR)
extern __nonnull((1)) kerrno_t kddist_close(struct kddist *self);

//////////////////////////////////////////////////////////////////////////
// Reset a given ddist after it had previously been closed.
// @return: KE_OK:        The ddist has been reset.
// @return: KS_UNCHANGED: The ddist was not destroyed.
extern __nonnull((1)) kerrno_t kddist_reset(struct kddist *self);
#else
#ifdef KSIGNAL_INIT_IS_MEMSET_ZERO
#define kddist_init(self) \
 kobject_initzero(self,KOBJECT_MAGIC_DDIST)
#else
#define kddist_init(self) \
 __xblock({ struct kddist *const __ddself = (self);\
            kobject_initzero(__ddself,KOBJECT_MAGIC_DDIST);\
            ksignal_init(&__ddself->dd_sigpoll);\
            ksignal_init(&__ddself->dd_sigpost);\
            (void)0;\
 })
#endif
#if KCONFIG_HAVE_DEBUG_TRACKEDDDIST
#define kddist_init_regonly(self) \
 __xblock({ struct kddist *const __ddroself = (self);\
            kddist_init(__ddroself);\
            __ddroself->dd_debug.ddd_flags |= KDDIST_DEBUG_FLAG_REGONLY;\
            (void)0;\
 })
#else
#define kddist_init_regonly kddist_init
#endif
#define kddist_close(self) \
 __xblock({ struct kddist *const __ddself = (self);\
            kerrno_t __ddcerror; kassert_kddist(__ddself);\
            ksignal_lock(&__ddself->dd_sigpoll,KSIGNAL_LOCK_WAIT);\
            __ddcerror = _ksignal_close_andunlock_c(&__ddself->dd_sigpoll,{\
             ksignal_close(&__ddself->dd_sigpost);\
             __kddist_debug_close(&__ddself->dd_debug);\
             __ddself->dd_ready = 0;\
             __ddself->dd_users = 0;\
            });\
            ksignal_endlock();\
            __xreturn __ddcerror;\
 })
#define kddist_reset(self) \
 __xblock({ struct kddist *const __ddself = (self);\
            kerrno_t __ddcerror; kassert_kddist(__ddself);\
            ksignal_lock(&__ddself->dd_sigpoll,KSIGNAL_LOCK_WAIT);\
            __ddcerror = _ksignal_reset_andunlock_c(&__ddself->dd_sigpoll,{\
             assert(!__ddself->dd_users);\
             assert(!__ddself->dd_ready);\
             __kddist_debug_reset(&__ddself->dd_debug);\
             ksignal_reset(&__ddself->dd_sigpost);\
            });\
            ksignal_endlock();\
            __xreturn __ddcerror;\
 })
#endif


//////////////////////////////////////////////////////////////////////////
// Add/Delete the calling task as a registered user from the data distributor.
// @return: KE_OK:        The caller was successfully registered as a receiver.
// @return: KE_OVERFLOW:  Too many other tasks have already been registered as users.
// @return: KE_DESTROYED: The given ddist was destroyed.
extern __crit __nonnull((1)) kerrno_t kddist_adduser(struct kddist *self);
extern __crit __nonnull((1)) void kddist_deluser(struct kddist *self);


//////////////////////////////////////////////////////////////////////////
// Receive data from the given ddist.
// WARNING: 'kddist_recv' may _ONLY_ be called when the caller was
//          registered as a user through a prior call to 'kddist_adduser'.
//          In the same way, 'kddist_recv_unregistered'
//          may _ONLY_ be used if they were not.
// NOTE: 'kddist_recv_unregistered' may be used if
//       loss-less transmission isn't a requirement.
// @return: KE_OK:        Data was successfully received and stored
//                        in the buffer pointed to by 'buf'.
// @return: KE_DESTROYED: The given ddist was destroyed.
// @return: KE_TIMEDOUT:  [kddist_v(timed|timeout)recv*] The given timeout has expired
// @return: KE_INTR:      The calling task was interrupted.
extern __wunused __nonnull((1,2))   kerrno_t kddist_vrecv(struct kddist *__restrict self, void *__restrict buf);
extern __wunused __nonnull((1,2,3)) kerrno_t kddist_vtimedrecv(struct kddist *__restrict self, struct timespec const *__restrict abstime, void *__restrict buf);
extern __wunused __nonnull((1,2,3)) kerrno_t kddist_vtimeoutrecv(struct kddist *__restrict self, struct timespec const *__restrict timeout, void *__restrict buf);
#if KCONFIG_HAVE_DEBUG_TRACKEDDDIST || defined(__INTELLISENSE__)
extern __wunused __nonnull((1,2))   kerrno_t kddist_vrecv_unregistered(struct kddist *__restrict self, void *__restrict buf);
extern __wunused __nonnull((1,2,3)) kerrno_t kddist_vtimedrecv_unregistered(struct kddist *__restrict self, struct timespec const *__restrict abstime, void *__restrict buf);
extern __wunused __nonnull((1,2,3)) kerrno_t kddist_vtimeoutrecv_unregistered(struct kddist *__restrict self, struct timespec const *__restrict timeout, void *__restrict buf);
#else
#define kddist_vrecv_unregistered(self,buf)                ksignal_vrecv(&(self)->dd_sigpoll,buf)
#define kddist_vtimedrecv_unregistered(self,abstime,buf)   ksignal_vtimedrecv(&(self)->dd_sigpoll,abstime,buf)
#define kddist_vtimeoutrecv_unregistered(self,timeout,buf) ksignal_vtimeoutrecv(&(self)->dd_sigpoll,timeout,buf)
#endif

//////////////////////////////////////////////////////////////////////////
// Sends data to all waiting receiving tasks.
// @return: KE_ISOK(*):    The amount of tasks that received data.
// @return: KE_DESTROYED:  The data distributor was destroyed.
// @return: KE_TIMEDOUT:   [kddist_v(timed|timeout)send] The given timeout has expired.
// @return: KE_WOULDBLOCK: [kddist_vtrysend] Failed to send data immediately.
// @return: KE_INTR:       [kddist_v{timed|timeout}send] The calling task was interrupted.
extern __nonnull((1,2))   __ssize_t kddist_vsend(struct kddist *__restrict self, void *__restrict buf, __size_t bufsize);
extern __nonnull((1,2))   __ssize_t kddist_vtrysend(struct kddist *__restrict self, void *__restrict buf, __size_t bufsize);
extern __nonnull((1,2,3)) __ssize_t kddist_vtimedsend(struct kddist *__restrict self, struct timespec const *__restrict abstime, void *__restrict buf, __size_t bufsize);
extern __nonnull((1,2,3)) __ssize_t kddist_vtimeoutsend(struct kddist *__restrict self, struct timespec const *__restrict timeout, void *__restrict buf, __size_t bufsize);
extern __nonnull((1,2))   __ssize_t kddist_vasyncsend(struct kddist *__restrict self, void *__restrict buf, __size_t bufsize);

#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Begin attempting to send data.
//  - This function is part of the internal implementation of vsend, and can be
//    used in situations where knowledge about the amount of tasks about to
//    receive the signal can influence the data (e.g.: reference counters).
//  - In such situations, the implementor is responsible to ensure that no
//    stray unregistered tasks are also receiving the signal, with the simplest
//    way of doing this being to simply to never to this.
//    Though to help enforce such a restriction, one can initialize the ddist
//    using either 'KDDIST_INIT_REGONLY' or 'kddist_init_regonly'
//    >> For REGONLY ddist's, the return value is always the
//       amount of tasks available for posting a value to, with all
//       functions except for 'kddist_asyncbeginsend_c' also returning
//       the amount of registered tasks.
//  - 'kddist_asyncbeginsend_c' is the non-blocking version that doesn't
//    require all registered 
// WARNING: Calling this function will also lock 'dd_sigpoll:KSIGNAL_LOCK_WAIT',
//          though this is only done for the 'KE_ISOK'-return case.
// @return: KE_ISOK(*):    The amount of registered tasks about to receive data.
// @return: KE_DESTROYED:  The data distributor was destroyed.
// @return: KE_WOULDBLOCK: [kddist_trybeginsend_c] Failed to send data immediately.
// @return: KE_TIMEDOUT:   [kddist_(timed|timeout)beginsend_c] The given timeout has expired.
// @return: KE_INTR:       [kddist_{timed|timeout}beginsend_c] The calling task was interrupted.
extern __crit __nonnull((1))   __ssize_t kddist_beginsend_c(struct kddist *__restrict self);
extern __crit __nonnull((1))   __ssize_t kddist_trybeginsend_c(struct kddist *__restrict self);
extern __crit __nonnull((1,2)) __ssize_t kddist_timedbeginsend_c(struct kddist *__restrict self, struct timespec const *__restrict abstime);
extern __crit __nonnull((1,2)) __ssize_t kddist_timeoutbeginsend_c(struct kddist *__restrict self, struct timespec const *__restrict timeout);
extern __crit __nonnull((1))   __ssize_t kddist_asyncbeginsend_c(struct kddist *__restrict self);
#else
#define kddist_beginsend_c(self) \
 __xblock({ struct kddist *const __ddsself = (self);\
            __ssize_t __ddserr; assert(ktask_iscrit());\
            ksignal_lock_c(&__ddsself->dd_sigpoll,KSIGNAL_LOCK_WAIT);\
            __ddserr = _kddist_beginsend_c_maybeunlock(__ddsself);\
            if __unlikely(KE_ISERR(__ddserr)) {\
             ksignal_unlock_c(&__ddsself->dd_sigpoll,KSIGNAL_LOCK_WAIT);\
            }\
            __xreturn __ddserr;\
 })
#define kddist_trybeginsend_c(self) \
 __xblock({ struct kddist *const __ddsself = (self);\
            __ssize_t __ddserr; assert(ktask_iscrit());\
            ksignal_lock_c(&__ddsself->dd_sigpoll,KSIGNAL_LOCK_WAIT);\
            __ddserr = _kddist_trybeginsend_c_maybeunlock(__ddsself);\
            if __unlikely(KE_ISERR(__ddserr)) {\
             ksignal_unlock_c(&__ddsself->dd_sigpoll,KSIGNAL_LOCK_WAIT);\
            }\
            __xreturn __ddserr;\
 })
#define kddist_timedbeginsend_c(self,abstime) \
 __xblock({ struct kddist *const __ddsself = (self);\
            __ssize_t __ddserr; assert(ktask_iscrit());\
            ksignal_lock_c(&__ddsself->dd_sigpoll,KSIGNAL_LOCK_WAIT);\
            __ddserr = _kddist_timedbeginsend_c_maybeunlock(__ddsself);\
            if __unlikely(KE_ISERR(__ddserr)) {\
             ksignal_unlock_c(&__ddsself->dd_sigpoll,KSIGNAL_LOCK_WAIT);\
            }\
            __xreturn __ddserr;\
 })
#define kddist_timeoutbeginsend_c(self,timeout) \
 __xblock({ struct kddist *const __ddsself = (self);\
            __ssize_t __ddserr; assert(ktask_iscrit());\
            ksignal_lock_c(&__ddsself->dd_sigpoll,KSIGNAL_LOCK_WAIT);\
            __ddserr = _kddist_timeoutbeginsend_c_maybeunlock(__ddsself);\
            if __unlikely(KE_ISERR(__ddserr)) {\
             ksignal_unlock_c(&__ddsself->dd_sigpoll,KSIGNAL_LOCK_WAIT);\
            }\
            __xreturn __ddserr;\
 })
#define kddist_asyncbeginsend_c(self,timeout) \
 __xblock({ struct kddist *const __ddsself = (self);\
            __ssize_t __ddserr; assert(ktask_iscrit());\
            ksignal_lock_c(&__ddsself->dd_sigpoll,KSIGNAL_LOCK_WAIT);\
            __ddserr = _kddist_asyncbeginsend_c_maybeunlock(__ddsself);\
            if __unlikely(KE_ISERR(__ddserr)) {\
             ksignal_unlock_c(&__ddsself->dd_sigpoll,KSIGNAL_LOCK_WAIT);\
            }\
            __xreturn __ddserr;\
 })
#endif

extern __crit __nonnull((1))   __ssize_t _kddist_beginsend_c_maybeunlock(struct kddist *__restrict self);
extern __crit __nonnull((1))   __ssize_t _kddist_trybeginsend_c_maybeunlock(struct kddist *__restrict self);
extern __crit __nonnull((1,2)) __ssize_t _kddist_timedbeginsend_c_maybeunlock(struct kddist *__restrict self, struct timespec const *__restrict abstime);
extern __crit __nonnull((1,2)) __ssize_t _kddist_timeoutbeginsend_c_maybeunlock(struct kddist *__restrict self, struct timespec const *__restrict timeout);
extern __crit __nonnull((1))   __ssize_t _kddist_asyncbeginsend_c_maybeunlock(struct kddist *__restrict self);


#if KCONFIG_HAVE_DEBUG_TRACKEDDDIST || defined(__INTELLISENSE__)
//////////////////////////////////////////////////////////////////////////
// Commit a send operation previously started through a call to 'kddist_b*eginsend_c'
// WARNING: The return value of this function may differ from 'n_tasks',
//          not just for regular ddists, but also for the REGONLY ones.
//          Note though, that for a REGONLY ddist, the returned value is
//          always <= n_tasks!
// @param: n_tasks: The amount of ready tasks, as returned by 'kddist_b*eginsend_c'
// @return: * : The actual amount of signalled tasks.
//              NOTE: For REGONLY ddists, that value can never be greater than 'n_tasks'
extern __crit __nonnull((1,3)) __size_t
kddist_commitsend_c(struct kddist *__restrict self, __size_t n_tasks,
                    void const *__restrict buf, __size_t bufsize);
#else
#define kddist_commitsend_c(self,n_tasks,buf,bufsize) \
 _ksignal_vsendall_andunlock_c(&(self)->dd_sigpoll,buf,bufsize)
#endif

#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Abort sending data after a previously successful call to 'kddist_b*eginsend_c'
// @param: n_tasks: The amount of ready tasks, as returned by 'kddist_b*eginsend_c'
extern __crit __nonnull((1)) void
kddist_abortsend_c(struct kddist *__restrict self, __size_t n_tasks);
#else
#define kddist_abortsend_c(self,n_tasks) \
 __xblock({ struct kddist *const __ddasself = (self);\
            assertf(ksignal_islocked(&self->dd_sigpoll,KSIGNAL_LOCK_WAIT),\
                    "No send operation was started within this ddist!");\
            assert(!__ddasself->dd_ready);\
            __ddasself->dd_ready = (n_tasks);\
            ksignal_unlock_c(&self->dd_sigpoll,KSIGNAL_LOCK_WAIT);\
            (void)0;\
 })
#endif



__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_DDIST_H__ */

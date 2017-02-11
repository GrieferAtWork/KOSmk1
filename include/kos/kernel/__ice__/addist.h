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
#ifndef __KOS_KERNEL_ADDIST_H__
#define __KOS_KERNEL_ADDIST_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/compiler.h>
#include <kos/kernel/features.h>
#include <kos/kernel/signal.h>
#include <kos/kernel/object.h>

//
// Similar to a regular ddist (<kos/include/ddist.h>),
// an asynchronous ddist merely adds the ability to
// send data from inside an IRQ handler, even when
// the calling task (and therefor its stack) are
// currently receiving that same signal.
// 
// This is achieved by kind-of bending the rules concerning
// unbuffered data transfer, by keeping a small internal
// buffer that is filled with data when that data cannot be
// send asynchronously.
//
// Take for example the keyboard driver: In 99% of all
// key strokes, this buffer will remain unused as strokes
// can be broadcast directly to all receivers.
// It is only if not all receivers are present that the
// dynamic data buffer will start being used.
//
// Correct order of data is still guarantied.
//
// NOTE: An asynchronous ddist cannot be used to
//       send/recv data in unregistered mode!
//

__DECL_BEGIN

// TODO: Redo asynchronous data distributors.
//    >> The user should be responsible for
//       tracking their version & the system
//       should be expanded to allow for O(1)
//       caching of buffered data.

#define KOBJECT_MAGIC_ADDIST  0xADD157 /*< ADDIST. */
#define kassert_kaddist(self) kassert_object(self,KOBJECT_MAGIC_ADDIST)

struct kaddistbuf {
 struct kaddistbuf *db_next;    /*< [0..1][owned] Next chunk of asynchronous data. */
 __u32              db_left;    /*< Amount of users left to interpret data (It's really just a reference counter). */
 __size_t           db_size;    /*< Size of the data originally send. */
 __u8               db_data[1]; /*< [db_size] Raw data. */
};

struct kaddistuser {
 struct kaddistuser *du_next; /*< [0..1][owned] Next user control block. */
 void const         *du_user; /*< [?..?] Unique ticket associated with this user. */
 __u32               du_aver; /*< Next packet that must be received through the buffer. */
};

struct kaddist {
 KOBJECT_HEAD
 struct ksignal        add_sigpoll; /*< Signal send when data can be polled (NOTE: Data is transferred through this signal). */
 __u32                 add_currver; /*< [lock(add_sigpoll:KSIGNAL_LOCK_WAIT)] Current asynchronous data id (ID of data stored in first entry of 'add_async'). */
 __u32                 add_nextver; /*< [lock(add_sigpoll:KSIGNAL_LOCK_WAIT)] Next asynchronous data id (ID of next packet that must be passed through the buffer).
                                         NOTE: 'add_nextver-add_currver == count(add_async..add_async->db_next)'. */
 struct kaddistbuf    *add_async;   /*< [lock(add_sigpoll:KSIGNAL_LOCK_WAIT)][0..1] Asynchronous data (When non-NULL, buffered data must be received by users). */
 struct kaddistuser   *add_userp;   /*< [lock(add_sigpoll:KSIGNAL_LOCK_WAIT)][0..1] Linked list of users, used to track packet ids already received. */
 __un(KDDIST_USERBITS) add_users;   /*< [lock(add_sigpoll:KSIGNAL_LOCK_WAIT)] Total amount of users (== count(add_userp..add_userp->du_next)). */
 __un(KDDIST_USERBITS) add_ready;   /*< [lock(add_sigpoll:KSIGNAL_LOCK_WAIT)] Amount of ready users. */
};

#if defined(KSIGNAL_INIT_IS_MEMSET_ZERO) && \
    defined(KOBJECT_INIT_IS_MEMSET_ZERO)
#define KADDIST_INIT_IS_MEMSET_ZERO 1
#endif
#define KADDIST_INIT  \
 {KOBJECT_INIT(KOBJECT_MAGIC_ADDIST) KSIGNAL_INIT,0,0,NULL,NULL,0,0}

#ifdef __INTELLISENSE__
extern void kaddist_init(struct kaddist *self);

//////////////////////////////////////////////////////////////////////////
// Closes a given ddist, causing any successive API calls to to fail
// NOTE: This function can safely be called at any time.
// @return: KE_OK:        The ddist was destroyed.
// @return: KS_UNCHANGED: The ddist was already destroyed.
// @return: KS_EMPTY:     The ddist was destroyed, but no consumer were signaled. (NOT AN ERROR)
extern kerrno_t kaddist_close(struct kaddist *self);

//////////////////////////////////////////////////////////////////////////
// Reset a given ddist after it had previously been closed.
// @return: KE_OK:        The ddist has been reset.
// @return: KS_UNCHANGED: The ddist was not destroyed.
extern kerrno_t kaddist_reset(struct kaddist *self);
#else
#ifdef KSIGNAL_INIT_IS_MEMSET_ZERO
#define kaddist_init(self) \
 kobject_initzero(self,KOBJECT_MAGIC_DDIST)
#else
#define kaddist_init(self) \
 __xblock({ struct kaddist *const __addself = (self);\
            kobject_initzero(__addself,KOBJECT_MAGIC_DDIST);\
            ksignal_init(&__addself->add_sigpoll);\
            __addself->add_currver = 0;\
            __addself->add_nextver = 0;\
            __addself->add_async = NULL;\
            __addself->add_userp = NULL;\
            __addself->add_users = 0;\
            __addself->add_ready = 0;\
            (void)0;\
 })
#endif
#define kaddist_close(self) \
 __xblock({ struct kaddist *const __addself = (self);\
            kerrno_t __addcerror; kassert_kaddist(__addself);\
            ksignal_lock(&__addself->add_sigpoll,KSIGNAL_LOCK_WAIT);\
            __addcerror = _ksignal_close_andunlock_c(&__addself->add_sigpoll,{\
             struct kaddistbuf *__diter,*__dnext;\
             struct kaddistuser *__uiter,*__unext;\
             for (__diter = __addself->add_async;\
                 (__dnext = __diter->db_next,__diter);\
                  __diter = __dnext) free(__diter);\
             for (__uiter = __addself->add_userp;\
                 (__unext = __uiter->du_next,__uiter);\
                  __uiter = __unext) free(__uiter);\
             __addself->add_async = NULL;\
             __addself->add_userp = NULL;\
             __addself->add_ready = 0;\
             __addself->add_users = 0;\
            });\
            ksignal_endlock();\
            __xreturn __addcerror;\
 })
#define kaddist_reset(self) \
 __xblock({ struct kaddist *const __addself = (self);\
            kerrno_t __addcerror; kassert_kaddist(__addself);\
            ksignal_lock(&__addself->add_sigpoll,KSIGNAL_LOCK_WAIT);\
            __addcerror = _ksignal_reset_andunlock_c(&__addself->add_sigpoll,{\
             assert(!__addself->add_async);\
             assert(!__addself->add_userp);\
             assert(!__addself->add_ready);\
             assert(!__addself->add_users);\
            });\
            ksignal_endlock();\
            __xreturn __addcerror;\
 })
#endif


//////////////////////////////////////////////////////////////////////////
// Add/Delete the calling task as a registered user from the data distributor.
// @param: ticket: A unique identifier to associate with the caller.
//           NOTE: If ddist-behavior is wanted, pass 'ktask_self()'.
//           HINT: If you want to implement some kind of loss-less device
//                 file such as '/dev/keyboard', pass an malloc-ed pointer,
//                 or the pointer of some containment structure.
// @return: KE_OK:        The caller was successfully registered as a receiver.
// @return: KE_OVERFLOW:  Too many other tasks have already been registered as users.
// @return: KE_NOMEM:     Not enough memory to register the caller.
// @return: KE_DESTROYED: The given ddist was destroyed.
extern __crit kerrno_t kaddist_adduser(struct kaddist *self, void const *ticket);
extern __crit void kaddist_deluser(struct kaddist *self, void const *ticket);


//////////////////////////////////////////////////////////////////////////
// Receive data from the given ddist.
// WARNING: 'kaddist_recv' may _ONLY_ be called when the caller was
//          registered as a user through a prior call to 'kaddist_adduser'.
// @return: KE_OK:         Data was successfully received and stored
//                         in the buffer pointed to by 'buf'.
// @return: KS_BLOCKING:   Data was received through a buffer.
// @return: KE_DESTROYED:  The given ddist was destroyed.
// @return: KE_TIMEDOUT:   [kaddist_v(timed|timeout)recv] The given timeout has expired.
// @return: KE_WOULDBLOCK: [kaddist_vtryrecv] Failed to immediately read a packet from cache.
// @return: KE_INTR:      [!kaddist_vtryrecv] The calling task was interrupted.
extern __wunused __nonnull((1,2))   kerrno_t kaddist_vrecv(struct kaddist *__restrict self, void *__restrict buf, void const *ticket);
extern __wunused __nonnull((1,2))   kerrno_t kaddist_vtryrecv(struct kaddist *__restrict self, void *__restrict buf, void const *ticket);
extern __wunused __nonnull((1,2,3)) kerrno_t kaddist_vtimedrecv(struct kaddist *__restrict self, struct timespec const *__restrict abstime, void *__restrict buf, void const *ticket);
extern __wunused __nonnull((1,2,3)) kerrno_t kaddist_vtimeoutrecv(struct kaddist *__restrict self, struct timespec const *__restrict timeout, void *__restrict buf, void const *ticket);

#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Sends data to all waiting receiving tasks.
// @return: KE_OK:        Data was send to 'self->add_users' tasks.
// @return: KS_EMPTY:     No data was send, because noone was listening...
// @return: KS_BLOCKING:  Some, or all users are not ready to receive data
//                        and the given buffer was queued for transfer instead
//                        of being send without intermediate buffering.
//                        NOTE: This is not an error. The data will
//                              still be received like intended.
// @return: KE_DESTROYED: The data distributor was destroyed.
// @return: KE_NOMEM:     Data would have had to be transferred through use
//                        of a buffer, but not enough memory was available
//                        to allocate that buffer.
// @return: KE_OVERFLOW:  The buffer has grown too big and a buffered
//                        package cannot actually be queued.
extern __nonnull((1)) kerrno_t
kaddist_vsend(struct kaddist *__restrict self,
              void *__restrict buf, __size_t bufsize);
#else
#define kaddist_vsend(self,buf,bufsize) \
 __xblock({ struct kaddist *const __advsself = (self);\
            kerrno_t __advserr; kassert_kaddist(__advsself);\
            ksignal_lock(&__advsself->add_sigpoll,KSIGNAL_LOCK_WAIT);\
            __advserr = _kaddist_vsend_andunlock(__advsself,buf,bufsize);\
            ksignal_endlock();\
            __xreturn __advserr;\
 })
#endif
extern __crit __nonnull((1)) kerrno_t
_kaddist_vsend_andunlock(struct kaddist *__restrict self,
                         void *__restrict buf, __size_t bufsize);

__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_ASYNCDDIST_H__ */

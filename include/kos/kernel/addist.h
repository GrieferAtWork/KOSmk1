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

#define KOBJECT_MAGIC_ADDIST2TICKET 0xAD15771C /*< ADISTTIC. */
#define KOBJECT_MAGIC_ADDIST2       0xADD157   /*< ADDIST. */
#define kassert_kaddistticket(self) kassert_object(self,KOBJECT_MAGIC_ADDIST2TICKET)
#define kassert_kaddist(self)       kassert_object(self,KOBJECT_MAGIC_ADDIST2)

struct kaddist;
struct kaddistticket;
struct kasyncdata;
struct tbtrace;

struct kaddistticket {
 KOBJECT_HEAD
#if KCONFIG_HAVE_DEBUG_TRACKEDDDIST
 struct kaddist          *dt_dist;  /*< [1..1] Distributor associated with this ticket. */
 struct tbtrace          *dt_mk_tb; /*< [0..1][owned] Traceback to when this ticket was generated. */
 struct tbtrace          *dt_rd_tb; /*< [0..1][lock(:ad_nbdat:KSIGNAL_LOCK_WAIT)][owned] Traceback to when a ticket became ready. */
#endif
 struct kaddistticket    *dt_prev;  /*< [0..1][lock(:ad_nbdat:KSIGNAL_LOCK_WAIT)] Previous ready/non-ready ticket. */
 struct kaddistticket    *dt_next;  /*< [0..1][lock(:ad_nbdat:KSIGNAL_LOCK_WAIT)] Next ready/non-ready ticket. */
 __ref struct kasyncdata *dt_async; /*< [0..1][lock(:ad_nbdat:KSIGNAL_LOCK_WAIT)] Next asynchronous chunk to-be parsed (NULL if the ticket can be registered as ready). */
};

struct kasyncdata {
 __atomic __un(KCONFIG_DDIST_USERBITS) sd_pending; /*< [!0] Amount of tickets pending to receive this chunk (acts as a reference counter),
                                                     but must not reach ZERO(0) if this isn't the first chunk (aka 'tc_prev' != NULL). */
 struct kasyncdata             *sd_prev;    /*< [0..1] Previous data chunk, or NULL if first. */
 struct kasyncdata             *sd_next;    /*< [0..1] Next data chunk, or NULL if last. */
 __u8                           sd_data[1]; /*< [:ad_chunksz] Inlined vector of memory. */
};
struct kaddist {
 KOBJECT_HEAD
 struct ksignal        ad_nbdat;    /*< Non-buffered data transfer signal. */
 __un(KCONFIG_DDIST_USERBITS) ad_ready;    /*< [lock(ad_nbdat:KSIGNAL_LOCK_WAIT)][<=ad_known] Amount of ticket holders ready for unbuffered transfer of data
                                        (equal to the linked list length in 'ad_tready', but due to race conditions may not be equal to the amount of scheduled tasks in 'ad_nbdat'). */
 __un(KCONFIG_DDIST_USERBITS) ad_known;    /*< [lock(ad_nbdat:KSIGNAL_LOCK_WAIT)][>=ad_ready] Amount of known ticket holders. */
 struct kaddistticket *ad_tiready;  /*< [lock(ad_nbdat:KSIGNAL_LOCK_WAIT)][0..1] Linked list of tickets that are ready. */
 struct kaddistticket *ad_tnready;  /*< [lock(ad_nbdat:KSIGNAL_LOCK_WAIT)][0..1] Linked list of tickets that are not ready. */
 __size_t              ad_chunksz;  /*< [const] Size of the raw data block in any given chunk. */
 __size_t              ad_chunkmax; /*< [const] Max amount of chunks allowed before data is overwritten. */
 __size_t              ad_chunkc;   /*< [lock(ad_nbdat:KSIGNAL_LOCK_WAIT)] Current amount of existing chunks. (When equal to 'ad_chunkmax', data in the last chunk 'ad_back' is overwritten). */
 struct kasyncdata    *ad_front;    /*< [lock(ad_nbdat:KSIGNAL_LOCK_WAIT)][->sd_prev == NULL][0..1][owned] First asynchronous data chunk. */
 struct kasyncdata    *ad_back;     /*< [lock(ad_nbdat:KSIGNAL_LOCK_WAIT)][->sd_next == NULL][0..1][owned] Last asynchronous data chunk. */
};

#define KADDIST_INIT(chunksize,chunkmax) \
 {KOBJECT_INIT(KOBJECT_MAGIC_ADDIST2) KSIGNAL_INIT\
 ,0,0,NULL,NULL,chunksize,chunkmax,0,NULL,NULL}

#ifdef __INTELLISENSE__
extern __nonnull((1)) void
kaddist_init(struct kaddist *__restrict self,
             __size_t chunksize, __size_t chunkmax);

//////////////////////////////////////////////////////////////////////////
// Closes a given ddist, causing any successive API calls to to fail
// NOTE: This function can safely be called at any time.
// @return: KE_OK:        The ddist was destroyed.
// @return: KS_UNCHANGED: The ddist was already destroyed.
// @return: KS_EMPTY:     The ddist was destroyed, but no consumer were signaled. (NOT AN ERROR)
extern __nonnull((1)) kerrno_t kaddist_close(struct kaddist *__restrict self);

//////////////////////////////////////////////////////////////////////////
// Reset a given ddist after it had previously been closed.
// @return: KE_OK:        The ddist has been reset.
// @return: KS_UNCHANGED: The ddist was not destroyed.
extern __nonnull((1)) kerrno_t kaddist_reset(struct kaddist *__restrict self);

//////////////////////////////////////////////////////////////////////////
// Returns the size of a single chunk of data, as
// send/received by a given asynchronous data distributer.
extern __wunused __constcall __nonnull((1)) size_t
kaddist_chunksize(struct kaddist const *__restrict self);

//////////////////////////////////////////////////////////////////////////
// Returns the max amount of chunks allowed in a given data
// distributer before existing chunks start getting overwritten.
extern __wunused __constcall __nonnull((1)) size_t
kaddist_chunkmax(struct kaddist const *__restrict self);

//////////////////////////////////////////////////////////////////////////
// Returns the currently used amount of chunks.
extern __wunused __nonnull((1)) size_t
kaddist_chunkcur(struct kaddist const *__restrict self);
#else
#define kaddist_init(self,chunksize,chunkmax) \
 __xblock({ struct kaddist *const __kadself = (self);\
            kobject_initzero(__kadself,KOBJECT_MAGIC_ADDIST2);\
            ksignal_init(&__kadself->ad_nbdat);\
            __kadself->ad_chunksz = (chunksize);\
            __kadself->ad_chunkmax = (chunkmax);\
            (void)0;\
 })
#define kaddist_close(self) ksignal_close(&(self)->ad_nbdat)
#define kaddist_reset(self) ksignal_reset(&(self)->ad_nbdat)
#define kaddist_chunksize(self) ((self)->ad_chunksz)
#define kaddist_chunkmax(self)  ((self)->ad_chunkmax)
#define kaddist_chunkcur(self) \
 __xblock({ struct kaddist const *const __kadself = (self);\
            __size_t __kadres;\
            ksignal_lock(&__kadself->ad_nbdat,KSIGNAL_LOCK_WAIT);\
            __kadres = __kadself->ad_chunkc;\
            ksignal_unlock(&__kadself->ad_nbdat,KSIGNAL_LOCK_WAIT);\
            __xreturn __kadres;\
 })
#endif

#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Generates, initialized and registers a new ticket for a given data distributor.
// NOTE: Once generated, a ticket must be passed in any receive-operation
//       before eventually being destroyed using 'kaddist_delticket'.
//       Failure to do so will cause a resource leak.
// WARNING: A ticket may only be registered once!
// @param: ticket: A globally unique pointer to an instance of a ticket.
//                 HINT: May be allocated dynamically, or on the stack.
// @return: KE_OK:        The ticket was successfully created.
// @return: KE_DESTROYED: The given distributer was closed.
// @return: KE_OVERFLOW:  Too many existing tickets.
extern __crit __wunused __nonnull((1,2)) kerrno_t
kaddist_genticket(struct kaddist *__restrict self,
                  struct kaddistticket *__restrict ticket);

//////////////////////////////////////////////////////////////////////////
// Deletes a given ticket, dropping any pending data from being received
// and causing undefined behavior if the ticket is continued to be used.
// @param: ticket: The same pointer as passed in a prior call to 'kaddist_genticket'.
extern __crit __nonnull((1,2)) void
kaddist_delticket(struct kaddist *__restrict self,
                  struct kaddistticket *__restrict ticket);
#else
#define kaddist_genticket(self,ticket) \
 __xblock({ struct kaddist *const __advgtself = (self);\
            kassert_kaddist(__advgtself);\
            ksignal_lock_c(&__advgtself->ad_nbdat,KSIGNAL_LOCK_WAIT);\
            __xreturn _kaddist_genticket_andunlock(__advgtself,ticket);\
 })
#define kaddist_delticket(self,ticket) \
 __xblock({ struct kaddist *const __advdtself = (self);\
            kassert_kaddist(__advdtself);\
            ksignal_lock_c(&__advdtself->ad_nbdat,KSIGNAL_LOCK_WAIT);\
            __xreturn _kaddist_delticket_andunlock(__advdtself,ticket);\
 })
#endif
extern __crit __wunused __nonnull((1,2)) kerrno_t
_kaddist_genticket_andunlock(struct kaddist *__restrict self,
                             struct kaddistticket *__restrict ticket);
extern __crit __nonnull((1,2)) void
_kaddist_delticket_andunlock(struct kaddist *__restrict self,
                             struct kaddistticket *__restrict ticket);


struct timespec;

#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Asynchronously receive a single chunk of data, storing its contents in 'buf'.
// If no asynchronous data is pending for the given ticket, and a function
// other than 'kaddist_vtryrecv' is used, blocking until a given timeout
// or until data can be transferred without the use of an intermediate buffer.
// Undefined behavior is invoked if:
//  - The given ticket was not generated through a call to 'kaddist_genticket'.
//  - The given ticket was not generated for a different distributer.
//  - The given ticket is already used in a blocking receive operation (not brought forth by prior call 'kaddist_vtryrecv').
//  - The given ticket is removed while also used in a blocking receive operation (not brought forth by prior call 'kaddist_vtryrecv').
// @param: buf: A pointer to a writable buffer large enough to hold 'chunksize' specified in 'kaddist_init'
// @return: KE_OK:         A chunk of data was successfully received and stored in '*buf'
// @return: KS_BLOCKING:   The call has invoked the scheduler when no asynchronous data was buffered.
// @return: KE_DESTROYED:  The given distributer was destroyed and the caller must delete their ticket using 'kaddist_delticket'
// @return: KE_TIMEDOUT:   [kaddist_v(timed|timeout)recv] The given timeout has expired.
// @return: KE_WOULDBLOCK: [kaddist_vtryrecv] Failed to immediately read a packet from cache.
// @return: KE_INTR:      [!kaddist_vtryrecv] The calling task was interrupted.
extern __wunused __nonnull((1,2,3))   kerrno_t kaddist_vrecv(struct kaddist *__restrict self, struct kaddistticket *__restrict ticket, void *__restrict buf);
extern __wunused __nonnull((1,2,3))   kerrno_t kaddist_vtryrecv(struct kaddist *__restrict self, struct kaddistticket *__restrict ticket, void *__restrict buf);
extern __wunused __nonnull((1,2,3,4)) kerrno_t kaddist_vtimedrecv(struct kaddist *__restrict self, struct kaddistticket *__restrict ticket, struct timespec const *__restrict abstime, void *__restrict buf);
extern __wunused __nonnull((1,2,3,4)) kerrno_t kaddist_vtimeoutrecv(struct kaddist *__restrict self, struct kaddistticket *__restrict ticket, struct timespec const *__restrict timeout, void *__restrict buf);
#else
#define kaddist_vrecv(self,ticket,buf) \
 __xblock({ struct kaddist *const __advrself = (self);\
            kerrno_t __advrerr; kassert_kaddist(__advrself);\
            KTASK_CRIT_BEGIN\
            ksignal_lock(&__advrself->ad_nbdat,KSIGNAL_LOCK_WAIT);\
            __advrerr = _kaddist_vrecv_andunlock(__advrself,ticket,buf);\
            ksignal_endlock();\
            KTASK_CRIT_END\
            __xreturn __advrerr;\
 })
#define kaddist_vtryrecv(self,ticket,buf) \
 __xblock({ struct kaddist *const __advrself = (self);\
            kerrno_t __advrerr; kassert_kaddist(__advrself);\
            KTASK_CRIT_BEGIN\
            ksignal_lock(&__advrself->ad_nbdat,KSIGNAL_LOCK_WAIT);\
            __advrerr = _kaddist_vtryrecv_andunlock(__advrself,ticket,buf);\
            ksignal_endlock();\
            KTASK_CRIT_END\
            __xreturn __advrerr;\
 })
#define kaddist_vtimedrecv(self,ticket,abstime,buf) \
 __xblock({ struct kaddist *const __advrself = (self);\
            kerrno_t __advrerr; kassert_kaddist(__advrself);\
            KTASK_CRIT_BEGIN\
            ksignal_lock(&__advrself->ad_nbdat,KSIGNAL_LOCK_WAIT);\
            __advrerr = _kaddist_vtimedrecv_andunlock(__advrself,ticket,abstime,buf);\
            ksignal_endlock();\
            KTASK_CRIT_END\
            __xreturn __advrerr;\
 })
#define kaddist_vtimeoutrecv(self,ticket,timeout,buf) \
 __xblock({ struct kaddist *const __advrself = (self);\
            kerrno_t __advrerr; kassert_kaddist(__advrself);\
            KTASK_CRIT_BEGIN\
            ksignal_lock(&__advrself->ad_nbdat,KSIGNAL_LOCK_WAIT);\
            __advrerr = _kaddist_vtimeoutrecv_andunlock(__advrself,ticket,timeout,buf);\
            ksignal_endlock();\
            KTASK_CRIT_END\
            __xreturn __advrerr;\
 })
#endif
extern __crit __wunused __nonnull((1,2,3))   kerrno_t _kaddist_vrecv_andunlock(struct kaddist *__restrict self, struct kaddistticket *__restrict ticket, void *__restrict buf);
extern __crit __wunused __nonnull((1,2,3))   kerrno_t _kaddist_vtryrecv_andunlock(struct kaddist *__restrict self, struct kaddistticket *__restrict ticket, void *__restrict buf);
extern __crit __wunused __nonnull((1,2,3,4)) kerrno_t _kaddist_vtimedrecv_andunlock(struct kaddist *__restrict self, struct kaddistticket *__restrict ticket, struct timespec const *__restrict abstime, void *__restrict buf);
extern __crit __wunused __nonnull((1,2,3,4)) kerrno_t _kaddist_vtimeoutrecv_andunlock(struct kaddist *__restrict self, struct kaddistticket *__restrict ticket, struct timespec const *__restrict timeout, void *__restrict buf);

#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Send a chunk of data to all registered tickets,
// preferring unbuffered transfer of data whenever possible.
// @param: buf: A pointer to readable memory holding 'chunksize' bytes to be send to all registered tickets.
// @return: KS_FULL:      The chunk buffer was full and the last chunk was overwritten.
// @return: KS_EMPTY:     Data was neither buffered, nor send because no tickets are registered.
// @return: KS_BLOCKING:  Some, or all data was scheduled for transfer using the asynchronous buffer.
// @return: KS_FOUND:     The distributer isn't allowed to use chunks, but only some all tickets were ready.
// @return: KE_BUSY:      The distributer isn't allowed to use chunks, but no tickets were ready.
// @return: KE_OK:        All data was send using non-blocking and unbuffered data transfer.
// @return: KE_DESTROYED: The data distributor was destroyed.
// @return: KE_NOMEM:     Not all tickets were ready, but not enough memory was available to allocate a new chunk.
extern __nonnull((1,2)) kerrno_t
kaddist_vsend(struct kaddist *__restrict self,
              void const *__restrict buf);
#else
#define kaddist_vsend(self,buf) \
 __xblock({ struct kaddist *const __advsself = (self);\
            kerrno_t __advserr; kassert_kaddist(__advsself);\
            ksignal_lock(&__advsself->ad_nbdat,KSIGNAL_LOCK_WAIT);\
            __advserr = _kaddist_vsend_andunlock(__advsself,buf);\
            ksignal_endlock();\
            __xreturn __advserr;\
 })
#endif
extern __crit __nonnull((1,2)) kerrno_t
_kaddist_vsend_andunlock(struct kaddist *__restrict self,
                         void const *__restrict buf);


__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_ADDIST_H__ */

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
#ifndef __KOS_KERNEL_SEM_H__
#define __KOS_KERNEL_SEM_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/compiler.h>
#include <kos/errno.h>
#include <kos/kernel/features.h>
#include <kos/kernel/object.h>
#include <kos/kernel/signal.h>
#include <kos/kernel/types.h>

__DECL_BEGIN

#define KOBJECT_MAGIC_SEM 0x5E77 /*< SEM. */

#define kassert_ksem(self) kassert_object(self,KOBJECT_MAGIC_SEM)

struct timespec;
struct ksem;

struct ksem {
 KOBJECT_HEAD
 struct ksignal       s_sig; /*< Signal used to manage sleeping tasks. */
#if KSEMAPHORE_TICKETBITS <= KSIGNAL_USERBITS
#define s_tck         s_sig.s_useru
#else
 __atomic ksemcount_t s_tck; /*< The amount of available tickets. */
#endif
};

#if KSEMAPHORE_TICKETBITS <= KSIGNAL_USERBITS
#define KSEM_INIT(tickets) {KOBJECT_INIT(KOBJECT_MAGIC_SEM) KSIGNAL_INIT_U(tickets)}
#else
#define KSEM_INIT(tickets) {KOBJECT_INIT(KOBJECT_MAGIC_SEM) KSIGNAL_INIT,tickets}
#endif
#define ksem_init(self,tickets) \
 __xblock({ struct ksem *const __semself=(self);\
            ksignal_init(&__semself->s_sig);\
            __semself->s_tck = (tickets);\
            (void)0;\
 })

#define ksem_reset(self,tickets) \
 __xblock({ struct ksem *const __sself = (self);\
            __xreturn ksignal_reset(&__sself->s_sig,__sself->s_tck = (tickets));\
 })

#define ksem_close(self) ksignal_close(&(self)->s_sig)

//////////////////////////////////////////////////////////////////////////
// Posts 'new_tickets' tickets to the semaphore, waking
// the correct amount of waiting tasks in the process.
//  - If non-NULL, '*old_tickets' is set to the amount of available tickets before the call.
//    HINT: To query the amount of available tickets, set 'new_tickets' to '0'
// @return: KE_OVERFLOW:  Adding 'new_tickets' would overflow the ticket counter
//                       (No tickets were added, but '*old_tickets' was filled)
// @return: KE_DESTROYED: The semaphore was destroyed
extern __wunused __nonnull((1)) kerrno_t ksem_post(struct ksem *self,
                                                   ksemcount_t  new_tickets,
                                            /*opt*/ksemcount_t *old_tickets);

//////////////////////////////////////////////////////////////////////////
// Waits for a ticket to become available, and returns it.
// @return: KE_DESTROYED:  The semaphore was destroyed.
// @return: KE_WOULDBLOCK: [ksem_trywait] Failed to acquire a ticket lock immediately.
// @return: KE_TIMEDOUT:   [ksem_(timed|timeout)wait] The given timeout has passed.
// @return: KE_INTR:      [!ksem_trywait] The calling task was interrupted.
extern __wunused __nonnull((1))   kerrno_t ksem_wait(struct ksem *self);
extern __wunused __nonnull((1))   kerrno_t ksem_trywait(struct ksem *self);
extern __wunused __nonnull((1,2)) kerrno_t ksem_timedwait(struct ksem *self, struct timespec const *abstime);
extern __wunused __nonnull((1,2)) kerrno_t ksem_timoutwait(struct ksem *self, struct timespec const *timeout);

__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_SEM_H__ */

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
#ifdef __INTELLISENSE__
#include "addist.c.inl"
#define TIMEDRECV
__DECL_BEGIN
#endif


#ifdef TIMEDRECV
__crit kerrno_t
_kaddist_vtimedrecv_andunlock(struct kaddist *__restrict self,
                              struct kaddistticket *__restrict ticket,
                              struct timespec const *__restrict abstime,
                              void *__restrict buf)
#else
__crit kerrno_t
_kaddist_vrecv_andunlock(struct kaddist *__restrict self,
                         struct kaddistticket *__restrict ticket,
                         void *__restrict buf)
#endif
{
 kerrno_t error;
 KTASK_CRIT_MARK
again:
 /* Check for asynchronous, pending data. */
 error = kaddist_vtryrecv_unlocked(self,ticket,buf);
 if (error != KE_WOULDBLOCK) goto end;
 /* Make use of unbuffered data transfer. */
 /* Register the ticket as ready and begin receiving data. */
 assert(self->ad_known != 0);
 assert(self->ad_ready < self->ad_known);
 /* Add the ticket to the ready counter. */
 ++self->ad_ready;
 /* Move the ticket to the ready list. */
 assert((ticket->dt_prev != NULL) == (ticket != self->ad_tnready));
 if (ticket->dt_next) ticket->dt_next->dt_prev = ticket->dt_prev;
 if (ticket->dt_prev) ticket->dt_prev->dt_next = ticket->dt_next;
 else self->ad_tnready = ticket->dt_next;
 ticket->dt_prev = NULL;
 ticket->dt_next = self->ad_tiready;
 self->ad_tiready = ticket;
 /* Unlock the signal and receive unbuffered data. */
#ifdef TIMEDRECV
 error = _ksignal_vtimedrecv_andunlock_c(&self->ad_nbdat,abstime,buf);
#else
 error = _ksignal_vrecv_andunlock_c(&self->ad_nbdat,buf);
#endif
 assertf(error != KE_DESTROYED,"But we've already checked that above...");
 if __unlikely(error == KS_NODATA) {
  /* This can happen if the sender cannot determine if data can be
   * send without use of a buffer, which might happen due to inconsistencies
   * between the amount of ready tasks and actually signal-scheduled tasks.
   * In this (very rare) case, data was send using a buffer, meaning we
   * can simply start over to check for asynchronous, pending data. */
  ksignal_lock_c(&self->ad_nbdat,KSIGNAL_LOCK_WAIT);
  goto again;
 }
 /* In the success-case, the sender has done the ready-cleanup
  * and moved the ticket into the list of non-ready tickets. */
 if __likely(KE_ISOK(error)) return error;
 /* Failed to receive data. - Must unregister the ticket due to that failure.
  * NOTE: The race condition between here and after vrecv is what is handled by 'KS_NODATA'. */
 ksignal_lock_c(&self->ad_nbdat,KSIGNAL_LOCK_WAIT);
 assert(kaddist_hasreadyticket(self,ticket) != NO);
 assert(self->ad_ready);
 assert((ticket->dt_prev != NULL) == (ticket != self->ad_tiready));
 --self->ad_ready;
 /* Do some cleanup to remove the ticket from the list of
  * ready tickets, adding it to that of non-ready ones. */
 if (ticket->dt_next) ticket->dt_next->dt_prev = ticket->dt_prev;
 if (ticket->dt_prev)
  ticket->dt_prev->dt_next = ticket->dt_next,
  ticket->dt_prev = NULL;
 else self->ad_tiready = ticket->dt_next;
 if ((ticket->dt_next = self->ad_tnready) != NULL)
  ticket->dt_next->dt_prev = ticket;
 self->ad_tnready = ticket;
#if KCONFIG_HAVE_DEBUG_TRACKEDDDIST
 free(ticket->dt_rd_tb);
 ticket->dt_rd_tb = tbtrace_captureex(1);
#endif /* KCONFIG_HAVE_DEBUG_TRACKEDDDIST */
end:
 ksignal_unlock_c(&self->ad_nbdat,KSIGNAL_LOCK_WAIT);
 return error;
}

#ifdef TIMEDRECV
#undef TIMEDRECV
#endif

#ifdef __INTELLISENSE__
__DECL_END
#endif


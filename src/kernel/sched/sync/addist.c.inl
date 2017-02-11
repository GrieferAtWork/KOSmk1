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
#ifndef __KOS_KERNEL_ADDIST_C_INL__
#define __KOS_KERNEL_ADDIST_C_INL__ 1

#include <kos/config.h>
#include <kos/kernel/addist.h>
#include <kos/kernel/debug.h>
#include <kos/kernel/signal_v.h>
#include <kos/kernel/syslog.h>
#include <kos/kernel/time.h>
#include <kos/timespec.h>
#include <malloc.h>
#include <stddef.h>
#if KDEBUG_HAVE_TRACKEDDDIST
#include <traceback.h>
#endif

__DECL_BEGIN

#define YES      1
#define NO       0
#define MAYBE  (-1)
#define ISDEAD(self) ((self)->ad_nbdat.s_flags&KSIGNAL_FLAG_DEAD)

#if KDEBUG_HAVE_TRACKEDDDIST
static int kaddist_hasticket(struct kaddist *__restrict self,
                             struct kaddistticket *__restrict ticket) {
 struct kaddistticket *iter;
 for (iter = self->ad_tiready; iter; iter = iter->dt_next) if (iter == ticket) return YES;
 for (iter = self->ad_tnready; iter; iter = iter->dt_next) if (iter == ticket) return YES;
 return NO;
}
static int kaddist_hasreadyticket(struct kaddist *__restrict self,
                                  struct kaddistticket *__restrict ticket) {
 struct kaddistticket *iter;
 for (iter = self->ad_tiready; iter; iter = iter->dt_next) if (iter == ticket) return YES;
 return NO;
}
#else
#define kaddist_hasticket(self,ticket)      MAYBE
#define kaddist_hasreadyticket(self,ticket) MAYBE
#endif



__crit kerrno_t
_kaddist_genticket_andunlock(struct kaddist *__restrict self,
                             struct kaddistticket *__restrict ticket) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kaddist(self);
 kassertobj(ticket);
 assert(ksignal_islocked(&self->ad_nbdat,KSIGNAL_LOCK_WAIT));
 assert(self->ad_ready <= self->ad_known);
 assert(!self->ad_front || !self->ad_front->sd_prev);
 assert(!self->ad_back || !self->ad_back->sd_next);
 assert(self->ad_chunkc <= self->ad_chunkmax);
#if KDEBUG_HAVE_TRACKEDDDIST
 if (kaddist_hasticket(self,ticket) == YES) {
  k_syslogf(KLOG_ERROR
           ,"[ADDIST] Ticket at %p is already in use\n"
            "See reference to ticket generation:\n"
           ,ticket);
  dtraceback_print(ticket->dt_mk_tb);
  assertf(0,"Ticket %p is already in use",ticket);
 }
#endif
 if __unlikely(ISDEAD(self)) { error = KE_DESTROYED; goto end; }
 if __unlikely(self->ad_known == (__un(KDDIST_USERBITS))-1) { error = KE_OVERFLOW; goto end; }
 ++self->ad_known;
 kobject_init(ticket,KOBJECT_MAGIC_ADDIST2TICKET);
#if KDEBUG_HAVE_TRACKEDDDIST
 ticket->dt_dist = self;
 ticket->dt_mk_tb = dtraceback_captureex(1);
 ticket->dt_rd_tb = NULL;
#endif
 ticket->dt_async = NULL;
 ticket->dt_prev = NULL;
 /* Hook the ticket into the list of non-ready tickets. */
 if ((ticket->dt_next = self->ad_tnready) != NULL)
  ticket->dt_next->dt_prev = ticket;
 self->ad_tnready = ticket;
 error = KE_OK;
end:
 ksignal_unlock_c(&self->ad_nbdat,KSIGNAL_LOCK_WAIT);
 return error;
}

__crit void
_kaddist_delticket_andunlock(struct kaddist *__restrict self,
                             struct kaddistticket *__restrict ticket) {
 struct kasyncdata *iter;
 KTASK_CRIT_MARK
 kassert_kaddist(self);
 kassert_kaddistticket(ticket);
 assert(ksignal_islocked(&self->ad_nbdat,KSIGNAL_LOCK_WAIT));
 assert(self->ad_ready <= self->ad_known);
 assert(!self->ad_front || !self->ad_front->sd_prev);
 assert(!self->ad_back || !self->ad_back->sd_next);
 assert(self->ad_chunkc <= self->ad_chunkmax);
#if KDEBUG_HAVE_TRACKEDDDIST
 assertf(ticket->dt_dist == self,"Ticket %p registered to different ddist (%p) than %p",
         ticket,ticket->dt_dist,self);
 if (kaddist_hasticket(self,ticket) == NO) {
  k_syslogf(KLOG_ERROR
           ,"[ADDIST] Ticket at %p was not registered\n"
            "See reference to ticket generation:\n"
           ,ticket);
  dtraceback_print(ticket->dt_mk_tb);
  assertf(0,"Ticket %p not registered",ticket);
 }
 if (kaddist_hasreadyticket(self,ticket) == YES) {
  k_syslogf(KLOG_ERROR
           ,"[ADDIST] Cannot delete ticket in use by a blocking receive operation\n"
            "See reference to ticket generation:\n"
           ,ticket);
  dtraceback_print(ticket->dt_mk_tb);
  k_syslogf(KLOG_ERROR,"See reference to ticket receive:\n");
  dtraceback_print(ticket->dt_rd_tb);
  assertf(0,"Ticket %p used by receive",ticket);
 }
#endif
 /* That this point we can assume the ticket is valid and part
  * of the non-ready list of tickets.
  * Now we must remove it from that list and decref all
  * pending chunks of asynchronous data meant to be read by it. */
 assert(self->ad_known != 0);
 assert(self->ad_known != self->ad_ready);
 assert((ticket->dt_prev != NULL) == (ticket != self->ad_tnready));
 if (ticket == self->ad_tnready) {
  if ((self->ad_tnready = ticket->dt_next) != NULL)
   ticket->dt_next->dt_prev = NULL;
 } else {
  ticket->dt_prev->dt_next = ticket->dt_next;
  if (ticket->dt_next) ticket->dt_next->dt_prev = ticket->dt_prev;
 }
#if KDEBUG_HAVE_TRACKEDDDIST
 ticket->dt_prev = NULL;
 ticket->dt_next = NULL;
#endif
 /* The ticket was removed. - Time to drop all pending chunks of data. */
 if ((iter = ticket->dt_async) != NULL) {
  assert((iter->sd_prev != NULL) == (iter != self->ad_front));
  assert((iter->sd_next != NULL) == (iter != self->ad_back));
  for (;;) {
   if (!--iter->sd_pending) {
    assertf(iter == self->ad_front,"Non-front chunk %p reference count dropped to ZERO",iter);
    assert(!iter->sd_prev);
    assert(self->ad_chunkc);
    /* Update the chunk counter. */
    --self->ad_chunkc;
    self->ad_front = iter->sd_next;
    /* Free the chunk */
    free(iter);
    if (self->ad_front == NULL) {
     /* All chunks have been deallocated. */
     assert(iter == self->ad_back);
     assert(!self->ad_chunkc);
     self->ad_back = NULL;
     break;
    }
    assert(self->ad_chunkc);
    iter = self->ad_front;
    /* Break the dead link to the now deallocated previous chunk. */
    iter->sd_prev = NULL;
   } else {
    assert(iter->sd_next != iter);
    iter = iter->sd_next;
    if (!iter) break;
   }
  }
#if KDEBUG_HAVE_TRACKEDDDIST
  ticket->dt_async = NULL;
#endif
 }
 /* Remove the ticket as being known. */
 --self->ad_known;
 /* Unlock the distributer. */
 ksignal_unlock_c(&self->ad_nbdat,KSIGNAL_LOCK_WAIT);
#if KDEBUG_HAVE_TRACKEDDDIST
 /* Free debug information. */
 ticket->dt_dist = NULL;
 dtraceback_free(ticket->dt_mk_tb);
 dtraceback_free(ticket->dt_rd_tb);
#endif
}


__local __crit kerrno_t
kaddist_vtryrecv_unlocked(struct kaddist *__restrict self,
                          struct kaddistticket *__restrict ticket,
                          void *__restrict buf) {
 struct kasyncdata *reschunk;
 KTASK_CRIT_MARK
 kassert_kaddist(self);
 kassert_kaddistticket(ticket);
 assert(ksignal_islocked(&self->ad_nbdat,KSIGNAL_LOCK_WAIT));
 assert(self->ad_ready <= self->ad_known);
 assert(!self->ad_front || !self->ad_front->sd_prev);
 assert(!self->ad_back || !self->ad_back->sd_next);
 assert(self->ad_chunkc <= self->ad_chunkmax);
 kassertmem(buf,self->ad_chunksz);
 if __unlikely(ISDEAD(self)) return KE_DESTROYED;
#if KDEBUG_HAVE_TRACKEDDDIST
 assertf(ticket->dt_dist == self,"Ticket %p registered to different ddist (%p) than %p",
         ticket,ticket->dt_dist,self);
 if (kaddist_hasticket(self,ticket) == NO) {
  k_syslogf(KLOG_ERROR
           ,"[ADDIST] Ticket at %p was not registered\n"
            "See reference to ticket generation:\n"
           ,ticket);
  dtraceback_print(ticket->dt_mk_tb);
  assertf(0,"Ticket %p not registered",ticket);
 }
 if (kaddist_hasreadyticket(self,ticket) == YES) {
  k_syslogf(KLOG_ERROR
           ,"[ADDIST] Cannot receive ticket in use by a blocking receive operation\n"
            "See reference to ticket generation:\n"
           ,ticket);
  dtraceback_print(ticket->dt_mk_tb);
  k_syslogf(KLOG_ERROR,"See reference to ticket receive:\n");
  dtraceback_print(ticket->dt_rd_tb);
  assertf(0,"Ticket %p used by blocking receive",ticket);
 }
#endif
 if ((reschunk = ticket->dt_async) != NULL) {
  assert(self->ad_chunkc);
  /* Found a pending asynchronous chunk of data. */
  ticket->dt_async = reschunk->sd_next;
  /* Copy the chunk into the result buffer. */
  memcpy(buf,reschunk->sd_data,self->ad_chunksz);
  if (!--reschunk->sd_pending) {
   assert(!reschunk->sd_prev);
   assert(reschunk == self->ad_front);
   /* Last consumer of first chunk. */
   --self->ad_chunkc;
   free(reschunk);
   self->ad_front = reschunk->sd_next;
   if (!self->ad_front) {
    assert(!self->ad_chunkc);
    self->ad_back = NULL;
   } else {
    assert(self->ad_chunkc);
    self->ad_front->sd_prev = NULL;
   }
  }
  return KE_OK;
 }
 return KE_WOULDBLOCK;
}

__crit kerrno_t
_kaddist_vtryrecv_andunlock(struct kaddist *__restrict self,
                            struct kaddistticket *__restrict ticket,
                            void *__restrict buf) {
 kerrno_t error;
 KTASK_CRIT_MARK
 error = kaddist_vtryrecv_unlocked(self,ticket,buf);
 ksignal_unlock_c(&self->ad_nbdat,KSIGNAL_LOCK_WAIT);
 return error;
}

#ifndef __INTELLISENSE__
#define TIMEDRECV
#include "addist-recv.c.inl"
#include "addist-recv.c.inl"
#endif

__crit kerrno_t
_kaddist_vtimeoutrecv_andunlock(struct kaddist *__restrict self,
                                struct kaddistticket *__restrict ticket,
                                struct timespec const *__restrict timeout,
                                void *__restrict buf) {
 KTASK_CRIT_MARK
 struct timespec abstime;
 ktime_getnoworcpu(&abstime);
 __timespec_add(&abstime,timeout);
 return _kaddist_vtimedrecv_andunlock(self,ticket,&abstime,buf);
}


__crit kerrno_t
_kaddist_vsend_andunlock(struct kaddist *__restrict self,
                         void const *__restrict buf) {
 size_t recv_count;
 kerrno_t error;
#ifdef __DEBUG__
 size_t count_not_ready = 0;
#endif
 int can_use_unbuffered;
 struct kasyncdata *reschunk;
 struct kaddistticket *tickets;
 KTASK_CRIT_MARK
 kassert_kaddist(self);
 assert(ksignal_islocked(&self->ad_nbdat,KSIGNAL_LOCK_WAIT));
 assert(self->ad_ready <= self->ad_known);
 assert(!self->ad_front || !self->ad_front->sd_prev);
 assert(!self->ad_back || !self->ad_back->sd_next);
 assert(self->ad_chunkc <= self->ad_chunkmax);
 assert((self->ad_front != NULL) == (self->ad_back != NULL));
 kassertmem(buf,self->ad_chunksz);
 if __unlikely(ISDEAD(self)) { error = KE_DESTROYED; goto err; }
 /* Check how many tasks are ready to receive unbuffered data. */
 recv_count = ksignal_cntrecv_unlocked(&self->ad_nbdat);
 /* Due to a race condition that can arise because of KE_INTR and KE_TIMEDOUT,
  * tickets may still be registered as ready when their associated tasks
  * are in fact no longer scheduled.
  * This is a rare occurrence, but can only be checked for by manually
  * counting the amount of tasks scheduled in the data signal.
  * Situations in which a receiver has no chance to recover in case of
  * termination are impossible due to the fact that the presence of a
  * real critical block is enforced during signal receive, meaning that we
  * can assume the true receiver to always perform its part of the cleanup. */
 /* If all known tasks will actually receive the signal, skip the buffer completely */
 assert(recv_count <= self->ad_ready);
 /* Check for optimized case: Potential for fully unbuffered transmission. */
 if (recv_count == self->ad_known) {
  assert(self->ad_ready == recv_count);
  /* Move the entire ready list to the non-ready stack. */
  if __unlikely((tickets = self->ad_tiready) == NULL) {
   assert(!self->ad_ready);
   assert(!self->ad_known);
   assert(!self->ad_tnready);
   /* Special sub-case: No tickets registered (ignore input). */
   k_syslogf(KLOG_TRACE,"[ADDIST] Not sending data because no tickets are registered\n");
   error = KS_EMPTY;
   goto err;
  }
  assert(self->ad_ready);
  assert(self->ad_known);
  /* Go the the last ready ticket. */
  while ((assert(!tickets->dt_async),tickets->dt_next)
         ) tickets = tickets->dt_next;
  /* Shift the entire list of ready tickets to the non-ready list. */
  if ((tickets->dt_next = self->ad_tnready) != NULL)
   tickets->dt_next->dt_prev = tickets;
  self->ad_tnready = self->ad_tiready;
  self->ad_tiready = NULL;
  self->ad_ready = 0;

  error = KE_OK;
  goto end_unbuffered;
 }
 assert(self->ad_known);
 /* We can only use unbuffered transmission at all when the amount
  * of actually ready tickets equals the amount of tasks scheduled.
  * If these numbers don't match, a race condition has occurred that
  * can only be fixed through use of buffered memory. */
 can_use_unbuffered = (self->ad_ready == recv_count);

 /* Create/Reuse a data chunk for the new packet we are about to post. */
 if (self->ad_chunkc == self->ad_chunkmax) {
  /* Re-use the last chunk.
   * NOTE: We are allowed to overwrite the reference counter without
   *       it dropping to zero due to the special circumstances we are
   *       in right now: All tasks that haven't received this chunk yet
   *       will still be included in the reference counter below,
   *       meaning that it will remain consistent. */
  reschunk = self->ad_back;
  /* TODO: Don't just always use the last chunk. */
  if __unlikely(!reschunk) {
   /* Very special and unlikely case: the distributer isn't allowed to use chunks. */
   assert(!self->ad_chunkc);
   assert(!self->ad_chunkmax);
   if (can_use_unbuffered) { error = KS_FOUND; goto end_unbuffered; }
   error = KE_BUSY;
   goto err;
  }
  k_syslogf(KLOG_WARN,"[ADDIST] Overwriting existing chunk %Iu at %p because buffer is full\n",
            self->ad_chunkc,reschunk);
  error = KS_FULL;
 } else {
  /* Create a new chunk. */
  reschunk = (struct kasyncdata *)malloc(offsetof(struct kasyncdata,sd_data)+
                                         self->ad_chunksz);
  if __unlikely(!reschunk) { error = KE_NOMEM; goto err; }
  ++self->ad_chunkc;
  error = KS_BLOCKING;
  reschunk->sd_next = NULL;
  if ((reschunk->sd_prev = self->ad_back) != NULL) {
   assert(self->ad_chunkc >= 2);
   reschunk->sd_prev->sd_next = reschunk;
  } else {
   assert(self->ad_chunkc == 1);
   assert(!self->ad_front);
   self->ad_front = reschunk;
  }
  self->ad_back = reschunk;
 }

 /* Fill the chunk with memory and configure its reference counter. */
 memcpy(reschunk->sd_data,buf,self->ad_chunksz);

 /* Register the chunk in all non-ready tickets. */
 for (tickets = self->ad_tnready; tickets;
      tickets = tickets->dt_next) {
  if (!tickets->dt_async)
   tickets->dt_async = reschunk;
#ifdef __DEBUG__
  ++count_not_ready;
#endif
 }
#ifdef __DEBUG__
 assert(count_not_ready == self->ad_known-self->ad_ready);
#endif

 if (can_use_unbuffered) {
  assert(recv_count == self->ad_ready);
  /* Set the pending reference counter to the amount of tickets
   * that must receive this chunk through unbuffered means. */
  reschunk->sd_pending = self->ad_known-recv_count;
 } else {
  reschunk->sd_pending = self->ad_known;
  assert((self->ad_ready != 0) == (self->ad_tiready != NULL));
  if ((tickets = self->ad_tiready) != NULL) {
   /* Register the asynchronous chunk in all ready tickets.
    * >> Since we don't know which of the tickets has caused
    *    the race condition that brought us here, we must
    *    handle the situation by posting a buffered chunk to
    *    all ready tickets. */
   for (;;) {
    assert(!tickets->dt_async);
    tickets->dt_async = reschunk;
    if (!tickets->dt_next) break;
    tickets = tickets->dt_next;
   }
   /* Move the entire ready list to the non-ready stack. */
   if ((tickets->dt_next = self->ad_tnready) != NULL)
    tickets->dt_next->dt_prev = tickets;
   self->ad_tnready = self->ad_tiready;
   self->ad_tiready = NULL;
   self->ad_ready = 0;
  }
 }

 if (can_use_unbuffered) {
end_unbuffered:
  asserte(_ksignal_vsendall_andunlock_c(&self->ad_nbdat,buf,self->ad_chunksz) == recv_count);
 } else {
  k_syslogf(KLOG_INFO,
            "[ADDIST] Must use buffer-based data transmission due "
                     "to race condition (%Iu tickets, but %Iu tasks)\n",
            self->ad_ready,recv_count);
  /* Signal all tasks without transmitting data, causing vrecv to return KS_NODATA,
   * which will be handled by another attempt at searching for buffer-based data such
   * as that which we just created. */
  asserte(_ksignal_sendall_andunlock_c(&self->ad_nbdat) == recv_count);
 }
 return error;
err:
 ksignal_unlock_c(&self->ad_nbdat,KSIGNAL_LOCK_WAIT);
 return error;
}


#ifndef __INTELLISENSE__
#undef ISDEAD
#undef MAYBE
#undef NO
#undef YES
#endif

__DECL_END

#endif /* !__KOS_KERNEL_ADDIST_C_INL__ */

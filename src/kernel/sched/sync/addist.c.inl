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
#include <kos/kernel/time.h>
#include <kos/timespec.h>
#include <kos/kernel/signal_v.h>
#include <stddef.h>
#include <kos/syslog.h>

__DECL_BEGIN

#define NO     0
#define YES    1
#define MAYBE  2
#define SIG(self)    (&(self)->add_sigpoll)
#define LOCK(self)   ksignal_lock_c(SIG(self),KSIGNAL_LOCK_WAIT)
#define UNLOCK(self) ksignal_unlock_c(SIG(self),KSIGNAL_LOCK_WAIT)

#ifdef __DEBUG__
static int kaddist_hasuser(struct kaddist const *self,
                           void const *ticket) {
 struct kaddistuser *iter = self->add_userp;
 for (; iter; iter = iter->du_next) {
  if (iter->du_user == ticket) return YES;
 }
 return NO;
}
#else
#define kaddist_hasuser(self,user) MAYBE
#endif

__crit kerrno_t kaddist_adduser(struct kaddist *self,
                                void const *ticket) {
 struct kaddistuser *user;
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kaddist(self);
 user = omalloc(struct kaddistuser);
 if __unlikely(!user) return KE_NOMEM;
 user->du_user = ticket;
 LOCK(self);
 if __unlikely(SIG(self)->s_flags&KSIGNAL_FLAG_DEAD) {
  error = KE_DESTROYED;
  goto end_fail;
 }
 assertf(kaddist_hasuser(self,user->du_user) != YES,
         "Caller %p has already been registered",ticket);
 // Set the user version to what will be send next
 // >> That way, we don't run the risk of the caller
 //    possibly receiving packets send before a call
 //    to this function.
 user->du_aver = self->add_nextver;
 user->du_next = self->add_userp;
 self->add_userp = user;
 if __unlikely(self->add_users == (__u32)-1) {
  error = KE_OVERFLOW;
end_fail:
  UNLOCK(self);
  free(user);
  return error;
 }
 ++self->add_users;
 UNLOCK(self);
 return KE_OK;
}

__crit void kaddist_deluser(struct kaddist *self,
                            void const *ticket) {
 struct kaddistuser *iter,**piter;
 KTASK_CRIT_MARK
 kassert_kaddist(self);
 LOCK(self);
 iter = *(piter = &self->add_userp);
 for (;;) {
  assertf(iter,"Caller %p wasn't registered",ticket);
  if (iter->du_user == ticket) break;
  iter = *(piter = &iter->du_next);
 }
 *piter = iter->du_next;
 assert(self->add_users);
 --self->add_users;
 if (iter->du_aver < self->add_nextver) {
  size_t n_before = iter->du_aver-self->add_currver;
  size_t n_drop = self->add_nextver-iter->du_aver;
  struct kaddistbuf *packet = self->add_async;
  assert(iter->du_aver >= self->add_currver);
  while (n_before--) {
   kassertobj(packet);
   packet = packet->db_next;
  }
  while (n_drop--) {
   if (!--packet->db_left) {
    assert(packet == self->add_async);
    self->add_async = packet->db_next;
    free(packet);
   } else {
    packet = packet->db_next;
   }
  }
 }
 UNLOCK(self);
 free(iter);
}

// Receive data from the ddist buffer without blocking.
// @return: KE_OK:       Data was not received.
// @return: KS_BLOCKING: Data was received.
// NOTE: I know these errors make no sense...
static kerrno_t
kaddist_bufrecv_unlocked(struct kaddist *__restrict self,
                         void *__restrict buf,
                         void const *ticket) {
 struct kaddistuser *user;
 struct kaddistbuf *packet;
 size_t packet_id;
 assertf(self->add_async,"No buffered packets available");
 assertf(self->add_currver != self->add_nextver,
         "No buffered packets available, but buffer pointer is non-NULL");
 user = self->add_userp;
 for (;;) {
  assertf(user,"Caller isn't registered");
#if 0 /* Supposedly ~lost~ packages indicate unbuffered packages. */
  assertf(user->du_aver >= self->add_currver
         ,"Task tries to receive lost packet %I32u (current: %I32u)"
         ,user->du_aver,self->add_currver);
#endif
  assertf(user->du_aver <= self->add_nextver
         ,"Task tries to receive future packet %I32u (latest: %I32u)"
         ,user->du_aver,self->add_nextver);
  if (user->du_user == ticket) break;
  user = user->du_next;
 }
 if (user->du_aver <= self->add_currver) {
  // Calling task must receive the true next packet.
  // (aka. the next unbuffered chunk of data)
  //user->du_aver = self->add_currver;
  return KE_OK;
 }
 // Select the packet that must be received next
 packet_id = user->du_aver-self->add_currver;
 packet = self->add_async;
 while (packet_id--) {
  assertf(packet,"Packet id is out-of-bounds");
  packet = packet->db_next;
 }
 memcpy(buf,packet->db_data,packet->db_size);
 ++user->du_aver;
 // Remove one receiver from the buffered packet.
 // Free and unlink the packet if everyone has received it.
 
 if (!--packet->db_left) {
  assertf(packet == self->add_async,
          "Only the first packet can be dropped, "
          "as receive order would otherwise break.");
  k_syslogf(KLOG_INFO,"[addist] Dropping first packet\n");
  ++self->add_currver;
  self->add_async = packet->db_next;
  free(packet);
 }
 return KS_BLOCKING;
}

kerrno_t kaddist_vrecv(struct kaddist *__restrict self,
                       void *__restrict buf, void const *ticket) {
 kerrno_t error;
 kassert_kaddist(self);
 ksignal_lock(SIG(self),KSIGNAL_LOCK_WAIT);
 assertf(kaddist_hasuser(self,ticket) != NO,
         "Caller %p is not a registered user",ticket);
 if (self->add_async) {
  // Check for receiving buffered data
  error = kaddist_bufrecv_unlocked(self,buf,ticket);
  if (error == KS_BLOCKING) {
   ksignal_unlock_c(SIG(self),KSIGNAL_LOCK_WAIT);
   goto end;
  }
 }
 // No asynchronous data in cache for us, or no data at all.
 // >> Actually receive the unbuffered signal!
 assertf(self->add_ready != self->add_users
        ,"Buf I'm not ready... (%I32u != %I32u)"
        ,self->add_ready,self->add_users);
 // Increment the ready counter to tell a sending task that
 // one more task is ready for unbuffered transfer of data.
 ++self->add_ready;
 error = _ksignal_vrecv_andunlock_c(&self->add_sigpoll,buf);
end:
 ksignal_endlock();
 return error;
}
kerrno_t kaddist_vtryrecv(struct kaddist *__restrict self,
                          void *__restrict buf, void const *ticket) {
 kerrno_t error;
 kassert_kaddist(self);
 ksignal_lock(SIG(self),KSIGNAL_LOCK_WAIT);
 assertf(kaddist_hasuser(self,ticket) != NO,
         "Caller %p is not a registered user",ticket);
 if (self->add_async) {
  // Check for receiving buffered data
  error = kaddist_bufrecv_unlocked(self,buf,ticket);
  if (error != KS_BLOCKING) error = KE_WOULDBLOCK;
 } else {
  error = KE_WOULDBLOCK;
 }
 ksignal_unlock(SIG(self),KSIGNAL_LOCK_WAIT);
 return error;
}

kerrno_t kaddist_vtimedrecv(struct kaddist *__restrict self,
                            struct timespec const *__restrict abstime,
                            void *__restrict buf, void const *ticket) {
 kerrno_t error;
 kassert_kaddist(self);
 ksignal_lock(SIG(self),KSIGNAL_LOCK_WAIT);
 assertf(kaddist_hasuser(self,ticket) != NO,
         "Caller %p is not a registered user",ticket);
 if (self->add_async) {
  // Check for receiving buffered data
  error = kaddist_bufrecv_unlocked(self,buf,ticket);
  if (error == KS_BLOCKING) {
   ksignal_unlock_c(SIG(self),KSIGNAL_LOCK_WAIT);
   goto end;
  }
 }
 // No asynchronous data in cache for us, or no data at all.
 // >> Actually receive the unbuffered signal!
 assert(self->add_ready != self->add_users);
 // Increment the ready counter to tell a sending task that
 // one more task is ready for unbuffered transfer of data.
 ++self->add_ready;
 error = _ksignal_vtimedrecv_andunlock_c(&self->add_sigpoll,abstime,buf);
end:
 ksignal_endlock();
 return error;
}

kerrno_t kaddist_vtimeoutrecv(struct kaddist *__restrict self,
                              struct timespec const *__restrict timeout,
                              void *__restrict buf, void const *ticket) {
 struct timespec abstime;
 ktime_getnoworcpu(&abstime);
 __timespec_add(&abstime,timeout);
 return kaddist_vtimedrecv(self,&abstime,buf,ticket);
}

__crit kerrno_t
_kaddist_vsend_andunlock(struct kaddist *__restrict self,
                         void *__restrict buf, __size_t bufsize) {
 size_t n_woken; kerrno_t error;
 struct kaddistbuf *packet,**ppacket;
#ifdef __DEBUG__
 size_t n_expected = self->add_ready;
 size_t cached_packets = 0;
#endif
 KTASK_CRIT_MARK
 kassert_kaddist(self);
 assert(ksignal_islocked(SIG(self),KSIGNAL_LOCK_WAIT));
 assert(self->add_ready <= self->add_users);
 assert((self->add_ready != 0) == ksignal_hasrecv_unlocked(&self->add_sigpoll));
 if __unlikely(SIG(self)->s_flags&KSIGNAL_FLAG_DEAD) {
  error = KE_DESTROYED;
  goto end_fail;
 }
 if __likely(self->add_ready == self->add_users) {
  // All users are ready to receive unbuffered data.
  // >> This (should) be the most likely case, as the
  //    asynchronous buffer is meant as a fallback in
  //    situations where a regular ddist would have to
  //    use preemption within the calling task.
  self->add_ready = 0; // Consume ready users
  n_woken = _ksignal_vsendall_andunlock_c(&self->add_sigpoll,buf,bufsize);
  // NOTE: vsendall returns 0 if the associated signal was destroyed.
  //       Since destroying a addist effectively sets its user count
  //       to ZERO(0), the special case of a destroyed distributor
  //       can be handled without a special exception.
#ifdef __DEBUG__
  assertf(n_woken == n_expected
         ,"Unexpected amount of task woken (Expected %Iu; Got %Iu)"
         ,n_expected,n_woken);
#endif
  return n_woken ? KE_OK : KS_EMPTY;
 }
#if __SIZEOF_POINTER__ > 4
 if __unlikely((self->add_nextver-self->add_currver) == (__u32)-1) {
  error = KE_OVERFLOW;
  goto end_fail;
 }
#else
 assertf((self->add_nextver-self->add_currver) != (__u32)-1,
         "How is this possible? You should have run out of memory a long time ago!");
#endif
 k_syslogf(KLOG_INFO,
           "[addist] Using packet to transmit %Iu bytes of buffered data to %I32u/%I32u receivers (%I8u)\n",
           bufsize,self->add_users-self->add_ready,self->add_users,*(__u8 *)buf);
 // This is the difficult case: We must
 // create a new buffer packet and post it.
 packet = (struct kaddistbuf *)malloc(offsetof(struct kaddistbuf,db_data)+bufsize);
 if __unlikely(!packet) {
  error = KE_NOMEM;
  goto end_fail;
 }
 packet->db_next = NULL;
 packet->db_size = bufsize;
 memcpy(packet->db_data,buf,bufsize);

 // NOTE: Only count tasks that will not receive the signal
 //       when we send it below to those that are actually waiting.
 packet->db_left = self->add_users-self->add_ready;

 // Append the new packet to the end of the queue
 // NOTE: This case isn't optimized, as it is intended as the fallback.
 ppacket = &self->add_async;
 while (*ppacket) {
  ppacket = &(*ppacket)->db_next;
#ifdef __DEBUG__
  ++cached_packets;
#endif
 }
#ifdef __DEBUG__
 assertf(cached_packets == self->add_nextver-self->add_currver
        ,"Incorrect amount of buffered packets (Expected %Iu; got %Iu)"
        ,self->add_nextver-self->add_currver,cached_packets);
#endif
 *ppacket = packet;
 // Increment the packet version counter
 ++self->add_nextver;
 self->add_ready = 0; // Consume ready users
#ifdef __DEBUG__
 n_woken = _ksignal_vsendall_andunlock_c(&self->add_sigpoll,buf,bufsize);
 assertf(n_woken == n_expected
        ,"Unexpected amount of task woken (Expected %Iu; Got %Iu)"
        ,n_expected,n_woken);
#else
 _ksignal_vsendall_andunlock_c(&self->add_sigpoll,buf,bufsize);
#endif
 return KE_OK;
end_fail:
 ksignal_unlock_c(SIG(self),KSIGNAL_LOCK_WAIT);
 return error;
}


#undef UNLOCK
#undef LOCK
#undef SIG
#undef MAYBE
#undef YES
#undef NO

__DECL_END

#endif /* !__KOS_KERNEL_ADDIST_C_INL__ */

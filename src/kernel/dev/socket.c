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
#ifndef __KOS_KERNEL_DEV_SOCKET_C__
#define __KOS_KERNEL_DEV_SOCKET_C__ 1

#include <kos/compiler.h>
#include <kos/config.h>
#include <kos/kernel/dev/socket.h>
#include <kos/net/ipv4_prot.h>
#include <kos/kernel/debug.h>
#include <kos/kernel/rwlock.h>
#include <sys/types.h>
#include <kos/syslog.h>
#include <kos/kernel/time.h>
#include <kos/timespec.h>
#include <kos/net/if_ether.h>
#include <sys/socket.h>

__DECL_BEGIN
#ifndef __INTELLISENSE__
STATIC_ASSERT(sizeof(struct etherframe) == 1518);
#endif


static void ksockdev_quit(struct ksockdev *self) {
 kassertobj(self);
 assert(!self->d_refcnt);
 kassertbyte(self->sd_cmd);
 kmutex_close(&self->sd_inup);
 kddist_close(&self->sd_distframes);
 if (self->sd_flags&KSOCKDEV_FLAG_ISUP) {
#ifdef __DEBUG__
  // Satisfy assertions in debug mode, if we were called
  // from 
  katomic_incfetch(self->d_refcnt);
#endif
  (*self->sd_cmd)(self,KSOCKDEV_CMD_IFDOWN,NULL);
#ifdef __DEBUG__
  katomic_decfetch(self->d_refcnt);
#endif
 }
}


__ref struct ksockdev *ksockdev_new_unfilled(__size_t devsize) {
 __ref struct ksockdev *result;
 assertf(devsize >= sizeof(struct ksockdev),"Given device size is too small");
 result = (__ref struct ksockdev *)malloc(devsize);
 kdev_init(result);
 result->d_quit = (void(*)(struct kdev *))&ksockdev_quit;
 result->d_setattr = NULL;
 result->d_getattr = NULL;
 result->sd_flags = KSOCKDEV_FLAG_NONE;
 kmutex_init(&result->sd_inup);
 kddist_init(&result->sd_distframes);
#ifdef __DEBUG__
 result->sd_cmd = NULL;
 result->sd_getmac = NULL;
 result->sd_in_delframe = NULL;
 result->sd_out_newframe = NULL;
 result->sd_out_delframe = NULL;
 result->sd_out_sendframe = NULL;
#endif
 memset(result->sd_padding,0,sizeof(result->sd_padding));
 return result;
}

struct kddist *
ksockdev_getddist(struct ksockdev *__restrict self,
                  __be16 etherframe_type) {
 kassert_ksockdev(self);
 (void)etherframe_type;
 /* TODO: Type-specific frame distributors. */
 return NULL;
}

/* TODO: Use type-specific data distributors for these. */
kerrno_t ksockdev_pollframe_t(struct ksockdev *__restrict self, __be16 ether_type,
                              struct ketherframe **__restrict frame) {
 kerrno_t error;
 kassert_ksockdev(self);
 kassertobj(frame);
 for (;;) {
  error = kddist_vrecv(&self->sd_distframes,frame);
  if __unlikely(KE_ISERR(error)) break;
  kassert_ketherframe(*frame);
  kassertobj((*frame)->ef_frame);
  if ((*frame)->ef_frame->ef_type == ether_type ||
      ether_type == ntoh16(ETH_P_ALL)) break;
  ketherframe_decref(*frame);
 }
 return error;
}
kerrno_t ksockdev_timedpollframe_t(struct ksockdev *__restrict self, __be16 ether_type,
                                   struct ketherframe **__restrict frame,
                                   struct timespec const *__restrict abstime) {
 kerrno_t error;
 kassert_ksockdev(self);
 kassertobj(frame);
 kassertobj(abstime);
 for (;;) {
  error = kddist_vtimedrecv(&self->sd_distframes,abstime,frame);
  if __unlikely(KE_ISERR(error)) break;
  kassert_ketherframe(*frame);
  kassertobj((*frame)->ef_frame);
  if ((*frame)->ef_frame->ef_type == ether_type ||
      ether_type == ntoh16(ETH_P_ALL)) break;
  ketherframe_decref(*frame);
 }
 return error;
}
kerrno_t ksockdev_timeoutpollframe_t(struct ksockdev *__restrict self, __be16 ether_type,
                                     struct ketherframe **__restrict frame,
                                     struct timespec const *__restrict timeout) {
 struct timespec abstime;
 ktime_getnow(&abstime);
 __timespec_add(&abstime,timeout);
 return ksockdev_timedpollframe_t(self,ether_type,frame,&abstime);
}



kerrno_t
ksockdev_getmac(struct ksockdev *__restrict self,
                struct macaddr *__restrict result) {
 kerrno_t error;
 kassert_ksockdev(self);
 kassertobj(result);
 kassertbyte(self->sd_getmac);
 if __unlikely(KE_ISERR(error = krwlock_beginread(&self->d_rwlock))) return error;
 error = (*self->sd_getmac)(self,result);
 krwlock_endread(&self->d_rwlock);
 return error;
}

__crit kerrno_t
ksockdev_out_newframe(struct ksockdev *__restrict self,
                      struct etherframe **__restrict frame,
                      size_t min_size) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_ksockdev(self);
 kassertobj(frame);
 kassertbyte(self->sd_out_newframe);
 if __unlikely(KE_ISERR(error = krwlock_beginread(&self->d_rwlock))) return error;
 error = (*self->sd_out_newframe)(self,frame,min_size);
 krwlock_endread(&self->d_rwlock);
 return error;
}

__crit void
ksockdev_out_delframe(struct ksockdev *__restrict self,
                      struct etherframe *__restrict frame,
                      size_t min_size) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_ksockdev(self);
 kassertobj(frame);
 kassertbyte(self->sd_out_delframe);
 if __unlikely(KE_ISERR(error = krwlock_beginread(&self->d_rwlock))) return;
 (*self->sd_out_delframe)(self,frame,min_size);
 krwlock_endread(&self->d_rwlock);
}

__crit kerrno_t
ksockdev_out_sendframe(struct ksockdev *__restrict self,
                       struct etherframe *__restrict frame,
                       size_t frame_size, __u32 flags) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_ksockdev(self);
 kassertobj(frame);
 assert(frame_size >= ETHERFRAME_MINSIZE);
 kassertbyte(self->sd_out_sendframe);
 if __unlikely(KE_ISERR(error = krwlock_beginread(&self->d_rwlock))) return error;
 error = (*self->sd_out_sendframe)(self,frame,frame_size,flags);
 krwlock_endread(&self->d_rwlock);
 return error;
}


__crit __local void
ddist_send_frame(struct kddist *dist,
                 struct ketherframe *f) {
 __u32 old_refcnt,new_refcnt; size_t did_send;
 ssize_t n_receivers = kddist_beginsend_c(dist);
 if __unlikely(KE_ISERR(n_receivers)) return; // Don't distribute anything...
 do {
  old_refcnt = katomic_load(f->ef_refcnt);
  new_refcnt = old_refcnt+n_receivers;
  if (new_refcnt < old_refcnt
#if __SIZEOF_POINTER__ > 4
   || n_receivers >= (size_t)(__u32)-1
#endif
      ) {
   // *Sigh* (TODO: Better error handling?)
   kddist_abortsend_c(dist,(size_t)n_receivers);
   return;
  }
  katomic_fetchadd(f->ef_refcnt,(__u32)n_receivers);
 } while (!katomic_cmpxch(f->ef_refcnt,old_refcnt,new_refcnt));
 // Send out a reference to each receiver.
 did_send = kddist_commitsend_c(dist,(size_t)n_receivers,&f,sizeof(struct ketherframe *));
 if (did_send != n_receivers) {
  if (!katomic_subfetch(f->ef_refcnt,(__u32)((size_t)n_receivers-did_send))) {
   ketherframe_destroy(f);
  }
 }
}

__crit void
ksockdev_in_distframe(struct ksockdev *__restrict self,
                      struct etherframe *__restrict /*inherited*/frame,
                      __size_t frame_size) {
 __ref struct ketherframe *f;
 struct kddist *dist_list;
 KTASK_CRIT_MARK
 kassert_ksockdev(self);
 kassertobj(frame);
 assert(frame_size >= ETHERFRAME_MINSIZE);
 if __unlikely(KE_ISERR(krwlock_beginread(&self->d_rwlock))) goto err;
 if __unlikely((f = ketherframe_new(self,frame,frame_size)) == NULL) goto err2;
 if ((dist_list = ksockdev_getddist(self,frame->ef_type)) != NULL) {
  // Frame-specific data distribution
  ddist_send_frame(dist_list,f);
 }
 // Global data distribution
 ddist_send_frame(&self->sd_distframes,f);
 ketherframe_decref(f);
 krwlock_endread(&self->d_rwlock);
 return;
err2: krwlock_endread(&self->d_rwlock);
err:
 k_syslogf(KLOG_ERROR
      ,"Dropping ethernet frame " MACADDR_PRINTF_FMT " -> "  MACADDR_PRINTF_FMT " (size: %Iu)\n"
      ,frame_size
      ,MACADDR_PRINTF_ARGS(&frame->ef_srcmac)
      ,MACADDR_PRINTF_ARGS(&frame->ef_dstmac));
}

__DECL_END

#ifndef __INTELLISENSE__
#include "socket-allocframe.c.inl"
#include "socket-Ne2000.c.inl"
#endif

#endif /* !__KOS_KERNEL_DEV_SOCKET_C__ */

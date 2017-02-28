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
#ifndef __KOS_KERNEL_SOCKET_PACKET_C_INL__
#define __KOS_KERNEL_SOCKET_PACKET_C_INL__ 1

#include <kos/compiler.h>
#include <kos/config.h>
#include <kos/kernel/socket.h>
#include <kos/kernel/dev/socket.h>
#include <kos/errno.h>
#include <kos/kernel/task.h>
#include <math.h>
#include <string.h>
#include <kos/kernel/proc.h>

__DECL_BEGIN

static void *ksocket_packet_threadmain(struct ksocket *self) {
 struct ksockdev *__restrict dev = self->s_dev;
 struct ketherframe *frame; kerrno_t error;
 KTASK_CRIT_BEGIN
 if __unlikely(KE_ISERR(ksockdev_beginpoll(dev))) goto end;
 /* The pollframe will fail/abort with KE_INTR if a termination was requested */
 while (KE_ISOK(ksockdev_pollframe_t(dev,self->s_pck.sp_proto,&frame))) {
  size_t wsize;
  /* TODO: This can be optimized:
   *   >> If the I/O buffer is full, stop polling frames until
   *      more space becomes available. - That way, we can reduce
   *      the load on the ddist used for spreading frames. */
  error = kiobuf_write(&self->s_iobuf,frame->ef_frame,
                       frame->ef_size,&wsize,
                       KIO_BLOCKNONE|KIO_NONE);
  ketherframe_decref(frame);
  if __unlikely(KE_ISERR(error)) break;
 }
 ksockdev_endpoll(dev);
end:
 KTASK_CRIT_END
 /* We shouldn't normally get here... */
 return NULL;
}

static kerrno_t
ksocket_packet_send(struct ksocket *__restrict self,
                    void const *__restrict buf, size_t bufsize,
                    size_t *__restrict wsize, __u32 flags) {
 struct etherframe *frame;
 kerrno_t error; size_t sendsize;
 if __unlikely(bufsize < ETHERFRAME_MINSIZE) return KE_INVAL;
 KTASK_CRIT_BEGIN
 *wsize = sendsize = min(bufsize,sizeof(struct etherframe));
 error = ksockdev_out_newframe(self->s_dev,&frame,sendsize);
 if __unlikely(KE_ISERR(error)) goto end;
 memcpy(frame,buf,sendsize);
 error = ksockdev_out_sendframe(self->s_dev,frame,sendsize,flags&MSG_DONTWAIT);
 if __unlikely(KE_ISERR(error)) ksockdev_out_delframe(self->s_dev,frame,sendsize);
end:
 KTASK_CRIT_END
 return error;
}
static kerrno_t
ksocket_packet_sendto(struct ksocket *__restrict self,
                      void const *__restrict buf, size_t bufsize,
                      size_t *__restrict wsize, __u32 flags,
                      struct sockaddr const *__restrict __unused(addr),
                      socklen_t __unused(addrlen)) {
 return ksocket_packet_send(self,buf,bufsize,wsize,flags);
}

static kerrno_t
ksocket_packet_recv(struct ksocket *__restrict self,
                    void *__restrict buf, __size_t bufsize,
                    __size_t *__restrict recv_size, __u32 flags) {
 return kiobuf_read(&self->s_iobuf,buf,bufsize,recv_size,
                   (flags&MSG_DONTWAIT ? KIO_BLOCKNONE : KIO_BLOCKALL)|
                   (flags&MSG_PEEK     ? KIO_PEEK : KIO_NONE));
}
static kerrno_t
ksocket_packet_recvfrom(struct ksocket *__restrict self,
                        void *__restrict buf, __size_t bufsize,
                        __size_t *__restrict recv_size, __u32 flags,
                        struct sockaddr *__restrict addr, socklen_t *addrlen) {
 if (addrlen) *addrlen = 0;
 return ksocket_packet_recv(self,buf,bufsize,recv_size,flags);
}

static __crit kerrno_t
ksocket_packet_shutdown(struct ksocket *__restrict self, int how) {
 KTASK_CRIT_MARK
 if (SHUT_ISRD(how)) {
  struct ktask *old_task = katomic_xch(self->s_pck.sp_recvthread,NULL);
  if __unlikely(!old_task) return KE_DESTROYED; /* Already shut down */
  /* Terminate & join the task (use kernel permissions to skip access checks) */
  ktask_terminate_k(old_task,NULL);
  ktask_decref(old_task);
 }
 return KE_OK;
}



static struct ksocket_operations ksocket_pack_ops = {
 .so_send     = &ksocket_packet_send,
 .so_sendto   = &ksocket_packet_sendto,
 .so_recv     = &ksocket_packet_recv,
 .so_recvfrom = &ksocket_packet_recvfrom,
 .so_shutdown = &ksocket_packet_shutdown
};

__crit kerrno_t
ksocket_packet_new_raw(__ref struct ksocket **__restrict result,
                       struct ksockdev *__restrict dev, __be16 sp_proto) {
 __ref struct ksocket *ressock; kerrno_t error;
 KTASK_CRIT_MARK
 ressock = (struct ksocket *)omalloc(struct ksocket_packet);
 if __unlikely(!ressock) return KE_NOMEM;
 kobject_init(ressock,KOBJECT_MAGIC_SOCKET);
 error = kdev_incref((struct kdev *)dev);
 if __unlikely(KE_ISERR(error)) goto err_res;
 ressock->s_refcnt   = 1;
 ressock->s_dev      = dev;
 ressock->s_ops      = &ksocket_pack_ops;
 ressock->s_af       = AF_PACKET;
 ressock->s_type     = SOCK_RAW;
 ressock->s_protocol = (int)sp_proto;
 ressock->s_opflags  = 0;
 kiobuf_init(&ressock->s_iobuf);
 ressock->s_pck.sp_proto = sp_proto;
 /* Create and start the kernel thread used to handle incoming packets.
  * NOTE: We use a fairly small stack (1k) and allocate it using malloc,
  *       in order to keep the memory footprint as small as possible.
  * NOTE: Also make sure to spawn the thread with the thread stack of the
  *       kernel, as it could otherwise be terminated prematurely, or be
  *       accessed in dangerous ways from within user-land. */
 ressock->s_pck.sp_recvthread = ktask_newctxex(ktask_zero(),kproc_kernel(),
                                              (void*(*)(void*))&ksocket_packet_threadmain,
                                               ressock,1024,KTASK_FLAG_MALLKSTACK);
 if __unlikely(!ressock->s_pck.sp_recvthread) { error = KE_NOMEM; goto err_iob; }
 *result = ressock;
 return error;
  err_iob:   kiobuf_quit(&ressock->s_iobuf);
/*err_dev:*/ kdev_decref((struct kdev *)dev);
  err_res:   free(ressock);
 return error;
}

__crit kerrno_t
ksocket_packet_new(__ref struct ksocket **__restrict result,
                   struct ksockdev *__restrict dev, int type, __be16 sp_proto) {
 KTASK_CRIT_MARK
 switch (type) {
  case SOCK_RAW: return ksocket_packet_new_raw(result,dev,sp_proto);
  default: return KE_NOSYS;
 }
}




__DECL_END

#endif /* !__KOS_KERNEL_SOCKET_PACKET_C_INL__ */

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
#ifndef __KOS_KERNEL_SOCKET_C__
#define __KOS_KERNEL_SOCKET_C__ 1

#include <kos/compiler.h>
#include <kos/config.h>
#include <kos/kernel/socket.h>

__DECL_BEGIN


__crit kerrno_t
ksocket_new(__ref struct ksocket **result, struct ksockdev *dev,
            sa_family_t af, int type, int protocol) {
 KTASK_CRIT_MARK
 switch (af) {
  case AF_PACKET: return ksocket_packet_new(result,dev,type,(__be16)protocol);
  default: return KE_NOSYS;
 }
}


__crit void ksocket_destroy(struct ksocket *self) {
 struct ksocket_operations *ops;
 KTASK_CRIT_MARK
 kassert_object(self,KOBJECT_MAGIC_SOCKET);
 assert(!self->s_refcnt);
 ops = self->s_ops; kassertobj(ops);
 if (ops->so_shutdown) {
  kassertbyte(ops->so_shutdown);
#ifdef __DEBUG__
  self->s_refcnt = 1;
#endif
  // Make sure that the socket is fully shut down
  (*ops->so_shutdown)(self,SHUT_RDWR);
#ifdef __DEBUG__
  assertf(self->s_refcnt == 1
          ,"Socket shutdown modified the reference counter (Expected: 1; Got: %I32u)"
          ,self->s_refcnt);
  self->s_refcnt = 0;
#endif
 }
 if (ops->so_quit) (*ops->so_quit)(self);
 kiobuf_quit(&self->s_iobuf);
 kdev_decref((struct kdev *)self->s_dev);
 free(self);
}


kerrno_t
ksocket_send(struct ksocket *__restrict self,
             void const *__restrict buf, __size_t bufsize,
             __size_t *__restrict send_size, __u32 flags) {
 kassert_ksocket(self);
 kassertmem(buf,bufsize);
 kassertobj(send_size);
 if __unlikely(!self->s_ops->so_send) return KE_NOSYS;
 return (*self->s_ops->so_send)(self,buf,bufsize,send_size,
                                flags|self->s_opflags);
}

kerrno_t
ksocket_sendto(struct ksocket *__restrict self,
               void const *__restrict buf, __size_t bufsize,
               __size_t *__restrict send_size, __u32 flags,
               struct sockaddr const *__restrict addr, socklen_t addrlen) {
 kassert_ksocket(self);
 kassertmem(buf,bufsize);
 kassertmemnull(addr,addrlen);
 kassertobj(send_size);
 if __unlikely(!self->s_ops->so_sendto) return KE_NOSYS;
 return (*self->s_ops->so_sendto)(self,buf,bufsize,send_size,
                                  flags|self->s_opflags,
                                  addr,addrlen);
}
kerrno_t
ksocket_recv(struct ksocket *__restrict self,
             void *__restrict buf, __size_t bufsize,
             __size_t *__restrict recv_size, __u32 flags) {
 kassert_ksocket(self);
 kassertmem(buf,bufsize);
 kassertobj(recv_size);
 if __unlikely(!self->s_ops->so_recv) return KE_NOSYS;
 return (*self->s_ops->so_recv)(self,buf,bufsize,recv_size,
                                flags|self->s_opflags);
}
kerrno_t
ksocket_recvfrom(struct ksocket *__restrict self,
                 void *__restrict buf, __size_t bufsize,
                 __size_t *__restrict recv_size, __u32 flags,
                 struct sockaddr *__restrict addr, socklen_t *addrlen) {
 kassert_ksocket(self);
 kassertmem(buf,bufsize);
 kassertobjnull(addrlen);
#if KCONFIG_HAVE_DEBUG_MEMCHECKS
 if (addrlen) kassertmemnull(addr,*addrlen);
#endif
 kassertobj(recv_size);
 if __unlikely(!self->s_ops->so_recvfrom) return KE_NOSYS;
 return (*self->s_ops->so_recvfrom)(self,buf,bufsize,recv_size,
                                    flags|self->s_opflags,
                                    addr,addrlen);
}

__crit kerrno_t
ksocket_shutdown(struct ksocket *__restrict self, int how) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_ksocket(self);
 if (self->s_ops->so_shutdown) {
  error = (*self->s_ops->so_shutdown)(self,how);
  if __unlikely(KE_ISERR(error)) return error;
 }
 if (SHUT_ISRD(how)) {
  error = kiobuf_close(&self->s_iobuf);
  if (error == KS_UNCHANGED) error = KE_DESTROYED;
 } else {
  error = KE_OK;
 }
 return error;
}



__DECL_END

#ifndef __INTELLISENSE__
#include "socket-packet.c.inl"
#endif

#endif /* !__KOS_KERNEL_SOCKET_C__ */

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
#ifndef __KOS_KERNEL_SOCKET_H__
#define __KOS_KERNEL_SOCKET_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/compiler.h>
#include <kos/types.h>
#include <kos/errno.h>
#include <kos/kernel/iobuf.h>
#include <kos/kernel/object.h>
#include <kos/kernel/dev/socket.h>
#include <sys/socket.h>
#include <kos/net/if_ether.h>

__DECL_BEGIN


#define KOBJECT_MAGIC_SOCKET 0x50C5E7 /*< SOCKET. */
#define kassert_ksocket(self) kassert_refobject(self,s_refcnt,KOBJECT_MAGIC_SOCKET)

struct ksocket;

struct ksocket_operations {
 void     (*so_quit)(struct ksocket *__restrict self);
 kerrno_t (*so_send)(struct ksocket *__restrict self,
                     void const *__restrict buf, __size_t bufsize,
                     __size_t *__restrict send_size, __u32 flags);
 kerrno_t (*so_sendto)(struct ksocket *__restrict self,
                       void const *__restrict buf, __size_t bufsize,
                       __size_t *__restrict send_size, __u32 flags,
                       struct sockaddr const *__restrict addr, socklen_t addrlen);
 kerrno_t (*so_recv)(struct ksocket *__restrict self,
                     void *__restrict buf, __size_t bufsize,
                     __size_t *__restrict recv_size, __u32 flags);
 kerrno_t (*so_recvfrom)(struct ksocket *__restrict self,
                         void *__restrict buf, __size_t bufsize,
                         __size_t *__restrict recv_size, __u32 flags,
                         struct sockaddr *__restrict addr, socklen_t *addrlen);
 kerrno_t (*so_shutdown)(struct ksocket *__restrict self, int how);
};

#define KSOCKET_HEAD \
 KOBJECT_HEAD \
 __atomic __u32             s_refcnt;   /*< Reference counter. */\
 __ref struct ksockdev     *s_dev;      /*< [1..1] Associated socket device. */\
 struct ksocket_operations *s_ops;      /*< [1..1] Set of supported socket operations. */\
 /* Parameters, as originally passed to 'ksocket_new()'. */\
 sa_family_t                s_af;       /*< Socket address family. */\
 int                        s_type;     /*< Socket type. */\
 int                        s_protocol; /*< Socket protocol. */\
 __u32                      s_opflags;  /*< Set of flags or-ed onto send/recv flags. */\
 struct kiobuf              s_iobuf;    /*< I/O Buffer for incoming data. */


struct ktask;
struct ksocket_common { KSOCKET_HEAD /*< Common socket header. */ };
struct ksocket_packet {
 KSOCKET_HEAD                       /*< AF_PACKET Socket. */
 __be16              sp_proto;      /*< Used socket protocol (ethhdr::h_proto). */
 __ref struct ktask *sp_recvthread; /*< [1..1][atomic] A thread used to receive ethernet packets (this is what populates s_iobuf). */
};

// Create a socket from 'socket(AF_PACKET,SOCK_RAW,ntoh16(sp_proto))'
extern __crit kerrno_t ksocket_packet_new_raw(__ref struct ksocket **result, struct ksockdev *dev, __be16 sp_proto);
extern __crit kerrno_t ksocket_packet_new(__ref struct ksocket **result, struct ksockdev *dev, int type, __be16 sp_proto);


struct ksocket {
#ifndef __INTELLISENSE__
union{
#endif
 struct { KSOCKET_HEAD };     /*< Common socket members. */
 struct ksocket_common s_com; /*< Any kind of socket. */
 struct ksocket_packet s_pck; /*< AF_PACKET socket. */
#ifndef __INTELLISENSE__
};
#endif
};

__local KOBJECT_DEFINE_INCREF(ksocket_incref,struct ksocket,s_refcnt,kassert_ksocket);
__local KOBJECT_DEFINE_DECREF(ksocket_decref,struct ksocket,s_refcnt,kassert_ksocket,ksocket_destroy);



//////////////////////////////////////////////////////////////////////////
// Create a new socket device, as specified by the given arguments.
// @return: KE_OK:    The new socket was successfully created.
// @return: KE_NOSYS: The given combination of af, type and protocol is not supported/invalid.
// @return: KE_NOMEM: Not enough memory.
extern __crit kerrno_t
ksocket_new(__ref struct ksocket **result, struct ksockdev *dev,
            sa_family_t af, int type, int protocol);

//////////////////////////////////////////////////////////////////////////
// Perform various socket-related operations
// @return: KE_OK:       Operation was successful.
// @return: KE_NOSYS:    Operation is not supported.
// @return: KE_ISERR(*): Operation/socket-specific error occurred.
extern __wunused __nonnull((1,2,4)) kerrno_t
ksocket_send(struct ksocket *__restrict self,
             void const *__restrict buf, __size_t bufsize,
             __size_t *__restrict wsize, __u32 flags);
extern __wunused __nonnull((1,2,4)) kerrno_t
ksocket_sendto(struct ksocket *__restrict self,
               void const *__restrict buf, __size_t bufsize,
               __size_t *__restrict wsize, __u32 flags,
               struct sockaddr const *__restrict addr, socklen_t addrlen);
extern __wunused __nonnull((1,2,4)) kerrno_t
ksocket_recv(struct ksocket *__restrict self,
             void *__restrict buf, __size_t bufsize,
             __size_t *__restrict recv_size, __u32 flags);
extern __wunused __nonnull((1,2,4)) kerrno_t
ksocket_recvfrom(struct ksocket *__restrict self,
                 void *__restrict buf, __size_t bufsize,
                 __size_t *__restrict recv_size, __u32 flags,
                 struct sockaddr *__restrict addr, socklen_t *addrlen);

extern __crit __nonnull((1)) kerrno_t
ksocket_shutdown(struct ksocket *__restrict self, int how);



__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_SOCKET_H__ */

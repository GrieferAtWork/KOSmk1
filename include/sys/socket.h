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
#ifndef __SYS_SOCKET_H__
#define __SYS_SOCKET_H__ 1

#include <kos/compiler.h>
#include <kos/types.h>
#include <kos/__byteswap.h>
#include <kos/endian.h>

__DECL_BEGIN

#ifndef __socklen_t_defined
#define __socklen_t_defined 1
typedef __socklen_t socklen_t;
#endif /* !__socklen_t_defined */

#ifndef __sa_family_t_defined
#define __sa_family_t_defined 1
typedef __sa_family_t sa_family_t;
#endif /* !__sa_family_t_defined */

struct sockaddr {
    sa_family_t sa_family;
    char        sa_data[14];
};

/*
#define AF_UNIX      0
#define AF_LOCAL     AF_UNIX
*/
#define AF_INET      1
/*
#define AF_INET6     2
#define AF_IPX       3
#define AF_NETLINK   4
#define AF_X25       5
#define AF_AX25      6
#define AF_ATMPVC    7
#define AF_APPLETALK 8
*/
#define AF_PACKET    9

#define MSG_OOB          0x00000001 /*< Process out-of-band data. */
#define MSG_PEEK         0x00000002 /*< Peek at incoming messages. */
#define MSG_DONTROUTE    0x00000004 /*< Don't use local routing. */
#define MSG_TRYHARD      MSG_DONTROUTE /*< DECnet uses a different name. */
#define MSG_CTRUNC       0x00000008 /*< Control data lost before delivery. */
#define MSG_PROXY        0x00000010 /*< Supply or ask second address. */
#define MSG_TRUNC        0x00000020
#define MSG_DONTWAIT     0x00000040 /*< Nonblocking IO. */
#define MSG_EOR          0x00000080 /*< End of record. */
#define MSG_WAITALL      0x00000100 /*< Wait for a full request. */
#define MSG_FIN          0x00000200
#define MSG_SYN          0x00000400
#define MSG_CONFIRM      0x00000800 /*< Confirm path validity. */
#define MSG_RST          0x00001000
#define MSG_ERRQUEUE     0x00002000 /*< Fetch message from error queue. */
#define MSG_NOSIGNAL     0x00004000 /*< Do not generate SIGPIPE. */
#define MSG_MORE         0x00008000 /*< Sender will send more. */
#define MSG_WAITFORONE   0x00010000 /*< Wait for at least one packet to return. */
#define MSG_FASTOPEN     0x20000000 /*< Send data in TCP SYN. */
#define MSG_CMSG_CLOEXEC 0x40000000 /*< Set close_on_exit for file descriptor received through SCM_RIGHTS. */


#define SOCK_STREAM    1 /* Sequenced, reliable, connection-based byte streams.  */
#define SOCK_DGRAM     2 /* Connectionless, unreliable datagrams of fixed maximum length.  */
#define SOCK_RAW       3 /* Raw protocol interface.  */
#define SOCK_RDM       4 /* Reliably-delivered messages.  */
#define SOCK_SEQPACKET 5 /* Sequenced, reliable, connection-based, datagrams of fixed maximum length.  */
#define SOCK_DCCP      6 /* Datagram Congestion Control Protocol.  */
#define SOCK_PACKET   10 /* Linux specific way of getting packets at the dev level. For writing rarp and other similar things on the user level. */

#define SHUT_RD   0 /* No more receptions. */
#define SHUT_WR   1 /* No more transmissions. */
#define SHUT_RDWR 2 /* No more receptions or transmissions. */

#define __SHUT_ISRD(x) ((x)!=1)
#define __SHUT_ISWR(x) ((x)>=1)

#ifdef __KERNEL__
#define SHUT_ISRD __SHUT_ISRD
#define SHUT_ISWR __SHUT_ISWR
#endif


/* Flags to be ORed into the type parameter of socket and socketpair and used for the __flags parameter of paccept.  */
#define SOCK_CLOEXEC  02000000 /* Atomically set close-on-exec flag for the new descriptor(s).  */
#define SOCK_NONBLOCK 00004000 /* Atomically mark descriptor(s) as non-blocking.  */


struct msghdr;

extern int socket __P((int __domain, int __type, int __protocol));
extern int socketpair __P((int __domain, int __type, int __protocol, int __fds[2]));
extern int bind __P((int __fd, struct sockaddr const *__addr, socklen_t __len));
extern int getsockname __P((int __fd, struct sockaddr *__addr, socklen_t *__restrict __len));
extern int connect __P((int __fd, struct sockaddr const *__addr, socklen_t __len));
extern int getpeername __P((int __fd, struct sockaddr *__addr, socklen_t *__restrict __len));
extern __ssize_t send __P((int __fd, const void *__restrict __buf, __size_t __n, int __flags));
extern __ssize_t recv __P((int __fd, void *__restrict __buf, __size_t __n, int __flags));
extern __ssize_t sendto __P((int __fd, const void *__restrict __buf, __size_t __n, int __flags, struct sockaddr const *__addr, socklen_t __addr_len));
extern __ssize_t recvfrom __P((int __fd, void *__restrict __buf, __size_t __n, int __flags, struct sockaddr *__addr, socklen_t *__restrict __addr_len));
extern __ssize_t sendmsg __P((int __fd, const struct msghdr *__message, int __flags));
extern __ssize_t recvmsg __P((int __fd, struct msghdr *__message, int __flags));
extern int getsockopt __P((int __fd, int __level, int __optname, void *__restrict __optval, socklen_t *__restrict __optlen));
extern int setsockopt __P((int __fd, int __level, int __optname, const void *__optval, socklen_t __optlen));
extern int listen __P((int __fd, int __n));
extern int accept __P((int __fd, struct sockaddr *__addr, socklen_t *__restrict __addr_len));
extern int accept4 __P((int __fd, struct sockaddr *__addr, socklen_t *__restrict __addr_len, int __flags));
extern int shutdown __P((int __fd, int __how));
extern int sockatmark __P((int __fd));
extern int isfdtype __P((int __fd, int __fdtype));

#ifdef __USE_GNU
extern int sendmmsg __P((int __fd, struct mmsghdr *__vmessages, unsigned int __vlen, int __flags));
extern int recvmmsg __P((int __fd, struct mmsghdr *__vmessages, unsigned int __vlen, int __flags, struct timespec *__tmo));
#endif

#ifndef __KERNEL__
extern __constcall __wunused unsigned short htons __P((unsigned short __x));
extern __constcall __wunused unsigned short ntohs __P((unsigned short __x));
extern __constcall __wunused unsigned long htonl __P((unsigned long __x));
extern __constcall __wunused unsigned long ntohl __P((unsigned long __x));
#endif

// Byte-swap to big-endian
#ifdef __INTELLISENSE__
#   define _hton16 ____INTELLISENE_beswap16
#   define _ntoh16 ____INTELLISENE_beswap16
#   define _hton32 ____INTELLISENE_beswap32
#   define _ntoh32 ____INTELLISENE_beswap32
#   define _hton64 ____INTELLISENE_beswap64
#   define _ntoh64 ____INTELLISENE_beswap64
#elif __BYTE_ORDER == __LITTLE_ENDIAN
#   define _hton16 __kos_bswap16
#   define _ntoh16 __kos_bswap16
#   define _hton32 __kos_bswap32
#   define _ntoh32 __kos_bswap32
#   define _hton64 __kos_bswap64
#   define _ntoh64 __kos_bswap64
#elif __BYTE_ORDER == __BIG_ENDIAN
#   define _hton16 /* nothing */
#   define _ntoh16 /* nothing */
#   define _hton32 /* nothing */
#   define _ntoh32 /* nothing */
#   define _hton64 /* nothing */
#   define _ntoh64 /* nothing */
#endif

#if __SIZEOF_SHORT__ == 2
#   define htons   _hton16
#   define ntohs   _ntoh16
#elif __SIZEOF_SHORT__ == 4
#   define htons   _hton32
#   define ntohs   _ntoh32
#else
#   define htons   __PP_CAT_2(_hton,__PP_MUL8(__SIZEOF_SHORT__))
#   define ntohs   __PP_CAT_2(_ntoh,__PP_MUL8(__SIZEOF_SHORT__))
#endif
#if __SIZEOF_LONG__ == 4
#   define htonl   _hton32
#   define ntohl   _ntoh32
#elif __SIZEOF_LONG__ == 8
#   define htonl   _hton64
#   define ntohl   _ntoh64
#else
#   define htons   __PP_CAT_2(_hton,__PP_MUL8(__SIZEOF_LONG__))
#   define ntohs   __PP_CAT_2(_ntoh,__PP_MUL8(__SIZEOF_LONG__))
#endif

#ifndef __STDC_PURE__
#define hton16  _hton16
#define ntoh16  _ntoh16
#define hton32  _hton32
#define ntoh32  _ntoh32
#define hton64  _hton64
#define ntoh64  _ntoh64
#endif

__DECL_END

#endif /* !__SYS_SOCKET_H__ */

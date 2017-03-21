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
#ifndef __KOS_KERNEL_DEV_SOCKET_H__
#define __KOS_KERNEL_DEV_SOCKET_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/compiler.h>
#include <kos/kernel/ddist.h>
#include <kos/kernel/dev/device.h>
#include <kos/net/ipv4_prot.h>
#include <kos/types.h>
#include <string.h>
#include <sys/socket.h>
#include <kos/kernel/mutex.h>
#include <kos/kernel/debug.h>

__DECL_BEGIN

COMPILER_PACK_PUSH(1)
struct __packed macaddr {
 __u8 ma_bytes[6]; /*< Mac address. */
};
#define MACADDR_PRINTF_FMT "%.2I8x:%.2I8x:%.2I8x:%.2I8x:%.2I8x:%.2I8x"
#define MACADDR_PRINTF_ARGS(self) \
 (self)->ma_bytes[0],(self)->ma_bytes[1],(self)->ma_bytes[2],\
 (self)->ma_bytes[3],(self)->ma_bytes[4],(self)->ma_bytes[5]

// Use broadcast mac addresses when doing initial connections.
#define MACADDR_INIT_BROADCAST  {{0xff,0xff,0xff,0xff,0xff,0xff}}
#define macaddr_init_broadcast(self) memset(self,0xff,6)


struct __packed ipv4_addr {
 __be32 v4_addr; /*< Full IPv4 address. */
};



//////////////////////////////////////////////////////////////////////////
// Layer 4 TCP packet
//////////////////////////////////////////////////////////////////////////
struct __packed tcp_packet {
 __be16       tcp_srcport;       /*< Source port address. */
 __be16       tcp_dstport;       /*< Destination port address. */
 __be32       tcp_seqnum;        /*< Sequence number. */
 __be32       tcp_acknum;        /*< Acknowledgment number (When ACK is set). */
union __packed {
 __be16       tcp_offset_flags;  /*< Data offset and flags. */
#define TCP_FLAG_FIN 0x0001      /*< Last package from sender. */
#define TCP_FLAG_SYN 0x0002      /*< Synchronize sequence numbers. Only the first packet sent from each end should have this flag set.
                                     Some other flags and fields change meaning based on this flag, and some are only valid for when it is set, and others when it is clear. */
#define TCP_FLAG_RES 0x0004      /*< Reset the connection. */
#define TCP_FLAG_PSH 0x0008      /*< Push function. Asks to push the buffered data to the receiving application. */
#define TCP_FLAG_ACK 0x0010      /*< indicates that the Acknowledgment field is significant. All packets after the initial SYN packet sent by the client should have this flag set. */
#define TCP_FLAG_URG 0x0020      /*< indicates that the Urgent pointer field is significant. */
#define TCP_FLAG_ECE 0x0040      /*< ECN-Echo. */
#define TCP_FLAG_CWR 0x0080      /*< Congestion Window Reduced. */
#define TCP_FLAG_NS  0x0100      /*< ECN-nonce concealment protection. */
 __u16        tcp_flags;         /*< TCP Flags (Set of 'TCP_FLAG_*'). */
struct __packed {
 unsigned int tcp_flag_fin: 1;   /*< [TCP_FLAG_FIN]. */
 unsigned int tcp_flag_syn: 1;   /*< [TCP_FLAG_SYN]. */
 unsigned int tcp_flag_res: 1;   /*< [TCP_FLAG_RES]. */
 unsigned int tcp_flag_psh: 1;   /*< [TCP_FLAG_PSH]. */
 unsigned int tcp_flag_ack: 1;   /*< [TCP_FLAG_ACK]. */
 unsigned int tcp_flag_urg: 1;   /*< [TCP_FLAG_URG]. */
 unsigned int tcp_flag_ece: 1;   /*< [TCP_FLAG_ECE]. */
 unsigned int tcp_flag_cwr: 1;   /*< [TCP_FLAG_CWR]. */
 unsigned int tcp_flag_ns:  1;   /*< [TCP_FLAG_NS]. */
 unsigned int tcp_reserved: 3;   /*< Reserved (Set to ZERO(0)). */
 unsigned int tcp_offset: 4;     /*< Data offset (divided by 4; offsetof(tcp_payload) == base+tcp_offset*4). */
};
};
 __be16       tcp_windowsize;    /*< Window size (Number of bytes the sender is willing to receive) (there might be more to this...). */
 __be16       tcp_checksum;      /*< TCP checksum. */
 __be16       tcp_urgent;        /*< Urgent pointer (For TCP_FLAG_URG: index of last urgent data byte). */

 /* More options (as allowed through tcp_offset > 5) would go here */
 __byte_t     tcp_payload[1460]; /*< [6..1460] TCP payload. */
};



//////////////////////////////////////////////////////////////////////////
// Layer 4 UDP packet
//////////////////////////////////////////////////////////////////////////
struct __packed udp_packet {
 __be16   udp_srcport;       /*< Source port address. */
 __be16   udp_dstport;       /*< Destination port address. */
 __be16   udp_length;        /*< Length of the entire datagram (aka. non-fragmented udp size). */
 __be16   udp_checksum;      /*< UDP Checksum. */
 __byte_t udp_payload[1472]; /*< [18..1472] Payload (of a single frame). */
};




//////////////////////////////////////////////////////////////////////////
// Layer 3 IPv4 packet
//////////////////////////////////////////////////////////////////////////
struct __packed ipv4_packet {
union __packed {
 __u8             v4_version_ihl;    /*< Version (bit 0..3) and IHL (bit 4..7). */
struct __packed {
 unsigned int     v4_ihl : 4;        /*< IP Header length (divided by 4; offsetof(v4_payload) == base+v4_ihl*4). */
#define IPv4_VERSION_4  4 /*< IPv4 packet version. */
 unsigned int     v4_version : 4;    /*< IP Version (Set to IPv4_VERSION_4). */
};
};
union __packed {
 __u8             v4_dscp_ecn;       /*< DSCP (bit 0..5) and ECN (bit 6..7). */
struct __packed {
 unsigned int     v4_dscp : 6;       /*< Differentiated Services Code Point. */
 unsigned int     v4_ecn : 2;        /*< Explicit Congestion Notification. */
};
};
 __be16           v4_length;         /*< Total size of the IPv4 datagram (non-fragmented; in bytes; including header+payload). */
 __be16           v4_ident;          /*< Packet identification number (Should be unique for any individual
                                         transmission, and is used to identify fragments of the same datagram). */
union __packed {
 __be16           v4_flags_fragid;   /*< Flags (bit 0..2) and Fragment id (bit 3..15). */
struct __packed {
 unsigned int     __padding : 13;    /*< Padding (it's the fragmentation id). */
 unsigned int     v4_flag_mf : 1;    /*< More fragments (When set, more fragments of the transmission are still to come.
                                         The last fragment has this bit cleared, but a non-zero 'v4_fragid'). */
 unsigned int     v4_flag_df : 1;    /*< Don't fragment (Drop the packet if it would be fragmented during routing). */
 unsigned int     v4_flag_ev : 1;    /*< Evil bit. Must be set for malicious intents, such as when ddos-ing (s.a.: https://tools.ietf.org/html/rfc3514). */
};
struct __packed {
 unsigned int     v4_fragid : 13;    /*< Fragmentation id (index within a fragmented data stream). */
#define IPv4_FLAG_EV 0x01 /*< [v4_flag_ev] Evil bit (Set for malicious intentions). */
#define IPv4_FLAG_DF 0x02 /*< [v4_flag_df] Don't fragment-flag. */
#define IPv4_FLAG_MF 0x04 /*< [v4_flag_mf] More fragments-flag. */
 unsigned int     v4_flags : 3;      /*< Set of 'IPv4_FLAG_*' flags. */
};
};
 __u8             v4_ttl;            /*< Time-to-live (aka. hop counter). Set to a high number for regular packets, every router
                                         the packet passes through decrements this by one, discarding the packet when the counter
                                         reaches ZERO(0), alongside sending a ICMP Time exceeded message to the sender.
                                         Can be used to implemented traceroute. */
 __u8             v4_protocol;       /*< IP Protocol number (as found in <kos/net/ipv4_prot.h>). */
 __be16           v4_checksum;       /*< Checksum for the 10 bytes above. */
 struct ipv4_addr v4_source;         /*< Source IP address. */
 struct ipv4_addr v4_destination;    /*< Destination IP address. */
 // More options (as allowed through v4_ihl > 5) would go here
union{
 struct tcp_packet v4_tcp;           /*< [IPV4_PROT_TCP] TCP packet payload. */
 struct udp_packet v4_udp;           /*< [IPV4_PROT_UDP] UDP packet payload. */
 __byte_t          v4_payload[1480]; /*< [26..1480] IPv4 Payload (within this fragment). */
};
};


//////////////////////////////////////////////////////////////////////////
// Layer 2 Ethernet frame
//////////////////////////////////////////////////////////////////////////
struct __packed etherframe {
 struct macaddr     ef_dstmac;        /*< Destination mac address. */
 struct macaddr     ef_srcmac;        /*< Source mac address. */
 __be16             ef_type;          /*< Frame type (IEEE 802.3 defines this as length). */
union __packed {
 struct ipv4_packet ef_ipv4;          /*< IPv4 packet. */
 __byte_t           ef_payload[1500]; /*< [46..1500] Frame payload. */
};
 __be32             ef_fcs;           /*< CRC checksum (depends on frame type). */
};
#define ETHERFRAME_MINSIZE  46
#define ETHERFRAME_MAXSIZE  1518

// NOTE: Layer 1 packet structures are handled by hardware

COMPILER_PACK_POP


// struct kddist sd_distframes;



#define kassert_ksockdev(self) kassert_kdev(self)
struct ksockdev {
 // WARNING: Socket device implementors should _NOT_ overwrite 'd_quit'.
 //       >> Instead, implement sd_cmd and use IFDOWN for shutting down, freeing
 //          and doing all that would be otherwise be required within 'd_quit'
 KDEV_HEAD
#define KSOCKDEV_FLAG_NONE 0x00000000
#define KSOCKDEV_FLAG_ISUP 0x00000001 /*< Set after a successfully bringing up the socket device. */
 __u32         sd_flags; /*< Set of KSOCKDEV_FLAG_*. */
 struct kmutex sd_inup;  /*< Lock for synchronizing calls to 'sd_cmd' with a 'cmd' of IFUP or IFDOWN. */
 // Used for distribution of frames.
 // This is a REGONLY dist, and is filled from the socket device's interrupt handler.
 struct kddist sd_distframes; /*< Global frame distributor. */
 // All of the below must manually be filled
 // after a call to 'ksockdev_new_unfilled()'
 // Also note, that all following callbacks _MUST_ be non-NULL
 // before safe use of the socket device may commence.

 // Bring the device up, causing interrupts to start happening when frames arrive.
 // The device is responsible for installing an interrupt handler that will
 // broadcast incoming frames to the device's 'sd_distframes' data distributer.
 // NOTE: Frame data is send as 'struct ketherframe *' instances, where for simplification,
 //       the device may use one of the sockdev helper functions provided below.
 // NOTE: Socket implementors may assume that calls IFUP and IFDOWN are already
 //       synchronized and protected by the caller, ensuring that neither is
 //       called twice without the other being called first, as well as neither
 //       being called while the other is still running.
#define KSOCKDEV_CMD_IFUP   1
#define KSOCKDEV_CMD_IFDOWN 2
 kerrno_t (*sd_cmd)(struct ksockdev *__restrict self, int cmd, __user void *arg);
 // Get the MAC address of a socket device.
 kerrno_t (*sd_getmac)(struct ksockdev *__restrict self, struct macaddr *__restrict addr);
 void     (*sd_in_delframe)(struct ksockdev *__restrict self, struct etherframe *__restrict frame, __size_t size);
 // Return a newly allocated ethernet frame for use in output (aka. send) operations.
 kerrno_t (*sd_out_newframe)(struct ksockdev *__restrict self, struct etherframe **__restrict frame, __size_t min_size);
 void     (*sd_out_delframe)(struct ksockdev *__restrict self, struct etherframe *__restrict frame, __size_t min_size);
 // NOTE: After successfully sending a frame using the following operator, the caller may
 //       not attempt to delete is using 'sd_out_delframe', essentially causing 'frame' to
 //       be inherited by the implementor of this operator.
 kerrno_t (*sd_out_sendframe)(struct ksockdev *__restrict self, struct etherframe *__restrict frame, __size_t min_size, __u32 flags);
#define KSOCKDEV_OUT_FLAG_NONE     0x00000000
#define KSOCKDEV_OUT_FLAG_NOBLOCK  MSG_DONTWAIT /*< Don't block (May be ignored by the device). */
 void      *sd_padding[8]; /*< Padding data (initialize to ZERO(0)) */
};

//////////////////////////////////////////////////////////////////////////
// Returns a newly allocated socket device with all
// device-specific callbacks not yet implemented.
// NOTE: Optionally, the caller may also override the following operators:
//       d_getattr, d_setattr 
// @param: devsize: The size of the device (must be at least 'sizeof(struct ksockdev)')
// @return: NULL: Not enough available memory.
extern __ref struct ksockdev *ksockdev_new_unfilled(__size_t devsize);


//////////////////////////////////////////////////////////////////////////
// Query, and return the MAC address associated with a given socket device
// @return: KE_OK:       The MAC address was successfully filled.
// @return: KE_NOSYS:    The operation is not supported by the socket device.
// @return: KE_ISERR(*): Some device-specific error has occurred.
extern __wunused __nonnull((1,2)) kerrno_t
ksockdev_getmac(struct ksockdev *__restrict self,
                struct macaddr *__restrict result);

// Returns the data distributor for the given ethernet frame type.
// NOTE: Returns NULL if no distributor is associated with the given type.
extern __wunused struct kddist *
ksockdev_getddist(struct ksockdev *__restrict self,
                  __be16 etherframe_type);

//////////////////////////////////////////////////////////////////////////
// Allocate a new ethernet frame and store its pointer in '*frame'.
// Upon success, the caller is responsible to fill the memory
// pointed to by '*frame' with meaningful data before passing
// it on to 'ksockdev_postframe' in order to queue the frame for posting.
//  - If the frame shall not be posted, it must be deleted using 'ksockdev_out_delframe'
//  - NOTE: The 'min_size' parameter in delframe must match that used in newframe
// @param: min_size: The minimum size required for the frame (in bytes)
// @return: KE_OK:       The frame was successfully allocated.
// @return: KE_NOMEM:    Not enough memory (RAM).
// @return: KE_NOSPC:    Not enough device resources to allocate a new frame.
// @return: KE_NOSYS:    The operation is not supported by the socket device.
// @return: KE_ISERR(*): Some device-specific error has occurred.
extern __crit __wunused __nonnull((1,2)) kerrno_t
ksockdev_out_newframe(struct ksockdev *__restrict self,
                      struct etherframe **__restrict frame,
                      __size_t min_size);
extern __crit __nonnull((1,2)) void
ksockdev_out_delframe(struct ksockdev *__restrict self,
                      struct etherframe *__restrict frame,
                      __size_t min_size);

//////////////////////////////////////////////////////////////////////////
// Port an ethernet frame to the socket device.
//  - The given frame must have previously been allocated using 'ksockdev_out_newframe',
//    and after a successful call to this function, 'ksockdev_out_delframe' must NOT
//    be invoked to free the frame:
//  >> struct etherframe *frame;
//  >> if (KE_ISERR(ksockdev_out_newframe(dev,&frame))) goto handle_error;
//  >> if (KE_ISERR(fill_frame(frame))) {
//  >>   // Not supposed to send the frame
//  >>   ksockdev_out_delframe(dev,frame);
//  >>   goto handle_error;
//  >> }
//  >> if (KE_ISERR(ksockdev_out_sendframe(dev,frame,KSOCKDEV_FRAMEFLAG_NONE))) {
//  >>   // Failed to send out the frame
//  >>   ksockdev_out_delframe(dev,frame);
//  >>   goto handle_error;
//  >> }
// @param: frame_size: The size of the frame to set (may not be lower than 'ETHERFRAME_MINSIZE')
// @param: flags: A set of 'KSOCKDEV_FRAMEFLAG_*' flags
// @return: KE_OK:       The frame was successfully allocated.
// @return: KE_NOSYS:    The operation is not supported by the socket device.
// @return: KE_ISERR(*): Some device-specific error has occurred.
extern __crit __nonnull((1,2)) kerrno_t
ksockdev_out_sendframe(struct ksockdev *__restrict self,
                       struct etherframe *__restrict frame,
                       __size_t frame_size, __u32 flags);


//////////////////////////////////////////////////////////////////////////
// Distribute the given frame within the socket device.
// NOTE: This function always inherited 'frame'
extern __crit __nonnull((1,2)) void
ksockdev_in_distframe(struct ksockdev *__restrict self,
                      struct etherframe *__restrict /*inherited*/frame,
                      __size_t frame_size);

struct timespec;
struct ketherframe;

#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Poll an ethernet frame from the socket device.
// NOTE: Once the caller is no longer in need of the received frame,
//       they must release it back to the socket device through a
//       call to 'ksockdev_out_delframe'.
// NOTE: The caller must ensure that frames are only polled after
//       a call to 'ksockdev_beginpoll()' and before the associated
//       call to 'ksockdev_endpoll()'
// @param: flags: A set of 'KSOCKDEV_FRAMEFLAG_*' flags
// @return: KE_OK:        The frame was successfully allocated.
// @return: KE_TIMEDOUT:  No frame was received in the given timeout.
// @return: KE_DESTROYED: The device was shut down.
extern __crit __wunused __nonnull((1,2))   kerrno_t ksockdev_pollframe(struct ksockdev *__restrict self, struct ketherframe **__restrict frame);
extern __crit __wunused __nonnull((1,2,3)) kerrno_t ksockdev_timedpollframe(struct ksockdev *__restrict self, struct ketherframe **__restrict frame, struct timespec const *__restrict abstime);
extern __crit __wunused __nonnull((1,2,3)) kerrno_t ksockdev_timeoutpollframe(struct ksockdev *__restrict self, struct ketherframe **__restrict frame, struct timespec const *__restrict timeout);

//////////////////////////////////////////////////////////////////////////
// Begin/End polling frame, thus telling the distributor that loss-less
// distribution of data is required within the calling task.
// WARNING: Don't say you're going to start polling, but then
//          do something stupid like returning to user-land!
//          These functions are meant to be used when implementing
//          a loss-less buffer, which in return is then read from,
//          from a user-land function call such as recv().
//       >> You should really just call this from a kernel thread...
// WARNING: These functions are not recursive within a single thread.
// @return: KE_OK:        Data was successfully received and stored
//                        in the buffer pointed to by 'buf'.
// @return: KE_DESTROYED: The given ddist was destroyed.
// @return: KE_TIMEDOUT:  [kddist_vtimedrecv*|kddist_vtimeoutrecv*] The given timeout has expired
extern __crit __wunused __nonnull((1)) kerrno_t ksockdev_beginpoll(struct ksockdev *__restrict self);
extern __crit __nonnull((1)) void ksockdev_endpoll(struct ksockdev *__restrict self);
#else
#define ksockdev_pollframe(self,frame)                kddist_vrecv(&(self)->sd_distframes,frame)
#define ksockdev_timedpollframe(self,frame,abstime)   kddist_vtimedrecv(&(self)->sd_distframes,frame,abstime)
#define ksockdev_timeoutpollframe(self,frame,timeout) kddist_vtimeoutrecv(&(self)->sd_distframes,frame,timeout)
#define ksockdev_beginpoll(self)                      kddist_adduser(&(self)->sd_distframes)
#define ksockdev_endpoll(self)                        kddist_deluser(&(self)->sd_distframes)
#endif

// These functions are similar to the poll functions above, but only poll frames of a specific type
extern __crit __wunused __nonnull((1,3))   kerrno_t ksockdev_pollframe_t(struct ksockdev *__restrict self, __be16 ether_type, struct ketherframe **__restrict frame);
extern __crit __wunused __nonnull((1,3,4)) kerrno_t ksockdev_timedpollframe_t(struct ksockdev *__restrict self, __be16 ether_type, struct ketherframe **__restrict frame, struct timespec const *__restrict abstime);
extern __crit __wunused __nonnull((1,3,4)) kerrno_t ksockdev_timeoutpollframe_t(struct ksockdev *__restrict self, __be16 ether_type, struct ketherframe **__restrict frame, struct timespec const *__restrict timeout);




#define KOBJECT_MAGIC_ETHERFRAME 0xE7EFAE /*< ETEFAE */
#define kassert_ketherframe(self) kassert_object(self,KOBJECT_MAGIC_ETHERFRAME)
struct ketherframe {
 // Reference counted controller object for incoming(IN) ethernet frames.
 // >> These objects are extremely short-lived and usually originate
 //    from a self-increasing buffer of them.
 // >> They are required to allow asynchronous processing of ethernet
 //    frames by any number of services using them.
 //    They are also used to track data within AF_PACKET sockets.
 KOBJECT_HEAD
 __atomic __u32         ef_refcnt; /*< Frame reference counter. */
 __ref struct ksockdev *ef_dev;    /*< [1..1] Device associated with this frame. */
 struct etherframe     *ef_frame;  /*< [1..1] Actual, physical frame. */
 __size_t               ef_size;   /*< Size of the frame. */
};
// Internal/global allocator for ethernet frame controllers.
extern __crit struct ketherframe *ketherframe_alloc(void);
extern __crit void ketherframe_free(struct ketherframe *ob);
extern __crit void ketherframe_clearcache(void);

__local __crit void ketherframe_destroy(struct ketherframe *ob) {
 kassertbyte(ob->ef_dev->sd_in_delframe);
 (*ob->ef_dev->sd_in_delframe)(ob->ef_dev,ob->ef_frame,ob->ef_size);
 kdev_decref((struct kdev *)ob->ef_dev);
 ketherframe_free(ob);
}
__local __crit __ref struct ketherframe *
ketherframe_new(struct ksockdev *__restrict dev,
                struct etherframe *frame,
                __size_t frame_size) {
 __ref struct ketherframe *result;
 kassert_ksockdev(dev);
 if __unlikely(KE_ISERR(kdev_incref((struct kdev *)dev))) return NULL;
 if __unlikely((result = ketherframe_alloc()) == NULL) return NULL;
 kobject_init(result,KOBJECT_MAGIC_ETHERFRAME);
 result->ef_refcnt = 1;
 result->ef_dev    = dev;
 result->ef_frame  = frame;
 result->ef_size   = frame_size;
 return result;
}

__local KOBJECT_DEFINE_INCREF(ketherframe_incref,struct ketherframe,ef_refcnt,kassert_ketherframe);
__local KOBJECT_DEFINE_DECREF(ketherframe_decref,struct ketherframe,ef_refcnt,kassert_ketherframe,ketherframe_destroy);





__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_DEV_SOCKET_H__ */

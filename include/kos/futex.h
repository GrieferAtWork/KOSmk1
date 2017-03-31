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
#ifndef __KOS_FUTEX_H__
#define __KOS_FUTEX_H__ 1

#include <kos/config.h>
#include <kos/syscall.h>
#include <kos/errno.h>
#include <kos/types.h>

__DECL_BEGIN

/* Operation codes for 'KFUTEX_RECVIF*' */
#  define KFUTEX_OPMASK         0xf
#  define KFUTEX_NOT            0x1
#  define KFUTEX_FALSE          0 /* Intentionally ZERO(0) (aka. 'false') */
#  define KFUTEX_TRUE           1 /* Intentionally ONE(1) (aka. 'true') */
#  define KFUTEX_EQUAL          0x2 /* 'if (*uaddr == val) { ... wait(); }' */
#  define KFUTEX_LOWER          0x4 /* 'if (*uaddr < val) { ... wait(); }' */
#  define KFUTEX_LOWER_EQUAL    0x6 /* 'if (*uaddr <= val) { ... wait(); }' */
#  define KFUTEX_AND            0x8 /* 'if ((*uaddr & val) != 0) { ... wait(); }' */
#  define KFUTEX_XOR            0xA /* 'if ((*uaddr ^ val) != 0) { ... wait(); }' */
#  define KFUTEX_SHL            0xC /* 'if ((*uaddr << val) != 0) { ... wait(); }' */
#  define KFUTEX_SHR            0xE /* 'if ((*uaddr >> val) != 0) { ... wait(); }' */
#  define KFUTEX_NOT_EQUAL      0x3 /* 'if (*uaddr != val) { ... wait(); }' */
#  define KFUTEX_GREATER_EQUAL  0x5 /* 'if (*uaddr >= val) { ... wait(); }' */
#  define KFUTEX_GREATER        0x7 /* 'if (*uaddr > val) { ... wait(); }' */
#  define KFUTEX_NOT_AND        0x9 /* 'if ((*uaddr & val) == 0) { ... wait(); }' */
#  define KFUTEX_NOT_XOR        0xB /* 'if ((*uaddr ^ val) == 0) { ... wait(); }' */
#  define KFUTEX_NOT_SHL        0xD /* 'if ((*uaddr << val) == 0) { ... wait(); }' */
#  define KFUTEX_NOT_SHR        0xF /* 'if ((*uaddr >> val) == 0) { ... wait(); }' */

/* Operation codes for 'KFUTEX_CCMD*'
 * NOTE: When 'uaddr2 == uaddr', these will be applied with atomic_compare_exchange. */
#  define KFUTEX_STORE   0x00 /* '*uaddr2 = val2'. */
#  define KFUTEX_ADD     0x10 /* '*uaddr2 += val2' */
#  define KFUTEX_SUB     0x20 /* '*uaddr2 -= val2' */
#  define KFUTEX_MUL     0x30 /* '*uaddr2 *= val2' */
#  define KFUTEX_DIV     0x40 /* '*uaddr2 /= val2' */
//#define KFUTEX_AND     0x08 /* '*uaddr2 &= val2' */
#  define KFUTEX_OR      0x50 /* '*uaddr2 |= val2' */
//#define KFUTEX_XOR     0x0A /* '*uaddr2 ^= val2' */
//#define KFUTEX_SHL     0x0C /* '*uaddr2 <<= val2' */
//#define KFUTEX_SHR     0x0E /* '*uaddr2 >>= val2' */
//#define KFUTEX_NOT     0x01 /* '*uaddr2 = ~(val2)' */
#  define KFUTEX_NOT_ADD 0x11 /* '*uaddr2 = ~(*uaddr2 + val2)' */
#  define KFUTEX_NOT_SUB 0x21 /* '*uaddr2 = ~(*uaddr2 - val2)' */
#  define KFUTEX_NOT_MUL 0x31 /* '*uaddr2 = ~(*uaddr2 * val2)' */
#  define KFUTEX_NOT_DIV 0x41 /* '*uaddr2 = ~(*uaddr2 / val2)' */
//#define KFUTEX_NOT_AND 0x09 /* '*uaddr2 = ~(*uaddr2 & val2)' */
#  define KFUTEX_NOT_OR  0x51 /* '*uaddr2 = ~(*uaddr2 | val2)' */
//#define KFUTEX_NOT_XOR 0x0B /* '*uaddr2 = ~(*uaddr2 ^ val2)' */
//#define KFUTEX_NOT_SHL 0x0D /* '*uaddr2 = ~(*uaddr2 << val2)' */
//#define KFUTEX_NOT_SHR 0x0F /* '*uaddr2 = ~(*uaddr2 >> val2)' */

/* Command bit flags. */
#define _KFUTEX_MASK_CMD     0x0000000f
#define _KFUTEX_MASK_FLAGS   0xff000000
#define _KFUTEX_RECV_OPSHIFT 20
#define _KFUTEX_RECV_OPMASK  0x00f00000
#define _KFUTEX_CCMD_OPSHIFT 24
#define _KFUTEX_CCMD_OPMASK  0xff000000

#define _KFUTEX_CMD_RECV    0x0 /* Receive a given futex object. */
#define _KFUTEX_CMD_SEND    0x1 /* Send a given futex object. */
#define _KFUTEX_CMD_C0      0x2 /* '_KFUTEX_CMD_RECV'+conditional-command. */
#define _KFUTEX_CMD_C1      0x3 /* '_KFUTEX_CMD_RECV'+conditional-command+secondary send_one. */
#define _KFUTEX_CMD_CX      0x4 /* '_KFUTEX_CMD_RECV'+conditional-command+secondary send_all. */


/* Atomically check '*uaddr [op] val' and start waiting if the expression is true (non-ZERO).
 * NOTE: This check is performed before a secondary operation may be executed,
 *       which will not be executed at all if the expression fails. */
#define KFUTEX_RECVIF(op)       (_KFUTEX_CMD_RECV|((op)&0xf) << _KFUTEX_RECV_OPSHIFT)

/* Secondary operations for 'KFUTEX_RECVIF' that can be or'd to it.
 * WARNING: When 'uaddr2 == uaddr', only 'KFUTEX_CCMD()' may be used.
 *       >> It is not possible to atomically send and receive the same futex! */
#define KFUTEX_CCMD(op)         (_KFUTEX_CMD_C0|((op)&0xff) << _KFUTEX_CCMD_OPSHIFT) /*< { *uaddr2 = *uaddr2 [op] val2; } */
#define KFUTEX_CCMD_SENDONE(op) (_KFUTEX_CMD_C1|((op)&0xff) << _KFUTEX_CCMD_OPSHIFT) /*< { *uaddr2 = *uaddr2 [op] val2; send_one(uaddr2); } */
#define KFUTEX_CCMD_SENDALL(op) (_KFUTEX_CMD_CX|((op)&0xff) << _KFUTEX_CCMD_OPSHIFT) /*< { *uaddr2 = *uaddr2 [op] val2; send_all(uaddr2); } */

/* Wake up to 'val' threads waiting at 'uaddr'.
 * Upon success, ECX is set to the amount of woken threads. */
#define KFUTEX_SEND        _KFUTEX_CMD_SEND


#ifndef __NO_PROTOTYPES
#ifndef __ASSEMBLY__
struct timespec;

typedef __attribute__((__aligned__(__SIZEOF_INT__))) volatile unsigned int kfutex_t;

struct __packed kfutexset {
 struct kfutexset *fs_next;  /*< [0..1] Pointer to another set of futex objects, or NULL. */
 unsigned int      fs_op;    /*< The futex operation that should be applied to those in the vector below. */
 unsigned int      fs_val;   /*< The value used as right-hand-side operand in some recv operations as set by 'fs_op'. */
 __size_t          fs_count; /*< Amount of futex objects, either inline, or dereferenced. */
 __size_t          fs_align; /*< Alignment of futex addresses, or < sizeof(kfutex_t) for a packed vector of pointers that should be dereferenced. */
union __packed {
 kfutex_t         *fs_avec;  /*< [if(fs_align >= sizeof(kfutex_t))][align(fs_align)][0..fs_count]
                              *   Vector of inline futex objects, each offset from each pother by 'fs_align'.
                              *   >> AT(int i) { return (kfutex_t *)((uintptr_t)fs_avec+i*fs_align); } */
 kfutex_t        **fs_dvec;  /*< [if(fs_align <  sizeof(kfutex_t))][1..1][0..fs_count]
                              *   Packed vector of futex addresses.
                              *   >> AT(int i) { return fs_dvec[i]; } */
};};


//////////////////////////////////////////////////////////////////////////
// Perform a futex operation:
// @return: KE_OK:       [*]           The operation completed successfully.
// @return: KE_FAULT:    [*]           The given 'uaddr' is faulty.
// @return: KE_FAULT:    [KFUTEX_WAIT] The given 'abstime' pointer is faulty and non-NULL.
// @return: KE_AGAIN:    [KFUTEX_WAIT] The 'KFUTEX_RECVIF' condition was false.
// @return: KE_TIMEDOUT: [KFUTEX_WAIT] The given 'abstime' or a previously set timeout has expired.
// @return: KE_NOMEM:    [KFUTEX_WAIT] No futex existed at the given address, and not enough memory was available to allocate one.
// @return: KE_INVAL:    [?]           The specified 'futex_op' is unknown.
// @return: KS_NODATA:   [KFUTEX_WAIT && buf != NULL] The futex was signaled, but no data was transmitted.
// @return: KE_INVAL:    [kfutex_ccmd][KFUTEX_WAIT][KFUTEX_CCMD_SENDALL|KFUTEX_CCMD_SENDONE]
//                       [uaddr2 == uaddr] The specified second futex is equal to the first,
//                                         but the caller attempted to send & receive a single
//                                         futex in the same atomic frame (which isn't possible).
// @return: KE_FAULT:    [KFUTEX_SEND] The given 'buf' pointer, or that of a receiver was faulty.
// @return: KS_EMPTY:    [KFUTEX_SEND] No threads were woken (ECX is set to ZERO(0))
// NOTE: Internally, 'kfutex_cmd' is an alias for 'kfutex_ccmd|uaddr2 = uaddr|val2 = val'
__local _syscall5(kerrno_t,kfutex_cmd,
                  kfutex_t *,uaddr,unsigned int,futex_op,
                  unsigned int,val,void *,buf,struct timespec const *,abstime);
__local _syscall7(kerrno_t,kfutex_ccmd, /* kfutex_c[onditional]cmd */
                  kfutex_t *,uaddr,unsigned int,futex_op,
                  unsigned int,val,void *,buf,struct timespec const *,abstime,
                  kfutex_t *,uaddr2,unsigned int,val2);

//////////////////////////////////////////////////////////////////////////
// Similar to 'kfutex_ccmd', but wait for multiple signals at once,
// atomically performing the atomic checks specified in all chained signal
// sets before starting to wait.
// NOTES:
//  - A secondary operation that should be performed on 'uaddr2'
//    must be specified in the first link of the 'ftxset' chain.
//    Secondary operations in any other links are silently ignored.
//  - In the event of KE_AGAIN, you must figure out which ftx
//    caused a check to fail, just as you have to when figuring
//    out which one caused you to wake afterwards.
// WARNING:
//  - This function can _only_ be used to receive futex objects.
//    If the futex operation specified in any of the 
// Example for receiving multiple futex objects:
// >> static kfutex_t ftxv[][2] = {{0,0},{0,0}};
// >> static kfutex_t *
// >> 
// >> static void *thread_main(void *p) {
// >>   char buf[256];
// >>   
// >>   printf("data from %p: %.256q\n",buf);
// >>   return p;
// >> }
// @return: * :       Same as 'kfutex_cmd'.
// @return: KE_INVAL: The secondary futex operation attempted to send 'uaddr2' which is already contained in 'ftxset'.
// @return: KE_INVAL: A primary futex operations attempted to send a signal (This function can only be used to receive futex objects; potentially multiple at once).
// @return: KE_OK:   'ftxset' is NULL and nothing needed to be done.
__local _syscall5(kerrno_t,kfutex_ccmds, /* kfutex_c[onditional]cmds (plural) */
                  struct kfutexset *,ftxset,void *,buf,
                  struct timespec const *,abstime,
                  kfutex_t *,uaddr2,unsigned int,val2);

/* Helper macros for easy futex signaling. */
#define kfutex_sendn(uaddr,max_receivers)             kfutex_cmd(uaddr,KFUTEX_SEND,max_receivers,NULL,NULL)
#define kfutex_sendone(uaddr)                         kfutex_sendn(uaddr,1)
#define kfutex_sendall(uaddr)                         kfutex_sendn(uaddr,(unsigned int)-1)
#define kfutex_vsend(uaddr,max_receivers,buf,bufsize) kfutex_cmd(uaddr,KFUTEX_SEND,max_receivers,(void *)(buf),(struct timespec const *)(__size_t)(bufsize))
#define kfutex_vsendone(uaddr,buf,bufsize)            kfutex_vsend(uaddr,1,buf,bufsize)
#define kfutex_vsendall(uaddr,buf,bufsize)            kfutex_vsend(uaddr,(unsigned int)-1,buf,bufsize)

/* NOTE: There are no receiver helpers because 'kfutex_{c}cmd'
 *       are already as simplified as possible.
 *       The next higher level would already be something like:
 * >> #define kfutex_recv_if_notequal(uaddr,val)                   kfutex_cmd(uaddr,KFUTEX_RECVIF(KFUTEX_NOT_EQUAL),val,NULL,NULL)
 * >> #define kfutex_vrecv_if_notequal(uaddr,val,buf)              kfutex_cmd(uaddr,KFUTEX_RECVIF(KFUTEX_NOT_EQUAL),val,buf,NULL)
 * >> #define kfutex_timedrecv_if_notequal(uaddr,val,abstime)      kfutex_cmd(uaddr,KFUTEX_RECVIF(KFUTEX_NOT_EQUAL),val,NULL,abstime)
 * >> #define kfutex_vtimedrecv_if_notequal(uaddr,val,buf,abstime) kfutex_cmd(uaddr,KFUTEX_RECVIF(KFUTEX_NOT_EQUAL),val,buf,abstime)
 */


#endif /* !__ASSEMBLY__ */
#endif /* !__NO_PROTOTYPES */


__DECL_END

#endif /* !__KOS_FUTEX_H__ */

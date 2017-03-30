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
#define KFUTEX_OPMASK         0xf
#define KFUTEX_NOT            0x1
#define KFUTEX_FALSE          0 /* Intentionally ZERO(0) (aka. 'false') */
#define KFUTEX_TRUE           1 /* Intentionally ONE(1) (aka. 'true') */
#define KFUTEX_EQUAL          0x2
#define KFUTEX_LOWER          0x4
#define KFUTEX_LOWER_EQUAL    0x6
#define KFUTEX_AND            0x8
#define KFUTEX_XOR            0xA
#define KFUTEX_SHL            0xC
#define KFUTEX_SHR            0xE
#define KFUTEX_NOT_EQUAL     (KFUTEX_NOT|KFUTEX_EQUAL)
#define KFUTEX_GREATER_EQUAL (KFUTEX_NOT|KFUTEX_LOWER)
#define KFUTEX_GREATER       (KFUTEX_NOT|KFUTEX_LOWER_EQUAL)
#define KFUTEX_NOT_AND       (KFUTEX_NOT|KFUTEX_AND)
#define KFUTEX_NOT_XOR       (KFUTEX_NOT|KFUTEX_XOR)

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
#  define KFUTEX_NOT_SHL 0x0D /* '*uaddr2 = ~(*uaddr2 << val2)' */
#  define KFUTEX_NOT_SHR 0x0F /* '*uaddr2 = ~(*uaddr2 >> val2)' */

/* Command bit flags. */
#define _KFUTEX_MASK_CMD     0x0000000f
#define _KFUTEX_MASK_FLAGS   0xff000000
#define _KFUTEX_RECV_OPSHIFT 20
#define _KFUTEX_RECV_OPMASK  0x00f00000
#define _KFUTEX_CCMD_OPSHIFT 24
#define _KFUTEX_CCMD_OPMASK  0xff000000

#define _KFUTEX_CMD_RECV    0x0 /* Wait for a set of futex objects. (NOTE: Accepts secondary commands) */
#define _KFUTEX_CMD_SEND    0x1 /* Wake a set of futex objects. */
#define _KFUTEX_CMD_C0      0x2 /* '_KFUTEX_CMD_RECV'+secondary command. */
#define _KFUTEX_CMD_C1      0x3 /* '_KFUTEX_CMD_RECV'+secondary command+secondary send_one. */
#define _KFUTEX_CMD_CX      0x4 /* '_KFUTEX_CMD_RECV'+secondary command+secondary send_all. */


/* Atomically check '(*uaddr [op] val) != 0' and start waiting if the expression is true.
 * NOTE: This check is performed before a secondary operation may be executed. */
#define KFUTEX_RECVIF(op)       (_KFUTEX_CMD_RECV|((op)&0xf) << _KFUTEX_RECV_OPSHIFT)

/* Secondary operations for 'KFUTEX_RECVIF' that can be or'd to it.
 * WARNING: When 'uaddr2 == uaddr', only 'KFUTEX_CCMD()' may be used.
 *       >> It is not possible to atomically send and receive the same futex! */
#define KFUTEX_CCMD(op)         (_KFUTEX_CMD_C0|((op)&0xff) << _KFUTEX_CCMD_OPSHIFT) /*< { *uaddr2 = *uaddr2 [op] val2; } */
#define KFUTEX_CCMD_SENDALL(op) (_KFUTEX_CMD_C1|((op)&0xff) << _KFUTEX_CCMD_OPSHIFT) /*< { *uaddr2 = *uaddr2 [op] val2; send_all(uaddr2); } */
#define KFUTEX_CCMD_SENDONE(op) (_KFUTEX_CMD_CX|((op)&0xff) << _KFUTEX_CCMD_OPSHIFT) /*< { *uaddr2 = *uaddr2 [op] val2; send_one(uaddr2); } */

/* Wake up to 'val' threads waiting at 'uaddr'.
 * Upon success, ECX is set to the amount of woken threads. */
#define KFUTEX_SEND        _KFUTEX_CMD_SEND


#ifndef __NO_PROTOTYPES
#ifndef __ASSEMBLY__
struct timespec;

typedef __attribute__((__aligned__(__SIZEOF_INT__))) volatile unsigned int kfutex_t;

//////////////////////////////////////////////////////////////////////////
// Perform a futex operation:
// @return: KE_OK:       [*]           The operation completed successfully.
// @return: KE_FAULT:    [*]           The given 'uaddr' is faulty.
// @return: KE_FAULT:    [KFUTEX_WAIT] The given 'abstime' pointer is faulty and non-NULL.
// @return: KE_AGAIN:    [KFUTEX_WAIT] The value at '*uaddr' was equal to 'val' (the function didn't block)
// @return: KE_TIMEDOUT: [KFUTEX_WAIT] The given 'abstime' or a previously set timeout has expired.
// @return: KE_FAULT:    [KFUTEX_SEND] The given 'buf' pointer, or that of a receiver was faulty.
// @return: KE_NOMEM:    [*]           No futex existed at the given address, and not enough memory was available to allocate one.
// @return: KE_INVAL:    [?]           The specified 'futex_op' is unknown.
// @return: KS_NODATA:   [KFUTEX_WAIT && buf != NULL] The futex was signaled, but no data was transmitted.
// @return: KS_EMPTY:    [KFUTEX_SEND] No threads were woken (ECX is set to ZERO(0))
__local _syscall5(kerrno_t,kfutex_cmd,
                  kfutex_t *,uaddr,unsigned int,futex_op,
                  unsigned int,val,void *,buf,struct timespec *,abstime);
__local _syscall7(kerrno_t,kfutex_ccmd,
                  kfutex_t *,uaddr,unsigned int,futex_op,
                  unsigned int,val,void *,buf,struct timespec *,abstime,
                  kfutex_t *,uaddr2,unsigned int,val2);


#endif /* !__ASSEMBLY__ */
#endif /* !__NO_PROTOTYPES */


__DECL_END

#endif /* !__KOS_FUTEX_H__ */

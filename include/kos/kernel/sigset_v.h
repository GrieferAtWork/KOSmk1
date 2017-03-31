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
#ifndef __KOS_KERNEL_SIGSET_V_H__
#define __KOS_KERNEL_SIGSET_V_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/types.h>
#include <kos/kernel/sigset.h>
#include <kos/kernel/signal_v.h>
#include <kos/kernel/debug.h>

__DECL_BEGIN

#ifndef __ASSEMBLY__

#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Waits for one of a given set of signals to be emitted.
// NOTE: If you are wondering why there is no 'ksigset_trywaits', think again,
//       because you obviously don't understand what signals are supposed to be.
//       >> They don't have a state. - You can't check if a signal was send.
//          The only thing you can do, is wait until the signal is send by
//          someone, or something, in which case you will be woken up.
//         (Or you can wait for that signal with an optional timeout)
// @return: KE_OK:         A signal was successfully received.
// @return: KE_DESTROYED:  A signal was/has-been killed.
// @return: KE_TIMEDOUT:   [ksigset_(timed|timeout)recvs] The given timeout has passed.
// @return: KE_INTR:       The calling task was interrupted.
extern           __nonnull((1))   kerrno_t ksigset_vrecvs(struct ksigset const *__restrict self, __kernel void *__restrict buf);
extern __wunused __nonnull((1,2)) kerrno_t ksigset_vtimedrecvs(struct ksigset const *__restrict self, struct timespec const *__restrict abstime, __kernel void *__restrict buf);
extern __wunused __nonnull((1,2)) kerrno_t ksigset_vtimeoutrecvs(struct ksigset const *__restrict self, struct timespec const *__restrict timeout, __kernel void *__restrict buf);
extern __crit __nonnull((1)) kerrno_t _ksigset_vrecvs_andunlock_c(struct ksigset const *__restrict self, __kernel void *__restrict buf);
extern __crit __wunused __nonnull((1,2)) kerrno_t _ksigset_vtimedrecvs_andunlock_c(struct ksigset const *__restrict self, struct timespec const *__restrict abstime, __kernel void *__restrict buf);
extern __crit __wunused __nonnull((1,2)) kerrno_t _ksigset_vtimeoutrecvs_andunlock_c(struct ksigset const *__restrict self, struct timespec const *__restrict timeout, __kernel void *__restrict buf);
#else
#define ksigset_vrecvs(sigset,buf) __KSIGNAL_VRECV(buf,ksigset_recvs(sigset))
#define ksigset_vtimedrecvs(sigset,abstime,buf) __KSIGNAL_VRECV(buf,ksigset_timedrecvs(sigset,abstime))
#define ksigset_vtimeoutrecvs(sigset,timeout,buf) __KSIGNAL_VRECV(buf,ksigset_timeoutrecvs(sigset,timeout))
#define _ksigset_vrecvs_andunlock_c(self,buf) __KSIGNAL_VRECV(buf,_ksigset_recvs_andunlock_c(self))
#define _ksigset_vtimedrecvs_andunlock_c(self,abstime,buf) __KSIGNAL_VRECV(buf,_ksigset_timedrecvs_andunlock_c(self,abstime))
#define _ksigset_vtimeoutrecvs_andunlock_c(self,timeout,buf) __KSIGNAL_VRECV(buf,_ksigset_timeoutrecvs_andunlock_c(self,timeout))
#endif

#endif /* !__ASSEMBLY__ */

__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_SIGSET_V_H__ */

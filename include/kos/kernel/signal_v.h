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
#ifndef __KOS_KERNEL_SIGNAL_V_H__
#define __KOS_KERNEL_SIGNAL_V_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/compiler.h>
#include <kos/kernel/signal.h>

__DECL_BEGIN

//
// [continuation of <kos/kernel/signal.h>:Advanced-Signal-functionality]
//
// You can use what I call vsend/vrecv (The 'v' stands for 'value') to
// include some Impulse-specific data to be transferred to whoever is
// on the receiving end of said Impulse (Then often referred to as 'v-signal').
//
// An extension to signals, these functions allow tasks to wait
// for a signal that broadcasts/generates a value alongside the Impulse.
// HINT: To achieve this, KOS uses the 't_sigval' pointer with the ktask structure.
// 
// - Using vsend/vrecv, you can easily create a signal-subsystem that
//   is capable of some interesting things, such as knowing which signal
//   was used to wake a given task, and so on.
//
// WARNING: It is not possible to specify how big a buffer is when
//          receiving, meaning that in order to safely receive data,
//          you must known the greatest length any data that may be
//          received can have.
//          Such functionality would have required additional overhead
//          within every ktask, in the form of 4/8 more bytes.
//          In the long run, it probably wouldn't have made much of
//          a difference, but it really seemed like something of an
//          unnecessary feature.
//
// == NOTE: All of the following text describes KOS-specific caveats
//          and implementation details of v-signals.
//
// WARNING: In order to safely ensure that the recipient of a signal
//          has actually received it, the recipient must receive it within
//          a task-level critical block (KTASK_CRIT_BEGIN/KTASK_CRIT_END).
//          Critical blocks based on nointerrupt sections will not suffice,
//          as interrupts are re-enabled during the process of performing
//          task switches, essentially allowing a potential receiver
//          to be terminated before they have a chance of processing
//          data (in case processing of said data is required, such
//          as when a reference to some object, or a newly allocated
//          chunk of memory is passed using this mechanism)
//
// HINT: This mechanism can easily be used to implement
//       unbuffered input from sources such as a keyboard:
//       Simply setup an interrupt handler to vsend some signal,
//       alongside data (such as a keyboard stroke) and whenever
//       you want to start acting on such data, just vrecv the signal.
//       Obviously you'd need to add additional protection to prevent
//       the signal from being broadcast while not everyone is back
//       at it and receiving, but even that is kind-of just a bonus
//       feature if loss-less processing is also a requirement.
//       Such functionality can be achieved by using a ddist. (<kos/kernel/ddist.h>)
// NOTE: This is also what KOS does to implement sockets at their lowest level
//



#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Waits for a given signal to be emitted, providing a given buffer
// inside of which to store a potential value send alongside the signal.
// @param: buf: A pointer to a buffer of sufficient size to sustain
//              any kind of value that may be send via any of the
//              signals that the caller is attempting to receive.
//              NOTE: The buffer must still describe valid memory at the
//                    time that the signal will eventually be send.
//              HINT: The easiest (and safest) way to create such a buffer is to
//                    simply allocate it on the stack. (aka. 'char buf[64];' or 'alloca')
// @return: KE_OK:        A signal was successfully received.
// @return: KS_NODATA:    A signal was received, but its sender didn't use
//                        vsend, leaving the provided 'buf' unchanged.
// @return: KE_DESTROYED: The signal was/has-been killed.
// @return: KE_TIMEDOUT:  [ksignal_v(timed|timeout)recv{s}] The given timeout has passed.
// @return: KE_INTR:      The calling task was interrupted.
extern           __nonnull((1))   kerrno_t ksignal_vrecv(struct ksignal *__restrict signal, __kernel void *__restrict buf);
extern __wunused __nonnull((1,2)) kerrno_t ksignal_vtimedrecv(struct ksignal *__restrict signal, struct timespec const *__restrict abstime, __kernel void *__restrict buf);
extern __wunused __nonnull((1,2)) kerrno_t ksignal_vtimeoutrecv(struct ksignal *__restrict signal, struct timespec const *__restrict timeout, __kernel void *__restrict buf);
extern           __nonnull((2))   kerrno_t ksignal_vrecvs(__size_t sigc, struct ksignal *const *__restrict sigv, __kernel void *__restrict buf);
extern __wunused __nonnull((2,3)) kerrno_t ksignal_vtimedrecvs(__size_t sigc, struct ksignal *const *__restrict sigv, struct timespec const *__restrict abstime, __kernel void *__restrict buf);
extern __wunused __nonnull((2,3)) kerrno_t ksignal_vtimeoutrecvs(__size_t sigc, struct ksignal *const *__restrict sigv, struct timespec const *__restrict timeout, __kernel void *__restrict buf);
extern __crit           __nonnull((1,2))   kerrno_t _ksignal_vrecv_andunlock_c(struct ksignal *__restrict self, __kernel void *__restrict buf);
extern __crit           __nonnull((2,3))   kerrno_t _ksignal_vrecvs_andunlock_c(__size_t sigc, struct ksignal *const *__restrict sigv, __kernel void *__restrict buf);
extern __crit __wunused __nonnull((1,2,3)) kerrno_t _ksignal_vtimedrecv_andunlock_c(struct ksignal *self, struct timespec const *__restrict abstime, __kernel void *__restrict buf);
extern __crit __wunused __nonnull((2,3,4)) kerrno_t _ksignal_vtimedrecvs_andunlock_c(__size_t sigc, struct ksignal *const *__restrict sigv, struct timespec const *__restrict abstime, __kernel void *__restrict buf);
extern __crit __wunused __nonnull((1,2,3)) kerrno_t _ksignal_vtimeoutrecv_andunlock_c(struct ksignal *self, struct timespec const *__restrict timeout, __kernel void *__restrict buf);
extern __crit __wunused __nonnull((2,3,4)) kerrno_t _ksignal_vtimeoutrecvs_andunlock_c(__size_t sigc, struct ksignal *const *__restrict sigv, struct timespec const *__restrict timeout, __kernel void *__restrict buf);
#else
#define ksignal_vrecv(signal,buf) \
 __xblock({ struct ksignal *const __ksrcvself = (signal);\
            kerrno_t __ksrcverr; kassert_ksignal(__ksrcvself);\
            ksignal_lock(__ksrcvself,KSIGNAL_LOCK_WAIT);\
            __ksrcverr = _ksignal_vrecv_andunlock_c(__ksrcvself,buf);\
            ksignal_endlock();\
            __xreturn __ksrcverr;\
 })
#define ksignal_vtimedrecv(signal,abstime,buf) \
 __xblock({ struct ksignal *const __kstrcvself = (signal);\
            kerrno_t __kstrcverr; kassert_ksignal(__kstrcvself);\
            ksignal_lock(__kstrcvself,KSIGNAL_LOCK_WAIT);\
            __kstrcverr = _ksignal_vtimedrecv_andunlock_c(__kstrcvself,abstime,buf);\
            ksignal_endlock();\
            __xreturn __kstrcverr;\
 })
#define ksignal_vtimeoutrecv(signal,timeout,buf) \
 __xblock({ struct ksignal *const __kstorcvself = (signal);\
            kerrno_t __kstorcverr; kassert_ksignal(__kstorcvself);\
            ksignal_lock(__kstorcvself,KSIGNAL_LOCK_WAIT);\
            __kstorcverr = _ksignal_vtimeoutrecv_andunlock_c(__kstorcvself,abstime,buf);\
            ksignal_endlock();\
            __xreturn __kstorcverr;\
 })
#define ksignal_vrecvs(sigc,sigv,buf) \
 __xblock({ kerrno_t __ksrcvserr; __size_t const __ksrcvssigc = (sigc);\
            struct ksignal *const *const __ksrcvssigv = (sigv);\
            kassert_ksignals(__ksrcvssigc,__ksrcvssigv);\
            ksignal_locks(__ksrcvssigc,__ksrcvssigv,KSIGNAL_LOCK_WAIT);\
            __ksrcvserr = _ksignal_vrecvs_andunlock_c(__ksrcvssigc,__ksrcvssigv),buf;\
            ksignal_endlock();\
            __xreturn __ksrcvserr;\
 })
#define ksignal_vtimedrecvs(sigc,sigv,abstime,buf) \
 __xblock({ kerrno_t __kstrcvserr; __size_t const __kstrcvssigc = (sigc);\
            struct ksignal *const *const __kstrcvssigv = (sigv);\
            kassert_ksignals(__kstrcvssigc,__kstrcvssigv);\
            ksignal_locks(__kstrcvssigc,__kstrcvssigv,KSIGNAL_LOCK_WAIT);\
            __kstrcvserr = _ksignal_vtimedrecvs_andunlock_c(__kstrcvssigc,__kstrcvssigv,abstime,buf);\
            ksignal_endlock();\
            __xreturn __kstrcvserr;\
 })
#define ksignal_vtimeoutrecvs(sigc,sigv,timeout,buf) \
 __xblock({ kerrno_t __kstorcvserr; __size_t const __kstorcvssigc = (sigc);\
            struct ksignal *const *const __kstorcvssigv = (sigv);\
            kassert_ksignals(__kstorcvssigc,__kstorcvssigv);\
            ksignal_locks(__kstorcvssigc,__kstorcvssigv,KSIGNAL_LOCK_WAIT);\
            __kstorcvserr = _ksignal_vtimeoutrecvs_andunlock_c(__kstorcvssigc,__kstorcvssigv,timeout,buf);\
            ksignal_endlock();\
            __xreturn __kstorcvserr;\
 })
#define __KSIGNAL_VRECV_BEGIN(buf) \
 { struct ktask *const __vrvcaller = ktask_self();\
   __vrvcaller->t_sigval = (buf);
#define __KSIGNAL_VRECV_END(error) \
   if (__vrvcaller->t_sigval) {\
    /* No data was transmitted (cleanup sigval pointer,\
       and return KS_NODATA in success case). */\
    ktask_lock(__vrvcaller,KTASK_LOCK_SIGVAL);\
    if (__vrvcaller->t_sigval) {\
     __vrvcaller->t_sigval = NULL;\
     if (error == KE_OK) error = KS_NODATA;\
    }\
    ktask_unlock(__vrvcaller,KTASK_LOCK_SIGVAL);\
   }\
 }
#define __KSIGNAL_VRECV(buf,recv) \
 __xblock({ kerrno_t __vrverror;\
            __KSIGNAL_VRECV_BEGIN(buf)\
            __vrverror = (recv);\
            __KSIGNAL_VRECV_END(__vrverror)\
            __xreturn __vrverror;\
 })
#define _ksignal_vrecv_andunlock_c(self,buf) \
 __KSIGNAL_VRECV(buf,_ksignal_recv_andunlock_c(self))
#define _ksignal_vrecvs_andunlock_c(sigc,sigv,buf) \
 __KSIGNAL_VRECV(buf,_ksignal_recvs_andunlock_c(sigc,sigv))
#define _ksignal_vtimedrecv_andunlock_c(self,abstime,buf) \
 __KSIGNAL_VRECV(buf,_ksignal_timedrecv_andunlock_c(self,abstime))
#define _ksignal_vtimedrecvs_andunlock_c(sigc,sigv,abstime,buf) \
 __KSIGNAL_VRECV(buf,_ksignal_timedrecvs_andunlock_c(sigc,sigv,abstime))
#define _ksignal_vtimeoutrecv_andunlock_c(self,timeout,buf) \
 __KSIGNAL_VRECV(buf,_ksignal_timeoutrecv_andunlock_c(self,timeout))
#define _ksignal_vtimeoutrecvs_andunlock_c(sigc,sigv,timeout,buf) \
 __KSIGNAL_VRECV(buf,_ksignal_timeoutrecvs_andunlock_c(sigc,sigv,timeout))
#endif


#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Send a given vsignal to one waiting task.
// @param: newcpu: If NULL, schedule any waiting task on the calling (aka. current) CPU
// @return: KE_DESTROYED: The task has been terminated
// @return: KS_EMPTY:     No task was waiting, or the signal is dead
// @return: KS_UNCHANGED: The task has already been re-scheduled
//                       (Due to race-conditions, this should not be considered an error)
// @return: KS_NODATA:    A task was found and was woken, but it didn't start receiving
//                        using a 'ksignal_vrecv*' function (the value was not transferred)
//                        NOTE: This error is only returned if KE_OK would have otherwise been returned.
extern        __nonnull((1)) kerrno_t ksignal_vsendone(struct ksignal *__restrict self, void const *__restrict buf, __size_t bufsize);
extern __crit __nonnull((1)) kerrno_t _ksignal_vsendone_andunlock_c(struct ksignal *__restrict self, void const *__restrict buf, __size_t bufsize);
extern __crit __nonnull((1)) kerrno_t ksignal_vsendoneex(struct ksignal *__restrict self, struct kcpu *__restrict newcpu, int hint, void const *__restrict buf, __size_t bufsize);

//////////////////////////////////////////////////////////////////////////
// Similar to the functions above, but used for user-user data transfer,
// copying data from the effective page directory of the caller to that
// of the receiving threads.
// @return: KE_FAULT: Either the given source buffer, or a destination buffer was faulty.
extern        __nonnull((1)) kerrno_t ksignal_vusendone(struct ksignal *__restrict self, __user void const *__restrict buf, __size_t bufsize);
extern __crit __nonnull((1)) kerrno_t _ksignal_vusendone_andunlock_c(struct ksignal *__restrict self, __user void const *__restrict buf, __size_t bufsize);
extern __crit __nonnull((1)) kerrno_t ksignal_vusendoneex(struct ksignal *__restrict self, struct kcpu *__restrict newcpu, int hint, __user void const *__restrict buf, __size_t bufsize);
#else
#define ksignal_vsendone(self,buf,bufsize) \
 ksignal_vsendoneex(self,kcpu_leastload(),KTASK_RESCHEDULE_HINT_DEFAULT,buf,bufsize)
#define /*__crit*/ _ksignal_vsendone_andunlock_c(self,buf,bufsize) \
 _ksignal_vsendoneex_andunlock_c(self,kcpu_leastload(),KTASK_RESCHEDULE_HINT_DEFAULT,buf,bufsize)
#define ksignal_vsendoneex(self,newcpu,hint,buf,bufsize) \
 __xblock({ struct ksignal *const __ksoeself = (self);\
            kerrno_t __ksoeerr; kassert_ksignal(__ksoeself);\
            ksignal_lock(__ksoeself,KSIGNAL_LOCK_WAIT);\
            __ksoeerr = _ksignal_vsendoneex_andunlock_c(__ksoeself,newcpu,hint,buf,bufsize);\
            ksignal_endlock();\
            __xreturn __ksoeerr;\
 })
#define ksignal_vusendone(self,buf,bufsize) \
 ksignal_vusendoneex(self,kcpu_leastload(),KTASK_RESCHEDULE_HINT_DEFAULT,buf,bufsize)
#define /*__crit*/ _ksignal_vusendone_andunlock_c(self,buf,bufsize) \
 _ksignal_vusendoneex_andunlock_c(self,kcpu_leastload(),KTASK_RESCHEDULE_HINT_DEFAULT,buf,bufsize)
#define ksignal_vusendoneex(self,newcpu,hint,buf,bufsize) \
 __xblock({ struct ksignal *const __ksoeself = (self);\
            kerrno_t __ksoeerr; kassert_ksignal(__ksoeself);\
            ksignal_lock(__ksoeself,KSIGNAL_LOCK_WAIT);\
            __ksoeerr = _ksignal_vusendoneex_andunlock_c(__ksoeself,newcpu,hint,buf,bufsize);\
            ksignal_endlock();\
            __xreturn __ksoeerr;\
 })

#endif
extern __crit __nonnull((1,4)) kerrno_t
_ksignal_vsendoneex_andunlock_c(struct ksignal *__restrict self,
                                struct kcpu *__restrict newcpu, int hint,
                                void const *__restrict buf, __size_t bufsize);
extern __crit __nonnull((1,4)) kerrno_t
_ksignal_vusendoneex_andunlock_c(struct ksignal *__restrict self,
                                 struct kcpu *__restrict newcpu, int hint,
                                 __user void const *__restrict buf, __size_t bufsize);


//////////////////////////////////////////////////////////////////////////
// Wake up to 'n_tasks' tasks.
//  - If non-NULL, '*woken_tasks' will be set to the amount of tasks woken.
// @return: * : Same as the first KE_ISERR of 'ksignal_vsendone' 
extern __nonnull((1,4)) kerrno_t
ksignal_vsendn(struct ksignal *__restrict self,
               __size_t n_tasks, __size_t *__restrict woken_tasks,
               void const *__restrict buf, __size_t bufsize);

//////////////////////////////////////////////////////////////////////////
// Similar to 'ksignal_vsendn', but used for user-user data transfer,
// copying data from the effective page directory of the caller to
// that of the receiving threads.
extern __nonnull((1,4)) kerrno_t
ksignal_vusendn(struct ksignal *__restrict self,
                __size_t n_tasks, __size_t *__restrict woken_tasks,
                __user void const *__restrict buf, __size_t bufsize);



#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Wakes all waiting tasks.
// WARNING: Errorous wakeups are silently discarded,
//          but do not count to the total returned.
// @return: * : The amount of woken tasks.
extern __nonnull((1,2)) __size_t
ksignal_vsendall(struct ksignal *__restrict self,
                 void const *__restrict buf, __size_t bufsize);

//////////////////////////////////////////////////////////////////////////
// Similar to 'ksignal_vsendall', but used for user-user data transfer,
// copying data from the effective page directory of the caller to
// that of the receiving threads.
extern __nonnull((1,2)) __size_t
ksignal_vusendall(struct ksignal *__restrict self,
                  __user void const *__restrict buf, __size_t bufsize);
#else
#define ksignal_vsendall(self,buf,bufsize)\
 __xblock({ struct ksignal *const __kssaself = (self);\
            __size_t __kssares; kassert_ksignal(__kssaself);\
            ksignal_lock(__kssaself,KSIGNAL_LOCK_WAIT);\
            __kssares = _ksignal_vsendall_andunlock_c(__kssaself,buf,bufsize);\
            ksignal_endlock();\
            __xreturn __kssares;\
 })
#define ksignal_vusendall(self,buf,bufsize)\
 __xblock({ struct ksignal *const __kssaself = (self);\
            __size_t __kssares; kassert_ksignal(__kssaself);\
            ksignal_lock(__kssaself,KSIGNAL_LOCK_WAIT);\
            __kssares = _ksignal_vusendall_andunlock_c(__kssaself,buf,bufsize);\
            ksignal_endlock();\
            __xreturn __kssares;\
 })
#endif
extern __crit __nonnull((1,2)) __size_t
_ksignal_vsendall_andunlock_c(struct ksignal *__restrict self,
                              void const *__restrict buf, __size_t bufsize);
extern __crit __nonnull((1,2)) __size_t
_ksignal_vusendall_andunlock_c(struct ksignal *__restrict self,
                               __user void const *__restrict buf, __size_t bufsize);



__DECL_END

#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_SIGNAL_V_H__ */

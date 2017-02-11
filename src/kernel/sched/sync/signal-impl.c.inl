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
#ifdef __INTELLISENSE__
#include "signal.c.inl"
__DECL_BEGIN
#define SINGLE
#endif

#ifdef SINGLE
#define sigc                           ((size_t)1)
#define sigv    (struct ksignal *const *)(&signal)
#define ASSERT_CHECKS \
 (kassert_ksignal(signal)\
 ,assert(ksignal_islocked(signal,KSIGNAL_LOCK_WAIT)))
#define CHECK_DEAD \
do{\
 if __unlikely(ksignal_isdead_unlocked(signal)) {\
  ksignal_unlock_c(signal,KSIGNAL_LOCK_WAIT);\
  return KE_DESTROYED;\
 }\
}while(0)
#else
#define ASSERT_CHECKS \
 (kassert_ksignals(sigc,sigv)\
 ,assert(ksignal_islockeds(sigc,sigv,KSIGNAL_LOCK_WAIT)))
#define CHECK_DEAD \
do{\
 if __unlikely(ksignal_isdeads_unlocked(sigc,sigv)) {\
  ksignal_unlocks_c(sigc,sigv,KSIGNAL_LOCK_WAIT);\
  return KE_DESTROYED;\
 }\
}while(0)
#endif

#ifdef SINGLE
__crit kerrno_t _ksignal_recv_andunlock_c(struct ksignal *__restrict signal)
#else
__crit kerrno_t _ksignal_recvs_andunlock_c(size_t sigc,
                                           struct ksignal *const *__restrict sigv)
#endif
{
 kerrno_t error;
 KTASK_CRIT_MARK
 ASSERT_CHECKS;
 CHECK_DEAD;
 k_syslogf(KLOG_TRACE,"Begin waiting for %Iu signals\n",sigc);
 error = ktask_unschedule_ex(ktask_self(),KTASK_STATE_WAITING,NULL,sigc,sigv);
 assertf(error != KS_UNCHANGED,"Should never happen for ktask_self()");
 return error;
}
#ifdef SINGLE
__crit kerrno_t _ksignal_timedrecv_andunlock_c(struct ksignal *signal,
                                               struct timespec const *__restrict abstime)
#else
__crit kerrno_t _ksignal_timedrecvs_andunlock_c(size_t sigc,
                                                struct ksignal *const *__restrict sigv,
                                                struct timespec const *__restrict abstime)
#endif
{
 kerrno_t error;
 KTASK_CRIT_MARK
 ASSERT_CHECKS;
 kassertobj(abstime);
 CHECK_DEAD;
 k_syslogf(KLOG_TRACE,"Begin waiting for %Iu signal (for up to %I32u seconds and %ld nanoseconds)\n",
           sigc,abstime->tv_sec,abstime->tv_nsec);
 error = ktask_unschedule_ex(ktask_self(),KTASK_STATE_WAITINGTMO,(void *)abstime,sigc,sigv);
 assertf(error != KS_UNCHANGED,"Should never happen for ktask_self()");
 return error;
}

#ifdef SINGLE
__crit kerrno_t _ksignal_timeoutrecv_andunlock_c(struct ksignal *signal,
                                                 struct timespec const *__restrict timeout)
#else
__crit kerrno_t _ksignal_timeoutrecvs_andunlock_c(size_t sigc,
                                                  struct ksignal *const *__restrict sigv,
                                                  struct timespec const *__restrict timeout)
#endif
{
 struct timespec abstime;
 KTASK_CRIT_MARK
 ASSERT_CHECKS; kassertobj(timeout);
 ktime_getnoworcpu(&abstime);
 __timespec_add(&abstime,timeout);
#ifdef SINGLE
 return _ksignal_timedrecv_andunlock_c(signal,&abstime);
#else
 return _ksignal_timedrecvs_andunlock_c(sigc,sigv,&abstime);
#endif
}

#undef ASSERT_CHECKS
#ifdef SINGLE
#undef sigc
#undef sigv
#undef SINGLE
#undef CHECK_DEAD
#endif

#ifdef __INTELLISENSE__
__DECL_END
#endif

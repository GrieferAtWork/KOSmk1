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
#include "mmutex.c.inl"
__DECL_BEGIN
#define LOCKALL
#endif

#ifdef LOCKALL
#define F(x)  x##s
#else
#define F(x)  x
#endif

#ifdef LOCKALL
#define TRYLOCK \
 kmmutex_lock_t oldlocks,newlocks;\
 for (;;) {\
  oldlocks = katomic_load(self->mmx_locks);\
  if (oldlocks&locks) break; /* At least one lock is already set. */\
  newlocks = oldlocks|locks;\
  if (katomic_cmpxch(self->mmx_locks,oldlocks,newlocks)) {\
   ONACQUIRE;\
   return KE_OK;\
  }\
 }
#define ONACQUIRE    KMMUTEX_ONACQUIRES(self,locks)
#define ONRELEASE    KMMUTEX_ONRELEASES(self,locks)
#define ONLOCKINUSE  KMMUTEX_ONLOCKINUSES(self,locks)
#else
#define TRYLOCK \
 assertf(KMMUTEX_ISONELOCK(lock),"More than one lock set in non-*locks call: %I32x",(__u32)lock);\
 if (!(katomic_fetchor(self->mmx_locks,lock)&lock)) { ONACQUIRE; return KE_OK; }
#define ONACQUIRE    KMMUTEX_ONACQUIRE(self,KMMUTEX_LOCKID(lock))
#define ONRELEASE    KMMUTEX_ONRELEASE(self,KMMUTEX_LOCKID(lock))
#define ONLOCKINUSE  KMMUTEX_ONLOCKINUSE(self,KMMUTEX_LOCKID(lock))
#endif

__crit kerrno_t F(kmmutex_lock)(struct kmmutex *self,
                                kmmutex_lock_t F(lock)) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kmmutex(self);
 do {
  TRYLOCK; ONLOCKINUSE;
  // Wait for the next signal
  error = ksignal_recv(&self->mmx_sig);
 } while (KE_ISOK(error));
 return error;
}
__crit kerrno_t F(kmmutex_trylock)(struct kmmutex *__restrict self,
                                   kmmutex_lock_t F(lock)) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kmmutex(self);
 TRYLOCK;
 ksignal_lock(&self->mmx_sig,KSIGNAL_LOCK_WAIT);
 if __unlikely(self->mmx_sig.s_flags&KSIGNAL_FLAG_DEAD) {
  error = KE_DESTROYED;
 } else {
  ONLOCKINUSE;
  error = KE_WOULDBLOCK;
 }
 ksignal_unlock(&self->mmx_sig,KSIGNAL_LOCK_WAIT);
 return error;
}
__crit kerrno_t F(kmmutex_timedlock)(struct kmmutex *__restrict self,
                                     struct timespec const *__restrict abstime,
                                     kmmutex_lock_t F(lock)) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kmmutex(self);
 kassertobj(abstime);
 do {
  TRYLOCK; ONLOCKINUSE;
  // Wait for the next signal
  error = ksignal_timedrecv(&self->mmx_sig,abstime);
 } while (KE_ISOK(error));
 return error;
}
__crit kerrno_t F(kmmutex_timeoutlock)(struct kmmutex *__restrict self,
                                       struct timespec const *__restrict timeout,
                                       kmmutex_lock_t F(lock)) {
 struct timespec abstime;
 KTASK_CRIT_MARK
 kassert_kmmutex(self);
 kassertobj(timeout);
 ktime_getnoworcpu(&abstime);
 __timespec_add(&abstime,timeout);
 return F(kmmutex_timedlock)(self,&abstime,F(lock));
}
__crit kerrno_t F(kmmutex_unlock)(struct kmmutex *__restrict self,
                                  kmmutex_lock_t F(lock)) {
 KTASK_CRIT_MARK
 kassert_kmmutex(self);
 ONRELEASE;
#ifdef LOCKALL
 assertf(kmmutex_islockeds(self,locks),"Locks %I32x not held",(__u32)locks);
#else
 assertf(kmmutex_islocked(self,lock),"Lock %I32x not held",(__u32)lock);
#endif
 katomic_fetchand(self->mmx_locks,~F(lock));
 return ksignal_sendall(&self->mmx_sig) ? KE_OK : KS_UNCHANGED;
}

#undef ONACQUIRE
#undef ONRELEASE
#undef ONLOCKINUSE
#undef TRYLOCK
#undef F
#ifdef LOCKALL
#undef LOCKALL
#endif

#ifdef __INTELLISENSE__
__DECL_END
#endif

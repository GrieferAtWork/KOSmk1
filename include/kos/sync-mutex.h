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
#ifndef __KOS_SYNC_MUTEX_H__
#define __KOS_SYNC_MUTEX_H__ 1

#include <kos/config.h>
#include <kos/compiler.h>

#ifndef __ASSEMBLY__
#ifndef __KERNEL__
#include <kos/atomic.h>
#include <kos/errno.h>
#include <kos/futex.h>
#ifndef __INTELLISENSE__
#include <proc-tls.h>
#include <string.h>
#include <assert.h>
#endif

/* Userspace synchronization primitives. */
__DECL_BEGIN

struct kmutex {
 kfutex_t m_futex; /*< Futex counter/mutex state (0: free; 1: locked). */
};

#define KMUTEX_INIT  {0} /* Static initializer. */

//////////////////////////////////////////////////////////////////////////
// Initialize a mutex object.
__local void kmutex_init __P((struct kmutex *__restrict __self));

//////////////////////////////////////////////////////////////////////////
// Acquire/Release a lock on a given mutex.
// @return: KE_OK:         The operation completed successfully.
// @return: KE_WOUDLBLOCK: [kmutex_trylock] Failed to acquire a lock immediately.
// @return: KE_TIMEDOUT:   [kmutex_{timed}lock] The given 'ABSTIME', or a previously set timeout has expired.
// @return: KE_NOMEM:      [kmutex_{timed}lock] Not enough available memory.
// @return: KE_PERM:       [kmutex_unlock_safe] The given mutex was not locked.
__local kerrno_t kmutex_lock __P((struct kmutex *__restrict __self));
__local kerrno_t kmutex_trylock __P((struct kmutex *__restrict __self));
__local kerrno_t kmutex_timedlock __P((struct kmutex *__restrict __self, struct timespec const *__abstime));
__local kerrno_t kmutex_unlock __P((struct kmutex *__restrict __self));
__local kerrno_t kmutex_unlock_safe __P((struct kmutex *__restrict __self));


struct krmutex {
 kfutex_t             rm_futex; /*< Futex counter/mutex state/recursion (0: free; 1-n: lock count). */
 volatile __uintptr_t rm_owner; /*< Unqiue identifier for the owner thread (thread_unique()). */
};
#define KRMUTEX_INIT     {0,0} /* Static initializer. */

//////////////////////////////////////////////////////////////////////////
// Initialize a recursive mutex object.
__local void krmutex_init __P((struct krmutex *__restrict __self));

//////////////////////////////////////////////////////////////////////////
// Acquire/Release a lock on a given recursive mutex.
// @return: KE_OK:         The operation completed successfully.
// @return: KE_WOUDLBLOCK: [krmutex_trylock*] Failed to acquire a lock immediately.
// @return: KE_TIMEDOUT:   [krmutex_{timed}lock*] The given 'ABSTIME', or a previously set timeout has expired.
// @return: KE_NOMEM:      [krmutex_{timed}lock*] Not enough available memory.
// @return: KS_BLOCKING:   [krmutex_*lock*] The lock was successfully acquired because the caller was already holding one.
// @return: KS_BLOCKING:   [krmutex_unlock*] The call is still blocking the associated mutex.
// @return: KE_PERM:       [krmutex_unlock_safe] The caller is not holding a lock to the rmutex.
__local kerrno_t krmutex_lock __P((struct krmutex *__restrict __self));
__local kerrno_t krmutex_trylock __P((struct krmutex *__restrict __self));
__local kerrno_t krmutex_timedlock __P((struct krmutex *__restrict __self, struct timespec const *__abstime));
__local kerrno_t krmutex_unlock __P((struct krmutex *__restrict __self));
__local kerrno_t krmutex_unlock_safe __P((struct krmutex *__restrict __self));



#ifndef __INTELLISENSE__
__local void kmutex_init __D1(struct kmutex *__restrict,__self) { __self->m_futex = 0; }
__local kerrno_t kmutex_trylock __D1(struct kmutex *__restrict,__self) {
 return katomic_cmpxch(*(unsigned int *)&__self->m_futex,0,1) ? KE_OK : KE_WOULDBLOCK;
}
__local kerrno_t kmutex_timedlock
__D2(struct kmutex *__restrict,__self,struct timespec const *,__abstime) {
 kerrno_t __error;
 while (((__error = kmutex_trylock(__self)) == KE_WOULDBLOCK) &&
         (__error = kfutex_cmd(&__self->m_futex,KFUTEX_RECVIF(KFUTEX_NOT_EQUAL),0,NULL,__abstime),
          KE_ISOK(__error) || __error == KE_AGAIN));
 return __error;
}
__local kerrno_t kmutex_lock __D1(struct kmutex *__restrict,__self) {
 return kmutex_timedlock(__self,NULL);
}
__local kerrno_t kmutex_unlock __D1(struct kmutex *__restrict,__self) {
 assertef(katomic_cmpxch(*(unsigned int *)&__self->m_futex,1,0),"Mutex not locked");
 return kfutex_sendone(&__self->m_futex);
}
__local kerrno_t kmutex_unlock_safe __D1(struct kmutex *__restrict,__self) {
 return katomic_cmpxch(*(unsigned int *)&__self->m_futex,1,0)
  ? kfutex_sendone(&__self->m_futex) : KE_PERM;
}
__local void krmutex_init __D1(struct krmutex *__restrict,__self) {
 memset(__self,0,sizeof(struct krmutex));
}

__local kerrno_t krmutex_trylock __D1(struct krmutex *__restrict,__self) {
 __uintptr_t __unique = _thread_unique();
 /* Check twice to prevent false readings. */
 if (__self->rm_owner == __unique &&
     __self->rm_owner == __unique) {
  katomic_fetchinc(*(unsigned int *)&__self->rm_futex);
  return KS_BLOCKING;
 }
 if (katomic_cmpxch(*(unsigned int *)&__self->rm_futex,0,1)) {
  __self->rm_owner = __unique;
  return KE_OK;
 }
 return KE_WOULDBLOCK;
}
__local kerrno_t krmutex_timedlock
__D2(struct krmutex *__restrict,__self,struct timespec const *,__abstime) {
 kerrno_t __error;
 while (((__error = krmutex_trylock(__self)) == KE_WOULDBLOCK) &&
         (__error = kfutex_cmd(&__self->rm_futex,KFUTEX_RECVIF(KFUTEX_NOT_EQUAL),0,NULL,__abstime),
          KE_ISOK(__error) || __error == KE_AGAIN));
 return __error;
}
__local kerrno_t krmutex_lock __D1(struct krmutex *__restrict,__self) {
 return krmutex_timedlock(__self,NULL);
}
__local kerrno_t krmutex_unlock __D1(struct krmutex *__restrict,__self) {
 unsigned int __oldval;
 assertf(__self->rm_owner == _thread_unique(),"You're not the owner");
 __oldval = katomic_fetchdec(*(unsigned int *)&__self->rm_futex);
 assertf(__oldval != 0,"Mutex was not locked");
 return __oldval == 1 ? kfutex_sendone(&__self->rm_futex) : KS_BLOCKING;
}
__local kerrno_t krmutex_unlock_safe __D1(struct krmutex *__restrict,__self) {
 unsigned int __oldval;
 if (__self->rm_owner != _thread_unique()) return KE_PERM;
 do {
  __oldval = katomic_load(*(unsigned int *)&__self->rm_futex);
  if (!__oldval) return KE_PERM;
 } while (!katomic_cmpxch(*(unsigned int *)&__self->rm_futex,__oldval,__oldval-1));
 return __oldval == 1 ? kfutex_sendone(&__self->rm_futex) : KS_BLOCKING;
}
#endif

__DECL_END
#else /* !__KERNEL__ */
#include <kos/kernel/mutex.h>
#endif /* __KERNEL__ */
#endif /* !__ASSEMBLY__ */

#endif /* !__KOS_SYNC_MUTEX_H__ */

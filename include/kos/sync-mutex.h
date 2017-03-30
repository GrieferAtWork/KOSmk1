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
#include <kos/futex.h>
#include <kos/errno.h>

/* Userspace synchronization primitives. */
__DECL_BEGIN

struct kmutex {
 unsigned int m_futex; /*< Futex counter/mutex state (0: free; 1: locked). */
};

//////////////////////////////////////////////////////////////////////////
// Initialize a mutex object.
#define KMUTEX_INIT  {0}
#define kmutex_init(self) (void)((self)->m_futex = 0)

//////////////////////////////////////////////////////////////////////////
// Acquire/Release a lock on a given mutex.
// @return: KE_OK:         The operation completed successfully.
// @return: KE_WOUDLBLOCK: [kmutex_trylock] Failed to acquire a lock immediatly.
// @return: KE_TIMEDOUT:   [kmutex_{timed}lock] The given 'ABSTIME', or a previously set timeout has expired.
// @return: KE_NOMEM:      [kmutex_{timed}lock] Not enough available memory.
__local kerrno_t kmutex_lock __P((struct kmutex *__restrict __self));
__local kerrno_t kmutex_trylock __P((struct kmutex *__restrict __self));
__local kerrno_t kmutex_timedlock __P((struct kmutex *__restrict __self, struct timespec *__abstime));
__local kerrno_t kmutex_unlock __P((struct kmutex *__restrict __self));


#ifndef __INTELLISENSE__
__local kerrno_t kmutex_trylock
__D1(struct kmutex *__restrict,__self) {
 return katomic_cmpxch(__self->m_futex,0,1) ? KE_OK : KE_WOULDBLOCK;
}
__local kerrno_t kmutex_timedlock
__D2(struct kmutex *__restrict,__self,struct timespec *,__abstime) {
 kerrno_t __error;
 while (((__error = kmutex_trylock(__self)) == KE_WOULDBLOCK) &&
         (__error = ktask_futex(&__self->m_futex,KTASK_FUTEX_WAIT|KTASK_FUTEX_WAIT_NE,0,__abstime),
          KE_ISOK(__error) || __error == KE_AGAIN));
 return __error;
}
__local kerrno_t kmutex_lock __D1(struct kmutex *__restrict,__self) {
 return kmutex_timedlock(__self,NULL);
}
__local kerrno_t kmutex_unlock __D1(struct kmutex *__restrict,__self) {
 return katomic_cmpxch(__self->m_futex,1,0)
  ? ktask_futex(&__self->m_futex,KTASK_FUTEX_WAKE,1,NULL)
  : KE_PERM;
}
#endif

__DECL_END
#else /* !__KERNEL__ */
#include <kos/kernel/mutex.h>
#endif /* __KERNEL__ */
#endif /* !__ASSEMBLY__ */

#endif /* !__KOS_SYNC_MUTEX_H__ */

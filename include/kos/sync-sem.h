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
#ifndef __KOS_SYNC_SEMAPHORE_H__
#define __KOS_SYNC_SEMAPHORE_H__ 1

#include <kos/config.h>
#include <kos/compiler.h>

#ifndef __ASSEMBLY__
#ifndef __KERNEL__
#include <kos/atomic.h>
#include <kos/errno.h>
#include <kos/futex.h>

/* Userspace synchronization primitives. */
__DECL_BEGIN

struct ksem {
 kfutex_t s_futex; /*< Futex counter/semaphore ticket count. */
};
#define KSEM_INIT(tickets)  {tickets} /* Static initializer. */
__local void ksem_init __P((struct ksem *__restrict __self, unsigned int __tickets));


__local kerrno_t ksem_acquire __P((struct ksem *__restrict __self));
__local kerrno_t ksem_tryacquire __P((struct ksem *__restrict __self));
__local kerrno_t ksem_timedacquire __P((struct ksem *__restrict __self,
                                        struct timespec const *abstime));
__local kerrno_t ksem_release __P((struct ksem *__restrict __self,
                                   unsigned int      __tickets,
                            /*opt*/unsigned int *__old_tickets));



#ifndef __INTELLISENSE__
__local void ksem_init __D2(struct ksem *__restrict,__self,unsigned int,__tickets) { __self->s_futex = __tickets; }
__local kerrno_t ksem_tryacquire __D1(struct ksem *__restrict,__self) {
 unsigned int __oldcnt;
 do {
  __oldcnt = __self->s_futex;
  if (!__oldcnt) return KE_WOULDBLOCK;
 } while (!katomic_cmpxch(*(unsigned int *)&__self->s_futex,__oldcnt,__oldcnt-1));
 return KE_OK;
}
__local kerrno_t ksem_timedacquire
__D2(struct ksem *__restrict,__self,struct timespec const *,__abstime) {
 kerrno_t __error;
 while ((__error == ksem_tryacquire(__self)) == KE_WOULDBLOCK &&
         /* atomic: if (__self->s_futex == 0) { recv(__self->s_futex); } */
        (__error == kfutex_cmd(&__self->s_futex,KFUTEX_RECVIF(KFUTEX_EQUAL),
                               0,NULL,__abstime),
         KE_ISOK(__error) || __error == KE_AGAIN));
 return __error;
}
__local kerrno_t ksem_acquire __D1(struct ksem *__restrict,__self) {
 return ksem_timedacquire(__self,NULL);
}
__local kerrno_t ksem_release
__D3(struct ksem *__restrict,__self,unsigned int,__tickets,unsigned int *,__old_tickets) {
 unsigned int __oldcnt = katomic_fetchadd(__self->s_futex,__tickets);
 if (__old_tickets) *__old_tickets = __oldcnt;
 return kfutex_sendn(&__self->s_futex,__tickets);
}
#endif


__DECL_END
#else /* !__KERNEL__ */
#include <kos/kernel/sem.h>
#endif /* __KERNEL__ */
#endif /* !__ASSEMBLY__ */

#endif /* !__KOS_SYNC_SEMAPHORE_H__ */

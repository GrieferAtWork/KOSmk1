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
#ifndef __PROC_SYNC_C__
#define __PROC_SYNC_C__ 1

#include <kos/config.h>
#ifndef __KERNEL__
#include <proc.h>
#include <errno.h>

__DECL_BEGIN

__public int mutex_init(mutex_t *self, unsigned int kind) {
 if (!self) { __set_errno(EINVAL); return -1; }
 self->__m_ftx = 0;
 self->__m_kind = kind;
 self->__m_owner = 0;
 return 0;
}
__public int mutex_trylock(mutex_t *self);
__public int mutex_timedlock(mutex_t *self, struct timespec const *__restrict abstime);
__public int mutex_timeoutlock(mutex_t *self, struct timespec const *__restrict timeout);
__public int mutex_lock(mutex_t *self);
__public int mutex_unlock(mutex_t *self);

__DECL_END
#endif

#endif /* !__PROC_SYNC_C__ */

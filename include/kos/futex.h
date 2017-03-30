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

#define KTASK_FUTEX_WAIT  0 /* Check if '*uaddr' is equal to 'val', waiting if it is, but returning KE_AGAIN if not. */
#define KTASK_FUTEX_WAKE  1 /* Wake up to 'val' threads waiting at 'uaddr'. (ECX is set to the amount of woken threads) */

#ifndef __NO_PROTOTYPES
#ifndef __ASSEMBLY__
struct timespec;

//////////////////////////////////////////////////////////////////////////
// Perform a futex operation:
// @return: KE_OK:        [*]                The operation completed successfully.
// @return: KE_FAULT:     [*]                The given 'uaddr' is faulty.
// @return: KE_FAULT:     [KTASK_FUTEX_WAIT] The given 'abstime' pointer is faulty and non-NULL.
// @return: KE_AGAIN:     [KTASK_FUTEX_WAIT] The value at '*uaddr' was equal to 'val' (the function didn't block)
// @return: KE_TIMEDOUT:  [KTASK_FUTEX_WAIT] The given 'abstime' or a previously set timeout has expired.
// @return: KE_NOMEM:     [*]                No futex existed at the given address, and not enough memory was available to allocate one.
// @return: KE_INVAL:     [?]                The specified 'futex_op' is unknown.
__local _syscall4(kerrno_t,ktask_futex,
                  unsigned int *,uaddr,unsigned int,futex_op,
                  unsigned int,val,struct timespec *,abstime);


#endif /* !__ASSEMBLY__ */
#endif /* !__NO_PROTOTYPES */


__DECL_END

#endif /* !__KOS_FUTEX_H__ */

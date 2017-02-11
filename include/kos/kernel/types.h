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
#ifndef __KOS_KERNEL_TYPES_H__
#define __KOS_KERNEL_TYPES_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#ifndef __ASSEMBLY__
#include <kos/types.h>
#include <kos/kernel/features.h>

__DECL_BEGIN

#ifndef __ktaskprio_t_defined
#define __ktaskprio_t_defined 1
typedef __ktaskprio_t ktaskprio_t;
#endif

typedef __u32 __kpageflag_t;

#define KTASKPRIO_MIN   (-32767-1)
#define KTASKPRIO_MAX     32767

typedef __un(KSEMAPHORE_TICKETBITS) ksemcount_t;

// Module ID
#ifndef __kmodid_t_defined
#define __kmodid_t_defined 1
typedef __kmodid_t kmodid_t;
#endif

__DECL_END

#endif /* !__ASSEMBLY__ */
#endif /* !__KERNEL__ */

#endif /* !__KOS_KERNEL_TYPES_H__ */

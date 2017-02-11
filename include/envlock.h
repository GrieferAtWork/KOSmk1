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
#ifndef __ENVLOCK_H__
#ifndef _INCLUDE_ENVLOCK_H_
#define __ENVLOCK_H__ 1
#define _INCLUDE_ENVLOCK_H_ 1

// Found in cygwin; locks the 'environ' extern variable,
// allowing multi-threaded code to safely enumerate it
// without the risk of another thread.
// (At least I think that's what it's for...)

#include <kos/compiler.h>
#ifndef __KERNEL__
#include <kos/atomic.h>
#include <kos/task.h>

__DECL_BEGIN

#define ENV_LOCK   __xblock({ while (!katomic_cmpxch(__env_spinlock,0,1)) ktask_yield(); (void)0; })
#define ENV_UNLOCK katomic_store(__env_spinlock,0)

extern __atomic int __env_spinlock;

__DECL_END

#endif /* __KERNEL__ */

#endif /* !_INCLUDE_ENVLOCK_H_ */
#endif /* !__ENVLOCK_H__ */

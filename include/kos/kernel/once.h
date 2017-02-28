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
#ifndef __KOS_KERNEL_ONCE_H__
#define __KOS_KERNEL_ONCE_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/compiler.h>
#ifndef __INTELLISENSE__
#include <kos/atomic.h>
#include <kos/kernel/sched_yield.h>
#endif

__DECL_BEGIN

struct konce { int ko_didrun; };
#define KONCE_INIT         {0}

#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Quick & easy run-once solution:
// >> for (i = 0; i < 200; ++i) {
// >>   KONCE init_data();
// >>   use_data();
// >> }
#define KONCE         if(0);else
#define KONCE_M(x)    if(0);else
static struct konce *KONCE_ALLOC(void);
#else
#define KONCE_ALLOC() \
 __xblock({ static struct konce __onc = KONCE_INIT;\
            __xreturn &__onc;\
 })

#define KONCE      KONCE_M(KONCE_ALLOC())
#define KONCE_M(x) \
 for (struct konce *const __koself = (x); __xblock({\
   int __rval = katomic_cmpxch_val(__koself->ko_didrun,0,1);\
   while (__rval == 1) { ktask_yield(); __rval = katomic_load(__koself->ko_didrun); }\
   __xreturn __rval == 0;\
 }); katomic_store(__koself->ko_didrun,2))
#endif

__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_ONCE_H__ */

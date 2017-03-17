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
#ifndef __KOS_COMPILER_SPARSE_H__
#define __KOS_COMPILER_SPARSE_H__ 1

#define __user             __attribute__((noderef,address_space(1)))
#define __kernel           __attribute__((address_space(0)))
#define __safe             __attribute__((safe))
#define __force            __attribute__((force))
#define __nocast           __attribute__((nocast))
#define __iomem            __attribute__((noderef,address_space(2)))
#define __must_hold(x)     __attribute__((context(x,1,1)))
#define __acquires(x)      __attribute__((context(x,0,1)))
#define __releases(x)      __attribute__((context(x,1,0)))
#define __acquire(x)       __context__(x,1)
#define __release(x)       __context__(x,-1)
#define __cond_lock(x,c) ((c) ? ({ __acquire(x); 1; }) : 0)
#define __percpu           __attribute__((noderef,address_space(3)))
#ifdef CONFIG_SPARSE_RCU_POINTER
#define __rcu  __attribute__((noderef, address_space(4)))
#endif

extern void __chk_user_ptr(void const volatile __user *);
extern void __chk_io_ptr(void const volatile __iomem *);

#endif /* !__KOS_COMPILER_SPARSE_H__ */

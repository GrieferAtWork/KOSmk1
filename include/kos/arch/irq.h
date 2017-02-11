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
#ifndef __KOS_ARCH_IRQ_H__
#define __KOS_ARCH_IRQ_H__ 1

#include <kos/compiler.h>
#include <kos/kernel/features.h>

#if !KCONFIG_HAVE_INTERRUPTS
#undef karch_irq_enable
#undef karch_irq_disable
#undef karch_irq_idle
#undef karch_irq_enabled
#else
#include <kos/arch.h>
#ifdef KOS_ARCH_HAVE_IRQ_H
#include KOS_ARCH_INCLUDE(irq.h)
#endif
#endif

#ifndef karch_irq_enable
#undef KOS_ARCH_HAVE_IRQ_H
#define karch_irq_enable()  (void)0
#define karch_irq_disable() (void)0
#define karch_irq_idle()    (void)0
#define karch_irq_enabled() 0
#endif


#endif /* !__KOS_ARCH_IRQ_H__ */

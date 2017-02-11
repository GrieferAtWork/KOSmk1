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
#ifndef __KOS_ARCH_H__
#define __KOS_ARCH_H__ 1

#if defined(__i386__) || defined(__x86_64__)
#define __x86__
#define KOS_ARCH_INCLUDE(__file) <kos/arch/x86/__file>
#else
#define KOS_ARCH_USE_GENERIC
#define KOS_ARCH_INCLUDE(__file) <kos/arch/generic/__file>
#endif

// Pull in arch-specific features
#include KOS_ARCH_INCLUDE(features.h)

#define KOS_ARCH_HAVE(name) (KOS_ARCH_HAVE_##name-0)

// Pull in generic features
#include <kos/arch/generic/features.h>

#endif /* !__KOS_ARCH_H__ */

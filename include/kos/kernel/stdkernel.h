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
#ifndef __KOS_KERNEL_STDKERNEL_H__
#define __KOS_KERNEL_STDKERNEL_H__ 1

#ifndef __KOS_COMPILER_H__
#include <kos/compiler.h>
#endif

#define COMPILER_PACK_PUSH   __COMPILER_PACK_PUSH
#define COMPILER_PACK_POP    __COMPILER_PACK_POP
#define COMPILER_ARRAYSIZE   __COMPILER_ARRAYSIZE
#define COMPILER_STRINGSIZE  __COMPILER_STRINGSIZE
#define COMPILER_ALIAS       __COMPILER_ALIAS
#define COMPILER_UNIQUE      __COMPILER_UNIQUE
#define STATIC_ASSERT        __STATIC_ASSERT
#define STATIC_ASSERT_M      __STATIC_ASSERT_M


#endif /* !__KOS_KERNEL_STDKERNEL_H__ */

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
#ifndef __KOS_ARCH_GENERIC_STRING_COMMON_H__
#define __KOS_ARCH_GENERIC_STRING_COMMON_H__ 1


/* Size limits used to determine string-operations large
 * enough to qualify use of a w/l/q arch-operation, even
 * if that means having to produce additional alignment code. */
#define __ARCH_STRING_LARGE_LIMIT_8  (1 << 12)
#define __ARCH_STRING_LARGE_LIMIT_4  (1 << 10)
#define __ARCH_STRING_LARGE_LIMIT_2  (1 << 5)

#define __ARCH_STRING_SMALL_LIMIT  8

#endif /* !__KOS_ARCH_GENERIC_STRING_COMMON_H__ */
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
#ifndef __KOS_ENDIAN_H__
#define __KOS_ENDIAN_H__ 1

#undef __BYTE_ORDER
#undef __LITTLE_ENDIAN
#undef __BIG_ENDIAN
#undef __PDP_ENDIAN

/* Some libraries define these macros for themself with different meaning.
 * Try not to confuse them and don't define anything is those cases. */
#ifndef __BYTE_ORDER
#if defined(__i386__) || defined(__i386) || defined(i386)\
 || defined(i486) || defined(intel) || defined(x86)\
 || defined(i86pc) || defined(__alpha) || defined(__osf__)
# define __BYTE_ORDER 1234
#elif 1
# define __BYTE_ORDER 4321
#else
# error "Unknown endian"
#endif
#endif

#define	__LITTLE_ENDIAN	1234
#define	__BIG_ENDIAN	   4321
#define	__PDP_ENDIAN	   3412

#endif /* !__KOS_ENDIAN_H__ */

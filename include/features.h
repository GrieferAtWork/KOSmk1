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
#ifndef __FEATURES_H__
#ifndef _FEATURES_H
#define __FEATURES_H__ 1
#define _FEATURES_H 1

#include <kos/compiler.h>
#include <kos/config.h>

/* TODO: These should default to 64-bit, but assembly offsets
 *      currently break (most notable: 'lldt' in 'ktask_asm.S'). */
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 32
#endif
#ifndef _TIME_T_BITS
#define _TIME_T_BITS 32
#endif

#define __HAVE_REENTRANT 1
#define __USE_ATFILE 1
#define __BSD_VISIBLE 1

#if _FILE_OFFSET_BITS >= 64
#define __USE_FILE_OFFSET64
#endif


#ifdef __LIBC_HAVE_ATEXIT
// It does work (in user-mode), but enabling this causes
// libc to be bigger, even when atexit is never used.
#define __HAVE_ATEXIT
#endif

#endif /* !_FEATURES_H */
#endif /* !__FEATURES_H__ */

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
#ifndef __KOS_KERNEL_IOFLAG_H__
#define __KOS_KERNEL_IOFLAG_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/compiler.h>
#include <kos/types.h>

__DECL_BEGIN

typedef __u8 kioflag_t;

// Unified I/O Flags
// When used, always use exactly one 'KIO_BLOCK*' value,
// that may be or'ed together with any of the other flags.

#define KIO_BLOCKNONE  0x00 /*< Don't block if no (more) data can be read/written. */
#define KIO_BLOCKFIRST 0x01 /*< Only block if no data existed when reading started/buffer was full when writing started. */
#define KIO_BLOCKALL   0x03 /*< Block until the entirety of the provided buffer is filled/written (implies behavior of 'KIO_BLOCKFIRST'). */
#define KIO_NONE       0x00 /*< This flag does nothing... */
#define KIO_PEEK       0x10 /*< Don't advance the read/write-pointer. */
#define KIO_QUICKMOVE  0x20 /*< Don't restart the read/write process if another task read the same data (May lead to data being read/written more than once).
                                NOTE: This flag may be ignored by locking I/O interfaces. */

__DECL_END

#endif /* !__KERNEL__ */

#endif /* !__KOS_KERNEL_IOFLAG_H__ */

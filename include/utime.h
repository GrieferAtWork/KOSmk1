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
#ifndef __UTIME_H__
#ifndef _UTIME_H
#define __UTIME_H__ 1
#define _UTIME_H 1

#include <features.h>
#include <kos/compiler.h>
#ifndef __ASSEMBLY__
#include <time.h>
#include <kos/types.h>

__DECL_BEGIN

struct utimbuf {
 __time_t actime;  /*< Access time. */
 __time_t modtime; /*< Modification time. */
};

#ifndef __KERNEL__
extern __nonnull((1)) int utime __P((const char *__file, struct utimbuf const *__file_times));
#endif /* !__KERNEL__ */


__DECL_END
#endif /* !__ASSEMBLY__ */

#endif /* !_UTIME_H */
#endif /* !__UTIME_H__ */

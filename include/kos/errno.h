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
#ifndef __KOS_ERRNO_H__
#define __KOS_ERRNO_H__ 1

#include <kos/compiler.h>

__DECL_BEGIN

#ifndef __ASSEMBLY__
typedef int kerrno_t;
#endif

#define KE_ISOK(e)   ((e)>=0)
#define KE_ISERR(e)  ((e)<0)
#define KE_ISSIG(e)  ((e)>0)

#define KE_OK             0
#define KE_NOMEM        (-1) /*< ERROR: Not enough dynamic memory available to complete this operation. */
#define KE_INVAL        (-2) /*< ERROR: A given argument is invalid. */
#define KE_DESTROYED    (-3) /*< ERROR: A given object was destroyed/the object's state doesn't allow for this operation. */
#define KE_TIMEDOUT     (-4) /*< ERROR: A given timeout has passed/a try-call would block. */
#define KE_RANGE        (-5) /*< ERROR: An integer/pointer lies out of its valid range. */
#define KE_OVERFLOW     (-6) /*< ERROR: An integer overflow/underflow would/did occur. */
#define KE_ACCES        (-7) /*< ERROR: Permissions were denied. */
#define KE_NOSYS        (-8) /*< ERROR: A given feature is not available. */
#define KE_LOOP         (-9) /*< ERROR: The operation would create an illegal loop of sorts. */
#define KE_DEVICE      (-10) /*< ERROR: A device responded unexpectedly, and is likely damaged, or broken. */
#define KE_NOENT       (-11) /*< ERROR: Entity not found (usually refers to a missing file/directory). */
#define KE_NODIR       (-12) /*< ERROR: Entity is not a directory. */
#define KE_NOSPC       (-13) /*< ERROR: Not enough available space (usually referring to a disk of sorts). */
#define KE_ISDIR       (-14) /*< ERROR: Entity is a directory. */
#define KE_EXISTS      (-15) /*< ERROR: Object is still in the possession of data. */
#define KE_BUFSIZ      (-16) /*< ERROR: Buffer too small. */
#define KE_BADF        (-17) /*< ERROR: Invalid (bad) file descriptor. */
#define KE_MFILE       (-18) /*< ERROR: Too many open file descriptors (NOTE: This is a kind of permissions error). */
#define KE_FAULT       (-19) /*< ERROR: A faulty pointer was given. */
#define KE_NOEXEC      (-20) /*< ERROR: Not an executable file. */
#define KE_NODEP       (-21) /*< ERROR: Missing dependency. */
#define KE_BUSY        (-22) /*< ERROR: A resource is currently busy, or otherwise in use (Usually meant as a reason why a resource could not be closed). */
#define KE_DOM         (-23) /*< ERROR: Domain error (floating point stuff...). */
#define KE_DEADLK      (-24) /*< ERROR: A deadlock (would have) occurred. */
#define KE_PERM        (-25) /*< ERROR: Operation not permitted. */
#define KE_INTR        (-26) /*< ERROR: A critical task was terminated (Send once). */
#if 1 /* Posix wants something different for this, even though I think they should be the same... */
#define KE_WOULDBLOCK  (-27) /*< ERROR: A non-blocking operation failed because it would have needed to block (e.g.: 'kmutex_trylock()'). */
#else
#define KE_WOULDBLOCK   KE_TIMEDOUT
#endif
#define KE_NOTEMPTY    (-28) /*< ERROR: An object cannot be destroyed because it is not empty. */
#define KE_NOROOT      (-29) /*< ERROR: The calling process does not have a valid root directory set. */
#define KE_NOCWD       (-30) /*< ERROR: The calling process does not have a valid cwd directory set. */
#define KE_NOFILE      (-31) /*< ERROR: Expected a file, but got a descriptor of a different type. */
#define KE_NOLNK       (-32) /*< ERROR: Expected a link, but the given file isn't one. */
#define KE_NAMETOOLONG (-33) /*< ERROR: A given filename, or part of one is too long for a selected technology. */

// NOTE: Signal codes should not be considered errors.
//       They tell the caller about some special state a given object may be in,
//       though that state only affected the behavior of the callee in some
//       way that the knowledge of might prove useful to the caller.
#define KS_UNCHANGED     1  /*< SIGNAL: Nothing/Only part of the object's state has changed. */
#define KS_EMPTY         2  /*< SIGNAL: The object in question is empty. */
#define KS_BLOCKING      3  /*< SIGNAL: The object in question is being blocked. */
#define KS_FOUND         4  /*< SIGNAL: The sought after object was found. */
#define KS_NODATA        5  /*< SIGNAL: No data was transferred (Used for v-signals). */
#define KS_FULL          6  /*< SIGNAL: A buffer is full and special behavior was invoked. */

__DECL_END

#endif /* !__KOS_ERRNO_H__ */

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
#ifndef __ERRNO_H__
#ifndef _ERRNO_H
#ifndef _INC_ERRNO
#define __ERRNO_H__ 1
#define _ERRNO_H 1
#define _INC_ERRNO 1

#include <kos/compiler.h>
#include <kos/errno.h>

__DECL_BEGIN

#ifndef __ASSEMBLY__
typedef int errno_t;
#endif /* !__ASSEMBLY__ */

/*[[[deemon
#include <file>
local names = [];
for (local line: file.open("kos/errno.h")) {
  local name;
  try name = line.scanf(" # define KE_%[^ ]")...;
  catch (...) continue;
  names.append(name);
}
local longest_name = (for (local n: names) #n) > ...;
for (local n: names) if ("(" !in n) {
  print "#define E"+n.ljust(longest_name),"(-KE_"+n+")";
}
]]]*/
#define EOK          (-KE_OK)
#define ENOMEM       (-KE_NOMEM)
#define EINVAL       (-KE_INVAL)
#define EDESTROYED   (-KE_DESTROYED)
#define ETIMEDOUT    (-KE_TIMEDOUT)
#define ERANGE       (-KE_RANGE)
#define EOVERFLOW    (-KE_OVERFLOW)
#define EACCES       (-KE_ACCES)
#define ENOSYS       (-KE_NOSYS)
#define ELOOP        (-KE_LOOP)
#define EDEVICE      (-KE_DEVICE)
#define ENOENT       (-KE_NOENT)
#define ENODIR       (-KE_NODIR)
#define ENOSPC       (-KE_NOSPC)
#define EISDIR       (-KE_ISDIR)
#define EEXISTS      (-KE_EXISTS)
#define EBUFSIZ      (-KE_BUFSIZ)
#define EBADF        (-KE_BADF)
#define EMFILE       (-KE_MFILE)
#define EFAULT       (-KE_FAULT)
#define ENOEXEC      (-KE_NOEXEC)
#define ENODEP       (-KE_NODEP)
#define EBUSY        (-KE_BUSY)
#define EDOM         (-KE_DOM)
#define EDEADLK      (-KE_DEADLK)
#define EPERM        (-KE_PERM)
#define EINTR        (-KE_INTR)
#define EWOULDBLOCK  (-KE_WOULDBLOCK)
#define EWOULDBLOCK  (-KE_WOULDBLOCK)
#define ENOTEMPTY    (-KE_NOTEMPTY)
#define ENOROOT      (-KE_NOROOT)
#define ENOCWD       (-KE_NOCWD)
#define ENOFILE      (-KE_NOFILE)
#define ENOLNK       (-KE_NOLNK)
#define ENAMETOOLONG (-KE_NAMETOOLONG)
#define EWRITABLE    (-KE_WRITABLE)
#define ECHANGED     (-KE_CHANGED)
#define ESYNTAX      (-KE_SYNTAX)
//[[[end]]]

#ifndef __ASSEMBLY__
#ifndef __CONFIG_MIN_BSS__
extern __wunused __constcall errno_t *__geterrno __P((void));
#define errno  (*__geterrno())
#endif
#endif /* !__ASSEMBLY__ */

#ifndef __ASSEMBLY__
#ifdef __CONFIG_MIN_BSS__
// Kernel-space code doesn't have errno.
// >> Instead, the kernel uses its own kerrno_t error type
//    that is usually passed through a function's return value.
#   define __get_errno()  EOK
#   define __set_errno   (void)
#elif defined(__GNUC__)
// Take advantage of cold calls to hint the compiler that
// anything setting the errno this way is unlikely code.
__forcelocal __coldcall void __set_errno __D1(errno_t,__eno) { errno = __eno; }
#   define __get_errno()         errno
#   define __set_errno     __set_errno
#else
#   define __get_errno()         errno
#   define __set_errno(v) (void)(errno = (v))
#endif

#ifndef __CONFIG_MIN_BSS__
/* Compatibility defines for MSVC's <errno.h> header */
#ifndef _CRT_ERRNO_DEFINED
#define _CRT_ERRNO_DEFINED 1
#define _errno               __geterrno
#define _set_errno(v)      (__set_errno(v),EOK)
#define _get_errno(v) (*(v)=__get_errno(),EOK)
#endif

#endif /* !__CONFIG_MIN_BSS__ */
#endif /* !__ASSEMBLY__ */

__DECL_END

#endif /* !_INC_ERRNO */
#endif /* !_ERRNO_H */
#endif /* !__ERRNO_H__ */

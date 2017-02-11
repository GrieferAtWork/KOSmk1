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
#ifndef __SYS_TYPES_H__
#define __SYS_TYPES_H__ 1

#include <kos/compiler.h>
#include <kos/types.h>

__DECL_BEGIN

#ifndef __ssize_t_defined
#define __ssize_t_defined 1
typedef __ssize_t ssize_t;
#endif

#ifndef __size_t_defined
#define __size_t_defined 1
typedef __size_t size_t;
#endif

#ifndef __int8_t_defined
#define __int8_t_defined 1
typedef __s8  int8_t;
typedef __s16 int16_t;
typedef __s32 int32_t;
typedef __s64 int64_t;
#endif

#ifndef __u_int8_t_defined
#define __u_int8_t_defined 1
typedef __u8  u_int8_t;
typedef __u16 u_int16_t;
typedef __u32 u_int32_t;
typedef __u64 u_int64_t;
#endif

#ifndef __time_t_defined
#define __time_t_defined 1
typedef __time_t time_t;
#endif

#ifndef __pid_t_defined
#define __pid_t_defined 1
typedef __pid_t pid_t;
#endif

#ifndef __dev_t_defined
#define __dev_t_defined 1
typedef __dev_t dev_t;
#endif

#ifndef __ino_t_defined
#define __ino_t_defined 1
typedef __ino_t ino_t;
#endif

#ifndef __mode_t_defined
#define __mode_t_defined 1
typedef __mode_t mode_t;
#endif

#ifndef __nlink_t_defined
#define __nlink_t_defined 1
typedef __nlink_t nlink_t;
#endif

#ifndef __uid_t_defined
#define __uid_t_defined 1
typedef __uid_t uid_t;
#endif

#ifndef __gid_t_defined
#define __gid_t_defined 1
typedef __gid_t gid_t;
#endif

#ifndef __off_t_defined
#define __off_t_defined 1
typedef __off_t off_t;
#endif

#ifndef __blksize_t_defined
#define __blksize_t_defined 1
typedef __blksize_t blksize_t;
#endif

#ifndef __blkcnt_t_defined
#define __blkcnt_t_defined 1
typedef __blkcnt_t blkcnt_t;
#endif

#ifndef __byte_t_defined
#define __byte_t_defined 1
typedef __byte_t byte_t;
#endif

#ifndef __clock_t_defined
#define __clock_t_defined 1
typedef __clock_t clock_t;
#endif
#ifndef __clockid_t_defined
#define __clockid_t_defined 1
typedef __clockid_t clockid_t;
#endif
#ifndef __fsblkcnt_t_defined
#define __fsblkcnt_t_defined 1
typedef __fsblkcnt_t fsblkcnt_t;
#endif
#ifndef __fsfilcnt_t_defined
#define __fsfilcnt_t_defined 1
typedef __fsfilcnt_t fsfilcnt_t;
#endif
#ifndef __fsblksize_t_defined
#define __fsblksize_t_defined 1
typedef __fsblksize_t fsblksize_t;
#endif
#ifndef __id_t_defined
#define __id_t_defined 1
typedef __id_t id_t;
#endif
#ifndef __key_t_defined
#define __key_t_defined 1
typedef __key_t key_t;
#endif
#ifndef __pthread_t_defined
#define __pthread_t_defined 1
typedef __pthread_t            pthread_t;
typedef __pthread_attr_t       pthread_attr_t;
typedef __pthread_cond_t       pthread_cond_t;
typedef __pthread_condattr_t   pthread_condattr_t;
typedef __pthread_key_t        pthread_key_t;
typedef __pthread_mutex_t      pthread_mutex_t;
typedef __pthread_mutexattr_t  pthread_mutexattr_t;
typedef __pthread_once_t       pthread_once_t;
typedef __pthread_rwlock_t     pthread_rwlock_t;
typedef __pthread_rwlockattr_t pthread_rwlockattr_t;
#endif
#ifndef __suseconds_t_defined
#define __suseconds_t_defined 1
typedef __suseconds_t suseconds_t;
#endif
#ifndef __timer_t_defined
#define __timer_t_defined 1
typedef __timer_t timer_t;
#endif
#ifndef __useconds_t_defined
#define __useconds_t_defined 1
typedef __useconds_t useconds_t;
#endif

#ifndef __STDC_PURE__
#ifndef __pos_t_defined
#define __pos_t_defined 1
typedef __pos_t pos_t; /*< Position within a file/stream. */
#endif
#ifndef __openmode_t_defined
#define __openmode_t_defined 1
typedef __openmode_t openmode_t; /*< Mode used to open a file ('O_*' constants from <fcntl.h>). */
#endif
#endif /* __STDC_PURE__ */

__DECL_END

#endif /* !__SYS_TYPES_H__ */

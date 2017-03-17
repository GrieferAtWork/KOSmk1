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
#ifndef __KOS_TYPES_H__
#define __KOS_TYPES_H__ 1

#include <kos/compiler.h>
#ifndef __ASSEMBLY__
#include <kos/arch.h>
#include <features.h>

__DECL_BEGIN

// TODO: Confirm sizes
#ifdef __INT8_TYPE__
typedef __INT8_TYPE__   __s8;
#else
typedef signed char     __s8;
#endif
#ifdef __INT16_TYPE__
typedef __INT16_TYPE__  __s16;
#else
typedef signed short    __s16;
#endif
#ifdef __INT32_TYPE__
typedef __INT32_TYPE__  __s32;
#else
typedef signed int      __s32;
#endif
#ifdef __INT64_TYPE__
typedef __INT64_TYPE__  __s64;
#else
__extension__ typedef signed long long __s64;
#endif
#ifdef __UINT8_TYPE__
typedef __UINT8_TYPE__  __u8;
#else
typedef unsigned char   __u8;
#endif
#ifdef __UINT16_TYPE__
typedef __UINT16_TYPE__ __u16;
#else
typedef unsigned short  __u16;
#endif
#ifdef __UINT32_TYPE__
typedef __UINT32_TYPE__ __u32;
#else
typedef unsigned int    __u32;
#endif
#ifdef __UINT64_TYPE__
typedef __UINT64_TYPE__ __u64;
#else
__extension__ typedef unsigned long long __u64;
#endif

#define __PRIVATE_SX_1 __s8
#define __PRIVATE_SX_2 __s16
#define __PRIVATE_SX_4 __s32
#define __PRIVATE_SX_8 __s64
#define __PRIVATE_UX_1 __u8
#define __PRIVATE_UX_2 __u16
#define __PRIVATE_UX_4 __u32
#define __PRIVATE_UX_8 __u64
#define __PRIVATE_CAT(a,b) a##b

#define __sx(size) __PRIVATE_CAT(__PRIVATE_SX_,size)
#define __ux(size) __PRIVATE_CAT(__PRIVATE_UX_,size)
#define __sn(bits) __PRIVATE_CAT(__s,bits)
#define __un(bits) __PRIVATE_CAT(__u,bits)

#ifdef __INTELLISENSE__
typedef ____INTELLISENE_integer<1234,__u16> __le16;
typedef ____INTELLISENE_integer<4321,__u16> __be16;
typedef ____INTELLISENE_integer<1234,__u32> __le32;
typedef ____INTELLISENE_integer<4321,__u32> __be32;
typedef ____INTELLISENE_integer<1234,__u64> __le64;
typedef ____INTELLISENE_integer<4321,__u64> __be64;
#else
typedef __u16 __bitwise __le16;
typedef __u16 __bitwise __be16;
typedef __u32 __bitwise __le32;
typedef __u32 __bitwise __be32;
typedef __u64 __bitwise __le64;
typedef __u64 __bitwise __be64;
#endif

#ifndef __SIZEOF_POINTER__
#if defined(__i386__)
#define __SIZEOF_POINTER__ 4
#elif defined(__x86_64__)
#define __SIZEOF_POINTER__ 8
#else
#define __SIZEOF_POINTER__ 4 /*< Guess. */
#endif
#endif


#ifdef __cplusplus
#define __true             true
#define __false            false
typedef bool               __bool_t;
#elif 1
#define __true             1
#define __false            0
typedef _Bool              __bool_t;
#else
#define __true             1
#define __false            0
typedef unsigned char      __bool_t;
#endif

typedef char               __char_t;
typedef signed char        __schar_t;
typedef unsigned char      __uchar_t;
typedef short              __short_t;
typedef unsigned short     __ushort_t;
typedef int                __int_t;
typedef unsigned int       __uint_t;
typedef long               __long_t;
typedef unsigned long      __ulong_t;
typedef long long          __llong_t;
typedef unsigned long long __ullong_t;

#ifdef __CHAR16_TYPE__
typedef __CHAR16_TYPE__ __char16_t;
#else
typedef __u16           __char16_t;
#endif
#ifdef __CHAR32_TYPE__
typedef __CHAR32_TYPE__ __char32_t;
#else
typedef __u32           __char32_t;
#endif
#ifndef __INTELLISENSE__
#ifdef __WCHAR_TYPE__
typedef __WCHAR_TYPE__ __wchar_t;
#elif defined(_WIN32) || defined(WIN32)
typedef __u16          __wchar_t;
#else
typedef __u32          __wchar_t;
#endif
#endif
#ifdef __INT_LEAST8_TYPE__
typedef __INT_LEAST8_TYPE__ __int_least8_t;
#else
typedef __s8                __int_least8_t;
#endif
#ifdef __UINT_LEAST8_TYPE__
typedef __UINT_LEAST8_TYPE__ __uint_least8_t;
#else
typedef __u8                 __uint_least8_t;
#endif
#ifdef __INT_LEAST16_TYPE__
typedef __INT_LEAST16_TYPE__ __int_least16_t;
#else
typedef __s16                __int_least16_t;
#endif
#ifdef __UINT_LEAST16_TYPE__
typedef __UINT_LEAST16_TYPE__ __uint_least16_t;
#else
typedef __u16                __uint_least16_t;
#endif
#ifdef __INT_LEAST32_TYPE__
typedef __INT_LEAST32_TYPE__ __int_least32_t;
#else
typedef __s32                __int_least32_t;
#endif
#ifdef __UINT_LEAST32_TYPE__
typedef __UINT_LEAST32_TYPE__ __uint_least32_t;
#else
typedef __u32                 __uint_least32_t;
#endif
#ifdef __INT_LEAST64_TYPE__
typedef __INT_LEAST64_TYPE__ __int_least64_t;
#else
typedef __s64                __int_least64_t;
#endif
#ifdef __UINT_LEAST64_TYPE__
typedef __UINT_LEAST64_TYPE__ __uint_least64_t;
#else
typedef __u64                 __uint_least64_t;
#endif
#ifdef __INT_FAST8_TYPE__
typedef __INT_FAST8_TYPE__ __int_fast8_t;
#else
typedef __s8               __int_fast8_t;
#endif
#ifdef __UINT_FAST8_TYPE__
typedef __UINT_FAST8_TYPE__ __uint_fast8_t;
#else
typedef __u8                __uint_fast8_t;
#endif
#ifdef __INT_FAST16_TYPE__
typedef __INT_FAST16_TYPE__ __int_fast16_t;
#else
typedef __s32               __int_fast16_t;
#endif
#ifdef __UINT_FAST16_TYPE__
typedef __UINT_FAST16_TYPE__ __uint_fast16_t;
#else
typedef __u32                __uint_fast16_t;
#endif
#ifdef __INT_FAST32_TYPE__
typedef __INT_FAST32_TYPE__ __int_fast32_t;
#else
typedef __s32               __int_fast32_t;
#endif
#ifdef __UINT_FAST32_TYPE__
typedef __UINT_FAST32_TYPE__ __uint_fast32_t;
#else
typedef __u32               __uint_fast32_t;
#endif
#ifdef __INT_FAST64_TYPE__
typedef __INT_FAST64_TYPE__ __int_fast64_t;
#else
typedef __s64               __int_fast64_t;
#endif
#ifdef __UINT_FAST64_TYPE__
typedef __UINT_FAST64_TYPE__ __uint_fast64_t;
#else
typedef __u64                __uint_fast64_t;
#endif
#ifdef __INTPTR_TYPE__
typedef __INTPTR_TYPE__          __intptr_t;
#else
typedef __sx(__SIZEOF_POINTER__) __intptr_t;
#endif
#ifdef __UINTPTR_TYPE__
typedef __UINTPTR_TYPE__         __uintptr_t;
#else
typedef __ux(__SIZEOF_POINTER__) __uintptr_t;
#endif
#ifdef __SIZE_TYPE__
typedef __SIZE_TYPE__ __size_t;
#else
typedef __uintptr_t   __size_t;
#endif
#ifdef __PTRDIFF_TYPE__
typedef __PTRDIFF_TYPE__ __ptrdiff_t;
#else
typedef __intptr_t       __ptrdiff_t;
#endif
#ifdef __INTMAX_TYPE__
typedef __INTMAX_TYPE__ __intmax_t;
#else
typedef __s64           __intmax_t;
#endif
#ifdef __UINTMAX_TYPE__
typedef __UINTMAX_TYPE__ __uintmax_t;
#else
typedef __u64            __uintmax_t;
#endif

typedef long double    __max_align_t;
typedef __ptrdiff_t    __ssize_t;
typedef __u8           __byte_t;
#if 1
typedef __u32          __time32_t;
typedef __u64          __time64_t;
#else
typedef __s32          __time32_t;
typedef __s64          __time64_t;
#endif
typedef __s32          __pid_t;  /*< Task/Process id. */
typedef unsigned int   __mode_t;
typedef __u32          __dev_t;
typedef __u32          __nlink_t;
typedef __u32          __uid_t;
typedef __u32          __gid_t;
typedef __u32          __ino_t;
typedef int            __openmode_t;
typedef __s64          __loff_t;
typedef __s32          __off32_t;
typedef __s64          __off64_t;
typedef __u32          __pos32_t;
typedef __u64          __pos64_t;
typedef __s32          __blksize32_t;
typedef __s64          __blksize64_t;
typedef __s32          __blkcnt32_t;
typedef __s64          __blkcnt64_t;
typedef __u32          __fsblksize32_t;
typedef __u64          __fsblksize64_t;
typedef __u32          __fsblkcnt32_t;
typedef __u64          __fsblkcnt64_t;
typedef __u32          __fsfilcnt32_t;
typedef __u32          __fsfilcnt64_t;
typedef __u32          __useconds_t;
typedef __s32          __suseconds_t;
typedef unsigned int   __socklen_t; /*< Keep this type's size equal to sizeof(int) for compatibility. */
typedef __u16          __sa_family_t;

// v types of these must be integral, but
//   their sign and size may be changed.
typedef __u32 __clock_t;
typedef __u32 __clockid_t;
typedef __u32 __id_t;
typedef __u32 __timer_t;

#ifdef __KERNEL__
typedef __uintptr_t __pthread_t; /*< 'struct ktask *'. */
#else
typedef int         __pthread_t; /*< File descriptor. */
#endif


typedef struct __key                 __key_t;
typedef struct __pthread_attr        __pthread_attr_t;
typedef struct __pthread_cond        __pthread_cond_t;
typedef struct __pthread_condattr    __pthread_condattr_t;
typedef __size_t                     __pthread_key_t;
typedef struct __pthread_mutex       __pthread_mutex_t;
typedef struct __pthread_mutexattr   __pthread_mutexattr_t;
typedef struct __pthread_once        __pthread_once_t;
typedef struct __pthread_rwlock      __pthread_rwlock_t;
typedef struct __pthread_rwlockattr  __pthread_rwlockattr_t;
typedef struct __pthread_spinlock    __pthread_spinlock_t;
typedef struct __pthread_barrierattr __pthread_barrierattr_t;
typedef struct __pthread_barrier     __pthread_barrier_t;


#ifdef __USE_FILE_OFFSET64
typedef __off64_t       __off_t;
typedef __pos64_t       __pos_t;
typedef __blksize64_t   __blksize_t;
typedef __blkcnt64_t    __blkcnt_t;
typedef __fsblksize64_t __fsblksize_t;
typedef __fsblkcnt64_t  __fsblkcnt_t;
typedef __fsfilcnt64_t  __fsfilcnt_t;
#else
typedef __off32_t       __off_t;
typedef __pos32_t       __pos_t;
typedef __blksize32_t   __blksize_t;
typedef __blkcnt32_t    __blkcnt_t;
typedef __fsblksize32_t __fsblksize_t;
typedef __fsblkcnt32_t  __fsblkcnt_t;
typedef __fsfilcnt32_t  __fsfilcnt_t;
#endif

#if _TIME_T_BITS == 64
typedef __time64_t  __time_t;
#else
typedef __time32_t  __time_t;
#endif

typedef __s16    __ktaskprio_t;
typedef __u16    __kfdtype_t;
#define __KTID_INVALID   ((__ktid_t)-1)
typedef __size_t __ktid_t;
typedef __size_t __ktls_t;
typedef int      __ksandbarrier_t;
typedef __size_t __kmodid_t;
typedef __u32    __ktaskopflag_t;


__DECL_END
#endif /* !__ASSEMBLY__ */

#if _TIME_T_BITS == 64
#define __SIZEOF_TIME_T  8
#else
#define __SIZEOF_TIME_T  4
#endif

#endif /* !__KOS_TYPES_H__ */

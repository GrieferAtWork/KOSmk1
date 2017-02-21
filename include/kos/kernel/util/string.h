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
#ifndef __KOS_KERNEL_UTIL_STRING_H__
#define __KOS_KERNEL_UTIL_STRING_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/compiler.h>
#include <kos/types.h>
#ifndef __INTELLISENSE__
#include <kos/kernel/task.h>
#include <kos/kernel/proc.h>
#endif

__DECL_BEGIN

//////////////////////////////////////////////////////////////////////////
// Get/Set memory within user space
// @return: 0 : Memory was successfully copied.
// @return: * : Amount of bytes that could not be copied.
#ifdef __INTELLISENSE__
extern __nonnull((2)) __wunused __size_t copy_from_user(__kernel void *dst, __user void const *src, __size_t bytes);
extern __nonnull((2)) __wunused __size_t copy_to_user(__user void *dst, __kernel void const *__restrict src, __size_t bytes);
extern __nonnull((2)) __wunused __size_t copy_in_user(__user void *dst, __user void const *src, __size_t bytes);
#else
extern __crit __nonnull((2)) __wunused __size_t __copy_from_user_c(__kernel void *dst, __user void const *src, __size_t bytes);
extern __crit __nonnull((2)) __wunused __size_t __copy_to_user_c(__user void *dst, __kernel void const *__restrict src, __size_t bytes);
extern __crit __nonnull((2)) __wunused __size_t __copy_in_user_c(__user void *dst, __user void const *src, __size_t bytes);
#define __copy_from_user(dst,src,bytes) KTASK_CRIT(__copy_from_user_c(dst,src,bytes))
#define __copy_to_user(dst,src,bytes)   KTASK_CRIT(__copy_to_user_c(dst,src,bytes))
#define __copy_in_user(dst,src,bytes)   KTASK_CRIT(__copy_in_user_c(dst,src,bytes))
#define copy_from_user(dst,src,bytes)  (KTASK_ISKEPD_P ? (memcpy(dst,src,bytes),0) : __copy_from_user(dst,src,bytes))
#define copy_to_user(dst,src,bytes)    (KTASK_ISKEPD_P ? (memcpy(dst,src,bytes),0) : __copy_to_user(dst,src,bytes))
#define copy_in_user(dst,src,bytes)    (KTASK_ISKEPD_P ? (memcpy(dst,src,bytes),0) : __copy_in_user(dst,src,bytes))
#endif


#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// 100% safe user string-style functions.
extern __wunused __user void *user_memchr(__user void const *p, int needle, size_t bytes);
extern __wunused __size_t user_strlen(__user char const *s);
extern __wunused __size_t user_strnlen(__user char const *s, __size_t maxchars);
extern __crit __wunused __malloccall __kernel char *user_strndup(__user char const *s, __size_t maxchars);
#else
extern __crit __wunused __user void *__user_memchr_c(__user void const *p, int needle, size_t bytes);
extern __crit __wunused __size_t __user_strlen_c(__user char const *s);
extern __crit __wunused __size_t __user_strnlen_c(__user char const *s, __size_t maxchars);
extern __crit __wunused __malloccall __kernel char *__user_strndup_c(__user char const *s, __size_t maxchars);
#define __user_memchr(p,needle,bytes) KTASK_CRIT(__user_memchr_c(p,needle,bytes))
#define __user_strlen(s)              KTASK_CRIT(__user_strlen_c(s))
#define __user_strnlen(s,maxchars)    KTASK_CRIT(__user_strnlen_c(s,maxchars))
#define __user_strndup(s,maxchars)    KTASK_CRIT(__user_strndup_c(s,maxchars))
#define user_memchr(p,needle,bytes) (KTASK_ISKEPD_P ? memchr(p,needle,bytes) : __user_memchr(p,needle,bytes))
#define user_strlen(s)              (KTASK_ISKEPD_P ? strlen(s) : __user_strlen(s))
#define user_strnlen(s,maxchars)    (KTASK_ISKEPD_P ? strnlen(s,maxchars) : __user_strnlen(s,maxchars))
#define user_strndup(s,maxchars)    (KTASK_ISKEPD_P ? strndup(s,maxchars) : __user_strndup(s,maxchars))
#endif




__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_UTIL_STRING_H__ */

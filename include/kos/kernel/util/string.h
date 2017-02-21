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
#include <kos/errno.h>
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
// Returns the amount of bytes not set
extern __wunused __size_t user_memset(__user void const *p, int byte, __size_t bytes);
#else
extern __wunused __size_t __user_memset_c(__user void const *p, int byte, __size_t bytes);
#define __user_memset(p,byte,bytes) KTASK_CRIT(__user_memset_c(p,byte,bytes))
#define user_memset(p,byte,bytes)  (KTASK_ISKEPD_P ? (memset(p,byte,bytes),0) : __user_memset(p,byte,bytes))
#endif

#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// 100% safe user string-style functions.
extern __wunused __user void *user_memchr(__user void const *p, int needle, __size_t bytes);
extern __wunused __size_t user_strlen(__user char const *s);
extern __wunused __size_t user_strnlen(__user char const *s, __size_t maxchars);
extern __crit __wunused __malloccall __kernel char *user_strndup(__user char const *s, __size_t maxchars);
#else
extern __crit __wunused __user void *__user_memchr_c(__user void const *p, int needle, __size_t bytes);
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

//////////////////////////////////////////////////////////////////////////
// Iterate all parts of a user-space pointer:
// >> extern void parse_user_memory(uintptr_t start_offset, __kernel void *addr, size_t size);
// >> extern void handle_fault(void);
// >> 
// >> __user   void *user_p; size_t user_s;
// >> __kernel void *part_p; size_t part_s;
// >> user_p = get_user_pointer();
// >> user_s = get_user_size();
// >> USER_FOREACH_BEGIN(user_p,user_s,part_p,part_s) {
// >>   parse_user_memory(USER_FOREACH_OFFSET,part_p,part_s);
// >> } USER_FOREACH_END({
// >>   handle_fault();
// >> });


/* The user-space address of the current kernel part. */
#define USER_FOREACH_UADDR    (__uf_p)

/* The offset (from the start) of the current kernel pointer/user address
 * (aka. how many bytes were already handled in previous iterations). */
#define USER_FOREACH_OFFSET   ((__uintptr_t)__uf_p-(__uintptr_t)__uf_start)

/* The amount of pending bytes. */
#define USER_FOREACH_PENDING  (__uf_s)

/* Begin a user-pointer foreach-part iteration. */
#define USER_FOREACH_BEGIN(p,s,part_p,part_s,writeable) \
do{ struct ktask *const __uf_caller = ktask_self();\
    struct kproc *const __uf_proc = ktask_getproc(__uf_caller);\
    struct kpagedir const *const __uf_epd = __uf_caller->t_epd;\
    int const __uf_kepd = KTASK_ISKEPD_P || __uf_epd == kpagedir_kernel();\
    __user void *__uf_p; __user __attribute_unused void *__uf_start;\
    __uf_p = __uf_start = (__user void *)(p);\
    __size_t __uf_part,__uf_s = (s);\
    KTASK_CRIT_BEGIN\
    if __likely(__uf_kepd || KE_ISOK(kproc_lock(__uf_proc,KPROC_LOCK_SHM))) {\
     while (__uf_s && (__uf_kepd\
        ? ( *(__kernel void **)&(part_p) = __uf_p,__uf_part = __uf_s,1)\
        : ((*(__kernel void **)&(part_p) = \
              kshm_translateuser(kproc_getshm(__uf_proc),\
                                 __uf_epd,\
                                 __uf_p,__uf_s,&__uf_part,\
                                 writeable)) != NULL))) {\
      assert(__uf_part);\
      assert(__uf_part <= __uf_s);\
      (part_s) = __uf_part;\
      if (1)

/* End a user-pointer foreach-part iteration. */
#define USER_FOREACH_END(...) \
      else;\
      __uf_s -= __uf_part;\
      *(__uintptr_t *)&__uf_p += __uf_part;\
     }\
     if (!__uf_kepd) kproc_unlock(__uf_proc,KPROC_LOCK_SHM);\
    }\
    KTASK_CRIT_END\
    if (__uf_s) {__VA_ARGS__;}\
}while(0)


__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_UTIL_STRING_H__ */

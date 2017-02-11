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

__DECL_BEGIN

//////////////////////////////////////////////////////////////////////////
// Get/Set memory within user space
// @return: 0 : Memory was successfully read/written.
// @return: * : Amount of bytes that could not be copied.
extern __crit __nonnull((2)) __wunused __size_t u_getmem_c(__user void const *addr, __kernel void *__restrict buf, __size_t bufsize);
extern __crit __nonnull((2)) __wunused __size_t u_setmem_c(__user void *addr, __kernel void const *__restrict buf, __size_t bufsize);
#ifdef __INTELLISENSE__
extern __nonnull((2)) __wunused __size_t u_getmem(__user void const *addr, __kernel void *__restrict buf, __size_t bufsize);
extern __nonnull((2)) __wunused __size_t u_setmem(__user void *addr, __kernel void const *__restrict buf, __size_t bufsize);
#else
#define u_getmem(addr,buf,bufsize) KTASK_CRIT(u_getmem_c(addr,buf,bufsize))
#define u_setmem(addr,buf,bufsize) KTASK_CRIT(u_setmem_c(addr,buf,bufsize))
#endif

#define u_setT(addr,T,value) \
 __xblock({ T const __svalue = (value);\
            __xreturn u_setmem(addr,&__svalue,sizeof(T));\
 })
#define u_get(addr,result) \
 u_getmem(addr,&(result),sizeof(result))
#define u_set(addr,value) \
 __xblock({ __typeof__(value) const __svalue = (value);\
            __xreturn u_setmem(addr,&__svalue,sizeof(__svalue));\
 })
#define u_setl(addr,value) \
 u_setmem(addr,&(value),sizeof(value))


extern __crit __wunused __size_t u_strlen_c(__user char const *s);
extern __crit __wunused __size_t u_strnlen_c(__user char const *s, __size_t maxchars);
#ifdef __INTELLISENSE__
extern __wunused __size_t u_strlen(__user char const *s);
extern __wunused __size_t u_strnlen(__user char const *s, __size_t maxchars);
#else
#define u_strlen(s)           KTASK_CRIT(u_strlen_c(s))
#define u_strnlen(s,maxchars) KTASK_CRIT(u_strnlen_c(s,maxchars))
#endif

extern __crit __wunused __malloccall __kernel char *
u_strndup_c(__user char const *s, __size_t maxchars);

/* Safely iterate user memory:
 * >> {
 * >>   int result = 0;
 * >>   __kernel void *kernel_p; size_t kernel_s;
 * >>   U_ITERATE_BEGIN(user_p,user_s,kernel_p,kernel_s,read_write,{ handle_fault(); }) {
 * >>     
 * >>     result += parse_memory(kernel_p,kernel_s);
 * >>     
 * >>   } U_ITERATE_END
 * >>   return result;
 * >> }
 */
#define U_ITERATE_BEGIN(user_p,user_s,kernel_part_p,kernel_part_s,rw,...) \
{ __u8 *__user_p = (__u8 *)(user_p);\
  __size_t __partsize,__user_s = (user_s);\
  struct kproc *caller = kproc_self();\
  KTASK_CRIT_BEGIN\
  if __likely(KE_ISOK(kproc_lock(caller,KPROC_LOCK_SHM))) {\
   for (;;) {\
    __partsize = __user_s;\
    if ((*(void **)&(kernel_part_p) = kshm_translate_u(&caller->p_shm,__user_p,&__partsize,rw)\
         ) == NULL) {{__VA_ARGS__;}break;}\
    (kernel_part_s) = __partsize;

#define U_ITERATE_END \
    if (__user_s == __partsize) break;\
    assert(__user_s > __partsize);\
    __user_p += __partsize;\
    __user_s -= __partsize;\
   }\
   kproc_unlock(caller,KPROC_LOCK_SHM);\
  }\
  KTASK_CRIT_END\
}


__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_UTIL_STRING_H__ */

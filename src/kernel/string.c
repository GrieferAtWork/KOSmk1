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
#ifndef __KOS_KERNEL_STRING_C__
#define __KOS_KERNEL_STRING_C__ 1

#include <kos/compiler.h>
#include <kos/kernel/proc.h>
#include <kos/kernel/paging.h>
#include <kos/kernel/util/string.h>
#include <string.h>

__DECL_BEGIN

__crit size_t
__copy_from_user_c(__kernel void *__restrict dst,
                   __user void const *src,
                   size_t bytes) {
 size_t maxbytes; __kernel void const *kpt;
 KTASK_CRIT_MARK
 USER_FOREACH_BEGIN(src,bytes,kpt,maxbytes,0) {
  memcpy(dst,kpt,maxbytes);
 } USER_FOREACH_END({
  return USER_FOREACH_PENDING;
 });
 return 0;
}
__crit size_t
__copy_to_user_c(__user void *dst,
                 __kernel void const *__restrict src,
                 size_t bytes) {
 size_t maxbytes; __kernel void *kptr;
 KTASK_CRIT_MARK
 USER_FOREACH_BEGIN(dst,bytes,kptr,maxbytes,1) {
  memcpy(kptr,src,maxbytes);
 } USER_FOREACH_END({
  return USER_FOREACH_PENDING;
 });
 return 0;
}
__crit size_t
__copy_in_user_c(__user void *dst,
                 __user void const *src,
                 size_t bytes) {
 size_t max_dst,max_src; __kernel void *kdst,*ksrc;
 struct ktask *caller = ktask_self();
 struct kproc *proc = ktask_getproc(caller);
 KTASK_CRIT_MARK
 if (caller->t_epd == kpagedir_kernel()) { memcpy(dst,src,bytes); return 0; }
 if __likely(KE_ISOK(kproc_lock(proc,KPROC_LOCK_SHM))) {
  while (bytes &&
        (kdst = kshm_translateuser(kproc_getshm(proc),caller->t_epd,dst,bytes,&max_dst,1)) != NULL &&
        (ksrc = kshm_translateuser(kproc_getshm(proc),caller->t_epd,src,max_dst,&max_src,0)) != NULL) {
   memcpy(kdst,ksrc,max_src);
   bytes -= max_src;
   *(uintptr_t *)&dst += max_src;
   *(uintptr_t *)&src += max_src;
  }
  kproc_unlock(proc,KPROC_LOCK_SHM);
 }
 return bytes;
}


__wunused size_t
__user_memset_c(__user void const *p, int byte, size_t bytes) {
 size_t maxbytes; __kernel void *kptr;
 KTASK_CRIT_MARK
 USER_FOREACH_BEGIN(p,bytes,kptr,maxbytes,1) {
  memset(kptr,byte,maxbytes);
 } USER_FOREACH_END({
  return USER_FOREACH_PENDING;
 });
 return 0;
}

/* TODO: Everything in this file should use 'USER_FOREACH'! */

__crit __user void *
__user_memchr_c(__user void const *p, int needle, size_t bytes) {
 size_t bytes_max; __kernel void *kp;
 __user void *result = NULL,*iter = (void *)p;
 struct ktask *caller = ktask_self();
 struct kproc *proc = ktask_getproc(caller);
 KTASK_CRIT_MARK
 if __unlikely(KE_ISERR(kproc_lock(proc,KPROC_LOCK_SHM))) return NULL;
 while ((kp = kshm_translateuser(kproc_getshm(proc),caller->t_epd,
                                 iter,bytes,&bytes_max,0)) != NULL) {
  assert(bytes_max <= bytes);
  if ((result = memchr(kp,needle,bytes_max)) != NULL) {
   /* Convert the kernel-pointer into a user-pointer. */
   result = (__user void *)((uintptr_t)p+((uintptr_t)result-(uintptr_t)kp));
   break;
  }
  if ((bytes -= bytes_max) == 0) break;
  *(uintptr_t *)&iter += bytes_max;
 }
 kproc_unlock(proc,KPROC_LOCK_SHM);
 return result;
}

__crit __kernel char *
__user_strndup_c(__user char const *s, size_t maxchars) {
 size_t bufsize; char *result;
 KTASK_CRIT_MARK
 bufsize = user_strnlen(s,maxchars);
 result = (char *)malloc((bufsize+1)*sizeof(char));
 if __unlikely(!result) return NULL;
 bufsize -= copy_from_user(result,s,bufsize);
 result[bufsize] = '\0';
#if 0
 {
  // It is possible that the user screwed with their memory to truncate the string
  // >> We can do the same to possibly same memory.
  // NOTE: This isn't really required
  size_t real_size = strlen(result);
  assert(real_size <= bufsize);
  if (real_size != bufsize) {
   char *newresult = (char *)realloc(result,(real_size+1)*sizeof(char));
   if (newresult) result = newresult;
  }
 }
#endif
 return result;
}

#define OPERATION_BUFFERSIZE  64


__crit size_t __user_strlen_c(__user char const *s) {
 struct ktask *caller = ktask_self();
 struct kproc *proc = ktask_getproc(caller);
 size_t result = 0,partmaxsize,partsize; char *addr;
 KTASK_CRIT_MARK
 if __unlikely(KE_ISERR(kproc_lock(proc,KPROC_LOCK_SHM))) return 0;
 for (;;) {
  addr = (char *)kshm_translateuser(kproc_getshm(proc),caller->t_epd,
                                    s,PAGESIZE,&partmaxsize,0);
  if __unlikely(!addr) break; /* FAULT */
  partmaxsize /= sizeof(char);
  partsize = strnlen(addr,partmaxsize);
  result += partsize;
  if (partmaxsize != partsize) break;
  *(uintptr_t *)&s += partmaxsize;
 }
 kproc_unlock(proc,KPROC_LOCK_SHM);
 return result;
}
__crit size_t __user_strnlen_c(__user char const *s, size_t maxchars) {
 struct ktask *caller = ktask_self();
 struct kproc *proc = ktask_getproc(caller);
 size_t result = 0,partmaxsize,partsize; char *addr;
 KTASK_CRIT_MARK
 if __unlikely(KE_ISERR(kproc_lock(proc,KPROC_LOCK_SHM))) return 0;
 for (;;) {
  addr = (char *)kshm_translateuser(kproc_getshm(proc),caller->t_epd,
                                    s,maxchars,&partmaxsize,0);
  if __unlikely(!addr) break; /* FAULT */
  partmaxsize /= sizeof(char);
  partsize = strnlen(addr,min(maxchars,partmaxsize))*sizeof(char);
  result += partsize;
  if (partmaxsize != (partsize/sizeof(char)) ||
      maxchars == partsize) break;
  *(uintptr_t *)&s += partmaxsize;
  maxchars -= partsize;
 }
 kproc_unlock(proc,KPROC_LOCK_SHM);
 return result/sizeof(char);
}



__DECL_END

#endif /* !__KOS_KERNEL_STRING_C__ */

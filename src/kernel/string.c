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
 size_t result;
 struct kproc *proc = kproc_self();
 KTASK_CRIT_MARK
 if __unlikely(KE_ISERR(kproc_lock(proc,KPROC_LOCK_SHM))) return 0;
 result = kshm_copyfromuser(kproc_getshm(proc),dst,src,bytes);
 kproc_unlock(proc,KPROC_LOCK_SHM);
 return result;
}
__crit size_t
__copy_to_user_c(__user void *dst,
                 __kernel void const *__restrict src,
                 size_t bytes) {
 size_t result;
 struct kproc *caller = kproc_self();
 KTASK_CRIT_MARK
 if __unlikely(KE_ISERR(kproc_lock(caller,KPROC_LOCK_SHM))) return 0;
 result = kshm_copytouser(kproc_getshm(caller),dst,src,bytes);
 kproc_unlock(caller,KPROC_LOCK_SHM);
 return result;
}
__crit size_t
__copy_in_user_c(__user void *dst,
                 __user void const *__restrict src,
                 size_t bytes) {
 size_t result;
 struct kproc *caller = kproc_self();
 KTASK_CRIT_MARK
 if __unlikely(KE_ISERR(kproc_lock(caller,KPROC_LOCK_SHM))) return 0;
 result = kshm_copyinuser(kproc_getshm(caller),dst,src,bytes);
 kproc_unlock(caller,KPROC_LOCK_SHM);
 return result;
}



__crit __user void *
__user_memchr_c(__user void const *p, int needle, size_t bytes) {
 size_t bytes_max; __kernel void *kp;
 __user void *result = NULL,*iter = (void *)p;
 struct kproc *caller = kproc_self();
 KTASK_CRIT_MARK
 if __unlikely(KE_ISERR(kproc_lock(caller,KPROC_LOCK_SHM))) return NULL;
 while ((kp = kshm_translateuser(kproc_getshm(caller),iter,bytes,&bytes_max,0)) != NULL) {
  assert(bytes_max <= bytes);
  if ((result = memchr(kp,needle,bytes_max)) != NULL) {
   /* Convert the kernel-pointer into a user-pointer. */
   result = (__user void *)((uintptr_t)p+((uintptr_t)result-(uintptr_t)kp));
   break;
  }
  if ((bytes -= bytes_max) == 0) break;
  *(uintptr_t *)&iter += bytes_max;
 }
 kproc_unlock(caller,KPROC_LOCK_SHM);
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
 struct kproc *caller = kproc_self();
 size_t result = 0,partmaxsize,partsize; char *addr;
 KTASK_CRIT_MARK
 if __unlikely(KE_ISERR(kproc_lock(caller,KPROC_LOCK_SHM))) return 0;
 for (;;) {
  addr = (char *)kshm_translateuser(kproc_getshm(caller),s,PAGESIZE,&partmaxsize,0);
  if __unlikely(!addr) break; /* FAULT */
  partmaxsize /= sizeof(char);
  partsize = strnlen(addr,partmaxsize);
  result += partsize;
  if (partmaxsize != partsize) break;
  *(uintptr_t *)&s += partmaxsize;
 }
 kproc_unlock(caller,KPROC_LOCK_SHM);
 return result;
}
__crit size_t __user_strnlen_c(__user char const *s, size_t maxchars) {
 struct kproc *caller = kproc_self();
 size_t result = 0,partmaxsize,partsize; char *addr;
 KTASK_CRIT_MARK
 if __unlikely(KE_ISERR(kproc_lock(caller,KPROC_LOCK_SHM))) return 0;
 for (;;) {
  addr = (char *)kshm_translateuser(kproc_getshm(caller),s,maxchars,&partmaxsize,0);
  if __unlikely(!addr) break; /* FAULT */
  partmaxsize /= sizeof(char);
  partsize = strnlen(addr,min(maxchars,partmaxsize))*sizeof(char);
  result += partsize;
  if (partmaxsize != (partsize/sizeof(char)) ||
      maxchars == partsize) break;
  *(uintptr_t *)&s += partmaxsize;
  maxchars -= partsize;
 }
 kproc_unlock(caller,KPROC_LOCK_SHM);
 return result/sizeof(char);
}



__DECL_END

#endif /* !__KOS_KERNEL_STRING_C__ */

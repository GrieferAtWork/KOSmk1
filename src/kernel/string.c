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
u_getmem_c(__user void const *addr,
           __kernel void *__restrict buf,
           size_t bufsize) {
 size_t result; struct kproc *proc;
 struct ktask *caller = ktask_self();
 KTASK_CRIT_MARK
 //if (caller->t_currpd == kpagedir_kernel()) { memcpy(buf,addr,bufsize); return 0; }
 proc = ktask_getproc(caller);
 if __unlikely(KE_ISERR(kproc_lock(proc,KPROC_LOCK_SHM))) return 0;
 result = kshm_memcpy_u2k(&proc->p_shm,buf,addr,bufsize)-bufsize;
 kproc_unlock(proc,KPROC_LOCK_SHM);
 return result;
}
__crit size_t
u_setmem_c(__user void *addr,
           __kernel void const *__restrict buf,
           size_t bufsize) {
 size_t result; struct kproc *caller = kproc_self();
 KTASK_CRIT_MARK
 if __unlikely(KE_ISERR(kproc_lock(caller,KPROC_LOCK_SHM))) return 0;
 result = kshm_memcpy_k2u(&caller->p_shm,addr,buf,bufsize)-bufsize;
 kproc_unlock(caller,KPROC_LOCK_SHM);
 return result;
}
__crit __kernel char *u_strndup_c(__user char const *s, size_t maxchars) {
 size_t bufsize; char *result;
 KTASK_CRIT_MARK
 bufsize = u_strnlen(s,maxchars);
 result = (char *)malloc((bufsize+1)*sizeof(char));
 if __unlikely(!result) return NULL;
 asserte(u_getmem(s,result,bufsize) == 0);
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


__crit size_t u_strlen_c(__user char const *s) {
 struct kproc *caller = kproc_self();
 size_t result = 0,partmaxsize,partsize; char *addr;
 KTASK_CRIT_MARK
 if __unlikely(KE_ISERR(kproc_lock(caller,KPROC_LOCK_SHM))) return 0;
 for (;;) {
  addr = (char *)kshm_translate_1(&caller->p_shm,s,&partmaxsize,0);
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
__crit size_t u_strnlen_c(__user char const *s, size_t maxchars) {
 struct kproc *caller = kproc_self();
 size_t result = 0,partmaxsize,partsize; char *addr;
 KTASK_CRIT_MARK
 if __unlikely(KE_ISERR(kproc_lock(caller,KPROC_LOCK_SHM))) return 0;
 for (;;) {
  addr = (char *)kshm_translate_1(&caller->p_shm,s,&partmaxsize,0);
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

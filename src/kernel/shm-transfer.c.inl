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
#ifndef __KOS_KERNEL_SHM_TRANSFER_C_INL__
#define __KOS_KERNEL_SHM_TRANSFER_C_INL__ 1

#include <kos/compiler.h>
#include <kos/kernel/shm.h>
#include <kos/kernel/paging.h>
#include <sys/types.h>
#include <assert.h>

__DECL_BEGIN

__crit __nomp __kernel void *
__kshm_qwtranslateuser_impl(struct kshm const *__restrict self,
                            struct kpagedir const *__restrict epd,
                            __user void const *addr) {
 uintptr_t result = 0;
 kpage_t *page = kpagedir_getpage((struct kpagedir *)epd,addr);
 if __likely(page && page->present) {
  if __unlikely(!page->read_write) {
   /* Try to touch the page. */
   if __unlikely(self->s_pd != epd) goto done;
   if __unlikely(!kshm_touch_unlocked((struct kshm *)self,
                                      (void *)alignd((uintptr_t)addr,PAGEALIGN),
                                       1,0)) goto done;
   assert(page == kpagedir_getpage((struct kpagedir *)epd,addr));
  }
  page->dirty = 1;
  result  = x86_pte_getpptr(page);
  result |= X86_VPTR_GET_POFF(addr);
 }
done:
 return (void *)result;
}
__crit __nomp __kernel void *
__kshm_w1qtranslateuser_impl(struct kshm const *__restrict self,
                             struct kpagedir const *__restrict epd,
                             __user void const *addr) {
 uintptr_t result = 0;
 kpage_t *page = kpagedir_getpage((struct kpagedir *)epd,addr);
 if __likely(page && page->present) {
  if __unlikely(!page->read_write) {
   /* Try to touch the page. */
   if __unlikely(self->s_pd != epd) goto done;
   if __unlikely(!kshm_touch_unlocked((struct kshm *)self,
                                      (void *)alignd((uintptr_t)addr,PAGEALIGN),1,
                                       KSHMREGION_FLAG_READ|KSHMREGION_FLAG_WRITE
                 )) goto done;
   assert(page == kpagedir_getpage((struct kpagedir *)epd,addr));
  }
  page->dirty = 1;
  result  = x86_pte_getpptr(page);
  result |= X86_VPTR_GET_POFF(addr);
 }
done:
 return (void *)result;
}


__crit __nomp size_t
__kshm_copyinuser_w_impl(struct kshm const *self, __user void *dst,
                         __user void const *src, size_t bytes) {
 size_t max_dst,max_src; __kernel void *kdst,*ksrc;
 while ((kdst = kshm_wtranslateuser(self,self->s_pd,dst,bytes,&max_dst)) != NULL &&
        (ksrc = kshm_translateuser(self,self->s_pd,src,max_dst,&max_src,0)) != NULL) {
  SHM_MEMCPY(kdst,ksrc,max_src);
  if ((bytes -= max_src) == 0) break;
  *(uintptr_t *)&dst += max_src;
  *(uintptr_t *)&src += max_src;
 }
 return bytes;
}
__crit __nomp size_t
__kshm_copytouser_w_impl(struct kshm const *self, __user void *dst,
                         __kernel void const *src, size_t bytes) {
 size_t maxbytes; __kernel void *kdst;
 while ((kdst = kshm_wtranslateuser(self,self->s_pd,dst,bytes,&maxbytes)) != NULL) {
  assert(maxbytes <= bytes);
  SHM_MEMCPY(kdst,src,maxbytes);
  if ((bytes -= maxbytes) == 0) break;
  *(uintptr_t *)&dst += maxbytes;
  *(uintptr_t *)&src += maxbytes;
 }
 return bytes;
}
__crit __nomp size_t
__kshm_copyinuser_impl(struct kshm const *self, __user void *dst,
                       __user void const *src, size_t bytes) {
 size_t max_dst,max_src; __kernel void *kdst,*ksrc;
 while ((kdst = kshm_translateuser(self,self->s_pd,dst,bytes,&max_dst,1)) != NULL &&
        (ksrc = kshm_translateuser(self,self->s_pd,src,max_dst,&max_src,0)) != NULL) {
  SHM_MEMCPY(kdst,ksrc,max_src);
  if ((bytes -= max_src) == 0) break;
  *(uintptr_t *)&dst += max_src;
  *(uintptr_t *)&src += max_src;
 }
 return bytes;
}
__crit __nomp size_t
__kshm_copytouser_impl(struct kshm const *self, __user void *dst,
                       __kernel void const *src, size_t bytes) {
 size_t maxbytes; __kernel void *kdst;
 while ((kdst = kshm_translateuser(self,self->s_pd,dst,bytes,&maxbytes,1)) != NULL) {
  assert(maxbytes <= bytes);
  SHM_MEMCPY(kdst,src,maxbytes);
  if ((bytes -= maxbytes) == 0) break;
  *(uintptr_t *)&dst += maxbytes;
  *(uintptr_t *)&src += maxbytes;
 }
 return bytes;
}
__crit __nomp size_t
__kshm_copyfromuser_impl(struct kshm const *self, __kernel void *dst,
                         __user void const *src, size_t bytes) {
 size_t maxbytes; __kernel void *ksrc;
 while ((ksrc = kshm_translateuser(self,self->s_pd,src,bytes,&maxbytes,0)) != NULL) {
  assert(maxbytes <= bytes);
  SHM_MEMCPY(dst,ksrc,maxbytes);
  if ((bytes -= maxbytes) == 0) break;
  *(uintptr_t *)&dst += maxbytes;
  *(uintptr_t *)&src += maxbytes;
 }
 return bytes;
}



__crit __kernel void *
__ktranslator_qwexec_impl(struct ktranslator *__restrict self,
                          __user void *addr) {
 uintptr_t result = 0;
 kpage_t *page = kpagedir_getpage((struct kpagedir *)self->t_epd,addr);
 if __likely(page && page->present) {
  if __unlikely(!page->read_write) {
   size_t page_count;
   /* Try to touch the page. */
   if __unlikely((self->t_flags&(KTRANSLATOR_FLAG_LOCK|KTRANSLATOR_FLAG_SEPD)) !=
                                (KTRANSLATOR_FLAG_LOCK|KTRANSLATOR_FLAG_SEPD)
                 ) goto done;
   {
    /* Upgrade the read-lock momentarily. */
    kerrno_t error;
    KTASK_NOINTR_BEGIN
    error = krwlock_upgrade(&self->t_shm->s_lock);
    KTASK_NOINTR_END
    if __unlikely(KE_ISERR(error)) goto err_nlock;
   }
   page_count = kshm_touch_unlocked(self->t_shm,
                                   (void *)alignd((uintptr_t)addr,PAGEALIGN),1,0);
   krwlock_downgrade(&self->t_shm->s_lock);
   if __unlikely(!page_count) goto done;
   assert(page == kpagedir_getpage((struct kpagedir *)self->t_epd,addr));
  }
  page->dirty = 1;
  result  = x86_pte_getpptr(page);
  result |= X86_VPTR_GET_POFF(addr);
 }
done:
 return (void *)result;
err_nlock:
 self->t_flags &= ~(KTRANSLATOR_FLAG_LOCK);
 return NULL;
}

__crit __kernel void *
__ktranslator_w1qexec_impl(struct ktranslator *__restrict self,
                           __user void const *addr) {
 uintptr_t result = 0;
 kpage_t *page = kpagedir_getpage((struct kpagedir *)self->t_epd,addr);
 if __likely(page && page->present) {
  if __unlikely(!page->read_write) {
   size_t page_count;
   /* Try to touch the page. */
   if __unlikely((self->t_flags&(KTRANSLATOR_FLAG_LOCK|KTRANSLATOR_FLAG_SEPD)) !=
                                (KTRANSLATOR_FLAG_LOCK|KTRANSLATOR_FLAG_SEPD)
                 ) goto done;
   {
    /* Upgrade the read-lock momentarily. */
    kerrno_t error;
    KTASK_NOINTR_BEGIN
    error = krwlock_upgrade(&self->t_shm->s_lock);
    KTASK_NOINTR_END
    if __unlikely(KE_ISERR(error)) goto err_nlock;
   }
   page_count = kshm_touch_unlocked(self->t_shm,
                                   (void *)alignd((uintptr_t)addr,PAGEALIGN),1,
                                    KSHMREGION_FLAG_READ|KSHMREGION_FLAG_WRITE);
   krwlock_downgrade(&self->t_shm->s_lock);
   if __unlikely(!page_count) goto done;
   assert(page == kpagedir_getpage((struct kpagedir *)self->t_epd,addr));
  }
  page->dirty = 1;
  result  = x86_pte_getpptr(page);
  result |= X86_VPTR_GET_POFF(addr);
 }
done:
 return (void *)result;
err_nlock:
 self->t_flags &= ~(KTRANSLATOR_FLAG_LOCK);
 return NULL;
}

size_t
__ktranslator_copyinuser_impl(struct ktranslator *__restrict self,
                              __user void *dst, __user void const *src,
                              size_t bytes) {
 size_t max_dst,max_src; __kernel void *kdst,*ksrc;
 while ((kdst = ktranslator_exec(self,dst,bytes,&max_dst,1)) != NULL &&
        (ksrc = ktranslator_exec(self,src,max_dst,&max_src,0)) != NULL) {
  SHM_MEMCPY(kdst,ksrc,max_src);
  if ((bytes -= max_src) == 0) break;
  *(uintptr_t *)&dst += max_src;
  *(uintptr_t *)&src += max_src;
 }
 return bytes;
}
size_t
__ktranslator_copytouser_impl(struct ktranslator *__restrict self,
                              __user void *dst, __kernel void const *src,
                              size_t bytes) {
 size_t maxbytes; __kernel void *kdst;
 while ((kdst = ktranslator_exec(self,dst,bytes,&maxbytes,1)) != NULL) {
  assert(maxbytes <= bytes);
  SHM_MEMCPY(kdst,src,maxbytes);
  if ((bytes -= maxbytes) == 0) break;
  *(uintptr_t *)&dst += maxbytes;
  *(uintptr_t *)&src += maxbytes;
 }
 return bytes;
}
size_t
__ktranslator_copyfromuser_impl(struct ktranslator *__restrict self,
                                __kernel void *dst, __user void const *src,
                                size_t bytes) {
 size_t maxbytes; __kernel void *ksrc;
 while ((ksrc = ktranslator_exec(self,src,bytes,&maxbytes,0)) != NULL) {
  assert(maxbytes <= bytes);
  SHM_MEMCPY(dst,ksrc,maxbytes);
  if ((bytes -= maxbytes) == 0) break;
  *(uintptr_t *)&dst += maxbytes;
  *(uintptr_t *)&src += maxbytes;
 }
 return bytes;
}
size_t
__ktranslator_copyinuser_w_impl(struct ktranslator *__restrict self,
                                __user void *dst, __user void const *src,
                                size_t bytes) {
 size_t max_dst,max_src; __kernel void *kdst,*ksrc;
 while ((kdst = ktranslator_wexec(self,dst,bytes,&max_dst)) != NULL &&
        (ksrc = ktranslator_exec(self,src,max_dst,&max_src,0)) != NULL) {
  SHM_MEMCPY(kdst,ksrc,max_src);
  if ((bytes -= max_src) == 0) break;
  *(uintptr_t *)&dst += max_src;
  *(uintptr_t *)&src += max_src;
 }
 return bytes;
}
size_t
__ktranslator_copytouser_w_impl(struct ktranslator *__restrict self,
                                __user void *dst, __kernel void const *src,
                                size_t bytes) {
 size_t maxbytes; __kernel void *kdst;
 while ((kdst = ktranslator_wexec(self,dst,bytes,&maxbytes)) != NULL) {
  assert(maxbytes <= bytes);
  SHM_MEMCPY(kdst,src,maxbytes);
  if ((bytes -= maxbytes) == 0) break;
  *(uintptr_t *)&dst += maxbytes;
  *(uintptr_t *)&src += maxbytes;
 }
 return bytes;
}

size_t
__ktranslator_memset_impl(struct ktranslator *__restrict self,
                          __user void *dst, int byte, size_t bytes) {
 size_t maxbytes; __kernel void *kdst;
 while ((kdst = ktranslator_exec(self,dst,bytes,&maxbytes,1)) != NULL) {
  assert(maxbytes <= bytes);
  SHM_MEMSET(kdst,byte,maxbytes);
  if ((bytes -= maxbytes) == 0) break;
  *(uintptr_t *)&dst += maxbytes;
 }
 return bytes;
}
size_t
__ktranslator_memset_w_impl(struct ktranslator *__restrict self,
                            __user void *dst, int byte, size_t bytes) {
 size_t maxbytes; __kernel void *kdst;
 while ((kdst = ktranslator_wexec(self,dst,bytes,&maxbytes)) != NULL) {
  assert(maxbytes <= bytes);
  SHM_MEMSET(kdst,byte,maxbytes);
  if ((bytes -= maxbytes) == 0) break;
  *(uintptr_t *)&dst += maxbytes;
 }
 return bytes;
}


__user void *
__ktranslator_memchr_impl(struct ktranslator *__restrict self,
                          __user void const *p, int needle,
                          size_t bytes) {
 size_t maxbytes; __kernel void *kp,*found_p;
 while ((kp = ktranslator_exec(self,p,bytes,&maxbytes,0)) != NULL) {
  assert(maxbytes <= bytes);
  found_p = SHM_MEMCHR(kp,needle,maxbytes);
  if (found_p) return (__user void *)((uintptr_t)p+((uintptr_t)found_p-(uintptr_t)kp));
  if ((bytes -= maxbytes) == 0) break;
  *(uintptr_t *)&p += maxbytes;
 }
 return bytes ? KTRANSLATOR_MEMCHR_FAULT : NULL;
}



__DECL_END

#endif /* !__KOS_KERNEL_SHM_TRANSFER_C_INL__ */

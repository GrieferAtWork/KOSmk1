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
#ifdef __INTELLISENSE__
#include "shm.c"
#define WRITEABLE
#endif

__DECL_BEGIN

#ifdef WRITEABLE
__crit __nomp __kernel void *
__kshm_wtranslateuser_impl(struct kshm const *__restrict self, struct kpagedir const *__restrict epd,
                            __user void *addr, __size_t *__restrict rwbytes)
#else
__crit __nomp __kernel void *
__kshm_translateuser_impl(struct kshm const *self, struct kpagedir const *__restrict epd,
                          __user void const *addr, size_t *__restrict rwbytes, int writable)
#endif
{
 __kernel void *result;
 uintptr_t address_page;
 size_t page_count,rw_request;
 KTASK_CRIT_MARK
 kassert_kshm(self);
 kassertobj(rwbytes);
 /* Try the page directory first. */
 if __unlikely((rw_request = *rwbytes) == 0) {
  /* Special handling for write-access to ZERO(0) bytes of memory.
   * >> We simply translate the pointer to still operate
   *    and assume a is-a-faulty-pointer-style request. */
  return kpagedir_translate(self->s_pd,addr);
 }
#ifdef WRITEABLE
 /* NOTE: Don't set the dirty bit here. - This one's mean
  *       for writing to restricted pages and can't be
  *       invoked randomly by usercode. */
 result = kpagedir_translate_u(epd,addr,rwbytes,1);
 if __likely(result || epd != self->s_pd)
#else
again:
 /* TODO: When writing, set the 'dirty' bit on all affected pages.
  *    >> Otherwise we can't use it to track root-fork privileges. */
 result = kpagedir_translate_u(epd,addr,rwbytes,writable);
 if __likely(result || !writable || epd != self->s_pd)
#endif
 {
#if 0
  return *rwbytes ? result : NULL;
#else
  assertf(!result || *rwbytes,"result: %p, rwbytes: %Iu",result,*rwbytes);
  return result;
#endif
 }
 /* The page directory lookup may have failed because we
  * were trying to write to a copy-on-write page. */
 /* Touch all pages within the specified area of memory, using linear touching. */
 address_page = alignd((uintptr_t)addr,PAGEALIGN);
 page_count   = ceildiv(rw_request+((uintptr_t)addr-address_page),PAGESIZE);
 assert(page_count);
 /* Actually touch the requested memory.
  * NOTE: The function itself will round up the
  *       pointer to cluster-borders, but we can easily
  *       notice when it touched something by checking
  *       the return value for being non-ZERO(0). */
#ifdef WRITEABLE
 if __unlikely(!kshm_touch_unlocked((struct kshm *)self,(void *)address_page,page_count,0)) return NULL;
 /* Translate again, but don't include a write-request.
  * >> In writable mode, we allow access to otherwise read-only memory. */
 *rwbytes = rw_request;
 return kpagedir_translate_u(epd,addr,rwbytes,0);
#else
 if __unlikely(!kshm_touch_unlocked((struct kshm *)self,(void *)address_page,page_count,
                                    KSHMREGION_FLAG_READ|KSHMREGION_FLAG_WRITE)) return NULL;
 /* Something was touched -> Try to translate the pointer again! */
 *rwbytes = rw_request;
 goto again;
#endif
}





#ifdef WRITEABLE
__crit __kernel void *
__ktranslator_wexec_impl(struct ktranslator *__restrict self,
                         __user void *addr,
                         __kernel __size_t *__restrict rwbytes)
#else
__crit __kernel void *
__ktranslator_exec_impl(struct ktranslator *__restrict self,
                        __user void const *addr,
                        __kernel __size_t *__restrict rwbytes,
                        int writable)
#endif
{
 __kernel void *result;
 uintptr_t address_page;
 size_t page_count,rw_request;
 KTASK_CRIT_MARK
 kassertobj(self);
 kassertobj(rwbytes);
 /* Try the page directory first. */
 if __unlikely((rw_request = *rwbytes) == 0) {
  /* Special handling for write-access to ZERO(0) bytes of memory.
   * >> We simply translate the pointer to still operate
   *    and assume a is-a-faulty-pointer-style request. */
  return kpagedir_translate(self->t_epd,addr);
 }
#ifdef WRITEABLE
 /* NOTE: Don't set the dirty bit here. - This one's mean
  *       for writing to restricted pages and can't be
  *       invoked randomly by usercode. */
 result = kpagedir_translate_u(self->t_epd,addr,rwbytes,1);
 if __likely(result || !(self->t_flags&KTRANSLATOR_FLAG_SEPD))
#else
again:
 /* TODO: When writing, set the 'dirty' bit on all affected pages.
  *    >> Otherwise we can't use it to track root-fork privileges. */
 result = kpagedir_translate_u(self->t_epd,addr,rwbytes,writable);
 if __likely(result || !writable || !(self->t_flags&KTRANSLATOR_FLAG_SEPD))
#endif
 {
#if 0
  return *rwbytes ? result : NULL;
#else
  assertf(!result || *rwbytes,"result: %p, rwbytes: %Iu",result,*rwbytes);
  return result;
#endif
 }
 if __unlikely(!(self->t_flags&KTRANSLATOR_FLAG_LOCK)) goto err;
 /* The page directory lookup may have failed because
  * we were trying to write to a copy-on-write page. */
 /* Touch all pages within the specified area of memory, using linear touching. */
 address_page = alignd((uintptr_t)addr,PAGEALIGN);
 page_count   = ceildiv(rw_request+((uintptr_t)addr-address_page),PAGESIZE);

 assert(page_count);
 /* Actually touch the requested memory.
  * NOTE: The function itself will round up the
  *       pointer to cluster-borders, but we can easily
  *       notice when it touched something by checking
  *       the return value for being non-ZERO(0).
  * NOTE: Since read locks are recursive and we're only temporarily upgrading
  *       them to write-locks here, address translation using a translator
  *       is completely recursive, even when writing. */
 kassert_kshm(self->t_shm);
 {
  kerrno_t error;
  KTASK_NOINTR_BEGIN
  error = krwlock_upgrade(&self->t_shm->s_lock);
  KTASK_NOINTR_END
  if __unlikely(KE_ISERR(error)) goto err_nlock;
 }
 page_count = kshm_touch_unlocked(self->t_shm,
                                 (void *)address_page,
                                  page_count,0);
 krwlock_downgrade(&self->t_shm->s_lock);
 if __unlikely(!page_count) goto err;
#ifdef WRITEABLE
 return kpagedir_translate_u(self->t_epd,addr,rwbytes,1);
#else
 /* Something was touched -> Try to translate the pointer again! */
 *rwbytes = rw_request;
 goto again;
#endif
err_nlock: self->t_flags &= ~(KTRANSLATOR_FLAG_LOCK);
err:      *rwbytes = 0;
 return NULL;
}



#ifdef WRITEABLE
#undef WRITEABLE
#endif

__DECL_END

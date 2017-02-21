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
__kshm_translateuser_w_impl(struct kshm const *__restrict self, struct kpagedir const *epd,
                            __user void *addr, __size_t *__restrict rwbytes)
#else
__crit __nomp __kernel void *
__kshm_translateuser_impl(struct kshm const *self, struct kpagedir const *epd,
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
again:
#ifdef WRITEABLE
 /* NOTE: Don't set the dirty bit here. - This one's mean
  *       for writing to restricted pages and can't be
  *       invoked randomly by usercode. */
 result = kpagedir_translate_u(epd,addr,rwbytes,1);
 if __likely(result || epd != self->s_pd)
#else
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
 /* The page directory lookup may have because we
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
 if __unlikely(!kshm_touch((struct kshm *)self,(void *)address_page,page_count,0)) return NULL;
#else
 if __unlikely(!kshm_touch((struct kshm *)self,(void *)address_page,page_count,
                           KSHMREGION_FLAG_READ|KSHMREGION_FLAG_WRITE)) return NULL;
#endif
 /* Something was touched -> Try to translate the pointer again! */
 *rwbytes = rw_request;
 goto again;
}

#ifdef WRITEABLE
#undef WRITEABLE
#endif

__DECL_END

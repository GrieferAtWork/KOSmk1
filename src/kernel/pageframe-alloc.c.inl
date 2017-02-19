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
#include "pageframe.c"
#define TRYALLOC
#endif

__DECL_BEGIN

__crit struct kpageframe *
#ifdef TRYALLOC
kpageframe_tryalloc(__size_t n_pages, __size_t *did_alloc_pages)
#else
kpageframe_alloc(__size_t n_pages)
#endif
{
 struct kpageframe *iter,*split;
#ifdef TRYALLOC
 struct kpageframe *winner;
 kassertobj(did_alloc_pages);
#endif
#ifdef KPAGEFRAME_ALLOC_ZERO_RETURN_VALUE
 if (!n_pages) return KPAGEFRAME_ALLOC_ZERO_RETURN_VALUE;
#else
 assertf(n_pages,"Cannot allocate ZERO pages!");
#endif
 k_syslogf(KLOG_TRACE,"Allocating %Iu pages\n",n_pages);
#ifdef TRYALLOC
 winner = KPAGEFRAME_INVPTR;
#endif
 kpagealloc_lock();
 iter = first_free_page;
 while (iter != KPAGEFRAME_INVPTR) {
  assert(iter->pff_size != 0);
  if (iter->pff_size >= n_pages) {
#ifdef TRYALLOC
   *did_alloc_pages = n_pages;
take_iter:
#endif
   assertf((iter->pff_prev == KPAGEFRAME_INVPTR) == (iter == first_free_page),
           "Only the first page may have no predecessor");
   assert((iter->pff_prev == KPAGEFRAME_INVPTR) || iter->pff_prev < iter);
   assert((iter->pff_next == KPAGEFRAME_INVPTR) || iter->pff_next > iter);
   // Usable page --> split it
   if __unlikely(iter->pff_size == n_pages) {
    // Exception: the free page is an exact fit
    if (iter->pff_next != KPAGEFRAME_INVPTR) iter->pff_next->pff_prev = iter->pff_prev;
    if (iter->pff_prev != KPAGEFRAME_INVPTR) iter->pff_prev->pff_next = iter->pff_next;
    else first_free_page = iter->pff_next;
    // And we're already done!
    goto enditer;
   }
   // Take away from the start of the free page
   // BEFORE: [P1-][P2------][P3----]
   // AFTER:  [P1-][P2--][P3][P4----]
   //               ^^^^ result
   assert(iter->pff_size > n_pages);
   split = iter+n_pages;
   // Transfer page information from 'iter' to 'split'
   if ((split->pff_next = iter->pff_next) != KPAGEFRAME_INVPTR) split->pff_next->pff_prev = split;
   if ((split->pff_prev = iter->pff_prev) != KPAGEFRAME_INVPTR) split->pff_prev->pff_next = split;
   else first_free_page = split; // First free region
   split->pff_size = iter->pff_size-n_pages;
enditer:
#if KCONFIG_HAVE_PAGEFRAME_COUNT_ALLOCATED
   total_allocated_pages += n_pages;
#endif /* KCONFIG_HAVE_PAGEFRAME_COUNT_ALLOCATED */
   kpagealloc_unlock();
   return iter;
  }
#ifdef TRYALLOC
  else if ((winner == KPAGEFRAME_INVPTR) ||
            winner->pff_size < iter->pff_size) {
   /* First winner, or better match. */
   winner = iter;
  }
#endif
  iter = iter->pff_next;
 }
 kpagealloc_unlock();
#ifdef TRYALLOC
 if __likely(winner != KPAGEFRAME_INVPTR) {
  /* At least something... */
  *did_alloc_pages = winner->pff_size;
  iter = winner;
  goto take_iter;
 } else {
  *did_alloc_pages = 0;
 }
#endif
 /* Not page available with enough memory --> OUT-OF-MEMORY */
 k_syslogf(KLOG_ERROR,"OUT-OF-MEMORY when trying to allocate %Iu pageframes\n_pages",n_pages);
 _printtraceback_d();
 return KPAGEFRAME_INVPTR;
}

#ifdef TRYALLOC
#undef TRYALLOC
#endif

__DECL_END

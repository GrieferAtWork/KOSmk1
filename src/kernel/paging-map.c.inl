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
#include "paging.c"
#define REMAP
__DECL_BEGIN
#endif


#ifdef REMAP
kerrno_t kpagedir_remap
#else
kerrno_t kpagedir_map
#endif
(struct kpagedir *self,
 __physicaladdr void const *phys,
 __virtualaddr void const *virt,
 size_t pages, kpageflag_t flags) {
 x86_pde *pde; x86_pte *begin,*iter,*end; size_t pdenusing;
 kassertobj(self);
 assertf(self != kpagedir_kernel(),"Can't re-map the kernel page directory");
 assertf(isaligned((uintptr_t)phys,PAGEALIGN),"Physical address %p is not page aligned",phys);
 assertf(isaligned((uintptr_t)virt,PAGEALIGN),"Virtual address %p is not page aligned",virt);
 assertf((uintptr_t)phys+pages*PAGESIZE >= (uintptr_t)phys,"Address overflow");
 assertf((uintptr_t)virt+pages*PAGESIZE >= (uintptr_t)virt,"Address overflow");
 assertf((flags&~(PAGEDIR_FLAG_MASK))==0,"Invalid flags");
 if (pages) for (;;) {
  uintptr_t ptid = X86_VPTR_GET_PTID(virt);
  pde = &self->d_entries[X86_VPTR_GET_PDID(virt)];
  pdenusing = min(pages,X86_PTE4PDE-ptid);
  assert(pdenusing <= pages);
  if __unlikely(!pde->present) {
   // Must allocate the table entries of this directory entry
   begin = (x86_pte *)kpageframe_alloc(ceildiv((sizeof(x86_pte)*X86_PTE4PDE),PAGESIZE));
   if __unlikely(!begin) return KE_NOMEM;
   assert(isaligned((uintptr_t)begin,PAGEALIGN));
   memset(begin,0,X86_PTE4PDE*sizeof(x86_pte));
   pde->u = (uintptr_t)begin|flags|X86_PDE_FLAG_PRESENT;
  } else {
   begin = x86_pde_getpte(pde);
#ifndef REMAP
   // Make sure that none of the are already present
   end = (iter = begin+ptid)+pdenusing;
   for (; iter != end; ++iter) if __unlikely(iter->present) return KE_EXISTS;
#endif
  }
  end = (iter = begin+ptid)+pdenusing;
  while (iter != end) {
   assert(isaligned((uintptr_t)phys,PAGEALIGN));
   // NOTE: The public PDE flags are identical to the same-meaning PTE flags.
   iter->u = (uintptr_t)phys|flags|X86_PTE_FLAG_PRESENT;
   *(uintptr_t *)&phys += PAGESIZE;
   ++iter;
  }
  if (pdenusing == pages) break;
  pages -= pdenusing;
  *(uintptr_t *)&virt += pdenusing*PAGESIZE;
 }
 return KE_OK;
}

#ifdef REMAP
#undef REMAP
#endif

#ifdef __INTELLISENSE__
__DECL_END
#endif

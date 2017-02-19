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
#ifndef __KOS_KERNEL_PAGING_C__
#define __KOS_KERNEL_PAGING_C__ 1

#include <kos/compiler.h>
#include <kos/kernel/debug.h>
#include <kos/syslog.h>
#include <kos/kernel/pageframe.h>
#include <kos/kernel/paging.h>
#include <kos/errno.h>
#include <malloc.h>
#include <math.h>
#include <assert.h>
#include <stdio.h>

__DECL_BEGIN

struct kpagedir *kpagedir_new(void) {
 struct kpagedir *result = (struct kpagedir *)
  kpageframe_alloc(ceildiv(sizeof(struct kpagedir),PAGESIZE));
 if __likely(result) kpagedir_init(result);
 return result;
}
void kpagedir_delete(struct kpagedir *self) {
 kassertobj(self);
 assertf(self != kpagedir_kernel(),"Can't delete the kernel page directory");
 kpagedir_quit(self);
 kpageframe_free((struct kpageframe *)self,
                ceildiv(sizeof(struct kpagedir),PAGESIZE));
}

void kpagedir_init(struct kpagedir *self) {
 assertf(self != kpagedir_kernel(),"Can't re-initialize the kernel page directory");
 memset(self,0,X86_PDE4DIC*sizeof(x86_pde));
}
void kpagedir_quit(struct kpagedir *self) {
 x86_pde *iter,*end;
 assertf(self != kpagedir_kernel(),"Can't destroy the kernel page directory");
 end = (iter = self->d_entries)+__compiler_ARRAYSIZE(self->d_entries);
 while (iter != end) {
  if (iter->present) {
   kpageframe_free((struct kpageframe *)x86_pde_getpte(iter),
                  ceildiv((sizeof(x86_pte)*1024),PAGESIZE));
  }
  ++iter;
 }
}

static int kpagedir_fillafterfork(__virtualaddr void *vbegin, __physicaladdr void *pbegin,
                                  size_t pagecount, kpageflag_t flags, void *closure) {
 struct kpageframe *newframes; kerrno_t error;
 if (flags&PAGEDIR_FLAG_USER) {
  // TODO: Copy-on-write
  // TODO: Track allocated pages
  newframes = (struct kpageframe *)kpageframe_alloc(pagecount);
  if __unlikely(!newframes) return KE_NOMEM;
  memcpy(newframes,pbegin,pagecount*PAGESIZE);
  error = kpagedir_remap((struct kpagedir *)closure,newframes,
                         vbegin,pagecount,flags);
  if __unlikely(KE_ISERR(error)) {
   kpageframe_free(newframes,pagecount);
   return error;
  }
 }
 return 0;
}


struct kpagedir *kpagedir_copy(struct kpagedir *self) {
 x86_pte *newentry; struct kpagedir *result;
 x86_pde *iter,*end,*src; kassertobj(self);
 if __unlikely((result = (struct kpagedir *)kpageframe_alloc(
  ceildiv(sizeof(struct kpagedir),PAGESIZE))) == NULL) return NULL;
 end = (iter = result->d_entries)+__compiler_ARRAYSIZE(result->d_entries);
 src = self->d_entries;
 for (; iter != end; ++iter) {
  if ((*iter = *src++).present) {
   // Copy this page table
   newentry = (x86_pte *)kpageframe_alloc(ceildiv((sizeof(x86_pte)*X86_PTE4PDE),PAGESIZE));
   if __unlikely(!newentry) {
    // Out-of-memory
    while (iter-- != result->d_entries) {
     if (iter->present) kpageframe_free((struct kpageframe *)x86_pde_getpte(iter),
                                        ceildiv((sizeof(x86_pte)*X86_PTE4PDE),PAGESIZE));
    }
    kpageframe_free((struct kpageframe *)result,
                   ceildiv(sizeof(struct kpagedir),PAGESIZE));
    return NULL;
   }
   memcpy(newentry,x86_pde_getpte(iter),sizeof(x86_pte)*X86_PTE4PDE);
   iter->u = (uintptr_t)newentry|(iter->u&X86_PDE_MASK_FLAGS);
  }
 }
 return result;
}

#if 1
struct kpagedir *kpagedir_fork(struct kpagedir *self) {
 struct kpagedir *result = kpagedir_new();
 if __unlikely(!result) return NULL;
 if __unlikely(KE_ISERR(kpagedir_mapkernel(result,PAGEDIR_FLAG_READ_WRITE))) {
err_r: kpagedir_delete(result); return NULL;
 }
 if (kpagedir_enum(self,&kpagedir_fillafterfork,result) != 0) goto err_r;
 return result;
}
#endif

__physicaladdr void *kpagedir_translate(struct kpagedir const *self,
                                        __virtualaddr void const *virt) {
 x86_pde used_pde; x86_pte used_pte;
 kassertobj(self);
 used_pde = self->d_entries[X86_VPTR_GET_PDID(virt)];
 if __unlikely(!used_pde.present) return NULL; // Page table not present
 used_pte = x86_pde_getpte(&used_pde)[X86_VPTR_GET_PTID(virt)];
 if __unlikely(!used_pte.present) return NULL; // Page table entry not present
 return (__physicaladdr void *)(x86_pte_getpptr(&used_pte)+X86_VPTR_GET_POFF(virt));
}
__physicaladdr void *kpagedir_translate_flags(struct kpagedir const *self,
                                              __virtualaddr void const *virt,
                                              kpageflag_t flags) {
 x86_pde used_pde; x86_pte used_pte;
 kassertobj(self);
#ifdef X86_PTE_FLAG_PRESENT
 assert(flags&X86_PTE_FLAG_PRESENT);
#endif
 used_pde = self->d_entries[X86_VPTR_GET_PDID(virt)];
 if __unlikely(!(used_pde.u&flags)) return NULL;
 used_pte = x86_pde_getpte(&used_pde)[X86_VPTR_GET_PTID(virt)];
 if __unlikely(!(used_pte.u&flags)) return NULL;
 return (__physicaladdr void *)(x86_pte_getpptr(&used_pte)+X86_VPTR_GET_POFF(virt));
}

#ifndef __INTELLISENSE__
#define REMAP
#include "paging-map.c.inl"
#include "paging-map.c.inl"
#endif


kerrno_t kpagedir_mapkernel(struct kpagedir *self, kpageflag_t flags) {
 extern __byte_t __kernel_begin[];
 extern __byte_t __kernel_end[];
 return kpagedir_map(self,
                    (void *)alignd((uintptr_t)__kernel_begin,PAGEALIGN),
                    (void *)alignd((uintptr_t)__kernel_begin,PAGEALIGN),
                    ceildiv(align((uintptr_t)__kernel_end,PAGEALIGN)-
                         alignd((uintptr_t)__kernel_begin,PAGEALIGN),PAGESIZE),flags);
}

#define KPAGEDIR_FOREACHADDR(self,titer) \
 ((((titer)-x86_pde_getpte(diter))+(diter-(self)->d_entries)*X86_PTE4PDE) << 12)

#define KPAGEDIR_FOREACHBEGIN(self,virt,pages,titer,on_nonpresent,...)\
 x86_pde *diter,*dend,*dbegin; x86_pte *titer,*tend;\
 kassertobj(self);\
 assert(isaligned((uintptr_t)virt,PAGEALIGN));\
 dbegin = diter = (x86_pde *)&self->d_entries[X86_VPTR_GET_PDID(virt)];\
 dend = diter+ceildiv(pages,X86_PDE4DIC);\
 for (; diter != dend; ++diter) {\
  if (!diter->present) {\
   on_nonpresent;\
  } else {\
   titer = x86_pde_getpte(diter);\
   if (diter == dend-1) {\
    if (diter == dbegin) {\
     assert(pages < X86_PTE4PDE);\
     titer += X86_VPTR_GET_PTID(virt);\
     tend = titer+pages;\
    } else {\
     tend = titer+pages%X86_PTE4PDE;\
    }\
   } else {\
    tend = titer+X86_PTE4PDE;\
    if (diter == dbegin) titer += X86_VPTR_GET_PTID(virt);\
   }\
   {__VA_ARGS__;}\
   for (; titer != tend; ++titer) {
#define KPAGEDIR_FOREACHEND }}}

int kpagedir_rangeattr(struct kpagedir const *self,
                       __virtualaddr void const *virt,
                       size_t pages) {
 int result = KPAGEDIR_RANGEATTR_NONE;
 KPAGEDIR_FOREACHBEGIN(self,virt,pages,titer,
                       result |= KPAGEDIR_RANGEATTR_HASUNMAPPED) {
  result |= (titer->present
  ? KPAGEDIR_RANGEATTR_HASMAPPED
  : KPAGEDIR_RANGEATTR_HASUNMAPPED);
 } KPAGEDIR_FOREACHEND;
 return result;
}

int kpagedir_ismapped(struct kpagedir const *self,
                      __virtualaddr void const *virt,
                      size_t pages) {
 KPAGEDIR_FOREACHBEGIN(self,virt,pages,titer,
                       return 0) {
  if (!titer->present) return 0;
 } KPAGEDIR_FOREACHEND;
 return 1;
}
int kpagedir_ismappedex(struct kpagedir const *self,
                        __virtualaddr void const *virt,
                        size_t pages, kpageflag_t mask, kpageflag_t flags) {
#ifdef X86_PTE_FLAG_PRESENT
 mask  |= X86_PTE_FLAG_PRESENT;
 flags |= X86_PTE_FLAG_PRESENT;
#endif
 KPAGEDIR_FOREACHBEGIN(self,virt,pages,titer,
                       return 0) {
  if ((titer->u&mask) != flags) return 0;
 } KPAGEDIR_FOREACHEND;
 return 1;
}

size_t
kpagedir_setflags(struct kpagedir *self, __virtualaddr void const *virt,
                  size_t pages, kpageflag_t mask, kpageflag_t flags) {
 size_t result = 0;
#ifdef X86_PTE_FLAG_PRESENT
 mask  |= X86_PTE_FLAG_PRESENT;
 flags |= X86_PTE_FLAG_PRESENT;
#endif
 KPAGEDIR_FOREACHBEGIN(self,virt,pages,titer,,{
  diter->u |= X86_PDE_FLAG_USER|X86_PDE_FLAG_READ_WRITE;
 }) {
  titer->u = (titer->u&mask)|flags;
  ++result;
 } KPAGEDIR_FOREACHEND;
 return result;
}


int kpagedir_ismappedex_b(struct kpagedir const *self, __virtualaddr void const *virt,
                          size_t bytes, kpageflag_t mask, kpageflag_t flags) {
 uintptr_t aligned_virt = alignd((uintptr_t)virt,PAGEALIGN);
 return kpagedir_ismappedex(self,(void *)aligned_virt,
                            ceildiv(bytes+((uintptr_t)virt-aligned_virt),PAGESIZE),
                            mask,flags);
}

int kpagedir_isunmapped(struct kpagedir const *self,
                        __virtualaddr void const *virt,
                        size_t pages) {
 KPAGEDIR_FOREACHBEGIN(self,virt,pages,titer,) {
  if (titer->present) return 0;
 } KPAGEDIR_FOREACHEND;
 return 1;
}

extern __virtualaddr void *
kpagedir_findfreerange(struct kpagedir const *self, size_t pages,
                       __virtualaddr void const *hint) {
 uintptr_t iter; int attr;
 kassertobj(self);
 assertf(isaligned((uintptr_t)hint,PAGEALIGN),"Address not aligned: %p",hint);
 iter = (uintptr_t)hint;
 if __unlikely(!iter) iter = PAGESIZE;
 do {
  attr = kpagedir_rangeattr(self,(void *)iter,pages);
  if (KPAGEDIR_RANGEATTR_EMPTY(attr)) {
   // Found a free area
   return (__virtualaddr void *)iter;
  } else if (KPAGEDIR_RANGEATTR_ALL(attr)) {
   iter += pages*PAGESIZE; // Skip this entry area
  } else {
   iter += PAGESIZE;
  }
 } while (iter > (uintptr_t)hint);
 assert(!iter);
 if __unlikely(!hint) return NULL;
 iter = PAGESIZE;
 do {
  attr = kpagedir_rangeattr(self,(void *)iter,pages);
  if (KPAGEDIR_RANGEATTR_EMPTY(attr)) {
   // Found a free area
   return (__virtualaddr void *)iter;
  } else if (KPAGEDIR_RANGEATTR_ALL(attr)) {
   iter += pages*PAGESIZE; // Skip this entry area
  } else {
   iter += PAGESIZE;
  }
 } while (iter < (uintptr_t)hint);
 return NULL;
}

extern __virtualaddr void *
kpagedir_mapanyex(struct kpagedir *self, __physicaladdr void const *phys,
                  size_t pages, __virtualaddr void const *hint, kpageflag_t flags) {
 void *result;
 kassertobj(self);
 assertf(isaligned((uintptr_t)phys,PAGEALIGN),"Address not aligned: %p",phys);
 assertf(isaligned((uintptr_t)hint,PAGEALIGN),"Address not aligned: %p",hint);
 result = kpagedir_findfreerange(self,pages,hint);
 if __unlikely(!result) return NULL;
 assertf(kpagedir_isunmapped(self,result,pages),"WTF?!");
 if __unlikely(KE_ISERR(kpagedir_remap(self,phys,result,pages,flags))) return NULL;
 return result;
}




void kpagedir_unmap(struct kpagedir *self,
                    __virtualaddr void const *virt,
                    size_t pages) {
 x86_pde *pde; x86_pte *iter,*end,*begin; size_t pdenusing;
 assertf(self != kpagedir_kernel(),"Can't unmap the kernel page directory");
 kassertobj(self);
 assertf(isaligned((uintptr_t)virt,PAGEALIGN),"Virtual address %p is not page aligned",virt);
 assertf((uintptr_t)virt+pages*PAGESIZE >= (uintptr_t)virt,"Address overflow");
 if (pages) for (;;) {
  uintptr_t ptid = X86_VPTR_GET_PTID(virt);
  pde = &self->d_entries[X86_VPTR_GET_PDID(virt)];
  pdenusing = min(pages,1024-ptid);
  assert(pdenusing <= pages);
  if __likely(pde->present) {
   if (pdenusing == 1024) {
free_pagetable:
    // Simple case: Free the entire thing
    kpageframe_free((struct kpageframe *)x86_pde_getpte(pde),
                   ceildiv((sizeof(x86_pte)*1024),PAGESIZE));
    pde->u = 0;
   } else {
    int free_area = 1;
    // Complicated case: Free only a range, then check if the entire table can be freed
    begin = x86_pde_getpte(pde);
    end = (iter = begin)+ptid;
    while (iter != end) { if (iter->present) { --free_area; break; } ++iter; }
    if (free_area) {
     end = (iter = begin+ptid+pdenusing)-(1024-(ptid+pdenusing));
     while (iter != end) { if (iter->present) { --free_area; break; } ++iter; }
     // Full range below & above is free. --> Can free the entire table
     if (free_area) goto free_pagetable;
    }
    // Free only a section
    end = (iter = begin+ptid)+pdenusing;
    while (iter != end) { iter->u = 0; ++iter; }
   }
  }
  if (pdenusing == pages) break;
  pages -= pdenusing;
  *(uintptr_t *)&virt += pdenusing*PAGESIZE;
 }
}


int kpagedir_enum(struct kpagedir const *self,
                  int (*callback)(__virtualaddr void *vbegin,
                                  __physicaladdr void *pbegin,
                                  size_t size,
                                  kpageflag_t flags,
                                  void *closure),
                  void *closure) {
 uintptr_t start_virt,end_virt,start_phys,end_phys;
 x86_pde const *iter,*end,*begin; int error;
 x86_pte const *pte_begin,*pte_iter,*pte_end;
 kpageflag_t current_flags;
 end = (iter = begin = self->d_entries)+__compiler_ARRAYSIZE(self->d_entries);
cont:
 while (iter != end) {
  while (iter != end && !iter->present) ++iter;
  if (iter == end) break;
  pte_begin = x86_pde_getpte(iter);
  pte_end = (pte_iter = pte_begin)+X86_PTE4PDE;
next_pte:
  while (pte_iter != pte_end && !pte_iter->present) ++pte_iter;
  if (pte_iter == pte_end) { ++iter; goto cont; }
  assert(pte_iter >= pte_begin && pte_iter < pte_end);
  start_virt  = (uintptr_t)(iter-begin)*PAGESIZE*X86_PTE4PDE;
  start_virt += (uintptr_t)(pte_iter-pte_begin)*PAGESIZE;
  end_phys = start_phys = x86_pte_getpptr(pte_iter);
  current_flags = pte_iter->u&KPAGEDIR_ENUM_FLAG_MASK;
  //++pte_iter;
  for (;;) {
   while (pte_iter != pte_end && pte_iter->present) {
    if (x86_pte_getpptr(pte_iter) != end_phys ||
       (pte_iter->u&KPAGEDIR_ENUM_FLAG_MASK) != current_flags) break;
    end_phys += PAGESIZE,++pte_iter;
   }
   if (pte_iter == pte_end) {
    if (++iter == end || !iter->present) {
     pte_iter = pte_begin = NULL;
     break;
    }
    pte_begin = x86_pde_getpte(iter);
    pte_end = (pte_iter = pte_begin)+X86_PTE4PDE;
   } else /*if (!pte_iter->present) */break;
  }
  end_virt  = (uintptr_t)(iter-begin)*PAGESIZE*X86_PTE4PDE;
  end_virt += (uintptr_t)(pte_iter-pte_begin)*PAGESIZE;
  if (!end_virt) end_virt = (uintptr_t)-1;
  assertf(start_virt <= end_virt,"%p > %p",start_virt,end_virt);
  if (start_virt != end_virt) {
   error = (*callback)((void *)start_virt,
                       (void *)start_phys,
                       (size_t)(end_virt-start_virt)/PAGESIZE,
                       current_flags,closure);
   if __unlikely(error != 0) return error;
  }
  if (iter == end) break;
  if (pte_iter != pte_end) goto next_pte;
  ++iter;
 }
 return 0;
}

#ifdef __DEBUG__
static int kpagedir_print_callback(__virtualaddr void *vbegin,
                                   __physicaladdr void *pbegin,
                                   size_t pagecount, kpageflag_t flags, void *closure) {
 return printf("MAP %#.8Ix bytes (%#.8Ix pages) (%c%c) V[%.8p ... %.8p] --> P[%.8p ... %.8p]\n"
               ,pagecount*PAGESIZE,pagecount
               ,flags&PAGEDIR_FLAG_USER ? 'R' : '-'
               ,flags&PAGEDIR_FLAG_READ_WRITE ? 'W' : '-'
               ,vbegin,(uintptr_t)vbegin+pagecount*PAGESIZE
               ,pbegin,(uintptr_t)pbegin+pagecount*PAGESIZE);
}
void kpagedir_print(struct kpagedir const *self) {
 printf("PAGEDIR: %p\n",self);
 kpagedir_enum(self,&kpagedir_print_callback,NULL);
}
#endif




// TODO: The kernel page table should be allocated dynamically
//    >> That way, we can reduce the static size of the kernel _A_ _LOT_
static x86_pte __attribute__((__aligned__(PAGEALIGN))) __kernel_pagetables[X86_PDE4DIC*X86_PTE4PDE];
__attribute__((__aligned__(4096))) struct kpagedir __kpagedir_kernel;

__local void mmu_init_pt(void) {
 x86_pte *iter,*end,entry;
 k_syslogf(KLOG_INFO,"[PAGING] Initializing pt... ");
 // Initialize to map vaddr x to the same paddr
 entry.u = X86_PDE_FLAG_READ_WRITE|X86_PDE_FLAG_PRESENT;
 end = (iter = __kernel_pagetables)+__compiler_ARRAYSIZE(__kernel_pagetables);
 while (iter != end) {
  *iter++ = entry;
  entry.u += PAGESIZE;
 }
 k_syslogf(KLOG_INFO,"done\n");
}

__local void mmu_init_pd(void) {
 x86_pde *iter,*end,entry;
 k_syslogf(KLOG_INFO,"[PAGING] Initializing pd... ");
 end = (iter = __kpagedir_kernel.d_entries)+__compiler_ARRAYSIZE(__kpagedir_kernel.d_entries);
 entry.u = ((__u32)&__kernel_pagetables)|X86_PTE_FLAG_READ_WRITE|X86_PTE_FLAG_PRESENT;
 while (iter != end) { *iter++ = entry; entry.u += (sizeof(x86_pte)*X86_PTE4PDE); }
 k_syslogf(KLOG_INFO,"done\n");
}

x86_pte *getpte(uintptr_t addr) {
 x86_pde *used_pde;
 kassertobj(self);
 used_pde = &__kpagedir_kernel.d_entries[X86_VPTR_GET_PDID(addr)];
 return &x86_pde_getpte(used_pde)[X86_VPTR_GET_PTID(addr)];
}

void paging_set_flags(uintptr_t start, size_t pages,
                      __u32 mask, __u32 flags) {
 x86_pte *pte;
 while (pages--) {
  pte = getpte(start);
  pte->u = (pte->u&mask)|flags;
  start += PAGESIZE;
 }
}

void kernel_make_ro_immutable(void) {
 extern __u8 __kernel_ro_begin[];
 extern __u8 __kernel_ro_end[];
 assert(isaligned((uintptr_t)__kernel_ro_begin,PAGEALIGN));
 assert(isaligned((uintptr_t)__kernel_ro_end,PAGEALIGN));
 paging_set_flags((uintptr_t)__kernel_ro_begin,
                 ((uintptr_t)__kernel_ro_end-(uintptr_t)__kernel_ro_begin)/PAGESIZE,
                 ~(X86_PTE_FLAG_READ_WRITE),0);
}

void kernel_initialize_paging(void) {
 mmu_init_pt();
 mmu_init_pd();
 kernel_make_ro_immutable();
 // TODO: When initializing RAM, make anything that isn't
 //       part of it, or the kernel non-present!
 // TODO: Allocate kernel page tables dynamically (save a lot of mem that may...)
 kpagedir_makecurrent(kpagedir_kernel());
 __arch_enablepaging();
#define WP (1 << 16)
 // Enable write-protection within kernel-land (ring 0)
 __u32 cr0; __asm_volatile__("movl %%cr0, %0" : "=r" (cr0));
 cr0 |= WP; __asm_volatile__("movl %0, %%cr0" : : "r" (cr0));
}

__DECL_END

#ifndef __INTELLISENSE__
#include "paging-transfer.c.inl"
#endif

#endif /* !__KOS_KERNEL_PAGING_C__ */

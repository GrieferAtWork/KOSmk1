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
#include <kos/kernel/panic.h>

#ifdef __x86__ 
#include <kos/arch/x86/vga.h>
#endif

__DECL_BEGIN

__crit struct kpagedir *kpagedir_new(void) {
 struct kpagedir *result;
 KTASK_CRIT_MARK
 result = (struct kpagedir *)kpageframe_alloc(ceildiv(sizeof(struct kpagedir),PAGESIZE));
 if __likely(result != (struct kpagedir *)PAGENIL) {
  memset(result,0,sizeof(struct kpagedir));
 }
 return result;
}
__crit void
kpagedir_delete(struct kpagedir *__restrict self) {
 x86_pde *iter,*end;
 KTASK_CRIT_MARK
 kassertobj(self);
 assertf(self != kpagedir_kernel(),"Can't delete the kernel page directory");
 end = (iter = self->d_entries)+COMPILER_ARRAYSIZE(self->d_entries);
 while (iter != end) {
  if (iter->present) {
   kpageframe_free((struct kpageframe *)x86_pde_getpte(iter),
                  ceildiv((sizeof(x86_pte)*1024),PAGESIZE));
  }
  ++iter;
 }
 kpageframe_free((struct kpageframe *)self,
                  ceildiv(sizeof(struct kpagedir),PAGESIZE));
}

__kernel void *KPAGEDIR_TRANSLATE_CALL
kpagedir_translate(struct kpagedir const *__restrict self,
                   __user void const *virt) {
 x86_pde used_pde; x86_pte used_pte;
 kassertobj(self);
 used_pde = self->d_entries[X86_VPTR_GET_PDID(virt)];
 if __unlikely(!used_pde.present) return NULL; /* Page table not present */
 used_pte = x86_pde_getpte(&used_pde)[X86_VPTR_GET_PTID(virt)];
 if __unlikely(!used_pte.present) return NULL; /* Page table entry not present */
 return (__kernel void *)(x86_pte_getpptr(&used_pte)+X86_VPTR_GET_POFF(virt));
}
__kernel void *KPAGEDIR_TRANSLATE_CALL
kpagedir_translate_flags(struct kpagedir const *__restrict self,
                         __user void const *virt,
                         kpageflag_t flags) {
 x86_pde used_pde; x86_pte used_pte;
 kassertobj(self);
#ifdef X86_PTE_FLAG_PRESENT
 assert(flags&X86_PTE_FLAG_PRESENT);
#endif
 used_pde = self->d_entries[X86_VPTR_GET_PDID(virt)];
 if __unlikely((used_pde.u&flags) != flags) return NULL;
 used_pte = x86_pde_getpte(&used_pde)[X86_VPTR_GET_PTID(virt)];
 if __unlikely((used_pte.u&flags) != flags) return NULL;
 return (__kernel void *)(x86_pte_getpptr(&used_pte)+X86_VPTR_GET_POFF(virt));
}
kpage_t *KPAGEDIR_TRANSLATE_CALL
kpagedir_getpage(struct kpagedir *__restrict self,
                 __user void const *virt) {
 x86_pde used_pde;
 kassertobj(self);
 used_pde = self->d_entries[X86_VPTR_GET_PDID(virt)];
 return __likely(used_pde.u&X86_PDE_FLAG_PRESENT)
  ? x86_pde_getpte(&used_pde)+X86_VPTR_GET_PTID(virt)
  : NULL;
}


#ifndef __INTELLISENSE__
#define REMAP
#include "paging-map.c.inl"
#include "paging-map.c.inl"
#endif


__crit kerrno_t
kpagedir_mapkernel(struct kpagedir *__restrict self,
                   kpageflag_t flags) {
 extern __byte_t __kernel_begin[];
 extern __byte_t __kernel_end[];
 return kpagedir_map(self,
                    (void *)alignd ((uintptr_t)__kernel_begin,PAGEALIGN),
                    (void *)alignd ((uintptr_t)__kernel_begin,PAGEALIGN),
                     ceildiv(align ((uintptr_t)__kernel_end,  PAGEALIGN)-
                             alignd((uintptr_t)__kernel_begin,PAGEALIGN),PAGESIZE),flags);
}

#define KPAGEDIR_FOREACHADDR(self,titer) \
 ((((titer)-x86_pde_getpte(diter))+(diter-(self)->d_entries)*X86_PTE4PDE) << 12)

#define KPAGEDIR_FOREACHBEGIN(self,virt,pages,titer,on_nonpresent,...)\
 x86_pde *diter,*dend,*dbegin; x86_pte *titer,*tend;\
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

int
kpagedir_rangeattr(struct kpagedir const *__restrict self,
                   __user void const *virt,
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

int
kpagedir_ismapped(struct kpagedir const *__restrict self,
                  __user void const *virt,
                  size_t pages) {
 KPAGEDIR_FOREACHBEGIN(self,virt,pages,titer,
                       return 0) {
  if (!titer->present) return 0;
 } KPAGEDIR_FOREACHEND;
 return 1;
}

int
kpagedir_ismappedex(struct kpagedir const *__restrict self,
                    __user void const *virt,
                    size_t pages, kpageflag_t mask,
                    kpageflag_t flags) {
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
kpagedir_setflags(struct kpagedir *__restrict self,
                  __user void const *virt,
                  size_t pages, kpageflag_t mask,
                  kpageflag_t flags) {
 size_t result = 0;
#ifdef X86_PTE_FLAG_PRESENT
 mask  |= X86_PTE_FLAG_PRESENT;
 flags |= X86_PTE_FLAG_PRESENT;
#endif
 KPAGEDIR_FOREACHBEGIN(self,virt,pages,titer,,{
  diter->u = (diter->u&mask)|flags;
 }) {
  titer->u = (titer->u&mask)|flags;
  ++result;
 } KPAGEDIR_FOREACHEND;
 return result;
}


int
kpagedir_ismappedex_b(struct kpagedir const *__restrict self,
                      __user void const *virt,
                      size_t bytes, kpageflag_t mask,
                      kpageflag_t flags) {
 uintptr_t aligned_virt = alignd((uintptr_t)virt,PAGEALIGN);
 return kpagedir_ismappedex(self,(void *)aligned_virt,
                            ceildiv(bytes+((uintptr_t)virt-aligned_virt),PAGESIZE),
                            mask,flags);
}

int
kpagedir_isunmapped(struct kpagedir const *__restrict self,
                    __user void const *virt,
                    size_t pages) {
 KPAGEDIR_FOREACHBEGIN(self,virt,pages,titer,) {
  if (titer->present) return 0;
 } KPAGEDIR_FOREACHEND;
 return 1;
}

__user void *
kpagedir_findfreerange(struct kpagedir const *__restrict self, size_t pages,
                       __pagealigned __user void const *hint) {
 uintptr_t iter; int attr;
 kassertobj(self);
 assertf(isaligned((uintptr_t)hint,PAGEALIGN),"Address not aligned: %p",hint);
 iter = (uintptr_t)hint;
 do {
  attr = kpagedir_rangeattr(self,(void *)iter,pages);
  if (KPAGEDIR_RANGEATTR_EMPTY(attr)) {
   // Found a free area
   return (__user void *)iter;
  } else if (KPAGEDIR_RANGEATTR_ALL(attr)) {
   iter += pages*PAGESIZE; // Skip this entry area
  } else {
   iter += PAGESIZE;
  }
 } while (iter+pages*PAGESIZE > (uintptr_t)hint);
 if __likely((uintptr_t)hint != 0) {
  iter = 0;
  do {
   attr = kpagedir_rangeattr(self,(void *)iter,pages);
   if (KPAGEDIR_RANGEATTR_EMPTY(attr)) {
    // Found a free area
    return (__user void *)iter;
   } else if (KPAGEDIR_RANGEATTR_ALL(attr)) {
    iter += pages*PAGESIZE; // Skip this entry area
   } else {
    iter += PAGESIZE;
   }
  } while (iter+pages*PAGESIZE < (uintptr_t)hint);
 }
 return KPAGEDIR_FINDFREERANGE_ERR;
}

__user void *
kpagedir_mapanyex(struct kpagedir *__restrict self, __kernel void const *phys,
                  size_t pages, __user void const *hint, kpageflag_t flags) {
 void *result;
 kassertobj(self);
 assertf(isaligned((uintptr_t)phys,PAGEALIGN),"Address not aligned: %p",phys);
 assertf(isaligned((uintptr_t)hint,PAGEALIGN),"Address not aligned: %p",hint);
 result = kpagedir_findfreerange(self,pages,hint);
 if __unlikely(result == KPAGEDIR_FINDFREERANGE_ERR) return NULL;
 assertf(kpagedir_isunmapped(self,result,pages),"WTF?!");
 if __unlikely(KE_ISERR(kpagedir_remap(self,phys,result,pages,flags))) return NULL;
 return result;
}




__crit void
kpagedir_unmap(struct kpagedir *__restrict self,
               __user void const *virt,
               size_t pages) {
 x86_pde *pde; x86_pte *iter,*end,*begin; size_t pdenusing;
 KTASK_CRIT_MARK
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
    assert(iter <= end);
    while (iter != end) { if (iter->present) { --free_area; break; } ++iter; }
    if __unlikely(free_area) {
     end = (iter = begin+ptid+pdenusing)+(1024-(ptid+pdenusing));
     assertf(iter <= end
            ,"iter      = %p\n"
             "end       = %p\n"
             "begin     = %p\n"
             "ptid      = %Iu\n"
             "pdenusing = %Iu\n"
            ,iter,end,begin,ptid,pdenusing);
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


__kernel void *
__kpagedir_translate_u_impl(struct kpagedir const *__restrict self,
                            __user void const *addr,
                            __kernel __size_t *__restrict max_bytes,
                            __u32 flags) {
 size_t req_max_bytes,avail_max_bytes;
 uintptr_t result,aligned_addr,found_aligned_addr;
 kassertobj(max_bytes);
 req_max_bytes = *max_bytes;
 result = (uintptr_t)kpagedir_translate_flags(self,addr,flags);
 if __unlikely(!result) { avail_max_bytes = 0; goto end; }
 aligned_addr = alignd(result,PAGEALIGN);
 avail_max_bytes = PAGESIZE-(result-aligned_addr);
 if (avail_max_bytes >= req_max_bytes) { avail_max_bytes = req_max_bytes; goto end; }
 req_max_bytes -= avail_max_bytes;
 for (;;) {
  *(uintptr_t *)&addr += PAGESIZE,aligned_addr += PAGESIZE;
  found_aligned_addr = (uintptr_t)kpagedir_translate_flags(self,addr,flags);
  if (found_aligned_addr != aligned_addr) break;
  if (req_max_bytes <= PAGESIZE) { avail_max_bytes += req_max_bytes; break; }
  avail_max_bytes += PAGESIZE;
  req_max_bytes   -= PAGESIZE;
 }
end:
 *max_bytes = avail_max_bytes;
 return (__kernel void *)result;
}



int
kpagedir_enum(struct kpagedir const *__restrict self,
              int (*callback)(__user void *vbegin,
                              __kernel void *pbegin,
                              size_t size, kpageflag_t flags,
                              void *closure),
              void *closure) {
 uintptr_t start_virt,end_virt,start_phys,end_phys;
 x86_pde const *iter,*end,*begin; int error;
 x86_pte const *pte_begin,*pte_iter,*pte_end;
 kpageflag_t current_flags;
 end = (iter = begin = self->d_entries)+COMPILER_ARRAYSIZE(self->d_entries);
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
static int
kpagedir_print_callback(__user void *vbegin,
                        __kernel void *pbegin,
                        size_t pagecount, kpageflag_t flags, void *closure) {
 return printf("MAP %#.8Ix bytes (%#.8Ix pages) (%c%c) V[%.8p ... %.8p] --> P[%.8p ... %.8p]\n"
               ,pagecount*PAGESIZE,pagecount
               ,flags&PAGEDIR_FLAG_USER ? 'U' : '-'
               ,flags&PAGEDIR_FLAG_READ_WRITE ? 'W' : '-'
               ,vbegin,(uintptr_t)vbegin+pagecount*PAGESIZE
               ,pbegin,(uintptr_t)pbegin+pagecount*PAGESIZE);
}
void kpagedir_print(struct kpagedir const *__restrict self) {
 printf("PAGEDIR: %p\n",self);
 kpagedir_enum(self,&kpagedir_print_callback,NULL);
}
#endif



/* The page directory itself is still statically allocated,
 * just so we already know its final memory address at compile-time. */
__attribute__((__aligned__(PAGEALIGN)))
struct kpagedir __kpagedir_kernel;

void
kpagedir_kernel_installmemory(__pagealigned __kernel void *addr,
                              size_t pages, kpageflag_t flags) {
 kerrno_t error;
 assertf(isaligned((uintptr_t)addr,PAGEALIGN),
         "Address not page-aligned: %p",addr);
 if __unlikely(!pages) return;
 k_syslogf(KLOG_DEBUG,"[PD] Installing kernel memory %p+%Ix...%p (%s)\n",
           addr,pages*PAGESIZE,(void *)((uintptr_t)addr+pages*PAGESIZE),
          (flags&X86_PDE_FLAG_READ_WRITE) ? "R/W" : "RO");
 error = kpagedir_remap(&__kpagedir_kernel,addr,addr,pages,flags);
 if __unlikely(KE_ISERR(error)) {
  PANIC("Failed to install kernel memory %p+%Ix...%p (%s): %d",
        addr,pages*PAGESIZE,(void *)((uintptr_t)addr+pages*PAGESIZE),
       (flags&X86_PDE_FLAG_READ_WRITE) ? "R/W" : "RO",error);
 }
}


void kernel_initialize_paging(void) {
 extern __u8 __kernel_ro_begin[];
 extern __u8 __kernel_ro_end[];
 extern __u8 __kernel_rw_begin[];
 extern __u8 __kernel_rw_end[];
 uintptr_t kernel_begin,kernel_end;
 k_syslogf(KLOG_INFO,"[init] Paging...\n");
 /* When initializing RAM, make anything
  * that isn't part of it non-present! */
 kernel_begin = alignd((uintptr_t)__kernel_ro_begin,PAGEALIGN);
 kernel_end   = align ((uintptr_t)__kernel_ro_end,  PAGEALIGN);
 kpagedir_kernel_installmemory((void *)kernel_begin,ceildiv(kernel_end-kernel_begin,PAGESIZE),0);
 kernel_begin = alignd((uintptr_t)__kernel_rw_begin,PAGEALIGN);
 kernel_end   = align ((uintptr_t)__kernel_rw_end,  PAGEALIGN);
 kpagedir_kernel_installmemory((void *)kernel_begin,ceildiv(kernel_end-kernel_begin,PAGESIZE),X86_PDE_FLAG_READ_WRITE);

#ifdef __x86__ 
 KERNEL_INSTALLMEMORY(VGA_ADDR,VGA_WIDTH*VGA_HEIGHT*2,1);
 KERNEL_INSTALLMEMORY(0xA0000,(0xBFFFF+1)-0xA0000,1);
#endif
#if 0
 kpagedir_kernel_installmemory((void *)NULL,((uintptr_t)-1)/PAGESIZE,X86_PDE_FLAG_READ_WRITE);
 kpagedir_print(kpagedir_kernel());
#endif
 kpagedir_makecurrent(kpagedir_kernel());

#ifdef __x86__
 /* Enable paging and write-protection within kernel-land (ring 0) */
 x86_setcr0(x86_getcr0()|X86_CR0_PG|X86_CR0_WP);
#else
 __arch_enablepaging();
#endif
}

__DECL_END

#endif /* !__KOS_KERNEL_PAGING_C__ */

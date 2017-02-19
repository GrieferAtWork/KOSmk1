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
#ifndef __KOS_KERNEL_PAGEFRAME_C__
#define __KOS_KERNEL_PAGEFRAME_C__ 1

#include <math.h>
#include <assert.h>
#include <kos/kernel/debug.h>
#include <kos/syslog.h>
#include <kos/kernel/pageframe.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <traceback.h>
#include <kos/kernel/serial.h>
#include <kos/kernel/spinlock.h>

#if defined(__i386__) || defined(__x86_64__)
#include <kos/kernel/arch/x86/realmode.h>
#endif

__DECL_BEGIN

__STATIC_ASSERT(sizeof(struct kpageframe) == PAGESIZE);

void raminfo_addregion(__u64 start, __u64 size) {
 __u64 end; uintptr_t native_start,used_start;
 size_t native_size,used_size;
 extern __u8 __kernel_begin[];
 extern __u8 __kernel_end[];
 // Quickly dismiss regions too small to use for anything
 if __unlikely(size < PAGESIZE) return;
 end = start+(size-1);
 if (end > UINT32_MAX || end < start) {
  k_syslogf(KLOG_MSG,"Can't use 64-bit RAM region: %x%x -- %x%x\n",
           (__u32)(start & UINT64_C(0xFFFFFFFF00000000)),
           (__u32)(start & UINT64_C(0x00000000FFFFFFFF)),
           (__u32)(end & UINT64_C(0xFFFFFFFF00000000)),
           (__u32)(end & UINT64_C(0x00000000FFFFFFFF)));
  return;
 }
 native_start = (uintptr_t)start;
 native_size = (size_t)size;
 if __unlikely(!native_size) return;
 if __unlikely(!native_start) {
  /* Don't map valid physical memory to NULL. - KOS uses NULL as... well: the NULL-pointer (ZERO-0). */
  //k_syslogf(KLOG_WARN,"KOS cannot use RAM chips mapped to NULL (Skipping first page)\n");
  ++native_start,--native_size;
  if __unlikely(!native_size) return;
 }

 if (native_start < (uintptr_t)__kernel_end &&
     native_start+native_size > (uintptr_t)__kernel_begin) {
  size_t temp;
  if ((uintptr_t)__kernel_begin >= native_start) {
   raminfo_addregion(native_start,(uintptr_t)__kernel_begin-native_start);
  }
  temp = (uintptr_t)__kernel_end-native_start;
  if (temp >= native_size) return;
  native_size -= temp;
  native_start = (uintptr_t)__kernel_end;
 }
 assert(!(native_start < (uintptr_t)__kernel_end &&
          native_start+native_size > (uintptr_t)__kernel_begin));

 // Align the used memory by the alignment of pages
#if PAGEALIGN > 1
 used_start = align(native_start,PAGEALIGN);
#else
 used_start = native_start;
#endif
 used_size = native_size-(used_start-native_start);
 k_syslogf(KLOG_INFO
          ,"[RAM] Determined usable memory %Ix+%Ix...%Ix (Using %Ix+%Ix...%Ix)\n"
          ,native_start,native_size,native_start+native_size
          ,used_start,used_size,used_start+used_size);
 // Simply mask the region as free!
 // NOTE: Dividing by PAGESIZE truncates
 kpageframe_free((struct kpageframe *)used_start,
                used_size/PAGESIZE);
}

multiboot_info_t *__grub_mbt;
unsigned int      __grub_magic;

#if defined(__i386__) || defined(__x86_64__)
static void x64_reserve_realmode_bootstrap(void) {
 /* Mark low memory used by realmode interfacing mode as in-use. */
 __evalexpr(kpageframe_allocat((struct kpageframe *)alignd(MODE16_RELOC_BASE,PAGEALIGN),
                               ceildiv(MODE16_RELOC_SIZE,PAGESIZE)));
}
#else
#define x64_reserve_realmode_bootstrap() (void)0
#endif


void kernel_initialize_raminfo(void) {
 extern int kernel_initialize_dlmalloc(void);
 memory_map_t *iter,*end;
 size_t cmdline_length;
 char  *cmdline_addr;
 struct kpageframe *cmdline_page_v;
 size_t             cmdline_page_c;
 char   cmdline_safe_v[3*sizeof(void *)];
 size_t cmdline_safe_c;
 if (__grub_magic != MULTIBOOT_BOOTLOADER_MAGIC) {
  k_syslogf(KLOG_ERROR,"KOS Must be booted with a multiboot-compatible bootloader (e.g.: grub)\n");
  goto nocmdline;
 }
 if (__grub_mbt->flags&MULTIBOOT_FLAG_HAVE_CMDLINE) {
  cmdline_addr = (char *)__grub_mbt->cmdline;
  if (!cmdline_addr) cmdline_length = 0;
  else cmdline_length = strlen(cmdline_addr);
  // Save the first 12/24 bytes of the commandline, as they may potentially get
  // overwritten if the commandline starts where a memory region starts.
  cmdline_safe_c = min(sizeof(cmdline_safe_v),cmdline_length);
  memcpy(cmdline_safe_v,cmdline_addr,cmdline_safe_c);
 } else {
  cmdline_addr   = NULL;
  cmdline_length = 0;
  cmdline_safe_c = 0;
 }
 if (__grub_mbt->flags&MULTIBOOT_FLAG_HAVE_MMAP) {
  iter = (memory_map_t *)__grub_mbt->mmap_addr;
  end = (memory_map_t *)((uintptr_t)iter+__grub_mbt->mmap_length);
  while (iter < end) {
   if (iter->type == 1) {
    __u64 begin = ((__u64)iter->base_addr_high << 32) | (__u64)iter->base_addr_low;
    __u64 size  = ((__u64)iter->length_high << 32) | (__u64)iter->length_low;
    debug_addknownramregion((void *)(uintptr_t)begin,(size_t)size);
    raminfo_addregion(begin,size);
   }
   iter = (memory_map_t *)((uintptr_t)iter+(iter->size+sizeof(iter->size)));
  }
 } else {
  k_syslogf(KLOG_WARN,"GRUB didn't specify any usable RAM (bit 6 in flags no set)\n");
 }

 if (cmdline_length) {
  // Available RAM was enumerated.
  // Time to allocate the pages belonging to the cmdline
  uintptr_t aligned_cmdline;
  if (!cmdline_length) goto nocmdline;
  aligned_cmdline = alignd((uintptr_t)cmdline_addr,PAGEALIGN);
  cmdline_page_c = ceildiv(((uintptr_t)cmdline_addr-aligned_cmdline)+cmdline_length,PAGESIZE);
  assert(cmdline_page_c);
  // Allocate
  cmdline_page_v = kpageframe_allocat((struct kpageframe *)aligned_cmdline,cmdline_page_c);
  if (!cmdline_page_v) {
   k_syslogf(KLOG_ERROR,"Failed to allocate CMDLINE page %px%Iu\n",
         aligned_cmdline,cmdline_page_c);
   goto nocmdline;
  }
 } else {
nocmdline:
  cmdline_addr   = NULL;
  cmdline_page_v = NULL;
  cmdline_page_c = 0;
  cmdline_length = 0;
 }
 kernel_initialize_dlmalloc();
 // Dynamic memory (including the kernel heap) is now initialized!
 // NOTE: The pages containing our cmdline are preserved
 assert((cmdline_addr != NULL) == (cmdline_page_v != NULL));
 assert((cmdline_addr != NULL) == (cmdline_length != 0));
 assert((cmdline_addr != NULL) == (cmdline_page_c != 0));
 if (cmdline_page_v) {
  // Implemented in '/src/kernel/procenv.c'
  extern void kernel_initialize_cmdline(char const *cmd, size_t cmdlen);
  // Restore the saved cmdline portion (leading (up to) 12/24 bytes)
  memcpy(cmdline_addr,cmdline_safe_v,cmdline_safe_c);
  x64_reserve_realmode_bootstrap();
  kernel_initialize_cmdline(cmdline_addr,cmdline_length);
  kpageframe_free(cmdline_page_v,cmdline_page_c);
 }
 /* Reserve memory again in case the first attempt
  * failed because the region of memory in question
  * was allocated as part of the commandline. */
 x64_reserve_realmode_bootstrap();

}


static struct kspinlock kpagealloc_lockno = KSPINLOCK_INIT;
static struct kpageframe *first_free_page = KPAGEFRAME_INVPTR;
#define kpagealloc_lock()   kspinlock_lock(&kpagealloc_lockno)
#define kpagealloc_unlock() kspinlock_unlock(&kpagealloc_lockno)

__crit __wunused __malloccall __pagealigned struct kpageframe *
kpageframe_allocat(__pagealigned struct kpageframe *__restrict start, size_t n_pages) {
 struct kpageframe *iter,*split;
#ifdef KPAGEFRAME_ALLOC_ZERO_RETURN_VALUE
 if (!n_pages) return KPAGEFRAME_ALLOC_ZERO_RETURN_VALUE;
#else
 assertf(n_pages != 0,"Cannot allocate ZERO pages!");
#endif
 assertf(isaligned((uintptr_t)start,PAGEALIGN),
         "The given address %p isn't aligned",start);
 kpagealloc_lock();
 iter = first_free_page;
 while (iter != KPAGEFRAME_INVPTR) {
  if ((uintptr_t)start < (uintptr_t)iter) break; /* We won't find this region... */
  if ((uintptr_t)start+n_pages < (uintptr_t)iter+iter->pff_size) {
   assert(n_pages <= iter->pff_size);
   if (start == iter) {
    /* Split 1 --> 0/1 */
    if (n_pages == iter->pff_size) {
     assert((iter->pff_prev != KPAGEFRAME_INVPTR) == (iter != first_free_page));
     if (iter->pff_prev ==KPAGEFRAME_INVPTR) first_free_page = iter->pff_next;
     else iter->pff_prev->pff_next = iter->pff_next;
    } else {
     split = iter+n_pages;
     split->pff_prev = iter->pff_prev;
     split->pff_next = iter->pff_next;
     split->pff_size = iter->pff_size-n_pages;
     if (split->pff_prev == KPAGEFRAME_INVPTR) first_free_page = split;
     else split->pff_prev->pff_next = split;
    }
   } else {
    size_t new_iter_size = (size_t)(start-iter);
    split = start+n_pages;
    assert(n_pages < iter->pff_size);
    /* Split 1 --> 2 */
    split->pff_prev = iter;
    split->pff_next = iter->pff_next;
    split->pff_size = iter->pff_size-(new_iter_size+n_pages);
    iter->pff_next = split;
    iter->pff_size = new_iter_size;
   }
   kpagealloc_unlock();
   return start;
  }
  iter = iter->pff_next;
 }
 kpagealloc_unlock();
 return KPAGEFRAME_INVPTR;
}


__crit void kpageframe_free(__pagealigned struct kpageframe *__restrict start, size_t n) {
 struct kpageframe *iter,*region_end;
#ifdef KPAGEFRAME_ALLOC_ZERO_RETURN_VALUE
 if (!n) return;
#else
 assertf(n,"Cannot free ZERO pages!");
#endif
 assertf(_isaligned((uintptr_t)start,PAGEALIGN),
         "Unaligned page frame address %p",start);
 assertf(start != NULL,"NULL is special and cannot be mapped as free memory");
 region_end = start+n;
 k_syslogf(KLOG_TRACE,"Marking pages as free: %p (%Iu pages)\n",start,n);
 assertf(start+n > start,
         "Overflow in the given free region: start @%p; end @%p",
         start,start+n);
 kpagealloc_lock();
 if __unlikely(first_free_page == KPAGEFRAME_INVPTR) {
  /* Special case: First free region. */
  first_free_page = start;
  start->pff_size = n;
  start->pff_prev = KPAGEFRAME_INVPTR;
  start->pff_next = KPAGEFRAME_INVPTR;
  goto end;
 }
 iter = first_free_page;
 /* Search for already free regions adjacent to the given frame region
  * >> That way, we automatically re-combine broken regions into big ones
  * WARNING: This function can not assure the validity of its arguments.
  *          This function can even be used to add new free regions on-the-fly.
  *          Be sure not to pass pointers outside the actual ram. */
 for (;;) {
  assert(iter != KPAGEFRAME_INVPTR);
  assertf((iter->pff_prev == KPAGEFRAME_INVPTR) == (iter == first_free_page),
          "Only the first page may have no predecessor");
  assertf(iter != start,"pageframe %p was already freed",start);
  assertf(!(start < iter+iter->pff_size && region_end > iter),
          "pageframe region %p...%p lies within free region %p...%p",
          start,region_end,iter,iter+iter->pff_size);
  assert((iter->pff_prev == KPAGEFRAME_INVPTR) || iter->pff_prev < iter);
  assert((iter->pff_next == KPAGEFRAME_INVPTR) || iter->pff_next > iter);
  if (region_end == iter) {
   /* Given region ends where 'iter' starts
    * >> Extend the given region to encompass 'iter', replacing
    *    the link of 'iter' with that of the given region. */
   if ((start->pff_next = iter->pff_next) != KPAGEFRAME_INVPTR) start->pff_next->pff_prev = start;
   if ((start->pff_prev = iter->pff_prev) != KPAGEFRAME_INVPTR) start->pff_prev->pff_next = start;
   else first_free_page = start; /* First free region */
   /* Update the size of the now merged region.
    * 'start' is the new list entry and 'iter' lies directly behind. */
   start->pff_size = n+iter->pff_size;
   break;
  }
  if (iter+iter->pff_size == start) {
   /* The given region starts where 'iter' ends
    * >> Very simple case: Only need to update the size of 'iter'. */
   iter->pff_size += n;
   /* Check for special case: through expanding 'iter', it now
    * borders against the next memory region, essentially allowing
    * us to merge 'iter' with 'iter->pff_next'. - Do so! */
   if (iter->pff_next == iter+iter->pff_size &&
       iter->pff_next != KPAGEFRAME_INVPTR) {
    /* Merge 3 frames (iter, start & iter->pff_next) into one.
     * NOTE: 'iter' and 'start' have already been merged. */
    /* Update the size of 'iter' to encompass its successor. */
    iter->pff_size += iter->pff_next->pff_size;
    /* Unlink the next free page to have 'iter' jump over it. */
    if ((iter->pff_next = iter->pff_next->pff_next) != KPAGEFRAME_INVPTR)
     iter->pff_next->pff_prev = iter;
   }
   break;
  }
  /* Check for case: Must insert new region chunk before 'iter'. */
  if (region_end < iter) {
   /* Insert 'start' before 'iter'. */
   start->pff_size = n;
   if ((start->pff_prev = iter->pff_prev) != KPAGEFRAME_INVPTR) start->pff_prev->pff_next = start;
   else first_free_page = start; /* Prepend before all known regions. */
   (start->pff_next = iter)->pff_prev = start;
   break;
  }
  /* Check for case: Must append new region chunk at the end of free memory. */
  if (iter->pff_next == KPAGEFRAME_INVPTR) {
   /* Append at the end of all free regions. */
   assertf(start > iter+iter->pff_size,
           "Expected a memory region above all known page frames, "
           "but %p lies below known end %p",start,iter+iter->pff_size);
   (iter->pff_next = start)->pff_prev = iter;
   start->pff_next = KPAGEFRAME_INVPTR;
   start->pff_size = n;
   break;
  }
  /* Continue searching. */
  iter = iter->pff_next;
 }
end:
 kpagealloc_unlock();
}

__crit __pagealigned struct kpageframe *
kpageframe_realloc_inplace(__pagealigned struct kpageframe *old_start,
                           size_t old_pages, size_t new_pages) {
 assert(kpageframe_isfreepage(old_start,old_pages));
 if (new_pages <= old_pages) {
  if (new_pages != old_pages) {
   /* Free upper parts of old_start. */
   kpageframe_free(old_start+new_pages,old_pages-new_pages);
  }
  return old_start;
 }
 /* Must allocate more memory above. */
 if (kpageframe_allocat(old_start+old_pages,new_pages-old_pages) ==
     KPAGEFRAME_INVPTR) return KPAGEFRAME_INVPTR;
 /* We successfully managed to allocate more pages directly above! */
 return old_start;
}


__crit __pagealigned struct kpageframe *
kpageframe_realloc(__pagealigned struct kpageframe *old_start,
                   size_t old_pages, size_t new_pages) {
 size_t more_pages;
 struct kpageframe *result;
 /* Try inplace-relocation to expand memory upwards. */
 if (kpageframe_realloc_inplace(old_start,old_pages,new_pages) !=
    (struct kpageframe *)KPAGEFRAME_INVPTR) return old_start;
 assert(new_pages > old_pages);
 more_pages = new_pages-old_pages;
 /* Check to expand memory downwards. */
 result = kpageframe_allocat(old_start-more_pages,more_pages);
 if (result != (struct kpageframe *)KPAGEFRAME_INVPTR) {
  /* Move memmove all old memory downwards.
   * TODO: While we can't use memcpy here directly, memmove is so slow in
   *       this situation that maybe we should look into scattered memcpy. */
  memmove(result,old_start,old_pages*PAGESIZE);
  return result;
 }
 /* Last change: Try to allocate an entirely new region of memory, then
  *              attempt memcpy everything from the old before freeing it. */
 result = kpageframe_alloc(new_pages);
 if __unlikely(result != (struct kpageframe *)KPAGEFRAME_INVPTR) {
  /* Pfew... */
  kpageframe_memcpy(result,old_start,old_pages);
  kpageframe_free(old_start,old_pages);
 }
 return result;
}


void
kpageframe_memcpy(__pagealigned struct kpageframe *__restrict dst,
                  __pagealigned struct kpageframe const *__restrict src,
                  size_t n_pages) {
 /* TODO: 'rep movsl' (copy 4 bytes at a time). */
 memcpy(dst,src,n_pages*PAGESIZE);
}



void kpageframe_getinfo(struct kpageframeinfo *__restrict info) {
 struct kpageframe *iter;
 kassertobj(info);
 kpagealloc_lock();
 if ((iter = first_free_page) != KPAGEFRAME_INVPTR) {
  assert(iter->pff_prev == KPAGEFRAME_INVPTR);
  assert((iter->pff_next == KPAGEFRAME_INVPTR) || iter->pff_next > iter);
  info->pfi_minregion = iter->pff_size;
  info->pfi_maxregion = iter->pff_size;
  info->pfi_freepages = iter->pff_size;
  info->pfi_freeregions = 1;
  while ((iter = iter->pff_next) != KPAGEFRAME_INVPTR) {
   assert((iter->pff_prev == KPAGEFRAME_INVPTR) || iter->pff_prev < iter);
   assert((iter->pff_next == KPAGEFRAME_INVPTR) || iter->pff_next > iter);
        if (iter->pff_size < info->pfi_minregion) info->pfi_minregion = iter->pff_size;
   else if (iter->pff_size > info->pfi_maxregion) info->pfi_maxregion = iter->pff_size;
   ++info->pfi_freeregions;
   info->pfi_freepages += iter->pff_size;
  }
 } else {
  info->pfi_freeregions = 0;
  info->pfi_freepages = 0;
 }
 kpagealloc_unlock();
 info->pfi_freebytes = info->pfi_freepages*PAGESIZE;
}

void kpageframe_printphysmem(void) {
 struct kpageframe *iter;
 kpagealloc_lock();
 k_syslogf(KLOG_DEBUG,"+++ Physical memory layout\n");
 iter = first_free_page;
 while (iter != KPAGEFRAME_INVPTR) {
  assert(iter != iter->pff_next);
  assert((iter->pff_prev == KPAGEFRAME_INVPTR) || iter->pff_prev < iter);
  assert((iter->pff_next == KPAGEFRAME_INVPTR) || iter->pff_next > iter);
  k_syslogf(KLOG_DEBUG,"Free pages %p + %Iu pages ends %p (next: %p)\n",
            iter,iter->pff_size,iter+iter->pff_size,iter->pff_next);
  iter = iter->pff_next;
  assert(iter != first_free_page);
 }
 k_syslogf(KLOG_DEBUG,"--- done\n");
 kpagealloc_unlock();
}


int kpageframe_isfreepage(void const *p, size_t s) {
 struct kpageframe *iter;
 uintptr_t begin,end;
 end = (begin = (uintptr_t)p)+s;
 kpagealloc_lock();
 iter = first_free_page;
 while (iter != KPAGEFRAME_INVPTR) {
  assert((iter->pff_prev == KPAGEFRAME_INVPTR) || iter->pff_prev < iter);
  assert((iter->pff_next == KPAGEFRAME_INVPTR) || iter->pff_next > iter);
  if (end > (uintptr_t)iter && begin < (uintptr_t)(iter+iter->pff_size)) {
   kpagealloc_unlock();
   return 1;
  }
  assert(iter != iter->pff_next);
  iter = iter->pff_next;
  assert(iter != first_free_page);
 }
 kpagealloc_unlock();
 return 0;
}

#ifndef __INTELLISENSE__
#define TRYALLOC
#include "pageframe-alloc.c.inl"
#include "pageframe-alloc.c.inl"
#endif

__DECL_END

#endif /* !__KOS_KERNEL_PAGEFRAME_C__ */

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
  kernel_initialize_cmdline(cmdline_addr,cmdline_length);
  kpageframe_free(cmdline_page_v,cmdline_page_c);
 }
}


static struct kspinlock kpagealloc_lockno = KSPINLOCK_INIT;
static struct kpageframe *first_free_page = NULL;
#define kpagealloc_lock()   kspinlock_lock(&kpagealloc_lockno)
#define kpagealloc_unlock() kspinlock_unlock(&kpagealloc_lockno)

__wunused __malloccall __pagealigned struct kpageframe *
kpageframe_allocat(__pagealigned struct kpageframe *__restrict start, __size_t n_pages) {
 struct kpageframe *iter,*split;
#ifdef KPAGEFRAME_ALLOC_ZERO_RETURN_VALUE
 if (!n_pages) return KPAGEFRAME_ALLOC_ZERO_RETURN_VALUE;
#else
 assertf(n_pages,"Cannot allocate ZERO pages!");
#endif
 assert(isaligned((uintptr_t)start,PAGEALIGN));
 kpagealloc_lock();
 iter = first_free_page;
 while (iter) {
  if ((uintptr_t)start < (uintptr_t)iter) break;
  if ((uintptr_t)start+n_pages < (uintptr_t)iter+iter->pff_size) {
   assert(n_pages <= iter->pff_size);
   if (start == iter) {
    // Split 1 --> 0/1
    if (n_pages == iter->pff_size) {
     assert((iter->pff_prev != NULL) == (iter != first_free_page));
     if (!iter->pff_prev) first_free_page = iter->pff_next;
     else iter->pff_prev->pff_next = iter->pff_next;
    } else {
     split = iter+n_pages;
     split->pff_prev = iter->pff_prev;
     split->pff_next = iter->pff_next;
     split->pff_size = iter->pff_size-n_pages;
     if (!split->pff_prev) first_free_page = split;
     else split->pff_prev->pff_next = split;
    }
   } else {
    size_t new_iter_size = (size_t)(start-iter);
    split = start+n_pages;
    assert(n_pages < iter->pff_size);
    // Split 1 --> 2
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
 return NULL;
}


void kpageframe_free(__pagealigned struct kpageframe *__restrict start, __size_t n) {
 struct kpageframe *iter,*region_end;
#ifdef KPAGEFRAME_ALLOC_ZERO_RETURN_VALUE
 if (!n) return;
#else
 assertf(n,"Cannot free ZERO pages!");
#endif
 assertf(_isaligned((uintptr_t)start,PAGEALIGN),
         "Unaligned page frame address %p",start);
 region_end = start+n;
 k_syslogf(KLOG_TRACE,"Marking pages as free: %p (%Iu pages)\n",start,n);
 assertf(start+n > start,
         "Overflow in the given free region: start @%p; end @%p",
         start,start+n);
 kpagealloc_lock();
 iter = first_free_page;
 // Search for already free regions adjacent to the given frame region
 // >> That way, we automatically re-combine broken regions into big ones
 // WARNING: This function can not assure the validity of its arguments.
 //          This function can even be used to add new free regions on-the-fly.
 //          Be sure not to pass pointers outside the existing ram.
 while (iter) {
  assertf((iter->pff_prev == NULL) == (iter == first_free_page),
          "Only the first page may have no predecessor");
  assertf(iter != start,"pageframe %p was already freed",start);
  assertf(!(start < iter+iter->pff_size && region_end > iter),
          "pageframe region %p...%p lies within free region %p...%p",
          start,region_end,iter,iter+iter->pff_size);
  if (region_end == iter) {
   // Given region ends where 'iter' starts
   if ((start->pff_next = iter->pff_next) != NULL) start->pff_next->pff_prev = start;
   if ((start->pff_prev = iter->pff_prev) != NULL) start->pff_prev->pff_next = start;
   else first_free_page = start; // First free region
   // Update the size of the merged region
   start->pff_size = n+iter->pff_size;
   goto end;
  } else if (iter+iter->pff_size == iter) {
   // Given region starts where 'iter' ends
   // >> Very simple case: Only need to update the region size
   iter->pff_size += n;
   if (iter->pff_next &&
       iter->pff_next == iter+iter->pff_size) {
    // Special case: Insert a frame to merge 3 frames into one
    iter->pff_size += iter->pff_next->pff_size;
    if ((iter->pff_next = iter->pff_next->pff_next) != NULL)
     iter->pff_next->pff_prev = iter;
   }
   goto end;
  }
  iter = iter->pff_next;
 }
 // The given frame isn't located adjacent to an already free frame
 // --> Integrate it as a new free root frame
 // TODO: Don't just insert the frame at the start. - Sort them!
 if ((start->pff_next = first_free_page) != NULL)
  first_free_page->pff_prev = start;
 start->pff_prev = NULL;
 start->pff_size = n;
 first_free_page = start;
end:
 kpagealloc_unlock();
}


void kpageframe_getinfo(struct kpageframeinfo *__restrict info) {
 struct kpageframe *iter;
 kassertobj(info);
 kpagealloc_lock();
 if ((iter = first_free_page) != NULL) {
  assert(!iter->pff_prev);
  info->pfi_minregion = iter->pff_size;
  info->pfi_maxregion = iter->pff_size;
  info->pfi_freepages = iter->pff_size;
  info->pfi_freeregions = 1;
  while ((iter = iter->pff_next) != NULL) {
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


int kpageframe_isfreepage(void const *p, __size_t s) {
 struct kpageframe *iter;
 uintptr_t begin,end;
 assert(kpagedir_current() == kpagedir_kernel());
 end = (begin = (uintptr_t)p)+s;
 kpagealloc_lock();
 iter = first_free_page;
 while (iter) {
  if (end > (uintptr_t)iter &&
      begin < (uintptr_t)(iter+iter->pff_size)) {
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

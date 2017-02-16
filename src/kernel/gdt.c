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
#ifndef __KOS_KERNEL_GDT_C__
#define __KOS_KERNEL_GDT_C__ 1

#include <kos/compiler.h>
#include <kos/kernel/gdt.h>
#include <kos/kernel/task.h>
#include <kos/errno.h>
#include <kos/syslog.h>
#include <kos/kernel/tss.h>
#include <malloc.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <kos/kernel/paging.h>
#include <kos/kernel/task.h>
#include <kos/kernel/spinlock.h>

__DECL_BEGIN

extern void gdt_flush(struct kidtpointer *p);
extern void tss_flush(ksegid_t sel);

#if 0
/* TODO: A dynamic GDT would have to be mapped in all user page directories.
 *       The static one already is because we always map the entire kernel.
 *    >> In the long run we must switch to a dynamic GDT system,
 *       because otherwise our kernel will be limited to a maximum
 *       of 'GDT_MAX_ENTRIES' parallel processes. */
#define GDT_MAX_ENTRIES         (-1)
#else
#define GDT_MAX_ENTRIES        (128)
#endif
#define GDT_USE_DYNAMIC_MEMORY (GDT_MAX_ENTRIES < 0)



#if GDT_USE_DYNAMIC_MEMORY
#define GDT_BUFSIZE      (16*sizeof(struct ksegment))
static struct kidtpointer gdt = {0,0};
#define gdt_segments      gdt.base
#define gdt_used_size     gdt.limit
#else
#if GDT_MAX_ENTRIES < KSEG_BUILTIN
#error "Must specify more static GDT entries!"
#endif

/* Do we need special memory for this? */
static struct ksegment gdt_segments[GDT_MAX_ENTRIES];
#endif

static struct kspinlock gdt_lock = KSPINLOCK_INIT;
#define GDT_ALLOC_ACQUIRE   kspinlock_lock(&gdt_lock);
#define GDT_ALLOC_RELEASE   kspinlock_unlock(&gdt_lock);

#if GDT_USE_DYNAMIC_MEMORY
__crit ksegid_t kgdt_alloc(struct ksegment const *seg) {
 ksegid_t result; __u16 newlimit;
 struct ksegment *iter,*begin,*newvec;
 kassertobj(seg);
 assertf(seg->present,"Segment must be present");
 GDT_ALLOC_ACQUIRE
 assert(gdt.limit >= KSEG(KSEG_BUILTIN));
 assert(isaligned(gdt.limit,sizeof(struct ksegment)));
 begin = gdt.base+KSEG_BUILTIN;
 iter = (struct ksegment *)((uintptr_t)gdt.base+gdt.limit);
 assertf(iter >= begin,"iter = %p; begin = %p",iter,begin);
 while (iter-- != begin) if (!iter->present) {
  /* Found an unused slot. */
  memcpy(iter,seg,sizeof(struct ksegment));
  result = (ksegid_t)((uintptr_t)iter-(uintptr_t)gdt.base);
  gdt_flush(&gdt);
  goto end;
 }
 /* Must allocate a new GDT vector. */
 newlimit = align(gdt.limit+sizeof(struct ksegment),GDT_BUFSIZE);
 assert(newlimit > gdt.limit);
 newvec = (struct ksegment *)(malloc)(newlimit);
 if __unlikely(!newvec) { result = KSEG_NULL; goto end; }
 iter = (struct ksegment *)((uintptr_t)newvec+gdt.limit);
 result = (ksegid_t)((uintptr_t)iter-(uintptr_t)newvec);
 memcpy(newvec,(begin = gdt.base),gdt.limit);
 memset((byte_t *)newvec+gdt.limit,0,newlimit-gdt.limit);
 memcpy(iter,seg,sizeof(struct ksegment));
 gdt.base  = newvec;
 gdt.limit = newlimit;
 gdt_flush(&gdt);
 free(begin);
end:
 GDT_ALLOC_RELEASE
 if __likely(result != KSEG_NULL) {
  k_syslogf(KLOG_INFO,"[GDT] Allocated GDT entry: %#.4I16x (%u)\n",
            result,(unsigned)KSEG_ID(result));
 } else {
  k_syslogf(KLOG_ERROR,"[GDT] Failed to allocate GDT entry\n");
  _printtraceback_d();
 }
 return result;
}
__crit void kgdt_free(ksegid_t id) {
 struct ksegment *seg;
 struct kidtpointer gdt;
 assert(isaligned(id,sizeof(struct ksegment)));
 GDT_ALLOC_ACQUIRE
 assert(gdt.limit >= KSEG(KSEG_BUILTIN));
 assert(id <= gdt.limit);
 seg = (struct ksegment *)((uintptr_t)gdt.base+id);
 memset(seg,0,sizeof(struct ksegment));
 gdt_flush(&gdt);
 GDT_ALLOC_RELEASE
 k_syslogf(KLOG_INFO,"[GDT] Freed GDT entry: %#.4I16x (%u)\n",
           id,(unsigned)KSEG_ID(id));
}
#else /* GDT_USE_DYNAMIC_MEMORY */
__crit ksegid_t kgdt_alloc(struct ksegment const *seg) {
 ksegid_t result = KSEG_NULL;
 struct kidtpointer gdt;
 struct ksegment *iter,*end;
 kassertobj(seg);
 assertf(seg->present,"Segment must be present");
 GDT_ALLOC_ACQUIRE
 iter = gdt_segments+KSEG_BUILTIN;
 end = gdt_segments+GDT_MAX_ENTRIES;
 for (; iter != end; ++iter) if (!iter->present) {
  /* Found one! */
  memcpy(iter,seg,sizeof(struct ksegment));
  result = (ksegid_t)((uintptr_t)iter-(uintptr_t)gdt_segments);
  gdt.base  = gdt_segments;
  gdt.limit = GDT_MAX_ENTRIES*sizeof(struct ksegment);
  gdt_flush(&gdt);
  goto end;
 }
 k_syslogf(KLOG_ERROR,"[GDT] Failed to allocate GDT entry\n");
 _printtraceback_d();
end:
 GDT_ALLOC_RELEASE
 if __likely(result != KSEG_NULL) {
  k_syslogf(KLOG_INFO,"[GDT] Allocated GDT entry: %#.4I16x (%u)\n",
            result,(unsigned)KSEG_ID(result));
 }
 return result;
}
__crit void kgdt_free(ksegid_t id) {
 struct ksegment *seg;
 struct kidtpointer gdt;
 assert(isaligned(id,sizeof(struct ksegment)));
 GDT_ALLOC_ACQUIRE
 assert(id < GDT_MAX_ENTRIES*sizeof(struct ksegment));
 seg = (struct ksegment *)((uintptr_t)gdt_segments+id);
 memset(seg,0,sizeof(struct ksegment));
 gdt.base  = gdt_segments;
 gdt.limit = GDT_MAX_ENTRIES*sizeof(struct ksegment);
 gdt_flush(&gdt);
 GDT_ALLOC_RELEASE
 k_syslogf(KLOG_INFO,"[GDT] Freed GDT entry: %#.4I16x (%u)\n",
           id,(unsigned)KSEG_ID(id));
}
#endif /* !GDT_USE_DYNAMIC_MEMORY */


__crit void kgdt_update(ksegid_t id, struct ksegment const *seg) {
 struct ksegment *dst;
#if !GDT_USE_DYNAMIC_MEMORY
 struct kidtpointer gdt;
#endif
 GDT_ALLOC_ACQUIRE
 assert(id != KSEG_NULL);
 assert(id <= GDT_MAX_ENTRIES*sizeof(struct ksegment));
 assert(isaligned(id,sizeof(struct ksegment)));
 kassertobj(seg);
 assertf(seg->present,"Segment must be present");
 dst = (struct ksegment *)((uintptr_t)gdt_segments+id);
 memcpy(dst,seg,sizeof(struct ksegment));
#if !GDT_USE_DYNAMIC_MEMORY
 gdt.base  = gdt_segments;
 gdt.limit = GDT_MAX_ENTRIES*sizeof(struct ksegment);
#endif /* !GDT_USE_DYNAMIC_MEMORY */
 gdt_flush(&gdt);
 GDT_ALLOC_RELEASE
}


void kernel_initialize_gdt(void) {
#if !GDT_USE_DYNAMIC_MEMORY
 struct kidtpointer gdt;
#endif /* !GDT_USE_DYNAMIC_MEMORY */
 struct ktss *tss0 = &kcpu_zero()->c_tss;
 assertf(!kpaging_enabled(),"Must initialize GDT before paging");

 memset(tss0,0,sizeof(struct ktss));
 tss0->esp0 = 0; /*< Set when switching to user tasks. */
 tss0->ss0  = KSEG_KERNEL_DATA;
/*
 tss0->cs         = KSEG_KERNEL_CODE;
 tss0->ss         = KSEG_KERNEL_DATA;
 tss0->ds         = KSEG_KERNEL_DATA;
 tss0->es         = KSEG_KERNEL_DATA;
 tss0->fs         = KSEG_KERNEL_DATA;
 tss0->gs         = KSEG_KERNEL_DATA;
 tss0->iomap_base = sizeof(struct ktss);
*/

#if GDT_USE_DYNAMIC_MEMORY
 gdt.limit = align(KSEG_BUILTIN*sizeof(struct ksegment),GDT_BUFSIZE);
 gdt.base  = (struct ksegment *)(calloc)(1,gdt.limit);
 assertf(gdt.base,"Failed to allocate GDT memory"); /* TODO: Panic */
#else /* GDT_USE_DYNAMIC_MEMORY */
 gdt.limit = GDT_MAX_ENTRIES*sizeof(struct ksegment);
 gdt.base  = gdt_segments;
#endif /* !GDT_USE_DYNAMIC_MEMORY */

 /* Configure the GDT entries. */
 ksegment_encode(&gdt_segments[KSEG_ID(KSEG_NULL          )],0,0,0);                           /* NULL segment */
 ksegment_encode(&gdt_segments[KSEG_ID(KSEG_KERNEL_CODE   )],0,SEG_LIMIT_MAX,SEG_CODE_PL0);    /* Kernel code segment */
 ksegment_encode(&gdt_segments[KSEG_ID(KSEG_KERNEL_DATA   )],0,SEG_LIMIT_MAX,SEG_DATA_PL0);    /* Kernel data segment */
 ksegment_encode(&gdt_segments[KSEG_ID(KSEG_KERNEL_CODE_16)],0,SEG_LIMIT_MAX,SEG_CODE_PL0_16); /* 16-bit kernel code segment. */
 ksegment_encode(&gdt_segments[KSEG_ID(KSEG_KERNEL_DATA_16)],0,SEG_LIMIT_MAX,SEG_DATA_PL0_16); /* 16-bit kernel data segment. */
 ksegment_encode(&gdt_segments[KSEG_ID(KSEG_KERNELLDT     )],0,0,SEG_LDT);                     /* Kernel LDT table (technically lives in 'kproc_kernel()->p_ldt', but is empty by default). */
 ksegment_encode(&gdt_segments[KSEG_ID(KSEG_CPU0TSS       )],(uintptr_t)tss0,sizeof(struct ktss),SEG_TSS); /* CPU-0 TSS */
#ifdef KSEG_USER_CODE
 ksegment_encode(&gdt_segments[KSEG_ID(KSEG_USER_CODE     )],0,SEG_LIMIT_MAX,SEG_CODE_PL3);    /* User code */
#endif
#ifdef KSEG_USER_DATA
 ksegment_encode(&gdt_segments[KSEG_ID(KSEG_USER_DATA     )],0,SEG_LIMIT_MAX,SEG_DATA_PL3);    /* User data */
#endif

 gdt_flush(&gdt); /* Install the GDT table (overwriting the one set up by GRUB) */
 assert(gdt_segments[KSEG_ID(KSEG_CPU0TSS)].rw == 0);
 tss_flush(KSEG_CPU0TSS|3); /* Set the TSS segment */
 assert(gdt_segments[KSEG_ID(KSEG_CPU0TSS)].rw == 1);
}

__DECL_END

#endif /* !__KOS_KERNEL_GDT_C__ */

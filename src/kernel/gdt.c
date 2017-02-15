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


__local void kldt_genseg(struct kldt *self, struct ksegment *seg) {
 ksegment_encode(seg,
                (__u32)(uintptr_t)self->ldt_table.base,
                (__u32)self->ldt_table.limit,SEG_LDT);
}
__local void kldt_update(struct kldt *self) {
 struct ksegment seg;
 kldt_genseg(self,&seg);
 kgdt_update(self->ldt_gdtid,&seg);
}

__crit kerrno_t
kldt_init(struct kldt *self, __u16 sizehint) {
 struct ksegment ldt_seg;
 kassertobj(self);
 self->ldt_table.limit = sizehint*sizeof(struct ksegment);
 self->ldt_table.base = (struct ksegment *)malloc(self->ldt_table.limit);
 if __unlikely(!self->ldt_table.base) return KE_NOMEM;
 memset(self->ldt_table.base,0,self->ldt_table.limit);
 kldt_genseg(self,&ldt_seg);
 self->ldt_gdtid = kgdt_alloc(&ldt_seg);
 if __unlikely(self->ldt_gdtid == KSEG_NULL) {
  free(self->ldt_table.base);
  return KE_NOMEM;
 }
 return KE_OK;
}

__crit kerrno_t
kldt_initcopy(struct kldt *self,
              struct kldt const *right) {
 struct ksegment ldt_seg;
 kassertobj(self);
 kassertobj(right);
 self->ldt_table.base = (struct ksegment *)memdup(right->ldt_table.base,
                                                  right->ldt_table.limit);
 if __unlikely(!self->ldt_table.base) return KE_NOMEM;
 self->ldt_table.limit = right->ldt_table.limit;
 self->ldt_gdtid = kgdt_alloc(&ldt_seg);
 if __unlikely(self->ldt_gdtid == KSEG_NULL) {
  free(self->ldt_table.base);
  return KE_NOMEM;
 }
 return KE_OK;
}

__crit void
kldt_quit(struct kldt *self) {
 kassertobj(self);
 kgdt_free(self->ldt_gdtid);
 free(self->ldt_table.base);
}

__nomp __crit ksegid_t
kldt_alloc(struct kldt *self,
           struct ksegment const *seg) {
 struct ksegment *iter,*begin,*newvec;
 __u16 new_limit; ksegid_t result;
 kassertobj(self);
 kassertobj(seg);
 assert(!(self->ldt_table.limit % sizeof(struct ksegment)));
 assert(seg->present);
 begin = self->ldt_table.base;
 iter = (struct ksegment *)((uintptr_t)begin+self->ldt_table.limit);
 while (iter-- != begin) if (!iter->present) {
  /* Found an empty slot */
  memcpy(iter,seg,sizeof(struct ksegment));
  result = (ksegid_t)((uintptr_t)iter-(uintptr_t)begin);
  goto end;
 }
 /* Must allocate a new segment */
 new_limit = self->ldt_table.limit*2;
 if (new_limit <= self->ldt_table.limit) new_limit = 0xffff;
 if (new_limit == self->ldt_table.limit) return KSEG_NULL;
 assert(!(new_limit % sizeof(struct ksegment)));
 assert(!(self->ldt_table.limit % sizeof(struct ksegment)));
 iter = newvec = (struct ksegment *)malloc(new_limit);
 if __unlikely(!newvec) return KSEG_NULL;
 assert(begin == self->ldt_table.base);
 memcpy(newvec,begin,(size_t)(result = self->ldt_table.limit));
 *(uintptr_t *)&iter += result;
 memcpy(iter,seg,sizeof(struct ksegment));
 self->ldt_table.base  = newvec;
 self->ldt_table.limit = new_limit;
 /* Update the LDT's area of memory before freeing the old one.
  * Not doing so would create a security hole where a process
  * could theoretically allocate memory still mapped as LDT
  * table for itself, or some other process. */
 kldt_update(self);
 free(begin);
end:
 return KSEG_TOLDT(result);
}

__nomp __crit ksegid_t
kldt_allocat(struct kldt *self, ksegid_t reqid,
             struct ksegment const *seg) {
 kassertobj(self);
 assert(KSEG_ISLDT(reqid));
 kassertobj(seg);
 reqid = reqid & (sizeof(struct ksegment)-1);


 return 0; // TODO
}

__nomp __crit void
kldt_free(struct kldt *self, ksegid_t id) {
 kassertobj(self);
 assert(KSEG_ISLDT(id));
 id = id & (sizeof(struct ksegment)-1);
 assert(id < self->ldt_table.limit);
 // TODO
}

__nomp __crit void
kldt_get(struct kldt *self, ksegid_t id,
         struct ksegment *seg) {
 kassertobj(self);
 kassertobj(seg);
 assert(KSEG_ISLDT(id));
 id = id & (sizeof(struct ksegment)-1);
 assert(id < self->ldt_table.limit);
 // TODO
}

__nomp __crit void
kldt_set(struct kldt *self, ksegid_t id,
         struct ksegment const *seg) {
 kassertobj(self);
 kassertobj(seg);
 assert(KSEG_ISLDT(id));
 id = id & (sizeof(struct ksegment)-1);
 assert(id < self->ldt_table.limit);
 // TODO
}


//#define GDT_MAX_ENTRIES  -1
#define GDT_MAX_ENTRIES  128


#if GDT_MAX_ENTRIES < 0
#define GDT_USE_DYNAMIC_MEMORY 1
#else
#define GDT_USE_DYNAMIC_MEMORY 0
#endif



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
static __u16           gdt_used_size;
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
  k_syslogf(KLOG_INFO,"Allocated GDT entry: %#.4I16x (%u)\n",
            result,(unsigned)KSEG_ID(result));
 } else {
  k_syslogf(KLOG_ERROR,"Failed to allocate GDT entry\n");
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
 k_syslogf(KLOG_INFO,"Freed GDT entry: %#.4I16x (%u)\n",
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
  assert(result <= gdt_used_size);
  if (result == gdt_used_size) {
   gdt_used_size += sizeof(struct ksegment);
  }
  gdt.base  = gdt_segments;
  gdt.limit = gdt_used_size;
  gdt_flush(&gdt);
  goto end;
 }
 k_syslogf(KLOG_ERROR,"Failed to allocate GDT entry\n");
 _printtraceback_d();
end:
 GDT_ALLOC_RELEASE
 if __likely(result != KSEG_NULL) {
  k_syslogf(KLOG_INFO,"Allocated GDT entry: %#.4I16x (%u)\n",
            result,(unsigned)KSEG_ID(result));
 }
 return result;
}
__crit void kgdt_free(ksegid_t id) {
 struct ksegment *seg;
 struct kidtpointer gdt;
 assert(isaligned(id,sizeof(struct ksegment)));
 GDT_ALLOC_ACQUIRE
 assert(id <= gdt_used_size);
 seg = (struct ksegment *)((uintptr_t)gdt_segments+id);
 memset(seg,0,sizeof(struct ksegment));
 if (id == gdt_used_size) {
  while ((assert(seg != gdt_segments),!seg[-1].present)) --seg;
  gdt_used_size = (__u16)((uintptr_t)seg-(uintptr_t)gdt_segments);
 }
 gdt.base  = gdt_segments;
 gdt.limit = gdt_used_size;
 gdt_flush(&gdt);
 GDT_ALLOC_RELEASE
 k_syslogf(KLOG_INFO,"Freed GDT entry: %#.4I16x (%u)\n",
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
 assert(id <= gdt_used_size);
 assert(isaligned(id,sizeof(struct ksegment)));
 kassertobj(seg);
 assertf(seg->present,"Segment must be present");
 dst = (struct ksegment *)((uintptr_t)gdt_segments+id);
 memcpy(dst,seg,sizeof(struct ksegment));
#if !GDT_USE_DYNAMIC_MEMORY
 gdt.base  = gdt_segments;
 gdt.limit = gdt_used_size;
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
 gdt.limit = gdt_used_size = KSEG_BUILTIN*sizeof(struct ksegment);
 gdt.base  = gdt_segments;
#endif /* !GDT_USE_DYNAMIC_MEMORY */

 /* Configure the GDT entries. */
 ksegment_encode(&gdt_segments[KSEG_ID(KSEG_NULL          )],0,0,0);                           /* NULL segment */
 ksegment_encode(&gdt_segments[KSEG_ID(KSEG_KERNEL_CODE   )],0,SEG_LIMIT_MAX,SEG_CODE_PL0);    /* Kernel code segment */
 ksegment_encode(&gdt_segments[KSEG_ID(KSEG_KERNEL_DATA   )],0,SEG_LIMIT_MAX,SEG_DATA_PL0);    /* Kernel data segment */
 ksegment_encode(&gdt_segments[KSEG_ID(KSEG_USER_CODE     )],0,SEG_LIMIT_MAX,SEG_CODE_PL3);    /* User code */
 ksegment_encode(&gdt_segments[KSEG_ID(KSEG_USER_DATA     )],0,SEG_LIMIT_MAX,SEG_DATA_PL3);    /* User data */
 ksegment_encode(&gdt_segments[KSEG_ID(KSEG_KERNEL_CODE_16)],0,SEG_LIMIT_MAX,SEG_CODE_PL0_16); /* 16-bit kernel code segment. */
 ksegment_encode(&gdt_segments[KSEG_ID(KSEG_KERNEL_DATA_16)],0,SEG_LIMIT_MAX,SEG_DATA_PL0_16); /* 16-bit kernel data segment. */
 ksegment_encode(&gdt_segments[KSEG_ID(KSEG_KERNELLDT     )],0,0,SEG_LDT);                     /* Kernel LDT table (technically lives in 'kproc_kernel()->p_ldt', but is empty by default). */
 ksegment_encode(&gdt_segments[KSEG_ID(KSEG_CPU0TSS       )],(uintptr_t)tss0,sizeof(struct ktss),SEG_TSS); /* CPU-0 TSS */

 gdt_flush(&gdt); /* Install the GDT table (overwriting the one set up by GRUB) */
 assert(gdt_segments[KSEG_ID(KSEG_CPU0TSS)].rw == 0);
 tss_flush(KSEG_CPU0TSS|3); /* Set the TSS segment */
 assert(gdt_segments[KSEG_ID(KSEG_CPU0TSS)].rw == 1);
}

__DECL_END

#endif /* !__KOS_KERNEL_GDT_C__ */

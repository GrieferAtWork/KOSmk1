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
#ifndef __KOS_KERNEL_SHM2_LDT_C_INL__
#define __KOS_KERNEL_SHM2_LDT_C_INL__ 1

#include <assert.h>
#include <malloc.h>
#include <kos/config.h>
#include <kos/kernel/debug.h>
#include <kos/kernel/gdt.h>
#include <kos/kernel/shm2.h>
#include <kos/syslog.h>
#include <math.h>

__DECL_BEGIN

#define KSHM_LDT_BUFSIZE    (8)
#define KSHM_LDT_PAGEFLAGS  (PAGEDIR_FLAG_USER|PAGEDIR_FLAG_READ_WRITE)



#define SELF          (self)
#define SEGAT(offset) ((struct ksegment *)((uintptr_t)SELF->s_ldt.ldt_vector+(offset)))
#define VEC_BEGIN     (SELF->s_ldt.ldt_vector)
#define VEC_SIZE      (SELF->s_ldt.ldt_limit)
#define VEC_END       ((struct ksegment *)((uintptr_t)SELF->s_ldt.ldt_vector+VEC_SIZE))
#define BUFSIZE       (KSHM_LDT_BUFSIZE*sizeof(struct ksegment))

static kerrno_t
kshm_alloc_and_map_ldt_vector(struct kshm *__restrict self,
                              __u16 limit) {
 kassert_kshm(self);
 assert(!(limit%sizeof(struct ksegment)));
 /* Initialize the LDT table with the given hint. */
 self->s_ldt.ldt_limit = limit;
 self->s_ldt.ldt_vector = (__kernel struct ksegment *)valloc(limit);
 if __unlikely(!self->s_ldt.ldt_vector) return KE_NOMEM;
 memset(self->s_ldt.ldt_vector,0,limit);
 assertf(isaligned((uintptr_t)self->s_ldt.ldt_vector,PAGEALIGN),"%p",self->s_ldt.ldt_vector);
 /* Map the LDT in the kernel-internal area of memory. */
 self->s_ldt.ldt_base = (__user struct ksegment *)
  kpagedir_mapanyex(self->s_pd,self->s_ldt.ldt_vector,ceildiv(limit,PAGESIZE),
                    KPAGEDIR_MAPANY_HINT_KINTERN,KSHM_LDT_PAGEFLAGS);
 if __unlikely(!self->s_ldt.ldt_base) goto err_ldtvec;
 /* Re-align the mapped table base to fit match the physical address. */
 k_syslogf(KLOG_DEBUG,"[SHM] Mapped LDT vector at %p (limit %I16x; %Iu pages)\n",
           self->s_ldt.ldt_base,limit,(size_t)ceildiv(limit,PAGESIZE));
 return KE_OK;
err_ldtvec:
 free(self->s_ldt.ldt_vector);
 return KE_NOMEM;
}

static kerrno_t
kshm_initldt(struct kshm *__restrict self,
             __u16 size_hint) {
 struct ksegment ldt_segment;
 kassert_kshm(self);
 /* Initialize the LDT table with the given hint. */
 if __unlikely(KE_ISERR(kshm_alloc_and_map_ldt_vector(self,
                        align(size_hint*sizeof(struct ksegment),BUFSIZE)
               ))) return KE_NOMEM;
 /* Encode the LDT segment. */
 ksegment_encode(&ldt_segment,
                (uintptr_t)self->s_ldt.ldt_base,
                (uintptr_t)self->s_ldt.ldt_limit,
                 SEG_LDT);
 /* Allocate a new GDT entry for our per-process segment. */
 self->s_ldt.ldt_gdtid = kgdt_alloc(&ldt_segment);
 if __unlikely(!self->s_ldt.ldt_gdtid) goto err_ldtvec;
 /* And we are ready! */
 return KE_OK;
err_ldtvec:
 free(self->s_ldt.ldt_vector);
 return KE_NOMEM;
}

static kerrno_t
kshm_initldtcopy(struct kshm *__restrict self,
                 struct kshm *__restrict right) {
 struct ksegment ldt_segment;
 kassert_kshm(self);
 kassertobj(right);
 /* Copy the LDT descriptor table. */
 if __unlikely(KE_ISERR(kshm_alloc_and_map_ldt_vector(self,
                        right->s_ldt.ldt_limit
               ))) return KE_NOMEM;
 /* Copy the segment data from the given right-hand-side SHM. */
 memcpy(self->s_ldt.ldt_vector,right->s_ldt.ldt_vector,
        right->s_ldt.ldt_limit);
 /* Encode the LDT segment. */
 ksegment_encode(&ldt_segment,
                (uintptr_t)self->s_ldt.ldt_base,
                (uintptr_t)self->s_ldt.ldt_limit,
                 SEG_LDT);
 /* Allocate a new GDT entry for our per-process segment. */
 self->s_ldt.ldt_gdtid = kgdt_alloc(&ldt_segment);
 if __unlikely(self->s_ldt.ldt_gdtid == KSEG_NULL) goto err_ldtvec;
 /* And we are ready! */
 return KE_OK;
err_ldtvec:
 free(self->s_ldt.ldt_vector);
 return KE_NOMEM;
}
static void
kshm_quitldt(struct kshm *__restrict self) {
 kgdt_free(self->s_ldt.ldt_gdtid);
 free(self->s_ldt.ldt_vector);
}


static kerrno_t
kshm_ldtsetlimit(struct kshm *__restrict self, __u16 newlimit) {
 __kernel struct ksegment *new_kernel_vector,*old_kernel_vector;
 __user struct ksegment *new_kernel_mapping,*old_kernel_mapping;
 size_t usage_table_size,old_limit; struct ksegment ldt_segment;
 assert(newlimit != self->s_ldt.ldt_limit);
 assert((newlimit % sizeof(struct ksegment)) == 0);
 old_limit = self->s_ldt.ldt_limit;
 old_kernel_vector = self->s_ldt.ldt_vector;
 assert(isaligned((uintptr_t)old_kernel_vector,PAGEALIGN));
 /* Figure out how much we can actually write inside of the existing vector. */
 usage_table_size = malloc_usable_size(old_kernel_vector);
 if (newlimit > usage_table_size) {
  assert(newlimit > old_limit);
  /* Must allocate a new table */
  new_kernel_vector = (__kernel struct ksegment *)valloc(newlimit);
  if __unlikely(!new_kernel_vector) return KE_NOMEM;
  assert(isaligned((uintptr_t)new_kernel_vector,PAGEALIGN));
  memcpy(new_kernel_vector,old_kernel_vector,old_limit);
  memset((void *)((uintptr_t)new_kernel_vector+old_limit),
         0,newlimit-old_limit);
  /* Map the new table. */
  new_kernel_mapping = (__user struct ksegment *)
   kpagedir_mapanyex(self->s_pd,new_kernel_vector,ceildiv(newlimit,PAGESIZE),
                     KPAGEDIR_MAPANY_HINT_KINTERN,KSHM_LDT_PAGEFLAGS);
  if __unlikely(!new_kernel_mapping) { free(new_kernel_mapping); return KE_NOMEM; }
  old_kernel_mapping = self->s_ldt.ldt_base;
  /* Update the GDT pointer entry. */
  self->s_ldt.ldt_base = new_kernel_mapping;
  self->s_ldt.ldt_limit = newlimit;
  /* Encode the LDT segment. */
  ksegment_encode(&ldt_segment,
                 (uintptr_t)new_kernel_mapping,
                 (uintptr_t)newlimit,SEG_LDT);
  /* Update the GDT entry. */
  kgdt_update(self->s_ldt.ldt_gdtid,&ldt_segment);
  /* Unmap the user-mapping of the old LDT vector. */
  kpagedir_unmap(self->s_pd,old_kernel_mapping,ceildiv(old_limit,PAGESIZE));
  /* Free the old LDT vector. */
  free(old_kernel_vector);
 } else {
  /* Can re-use the existing LDT vector. - Only need to update its limit. */
  self->s_ldt.ldt_limit = newlimit;
  /* Encode the LDT segment. */
  ksegment_encode(&ldt_segment,
                 (uintptr_t)self->s_ldt.ldt_base,
                 (uintptr_t)newlimit,SEG_LDT);
  /* Update the GDT entry. */
  kgdt_update(self->s_ldt.ldt_gdtid,&ldt_segment);
 }
 return KE_OK;
}

__crit __nomp ksegid_t
kshm_ldtalloc(struct kshm *__restrict self,
              struct ksegment const *__restrict seg) {
 struct ksegment *iter,*begin;
 ksegid_t result;
 kassert_kshm(self);
 kassertobj(seg);
 assertf(!(self->s_ldt.ldt_limit % sizeof(struct ksegment)),"Internal LDT alignment error");
 assertf(seg->present,"The given segment configuration must be marked as present");
 begin = self->s_ldt.ldt_vector;
 iter = (struct ksegment *)((uintptr_t)begin+self->s_ldt.ldt_limit);
 while (iter-- != begin) if (!iter->present) {
  /* Found an empty slot */
  memcpy(iter,seg,sizeof(struct ksegment));
  result = (ksegid_t)((uintptr_t)iter-(uintptr_t)begin);
  k_syslogf(KLOG_DEBUG,"[SHM] Handing out LDT segment %I16x\n",result);
  goto end;
 }
 /* Must allocate a new segment */
 result = self->s_ldt.ldt_limit;
 if __unlikely(KE_ISERR(kshm_ldtsetlimit(self,
                        align(self->s_ldt.ldt_limit+sizeof(struct ksegment),BUFSIZE))
               )) return KSEG_NULL;
 /* Use the lowest LDT address to speed up the next call to ldtalloc! */
 iter = SEGAT(result);
 assert(iter < VEC_END);
 memcpy(iter,seg,sizeof(struct ksegment));
end:
 return KSEG_TOLDT(result);
}

__crit __nomp ksegid_t
kshm_ldtallocat(struct kshm *__restrict self, ksegid_t reqid,
                struct ksegment const *__restrict seg) {
 struct ksegment *result_seg;
 kassert_kshm(self);
 kassertobj(seg);
 assertf(!(self->s_ldt.ldt_limit % sizeof(struct ksegment)),"Internal LDT alignment error");
 assertf(seg->present,"The given segment configuration must be marked as present");
 assertf(KSEG_ISLDT(reqid),"The given segment ID %I16x isn't part of the LDT",reqid);
 assertf(!(reqid&0x3),"The given segment ID %I16x must not include privilege information",reqid);
 reqid &= ~0x7; /* Strip away internal bits. */
 if (reqid >= self->s_ldt.ldt_limit) {
  /* Must allocate more memory. */
  if __unlikely(KE_ISERR(kshm_ldtsetlimit(self,
                         align(reqid+sizeof(struct ksegment),BUFSIZE))
                )) return KSEG_NULL;
  result_seg = SEGAT(reqid);
 } else {
  result_seg = SEGAT(reqid);
  if __unlikely(result_seg->present)
   return KSEG_NULL; /* Segment already in use. */
 }
 memcpy(result_seg,seg,sizeof(struct ksegment));
 return KSEG_TOLDT(reqid);
}
__crit __nomp void
kshm_ldtfree(struct kshm *__restrict self, ksegid_t id) {
 struct ksegment *iter,*begin,*end;
 kassert_kshm(self);
 assertf(!(self->s_ldt.ldt_limit % sizeof(struct ksegment)),"Internal LDT alignment error");
 assertf(KSEG_ISLDT(id),"The given ID %I16x isn't part of the LDT",id);
 id &= ~0x7; /* Strip away internal bits. */
 assertf(id < self->s_ldt.ldt_limit,"The given ID %I16x is out-of-bounds",id);
 iter = SEGAT(id);
 assertf(iter->present,"The segment described by %I16x isn't present",id);
 memset(iter,0,sizeof(struct ksegment));
 begin = VEC_BEGIN,iter = end = VEC_END;
 while (iter != begin && !iter[-1].present) --iter;
 if (iter != end) {
  __u16 new_limit = align((uintptr_t)iter-(uintptr_t)begin,BUFSIZE);
  assert(new_limit <= self->s_ldt.ldt_limit);
  /* Try to reduce memory usage by releasing unused LDT entires. */
  if ((self->s_ldt.ldt_limit-new_limit) >= (BUFSIZE*2)
      ) kshm_ldtsetlimit(self,new_limit);
 }
}

__nomp void
kshm_ldtget(struct kshm const *__restrict self,
            ksegid_t id, struct ksegment *__restrict seg) {
 kassert_kshm(self);
 assertf(!(self->s_ldt.ldt_limit % sizeof(struct ksegment)),"Internal LDT alignment error");
 assertf(KSEG_ISLDT(id),"The given ID %I16x isn't part of the LDT",id);
 id &= ~0x7; /* Strip away internal bits. */
 assertf(id < self->s_ldt.ldt_limit,"The given ID %I16x is out-of-bounds",id);
 memcpy(seg,SEGAT(id),sizeof(struct ksegment));
}
__nomp void
kshm_ldtset(struct kshm *__restrict self, ksegid_t id,
            struct ksegment const *__restrict seg) {
 kassert_kshm(self);
 assertf(!(self->s_ldt.ldt_limit % sizeof(struct ksegment)),"Internal LDT alignment error");
 assertf(KSEG_ISLDT(id),"The given ID %I16x isn't part of the LDT",id);
 id &= ~0x7; /* Strip away internal bits. */
 assertf(id < self->s_ldt.ldt_limit,"The given ID %I16x is out-of-bounds",id);
 memcpy(SEGAT(id),seg,sizeof(struct ksegment));
}

#undef BUFSIZE
#undef SELF
#undef SEGAT
#undef VEC_BEGIN
#undef VEC_SIZE
#undef VEC_END
              
__DECL_END

#endif /* !__KOS_KERNEL_SHM2_LDT_C_INL__ */

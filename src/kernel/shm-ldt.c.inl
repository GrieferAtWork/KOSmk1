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
#ifndef __KOS_KERNEL_SHM_LDT_C_INL__
#define __KOS_KERNEL_SHM_LDT_C_INL__ 1

#include <assert.h>
#include <malloc.h>
#include <kos/config.h>
#include <kos/kernel/debug.h>
#include <kos/kernel/gdt.h>
#include <kos/kernel/shm.h>
#include <kos/syslog.h>

__DECL_BEGIN

static kerrno_t kshm_alloc_and_map_ldt_vector(struct kshm *self, __u16 limit) {
 size_t page_offset,pages;
 __kernel void *ldt_page;
 kassert_kshm(self);
 assert(!(limit%sizeof(struct ksegment)));
 /* Initialize the LDT table with the given hint. */
 self->sm_ldt.ldt_table.limit = limit;
 self->sm_ldt.ldt_vector = (__kernel struct ksegment *)memalign(KSHM_LDT_ALIGNMENT,limit);
 if __unlikely(!self->sm_ldt.ldt_vector) return KE_NOMEM;
 memset(self->sm_ldt.ldt_vector,0,limit);
 ldt_page = (void *)alignd((uintptr_t)self->sm_ldt.ldt_vector,PAGEALIGN);
 /* Map the LDT in the kernel-internal area of memory. */
 page_offset = (uintptr_t)self->sm_ldt.ldt_vector-(uintptr_t)ldt_page;
 pages       = ceildiv(page_offset+limit,PAGESIZE);
 self->sm_ldt.ldt_table.base = (__user struct ksegment *)
  kpagedir_mapanyex(self->sm_pd,ldt_page,pages,
                    KPAGEDIR_MAPANY_HINT_KINTERN,KSHM_LDT_PAGEFLAGS);
 if __unlikely(!self->sm_ldt.ldt_table.base) goto err_ldtvec;
 *(uintptr_t *)&self->sm_ldt.ldt_table.base += page_offset;
 /* Re-align the mapped table base to fit match the physical address. */
 k_syslogf(KLOG_INFO,"[SHM] Mapped LDT vector at %p (limit %I16x; %Iu pages)\n",
           self->sm_ldt.ldt_table.base,limit,pages);
 return KE_OK;
err_ldtvec:
 free(self->sm_ldt.ldt_vector);
 return KE_NOMEM;
}

static kerrno_t kshm_initldt(struct kshm *self, __u16 size_hint) {
 struct ksegment ldt_segment;
 kassert_kshm(self);
 /* Initialize the LDT table with the given hint. */
 if __unlikely(KE_ISERR(kshm_alloc_and_map_ldt_vector(self,
                        align(size_hint,KSHM_LDT_BUFSIZE)*sizeof(struct ksegment)
               ))) return KE_NOMEM;
 /* Encode the LDT segment. */
 ksegment_encode(&ldt_segment,
                (uintptr_t)self->sm_ldt.ldt_table.base,
                (uintptr_t)self->sm_ldt.ldt_table.limit,
                 SEG_LDT);
 /* Allocate a new GDT entry for our per-process segment. */
 self->sm_ldt.ldt_gdtid = kgdt_alloc(&ldt_segment);
 if __unlikely(!self->sm_ldt.ldt_gdtid) goto err_ldtvec;
 /* And we are ready! */
 return KE_OK;
err_ldtvec:
 free(self->sm_ldt.ldt_vector);
 return KE_NOMEM;
}

static kerrno_t kshm_initldtcopy(struct kshm *self, struct kshm *right) {
 struct ksegment ldt_segment;
 kassert_kshm(self);
 kassertobj(right);
 /* Copy the LDT descriptor table. */
 if __unlikely(KE_ISERR(kshm_alloc_and_map_ldt_vector(self,
                        right->sm_ldt.ldt_table.limit
               ))) return KE_NOMEM;
 /* Copy the segment data from the given right-hand-side SHM. */
 memcpy(self->sm_ldt.ldt_vector,right->sm_ldt.ldt_vector,
        right->sm_ldt.ldt_table.limit);
 /* Encode the LDT segment. */
 ksegment_encode(&ldt_segment,
                (uintptr_t)self->sm_ldt.ldt_table.base,
                (uintptr_t)self->sm_ldt.ldt_table.limit,
                 SEG_LDT);
 /* Allocate a new GDT entry for our per-process segment. */
 self->sm_ldt.ldt_gdtid = kgdt_alloc(&ldt_segment);
 if __unlikely(self->sm_ldt.ldt_gdtid == KSEG_NULL) goto err_ldtvec;
 /* And we are ready! */
 return KE_OK;
err_ldtvec:
 free(self->sm_ldt.ldt_vector);
 return KE_NOMEM;
}
static void kshm_quitldt(struct kshm *self) {
 kgdt_free(self->sm_ldt.ldt_gdtid);
 free(self->sm_ldt.ldt_vector);
}



__nomp __crit ksegid_t
kshm_ldtalloc(struct kshm *self, struct ksegment const *seg) {
 struct ksegment *iter,*begin; ksegid_t result;
 kassert_kshm(self);
 kassertobj(seg);
 assert(!(self->sm_ldt.ldt_table.limit % sizeof(struct ksegment)));
 assert(seg->present);
 begin = self->sm_ldt.ldt_vector;
 iter = (struct ksegment *)((uintptr_t)begin+self->sm_ldt.ldt_table.limit);
 while (iter-- != begin) if (!iter->present) {
  /* Found an empty slot */
  memcpy(iter,seg,sizeof(struct ksegment));
  result = (ksegid_t)((uintptr_t)iter-(uintptr_t)begin);
  /*GDT_UPDATE();*/
  k_syslogf(KLOG_INFO,"[SHM] Handing out LDT segment %I16x\n",result);
  goto end;
 }
 /* Must allocate a new segment */
 assertf(0,"TODO");
end:
 return KSEG_TOLDT(result);
}
__nomp __crit ksegid_t
kshm_ldtallocat(struct kshm *self, ksegid_t reqid, struct ksegment const *seg) {
 return 0; // TODO
}
__nomp __crit void
kshm_ldtfree(struct kshm *self, ksegid_t id) {
 kassert_kshm(self);
 assert(KSEG_ISLDT(id));
 id = id & (sizeof(struct ksegment)-1);
 assert(id < self->sm_ldt.ldt_table.limit);
 // TODO
}

__nomp __crit void
kshm_ldtget(struct kshm const *self,
            ksegid_t id, struct ksegment *seg) {
 // TODO
}
__nomp __crit void
kshm_ldtset(struct kshm *self, ksegid_t id,
            struct ksegment const *seg) {
 // TODO
}


__DECL_END

#endif /* !__KOS_KERNEL_SHM_LDT_C_INL__ */

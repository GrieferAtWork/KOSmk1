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
#ifndef __KOS_KERNEL_SHM_C__
#define __KOS_KERNEL_SHM_C__ 1

#include <assert.h>
#include <stddef.h>
#include <kos/config.h>
#include <kos/kernel/debug.h>
#include <kos/kernel/shm.h>
#include <kos/kernel/pageframe.h>
#include <kos/kernel/paging.h>
#include <kos/kernel/task.h>
#include <kos/kernel/proc.h>
#include <math.h>
#include <stdint.h>
#include <kos/syslog.h>

__DECL_BEGIN


/* Automatically merge branches
 * >> s.a. long comment atop of the large
 *    AUTOMERGE_BRANCHES block near the end of kshm_touch. */
#define AUTOMERGE_BRANCHES 1

/* Define the log_level for touch+merge syslog entries. */
#define COW_LOGLEVEL  KLOG_TRACE


static kerrno_t kshm_initldt(struct kshm *__restrict self, kseglimit_t size_hint);
static kerrno_t kshm_initldtcopy(struct kshm *__restrict self, struct kshm *__restrict right);
static void kshm_quitldt(struct kshm *__restrict self);




//////////////////////////////////////////////////////////////////////////
// ===== kshmchunk
__crit kerrno_t
kshmchunk_initlinear(struct kshmchunk *__restrict self,
                     size_t pages, kshm_flag_t flags) {
 KTASK_CRIT_MARK
 assertf(pages != 0,"Cannot create an empty SHM chunk");
 kobject_init(self,KOBJECT_MAGIC_SHMCHUNK);
 self->sc_partv = (struct kshmpart *)malloc(1*sizeof(struct kshmpart));
 if __unlikely(!self->sc_partv) return KE_NOMEM;
 self->sc_partv[0].sp_start = 0;
 self->sc_partv[0].sp_pages = pages;
 self->sc_partv[0].sp_frame = kpageframe_alloc(pages);
 self->sc_partv[0].sp_futex.fp_futex = 0;
 if __unlikely(self->sc_partv[0].sp_frame == PAGEFRAME_NIL) goto err_r;
 self->sc_flags = flags&~(KSHMREGION_FLAG_NOFREE);
 self->sc_pages = pages;
 self->sc_partc = 1;
 return KE_OK;
err_r:
 free(self->sc_partv);
 return KE_NOMEM;
}


__crit kerrno_t
kshmchunk_initphys(struct kshmchunk *__restrict self,
                   __pagealigned __kernel void *addr,
                   size_t pages, kshm_flag_t flags) {
 assertf(pages != 0,"Cannot create an empty SHM chunk");
 kobject_init(self,KOBJECT_MAGIC_SHMCHUNK);
 self->sc_partv = (struct kshmpart *)malloc(1*sizeof(struct kshmpart));
 if __unlikely(!self->sc_partv) return KE_NOMEM;
 self->sc_flags             = flags|KSHMREGION_FLAG_NOFREE;
 self->sc_pages             = pages;
 self->sc_partc             = 1;
 self->sc_partv[0].sp_frame = (__pagealigned struct kpageframe *)addr;
 self->sc_partv[0].sp_start = 0;
 self->sc_partv[0].sp_pages = pages;
 self->sc_partv[0].sp_futex.fp_futex = 0;
 return KE_OK;
}


__crit kerrno_t
kshmchunk_initram(struct kshmchunk *__restrict self,
                  size_t pages, kshm_flag_t flags) {
 struct kshmpart *partvec,*newpartvec;
 size_t total_page_count;
 assertf(pages != 0,"Cannot create an empty SHM chunk");
 kobject_init(self,KOBJECT_MAGIC_SHMCHUNK);
 /* Assume most likely case: Linear allocation (aka. ONE(1) part). */
 partvec = (struct kshmpart *)malloc(1*sizeof(struct kshmpart));
 if __unlikely(!partvec) return KE_NOMEM;
 self->sc_flags      = flags&~(KSHMREGION_FLAG_NOFREE);
 self->sc_pages      = pages;
 self->sc_partc      = 1;
 self->sc_partv      = partvec;
 partvec[0].sp_frame = kpageframe_tryalloc(pages,&total_page_count);
 partvec[0].sp_start = 0;
 partvec[0].sp_pages = total_page_count;
 partvec[0].sp_futex.fp_futex = 0;
 if __unlikely(total_page_count != pages) {
  /* Must allocate more / failed to allocate more. */
  if __unlikely(self->sc_partv[0].sp_frame == PAGEFRAME_NIL) goto err_partvec;
  for (;;) {
   newpartvec = (struct kshmpart *)realloc(partvec,(self->sc_partc+1)*
                                           sizeof(struct kshmpart));
   if __unlikely(!newpartvec) {
    struct kshmpart *iter;
err_free_got_parts:
    iter = self->sc_partv+self->sc_partc;
    do --iter,kpageframe_free(iter->sp_frame,
                              iter->sp_pages);
    while (iter != partvec);
    goto err_partvec;
   }
   self->sc_partv = newpartvec;
   newpartvec += self->sc_partc;
   if __unlikely((newpartvec->sp_frame =
                  kpageframe_tryalloc(pages,&newpartvec->sp_pages)
                  ) == PAGEFRAME_NIL) goto err_free_got_parts;
   newpartvec->sp_futex.fp_futex = 0;
   ++self->sc_partc;
   total_page_count += newpartvec->sp_pages;
   assert(total_page_count <= pages);
   if (total_page_count == pages) break;
  }
 }
 return KE_OK;
err_partvec:
 free(partvec);
 return KE_NOMEM;
}


__crit kerrno_t
kshmchunk_inithardcopy(struct kshmchunk *__restrict self,
                       struct kshmchunk const *__restrict right) {
 kerrno_t error;
 kassert_kshmchunk(right);
 kassertobj(self);
 assert(right->sc_partc);
 assert(right->sc_pages);
 if (right->sc_flags&KSHMREGION_FLAG_NOCOPY) {
  /* Special case: Alias existing memory instead of copying it. */
  /* WARNING: Though neither enforced here or anywhere else, you should
   *          probably only use the 'KSHMREGION_FLAG_NOCOPY' flag in
   *          conjunction with 'KSHMREGION_FLAG_NOFREE'. */
  self->sc_pages = right->sc_pages;
  self->sc_flags = right->sc_flags;
  self->sc_partc = right->sc_partc;
  self->sc_partv = (struct kshmpart *)memdup(self,right->sc_partc*
                                             sizeof(struct kshmpart));
  return self->sc_partv ? KE_OK : KE_NOMEM;
 }
 /* Create hard-copy of the given chunk, duplicating all of its memory. */
 error = kshmchunk_initram(self,right->sc_pages,right->sc_flags);
 if __unlikely(KE_ISERR(error)) return error;
 assert(self->sc_partc != 0);
 if __likely(self->sc_partc == 1) {
  /* Optimization for most likely case... */
  kshmchunk_copypages(self,0,self->sc_partv[0].sp_frame,self->sc_pages);
 } else {
  struct kshmpart *iter,*end;
  kshmchunk_page_t page_position = 0;
  end = (iter = self->sc_partv)+self->sc_partc;
  do kshmchunk_copypages(self,page_position,
                         iter->sp_frame,iter->sp_pages),
     page_position += iter->sp_pages;
  while (++iter != end);
 }
 return error;
}

void kshmchunk_copypages(struct kshmchunk const *self,
                         kshmchunk_page_t first_page,
                         __kernel void *buf, size_t page_count) {
 struct kshmpart const *iter;
 kshmregion_page_t page_position,copy_end;
 kassert_kshmchunk(self);
 kassertmem(buf,page_count*PAGESIZE);
 assert(self->sc_pages);
 assert(self->sc_partc);
 assert(first_page+page_count >= first_page);
 assertf(first_page+page_count < self->sc_pages,
         "The given range is out-of-bounds: %Iu >= %Iu",
         first_page+page_count,self->sc_pages);
 if __likely(self->sc_partc == 1) {
  /* Optimize most likely case. */
  kpageframe_memcpy((struct kpageframe *)buf,
                    self->sc_partv[0].sp_frame+first_page,
                    page_count);
  return;
 }
 iter = self->sc_partv;
 copy_end = first_page+page_count;
 for (page_position = 0;
      page_position != copy_end;
      page_position += (iter++)->sp_pages) {
  size_t copy_max;
  assert(page_position < copy_end);
  assert(iter < self->sc_partv+self->sc_partc);
  copy_max = min(copy_end-page_position,iter->sp_pages);
  kpageframe_memcpy((struct kpageframe *)buf,
                    iter->sp_frame,copy_max);
  *(uintptr_t *)&buf += copy_max*PAGESIZE;
 }
}

__crit void
kshmchunk_quit(struct kshmchunk *__restrict self) {
 struct kshmpart *iter,*end;
 kassert_kshmchunk(self);
 if (!(self->sc_flags&KSHMREGION_FLAG_NOFREE)) {
  end = (iter = self->sc_partv)+self->sc_partc;
  for (; iter != end; ++iter) kpageframe_free(iter->sp_frame,iter->sp_pages);
 }
 free(self->sc_partv);
}








//////////////////////////////////////////////////////////////////////////
// ===== kshmregion
__crit kerrno_t
kshmregion_incref_clusters(struct kshmregion *__restrict self,
                           struct kshmcluster *cls_min,
                           struct kshmcluster *cls_max) {
 struct kshmcluster *iter;
 kshmrefcnt_t oldrefcnt,newrefcnt;
 KTASK_CRIT_MARK
 kassert_kshmregion(self);
 assert(cls_min >= self->sre_clusterv && cls_min < self->sre_clusterv+self->sre_clusterc);
 assert(cls_max >= self->sre_clusterv && cls_max < self->sre_clusterv+self->sre_clusterc);
 assert(cls_min <= cls_max);
 iter = cls_min;
 do {
  do {
   oldrefcnt = katomic_load(iter->sc_refcnt);
   assertf(oldrefcnt != 0,"Dead cluster at %p (%Iu)",
           iter,iter-self->sre_clusterv);
   newrefcnt = oldrefcnt+1;
   if __unlikely(!newrefcnt) goto overflow;
  } while (!katomic_cmpxch(iter->sc_refcnt,oldrefcnt,newrefcnt));
 } while (iter++ != cls_max);
 return KE_OK;
overflow:
 if (iter-- != cls_min) kshmregion_decref(self,cls_min,iter);
 return KE_OVERFLOW;
}
__crit kerrno_t
kshmregion_incref(struct kshmregion *__restrict self,
                  struct kshmcluster *cls_min,
                  struct kshmcluster *cls_max) {
 kerrno_t error;
 asserte(katomic_fetchinc(self->sre_branches) != 0);
 error = kshmregion_incref_clusters(self,cls_min,cls_max);
 if __unlikely(KE_ISERR(error)) {
  asserte(katomic_decfetch(self->sre_branches) != 0);
 }
 return error;
}

#ifdef __DEBUG__
/* Called when 'self->sre_clustera' reaches ZERO(0) to assert that
 * all cluster reference counters have really dropped to ZERO(0). */
static void
kshmregion_assert_reallynoclusters(struct kshmregion *__restrict self) {
 struct kshmcluster *citer,*cend;
 cend = (citer = self->sre_clusterv)+self->sre_clusterc;
 for (; citer != cend; ++citer) {
  kshmrefcnt_t refs = katomic_load(citer->sc_refcnt);
  assertf(!refs,
          "Cluster %Iu/%Iu still alive (%I32u references)",
          citer-self->sre_clusterv,self->sre_clusterc,refs);
 }
}
#else
#define kshmregion_assert_reallynoclusters(self) (void)0
#endif

//////////////////////////////////////////////////////////////////////////
// Free all parts associated with the given cluster.
static __crit void
kshmregion_freecluster(struct kshmregion *__restrict self,
                       struct kshmcluster *__restrict cluster) {
 struct kshmpart *iter,*end;
 kshmregion_page_t cluster_start;
 kshmregion_page_t cluster_delete = KSHM_CLUSTERSIZE;
 size_t max_pages;
 kassert_kshmregion(self);
 kassertobj(cluster);
 assert(cluster >= self->sre_clusterv &&
        cluster <  self->sre_clusterv+self->sre_clusterc);
 cluster_start = (cluster-self->sre_clusterv)*KSHM_CLUSTERSIZE;
 iter = cluster->sc_part;
 kassertobj(iter);
 assert(iter >= self->sre_chunk.sc_partv &&
        iter <  self->sre_chunk.sc_partv+self->sre_chunk.sc_partc);
 assertf(cluster_start >= iter->sp_start &&
         cluster_start <  iter->sp_start+iter->sp_pages,
         "Region cluster page %Iu (cluster %Iu) "
         "is out-of-bounds of associated part %Iu (covers %Iu..%Iu)\n"
         "self->sre_clusterc       = %Iu\n"
         "self->sre_chunk.sc_pages = %Iu\n"
         "self->sre_chunk.sc_partc = %Iu\n",
         cluster_start,cluster_start/KSHM_CLUSTERSIZE,
        (size_t)(iter-self->sre_chunk.sc_partv),
         iter->sp_start,iter->sp_start+iter->sp_pages,
         self->sre_clusterc,
         self->sre_chunk.sc_pages,
         self->sre_chunk.sc_partc);
 cluster_start -= iter->sp_start;
 end = self->sre_chunk.sc_partv+self->sre_chunk.sc_partc;
 /* Delete all frames associated with this cluster. */
#ifndef __DEBUG__
 if (!(self->sre_chunk.sc_flags&KSHMREGION_FLAG_NOFREE))
#endif
 for (;;) {
  assertf(iter >= self->sre_chunk.sc_partv && iter < end
         ,"Part index %Id is out of bounds of 0..%Iu ()"
         ,iter-self->sre_chunk.sc_partv
         ,     self->sre_chunk.sc_partc);
  max_pages = min(cluster_delete,iter->sp_pages-cluster_start);
#ifdef __DEBUG__
  assert(iter->sp_frame != NULL);
  if (!(self->sre_chunk.sc_flags&KSHMREGION_FLAG_NOFREE))
#endif
  {
   kpageframe_free(iter->sp_frame+cluster_start,max_pages);
  }
#ifdef __DEBUG__
  if (!cluster_start && max_pages == iter->sp_pages) iter->sp_frame = NULL;
#endif
  if ((cluster_delete -= max_pages) == 0) break;
  cluster_start = 0;
  /* The last cluster may not be fully used. - Handled by checking for part end. */
  if (++iter == end) break;
 }
 /* Drop the reference the cluster had on the region. */
#ifdef __DEBUG__
 {
  size_t cnt = katomic_fetchdec(self->sre_clustera);
  assert(cnt != 0);
  if (cnt == 1) kshmregion_assert_reallynoclusters(self);
 }
#else
 asserte(katomic_fetchdec(self->sre_clustera) != 0);
#endif
}

__crit void
kshmregion_decref_clusters(struct kshmregion *__restrict self,
                           struct kshmcluster *cls_min,
                           struct kshmcluster *cls_max) {
 KTASK_CRIT_MARK
 kassert_kshmregion(self);
 assert(cls_min >= self->sre_clusterv && cls_min < self->sre_clusterv+self->sre_clusterc);
 assert(cls_max >= self->sre_clusterv && cls_max < self->sre_clusterv+self->sre_clusterc);
 assert(cls_min <= cls_max);
 do {
#ifdef __DEBUG__
  kshmrefcnt_t refcnt;
  refcnt = katomic_fetchdec(cls_min->sc_refcnt);
  assertf(refcnt != 0,"Dead cluster at %p (%Iu)",
          cls_min,cls_min-self->sre_clusterv);
  if __unlikely(refcnt == 1)
#else
  if __unlikely(!katomic_decfetch(cls_min->sc_refcnt))
#endif
  {
   /* Last reference was removed. */
   kshmregion_freecluster(self,cls_min);
  }
 } while (cls_min++ != cls_max);
}

__crit void
kshmregion_decref(struct kshmregion *__restrict self,
                  struct kshmcluster *cls_min,
                  struct kshmcluster *cls_max) {
 KTASK_CRIT_MARK
 kassert_kshmregion(self);
 kshmregion_decref_clusters(self,cls_min,cls_max);
 { /* Drop a reference counter the branch counter. */
#ifdef __DEBUG__
  size_t refcnt = katomic_decfetch(self->sre_branches);
  assert(refcnt != (size_t)-1);
  if (!refcnt)
#else
  if (!katomic_decfetch(self->sre_branches))
#endif
  {
   free(self->sre_chunk.sc_partv);
   free(self);
  }
 }
}



#define SIZEOF_KSHMREGION(cluster_c) \
 (offsetof(struct kshmregion,sre_clusterv)+\
 (cluster_c)*sizeof(struct kshmcluster))


//////////////////////////////////////////////////////////////////////////
// ===== kshmregion

/* Allocate a new SHM region without its part list initialized. */
#define kshmregion_raw_free  free
static __crit __ref struct kshmregion *
kshmregion_raw_alloc(size_t clusters, kshm_flag_t flags) {
 __ref struct kshmregion *result;
 result = (__ref struct kshmregion *)malloc(SIZEOF_KSHMREGION(clusters));
 if __unlikely(!result) return NULL;
 kobject_init(result,KOBJECT_MAGIC_SHMREGION);
 result->sre_origin   = NULL;
 result->sre_branches = 1;
 result->sre_clustera = clusters;
 result->sre_clusterc = clusters;
 return result;
}

static __crit void 
kshmregion_setup_clusters(struct kshmregion *__restrict self) {
 struct kshmcluster *iter,*end;
 struct kshmpart *part;
 kshmregion_page_t part_page = 0;
 kassert_kshmregion(self);
 assert(self->sre_clusterc);
 assert(self->sre_chunk.sc_partc);
 assert(self->sre_clusterc == ceildiv(self->sre_chunk.sc_pages,KSHM_CLUSTERSIZE));
 end = (iter = self->sre_clusterv)+self->sre_clusterc;
 part = self->sre_chunk.sc_partv;
 do {
  while ((assert(part != self->sre_chunk.sc_partv+
                         self->sre_chunk.sc_partc),
          part_page >= part->sp_pages)) {
   part_page -= part->sp_pages;
   ++part;
  }
  iter->sc_part   = part;
  iter->sc_refcnt = 1;
  part_page      += KSHM_CLUSTERSIZE;
 } while (++iter != end);
}


#define CLUSTERS_FOR_PAGES(pages) ceildiv(pages,KSHM_CLUSTERSIZE)


__crit __ref struct kshmregion *
kshmregion_newram(size_t pages, kshm_flag_t flags) {
 __ref struct kshmregion *result;
 result = kshmregion_raw_alloc(CLUSTERS_FOR_PAGES(pages),flags);
 if __unlikely(!result) return NULL;
 if __unlikely(KE_ISERR(kshmchunk_initram(
                        &result->sre_chunk,pages,flags)
               )) kshmregion_raw_free(result),result = NULL;
 else kshmregion_setup_clusters(result);
 return result;
}
__crit __ref struct kshmregion *
kshmregion_newlinear(size_t pages, kshm_flag_t flags) {
 __ref struct kshmregion *result;
 result = kshmregion_raw_alloc(CLUSTERS_FOR_PAGES(pages),flags);
 if __unlikely(!result) return NULL;
 if __unlikely(KE_ISERR(kshmchunk_initlinear(
                        &result->sre_chunk,pages,flags)
               )) kshmregion_raw_free(result),result = NULL;
 else kshmregion_setup_clusters(result);
 return result;
}
__crit __ref struct kshmregion *
kshmregion_newphys(__pagealigned __kernel void *addr,
                   size_t pages, kshm_flag_t flags) {
 __ref struct kshmregion *result;
 result = kshmregion_raw_alloc(CLUSTERS_FOR_PAGES(pages),flags);
 if __unlikely(!result) return NULL;
 if __unlikely(KE_ISERR(kshmchunk_initphys(
                        &result->sre_chunk,addr,pages,flags)
               )) kshmregion_raw_free(result),result = NULL;
 else kshmregion_setup_clusters(result);
 return result;
}


__crit __ref struct kshmregion *
kshmregion_merge(__ref struct kshmregion *__restrict min_region,
                 __ref struct kshmregion *__restrict max_region) {
 __ref struct kshmregion *result;
 size_t total_pages,total_clusters,total_parts;
 struct kshmpart *minpart_max,*maxpart_min;
 struct kshmpart *partvec,*iter,*end;
 struct kshmcluster *citer,*cend;
 kshmregion_page_t part_start;
 kassert_kshmregion(min_region);
 kassert_kshmregion(max_region);
 assert(min_region->sre_branches == 1);
 assert(max_region->sre_branches == 1);
 assert(min_region->sre_chunk.sc_partc);
 assert(min_region->sre_clusterc);
 assert(max_region->sre_chunk.sc_partc);
 assert(max_region->sre_clusterc);
 assert(min_region->sre_chunk.sc_flags ==
        max_region->sre_chunk.sc_flags);
 assert(min_region->sre_clustera == min_region->sre_clusterc);
 assert(max_region->sre_clustera == max_region->sre_clusterc);
#ifdef __DEBUG__
 {
  /* Although this should already be asserted by 'sre_clustera == sre_clusterc',
   * and 'sre_branches == 1', we also check all cluster reference counters to
   * equal one as well. */
  cend = (citer = min_region->sre_clusterv)+min_region->sre_clusterc;
  for (; citer != cend; ++citer) {
   assertf(citer->sc_refcnt == 1
          ,"Min region cluster %Iu/%Iu has refcnt %I32u"
          ,citer-min_region->sre_clusterv
          ,min_region->sre_clusterc
          ,citer->sc_refcnt);
  }
  cend = (citer = max_region->sre_clusterv)+max_region->sre_clusterc;
  for (; citer != cend; ++citer) {
   assertf(citer->sc_refcnt == 1
          ,"Max region cluster %Iu/%Iu has refcnt %I32u"
          ,citer-max_region->sre_clusterv
          ,max_region->sre_clusterc
          ,citer->sc_refcnt);
  }
 }
#endif
 /* Merge the given regions into one. */
 total_pages = min_region->sre_chunk.sc_pages+
               max_region->sre_chunk.sc_pages;
 total_clusters = ceildiv(total_pages,KSHM_CLUSTERSIZE);
 assert(total_clusters <= min_region->sre_clusterc+max_region->sre_clusterc);
 result = (__ref struct kshmregion *)malloc(SIZEOF_KSHMREGION(total_clusters));
 if __unlikely(!result) return NULL;
 kobject_init(result,KOBJECT_MAGIC_SHMREGION);
 kobject_init(&result->sre_chunk,KOBJECT_MAGIC_SHMCHUNK);
 result->sre_chunk.sc_flags = min_region->sre_chunk.sc_flags;
 result->sre_chunk.sc_pages = total_pages;
 result->sre_origin         = NULL; /*< Without an explicit origin, this region doesn't have one. */
 result->sre_clustera       = total_clusters;
 result->sre_clusterc       = total_clusters;
 result->sre_branches       = 1;
 minpart_max = &min_region->sre_chunk.sc_partv[min_region->sre_chunk.sc_partc-1];
 maxpart_min = &max_region->sre_chunk.sc_partv[0];
 total_parts = min_region->sre_chunk.sc_partc+
               max_region->sre_chunk.sc_partc;
 /* Page start of the max-region. */
 part_start = minpart_max->sp_start+minpart_max->sp_pages;
#if 1
 if (minpart_max->sp_frame+minpart_max->sp_pages == maxpart_min->sp_frame) {
  /* Can combine the last and first parts of the given regions (one less part). */
  --total_parts;
  partvec = (struct kshmpart *)malloc(total_parts*sizeof(struct kshmpart));
  if __unlikely(!partvec) goto err_r;
  result->sre_chunk.sc_partv = partvec;
  memcpy(partvec,min_region->sre_chunk.sc_partv,
                 min_region->sre_chunk.sc_partc*
                 sizeof(struct kshmpart));
  partvec += min_region->sre_chunk.sc_partc-1;
  /* Extend the last part of the min-region. */
  partvec->sp_pages += maxpart_min->sp_pages;
  /* Copy all remaining parts. */
  memcpy(partvec+1,(max_region->sre_chunk.sc_partv+1),
                   (max_region->sre_chunk.sc_partc-1)*
                    sizeof(struct kshmpart));
  /* Update the start offsets of all remaining parts (NOTE: There may not be any remaining). */
  iter = partvec+1,end = partvec+max_region->sre_chunk.sc_partc;
  for (assert(iter <= end); iter != end; ++iter) {
   iter->sp_start = part_start;
   part_start += iter->sp_pages;
  }
 } else
#endif
 {
  /* Regular part merge */
  partvec = (struct kshmpart *)malloc(total_parts*sizeof(struct kshmpart));
  if __unlikely(!partvec) goto err_r;
  result->sre_chunk.sc_partv = partvec;
  memcpy(partvec,min_region->sre_chunk.sc_partv,
                 min_region->sre_chunk.sc_partc*
                 sizeof(struct kshmpart));
  partvec += min_region->sre_chunk.sc_partc;
  memcpy(partvec,max_region->sre_chunk.sc_partv,
                 max_region->sre_chunk.sc_partc*
                 sizeof(struct kshmpart));
  part_start = min_region->sre_chunk.sc_pages;
  end = (iter = partvec)+max_region->sre_chunk.sc_partc;
  assert(iter < end);
  assertf(!iter->sp_start,"First part of max-region must start at ZERO(0)");
  do iter->sp_start += part_start;
  while (++iter != end);
 }
 /* Now that we've extracted all data from the given regions, free() them. */
 free(min_region->sre_chunk.sc_partv);
 free(max_region->sre_chunk.sc_partv);
 free(min_region);
 free(max_region);

 result->sre_chunk.sc_partc = total_parts;
 /* The chunk and all of its parts are now fully initialized.
  * Since part of this function's contract is both regions
  * being fully allocated, this is trivial as we can simply
  * initialize all clusters the normal way and set their
  * reference counters to ONE(1).
  */
 kshmregion_setup_clusters(result);
 return result;
err_r: free(result);
 return NULL;
}

__crit __ref struct kshmregion *
kshmregion_hardcopy(struct kshmregion const *__restrict self,
                    kshmregion_page_t page_offset,
                    size_t pages) {
 kassert_kshmregion(self);
 assert(self->sre_chunk.sc_partc);
 assert(self->sre_clustera);
 assert(self->sre_clusterc);
 assert(page_offset+pages > page_offset);
 assert(page_offset+pages <= self->sre_chunk.sc_pages);

 /* TODO: Only used in fallback code (can be implemented later). */

 return NULL;
}



//////////////////////////////////////////////////////////////////////////
// Add a given part to a chunk during its build process.
#define sc_parta   sc_pages /*< Abuse this field to track allocated size. */
#define sc_owned   sp_start /*< Abuse this field to track parts we actually own. */
__local void kshmchunk_builder_init(struct kshmchunk *self) {
 kassertobj(self);
 kobject_init(self,KOBJECT_MAGIC_SHMCHUNK);
 self->sc_parta = 0;
 self->sc_partc = 0;
 self->sc_partv = NULL;
}
__local void kshmchunk_builder_quit(struct kshmchunk *self) {
 struct kshmpart *iter,*end;
 kassertobj(self);
 end = (iter = self->sc_partv)+self->sc_partc;
 for (; iter != end; ++iter) if (iter->sc_owned) {
  /* Free all parts that we actually own. */
  kpageframe_free(iter->sp_frame,iter->sp_pages);
 }
 free(self->sc_partv);
}
__local void kshmchunk_builder_finish(struct kshmchunk *self) {
 struct kshmpart *iter,*end;
 kshmregion_page_t start = 0;
 /* Try to clean up unused part memory & initialize
  * the 'sp_start' fields of all usable parts. */
 kassertobj(self);
 assert(self->sc_parta >= self->sc_partc);
 assert(self->sc_partv != NULL);
 assert(self->sc_partc != 0);
 if __likely(self->sc_parta != self->sc_partc) {
  iter = (struct kshmpart *)realloc(self->sc_partv,self->sc_partc*
                                    sizeof(struct kshmpart));
  if __likely(iter) self->sc_partv = iter;
 } else iter = self->sc_partv;
#ifdef __DEBUG__
 self->sc_parta = 0;
#endif
 end = iter+self->sc_partc;
 do {
  iter->sp_start  = start;
  start          += iter->sp_pages;
 } while (++iter != end);
 self->sc_pages = start;
}
__local int
kshmchunk_builder_inheritscluster(struct kshmchunk *self,
                                  struct kshmcluster const *cluster) {
 struct kshmpart *iter,*end;
 struct kpageframe *frameaddr;
 kassertobj(self);
 kassertobj(cluster);
 kassertobj(cluster->sc_part);
 end = (iter = self->sc_partv)+self->sc_partc;
 frameaddr = cluster->sc_part->sp_frame;
 for (; iter != end; ++iter) {
  if (!iter->sc_owned && iter->sp_frame == frameaddr) return 1;
 }
 return 0;
}
__local kerrno_t
kshmchunk_builder_addpart(struct kshmchunk *self,
                          __pagealigned struct kpageframe *start,
                          size_t pages, int owned) {
 struct kshmpart *newvec;
 /* Append the given parts to the vector. */
 if __unlikely(self->sc_partc == self->sc_parta) {
  size_t newalloc = self->sc_parta ? self->sc_parta*2 : 2;
  /* Must allocate more memory for vector entries. */
  newvec = (struct kshmpart *)realloc(self->sc_partv,newalloc*
                                      sizeof(struct kshmpart));
  if __unlikely(!newvec) return KE_NOMEM;
  self->sc_parta = newalloc;
  self->sc_partv = newvec;
 } else {
  newvec = self->sc_partv;
 }
 newvec += self->sc_partc++;
 newvec->sp_frame = start;
 newvec->sp_pages = pages;
 newvec->sc_owned = owned;
 newvec->sp_futex.fp_futex = 0;
 return KE_OK;
}
#undef sc_owned
#undef sc_parta



static kerrno_t
kshmchunk_builder_copy_clusters(struct kshmchunk *self,
                                struct kshmregion const *original_region,
                                struct kshmcluster const *first_cluster,
                                size_t cluster_count) {
 struct kshmpart *iter,*end;
 struct kpageframe *framecopy;
 kshmregion_page_t cluster_start;
 kshmregion_page_t cluster_pages = cluster_count*KSHM_CLUSTERSIZE;
 size_t max_pages; kerrno_t error;
 kassertobj(self);
 kassert_kshmregion(original_region);
 assert(cluster_count);
 kassertmem(first_cluster,cluster_count*sizeof(struct kshmcluster));
 /* Append (owned) copies of parts in the given vector of clusters. */
 assertf(first_cluster               >= original_region->sre_clusterv &&
         first_cluster+cluster_count <= original_region->sre_clusterv+
                                        original_region->sre_clusterc
        ,"Cluster range %Id..%Id is out-of-bounds of 0..%Iu"
        ,first_cluster-original_region->sre_clusterv
        ,(first_cluster-original_region->sre_clusterv)+cluster_count
        ,original_region->sre_clusterc);
 cluster_start = (first_cluster-original_region->sre_clusterv)*KSHM_CLUSTERSIZE;
 iter          =  first_cluster->sc_part;
 end           =  original_region->sre_chunk.sc_partv+
                  original_region->sre_chunk.sc_partc;
 assert(cluster_start >= iter->sp_start &&
        cluster_start <  iter->sp_start+iter->sp_pages);
 cluster_start -= iter->sp_start;
 /* Copy all frames associated with these clusters. */
 for (;;) {
  assertf(iter >= original_region->sre_chunk.sc_partv && iter < end
         ,"Part %Id is out-of-bounds of 0..%Iu (%Iu pages remaining)"
         ,iter-original_region->sre_chunk.sc_partv
         ,original_region->sre_chunk.sc_partc,cluster_pages);
  max_pages = min(cluster_pages,iter->sp_pages-cluster_start);
#ifdef __DEBUG__
  assert(iter->sp_frame != NULL);
#endif
  /* todo: Try to undo scattering of pageframes.
   *     - While unlikely to succeed, we should try to merge multiple parts here... */
  framecopy = kpageframe_alloc(max_pages);
  if __unlikely(framecopy == PAGEFRAME_NIL) return KE_NOMEM;
  kpageframe_memcpy(framecopy,iter->sp_frame+cluster_start,max_pages);
  assert(memcmp(framecopy,iter->sp_frame+cluster_start,max_pages*PAGESIZE) == 0);
  error = kshmchunk_builder_addpart(self,framecopy,max_pages,1);
  if __unlikely(KE_ISERR(error)) {
   kpageframe_free(framecopy,max_pages);
   return error;
  }
  if ((cluster_pages -= max_pages) == 0) break;
  cluster_start = 0;
  /* The last cluster may not be fully used. - Handled by checking for part end. */
  if (++iter == end) break;
 }
 return KE_OK;
}

static kerrno_t
kshmchunk_builder_inherit_clusters(struct kshmchunk *self,
                                   struct kshmregion const *original_region,
                                   struct kshmcluster const *first_cluster,
                                   size_t cluster_count) {
 struct kshmpart *iter,*end;
 kshmregion_page_t cluster_start;
 kshmregion_page_t cluster_pages = cluster_count*KSHM_CLUSTERSIZE;
 size_t max_pages; kerrno_t error;
 kassertobj(self);
 kassert_kshmregion(original_region);
 assert(cluster_count);
 kassertmem(first_cluster,cluster_count*sizeof(struct kshmcluster));
 /* Append the parts of all given clusters. */
 assertf(first_cluster               >= original_region->sre_clusterv &&
         first_cluster+cluster_count <= original_region->sre_clusterv+
                                        original_region->sre_clusterc
        ,"Cluster range %Id..%Id is out-of-bounds of 0..%Iu"
        ,first_cluster-original_region->sre_clusterv
        ,(first_cluster-original_region->sre_clusterv)+cluster_count
        ,original_region->sre_clusterc);
 cluster_start = (first_cluster-original_region->sre_clusterv)*KSHM_CLUSTERSIZE;
 iter          =  first_cluster->sc_part;
 end           =  original_region->sre_chunk.sc_partv+
                  original_region->sre_chunk.sc_partc;
 assert(cluster_start >= iter->sp_start &&
        cluster_start <  iter->sp_start+iter->sp_pages);
 cluster_start -= iter->sp_start;
 /* Inherit all frames associated with these clusters. */
 for (;;) {
  assertf(iter >= original_region->sre_chunk.sc_partv && iter < end
         ,"Part %Id is out-of-bounds of 0..%Iu (%Iu pages remaining)"
         ,iter-original_region->sre_chunk.sc_partv
         ,original_region->sre_chunk.sc_partc,cluster_pages);
  max_pages = min(cluster_pages,iter->sp_pages-cluster_start);
#ifdef __DEBUG__
  assert(iter->sp_frame != NULL);
#endif
  error = kshmchunk_builder_addpart(self,iter->sp_frame+cluster_start,max_pages,0);
  if __unlikely(KE_ISERR(error)) return error;
  if ((cluster_pages -= max_pages) == 0) break;
  cluster_start = 0;
  /* The last cluster may not be fully used. - Handled by checking for part end. */
  if (++iter == end) break;
 }
 return KE_OK;
}





__crit __ref struct kshmregion *
kshmregion_extractpart(struct kshmregion *__restrict self,
                       struct kshmcluster *cls_min,
                       struct kshmcluster *cls_max) {
 __ref struct kshmregion *result;
 struct kshmcluster *dest,*iter,*cls_end;
 struct kshmcluster *first_inheritable_cluster;
 struct kshmcluster *first_copyable_cluster;
 size_t cluster_count,partial_count;
 kerrno_t error;
 kassert_kshmregion(self);
 assert(self->sre_clusterc);
 assert(self->sre_chunk.sc_partc);
 assert(self->sre_chunk.sc_pages);
 assert(cls_min >= self->sre_clusterv && cls_min < self->sre_clusterv+self->sre_clusterc);
 assert(cls_max >= self->sre_clusterv && cls_max < self->sre_clusterv+self->sre_clusterc);
 assert(cls_max >= cls_min);
 cluster_count = (size_t)((cls_max-cls_min)+1);
 assertf(cluster_count <= self->sre_clustera,
         "Not enough clusters: %Iu < %Iu",
         cluster_count,self->sre_clustera);
 assert(cluster_count <= self->sre_clusterc);
 result = (__ref struct kshmregion *)malloc(SIZEOF_KSHMREGION(cluster_count));
 if __unlikely(!result) return NULL;
 kobject_init(result,KOBJECT_MAGIC_SHMREGION);
 result->sre_origin   = self->sre_origin ? self->sre_origin : self;
 result->sre_branches = 1;
 result->sre_clustera = cluster_count;
 result->sre_clusterc = cluster_count;
 /* NOTE: We don't rely on the fact that clusters may have a reference
  *       counter of one, in which case they can be extracted instead
  *       of having to be copied the hard way.
  *       Instead, we consider that situation as the optimized path,
  *       with a reference counter '> 1' being the default, which
  *       then can be handled through regular hard-copy.
  */
 /* NOTE: This function does a few hacky things:
  *        - It uses the chunk-builder, meaning that 'sre_chunk.sc_pages'
  *          is re-interpreted as a allocated-vector-size member,
  *          as well as 'sre_chunk.sc_partv[*].sp_start' being used
  *          to track which parts were inherited from the original region.
  */
 dest = result->sre_clusterv;
 first_inheritable_cluster = NULL;
 first_copyable_cluster    = NULL;
 kshmchunk_builder_init(&result->sre_chunk);
 cls_end = cls_max+1;
 for (iter = cls_min;;) {
  assert(iter->sc_refcnt != 0);
  if (iter->sc_refcnt == 1) {
   if (first_copyable_cluster) {
flush_copyable_clusters:
    partial_count = (size_t)(iter-first_copyable_cluster);
    /* Add the parts of all copyable clusters. */
    error = kshmchunk_builder_copy_clusters(&result->sre_chunk,self,
                                            first_copyable_cluster,
                                            partial_count);
    if __unlikely(KE_ISERR(error)) goto err_r;
    first_copyable_cluster = NULL;
    dest += partial_count;
    assert(dest <= result->sre_clusterv+result->sre_clusterc);
    if (iter == cls_end) break;
   }
   if (!first_inheritable_cluster) {
    /* Optimization: Inherit this cluster instead of copying it. */
    first_inheritable_cluster = iter;
   }
  } else {
   if (first_inheritable_cluster) {
flush_inheritable_clusters:
    partial_count = (size_t)(iter-first_inheritable_cluster);
    /* Add the parts of all inheritable clusters. */
    error = kshmchunk_builder_inherit_clusters(&result->sre_chunk,self,
                                               first_inheritable_cluster,
                                               partial_count);
    if __unlikely(KE_ISERR(error)) goto err_r;
    first_inheritable_cluster = NULL;
    dest += partial_count;
    assert(dest <= result->sre_clusterv+result->sre_clusterc);
    if (iter == cls_end) break;
   }
   if (!first_copyable_cluster) {
    /* Default: This cluster must be copied. */
    first_copyable_cluster = iter;
   }
  }
  assert((first_inheritable_cluster != NULL) !=
         (first_copyable_cluster != NULL));
  if (iter++ == cls_max) {
   if (first_inheritable_cluster) goto flush_inheritable_clusters;
   else {
    assert(first_copyable_cluster);
    goto flush_copyable_clusters;
   }
  }
 }
 assert(!first_copyable_cluster);
 assert(!first_inheritable_cluster);
 /* Must go through all chunks we've (potentially)
  * extracted again and remove one reference from each.
  * This must be done once we've reached the point of noexcept
  * because decref'ing reference counter greater than ONE(1)
  * could have the potential of another process doing the same,
  * leaving the chunk to actually be deallocated before we are
  * able to recover by incrementing the counter again on error.
  * >> Some counters actually dropping to ZERO(0) during this
  *    process is also intended, as those are the clusters we've
  *    managed to inherit (which is why we don't free them here).
  * NOTE: We can rely on the fact that all clusters that only
  *       had ONE(1) reference (aka. those we've inherited) will
  *       still only have one reference, simply because that's
  *       what this reference counter is meant to represent:
  *       How many users of the cluster there are.
  *    >> Since we were the only user, and we _most_definitely_
  *       didn't just grant someone else access to that cluster,
  *       there should be no (legal) way the counter could
  *       have increased in the mean time.
  */
 for (iter = cls_min;;) {
  kshmrefcnt_t refcnt = katomic_decfetch(iter->sc_refcnt);
  assert(refcnt != (kshmrefcnt_t)-1);
  if (!refcnt) {
   /* Some other process may have dropped his reference
    * to some cluster, reducing it to ONE(1) after we've
    * decided not to inherit it.
    * >> We must go through our list of parts to free
    *    any zero-reference clusters that we didn't inherit.
    */
   if __unlikely(!kshmchunk_builder_inheritscluster(&result->sre_chunk,iter)) {
    kshmregion_freecluster(self,iter);
   }
#ifdef __DEBUG__
   size_t cnt = katomic_decfetch(self->sre_clustera);
   assert(cnt != (size_t)-1);
   if (!cnt) {
    /* Last cluster has been decref'ed. */
    assert(iter == cls_max);
    kshmregion_assert_reallynoclusters(self);
   }
#else
   asserte(katomic_fetchdec(self->sre_clustera) != 0);
#endif
  }
  if (iter == cls_max) break;
  ++iter;
 }
 /* Try to clean up vector memory not used by the builder. */
 kshmchunk_builder_finish(&result->sre_chunk);
 result->sre_chunk.sc_flags = self->sre_chunk.sc_flags;
 /* Generate the cluster-start cache. */
 kshmregion_setup_clusters(result);
 
 /* Do some sanity checks on the newly created region. */
 assert(result->sre_branches == 1);
 assert(result->sre_clusterc == cluster_count);
 assert(result->sre_clusterc == cluster_count);
 assert(result->sre_chunk.sc_pages <= cluster_count*KSHM_CLUSTERSIZE);
#ifdef __DEBUG__
 cls_end = (iter = result->sre_clusterv)+cluster_count;
 do assert(iter->sc_refcnt == 1);
 while (++iter != cls_end);
#endif

 return result;
err_r:
 kshmchunk_builder_quit(&result->sre_chunk);
 free(result);
 return NULL;
}


__kernel struct kpageframe *
kshmregion_getphyspage(struct kshmregion *__restrict self,
                       kshmregion_page_t page_index,
                       size_t *__restrict max_pages) {
 struct kshmpart *part;
 kassert_kshmregion(self);
 kassertobj(max_pages);
 assertf(page_index < self->sre_chunk.sc_pages,
         "The given page index is out-of-bounds (%Iu >= %Iu)",
         page_index,self->sre_chunk.sc_pages);
 part = kshmregion_getcluster(self,page_index)->sc_part;
 assertf(part >= self->sre_chunk.sc_partv &&
         part <  self->sre_chunk.sc_partv+
                 self->sre_chunk.sc_partc
        ,"Found part %Id is out-of-bounds of 0..%Iu"
        ,self->sre_chunk.sc_partv-part
        ,self->sre_chunk.sc_partc);
 assertf(page_index >= part->sp_start,
         "%Iu < %Iu",page_index,part->sp_start);
 page_index -= part->sp_start;
 while (page_index >= part->sp_pages) {
  page_index -= part->sp_pages;
  ++part;
  assert(part != self->sre_chunk.sc_partv+
                 self->sre_chunk.sc_partc);
 }
 *max_pages = part->sp_pages-page_index;
 return part->sp_frame+page_index;
}

__kernel void *
kshmregion_translate_fast(struct kshmregion *__restrict self,
                          kshmregion_addr_t address_offset,
                          size_t *__restrict max_bytes) {
 struct kpageframe *frame;
 kshmregion_addr_t page_address;
 size_t max_pages;
 kassert_kshmregion(self);
 assertf(address_offset < self->sre_chunk.sc_pages*PAGESIZE,
         "The given address offset is out-of-bounds (%Iu >= %Iu)",
         address_offset,self->sre_chunk.sc_pages*PAGESIZE);
 page_address = alignd(address_offset,PAGEALIGN);
 frame = kshmregion_getphyspage(self,page_address/PAGESIZE,&max_pages);
 assert(frame);
 address_offset -= page_address;
 assert(max_pages);
 assert(address_offset < PAGESIZE);
 *max_bytes = (max_pages*PAGESIZE)-address_offset;
 assert(*max_bytes);
 return (void *)((uintptr_t)frame+address_offset);
}

__kernel void *
kshmregion_translate(struct kshmregion *__restrict self,
                     kshmregion_addr_t address_offset,
                     size_t *__restrict max_bytes) {
 kassert_kshmregion(self);
 if __unlikely(address_offset >= self->sre_chunk.sc_pages*PAGESIZE) { *max_bytes = 0; return NULL; }
 return kshmregion_translate_fast(self,address_offset,max_bytes);
}

__crit void
kshmregion_memset(__ref struct kshmregion *__restrict self,
                  int byte) {
 __kernel void *addr;
 size_t maxbytes;
 kshmregion_addr_t offset = 0;
 while ((addr = kshmregion_translate(self,offset,&maxbytes)) != NULL) {
  memset(addr,byte,maxbytes);
  offset += maxbytes;
 }
}





//////////////////////////////////////////////////////////////////////////
// ===== kshmbranch/kshmmap


__crit __nomp kerrno_t
kshmbranch_remap(struct kshmbranch const *__restrict self,
                 struct kpagedir *__restrict pd) {
 kerrno_t error; struct kshmregion *region;
 size_t part_offset,remaning_pages,map_pages;
 kshm_flag_t region_flags; kpageflag_t page_flags;
 __pagealigned __user void *virtual_address;
 kassertobj(self);
 region = self->sb_region;
 kassert_kshmregion(region);
 part_offset = self->sb_rstart % KSHM_CLUSTERSIZE;
 remaning_pages = self->sb_rpages;
 virtual_address = self->sb_map;
 region_flags = region->sre_chunk.sc_flags;
 if (KSHMREGION_FLAG_USECOW(region_flags)) {
  struct kshmcluster *iter,*clusters_start;
  struct kpageframe *phys_addr;
  size_t map_pages;
  kpageflag_t used_page_flags;
  if (region_flags&KSHMREGION_FLAG_READ) {
   page_flags = PAGEDIR_FLAG_USER|PAGEDIR_FLAG_READ_WRITE;
  } else {
   /* Don't map access from user-space. */
   page_flags = PAGEDIR_FLAG_READ_WRITE;
   goto map_normal;
  }
  /* Special case: Must look at the reference counters
   *               of all the different clusters to
   *               determine if they should be mapped
   *               read-only for COW, or read-write. */
  iter           = self->sb_cluster_min;
  clusters_start = region->sre_clusterv;
  for (;;) {
   assert(iter != clusters_start+region->sre_clusterc);
   assert(part_offset < KSHM_CLUSTERSIZE);
   phys_addr = kshmregion_getphyspage(region,
                                    ((iter-clusters_start)*KSHM_CLUSTERSIZE)+part_offset,
                                      &map_pages);
   assert(phys_addr);
   map_pages = min(map_pages,remaning_pages);
   map_pages = min(map_pages,KSHM_CLUSTERSIZE-part_offset);
   /* Determine the exact set of flags we're going to use. */
   used_page_flags = page_flags;
   assert(katomic_load(region->sre_branches) >=
          katomic_load(iter->sc_refcnt));
   if (iter->sc_refcnt > 1) {
    /* Map the cluster as read-only (COW-style) if its
     * reference counter indicates more than one user. */
    used_page_flags &= ~(PAGEDIR_FLAG_READ_WRITE);
   }
   error = kpagedir_remap(pd,phys_addr,virtual_address,
                          map_pages,used_page_flags);
   if __unlikely(KE_ISERR(error)) goto unmap_partial_and_return_error;
   if __likely((remaning_pages -= map_pages) == 0) break;
   *(uintptr_t *)&virtual_address += (map_pages*PAGESIZE);
   part_offset += map_pages;
   if (part_offset >= KSHM_CLUSTERSIZE) {
    part_offset -= KSHM_CLUSTERSIZE;
    assert(part_offset < KSHM_CLUSTERSIZE);
    ++iter;
   }
  }
 } else {
  struct kshmpart *part;
  page_flags = 0;
  if (region_flags&KSHMREGION_FLAG_WRITE) page_flags |= PAGEDIR_FLAG_READ_WRITE;
  if (region_flags&KSHMREGION_FLAG_READ)  page_flags |= PAGEDIR_FLAG_USER;
  else page_flags |= PAGEDIR_FLAG_READ_WRITE;
map_normal:
  part = self->sb_cluster_min->sc_part;
  /* Find the first mapped part. */
  while (part_offset >= part->sp_pages) {
   part_offset -= part->sp_pages;
   ++part;
   assert(part != region->sre_chunk.sc_partv+
                  region->sre_chunk.sc_partc);
  }
  for (;;) {
   map_pages = min(remaning_pages,part->sp_pages);
   /* Remap the physical part of this region. */
   assert(part->sp_frame);
   error = kpagedir_remap(pd,part->sp_frame+part_offset,
                          virtual_address,map_pages,
                          page_flags);
   if __unlikely(KE_ISERR(error)) {
unmap_partial_and_return_error:
    /* Unmap everything we've managed to map thus far. */
    kpagedir_unmap(pd,self->sb_map,
                  ((uintptr_t)virtual_address-
                   (uintptr_t)self->sb_map)/PAGESIZE);
    return error;
   }
   if ((remaning_pages -= map_pages) == 0) break;
   *(uintptr_t *)&virtual_address += (map_pages*PAGESIZE);
   ++part;
   part_offset = 0;
  }
 }
 return KE_OK;
}


__crit __nomp kerrno_t
kshmbranch_unmap_portion(struct kshmbranch **__restrict pself,
                         struct kpagedir *__restrict pd,
                         kshmregion_page_t first_page,
                         uintptr_t page_count,
                         uintptr_t addr_semi) {
 kerrno_t error;
 struct kshmbranch *branch;
 struct kshmcluster *new_cluster;
 struct kshmregion *region;
 kassertobj(pself);
 branch = *pself;
 kassertobj(branch);
 region = branch->sb_region;
 kassert_kshmregion(region);
 assertf(first_page != branch->sb_rstart ||
         page_count != branch->sb_rpages,
         "The full-unmap case isn't handled by this function");
 assert(first_page >= branch->sb_rstart);
 assertf(first_page+page_count > first_page,"page_count = %Iu",page_count);
 assert(first_page+page_count <= branch->sb_rstart+branch->sb_rpages);
 assert(branch->sb_rstart+branch->sb_rpages > branch->sb_rstart);
 assert(branch->sb_rstart+branch->sb_rpages <= region->sre_chunk.sc_pages);
 if (first_page == branch->sb_rstart) {
trim_front:
  /* Trim at the front. */
  new_cluster = kshmregion_getcluster(region,first_page+page_count);
  assert(new_cluster >= branch->sb_cluster_min);
  if (branch->sb_cluster_min != new_cluster) {
   /* Must drop reference to all clusters between
    * 'branch->sb_cluster_min' and 'new_cluster-1' */
   kshmregion_decref_clusters(region,branch->sb_cluster_min,new_cluster-1);
   branch->sb_cluster_min = new_cluster;
  }
  branch->sb_map_min += page_count*PAGESIZE;
  branch->sb_rstart   = first_page+page_count;
  branch->sb_rpages  -= page_count;
  assert(branch->sb_rpages);
  return KE_OK;
 }
 if (first_page+page_count == branch->sb_rstart+branch->sb_rpages) {
  /* Trim at the back. */
  assertf(first_page+page_count <= region->sre_chunk.sc_pages,
          "%Iu >= %Iu",first_page,region->sre_chunk.sc_pages);
  new_cluster = kshmregion_getcluster(region,first_page);
  assertf(new_cluster >= branch->sb_cluster_min &&
          new_cluster <= branch->sb_cluster_max,
          "Cluster %Id is out of bounds of %Id..%Id",
          new_cluster-region->sre_clusterv,
          branch->sb_cluster_min-region->sre_clusterv,
          branch->sb_cluster_max-region->sre_clusterv);
  if (branch->sb_cluster_max != new_cluster) {
   /* Must drop reference to all clusters between
    * 'new_cluster+1' and 'branch->sb_cluster_max' */
   kshmregion_decref_clusters(region,new_cluster+1,branch->sb_cluster_max);
   branch->sb_cluster_max = new_cluster;
  }
  branch->sb_map_max -= page_count*PAGESIZE;
  branch->sb_rpages  -= page_count;
  assert(branch->sb_rpages);
  return KE_OK;
 }
 /* Must split the region, then trim at the front. */
 error = kshmbrach_putsplit(pself,addr_semi,NULL,NULL,
                           (struct kshmbranch ***)&pself,NULL,
                            first_page);
 if __unlikely(KE_ISERR(error)) return error;
 kassertobj(pself);
 branch = *pself;
 kassertobj(branch);
 assert(branch->sb_region == region);
 /* Now we just need to trim the new branch at the front. */
 assert(first_page == branch->sb_rstart);
 goto trim_front;
}

__crit __nomp size_t
kshmbranch_unmap(struct kshmbranch **__restrict proot,
                 struct kpagedir *__restrict pd,
                 __pagealigned uintptr_t addr_min,
                               uintptr_t addr_max,
                 uintptr_t addr_semi, unsigned int addr_level,
                 kshmunmap_flag_t flags, struct kshmregion *origin) {
 struct kshmbranch *root;
 struct kshmregion *region;
 size_t result = 0;
 uintptr_t new_semi;
 unsigned int new_level;
 kassertobj(proot);
search_again:
 root = *proot;
 kassertobj(root);
 kassert_kshmregion(root->sb_region);
 assert(isaligned(addr_min,PAGEALIGN));
 assert(addr_min < addr_max);
 if (addr_min <= root->sb_map_max &&
     addr_max >= root->sb_map_min) {
  region = root->sb_region;
  /* Found a matching entry. */
  /* Make sure that we're actually allowed to unmap this one... */
  if (((region->sre_chunk.sc_flags&KSHMREGION_FLAG_RESTRICTED) &&
      !(flags&KSHMUNMAP_FLAG_RESTRICTED)) ||
       (origin && region != origin &&
        region->sre_origin != origin)) goto continue_search;
  /* Let's-a-go! */

  /* First off, we should figure out if supposed to unmap this
   * entire branch, instead of simply unmapping apart of it.
   * >> Because if we should remove it completely, we must
   *    continue our special kind-of in a special way. */
  if (addr_min <= root->sb_map_min &&
      addr_max >= root->sb_map_max) {
   /* Remove this entire branch! */
   asserte(kshmbranch_popone(proot,addr_semi,addr_level) == root);
   kpagedir_unmap(pd,root->sb_map,root->sb_rpages);
   result += root->sb_rpages;
   /* Drop a references from all used clusters. */
   kshmregion_decref(region,root->sb_cluster_min,root->sb_cluster_max);
   /* Free the branch. */
   free(root);
   /* continue searching inline, or stop if this was the last branch. */
   if (!*proot) goto done;
   goto search_again;
  } else {
   /* Only remove a part of this branch. */
   kshmregion_page_t start_page;
   size_t unmap_pages;
   uintptr_t unmap_min;
   if (addr_min > root->sb_map_min) {
    unmap_min  = addr_min;
    start_page = (addr_min-root->sb_map_min)/PAGESIZE;
   } else {
    unmap_min  = root->sb_map_min;
    start_page = 0;
   }
   start_page += root->sb_rstart;
   unmap_pages = ceildiv((min(addr_max,root->sb_map_max)+1)-unmap_min,PAGESIZE);
   assert(unmap_pages);
   assertf(start_page+unmap_pages > start_page,"unmap_pages = %Iu",unmap_pages);
   assert(start_page >= root->sb_rstart);
   assertf(root->sb_rpages == ceildiv((root->sb_map_max-root->sb_map_min)+1,PAGESIZE),
           "%Iu != %Iu",root->sb_rpages,ceildiv((root->sb_map_max-root->sb_map_min)+1,PAGESIZE));
   assertf(start_page+unmap_pages <= root->sb_rstart+root->sb_rpages,
           "addr_min                        = %p\n"
           "addr_max                        = %p\n"
           "root->sb_map_min                = %p\n"
           "root->sb_map_max                = %p\n"
           "start_page                      = %Iu\n"
           "unmap_pages                     = %Iu\n"
           "root->sb_rstart                 = %Iu\n"
           "root->sb_rpages                 = %Iu\n"
           "root->sb_rstart+root->sb_rpages = %Iu\n",
           addr_min,addr_max,
           root->sb_map_min,root->sb_map_max,
           start_page,unmap_pages,
           root->sb_rstart,root->sb_rpages,
           root->sb_rstart+root->sb_rpages);
   assert(start_page+unmap_pages <= root->sb_region->sre_chunk.sc_pages);
   k_syslogf(KLOG_DEBUG,"[SHM] Unmap portion %Iu..%Iu (%Iu pages) of region at %p\n",
             start_page,start_page+unmap_pages,unmap_pages,root->sb_region);
   if __likely(KE_ISOK(kshmbranch_unmap_portion(proot,pd,start_page,unmap_pages,addr_semi))) {
    result += unmap_pages;
   } else {
    /* This is kind-of bad, but hopefully no one will notice...
     * todo: maybe add a syslog for this? */
    k_syslogf(KLOG_ERROR,"[SHM] Failed to unmap portion %Iu..%Iu (%Iu pages) of region at %p\n",
              start_page,start_page+unmap_pages,unmap_pages,root->sb_region);
   }
  }
 }
continue_search:
 /* Continue searching. */
 /* todo: If recursion isn't required ('root' only has one branch),
  *       continue searching inline without calling our own function again. */
 if (addr_min < addr_semi && root->sb_min) {
  /* Recursively continue searching left. */
  new_semi = addr_semi,new_level = addr_level;
  KSHMBRANCH_WALKMIN(new_semi,new_level);
  result += kshmbranch_unmap(&root->sb_min,pd,addr_min,addr_max,
                             new_semi,new_level,flags,origin);
 }
 if (addr_max >= addr_semi && root->sb_max) {
  /* Recursively continue searching right. */
  new_semi = addr_semi,new_level = addr_level;
  KSHMBRANCH_WALKMAX(new_semi,new_level);
  result += kshmbranch_unmap(&root->sb_max,pd,addr_min,addr_max,
                             new_semi,new_level,flags,origin);
 }
done:
 return result;
}

__crit __nomp kerrno_t
kshmbrach_putsplit(struct kshmbranch **__restrict pself, uintptr_t addr_semi,
                   struct kshmbranch ***pmin, uintptr_t *pmin_semi,
                   struct kshmbranch ***pmax, uintptr_t *pmax_semi,
                   kshmregion_page_t page_offset) {
 struct kshmbranch *min_branch,*max_branch;
 struct kshmregion *region;
 __user uintptr_t split_address;
 struct kshmbranch **branch_address;
 uintptr_t new_semi; unsigned int new_level;
 kassertobj(pself);
 kassertobjnull(pmin);
 kassertobjnull(pmax);
 kassertobjnull(pmin_semi);
 kassertobjnull(pmax_semi);
 min_branch = *pself;
 kassertobj(min_branch);
 region = min_branch->sb_region;
 kassert_kshmregion(region);
 assert(min_branch->sb_map_min <= min_branch->sb_map_max);
 assertf(page_offset >= min_branch->sb_rstart &&
         page_offset < min_branch->sb_rstart+min_branch->sb_rpages
        ,"The given index %Iu is out-of-bounds of the branch %Iu..%Iu"
        ,page_offset,min_branch->sb_rstart
        ,min_branch->sb_rstart+min_branch->sb_rpages);
 assertf(page_offset != min_branch->sb_rstart
        ,"Can't split branch at start %Iu (this would create an empty leaf)"
        ,min_branch->sb_rstart);
 assertf(page_offset != (min_branch->sb_rstart+min_branch->sb_rpages)
        ,"Can't split branch at index %Iu (this would create an empty leaf at the back)"
        ,page_offset);
 assertf(page_offset < region->sre_chunk.sc_pages
        ,"The given page_offset %Iu is ouf-of-bounds of the region 0..%Iu"
        ,page_offset,region->sre_chunk.sc_pages);
 /* We can only hope that 'addr_semi' is correct... */
 max_branch = omalloc(struct kshmbranch);
 if __unlikely(!max_branch) return KE_NOMEM;
 max_branch->sb_region = region;
 /* By default, we create the new branch as the high-address portion. */
 split_address              = (min_branch->sb_map_min+(page_offset-min_branch->sb_rstart)*PAGESIZE);
 assert(split_address < min_branch->sb_map_max);
 max_branch->sb_rstart      = page_offset;
 max_branch->sb_rpages      = min_branch->sb_rpages-(page_offset-min_branch->sb_rstart);
 assert(max_branch->sb_rstart > min_branch->sb_rstart);
 min_branch->sb_rpages      = max_branch->sb_rstart-min_branch->sb_rstart;
 max_branch->sb_cluster_min = kshmregion_getcluster(region,max_branch->sb_rstart);
 max_branch->sb_cluster_max = min_branch->sb_cluster_max;
 assert(max_branch->sb_rpages);
 assert(min_branch->sb_rpages);
 min_branch->sb_cluster_max = kshmregion_getcluster(region,min_branch->sb_rstart+(min_branch->sb_rpages-1));
 max_branch->sb_map_max = min_branch->sb_map_max;
 max_branch->sb_map_min = split_address;
 min_branch->sb_map_max = split_address-1;
 /* Incref the branch counter for the region to mirror the new region part. */
 asserte(katomic_fetchinc(region->sre_branches) != 0);
 /* Special case: We've inherited all upper clusters from the min-branch, but
  *               the the min cluster of the max-branch may be equal to the
  *               new max-cluster if the min-branch if the page-offset isn't
  *               cluster-aligned, meaning we'll have to incref it. */
 if (min_branch->sb_cluster_max == max_branch->sb_cluster_min) {
  /* Acquire a min-cluster reference for the max-branch. */
  asserte(katomic_fetchinc(max_branch->sb_cluster_min->sc_refcnt) >= 1);
 }

#define ASSERT_CLUSTER(p) \
 assertf((p) >= region->sre_clusterv && \
         (p) <  region->sre_clusterv+region->sre_clusterc\
        ,"Cluster index %Id is out-of-bounds of 0..%Iu"\
        ,(p)-region->sre_clusterv,region->sre_clusterc)
 ASSERT_CLUSTER(min_branch->sb_cluster_min);
 ASSERT_CLUSTER(min_branch->sb_cluster_max);
 ASSERT_CLUSTER(max_branch->sb_cluster_min);
 ASSERT_CLUSTER(max_branch->sb_cluster_max);
#undef ASSERT_CLUSTER
 assert(min_branch->sb_cluster_min <= min_branch->sb_cluster_max);
 assert(max_branch->sb_cluster_min <= max_branch->sb_cluster_max);
 assert(min_branch->sb_cluster_max <= max_branch->sb_cluster_min);

 /* Insert the new branch into the binary tree. */
 /* Now the only question remains if 'max_branch' must override '*pself' */
 asserte(kshmbranch_insert(pself,max_branch,addr_semi,KSHMBRANCH_SEMILEVEL(addr_semi)) == KE_OK);
#if defined(__DEBUG__) && 0
 new_semi = addr_semi,new_level = KSHMBRANCH_SEMILEVEL(addr_semi);
 assert(min_branch == *kshmbranch_plocateat(pself,min_branch->sb_map_min,&new_semi,&new_level));
 new_semi = addr_semi,new_level = KSHMBRANCH_SEMILEVEL(addr_semi);
 assert(min_branch == *kshmbranch_plocateat(pself,min_branch->sb_map_max,&new_semi,&new_level));
 new_semi = addr_semi,new_level = KSHMBRANCH_SEMILEVEL(addr_semi);
 assert(max_branch == *kshmbranch_plocateat(pself,max_branch->sb_map_min,&new_semi,&new_level));
 new_semi = addr_semi,new_level = KSHMBRANCH_SEMILEVEL(addr_semi);
 assert(max_branch == *kshmbranch_plocateat(pself,max_branch->sb_map_max,&new_semi,&new_level));
#endif

 if (pmin || pmin_semi) {
  new_semi = addr_semi,new_level = KSHMBRANCH_SEMILEVEL(addr_semi);
  branch_address = kshmbranch_plocateat(pself,min_branch->sb_map_max,&new_semi,&new_level);
  assert(branch_address);
  if (pmin) *pmin = branch_address;
  if (pmin_semi) *pmin_semi = new_semi;
 }
 if (pmax || pmax_semi) {
  new_semi = addr_semi,new_level = KSHMBRANCH_SEMILEVEL(addr_semi);
  branch_address = kshmbranch_plocateat(pself,max_branch->sb_map_min,&new_semi,&new_level);
  assert(branch_address);
  if (pmax) *pmax = branch_address;
  if (pmax_semi) *pmax_semi = new_semi;
 }
 return KE_OK;
}

__crit __nomp kerrno_t
kshmbrach_merge(struct kshmbranch **__restrict pmin, uintptr_t min_semi,
                struct kshmbranch **__restrict pmax, uintptr_t max_semi,
                struct kshmbranch ***presult, uintptr_t *presult_semi) {
 struct kshmbranch *min_branch,*max_branch;
 kassertobj(pmin);
 kassertobj(pmax);
 kassertobjnull(presult);
 kassertobjnull(presult_semi);
 min_branch = *pmin;
 max_branch = *pmax;
 kassertobj(min_branch);
 kassertobj(max_branch);
 kassert_kshmregion(min_branch->sb_region);
 kassert_kshmregion(max_branch->sb_region);
 assert(min_branch->sb_map_min >= KSHMBRANCH_MAPMIN(min_semi,KSHMBRANCH_SEMILEVEL(min_semi)));
 assert(min_branch->sb_map_max <= KSHMBRANCH_MAPMAX(min_semi,KSHMBRANCH_SEMILEVEL(min_semi)));
 assert(max_branch->sb_map_min >= KSHMBRANCH_MAPMIN(max_semi,KSHMBRANCH_SEMILEVEL(max_semi)));
 assert(max_branch->sb_map_max <= KSHMBRANCH_MAPMAX(max_semi,KSHMBRANCH_SEMILEVEL(max_semi)));

 /* Make sure that both branches reference unique regions. */
 assert(min_branch->sb_region->sre_branches == 1);
 assert(max_branch->sb_region->sre_branches == 1);
 assert(kshmbranch_canexpand_max(min_branch));
 assert(kshmbranch_canexpand_min(max_branch));
 /* Make sure that both branches lie directly next to each other. */
 assert(min_branch->sb_map_max+1 == max_branch->sb_map_min);

 /* NOTE: this function is only used for optimizations and disabling
  *       this doesn't cause the system to become unstable. */
#if 1
 {
  struct kshmbranch **proot;
  struct kshmregion *merged_region;
  uintptr_t root_semi;
  unsigned int min_level,max_level,root_level;
  k_syslogf(COW_LOGLEVEL,"[SHM] Merge branch at %p..%p with %p..%p\n",
            min_branch->sb_map_min,min_branch->sb_map_max,
            max_branch->sb_map_min,max_branch->sb_map_max);
  /* kshmbranch_print(min_branch,min_semi,KSHMBRANCH_SEMILEVEL(min_semi)); */
  /* kshmbranch_print(max_branch,max_semi,KSHMBRANCH_SEMILEVEL(max_semi)); */

  /* Create the merged region.
   * This might seem an odd choice for the first step, but
   * in fact by re-using one of the branches, this is the
   * only this in this entire function that isn't noexcept. */
  merged_region = kshmregion_merge(min_branch->sb_region,
                                   max_branch->sb_region);
  if __unlikely(!merged_region) return KE_NOMEM;
  min_level = KSHMBRANCH_SEMILEVEL(min_semi);
  max_level = KSHMBRANCH_SEMILEVEL(max_semi);
  assertf(min_level != max_level
         ,"The min and max address-levels %u cannot be equal, as that would "
          "imply some other branch lying in between. Instead, one must be "
          "greater than the other to create a dependency ordering."
         ,min_level);
  /* The branch with the greater level lies further up the tree,
   * meaning that it will be the root branch to insert the merged leaf into.
   * NOTE: The remove order is important to not invalidate our root pointers. */
  if (min_level > max_level) {
   /* Remove the two branches from the tree. */
   asserte(kshmbranch_popone(pmax,max_semi,max_level) == max_branch);
   asserte(kshmbranch_popone(pmin,min_semi,min_level) == min_branch);
   proot      = pmin;
   root_semi  = min_semi;
   root_level = min_level;
  } else {
   /* Remove the two branches from the tree. */
   asserte(kshmbranch_popone(pmin,min_semi,min_level) == min_branch);
   asserte(kshmbranch_popone(pmax,max_semi,max_level) == max_branch);
   proot      = pmax;
   root_semi  = max_semi;
   root_level = max_level;
  }
  /* At this point we've inherited both the min and max branches.
   * >> We'll reuse one (min) and free the other (max). */
#define merged_branch  min_branch
  merged_branch->sb_region      = merged_region;
  merged_branch->sb_map_max     = max_branch->sb_map_max;
  merged_branch->sb_rpages     += max_branch->sb_rpages;
  merged_branch->sb_cluster_min = kshmregion_getcluster(merged_region,merged_branch->sb_rstart);
  merged_branch->sb_cluster_max = kshmregion_getcluster(merged_region,merged_branch->sb_rstart+(merged_branch->sb_rpages-1));
  /* Free the other region. */
  free(max_branch);
  asserte(KE_ISOK(kshmbranch_insert(proot,merged_branch,
                                    root_semi,root_level)));
  /* Re-locate the now inserted branch. */
  proot = kshmbranch_plocateat(proot,(uintptr_t)merged_branch->sb_map,
                               &root_semi,&root_level);
  assert(proot != NULL);
  if (presult)      *presult      = proot;
  if (presult_semi) *presult_semi = root_semi;
#undef merged_branch
  return KE_OK;
 }
#else
 return KE_NOSYS;
#endif
}

void kshmbranch_deltree(struct kshmbranch *__restrict start) {
 struct kshmbranch *min_branch,*max_branch;
again:
 kshmregion_decref(start->sb_region,
                   start->sb_cluster_min,
                   start->sb_cluster_max);
 min_branch = start->sb_min;
 max_branch = start->sb_max;
 free(start);
 /* Prefer inline-recursion (don't exhaust the stack as much & is potentially faster). */
 if (min_branch) {
  if (!max_branch) { start = min_branch; goto again; }
  kshmbranch_deltree(min_branch);
free_max_branch:
  start = max_branch;
  goto again;
 } else if (max_branch) {
  goto free_max_branch;
 }
}


__crit __nomp kerrno_t
kshmbranch_fillfork(struct kshmbranch **__restrict proot,
                    struct kpagedir *__restrict self_pd,
                    struct kshmbranch *__restrict source,
                    struct kpagedir *__restrict source_pd) {
 struct kshmbranch *newbranch;
 kshm_flag_t source_flags;
 kerrno_t error = KE_OK;
again:
 kassertobj(proot);
 kassertobj(source);
 kassert_kshmregion(source->sb_region);
 source_flags = source->sb_region->sre_chunk.sc_flags;
 if (!(source_flags&KSHMREGION_FLAG_LOSEONFORK)) {
  newbranch = (struct kshmbranch *)memdup(source,sizeof(struct kshmbranch));
  if __unlikely(!newbranch) return KE_NOMEM;
  /* Add new references to all clusters covered by us. */
  error = kshmregion_incref(newbranch->sb_region,
                            newbranch->sb_cluster_min,
                            newbranch->sb_cluster_max);
  if __likely(KE_ISOK(error)) {
   size_t affected_pages;
   if (KSHMREGION_FLAG_USECOW(source_flags)) {
    /* re-map the region as read-only in 'source_pd'.
     * NOTE: Even if we fail later, it is OK to leave the mapping as
     *       read-only, as the #PF-handler will see it as a COW-page
     *       that has returned to its original owner. */
    affected_pages = kpagedir_setflags(source_pd,source->sb_map,source->sb_rpages,
                                     ~(PAGEDIR_FLAG_READ_WRITE),0);
    assertf(affected_pages == source->sb_rpages,
            "Unexpected amount of affected pages (expected %Iu, got %Iu)",
            source->sb_rpages,affected_pages);
   }
   goto ok;
  }
  /* Fallback: Try to create a hard copy of the given branch. */
  newbranch->sb_region = kshmregion_hardcopy(newbranch->sb_region,
                                             newbranch->sb_rstart,
                                             newbranch->sb_rpages);
  if __unlikely(!newbranch->sb_region) { free(newbranch); return KE_NOMEM; }
  /* Must update the clusters and offsets to mirror the new branch. */
  newbranch->sb_rstart      = 0;
  newbranch->sb_cluster_min = kshmregion_getcluster(newbranch->sb_region,0);
  newbranch->sb_cluster_max = kshmregion_getcluster(newbranch->sb_region,newbranch->sb_rpages-1);
ok:
  error = kshmbranch_remap(newbranch,self_pd);
  if __unlikely(KE_ISERR(error)) {
/*err_newbranch:*/
   kshmregion_decref(newbranch->sb_region,
                     newbranch->sb_cluster_min,
                     newbranch->sb_cluster_max);
   free(newbranch);
   return error;
  }
  asserte(kshmbranch_insert(proot,newbranch,
                            KSHMBRANCH_ADDRSEMI_INIT,
                            KSHMBRANCH_ADDRLEVEL_INIT
          ) == KE_OK);
 }
 if (source->sb_min) {
  if (source->sb_max) {
   error = kshmbranch_fillfork(proot,self_pd,source->sb_max,source_pd);
   if __unlikely(KE_ISERR(error)) return error;
  }
  source = source->sb_min;
  goto again;
 }
 if (source->sb_max) {
  source = source->sb_max;
  goto again;
 }
 return error;
}









/* We must map the kernel as read-write to actually have access to it ourselves.
 * NOTE: THIS DOES NOT AFFECT RING-#3 CODE BECAUSE 'PAGEDIR_FLAG_USER' ISN'T SET!
 * >> The reason we still need to map it as writable is because during an IRQ
 *    that happened while inside user-space, the page directory doesn't get
 *    swapped immediately, causing us to enter kernel-space while still running
 *    under the user's page directory.
 *    Obviously we switch to 'kpagedir_kernel()' asap,
 *    but before then, we do require write-access. */
#define KERNEL_MAP_FLAGS  (PAGEDIR_FLAG_READ_WRITE)

//////////////////////////////////////////////////////////////////////////
// ===== kshm
__crit kerrno_t
kshm_init(struct kshm *__restrict self,
          kseglimit_t ldt_size_hint) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassertobj(self);
 kobject_init(self,KOBJECT_MAGIC_SHM);
 krwlock_init(&self->s_lock);
 if __unlikely((self->s_pd = kpagedir_new()) == (struct kpagedir *)PAGENIL) return KE_NOMEM;
 if __unlikely(KE_ISERR(error = kpagedir_mapkernel(self->s_pd,KERNEL_MAP_FLAGS))) goto err_pd;
 if __unlikely(KE_ISERR(error = kshm_initldt(self,ldt_size_hint))) goto err_pd;
 kshmmap_init(&self->s_map);
 return KE_OK;
err_pd:
 kpagedir_delete(self->s_pd);
 return error;
}

__crit kerrno_t
kshm_close(struct kshm *__restrict self) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kshm(self);
 error = krwlock_close(&self->s_lock);
 if (error == KE_OK && self != &kproc_kernel()->p_shm) {
  kshmmap_quit(&self->s_map);
  kshm_quitldt(self);
  k_syslogf(KLOG_DEBUG,"[SHM] Deleting page directory at %p\n",self->s_pd);
  kpagedir_delete(self->s_pd);
 }
 return error;
}





__crit __nomp kerrno_t
kshm_initfork_unlocked(struct kshm *__restrict self,
                       struct kshm *__restrict right) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassertobj(self);
 kassert_kshm(right);
 kobject_init(self,KOBJECT_MAGIC_SHM);
 assert(krwlock_iswritelocked(&right->s_lock));
 krwlock_init(&self->s_lock);
 if __unlikely((self->s_pd = kpagedir_new()) == (struct kpagedir *)PAGENIL) return KE_NOMEM;
 if __unlikely(KE_ISERR(error = kpagedir_mapkernel(self->s_pd,KERNEL_MAP_FLAGS))) goto err_pd;
 if __unlikely(KE_ISERR(error = kshm_initldtcopy(self,right))) goto err_pd;
 /* Enable copy-on-write semantics for the original process. */
 kshmmap_init(&self->s_map);
 if (right->s_map.m_root) {
  error = kshmbranch_fillfork(&self->s_map.m_root,self->s_pd,
                              right->s_map.m_root,right->s_pd);
  if __unlikely(KE_ISERR(error)) goto err_map;
 }
 return error;
err_map: kshmmap_quit(&self->s_map);
         kshm_quitldt(self);
err_pd:  kpagedir_delete(self->s_pd);
 return error;
}
__crit kerrno_t
kshm_initfork(struct kshm *__restrict self,
              struct kshm *__restrict right) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kshm(right);
 error = krwlock_beginwrite(&right->s_lock);
 if __unlikely(KE_ISERR(error)) return error;
 error = kshm_initfork_unlocked(self,right);
 krwlock_endwrite(&right->s_lock);
 return error;
}



__crit __nomp kerrno_t
kshm_mapregion_inherited_unlocked(struct kshm *__restrict self,
                                  __pagealigned __user void *address,
                                  __ref struct kshmregion *__restrict region,
                                  kshmregion_page_t in_region_page_start,
                                  size_t in_region_page_count) {
 struct kshmbranch *branch;
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kshm(self);
 assert(krwlock_iswritelocked(&self->s_lock));
 kassert_kshmregion(region);
 assert(katomic_load(region->sre_branches) != 0);
 assert(in_region_page_count != 0);
 assert(in_region_page_start+in_region_page_count > in_region_page_start);
 assert(in_region_page_start+in_region_page_count <= region->sre_chunk.sc_pages);
 assert(isaligned((uintptr_t)address,PAGEALIGN));
 branch = omalloc(struct kshmbranch);
 if __unlikely(!branch) return KE_NOMEM;
 /* Fill the new branch with information. */
 branch->sb_rstart      = in_region_page_start;
 branch->sb_rpages      = in_region_page_count;
 branch->sb_map_min     =  (uintptr_t)address;
 branch->sb_map_max     = ((uintptr_t)address+in_region_page_count*PAGESIZE)-1;
 branch->sb_region      = region;
 branch->sb_cluster_min = kshmregion_getcluster(region,in_region_page_start);
 branch->sb_cluster_max = kshmregion_getcluster(region,(in_region_page_start+in_region_page_count)-1);
 /* Insert the branch into the tree of SHM mappings.
  * NOTE: Must do this before attempting to map it, as to not overwrite
  *       existing page directory mapping in case of KE_EXISTS. */
#if 0
 printf("BEGIN Insert leaf %p .. %p\n",branch->sb_map_min,branch->sb_map_max);
 kshmbranch_print(self->s_map.m_root,
                  KSHMBRANCH_ADDRSEMI_INIT,
                  KSHMBRANCH_ADDRLEVEL_INIT);                                  
#endif
 error = kshmmap_insert(&self->s_map,branch);
 if __unlikely(KE_ISERR(error)) goto err_branch;
#if 0
 printf("END Insert leaf %p .. %p\n",branch->sb_map_min,branch->sb_map_max);
 kshmbranch_print(self->s_map.m_root,
                  KSHMBRANCH_ADDRSEMI_INIT,
                  KSHMBRANCH_ADDRLEVEL_INIT);
#endif
 assert(kshmmap_locate(&self->s_map,branch->sb_map_min) == branch);
 assert(kshmmap_locate(&self->s_map,branch->sb_map_max) == branch);

 /* (Re-)map the paging memory of the branch. */
 error = kshmbranch_remap(branch,self->s_pd);
 if __likely(KE_ISOK(error)) {
#if AUTOMERGE_BRANCHES
  /* Check if we can merge this region with potentially existing neighbors. */
  if (__likely(KE_ISOK(error)) && katomic_load(region->sre_branches) == 1) {
   struct kshmbranch **pneighbor,**pbranch = NULL;
   uintptr_t neighbor_semi,branch_semi;
   kerrno_t merge_error; kshm_flag_t region_flags;

   region_flags = KSHMREGION_EFFECTIVEFLAGS(region->sre_chunk.sc_flags);
   /* Check for max-region merge.
    * NOTE: Check this first so we don't need to store
    *       'region->sre_chunk.sc_pages' for later. */
   if (in_region_page_start+in_region_page_count == region->sre_chunk.sc_pages) {
    pneighbor = kshmbranch_plocate(&self->s_map.m_root,branch->sb_map_max+1,&neighbor_semi);
    if (pneighbor && kshmbranch_canexpand_min(*pneighbor) && region_flags ==
        KSHMREGION_EFFECTIVEFLAGS((*pneighbor)->sb_region->sre_chunk.sc_flags)) {
     pbranch = kshmbranch_plocate(&self->s_map.m_root,branch->sb_map_min,&branch_semi);
     assert(pbranch && *pbranch == branch);
     /* Merge the branch with its max-neighbor. */
     k_syslogf(KLOG_DEBUG,"[SHM] Merging branch [%p..%p] with %p..%p during mmap()\n",
               branch->sb_map_min,branch->sb_map_max,
              (*pneighbor)->sb_map_min,(*pneighbor)->sb_map_max);
     merge_error = kshmbrach_merge(pbranch,branch_semi,
                                   pneighbor,neighbor_semi,
                                  &pbranch,&branch_semi);
     /* Ignore errors here. */
     if __likely(KE_ISOK(merge_error)) branch = *pbranch;
    }
   }
   if (in_region_page_start == 0) {
    pneighbor = kshmbranch_plocate(&self->s_map.m_root,branch->sb_map_min-1,&neighbor_semi);
    if (pneighbor && kshmbranch_canexpand_max(*pneighbor) && region_flags ==
        KSHMREGION_EFFECTIVEFLAGS((*pneighbor)->sb_region->sre_chunk.sc_flags)) {
     if (!pbranch) {
      pbranch = kshmbranch_plocate(&self->s_map.m_root,branch->sb_map_min,&branch_semi);
      assert(pbranch && *pbranch == branch);
     }
     /* Merge the branch with its min-neighbor. */
     k_syslogf(KLOG_DEBUG,"[SHM] Merging branch %p..%p with [%p..%p] during mmap()\n",
              (*pneighbor)->sb_map_min,(*pneighbor)->sb_map_max,
               branch->sb_map_min,branch->sb_map_max);
     merge_error = kshmbrach_merge(pneighbor,neighbor_semi,
                                   pbranch,branch_semi,
                                  &pbranch,&branch_semi);
     /* Ignore errors here. */
     if __likely(KE_ISOK(merge_error)) branch = *pbranch;
    }
   }
  }
#endif
  return error;
 }
#ifdef __DEBUG__
 {
  struct kshmbranch *oldbranch;
  oldbranch = kshmmap_remove(&self->s_map,branch->sb_map_min);
  assertf(oldbranch == branch,
          "Remove incorrect branch %p (%p .. %p) instead of %p (%p .. %p)",
          oldbranch,oldbranch->sb_map_min,oldbranch->sb_map_max,
          branch,branch->sb_map_min,branch->sb_map_max);
 }
#else
 kshmmap_remove(&self->s_map,branch->sb_map_min);
#endif
err_branch:
 free(branch);
 return error;
}

__crit __nomp kerrno_t
kshm_mapregion_unlocked(struct kshm *__restrict self,
                        __pagealigned __user void *address,
                        struct kshmregion *__restrict region,
                        kshmregion_page_t in_region_page_start,
                        size_t in_region_page_count) {
 kerrno_t error;
 struct kshmcluster *cls_min,*cls_max;
 KTASK_CRIT_MARK
 kassert_kshmregion(region);
 assert(in_region_page_count != 0);
 assert(in_region_page_start+in_region_page_count > in_region_page_start);
 assert(in_region_page_start+in_region_page_count <= region->sre_chunk.sc_pages);
 assert(isaligned((uintptr_t)address,PAGEALIGN));
 cls_min = kshmregion_getcluster(region,in_region_page_start);
 cls_max = kshmregion_getcluster(region,(in_region_page_start+in_region_page_count)-1);
 /* Create references to all affected clusters. */
 error = kshmregion_incref(region,cls_min,cls_max);
 if __unlikely(KE_ISERR(error)) return error;
 /* Actually do the mapping. */
 error = kshm_mapregion_inherited_unlocked(self,address,region,
                                           in_region_page_start,
                                           in_region_page_count);
 if __likely(KE_ISOK(error)) return error;
 /* Must delete all previously created references. */
 kshmregion_decref(region,cls_min,cls_max);
 return error;
}

__crit kerrno_t
kshm_mapregion_inherited(struct kshm *__restrict self,
                         __pagealigned __user void *address,
                         __ref struct kshmregion *__restrict region,
                         kshmregion_page_t in_region_page_start,
                         size_t in_region_page_count) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kshm(self);
 error = krwlock_beginwrite(&self->s_lock);
 if __unlikely(KE_ISERR(error)) return error;
 error = kshm_mapregion_inherited_unlocked(self,address,region,
                                           in_region_page_start,
                                           in_region_page_count);
 krwlock_endwrite(&self->s_lock);
 return error;
}
__crit kerrno_t
kshm_mapregion(struct kshm *__restrict self,
               __pagealigned __user void *address,
               __ref struct kshmregion *__restrict region,
               kshmregion_page_t in_region_page_start,
               size_t in_region_page_count) {
 kerrno_t error;
 struct kshmcluster *cls_min,*cls_max;
 KTASK_CRIT_MARK
 kassert_kshmregion(region);
 assert(in_region_page_count != 0);
 assert(in_region_page_start+in_region_page_count > in_region_page_start);
 assert(in_region_page_start+in_region_page_count <= region->sre_chunk.sc_pages);
 assert(isaligned((uintptr_t)address,PAGEALIGN));
 cls_min = kshmregion_getcluster(region,in_region_page_start);
 cls_max = kshmregion_getcluster(region,(in_region_page_start+in_region_page_count)-1);
 /* Create references to all affected clusters. */
 error = kshmregion_incref(region,cls_min,cls_max);
 if __unlikely(KE_ISERR(error)) return error;
 /* Actually do the mapping. */
 error = kshm_mapregion_inherited(self,address,region,
                                  in_region_page_start,
                                  in_region_page_count);
 if __likely(KE_ISOK(error)) return error;
 /* Must delete all previously created references. */
 kshmregion_decref(region,cls_min,cls_max);
 return error;
}


__crit __nomp size_t
kshm_touch_unlocked(struct kshm *__restrict self,
                    __pagealigned __user void *address,
                    size_t touch_pages, kshm_flag_t req_flags) {
 struct kshmbranch **pbranch,*branch;
 struct kshmregion *region;
 uintptr_t         addr_semi;
 kshmregion_page_t min_page;
 kshmregion_page_t max_page;
 KTASK_CRIT_MARK
 kassert_kshm(self);
 assert(krwlock_iswritelocked(&self->s_lock));
 assert(isaligned((uintptr_t)address,PAGEALIGN));
 assertf(touch_pages != 0,"Undefined behavior: 'touch_pages' is ZERO(0)");
 /* First step: Figure out the branch at the start address.
  * >> If the given address lies outside of a branch, fail immediately.
  * >> If the given address is at the start of a branch, no need to split the branch before. */
 pbranch = kshmbranch_plocate(&self->s_map.m_root,(uintptr_t)address,&addr_semi);
 /* Fail if no branch is mapped to the specified base address. */
 if __unlikely(!pbranch) {
  k_syslogf(KLOG_DEBUG,"[SHM] Failed to locate SHM branch at: %p\n",address);
  kshmbranch_print(self->s_map.m_root,
                   KSHMBRANCH_ADDRSEMI_INIT,
                   KSHMBRANCH_ADDRLEVEL_INIT);
  return 0;
 }
 branch = *pbranch;
 kassertobj(branch);
 region = branch->sb_region;
 kassert_kshmregion(region);
 assert((uintptr_t)address >= branch->sb_map_min);
 assert((uintptr_t)address <= branch->sb_map_max);
 assert(isaligned((uintptr_t)branch->sb_map,PAGEALIGN));

 /* Make sure that the region associated with the
  * address in question is actually writable.
  * >> If it isn't we can't touch it.
  * NOTE: This situation should really never arise, but the specs of this
  *       function (intentionally) don't list this situation as undefined
  *       behavior. */
 if __unlikely((region->sre_chunk.sc_flags&req_flags) != req_flags) return 0;
 /* Figure out all the pages we're actually going to touch within this branch. */
 min_page  = ((uintptr_t)address-(uintptr_t)branch->sb_map)/PAGESIZE;
 min_page += branch->sb_rstart;
 max_page  = min_page+touch_pages;

 return kshm_touchex_unlocked(self,pbranch,addr_semi,
                              min_page,max_page);
}


static __crit __nomp size_t
kshmbranch_touchall_unlocked(struct kshm *__restrict self,
                             struct kshmbranch **__restrict proot,
                             __pagealigned uintptr_t addr_min,
                                           uintptr_t addr_max,
                             uintptr_t addr_semi, unsigned int addr_level,
                             kshmunmap_flag_t req_flags) {
 struct kshmbranch *root;
 size_t result = 0;
 uintptr_t new_semi;
 unsigned int new_level;
 kassert_kshm(self);
 assert(self->s_pd != kpagedir_kernel());
 assert(krwlock_iswritelocked(&self->s_lock));
 kassertobj(proot);
 root = *proot;
 kassertobj(root);
 kassert_kshmregion(root->sb_region);
 assert(isaligned(addr_min,PAGEALIGN));
 assert(addr_min < addr_max);
 if (addr_min <= root->sb_map_max &&
     addr_max >= root->sb_map_min) {
  kshmregion_page_t min_page,max_page; /* Found a matching entry. */
  if __unlikely((root->sb_region->sre_chunk.sc_flags&req_flags) == req_flags) {
   if (addr_min <= root->sb_map_min) min_page = 0;
   else min_page = (addr_min-root->sb_map_min)/PAGESIZE;
   if (addr_max >= root->sb_map_max) max_page = root->sb_rpages;
   else max_page = root->sb_rpages-((root->sb_map_max-addr_max)/PAGESIZE);
   min_page += root->sb_rstart;
   max_page += root->sb_rstart;
   result   += kshm_touchex_unlocked(self,proot,addr_semi,
                                     min_page,max_page);
  }
 }
 /* Continue searching. */
 /* todo: If recursion isn't required ('root' only has one branch),
  *       continue searching inline without calling our own function again. */
 if (addr_min < addr_semi && root->sb_min) {
  /* Recursively continue searching left. */
  new_semi = addr_semi,new_level = addr_level;
  KSHMBRANCH_WALKMIN(new_semi,new_level);
  result += kshmbranch_touchall_unlocked(self,&root->sb_min,addr_min,addr_max,
                                         new_semi,new_level,req_flags);
 }
 if (addr_max >= addr_semi && root->sb_max) {
  /* Recursively continue searching right. */
  new_semi = addr_semi,new_level = addr_level;
  KSHMBRANCH_WALKMAX(new_semi,new_level);
  result += kshmbranch_touchall_unlocked(self,&root->sb_max,addr_min,addr_max,
                                         new_semi,new_level,req_flags);
 }
 return result;
}

__crit size_t
kshm_touch(struct kshm *__restrict self,
           __pagealigned __user void *address,
           size_t touch_pages, kshm_flag_t req_flags) {
 size_t result;
 KTASK_CRIT_MARK
 kassert_kshm(self);
 KTASK_NOINTR_BEGIN
 if __unlikely(KE_ISERR(krwlock_beginwrite(&self->s_lock))) result = 0;
 else {
  result = kshm_touch_unlocked(self,address,touch_pages,req_flags);
  krwlock_endwrite(&self->s_lock);
 }
 KTASK_NOINTR_END
 return result;
}

__crit __nomp size_t
kshm_touchall(struct kshm *__restrict self,
              __pagealigned __user void *address,
              size_t touch_pages, kshm_flag_t req_flags) {
 size_t result;
 KTASK_CRIT_MARK
 kassert_kshm(self);
 assert(touch_pages);
 KTASK_NOINTR_BEGIN
 if __unlikely(KE_ISERR(krwlock_beginwrite(&self->s_lock))) result = 0;
 else {
  result = kshmbranch_touchall_unlocked(self,&self->s_map.m_root,
                                       (uintptr_t)address,
                                       (uintptr_t)address+(touch_pages*PAGESIZE)-1,
                                        KSHMBRANCH_ADDRSEMI_INIT,
                                        KSHMBRANCH_ADDRLEVEL_INIT,
                                        req_flags);
  krwlock_endwrite(&self->s_lock);
 }
 KTASK_NOINTR_END
 return result;
}

__crit size_t
kshm_touchex(struct kshm *__restrict self,
             struct kshmbranch **__restrict pbranch,
             uintptr_t addr_semi,
             kshmregion_page_t min_page,
             kshmregion_page_t max_page) {
 size_t result;
 KTASK_CRIT_MARK
 kassert_kshm(self);
 KTASK_NOINTR_BEGIN
 if __unlikely(KE_ISERR(krwlock_beginwrite(&self->s_lock))) result = 0;
 else {
  result = kshm_touchex_unlocked(self,pbranch,addr_semi,min_page,max_page);
  krwlock_endwrite(&self->s_lock);
 }
 KTASK_NOINTR_END
 return result;
}

__crit __nomp size_t
kshm_touchex_unlocked(struct kshm *__restrict self,
                      struct kshmbranch **__restrict pbranch,
                      uintptr_t addr_semi,
                      kshmregion_page_t min_page,
                      kshmregion_page_t max_page) {
 struct kshmbranch *branch;
 struct kshmregion *region,*subregion;
 kerrno_t          error;
#if AUTOMERGE_BRANCHES
 struct kshmbranch **new_branch,**pneighbor;
 uintptr_t         new_addr_semi,neighbor_semi;
 int did_min_split,did_max_split;
#endif /* AUTOMERGE_BRANCHES */
 KTASK_CRIT_MARK
 kassert_kshm(self);
 kassertobj(pbranch);
 assert(max_page >= min_page);
 branch = *pbranch;
 kassertobj(branch);
 region = branch->sb_region;
 kassert_kshmregion(region);
 assert(branch->sb_rpages);

 /* In order to prevent entire clusters being copied more than once
  * because only a page at a time is being accessed from them, align
  * the page numbers we just figured out by cluster-borders. */
 if (KSHM_CLUSTERSIZE > 1) {
  min_page = alignd(min_page,KSHM_CLUSTERSIZE);
  max_page = align (max_page+1,KSHM_CLUSTERSIZE);
  /* Make sure we don't exceed the maximum! */
  max_page = min(max_page,branch->sb_rstart+branch->sb_rpages);
  --max_page;
 } else {
  /* Make sure we don't exceed the maximum! */
  max_page = min(max_page,branch->sb_rstart+(branch->sb_rpages-1));
 }
 assert(max_page >= min_page);
 assert(min_page >= branch->sb_rstart);
 assert(max_page <  branch->sb_rstart+branch->sb_rpages);
 assert(branch->sb_rstart+branch->sb_rpages > branch->sb_rstart);
 assert(branch->sb_rstart+branch->sb_rpages <= region->sre_chunk.sc_pages);

#if 1
 if (katomic_load(region->sre_branches) == 1) {
#ifdef __DEBUG__
  struct kshmcluster *iter,*end;
  iter = kshmregion_getcluster(region,min_page);
  end  = kshmregion_getcluster(region,max_page)+1;
  do assert(katomic_load(iter->sc_refcnt) == 1);
  while (++iter != end);
#endif
  goto true_unique_access;
 }
#endif

 {
  struct kshmcluster *iter,*end;
  size_t update_pages,affected_pages;
  __user void *effective_address_start;
  kpageflag_t pageflags;
  /* Special optimization:
   * Due to latency and the fact that we can only
   * modify more than one directory during fork(),
   * it is possible for all processes that had
   * simultaneously been using these pages to die
   * off before we started touching them.
   * >> In that case, the pages in question might
   *    still be mapped as read-only in our own page
   *    directory, even though we're actually their sole
   *    owner (either again, or through inheritance).
   * Here we detect that situation by checking all
   * clusters affected by the touch for a reference
   * counter of ONE(1), indicating that we're their owner, and
   * handle it by simply remapping the touch-area as read-write.
   */
  iter = kshmregion_getcluster(region,min_page);
  end  = kshmregion_getcluster(region,max_page)+1;
  do if (katomic_load(iter->sc_refcnt) != 1) goto true_shared_access;
  while (++iter != end);
true_unique_access:
  if ((region->sre_chunk.sc_flags&(KSHMREGION_FLAG_READ|KSHMREGION_FLAG_WRITE)) ==
                                  (KSHMREGION_FLAG_READ|KSHMREGION_FLAG_WRITE)) {
   /* We really are the sole owner of this area of memory.
    * This can happen if we inherited the region after a fork(). */
   effective_address_start = (__user void *)((uintptr_t)branch->sb_map+(min_page-branch->sb_rstart)*PAGESIZE);
   update_pages = (size_t)(max_page-min_page)+1;
   k_syslogf(KLOG_INSANE,"[SHM] Mapping user R/W access to uniquely owned region at %p..%p (effective range %p..%p)\n",
             branch->sb_map_min,branch->sb_map_max,effective_address_start,
            (__user void *)((uintptr_t)effective_address_start+(update_pages*PAGESIZE)-1));
   pageflags = PAGEDIR_FLAG_USER|PAGEDIR_FLAG_READ_WRITE;
   affected_pages = kpagedir_setflags(self->s_pd,effective_address_start,
                                      update_pages,(kpageflag_t)-1,pageflags);
   assertf(affected_pages == update_pages,
           "Unexpected amount of pages affected during R/W-remapping (Expected %Iu; got %Iu)",
          update_pages,affected_pages);
   assertf(kpagedir_ismappedex(self->s_pd,effective_address_start,
                               update_pages,pageflags,pageflags),
           "Invalid paging flags in range %p..%p",effective_address_start,
          (__user void *)(((uintptr_t)effective_address_start+update_pages*PAGESIZE)-1));
  }
  goto end_success;
true_shared_access:;
  /* Access to this region must truly be shared. */
 }

 assert(branch->sb_cluster_min <= branch->sb_cluster_max);
 assert(branch->sb_cluster_min >= region->sre_clusterv &&
        branch->sb_cluster_min <  region->sre_clusterv+
                                  region->sre_clusterc);
 /* If out min_page lies at the start of the region,
  * we don't need to create an additional branch immediately
  * before, as it would cover less than one cluster
  * (which is unnecessary, potentially illegal (if its
  *  really empty), as well as completely wasteful).
  * NOTE: The standard itself technically doesn't make
  *       regions smaller than a single sector illegal! */
 if (min_page == branch->sb_rstart) {
  /* The area we're touching begins in the first cluster,
   * meaning we don't need to create a split directly before! */
#if AUTOMERGE_BRANCHES
  did_min_split = 0;
#endif /* AUTOMERGE_BRANCHES */
 } else {
  /* Must split the branch at 'min_page'! */
  error = kshmbrach_putsplit(pbranch,addr_semi,NULL,NULL,
                            (struct kshmbranch ***)&pbranch,
                             &addr_semi,min_page);
  if __unlikely(KE_ISERR(error)) {
   k_syslogf(KLOG_ERROR,"[SHM] Failed to split min-branch %p..%p at %p: %d\n",
             branch->sb_map_min,branch->sb_map_max,
             branch->sb_map_min+(min_page-branch->sb_rstart)*PAGESIZE,
             error);
   return 0;
  }
  kassertobj(pbranch);
  branch = *pbranch;
  kassertobj(branch);
  assert(branch == kshmmap_locate(&self->s_map,branch->sb_map_min));
  assert(branch == kshmmap_locate(&self->s_map,branch->sb_map_max));
  assert(branch->sb_region == region);
#if AUTOMERGE_BRANCHES
  did_min_split = 1;
#endif /* AUTOMERGE_BRANCHES */
  /* The branch we're now working with has been modified
   * to basically strip away the first 'min_page' pages of
   * memory associated with the branch. */
 }
 /* At this point we know which branch we're going to copy
  * clusters from, as well as how many clusters it's going to be.
  * But we still must check for the potential of
  * a second split near the end of 'pbranch'. */
 assert(max_page <  region->sre_chunk.sc_pages);
 assert(max_page <  branch->sb_rstart+branch->sb_rpages);
 assert(max_page >= branch->sb_rstart);
 assert(branch->sb_cluster_min <= branch->sb_cluster_max);
 assert(branch->sb_cluster_max >= region->sre_clusterv &&
        branch->sb_cluster_max <  region->sre_clusterv+
                                  region->sre_clusterc);
 assert(branch == *pbranch);
 if (max_page == branch->sb_rstart+(branch->sb_rpages-1)) {
  /* The greatest touched page lies within the last cluster.
   * >> No need to create the second split! */
#if AUTOMERGE_BRANCHES
  did_max_split = 0;
#endif /* AUTOMERGE_BRANCHES */
 } else {
  error = kshmbrach_putsplit(pbranch,addr_semi,
                            (struct kshmbranch ***)&pbranch,
                             &addr_semi,NULL,NULL,
                             max_page+1);
  if __unlikely(KE_ISERR(error)) {
   /* todo: We should probably clean up the first split, although
    *       the system still remains stable if we don't... */
   k_syslogf(KLOG_ERROR,"[SHM] Failed to split max-branch %p..%p at %p: %d\n",
             branch->sb_map_min,branch->sb_map_max,
             branch->sb_map_min+((max_page+1)-branch->sb_rstart)*PAGESIZE,
             error);
   return 0;
  }
  kassertobj(pbranch);
  branch = *pbranch;
  kassertobj(branch);
  assert(branch->sb_region == region);
  assert(branch == kshmmap_locate(&self->s_map,branch->sb_map_min));
  assert(branch == kshmmap_locate(&self->s_map,branch->sb_map_max));
#if AUTOMERGE_BRANCHES
  did_max_split = 1;
#endif /* AUTOMERGE_BRANCHES */
 }
 /* All splits have been created, and we are ready
  * to create a sub-range copy of 'region', as
  * described by the min/max ranges within 'branch'.
  * >> Then all that's left is to re-map all the
  *    newly copied pages as read-write. */
 subregion = kshmregion_extractpart(region,
                                    branch->sb_cluster_min,
                                    branch->sb_cluster_max);
 if __unlikely(!subregion) {
  /* todo: We should probably clean up the first split, although
   *       the system still remains stable if we don't... */
  k_syslogf(KLOG_ERROR,"[SHM] Failed to extract sub-region from %p mapped at %p..%p\n",
            region,branch->sb_map_min,branch->sb_map_max);
  return 0;
 }
 assert(subregion->sre_chunk.sc_partc != 0);
 assert(subregion->sre_chunk.sc_pages != 0);
 assert(subregion->sre_clusterc       != 0);
 assert(subregion->sre_branches       == 1);
 assert(subregion->sre_chunk.sc_pages >= branch->sb_rpages);
 assert(branch->sb_region == region);
 /* The branch still holds a reference to the branch counter of the old region.
  * We must drop that reference, noting that it can actually fall to ZERO(0) here. */
 {
#ifdef __DEBUG__
  size_t branchc = katomic_decfetch(region->sre_branches);
  assert(branchc != (size_t)-1);
  if (!branchc)
#else
  if (!katomic_decfetch(region->sre_branches))
#endif
  {
   free(region->sre_chunk.sc_partv);
   free(region);
  }
 }

 /* Overwrite the existing region within the branch with the new sub-region.
  * >> Since the new sub-region describes the exact sub-range previously covered
  *    by the branch, the branch now simply describes the entirety of its region. */
 branch->sb_region  = subregion; /*< Inherit reference. */
 branch->sb_rstart %= KSHM_CLUSTERSIZE; /*< Within the new sub-region, the start points into the first cluster. */
 /*branch->sb_KSHMREGION_FLAG_NOCOPYrpages = ...; This didn't change. */
 branch->sb_cluster_min = subregion->sre_clusterv;
 branch->sb_cluster_max = subregion->sre_clusterv+(subregion->sre_clusterc-1);
 assert(branch->sb_cluster_min == kshmregion_getcluster(subregion,branch->sb_rstart));
 assert(branch->sb_cluster_max == kshmregion_getcluster(subregion,(branch->sb_rstart+branch->sb_rpages)-1));
 /* Make sure that the area of memory described by this branch is really mapped.
  * If this fails (which it shouldn't unless something went wrong), the call to
  * 'kshmbranch_remap' would have a potential to fail at a time when we
  * are past the point of no return when it comes to reverting changes. */
 assert(kpagedir_ismapped(self->s_pd,branch->sb_map,branch->sb_rpages));
 /* Now all that's left is to re-map the area of memory covered by this
  * now writable branch and to do some optional checks for merging branches.
  * NOTE: Since we're not actually mapping any new memory, there should already
  *       be an existing mapping of this area of memory in the page directory,
  *       meaning that writing to the page directory will not have to allocate
  *       new entries (which could potentially fail), but can simply overwrite
  *       existing ones (which cannot fail). */
 error = kshmbranch_remap(branch,self->s_pd);
 assertf(KE_ISOK(error),"%d",error);
#ifdef __DEBUG__
 { /* Make sure the region is still mapped correctly (and with full access). */
  kpageflag_t flags = 0;
  if (region->sre_chunk.sc_flags&KSHMREGION_FLAG_READ) {
   flags |= PAGEDIR_FLAG_USER;
   if (region->sre_chunk.sc_flags&KSHMREGION_FLAG_WRITE)
    flags |= PAGEDIR_FLAG_READ_WRITE;
  }
  assertf(kpagedir_ismappedex_b(self->s_pd,branch->sb_map,branch->sb_rpages,flags,flags),
          "Region (%c%c) at %p..%p is not mapped correctly",
         (region->sre_chunk.sc_flags&KSHMREGION_FLAG_READ) ? 'R' : '-',
         (region->sre_chunk.sc_flags&KSHMREGION_FLAG_WRITE) ? 'W' : '-',
          branch->sb_map_min,branch->sb_map_max);
 }
#endif

#if AUTOMERGE_BRANCHES
 /* The branch we're supposed to copy doesn't map the start of its associated
  * region, yet we didn't perform a lower-bound split, meaning there's a high
  * probability that we're going to find another piece to the puzzle if we look
  * for another leaf mapped to the user-space memory directly below the mapped
  * memory of our own branch.
  * If it exists, it's likely to be a fully mapped and unique
  * region that we are therefor allowed to modify and expand upon.
  *
  * Such a memory layout will result from multiple touches of
  * consecutive branches, resulting from something like:
  * >> void *big_memory = malloc(4096*4096*128); // 128Mib
  * >> fork(); // Invoke COW semantics
  * >> memset(big_memory,0xAA,malloc_usable_size(big_memory));
  * >> exit(0);
  *
  * Memory is modified post-fork, meaning that copy-on-write is active,
  * but since the memory modifications aren't performed all at once,
  * the kernel will only get informed about the touches in chunks of
  * (by default) about ~64Kib (KSHM_CLUSTERSIZE*PAGESIZE).
  * The actual SHM mappings at that time will look as follows:
  * #1 [.................big_memory....] (Before 'memset')
  * #2 [.][..............big_memory....] (After 'memset' begins writing the first cluster)
  * #3 [.][.][...........big_memory....] (After 'memset' begins writing the second cluster)
  * #4 [.][.][.][........big_memory....] (...)
  * #6 [.][.][.][.][.][..big_memory....] (It get worse and worse...)
  *
  * The following code detects such situations starting at #3,
  * meaning the two branches and the associated regions into
  * a single branch+region.
  * #3 [....][...........big_memory....] (Layout after #3 with auto-merging)
  * #4 [.......][........big_memory....] (...)
  */
#define CHECK_MIN_NEIGHBOR   (!did_min_split && branch->sb_rstart == 0)
#define CHECK_MAX_NEIGHBOR   (!did_max_split && (branch->sb_rstart+branch->sb_rpages) == region->sre_chunk.sc_pages)
 assertf(branch->sb_rstart+branch->sb_rpages <= region->sre_chunk.sc_pages
        ,"Branch is mapping an out-of-bounds region of pages %Iu..%Iu not in 0..%Iu"
        ,branch->sb_rstart,branch->sb_rstart+branch->sb_rpages
        ,region->sre_chunk.sc_pages);

 if (CHECK_MIN_NEIGHBOR) {
  /* Fully mapped region memory at the front (high potential for a neighbor below) */
  pneighbor = kshmbranch_plocate(&self->s_map.m_root,branch->sb_map_min-1,&neighbor_semi);
  /* Note how we also compare the flags, as not to
   * accidentally merge a '.text' with a '.bss' section. */
  if (pneighbor && kshmbranch_canexpand_max(*pneighbor) &&
      KSHMREGION_EFFECTIVEFLAGS(region->sre_chunk.sc_flags) ==
      KSHMREGION_EFFECTIVEFLAGS((*pneighbor)->sb_region->sre_chunk.sc_flags)) {
   /* We can expand this 'neighbor' to merge it with 'branch' */
   assert((*pneighbor)->sb_map_max == branch->sb_map_min-1);
   error = kshmbrach_merge(pneighbor,neighbor_semi,
                           pbranch,addr_semi,
                          &new_branch,&new_addr_semi);
   if __likely(KE_ISOK(error)) {
    pbranch   = new_branch;
    addr_semi = new_addr_semi;
    branch    = *pbranch;
   }
  }
 }
 if (CHECK_MAX_NEIGHBOR) {
  /* Fully mapped region memory at the back (high potential for a neighbor above) */
  pneighbor = kshmbranch_plocate(&self->s_map.m_root,branch->sb_map_max+1,&neighbor_semi);
  if (pneighbor && kshmbranch_canexpand_min(*pneighbor) &&
      KSHMREGION_EFFECTIVEFLAGS(region->sre_chunk.sc_flags) ==
      KSHMREGION_EFFECTIVEFLAGS((*pneighbor)->sb_region->sre_chunk.sc_flags)) {
   /* We can expand this 'neighbor' to merge it with 'branch' */
   assert((*pneighbor)->sb_map_min == branch->sb_map_max+1);
   error = kshmbrach_merge(pbranch,addr_semi,
                           pneighbor,neighbor_semi,
                          &new_branch,&new_addr_semi);
   if __likely(KE_ISOK(error)) {
    pbranch   = new_branch;
    addr_semi = new_addr_semi;
    branch    = *pbranch;
   }
  }
 }
#endif /* AUTOMERGE_BRANCHES */

#undef CHECK_MAX_NEIGHBOR
#undef CHECK_MIN_NEIGHBOR

 k_syslogf(COW_LOGLEVEL,"[SHM] Touched %Iu SHM pages at address %p (%p..%p)\n",
          (max_page-min_page)+1,
          (uintptr_t)branch->sb_map+(min_page-branch->sb_rstart)*PAGESIZE,
           branch->sb_map_min,branch->sb_map_max);
end_success:
 /* Tell the caller how many pages we actually touched. */
 return (max_page-min_page)+1;
}


__crit __nomp size_t
kshm_unmap_unlocked(struct kshm *__restrict self,
                    __pagealigned __user void *address,
                    size_t pages, kshmunmap_flag_t flags,
                    struct kshmregion *origin) {
 kassert_kshm(self);
 if __unlikely(!pages || !self->s_map.m_root) return 0;
 return kshmbranch_unmap(&self->s_map.m_root,self->s_pd,
                        (uintptr_t)address,
                        (uintptr_t)address+(pages*PAGESIZE)-1,
                         KSHMBRANCH_ADDRSEMI_INIT,
                         KSHMBRANCH_ADDRLEVEL_INIT,
                         flags,origin);
}
__crit size_t
kshm_unmap(struct kshm *__restrict self,
           __pagealigned __user void *address,
           size_t pages, kshmunmap_flag_t flags,
           struct kshmregion *origin) {
 size_t result;
 KTASK_CRIT_MARK
 kassert_kshm(self);
 KTASK_NOINTR_BEGIN
 if __unlikely(KE_ISERR(krwlock_beginwrite(&self->s_lock))) result = 0;
 else {
  result = kshm_unmap_unlocked(self,address,pages,flags,origin);
  krwlock_endwrite(&self->s_lock);
 }
 KTASK_NOINTR_END
 return result;
}

__crit __nomp kerrno_t
kshm_unmapregion_unlocked(struct kshm *self,
                          __pagealigned __user void *base_address,
                          struct kshmregion *region) {
 struct kshmbranch **pbranch,*branch;
 uintptr_t addr_semi;
 kerrno_t error;
 kassert_kshm(self);
 assert(krwlock_iswritelocked(&self->s_lock));
 kassert_kshmregion(region);
 assert(isaligned((uintptr_t)base_address,PAGEALIGN));
 pbranch = kshmbranch_plocate(&self->s_map.m_root,
                             (uintptr_t)base_address,
                              &addr_semi);
 /* Make sure that there really is a branch mapped at the given address. */
 if __unlikely(!pbranch) { error = KE_FAULT; goto unmap_origin; }
 branch = *pbranch;
 kassertobj(branch);
 /* Make sure that the given address is really the base of that branch. */
 if __unlikely(base_address != branch->sb_map) { error = KE_RANGE; goto unmap_origin; }
 /* Finally, make sure that it is the given region, that is mapped here. */
 if __unlikely(branch->sb_region != region) { error = KE_PERM; goto unmap_origin; }
 /* OK! Time to delete this thing! */
 asserte(kshmbranch_popone(pbranch,addr_semi,KSHMBRANCH_SEMILEVEL(addr_semi)) == branch);
 kpagedir_unmap(self->s_pd,base_address,branch->sb_rpages);
 /* Drop references from all affected clusters. */
 kshmregion_decref(region,
                   branch->sb_cluster_min,
                   branch->sb_cluster_max);
 /* Free the branch. */
 free(branch);
 return KE_OK;
unmap_origin:
 /* Scan the entire address range covered by the region,
  * unmapping everything that is originating from it. */
 if (kshm_unmap_unlocked(self,base_address,region->sre_chunk.sc_pages,
                         KSHMUNMAP_FLAG_RESTRICTED,region)) {
  /* If we managed to unmap regions with this one as origin, we
   * still managed to do it. (So don't return an error in that case) */
  error = KE_OK;
 }
 return error;
}

__crit kerrno_t
kshm_unmapregion(struct kshm *__restrict self,
                 __pagealigned __user void *base_address,
                 struct kshmregion *__restrict region) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kshm(self);
 error = krwlock_beginwrite(&self->s_lock);
 if __unlikely(KE_ISERR(error)) return error;
 error = kshm_unmapregion_unlocked(self,base_address,region);
 krwlock_endwrite(&self->s_lock);
 return error;
}




__crit __nomp kerrno_t
kshm_mapautomatic_inherited_unlocked(struct kshm *__restrict self,
                              /*out*/__pagealigned __user void **__restrict user_address,
                                     struct kshmregion *__restrict region, __pagealigned __user void *hint,
                                     kshmregion_page_t in_region_page_start, size_t in_region_page_count) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kshm(self);
 assert(krwlock_iswritelocked(&self->s_lock));
 assert(isaligned((uintptr_t)hint,PAGEALIGN));
 kassertobj(user_address);
 kassert_kshmregion(region);
 if __unlikely((*user_address = kpagedir_findfreerange(
                self->s_pd,region->sre_chunk.sc_pages,hint
               )) == KPAGEDIR_FINDFREERANGE_ERR) error = KE_NOSPC;
 else {
  error = kshm_mapregion_inherited_unlocked(self,*user_address,region,
                                            in_region_page_start,
                                            in_region_page_count);
  assertf(error != KE_EXISTS,"But we've just found this as an unused memory range...");
 }
 return error;
}
__crit __nomp kerrno_t
kshm_mapautomatic_inherited(struct kshm *__restrict self,
                            /*out*/__pagealigned __user void **__restrict user_address,
                            struct kshmregion *__restrict region, __pagealigned __user void *hint,
                            kshmregion_page_t in_region_page_start, size_t in_region_page_count) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kshm(self);
 kassertobj(user_address);
 kassert_kshmregion(region);
 error = krwlock_beginwrite(&self->s_lock);
 if __unlikely(KE_ISERR(error)) return error;
 error = kshm_mapautomatic_inherited_unlocked(self,user_address,region,hint,
                                              in_region_page_start,
                                              in_region_page_count);
 krwlock_endwrite(&self->s_lock);
 return error;
}
__crit __nomp kerrno_t
kshm_mapautomatic_unlocked(struct kshm *__restrict self,
                    /*out*/__pagealigned __user void **__restrict user_address,
                           struct kshmregion *__restrict region, __pagealigned __user void *hint,
                           kshmregion_page_t in_region_page_start, size_t in_region_page_count) {
 kerrno_t error;
 struct kshmcluster *cls_min;
 struct kshmcluster *cls_max;
 KTASK_CRIT_MARK
 kassert_kshm(self);
 assert(in_region_page_count);
 cls_min = kshmregion_getcluster(region,in_region_page_start);
 cls_max = kshmregion_getcluster(region,in_region_page_start+(in_region_page_count-1));
 error = kshmregion_incref(region,cls_min,cls_max);
 if __unlikely(KE_ISERR(error)) return error;
 error = kshm_mapautomatic_inherited_unlocked(self,user_address,region,hint,
                                              in_region_page_start,in_region_page_count);
 if __unlikely(KE_ISERR(error)) kshmregion_decref(region,cls_min,cls_max);
 return error;
}
__crit kerrno_t
kshm_mapautomatic(struct kshm *__restrict self,
           /*out*/__pagealigned __user void **__restrict user_address,
                  struct kshmregion *__restrict region, __pagealigned __user void *hint,
                  kshmregion_page_t in_region_page_start, size_t in_region_page_count) {
 kerrno_t error;
 struct kshmcluster *cls_min;
 struct kshmcluster *cls_max;
 KTASK_CRIT_MARK
 kassert_kshm(self);
 assert(in_region_page_count);
 cls_min = kshmregion_getcluster(region,in_region_page_start);
 cls_max = kshmregion_getcluster(region,in_region_page_start+(in_region_page_count-1));
 error = kshmregion_incref(region,cls_min,cls_max);
 if __unlikely(KE_ISERR(error)) return error;
 error = kshm_mapautomatic_inherited(self,user_address,region,hint,
                                     in_region_page_start,in_region_page_count);
 if __unlikely(KE_ISERR(error)) kshmregion_decref(region,cls_min,cls_max);
 return error;
}

__crit kerrno_t
kshm_devaccess(struct ktask *__restrict caller,
               __pagealigned __kernel void const *base_address,
               size_t pages) {
 __pagealigned void *end_address;
 assert(isaligned((uintptr_t)base_address,PAGEALIGN));
 end_address = (void *)((uintptr_t)base_address+pages*PAGESIZE);
 assert(!end_address || end_address >= base_address);
#define WHITELIST(start,size) \
 if ((uintptr_t)base_address >= alignd(start,PAGEALIGN) && \
     (uintptr_t)end_address  <= align((start)+(size),PAGEALIGN)) return KE_OK;
 kassert_ktask(caller);
 /* If the caller has SETMEM access to the kernel-ZERO task,
  * they obviously have access to all of its memory
  * (which is the entirety of the real, physical memory). */
 if (ktask_accesssm_ex(ktask_zero(),caller)) return KE_OK;

#ifdef __x86__
 /* Special region of memory: The X86 VGA terminal.
  * TODO: We still shouldn't allow everyone access to this... */
 WHITELIST(0xB8000,80*25*2);
#endif

 /* Deny access by default. */
 return KE_ACCES;
}





void kshm_pf_handler(struct kirq_registers *__restrict regs) {
 struct ktask *caller = ktask_self();
 struct kproc *caller_process;
 size_t touched_pages;
#define PF_PRESENT 0x01 /*< The fault was caused by a present page. */
#define PF_WRITE   0x02 /*< The fault was caused by a write operation. */
#define PF_USER    0x04 /*< The fault occurred in ring-#3. */
#define PF_IFETCH  0x10 /*< The fault was caused during an instruction fetch. */
#define PF_COW   (PF_PRESENT|PF_WRITE|PF_USER)
 __user void *address_in_question;
 /* Make sure that we're not in some strange
  * situation in which the kernel is burning... */
 if ((regs->regs.ecode&PF_COW) != PF_COW) goto def_handler;
 if __unlikely(!caller) goto def_handler;
 assertf(caller->t_flags&KTASK_FLAG_USERTASK,
         "But we've already checked for ring #3");
 address_in_question = x86_getcr2();
 kassert_ktask(caller);
 caller_process = caller->t_proc;
 kassert_kproc(caller_process);
 assertf(caller_process != kproc_kernel(),
         "Should have been detected by kernel-task detection");
 /* Acquire the SHM-lock to ensure that nothing gets in our way... */
 NOIRQ_BEGIN
 if __unlikely(KE_ISERR(krwlock_beginwrite(&caller_process->p_shm.s_lock))) goto def_handler;
 touched_pages = kshm_touch_unlocked(kproc_getshm(caller_process),
                                    (__user void *)alignd((uintptr_t)address_in_question,PAGEALIGN),1,
                                     KSHMREGION_FLAG_READ|KSHMREGION_FLAG_WRITE);
 /* kshmbranch_print(kproc_getshm(caller_process)->s_map.m_root,
  *                  KSHMBRANCH_ADDRSEMI_INIT,KSHMBRANCH_ADDRLEVEL_INIT); */
 assertf(!touched_pages || kpagedir_ismappedex_b(kproc_getpagedir(caller_process),address_in_question,1,
                                                 PAGEDIR_FLAG_USER|PAGEDIR_FLAG_READ_WRITE,
                                                 PAGEDIR_FLAG_USER|PAGEDIR_FLAG_READ_WRITE
         ),"Failed to remap COW pages at %p (%p) (touched %Iu pages)"
          ,address_in_question
          ,(__user void *)alignd((uintptr_t)address_in_question,PAGEALIGN)
          ,touched_pages);
 krwlock_endwrite(&caller_process->p_shm.s_lock);
 NOIRQ_END

 /* If nothing was touched, call the default handler. */
 if __unlikely(!touched_pages) goto def_handler;
 return;

def_handler:
 /* Fallback: Call the default IRQ handler. */
 (*kirq_default_handler)(regs);
}


void kernel_initialize_copyonwrite(void) {
 k_syslogf(KLOG_INFO,"[init] Copy-on-write...\n");
 /* Install the COW #PF handler. */
 kirq_sethandler(KIRQ_EXCEPTION_PF,&kshm_pf_handler);
}

__DECL_END

#ifndef __INTELLISENSE__
#include "shm-futex.c.inl"
#include "shm-ldt.c.inl"
#include "shm-map.c.inl"
#include "shm-syscall.c.inl"
#include "shm-transfer.c.inl"
#define WRITEABLE
#include "shm-translate.c.inl"
#include "shm-translate.c.inl"
#define UNLOCKED
#include "shm-mmap-impl.c.inl"
#include "shm-mmap-impl.c.inl"
#endif

#endif /* !__KOS_KERNEL_SHM_C__ */

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
#ifndef __KOS_KERNEL_SCHED_TLS_C__
#define __KOS_KERNEL_SCHED_TLS_C__ 1

#include <kos/compiler.h>
#include <kos/kernel/debug.h>
#include <kos/kernel/proc.h>
#include <kos/kernel/linker.h>
#include <kos/kernel/shm.h>
#include <kos/kernel/task.h>
#include <kos/kernel/tls-perthread.h>
#include <kos/kernel/tls.h>
#include <kos/syslog.h>
#include <malloc.h>
#include <math.h>
#include <stddef.h>
#include <string.h>
#include <strings.h>
#include <kos/exception.h>
#include <kos/mod.h>
#if !KCONFIG_HAVE_TASK_STATS_START
#include <kos/kernel/time.h>
#endif /* !KCONFIG_HAVE_TASK_STATS_START */

__DECL_BEGIN

//////////////////////////////////////////////////////////////////////////
// === ktlsman ===
void ktlsman_init(struct ktlsman *__restrict self) {
 kassertobj(self);
 kobject_init(self,KOBJECT_MAGIC_TLSMAN);
 krwlock_init(&self->tls_lock);
 self->tls_hiend     = NULL;
 self->tls_map_pages = 0;
 self->tls_all_pages = KUTHREAD_PAGESIZE;
}
void ktlsman_quit(struct ktlsman *__restrict self) {
 struct ktlsmapping *iter,*prev;
 kassert_ktlsman(self);
 iter = self->tls_hiend;
 while (iter) {
  prev = iter->tm_prev;
  kshmregion_decref_full(iter->tm_region);
  free(iter);
  iter = prev;
 }
}

kerrno_t
ktlsman_initcopy_unlocked(struct ktlsman *__restrict self,
                          struct ktlsman const *__restrict right) {
 struct ktlsmapping **pdst,*iter,*copy;
 kerrno_t error;
 kassertobj(self);
 kassert_ktlsman(right);
 kobject_init(self,KOBJECT_MAGIC_TLSMAN);
 assert(krwlock_isreadlocked(&right->tls_lock) ||
        krwlock_iswritelocked(&right->tls_lock));
 krwlock_init(&self->tls_lock);
 self->tls_map_pages = right->tls_map_pages;
 self->tls_all_pages = right->tls_all_pages;
 pdst = &self->tls_hiend;
 iter = right->tls_hiend;
 while (iter) {
  copy = (struct ktlsmapping *)memdup(iter,sizeof(struct ktlsmapping));
  if __unlikely(!copy) { error = KE_NOMEM; goto err; }
  error = kshmregion_incref_full(copy->tm_region);
  if __unlikely(KE_ISERR(error)) { free(copy); goto err; }
  *pdst = copy;
  pdst = &copy->tm_prev;
  iter = iter->tm_prev;
 }
 *pdst = NULL;
 return KE_OK;
err:
 *pdst = NULL;
 iter = self->tls_hiend;
 while (iter) {
  copy = iter->tm_prev;
  kshmregion_decref_full(iter->tm_region);
  free(iter);
  iter = copy;
 }
 return error;
}

__local __crit __nomp void
ktlsman_insmapping_inherited(struct ktlsman *__restrict self,
                             struct ktlsmapping *__restrict mapping) {
 struct ktlsmapping *old_mapping,*prev_mapping;
 /* Search for unused address ranges between mappings of suitable size, as
  * well as finding the end of the mapping chain as required in case we
  * have to prepend the new mapping in front of all others. */
 if ((old_mapping = self->tls_hiend) != NULL) {
  for (;;) {
   size_t free_pages;
   if ((prev_mapping = old_mapping->tm_prev) == NULL) break;
   assert(prev_mapping->tm_pages);
   assert(prev_mapping->tm_offset <= old_mapping->tm_offset);
   assert(prev_mapping->tm_offset+prev_mapping->tm_pages*PAGESIZE <= old_mapping->tm_offset);
   free_pages = (size_t)(old_mapping->tm_offset-
                        (prev_mapping->tm_offset+
                         prev_mapping->tm_pages*PAGESIZE))/PAGESIZE;
   if (free_pages >= mapping->tm_pages) {
    /* We're in business! - Link the new mapping. */
    mapping->tm_offset   = prev_mapping->tm_offset+prev_mapping->tm_pages*PAGESIZE;
    old_mapping->tm_prev = mapping;
    mapping->tm_prev     = prev_mapping;
    return;
   }
   old_mapping = prev_mapping;
  }
  /* Special case: Prepend before 'old_mapping'. */
  assert(!old_mapping->tm_prev); /*< Sanity check. */
  mapping->tm_prev     =  NULL;
  mapping->tm_offset   =  old_mapping->tm_offset-mapping->tm_pages*PAGESIZE;
  old_mapping->tm_prev =  mapping;
  self->tls_map_pages  = (size_t)(-mapping->tm_offset)/PAGESIZE;
  goto update_allpages;
 } else {
  /* Special case: First mapping. */
  assert(self->tls_map_pages == 0);
  assert(self->tls_all_pages == KUTHREAD_PAGESIZE);
  mapping->tm_prev    = NULL;
  mapping->tm_offset  = -(ktls_addr_t)(mapping->tm_pages*PAGESIZE);
  self->tls_hiend     = mapping;
  self->tls_map_pages = mapping->tm_pages;
update_allpages:
  self->tls_all_pages = self->tls_map_pages+KUTHREAD_PAGESIZE;
 }
}
__local __crit __nomp void
ktlsman_delmapping(struct ktlsman *__restrict self,
                   struct ktlsmapping *__restrict find_mapping) {
 struct ktlsmapping *mapping,**pmapping;
 pmapping = &self->tls_hiend;
 while ((mapping = *pmapping) != NULL) {
  if (find_mapping == mapping) { *pmapping = mapping->tm_prev; break; }
  pmapping = &mapping->tm_prev;
 }
}
__local __crit __nomp kerrno_t
ktlsman_delregion(struct ktlsman *__restrict self,
                  struct kshmregion *__restrict region) {
 struct ktlsmapping *mapping,**pmapping;
 pmapping = &self->tls_hiend;
 while ((mapping = *pmapping) != NULL) {
  if (region == mapping->tm_region) {
   *pmapping = mapping->tm_prev;
   free(mapping);
   return KE_OK;
  }
  pmapping = &mapping->tm_prev;
 }
 return KE_NOENT;
}

/* Without performing any locking, unmap all TLS regions mapped to the given task's PT.
 * NOTE: Regions not properly mapped are silently ignored.
 * WARNING: To prevent race conditions, the caller should suspend the task beforehand. */
__local __crit __nomp void
kproc_unmap_pt(struct kproc *__restrict self,
               struct ktask *__restrict task,
               __pagealigned __user struct kuthread *oldbase) {
 struct ktlsmapping *iter;
 kassert_kproc(self);
 kassert_ktask(task);
 assert(task->t_proc == self);
 assert(krwlock_iswritelocked(&self->p_shm.s_lock));
 assert(isaligned((uintptr_t)oldbase,PAGEALIGN));
 iter = self->p_tls.tls_hiend;
 while (iter) {
  assert(isaligned(iter->tm_offset,PAGEALIGN));
  kshm_unmapregion_unlocked(&self->p_shm,
                           (__user void *)((uintptr_t)oldbase+iter->tm_offset),
                            iter->tm_region);
  iter = iter->tm_prev;
 }
 kshm_unmapregion_unlocked(&self->p_shm,
                           task->t_tls.pt_uthread,
                           task->t_tls.pt_uregion);
}

/* Without performing any locking, remap all TLS regions to the given task's PT at 'newbase'.
 * NOTE: Regions not properly mapped are silently ignored.
 * WARNING: To prevent race conditions, the caller should suspend the task beforehand.
 * @return: KE_OK:    Successfully remapped the task.
 * @return: KE_NOMEM: Not enough available memory. */
__local __crit __nomp kerrno_t
kproc_remap_pt(struct kproc *__restrict self,
               struct ktask *__restrict task,
               __pagealigned __user struct kuthread *newbase) {
 struct ktlsmapping *iter,*iter2;
 kerrno_t error;
 kassert_kproc(self);
 kassert_ktask(task);
 assert(task->t_proc == self);
 assert(krwlock_iswritelocked(&self->p_shm.s_lock));
 assert(isaligned((uintptr_t)newbase,PAGEALIGN));
 k_syslogf(KLOG_TRACE,"Mapping thread control block for %p:%I32d:%Iu:%s at %p\n",
           task,task->t_proc->p_pid,task->t_tid,ktask_getname(task),newbase);
 error = kshm_mapfullregion_unlocked(&self->p_shm,newbase,task->t_tls.pt_uregion);
 if __unlikely(KE_ISERR(error)) return error;
 iter = self->p_tls.tls_hiend;
 while (iter) {
  assert(isaligned(iter->tm_offset,PAGEALIGN));
  assert(iter->tm_pages == iter->tm_region->sre_chunk.sc_pages);
  k_syslogf(KLOG_TRACE,"Mapping TLS region at %p\n",(__user void *)((uintptr_t)newbase+iter->tm_offset));
  error = kshm_mapfullregion_unlocked(&self->p_shm,
                                     (__user void *)((uintptr_t)newbase+iter->tm_offset),
                                      iter->tm_region);
  if __unlikely(KE_ISERR(error)) {
   k_syslogf(KLOG_ERROR,"Failed to remap TLS PT at %p (offset %Id): %d\n",
            (__user void *)((uintptr_t)newbase+iter->tm_offset),iter->tm_offset,error);
   kpagedir_print(self->p_shm.s_pd);
   goto err;
  }
  assert(!iter->tm_prev ||
         iter->tm_prev->tm_offset+
         iter->tm_prev->tm_pages*PAGESIZE <=
         iter->tm_offset);
  iter = iter->tm_prev;
 }
 return KE_OK;
err:
 iter2 = self->p_tls.tls_hiend;
 while (iter2 != iter) {
  assert(isaligned(iter2->tm_offset,PAGEALIGN));
  kshm_unmapregion_unlocked(&self->p_shm,
                           (__user void *)((uintptr_t)newbase+iter2->tm_offset),
                            iter2->tm_region);
  iter2 = iter2->tm_prev;
 }
 kshm_unmapregion_unlocked(&self->p_shm,newbase,
                           task->t_tls.pt_uregion);
 return error;
}

__crit kerrno_t
kproc_tls_alloc_inherited(struct kproc *__restrict self,
                          __ref struct kshmregion *__restrict region,
                          ktls_addr_t *__restrict offset) {
 struct ktlsmapping *mapping;
 struct ktask **iter,**end,*task;
 struct ktask *calling_task;
 __user __pagealigned void *block_address;
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kproc(self);
 kassert_kshmregion(region);
 kassertobj(offset);
 mapping = omalloc(struct ktlsmapping);
 if __unlikely(!mapping) return KE_NOMEM;
 /* Fill out some initial information in the mapping, such as alignment and size. */
 mapping->tm_region = region; /* Inherit reference. */
 mapping->tm_pages  = region->sre_chunk.sc_pages;
 assert(mapping->tm_pages);
 error = krwlock_beginwrite(&self->p_tls.tls_lock);
 if __unlikely(KE_ISERR(error)) goto err_mapping;
 ktlsman_insmapping_inherited(&self->p_tls,mapping);
 /* At this point, we only need read-access.
  * >> So downgrade the TLS lock to read-only. */
 krwlock_downgrade(&self->p_tls.tls_lock);
 /* Acquire a lock to the process's thread-list. */
 error = kproc_lock(self,KPROC_LOCK_THREADS);
 if __unlikely(KE_ISERR(error)) goto err_unlink;
 end = (iter = self->p_threads.t_taskv)+self->p_threads.t_taska;
 if (iter != end) {
  /* Must map the new thread-local region in existing threads.
   * >> Therefor we must also acquire a lock to the process's SHM.
   * WARNING: Careful: At this point we're already holding 3 locks (TLS,THREAD,SHM).
   *                 - So make sure not to cause a deadlock here! */
  calling_task = ktask_self();
  error = krwlock_beginwrite(&self->p_shm.s_lock);
  if __unlikely(KE_ISERR(error)) goto err_threads;
  for (; iter != end; ++iter) if ((task = *iter) != NULL && ktask_isusertask(task)) {
   kassert_ktlspt(&task->t_tls);
   /* Map the region in all threads already existing. */
   block_address = (__user void *)((uintptr_t)task->t_tls.pt_uthread+
                                    mapping->tm_offset);
   assertf(isaligned((uintptr_t)block_address,PAGEALIGN)
          ,"addr   = %p\n"
           "offset = %Id\n"
          ,block_address
          ,mapping->tm_offset);
   /* Check for the simple case: When taking the existing thread-local block,
    *                            we don't have to relocate anything if the
    *                            pages used by the block are unused. */
   if (kpagedir_isunmapped(self->p_shm.s_pd,block_address,mapping->tm_pages)) {
    error = kshm_mapregion_unlocked(&self->p_shm,block_address,
                                    region,0,mapping->tm_pages);
    if __unlikely(KE_ISERR(error)) {
     if (error == KE_EXISTS) goto relocate_tls;
     goto err_threads_iter;
    }
   } else {
    __user void *newbase;
    __pagealigned __user struct kuthread *new_uthread;
relocate_tls:
    /* Must relocate the entire TLS vector. */
    /* Step #1: Find a new address range of sufficient size. */
    newbase = kpagedir_findfreerange(self->p_shm.s_pd,mapping->tm_pages,
                                     KPAGEDIR_MAPANY_HINT_TLS);
    if __unlikely(newbase == KPAGEDIR_FINDFREERANGE_ERR) {
     error = KE_NOSPC;
     goto err_threads_iter;
    }
    new_uthread = (__pagealigned __user struct kuthread *)((uintptr_t)newbase+
                                                           self->p_tls.tls_map_pages*PAGESIZE);
    /* Step #2: Suspend the thread to make sure it's not attempting to
     *          access existing TLS data while we're relocating its vector.
     * NOTE: Make sure not to suspend ourselves! */
    if (task != calling_task) {
     if __unlikely(KE_ISERR(error = ktask_suspend_k(task))) {
      /* The task was terminated, but due to race
       * conditions hasn't finished cleanup yet (Just skip it). */
      if (error == KE_DESTROYED) goto next_task;
      /* Handle other errors by undoing everything. */
      goto err_threads_iter;
     }
    }
    /* Step #3: Remap the new TLS vector. */
    error = kproc_remap_pt(self,task,new_uthread);
    if __likely(KE_ISOK(error)) {
     struct ksegment new_segment;
     __pagealigned __kernel struct kuthread *phys_newbase;
     /* Step #4: Unmap the old TLS vector. */
     kproc_unmap_pt(self,task,task->t_tls.pt_uthread);
     /* Step #5: Install a new LDT segment and update the TLS pointers. */
     kassert_kshmregion(task->t_tls.pt_uregion);
     assert(task->t_tls.pt_uregion->sre_chunk.sc_pages);
     assert(task->t_tls.pt_uregion->sre_chunk.sc_partc);
     assert(task->t_tls.pt_uregion->sre_chunk.sc_partv[0].sp_start == 0);
     assert(task->t_tls.pt_uregion->sre_chunk.sc_partv[0].sp_pages);
     { /* Update the uthread self-pointer. */
      assert(offsetafter(struct kuthread,u_parent) <= PAGESIZE);
      phys_newbase = (__pagealigned __kernel struct kuthread *)
       task->t_tls.pt_uregion->sre_chunk.sc_partv[0].sp_frame;
      assert(isaligned((uintptr_t)phys_newbase,PAGEALIGN));
      phys_newbase->u_self   = new_uthread;
      { /* Update the parent-pointer. */
       struct ktask *par = task->t_parent;
       if (ktask_isusertask(par) && par->t_proc == self) {
        phys_newbase->u_parent = par->t_tls.pt_uthread;
       }
      }
      task->t_tls.pt_uthread = new_uthread;
     }
     /* Encode and setup the new TLS segment. */
     ksegment_encode(&new_segment,(uintptr_t)new_uthread,
                     KUTHREAD_PAGESIZE*PAGESIZE,SEG_DATA_PL3);
     kshm_ldtset_unlocked(&self->p_shm,task->t_tls.pt_segid,&new_segment);
    }
    /* Resume execution of the task */
    if (task != calling_task) ktask_resume_k(task);
    if __unlikely(KE_ISERR(error)) goto err_threads_iter;
   }
next_task:;
  }
  krwlock_endwrite(&self->p_shm.s_lock);
 }
 kproc_unlock(self,KPROC_LOCK_THREADS);
 *offset = mapping->tm_offset;
 krwlock_endread(&self->p_tls.tls_lock);
 return KE_OK;
err_threads_iter:
 /* Unmap the region in all threads it's already been mapped to. */
 while (iter != self->p_threads.t_taskv) {
  task = *--iter;
  if (task && ktask_isusertask(task)) {
   block_address = (__user void *)((uintptr_t)task->t_tls.pt_uthread+
                                    mapping->tm_offset);
   assert(isaligned((uintptr_t)block_address,PAGEALIGN));
   kshm_unmapregion_unlocked(&self->p_shm,block_address,region);
  }
 }
err_threads: kproc_unlock(self,KPROC_LOCK_THREADS);
err_unlink:  ktlsman_delmapping(&self->p_tls,mapping);
             krwlock_endread(&self->p_tls.tls_lock);
err_mapping: free(mapping);
 return error;
}


__crit kerrno_t
kproc_tls_alloc(struct kproc *__restrict self,
                struct kshmregion *__restrict region,
                ktls_addr_t *__restrict offset) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kproc(self);
 kassert_kshmregion(region);
 error = kshmregion_incref_full(region);
 if __unlikely(KE_ISERR(error)) return error;
 error = kproc_tls_alloc_inherited(self,region,offset);
 if __unlikely(KE_ISERR(error)) kshmregion_decref_full(region);
 return error;
}

static __crit kerrno_t
kproc_tls_free_mapping(struct kproc *__restrict self,
                       struct ktlsmapping **__restrict pmapping) {
 struct ktlsmapping *mapping;
 struct ktask **iter,**end,*task;
 kerrno_t error;
 ktls_addr_t map_offset;
 kassert_kproc(self);
 kassert_ktlsman(&self->p_tls);
 kassertobj(pmapping);
 kassertobj(*pmapping);
 assert(krwlock_iswritelocked(&self->p_tls.tls_lock));
 mapping = *pmapping;
 kassert_kshmregion(mapping->tm_region);
 assert(mapping->tm_pages);
 assert(mapping->tm_pages == mapping->tm_region->sre_chunk.sc_pages);
 assert(mapping->tm_region->sre_chunk.sc_partc);
 error = kproc_lock(self,KPROC_LOCK_THREADS);
 if __unlikely(KE_ISERR(error)) return error;
 end = (iter = self->p_threads.t_taskv)+self->p_threads.t_taska;
 if __likely(iter != end) { /*< likely because it's probably that process that's calling us. */
  /* Go through all running threads and delete thread-local SHM mappings. */
  map_offset = mapping->tm_offset;
  error = krwlock_beginwrite(&self->p_shm.s_lock);
  if __unlikely(KE_ISERR(error)) goto end_threads;
  for (; iter != end; ++iter) if ((task = *iter) != NULL && ktask_isusertask(task)) {
   __user void *umap_addr; kerrno_t unmap_error;
   kassert_ktlspt(&task->t_tls);
   umap_addr = (__user void *)((uintptr_t)task->t_tls.pt_uthread+map_offset);
   /* Unmap the region from this thread (Make sure we're only unmapping that exact region!). */
   unmap_error = kshm_unmapregion_unlocked(&self->p_shm,umap_addr,mapping->tm_region);
   if __unlikely(KE_ISERR(unmap_error)) {
    /* This can still happen if the user manually munmap'ed the region before. */
    k_syslogf(KLOG_WARN,"[TLS] Failed to unmap TLS region at %p (%Iu pages): %d\n",
              umap_addr,mapping->tm_pages,unmap_error);
   }
  }
  krwlock_endwrite(&self->p_shm.s_lock);
 }
 kproc_unlock(self,KPROC_LOCK_THREADS);
 kshmregion_decref_full(mapping->tm_region);
 free(mapping);
 *pmapping = mapping->tm_prev;
 return error;
end_threads:
 kproc_unlock(self,KPROC_LOCK_THREADS);
 return error;
}

__crit kerrno_t
kproc_tls_free_region(struct kproc *__restrict self,
                      struct kshmregion *__restrict region) {
 kerrno_t error;
 struct ktlsmapping **piter,*iter;
 kassert_kproc(self);
 kassert_kshmregion(region);
 error = krwlock_beginwrite(&self->p_tls.tls_lock);
 if __unlikely(KE_ISERR(error)) return error;
 piter = &self->p_tls.tls_hiend;
 while ((iter = *piter) != NULL) {
  if (iter->tm_region == region) {
   error = kproc_tls_free_mapping(self,piter);
   if __likely(KE_ISOK(error) && !*piter) {
    /* The last mapping was deleted. */
    if (piter == &self->p_tls.tls_hiend) {
     self->p_tls.tls_map_pages = 0;
     self->p_tls.tls_all_pages = KUTHREAD_PAGESIZE;
    } else {
     /* Figure out which mapping is now the last one.
      * Then, using that information, calculate the new TLS
      * region size using the greatest, absolute offset. */
     iter = (struct ktlsmapping *)((uintptr_t)piter-offsetof(struct ktlsmapping,tm_prev));
     self->p_tls.tls_map_pages = (size_t)(-iter->tm_offset)/PAGESIZE;
     self->p_tls.tls_all_pages = self->p_tls.tls_map_pages+KUTHREAD_PAGESIZE;
    }
   }
   goto done;
  }
  piter = &iter->tm_prev;
 }
 error = KE_NOENT;
done:
 krwlock_endwrite(&self->p_tls.tls_lock);
 return error;
}

__crit kerrno_t
kproc_tls_free_offset(struct kproc *__restrict self,
                      ktls_addr_t offset) {
 kerrno_t error;
 struct ktlsmapping **piter,*iter;
 kassert_kproc(self);
 error = krwlock_beginwrite(&self->p_tls.tls_lock);
 if __unlikely(KE_ISERR(error)) return error;
 piter = &self->p_tls.tls_hiend;
 while ((iter = *piter) != NULL) {
  if (iter->tm_offset == offset) {
   error = kproc_tls_free_mapping(self,piter);
   if __likely(KE_ISOK(error) && !*piter) {
    /* The last mapping was deleted. */
    if (piter == &self->p_tls.tls_hiend) {
     self->p_tls.tls_map_pages = 0;
     self->p_tls.tls_all_pages = KUTHREAD_PAGESIZE;
    } else {
     /* Figure out which mapping is now the last one.
      * Then, using that information, calculate the new TLS
      * region size using the greatest, absolute offset. */
     iter = (struct ktlsmapping *)((uintptr_t)piter-offsetof(struct ktlsmapping,tm_prev));
     self->p_tls.tls_map_pages = (size_t)(-iter->tm_offset)/PAGESIZE;
     self->p_tls.tls_all_pages = self->p_tls.tls_map_pages+KUTHREAD_PAGESIZE;
    }
   }
   goto done;
  }
  piter = &iter->tm_prev;
 }
 error = KE_NOENT;
done:
 krwlock_endwrite(&self->p_tls.tls_lock);
 return error;
}





//////////////////////////////////////////////////////////////////////////
// === ktlspt ===
__crit kerrno_t
kproc_tls_alloc_pt_unlocked(struct kproc *__restrict self,
                            struct ktask *__restrict task) {
 __pagealigned __user struct kuthread *user_uthread;
 __pagealigned __user void *baseaddr;
 kerrno_t error;
 struct ksegment tls_segment;
 kassert_kproc(self);
 kassert_ktask(task);
 assert(task->t_proc == self);
 assert(krwlock_iswritelocked(&self->p_shm.s_lock));
 assert(krwlock_iswritelocked(&self->p_tls.tls_lock) ||
        krwlock_isreadlocked(&self->p_tls.tls_lock));
 assert(KUTHREAD_PAGESIZE == ceildiv(sizeof(struct kuthread),PAGESIZE));
 assertf(self->p_tls.tls_map_pages ==
         self->p_tls.tls_all_pages-KUTHREAD_PAGESIZE
        ,"self->p_tls.tls_map_pages = %Iu\n"
         "self->p_tls.tls_all_pages = %Iu\n"
        ,self->p_tls.tls_map_pages
        ,self->p_tls.tls_all_pages);
 kobject_init(&task->t_tls,KOBJECT_MAGIC_TLSPT);
 task->t_tls.pt_uregion = kshmregion_newram(KUTHREAD_PAGESIZE,
                                            KTLS_UREGION_FLAGS);
 if __unlikely(!task->t_tls.pt_uregion) return KE_NOMEM;
 /* default-initialize the region with ZEROes. */
 kshmregion_memset(task->t_tls.pt_uregion,0);
 /* Figure out where we should map the TLS block to. */
 baseaddr = kpagedir_findfreerange(self->p_shm.s_pd,self->p_tls.tls_all_pages,
                                   KPAGEDIR_MAPANY_HINT_TLS);
 if __unlikely(baseaddr == KPAGEDIR_FINDFREERANGE_ERR) { error = KE_NOSPC; goto err_region; }
 user_uthread = (__pagealigned __user struct kuthread *)((uintptr_t)baseaddr+
                                                          self->p_tls.tls_map_pages*PAGESIZE);
 assert(isaligned((uintptr_t)baseaddr,PAGEALIGN));
 assert(isaligned((uintptr_t)user_uthread,PAGEALIGN));
 task->t_tls.pt_uthread = user_uthread;
 /* Map the PT block at the address we've just figured out. */
 error = kproc_remap_pt(self,task,user_uthread);
 if __unlikely(KE_ISERR(error)) goto err_region;
 /* OK! The TLS block is mapped just how it needs to be. - Time to create the TLS segment. */
 ksegment_encode(&tls_segment,(uintptr_t)user_uthread,
                 KUTHREAD_PAGESIZE*PAGESIZE,SEG_DATA_PL3);
 /* Now simply allocate a LDT segment. */
 task->t_tls.pt_segid = kshm_ldtalloc_unlocked(&self->p_shm,&tls_segment);
 if __unlikely(task->t_tls.pt_segid == KSEG_NULL) { error = KE_NOMEM; goto err_unmappt; }
 return error;
err_unmappt: kproc_unmap_pt(self,task,user_uthread);
err_region:  kshmregion_decref_full(task->t_tls.pt_uregion);
 return error;
}


__crit void
kproc_tls_free_pt_unlocked(struct kproc *__restrict self,
                           struct ktask *__restrict task) {
 kassert_kproc(self);
 kassert_object(task,KOBJECT_MAGIC_TASK);
 kassert_ktlspt(&task->t_tls);
 assert(task->t_proc == self);
 assert(krwlock_iswritelocked(&self->p_shm.s_lock));
 /* Free the LDT TLS-entry. */
 kshm_ldtfree_unlocked(&self->p_shm,task->t_tls.pt_segid);
 /* Unmap and free the TLS mapping. */
 kproc_unmap_pt(self,task,task->t_tls.pt_uthread);
 kshmregion_decref_full(task->t_tls.pt_uregion);
 kobject_badmagic(&task->t_tls);
}

__crit void
ktlspt_copyuthread_unlocked(struct ktlspt *__restrict self,
                            struct ktlspt const *__restrict right) {
 size_t max_dst,max_src,bytes;
 __kernel void *kdst,*ksrc;
 kshmregion_addr_t dst,src;
 kassert_ktlspt(self);
 kassert_ktlspt(right);
 kassert_kshmregion(self ->pt_uregion);
 kassert_kshmregion(right->pt_uregion);
 assert(self ->pt_uregion->sre_chunk.sc_pages == KUTHREAD_PAGESIZE);
 assert(right->pt_uregion->sre_chunk.sc_pages == KUTHREAD_PAGESIZE);
 dst = src = 0,bytes = sizeof(struct kuthread);
 while ((kdst = kshmregion_translate_fast(self ->pt_uregion,dst,&max_dst)) != NULL &&
        (ksrc = kshmregion_translate_fast(right->pt_uregion,src,&max_src)) != NULL) {
  if (bytes   < max_src) max_src = bytes;
  if (max_dst < max_src) max_src = max_dst;
  memcpy(kdst,ksrc,max_src);
  if ((bytes -= max_src) == 0) break;
  dst += max_src;
  src += max_src;
 }
}


__crit void
kproc_tls_pt_setup(struct kproc *__restrict self,
                   struct ktask *__restrict task) {
 struct kuthread *uthread;
 struct ktask *parent;
 kassert_kproc(self);
 kassert_ktask(task);
 assert(krwlock_isreadlocked(&self->p_shm.s_lock) ||
        krwlock_iswritelocked(&self->p_shm.s_lock));
 assertf(ktask_isusertask(task),"Kernel tasks can't be used for TLS");
 kassert_ktlspt(&task->t_tls);
 kassert_kshmregion(task->t_tls.pt_uregion);
 assert(task->t_tls.pt_uregion->sre_chunk.sc_pages);
 assert(task->t_tls.pt_uregion->sre_chunk.sc_partc);
 assert(task->t_tls.pt_uregion->sre_chunk.sc_partv[0].sp_start == 0);
 assert(task->t_tls.pt_uregion->sre_chunk.sc_partv[0].sp_pages);
#if 1
 assert(sizeof(struct kuthread) <= PAGESIZE);
 uthread = (struct kuthread *)task->t_tls.pt_uregion->sre_chunk.sc_partv[0].sp_frame;
#else
 {
  size_t maxbytes;
  uthread = (struct kuthread *)kshm_translateuser(&self->p_shm,self->p_shm.s_pd,task->t_tls.pt_uthread,
                                                  sizeof(struct kuthread),&maxbytes,0);
  if __unlikely(!uthread || maxbytes < sizeof(struct kuthread)) {
   k_syslogf(KLOG_WARN,"[TLS] Failed to translate TLS uthread for task %p:%I32d:%Iu:%s\n",
             task,task->t_proc->p_pid,task->t_tid,ktask_getname(task));
   return;
  }
 }
#endif
 /* Fill in all kinds of useful information. */
 parent = task->t_parent;
 assertf(parent != NULL,"Must be non-NULL");
 assertf(parent != task,"Only ktask_zero() (which is a kernel task) would apply for this");
 if (parent->t_proc == self && ktask_isusertask(parent)) {
  /* Parent thread is part of the same process. */
  uthread->u_parent = parent->t_tls.pt_uthread;
 }
 uthread->u_self       = task->t_tls.pt_uthread;
 uthread->u_stackbegin = task->t_ustackvp;
 uthread->u_stackend   = (__user void *)((uintptr_t)task->t_ustackvp+task->t_ustacksz);
 uthread->u_parid      = task->t_parid;
 uthread->u_tid        = task->t_tid;
 uthread->u_pid        = task->t_proc->p_pid;
#if KCONFIG_HAVE_TASK_STATS_START
 memcpy(&uthread->u_start,&task->t_stats.ts_abstime_start,sizeof(struct timespec));
#else
 ktime_getnow(&uthread->u_start);
#endif
}

void
ktlspt_setname(struct ktlspt *__restrict self,
               char const *__restrict name) {
 struct kuthread *uthread;
 assert(offsetof(struct kuthread,u_zero) <= PAGESIZE);
 kassert_ktlspt(self);
 kassert_kshmregion(self->pt_uregion);
 assert(self->pt_uregion->sre_chunk.sc_pages);
 assert(self->pt_uregion->sre_chunk.sc_partc);
 assert(self->pt_uregion->sre_chunk.sc_partv[0].sp_start == 0);
 assert(self->pt_uregion->sre_chunk.sc_partv[0].sp_pages);
 uthread = (struct kuthread *)self->pt_uregion->sre_chunk.sc_partv[0].sp_frame;
 strncpy(uthread->u_name,name,KUTHREAD_NAMEMAX);
}


static __crit __user void *
kproc_get_next_ueh_bymodule(struct kproc *__restrict self, kmodid_t modid) {
 static char const *ueh_names[] = {
  /* Various spellings from difference linkers/languages. */
  "__kos_unhandled_exception",
  "___kos_unhandled_exception",
  "__impl____kos_unhandled_exception",NULL};
 char const *name,**iter; struct kshlib *lib;
 struct kprocmodule *module; struct ksymbol const *symbol;
 assert(modid < self->p_modules.pms_moda);
 module = &self->p_modules.pms_modv[modid];
 lib = module->pm_lib;
 kassert_kshlib(lib);
 for (iter = ueh_names; (name = *iter) != NULL; ++iter) {
  size_t name_size = strlen(name);
  size_t name_hash = ksymhash_of(name,name_size);
  /* Search for a symbol in the public and weak hashtable.
   * NOTE: Don't search through private symbols
   *   >> Those are likely to only be available in debug builds
   *      of the application, as well as being restricted to ELF. */
               symbol = ksymtable_lookupname_h(&lib->sh_publicsym,name,name_size,name_hash);
  if (!symbol) symbol = ksymtable_lookupname_h(&lib->sh_weaksym,name,name_size,name_hash);
  if (symbol && symbol->s_shndx != SHN_UNDEF) {
   return (__user void *)((uintptr_t)module->pm_base+symbol->s_addr);
  }
 }
 return NULL;
}

static __crit __user void *
kproc_get_next_ueh_unlocked(struct kproc *__restrict self, __user void *caller_eip) {
#define TRY_SEARCH(id) \
 (!(katomic_fetchor(self->p_modules.pms_modv[id].pm_flags,\
                    KPROCMODULE_FLAG_UEH_CALLED)&\
                    KPROCMODULE_FLAG_UEH_CALLED))
 kmodid_t first_id,modid,maxid; __user void *result;
 if __unlikely(KE_ISERR(kproc_getmodat_unlocked(self,&first_id,caller_eip))) first_id = 0;
 maxid = self->p_modules.pms_moda;
 if (TRY_SEARCH(first_id)) {
  result = kproc_get_next_ueh_bymodule(self,first_id);
  if (result != NULL) return result;
 }

 /* Search above. */
 for (modid = first_id+1; modid < maxid; ++modid) if (TRY_SEARCH(modid)) {
  result = kproc_get_next_ueh_bymodule(self,modid);
  if (result != NULL) return result;
 }
 /* Search below. */
 if (first_id != 0) {
  modid = first_id-1;
  for (;;) {
   if (TRY_SEARCH(modid)) {
    result = kproc_get_next_ueh_bymodule(self,modid);
    if (result != NULL) return result;
   }
   if (!modid) break;
   --modid;
  }
 }
#undef TRY_SEARCH
 return NULL;
}

static __user void *
kproc_get_next_ueh(struct kproc *__restrict self, __user void *caller_eip) {
 void *result;
 KTASK_CRIT_BEGIN
 KTASK_NOINTR_BEGIN
 if __likely(KE_ISOK(kproc_lock(self,KPROC_LOCK_MODS))) {
  result = kproc_get_next_ueh_unlocked(self,caller_eip);
  kproc_unlock(self,KPROC_LOCK_MODS);
 } else {
  result = NULL;
 }
 KTASK_NOINTR_END
 KTASK_CRIT_END
 return result;
}

kerrno_t
ktask_raise_exception(struct ktask *__restrict self,
                      struct kirq_registers *__restrict regs,
                      struct kexinfo const *__restrict exinfo) {
 __kernel struct kuthread *uthread;
 struct ktranslator trans; size_t copy_error;
 struct kexrecord exrecord; kerrno_t error;
 __user struct kexrecord *exaddr;
 kassert_ktask(self);
 assert(ktask_isusertask(self));
 kassertobj(regs);
 kassertobj(exinfo);
 kassert_ktlspt(&self->t_tls);
 kassert_kshmregion(self->t_tls.pt_uregion);
 assertf(exinfo->ex_no != 0,"Invalid exception number");
 assert(self->t_tls.pt_uregion->sre_chunk.sc_pages);
 assert(self->t_tls.pt_uregion->sre_chunk.sc_partc);
 assert(self->t_tls.pt_uregion->sre_chunk.sc_partv[0].sp_start == 0);
 assert(self->t_tls.pt_uregion->sre_chunk.sc_partv[0].sp_pages);
 uthread = (struct kuthread *)self->t_tls.pt_uregion->sre_chunk.sc_partv[0].sp_frame;
 KTASK_CRIT_BEGIN
 if __likely(KE_ISOK(error = ktranslator_init(&trans,self))) {
  /* Make sure this is only read once!
   * (Because it's also used as the new ESP upon return to userspace). */
  exaddr = uthread->u_exh;
  __asm_volatile__("mfence\n" : : : "memory");
  copy_error = ktranslator_copyfromuser(&trans,&exrecord,exaddr,
                                        sizeof(struct kexrecord));
  ktranslator_quit(&trans);
 }
 KTASK_CRIT_END
 if __unlikely(KE_ISERR(error)) return error;
 /* If the copy failed, check why it did. */
 if __unlikely(copy_error) {
  void *default_exception_handler;
  /* Search for an unhandled exception handler. */
  default_exception_handler = kproc_get_next_ueh(ktask_getproc(self),
                                                (__user void *)regs->regs.eip);
  /* If no handler was found, return an error code. */
  if (!default_exception_handler) return exaddr ? KE_FAULT : KE_NOENT;
  exrecord.eh_handler = (kexhandler_t)default_exception_handler;
  exaddr = NULL;
 } else {
  /* Update the exception handler chain. */
  uthread->u_exh = exrecord.eh_prev;
 }
 /* Fill in user-accessible exception state information. */
 uthread->u_exstate.edi    = regs->regs.edi;
 uthread->u_exstate.esi    = regs->regs.esi;
 uthread->u_exstate.ebp    = regs->regs.ebp;
 uthread->u_exstate.esp    = regs->regs.useresp;
 uthread->u_exstate.ebx    = regs->regs.ebx;
 uthread->u_exstate.edx    = regs->regs.edx;
 uthread->u_exstate.ecx    = regs->regs.ecx;
 uthread->u_exstate.eax    = regs->regs.eax;
 uthread->u_exstate.eip    = regs->regs.eip;
 uthread->u_exstate.eflags = regs->regs.eflags;
 memcpy(&uthread->u_exinfo,exinfo,sizeof(struct kexinfo));
 /* Update the given registers to mirror the used handler. */
 if (exaddr) {
  regs->regs.useresp = (uintptr_t)(__user void *)(exaddr+1);
 }
 regs->regs.eip = (uintptr_t)(__user void *)exrecord.eh_handler;
 return error;
}


#define IRQ_EXCEPTION_EXIT_CODE(int_no) \
 (void *)(((__uintptr_t)1 << ((__SIZEOF_POINTER__*8)-4))+(__uintptr_t)(int_no))

void
ktask_unhandled_irq_exception(struct ktask *__restrict self,
                              struct kirq_registers const *__restrict regs) {
 static char const *fmt = "Terminating process %I32d with %p after failure to handle IRQ exception %I32d\n";
 struct kproc *proc;
 struct ktask *root_task;
 void *exitcode;
 __ref struct kshlib *root_exe;
 KTASK_CRIT_BEGIN
 kassert_ktask(self);
 kassertobj(regs);
 proc = ktask_getproc(self);
 /* Special exitcode containing the INT number. */
 exitcode = IRQ_EXCEPTION_EXIT_CODE(regs->regs.intno);
 /* Try to log at least something about what happened. */
 if ((root_exe = kproc_getrootexe(proc)) != NULL) {
  k_syslogf_prefixfile(KLOG_ERROR,root_exe->sh_file,fmt,
                       proc->p_pid,exitcode,regs->regs.intno);
  kshlib_decref(root_exe);
 } else {
  k_syslogf(KLOG_ERROR,fmt,proc->p_pid,exitcode,regs->regs.intno);
 }
 proc = ktask_getproc(self);
 root_task = kproc_getroottask(proc);
 if __unlikely(!root_task) {
  ktask_terminateex_k(self,exitcode,KTASKOPFLAG_RECURSIVE);
 } else {
  ktask_terminateex_k(root_task,exitcode,KTASKOPFLAG_RECURSIVE);
  ktask_decref(root_task);
 }
 /* The following line will not return if 'self == ktask_self()'. */
 KTASK_CRIT_END
}


kerrno_t
ktask_raise_irq_exception(struct ktask *__restrict self,
                          struct kirq_registers *__restrict regs) {
 struct kexinfo exinfo;
 memset(&exinfo,0,sizeof(struct kexinfo));
 switch (regs->regs.intno) {
#ifdef __i386__
  case KARCH_X64_IRQ_DE:
  case KARCH_X64_IRQ_OF:
  case KARCH_X64_IRQ_BR:
  case KARCH_X64_IRQ_UD:
  case KARCH_X64_IRQ_GP:
   exinfo.ex_no = KEXCEPTION_X86_IRQ(regs->regs.intno);
   break;
  case KARCH_X64_IRQ_PF:
   exinfo.ex_no = KEXCEPTION_X86_IRQ(KARCH_X64_IRQ_PF);
   exinfo.ex_info = regs->regs.ecode&0x00000012; /* write+instr_fetch. */
   exinfo.ex_ptr[0] = x86_getcr2();
   if (regs->regs.ecode&0x00000001) { /* Present. */
    struct kshmbranch *branch;
    struct kproc *proc = ktask_getproc(self);
    /* Figure out if the branch is really present in userspace. */
    if __likely(KE_ISOK(krwlock_beginread(&proc->p_shm.s_lock))) {
     branch = kshmbranch_locate(proc->p_shm.s_map.m_root,
                               (uintptr_t)exinfo.ex_ptr[0]);
     if (branch) {
      kshm_flag_t flags = branch->sb_region->sre_chunk.sc_flags;
      if (!(flags&KSHMREGION_FLAG_RESTRICTED) ||
           (flags&(KSHMREGION_FLAG_READ|KSHMREGION_FLAG_WRITE))
          ) exinfo.ex_info |= 1; /* Present. */
     }
     krwlock_endread(&proc->p_shm.s_lock);
    }
   }
   break;
#endif
  default: return KE_INVAL;
 }
 return ktask_raise_exception(self,regs,&exinfo);
}



__DECL_END


#include <kos/kernel/syscall.h>
__DECL_BEGIN

KSYSCALL_DEFINE_EX2(cr,kerrno_t,kproc_tlsalloc,
                    __user void const *,template_,
                    size_t,template_size) {
 struct ktranslator trans; kerrno_t error;
 struct kshmregion *region;
 struct ktask *caller = ktask_self();
 size_t template_pages;
 ktls_addr_t resoffset;
 KTASK_CRIT_MARK
 if __unlikely(!template_size) template_size = 1;
 template_pages = ceildiv(template_size,PAGESIZE);
 assert(template_pages);
 /* TODO: Limit the template region size to what will be
  *       allowed based on the process's memory limits. */
 region = kshmregion_newram(template_pages,
                            KSHMREGION_FLAG_READ|
                            KSHMREGION_FLAG_WRITE|
                            KSHMREGION_FLAG_LOSEONFORK);
 if __unlikely(!region) return KE_NOMEM;
 /* Initialize the TLS region with the given template, or fill it with ZEROes. */
 if (template_) {
  size_t max_dst,max_src;
  kshmregion_addr_t region_address = 0;
  __kernel void *kdst,*ksrc;
  error = ktranslator_init(&trans,caller);
  if __unlikely(KE_ISERR(error)) goto err_region;
  while ((kdst = kshmregion_translate_fast(region,region_address,&max_dst)) != NULL &&
         (ksrc = ktranslator_exec(&trans,template_,min(max_dst,template_size),&max_src,0)) != NULL) {
   memcpy(kdst,ksrc,max_src);
   if ((template_size -= max_src) == 0) break;
   region_address           += max_src;
   *(uintptr_t *)&template_ += max_src;
  }
  ktranslator_quit(&trans);
  /* Make sure the entire template got copied. */
  if __unlikely(template_size) { error = KE_FAULT; goto err_region; }
 } else {
  kshmregion_memset(region,0);
 }
 /* Allocate a new TLS block using the new region as template. */
 error = kproc_tls_alloc_inherited(ktask_getproc(caller),
                                   region,&resoffset);
 if __unlikely(KE_ISERR(error)) goto err_region;
 /* Store the actual offset in ECX. */
 regs->regs.ecx = (uintptr_t)resoffset;
 return error;
err_region:
 kshmregion_decref_full(region);
 return error;
}
KSYSCALL_DEFINE_EX1(c,kerrno_t,kproc_tlsfree,
                    ptrdiff_t,tls_offset) {
 KTASK_CRIT_MARK
 return kproc_tls_free_offset(kproc_self(),tls_offset);
}
KSYSCALL_DEFINE_EX2(cr,kerrno_t,kproc_tlsenum,
                    __user struct ktlsinfo *,infov,
                    size_t,infoc) {
 kerrno_t error; struct ktranslator trans;
 __user struct ktlsinfo *infov_iter,*infov_end;
 struct ktlsmapping *iter;
 struct ktask *caller = ktask_self();
 struct kproc *proc = ktask_getproc(caller);
 KTASK_CRIT_MARK
 error = krwlock_beginread(&proc->p_tls.tls_lock);
 if __unlikely(KE_ISERR(error)) return error;
 error = ktranslator_init(&trans,caller);
 if __unlikely(KE_ISERR(error)) goto end_tls;
 iter = proc->p_tls.tls_hiend;
 infov_end = (infov_iter = infov)+infoc;
 while (iter) {
  if (infov_iter < infov_end) {
   struct ktlsinfo info;
   info.ti_offset = iter->tm_offset;
   info.ti_size   = iter->tm_pages*PAGESIZE;
   if __unlikely(ktranslator_copytouser(&trans,infov_iter,
                                        &info,sizeof(info))
                 ) { error = KE_FAULT; goto end_trans; }
  }
  ++infov_iter;
  iter = iter->tm_prev;
 }
 regs->regs.ecx = (size_t)(infov_iter-infov);
end_trans: ktranslator_quit(&trans);
end_tls:   krwlock_endread(&proc->p_tls.tls_lock);
 return error;
}

__DECL_END

#endif /* !__KOS_KERNEL_SCHED_TLS_C__ */

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
#include <kos/kernel/tls.h>
#include <kos/kernel/tls-perthread.h>
#include <kos/kernel/task.h>
#include <kos/kernel/shm.h>
#include <kos/kernel/proc.h>
#include <strings.h>
#include <malloc.h>
#include <kos/syslog.h>
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
ktlsman_initcopy(struct ktlsman *__restrict self,
                 struct ktlsman const *__restrict right) {
 struct ktlsmapping **pdst,*iter,*copy;
 kerrno_t error;
 kassertobj(self);
 kassert_ktlsman(right);
 kobject_init(self,KOBJECT_MAGIC_TLSMAN);
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
     assert(task->t_tls.pt_uregion->sre_chunk.sc_partv[0].sp_pages);
     {
      /* Update the uthread self-pointer. */
      __STATIC_ASSERT(sizeof(phys_newbase->u_self) <= PAGESIZE);
      phys_newbase = (__pagealigned __kernel struct kuthread *)
       task->t_tls.pt_uregion->sre_chunk.sc_partv[0].sp_frame;
      assert(isaligned((uintptr_t)phys_newbase,PAGEALIGN));
      phys_newbase->u_self   = new_uthread;
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

__crit kerrno_t
kproc_tls_free_region(struct kproc *__restrict self,
                      struct kshmregion *__restrict region) {
 kassert_kproc(self);
 kassert_kshmregion(region);

 /* TODO */

 return KE_OK;
}

__crit kerrno_t
kproc_tls_free_offset(struct kproc *__restrict self,
                      ktls_addr_t offset) {
 kassert_kproc(self);

 /* TODO */

 return KE_OK;
}





//////////////////////////////////////////////////////////////////////////
// === ktlspt ===
__crit kerrno_t
kproc_tls_alloc_pt_unlocked(struct kproc *__restrict self,
                            struct ktask *__restrict task) {
 __pagealigned __kernel struct kuthread *kernel_uthread;
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
 /* Setup the uthread self-pointer. */
 kernel_uthread = (__pagealigned __kernel struct kuthread *)
  task->t_tls.pt_uregion->sre_chunk.sc_partv[0].sp_frame;
 assert(isaligned((uintptr_t)kernel_uthread,PAGEALIGN));
 kernel_uthread->u_self = user_uthread;
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
 size_t maxbytes;
 kassert_kproc(self);
 kassert_ktask(task);
 assert(krwlock_isreadlocked(&self->p_shm.s_lock) ||
        krwlock_iswritelocked(&self->p_shm.s_lock));
 assertf(ktask_isusertask(task),"Kernel tasks can't be used for TLS");
 uthread = (struct kuthread *)kshm_translateuser(&self->p_shm,self->p_shm.s_pd,task->t_tls.pt_uthread,
                                                 sizeof(struct kuthread),&maxbytes,0);
 if __unlikely(!uthread || maxbytes < sizeof(struct kuthread)) {
  k_syslogf(KLOG_WARN,"[TLS] Failed to translate TLS uthread for task %p:%I32d:%Iu:%s\n",
            task,task->t_proc->p_pid,task->t_tid,ktask_getname(task));
  return;
 }
 parent = task->t_parent;
 assertf(parent != NULL,"Must be non-NULL");
 assertf(parent != task,"Only ktask_zero() (which is a kernel task) would apply for this");
 if (parent->t_proc == self && ktask_isusertask(parent)) {
  /* Parent thread is part of the same process. */
  uthread->u_parent = parent->t_tls.pt_uthread;
 }
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



__DECL_END

#endif /* !__KOS_KERNEL_SCHED_TLS_C__ */

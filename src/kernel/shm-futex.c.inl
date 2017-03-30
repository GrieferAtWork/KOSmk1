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
#ifndef __KOS_KERNEL_SHM_FUTEX_C_INL__
#define __KOS_KERNEL_SHM_FUTEX_C_INL__ 1

#include <kos/compiler.h>
#include <kos/atomic.h>
#include <kos/futex.h>
#include <kos/kernel/shm.h>
#include <kos/syslog.h>
#include <kos/kernel/rwlock.h>
#include <sys/types.h>
#include <kos/kernel/sched_yield.h>

__DECL_BEGIN

__local void
kshmfutex_ptr_beginread(union kshmfutex_ptr *__restrict self) {
 byte_t lold,lnew;
again: do {
  lold = katomic_load(self->fp_lockbyte);
  if ((lold&KSHMPART_FUTEX_MASK_WMODE) ||  /* When the pointer is in write-mode, wait until it no longer is. */
      (lold&KSHMPART_FUTEX_MASK_RCNT) == KSHMPART_FUTEX_MASK_RCNT) /* Wait until there are reader slots available. */
  { ktask_yield(); goto again; }
  lnew = lold+(1 << KSHMPART_FUTEX_SHIFT_RCNT);
 } while (!katomic_cmpxch(self->fp_lockbyte,lold,lnew));
}
__local void
kshmfutex_ptr_endread(union kshmfutex_ptr *__restrict self) {
#ifdef __DEBUG__
 byte_t lold;
 do {
  lold = katomic_load(self->fp_lockbyte);
  assertf((lold&KSHMPART_FUTEX_MASK_RCNT) != 0,"No readers registers.");
 } while (!katomic_cmpxch(self->fp_lockbyte,lold,lold-(1 << KSHMPART_FUTEX_SHIFT_RCNT)));
#elif KSHMPART_FUTEX_SHIFT_RCNT
 katomic_fetchsub(self->fp_lockbyte,1 << KSHMPART_FUTEX_SHIFT_RCNT);
#else
 katomic_fetchdec(self->fp_lockbyte);
#endif
}


__local int
kshmfutex_ptr_trybeginwrite(union kshmfutex_ptr *__restrict self) {
 byte_t lold;
 do {
  lold = katomic_load(self->fp_lockbyte);
  /* Wait while the lock already is in write-mode or has readers. */
  if ((lold&(KSHMPART_FUTEX_MASK_WMODE|KSHMPART_FUTEX_MASK_RCNT)) != 0) return 0;
 } while (!katomic_cmpxch(self->fp_lockbyte,lold,lold|KSHMPART_FUTEX_MASK_WMODE));
 return 1;
}
__local void
kshmfutex_ptr_beginwrite(union kshmfutex_ptr *__restrict self) {
 byte_t lold;
again: do {
  lold = katomic_load(self->fp_lockbyte);
  /* Wait while the lock already is in write-mode or has readers. */
  if ((lold&(KSHMPART_FUTEX_MASK_WMODE|KSHMPART_FUTEX_MASK_RCNT)) != 0) { ktask_yield(); goto again; }
 } while (!katomic_cmpxch(self->fp_lockbyte,lold,lold|KSHMPART_FUTEX_MASK_WMODE));
}
__local void
kshmfutex_ptr_endwrite(union kshmfutex_ptr *__restrict self) {
#ifdef __DEBUG__
 byte_t lold;
 do {
  lold = katomic_load(self->fp_lockbyte);
  assertf((lold&KSHMPART_FUTEX_MASK_WMODE) != 0,"Not in write mode.");
 } while (!katomic_cmpxch(self->fp_lockbyte,lold,lold&~(KSHMPART_FUTEX_MASK_WMODE)));
#else
 katomic_fetchand(self->fp_lockbyte,~(KSHMPART_FUTEX_MASK_WMODE));
#endif
}

__local int
kshmfutex_ptr_upgrade(union kshmfutex_ptr *__restrict self) {
 byte_t lold;
 do {
  lold = katomic_load(self->fp_lockbyte);
  assertf(((lold&(KSHMPART_FUTEX_MASK_RCNT)) >> KSHMPART_FUTEX_SHIFT_RCNT) >= 1,
          "You're not holding a read-lock");
  /* Wait while the lock already is in write-mode or has readers. */
  if ((lold&(KSHMPART_FUTEX_MASK_RCNT)) !=
      (1 << KSHMPART_FUTEX_SHIFT_RCNT)) return 0; /* There is more than one reader. */
 } while (!katomic_cmpxch(self->fp_lockbyte,lold,
         (lold&~(KSHMPART_FUTEX_MASK_RCNT))|KSHMPART_FUTEX_MASK_WMODE));
 return 1; /* Successfully upgraded the read-lock. */
}


__crit void
kshmfutex_destroy(struct kshmfutex *__restrict self) {
 struct kshmfutex *next_futex;
 kassert_kshmfutex(self);
 assert(!self->f_refcnt);
again:
 kshmfutex_ptr_beginwrite(&self->f_next); /* lock 'f_pself' from being modified. */
 kassertobj(self->f_pself);
 /* Lock the self-pointer to we can update it to point to our successor. */
 if (!kshmfutex_ptr_trybeginwrite(self->f_pself)) {
  /* This may fail due to race conditions. */
  kshmfutex_ptr_endwrite(&self->f_next);
  ktask_yield();
  goto again;
 }
 next_futex = (struct kshmfutex *)(self->f_next.fp_futex&KSHMPART_FUTEX_ADDRESS);
 if (next_futex) {
  /* Must update the next->pself pointer. */
  if (!kshmfutex_ptr_trybeginwrite(&next_futex->f_next)) {
   kshmfutex_ptr_endwrite(self->f_pself);
   kshmfutex_ptr_endwrite(&self->f_next);
   goto again;
  }
  next_futex->f_pself = self->f_pself;
  /* Update the pself-pointer to point to the next futex, and unlock the self pointer. */
  katomic_store(self->f_pself->fp_futex,(uintptr_t)next_futex);
  kshmfutex_ptr_endwrite(&next_futex->f_next);
 } else {
  /* Update the pself-pointer to point to the next futex, and unlock the self pointer. */
  katomic_store(self->f_pself->fp_futex,(uintptr_t)next_futex);
 }
 /* 'self' is now fully unlinked. */
 ksignal_close(&self->f_sig); /* Shouldn't do anything, but better be safe and wake anything that's still waiting. */
 free(self);
}

__crit __ref struct kshmfutex *
kshmfutex_ptr_getfutex(union kshmfutex_ptr *__restrict self,
                       kshmfutex_id_t id) {
 kerrno_t ref_error;
 struct kshmfutex *result;
 KTASK_CRIT_MARK
 kassertobj(self);
 kshmfutex_ptr_beginread(self);
 while ((result = kshmfutex_ptr_get(self)) != NULL) {
  kassert_kshmfutex(result);
  if (result->f_id >= id) { /* Found it, or doesn't exist. */
   if (result->f_id != id) break; /* Doesn't exist. */
   ref_error = kshmfutex_tryincref(result);
   kshmfutex_ptr_endread(self);
   return __likely(KE_ISOK(ref_error)) ? result : NULL;
  }
  kshmfutex_ptr_beginread(&result->f_next);
  kshmfutex_ptr_endread(self);
  self = &result->f_next;
 }
 kshmfutex_ptr_endread(self);
 return NULL;
}

__crit __ref struct kshmfutex *
kshmfutex_ptr_getfutex_newmissing(union kshmfutex_ptr *__restrict self,
                                  kshmfutex_id_t id) {
 kerrno_t ref_error;
 struct kshmfutex *result,*next_futex;
 union kshmfutex_ptr *iter = self;
 KTASK_CRIT_MARK
 kassertobj(self);
again:
 kshmfutex_ptr_beginread(iter);
 while ((result = kshmfutex_ptr_get(iter)) != NULL) {
  kassert_kshmfutex(result);
  if (result->f_id >= id) { /* Found it, or doesn't exist. */
   if (result->f_id != id) {
insert_futex_into_iter:
    /* Must insert a new futex here. */
    result = (struct kshmfutex *)memalign(KSHMFUTEX_ALIGN,sizeof(struct kshmfutex));
    if __unlikely(!result) goto err_nomem;
    /* Upgrade the read-lock into a write-lock. */
    if (!kshmfutex_ptr_upgrade(iter)) {
     kshmfutex_ptr_endread(iter);
     free(result);
     iter = self;
     goto again;
    }
    result->f_refcnt = 1;
    result->f_id     = id;
    ksignal_init(&result->f_sig);
    /* Link in the new futex. */
    result->f_pself = iter; /* Prepare the self-pointer for setting iter  */
    result->f_next.fp_futex = iter->fp_futex&KSHMPART_FUTEX_ADDRESS;
    if ((next_futex = (struct kshmfutex *)result->f_next.fp_futex) != NULL) {
     kassert_kshmfutex(next_futex);
     /* Try to lock the successor (If this fails, we must unlock everything and start over!) */
     if __unlikely(!kshmfutex_ptr_trybeginwrite(&next_futex->f_next)) {
      kshmfutex_ptr_endwrite(iter);
      free(result);
      ktask_yield();
      goto again;
     }
     next_futex->f_pself = &result->f_next;
     kshmfutex_ptr_endwrite(&next_futex->f_next);
    }
    katomic_store(iter->fp_futex,(uintptr_t)result); /* NOTE: This like also unlocks 'iter'. */
    return result;
   }
   ref_error = kshmfutex_tryincref(result);
   if __unlikely(KE_ISERR(ref_error)) {
    /* This can happen due to a race-condition where an
     * existing futex with the same ID is currently being
     * deallocated, and was currently trying to lock its
     * self-pointer when we were faster. */
    goto insert_futex_into_iter;
   }
   kshmfutex_ptr_endread(iter);
   return result;
  }
  kshmfutex_ptr_beginread(&result->f_next);
  kshmfutex_ptr_endread(iter);
  iter = &result->f_next;
 }
 goto insert_futex_into_iter;
err_nomem:
 kshmfutex_ptr_endread(iter);
 return NULL;
}

__crit kerrno_t
kshm_getfutex_unlocked(struct kshm *__restrict self, __user void *address,
                       __ref struct kshmfutex **user_futex, int alloc_missing) {
 struct kshmbranch *branch;
 struct kshmregion *region;
 struct kshmcluster *futex_cluster;
 struct kshmpart *futex_part;
 kshmregion_addr_t futex_address;
 kshmregion_page_t futex_page;
 kshmfutex_id_t futex_id;
 KTASK_CRIT_MARK
 kassert_kshm(self);
 assert(krwlock_isreadlocked(&self->s_lock) ||
        krwlock_iswritelocked(&self->s_lock));
 /* Lookup the branch associated with the futex address. */
 branch = kshmmap_locate(&self->s_map,(uintptr_t)address);
 if __unlikely(!branch) return KE_FAULT;
 region = branch->sb_region;
 kassert_kshmregion(region);
 assert(region->sre_clusterc);
 assert(region->sre_chunk.sc_pages);
 assert(region->sre_chunk.sc_partc);
 assert((uintptr_t)address >= branch->sb_map_min &&
        (uintptr_t)address <= branch->sb_map_max);
 /* Figure out the region-relative futex address. */
 futex_address = (branch->sb_rstart*PAGESIZE)+((uintptr_t)address-branch->sb_map_min);
 assert(futex_address < region->sre_chunk.sc_pages*PAGESIZE);
 futex_page = futex_address/PAGESIZE;
 /* Figure out which cluster the futex belongs to. */
 assert(futex_page/KSHM_CLUSTERSIZE < region->sre_clusterc);
 futex_cluster = &region->sre_clusterv[futex_page/KSHM_CLUSTERSIZE];
 /* Make sure the cluster is allocated (It should always be, but better be careful here...) */
 if __unlikely(!futex_cluster->sc_refcnt) return KE_FAULT;
 futex_part = futex_cluster->sc_part;
 /* Walk to the first part associated with the futex. */
 while ((assert(futex_part >= region->sre_chunk.sc_partv &&
                futex_part <  region->sre_chunk.sc_partv+
                              region->sre_chunk.sc_partc),
         futex_part->sp_start < futex_page)) ++futex_part;
 /* Figure out the futex ID. */
 futex_id = (futex_address%PAGESIZE)/sizeof(unsigned int);
 /* Lookup/generate the futex. */
 if (alloc_missing) {
  *user_futex = kshmfutex_ptr_getfutex_newmissing(&futex_part->sp_futex,futex_id);
 } else {
  *user_futex = kshmfutex_ptr_getfutex(&futex_part->sp_futex,futex_id);
 }
 return *user_futex ? KE_OK : alloc_missing ? KE_NOMEM : KE_NOENT;
}

__crit kerrno_t
kshm_getfutex(struct kshm *__restrict self, __user void *address,
              __ref struct kshmfutex **user_futex, int alloc_missing) {
 kerrno_t error;
 error = krwlock_beginread(&self->s_lock);
 if __unlikely(KE_ISERR(error)) return error;
 error = kshm_getfutex_unlocked(self,address,user_futex,alloc_missing);
 krwlock_endread(&self->s_lock);
 return error;
}




#define FUTEXOP_MAYALLOCATE(op) (((op)&KTASK_FUTEX_OP_KINDMASK) == KTASK_FUTEX_WAIT)

__crit kerrno_t
kshm_futex(struct kshm *__restrict self,
           __user void *address, unsigned int futex_op, unsigned int val,
           __user struct timespec *abstime,
           __kernel unsigned int *woken_tasks) {
 kerrno_t error;
 __ref struct kshmfutex *user_futex;
 KTASK_CRIT_MARK
 kassert_kshm(self);
 error = krwlock_beginread(&self->s_lock);
 if __unlikely(KE_ISERR(error)) return error;
 /* Make sure the given pointer is properly aligned (misalignments are silently ignored).
  * TODO: Shouldn't we just fail if it's misaligned? (But with what error...) */
 *(uintptr_t *)&address &= ~((uintptr_t)sizeof(unsigned int)-1);
 error = kshm_getfutex_unlocked(self,address,&user_futex,
                                FUTEXOP_MAYALLOCATE(futex_op));
 if __unlikely(KE_ISERR(error)) {
  if (error == KE_NOENT) error = KE_OK;
  goto end_unlock;
 }
 kassert_kshmfutex(user_futex);
 /* We've got a reference to the futex. Time to do this! */
 switch (futex_op&KTASK_FUTEX_OP_KINDMASK) {

  /* Wait for a futex to be signaled. */
  case KTASK_FUTEX_WAIT: {
   __kernel unsigned int *kernel_futex,kernel_futex_val;
   struct timespec kernel_abstime;
   size_t kernel_futex_size;
   kernel_futex = (unsigned int *)kshm_translateuser(self,self->s_pd,address,
                                                     sizeof(unsigned int),
                                                    &kernel_futex_size,0);
   if __unlikely(!kernel_futex) {efault: error = KE_FAULT; break; } /* Really shouldn't happen... */
   assertf(kernel_futex_size == sizeof(unsigned int),
           "This should always be the case due to the alignment fix above!");
   if __unlikely(abstime && kshm_copyfromuser(self,&kernel_abstime,abstime,
                                              sizeof(struct timespec))
                 ) goto efault;
   /* Lock the signal, thus preventing anyone from sending it.
    * >> Past this point, everything we do will appear atomic in userspace. */
   ksignal_lock_c(&user_futex->f_sig,KSIGNAL_LOCK_WAIT);
   /* Atomically load the futex's value from userspace. */
   kernel_futex_val = katomic_load(*kernel_futex);
   /* Only unlock the SHM manager after we've dereferenced the userspace
    * value, thus ensuring that no race condition of the user munmap-ing
    * the memory in the meantime. */
   krwlock_endread(&self->s_lock);
   /* Do the ~magical~ comparison, making sure that the futex value matches what is given. */
   switch (futex_op&KTASK_FUTEX_WAIT_MASK) {
    default                   : if (kernel_futex_val != val) goto fail_again; break;
    case KTASK_FUTEX_WAIT_LO  : if (kernel_futex_val >= val) goto fail_again; break;
    case KTASK_FUTEX_WAIT_LE  : if (kernel_futex_val > val) goto fail_again; break;
    case KTASK_FUTEX_WAIT_AND : if (!(kernel_futex_val & val)) goto fail_again; break;
    case KTASK_FUTEX_WAIT_XOR : if (!(kernel_futex_val ^ val)) goto fail_again; break;
    case KTASK_FUTEX_WAIT_NE  : if (kernel_futex_val == val) goto fail_again; break;
    case KTASK_FUTEX_WAIT_GE  : if (kernel_futex_val < val) goto fail_again; break;
    case KTASK_FUTEX_WAIT_GR  : if (kernel_futex_val <= val) goto fail_again; break;
    case KTASK_FUTEX_WAIT_NAND: if (kernel_futex_val & val) goto fail_again; break;
    case KTASK_FUTEX_WAIT_NXOR: if (kernel_futex_val ^ val) goto fail_again; break;
   }
   
   error = abstime /* Unlock and receive the signal. */
    ? _ksignal_timedrecv_andunlock_c(&user_futex->f_sig,&kernel_abstime)
    : _ksignal_recv_andunlock_c(&user_futex->f_sig);
end_futex:
   kshmfutex_decref(user_futex);
   return error;
fail_again:
   error = KE_AGAIN;
   ksignal_unlock_c(&user_futex->f_sig,KSIGNAL_LOCK_WAIT);
   k_syslogf(KLOG_DEBUG,"[FUTEX] fail(EAGAIN)\n");
   goto end_futex;
  } break;

  /* Wake a given amount of threads waiting for a futex. */
  case KTASK_FUTEX_WAKE: {
   *woken_tasks = 0;
   while (val-- && (ksignal_sendone(&user_futex->f_sig) != KS_EMPTY)) ++*woken_tasks;
   error = KE_OK;
  } break;

  default:
   error = KE_INVAL;
   break;
 }

 kshmfutex_decref(user_futex);
end_unlock:
 krwlock_endread(&self->s_lock);
 return error;
}

__DECL_END

#endif /* !__KOS_KERNEL_SHM_FUTEX_C_INL__ */

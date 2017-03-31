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

#include <kos/atomic.h>
#include <kos/compiler.h>
#include <kos/futex.h>
#include <kos/kernel/rwlock.h>
#include <kos/kernel/sched_yield.h>
#include <kos/kernel/shm.h>
#include <kos/kernel/signal.h>
#include <kos/kernel/signal_v.h>
#include <kos/syslog.h>
#include <sys/types.h>
#include <kos/kernel/sigset.h>
#include <kos/kernel/sigset_v.h>

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
 assert(futex_page < region->sre_chunk.sc_pages);
 assert(futex_page/KSHM_CLUSTERSIZE < region->sre_clusterc);
 futex_cluster = kshmregion_getcluster(region,futex_page);
 /* Make sure the cluster is allocated (It should always be, but better be careful here...) */
 if __unlikely(!futex_cluster->sc_refcnt) return KE_FAULT;
 futex_part = futex_cluster->sc_part;
 /* Walk to the first part associated with the futex. */
 while ((assertf(futex_part >= region->sre_chunk.sc_partv &&
                 futex_part <  region->sre_chunk.sc_partv+
                               region->sre_chunk.sc_partc,
                 "%p <= %p < %p",region->sre_chunk.sc_partv,futex_part,
                 region->sre_chunk.sc_partv+region->sre_chunk.sc_partc),
         futex_part->sp_start+futex_part->sp_pages <= futex_page)) ++futex_part;
 /* Figure out the futex ID. */
#if __SIZEOF_INT__ == 4
 futex_id = (futex_address%PAGESIZE) >> 2;
#else
 futex_id = (futex_address%PAGESIZE)/sizeof(int);
#endif
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




#define FUTEXOP_MAYALLOCATE(opcode) ((opcode) != _KFUTEX_CMD_SEND)

__crit kerrno_t
kshm_futex(struct kshm *__restrict self,
           __user void *address, unsigned int futex_op, unsigned int val,
           __user void *buf, __user struct timespec const *abstime,
           __kernel unsigned int *woken_tasks,
           __user void *address2, unsigned int val2) {
 kerrno_t error; int opcode;
 __ref struct kshmfutex *user_futex,*user_futex2;
 KTASK_CRIT_MARK
 kassert_kshm(self);
 error = krwlock_beginread(&self->s_lock);
 if __unlikely(KE_ISERR(error)) return error;
 opcode = futex_op&_KFUTEX_MASK_CMD;
 error = kshm_getfutex_unlocked(self,address,&user_futex,
                                FUTEXOP_MAYALLOCATE(opcode));
 if __unlikely(KE_ISERR(error)) {
  if (error == KE_NOENT) {
   /* Special error codes for non-present futex. */
   error = opcode == _KFUTEX_CMD_SEND ? KS_EMPTY : KE_OK;
  }
  goto end_unlock;
 }
 kassert_kshmfutex(user_futex);
 /* We've got a reference to the futex. Time to do this! */
 switch (opcode) {

  { /* Wait for a futex to be signaled (aka. receive it). */
   __kernel unsigned int *kernel_futex,kernel_futex_val;
   __kernel unsigned int *kernel_futex2,kernel_futex2_val;
   struct timespec kernel_abstime;
  case _KFUTEX_CMD_C1:
  case _KFUTEX_CMD_CX:
  case _KFUTEX_CMD_RECV:
  case _KFUTEX_CMD_C0:
user_lock_again:
   kernel_futex = (unsigned int *)kshm_qtranslateuser(self,self->s_pd,address,0);
   if __unlikely(!kernel_futex) { error = KE_FAULT; break; } /* Really shouldn't happen... */
   /* Load the secondary futex and its physical address. */
   if (opcode == _KFUTEX_CMD_RECV) user_futex2 = NULL;
   else {
    if (address2 == address) {
     /* Same userspace address --> Same futex. */
same_futex:
     if (opcode != _KFUTEX_CMD_C0) { error = KE_INVAL; break; }
     kernel_futex2 = kernel_futex;
     user_futex2 = NULL;
    } else {
     error = kshm_getfutex_unlocked(self,address2,&user_futex2,0);
     if __unlikely(KE_ISERR(error)) {
      if (error == KE_NOENT) user_futex2 = NULL;
      else break;
     }
     if __unlikely(user_futex2 == user_futex) {
      /* Special case: Physical memory is mapped more than once
       * >> These are two different pointers for the same memory location. */
      kshmfutex_decref(user_futex2);
      goto same_futex;
     }
    }
    error = krwlock_upgrade(&self->s_lock);
    if __unlikely(KE_ISERR(error)) goto err_futex2;
    if (error == KS_UNLOCKED) goto user_lock_again;
    kernel_futex2 = (unsigned int *)kshm_qtranslateuser(self,self->s_pd,address2,1);
    /* Really shouldn't happen... */
    if __unlikely(!kernel_futex2) {
err_futex2_fault:
     error = KE_FAULT;
err_futex2:
     if (user_futex2) kshmfutex_decref(user_futex2);
     break;
    }
    assert(kernel_futex2 != kernel_futex);
   }
   assert(user_futex2 != user_futex);
   if __unlikely(abstime && kshm_copyfromuser(self,&kernel_abstime,abstime,
                                              sizeof(struct timespec))
                 ) goto err_futex2_fault;
   /* Lock the signal, thus preventing anyone from sending it.
    * >> Past this point, everything we do will appear atomic in userspace. */
   ksignal_lock_c(&user_futex->f_sig,KSIGNAL_LOCK_WAIT);
   /* Atomically load the futex's value from userspace. */
reload_kernel_futex:
   kernel_futex_val = katomic_load(*kernel_futex);
   /* Do the ~magical~ comparison, making sure that the futex value matches what is given. */
   switch ((futex_op & _KFUTEX_RECV_OPMASK) >> _KFUTEX_RECV_OPSHIFT) {
    case KFUTEX_EQUAL        : if (!(kernel_futex_val == val)) goto dont_receive; break;
    case KFUTEX_LOWER        : if (!(kernel_futex_val < val)) goto dont_receive; break;
    case KFUTEX_LOWER_EQUAL  : if (!(kernel_futex_val <= val)) goto dont_receive; break;
    case KFUTEX_AND          : if (!(kernel_futex_val & val)) goto dont_receive; break;
    case KFUTEX_XOR          : if (!(kernel_futex_val ^ val)) goto dont_receive; break;
    case KFUTEX_SHL          : if (!(kernel_futex_val << val)) goto dont_receive; break;
    case KFUTEX_SHR          : if (!(kernel_futex_val >> val)) goto dont_receive; break;
    case KFUTEX_NOT_EQUAL    : if (!(kernel_futex_val != val)) goto dont_receive; break;
    case KFUTEX_GREATER_EQUAL: if (!(kernel_futex_val >= val)) goto dont_receive; break;
    case KFUTEX_GREATER      : if (!(kernel_futex_val > val)) goto dont_receive; break;
    case KFUTEX_NOT_AND      : if ((kernel_futex_val & val)) goto dont_receive; break;
    case KFUTEX_NOT_XOR      : if ((kernel_futex_val ^ val)) goto dont_receive; break;
    case KFUTEX_NOT_SHL      : if ((kernel_futex_val << val)) goto dont_receive; break;
    case KFUTEX_NOT_SHR      : if ((kernel_futex_val >> val)) goto dont_receive; break;
    case KFUTEX_FALSE        : goto dont_receive; break; /* Doesn't make much sense, but it is logical... */
    default: break;
   }
   if (opcode != _KFUTEX_CMD_RECV) {
    unsigned int newval;
    /* Secondary operation (used for condition variables). */
    if (user_futex2) {
     if __unlikely(!ksignal_trylock_c(&user_futex2->f_sig,KSIGNAL_LOCK_WAIT)) {
      /* Must idle back and try again (can happen if two futex objects try to lock each other). */
      ksignal_unlock_c(&user_futex->f_sig,KSIGNAL_LOCK_WAIT);
      krwlock_end(&self->s_lock); /* Could be a read- or a write-lock. */
      ktask_yield(); /* Yield execution, hopefully getting the other side to do its job first. */
      error = krwlock_beginread(&self->s_lock);
      if __unlikely(KE_ISERR(error)) goto err_futex2;
      goto user_lock_again;
     }
    }
    if (kernel_futex == kernel_futex2) {
     kernel_futex2_val = kernel_futex_val;
    } else {
reload_kernel_futex2:
     kernel_futex2_val = katomic_load(*kernel_futex2);
    }
    /* Perform the 'KFUTEX_CCMD(*)' operation. */
    switch ((opcode & _KFUTEX_CCMD_OPMASK) >> _KFUTEX_CCMD_OPSHIFT) {
     default            : newval = val2; break; /* '*uaddr2 = val2'. */
     case KFUTEX_ADD    : newval = kernel_futex2_val+val2; break; /* '*uaddr2 += val2' */
     case KFUTEX_SUB    : newval = kernel_futex2_val-val2; break; /* '*uaddr2 -= val2' */
     case KFUTEX_MUL    : newval = kernel_futex2_val*val2; break; /* '*uaddr2 *= val2' */
     case KFUTEX_DIV    : newval = kernel_futex2_val/val2; break; /* '*uaddr2 /= val2' */
     case KFUTEX_AND    : newval = kernel_futex2_val&val2; break; /* '*uaddr2 &= val2' */
     case KFUTEX_OR     : newval = kernel_futex2_val|val2; break; /* '*uaddr2 |= val2' */
     case KFUTEX_XOR    : newval = kernel_futex2_val^val2; break; /* '*uaddr2 ^= val2' */
     case KFUTEX_SHL    : newval = kernel_futex2_val<<val2; break; /* '*uaddr2 <<= val2' */
     case KFUTEX_SHR    : newval = kernel_futex2_val>>val2; break; /* '*uaddr2 >>= val2' */
     case KFUTEX_NOT    : newval = ~(val2); break; /* '*uaddr2 = ~(val2)' */
     case KFUTEX_NOT_ADD: newval = ~(kernel_futex2_val+val2); break; /* '*uaddr2 = ~(*uaddr2 + val2)' */
     case KFUTEX_NOT_SUB: newval = ~(kernel_futex2_val-val2); break; /* '*uaddr2 = ~(*uaddr2 - val2)' */
     case KFUTEX_NOT_MUL: newval = ~(kernel_futex2_val*val2); break; /* '*uaddr2 = ~(*uaddr2 * val2)' */
     case KFUTEX_NOT_DIV: newval = ~(kernel_futex2_val/val2); break; /* '*uaddr2 = ~(*uaddr2 / val2)' */
     case KFUTEX_NOT_AND: newval = ~(kernel_futex2_val&val2); break; /* '*uaddr2 = ~(*uaddr2 & val2)' */
     case KFUTEX_NOT_OR : newval = ~(kernel_futex2_val|val2); break; /* '*uaddr2 = ~(*uaddr2 | val2)' */
     case KFUTEX_NOT_XOR: newval = ~(kernel_futex2_val^val2); break; /* '*uaddr2 = ~(*uaddr2 ^ val2)' */
     case KFUTEX_NOT_SHL: newval = ~(kernel_futex2_val<<val2); break; /* '*uaddr2 = ~(*uaddr2 << val2)' */
     case KFUTEX_NOT_SHR: newval = ~(kernel_futex2_val>>val2); break; /* '*uaddr2 = ~(*uaddr2 >> val2)' */
    }
    /* Set the new value for the secondary futex. */
    k_syslogf(KLOG_TRACE,"[FUTEX] Update word at %p:%p: %u -> %u\n",
              address2,kernel_futex2,kernel_futex2_val,newval);
    if (!katomic_cmpxch(*kernel_futex2,kernel_futex2_val,newval)) {
     /* Failed to apply new futex2 value. */
     if (kernel_futex == kernel_futex2) {
      assert(!user_futex2);
      goto reload_kernel_futex; /* Reload the primary futex. */
     } else {
      goto reload_kernel_futex2; /* Reload the secondary futex. */
     }
    }
    /* Send a signal to the secondary futex. */
    if (user_futex2) {
     assert(opcode == _KFUTEX_CMD_C1 || opcode == _KFUTEX_CMD_CX);
     assert(kernel_futex != kernel_futex2);
     k_syslogf(KLOG_TRACE,"[FUTEX] send(%p)\n",address2);
     if (opcode == _KFUTEX_CMD_C1) { /* Send one in 'user_futex2' */
      while (_ksignal_sendone_andunlock_c(&user_futex2->f_sig) == KE_DESTROYED);
     } else { /* Send all in 'user_futex2' */
      _ksignal_sendall_andunlock_c(&user_futex2->f_sig);
     }
     kshmfutex_decref(user_futex2);
    } else {
    }
   } else {
    assert(!user_futex2);
   }
   /* Only unlock the SHM manager after we've dereferenced the userspace
    * value, thus ensuring that no race condition of the user munmap-ing
    * the memory in the meantime. */
   krwlock_end(&self->s_lock); /* Could be a read- or a write-lock. */
   k_syslogf(KLOG_TRACE,"[FUTEX] recv(%p)\n",address);
   /* Select the appropriate receiver implementation
    * to unlock and receive the signal. */
   error = buf ? (abstime
    ?  _ksignal_vtimedrecv_andunlock_c(&user_futex->f_sig,&kernel_abstime,buf)
    :       _ksignal_vrecv_andunlock_c(&user_futex->f_sig,buf)) : (abstime
    ?   _ksignal_timedrecv_andunlock_c(&user_futex->f_sig,&kernel_abstime)
    :        _ksignal_recv_andunlock_c(&user_futex->f_sig));
   kshmfutex_decref(user_futex);
   return error;
dont_receive:
   ksignal_unlock_c(&user_futex->f_sig,KSIGNAL_LOCK_WAIT);
   if (user_futex2) kshmfutex_decref(user_futex2);
   k_syslogf(KLOG_TRACE,"[FUTEX] fail(EAGAIN)\n");
   error = KE_AGAIN;
  } break;

  /* Wake a given amount of threads waiting for a futex. */
  case _KFUTEX_CMD_SEND: {
   *woken_tasks = 0;
   k_syslogf(KLOG_TRACE,"[FUTEX] send(%p)\n",address);
   if (buf) {
    size_t bufsize = (size_t)abstime;
    while (val-- && (ksignal_vusendone(&user_futex->f_sig,buf,bufsize) != KS_EMPTY)) ++*woken_tasks;
   } else {
    while (val-- && (ksignal_sendone(&user_futex->f_sig) != KS_EMPTY)) ++*woken_tasks;
   }
   error = *woken_tasks ? KE_OK : KS_EMPTY;
  } break;

  default:
   error = KE_INVAL;
   break;
 }
 kshmfutex_decref(user_futex);
end_unlock:
 krwlock_end(&self->s_lock); /* Could be a read- or a write-lock. */
 return error;
}

struct kernel_futexset {
 struct ksigset              kfs_sigset; /*< Signal set initialized to mirror  */
 /* The following is actually a 'struct kernel_futexset *' */
#define kfs_next  kfs_sigset.ss_next     /*< [0..1][owned] Next futex set. */
 /* The following is actually a '__ref struct kshmfutex **' */
#define kfs_fvec  kfs_sigset.ss_elemvec  /*< [1..1][0..kfs_sigset.ss_elemcnt][owned] Vector of references to futex objects. */
 unsigned int                kfs_op;     /*< The futex operation that should be applied to those in the vector below. */
 unsigned int                kfs_val;    /*< The value used as right-hand-side operand in some recv operations as set by 'fs_op'. */
 __kernel unsigned int     **kfs_addrv;  /*< [1..1][0..kfs_addrc][owned] Vector of physical futex addresses. */
};


/* Load a futexset from userspace.
 * If the given 'address2' is part of the futex set, fill
 * '*user_futex2' and '*kernel_futex2' with its data.
 * @return: KS_EMPTY: empty set. */
static __crit kerrno_t
init_futexset(struct kernel_futexset *__restrict self, struct kshm *__restrict shm,
              __user struct kfutexset *ftxset, __user void *address2,
              struct kshmfutex **user_futex2, __kernel kfutex_t **kernel_futex2) {
#define V(x) *((__ref struct kshmfutex ***)&(x)->kfs_fvec)
 struct kfutexset kernel_set;
 struct kernel_futexset *currset,*nextset,*iterset;
 __ref struct kshmfutex **iter;
 kerrno_t error = KS_EMPTY;
 kassertobj(self);
 kassertobj(user_futex2);
 kassertobj(kernel_futex2);
 kassert_kshm(shm);
 assert(krwlock_isreadlocked(&shm->s_lock) ||
        krwlock_iswritelocked(&shm->s_lock));
 currset = self;
 /* Iterate all user futex sets. */
 if (!ftxset) {
  currset->kfs_sigset.ss_elemsiz = 0;
  currset->kfs_sigset.ss_elemcnt = 0;
  currset->kfs_sigset.ss_elemoff = offsetof(struct kshmfutex,f_sig);
  V(currset)                     = NULL;
 } else for (;;) {
  size_t max_vector_align;
  kfutex_t **futex_iter,**futex_end;
  __ref struct kshmfutex **userfutex_iter;
  /* Copy the user-descriptor for this futex set. */
  if __unlikely(kshm_copyfromuser(shm,&kernel_set,ftxset,sizeof(struct kfutexset))
                ) { error = KE_FAULT; goto err; }
  /* Start with the simple stuff */
  currset->kfs_op                = kernel_set.fs_op;
  currset->kfs_val               = kernel_set.fs_val;
  currset->kfs_sigset.ss_elemcnt = kernel_set.fs_count;
  currset->kfs_sigset.ss_elemsiz = 0;
  currset->kfs_sigset.ss_elemoff = offsetof(struct kshmfutex,f_sig);
  if (kernel_set.fs_count) error = KE_OK; /* Don't return KS_EMPTY. */
  /* Check for overflow in the futex count. */
  max_vector_align = kernel_set.fs_align&KFUTEXSET_ALIGN_ISALIGN
   ? kernel_set.fs_align&~(KFUTEXSET_ALIGN_ISALIGN)
   : sizeof(void *);
  if ((kernel_set.fs_count*max_vector_align) <
       kernel_set.fs_count) { error = KE_OVERFLOW; goto err; }
  /* Allocate the buffer for all of the physical addresses.
   * todo: other than checking for overflow, shouldn't we restrict this buffer? */
  currset->kfs_addrv = (__kernel unsigned int **)malloc(kernel_set.fs_count*
                                                        sizeof(__kernel unsigned int *));
  /* Copy the userspace futex/futex-pointer vector. */
  if (kernel_set.fs_align&KFUTEXSET_ALIGN_ISALIGN) {
   /* Inline vector (Must use 'fs_align' as alignment offset). */
   uintptr_t futex_addr = (uintptr_t)kernel_set.fs_avec;
   uintptr_t elem_offset = kernel_set.fs_align&~(KFUTEXSET_ALIGN_ISALIGN);
   futex_end = (futex_iter = (kfutex_t **)currset->kfs_addrv)+kernel_set.fs_count;
   /* Calculate the virtual addresses of all futex objects. */
   for (; futex_iter != futex_end; ++futex_iter,futex_addr += elem_offset
        ) *futex_iter = (kfutex_t *)futex_addr;
  } else {
   /* Dereferenced vector (Every element is a pointer). */
   if __unlikely(kshm_copyfromuser(shm,currset->kfs_addrv,kernel_set.fs_dvec,
                                   kernel_set.fs_count*sizeof(void *))) {
           error = KE_FAULT;
err_addrv: free(currset->kfs_addrv);
    goto err;
   }
   if (kernel_set.fs_align != 0) {
    /* Must add an alignment offset to every element. */
    futex_end = (futex_iter = (kfutex_t **)currset->kfs_addrv)+kernel_set.fs_count;
    for (; futex_iter != futex_end; ++futex_iter) *(uintptr_t *)futex_iter += kernel_set.fs_align;
   }
  }
  /* At this point 'kfs_addrv' contains the virtual
   * addresses of all futexes in this set part.
   * >> Now we must align and convert them to physical addresses, as
   *    well as acquire references to the associated futex objects. */
  /* Step #1: Allocate the vector of user-futex reference pointers. */
  V(currset) = (__ref struct kshmfutex **)malloc(kernel_set.fs_count*
                                                 sizeof(__ref struct kshmfutex *));
  if (!V(currset)) { error = KE_NOMEM; goto err_addrv; }
  /* Step #2: Walk along the virtual address vector, converting each into their
   *          physical counterpart, as well as looking up the associated futex. */
  futex_end = (futex_iter = (kfutex_t **)currset->kfs_addrv)+kernel_set.fs_count;
  userfutex_iter = V(currset);
  for (; futex_iter != futex_end; ++futex_iter,++userfutex_iter) {
   __user kfutex_t *user_futex_addr;
   __kernel kfutex_t *kernel_futex_addr;
   user_futex_addr = *futex_iter;
   /* Translate the virtual address. */
   kernel_futex_addr = (__kernel kfutex_t *)kshm_qtranslateuser(shm,shm->s_pd,
                                                               (__user void *)user_futex_addr,0);
   if __unlikely(!kernel_futex_addr) {
            error = KE_FAULT;
err_futexv: while (userfutex_iter-- != V(currset)) kshmfutex_decref(*userfutex_iter);
            free(V(currset));
            goto err_addrv;
   }
   /* Store the physical address. */
   *futex_iter = (kfutex_t *)kernel_futex_addr;
   /* Retrieve the futex object associated with this address. */
   error = kshm_getfutex_unlocked(shm,(void *)user_futex_addr,userfutex_iter,1);
   if __unlikely(KE_ISERR(error)) goto err_futexv;
   if (address2 == user_futex_addr) {
    *user_futex2   = *userfutex_iter;
    *kernel_futex2 = kernel_futex_addr;
   }
  }

  if ((ftxset = kernel_set.fs_next) == NULL) break; /* This was the last set. */
  /* Allocate a new futex set and chain it to the last one. */
  nextset = omalloc(struct kernel_futexset);
  if __unlikely(!nextset) { error = KE_NOMEM; goto err; }
  currset->kfs_next = (struct ksigset *)nextset;
  currset = nextset;
 }
 currset->kfs_next = NULL;
 return error;
err:
 iterset = self;
 while (iterset != currset) {
  iter = V(iterset)+iterset->kfs_sigset.ss_elemcnt;
  while (iter-- != V(iterset)) kshmfutex_decref(*iter);
  free(V(iterset));
  free(iterset->kfs_addrv);
  nextset = (struct kernel_futexset *)iterset->kfs_next;
  if (iterset != self) free(iterset);
  iterset = nextset;
 }
 return error;
}


static __crit void
quit_futexset(struct kernel_futexset *__restrict self) {
 __ref struct kshmfutex **iter;
 struct kernel_futexset *iterset,*nextset;
 assert(self->kfs_sigset.ss_elemsiz == 0);
 assert(self->kfs_sigset.ss_elemoff == offsetof(struct kshmfutex,f_sig));
 iterset = self;
 for (;;) {
  iter = V(iterset)+iterset->kfs_sigset.ss_elemcnt;
  while (iter-- != V(iterset)) kshmfutex_decref(*iter);
  free(V(iterset));
  free(iterset->kfs_addrv);
  nextset = (struct kernel_futexset *)iterset->kfs_next;
  if (iterset != self) free(iterset);
  if (!nextset) break;
  iterset = nextset;
 }
}

__crit kerrno_t
kshm_futexs(struct kshm *__restrict self, __user struct kfutexset *ftxset,
            __user void *buf, __user struct timespec const *abstime,
            __user void *address2, unsigned int val2) {
 kerrno_t error; struct timespec kernel_abstime;
 int secondary_op,secondary_opcode,has_write_lock = 0;
 int user_futex2_is_reference,futex_op;
 unsigned int futex_val,old_addr2_word,addr2_partof_set;
 __ref struct kshmfutex *user_futex2;
 __kernel kfutex_t *kernel_futex2;
 struct kernel_futexset set;
 struct kernel_futexset *set_iter;
 __kernel kfutex_t **kernel_futex_iter,**kernel_futex_end;
 KTASK_CRIT_MARK
 kassert_kshm(self);
 k_syslogf(KLOG_INFO,"[IRQ] %d\n",karch_irq_enabled());
 error = krwlock_beginread(&self->s_lock);
 if __unlikely(KE_ISERR(error)) return error;
 if __unlikely(abstime && kshm_copyfromuser(self,&kernel_abstime,
               abstime,sizeof(struct timespec))) { error = KE_FAULT; goto end_unlock; }
reload_futexset:
 user_futex2 = NULL,kernel_futex2 = NULL;
 error = init_futexset(&set,self,ftxset,address2,&user_futex2,&kernel_futex2);
#if 0
 k_syslogf(KLOG_DEBUG
          ,"set.kfs_sigset.ss_next    = %p\n"
           "set.kfs_sigset.ss_elemcnt = %Iu\n"
           "set.kfs_sigset.ss_elemsiz = %Iu\n"
           "set.kfs_sigset.ss_elemoff = %Iu\n"
           "set.kfs_sigset.ss_elemvec = %p\n"
           "set.kfs_op                = %#x\n"
           "set.kfs_val               = %#x\n"
           "set.kfs_addrv             = %p\n"
          ,set.kfs_sigset.ss_next
          ,set.kfs_sigset.ss_elemcnt
          ,set.kfs_sigset.ss_elemsiz
          ,set.kfs_sigset.ss_elemoff
          ,set.kfs_sigset.ss_elemvec
          ,set.kfs_op
          ,set.kfs_val
          ,set.kfs_addrv);
#endif
 if __unlikely(KE_ISERR(error)) goto end_unlock;
 /* Must handle special case ~empty~ here because the scheduler
  * would put the calling thread into an unwakeable waiting state. */
 if __unlikely(error == KS_EMPTY) { error = KE_OK; goto end_futexset; }

 secondary_opcode = (secondary_op = set.kfs_op)&_KFUTEX_MASK_CMD;
 if (secondary_opcode == _KFUTEX_CMD_C0 ||
     secondary_opcode == _KFUTEX_CMD_C1 ||
     secondary_opcode == _KFUTEX_CMD_CX) {
  if (!has_write_lock) {
   /* We need a write-lock to perform the secondary command. */
   error = krwlock_upgrade(&self->s_lock);
   if __unlikely(KE_ISERR(error)) goto end_futexset;
   has_write_lock = 1;
   if (error == KS_UNLOCKED) {
    /* NOTE: Since we already need to reload the futex set, check
     *       if we also need to touch the secondary futex page.
     * >> If we do need to, already touch the page so we don't need to start over twice! */
    {
     kpage_t *page = kpagedir_getpage(self->s_pd,address2);
     if __unlikely(!page || !page->present) goto end_futexset_fault;
     if __unlikely(!page->read_write) {
      /* Try to touch the secondary futex page. */
      if __unlikely(!kshm_touch_unlocked((struct kshm *)self,
                                         (void *)alignd((uintptr_t)address2,PAGEALIGN),1,
                                          KSHMREGION_FLAG_READ|KSHMREGION_FLAG_WRITE
                    )) goto end_futexset_fault;
      assert(page == kpagedir_getpage(self->s_pd,address2));
     }
    }
quit_futexset_and_reload:
    /* Must reload the futex set in case something was unmapped. */
    quit_futexset(&set);
    goto reload_futexset;
   }
  }
  user_futex2_is_reference = user_futex2 == NULL;
  addr2_partof_set = !user_futex2_is_reference;
  if (user_futex2_is_reference) {
   /* Must manually lookup the secondary futex. */
   error = kshm_getfutex_unlocked(self,address2,&user_futex2,0);
   if __unlikely(KE_ISERR(error)) {
    /* Check for special case: Secondary futex doesn't exist. */
    if (error != KE_NOENT) goto end_futexset;
    user_futex2_is_reference = 0;
    assert(!user_futex2);
   } else {
    kassert_kshmfutex(user_futex2);
   }
   /* Translate the secondary futex.
    * NOTE: In the rare even that we need to touch its page, we can't take
    *       any chances and _have_ to reload the sigset. */
   {
    kpage_t *page = kpagedir_getpage(self->s_pd,address2);
    if __likely(page && page->present) {
     if __unlikely(!page->read_write) {
      /* Try to touch the page. */
      if __unlikely(!kshm_touch_unlocked((struct kshm *)self,
                                         (void *)alignd((uintptr_t)address2,PAGEALIGN),1,
                                          KSHMREGION_FLAG_READ|KSHMREGION_FLAG_WRITE
                    )) goto end_futexset_fault;
      assert(page == kpagedir_getpage(self->s_pd,address2));
      goto quit_futexset_and_reload;
     }
     page->dirty = 1;
     *(uintptr_t *)&kernel_futex2  = x86_pte_getpptr(page);
     *(uintptr_t *)&kernel_futex2 |= X86_VPTR_GET_POFF(address2);
    }
   }
  } else {
   __kernel kfutex_t *writable_kernel_futex2;
   assert(kernel_futex2);
   /* The secondary futex was included as part of the user-given
    * futex set. - Make sure we're not trying to send anything on it. */
   if (secondary_opcode != _KFUTEX_CMD_C0) {
    /* ERROR: Can't send & receive the same futex during the same atomic operation. */
    error = KE_INVAL;
    goto end_futexset;
   }
   writable_kernel_futex2 = (__kernel kfutex_t *)kshm_qtranslateuser(self,self->s_pd,address2,1);
   if __unlikely(!writable_kernel_futex2) { end_futexset_fault: error = KE_FAULT; goto end_futexset; } /* Shouldn't happen. */
   if (writable_kernel_futex2 != kernel_futex2) {
    /* Must handle special case: The futex2 address wasn't writable due to COW,
     *                           yet now that now was invoked, its physical address has changed,
     *                           meaning that the physical address of any number of other
     *                           futex words may have also changed.
     *                        >> Sadly this means that we must reload _everything_...
     */
    goto quit_futexset_and_reload;
   }
  }
 } else {
  user_futex2              = NULL;
  kernel_futex2            = NULL;
  addr2_partof_set         = 0;
  user_futex2_is_reference = 0;
 }
#define has_address2_partof_set   (addr2_partof_set)
#define has_address2_value_loaded (addr2_partof_set) /* If it's part of the set, its value was already loaded. */

 /* Lock all 'dem signals! */
 if (user_futex2 && !has_address2_partof_set) {
  /* Must also lock the secondary futex. */
again_load_secondary_futex:
  ksignal_lock_c(&user_futex2->f_sig,KSIGNAL_LOCK_WAIT);
  if (!ksigset_trylocks_c(&set.kfs_sigset,KSIGNAL_LOCK_WAIT)) {
   ksignal_unlock_c(&user_futex2->f_sig,KSIGNAL_LOCK_WAIT);
   goto again_load_secondary_futex;
  }
 } else {
  ksigset_locks_c(&set.kfs_sigset,KSIGNAL_LOCK_WAIT);
 }
 /* This is it! This is where all the atomic magic happens! */

 /* Step #1: Check all the conditions that must apply atomically.
  *       >> Since we're locking all signals in question,
  *          everything here will appear atomic to the user. */
reload_primary_conditions:
 set_iter = &set;
 do {
  futex_op = (set_iter->kfs_op&_KFUTEX_RECV_OPMASK) >> _KFUTEX_RECV_OPSHIFT;
  /* Minor optimizations for special cases. */
  if __unlikely(futex_op == KFUTEX_FALSE) goto end_EAGAIN;
  if           (futex_op == KFUTEX_TRUE) goto next_set;
  futex_val = set_iter->kfs_val;
  kernel_futex_end = (kernel_futex_iter = (__kernel kfutex_t **)set_iter->kfs_addrv)+set_iter->kfs_sigset.ss_elemcnt;
  for (; kernel_futex_iter != kernel_futex_end; ++kernel_futex_iter) {
   unsigned int futex_word = katomic_load(*(unsigned int *)*kernel_futex_iter);
   if (*kernel_futex_iter == kernel_futex2) {
    assert(has_address2_partof_set);
    k_syslogf(KLOG_INFO,"Found addr2 at %p\n",(unsigned int *)*kernel_futex_iter);
    old_addr2_word = futex_word;
   }
   switch (futex_op) {
    case KFUTEX_EQUAL        : if (!(futex_word == futex_val)) goto end_EAGAIN; break; /* 'if (*uaddr == val) { ... wait(); }' */
    case KFUTEX_LOWER        : if (!(futex_word < futex_val)) goto end_EAGAIN; break; /* 'if (*uaddr < val) { ... wait(); }' */
    case KFUTEX_LOWER_EQUAL  : if (!(futex_word <= futex_val)) goto end_EAGAIN; break; /* 'if (*uaddr <= val) { ... wait(); }' */
    case KFUTEX_AND          : if (!(futex_word & futex_val)) goto end_EAGAIN; break; /* 'if ((*uaddr & val) != 0) { ... wait(); }' */
    case KFUTEX_XOR          : if (!(futex_word ^ futex_val)) goto end_EAGAIN; break; /* 'if ((*uaddr ^ val) != 0) { ... wait(); }' */
    case KFUTEX_SHL          : if (!(futex_word << futex_val)) goto end_EAGAIN; break; /* 'if ((*uaddr << val) != 0) { ... wait(); }' */
    case KFUTEX_SHR          : if (!(futex_word >> futex_val)) goto end_EAGAIN; break; /* 'if ((*uaddr >> val) != 0) { ... wait(); }' */
    case KFUTEX_NOT_EQUAL    : if (!(futex_word != futex_val)) goto end_EAGAIN; break; /* 'if (*uaddr != val) { ... wait(); }' */
    case KFUTEX_GREATER_EQUAL: if (!(futex_word >= futex_val)) goto end_EAGAIN; break; /* 'if (*uaddr >= val) { ... wait(); }' */
    case KFUTEX_GREATER      : if (!(futex_word > futex_val)) goto end_EAGAIN; break; /* 'if (*uaddr > val) { ... wait(); }' */
    case KFUTEX_NOT_AND      : if ((futex_word & futex_val)) goto end_EAGAIN; break; /* 'if ((*uaddr & val) == 0) { ... wait(); }' */
    case KFUTEX_NOT_XOR      : if ((futex_word ^ futex_val)) goto end_EAGAIN; break; /* 'if ((*uaddr ^ val) == 0) { ... wait(); }' */
    case KFUTEX_NOT_SHL      : if ((futex_word << futex_val)) goto end_EAGAIN; break; /* 'if ((*uaddr << val) == 0) { ... wait(); }' */
    case KFUTEX_NOT_SHR      : if ((futex_word >> futex_val)) goto end_EAGAIN; break; /* 'if ((*uaddr >> val) == 0) { ... wait(); }' */
    default: break;
   }
  }
next_set:;
 } while ((set_iter = (struct kernel_futexset *)set_iter->kfs_next) != NULL);

 /* Step #2: [OPT] Perform the secondary memory-modifying
  *                operation and signal the futex at 'address2'. */
 switch (secondary_opcode) {
  {
   unsigned int new_addr2_word;
  case _KFUTEX_CMD_C0:
  case _KFUTEX_CMD_C1:
  case _KFUTEX_CMD_CX:
   /* NOTE: The code above will have acquired write-access to the
    *       address2 futex word, meaning that we are free to modify it. */
   if (!user_futex2 || !has_address2_value_loaded) {
    /* If primary conditions didn't already load the secondary futex word, load it now. */
reload_secondary_word:
    old_addr2_word = katomic_load(*(unsigned int *)kernel_futex2);
   }
   /* Perform the secondary operation. */
   switch ((secondary_op & _KFUTEX_CCMD_OPMASK) >> _KFUTEX_CCMD_OPSHIFT) {
    default            : new_addr2_word = val2; break; /* '*uaddr2 = val2'. */
    case KFUTEX_ADD    : new_addr2_word = old_addr2_word+val2; break; /* '*uaddr2 += val2' */
    case KFUTEX_SUB    : new_addr2_word = old_addr2_word-val2; break; /* '*uaddr2 -= val2' */
    case KFUTEX_MUL    : new_addr2_word = old_addr2_word*val2; break; /* '*uaddr2 *= val2' */
    case KFUTEX_DIV    : new_addr2_word = old_addr2_word/val2; break; /* '*uaddr2 /= val2' */
    case KFUTEX_AND    : new_addr2_word = old_addr2_word&val2; break; /* '*uaddr2 &= val2' */
    case KFUTEX_OR     : new_addr2_word = old_addr2_word|val2; break; /* '*uaddr2 |= val2' */
    case KFUTEX_XOR    : new_addr2_word = old_addr2_word^val2; break; /* '*uaddr2 ^= val2' */
    case KFUTEX_SHL    : new_addr2_word = old_addr2_word<<val2; break; /* '*uaddr2 <<= val2' */
    case KFUTEX_SHR    : new_addr2_word = old_addr2_word>>val2; break; /* '*uaddr2 >>= val2' */
    case KFUTEX_NOT    : new_addr2_word = ~(val2); break; /* '*uaddr2 = ~(val2)' */
    case KFUTEX_NOT_ADD: new_addr2_word = ~(old_addr2_word+val2); break; /* '*uaddr2 = ~(*uaddr2 + val2)' */
    case KFUTEX_NOT_SUB: new_addr2_word = ~(old_addr2_word-val2); break; /* '*uaddr2 = ~(*uaddr2 - val2)' */
    case KFUTEX_NOT_MUL: new_addr2_word = ~(old_addr2_word*val2); break; /* '*uaddr2 = ~(*uaddr2 * val2)' */
    case KFUTEX_NOT_DIV: new_addr2_word = ~(old_addr2_word/val2); break; /* '*uaddr2 = ~(*uaddr2 / val2)' */
    case KFUTEX_NOT_AND: new_addr2_word = ~(old_addr2_word&val2); break; /* '*uaddr2 = ~(*uaddr2 & val2)' */
    case KFUTEX_NOT_OR : new_addr2_word = ~(old_addr2_word|val2); break; /* '*uaddr2 = ~(*uaddr2 | val2)' */
    case KFUTEX_NOT_XOR: new_addr2_word = ~(old_addr2_word^val2); break; /* '*uaddr2 = ~(*uaddr2 ^ val2)' */
    case KFUTEX_NOT_SHL: new_addr2_word = ~(old_addr2_word<<val2); break; /* '*uaddr2 = ~(*uaddr2 << val2)' */
    case KFUTEX_NOT_SHR: new_addr2_word = ~(old_addr2_word>>val2); break; /* '*uaddr2 = ~(*uaddr2 >> val2)' */
   }
   /* Apply the new futex word. */
   k_syslogf(KLOG_TRACE,"[FUTEX] Update word at %p:%p: %u -> %u\n",
             address2,kernel_futex2,old_addr2_word,new_addr2_word);
   if (!katomic_cmpxch(*(unsigned int *)kernel_futex2,old_addr2_word,new_addr2_word)) {
    if (has_address2_partof_set) {
     /* Must reload primary conditions because the secondary word depends on them. */
     k_syslogf(KLOG_DEBUG,"[FUTEX] Reload primary conditions cmpxch(%x,%x,%x)\n",
               *(unsigned int *)kernel_futex2,old_addr2_word,new_addr2_word);
     goto reload_primary_conditions;
    } else {
     /* Must only reload the secondary codeword. */
     goto reload_secondary_word;
    }
   }
   assertf(!has_address2_partof_set ||
           secondary_opcode == _KFUTEX_CMD_C0,
           "Can only signal a secondary futex if it's not part of the primary set");
   if (user_futex2) {
    /* Signal or unlock the secondary futex. */
    if (secondary_opcode == _KFUTEX_CMD_C1) {
     k_syslogf(KLOG_DEBUG,"[FUTEX] send(%p)\n",address2);
     while (_ksignal_sendone_andunlock_c(&user_futex2->f_sig) == KE_DESTROYED);
    } else if (secondary_opcode == _KFUTEX_CMD_CX) {
     k_syslogf(KLOG_DEBUG,"[FUTEX] send(%p)\n",address2);
     _ksignal_sendall_andunlock_c(&user_futex2->f_sig);
    } else if (!has_address2_partof_set) {
     ksignal_unlock_c(&user_futex2->f_sig,KSIGNAL_LOCK_WAIT);
    }
   }
  } break;
  default: break;
 }
 krwlock_end(&self->s_lock); /* Could be a read- or a write-lock. */

 /* Step #3: Begin receiving everything from the generated sigset. */
 error = buf ? (abstime  // TODO: vrecv
  ?  _ksigset_vtimedrecvs_andunlock_c(&set.kfs_sigset,&kernel_abstime,buf)
  :       _ksigset_vrecvs_andunlock_c(&set.kfs_sigset,buf)) : (abstime
  ?   _ksigset_timedrecvs_andunlock_c(&set.kfs_sigset,&kernel_abstime)
  :        _ksigset_recvs_andunlock_c(&set.kfs_sigset));
 if (user_futex2_is_reference) kshmfutex_decref(user_futex2);
 quit_futexset(&set);
 return error;
end_EAGAIN: error = KE_AGAIN;
 k_syslogf(KLOG_DEBUG,"[FUTEX] fail(EAGAIN)\n");
/*end_unlock_set:*/
 if (user_futex2 && !has_address2_partof_set) {
  ksignal_unlock_c(&user_futex2->f_sig,KSIGNAL_LOCK_WAIT);
 }
 ksigset_unlocks_c(&set.kfs_sigset,KSIGNAL_LOCK_WAIT);
/*end_user_futex2:*/ if (user_futex2_is_reference) kshmfutex_decref(user_futex2);
end_futexset:    quit_futexset(&set);
end_unlock:      krwlock_end(&self->s_lock); /* Could be a read- or a write-lock. */
 return error;
}

#undef V

__DECL_END

#endif /* !__KOS_KERNEL_SHM_FUTEX_C_INL__ */

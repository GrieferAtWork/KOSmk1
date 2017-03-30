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




#define FUTEXOP_MAYALLOCATE(opcode) ((opcode) != _KFUTEX_CMD_SEND)

__crit kerrno_t
kshm_futex(struct kshm *__restrict self,
           __user void *address, unsigned int futex_op, unsigned int val,
           __user void *buf, __user struct timespec const *abstime,
           __kernel unsigned int *woken_tasks,
           __user void *address2, unsigned int val2) {
 kerrno_t error; int opcode,has_write_lock = 0;
 __ref struct kshmfutex *user_futex,*user_futex2;
 KTASK_CRIT_MARK
 kassert_kshm(self);
 error = krwlock_beginread(&self->s_lock);
 if __unlikely(KE_ISERR(error)) return error;
 /* Make sure the given pointer is properly aligned (misalignments are silently ignored).
  * TODO: Shouldn't we just fail if it's misaligned? (But with what error...) */
 *(uintptr_t *)&address &= ~((uintptr_t)sizeof(unsigned int)-1);
 *(uintptr_t *)&address2 &= ~((uintptr_t)sizeof(unsigned int)-1);
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
   size_t kernel_futex_size;
  case _KFUTEX_CMD_C1:
  case _KFUTEX_CMD_CX:
  case _KFUTEX_CMD_RECV:
  case _KFUTEX_CMD_C0:
user_lock_again:
   kernel_futex = (unsigned int *)kshm_translateuser(self,self->s_pd,address,
                                                     sizeof(unsigned int),
                                                    &kernel_futex_size,0);
   if __unlikely(!kernel_futex) { error = KE_FAULT; break; } /* Really shouldn't happen... */
   assertf(kernel_futex_size == sizeof(unsigned int),
           "This should always be the case due to the alignment fix above!");
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
    has_write_lock = 1;
    if (error == KS_UNLOCKED) goto user_lock_again;
    kernel_futex2 = (unsigned int *)kshm_translateuser(self,self->s_pd,address2,
                                                       sizeof(unsigned int),
                                                      &kernel_futex_size,1);
    /* Really shouldn't happen... */
    if __unlikely(!kernel_futex2) {
err_futex2_fault:
     error = KE_FAULT;
err_futex2:
     if (user_futex2) kshmfutex_decref(user_futex2);
     break;
    }
    assertf(kernel_futex_size == sizeof(unsigned int),
            "This should always be the case due to the alignment fix above!");
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
      has_write_lock ? krwlock_endwrite(&self->s_lock) : krwlock_endread(&self->s_lock);
      has_write_lock = 0;
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
   has_write_lock ? krwlock_endwrite(&self->s_lock) : krwlock_endread(&self->s_lock);
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
 has_write_lock ? krwlock_endwrite(&self->s_lock) : krwlock_endread(&self->s_lock);
 return error;
}

__DECL_END

#endif /* !__KOS_KERNEL_SHM_FUTEX_C_INL__ */

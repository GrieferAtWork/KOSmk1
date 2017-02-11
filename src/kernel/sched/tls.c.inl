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
#ifndef __KOS_KERNEL_SCHED_TLS_C_INL__
#define __KOS_KERNEL_SCHED_TLS_C_INL__ 1

#include <kos/compiler.h>
#include <kos/kernel/tls.h>
#include <kos/kernel/task.h>

__DECL_BEGIN

kerrno_t ktlsman_initcopy(struct ktlsman *self,
                          struct ktlsman const *right) {
 kassertobj(self);
 kassert_ktlsman(right);
 kobject_init(self,KOBJECT_MAGIC_TLSMAN);
 self->tls_usedv = (__u8 *)memdup(right->tls_usedv,(right->tls_usedc/8)*sizeof(__u8));
 if __unlikely(!self->tls_usedv) return KE_NOMEM;
 self->tls_cnt   = right->tls_cnt;
 self->tls_max   = right->tls_max;
 self->tls_usedc = right->tls_usedc;
 self->tls_free  = right->tls_free;
 return KE_OK;
}

#define GETBIT(i)   (self->tls_usedv[(i)/8] & (1 << ((i)%8)))
#define SETBIT(i)   (self->tls_usedv[(i)/8] |= (1 << ((i)%8)))
#define UNSETBIT(i) (self->tls_usedv[(i)/8] &= ~(1 << ((i)%8)))

kerrno_t ktlsman_alloctls(struct ktlsman *__restrict self,
                          __ktls_t *__restrict result) {
 size_t index; __u8 *newvec;
 kassert_ktlsman(self);
 kassertobj(result);
 if __unlikely(self->tls_cnt == self->tls_max) return KE_ACCES;
 index = self->tls_free;
 while (index < self->tls_usedc) {
  if (!GETBIT(index)) {
   *result = index;
   ++self->tls_cnt;
   SETBIT(index);
   return KE_OK;
  }
  ++index;
 }
 index = 0;
 while (index < self->tls_free) {
  if (!GETBIT(index)) {
   *result = index;
   ++self->tls_cnt;
   SETBIT(index);
   return KE_OK;
  }
  ++index;
 }
 assert(self->tls_usedc == self->tls_cnt);
 // Must allocate more TLS slots
 index = self->tls_usedc++;
 self->tls_free = index+1;
 if (!(index%8)) {
  size_t vecslot = index/8;
  // Need a new vector slot
  newvec = (__u8 *)realloc(self->tls_usedv,(vecslot+1)*sizeof(__u8));
  if __unlikely(!newvec) return KE_NOMEM;
  self->tls_usedv = newvec;
  newvec[vecslot] = 0x01;
 } else {
  SETBIT(index);
 }
 *result = index;
 ++self->tls_cnt;
 return KE_OK;
}

void ktlsman_freetls(struct ktlsman *__restrict self, __ktls_t slot) {
 kassert_ktlsman(self);
 if (slot >= self->tls_usedc || !GETBIT(slot)) return;
 UNSETBIT(slot);
 --self->tls_cnt;
 self->tls_free = slot;
 if (slot == self->tls_usedc-1) {
  while (slot && !GETBIT(slot-1)) --slot;
  if (slot) {
   size_t oldsize = (self->tls_usedc/8)+1;
   size_t newsize = (slot/8)+1;
   if (oldsize != newsize) {
    __u8 *newvec = (__u8 *)realloc(self->tls_usedv,
                                   newsize*sizeof(__u8));
    if __likely(newvec) self->tls_usedv = newvec;
   }
  } else {
   free(self->tls_usedv);
   self->tls_usedv = NULL;
  }
  self->tls_usedc = slot;
 }
}

int ktlsman_validtls(struct ktlsman const *__restrict self, __ktls_t slot) {
 kassert_ktlsman(self);
 return GETBIT(slot);
}



__crit kerrno_t ktlspt_initcopy(struct ktlspt *__restrict self,
                                struct ktlspt const *__restrict right) {
 KTASK_CRIT_MARK
 kassertobj(self);
 kassert_ktlspt(right);
 kobject_init(self,KOBJECT_MAGIC_TLSPT);
 if ((self->tpt_vecc = right->tpt_vecc) != 0) {
  self->tpt_vecv = (void **)memdup(right->tpt_vecv,
                                   self->tpt_vecc*sizeof(void *));
  if __unlikely(!self->tpt_vecv) return KE_NOMEM;
 } else {
  self->tpt_vecv = NULL;
 }
 return KE_OK;
}

void ktlspt_del(struct ktlspt *__restrict self,
                __ktls_t slot) {
 size_t newsize; void **newvec;
 kassert_ktlspt(self);
 if (slot >= self->tpt_vecc) return;
 if (slot != self->tpt_vecc-1) {
  self->tpt_vecv[slot] = NULL;
 }
 newsize = slot;
 while (newsize && !self->tpt_vecv[newsize-1]) --newsize;
 if (!newsize) {
  free(self->tpt_vecv);
  self->tpt_vecc = 0;
  self->tpt_vecv = NULL;
 } else {
  newvec = (void **)realloc(self->tpt_vecv,
                            newsize*sizeof(void *));
  if __likely(newvec) {
   self->tpt_vecc = newsize;
   self->tpt_vecv = newvec;
  }
 }
}
kerrno_t ktlspt_set(struct ktlspt *__restrict self,
                    __ktls_t slot, void *value) {
 void **newvec;
 kassert_ktlspt(self);
 if (slot < self->tpt_vecc) {
  self->tpt_vecv[slot] = value;
  return KS_FOUND;
 }
 // Must allocate vector space for this slot
 newvec = (void **)realloc(self->tpt_vecv,
                          (slot+1)*sizeof(void *));
 if __unlikely(!newvec) return KE_NOMEM;
 memset(newvec+self->tpt_vecc,0,
       (self->tpt_vecc-slot)*sizeof(void *));
 newvec[slot] = value;
 self->tpt_vecc = slot+1;
 self->tpt_vecv = newvec;
 return KE_OK;
}

__DECL_END

#endif /* !__KOS_KERNEL_SCHED_TLS_C_INL__ */

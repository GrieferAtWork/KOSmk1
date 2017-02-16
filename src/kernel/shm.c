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

#include <kos/config.h>
#ifdef __KERNEL__
#include <assert.h>
#include <kos/syslog.h>
#include <kos/kernel/pageframe.h>
#include <kos/kernel/paging.h>
#include <kos/kernel/shm.h>
#include <malloc.h>
#include <math.h>
#include <stddef.h>
#include <kos/kernel/task.h>
#include <kos/kernel/proc.h>

__DECL_BEGIN

kpageflag_t kshmtab_getpageflags(struct kshmtab const *__restrict self) {
#ifdef X86_PTE_FLAG_PRESENT
 kpageflag_t result = X86_PTE_FLAG_PRESENT;
#else
 kpageflag_t result = 0;
#endif
 // TODO: The map flags must also depend on uniqueness of
 //       the given tab ('tab->mt_refcnt > 1'; aka. copy-on-write)
 if (!(self->mt_flags&KSHMTAB_FLAG_K)) result |= PAGEDIR_FLAG_USER;
 if ((self->mt_flags&KSHMTAB_FLAG_W)
#ifdef KCONFIG_HAVE_SHM_COPY_ON_WRITE
  && katomic_load(self->mt_refcnt) == 1
#endif
     ) result |= PAGEDIR_FLAG_READ_WRITE;
 return result;
}

__local kerrno_t kpagedir_mapscatter(struct kpagedir *__restrict self,
                                     struct kshmtabscatter *__restrict scatter,
                                     __user void *destaddr, kpageflag_t flags) {
 struct kshmtabscatter *iter; uintptr_t dest; kerrno_t error;
 kassertobj(self); kassertobj(scatter);
 assert(isaligned((uintptr_t)destaddr,PAGEALIGN));
 dest = (uintptr_t)destaddr; iter = scatter; do {
  assert(!kpagedir_ismapped(self,(void *)dest,iter->ts_pages));
  error = kpagedir_remap(self,iter->ts_addr,(void *)dest,iter->ts_pages,flags);
  if __unlikely(KE_ISERR(error)) {
   dest = (uintptr_t)destaddr;
   while (scatter != iter) {
    kpagedir_unmap(self,(void *)dest,scatter->ts_pages);
    dest += scatter->ts_pages*PAGESIZE;
    scatter = scatter->ts_next;
   }
   return error;
  }
  dest += iter->ts_pages*PAGESIZE;
 } while ((iter = iter->ts_next) != NULL);
 return KE_OK;
}


kerrno_t kshmtabscatter_init(struct kshmtabscatter *__restrict self, size_t pages) {
 size_t part; struct kshmtabscatter *next,*iter,*last;
 kassertobj(self);
 self->ts_addr = kpageframe_tryalloc(pages,&part);
 if __unlikely(!self->ts_addr) return KE_NOMEM;
 self->ts_pages = part;
 assert(part <= pages);
 if __likely(part == pages) self->ts_next = NULL;
 else {
  // Try to allocate as a scattered SHM tab.
  last = self;
  for (;;) {
   pages -= part;
   next = omalloc(struct kshmtabscatter);
   if __unlikely(!next) goto nomem;
   next->ts_addr = kpageframe_tryalloc(pages,&part);
   if __unlikely(!next->ts_addr) { free(next); goto nomem; }
   assert(part <= pages);
   next->ts_pages = part;
   last->ts_next = next;
   if __likely(part == pages) {
    next->ts_next = NULL;
    break;
   }
   last = next;
  }
 }
 return KE_OK;
nomem:
 for (iter = self;;) {
  next = iter->ts_next;
  kpageframe_free((struct kpageframe *)iter->ts_addr,iter->ts_pages);
  if (iter != self) free(iter);
  if (iter == last) break;
  assertf(next,"NULL link before last");
  iter = next;
 }
 return KE_NOMEM;
}
kerrno_t kshmtabscatter_init_linear(struct kshmtabscatter *__restrict self, size_t pages) {
 kassertobj(self);
 self->ts_addr = kpageframe_alloc(pages);
 if __unlikely(!self->ts_addr) return KE_NOMEM;
 self->ts_next = NULL;
 self->ts_pages = pages;
 return KE_OK;
}

void kshmtabscatter_quit(struct kshmtabscatter *__restrict self) {
 struct kshmtabscatter *next_scatter;
 kassertobj(self);
 assert(isaligned((uintptr_t)self->ts_addr,PAGEALIGN));
 k_syslogf(KLOG_TRACE,"Free scatter at physical address %p (%Iu pages)\n",self->ts_addr,self->ts_pages);
 kpageframe_free((struct kpageframe *)self->ts_addr,self->ts_pages);
 if ((self = self->ts_next) != NULL) do {
  next_scatter = self->ts_next;
  kpageframe_free((struct kpageframe *)self->ts_addr,self->ts_pages);
  free(self);
  self = next_scatter;
 } while (self);
}

int kshmtabscatter_contains(struct kshmtabscatter *__restrict self,
                            __kernel void *addr) {
 kassertobj(self);
 do {
  assert(isaligned((uintptr_t)self->ts_addr,PAGEALIGN));
  if ((uintptr_t)addr >= (uintptr_t)self->ts_addr &&
      (uintptr_t)addr < ((uintptr_t)self->ts_addr+self->ts_pages*PAGESIZE)
      ) return 1;
 } while ((self = self->ts_next) != NULL);
 return 0;
}
void kshmtabscatter_copybytes(struct kshmtabscatter const *__restrict dst,
                              struct kshmtabscatter const *__restrict src) {
 uintptr_t dst_iter,src_iter;
 size_t dst_size,src_size,copy_size;
 kassertobj(dst); kassertobj(src);
 assert(dst != src);
 dst_iter = (uintptr_t)dst->ts_addr;
 src_iter = (uintptr_t)src->ts_addr;
 dst_size = dst->ts_pages*PAGESIZE;
 src_size = src->ts_pages*PAGESIZE;
 assert(dst_size >= dst->ts_pages);
 assert(src_size >= src->ts_pages);
 for (;;) {
  copy_size = min(dst_size,src_size);
  assert(copy_size);
  memcpy((void *)dst_iter,(void *)src_iter,copy_size);
  if ((dst_size -= copy_size) == 0) {
   dst = dst->ts_next;
   if __unlikely(!dst) {
    assertf(src_size == copy_size && !src->ts_next
            ,"The given SHM tabs are not of equal size! (%Iu, %Iu, %p)"
            ,src_size,copy_size,src->ts_next);
    return;
   }
   dst_iter = (uintptr_t)dst->ts_addr;
   dst_size = dst->ts_pages*PAGESIZE;
   assert(dst_size >= dst->ts_pages);
  } else dst_iter += copy_size;
  if ((src_size -= copy_size) == 0) {
   src = src->ts_next;
   assertf(src,"The given SHM tabs are not of equal size!");
   src_iter = (uintptr_t)src->ts_addr;
   src_size = src->ts_pages*PAGESIZE;
   assert(src_size >= src->ts_pages);
  } else src_iter += copy_size;
 }
}

__kernel void *__kshmtab_translate_offset(struct kshmtab const *__restrict self,
                                          uintptr_t offset, size_t *maxbytes) {
 struct kshmtabscatter const *scatter;
 size_t scatter_size;
 kassert_kshmtab(self);
 kassertobj(maxbytes);
 scatter = &self->mt_scat;
 do {
  scatter_size = scatter->ts_pages*PAGESIZE;
  if (offset < scatter_size) {
   *maxbytes = scatter_size-offset;
   return (__kernel void *)((uintptr_t)scatter->ts_addr+offset);
  }
  offset -= scatter_size;
 } while ((scatter = scatter->ts_next) != NULL);
 return NULL;
}







extern void kshmtab_destroy(struct kshmtab *__restrict self);
void kshmtab_destroy(struct kshmtab *__restrict self) {
 if (!(self->mt_flags&KSHMTAB_FLAG_D)) {
  kshmtabscatter_quit(&self->mt_scat);
 }
 free(self);
}

__ref struct kshmtab *kshmtab_newram(size_t pages, __u32 flags) {
 __ref struct kshmtab *result;
 result = omalloc(__ref struct kshmtab);
 if __unlikely(!result) return NULL;
 kobject_init(result,KOBJECT_MAGIC_SHMTAB);
 result->mt_refcnt = 1;
 result->mt_flags = flags;
 result->mt_pages = pages;
 if __unlikely(KE_ISERR(kshmtabscatter_init(&result->mt_scat,pages))) {
  free(result);
  result = NULL;
 }
 return result;
}
__ref struct kshmtab *kshmtab_newram_linear(size_t pages, __u32 flags) {
 __ref struct kshmtab *result;
 result = omalloc(__ref struct kshmtab);
 if __unlikely(!result) return NULL;
 kobject_init(result,KOBJECT_MAGIC_SHMTAB);
 result->mt_refcnt = 1;
 result->mt_flags = flags;
 result->mt_pages = pages;
 if __unlikely(KE_ISERR(kshmtabscatter_init_linear(&result->mt_scat,pages))) {
  free(result);
  result = NULL;
 }
 return result;
}
__ref struct kshmtab *kshmtab_newfp(struct kfile *__restrict fp, __u64 offset,
                                    size_t bytes, __u32 flags) {
 k_syslogf(KLOG_ERROR,"NOT IMPLEMENTED: mmap(file)\n");
 (void)fp,(void)offset;
 return kshmtab_newram(ceildiv(bytes,PAGESIZE),flags);
}
__ref struct kshmtab *kshmtab_newdev(__pagealigned __kernel void *start, size_t pages, __u32 flags) {
 __ref struct kshmtab *result;
 assert(isaligned((uintptr_t)start,PAGEALIGN));
 result = omalloc(__ref struct kshmtab);
 if __unlikely(!result) return NULL;
 kobject_init(result,KOBJECT_MAGIC_SHMTAB);
 result->mt_refcnt        = 1;
 result->mt_flags         = flags|KSHMTAB_FLAG_D;
 result->mt_pages         = pages;
 result->mt_scat.ts_addr  = start;
 result->mt_scat.ts_pages = pages;
 result->mt_scat.ts_next  = NULL;
 return result;
}

__ref struct kshmtab *kshmtab_copy(struct kshmtab const *__restrict self) {
 __ref struct kshmtab *result;
 kassert_kshmtab(self);
 result = omalloc(__ref struct kshmtab);
 if __unlikely(!result) return NULL;
 kobject_init(result,KOBJECT_MAGIC_SHMTAB);
 result->mt_refcnt = 1;
 result->mt_flags = self->mt_flags;
 result->mt_pages = self->mt_pages;
 if __unlikely(KE_ISERR(kshmtabscatter_init(&result->mt_scat,self->mt_pages))) {
  free(result);
  return NULL;
 }
 // Now just copy the data!
 kshmtabscatter_copybytes(&result->mt_scat,&self->mt_scat);
 return result;
}

kerrno_t kshmtab_unique(__ref struct kshmtab **__restrict self) {
 struct kshmtab *oldtab,*newtab;
 kassertobj(self); oldtab = *self;
 kassert_kshmtab(oldtab);
 if (katomic_load(oldtab->mt_refcnt) > 1) {
  // Must copy this tab
  newtab = kshmtab_copy(oldtab);
  if __unlikely(!newtab) return KE_NOMEM;
  assert(newtab->mt_refcnt == 1);
  *self = newtab;
  kshmtab_decref(oldtab);
  return KE_OK;
 }
 return KS_UNCHANGED;
}


static kerrno_t kshm_initldt(struct kshm *self, __u16 size_hint);
static kerrno_t kshm_initldtcopy(struct kshm *self, struct kshm *right);
static void kshm_quitldt(struct kshm *self);

kerrno_t kshm_init(struct kshm *__restrict self,
                   __u16 ldt_size_hint) {
 kerrno_t error;
 kassertobj(self);
 kobject_init(self,KOBJECT_MAGIC_SHM);
 self->sm_pd = kpagedir_new();
 if __unlikely(!self->sm_pd) return KE_NOMEM;
 if __unlikely(KE_ISERR(error = kpagedir_mapkernel(self->sm_pd,PAGEDIR_FLAG_READ_WRITE))) goto err_pd;
 if __unlikely(KE_ISERR(error = kshm_initldt(self,ldt_size_hint))) goto err_pd;
 memset(self->sm_groups,0,sizeof(self->sm_groups));
 self->sm_tabc = 0;
 self->sm_tabv = NULL;
 return KE_OK;
err_pd:
 kpagedir_delete(self->sm_pd);
 return error;
}

#ifdef __DEBUG__
#define KSHM_VALIDATE_GROUPS(self) \
 {\
  struct kshmtabentry *_viter,*_vend;\
  struct kshmtabentry **_giter,**_gend;\
  _vend = (_viter = self->sm_tabv)+self->sm_tabc;\
  for (; _viter != _vend; ++_viter) kassert_kshmtab(_viter->te_tab);\
  _gend = (_giter = (self)->sm_groups)+KSHM_GROUPCOUNT;\
  for (; _giter != _gend; ++_giter) {\
   assertf(!(*_giter)\
        || ((*_giter) >= (self)->sm_tabv\
        &&  (*_giter) <  (self)->sm_tabv+(self)->sm_tabc),\
           "*_giter                     = %p\n"\
           "self->sm_tabv               = %p\n"\
           "self->sm_tabv+self->sm_tabc = %p\n"\
           ,*_giter,(self)->sm_tabv,(self)->sm_tabv+(self)->sm_tabc);\
  }\
 }
#else
#define KSHM_VALIDATE_GROUPS(self) {}
#endif

__local void kshm_updategroups(struct kshm *__restrict self) {
 struct kshmtabentry *iter,*end;
 struct kshmtabentry **group;
 kassert_kshm(self);
 memset(self->sm_groups,0,sizeof(self->sm_groups));
 end = (iter = self->sm_tabv)+self->sm_tabc;
 for (; iter != end; ++iter) {
  kassert_kshmtab(iter->te_tab);
  group = self->sm_groups+KSHM_GROUPOF(iter->te_map);
  if (!*group) *group = iter;
  else {
   assertf((*group)->te_map != iter->te_map,"Double tab entry for %p",iter->te_map);
   assertf((*group)->te_map < iter->te_map,"Unsorted tabs");
  }
 }
 KSHM_VALIDATE_GROUPS(self)
}

#ifdef KCONFIG_HAVE_SHM_COPY_ON_WRITE
kerrno_t kshm_initfork(struct kshm *__restrict self,
                       struct kshm *__restrict right) {
 struct kshmtabentry *tabiter,*tabend;
 kerrno_t error;
 kassertobj(self);
 kassert_kshm(right);
 kobject_init(self,KOBJECT_MAGIC_SHM);
 self->sm_pd = kpagedir_copy(right->sm_pd);
 if __unlikely(!self->sm_pd) return KE_NOMEM;
 self->sm_tabc = right->sm_tabc;
 self->sm_tabv = (struct kshmtabentry *)memdup(right->sm_tabv,right->sm_tabc*
                                               sizeof(struct kshmtabentry));
 if __unlikely(!self->sm_tabv) { error = KE_NOMEM; err_pd: kpagedir_delete(self->sm_pd); return error; }
 tabend = (tabiter = self->sm_tabv)+self->sm_tabc;
 for (; tabiter != tabend; ++tabiter) {
  kassert_kshmtab(tabiter->te_tab);
  if (tabiter->te_tab->mt_flags&KSHMTAB_FLAG_K) {
   --tabend;
   memmove(tabiter,tabiter+1,(tabend-tabiter)*
           sizeof(struct kshmtabentry));
   --self->sm_tabc;
   continue;
  } else {
   if __unlikely(KE_ISERR(error = kshmtab_incref(tabiter->te_tab))) {
    if (tabiter->te_tab->mt_flags&KSHMTAB_FLAG_S) goto err_tabs;
    // We can actually try to recover from this by doing a hard copy
    tabiter->te_tab = kshmtab_copy(tabiter->te_tab);
    if __unlikely(!tabiter->te_tab) {
     error = KE_NOMEM; // No memory...
err_tabs:
     while (tabiter != self->sm_tabv) {
      --tabiter;
      kshmtab_decref(tabiter->te_tab);
     }
     free(self->sm_tabv);
     goto err_pd;
    }
    error = kpagedir_mapscatter(self->sm_pd,&tabiter->te_tab->mt_scat,tabiter->te_map,
                                kshmtab_getpageflags(tabiter->te_tab));
    if __unlikely(KE_ISERR(error)) { kshmtab_decref(tabiter->te_tab); goto err_tabs; }
   } else {
    // TODO: Remap this tab as read-only in 'self->sm_pd' and 'right->sm_pd'
    //    >> While that is fairly simple (only needing to remove the
    //       'PAGEDIR_FLAG_READ_WRITE' flag from all affected pages),
    //       the hard part is writing a #PF (Page Fault) handler capable
    //       of correctly copying the pages.
    //    >> OK. That shouldn't be too difficult either... (Yay!)
   }
  }
 }
 if (self->sm_tabc != right->sm_tabc) {
  // Some kernel tabs were not copied. Release memory that was used for them
  tabiter = (struct kshmtabentry *)realloc(self->sm_tabv,self->sm_tabc*
                                           sizeof(struct kshmtabentry));
  if __likely(tabiter) self->sm_tabv = tabiter;
 }
 // Finally, do a forced update the group cache
 kshm_updategroups(self);
 return KE_OK;
}
#endif

kerrno_t kshm_initcopy(struct kshm *__restrict self,
                       struct kshm *__restrict right) {
 struct kshmtabentry *tabiter,*tabend;
 kerrno_t error;
 kassertobj(self);
 kassert_kshm(right);
 kobject_init(self,KOBJECT_MAGIC_SHM);
 self->sm_pd = kpagedir_new();
 if __unlikely(!self->sm_pd) return KE_NOMEM;
 if __unlikely(KE_ISERR(error = kpagedir_mapkernel(self->sm_pd,PAGEDIR_FLAG_READ_WRITE))) goto err_pd;
 if __unlikely(KE_ISERR(error = kshm_initldtcopy(self,right))) goto err_pd;
 self->sm_tabc = right->sm_tabc;
 self->sm_tabv = (struct kshmtabentry *)memdup(right->sm_tabv,right->sm_tabc*
                                               sizeof(struct kshmtabentry));
 if __unlikely(!self->sm_tabv) {
  error = KE_NOMEM;
err_ldt:
  kshm_quitldt(self);
err_pd:
  kpagedir_delete(self->sm_pd);
  return error;
 }
 tabend = (tabiter = self->sm_tabv)+self->sm_tabc;
 while (tabiter != tabend) {
  kassert_kshmtab(tabiter->te_tab);
  if (tabiter->te_tab->mt_flags&KSHMTAB_FLAG_K) {
   --tabend;
   memmove(tabiter,tabiter+1,(tabend-tabiter)*
           sizeof(struct kshmtabentry));
   --self->sm_tabc;
   continue;
  } else {
   if ((tabiter->te_tab->mt_flags&KSHMTAB_FLAG_S) &&
       (tabiter->te_flags&MAP_SHARED)) {
    // This tab must be shared
    if __unlikely(KE_ISERR(error = kshmtab_incref(tabiter->te_tab))) {
err_tabs:
     while (tabiter != self->sm_tabv) {
      --tabiter;
      kshmtab_decref(tabiter->te_tab);
     }
     free(self->sm_tabv);
     goto err_ldt;
    }
   } else {
    // Copy the tab and its data
    tabiter->te_tab = kshmtab_copy(tabiter->te_tab);
    if __unlikely(!tabiter->te_tab) { error = KE_NOMEM; goto err_tabs; }
   }
   error = kpagedir_mapscatter(self->sm_pd,&tabiter->te_tab->mt_scat,tabiter->te_map,
                               kshmtab_getpageflags(tabiter->te_tab));
   if __unlikely(KE_ISERR(error)) { kshmtab_decref(tabiter->te_tab); goto err_tabs; }
  }
  ++tabiter;
 }
 if (self->sm_tabc != right->sm_tabc) {
  // Some kernel tabs were not copied. Release memory that was used for them
  tabiter = (struct kshmtabentry *)realloc(self->sm_tabv,self->sm_tabc*
                                           sizeof(struct kshmtabentry));
  if __likely(tabiter) self->sm_tabv = tabiter;
 }
 // Finally, do a forced update the group cache
 kshm_updategroups(self);
 return KE_OK;
}
void kshm_quit(struct kshm *__restrict self) {
 struct kshmtabentry *iter,*end;
 kassert_kshm(self);
 end = (iter = self->sm_tabv)+self->sm_tabc;
 for (; iter != end; ++iter) {
  kshmtab_decref(iter->te_tab);
 }
 free(self->sm_tabv);
 kshm_quitldt(self);
 kpagedir_delete(self->sm_pd);
}

__crit __local struct kshmtabentry *
kshm_allocslot(struct kshm *__restrict self,
               size_t index, __user void *uaddr) {
 struct kshmtabentry *newvec,*result;
 struct kshmtabentry **iter,**end; __u8 group;
 KTASK_CRIT_MARK
 assertf(index <= self->sm_tabc,"Invalid index");
 KSHM_VALIDATE_GROUPS(self);
 newvec = (struct kshmtabentry *)realloc(self->sm_tabv,(self->sm_tabc+1)*
                                         sizeof(struct kshmtabentry));
 if __unlikely(!newvec) return NULL;
 end = (iter = self->sm_groups)+KSHM_GROUPCOUNT;
 result = newvec+index;
 for (; iter != end; ++iter) if (*iter) {
  *iter = newvec+(*iter-self->sm_tabv);
  if (*iter >= result) ++*iter;
 }
 memmove(result+1,result,(self->sm_tabc-index)*
         sizeof(struct kshmtabentry));
 ++self->sm_tabc;
 self->sm_tabv = newvec;
 result->te_map = uaddr;
 group = KSHM_GROUPOF(uaddr);
 assert(group < KSHM_GROUPCOUNT);
 if (!self->sm_groups[group] ||
    (kassert_kshmtab(self->sm_groups[group]->te_tab)
    ,assertf(group == KSHM_GROUPOF(self->sm_groups[group]->te_map)
            ,"Group start for %I8u (address %p) has invalid address %p (group %I8u)"
            ,group,uaddr,self->sm_groups[group]->te_map
            ,KSHM_GROUPOF(self->sm_groups[group]->te_map)),
     self->sm_groups[group]->te_map > uaddr
     )) self->sm_groups[group] = result;
 assert(index < self->sm_tabc);
 assert(result == &self->sm_tabv[index]);
 return result;
}
__local struct kshmtabentry *kshm_findslot(struct kshm *__restrict self,
                                           struct kshmtab *__restrict tab,
                                           __user void *uaddr) {
 struct kshmtabentry *result,*end;
 __u8 group = KSHM_GROUPOF(uaddr);
 assert(group < KSHM_GROUPCOUNT);
 result = self->sm_groups[group];
 if __likely(result) {
  do if (result->te_tab == tab) return result;
  while ((++result,KSHM_GROUPOF(result->te_map) == group));
  // Rare case: 'uaddr' might point into the middle of a lower tab
  result = self->sm_tabv;
  end = self->sm_groups[group];
  for (; result != end; ++result) {
   if (result->te_tab == tab) return result;
  }
  return NULL;
 }
 end = (result = self->sm_tabv)+self->sm_tabc;
 for (; result != end && result->te_map <= uaddr; ++result) {
  if (result->te_tab == tab) return result;
 }
 return NULL;
}

size_t kshm_findindexof(struct kshm *__restrict self, __user void *addr) {
 struct kshmtabentry *groupslots,*slotend;
 __u8 group = KSHM_GROUPOF(addr);
 assert(group < KSHM_GROUPCOUNT);
 groupslots = self->sm_groups[group];
 slotend = self->sm_tabv+self->sm_tabc;
 if (groupslots) {
  // Insert into existing group
  while (addr > groupslots->te_map && groupslots != slotend) ++groupslots;
  return (size_t)(groupslots-self->sm_tabv);
 }
 // Find next group; insert before
 while (++group != KSHM_GROUPCOUNT && !self->sm_groups[group]);
 if (group == KSHM_GROUPCOUNT) return self->sm_tabc; // Append at the end
 return (size_t)(self->sm_groups[group]-self->sm_tabv);
}

__crit kerrno_t
kshm_instab_inherited(struct kshm *__restrict self,
                      struct kshmtab *__restrict tab,
                      int flags, __user void *hint,
                      __user void **mapped_address) {
 void *resaddr; struct kshmtabentry *result; size_t resindex;
 KTASK_CRIT_MARK
 kassert_kshm(self);
 kassert_kshmtab(tab);
 if (flags&MAP_FIXED) {
  // Make sure the given, fixed range isn't already mapped
  if __unlikely(kpagedir_ismapped(self->sm_pd,hint,tab->mt_pages)) return KE_RANGE;
  resaddr = hint;
 } else {
  resaddr = kpagedir_findfreerange(self->sm_pd,tab->mt_pages,hint);
  if __unlikely(!resaddr) return KE_NOSPC;
 }
 if __unlikely(KE_ISERR(kpagedir_mapscatter(self->sm_pd,&tab->mt_scat,resaddr,
                                            kshmtab_getpageflags(tab)))
               ) return KE_NOMEM;
 resindex = kshm_findindexof(self,resaddr);
 assert(resindex <= self->sm_tabc);
 result = kshm_allocslot(self,resindex,resaddr);
 if __unlikely(!result) {
  kpagedir_unmap(self->sm_pd,resaddr,tab->mt_pages);
  return KE_NOMEM;
 }
 assert(resindex < self->sm_tabc);
 assert(result == self->sm_tabv+resindex);
 result->te_flags = flags&(MAP_PRIVATE|MAP_SHARED);
 result->te_tab   = tab; // Inherit reference
 KSHM_VALIDATE_GROUPS(self);
 if (mapped_address) *mapped_address = result->te_map;
 return KE_OK;
}

__crit void kshm_delslot(struct kshm *__restrict self,
                         struct kshmtabentry *__restrict slot) {
 struct kshmtabentry **iter,**end,*newvec,*elem; __u8 group;
 KTASK_CRIT_MARK
 kassert_kshm(self); kassertobj(slot);
 assertf(slot >= self->sm_tabv && slot < self->sm_tabv+self->sm_tabc,
         "Slot %p (technical index %Iu) is out-of-bounds (base: %p; max: %Iu)",
         slot,slot-self->sm_tabv,self->sm_tabv,self->sm_tabc);
 kassert_kshmtab(slot->te_tab);
 assert(isaligned((uintptr_t)slot->te_map,PAGEALIGN));
 //printf("Unmapping SHM tab mapped to %p\n",slot->te_map);
 kpagedir_unmap(self->sm_pd,slot->te_map,slot->te_tab->mt_pages);
 if __unlikely(!--self->sm_tabc) {
  assert(slot == self->sm_tabv);
  memset(self->sm_groups,0,sizeof(self->sm_groups));
  free(self->sm_tabv);
  self->sm_tabv = NULL;
  return;
 }
 group = KSHM_GROUPOF(slot->te_map);
 memmove(slot,slot+1,(self->sm_tabc-(slot-self->sm_tabv))*
         sizeof(struct kshmtabentry));
 newvec = (struct kshmtabentry *)realloc(self->sm_tabv,self->sm_tabc*
                                         sizeof(struct kshmtabentry));
 if __unlikely(!newvec) newvec = self->sm_tabv;
 end = (iter = self->sm_groups)+KSHM_GROUPCOUNT;
 slot = newvec+(slot-self->sm_tabv);
 for (; iter != end; ++iter) if (*iter) {
  *iter = elem = newvec+(*iter-self->sm_tabv);
  if (elem >= slot) {
   --*iter;
   if (elem == slot && (*iter < newvec ||
      (kassert_kshmtab((*iter)->te_tab),
       KSHM_GROUPOF((*iter)->te_map) != group))
       ) *iter = NULL;
  }
  assertf(!*iter ||
         (*iter >= newvec &&
          *iter < newvec+self->sm_tabc)
         ,"slot                 = %p\n"
          "elem                 = %p\n"
          "*iter                = %p\n"
          "newvec               = %p\n"
          "newvec+self->sm_tabc = %p\n"
         ,slot,elem,*iter,newvec
         ,newvec+self->sm_tabc);
  assert(!*iter ||
        (KSHM_GROUPOF((*iter)->te_map) == (iter-self->sm_groups)));
 }
 self->sm_tabv = newvec;
 KSHM_VALIDATE_GROUPS(self)
}
__crit kerrno_t kshm_deltab(struct kshm *__restrict self,
                            struct kshmtab *__restrict tab,
                            __user void *uaddr) {
 struct kshmtabentry *slot;
 KTASK_CRIT_MARK
 kassert_kshm(self); kassert_kshmtab(tab);
 assert(kpagedir_ismappedp(self->sm_pd,uaddr));
 assert(kshmtab_contains(tab,kpagedir_translate(self->sm_pd,uaddr)));
 slot = kshm_findslot(self,tab,uaddr);
 if __unlikely(!slot) return KE_NOENT;
 assert(slot >= self->sm_tabv && slot < self->sm_tabv+self->sm_tabc);
 kassert_kshmtab(slot->te_tab);
 assert(uaddr >= slot->te_map &&
        uaddr < (void *)((uintptr_t)slot->te_map+slot->te_tab->mt_pages*PAGESIZE));
 kshm_delslot(self,slot);
 return KE_OK;
}

struct kshmtab *kshm_gettab(struct kshm *__restrict self,
                            __user void *uaddr) {
 struct kshmtabentry *entry;
 entry = kshm_getentry(self,uaddr);
 return entry ? entry->te_tab : NULL;
}
struct kshmtabentry *
kshm_getentry(struct kshm *__restrict self, __user void *uaddr) {
 __u8 group;
 struct kshmtabentry *iter;
 kassert_kshm(self);
 group = KSHM_GROUPOF(uaddr);
 assert(group < KSHM_GROUPCOUNT);
 do {
  iter = self->sm_groups[group];
  while (iter) {
   void *entryend;
   assert(KSHM_GROUPOF(iter->te_map) == group);
   if (iter->te_map > uaddr) break;
   entryend = (void *)((uintptr_t)iter->te_map+iter->te_tab->mt_pages*PAGESIZE);
   if (uaddr >= iter->te_map && uaddr < entryend) return iter;
   if (uaddr >= entryend) break;
  }
 } while (group--);
 return NULL;
}


__crit __user void *kshm_mmap(struct kshm *__restrict self, __user void *hint, size_t len,
                              int prot, int flags, struct kfile *fp, __u64 off) {
 struct kshmtab *tab; __user void *result;
 KTASK_CRIT_MARK
 assert(isaligned((uintptr_t)hint,PAGEALIGN));
 kassert_kshm(self);
 if (flags&MAP_ANONYMOUS || !fp)
  return kshm_mmapram(self,hint,len,prot,flags);
 tab = (flags&MAP_ANONYMOUS || !fp)
  ? kshmtab_newram(ceildiv(len,PAGESIZE),prot)
  : kshmtab_newfp(fp,off,len,prot);
 if __unlikely(!tab) return MAP_FAIL;
 if __unlikely(KE_ISERR(kshm_instab_inherited(self,tab,flags,hint,&result))
               ) { kshmtab_decref(tab); result = MAP_FAIL; }
 return result;
}

__crit __user void *kshm_mmapram(struct kshm *__restrict self, __user void *hint,
                                 size_t len, int prot, int flags) {
 struct kshmtab *tab; __user void *result;
 KTASK_CRIT_MARK
 assert(isaligned((uintptr_t)hint,PAGEALIGN));
 kassert_kshm(self);
 tab = kshmtab_newram(ceildiv(len,PAGESIZE),prot);
 if __unlikely(!tab) return MAP_FAIL;
 if __unlikely(KE_ISERR(kshm_instab_inherited(self,tab,flags,hint,&result))
               ) { kshmtab_decref(tab); result = MAP_FAIL; }
 return result;
}
__crit __user void *kshm_mmap_linear(struct kshm *__restrict self, __user void *hint,
                                     size_t len, int prot, int flags,
                                     __kernel void **__restrict kaddr) {
 struct kshmtab *tab; __user void *result;
 KTASK_CRIT_MARK
 assert(isaligned((uintptr_t)hint,PAGEALIGN));
 kassert_kshm(self);
 kassertobj(kaddr);
 tab = kshmtab_newram_linear(ceildiv(len,PAGESIZE),prot);
 if __unlikely(!tab) return MAP_FAIL;
 assertf(!tab->mt_scat.ts_next,"This! Is! Linear! *knocks guy down hole*");
 *kaddr = tab->mt_scat.ts_addr;
 if __unlikely(KE_ISERR(kshm_instab_inherited(self,tab,flags,hint,&result))
               ) { kshmtab_decref(tab); result = MAP_FAIL; }
 return result;
}

__crit kerrno_t
kshm_mmapdev(struct kshm *__restrict self,
             void __user * __kernel *hint_and_result,
             size_t len, int prot, int flags,
             __kernel void *phys_ptr) {
 struct kshmtab *tab; kerrno_t error;
 size_t pages; void *result,*aligned_phys;
 uintptr_t align_offset;
 KTASK_CRIT_MARK
 assert(isaligned((uintptr_t)*hint_and_result,PAGEALIGN));
 kassert_kshm(self);
 aligned_phys = (void *)alignd((uintptr_t)phys_ptr,PAGEALIGN);
 align_offset = (uintptr_t)phys_ptr-(uintptr_t)aligned_phys;
 pages = ceildiv(len+align_offset,PAGESIZE);
 error = kshm_allow_devmap((uintptr_t)aligned_phys,pages);
 if __unlikely(KE_ISERR(error)) return error;
 tab = kshmtab_newdev(phys_ptr,pages,prot);
 if __unlikely(!tab) return KE_NOMEM;
 assertf(!tab->mt_scat.ts_next,"Device memory must not be scattered");
 error = kshm_instab_inherited(self,tab,flags,*hint_and_result,&result);
 if __unlikely(KE_ISERR(error)) { kshmtab_decref(tab); return error; }
 *(uintptr_t **)hint_and_result += align_offset;
 return error;
}


__crit size_t kshm_munmap(struct kshm *__restrict self, __user void *addr,
                          size_t length, int allow_kernel) {
 __u8 group; void *addr_end; size_t result = 0;
 struct kshmtabentry *iter; size_t index;
 KTASK_CRIT_MARK
 kassert_kshm(self);
 addr_end = (void *)((uintptr_t)addr+length);
 if (addr_end < addr) group = KSHM_GROUPCOUNT;
 else group = KSHM_GROUPOF(addr_end)+1;
 do {
  iter = self->sm_groups[--group];
  assertf(group < KSHM_GROUPCOUNT,"%d",group);
  assert(!iter || KSHM_GROUPOF(iter->te_map) == group);
  while (iter) {
   void *entryend;
   assertf(iter >= self->sm_tabv && iter < self->sm_tabv+self->sm_tabc
          ,"Slot %p (technical index %Iu) is out-of-bounds (base: %p; max: %Iu)"
          ,iter,iter-self->sm_tabv,self->sm_tabv,self->sm_tabc);
   if (addr_end <= iter->te_map) break;
   entryend = (void *)((uintptr_t)iter->te_map+iter->te_tab->mt_pages*PAGESIZE);
   if (addr >= entryend) goto end;
   assertf(addr_end >= iter->te_map && addr < entryend,"Tab not actually mapped");
   if (!allow_kernel && (iter->te_tab->mt_flags&KSHMTAB_FLAG_K)) ++iter;
   else {
    struct kshmtab *oldtab = iter->te_tab;
    index = (size_t)(iter-self->sm_tabv);
    kshm_delslot(self,iter);
    ++result;
    kshmtab_decref(oldtab);
    if (index == self->sm_tabc) break;
    iter = self->sm_tabv+index;
   }
  }
 } while (group);
end:
 return result;
}


// TODO: Access to non-mapped file (aka. SWAP) mappings
size_t __kshm_memcpy_k2u(struct kshm const *__restrict self, __user void *dst,
                         __kernel void const *__restrict src, size_t bytes) {
 return __kpagedir_memcpy_k2u(self->sm_pd,dst,src,bytes);
}
size_t __kshm_memcpy_u2k(struct kshm const *__restrict self, __kernel void *__restrict dst,
                         __user void const *src, size_t bytes) {
 return __kpagedir_memcpy_u2k(self->sm_pd,dst,src,bytes);
}
size_t __kshm_memcpy_u2u(struct kshm const *__restrict self, __user void *dst,
                         __user void const *src, size_t bytes) {
 return __kpagedir_memcpy_u2u(self->sm_pd,dst,src,bytes);
}
__kernel void *__kshm_translate_1(struct kshm const *__restrict self,
                                  __user void const *addr,
                                  size_t *__restrict max_bytes, int read_write) {
 void *result = kpagedir_translate_flags(self->sm_pd,addr,KPAGEDIR_TRANSLATE_FLAGS
                                        ((read_write)?PAGEDIR_FLAG_READ_WRITE:0));
#ifdef X86_VPTR_GET_POFF
 *max_bytes = (size_t)X86_VPTR_GET_POFF(addr);
#else
 *max_bytes = PAGESIZE-((uintptr_t)addr-alignd((uintptr_t)addr,PAGEALIGN));
#endif
 return result;
}
__kernel void *__kshm_translate_u(struct kshm const *__restrict self,
                                  __user void const *addr,
                                  size_t *__restrict max_bytes, int read_write) {
 return __kpagedir_translate_u(self->sm_pd,addr,max_bytes,read_write);
}


kerrno_t kshm_allow_devmap(uintptr_t addr, size_t pages) {
 uintptr_t end = addr+pages*PAGESIZE;
 if (ktask_accesssm(ktask_zero())) return KE_OK; // Well... You can do whateeever you like!
#define IN_RANGE(start,size) \
 (addr >= alignd(start,PAGEALIGN) && \
  end <= align((start)+(size),PAGEALIGN))

#ifdef __x86__
 // Special region of memory: The X86 VGA terminal.
 // TODO: We still shouldn't allow everyone access to this...
 if (IN_RANGE(0xB8000,80*25*2)) return KE_OK;
#endif


#undef IN_RANGE
 return KE_ACCES; // Deny access
}

__DECL_END

#ifndef __INTELLISENSE__
#include "shm-ldt.c.inl"
#endif
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_SHM_C__ */

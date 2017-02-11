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
#ifndef __KOS_KERNEL_FS_FAT_INTERNAL_FATCACHE_C_INL__
#define __KOS_KERNEL_FS_FAT_INTERNAL_FATCACHE_C_INL__ 1

#include <assert.h>
#include <kos/compiler.h>
#include <kos/kernel/debug.h>
#include <kos/errno.h>
#include <kos/kernel/fs/fat-internal.h>
#include <kos/kernel/rwlock.h>
#include <kos/kernel/types.h>
#include <math.h>
#include <sys/types.h>
#include <stdio.h>
#include <kos/syslog.h>

__DECL_BEGIN

kerrno_t kfatfs_fat_allocfirst(struct kfatfs *self, kfatcls_t *target) {
 kerrno_t error; size_t valaddr; __u8 valmask;
 if __unlikely(KE_ISERR(error = krwlock_beginwrite(&self->f_fatlock))) return error;
 error = _kfatfs_fat_getfreecluster_unlocked(self,target,0);
 if __likely(KE_ISOK(error)) {
  // If a cluster was found, link the previous one,
  // and mark the new one as pointing to EOF
  (*self->f_writefat)(self,*target,self->f_clseof_marker);
  // Also need to mark the newly allocated cluster as modified
  valaddr = (*target)/4,valmask = (__u8)(1 << ((*target)%4)*2);
  self->f_fatmeta[valaddr] |= valmask;
  // Finally, set the global flag indicating a change
  self->f_flags |= KFATFS_FLAG_FATCHANGED;
 }
 krwlock_endwrite(&self->f_fatlock);
 return error;
}
kerrno_t kfatfs_fat_readandalloc(struct kfatfs *self, kfatcls_t index, kfatcls_t *target) {
 kerrno_t error; size_t valaddr;
 __u8 valmask,valval; kfatsec_t fatsec;
 kassertobj(self);
 kassertbyte(self->f_readfat);
 kassertbyte(self->f_writefat);
 kassertbyte(self->f_getfatsec);
 kassertmem(self->f_fatv,self->f_fatsize);
 kassertmem(self->f_fatmeta,ceildiv(self->f_fatsize,self->f_sec4fat*4));
 assert(self->f_secsize);
 assertf(index < self->f_clseof,"Given FAT byte is out-of-bounds");
#if 1
 fatsec = (*self->f_getfatsec)(self,index);
#else
 fatsec = index/self->f_secsize;
#endif
 valaddr = fatsec/4,valmask = (__u8)(2 << ((fatsec%4)*2));
 if __unlikely(KE_ISERR(error = krwlock_beginread(&self->f_fatlock))) return error;
 valval = self->f_fatmeta[valaddr];
 if (valval&valmask) {
  // Entry is already cached (no need to start writing)
  *target = (*self->f_readfat)(self,index);
  krwlock_endread(&self->f_fatlock);
  if (!kfatfs_iseofcluster(self,*target)) goto end;
 } else {
  krwlock_endread(&self->f_fatlock);
 }

 if __unlikely(KE_ISERR(error = krwlock_beginwrite(&self->f_fatlock))) return error;
 valval = self->f_fatmeta[valaddr];
 // Make sure that no other task loaded the
 // FAT while we were switching to write-mode.
 if (valval&valmask) {
  *target = (*self->f_readfat)(self,index);
 } else {
  // Entry is not cached --> Load the fat's entry.
  error = kfatfs_loadsectors(self,self->f_firstfatsec+fatsec,1,
                            (void *)((uintptr_t)self->f_fatv+fatsec*self->f_secsize));
  if __unlikely(KE_ISERR(error)) goto end_write;
  *target = (*self->f_readfat)(self,index);
  // Mark the FAT cache entry as loaded
  assert(self->f_fatmeta[valaddr] == valval);
  self->f_fatmeta[valaddr] = valval|valmask;
 }
 if (kfatfs_iseofcluster(self,*target)) {
  k_syslogf(KLOG_INFO,"Allocating FAT cluster after EOF %I32u (out-of-bounds for %I32u)\n",*target,self->f_clseof);
  // Search for a free cluster (We prefer to use the nearest chunk to reduce fragmentation)
  error = _kfatfs_fat_getfreecluster_unlocked(self,target,index+1);
  if __likely(KE_ISOK(error)) {
   // If a cluster was found, link the previous one,
   // and mark the new one as pointing to EOF
   (*self->f_writefat)(self,index,*target);
   (*self->f_writefat)(self,*target,self->f_clseof_marker);
   // Don't forget to mark the cache as modifed
   self->f_fatmeta[valaddr] |= valmask|(valmask >> 1);
   // Also need to mark the newly allocated cluster as modified
   fatsec = *target/self->f_secsize;
   valaddr = fatsec/4,valmask = (__u8)(1 << (fatsec%4)*2);
   self->f_fatmeta[valaddr] |= valmask;
   // Finally, set the global flag indicating a change
   self->f_flags |= KFATFS_FLAG_FATCHANGED;
  }
 }
end_write:
 krwlock_endwrite(&self->f_fatlock);
end:
 return error;
}

kerrno_t kfatfs_fat_read(struct kfatfs *self, kfatcls_t index, kfatcls_t *target) {
 kerrno_t error; size_t valaddr;
 __u8 valmask,valval; kfatsec_t fatsec;
 kassertobj(self);
 kassertbyte(self->f_readfat);
 kassertbyte(self->f_writefat);
 kassertmem(self->f_fatv,self->f_fatsize);
 kassertmem(self->f_fatmeta,ceildiv(self->f_fatsize,self->f_sec4fat*4));
 assert(self->f_secsize);
 assertf(index < self->f_clseof,"Given FAT byte is out-of-bounds");
#if 1
 fatsec = (*self->f_getfatsec)(self,index);
#else
 fatsec = index/self->f_secsize;
#endif
 valaddr = fatsec/4,valmask = (__u8)(2 << ((fatsec%4)*2));
 if __unlikely(KE_ISERR(error = krwlock_beginread(&self->f_fatlock))) return error;
 valval = self->f_fatmeta[valaddr];
 if (valval&valmask) {
  // Entry is already cached (no need to start writing)
  *target = (*self->f_readfat)(self,index);
  krwlock_endread(&self->f_fatlock);
  return KE_OK;
 }
 krwlock_endread(&self->f_fatlock);
 if __unlikely(KE_ISERR(error = krwlock_beginwrite(&self->f_fatlock))) return error;
 valval = self->f_fatmeta[valaddr];
 // Make sure that no other task loaded the
 // FAT while we were switching to write-mode.
 if __unlikely(valval&valmask) {
  *target = (*self->f_readfat)(self,index);
  krwlock_endwrite(&self->f_fatlock);
  return KE_OK;
 }
 // Entry is not cached --> Load the fat's entry.
 error = kfatfs_loadsectors(self,self->f_firstfatsec+fatsec,1,
                           (void *)((uintptr_t)self->f_fatv+fatsec*self->f_secsize));
 if __likely(KE_ISOK(error)) {
  *target = (*self->f_readfat)(self,index);
  // Mark the FAT cache entry as loaded
  assert(self->f_fatmeta[valaddr] == valval);
  self->f_fatmeta[valaddr] = valval|valmask;
 }
 krwlock_endwrite(&self->f_fatlock);
 return error;
}
kerrno_t kfatfs_fat_write(struct kfatfs *self, kfatcls_t index, kfatcls_t target) {
 kerrno_t error; size_t valaddr; __u8 valmask,valval;
 kfatsec_t fatsec;
 kassertobj(self);
 kassertbyte(self->f_readfat);
 kassertbyte(self->f_writefat);
 kassertmem(self->f_fatv,self->f_fatsize);
 kassertmem(self->f_fatmeta,ceildiv(self->f_fatsize,self->f_sec4fat*4));
 assert(self->f_secsize);
 assertf(index < self->f_clseof,"Given FAT byte is out-of-bounds");
#if 1
 fatsec = (*self->f_getfatsec)(self,index);
#else
 fatsec = index/self->f_secsize;
#endif
 valaddr = fatsec/4,valmask = (__u8)(2 << ((fatsec%4)*2));

 // Make sure the FAT cache is loaded
 if __unlikely(KE_ISERR(error = krwlock_beginwrite(&self->f_fatlock))) return error;
 valval = self->f_fatmeta[valaddr];
 if (!(valval&valmask)) { // Load the fat's entry.
  error = kfatfs_loadsectors(self,self->f_firstfatsec+fatsec,1,
                            (void *)((uintptr_t)self->f_fatv+fatsec*self->f_secsize));
  if __unlikely(KE_ISERR(error)) { krwlock_endwrite(&self->f_fatlock); return error; }
  valval |= valmask;
 }
 valmask >>= 1; // Switch to the associated changed-mask
 assert(valmask);
 // Actually write the cache
 (*self->f_writefat)(self,index,target);
 if (!(valval&valmask)) {
  // New change --> mark the chunk as such
  // Also update the changed flag in the FS (which is used to speed up flushing)
  self->f_flags |= KFATFS_FLAG_FATCHANGED;
  valval |= valmask;
 }
 // Update the metadata flags
 self->f_fatmeta[valaddr] = valval;
 krwlock_endwrite(&self->f_fatlock);
 return KE_OK;
}

kerrno_t kfatfs_fat_flush(struct kfatfs *self) {
 kerrno_t error;
 kassertobj(self);
 kassertbyte(self->f_readfat);
 kassertbyte(self->f_writefat);
 kassertmem(self->f_fatv,self->f_fatsize);
 kassertmem(self->f_fatmeta,ceildiv(self->f_fatsize,self->f_sec4fat*4));
 if __unlikely(KE_ISERR(error = krwlock_beginwrite(&self->f_fatlock))) return error;
 if (self->f_flags&KFATFS_FLAG_FATCHANGED) {
  // Changes were made, so we need to flush them!
  error = _kfatfs_fat_doflush_unlocked(self);
  if __likely(KE_ISOK(error)) self->f_flags &= ~(KFATFS_FLAG_FATCHANGED);
 }
 krwlock_endwrite(&self->f_fatlock);
 return error;
}
kerrno_t _kfatfs_fat_doflush_unlocked(struct kfatfs *self) {
 kerrno_t error; byte_t *fat_data,*meta_data,metamask;
 kfatsec_t i; size_t j;
 kassertobj(self);
 kassertbyte(self->f_readfat);
 kassertbyte(self->f_writefat);
 kassertmem(self->f_fatv,self->f_fatsize);
 kassertmem(self->f_fatmeta,ceildiv(self->f_fatsize,self->f_sec4fat*4));
 assert(self->f_fatsize == self->f_secsize*self->f_sec4fat);
 for (i = 0; i < self->f_sec4fat; ++i) {
  fat_data = (byte_t *)self->f_fatv+(i*self->f_secsize);
  meta_data = (byte_t *)self->f_fatmeta+(i/4);
  metamask = (__u8)(1 << ((i%4)*2));
  if (*meta_data&metamask) {
   // Modified fat sector (flush it)
   k_syslogf(KLOG_MSG,"Saving modified FAT sector %I32u (%I8x %I8x)\n",i,*meta_data,metamask);
   // Since there might be multiple FATs, we need to flush the changes to all of them!
   // todo: We can optimize this by flushing multiple consecutive sectors at once
   for (j = 0; j < self->f_fatcount; ++j) {
    error = kfatfs_savesectors(self,self->f_firstfatsec+i+j*self->f_sec4fat,1,fat_data);
    if __unlikely(KE_ISERR(error)) return error;
   }
   *meta_data &= ~(metamask);
  }
 }
 return KE_OK;
}

kerrno_t _kfatfs_fat_getfreecluster_unlocked(
 struct kfatfs *self, kfatcls_t *result, kfatcls_t hint) {
 kerrno_t error; kfatcls_t cls; size_t valaddr;
 __u8 valmask,valval; kfatsec_t fatsec;
 kassertobj(self);
 kassertbyte(self->f_readfat);
 kassertbyte(self->f_writefat);
 kassertmem(self->f_fatv,self->f_fatsize);
 kassertmem(self->f_fatmeta,ceildiv(self->f_fatsize,self->f_sec4fat*4));
 assert(self->f_secsize);
 // TODO: Start searching for an empty cluster at 'hint' (but only if it's 'hint > 2')
 for (cls = 2; cls < self->f_clseof; ++cls) {
  fatsec = cls/self->f_secsize;
  valaddr = fatsec/4,valmask = (__u8)(2 << ((fatsec%4)*2));
  valval = self->f_fatmeta[valaddr];
  if __unlikely(!(valval&valmask)) {
   // Load the FAT entry, if it wasn't already
   error = kfatfs_loadsectors(self,self->f_firstfatsec+fatsec,1,
                             (void *)((uintptr_t)self->f_fatv+fatsec*self->f_secsize));
   if __unlikely(KE_ISERR(error)) return error;
   assert(self->f_fatmeta[valaddr] == valval);
   self->f_fatmeta[valaddr] = valval|valmask;
  }
  if ((*self->f_readfat)(self,cls) == KFATFS_CUSTER_UNUSED) {
   // Found an unused cluster
   *result = cls;
   return KE_OK;
  }
 }
 _printtraceback_d();
 return KE_NOSPC;
}


__DECL_END

#endif /* !__KOS_KERNEL_FS_FAT_INTERNAL_FATCACHE_C_INL__ */

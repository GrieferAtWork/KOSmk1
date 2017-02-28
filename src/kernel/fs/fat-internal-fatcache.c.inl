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
#include <limits.h>

__DECL_BEGIN

/* Bits used in FAT metadata. */
#define FATMETA_CHANGED  01 /*< The  */
#define FATMETA_LOADED   02
#define FATMETA_BITS      2
#define FATMETA_PERBYTE  (CHAR_BIT/FATMETA_BITS)

#define FATMETA_ADDR(cluster)                          ((cluster)/FATMETA_PERBYTE)
#define FATMETA_MASK(cluster,bits) ((byte_t)((bits) << ((cluster)%FATMETA_PERBYTE)*FATMETA_BITS))

#if FATMETA_BITS > CHAR_BIT || CHAR_BIT % FATMETA_BITS
#error "FIXME: Invalid amount of FAT meta bits"
#endif

#define SELF                       (self)
#define FAT_VALIDCLUSTER(cluster) ((cluster) < ceildiv(SELF->f_fatsize,SELF->f_sec4fat))


#define FATBIT_ON(cluster,bits) \
 (assertf(FAT_VALIDCLUSTER(cluster),"Invalid cluster: %Iu",(size_t)(cluster)),\
  SELF->f_fatmeta[FATMETA_ADDR(cluster)] |= FATMETA_MASK(cluster,bits))
#define FATBIT_OFF(cluster,bits) \
 (assertf(FAT_VALIDCLUSTER(cluster),"Invalid cluster: %Iu",(size_t)(cluster)),\
  SELF->f_fatmeta[FATMETA_ADDR(cluster)] &= ~FATMETA_MASK(cluster,bits))
#define FAT_CHANGED(cluster) \
 (FATBIT_ON(cluster,FATMETA_CHANGED),\
  SELF->f_flags |= KFATFS_FLAG_FATCHANGED)\

#define FAT_WRITE(cluster,value)  ((*SELF->f_writefat)(SELF,cluster,value))
#define FAT_WRITE_AND_UPDATE(cluster,value) \
 (FAT_WRITE(cluster,value),FAT_CHANGED(cluster))


typedef kfatsec_t kfatoff_t;

/* Returns the sector offset associated with a given cluster. */
#define /*kfatoff_t*/FAT_GETCLUSTEROFFSET(cluster) (*SELF->f_getfatsec)(SELF,cluster)
#define /*kfatsec_t*/FAT_GETCLUSTERADDR(offset)    (SELF->f_firstfatsec+(offset))


#define LOCK_WRITE_BEGIN()   krwlock_beginwrite(&SELF->f_fatlock)
#define LOCK_WRITE_END()     krwlock_endwrite(&SELF->f_fatlock)
#define LOCK_READ_BEGIN()    krwlock_beginread(&SELF->f_fatlock)
#define LOCK_READ_END()      krwlock_endread(&SELF->f_fatlock)
#define LOCK_DOWNGRADE()     krwlock_downgrade(&SELF->f_fatlock)


kerrno_t _kfatfs_fat_freeall_unlocked(struct kfatfs *__restrict self, kfatcls_t first) {
 kerrno_t error; kfatcls_t next;
 error = _kfatfs_fat_read_unlocked(self,first,&next);
 if __unlikely(KE_ISERR(error)) return error;
 if (!kfatfs_iseofcluster(self,next)) {
  error = _kfatfs_fat_freeall_unlocked(self,next);
  if __unlikely(KE_ISERR(error)) return error;
 }
 /* Actually mark the cluster as unused. */
 FAT_WRITE_AND_UPDATE(first,KFATFS_CUSTER_UNUSED);
 return KE_OK;
}

kerrno_t kfatfs_fat_freeall(struct kfatfs *__restrict self, kfatcls_t first) {
 kerrno_t error;
 if __unlikely(KE_ISERR(error = LOCK_WRITE_BEGIN())) return error;
 error = _kfatfs_fat_freeall_unlocked(self,first);
 LOCK_WRITE_END();
 return error;
}

kerrno_t kfatfs_fat_allocfirst(struct kfatfs *__restrict self, kfatcls_t *__restrict target) {
 kerrno_t error;
 if __unlikely(KE_ISERR(error = LOCK_WRITE_BEGIN())) return error;
 error = _kfatfs_fat_getfreecluster_unlocked(self,target,0);
 if __likely(KE_ISOK(error)) {
  /* If a cluster was found, link the previous one,
   * and mark the new one as pointing to EOF */
  FAT_WRITE_AND_UPDATE(*target,self->f_clseof_marker);
 }
 LOCK_WRITE_END();
 return error;
}
kerrno_t kfatfs_fat_readandalloc(struct kfatfs *__restrict self, kfatcls_t index, kfatcls_t *__restrict target) {
 kerrno_t error; size_t meta_addr;
 byte_t meta_mask,meta_val; kfatoff_t fat_index;
 kassertobj(self);
 kassertbyte(self->f_readfat);
 kassertbyte(self->f_writefat);
 kassertmem(self->f_fatv,self->f_fatsize);
 assert(self->f_secsize);
 assertf(index < self->f_clseof,"Given FAT byte is out-of-bounds");
 fat_index = FAT_GETCLUSTEROFFSET(index);
 meta_addr = FATMETA_ADDR(fat_index);
 meta_mask = FATMETA_MASK(fat_index,FATMETA_LOADED);
 if __unlikely(KE_ISERR(error = krwlock_beginread(&self->f_fatlock))) return error;
 meta_val = self->f_fatmeta[meta_addr];
 if (meta_val&meta_mask) {
  // Entry is already cached (no need to start writing)
  *target = (*self->f_readfat)(self,index);
  if (!kfatfs_iseofcluster(self,*target)) {
   krwlock_endread(&self->f_fatlock);
   goto end;
  }
 }
 error = krwlock_upgrade(&self->f_fatlock);
 if __unlikely(KE_ISERR(error)) return error;
 meta_val = self->f_fatmeta[meta_addr];
 // Make sure that no other task loaded the
 // FAT while we were switching to write-mode.
 if (meta_val&meta_mask) {
  *target = (*self->f_readfat)(self,index);
  error   = KE_OK;
 } else {
  // Entry is not cached --> Load the fat's entry.
  error = kfatfs_loadsectors(self,FAT_GETCLUSTERADDR(fat_index),1,
                            (void *)((uintptr_t)self->f_fatv+fat_index*self->f_secsize));
  if __unlikely(KE_ISERR(error)) goto end_write;
  *target = (*self->f_readfat)(self,index);
  // Mark the FAT cache entry as loaded
  assert(self->f_fatmeta[meta_addr] == meta_val);
  self->f_fatmeta[meta_addr] = meta_val|meta_mask;
 }
 if (kfatfs_iseofcluster(self,*target)) {
  k_syslogf(KLOG_INFO
           ,"Allocating FAT cluster after EOF %I32u from %I32u (out-of-bounds for %I32u)\n"
           ,*target,index,self->f_clseof);
  // Search for a free cluster (We prefer to use the nearest chunk to reduce fragmentation)
  error = _kfatfs_fat_getfreecluster_unlocked(self,target,index+1);
  if __likely(KE_ISOK(error)) {
   // If a cluster was found, link the previous one,
   // and mark the new one as pointing to EOF
   (*self->f_writefat)(self,index,*target);
   (*self->f_writefat)(self,*target,self->f_clseof_marker);
   /* Don't forget to mark the cache as modified */
   self->f_fatmeta[meta_addr] |= meta_mask|(meta_mask >> 1);
   /* Also need to mark the newly allocated cluster as modified */
   fat_index = FAT_GETCLUSTEROFFSET(*target);
   meta_addr = FATMETA_ADDR(fat_index);
   meta_mask = FATMETA_MASK(fat_index,FATMETA_CHANGED);
   self->f_fatmeta[meta_addr] |= meta_mask;
   /* Finally, set the global flag indicating a change */
   self->f_flags |= KFATFS_FLAG_FATCHANGED;
  }
 }
end_write:
 LOCK_WRITE_END();
end:
 return error;
}

kerrno_t _kfatfs_fat_read_unlocked(struct kfatfs *__restrict self, kfatcls_t index, kfatcls_t *__restrict target) {
 kerrno_t error; size_t meta_addr;
 byte_t meta_mask,meta_val; kfatoff_t fat_index;
 kassertobj(self);
 kassertbyte(self->f_readfat);
 kassertbyte(self->f_writefat);
 kassertmem(self->f_fatv,self->f_fatsize);
 assert(self->f_secsize);
 assertf(index < self->f_clseof,"Given FAT byte is out-of-bounds");
 assert(FAT_VALIDCLUSTER(index));
 fat_index = FAT_GETCLUSTEROFFSET(index);
 meta_addr = FATMETA_ADDR(fat_index);
 meta_mask = FATMETA_MASK(fat_index,FATMETA_LOADED);
 meta_val  = self->f_fatmeta[meta_addr];
 if (meta_val&meta_mask) {
  /* Entry is already cached */
  *target = (*self->f_readfat)(self,index);
  return KE_OK;
 }
 meta_val = self->f_fatmeta[meta_addr];
 // Make sure that no other task loaded the
 // FAT while we were switching to write-mode.
 if __unlikely(meta_val&meta_mask) {
  *target = (*self->f_readfat)(self,index);
  LOCK_WRITE_END();
  return KE_OK;
 }
 // Entry is not cached --> Load the fat's entry.
 error = kfatfs_loadsectors(self,FAT_GETCLUSTERADDR(fat_index),1,
                           (void *)((uintptr_t)self->f_fatv+fat_index*self->f_secsize));
 if __likely(KE_ISOK(error)) {
  *target = (*self->f_readfat)(self,index);
  // Mark the FAT cache entry as loaded
  assert(self->f_fatmeta[meta_addr] == meta_val);
  self->f_fatmeta[meta_addr] = meta_val|meta_mask;
 }
 return error;
}
kerrno_t kfatfs_fat_read(struct kfatfs *__restrict self, kfatcls_t index, kfatcls_t *__restrict target) {
 kerrno_t error; size_t meta_addr;
 byte_t meta_mask,meta_val; kfatoff_t fat_index;
 kassertobj(self);
 kassertbyte(self->f_readfat);
 kassertbyte(self->f_writefat);
 kassertmem(self->f_fatv,self->f_fatsize);
 assert(self->f_secsize);
 assertf(index < self->f_clseof,"Given FAT byte is out-of-bounds");
 fat_index = FAT_GETCLUSTEROFFSET(index);
 meta_addr = FATMETA_ADDR(fat_index);
 meta_mask = FATMETA_MASK(fat_index,FATMETA_LOADED);
 if __unlikely(KE_ISERR(error = krwlock_beginread(&self->f_fatlock))) return error;
 meta_val = self->f_fatmeta[meta_addr];
 if (meta_val&meta_mask) {
  // Entry is already cached (no need to start writing)
  *target = (*self->f_readfat)(self,index);
  krwlock_endread(&self->f_fatlock);
  return KE_OK;
 }
 error = krwlock_upgrade(&self->f_fatlock);
 if __unlikely(KE_ISERR(error)) return error;
 meta_val = self->f_fatmeta[meta_addr];
 // Make sure that no other task loaded the
 // FAT while we were switching to write-mode.
 if __unlikely(meta_val&meta_mask) {
  *target = (*self->f_readfat)(self,index);
  LOCK_WRITE_END();
  return KE_OK;
 }
 // Entry is not cached --> Load the fat's entry.
 error = kfatfs_loadsectors(self,FAT_GETCLUSTERADDR(fat_index),1,
                           (void *)((uintptr_t)self->f_fatv+fat_index*self->f_secsize));
 if __likely(KE_ISOK(error)) {
  *target = (*self->f_readfat)(self,index);
  // Mark the FAT cache entry as loaded
  assert(self->f_fatmeta[meta_addr] == meta_val);
  self->f_fatmeta[meta_addr] = meta_val|meta_mask;
 }
 LOCK_WRITE_END();
 return error;
}
kerrno_t kfatfs_fat_write(struct kfatfs *__restrict self, kfatcls_t index, kfatcls_t target) {
 kerrno_t error; size_t meta_addr; byte_t meta_mask,meta_val;
 kfatoff_t fat_index;
 kassertobj(self);
 kassertbyte(self->f_readfat);
 kassertbyte(self->f_writefat);
 kassertmem(self->f_fatv,self->f_fatsize);
 assert(self->f_secsize);
 assertf(index < self->f_clseof,"Given FAT byte is out-of-bounds");
 fat_index = FAT_GETCLUSTEROFFSET(index);
 meta_addr = FATMETA_ADDR(fat_index);
 meta_mask = FATMETA_MASK(fat_index,FATMETA_LOADED);

 // Make sure the FAT cache is loaded
 if __unlikely(KE_ISERR(error = LOCK_WRITE_BEGIN())) return error;
 meta_val = self->f_fatmeta[meta_addr];
 if (!(meta_val&meta_mask)) { // Load the fat's entry.
  error = kfatfs_loadsectors(self,FAT_GETCLUSTERADDR(fat_index),1,
                            (void *)((uintptr_t)self->f_fatv+fat_index*self->f_secsize));
  if __unlikely(KE_ISERR(error)) { LOCK_WRITE_END(); return error; }
  meta_val |= meta_mask;
 }
 meta_mask >>= 1; // Switch to the associated changed-mask
 assert(meta_mask);
 // Actually write the cache
 (*self->f_writefat)(self,index,target);
 if (!(meta_val&meta_mask)) {
  // New change --> mark the chunk as such
  // Also update the changed flag in the FS (which is used to speed up flushing)
  self->f_flags |= KFATFS_FLAG_FATCHANGED;
  meta_val |= meta_mask;
 }
 // Update the metadata flags
 self->f_fatmeta[meta_addr] = meta_val;
 LOCK_WRITE_END();
 return KE_OK;
}

kerrno_t kfatfs_fat_flush(struct kfatfs *__restrict self) {
 kerrno_t error;
 kassertobj(self);
 kassertbyte(self->f_readfat);
 kassertbyte(self->f_writefat);
 kassertmem(self->f_fatv,self->f_fatsize);
 if __unlikely(KE_ISERR(error = LOCK_WRITE_BEGIN())) return error;
 if (self->f_flags&KFATFS_FLAG_FATCHANGED) {
  // Changes were made, so we need to flush them!
  error = _kfatfs_fat_doflush_unlocked(self);
  if __likely(KE_ISOK(error)) self->f_flags &= ~(KFATFS_FLAG_FATCHANGED);
 }
 LOCK_WRITE_END();
 return error;
}
kerrno_t _kfatfs_fat_doflush_unlocked(struct kfatfs *__restrict self) {
 kerrno_t error; byte_t *fat_data,*meta_data,metamask;
 kfatsec_t i; size_t j;
 kassertobj(self);
 kassertbyte(self->f_readfat);
 kassertbyte(self->f_writefat);
 kassertmem(self->f_fatv,self->f_fatsize);
 assert(self->f_fatsize == self->f_secsize*self->f_sec4fat);
 for (i = 0; i < self->f_sec4fat; ++i) {
  fat_data  = (byte_t *)self->f_fatv+(i*self->f_secsize);
  meta_data = (byte_t *)self->f_fatmeta+FATMETA_ADDR(i);
  metamask  = FATMETA_MASK(i,FATMETA_CHANGED);
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

kerrno_t
_kfatfs_fat_getfreecluster_unlocked(struct kfatfs *__restrict self,
                                    kfatcls_t *__restrict result,
                                    kfatcls_t hint) {
 kerrno_t error; kfatcls_t cls; size_t meta_addr;
 byte_t meta_mask,meta_val; kfatoff_t fat_index;
 kassertobj(self);
 kassertbyte(self->f_readfat);
 kassertbyte(self->f_writefat);
 kassertmem(self->f_fatv,self->f_fatsize);
 assert(self->f_secsize);
 /* TODO: Start searching for an empty cluster at 'hint' (but only if it's 'hint > 2') */
 for (cls = 2; cls < self->f_clseof; ++cls) {
  fat_index = FAT_GETCLUSTEROFFSET(cls);
  meta_addr = FATMETA_ADDR(fat_index);
  meta_mask = FATMETA_MASK(fat_index,FATMETA_LOADED);
  meta_val = self->f_fatmeta[meta_addr];
  if __unlikely(!(meta_val&meta_mask)) {
   /* Load the FAT entry, if it wasn't already */
   error = kfatfs_loadsectors(self,FAT_GETCLUSTERADDR(fat_index),1,
                             (void *)((uintptr_t)self->f_fatv+fat_index*self->f_secsize));
   if __unlikely(KE_ISERR(error)) return error;
   assert(self->f_fatmeta[meta_addr] == meta_val);
   self->f_fatmeta[meta_addr] = meta_val|meta_mask;
  }
  if ((*self->f_readfat)(self,cls) == KFATFS_CUSTER_UNUSED) {
   /* Found an unused cluster */
   *result = cls;
   return KE_OK;
  }
 }
 tb_print();
 return KE_NOSPC;
}


__DECL_END

#endif /* !__KOS_KERNEL_FS_FAT_INTERNAL_FATCACHE_C_INL__ */

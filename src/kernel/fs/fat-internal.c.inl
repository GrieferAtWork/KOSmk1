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
#ifndef __KOS_KERNEL_FS_FAT_INTERNAL_C_INL__
#define __KOS_KERNEL_FS_FAT_INTERNAL_C_INL__ 1

#include <kos/compiler.h>
#include <kos/kernel/debug.h>
#include <kos/errno.h>
#include <kos/kernel/fs/fat-internal.h>
#include <kos/kernel/fs/fat.h>
#include <kos/kernel/rwlock.h>
#include <kos/kernel/types.h>
#include <math.h>
#include <stdint.h>
#include <sys/types.h>

__DECL_BEGIN

kerrno_t kfatfs_fat_close(struct kfatfs *self) {
 kerrno_t error;
 kassertobj(self);
 kassertmem(self->f_fatv,self->f_fatsize);
 kassertmem(self->f_fatmeta,ceildiv(self->f_fatsize,self->f_sec4fat*4));
 if __unlikely(KE_ISERR(error = krwlock_close(&self->f_fatlock))) return error;
 if (self->f_flags&KFATFS_FLAG_FATCHANGED) {
  // Changes were made, so we need to flush them!
  error = _kfatfs_fat_doflush_unlocked(self);
  if __unlikely(KE_ISERR(error)) {
   // Failed to flush, meaning failed to close
   krwlock_reset(&self->f_fatlock);
   return error;
  }
 }
 return KE_OK;
}


kfatsec_t kfatfs_getfatsec_12(struct kfatfs *self, kfatcls_t index) {
 kassertobj(self);
 return (index+(index/2))/self->f_secsize;
}
kfatsec_t kfatfs_getfatsec_16(struct kfatfs *self, kfatcls_t index) {
 kassertobj(self);
 return (index*2)/self->f_secsize;
}
kfatsec_t kfatfs_getfatsec_32(struct kfatfs *self, kfatcls_t index) {
 kassertobj(self);
 return (index*4)/self->f_secsize;
}


kfatcls_t kfatfs_readfat_12(struct kfatfs *self, kfatcls_t cluster) {
 size_t fatoff; kassertobj(self);
 fatoff = (cluster+(cluster/2)) % self->f_fatsize;
 __u16 val = (*(__u16 *)((uintptr_t)self->f_fatv+fatoff));
 if (cluster&1) val >>= 4; else val &= 0x0fff;
 return val;
}
kfatcls_t kfatfs_readfat_16(struct kfatfs *self, kfatcls_t cluster) {
 size_t fatoff; kassertobj(self);
 fatoff = (cluster*2) % self->f_fatsize;
 return (*(__u16 *)((uintptr_t)self->f_fatv+fatoff));
}
kfatcls_t kfatfs_readfat_32(struct kfatfs *self, kfatcls_t cluster) {
 size_t fatoff; kassertobj(self);
 fatoff = (size_t)((cluster*4) % self->f_fatsize);
 return (*(__u32 *)((uintptr_t)self->f_fatv+fatoff)) & 0x0FFFFFFF;
}
void kfatfs_writefat_12(struct kfatfs *self, kfatcls_t cluster, kfatcls_t value) {
 size_t fatoff; kassertobj(self);
 fatoff = (cluster+(cluster/2)) % self->f_fatsize;
 __u16 *val = ((__u16 *)((uintptr_t)self->f_fatv+fatoff));
 if (cluster&1) {
  *val = (*val&0xf)|(value << 4);
 } else {
  *val |= value&0xfff;
 }
}
void kfatfs_writefat_16(struct kfatfs *self, kfatcls_t cluster, kfatcls_t value) {
 size_t fatoff; kassertobj(self);
 fatoff = (cluster*2) % self->f_fatsize;
 (*(__u16 *)((uintptr_t)self->f_fatv+fatoff)) = value;
}
void kfatfs_writefat_32(struct kfatfs *self, kfatcls_t cluster, kfatcls_t value) {
 size_t fatoff; kassertobj(self);
 fatoff = (size_t)((cluster*4) % self->f_fatsize);
 *(__u32 *)((uintptr_t)self->f_fatv+fatoff) = value & 0x0FFFFFFF;
}


kerrno_t kfatfs_delheader(struct kfatfs *self, __u64 headpos) {
 return KE_NOSYS; // TODO
}
void kfatfs_delchain(struct kfatfs *self, kfatcls_t start) {
 // TODO
}


__DECL_END

#ifndef __INTELLISENSE__
#include "fat-internal-create.c.inl"
#include "fat-internal-dos8.3.inl"
#include "fat-internal-enum.c.inl"
#include "fat-internal-fatcache.c.inl"
#include "fat-internal-fhead.c.inl"
#include "fat-internal-init.c.inl"
#include "fat-internal-rw.c.inl"
#endif

#endif /* !__KOS_KERNEL_FS_FAT_INTERNAL_C_INL__ */

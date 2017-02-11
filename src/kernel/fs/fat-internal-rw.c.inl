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
#ifndef __KOS_KERNEL_FS_FAT_INTERNAL_RW_C_INL__
#define __KOS_KERNEL_FS_FAT_INTERNAL_RW_C_INL__ 1

#include <alloca.h>
#include <assert.h>
#include <kos/compiler.h>
#include <kos/kernel/debug.h>
#include <kos/errno.h>
#include <kos/kernel/fs/fat-internal.h>
#include <kos/kernel/types.h>
#include <math.h>
#include <sys/types.h>

__DECL_BEGIN

kerrno_t kfatfs_loadsectors(struct kfatfs const *self, kfatsec_t sec, size_t c, void *__restrict buf) {
 struct ksdev *sdev; kassertobj(self);
 kassertmem(buf,self->f_secsize);
 sdev = self->f_dev;
 kassertobj(sdev);
 assert(sdev->sd_blocksize);
 if (self->f_secsize == sdev->sd_blocksize) {
  // Simple case: Sectors and device blocks are aligned (meaning 'sec' is an LBA)
  if __unlikely(sec >= sdev->sd_blockcount) return KE_RANGE;
  return ksdev_readallblocks(sdev,sec,buf,c);
 } else {
  __u64 secaddr,lbaaddr; kslba_t lba;
  void *lbabuf; kerrno_t error;
  size_t addroff,lbacount,secbytes;
  // Must align 'sec' to an lba
  secaddr  = (__u64)sec*self->f_secsize;
  lba      = (kslba_t)(secaddr/sdev->sd_blocksize);
  lbaaddr  = (__u64)lba*sdev->sd_blocksize;
  secbytes = self->f_secsize*c;
  lbacount = ceildiv(secbytes,sdev->sd_blocksize);
  lbabuf   = alloca(sdev->sd_blocksize*lbacount);
  if __unlikely(lba >= sdev->sd_blockcount) return KE_RANGE;
  error    = ksdev_readallblocks(sdev,lba,lbabuf,lbacount);
  if (KE_ISOK(error)) {
   assert(lbaaddr <= secaddr);
   addroff  = (size_t)(secaddr-lbaaddr);
   memcpy(buf,(void *)((uintptr_t)lbabuf+addroff),secbytes);
  }
  return error;
 }
}
kerrno_t kfatfs_savesectors(struct kfatfs *self, kfatsec_t sec, size_t c, void const *__restrict buf) {
 struct ksdev *sdev; kassertobj(self);
 kassertmem(buf,self->f_secsize);
 sdev = self->f_dev;
 kassertobj(sdev);
 assert(sdev->sd_blocksize);
 if (self->f_secsize == sdev->sd_blocksize) {
  // Simple case: Sectors and device blocks are aligned (meaning 'sec' is an LBA)
  if __unlikely(sec >= sdev->sd_blockcount) return KE_RANGE;
  return ksdev_writeallblocks(sdev,sec,buf,c);
 } else {
  __u64 secaddr,lbaaddr; kslba_t lba;
  void *lbabuf; kerrno_t error;
  size_t addroff,lbacount,secbytes,lbabytes,lbaupper,lbaupperbytes;
  // Must align 'sec' to an lba
  secaddr  = (__u64)sec*self->f_secsize;
  lba      = (kslba_t)(secaddr/sdev->sd_blocksize);
  lbaaddr  = (__u64)lba*sdev->sd_blocksize;
  secbytes = self->f_secsize*c;
  lbacount = ceildiv(secbytes,sdev->sd_blocksize);
  lbabytes = sdev->sd_blocksize*lbacount;
  lbabuf   = alloca(lbabytes);
  assert(lbaaddr <= secaddr);
  addroff  = (size_t)(secaddr-lbaaddr);
  assert(lbabytes-addroff >= secbytes);
  assert((addroff % sdev->sd_blocksize) == 0);
  // Load blocks below the sector
  if __unlikely(lba >= sdev->sd_blockcount) return KE_RANGE;
  if __unlikely(addroff && KE_ISERR(error = ksdev_readallblocks(
   sdev,lba,lbabuf,addroff/sdev->sd_blocksize))) return error;
  memcpy((void *)((uintptr_t)lbabuf+addroff),buf,secbytes);
  // Load blocks above the sector
  lbaupperbytes = (size_t)(lbabytes-secbytes);
  assert((lbaupperbytes % sdev->sd_blocksize) == 0);
  if __unlikely(lbaupperbytes) {
   lbaupper = lbaupperbytes/sdev->sd_blocksize;
   if (KE_ISERR(error = ksdev_readallblocks(sdev
    ,lba+(lbacount-lbaupper)
    ,(void *)((uintptr_t)lbabuf+(lbabytes-lbaupperbytes))
    ,lbaupper))) return error;
  }
  return ksdev_writeallblocks(sdev,lba,lbabuf,lbacount);
 }
}

__DECL_END

#endif /* !__KOS_KERNEL_FS_FAT_INTERNAL_RW_C_INL__ */

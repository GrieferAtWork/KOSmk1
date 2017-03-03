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
#ifndef __KOS_KERNEL_DEV_STORAGE_C_INL__
#define __KOS_KERNEL_DEV_STORAGE_C_INL__ 1

#include <kos/compiler.h>
#include <kos/kernel/debug.h>
#include <kos/kernel/dev/storage-ata.h>
#include <kos/kernel/dev/storage-ramdisk.h>
#include <kos/kernel/dev/storage-part.h>
#include <kos/kernel/dev/storage.h>
#include <kos/errno.h>
#include <malloc.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

__DECL_BEGIN

kerrno_t
ksdev_readallblocks_unlocked(struct ksdev const *__restrict self, kslba_t addr,
                             __kernel void *__restrict blocks, size_t c) {
 size_t rc,nextc; kerrno_t error;
 error = ksdev_readblocks_unlocked(self,addr,blocks,c,&rc);
 if __unlikely(KE_ISERR(error)) return error;
 assert(rc <= c);
 if (rc != c) {
  nextc = rc;
  while (c) {
   *(uintptr_t *)&blocks += nextc*self->sd_blocksize,c -= nextc;
   error = ksdev_readblocks_unlocked(self,addr,blocks,c,&nextc);
   if __unlikely(KE_ISERR(error)) return error;
   assert(nextc <= c);
   rc += nextc;
  }
 }
 return KE_OK;
}
kerrno_t
ksdev_writeallblocks_unlocked(struct ksdev *__restrict self, kslba_t addr,
                              __kernel void const *__restrict blocks, size_t c) {
 size_t rc,nextc; kerrno_t error;
 error = ksdev_writeblocks_unlocked(self,addr,blocks,c,&rc);
 if __unlikely(KE_ISERR(error)) return error;
 assert(rc <= c);
 if (rc != c) {
  nextc = rc;
  while (c) {
   *(uintptr_t *)&blocks += nextc*self->sd_blocksize,c -= nextc;
   error = ksdev_writeblocks_unlocked(self,addr,blocks,c,&nextc);
   if __unlikely(KE_ISERR(error)) return error;
   assert(nextc <= c);
   rc += nextc;
  }
 }
 return KE_OK;
}


kerrno_t
ksdev_generic_readblock(struct ksdev const *__restrict self, kslba_t addr,
                        __kernel void *__restrict block) {
 size_t rc;
 kassertobj(self);
 kassertmem(block,self->sd_blocksize);
 assertf(addr < self->sd_blockcount,
         "Invalid block address (range)\n"
         "    addr       = %I64u\n"
         "    c          = 1\n"
         "    blockcount = %I64u\n"
         ,addr,self->sd_blockcount);
 kassertbyte(self->sd_readblocks);
 return (*self->sd_readblocks)(self,addr,block,1,&rc);
}
kerrno_t
ksdev_generic_writeblock(struct ksdev *__restrict self, kslba_t addr,
                         __kernel void const *__restrict block) {
 size_t wc;
 kassertobj(self);
 kassertmem(block,self->sd_blocksize);
 assertf(addr < self->sd_blockcount,
         "Invalid block address (range)\n"
         "    addr       = %I64u\n"
         "    c          = 1\n"
         "    blockcount = %I64u\n"
         ,addr,self->sd_blockcount);
 kassertbyte(self->sd_writeblocks);
 return (*self->sd_writeblocks)(self,addr,block,1,&wc);
}
kerrno_t
ksdev_generic_readblocks(struct ksdev const *__restrict self, kslba_t addr,
                         __kernel void *__restrict blocks, size_t c,
                         __kernel size_t *rc) {
 size_t i; kerrno_t error;
 kassertobj(self);
 kassertobj(rc);
 assertf(c,"Must at least specify 1 block");
 kassertmem(blocks,c*self->sd_blocksize);
 assertf(addr < self->sd_blockcount &&
         addr+c > addr &&
         addr+c < self->sd_blockcount,
         "Invalid block address (range)\n"
         "    addr       = %I64u\n"
         "    c          = %I64u\n"
         "    blockcount = %I64u\n"
         ,addr,c,self->sd_blockcount);
 kassertbyte(self->sd_readblock);
 for (i = 0; i < c; ++i) {
  if __unlikely(KE_ISERR(error = (*self->sd_readblock)(self,addr,blocks))) break;
 }
 *rc = i;
 return error;
}
kerrno_t
ksdev_generic_writeblocks(struct ksdev *__restrict self, kslba_t addr,
                          __kernel void const *__restrict blocks, size_t c,
                          __kernel size_t *wc) {
 size_t i; kerrno_t error;
 kassertobj(self);
 kassertobj(wc);
 assertf(c,"Must at least specify 1 block");
 kassertmem(blocks,c*self->sd_blocksize);
 assertf(addr < self->sd_blockcount &&
         addr+c > addr &&
         addr+c < self->sd_blockcount,
         "Invalid block address (range)\n"
         "    addr       = %I64u\n"
         "    c          = %I64u\n"
         "    blockcount = %I64u\n"
         ,addr,c,self->sd_blockcount);
 kassertbyte(self->sd_writeblock);
 for (i = 0; i < c; ++i) {
  if __unlikely(KE_ISERR(error = (*self->sd_writeblock)(self,addr,blocks))) break;
 }
 *wc = i;
 return error;
}

kerrno_t
ksdev_new_ramdisk(struct ksdev **__restrict result,
                  size_t blocksize, size_t blockcount) {
 struct krddev *resdisk; kerrno_t error;
 kassertobj(result);
 assertf(blocksize*blockcount != 0,
         "blocksize = %Iu\n"
         "blockcount = %Iu\n",
         blocksize,blockcount);
 resdisk = (struct krddev *)malloc(sizeof(struct krddev));
 if __unlikely(!resdisk) return KE_NOMEM;
 error = krddev_init(resdisk,blocksize,blockcount);
 if __unlikely(KE_ISERR(error)) free(resdisk);
 else *result = (struct ksdev *)resdisk;
 return error;
}
kerrno_t
ksdev_new_ata(struct ksdev **__restrict result,
              katabus_t bus, katadrive_t drive) {
 struct katadev *resdisk; kerrno_t error;
 kassertobj(result);
 resdisk = (struct katadev *)malloc(sizeof(struct katadev));
 if __unlikely(!resdisk) return KE_NOMEM;
 error = katadev_init(resdisk,bus,drive);
 if __unlikely(KE_ISERR(error)) free(resdisk);
 else *result = (struct ksdev *)resdisk;
 return error;
}
kerrno_t
ksdev_new_findfirstata(struct ksdev **__restrict result) {
 struct katadev *resdisk; kerrno_t error;
 kassertobj(result);
 resdisk = (struct katadev *)malloc(sizeof(struct katadev));
 if __unlikely(!resdisk) return KE_NOMEM;
 if __unlikely(KE_ISERR(error = katadev_init(resdisk,ATA_IOPORT_PRIMARY,ATA_DRIVE_MASTER)))
 if __unlikely(KE_ISERR(error = katadev_init(resdisk,ATA_IOPORT_PRIMARY,ATA_DRIVE_SLAVE)))
 if __unlikely(KE_ISERR(error = katadev_init(resdisk,ATA_IOPORT_SECONDARY,ATA_DRIVE_MASTER)))
 if __unlikely(KE_ISERR(error = katadev_init(resdisk,ATA_IOPORT_SECONDARY,ATA_DRIVE_SLAVE))) free(resdisk);
 *result = (struct ksdev *)resdisk;
 return error;
}
kerrno_t
ksdev_new_part(struct ksdev **__restrict result,
               struct ksdev *__restrict dev,
               kslba_t start, kslba_t size) {
 struct kpartdev *resdisk;
 kassertobj(result); kassertobj(dev);
 assertf(start+size > size,"Empty size, or lba rolls over");
 assertf(start+size <= dev->sd_blockcount
         ,"Overflowing LBA address (%I64u > %I64u)"
         ,start+size,dev->sd_blockcount);
 resdisk = (struct kpartdev *)malloc(sizeof(struct kpartdev));
 if __unlikely(!resdisk) return KE_NOMEM;
 kpartdev_create(resdisk,dev,start,size);
 *result = (struct ksdev *)resdisk;
 return KE_OK;
}


__DECL_END

#ifndef __INTELLISENSE__
#include "storage-ata.c.inl"
#include "storage-diskinfo512.c.inl"
#include "storage-part.c.inl"
#include "storage-ramdisk.c.inl"
#endif

#endif /* !__KOS_KERNEL_DEV_STORAGE_C_INL__ */

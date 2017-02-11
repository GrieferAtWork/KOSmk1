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
#ifndef __KOS_KERNEL_DEV_STORAGE_PART_C_INL__
#define __KOS_KERNEL_DEV_STORAGE_PART_C_INL__ 1

#include <kos/compiler.h>
#include <kos/kernel/dev/storage.h>
#include <kos/kernel/dev/storage-part.h>

__DECL_BEGIN

kerrno_t kpartdev_readblock(kpartdev_t const *self, kslba_t addr, void *block) {
 kassertobj(self);
 kassertmem(block,self->sd_blocksize);
 assertf(addr < self->sd_blockcount,
         "Invalid block address (range)\n"
         "    addr       = %I64u\n"
         "    c          = 1\n"
         "    blockcount = %I64u\n"
         ,addr,self->sd_blockcount);
 assertf(addr+self->pd_start >= addr,
         "Overflow when adding offset to LBA address (%I64u < %I64u)",
         addr+self->pd_start,addr);
 return ksdev_readblock_unlocked(self->pd_disk,self->pd_start+addr,block);
}
kerrno_t kpartdev_writeblock(kpartdev_t *self, kslba_t addr, void const *block) {
 kassertobj(self);
 kassertmem(block,self->sd_blocksize);
 assertf(addr < self->sd_blockcount,
         "Invalid block address (range)\n"
         "    addr       = %I64u\n"
         "    c          = 1\n"
         "    blockcount = %I64u\n"
         ,addr,self->sd_blockcount);
 assertf(addr+self->pd_start >= addr,
         "Overflow when adding offset to LBA address (%I64u < %I64u)",
         addr+self->pd_start,addr);
 return ksdev_writeblock_unlocked(self->pd_disk,self->pd_start+addr,block);
}
kerrno_t kpartdev_readblocks(kpartdev_t const *self, kslba_t addr, void *blocks, __size_t c, __size_t *rc) {
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
 assertf(addr+self->pd_start >= addr,
         "Overflow when adding offset to LBA address (%I64u < %I64u)",
         addr+self->pd_start,addr);
 return ksdev_readblocks_unlocked(self->pd_disk,self->pd_start+addr,blocks,c,rc);
}
kerrno_t kpartdev_writeblocks(kpartdev_t *self, kslba_t addr, void const *blocks, __size_t c, __size_t *wc) {
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
 assertf(addr+self->pd_start >= addr,
         "Overflow when adding offset to LBA address (%I64u < %I64u)",
         addr+self->pd_start,addr);
 return ksdev_writeblocks_unlocked(self->pd_disk,self->pd_start+addr,blocks,c,wc);
}


kerrno_t kpartdev_getattr(kpartdev_t const *self, kattr_t attr, void *__restrict buf, __size_t bufsize, __size_t *__restrict reqsize) {
 kassertobj(self);
 kassertmem(buf,bufsize);
 kassertobjnull(reqsize);
 return kdev_getattr_unlocked((struct kdev *)self->pd_disk,attr,buf,bufsize,reqsize);
}
kerrno_t kpartdev_setattr(kpartdev_t *self, kattr_t attr, void const *__restrict buf, __size_t bufsize) {
 kassertobj(self);
 kassertmem(buf,bufsize);
 return kdev_setattr_unlocked((struct kdev *)self->pd_disk,attr,buf,bufsize);
}


__DECL_END

#endif /* !__KOS_KERNEL_DEV_STORAGE_PART_C_INL__ */

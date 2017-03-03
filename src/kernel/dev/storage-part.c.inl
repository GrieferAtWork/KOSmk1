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

kerrno_t
kpartdev_readblock(struct kpartdev const *__restrict self, kslba_t addr,
                   __kernel void *__restrict block) {
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
kerrno_t
kpartdev_writeblock(struct kpartdev *__restrict self, kslba_t addr,
                    __kernel void const *__restrict block) {
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
kerrno_t
kpartdev_readblocks(struct kpartdev const *__restrict self, kslba_t addr,
                    __kernel void *__restrict blocks, size_t c,
                    __kernel size_t *rc) {
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
kerrno_t
kpartdev_writeblocks(struct kpartdev *__restrict self, kslba_t addr,
                     __kernel void const *__restrict blocks, size_t c,
                     __kernel size_t *wc) {
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


kerrno_t
kpartdev_getattr(struct kpartdev const *__restrict self, kattr_t attr,
                 __user void *__restrict buf, size_t bufsize,
                 __kernel size_t *__restrict reqsize) {
 kassertobj(self);
 kassertmem(buf,bufsize);
 kassertobjnull(reqsize);
 return kdev_user_getattr_unlocked((struct kdev *)self->pd_disk,
                                    attr,buf,bufsize,reqsize);
}
kerrno_t
kpartdev_setattr(struct kpartdev *__restrict self, kattr_t attr,
                 __user void const *__restrict buf, size_t bufsize) {
 kassertobj(self);
 kassertmem(buf,bufsize);
 return kdev_user_setattr_unlocked((struct kdev *)self->pd_disk,
                                    attr,buf,bufsize);
}


__DECL_END

#endif /* !__KOS_KERNEL_DEV_STORAGE_PART_C_INL__ */

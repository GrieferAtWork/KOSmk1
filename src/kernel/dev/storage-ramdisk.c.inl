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
#ifndef __KOS_KERNEL_DEV_STORAGE_RAMDISK_C_INL__
#define __KOS_KERNEL_DEV_STORAGE_RAMDISK_C_INL__ 1

#include <kos/compiler.h>
#include <kos/kernel/dev/storage-ramdisk.h>
#include <kos/kernel/mutex.h>
#include <malloc.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

__DECL_BEGIN

static void krddev_finalize(krddev_t *self) { free(self->rd_begin); }
static kerrno_t krddev_readblock(krddev_t const *self, kslba_t addr, void *block) {
 uintptr_t bufdest;
 kassertobj(self);
 kassertmem(block,self->sd_blocksize);
 assertf(addr < self->sd_blockcount,
         "Invalid block address (range)\n"
         "    addr       = %I64u\n"
         "    c          = 1\n"
         "    blockcount = %I64u\n"
         ,addr,self->sd_blockcount);
 bufdest = ((uintptr_t)self->rd_begin+(size_t)addr*self->sd_blocksize);
 memcpy(block,(void *)bufdest,self->sd_blocksize);
 return KE_OK;
}
static kerrno_t krddev_writeblock(krddev_t *self, kslba_t addr, void const *block) {
 uintptr_t bufdest;
 kassertobj(self);
 kassertmem(block,self->sd_blocksize);
 assertf(addr < self->sd_blockcount,
         "Invalid block address (range)\n"
         "    addr       = %I64u\n"
         "    c          = 1\n"
         "    blockcount = %I64u\n"
         ,addr,self->sd_blockcount);
 bufdest = ((uintptr_t)self->rd_begin+(size_t)addr*self->sd_blocksize);
 memcpy((void *)bufdest,block,self->sd_blocksize);
 return KE_OK;
}
static kerrno_t krddev_readblocks(krddev_t const *self, kslba_t addr,
                                  void *blocks, size_t c, size_t *rc) {
 uintptr_t bufdest;
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
 bufdest = ((uintptr_t)self->rd_begin+(size_t)addr*self->sd_blocksize);
 memcpy(blocks,(void *)bufdest,c*self->sd_blocksize);
 *rc = c;
 return KE_OK;
}
static kerrno_t krddev_writeblocks(krddev_t *self, kslba_t addr,
                                   void const *blocks, size_t c, size_t *wc) {
 uintptr_t bufdest;
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
 bufdest = ((uintptr_t)self->rd_begin+(size_t)addr*self->sd_blocksize);
 memcpy((void *)bufdest,blocks,c*self->sd_blocksize);
 *wc = c;
 return KE_OK;
}


static kerrno_t krddev_getattr(
 krddev_t const *self, kattr_t attr,
 void *__restrict buf, size_t bufsize, size_t *__restrict reqsize) {
 kassertobj(self);
 kassertmem(buf,bufsize);
 kassertobjnull(reqsize);
 switch (attr) {
  case KATTR_GENERIC_NAME:
  case KATTR_DEV_NAME: {
   size_t usedsize; size_t capacity;
   capacity = ksdev_getcapacity((struct ksdev *)self);
   if (capacity > 1024*1024*1024) {
    usedsize = snprintf((char *)buf,bufsize,"%IuGb RamDisk",capacity/(1024*1024*1024));
   } else if (capacity > 1024*1024) {
    usedsize = snprintf((char *)buf,bufsize,"%IuMb RamDisk",capacity/(1024*1024));
   } else if (capacity > 1024) {
    usedsize = snprintf((char *)buf,bufsize,"%IuKb RamDisk",capacity/1024);
   } else {
    usedsize = snprintf((char *)buf,bufsize,"%Iub RamDisk",capacity);
   }
   if (reqsize) *reqsize = usedsize;
   return KE_OK;
  } break;

  case KATTR_DEV_TYPE: {
   size_t usedsize = snprintf((char *)buf,bufsize,"RAMDISK");
   if (reqsize) *reqsize = usedsize;
   return KE_OK;
  } break;
   
  case KATTR_DEV_PARTITIONS:
   if (self->sd_blocksize == 512)
    return ksdev_generic_getdiskinfo512((struct ksdev *)self,(struct ksdev_diskinfo *)buf,bufsize,reqsize);
   break;
  case KATTR_DEV_MAXDISKNAME:
   if (self->sd_blocksize == 512) {
    if (reqsize) *reqsize = sizeof(size_t);
    if (bufsize >= sizeof(size_t)) *(size_t *)buf = KSDEV_GENERIC_MAXDISKNAMESIZE512;
    return KE_OK;
   }
   break;
  case KATTR_DEV_DISKNAME:
   if (self->sd_blocksize == 512)
    return ksdev_generic_getdiskname512((struct ksdev *)self,(char *)buf,bufsize,reqsize);
   break;

  case KATTR_DEV_RAMSIZE: {
   if (reqsize) *reqsize = sizeof(struct ksdev_ramsize);
   if (bufsize >= sizeof(struct ksdev_ramsize)) {
    struct ksdev_ramsize *info = (struct ksdev_ramsize *)buf;
    info->blockcount = self->sd_blockcount;
    info->blocksize  = self->sd_blocksize;
   }
   return KE_OK;
  } break;
   
  default: break;
 }
 return KE_NOSYS;
}

static kerrno_t krddev_setattr(
 krddev_t *self, kattr_t attr, void const *__restrict buf, size_t bufsize) {
 kassertobj(self);
 kassertmem(buf,bufsize);
 switch (attr) {

  case KATTR_DEV_PARTITIONS:
   if (self->sd_blocksize == 512)
    return ksdev_generic_setdiskinfo512((struct ksdev *)self,(struct ksdev_diskinfo *)buf,bufsize);
   break;
  case KATTR_DEV_DISKNAME:
   if (self->sd_blocksize == 512)
    return ksdev_generic_setdiskname512((struct ksdev *)self,(char *)buf,bufsize);
   break;

  case KATTR_DEV_RAMSIZE: {
   size_t newcapacity; void *newmem;
   struct ksdev_ramsize const *info = (struct ksdev_ramsize const *)buf;
   if __unlikely(bufsize < sizeof(struct ksdev_ramsize)) return KE_BUFSIZ;
   kassertobj(info);
   newcapacity = (info->blockcount*info->blocksize);
   if (!newcapacity) return KE_RANGE;
   newmem = realloc(self->rd_begin,newcapacity);
   if (!newmem) return KE_NOMEM;
   self->rd_begin = newmem;
   self->rd_end = (void *)((uintptr_t)newmem+newcapacity);
   self->sd_blocksize = info->blocksize;
   self->sd_blockcount = info->blockcount;
  } break;
   
  default: break;
 }
 return KE_NOSYS;
}



kerrno_t krddev_create(krddev_t *self, size_t blocksize, size_t blockcount) {
 size_t memsize;
 kassertobj(self); memsize = blocksize*blockcount;
 assertf(memsize != 0,"Can't create empty ramdisk");
 self->rd_begin = malloc(memsize);
 if __unlikely(!self->rd_begin) return KE_NOMEM;
 self->rd_end          = (void *)((uintptr_t)self->rd_begin+memsize);
 self->sd_blocksize   = blocksize;
 self->sd_blockcount  = blockcount;
 self->sd_readblock   = (kerrno_t(*)(struct ksdev const *,kslba_t,void *))&krddev_readblock;
 self->sd_writeblock  = (kerrno_t(*)(struct ksdev *,kslba_t,void const *))&krddev_writeblock;
 self->sd_readblocks  = (kerrno_t(*)(struct ksdev const *,kslba_t,void *,size_t,size_t *))&krddev_readblocks;
 self->sd_writeblocks = (kerrno_t(*)(struct ksdev *,kslba_t,void const *,size_t,size_t *))&krddev_writeblocks;
 self->d_quit         = (void(*)(struct kdev *))&krddev_finalize;
 self->d_getattr      = (kerrno_t(*)(struct kdev const *,kattr_t,void *,size_t,size_t *))&krddev_getattr;
 self->d_setattr      = (kerrno_t(*)(struct kdev *,kattr_t,const void *,size_t))&krddev_setattr;
 kdev_init(self);
 return KE_OK;
}


__DECL_END

#endif /* !__KOS_KERNEL_DEV_STORAGE_RAMDISK_C_INL__ */

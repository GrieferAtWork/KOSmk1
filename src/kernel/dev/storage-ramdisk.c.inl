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
#include <kos/kernel/util/string.h>
#include <malloc.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

__DECL_BEGIN

static void
krddev_quit(struct krddev *__restrict self) {
 free(self->rd_begin);
}

static kerrno_t
krddev_readblock(struct krddev const *__restrict self, kslba_t addr,
                 __kernel void *__restrict block) {
 uintptr_t bufdest;
 kassertobj(self);
 kassertmem(block,self->sd_blocksize);
 assertf(addr < self->sd_blockcount,
         "Invalid block address (range)\n"
         "    addr       = %I64u\n"
         "    c          = 1\n"
         "    blockcount = %I64u\n",
         addr,self->sd_blockcount);
 bufdest = ((uintptr_t)self->rd_begin+(size_t)addr*self->sd_blocksize);
 memcpy(block,(void *)bufdest,self->sd_blocksize);
 return KE_OK;
}
static kerrno_t
krddev_writeblock(struct krddev *__restrict self, kslba_t addr,
                  __kernel void const *__restrict block) {
 uintptr_t bufdest;
 kassertobj(self);
 kassertmem(block,self->sd_blocksize);
 assertf(addr < self->sd_blockcount,
         "Invalid block address (range)\n"
         "    addr       = %I64u\n"
         "    c          = 1\n"
         "    blockcount = %I64u\n",
         addr,self->sd_blockcount);
 bufdest = ((uintptr_t)self->rd_begin+(size_t)addr*self->sd_blocksize);
 memcpy((void *)bufdest,block,self->sd_blocksize);
 return KE_OK;
}
static kerrno_t
krddev_readblocks(struct krddev const *__restrict self, kslba_t addr,
                  __kernel void *__restrict blocks, size_t c,
                  __kernel size_t *rc) {
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
         "    blockcount = %I64u\n",
         addr,c,self->sd_blockcount);
 bufdest = ((uintptr_t)self->rd_begin+(size_t)addr*self->sd_blocksize);
 memcpy(blocks,(void *)bufdest,c*self->sd_blocksize);
 *rc = c;
 return KE_OK;
}
static kerrno_t
krddev_writeblocks(struct krddev *__restrict self, kslba_t addr,
                   __kernel void const *__restrict blocks, size_t c,
                   __kernel size_t *wc) {
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


static kerrno_t
krddev_getattr(struct krddev const *__restrict self, kattr_t attr,
               __user void *__restrict buf, size_t bufsize,
               __kernel size_t *__restrict reqsize) {
 kassertobj(self);
 kassertobjnull(reqsize);
 switch (attr) {
  case KATTR_GENERIC_NAME:
  case KATTR_DEV_NAME: {
   ssize_t error; size_t capacity;
   capacity = ksdev_getcapacity((struct ksdev *)self);
   if (capacity > 1024*1024*1024) {
    error = user_snprintf((__user char *)buf,bufsize,reqsize,
                          "%IuGb RamDisk",capacity/(1024*1024*1024));
   } else if (capacity > 1024*1024) {
    error = user_snprintf((__user char *)buf,bufsize,reqsize,
                          "%IuMb RamDisk",capacity/(1024*1024));
   } else if (capacity > 1024) {
    error = user_snprintf((__user char *)buf,bufsize,reqsize,
                          "%IuKb RamDisk",capacity/1024);
   } else {
    error = user_snprintf((char *)buf,bufsize,reqsize,
                          "%Iub RamDisk",capacity);
   }
   return error < 0 ? KE_FAULT : KE_OK;
  } break;

  case KATTR_DEV_TYPE: {
   return user_snprintf((char *)buf,bufsize,reqsize,"RAMDISK") < 0 ? KE_FAULT : KE_OK;
  } break;
   
  case KATTR_DEV_PARTITIONS:
   if (self->sd_blocksize == 512) {
    /* TODO: user-pointer translation. */
    return ksdev_generic_getdiskinfo512((struct ksdev *)self,
                                        (struct ksdev_diskinfo *)buf,
                                         bufsize,reqsize);
   }
   break;
  case KATTR_DEV_MAXDISKNAME:
   if (self->sd_blocksize == 512) {
    if (reqsize) *reqsize = sizeof(size_t);
    if (bufsize >= sizeof(size_t)) {
     size_t val = KSDEV_GENERIC_MAXDISKNAMESIZE512;
     if __unlikely(copy_to_user(buf,&val,sizeof(size_t))) return KE_FAULT;
    }
    return KE_OK;
   }
   break;
  case KATTR_DEV_DISKNAME:
   if (self->sd_blocksize == 512) {
    /* TODO: user-pointer translation. */
    return ksdev_generic_getdiskname512((struct ksdev *)self,
                                        (char *)buf,bufsize,reqsize);
   }
   break;

  case KATTR_DEV_RAMSIZE: {
   struct ksdev_ramsize info;
   if (reqsize) *reqsize = sizeof(struct ksdev_ramsize);
   info.blockcount = self->sd_blockcount;
   info.blocksize  = self->sd_blocksize;
   if __unlikely(copy_to_user(buf,&info,
                 min(bufsize,sizeof(struct ksdev_ramsize))
                 )) return KE_FAULT;
   return KE_OK;
  } break;
   
  default: break;
 }
 return KE_NOSYS;
}

static kerrno_t
krddev_setattr(struct krddev *__restrict self, kattr_t attr,
               __user void const *__restrict buf, size_t bufsize) {
 kassertobj(self);
 kassertmem(buf,bufsize);
 switch (attr) {

  case KATTR_DEV_PARTITIONS:
   if (self->sd_blocksize == 512) {
    /* TODO: user-pointer translation. */
    return ksdev_generic_setdiskinfo512((struct ksdev *)self,
                                        (struct ksdev_diskinfo *)buf,
                                         bufsize);
   }
   break;
  case KATTR_DEV_DISKNAME:
   if (self->sd_blocksize == 512) {
    /* TODO: user-pointer translation. */
    return ksdev_generic_setdiskname512((struct ksdev *)self,
                                        (char *)buf,bufsize);
   }
   break;

  case KATTR_DEV_RAMSIZE: {
   struct ksdev_ramsize info;
   size_t newcapacity; void *newmem;
   if __unlikely(bufsize < sizeof(struct ksdev_ramsize)) return KE_BUFSIZ;
   if __unlikely(copy_from_user(&info,buf,sizeof(struct ksdev_ramsize))) return KE_FAULT;
   newcapacity = (info.blockcount*info.blocksize);
   if __unlikely(!newcapacity) return KE_RANGE;
   newmem = realloc(self->rd_begin,newcapacity);
   if __unlikely(!newmem) return KE_NOMEM;
   self->rd_begin      = newmem;
   self->rd_end        = (void *)((uintptr_t)newmem+newcapacity);
   self->sd_blocksize  = info.blocksize;
   self->sd_blockcount = info.blockcount;
  } break;
   
  default: break;
 }
 return KE_NOSYS;
}



kerrno_t
krddev_init(struct krddev *__restrict self,
            size_t blocksize,
            size_t blockcount) {
 size_t memsize;
 kassertobj(self);
 memsize = blocksize*blockcount;
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
 self->d_quit         = (void(*)(struct kdev *))&krddev_quit;
 self->d_getattr      = (kerrno_t(*)(struct kdev const *,kattr_t,void *,size_t,size_t *))&krddev_getattr;
 self->d_setattr      = (kerrno_t(*)(struct kdev *,kattr_t,const void *,size_t))&krddev_setattr;
 kdev_init(self);
 return KE_OK;
}


__DECL_END

#endif /* !__KOS_KERNEL_DEV_STORAGE_RAMDISK_C_INL__ */

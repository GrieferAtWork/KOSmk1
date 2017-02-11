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
#ifndef __KOS_KERNEL_DEV_STORAGE_HDDPART_H__
#define __KOS_KERNEL_DEV_STORAGE_HDDPART_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/compiler.h>
#include <kos/attr.h>
#include <kos/kernel/types.h>
#include <kos/errno.h>
#include <kos/kernel/dev/storage.h>
#include <kos/kernel/debug.h>

__DECL_BEGIN

typedef struct __kpartdev kpartdev_t;

struct __kpartdev { /*< Disk partition device. */
 KSDEV_HEAD
 struct ksdev *pd_disk;  /*< [1..1] Underlying disk (NOTE: Weakly referenced). */
 kslba_t  pd_start; /*< Partition start. */
};

extern __wunused __nonnull((1,3))   kerrno_t kpartdev_readblock(kpartdev_t const *self, kslba_t addr, void *block);
extern __wunused __nonnull((1,3))   kerrno_t kpartdev_writeblock(kpartdev_t *self, kslba_t addr, void const *block);
extern __wunused __nonnull((1,3,5)) kerrno_t kpartdev_readblocks(kpartdev_t const *self, kslba_t addr, void *blocks, __size_t c, __size_t *rc);
extern __wunused __nonnull((1,3,5)) kerrno_t kpartdev_writeblocks(kpartdev_t *self, kslba_t addr, void const *blocks, __size_t c, __size_t *wc);
extern __wunused __nonnull((1,3))   kerrno_t kpartdev_getattr(kpartdev_t const *self, kattr_t attr, void *__restrict buf, __size_t bufsize, __size_t *__restrict reqsize);
extern __wunused __nonnull((1,3))   kerrno_t kpartdev_setattr(kpartdev_t *self, kattr_t attr, void const *__restrict buf, __size_t bufsize);

__local __nonnull((1,2)) void kpartdev_create(
 kpartdev_t *self, struct ksdev *dev, kslba_t start, kslba_t size) {
 kassertobj(self); kassertobj(dev);
 assertf(start+size > size,"Empty size, or lba rolls over");
 assertf(start+size <= dev->sd_blockcount
         ,"Overflowing LBA address (%I64u > %I64u)"
         ,start+size,dev->sd_blockcount);
 kdev_init(self);
 self->d_quit         = NULL;
 self->sd_blocksize   = dev->sd_blocksize;
 self->sd_blockcount  = size;
 self->sd_readblock   = (kerrno_t(*)(struct ksdev const *,kslba_t,void *))&kpartdev_readblock;
 self->sd_writeblock  = (kerrno_t(*)(struct ksdev *,kslba_t,void const *))&kpartdev_writeblock;
 self->sd_readblocks  = (kerrno_t(*)(struct ksdev const *,kslba_t,void *,__size_t,__size_t *))&kpartdev_readblocks;
 self->sd_writeblocks = (kerrno_t(*)(struct ksdev *,kslba_t,void const *,__size_t,__size_t *))&kpartdev_writeblocks;
 self->d_getattr      = (kerrno_t(*)(struct kdev const *,kattr_t,void *,__size_t,__size_t *))&kpartdev_getattr;
 self->d_setattr      = (kerrno_t(*)(struct kdev *,kattr_t,void const *,__size_t))&kpartdev_setattr;
 if (dev->sd_readblock == (kerrno_t(*)(struct ksdev const *,kslba_t,void *))&kpartdev_readblock) {
  // Optimize sub-partitioning of partitions
  self->pd_disk  = ((kpartdev_t *)dev)->pd_disk;
  self->pd_start = ((kpartdev_t *)dev)->pd_start+start;
 } else {
  self->pd_disk  = dev;
  self->pd_start = start;
 }
}

__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_DEV_STORAGE_HDDPART_H__ */

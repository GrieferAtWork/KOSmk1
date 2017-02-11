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
#ifndef __KOS_KERNEL_DEV_STORAGE_H__
#define __KOS_KERNEL_DEV_STORAGE_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/compiler.h>
#include <kos/kernel/dev/device.h>
#include <kos/kernel/dev/fstype.h>
#include <kos/errno.h>
#include <kos/kernel/mutex.h>
#include <kos/kernel/types.h>
#include <stddef.h>

__DECL_BEGIN

struct ksdev;
typedef __u64 kslba_t; /*< Kernel storage logical block address. */

#define KSDEV_HEAD \
 KDEV_HEAD \
 /* The following members are constant */\
 __size_t   sd_blocksize;  /*< [const] Size of a single block. */\
 kslba_t    sd_blockcount; /*< [const] Total amount of available blocks (0..sd_blockcount-1) */\
 kerrno_t (*sd_readblock)(struct ksdev const *self, kslba_t addr, void *block);\
 kerrno_t (*sd_writeblock)(struct ksdev *self, kslba_t addr, void const *block);\
 kerrno_t (*sd_readblocks)(struct ksdev const *self, kslba_t addr, void *block, __size_t c, __size_t *rc);\
 kerrno_t (*sd_writeblocks)(struct ksdev *self, kslba_t addr, void const *block, __size_t c, __size_t *wc);

struct ksdev { KSDEV_HEAD };

#define ksdev_incref(self) kdev_incref((struct kdev *)(self))
#define ksdev_decref(self) kdev_decref((struct kdev *)(self))

#define ksdev_getcapacity(self) \
 __xblock({ struct ksdev const *const __ksgc_self = (self);\
            __xreturn __ksgc_self->sd_blocksize*__ksgc_self->sd_blockcount;\
 })


//////////////////////////////////////////////////////////////////////////
// [RW] struct ksdev_diskinfo - KATTR_DEV_PARTITIONS
//        Get/Set Partitioning information on the given device.
//      NOTES:
//       - Setting the partitioning information will likely
//         break any existing filesystem, something the
//         caller is responsible for preventing/fixing.
//       - When partitioning, only the first 'n' partitions
//         are formatted to be considered primary partitions.
//      HINTS:
//       - After reading this information, you can use 'ksdev_new_part'
//         to create sub-partitions of a master partition device.
//       - Recursive partition tables will already be resolved by this getter.
//        (On ata-hdd devices, this is will enumerate all secondary
//         partitions, just like primary ones. Also note, that this kernel
//         allows for an infinite amount of secondary hdd partitions by
//         enabling extended partitions to be recursive, as well as allow
//         more than a single extended partition entry)
//      @param: buf:     [get|set] A 'struct ksdev_diskinfo' structure
//      @param: bufsize: [get|set] The amount of available buffer bytes.
//      @return: * :          [get|set] Same as reading/writing to/from the device.
//      @return: KE_DEVICE:   [get|set] The storage device encountered an error.
//      @return: KE_RANGE:    [set] The given name is too long (for the device).
//                                  The device-specific max-name length can
//                                  be queried with 'KATTR_DEV_MAXDISKNAME'
//                                  NOTE: May also be returned if a given partition is out-of-bounds.
//      @return: KE_OVERFLOW: [set] Too many partitions where specified.
//                                  NOTE: Some devices may never return this error.
//      @return: KE_INVAL:    [get] The partition record is broken
#define KATTR_DEV_PARTITIONS    \
 KATTR_BVAR(KATTR_TOKEN_DEV,KATTR_GROUP_DEV_STORAGE,0,\
            char[KSDEV_DISKINFO_MAXNAMESIZE],struct ksdev_partinfo)

struct ksdev_partinfo {
 kslba_t   partstart; /*< Start lba for the partition. */
 kslba_t   partsize;  /*< Size of the partition. */
 kfstype_t fstype;    /*< File system type (from <kos/kernel/dev/fstype.h>). */
#define KSDEV_PARTINFO_FLAG_NONE    0x00
#define KSDEV_PARTINFO_FLAG_PRIMARY 0x01 /*< Primary partition (When writing, this flag is ignored;
                                             Only the first few partitions will be are primary, then). */
 __u8      fsflags;   /*< Partition flags. */
};

#define KSDEV_DISKINFOBUFSIZE(npart)\
 (offsetof(struct ksdev_diskinfo,partitions)+(npart)*sizeof(struct ksdev_partinfo))

struct ksdev_diskinfo {
#define KSDEV_DISKINFO_MAXNAMESIZE 16 /* Max amount of disk name characters (of any format) */
 char                  name[KSDEV_DISKINFO_MAXNAMESIZE]; /*< (possibly) Zero-terminated disk name. */
 struct ksdev_partinfo partitions[1]; /*< Actual size depends on 'bufsize' / '*reqsize' */
};

//////////////////////////////////////////////////////////////////////////
// [RO] size_t - KATTR_DEV_MAXDISKNAME  (In characters)
// [RW] char[] - KATTR_DEV_DISKNAME     (Including \0 character)
//      Get the longest name size, or get/set the name of a disk.
//      NOTE: The name of any disk may not necessarily be human-readable.
//            It might possibly even contain \0 characters.
//      @return: * :       [get|set] Same as reading/writing to/from the device.
//      @return: KE_INVAL: [get|set] Partition record is missing/broken
//      @return: KE_RANGE: [set] The given name is too long (for the device).
#define KATTR_DEV_MAXDISKNAME   KATTR(KATTR_TOKEN_DEV,KATTR_GROUP_DEV_STORAGE,1,size_t)
#define KATTR_DEV_DISKNAME      KATTR_VAR(KATTR_TOKEN_DEV,KATTR_GROUP_DEV_STORAGE,2,char)


//////////////////////////////////////////////////////////////////////////
// [RW] Get/Set information of a RAMDISK (Can be used to resize ramdisks)
//      NOTE: The caller is responsible for synchronizing changes to the
//            otherwise constant 'sd_blocksize' and 'sd_blockcount' attributes.
//      @param: buf: [get|set] 'struct ksdev_ramsize'
//      @return: KE_NOMEM: [set] Not enough memory available for re-allocation
//      @return: KE_RANGE: [set] Invalid disk size
#define KATTR_DEV_RAMSIZE    KATTR_VAR(KATTR_TOKEN_DEV,KATTR_GROUP_DEV_STORAGE,3,struct ksdev_ramsize)
struct ksdev_ramsize {
 __size_t blocksize;  /*< Size of a single block. */
 kslba_t  blockcount; /*< Total amount of available blocks. */
};


#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Read/Write blocks of memory
// NOTES:
//  - The caller is responsible for locking 'd_rwlock'.
//    He is also responsible for ensuring proper ranges for 'addr' and 'c'
//  - In all error cases, the amount of read/written blocks will not equal 'c', or '1',
//    and the amount of successfully read/written blocks is stored in '*rc'/'*wc'
//  - In the case of success, not all blocks may have been read/written, with
//    the amount of actually read/written blocks stored in '*rc'/'*wc'.
//    In this case, KE_OK is returned and the caller is responsible for
//    advancing and execution the IO operation for what is left.
//    >> It is guarantied, that at least 1 block is read/written on success.
// @return: KE_OK:        Everything's OK.
// @return: KE_DESTROYED: The given device was closed.
// @return: KE_DEVICE:    The given device is broken.
extern __wunused kerrno_t ksdev_readblock(struct ksdev const *self, kslba_t addr, void *block);
extern __wunused kerrno_t ksdev_writeblock(struct ksdev *self, kslba_t addr, void const *block);
extern __wunused kerrno_t ksdev_readblocks(struct ksdev const *self, kslba_t addr, void *block, __size_t c, __size_t *rc);
extern __wunused kerrno_t ksdev_writeblocks(struct ksdev *self, kslba_t addr, void const *block, __size_t c, __size_t *wc);
extern __wunused kerrno_t ksdev_readblock_unlocked(struct ksdev const *self, kslba_t addr, void *block);
extern __wunused kerrno_t ksdev_writeblock_unlocked(struct ksdev *self, kslba_t addr, void const *block);
extern __wunused kerrno_t ksdev_readblocks_unlocked(struct ksdev const *self, kslba_t addr, void *block, __size_t c, __size_t *rc);
extern __wunused kerrno_t ksdev_writeblocks_unlocked(struct ksdev *self, kslba_t addr, void const *block, __size_t c, __size_t *wc);
extern __wunused kerrno_t ksdev_readallblocks(struct ksdev const *self, kslba_t addr, void *block, __size_t c);
extern __wunused kerrno_t ksdev_writeallblocks(struct ksdev *self, kslba_t addr, void const *block, __size_t c);
#else
#define ksdev_readblock_unlocked(self,addr,block) \
 __xblock({ struct ksdev const *const __ksrbu_self = (self);\
            kassertbyte(__ksrbu_self->sd_readblock);\
            __xreturn (*__ksrbu_self->sd_readblock)(__ksrbu_self,addr,block);\
 })
#define ksdev_writeblock_unlocked(self,addr,block) \
 __xblock({ struct ksdev *const __kswbu_self = (self);\
            kassertbyte(__kswbu_self->sd_writeblock);\
            __xreturn (*__kswbu_self->sd_writeblock)(__kswbu_self,addr,block);\
 })
#define ksdev_readblocks_unlocked(self,addr,block,c,rc) \
 __xblock({ struct ksdev const *const __ksrbsu_self = (self);\
            kassertbyte(__ksrbsu_self->sd_readblocks);\
            __xreturn (*__ksrbsu_self->sd_readblocks)(__ksrbsu_self,addr,block,c,rc);\
 })
#define ksdev_writeblocks_unlocked(self,addr,block,c,wc) \
 __xblock({ struct ksdev *const __kswbsu_self = (self);\
            kassertbyte(__kswbsu_self->sd_writeblocks);\
            __xreturn (*__kswbsu_self->sd_writeblocks)(__kswbsu_self,addr,block,c,wc);\
 })
#define ksdev_readblock(self,addr,block) \
 __xblock({ struct ksdev const *const __ksrb_self = (self); kerrno_t __ksrb_err;\
            kassertbyte(__ksrb_self->sd_readblock);\
            if __likely(KE_ISOK(__ksrb_err = kdev_lock_r(__ksrb_self))) {\
             __ksrb_err = (*__ksrb_self->sd_readblock)(__ksrb_self,addr,block);\
             kdev_unlock_r(__ksrb_self);\
            }\
            __xreturn __ksrb_err;\
 })
#define ksdev_writeblock(self,addr,block) \
 __xblock({ struct ksdev *const __kswb_self = (self); kerrno_t __kswb_err;\
            kassertbyte(__kswb_self->sd_writeblock);\
            if __likely(KE_ISOK(__kswb_err = kdev_lock_w(__kswb_self))) {\
             __kswb_err = (*__kswb_self->sd_writeblock)(__kswb_self,addr,block);\
             kdev_unlock_w(__kswb_self);\
            }\
            __xreturn __kswb_err;\
 })
#define ksdev_readblocks(self,addr,block,c,rc) \
 __xblock({ struct ksdev const *const __ksrbs_self = (self); kerrno_t __ksrbs_err;\
            kassertbyte(__ksrbs_self->sd_readblocks);\
            if __likely(KE_ISOK(__ksrbs_err = kdev_lock_r(__ksrbs_self))) {\
             __ksrbs_err = (*__ksrbs_self->sd_readblocks)(__ksrbs_self,addr,block,c,rc);\
             kdev_unlock_r(__ksrbs_self);\
            }\
            __xreturn __ksrbs_err;\
 })
#define ksdev_writeblocks(self,addr,block,c,wc) \
 __xblock({ struct ksdev *const __kswbs_self = (self); kerrno_t __kswbs_err;\
            kassertbyte(__kswbs_self->sd_writeblocks);\
            if __likely(KE_ISOK(__kswbs_err = kdev_lock_w(__kswbs_self))) {\
             __kswbs_err = (*__kswbs_self->sd_writeblocks)(__kswbs_self,addr,block,c,wc);\
             kdev_unlock_w(__kswbs_self);\
            }\
            __xreturn __kswbs_err;\
 })
#define ksdev_readallblocks(self,addr,block,c) \
 __xblock({ struct ksdev const *const __ksrabs_self = (self); kerrno_t __ksrabs_err;\
            if __likely(KE_ISOK(__ksrabs_err = kdev_lock_r(__ksrabs_self))) {\
             __ksrabs_err = ksdev_readallblocks_unlocked(__ksrabs_self,addr,block,c);\
             kdev_unlock_r(__ksrabs_self);\
            }\
            __xreturn __ksrabs_err;\
 })
#define ksdev_writeallblocks(self,addr,block,c) \
 __xblock({ struct ksdev *const __kswabs_self = (self); kerrno_t __kswabs_err;\
            if __likely(KE_ISOK(__kswabs_err = kdev_lock_w(__kswabs_self))) {\
             __kswabs_err = ksdev_writeallblocks_unlocked(__kswabs_self,addr,block,c);\
             kdev_unlock_w(__kswabs_self);\
            }\
            __xreturn __kswabs_err;\
 })
#endif

extern __wunused kerrno_t ksdev_readallblocks_unlocked(struct ksdev const *self, kslba_t addr, void *block, __size_t c);
extern __wunused kerrno_t ksdev_writeallblocks_unlocked(struct ksdev *self, kslba_t addr, void const *block, __size_t c);

extern __wunused kerrno_t ksdev_generic_readblock(struct ksdev const *self, kslba_t addr, void *block);
extern __wunused kerrno_t ksdev_generic_writeblock(struct ksdev *self, kslba_t addr, void const *block);
extern __wunused kerrno_t ksdev_generic_readblocks(struct ksdev const *self, kslba_t addr, void *block, __size_t c, __size_t *rc);
extern __wunused kerrno_t ksdev_generic_writeblocks(struct ksdev *self, kslba_t addr, void const *block, __size_t c, __size_t *wc);


typedef __u16 katabus_t;
typedef __u8  katadrive_t;


//////////////////////////////////////////////////////////////////////////
// Disk creation (These disks must be destroyed with 'ksdev_delete')
// @return: KE_NOMEM:  Not enough memory of the storage device controller
// @return: KE_OK:     A valid device was detected
// @return: KE_NOSYS:  The given device isn't available
// @return: KE_DEVICE: The given device is errorous
extern __wunused __nonnull((1)) kerrno_t ksdev_new_ata(struct ksdev **result, katabus_t bus, katadrive_t drive);
extern __wunused __nonnull((1)) kerrno_t ksdev_new_findfirstata(struct ksdev **result);

//////////////////////////////////////////////////////////////////////////
// Allocate a new ramdisk with a given block size & count
// NOTE: It is recommended to set 'blocksize' to 512, or at least any other 2**n number
// @return: KE_NOMEM: Not enough memory for the storage device controller/actual ram
extern __wunused __nonnull((1)) kerrno_t ksdev_new_ramdisk(struct ksdev **result, __size_t blocksize, __size_t blockcount);

//////////////////////////////////////////////////////////////////////////
// Create a new partition-device for a given device.
// NOTE: The caller is responsible for ensuring the given
//       start+size are in bounds of the parent device.
// @return: KE_NOMEM: Not enough memory for the storage device controller
extern __wunused __nonnull((1,2)) kerrno_t ksdev_new_part(struct ksdev **result, struct ksdev *dev,
                                                          kslba_t start, kslba_t size);

//////////////////////////////////////////////////////////////////////////
// Disk formatting attribute callbacks usable by 512b disks.
extern kerrno_t ksdev_generic_getdiskinfo512(struct ksdev const *self, struct ksdev_diskinfo *buf, __size_t bufsize, __size_t *__restrict reqsize);
extern kerrno_t ksdev_generic_setdiskinfo512(struct ksdev *self, struct ksdev_diskinfo const *buf, __size_t bufsize);
extern kerrno_t ksdev_generic_getdiskname512(struct ksdev const *self, char *__restrict buf, __size_t bufsize, __size_t *__restrict reqsize);
extern kerrno_t ksdev_generic_setdiskname512(struct ksdev *self, char const *buf, __size_t bufsize);
#define KSDEV_GENERIC_MAXDISKNAMESIZE512 10

__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_DEV_STORAGE_H__ */

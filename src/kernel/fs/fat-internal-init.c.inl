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
#ifndef __KOS_KERNEL_FS_FAT_INTERNAL_INIT_C_INL__
#define __KOS_KERNEL_FS_FAT_INTERNAL_INIT_C_INL__ 1

#include <alloca.h>
#include <byteswap.h>
#include <ctype.h>
#include <kos/compiler.h>
#include <kos/errno.h>
#include <kos/kernel/debug.h>
#include <kos/kernel/dev/storage.h>
#include <kos/kernel/fs/fat-internal.h>
#include <kos/kernel/rwlock.h>
#include <malloc.h>
#include <math.h>
#include <string.h>
#include <sys/types.h>

__DECL_BEGIN

static void trimspecstring(char *__restrict buf, size_t size) {
 while (size && isspace(*buf)) { memmove(buf,buf+1,--size); buf[size] = '\0'; }
 while (size && isspace(buf[size-1])) buf[--size] = '\0';
}

kerrno_t
kfatfs_init(struct kfatfs *__restrict self,
            struct ksdev *__restrict dev) {
 union kfat_header *header; kerrno_t error;
 byte_t *boot_signature; kassertobj(self); kassertobj(dev);
 if __unlikely(dev->sd_blocksize < (sizeof(union kfat_header)+2)) return KE_RANGE;
 header = (union kfat_header *)alloca(dev->sd_blocksize);
 error = ksdev_readblock(dev,0,header);
 if __unlikely(KE_ISERR(error)) return error;
 // Check for a valid boot signature
 boot_signature = (byte_t *)header+(dev->sd_blocksize-2);
 if __unlikely(boot_signature[0] != 0x55 || boot_signature[1] != 0xAA) return KE_INVAL;
 // Check some other clear identifiers for an invalid FAT header
 self->f_secsize = leswap_16(header->com.bpb_bytes_per_sector);
 if __unlikely(self->f_secsize != 512 && self->f_secsize != 1024 &&
               self->f_secsize != 2048 && self->f_secsize != 4096) return KE_INVAL;
 if __unlikely(!header->com.bpb_sectors_per_cluster) return KE_INVAL;
 if __unlikely(!header->com.bpb_reserved_sectors) return KE_INVAL; // What's the first sector, then?
 if __unlikely((self->f_secsize % sizeof(struct kfatfileheader)) != 0) return KE_INVAL;
 // Extract some common information
 self->f_firstfatsec = (kfatsec_t)leswap_16(header->com.bpb_reserved_sectors);
 self->f_sec4clus = (size_t)header->com.bpb_sectors_per_cluster;
 self->f_fatcount = (__u32)header->com.bpb_fatc;

 if (!header->com.bpb_sectors_per_fat ||
     !header->com.bpb_maxrootsize) {
  self->f_type = KFSTYPE_FAT32;
  self->f_firstdatasec = self->f_firstfatsec+
   (leswap_32(header->x32.x32_sectors_per_fat)*header->com.bpb_fatc);
 } else {
  kfstype_t fstype; __u32 root_sectors,data_sectors,total_clusters;
  root_sectors = ceildiv(leswap_16(header->com.bpb_maxrootsize)*sizeof(struct kfatfileheader),self->f_secsize);
  self->f_firstdatasec = (leswap_16(header->com.bpb_reserved_sectors)+(
   header->com.bpb_fatc*leswap_16(header->com.bpb_sectors_per_fat))+root_sectors);
  // Calculate the total number of data sectors.
  data_sectors = kfatcom_totalsectors(&header->com)-
   (leswap_16(header->com.bpb_reserved_sectors)+
   (header->com.bpb_fatc*leswap_16(header->com.bpb_sectors_per_fat))+root_sectors);
  // Calculate the total number of data clusters.
  total_clusters = data_sectors/header->com.bpb_sectors_per_cluster;
       if (total_clusters > KFAT16_MAXCLUSTERS) fstype = KFSTYPE_FAT32;
  else if (total_clusters > KFAT12_MAXCLUSTERS) fstype = KFSTYPE_FAT16;
  else fstype = KFSTYPE_FAT12;
  self->f_type = fstype;
 }

 switch (self->f_type) {
  {
   if (0) { case KFSTYPE_FAT12: self->f_readfat   =  &kfatfs_readfat_12;
                                self->f_writefat  = &kfatfs_writefat_12;
                                self->f_getfatsec = &kfatfs_getfatsec_12;
                                self->f_clseof_marker = 0xfff;
   }
   if (0) { case KFSTYPE_FAT16: self->f_readfat   =  &kfatfs_readfat_16;
                                self->f_writefat  = &kfatfs_writefat_16;
                                self->f_getfatsec = &kfatfs_getfatsec_16;
                                self->f_clseof_marker = 0xffff;
   }
   // Check the FAT12/16 signature
   if (header->x16.x16_signature != 0x28 &&
       header->x16.x16_signature != 0x29) return KE_INVAL;
   memcpy(self->f_label,header->x16.x16_label,sizeof(header->x16.x16_label));
   memcpy(self->f_sysname,header->x16.x16_sysname,sizeof(header->x16.x16_sysname));
   self->f_rootsec = leswap_16(header->com.bpb_reserved_sectors)+
                    (header->com.bpb_fatc*leswap_16(header->com.bpb_sectors_per_fat));
   self->f_sec4fat = leswap_16(header->com.bpb_sectors_per_fat);
   self->f_rootmax = (__u32)leswap_16(header->com.bpb_maxrootsize);
   self->f_clseof  = (self->f_sec4fat*self->f_secsize)/2;
   break;
  }
  {
  case KFSTYPE_FAT32:
   // Check the FAT32 signature
   if (header->x32.x32_signature != 0x28 &&
       header->x32.x32_signature != 0x29) return KE_INVAL;
   memcpy(self->f_label,header->x32.x32_label,sizeof(header->x32.x32_label));
   memcpy(self->f_sysname,header->x32.x32_sysname,sizeof(header->x32.x32_sysname));
   self->f_readfat       = &kfatfs_readfat_32;
   self->f_writefat      = &kfatfs_writefat_32;
   self->f_getfatsec     = &kfatfs_getfatsec_32;
   self->f_sec4fat       = leswap_32(header->x32.x32_sectors_per_fat);
#ifdef __DEBUG__
   self->f_rootmax       = (__u32)-1;
#endif
   self->f_clseof        = (self->f_sec4fat*self->f_secsize)/4;
   self->f_clseof_marker = 0x0FFFFFFF;
   // Must lookup the cluster of the root directory
   self->f_rootcls       = leswap_32(header->x32.x32_root_cluster);
   break;
  }
  default: __compiler_unreachable();
 }
 if __unlikely(self->f_clseof_marker < self->f_clseof)
  self->f_clseof = self->f_clseof_marker;
 memcpy(&self->f_oem,header->com.bpb_oem,8*sizeof(char));
 self->f_fatsize = self->f_sec4fat*self->f_secsize;
 self->f_oem[8] = '\0',trimspecstring(self->f_oem,8);
 self->f_label[11] = '\0',trimspecstring(self->f_label,11);
 self->f_sysname[8] = '\0',trimspecstring(self->f_sysname,8);

 if __unlikely(KE_ISERR(error = ksdev_incref(dev))) return error;
 self->f_dev = dev; // Inherit reference

 // Allocate memory for the actual FAT
 self->f_fatv = malloc(self->f_fatsize);
 if __unlikely(!self->f_fatv) {
nomem_dev:
  ksdev_decref(dev);
  return KE_NOMEM;
 }
#ifdef __DEBUG__
 memset(self->f_fatv,0xAA,self->f_fatsize);
#endif

 // Allocate the cache for loaded FAT entries.
 // >> The FAT can get pretty big, so it's faster to keep
 //    most of it unloaded until we actually need it.
 // PLUS: Unless the volume is completely full,
 //       we will only need a small portion of it.
 self->f_fatmeta = (byte_t *)calloc(1,ceildiv(self->f_fatsize,self->f_sec4fat*4));
 if __unlikely(!self->f_fatmeta) { free(self->f_fatv); goto nomem_dev; }

 krwlock_init(&self->f_fatlock);
 self->f_flags = KFATFS_FLAG_NONE;

 /* Do some sanity assertions. */
 kassertmem(self->f_fatmeta,ceildiv(self->f_fatsize,self->f_sec4fat*4));
 return KE_OK;
}

void kfatfs_quit(struct kfatfs *__restrict self) {
 kassertobj(self);
 free(self->f_fatmeta);
 free(self->f_fatv);
 ksdev_decref(self->f_dev);
}

__DECL_END

#endif /* !__KOS_KERNEL_FS_FAT_INTERNAL_INIT_C_INL__ */

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
#ifndef __KOS_KERNEL_DEV_STORAGE_DISKINFO512_C_INL__
#define __KOS_KERNEL_DEV_STORAGE_DISKINFO512_C_INL__ 1

#include <kos/compiler.h>
#include <kos/kernel/debug.h>
#include <kos/kernel/dev/fstype.h>
#include <kos/kernel/dev/storage.h>
#include <kos/kernel/fs/hdd-mbr.h>
#include <kos/syslog.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

__DECL_BEGIN

__local __nonnull((1,2,3,5)) kerrno_t
ksdev_readmbr(struct ksdev const *__restrict self,
              struct khddmbr *__restrict mbr,
              struct ksdev_partinfo *__restrict buf,
              size_t bufelem, size_t *__restrict reqelem,
              kslba_t partstart, kslba_t partsize);

static __nonnull((1,2,3,5)) kerrno_t
ksdev_readpart(struct ksdev const *__restrict self,
               union khddpart const *__restrict part,
               struct ksdev_partinfo *__restrict buf,
               size_t bufelem, size_t *__restrict reqelem,
               kslba_t partstart, kslba_t partsize) {
 kerrno_t error;
 kslba_t localstart = khddpart_lbastart(part);
 kslba_t localsize = khddpart_lbasize(part);
 // Check for an empty partition
 // NOTE: If 'localstart' is 0, this too is a valid indicator
 //       for an empty partition, as the first sector is always
 //       used by the MBR itself (meaning no partition may use it)
 if (!localstart || !localsize) return KE_OK;
 // Make sure the given partition isn't outside given given are
 // NOTE: Single this technology only uses by to 48 bytes,
 //       we don't even have to check if the 64-bit lba index
 //       overflows in the addition performed by the check.
 // NOTE: No need to check for underflow, as sub-partitions
 //       use their parent partition's lba as base.
 if (localstart+localsize > partsize) {
  k_syslogf(KLOG_ERROR,"MBR partition exceeds maximum LBA (%I64u > %I64u)\n",
            localstart+localsize,partsize);
  return KE_INVAL;
 }
 // Adjust for absolute LBA indices
 localstart += partstart;
 // Special handling for extended partition entries.
 if (KFSTYPE_ISEXTPART(part->pt.pt_sysid)) {
  struct khddmbr extmbr; // parse an extended partition entry
  if __unlikely(KE_ISERR(error = ksdev_readblock_unlocked(self,localstart,&extmbr))) return error;
  if (extmbr.mbr_sig[0] != 0x55 || extmbr.mbr_sig[1] != 0xAA) {
   k_syslogf(KLOG_WARN,"MBR extended partition at %I64u has invalid signature '%.2I8x%.2I8x'\n",
             localstart,extmbr.mbr_sig[0],extmbr.mbr_sig[1]);
   return KE_INVAL;
  }
  return ksdev_readmbr(self,&extmbr,buf,bufelem,reqelem,localstart,localsize);
 }
 k_syslogf(KLOG_MSG,"Found partition at %I64u(%I64x) += %I64u(%I64x)\n",
           localstart,localstart,localsize,localsize);
 // Regular partition
 *reqelem = 1;
 if (bufelem) {
  buf->partstart = localstart;
  buf->partsize = localsize;
  buf->fstype = part->pt.pt_sysid;
  buf->fsflags = KSDEV_PARTINFO_FLAG_NONE;
  // Without a parent, this is a primary partition.
  if (!partstart) buf->fsflags |= KSDEV_PARTINFO_FLAG_PRIMARY;
 }
 return KE_OK;
}

__local kerrno_t
ksdev_readmbr(struct ksdev const *__restrict self,
              struct khddmbr *__restrict mbr,
              struct ksdev_partinfo *__restrict buf,
              size_t bufelem, size_t *__restrict reqelem,
              kslba_t partstart, kslba_t partsize) {
 kerrno_t error; size_t usedelem,elemc = 0;
 if __unlikely(KE_ISERR(error = ksdev_readpart(self,&mbr->mbr_part[0],buf,bufelem,&usedelem,partstart,partsize))) return error;
 elemc += usedelem; buf += usedelem; if (usedelem > bufelem) bufelem -= usedelem; else bufelem = 0;
 if __unlikely(KE_ISERR(error = ksdev_readpart(self,&mbr->mbr_part[1],buf,bufelem,&usedelem,partstart,partsize))) return error;
 elemc += usedelem; buf += usedelem; if (usedelem > bufelem) bufelem -= usedelem; else bufelem = 0;
 if __unlikely(KE_ISERR(error = ksdev_readpart(self,&mbr->mbr_part[2],buf,bufelem,&usedelem,partstart,partsize))) return error;
 elemc += usedelem; buf += usedelem; if (usedelem > bufelem) bufelem -= usedelem; else bufelem = 0;
 if __unlikely(KE_ISERR(error = ksdev_readpart(self,&mbr->mbr_part[3],buf,bufelem,&usedelem,partstart,partsize))) return error;
 elemc += usedelem; buf += usedelem; if (usedelem > bufelem) bufelem -= usedelem; else bufelem = 0;
 *reqelem = elemc;
 return KE_OK;
}

kerrno_t
ksdev_generic_getdiskinfo512(struct ksdev const *__restrict self,
                             struct ksdev_diskinfo *__restrict buf,
                             size_t bufsize, size_t *__restrict reqsize) {
 struct khddmbr mbr; kerrno_t error;
 size_t bufelem,reqelem;
 kassertobj(self);
 kassertmem(buf,bufsize);
 assert(self->sd_blocksize == 512);
 if __unlikely(KE_ISERR(error = ksdev_readblock_unlocked(self,0,&mbr))) return error;
 if (mbr.mbr_sig[0] != 0x55 || mbr.mbr_sig[1] != 0xAA) return KE_INVAL;
 strncpy(buf->name,mbr.mbr_diskuid,bufsize < KSDEV_GENERIC_MAXDISKNAMESIZE512
         ? bufsize : KSDEV_GENERIC_MAXDISKNAMESIZE512);
 if (bufsize > offsetof(struct ksdev_diskinfo,partitions))
  bufelem = (bufsize-offsetof(struct ksdev_diskinfo,partitions))/sizeof(struct ksdev_partinfo);
 else bufelem = 0;
 error = ksdev_readmbr(self,&mbr,buf->partitions,bufelem,
                       &reqelem,0,self->sd_blockcount);
 if (reqsize) {
  if (reqelem) {
   *reqsize = offsetof(struct ksdev_diskinfo,partitions)
             +reqelem*sizeof(struct ksdev_partinfo);
  } else *reqsize = strnlen(buf->name,bufsize < KSDEV_GENERIC_MAXDISKNAMESIZE512
                            ? bufsize : KSDEV_GENERIC_MAXDISKNAMESIZE512);
 }
 return KE_OK;
}

kerrno_t
ksdev_generic_getdiskname512(struct ksdev const *__restrict self,
                             char *__restrict buf, size_t bufsize,
                             size_t *__restrict reqsize) {
 struct khddmbr mbr; kerrno_t error; size_t used_size;
 kassertobj(self);
 kassertmem(buf,bufsize);
 assert(self->sd_blocksize == 512);
 if __unlikely(KE_ISERR(error = ksdev_readblock_unlocked(self,0,&mbr))) return error;
 if (mbr.mbr_sig[0] != 0x55 || mbr.mbr_sig[1] != 0xAA) return KE_INVAL;
 used_size = bufsize < KSDEV_GENERIC_MAXDISKNAMESIZE512 ? bufsize : KSDEV_GENERIC_MAXDISKNAMESIZE512;
 memcpy(buf,mbr.mbr_diskuid,used_size);
 if (used_size < bufsize) buf[used_size] = '\0';
 if (reqsize) *reqsize = used_size+1;
 return KE_OK;
}
kerrno_t
ksdev_generic_setdiskname512(struct ksdev *__restrict self,
                             char const *__restrict buf,
                             size_t bufsize) {
 struct khddmbr mbr; kerrno_t error;
 kassertobj(self);
 kassertmem(buf,bufsize);
 assert(self->sd_blocksize == 512);
 if (bufsize > KSDEV_GENERIC_MAXDISKNAMESIZE512) return KE_RANGE;
 if __unlikely(KE_ISERR(error = ksdev_readblock_unlocked(self,0,&mbr))) return error;
 if (mbr.mbr_sig[0] != 0x55 || mbr.mbr_sig[1] != 0xAA) return KE_INVAL;
 memcpy(mbr.mbr_diskuid,buf,bufsize);
 memset(mbr.mbr_diskuid+bufsize,0,KSDEV_GENERIC_MAXDISKNAMESIZE512-bufsize);
 return KE_OK;
}


__asm__("noop_bootloader:\n"
        "\t cli\n"
        "1: hlt\n"
        "\t jmp 1b\n"
        "noop_bootloader_end:\n");

extern byte_t noop_bootloader[];
extern byte_t noop_bootloader_end[];

__local __nonnull((1,2,3)) kerrno_t
khddpart_frompartinfo(union khddpart *__restrict self,
                      struct ksdev_partinfo const *__restrict part,
                      struct ksdev const *__restrict disk) {
 kslba_t partend;
 kassertobj(self);
 kassertobj(part);
 kassertobj(disk);
 partend = part->partstart+part->partsize;
 if (partend < part->partstart || partend > disk->sd_blockcount) return KE_RANGE;
 self->pt.pt_bootable = KHDD_BOOTABLEFLAG_NONE;
 self->pt.pt_sysid = part->fstype;
 if ((part->partstart & UINT64_C(0x0000ffff00000000)) != 0
  || (part->partsize  & UINT64_C(0x0000ffff00000000)) != 0) {
  // Partition requires 48-bit LBA extension
  self->pt_48.pt_bootable |= KHDD_BOOTABLEFLAG_LBA48;
  self->pt_48.pt_sig1 = KHDDPART48_SIG1;
  self->pt_48.pt_lbastarthi = (__u16)(part->partstart >> 32);
  self->pt_48.pt_sig2 = KHDDPART48_SIG2;
  self->pt_48.pt_lbasizehi = (__u16)(part->partsize >> 32);
  self->pt_48.pt_lbastart = (__u32)part->partstart;
  self->pt_48.pt_lbasize = (__u32)part->partsize;
 } else {
  self->pt_32.pt_headstart = 0;
  self->pt_32.pt_sectstart = 0;
  self->pt_32.pt_cylistart = 0;
  self->pt_32.pt_headend = 0;
  self->pt_32.pt_sectend = 0;
  self->pt_32.pt_cyliend = 0;
  self->pt_32.pt_lbastart = (__u32)part->partstart;
  self->pt_32.pt_lbasize = (__u32)part->partsize;
 }
 return KE_OK;
}

kerrno_t
ksdev_generic_setdiskinfo512(struct ksdev *__restrict self,
                             struct ksdev_diskinfo const *__restrict buf,
                             size_t bufsize) {
 struct ksdev_partinfo const *partv; size_t partc;
 struct khddmbr mbr; size_t namelen; kerrno_t error;
 kassertobj(self);
 kassertmem((void *)buf,bufsize);
 partv = buf->partitions;
 partc = (bufsize-offsetof(struct ksdev_diskinfo,partitions))/sizeof(struct ksdev_partinfo);
 assertf(partc <= 4,"More aren't implemented yet...");
 memset(mbr.mbr_bootstrap,0x90,sizeof(mbr.mbr_bootstrap)); // Fill with no-ops
 memset((void *)((uintptr_t)&mbr+sizeof(mbr.mbr_bootstrap)),0,
        sizeof(mbr)-sizeof(mbr.mbr_bootstrap)); // Fill the rest with ZEROs
 memcpy(mbr.mbr_bootstrap,noop_bootloader,(size_t)(noop_bootloader_end-noop_bootloader));
 namelen = strnlen(buf->name,bufsize < KSDEV_DISKINFO_MAXNAMESIZE
                   ? bufsize : KSDEV_DISKINFO_MAXNAMESIZE);
 if __unlikely(namelen > KSDEV_GENERIC_MAXDISKNAMESIZE512) return KE_RANGE;
 memcpy(mbr.mbr_diskuid,buf->name,namelen);
 memset(mbr.mbr_diskuid+namelen,0,KSDEV_GENERIC_MAXDISKNAMESIZE512-namelen/sizeof(char));
 if (partc >= 1) { if __unlikely(KE_ISERR(error = khddpart_frompartinfo(&mbr.mbr_part[0],&partv[0],self))) return error; } else memset(&mbr.mbr_part[0],0,sizeof(union khddpart));
 if (partc >= 2) { if __unlikely(KE_ISERR(error = khddpart_frompartinfo(&mbr.mbr_part[1],&partv[1],self))) return error; } else memset(&mbr.mbr_part[1],0,sizeof(union khddpart));
 if (partc >= 3) { if __unlikely(KE_ISERR(error = khddpart_frompartinfo(&mbr.mbr_part[2],&partv[2],self))) return error; } else memset(&mbr.mbr_part[2],0,sizeof(union khddpart));
 if (partc >= 4) { if __unlikely(KE_ISERR(error = khddpart_frompartinfo(&mbr.mbr_part[3],&partv[3],self))) return error; } else memset(&mbr.mbr_part[3],0,sizeof(union khddpart));
 mbr.mbr_sig[0] = 0x55;
 mbr.mbr_sig[1] = 0xAA;
 return ksdev_writeblock_unlocked(self,0,&mbr);
}

__DECL_END

#endif /* !__KOS_KERNEL_DEV_STORAGE_DISKINFO512_C_INL__ */

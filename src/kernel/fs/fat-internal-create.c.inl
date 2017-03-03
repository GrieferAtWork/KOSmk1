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
#ifndef __KOS_KERNEL_FS_FAT_INTERNAL_CREATE_C_INL__
#define __KOS_KERNEL_FS_FAT_INTERNAL_CREATE_C_INL__ 1

#include <kos/compiler.h>
#include <kos/kernel/dev/storage.h>
#include <kos/kernel/fs/fat-internal.h>
#include <kos/errno.h>

__DECL_BEGIN

#if 0
extern byte_t noop_bootloader[];
extern byte_t noop_bootloader_end[];
__asm__("noop_bootloader:\n"
        "\t cli\n"
        "1: hlt\n"
        "\t jmp 1b\n"
        "noop_bootloader_end:\n");

char dummy_boot_code[] =
  "\x0e"			/* push cs */
  "\x1f"			/* pop ds */
  "\xbe\x5b\x7c"		/* mov si, offset message_txt */
    /* write_msg: */
  "\xac"			/* lodsb */
  "\x22\xc0"			/* and al, al */
  "\x74\x0b"			/* jz key_press */
  "\x56"			/* push si */
  "\xb4\x0e"			/* mov ah, 0eh */
  "\xbb\x07\x00"		/* mov bx, 0007h */
  "\xcd\x10"			/* int 10h */
  "\x5e"			/* pop si */
  "\xeb\xf0"			/* jmp write_msg */
    /* key_press: */
  "\x32\xe4"			/* xor ah, ah */
  "\xcd\x16"			/* int 16h */
  "\xcd\x19"			/* int 19h */
  "\xeb\xfe"			/* foo: jmp foo */
    /* message_txt: */

  "This is not a bootable disk.  Please insert a bootable floppy and\r\n"
  "press any key to try again ... \r\n";

__crit kerrno_t
kfatfs_create(struct kfatfs *__restrict self,
              struct ksdev *__restrict dev) {
 struct kfatext16 sec0;
 __u32 sector_count,clust16,fatdata;
 kerrno_t error; char *iter,*end;
 kassertobj(self);
 kassertobj(dev);
 assert(dev->sd_blocksize == 512);
 memset(sec0.ebr_bootcode,0x90,sizeof(sec0.ebr_bootcode)); // Fill with no-ops
 memcpy(sec0.ebr_bootcode,noop_bootloader,
       (size_t)(noop_bootloader_end-noop_bootloader));
 sec0.ebr_epb.bpb_jmp[0] = 0xEB;
 sec0.ebr_epb.bpb_jmp[1] = 0x3C;
 sec0.ebr_epb.bpb_jmp[2] = 0x90;
 memcpy(sec0.ebr_epb.bpb_oem,"MSWIN4.1",8);
 sec0.ebr_epb.bpb_bytes_per_sector    = 512;
 sec0.ebr_epb.bpb_sectors_per_cluster = 2; // TODO?
 sec0.ebr_epb.bpb_reserved_sectors    = 2; // TODO?
 sec0.ebr_epb.bpb_fatc                = 1; // TODO?
 sec0.ebr_epb.bpb_dirsize             = 2; // TODO?
 sector_count = (dev->sd_blockcount/dev->sd_blocksize)*sec0.ebr_epb.bpb_bytes_per_sector;
 if (sector_count <= (__u16)-1) {
  sec0.ebr_epb.bpb_shortsectorc = (__u16)sector_count;
  sec0.ebr_epb.bpb_longsectorc = 0;
 } else {
  sec0.ebr_epb.bpb_shortsectorc = 0;
  sec0.ebr_epb.bpb_longsectorc = sector_count;
 }
 sec0.ebr_epb.bpb_mediatype = 0xf8; // TODO?
 fatdata = sector_count-cdiv(root_dir_entries*32,sec0.ebr_epb.bpb_bytes_per_sector)-
           sec0.ebr_epb.bpb_reserved_sectors;
 clust16 = ((fatdata*sec0.ebr_epb.bpb_bytes_per_sector)
            +(sec0.ebr_epb.bpb_fatc*4))/
           (((int)sec0.ebr_epb.bpb_sectors_per_cluster*sec0.ebr_epb.bpb_bytes_per_sector)
            +(sec0.ebr_epb.bpb_fatc*2));
 sec0.ebr_epb.bpb_sectors_per_fat = cdiv((clust16+2) * 2,sec0.ebr_epb.bpb_bytes_per_sector);
 sec0.ebr_epb.bpb_sectors_per_track;   /*< Number of sectors per track. */
 sec0.ebr_epb.bpb_numheads;            /*< Number of heads or sides on the storage media. */
 sec0.ebr_epb.bpb_hiddensectors;       /*< Number of hidden sectors. (i.e. the LBA of the beginning of the partition.) */

 sec0.ebr_driveno    = 42;
 sec0.ebr_ntflags    = 0;
 sec0.ebr_signature  = 0x29;
 sec0.ebr_volid      = 0xDEADBEEF;
 sec0.ebr_bootsig[0] = 0x55;
 sec0.ebr_bootsig[1] = 0xAA;
 memset(sec0.ebr_label,' ',sizeof(sec0.ebr_label));
 memcpy(sec0.ebr_label,"FAT1",4);
 memset(sec0.ebr_sysname,' ',sizeof(sec0.ebr_sysname));
 memcpy(sec0.ebr_sysname,"KOS",3);
 self->f_dev = dev;
 return KE_OK;
}
#else
__crit kerrno_t
kfatfs_create(struct kfatfs *__restrict self,
              struct ksdev *__restrict dev) {
 KTASK_CRIT_MARK
 kassertobj(self);
 kassert_kdev(dev);


 return KE_NOSYS;
}
#endif

__DECL_END

#endif /* !__KOS_KERNEL_FS_FAT_INTERNAL_CREATE_C_INL__ */

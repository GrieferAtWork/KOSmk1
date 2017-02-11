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
#ifndef __KOS_KERNEL_FS_HDD_MBR_H__
#define __KOS_KERNEL_FS_HDD_MBR_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/compiler.h>
#include <kos/kernel/types.h>

__DECL_BEGIN

__COMPILER_PACK_PUSH(1)

// Flags used by pt_bootable
#define KHDD_BOOTABLEFLAG_NONE   0x00
#define KHDD_BOOTABLEFLAG_ACTICE 0x80
#define KHDD_BOOTABLEFLAG_LBA48  0x01


// http://wiki.osdev.org/Partition_Table
struct __packed khddpartcom { /*< HDD Partition */
 __u8         pt_bootable;       /*< Boot indicator bit flag: 0 = no, 0x80 = bootable (or "active"). */
 __u8         data1[3];
 __u8         pt_sysid;          /*< System ID. */
 __u8         data2[11];
};
struct __packed khddpart32 { /*< HDD Partition */
 __u8         pt_bootable;       /*< Boot indicator bit flag: 0 = no, 0x80 = bootable (or "active"). */
 __u8         pt_headstart;      /*< Starting Head. */
 unsigned int pt_sectstart : 6;  /*< Starting Sector. */
 unsigned int pt_cylistart : 10; /*< Starting Cylinder. */
 __u8         pt_sysid;          /*< System ID. */
 __u8         pt_headend;        /*< Ending Head. */
 unsigned int pt_sectend : 6;    /*< Ending Sector. */
 unsigned int pt_cyliend : 10;   /*< Ending Cylinder. */
 __u32        pt_lbastart;       /*< Relative Sector (to start of partition -- also equals the partition's starting LBA value). */
 __u32        pt_lbasize;        /*< Total Sectors in partition. */
};

// NOTE: This one should really be a standard!
//      (Though some say it isn't, this kernel uses it)
#define KHDDPART48_SIG1 0x14
#define KHDDPART48_SIG2 0xeb
struct __packed khddpart48 { /*< LBA-48 HDD Partition */
 __u8         pt_bootable;       /*< Boot indicator bit flag: 1 = no, 0x81 = bootable (or "active"). */
 __u8         pt_sig1;           /*< Signature #1 (== 0x14). */
 __u16        pt_lbastarthi;     /*< High 2 bytes for pt_lbastart. */
 __u8         pt_sysid;          /*< System ID. */
 __u8         pt_sig2;           /*< Signature #2 (== 0xeb). */
 __u16        pt_lbasizehi;      /*< High 2 bytes for pt_lbasize. */
 __u32        pt_lbastart;       /*< Relative Sector (to start of partition -- also equals the partition's starting LBA value). */
 __u32        pt_lbasize;        /*< Total Sectors in partition. */
};

union khddpart {
 struct khddpartcom pt;
 struct khddpart32  pt_32;
 struct khddpart48  pt_48;
};



// http://wiki.osdev.org/MBR_(x86)
struct __packed khddmbr { /*< Master boot record */
 __u8           mbr_bootstrap[436]; /*< MBR Bootstrap (flat binary executable code). */
 char           mbr_diskuid[10];    /*< Optional "unique" disk ID1. */
 union khddpart mbr_part[4];        /*< Partition table entries. */
 __u8           mbr_sig[2];         /*< "Valid bootsector" signature bytes (== 0x55, 0xAA) */
};


__COMPILER_PACK_POP

#ifndef __INTELLISENSE__
__STATIC_ASSERT(sizeof(union khddpart) == 16);
__STATIC_ASSERT(sizeof(struct khddmbr) == 512);
#endif

#define khddpart_islba48(self) \
 (((self)->pt.pt_bootable&KHDD_BOOTABLEFLAG_LBA48) &&\
   (self)->pt_48.pt_sig1 == KHDDPART48_SIG1 && \
   (self)->pt_48.pt_sig2 == KHDDPART48_SIG2)

__local __wunused __nonnull((1)) __u64 khddpart_lbastart(union khddpart const *self) {
 return khddpart_islba48(self)
  ? ((__u64)self->pt_48.pt_lbastart | (__u64)self->pt_48.pt_lbastarthi << 32)
  : ((__u64)self->pt_32.pt_lbastart);
}
__local __wunused __nonnull((1)) __u64 khddpart_lbasize(union khddpart const *self) {
 return khddpart_islba48(self)
  ? ((__u64)self->pt_48.pt_lbasize | (__u64)self->pt_48.pt_lbasizehi << 32)
  : ((__u64)self->pt_32.pt_lbasize);
}

__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_FS_HDD_MBR_H__ */

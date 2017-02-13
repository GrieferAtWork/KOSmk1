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
#ifndef __KOS_KERNEL_FS_FAT_INTERNAL_H__
#define __KOS_KERNEL_FS_FAT_INTERNAL_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/compiler.h>
#include <kos/errno.h>
#include <kos/kernel/types.h>
#include <kos/kernel/rwlock.h>
#include <kos/kernel/dev/fstype.h>
#include <kos/kernel/dev/storage.h>
#include <kos/kernel/fs/fs.h>
#include <byteswap.h>

__DECL_BEGIN

__COMPILER_PACK_PUSH(1)

#define KFAT12_MAXCLUSTERS 4084
#define KFAT16_MAXCLUSTERS 65524
#define KFAT32_MAXCLUSTERS 4294967284

// Derived from diagrams here: http://wiki.osdev.org/FAT
struct __packed kfatcom { /*< FAT BIOS Parameter Block (common header) */
 __u8   bpb_jmp[3];              /*< Jump instructions (executable). */
 char   bpb_oem[8];              /*< OEM identifier */
 __le16 bpb_bytes_per_sector;    /*< The number of Bytes per sector. */
 __u8   bpb_sectors_per_cluster; /*< Number of sectors per cluster. */
 __le16 bpb_reserved_sectors;    /*< Number of reserved sectors. */
 __u8   bpb_fatc;                /*< Number of File Allocation Tables (FAT's) on the storage media (1..4). */
 __le16 bpb_maxrootsize;         /*< [!FAT32] Max number of entries in the root directory. */
 __le16 bpb_shortsectorc;        /*< The total sectors in the logical volume (If 0, use 'bpb_numheads' instead). */
 __u8   bpb_mediatype;           /*< Indicates the media descriptor type. */
 __le16 bpb_sectors_per_fat;     /*< [!FAT32] Number of sectors per FAT. */
 __le16 bpb_sectors_per_track;   /*< Number of sectors per track. */
 __le16 bpb_numheads;            /*< Number of heads or sides on the storage media. */
 __le32 bpb_hiddensectors;       /*< Absolute sector address of the fat header (lba of the fat partition). */
 __le32 bpb_longsectorc;         /*< Large amount of sector on media (Used for more than '65535' sectors) */
};

struct __packed kfatext16 { /*< FAT12/16 Extended boot record. */
 struct kfatcom x16_epb;           /*< BIOS Parameter Block. */
 __u8           x16_driveno;       /*< Drive number. The value here should be identical to the value returned by BIOS interrupt 0x13,
                                       or passed in the DL register; i.e. 0x00 for a floppy disk and 0x80 for hard disks.
                                       This number is useless because the media is likely to be moved to another
                                       machine and inserted in a drive with a different drive number.  */
 __u8           x16_ntflags;       /*< Windows NT Flags. (Set to 0) */
 __u8           x16_signature;     /*< Signature (Must be 0x28 or 0x29). */
 __le32         x16_volid;         /*< VolumeID 'Serial' number. Used for tracking volumes between computers. */
 char           x16_label[11];     /*< Volume label string. This field is padded with spaces. */
 char           x16_sysname[8];    /*< System identifier string. This field is a string representation of the FAT file system type.
                                       It is padded with spaces. The spec says never to trust the contents of this string for any use. */
/*
 __u8           x16_bootcode[448]; / *< Boot code. * /
 __u8           x16_bootsig[2];    / *< Bootable partition signature (0x55, 0xAA). * /
*/
};
struct __packed kfatext32 { /*< FAT32 Extended boot record. */
 struct kfatcom x32_epb;             /*< BIOS Parameter Block. */
 __le32         x32_sectors_per_fat; /*< Number of sectors per FAT. */
 __le16         x32_flags;           /*< Flags. */
 __le16         x32_version;         /*< FAT version number. The high byte is the major version and the low byte is the minor version. FAT drivers should respect this field. */
 __le32         x32_root_cluster;    /*< The cluster number of the root directory. Often this field is set to 2. */
 __le16         x32_fsinfo_cluster;  /*< The sector number of the FSInfo structure. */
 __le16         x32_backup_cluster;  /*< The sector number of the backup boot sector. */
 __u8           x32_set2zero[12];    /*< Reserved. When the volume is formated these bytes should be zero. */
 __u8           x32_driveno;         /*< Drive number. The value here should be identical to the value returned by BIOS interrupt 0x13,
                                         or passed in the DL register; i.e. 0x00 for a floppy disk and 0x80 for hard disks.
                                         This number is useless because the media is likely to be moved to another
                                         machine and inserted in a drive with a different drive number.  */
 __u8           x32_ntflags;         /*< Windows NT Flags. (Set to 0) */
 __u8           x32_signature;       /*< Signature (Must be 0x28 or 0x29). */
 __le32         x32_volid;           /*< VolumeID 'Serial' number. Used for tracking volumes between computers. */
 char           x32_label[11];       /*< Volume label string. This field is padded with spaces. */
 char           x32_sysname[8];      /*< System identifier string. This field is a string representation of the FAT file system type.
                                         It is padded with spaces. The spec says never to trust the contents of this string for any use. */
/*
 __u8           x32_bootcode[420];   / *< Boot code. * /
 __u8           x32_bootsig[2];      / *< Bootable partition signature (0x55, 0xAA). * /
*/
};

union kfat_header {
 struct kfatcom   com;
 struct kfatext16 x12;
 struct kfatext16 x16;
 struct kfatext32 x32;
};
#define kfatcom_totalsectors(self) \
 ((self)->bpb_shortsectorc\
  ? (__u32)leswap_16((self)->bpb_shortsectorc)\
  : leswap_32((self)->bpb_longsectorc))


// File attribute flags for 'struct kfatfileheader::f_attr'
#define KFATFILE_ATTR_READONLY  0x01
#define KFATFILE_ATTR_HIDDEN    0x02
#define KFATFILE_ATTR_SYSTEM    0x04
#define KFATFILE_ATTR_VOLUMEID  0x08
#define KFATFILE_ATTR_DIRECTORY 0x10
#define KFATFILE_ATTR_ARCHIVE   0x20
#define KFATFILE_ATTR_DEVICE    0x40
#define KFATFILE_ATTR_LONGFILENAME \
 (KFATFILE_ATTR_READONLY|KFATFILE_ATTR_HIDDEN\
 |KFATFILE_ATTR_SYSTEM|KFATFILE_ATTR_VOLUMEID)

struct __packed kfatfile_time {
 union __packed {
struct __packed {
  unsigned int ft_sec : 5;
  unsigned int ft_min : 6;
  unsigned int ft_hour : 5;
 }; __u16 fd_hash; };
};
struct __packed kfatfile_date {
 union __packed {
struct __packed {
  unsigned int fd_day : 5;
  unsigned int fd_month : 4;
  unsigned int fd_year : 7;
 }; __u16 fd_hash; };
};


struct __packed kfatfile_ctime {
 __u8                 fc_sectenth; /*< Creation time in 10ms resolution (0-199). */
 struct kfatfile_time fc_time;     /*< Creation time. */
 struct kfatfile_date fc_date;     /*< Creation date. */
};
struct __packed kfatfile_mtime {
 struct kfatfile_time fc_time;     /*< Modification time. */
 struct kfatfile_date fc_date;     /*< Modification date. */
};

#define KFATFILE_NAMEMAX 8
#define KFATFILE_EXTMAX  3

typedef __u16 kfat_usc2char;

struct kfatfileheader;
struct __packed kfatfileheader {
union __packed {struct __packed {
union __packed {struct __packed {union __packed {
#define KFATFILE_MARKER_DIREND 0x00 /*< End of directory. */
#define KFATFILE_MARKER_IS0XE5 0x05 /*< Character: First character is actually 0xE5. */
#define KFATFILE_MARKER_UNUSED 0xE5 /*< Unused entry (ignore). */
 __u8                  f_marker;    /*< Special directory entry marker. */
 char                  f_name[KFATFILE_NAMEMAX]; /*< Short (8-character) filename. */};
 char                  f_ext[KFATFILE_EXTMAX]; /*< File extension. */};
 char                  f_nameext[KFATFILE_NAMEMAX+KFATFILE_EXTMAX];/*< Name+extension. */};
 __u8                  f_attr;      /*< File attr. */
 // https://en.wikipedia.org/wiki/8.3_filename
 // NOTE: After testing, the flags specified by that link are wrong.
 //       >> The following lowercase flags are correct though!
#define KFATFILE_NFLAG_NONE    0x00
#define KFATFILE_NFLAG_LOWBASE 0x08 /*< Lowercase basename. */
#define KFATFILE_NFLAG_LOWEXT  0x10 /*< Lowercase extension. */
 __u8                  f_ntflags;   /*< NT Flags. */
 struct kfatfile_ctime f_ctime;     /*< Creation time. */
 struct kfatfile_date  f_atime;     /*< Last access time (or rather date...). */
 __le16                f_clusterhi; /*< High 2 bytes of the file's cluster. */
 struct kfatfile_mtime f_mtime;     /*< Last modification time. */
 __le16                f_clusterlo; /*< Lower 2 bytes of the file's cluster. */
 __le32                f_size;      /*< File size. */
};struct __packed { /* Long directory entry. */
#define KFAT_LFN_SEQNUM_MIN        0x01
#define KFAT_LFN_SEQNUM_MAX        0x14
#define KFAT_LFN_SEQNUM_MAXCOUNT ((KFAT_LFN_SEQNUM_MAX-KFAT_LFN_SEQNUM_MIN)+1)
 __u8                  lfn_seqnum; /*< Sequence number (KOS uses it as hint for where a name part should go). */
 /* Sizes of the three name portions. */
#define KFAT_LFN_NAME1      5
#define KFAT_LFN_NAME2      6
#define KFAT_LFN_NAME3      2
#define KFAT_LFN_NAME      (KFAT_LFN_NAME1+KFAT_LFN_NAME2+KFAT_LFN_NAME3)
 kfat_usc2char         lfn_name_1[KFAT_LFN_NAME1];
 __u8                  lfn_attr;   /*< Attributes (always 'KFATFILE_ATTR_LONGFILENAME') */
 __u8                  lfn_type;   /*< Long directory entry type (set to ZERO(0)) */
 __u8                  lfn_csum;   /*< Checksum of DOS filename (s.a.: 'kfat_genlfnchecksum'). */
 kfat_usc2char         lfn_name_2[KFAT_LFN_NAME2];
 __le16                lfn_clus;   /*< First cluster (Always 0x0000). */
 kfat_usc2char         lfn_name_3[KFAT_LFN_NAME3];
};};};
struct timespec;
// NOTE: The setters return non-zero if the given header was modified
extern void kfatfileheader_getatime(struct kfatfileheader const *self, struct timespec *result);
extern void kfatfileheader_getctime(struct kfatfileheader const *self, struct timespec *result);
extern void kfatfileheader_getmtime(struct kfatfileheader const *self, struct timespec *result);
extern int kfatfileheader_setatime(struct kfatfileheader *self, struct timespec const *value);
extern int kfatfileheader_setctime(struct kfatfileheader *self, struct timespec const *value);
extern int kfatfileheader_setmtime(struct kfatfileheader *self, struct timespec const *value);

//////////////////////////////////////////////////////////////////////////
// Generate a checksum for a given short filename, for use in long filename entries.
extern __u8 kfat_genlfnchecksum(char const short_name[KFATFILE_NAMEMAX+KFATFILE_EXTMAX]);

#define kfatfileheader_getcluster(self) \
 ((kfatcls_t)leswap_16((self)->f_clusterlo) |\
 ((kfatcls_t)leswap_16((self)->f_clusterhi) << 16))
#define kfatfileheader_setcluster(self,v) \
 ((self)->f_clusterlo = leswap_16((v) & 0xffff),\
 ((self)->f_clusterhi = leswap_16(((v) >> 16) & 0xffff)))
#define kfatfileheader_getsize(self) leswap_32((self)->f_size)

__COMPILER_PACK_POP

#ifndef __INTELLISENSE__
__STATIC_ASSERT(sizeof(struct kfatfileheader) == 32);
#endif

typedef __u32     kfatsec_t; /*< Sector number. */
typedef __u32     kfatcls_t; /*< Cluster/Fat index number. */
typedef kfatsec_t kfatoff_t; /*< Sector offset in the FAT table (Add to 'f_firstfatsec' to get a 'kfatsec_t'). */
typedef kfatsec_t kfatdir_t;

struct kfatfs;

#define KFATFS_CUSTER_UNUSED 0 /*< Cluster number found in the FAT, marking an unused cluster. */
typedef kfatoff_t (*pkfatfs_getfatsector)(struct kfatfs *self, kfatcls_t index);
typedef kfatcls_t (*pkfatfs_readfat)(struct kfatfs *self, kfatcls_t cluster);
typedef void (*pkfatfs_writefat)(struct kfatfs *self, kfatcls_t cluster, kfatcls_t value);

#define kfatfs_iseofcluster(self,cls) \
 ((cls) >= (self)->f_clseof/* || (cls) < 2*/)



struct kfatfs {
 __ref struct ksdev *f_dev;           /*< [1..1][const] Storage device used for this FAT. */
 kfstype_t           f_type;          /*< [const] The type of FAT filesystem (KFSTYPE_FAT12/KFSTYPE_FAT16/KFSTYPE_FAT32). */
 char                f_oem[9];        /*< [const] OEM identifier. */
 char                f_label[12];     /*< [const] Volume label string (zero-terminated). */
 char                f_sysname[9];    /*< [const] System identifier string (zero-terminated). */
union{
 kfatcls_t           f_rootcls;       /*< [const] Cluster of the root directory (FAT32 only). */
 kfatsec_t           f_rootsec;       /*< [const] Sector of the root directory (non-FAT32 only). */
};
 __size_t            f_secsize;       /*< [const] Size of a sector in bytes. */
 kfatsec_t           f_sec4clus;      /*< [const] Amount of sectors for each cluster. */
 kfatsec_t           f_sec4fat;       /*< [const] Amount of sectors for each FAT. */
 kfatsec_t           f_firstdatasec;  /*< [const] First data sector. */
 kfatsec_t           f_firstfatsec;   /*< [const] Sector number of the first FAT. */
 __u32               f_rootmax;       /*< [const] Max amount of entries within the root directory (max_size parameter when enumerating the root directory). */
 __u32               f_fatcount;      /*< [const] Amount of consecutive FAT copies. */
 __size_t            f_fatsize;       /*< [const] == f_sec4fat*f_secsize. */
 kfatcls_t           f_clseof;        /*< [const] Cluster indices greater than or equal to this are considered EOF. */
 kfatcls_t           f_clseof_marker; /*< [const] Marker that should be used to mark EOF entries in the FAT. */
 pkfatfs_getfatsector f_getfatsec;    /*< [const][1..1] Get the sector offset of a given FAT entry (From 'f_firstfatsec' where the FAT table portion is written to). */
 pkfatfs_readfat     f_readfat;       /*< [const][1..1] Read a FAT entry at a given position. */
 pkfatfs_writefat    f_writefat;      /*< [const][1..1] Write a FAT entry at a given position. */
 struct krwlock      f_fatlock;       /*< R/W lock for the CACHE (meaning to modify the cache, by reading from disk, you need to write-lock this) */
#define KFATFS_FLAG_NONE       0x00000000
#define KFATFS_FLAG_FATCHANGED 0x00000001 /*< The FAT contains changes that must be flushed. */
 __u32               f_flags;         /*< [lock(f_fatlock)] Flags describing the current fat state. */
 void               *f_fatv;          /*< [1..f_fatsize][owned][lock(f_fatlock)] FAT table. */
 __byte_t           *f_fatmeta;       /*< [1..ceildiv(f_fatsize,f_sec4fat*4)][owned][lock(f_fatlock)] Bits masking valid fat sectors
                                          (Every first bit describes validity, and every second a changed cache). */
};

//////////////////////////////////////////////////////////////////////////
// Perform operations on the fat's FAT cache
// @return: KE_DESTROYED: The FAT was already destroyed.
extern kerrno_t kfatfs_fat_close(struct kfatfs *self);
extern kerrno_t kfatfs_fat_read(struct kfatfs *self, kfatcls_t index, kfatcls_t *target);
extern kerrno_t kfatfs_fat_write(struct kfatfs *self, kfatcls_t index, kfatcls_t target);
extern kerrno_t kfatfs_fat_flush(struct kfatfs *self);
extern kerrno_t kfatfs_fat_readandalloc(struct kfatfs *self, kfatcls_t index, kfatcls_t *target);
extern kerrno_t kfatfs_fat_allocfirst(struct kfatfs *self, kfatcls_t *target);
extern kerrno_t kfatfs_fat_freeall(struct kfatfs *self, kfatcls_t first); /*< Free all clusters in a chain starting at 'first'. */
extern kerrno_t _kfatfs_fat_doflush_unlocked(struct kfatfs *self);
extern kerrno_t _kfatfs_fat_read_unlocked(struct kfatfs *self, kfatcls_t index, kfatcls_t *target);
extern kerrno_t _kfatfs_fat_load_unlocked(struct kfatfs *self, kfatcls_t index);
extern kerrno_t _kfatfs_fat_getfreecluster_unlocked(struct kfatfs *self, kfatcls_t *result, kfatcls_t hint);
extern kerrno_t _kfatfs_fat_freeall_unlocked(struct kfatfs *self, kfatcls_t first);


__local __wunused __nonnull((1)) kfatsec_t
kfatfs_clusterstart(struct kfatfs const *self, kfatcls_t cluster) {
 return ((cluster-2)*self->f_sec4clus)+self->f_firstdatasec;
}
/*
__local __wunused __nonnull((1)) kfatcls_t
kfatfs_sec2clus(struct kfatfs const *self, kfatsec_t sector) {
 return ((sector-self->f_firstdatasec)/self->f_sec4clus)+2;
}
*/

extern kerrno_t kfatfs_savefileheader(struct kfatfs *self, __u64 headpos,
                                      struct kfatfileheader const *header);

//////////////////////////////////////////////////////////////////////////
// Initialize a fat filesystem by loading it from a given storage device.
// HINT: The storage device can be a partition
// @param: fstype: A FAT filesystem type from '<kos/kernel/dev/fstype.h>'
// @return: KE_DEVICE: The storage device encountered an error.
// @return: KE_RANGE:  Blocks of the device are too small for a FAT filesystem.
// @return: KE_INVAL:  The partition record is broken
extern __wunused __nonnull((1,2)) kerrno_t kfatfs_init(struct kfatfs *self, struct ksdev *dev);
extern __wunused __nonnull((1,2)) kerrno_t kfatfs_create(struct kfatfs *self, struct ksdev *dev);
extern           __nonnull((1)) void kfatfs_quit(struct kfatfs *self);

extern __wunused __nonnull((1,4)) kerrno_t kfatfs_loadsectors(struct kfatfs const *self, kfatsec_t sec, __size_t c, void *__restrict buf);
extern __wunused __nonnull((1,4)) kerrno_t kfatfs_savesectors(struct kfatfs *self, kfatsec_t sec, __size_t c, void const *__restrict buf);

#define kfatfs_getrootdir(self) ((self)->f_rootdir)

struct kfatfilepos {
 __u64 fm_namepos; /*< On-disk, sector-based location (in bytes) of the first name header. */
 __u64 fm_headpos; /*< On-disk, sector-based location (in bytes) of the file's header. */
};

typedef kerrno_t (*__wunused __nonnull((1,2,3)) pkfatfs_enumdir_callback)(
 struct kfatfs *fs, struct kfatfilepos const *fpos, struct kfatfileheader const *file,
 char const *filename, __size_t filename_size, void *closure);

//////////////////////////////////////////////////////////////////////////
// Enumerate all files withing a given directory
//  - This function will automatically handle long filenames,
//    as well as the lower-case extension for 8.3 filenames.
// @return: * : Any value other than KE_OK, returned by 'callback'
// @return: KE_NOMEM: Not enough available memory
extern __wunused __nonnull((1,3)) kerrno_t
kfatfs_enumdir(struct kfatfs *self, kfatcls_t dir,
               pkfatfs_enumdir_callback callback, void *closure);
extern __wunused __nonnull((1,4)) kerrno_t
kfatfs_enumdirsec(struct kfatfs *self, kfatsec_t dir, __u32 maxsize,
                  pkfatfs_enumdir_callback callback, void *closure);

//////////////////////////////////////////////////////////////////////////
// Check for the existance of a given short name within a directory 'dir'
// @return: KE_OK:     The given name does not exist.
// @return: KE_EXISTS: The given short name already exists.
// @return: KE_NOENT:  The given long name already exists (file already exists!).
// @return: *: Some device-specific error occurred.
extern __wunused __nonnull((1,4,6)) kerrno_t
kfatfs_checkshort(struct kfatfs *self, kfatcls_t dir, int dir_is_sector,
                  char const *long_name, size_t long_name_size,
                  char const name[KFATFILE_NAMEMAX+KFATFILE_EXTMAX]);

//////////////////////////////////////////////////////////////////////////
// Mark a file header at a given location as deleted (aka. unused)
extern __wunused __nonnull((1)) kerrno_t
kfatfs_rmheaders(struct kfatfs *self, __u64 headpos, unsigned int count);

//////////////////////////////////////////////////////////////////////////
// Allocate and save a set of consecutive headers.
// @param: filepos->fm_namepos: Address of the first header upon success.
// @param: filepos->fm_headpos: Address of the last header upon success.
// @return: KE_OK: Successfully allocated space for the new headers.
extern __wunused __nonnull((1,4,6)) kerrno_t
kfatfs_allocheaders(struct kfatfs *self, kfatcls_t dir, int dir_is_sector,
                    struct kfatfileheader const *first_header,
                    __size_t header_count, struct kfatfilepos *filepos);

#define kfatfs_mkheader(self,dir,dir_is_sector,header,filepos) \
 kfatfs_allocheaders(self,dir,dir_is_sector,header,1,filepos)

//////////////////////////////////////////////////////////////////////////
// Generates a long filename entry.
// @return: KE_NAMETOOLONG: The given long_name is too long, even for a long-filename-entry.
extern __wunused __nonnull((1,4,5,7)) kerrno_t
kfatfs_mklongheader(struct kfatfs *self, kfatcls_t dir, int dir_is_sector,
                    struct kfatfileheader const *header,
                    char const *long_name, __size_t long_name_size,
                    struct kfatfilepos *filepos);

//////////////////////////////////////////////////////////////////////////
// Generate a short filename for storage within the given directory.
// @return: KE_OVERFLOW: Too many variations of the same DOS 8.3 filename already exist.
// @return: *: Some other, device-specific error occurred.
extern __wunused __nonnull((1,4,5,7)) kerrno_t
kfatfs_mkshortname(struct kfatfs *self, kfatcls_t dir, int dir_is_sector,
                   struct kfatfileheader *header, char const *long_name,
                   __size_t long_name_size, int *need_long_header);

//////////////////////////////////////////////////////////////////////////
// Create a Dos 8.3 short filename.
// The caller is responsible for checking if the generated name already
// exists, and if it does, they should increment 'retry' by one and call try again.
// WARNING: Once 'retry' is equal to 'KDOS83_RETRY_COUNT', the function may
//          no longer be called, and the filename generation has failed from
//          the lack of finding a valid short filename.
// NOTE: Unlike many other functions, 'namesize' is expected to be the
//       exact length of 'name', with any \0 character completely ignored.
//    >> When called from a strn-style function, pass the result of strnlen to 'namesize'
// NOTE: Invalid characters are substituted with '~'
// @return: KDOS83_KIND_SHORT: The given name was short enough to fit a 8.3 name
//                          >> '*ntflags' may have been set to use the lowercase extension flags
// @return: KDOS83_KIND_LONG:  The given name was too long to fit a dos 8.3 filename.
//                          >> Only in this situation, the 'retry' can be used to generate
//                             additional filenames in the case of name collisions.
// @return: KDOS83_KIND_CASE:  The given name was short enough, but its mixed casing
//                             requires the use of a long filename entry.
//                             Note through, that 'retry' cannot be used to generate more names.
extern int kdos83_makeshort(char const *__restrict name, __size_t namesize,
                            int retry, __u8 *ntflags, char result[11]);
#define KDOS83_KIND_SHORT   0
#define KDOS83_KIND_LONG    1
#define KDOS83_KIND_CASE    2
#define KDOS83_RETRY_COUNT (0xffff*9) /*< Amount of possible retries when a short filename already exists. */





extern kfatoff_t kfatfs_getfatsec_12(struct kfatfs *self, kfatcls_t index);
extern kfatoff_t kfatfs_getfatsec_16(struct kfatfs *self, kfatcls_t index);
extern kfatoff_t kfatfs_getfatsec_32(struct kfatfs *self, kfatcls_t index);
extern kfatcls_t kfatfs_readfat_12(struct kfatfs *self, kfatcls_t cluster);
extern kfatcls_t kfatfs_readfat_16(struct kfatfs *self, kfatcls_t cluster);
extern kfatcls_t kfatfs_readfat_32(struct kfatfs *self, kfatcls_t cluster);
extern void kfatfs_writefat_12(struct kfatfs *self, kfatcls_t cluster, kfatcls_t value);
extern void kfatfs_writefat_16(struct kfatfs *self, kfatcls_t cluster, kfatcls_t value);
extern void kfatfs_writefat_32(struct kfatfs *self, kfatcls_t cluster, kfatcls_t value);

__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_FS_FAT_INTERNAL_H__ */

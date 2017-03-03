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
#ifndef __KOS_KERNEL_FS_FS_BLOCKFILE_H__
#define __KOS_KERNEL_FS_FS_BLOCKFILE_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/compiler.h>
#include <kos/kernel/fs/file.h>
#include <kos/errno.h>
#include <kos/kernel/types.h>

__DECL_BEGIN

//////////////////////////////////////////////////////////////////////////
// Builtin helper types: blockfile
// >> A blockfile acts like a regular file,
//    but by the use of an internal buffer,
//    data is only read/written a chunk-at-a-time.
struct kblockfiletype;
struct kblockfile;

struct kfilechunk {
 // Abstract representation of a file chunk through two __u64-s.
 // NOTE: FAT file systems use 'fc_data' as the address of the current sector.
 __fsblksize_t fc_index; /*< Chunk index (0: Start of file) */
 __u64         fc_data;  /*< Block-type-specific index data (usually an LBA address for this chunk). */
};

struct kblockfiletype {
 struct kfiletype bft_file; /*< Underlying, regular filetype. */
 kerrno_t (*bft_loadchunk)(struct kblockfile const *__restrict self, struct kfilechunk const *__restrict chunk, void *__restrict buf);
 kerrno_t (*bft_savechunk)(struct kblockfile *__restrict self, struct kfilechunk const *__restrict chunk, void const *__restrict buf);
 kerrno_t (*bft_nextchunk)(struct kblockfile *__restrict self, struct kfilechunk *__restrict chunk);
 kerrno_t (*bft_findchunk)(struct kblockfile *__restrict self, struct kfilechunk *__restrict chunk);
 // Release all chunks following the one at index 'count', including 'count' itself
 kerrno_t (*bft_releasechunks)(struct kblockfile *__restrict self, __fsblkcnt_t count);
 // Get/Set the actual file size
 kerrno_t (*bft_getsize)(struct kblockfile const *__restrict self, __pos_t *fsize);
 kerrno_t (*bft_setsize)(struct kblockfile *__restrict self, __pos_t fsize);
 kerrno_t (*bft_getchunksize)(struct kblockfile const *__restrict self, __size_t *chsize);
};
struct kblockfile {
 struct kfile      bf_file;       /*< Underlying, regular file. */
 __ref struct kdirent *bf_dirent; /*< [1..1][const] Directory entry associated with this file. */
 __ref struct kinode  *bf_inode;  /*< [1..1][const] INode associated with this file. */
 struct kmutex     bf_lock;       /*< File-level lock for the buffer. */
#define KBLOCKFILE_FLAG_NONE     0x00
#define KBLOCKFILE_FLAG_CHANGED  0x01 /*< [lock(bf_lock)] Set when the given buffer was modified and must be flushed once exchanged. */
#define KBLOCKFILE_FLAG_BUFDATA  0x02 /*< [lock(bf_lock)] The buffer is filled with valid data. */
#define KBLOCKFILE_FLAG_CHUNK    0x04 /*< [lock(bf_lock)] The current chunk is selected ('fc_data' is valid). */
#define KBLOCKFILE_FLAG_WASPREV  0x08 /*< [lock(bf_lock)] Used for optimizations: The last chunks was the chunk previous to the current. */
#define KBLOCKFILE_FLAG_APPEND   0x10 /*< [lock(bf_lock)] Only allow appending to the file. */
#define KBLOCKFILE_FLAG_CANREAD  0x20 /*< [lock(bf_lock)] Read permissions are granted. */
#define KBLOCKFILE_FLAG_CANWRITE 0x40 /*< [lock(bf_lock)] Write permissions are granted. */
 __u8              bf_flags;      /*< Blockfile flags. */
 __u8              padding1[3];   /*< ... */
 __size_t          bf_chunksize;  /*< [const] == bf_file->f_dirent->d_inode->i_sblock->s_chunksize. */
 __byte_t         *bf_buffer;     /*< [lock(bf_lock)][1..bf_chunksize][const][owned] Intermediate file buffer. */
 __byte_t         *bf_bufpos;     /*< [lock(bf_lock)][<bf_buffer>] Current r/w position in the buffer. */
 __byte_t         *bf_bufend;     /*< (usually equals 'bf_buffer+bf_chunksize', but the last chunk may be smaller). */
 __byte_t         *bf_bufmax;     /*< [const] == bf_buffer+bf_chunksize. */
 struct kfilechunk bf_currchunk;  /*< [lock(bf_lock)] The chunk id represented by 'bf_buffer'. */
 __pos_t           bf_filesaved;  /*< [lock(bf_lock)] Saved file size. */
 __pos_t           bf_filesize;   /*< [lock(bf_lock)] Actual file size (When swapping the current chunk, this value is written). */
};                                     
#define kblockfile_type(self) ((struct kblockfiletype *)(self)->bf_file.f_type)
#define kblockfile_chunkcount(self) ((__fsblksize_t)ceildiv((self)->bf_filesize,(self)->bf_chunksize))
#define kblockfile_updatebufend(self) \
 __xblock({ __fsblksize_t chunkcount = kblockfile_chunkcount(self);\
            if ((self)->bf_currchunk.fc_index >= chunkcount) {\
             (self)->bf_bufend = (self)->bf_buffer;\
            } else if ((self)->bf_currchunk.fc_index == chunkcount-1) {\
             (self)->bf_bufend = (self)->bf_buffer+((self)->bf_filesize % (self)->bf_chunksize);\
            } else {\
             (self)->bf_bufend = (self)->bf_bufmax;\
            }\
 })

#define KBLOCKFILETYPE_INITFILE \
{\
 .ft_size      = sizeof(struct kblockfile),\
 .ft_quit      = &kblockfile_quit,\
 .ft_open      = &kblockfile_open,\
 .ft_read      = &kblockfile_read,\
 .ft_write     = &kblockfile_write,\
 .ft_seek      = &kblockfile_seek,\
 .ft_trunc     = &kblockfile_trunc,\
 .ft_ioctl     = &kblockfile_ioctl,\
 .ft_flush     = &kblockfile_flush,\
 .ft_getattr   = &kblockfile_getattr,\
 .ft_setattr   = &kblockfile_setattr,\
 .ft_getinode  = &kblockfile_inode,\
 .ft_getdirent = &kblockfile_dirent,\
}

// Supported attributes (in addition to generically supported):
//  - [RO] KATTR_FS_BLOCKCNT
//  - [RO] KATTR_FS_BLOCKSIZE
//  - [RO] KATTR_FS_SIZE (Loads the buffer's size, instead of the file's)


//////////////////////////////////////////////////////////////////////////
// Set of filetype functions that should be set/called for blockfiles
#define kblockfile_quit     (*(void    (*)(struct kfile*))&_kblockfile_quit)
#define kblockfile_open     (*(kerrno_t(*)(struct kfile*,struct kdirent *,struct kinode *,__openmode_t))&_kblockfile_open)
#define kblockfile_read     (*(kerrno_t(*)(struct kfile*,void*,__size_t,__size_t*))&_kblockfile_read)
#define kblockfile_write    (*(kerrno_t(*)(struct kfile*,void const*,__size_t,__size_t*))&_kblockfile_write)
#define kblockfile_seek     (*(kerrno_t(*)(struct kfile*,__off_t,int,__pos_t*))&_kblockfile_seek)
#define kblockfile_trunc    (*(kerrno_t(*)(struct kfile*,__pos_t))&_kblockfile_trunc)
#define kblockfile_ioctl    (*(kerrno_t(*)(struct kfile*,kattr_t,void*))&_kblockfile_ioctl)
#define kblockfile_flush    (*(kerrno_t(*)(struct kfile*))&_kblockfile_flush)
#define kblockfile_getattr  (*(kerrno_t(*)(struct kfile const*,kattr_t,void*,__size_t,__size_t*))&_kblockfile_getattr)
#define kblockfile_setattr  (*(kerrno_t(*)(struct kfile*,kattr_t,void const*,__size_t))&_kblockfile_setattr)
#define kblockfile_inode    (*(__ref struct kinode *(*)(struct kfile *))&_kblockfile_inode)
#define kblockfile_dirent   (*(__ref struct kdirent *(*)(struct kfile *))&_kblockfile_dirent)

extern void     _kblockfile_quit(struct kblockfile *__restrict self);
extern kerrno_t _kblockfile_open(struct kblockfile *__restrict self, struct kdirent *__restrict dirent, struct kinode *__restrict inode, __openmode_t mode);
#define _kblockfile_close  _kblockfile_flush
extern kerrno_t _kblockfile_read(struct kblockfile *__restrict self, void *__restrict buf, __size_t c, __size_t *__restrict rc);
extern kerrno_t _kblockfile_write(struct kblockfile *__restrict self, void const *__restrict buf, __size_t c, __size_t *__restrict wc);
extern kerrno_t _kblockfile_seek(struct kblockfile *__restrict self, __off_t off, int whence, __pos_t *newpos);
extern kerrno_t _kblockfile_trunc(struct kblockfile *__restrict self, __pos_t size);
extern kerrno_t _kblockfile_ioctl(struct kblockfile *__restrict self, kattr_t cmd, __user void *arg);
extern kerrno_t _kblockfile_flush(struct kblockfile *__restrict self);
extern kerrno_t _kblockfile_getattr(struct kblockfile const *__restrict self, kattr_t attr, void *__restrict buf, __size_t bufsize, __size_t *__restrict reqsize);
extern kerrno_t _kblockfile_setattr(struct kblockfile *__restrict self, kattr_t attr, void const *__restrict buf, __size_t bufsize);
extern __ref struct kinode  *_kblockfile_inode(struct kblockfile *__restrict self);
extern __ref struct kdirent *_kblockfile_dirent(struct kblockfile *__restrict self);

__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_FS_FS_BLOCKFILE_H__ */

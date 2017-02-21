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
#ifndef __KOS_KERNEL_FS_VFS_H__
#define __KOS_KERNEL_FS_VFS_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/kernel/fs/file.h>
#include <kos/kernel/fs/fs.h>
#include <sys/stat.h>

__DECL_BEGIN

//////////////////////////////////////////////////////////////////////////
// Generic, virtual file.
// This file type is used if a virtual INode does not implement
// any special file-related features, only requiring the ability
// to be opened for access to the INode.
struct kvfile {
 struct kfile          vf_file;  /*< Underlying file. */
 __ref struct kinode  *vf_inode; /*< [1..1] Associated INode. */
 __ref struct kdirent *vf_dent;  /*< [1..1] Associated Dirent. */
};
#ifdef __INTELLISENSE__
#define KVFILE_TYPE_INIT
#else
#define KVFILE_TYPE_INIT \
 .ft_quit      = &kvfile_quit,\
 .ft_open      = &kvfile_open,\
 .ft_getinode  = &kvfile_getinode,\
 .ft_getdirent = &kvfile_getdirent,
#endif

extern void kvfile_quit(struct kfile *__restrict self);
extern kerrno_t kvfile_open(struct kfile *__restrict self, struct kdirent *__restrict dirent, struct kinode *__restrict inode, __openmode_t mode);
extern __ref struct kinode *kvfile_getinode(struct kfile *__restrict self);
extern __ref struct kdirent *kvfile_getdirent(struct kfile *__restrict self);
extern struct kfiletype kvfile_empty_type;





//////////////////////////////////////////////////////////////////////////
// A virtual static directory entry, used to describe the
// contents of a virtual-only directory, such as '/dev'.
// NOTE: Remember that ontop of this directory data,
//       additional entries may be added, as available
//       through the additional virtual layer of
//       entries in kdirent.
struct kvsdirent {
 struct kdirentname   vd_name;  /*< [eof(vd_name.dn_name == NULL)] Entry name, or NULL for directory end. */
 __ref struct kinode *vd_node;  /*< [1..1] Entry node. */
};
#define KVSDIRENT_INIT(name,node) {KDIRENTNAME_INIT(name),node}
#define KVSDIRENT_INIT_SENTINAL   {KDIRENTNAME_INIT_EMPTY,NULL}

//////////////////////////////////////////////////////////////////////////
// A virtual, static directory node.
// This known of node is used to represent virtual-only directories.
struct kvsdirnode {
 struct kinode     dn_node; /*< Underlying INode. */
 struct kvsdirent *dn_dir;  /*< [1..1] Vector of virtual directory entries. */
};
#define KVSDIRNODE_INIT_EX(superblock,entries,type) \
 {KINODE_INIT(type,&kdirfile_type,superblock,S_IFDIR),entries}
#define KVSDIRNODE_INIT(superblock,entries) \
 KVSDIRNODE_INIT_EX(superblock,entries,&kvsdirnode_type.st_node)
extern struct ksuperblocktype kvsdirnode_type;
extern kerrno_t kvsdirnode_walk(struct kinode *__restrict self, struct kdirentname const *__restrict name, __ref struct kinode **resnode);
extern kerrno_t kvsdirnode_enumdir(struct kinode *__restrict self, pkenumdir callback, void *closure);


//////////////////////////////////////////////////////////////////////////
// The same as a static directory node, but suitable for use as superblock.
struct kvsdirsuperblock { KSUPERBLOCK_TYPE(struct kvsdirnode) };
#define KVSDIRSUPERBLOCK_INIT(entries) \
 {KSUPERBLOCK_INIT_HEAD KVSDIRNODE_INIT(NULL,entries)}



//////////////////////////////////////////////////////////////////////////
// Virtual symbol link node.
// This kine of virtual node is simply a symbolic link, pointing
// to some other relative, or absolute filesystem location.
struct kvlinknode {
 struct kinode      ln_node; /*< Underlying INode. */
 struct kdirentname ln_name; /*< Link text, as read by readlink(). */
};
#define KVLINKNODE_INIT(superblock,text) \
 {KINODE_INIT(&kvlinknode_type,&kvfile_empty_type,superblock,S_IFLNK),\
  KDIRENTNAME_INIT(text)}
extern struct kinodetype kvlinknode_type;

//////////////////////////////////////////////////////////////////////////
// Return, and return a new virtual
extern __ref struct kinode *
kvlinknode_new(struct ksuperblock *superblock, char const *text);



//////////////////////////////////////////////////////////////////////////
// A virtual proxy file, capable of relaying any,
// and all requests to a different, also opened file.
// HINT: This kind of proxy file is used to implement the "/dev/std*"-files.
struct kvproxyfile {
       struct kfile  pf_file; /*< Underlying file. */
 __ref struct kfile *pf_used; /*< [1..1] Used file that data is forwarded into. */
};

extern void     kvproxyfile_quit(struct kfile *__restrict self);
extern kerrno_t kvproxyfile_read(struct kfile *__restrict self, __user void *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict rsize);
extern kerrno_t kvproxyfile_write(struct kfile *__restrict self, __user void const *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict wsize);
extern kerrno_t kvproxyfile_ioctl(struct kfile *__restrict self, kattr_t cmd, __user void *arg);
extern kerrno_t kvproxyfile_getattr(struct kfile const *__restrict self, kattr_t attr, void *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict reqsize); /* NOTE: 'reqsize' may be NULL. */
extern kerrno_t kvproxyfile_setattr(struct kfile *__restrict self, kattr_t attr, void const *__restrict buf, __size_t bufsize);
extern kerrno_t kvproxyfile_pread(struct kfile *__restrict self, __pos_t pos, __user void *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict rsize);
extern kerrno_t kvproxyfile_pwrite(struct kfile *__restrict self, __pos_t pos, __user void const *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict wsize);
extern kerrno_t kvproxyfile_seek(struct kfile *__restrict self, __off_t off, int whence, __kernel __pos_t *__restrict newpos); /* NOTE: 'newpos' may be NULL. */
extern kerrno_t kvproxyfile_trunc(struct kfile *__restrict self, __pos_t size); /*< Set the current end-of-file marker to 'size' */
extern kerrno_t kvproxyfile_flush(struct kfile *__restrict self);
extern kerrno_t kvproxyfile_readdir(struct kfile *__restrict self, __ref struct kinode **__restrict inode, struct kdirentname **__restrict name, __u32 flags); /*< Returns KS_EMPTY when the end is reached. */
extern __ref struct kinode *kvproxyfile_getinode(struct kfile *__restrict self);
extern __ref struct kdirent *kvproxyfile_getdirent(struct kfile *__restrict self);
#ifdef __INTELLISENSE__
#define KVPROXYFILE_TYPE_INIT /* nothing */
#else
#define KVPROXYFILE_TYPE_INIT \
 .ft_size = sizeof(struct kvproxyfile), .ft_quit = &kvproxyfile_quit,\
 .ft_read = &kvproxyfile_read, .ft_write = &kvproxyfile_write,\
 .ft_ioctl = &kvproxyfile_ioctl, .ft_getattr = &kvproxyfile_getattr,\
 .ft_setattr = &kvproxyfile_setattr, .ft_pread = &kvproxyfile_pread,\
 .ft_pwrite = &kvproxyfile_pwrite, .ft_seek = &kvproxyfile_seek,\
 .ft_trunc = &kvproxyfile_trunc, .ft_flush = &kvproxyfile_flush,\
 .ft_readdir = &kvproxyfile_readdir, .ft_getinode = &kvproxyfile_getinode,\
 .ft_getdirent = &kvproxyfile_getdirent,
#endif


#ifdef __MAIN_C__
//////////////////////////////////////////////////////////////////////////
// Mount all virtual file systems in their default locations
// NOTE: Must be called after 'kernel_initialize_filesystem()'
extern void kernel_initialize_vfs(void);
#endif

__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_FS_VFS_H__ */

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
#ifndef __KOS_KERNEL_FS_FAT_C__
#define __KOS_KERNEL_FS_FAT_C__ 1

#include <alloca.h>
#include <byteswap.h>
#include <kos/compiler.h>
#include <kos/kernel/debug.h>
#include <kos/kernel/dev/storage.h>
#include <kos/kernel/fs/fat-internal.h>
#include <kos/kernel/fs/fat.h>
#include <kos/kernel/fs/fs-dirfile.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <kos/syslog.h>

__DECL_BEGIN


__ref struct kfatinode *
kfatinode_new(struct kfatsuperblock *superblock,
              struct kfatfilepos const *fpos,
              struct kfatfileheader const *fheader) {
 struct kfatinode *result;
 kassert_ksuperblock((struct ksuperblock *)superblock);
 kassertobj(fheader);
 if (fheader->f_attr&KFATFILE_ATTR_DIRECTORY) {
  result = (struct kfatinode *)__kinode_alloc((struct ksuperblock *)superblock,
                                              &kfatinode_dir_type,
                                              &kdirfile_type,
                                              S_IFDIR);
 } else {
  result = (struct kfatinode *)__kinode_alloc((struct ksuperblock *)superblock,
                                              &kfatinode_file_type,
                                              (struct kfiletype *)&kfatfile_file_type,
                                              S_IFREG);
 }
 if __likely(result) {
  result->fi_pos  = *fpos;
  result->fi_file = *fheader;
 }
 return result;
}

struct kfat_walkdata {
 char const       *name_str;
 size_t            name_len;
 struct kfatinode *resnode;
};

static kerrno_t kfat_walkcallback(struct kfatfs *fs, struct kfatfilepos const *fpos,
                                  struct kfatfileheader const *file,
                                  char const *filename, size_t filename_size,
                                  struct kfat_walkdata *data) {
#if 0
 printf("FAT Directory entry: %.*q (Cluster: %I32u) (Looking for %.*q)\n",
        (unsigned)filename_size,filename,kfatfileheader_getcluster(file),
        (unsigned)data->name_len,data->name_str);
#endif
 if (data->name_len == filename_size &&
    !memcmp(data->name_str,filename,filename_size*sizeof(char))) {
  /* Found it */
  data->resnode->fi_file = *file;
  data->resnode->fi_pos = *fpos;
  return KS_FOUND;
 }
 return KE_OK;
}

__local kerrno_t
kfatinode_dir_walk_finalize(struct ksuperblock *sblock,
                            struct kfatinode *resnode) {
 kerrno_t error;
 if __unlikely(KE_ISERR(error = ksuperblock_incref(sblock))) return error;
 // Initialize the found node
 kobject_init(&resnode->fi_inode,KOBJECT_MAGIC_INODE);
 __kinode_initcommon(&resnode->fi_inode);
 kcloselock_init(&resnode->fi_inode.i_closelock);
 resnode->fi_inode.i_sblock = sblock;
 if (resnode->fi_file.f_attr&KFATFILE_ATTR_DIRECTORY) {
  *(mode_t *)&resnode->fi_inode.i_kind = S_IFDIR;
  resnode->fi_inode.i_type     = &kfatinode_dir_type;
  resnode->fi_inode.i_filetype = &kdirfile_type;
 } else {
  *(mode_t *)&resnode->fi_inode.i_kind = S_IFREG;
  resnode->fi_inode.i_type     = &kfatinode_file_type;
  resnode->fi_inode.i_filetype = (struct kfiletype *)&kfatfile_file_type;
 }
 return KE_OK;
}
static kerrno_t kfatinode_dir_walk(struct kfatinode *self,
                                   struct kdirentname const *name,
                                   __ref struct kinode **resnod) {
 struct kfat_walkdata data; kerrno_t error;
 kassert_kinode(&self->fi_inode);
 kassertobj(name); kassertobj(resnod);
 kassert_ksuperblock(self->fi_inode.i_sblock);
 data.resnode = omalloc(struct kfatinode);
 if __unlikely(!data.resnode) return KE_NOMEM;
 data.name_len = name->dn_size;
 data.name_str = name->dn_name;
 error = kfatfs_enumdir(&((struct kfatsuperblock *)self->fi_inode.i_sblock)->f_fs,
                        kfatfileheader_getcluster(&self->fi_file),
                       (pkfatfs_enumdir_callback)&kfat_walkcallback,&data);
 if __unlikely(error != KS_FOUND) {
  if (!KE_ISERR(error)) error = KE_NOENT;
err_freenode:
  free(data.resnode);
  return error;
 }
 error = kfatinode_dir_walk_finalize(self->fi_inode.i_sblock,data.resnode);
 if __unlikely(KE_ISERR(error)) goto err_freenode;
 *resnod = (struct kinode *)data.resnode; // Inherit reference
 return KE_OK;
}
__local kerrno_t kfatinode_root_walk(struct kfatinode *self,
                                     struct kdirentname const *name,
                                     __ref struct kinode **resnod) {
 struct kfat_walkdata data; kerrno_t error;
 struct kfatsuperblock *sb;
 kassert_kinode(&self->fi_inode);
 kassertobj(name); kassertobj(resnod);
 assert(!self->fi_inode.i_sblock);
 sb = (struct kfatsuperblock *)__kinode_superblock(self);
 kassert_ksuperblock((struct ksuperblock *)sb);
 data.resnode = omalloc(struct kfatinode);
 if __unlikely(!data.resnode) return KE_NOMEM;
 data.name_len = name->dn_size;
 data.name_str = name->dn_name;
 if (sb->f_fs.f_type == KFSTYPE_FAT32) {
  error = kfatfs_enumdir(&sb->f_fs,sb->f_fs.f_rootcls,
                        (pkfatfs_enumdir_callback)&kfat_walkcallback,&data);
 } else {
  error = kfatfs_enumdirsec(&sb->f_fs,sb->f_fs.f_rootsec,sb->f_fs.f_rootmax,
                           (pkfatfs_enumdir_callback)&kfat_walkcallback,&data);
 }
 if __unlikely(error != KS_FOUND) {
  if (!KE_ISERR(error)) error = KE_NOENT;
err_freenode:
  free(data.resnode);
  return error;
 }
 error = kfatinode_dir_walk_finalize((struct ksuperblock *)sb,data.resnode);
 if __unlikely(KE_ISERR(error)) goto err_freenode;
 *resnod = (struct kinode *)data.resnode; // Inherit reference
 return KE_OK;
}


struct kfat_enumdirdata {
 pkenumdir callback;
 void     *closure;
};

static kerrno_t kfat_enumdir(struct kfatfs *fs, struct kfatfilepos const *fpos,
                             struct kfatfileheader const *file,
                             char const *filename, size_t filename_size,
                             struct kfat_enumdirdata *data) {
 struct kfatinode *fatinode; kerrno_t error;
 struct kdirentname name;
 kassertobj(data);
 fatinode = kfatinode_new(kfatsuperblock_fromfatfs(fs),fpos,file);
 if __unlikely(!fatinode) return KE_NOMEM;
 name.dn_hash = kdirentname_genhash(filename,filename_size);
 name.dn_name = (char *)filename;
 name.dn_size = filename_size;
 error = (*data->callback)((struct kinode *)fatinode,&name,data->closure);
 kinode_decref((struct kinode *)fatinode);
 return error;
}

static kerrno_t kfatinode_dir_enumdir(
 struct kfatinode *self, pkenumdir callback, void *closure) {
 struct kfat_enumdirdata data = {callback,closure};
 return kfatfs_enumdir(&((struct kfatsuperblock *)self->fi_inode.i_sblock)->f_fs,
                       kfatfileheader_getcluster(&self->fi_file),
                       (pkfatfs_enumdir_callback)&kfat_enumdir,&data);
}
static kerrno_t kfatinode_root_enumdir(
 struct kfatinode *self, pkenumdir callback, void *closure) {
 struct kfat_enumdirdata data = {callback,closure};
 struct kfatsuperblock *sblock = (struct kfatsuperblock *)__kinode_superblock(self);
 if (sblock->f_fs.f_type == KFSTYPE_FAT32) {
  return kfatfs_enumdir(&sblock->f_fs,sblock->f_fs.f_rootcls,
                       (pkfatfs_enumdir_callback)&kfat_enumdir,&data);
 } else {
  return kfatfs_enumdirsec(&sblock->f_fs,sblock->f_fs.f_rootsec,sblock->f_fs.f_rootmax,
                           (pkfatfs_enumdir_callback)&kfat_enumdir,&data);
 }
}

static kerrno_t kfatinode_getattr(struct kfatinode const *self, size_t ac, union kinodeattr *av) {
 union kinodeattr *iter,*end;
 end = (iter = av)+ac;
 while (iter != end) {
  switch (iter->ia_common.a_id) {
   case KATTR_FS_ATIME: kfatfileheader_getatime(&self->fi_file,&iter->ia_time.tm_time); break;
   case KATTR_FS_CTIME: kfatfileheader_getctime(&self->fi_file,&iter->ia_time.tm_time); break;
   case KATTR_FS_MTIME: kfatfileheader_getmtime(&self->fi_file,&iter->ia_time.tm_time); break;
   case KATTR_FS_PERM:  iter->ia_perm.p_perm = self->fi_file.f_attr&KFATFILE_ATTR_READONLY ? 0555 : 0777; break;
   case KATTR_FS_OWNER: iter->ia_owner.o_owner = 0; break;
   case KATTR_FS_GROUP: iter->ia_group.g_group = 0; break;
   case KATTR_FS_SIZE:
    iter->ia_size.sz_size = (pos_t)kfatfileheader_getsize(&self->fi_file);
    if (!iter->ia_size.sz_size && S_ISDIR(self->fi_inode.i_kind))
     iter->ia_size.sz_size = 4096;
    break;
   case KATTR_FS_INO:   iter->ia_ino.i_ino = (ino_t)kfatfileheader_getcluster(&self->fi_file); break;
   case KATTR_FS_NLINK: iter->ia_nlink.n_lnk = 1; break;
   case KATTR_FS_KIND:  iter->ia_kind.k_kind = self->fi_inode.i_kind; break;
   default: return KE_NOSYS;
  }
  ++iter;
 }
 return KE_OK;
}
static kerrno_t kfatinode_setattr(struct kfatinode *self, size_t ac, union kinodeattr const *av) {
 union kinodeattr const *iter,*end; int changed = 0;
 struct kfatfs *fs = &((struct kfatsuperblock *)kinode_superblock(&self->fi_inode))->f_fs;
 if __unlikely(!ac) return KE_OK;
 end = (iter = av)+ac;
 while (iter != end) {
  switch (iter->ia_common.a_id) {
   case KATTR_FS_ATIME: if (kfatfileheader_setatime(&self->fi_file,&iter->ia_time.tm_time)) changed = 1; break;
   case KATTR_FS_CTIME: if (kfatfileheader_setctime(&self->fi_file,&iter->ia_time.tm_time)) changed = 1; break;
   case KATTR_FS_MTIME: if (kfatfileheader_setmtime(&self->fi_file,&iter->ia_time.tm_time)) changed = 1; break;
   case KATTR_FS_PERM:
    if (iter->ia_perm.p_perm&0222) {
     if (self->fi_file.f_attr&KFATFILE_ATTR_READONLY) {
      changed = 1;
      self->fi_file.f_attr &= ~(KFATFILE_ATTR_READONLY);
     }
    } else if (!(self->fi_file.f_attr&KFATFILE_ATTR_READONLY)) {
     changed = 1;
     self->fi_file.f_attr |= KFATFILE_ATTR_READONLY;
    }
    break;
   default: return KE_NOSYS;
  }
  ++iter;
 }
 // If nothing changed, skip saving the header
 if __unlikely(!changed) return KE_OK;
 return kfatfs_savefileheader(fs,self->fi_pos.fm_headpos,&self->fi_file);
}

static kerrno_t kfatinode_unlink(struct kfatinode *self,
                                 struct kdirentname const *name,
                                 struct kfatinode *inode) {
 struct kfatfs *fatfs; kerrno_t error;
 assert(self->fi_inode.i_type == &kfatinode_dir_type ||
        self->fi_inode.i_type == &kfatsuperblock_type.st_node);
 assert(inode->fi_inode.i_type == &kfatinode_file_type);
 fatfs = &((struct kfatsuperblock *)kinode_superblock(&self->fi_inode))->f_fs;
 assert(inode->fi_pos.fm_namepos <= inode->fi_pos.fm_headpos);
 error = kfatfs_rmheaders(fatfs,inode->fi_pos.fm_namepos,
                         ((unsigned int)(inode->fi_pos.fm_headpos-inode->fi_pos.fm_namepos))+1);
 if (KE_ISOK(error)) {
  /* Free unused sectors from 'self' */
  error = kfatfs_fat_freeall(fatfs,kfatfileheader_getcluster(&((struct kfatinode *)inode)->fi_file));
 }
 return error;
}
static kerrno_t kfatinode_rmdir(struct kfatinode *self,
                                struct kdirentname const *name,
                                struct kfatinode *inode) {
 struct kfatfs *fatfs; kerrno_t error;
 assert(self->fi_inode.i_type == &kfatinode_dir_type);
 assert(inode->fi_inode.i_type == &kfatinode_dir_type);
 fatfs = &((struct kfatsuperblock *)kinode_superblock(&self->fi_inode))->f_fs;
 error = kfatfs_rmheaders(fatfs,inode->fi_pos.fm_namepos,
                         ((unsigned int)(inode->fi_pos.fm_headpos-inode->fi_pos.fm_namepos))+1);
 if __likely(KE_ISOK(error)) {
  error = kfatfs_fat_freeall(fatfs,kfatfileheader_getcluster(&((struct kfatinode *)inode)->fi_file));
 }
 return error;
}
static kerrno_t
kfatinode_mkdat(struct kfatinode *self,
                struct kdirentname const *name,
                size_t ac, union kinodeattr const *av, 
                __ref struct kfatinode **resnode,
                __u8 fat_file_attributes) {
 kerrno_t error; int need_long_header;
 kfatcls_t dir; int dir_is_sector;
 struct kfatsuperblock *sb = (struct kfatsuperblock *)kinode_superblock((struct kinode *)self);
 struct kfatfs *fs = &sb->f_fs;
 __ref struct kfatinode *newnode;
 newnode = (__ref struct kfatinode *)__kinode_alloc((struct ksuperblock *)sb,
                                                    &kfatinode_dir_type,
                                                    &kdirfile_type,
                                                    S_IFDIR);
 if __unlikely(!newnode) return KE_NOMEM;
 /* Generate the DOS 8.3 short filename. */
 if (kinode_issuperblock((struct kinode *)self)) {
  dir = (dir_is_sector = (fs->f_type != KFSTYPE_FAT32)) ? fs->f_rootsec : fs->f_rootcls;
 } else {
  dir = kfatfileheader_getcluster(&self->fi_file);
  dir_is_sector = 0;
 }
 error = kfatfs_mkshortname(fs,dir,dir_is_sector,&newnode->fi_file,
                            name->dn_name,name->dn_size,&need_long_header);
 if __unlikely(KE_ISERR(error)) goto err_newnode;
 /* Set the EOF marker as cluster, thus marking the directory as empty. */
 kfatfileheader_setcluster(&newnode->fi_file,fs->f_clseof_marker);
 newnode->fi_file.f_attr = fat_file_attributes;
 newnode->fi_file.f_size = leswap_32(0u);
 /* TODO: Initialize timestamps. */

 /* Insert the header into the parent directory. */
 error = need_long_header
  ? kfatfs_mklongheader(fs,dir,dir_is_sector,&newnode->fi_file,name->dn_name,name->dn_size,&newnode->fi_pos)
  : kfatfs_mkheader    (fs,dir,dir_is_sector,&newnode->fi_file,                            &newnode->fi_pos);
 if __unlikely(KE_ISERR(error)) goto err_newnode;
 *resnode = newnode;
 return KE_OK;
err_newnode:
 __kinode_free((struct kinode *)newnode);
 return error;
}
static kerrno_t kfatinode_mkdir(struct kfatinode *self,
                                struct kdirentname const *name,
                                size_t ac, union kinodeattr const *av, 
                                __ref struct kfatinode **resnode) {
 return kfatinode_mkdat(self,name,ac,av,resnode,KFATFILE_ATTR_DIRECTORY);
}
static kerrno_t kfatinode_mkreg(struct kfatinode *self,
                                struct kdirentname const *name,
                                size_t ac, union kinodeattr const *av, 
                                __ref struct kfatinode **resnode) {
 return kfatinode_mkdat(self,name,ac,av,resnode,KFATFILE_ATTR_DIRECTORY);
}

struct kinodetype kfatinode_file_type = {
 .it_size    = sizeof(struct kfatinode),
 .it_getattr = (kerrno_t(*)(struct kinode const *,size_t ac, union kinodeattr *av))&kfatinode_getattr,
 .it_setattr = (kerrno_t(*)(struct kinode *,size_t ac, union kinodeattr const *av))&kfatinode_setattr,
};
struct kinodetype kfatinode_dir_type = {
 .it_size    = sizeof(struct kfatinode),
 .it_walk    = (kerrno_t(*)(struct kinode *,struct kdirentname const *,__ref struct kinode **))&kfatinode_dir_walk,
 .it_enumdir = (kerrno_t(*)(struct kinode *,pkenumdir,void *))&kfatinode_dir_enumdir,
 .it_getattr = (kerrno_t(*)(struct kinode const *,size_t ac, union kinodeattr *av))&kfatinode_getattr,
 .it_setattr = (kerrno_t(*)(struct kinode *,size_t ac, union kinodeattr const *av))&kfatinode_setattr,
 .it_unlink  = (kerrno_t(*)(struct kinode *,struct kdirentname const *,struct kinode *))&kfatinode_unlink,
 .it_rmdir   = (kerrno_t(*)(struct kinode *,struct kdirentname const *,struct kinode *))&kfatinode_rmdir,
 .it_mkdir   = (kerrno_t(*)(struct kinode *,struct kdirentname const *,size_t,union kinodeattr const *,__ref struct kinode **))&kfatinode_mkdir,
 .it_mkreg   = (kerrno_t(*)(struct kinode *,struct kdirentname const *,size_t,union kinodeattr const *,__ref struct kinode **))&kfatinode_mkreg,
};


#define FILE       (self)
#define FATNODE   ((struct kfatinode *)FILE->bf_inode)
#define FATSUPER  ((struct kfatsuperblock *)FILE->bf_inode->i_sblock)
#define FATFS     (&FATSUPER->f_fs)
static kerrno_t kfatfile_loadchunk(struct kblockfile const *self,
                                   struct kfilechunk const *chunk, void *__restrict buf) {
 struct kfatfs const *fs = FATFS;
 return kfatfs_loadsectors(fs,kfatfs_clusterstart(fs,(kfatcls_t)chunk->fc_data),
                           fs->f_sec4clus,buf);
}
static kerrno_t kfatfile_savechunk(struct kblockfile *self,
                                   struct kfilechunk const *chunk, void const *__restrict buf) {
 struct kfatfs *fs = FATFS;
 return kfatfs_savesectors(fs,kfatfs_clusterstart(fs,(kfatcls_t)chunk->fc_data),
                           fs->f_sec4clus,buf);
}
static kerrno_t kfatfile_nextchunk(struct kblockfile *self, struct kfilechunk *chunk) {
 kerrno_t error; kfatcls_t target;
 struct kfatfs *fs = FATFS;
 assertf(chunk->fc_data < fs->f_clseof,"The given chunk contains invalid data");
 error = kfatfs_fat_readandalloc(fs,(kfatcls_t)chunk->fc_data,&target);
 if __likely(KE_ISOK(error)) chunk->fc_data = (__u64)target;
 return error;
}
static kerrno_t kfatfile_findchunk(struct kblockfile *self, struct kfilechunk *chunk) {
 kfatcls_t rescls,newcls; kerrno_t error; __u64 count;
 struct kfatinode *fatnode = FATNODE;
 struct kfatfs *fs = &((struct kfatsuperblock *)fatnode->fi_inode.i_sblock)->f_fs;
 rescls = kfatfileheader_getcluster(&fatnode->fi_file);
 if __unlikely(kfatfs_iseofcluster(fs,rescls)) {
  // Allocate the initial chunk
  if __unlikely(KE_ISERR(error = kfatfs_fat_allocfirst(fs,&rescls))) return error;
  assert(!kfatfs_iseofcluster(fs,rescls));
  // Link the file header to point to that first chunk
  fatnode->fi_file.f_clusterlo = leswap_16((__u16)(rescls));
  fatnode->fi_file.f_clusterhi = leswap_16((__u16)(rescls >> 16));
  // Save the file header to make sure the changes are visible on-disk
  error = kfatfs_savefileheader(fs,fatnode->fi_pos.fm_headpos,&fatnode->fi_file);
  if __unlikely(KE_ISERR(error)) return error;
 }
 count = chunk->fc_index;
 while (count--) {
  error = kfatfs_fat_readandalloc(fs,rescls,&newcls);
  if __unlikely(KE_ISERR(error)) return error;
  rescls = newcls;
 }
 chunk->fc_data = rescls;
 return KE_OK;
}
static kerrno_t kfatfile_releasechunks(struct kblockfile *self, __u64 cindex) {
 kfatcls_t rescls,newcls; kerrno_t error;
 struct kfatinode *fatnode = FATNODE;
 struct kfatfs *fs = &((struct kfatsuperblock *)fatnode->fi_inode.i_sblock)->f_fs;
 rescls = kfatfileheader_getcluster(&fatnode->fi_file);
 /* The following shouldn't happen, but left be careful. */
 if __unlikely(kfatfs_iseofcluster(fs,rescls)) return KE_OK;
 if (cindex) while (--cindex) {
  error = kfatfs_fat_read(fs,rescls,&newcls);
  if __unlikely(KE_ISERR(error)) return error;
  rescls = newcls;
 }
 return kfatfs_fat_freeall(fs,rescls);
}
static kerrno_t kfatfile_getsize(struct kblockfile const *self, __pos_t *fsize) {
 *fsize = (__pos_t)kfatfileheader_getsize(&(FATNODE)->fi_file);
 return KE_OK;
}
static kerrno_t kfatfile_setsize(struct kblockfile *self, __pos_t fsize) {
 struct kfatinode *fatinode = FATNODE;
 struct kfatfs *fs = FATFS;
#ifdef __USE_FILE_OFFSET64
 if __unlikely(fsize > (__pos_t)(__u32)-1) return KE_NOSPC;
#endif
 fatinode->fi_file.f_size = leswap_32((__u32)fsize);
 return kfatfs_savefileheader(fs,fatinode->fi_pos.fm_headpos,&fatinode->fi_file);
}
static kerrno_t kfatfile_getchunksize(struct kblockfile const *self, size_t *chsize) {
 struct kfatfs const *fatfs;
 fatfs = FATFS;
 *chsize = fatfs->f_secsize*fatfs->f_sec4clus;
 return KE_OK;
}


struct kblockfiletype kfatfile_file_type = {
 .bft_file          = KBLOCKFILETYPE_INITFILE,
 .bft_loadchunk     = kfatfile_loadchunk,
 .bft_savechunk     = kfatfile_savechunk,
 .bft_nextchunk     = kfatfile_nextchunk,
 .bft_findchunk     = kfatfile_findchunk,
 .bft_releasechunks = kfatfile_releasechunks,
 .bft_getsize       = kfatfile_getsize,
 .bft_setsize       = kfatfile_setsize,
 .bft_getchunksize  = kfatfile_getchunksize,
};







kerrno_t kfatsuperblock_init(struct kfatsuperblock *self, struct ksdev *dev) {
 kerrno_t error;
 if __unlikely(KE_ISERR(error = kfatfs_init(&self->f_fs,dev))) return error;
 // todo: Special node type for root directory
 memset(&self->s_root.fi_file.f_name,' ',offsetof(struct kfatfileheader,f_attr));
 memset(&self->s_root.fi_file.f_attr,0,
        sizeof(struct kfatfileheader)-
        offsetof(struct kfatfileheader,f_attr));
 self->s_root.fi_file.f_attr = KFATFILE_ATTR_DIRECTORY;
 return KE_OK;
}
static kerrno_t kfatsuperblock_close(struct ksuperblock *self) {
 return kfatfs_fat_close(&((struct kfatsuperblock *)self)->f_fs);
}
static kerrno_t kfatsuperblock_flush(struct ksuperblock *self) {
 return kfatfs_fat_flush(&((struct kfatsuperblock *)self)->f_fs);
}
static void kfatsuperinode_quit(struct kinode *self) {
 struct kfatfs *fs = &((struct kfatsuperblock *)__kinode_superblock(self))->f_fs;
 kerrno_t error = kfatfs_fat_close(fs);
 if (KE_ISERR(error) && error != KE_DESTROYED) {
  k_syslogf(KLOG_ERROR,"Failed to properly close FAT filesystem: %d\n",error);
 }
 kfatfs_quit(fs);
}


static __ref struct ksdev *kfatsuperblock_getdev(struct kfatsuperblock *self) {
 struct ksdev *result = self->f_fs.f_dev;
 if __unlikely(KE_ISERR(ksdev_incref(result))) result = NULL;
 return result;
}
struct ksuperblocktype kfatsuperblock_type = {
 .st_node = {
  .it_size    = sizeof(struct kfatsuperblock),
  .it_quit    = (void(*)(struct kinode *))&kfatsuperinode_quit,
  .it_walk    = (kerrno_t(*)(struct kinode *,struct kdirentname const *,__ref struct kinode **))&kfatinode_root_walk,
  .it_enumdir = (kerrno_t(*)(struct kinode *,pkenumdir,void *))&kfatinode_root_enumdir,
  .it_getattr = (kerrno_t(*)(struct kinode const *,size_t ac, union kinodeattr *av))&kfatinode_getattr,
  .it_setattr = (kerrno_t(*)(struct kinode *,size_t ac, union kinodeattr const *av))&kfatinode_setattr,
  .it_unlink  = (kerrno_t(*)(struct kinode *,struct kdirentname const *,struct kinode *))&kfatinode_unlink,
  .it_rmdir   = (kerrno_t(*)(struct kinode *,struct kdirentname const *,struct kinode *))&kfatinode_rmdir,
  .it_mkdir   = (kerrno_t(*)(struct kinode *,struct kdirentname const *,size_t,union kinodeattr const *,__ref struct kinode **))&kfatinode_mkdir,
  .it_mkreg   = (kerrno_t(*)(struct kinode *,struct kdirentname const *,size_t,union kinodeattr const *,__ref struct kinode **))&kfatinode_mkreg,
 },
 .st_close    = (kerrno_t(*)(struct ksuperblock *))&kfatsuperblock_close,
 .st_flush    = (kerrno_t(*)(struct ksuperblock *))&kfatsuperblock_flush,
 .st_getdev   = (__ref struct ksdev *(*)(struct ksuperblock const *))&kfatsuperblock_getdev,
 .st_getattr  = NULL,
 .st_setattr  = NULL,
};



__DECL_END

#ifndef __INTELLISENSE__
#include "fat-internal.c.inl"
#endif

#endif /* !__KOS_KERNEL_FS_FAT_H__ */

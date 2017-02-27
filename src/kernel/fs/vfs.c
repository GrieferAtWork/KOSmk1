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
#ifndef __KOS_KERNEL_FS_VFS_C__
#define __KOS_KERNEL_FS_VFS_C__ 1

#include <kos/kernel/fs/vfs.h>
#include <kos/kernel/fs/fs.h>
#include <kos/kernel/fs/vfs-dev.h>
#include <kos/kernel/fs/vfs-proc.h>
#include <kos/kernel/fs/fs-dirfile.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <kos/kernel/tty.h>
#include <kos/syslog.h>

__DECL_BEGIN

//////////////////////////////////////////////////////////////////////////
// === kvfile ===
extern struct kfiletype kvfile_empty_type;
void kvfile_quit(struct kfile *__restrict self) {
 kinode_decref(((struct kvfile *)self)->vf_inode);
 kdirent_decref(((struct kvfile *)self)->vf_dent);
}
kerrno_t kvfile_open(struct kfile *__restrict self, struct kdirent *__restrict dirent,
                    struct kinode *__restrict inode, __openmode_t __unused(mode)) {
 kerrno_t error;
 if __unlikely(KE_ISERR(error = kinode_incref(inode))) return error;
 if __unlikely(KE_ISERR(error = kdirent_incref(dirent))) { kinode_decref(inode); return error; }
 ((struct kvfile *)self)->vf_inode = inode;
 ((struct kvfile *)self)->vf_dent = dirent;
 return KE_OK;
}
__ref struct kinode *kvfile_getinode(struct kfile *__restrict self) {
 struct kinode *result;
 result = ((struct kvfile *)self)->vf_inode;
 if __unlikely(KE_ISERR(kinode_incref(result))) result = NULL;
 return result;
}
__ref struct kdirent *kvfile_getdirent(struct kfile *__restrict self) {
 struct kdirent *result;
 result = ((struct kvfile *)self)->vf_dent;
 if __unlikely(KE_ISERR(kdirent_incref(result))) result = NULL;
 return result;
}
struct kfiletype kvfile_empty_type = {
 .ft_size      = sizeof(struct kvfile),
 .ft_quit      = &kvfile_quit,
 .ft_open      = &kvfile_open,
 .ft_getinode  = &kvfile_getinode,
 .ft_getdirent = &kvfile_getdirent,
};



//////////////////////////////////////////////////////////////////////////
// === kvsdirnode ===
extern struct ksuperblocktype kvsdirnode_type;
kerrno_t
kvsdirnode_walk(struct kinode *__restrict self,
                struct kdirentname const *__restrict name,
                __ref struct kinode **resnode) {
 struct kvsdirent *iter;
 //assert(self->i_type == &kvsdirnode_type.st_node);
 iter = ((struct kvsdirnode *)self)->dn_dir;
 kassertobj(iter);
 while (!kdirentname_isempty(&iter->vd_name)) {
  if (kdirentname_lazyequal(&iter->vd_name,name))
   return kinode_incref(*resnode = iter->vd_node);
  ++iter;
 }
 return KE_NOENT;
}
kerrno_t
kvsdirnode_enumdir(struct kinode *__restrict self, pkenumdir callback, void *closure) {
 kerrno_t error; struct kvsdirent *iter;
 //assert(self->i_type == &kvsdirnode_type.st_node);
 iter = ((struct kvsdirnode *)self)->dn_dir;
 kassertobj(iter);
 while (!kdirentname_isempty(&iter->vd_name)) {
  kdirentname_lazyhash(&iter->vd_name);
  error = (*callback)(iter->vd_node,&iter->vd_name,closure);
  if __unlikely(error != KE_OK) return error;
  ++iter;
 }
 return KE_OK;
}
struct ksuperblocktype kvsdirnode_type = {
 .st_node = {
  .it_size    = sizeof(struct kvsdirnode),
  .it_walk    = &kvsdirnode_walk,
  .it_enumdir = &kvsdirnode_enumdir,
 },
};





//////////////////////////////////////////////////////////////////////////
// === Virtual Symbolic Link File ===
__ref struct kinode *
kvlinknode_new(struct ksuperblock *superblock,
               char const *text) {
 __ref struct kvlinknode *result;
 if ((text = (char const *)strdup(text)) == NULL) return NULL;
 result = (__ref struct kvlinknode *)__kinode_alloc(superblock,
                                                     &kvlinknode_type,
                                                     &kvfile_empty_type,
                                                      S_IFLNK);
 if __unlikely(!result) { free((void *)text); return NULL; }
 kdirentname_init(&result->ln_name,(char *)text);
 return (struct kinode *)result;
}
static void kvlinknode_quit(struct kinode *__restrict self) {
 kdirentname_quit(&((struct kvlinknode *)self)->ln_name);
}
static kerrno_t
kvlinknode_readlink(struct kinode *__restrict self,
                    struct kdirentname *target) {
 return kdirentname_initcopy(target,&((struct kvlinknode *)self)->ln_name);
}
struct kinodetype kvlinknode_type = {
 .it_size     = sizeof(struct kvlinknode),
 .it_quit     = &kvlinknode_quit,
 .it_readlink = &kvlinknode_readlink,
};








//////////////////////////////////////////////////////////////////////////
// === Virtual Proxy File ===
#define SELF  ((struct kvproxyfile *)self)
void kvproxyfile_quit(struct kfile *__restrict self) {
 kfile_decref(SELF->pf_used);
}
kerrno_t
kvproxyfile_read(struct kfile *__restrict self,
                 __user void *__restrict buf, size_t bufsize,
                 __kernel size_t *__restrict rsize) {
 return kfile_user_read(SELF->pf_used,buf,bufsize,rsize);
}
kerrno_t
kvproxyfile_write(struct kfile *__restrict self,
                  __user void const *__restrict buf, size_t bufsize,
                  __kernel size_t *__restrict wsize) {
 return kfile_user_write(SELF->pf_used,buf,bufsize,wsize);
}
kerrno_t
kvproxyfile_ioctl(struct kfile *__restrict self,
                  kattr_t cmd, __user void *arg) {
 return kfile_user_ioctl(SELF->pf_used,cmd,arg);
}
kerrno_t
kvproxyfile_getattr(struct kfile const *__restrict self, kattr_t attr,
                    __user void *__restrict buf, size_t bufsize,
                    __kernel size_t *__restrict reqsize) {
 return kfile_user_getattr(SELF->pf_used,attr,buf,bufsize,reqsize);
}
kerrno_t
kvproxyfile_setattr(struct kfile *__restrict self, kattr_t attr,
                    __user void const *__restrict buf, size_t bufsize) {
 return kfile_user_setattr(SELF->pf_used,attr,buf,bufsize);
}
kerrno_t
kvproxyfile_pread(struct kfile *__restrict self, pos_t pos,
                  __user void *__restrict buf, size_t bufsize,
                  __kernel size_t *__restrict rsize) {
 return kfile_user_pread(SELF->pf_used,pos,buf,bufsize,rsize);
}
kerrno_t
kvproxyfile_pwrite(struct kfile *__restrict self, pos_t pos,
                   __user void const *__restrict buf, size_t bufsize,
                   __kernel size_t *__restrict wsize) {
 return kfile_user_pwrite(SELF->pf_used,pos,buf,bufsize,wsize);
}
kerrno_t
kvproxyfile_seek(struct kfile *__restrict self, off_t off,
                 int whence, __kernel pos_t *__restrict newpos) {
 return kfile_seek(SELF->pf_used,off,whence,newpos);
}
kerrno_t
kvproxyfile_trunc(struct kfile *__restrict self,
                  pos_t size) {
 return kfile_trunc(SELF->pf_used,size);
}
kerrno_t
kvproxyfile_flush(struct kfile *__restrict self) {
 return kfile_flush(SELF->pf_used);
}
kerrno_t
kvproxyfile_readdir(struct kfile *__restrict self,
                    __ref struct kinode **__restrict inode,
                    struct kdirentname **__restrict name,
                    __u32 flags) {
 return kfile_readdir(SELF->pf_used,inode,name,flags);
}
__crit __ref struct kinode *
kvproxyfile_getinode(struct kfile *__restrict self) {
 return kfile_getinode(SELF->pf_used);
}
__crit __ref struct kdirent *
kvproxyfile_getdirent(struct kfile *__restrict self) {
 return kfile_getdirent(SELF->pf_used);
}
#undef SELF



//////////////////////////////////////////////////////////////////////////
// Virtual File system Initialization
static void
vfs_mount(char const *__restrict path,
          struct ksuperblock *__restrict block) {
 kerrno_t error;
 error = krootfs_mount(path,(size_t)-1,block,NULL);
 if __unlikely(KE_ISERR(error)) {
  k_syslogf(KLOG_ERROR,
            "[VFS] Failed to mount virtual filesystem under %q (error %d)\n",
            path,error);
 }
}


void kernel_initialize_vfs(void) {
 vfs_mount("/dev",(struct ksuperblock *)&kvfs_dev);
 vfs_mount("/proc",(struct ksuperblock *)&kvfs_proc);
}


__DECL_END

#endif /* !__KOS_KERNEL_FS_VFS_C__ */

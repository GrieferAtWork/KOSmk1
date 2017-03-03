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
#ifndef __KOS_KERNEL_FS_FS_DIRFILE_C_INL__
#define __KOS_KERNEL_FS_FS_DIRFILE_C_INL__ 1

#include <kos/compiler.h>
#include <kos/kernel/fs/fs.h>
#include <kos/kernel/fs/fs-dirfile.h>
#include <malloc.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

__DECL_BEGIN

// TODO: When a directory is longer than a set amount of entries
//      (say 64), a different form of enumeration should be
//       performed using a specially allocated kernel-stack for
//       enumerating the directory, only ever reading as far
//       ahead as required for the current seek()+readdir() position.
//    >> This way, enumerating ~really~ big folders won't cause
//       the kernel to hang momentarily on the first entry, instead
//       evenly spreading the load over all entries while allowing
//       us to simply stop enumerating by returned non-zero from
//       what is the current implementation of 'kdirfile_enum'.
// NOTE: The hard part will be differentiating between short and
//       long directories, as there is no API for figuring out
//       the length of one without enumerating it as well.
//       We can't simply copy the kernel-stack, once we reach a
//       certain limit, as that would break any stack-based pointers.
//       Instead, we have to decide if long enumeration makes sense
//       before even staring, or knowing anything about the folder...

struct kdirfilelist {
 size_t                ent_c;
 size_t                ent_a;
 struct kdirfileentry *ent_v;
};

__local kerrno_t
kdirfilelist_append(struct kdirfilelist *__restrict self,
                    __ref struct kinode *__restrict inode,
                    struct kdirentname const *__restrict name) {
 kerrno_t error;
 struct kdirfileentry *entry;
 kassert_kinode(inode);
 kassertobj(name);
 kassertobj(self);
 if __unlikely(self->ent_c == self->ent_a) {
  size_t newbufsize; struct kdirfileentry *newbuf;
  if (!self->ent_a) newbufsize = 2;
  else newbufsize = self->ent_a*2;
  newbuf = (struct kdirfileentry *)realloc(self->ent_v,newbufsize*
                                           sizeof(struct kdirfileentry));
  if __unlikely(!newbuf) return KE_NOMEM;
  self->ent_a = newbufsize;
  self->ent_v = newbuf;
 }
 entry = &self->ent_v[self->ent_c++];
 entry->dfe_inode = inode; /* Inherit reference. */
 entry->dfe_dirent = NULL;
 error = kdirentname_initcopy(&entry->dfe_name,name);
 if __unlikely(KE_ISERR(error)) --self->ent_c;
 return error;
}
__local kerrno_t
kdirfilelist_insertnew(struct kdirfilelist *__restrict self,
                       __ref struct kinode *__restrict inode,
                       struct kdirentname const *__restrict name) {
 struct kdirfileentry *iter,*end;
 kassertobj(self);
 kassertobj(name);
 kassert_kinode(inode);
 end = (iter = self->ent_v)+self->ent_c;
 // Make sure the given data wasn't already enumerated
 while (iter != end) {
  if (kdirentname_equal(&iter->dfe_name,name))
   return KS_FOUND;
  ++iter;
 }
 return kdirfilelist_append(self,inode,name);
}



static kerrno_t
kdirfile_enum(struct kinode *__restrict inode,
              struct kdirentname const *__restrict name,
              struct kdirfilelist *__restrict buf) {
 kerrno_t error;
 if __unlikely(KE_ISERR(error = kinode_incref(inode))) return error;
 error = kdirfilelist_insertnew(buf,inode,name);
 if __unlikely(error != KE_OK) {
  if (error == KS_FOUND) error = KE_OK;
  kinode_decref(inode);
 }
 return error;
}

__local kerrno_t
kdirfile_fillcache(struct kdirfilelist *__restrict self,
                   struct kdirentcache *__restrict cache) {
 // Search the dirent's child cache for virtual files
 // NOTE: For this, we must be careful not to re-use 
 struct kdirent **iter,**end; size_t totalsize;
 struct kdirentcachevec *veciter,*vecend;
 struct kdirfileentry *diter;
 kerrno_t error = KE_OK;
 kassertobj(self);
 kassertobj(cache);
 vecend = (veciter = cache->dc_cache)+__compiler_ARRAYSIZE(cache->dc_cache);
 totalsize = 0;
 while (veciter != vecend) totalsize += (*veciter++).d_vecc;
 self->ent_a = totalsize;
 self->ent_c = totalsize;
 if (!totalsize) {
  // No cached entries.
  self->ent_v = NULL;
  return KE_OK;
 }
 self->ent_v = (struct kdirfileentry *)malloc(totalsize*sizeof(struct kdirfileentry));
 if __unlikely(!self->ent_v) return KE_NOMEM;
 diter = self->ent_v;
 veciter = cache->dc_cache;
 while (veciter != vecend) {
  end = (iter = veciter->d_vecv)+veciter->d_vecc;
  while (iter != end) {
   diter->dfe_dirent = *iter;
   diter->dfe_name = diter->dfe_dirent->d_name; // Weakly referenced
   diter->dfe_inode = kdirent_getnode(diter->dfe_dirent);
   if __unlikely(!diter->dfe_inode) {
    --self->ent_c; // Dead cache entry (ignore & re-use as buffer for later)
    goto nextentry;
   }
   kassert_kinode(diter->dfe_inode);
   if __unlikely(KE_ISERR(error = kdirent_incref(diter->dfe_dirent))) {
    kinode_decref(diter->dfe_inode);
/*errvec:*/
    while (diter-- != self->ent_v) {
     assertf(diter->dfe_dirent,"This generate should only generate dirent-dependent entries");
     kinode_decref(diter->dfe_inode);
     kdirent_decref(diter->dfe_dirent);
    }
    free(self->ent_v);
    return error;
   }
   ++diter;
nextentry:
   ++iter;
  }
  ++veciter;
 }
 return KE_OK;
}

static kerrno_t
kdirfile_fill(struct kdirfile *__restrict self) {
 kerrno_t error; struct kinode *inode; struct kdirent *mydirent;
 struct kdirfilelist data; size_t oldseek;
 kerrno_t(*callback)(struct kinode *,pkenumdir,void *);
 if __likely(self->df_entv) return KE_OK;
 inode = self->df_inode;
 kassert_kinode(inode);
 kassertobj(inode->i_type);
 callback = inode->i_type->it_enumdir;
 oldseek = (size_t)(self->df_curr-(struct kdirfileentry *)NULL);
 if __unlikely(!callback) return KE_NOSYS;
 // Start iterating
 data.ent_a = 0;
 data.ent_c = 0;
 data.ent_v = NULL;
 mydirent = self->df_dirent;
 kdirent_lock(mydirent,KDIRENT_LOCK_CACHE);
 error = kdirfile_fillcache(&data,&mydirent->d_cache);
 kdirent_unlock(mydirent,KDIRENT_LOCK_CACHE);
 if __unlikely(KE_ISERR(error)) {
  struct kdirfileentry *iter,*end;
err_data:
  end = (iter = data.ent_v)+data.ent_c;
  for (;iter != end; ++iter) kdirfileentry_quit(iter);
  free(data.ent_v);
  return error;
 }
 // Now that the cached directory data is enumerated, it's time
 // to list the part that isn't cached (aka. data originating from the inode)
 error = (*callback)(inode,(pkenumdir)&kdirfile_enum,&data);
 if __unlikely(KE_ISERR(error)) goto err_data;
 if (data.ent_c) {
  if __likely(data.ent_c != data.ent_a) {
   // Try to free up some memory
   self->df_entv = (struct kdirfileentry *)realloc(data.ent_v,data.ent_c*
                                                   sizeof(struct kdirfileentry));
   if __unlikely(!self->df_entv) self->df_entv = data.ent_v;
  } else {
   self->df_entv = data.ent_v;
  }
  self->df_endv = self->df_entv+data.ent_c;
 } else {
  self->df_endv = KDIRFILE_ENTV_EMPTY;
  self->df_entv = KDIRFILE_ENTV_EMPTY;
 }
 self->df_curr = self->df_entv+oldseek;
 return error;
}

__local void
kdirfile_freeentries(struct kdirfile *__restrict self) {
 struct kdirfileentry *iter,*end;
 if (self->df_entv != KDIRFILE_ENTV_EMPTY) {
  iter = self->df_entv,end = self->df_endv;
  for (; iter != end; ++iter) kdirfileentry_quit(iter);
  free(self->df_entv);
 }
}

static void
kdirfile_quit(struct kdirfile *__restrict self) {
 kdirfile_freeentries(self);
 kdirent_decref(self->df_dirent);
 kinode_decref(self->df_inode);
}

static kerrno_t
kdirfile_open(struct kdirfile *__restrict self,
              struct kdirent *__restrict dirent,
              struct kinode *__restrict inode, openmode_t mode) {
 kerrno_t error;
 // Directories must be opened read-only
 if (mode&(O_WRONLY|O_RDWR)) return KE_ISDIR;
 kmutex_init(&self->df_lock);
 self->df_curr = NULL;
 self->df_entv = NULL;
 self->df_endv = NULL;
 if __unlikely(KE_ISERR(error = kdirent_incref(self->df_dirent = dirent))) return error;
 if __unlikely(KE_ISERR(error = kinode_incref(self->df_inode = inode))) { kdirent_decref(dirent); return error; }
 return KE_OK;
}

static kerrno_t
kdirfile_seek(struct kdirfile *__restrict self, off_t off,
              int whence, pos_t *__restrict newpos) {
 kerrno_t error;
 if __unlikely(KE_ISERR(error = kmutex_lock(&self->df_lock))) return error;
 switch (whence) {
  case SEEK_SET: self->df_curr = self->df_entv+(size_t)off; break;
  case SEEK_CUR: self->df_curr += (ssize_t)off; break;
  case SEEK_END:
   if __likely(KE_ISOK(error = kdirfile_fill(self))) goto end;
   self->df_curr = self->df_endv-(size_t)off;
   break;
 }
 error = KE_OK;
end:
 kmutex_unlock(&self->df_lock);
 return error;
}

static kerrno_t
kdirfile_readdir(struct kdirfile *__restrict self,
                 __ref struct kinode **__restrict inode,
                 struct kdirentname **__restrict name,
                 __u32 flags) {
 kerrno_t error;
 kassertobj(inode);
 kassertobj(name);
 kassert_kfile(&self->df_file);
 if __unlikely(KE_ISERR(error = kmutex_lock(&self->df_lock))) return error;
 if __unlikely(KE_ISERR(error = kdirfile_fill(self))) goto end;
 kassert_kfile(&self->df_file);
 if __likely(self->df_curr >= self->df_entv &&
             self->df_curr < self->df_endv) {
  error = kinode_incref((*inode = self->df_curr->dfe_inode));
  if __unlikely(KE_ISERR(error)) goto end;
  *name = &self->df_curr->dfe_name;
  if (!(flags&KFILE_READDIR_FLAG_PEEK)) ++self->df_curr;
 } else {
  error = KS_EMPTY;
 }
end:
 kmutex_unlock(&self->df_lock);
 return error;
}

static kerrno_t
kdirfile_ioctl(struct kdirfile *__restrict self,
               kattr_t cmd, __user void *arg) {
 return kfile_generic_ioctl(&self->df_file,cmd,arg);
}

static kerrno_t
kdirfile_getattr(struct kdirfile const *self, kattr_t attr,
                 __user void *__restrict buf, size_t bufsize,
                 __kernel size_t *__restrict reqsize) {
 return kfile_generic_getattr(&self->df_file,attr,buf,bufsize,reqsize);
}
static kerrno_t
kdirfile_setattr(struct kdirfile *__restrict self, kattr_t attr,
                 __user void const *__restrict buf, size_t bufsize) {
 return kfile_generic_setattr(&self->df_file,attr,buf,bufsize);
}

static __ref struct kinode *
kdirfile_getinode(struct kdirfile *__restrict self) {
 return __likely(KE_ISOK(kinode_incref(self->df_inode))) ? self->df_inode : NULL;
}

static __ref struct kdirent *
kdirfile_getdirent(struct kdirfile *__restrict self) {
 return __likely(KE_ISOK(kdirent_incref(self->df_dirent))) ? self->df_dirent : NULL;
}


__ref struct kdirfile *kdirfile_newroot(void) {
 struct kdirfile *result;
 if __unlikely((result = omalloc(struct kdirfile)) == NULL) return NULL;
 if __unlikely((result->df_inode = kdirent_getnode(&__kfs_root)) == NULL) goto err_r;
 if __unlikely(KE_ISERR(kdirent_incref(&__kfs_root))) goto err_root;
 result->df_dirent = &__kfs_root;
 kobject_init(&result->df_file,KOBJECT_MAGIC_FILE);
 result->df_file.f_type = &kdirfile_type;
 result->df_file.f_refcnt = 1;
 kmutex_init(&result->df_lock);
 result->df_curr = NULL;
 result->df_entv = NULL;
 result->df_endv = NULL;
 return result;
err_root: kinode_decref(result->df_inode);
err_r: free(result);
 return NULL;
}


struct kfiletype kdirfile_type = {
 .ft_size      = sizeof(struct kdirfile),
 .ft_quit      = (void(*)(struct kfile *))&kdirfile_quit,
 .ft_open      = (kerrno_t(*)(struct kfile *,struct kdirent *,struct kinode *,openmode_t))&kdirfile_open,
 .ft_seek      = (kerrno_t(*)(struct kfile *,off_t,int,pos_t *))&kdirfile_seek,
 .ft_ioctl     = (kerrno_t(*)(struct kfile *,kattr_t,void *))&kdirfile_ioctl,
 .ft_getattr   = (kerrno_t(*)(struct kfile const *,kattr_t,void *,size_t,size_t *))&kdirfile_getattr,
 .ft_setattr   = (kerrno_t(*)(struct kfile *,kattr_t,void const *,size_t))&kdirfile_setattr,
 .ft_readdir   = (kerrno_t(*)(struct kfile *,__ref struct kinode **,struct kdirentname **,__u32))&kdirfile_readdir,
 .ft_getinode  = (struct kinode *(*)(struct kfile *))&kdirfile_getinode,
 .ft_getdirent = (struct kdirent *(*)(struct kfile *))&kdirfile_getdirent,
 .ft_read      = &kfile_generic_read_isdir,
 .ft_write     = &kfile_generic_write_isdir,
 .ft_pread     = &kfile_generic_readat_isdir,
 .ft_pwrite    = &kfile_generic_writeat_isdir,
};

__DECL_END

#endif /* !__KOS_KERNEL_FS_FS_DIRFILE_C_INL__ */

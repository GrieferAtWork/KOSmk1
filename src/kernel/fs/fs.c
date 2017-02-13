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
#ifndef __KOS_KERNEL_FS_FS_C__
#define __KOS_KERNEL_FS_FS_C__ 1

#include <ctype.h>
#include <fcntl.h>
#include <kos/compiler.h>
#include <kos/kernel/debug.h>
#include <kos/errno.h>
#include <kos/kernel/fs/fat.h>
#include <kos/kernel/fs/file.h>
#include <kos/kernel/fs/fs-dirfile.h>
#include <kos/kernel/fs/fs.h>
#include <kos/kernel/fs/vfs.h>
#include <kos/kernel/mutex.h>
#include <kos/kernel/proc.h>
#include <kos/syslog.h>
#include <malloc.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

__DECL_BEGIN

#define ksuperblock_destroy(self) kinode_destroy((struct kinode *)(self))


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// --- KINODE
__crit __ref struct kinode *
__kinode_alloc(struct ksuperblock *superblock,
               struct kinodetype *nodetype,
               struct kfiletype *filetype,
               mode_t filekind) {
 struct kinode *result;
 kassert_ksuperblock(superblock);
 kassertobj(nodetype);
 kassertobj(filetype);
 assertf(nodetype->it_size >= sizeof(struct kinode),
         "Size described by the given node type is too small (%Iu < %Iu)",
         nodetype->it_size,sizeof(struct kinode));
 if __unlikely(KE_ISERR(ksuperblock_incref(superblock))) return NULL;
 result = (struct kinode *)malloc(nodetype->it_size);
 if __likely(result) {
  kobject_init(result,KOBJECT_MAGIC_INODE);
  __kinode_initcommon(result);
  result->i_type     = nodetype;
  result->i_filetype = filetype;
  result->i_sblock   = superblock;
  result->i_kind = filekind;
  kcloselock_init(&result->i_closelock);
 } else {
  ksuperblock_decref(superblock);
 }
 return result;
}
__crit void __kinode_free(struct kinode *self) {
 kassert_kinode(self);
 assert(self->i_refcnt == 1);
 kassert_ksuperblock(self->i_sblock);
 ksuperblock_decref(self->i_sblock);
 free(self);
}



void ksuperblock_syslogprefix(int level, struct ksuperblock *self) {
 char fs_type[64],fs_name[64];
 if (KE_ISERR(ksuperblock_getattr(self,KATTR_DEV_TYPE,fs_type,sizeof(fs_type),NULL))) strcpy(fs_type,"??" "?");
 if (KE_ISERR(ksuperblock_getattr(self,KATTR_DEV_NAME,fs_name,sizeof(fs_name),NULL))) strcpy(fs_name,"??" "?");
 k_dosyslogf(level,NULL,NULL,"[%.64s|%.64s] ",fs_type,fs_name);
}

kerrno_t kinode_close(struct kinode *self) {
 kerrno_t error;
 if __likely(KE_ISOK(error = kcloselock_close(&self->i_closelock)) &&
             kinode_issuperblock(self)) {
  kerrno_t(*closecall)(struct ksuperblock *self);
  struct ksuperblock *sblock = __kinode_superblock(self);
  kassert_ksuperblock(sblock);
  if (k_sysloglevel >= KLOG_MSG) {
   ksuperblock_syslogprefix(KLOG_MSG,sblock);
   k_dosyslog(KLOG_MSG,NULL,NULL,"Closing filesystem\n",(size_t)-1);
  }
  closecall = ksuperblock_type(sblock)->st_close;
  if (closecall && __unlikely(KE_ISERR(error = (*closecall)(sblock)))) {
   kcloselock_reset(&self->i_closelock);
   if (k_sysloglevel >= KLOG_ERROR) {
    ksuperblock_syslogprefix(KLOG_ERROR,sblock);
    k_dosyslogf(KLOG_ERROR,NULL,NULL,"Failed to close filesystem: %d\n",error);
   }
   return error;
  }
  _ksuperblock_clearmnt(sblock);
 }
 return error;
}

extern void kinode_destroy(struct kinode *self);
void kinode_destroy(struct kinode *self) {
 struct ksuperblock *obbase;
 void(*quitcall)(struct kinode *);
 kassert_object(self,KOBJECT_MAGIC_INODE);
 if (kinode_issuperblock(self)) {
  kerrno_t(*closecall)(struct ksuperblock *self);
  obbase = __kinode_superblock(self);
  kassert_object((struct ksuperblock *)obbase,KOBJECT_MAGIC_SUPERBLOCK);
  assertf(!obbase->s_mount.sm_mntc,
          "How can there be no references if "
          "this superblock is still mounted?");
  closecall = ksuperblock_type(obbase)->st_close;
  if (closecall && KE_ISOK(kcloselock_close(&self->i_closelock))) (*closecall)(obbase);
 } else obbase = (struct ksuperblock *)self;
 quitcall = self->i_type->it_quit;
 if (quitcall) { kassertbyte(quitcall); (*quitcall)(self); }
 if (self->i_sblock) ksuperblock_decref(self->i_sblock);
 // Finally, free the memory allocated for the node
 free(obbase);
}

kerrno_t kinode_getattr(struct kinode const *self, size_t ac, union kinodeattr *av) {
 kerrno_t error; kassert_kinode(self);
 kassertmem(av,ac*sizeof(union kinodeattr));
 error = kcloselock_beginop((struct kcloselock *)&self->i_closelock);
 if __unlikely(KE_ISERR(error)) return error;
 /* TODO: Make use of the INode cache. */
 error = self->i_type->it_getattr ? (*self->i_type->it_getattr)(self,ac,av)
                                  : kinode_generic_getattr(self,ac,av);
 kcloselock_endop((struct kcloselock *)&self->i_closelock);
 return error;
}
kerrno_t kinode_setattr(struct kinode *self, size_t ac, union kinodeattr const *av) {
 kerrno_t error; kassert_kinode(self);
 kassertmem(av,ac*sizeof(union kinodeattr));
 error = kcloselock_beginop((struct kcloselock *)&self->i_closelock);
 if __unlikely(KE_ISERR(error)) return error;
 /* TODO: Make use of the INode cache. */
 error = self->i_type->it_setattr ? (*self->i_type->it_setattr)(self,ac,av)
                                  : kinode_generic_setattr(self,ac,av);
 kcloselock_endop((struct kcloselock *)&self->i_closelock);
 return error;
}
__local size_t kinode_getlegacyattributesize(kattr_t attr) {
 switch (attr) {
  case KATTR_FS_ATIME:
  case KATTR_FS_CTIME:
  case KATTR_FS_MTIME:
   return sizeof(struct kinodeattr_time)-sizeof(struct kinodeattr_common);
  case KATTR_FS_PERM:
   return sizeof(struct kinodeattr_perm)-sizeof(struct kinodeattr_common);
  case KATTR_FS_OWNER:
   return sizeof(struct kinodeattr_owner)-sizeof(struct kinodeattr_common);
  case KATTR_FS_GROUP:
   return sizeof(struct kinodeattr_group)-sizeof(struct kinodeattr_common);
  case KATTR_FS_SIZE:
  case KATTR_FS_BUFSIZE:
  case KATTR_FS_MAXSIZE:
   return sizeof(struct kinodeattr_size)-sizeof(struct kinodeattr_common);
  case KATTR_FS_KIND:
   return sizeof(struct kinodeattr_kind)-sizeof(struct kinodeattr_common);
  default: break;
 }
 return 0;
}

kerrno_t __kinode_getattr_legacy(struct kinode const *self, kattr_t attr,
                                 void *__restrict buf, size_t bufsize, size_t *__restrict reqsize) {
 kerrno_t error; union kinodeattr attrib; size_t minsize;
 kassert_kinode(self);
 kassertmem(buf,bufsize);
 kassertobjnull(reqsize);
 if (attr == KATTR_FS_ATTRS) {
  size_t n_elem = bufsize/sizeof(union kinodeattr);
  if (reqsize) *reqsize = n_elem*sizeof(union kinodeattr);
  return kinode_getattr(self,n_elem,(union kinodeattr *)buf);
 }
 minsize = kinode_getlegacyattributesize(attr);
 if (!minsize) return KE_NOSYS;
 if (reqsize) *reqsize = minsize;
 if (bufsize < minsize) return KE_OK;
 attrib.ia_common.a_id = attr;
 error = kinode_getattr(self,1,&attrib);
 if (KE_ISOK(error)) memcpy(buf,(&attrib.ia_common.a_id)+1,minsize);
 return error;
}
kerrno_t __kinode_setattr_legacy(struct kinode *self, kattr_t attr,
                                 void const *__restrict buf, size_t bufsize) {
 union kinodeattr attrib; size_t minsize;
 kassert_kinode(self);
 kassertmem(buf,bufsize);
 if (attr == KATTR_FS_ATTRS) {
  size_t n_elem = bufsize/sizeof(union kinodeattr);
  return kinode_setattr(self,n_elem,(union kinodeattr const *)buf);
 }
 minsize = kinode_getlegacyattributesize(attr);
 if (!minsize) return KE_NOSYS;
 if (bufsize < minsize) return KE_BUFSIZ;
 attrib.ia_common.a_id = attr;
 memcpy((&attrib.ia_common.a_id)+1,buf,minsize);
 return kinode_setattr(self,1,&attrib);
}
kerrno_t kinode_getattr_legacy(struct kinode const *self, kattr_t attr,
                               void *__restrict buf, size_t bufsize,
                               size_t *__restrict reqsize) {
 kerrno_t error;
 kassert_kinode(self);
 kassertmem(buf,bufsize);
 kassertobjnull(reqsize);
 if (kinode_issuperblock(self)) {
  struct ksuperblocktype *tp = (struct ksuperblocktype *)self->i_type;
  error = tp->st_getattr
   ? (*tp->st_getattr)(__kinode_superblock(self),attr,buf,bufsize,reqsize)
   : ksuperblock_generic_getattr(__kinode_superblock(self),attr,buf,bufsize,reqsize);
  if (error != KE_NOSYS) return error;
 }
 return __kinode_getattr_legacy(self,attr,buf,bufsize,reqsize);
}
kerrno_t kinode_setattr_legacy(struct kinode *self, kattr_t attr,
                               void const *__restrict buf, size_t bufsize) {
 kerrno_t error;
 kassert_kinode(self);
 kassertmem(buf,bufsize);
 if (kinode_issuperblock(self)) {
  struct ksuperblocktype *tp = (struct ksuperblocktype *)self->i_type;
  error = tp->st_setattr
   ? (*tp->st_setattr)(__kinode_superblock(self),attr,buf,bufsize)
   : ksuperblock_generic_setattr(__kinode_superblock(self),attr,buf,bufsize);
   if (error != KE_NOSYS) return error;
 }
 return __kinode_setattr_legacy(self,attr,buf,bufsize);
}

kerrno_t kinode_walk(struct kinode *self,
                     struct kdirentname const *name,
                     __ref struct kinode **result) {
 kerrno_t error;
 kerrno_t(*callback)(struct kinode *,struct kdirentname const *,struct kinode **);
 kassert_kinode(self);
 kassertobj(self);
 kassertobj(result);
 kassertobj(self->i_type);
 callback = self->i_type->it_walk;
 if (!callback) {
  // Must differentiate between 'KE_NODIR' and 'KE_NOSYS'
  return S_ISDIR(self->i_kind) ? KE_NOSYS : KE_NODIR;
 }
 error = kcloselock_beginop(&self->i_closelock);
 if (KE_ISOK(error)) {
#ifdef __DEBUG__
  *result = NULL;
#endif
  error = (*callback)(self,name,result);
  assert(KE_ISERR(error) || *result != NULL);
  kcloselock_endop(&self->i_closelock);
 }
 return error;
}

kerrno_t kinode_unlink(struct kinode *self, struct kdirentname const *name, struct kinode *__restrict node) {
 kerrno_t(*callback)(struct kinode *,struct kdirentname const *,struct kinode *);
 kerrno_t error; kassert_kinode(self); kassert_kinode(node); kassertobj(name);
 if __unlikely((callback = self->i_type->it_unlink) == NULL)
  return S_ISDIR(self->i_kind) ? KE_NOSYS : KE_NODIR;
 if __unlikely(S_ISDIR(node->i_kind)) return KE_NODIR;
 else if (S_ISLNK(node->i_kind) || S_ISREG(node->i_kind)) {
  if __unlikely(KE_ISERR(error = kcloselock_beginop(&self->i_closelock))) return error;
  if __unlikely(KE_ISOK(error = kcloselock_close(&node->i_closelock))) {
   error = (*callback)(self,name,node);
   if __unlikely(!KE_ISOK(error)) kcloselock_reset(&node->i_closelock);
  }
  kcloselock_endop(&self->i_closelock);
 } else error = KE_OK; // Special file (not handled by the file system)
 return error;
}
kerrno_t kinode_rmdir(struct kinode *self, struct kdirentname const *name, struct kinode *__restrict node) {
 kerrno_t(*callback)(struct kinode *,struct kdirentname const *,struct kinode *);
 kerrno_t error; kassert_kinode(self); kassert_kinode(node); kassertobj(name);
 if __unlikely((callback = self->i_type->it_rmdir) == NULL)
  return S_ISDIR(self->i_kind) ? KE_NOSYS : KE_NODIR;
 if __unlikely(!S_ISDIR(node->i_kind)) return KE_NODIR;
 if __unlikely(KE_ISERR(error = kcloselock_beginop(&self->i_closelock))) return error;
 if __unlikely(KE_ISOK(error = kcloselock_close(&node->i_closelock))) {
  error = (*callback)(self,name,node);
  if __unlikely(!KE_ISOK(error)) kcloselock_reset(&node->i_closelock);
 }
 kcloselock_endop(&self->i_closelock);
 return error;
}
kerrno_t kinode_remove(struct kinode *self, struct kdirentname const *name, struct kinode *__restrict node) {
 kerrno_t(*callback)(struct kinode *,struct kdirentname const *,struct kinode *);
 kerrno_t error; kassert_kinode(self); kassert_kinode(node); kassertobj(name);
 if (S_ISDIR(node->i_kind)) callback = self->i_type->it_rmdir;
 else if (S_ISLNK(node->i_kind) || S_ISREG(node->i_kind)) {
  callback = self->i_type->it_unlink;
 } else return KE_OK; // Special file (not handled by the file system)
 if __unlikely(!callback) return S_ISDIR(self->i_kind) ? KE_NOSYS : KE_NODIR;
 if __unlikely(KE_ISERR(error = kcloselock_beginop(&self->i_closelock))) return error;
 if __unlikely(KE_ISOK(error = kcloselock_close(&node->i_closelock))) {
  error = (*callback)(self,name,node);
  if __unlikely(!KE_ISOK(error)) kcloselock_reset(&node->i_closelock);
 }
 kcloselock_endop(&self->i_closelock);
 return error;
}
kerrno_t kinode_mkdir(struct kinode *self, struct kdirentname const *name,
                      size_t ac, union kinodeattr const *av, __ref struct kinode **resnode) {
 kerrno_t(*callback)(struct kinode *,struct kdirentname const *,size_t,union kinodeattr const *,struct kinode **);
 kerrno_t error;
 kassert_kinode(self);
 kassertobj(name);
 kassertobj(resnode);
 if __unlikely((callback = self->i_type->it_mkdir) == NULL)
  return S_ISDIR(self->i_kind) ? KE_NOSYS : KE_NODIR;
 if __unlikely(KE_ISERR(error = kcloselock_beginop(&self->i_closelock))) return error;
 error = (*callback)(self,name,ac,av,resnode);
 kcloselock_endop(&self->i_closelock);
 if __unlikely(error == KE_NOSYS && !S_ISDIR(self->i_kind)) error = KE_NODIR;
 return error;
}
kerrno_t kinode_mkreg(struct kinode *self, struct kdirentname const *name,
                      size_t ac, union kinodeattr const *av, __ref struct kinode **resnode) {
 kerrno_t(*callback)(struct kinode *,struct kdirentname const *,size_t,union kinodeattr const *,struct kinode **);
 kerrno_t error;
 kassert_kinode(self);
 kassertobj(name);
 kassertobj(resnode);
 kassertmem(av,ac*sizeof(union kinodeattr));
 if __unlikely((callback = self->i_type->it_mkreg) == NULL)
  return S_ISDIR(self->i_kind) ? KE_NOSYS : KE_NODIR;
 if __unlikely(KE_ISERR(error = kcloselock_beginop(&self->i_closelock))) return error;
 error = (*callback)(self,name,ac,av,resnode);
 kcloselock_endop(&self->i_closelock);
 if __unlikely(error == KE_NOSYS && !S_ISDIR(self->i_kind)) error = KE_NODIR;
 return error;
}
kerrno_t kinode_mklnk(struct kinode *self, struct kdirentname const *name,
                      size_t ac, union kinodeattr const *av, 
                      struct kdirentname const *target, __ref struct kinode **resnode) {
 kerrno_t(*callback)(struct kinode *,struct kdirentname const *,size_t,
                     union kinodeattr const *,struct kdirentname const *,
                     __ref struct kinode **);
 kerrno_t error;
 kassert_kinode(self);
 kassertobj(name);
 kassertobj(target);
 kassertobj(resnode);
 if __unlikely((callback = self->i_type->it_mklnk) == NULL)
  return S_ISDIR(self->i_kind) ? KE_NOSYS : KE_NODIR;
 if __unlikely(KE_ISERR(error = kcloselock_beginop(&self->i_closelock))) return error;
 error = (*callback)(self,name,ac,av,target,resnode);
 kcloselock_endop(&self->i_closelock);
 if __unlikely(error == KE_NOSYS && !S_ISDIR(self->i_kind)) error = KE_NODIR;
 return error;
}
kerrno_t kinode_insnod(struct kinode *self, struct kdirentname const *name, struct kinode *node) {
 kerrno_t(*callback)(struct kinode *,struct kdirentname const *,struct kinode *);
 kerrno_t error;
 kassert_kinode(self);
 kassertobj(name);
 if __unlikely((callback = self->i_type->it_insnod) == NULL)
  return S_ISDIR(self->i_kind) ? KE_NOSYS : KE_NODIR;
 if __unlikely(KE_ISERR(error = kcloselock_beginop(&self->i_closelock))) return error;
 error = (*callback)(self,name,node);
 kcloselock_endop(&self->i_closelock);
 if __unlikely(error == KE_NOSYS && !S_ISDIR(self->i_kind)) error = KE_NODIR;
 return error;
}
void kinode_delnod(struct kinode *self, struct kdirentname const *name, struct kinode *node) {
 void(*callback)(struct kinode *,struct kdirentname const *,struct kinode *);
 kassert_kinode(self);
 kassertobj(name);
 if __unlikely((callback = self->i_type->it_delnod) == NULL) return;
 if __unlikely(KE_ISERR(kcloselock_beginop(&self->i_closelock))) return;
 (*callback)(self,name,node);
 kcloselock_endop(&self->i_closelock);
}
kerrno_t kinode_readlink(struct kinode *self, struct kdirentname *target) {
 kerrno_t(*callback)(struct kinode *,struct kdirentname *);
 kerrno_t error; kassert_kinode(self); kassertobj(target);
 callback = self->i_type->it_readlink;
 if __unlikely(!callback) return S_ISLNK(self->i_kind) ? KE_NOLNK : KE_INVAL;
 if __unlikely(KE_ISERR(error = kcloselock_beginop(&self->i_closelock))) return error;
 error = (*callback)(self,target);
 kcloselock_endop(&self->i_closelock);
 if __unlikely(error == KE_NOLNK && !S_ISLNK(self->i_kind)) error = KE_INVAL;
 return error;
}
kerrno_t kinode_mkhardlink(struct kinode *self,
                           struct kdirentname const *name,
                           struct kinode *target) {
 kerrno_t(*callback)(struct kinode *,struct kdirentname const *,struct kinode *);
 kerrno_t error; kassert_kinode(self); kassertobj(name);
 kassertobj(self->i_type); kassert_kinode(target);
 callback = self->i_type->it_hrdlnk;
 if __unlikely(!callback) return S_ISDIR(self->i_kind) ? KE_NOSYS : KE_NODIR;
 if __unlikely(KE_ISERR(error = kcloselock_beginop(&self->i_closelock))) return error;
 error = (*callback)(self,name,target);
 kcloselock_endop(&self->i_closelock);
 if __unlikely(error == KE_NOSYS && !S_ISDIR(self->i_kind)) error = KE_NODIR;
 return error;
}




//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// --- KDIRENT
kerrno_t kdirentname_initcopy(struct kdirentname *self,
                              struct kdirentname const *right) {
 kassertobj(self);
 kassertobj(right);
 self->dn_hash = right->dn_hash;
 self->dn_size = right->dn_size;
#ifdef __DEBUG__
 // Create zero-terminated strings in debug mode
 self->dn_name = (char *)malloc((right->dn_size+1)*sizeof(char));
 if (self->dn_name) {
  memcpy(self->dn_name,right->dn_name,right->dn_size);
  self->dn_name[right->dn_size] = '\0';
 }
#else
 self->dn_name = (char *)memdup(right->dn_name,
                                right->dn_size*
                                sizeof(char));
#endif
 return self->dn_name ? KE_OK : KE_NOMEM;
}
size_t kdirentname_genhash(char const *data, size_t bytes) {
 size_t hash = 0;
 size_t const *iter,*end;
 end = (iter = (size_t const *)data)+(bytes/sizeof(size_t));
 while (iter != end) hash += *iter++,hash *= 9;
 switch (bytes % sizeof(size_t)) {
#if __SIZEOF_POINTER__ > 4
  case 7:  hash += (size_t)((byte_t *)data)[6] << 48;
  case 6:  hash += (size_t)((byte_t *)data)[5] << 40;
  case 5:  hash += (size_t)((byte_t *)data)[4] << 32;
  case 4:  hash += (size_t)((byte_t *)data)[3] << 24;
#endif
  case 3:  hash += (size_t)((byte_t *)data)[2] << 16;
  case 2:  hash += (size_t)((byte_t *)data)[1] << 8;
  case 1:  hash += (size_t)((byte_t *)data)[0];
  default: break;
 }
 // NOTE: The hash '(size_t)-1' is reserved for internal use.
 return __likely(hash != (size_t)-1) ? hash : (size_t)-2;
}
kerrno_t kdirentcache_insert(struct kdirentcache *self, struct kdirent *child,
                             struct kdirent **existing_child) {
 size_t hashindex; struct kdirent **newvec;
 struct kdirent **iter,**end,*elem;
 struct kdirentcachevec *childvec;
 struct kdirentname *childname;
 kassertobj(self);
 kassertobjnull(existing_child);
 kassert_kdirent(child);
 hashindex = child->d_name.dn_hash % KDIRENTCACHE_SIZE;
 childvec = &self->dc_cache[hashindex];
 end = (iter = childvec->d_vecv)+childvec->d_vecc;
 childname = &child->d_name;
 while (iter != end) {
  elem = *iter++;
  if (elem->d_name.dn_hash == childname->dn_hash &&
      elem->d_name.dn_size == childname->dn_size &&
      memcmp(elem->d_name.dn_name,childname->dn_name,childname->dn_size) == 0) {
   if (existing_child) *existing_child = elem; // Already exists
   return KE_EXISTS;
  }
 }
 assert((childvec->d_vecc != 0) == (childvec->d_vecv != NULL));
 kassertmem(childvec->d_vecv,childvec->d_vecc*sizeof(struct kdirent *));
 newvec = (struct kdirent **)realloc(childvec->d_vecv,
                                    (childvec->d_vecc+1)*
                                     sizeof(struct kdirent *));
 if __unlikely(!newvec) return KE_NOMEM;
 childvec->d_vecv = newvec;
 newvec[childvec->d_vecc++] = child;
 return KE_OK;
}
kerrno_t kdirentcache_remove(struct kdirentcache *self, struct kdirent *child) {
 size_t hashindex; struct kdirent **iter,**end;
 struct kdirentcachevec *childvec;
 kassertobj(self);
 kassert_object(child,KOBJECT_MAGIC_DIRENT);
 hashindex = child->d_name.dn_hash % KDIRENTCACHE_SIZE;
 childvec = &self->dc_cache[hashindex];
 end = (iter = childvec->d_vecv)+childvec->d_vecc;
 while (iter != end) {
  if (*iter == child) {
   memmove(iter,iter+1,(size_t)((end-iter)-1)*
           sizeof(struct kdirent *));
   --childvec->d_vecc;
   if (!childvec->d_vecc) {
    free(childvec->d_vecv);
    childvec->d_vecv = NULL;
   } else {
    iter = (struct kdirent **)realloc(childvec->d_vecv,
                                      childvec->d_vecc*
                                      sizeof(struct kdirent *));
    if __likely(iter) childvec->d_vecv = iter;
   }
   return KE_OK;
  }
  ++iter;
 }
 return KE_NOENT;
}
struct kdirent *kdirentcache_lookup(
 struct kdirentcache *self, struct kdirentname const *name) {
 size_t hashindex; struct kdirent **iter,**end,*elem;
 struct kdirentcachevec *childvec;
 kassertobj(self);
 kassertobj(name);
 hashindex = name->dn_hash % KDIRENTCACHE_SIZE;
 childvec = &self->dc_cache[hashindex];
 end = (iter = childvec->d_vecv)+childvec->d_vecc;
 while (iter != end) {
  elem = *iter++;
  kassert_object(elem,KOBJECT_MAGIC_DIRENT);
  if (elem->d_name.dn_hash == name->dn_hash &&
      elem->d_name.dn_size == name->dn_size &&
      memcmp(elem->d_name.dn_name,name->dn_name,name->dn_size*sizeof(char)) == 0) {
   // Found it!
   return elem;
  }
 }
 return NULL;
}




extern void kdirent_destroy(struct kdirent *self);
void kdirent_destroy(struct kdirent *self) {
 struct kdirent *parent;
 kassert_object(self,KOBJECT_MAGIC_DIRENT);
 if __likely((parent = self->d_parent) != NULL) {
  kdirent_lock(parent,KDIRENT_LOCK_CACHE);
  // Remove ourselves from the cache of our parent
  __evalexpr(kdirentcache_remove(&parent->d_cache,self));
  kdirent_unlock(parent,KDIRENT_LOCK_CACHE);
  if (self->d_inode && (self->d_flags&(KDIRENT_FLAG_VIRT|KDIRENT_FLAG_INSD)) ==
                                      (KDIRENT_FLAG_VIRT|KDIRENT_FLAG_INSD)) {
   __ref struct kinode *parent_inode;
   if ((parent_inode = kdirent_getnode(parent)) != NULL) {
    kinode_delnod(parent_inode,&self->d_name,self->d_inode);
    kinode_decref(parent_inode);
   }
  }
  kdirent_decref(parent);
 }
 if __likely(self->d_inode) kinode_decref(self->d_inode);
 free(self->d_name.dn_name);
#ifdef __DEBUG__
 struct kdirentcachevec *iter,*end;
 end = (iter = self->d_cache.dc_cache)+KDIRENTCACHE_SIZE;
 for (; iter != end; ++iter) assertf(iter->d_vecc == 0,"dirent still has cached children");
#endif
 free(self);
}

__ref struct kdirent *__kdirent_alloc(struct kdirent *parent, struct kdirentname const *name) {
 struct kdirent *result;
 kassert_kdirent(parent);
 kassertobj(name);
 result = omalloc(struct kdirent);
 if __likely(result) {
  kobject_init(result,KOBJECT_MAGIC_DIRENT);
  result->d_refcnt = 1;
  result->d_locks = 0;
  result->d_flags = KDIRENT_FLAG_NONE;
  result->d_padding = 0;
  if (KE_ISERR(kdirentname_initcopy(&result->d_name,name))) {
err_freer: free(result); return NULL;
  }
  if __unlikely(KE_ISERR(kdirent_incref(parent))) {
   free(result->d_name.dn_name);
   goto err_freer;
  }
  result->d_parent = parent; // Inherit reference
  kdirentcache_init(&result->d_cache);
  result->d_inode  = NULL;
 }
 return result;
}

kerrno_t kdirent_insnod(struct kdirent *self, struct kdirentname const *name,
                        struct kinode *__restrict node, __ref struct kdirent **resent) {
 struct kdirent *used_resent;
 struct kinode *selfnode;
 kerrno_t error;
 kassert_kdirent(self);
 kassertobj(name);
 kassert_kinode(node);
 kassertobj(resent);
 /* Do some checking to make sure that the given
  * 'self' dirent is a directory and not destroyed. */
 if __unlikely((selfnode = kdirent_getnode(self)) == NULL) return KE_DESTROYED;
 if __unlikely(!S_ISDIR(selfnode->i_kind)) { error = KE_NODIR; goto end_selfnode; }
 if __unlikely(KE_ISERR(error = kinode_incref(node))) goto end_selfnode;
 if __unlikely((used_resent = __kdirent_alloc(self,name)) == NULL) { error = KE_NOMEM; goto err_node; }
 used_resent->d_flags = KDIRENT_FLAG_VIRT;
 /* Inform the underlying filesystem about the addition of a virtual INode. */
 error = kinode_insnod(selfnode,&used_resent->d_name,node);
 if __unlikely(KE_ISERR(error)) {
  if __unlikely(error != KE_NOSYS) goto err_node;
 } else used_resent->d_flags |= KDIRENT_FLAG_INSD;
 used_resent->d_inode = node; /*< Inherit reference */
 kdirent_lock(self,KDIRENT_LOCK_CACHE);
 error = kdirentcache_insert(&self->d_cache,used_resent,NULL);
 kdirent_unlock(self,KDIRENT_LOCK_CACHE);
 if __likely(KE_ISOK(error)) *resent = used_resent;
 else kdirent_decref(used_resent);
end_selfnode:
 kinode_decref(selfnode);
 return error;
err_node:
 kinode_decref(node);
 goto end_selfnode;
}

kerrno_t kdirent_mount(struct kdirent *self, struct kdirentname const *name,
                       struct ksuperblock *sblock, __ref /*opt*/struct kdirent **resent) {
 struct kdirent *used_resent; kerrno_t error;
 kassert_kdirent(self);
 kassert_ksuperblock(sblock);
 kassertobj(name);
 kassertobjnull(resent);
 // Insert the root node and create the associated directory entry.
 // After this call returns, the given superblock  is technically
 // already mounted.
 // >> The remaining code is there for tracking of mounted superblocks,
 //    as well as to ensure that superblocks remain mounted, even
 //    when all (obvious) references to them are lock (such as would
 //    be the case if 'resent' is NULL, which is even something that's allowed).
 error = kdirent_insnod(self,name,ksuperblock_root(sblock),&used_resent);
 if __unlikely(KE_ISERR(error)) return error;
 if (resent) {
  *resent = used_resent;
  error = _ksuperblock_addmnt(sblock,used_resent);
 } else {
  error = _ksuperblock_addmnt_inherited(sblock,used_resent);
 }
 if __unlikely(KE_ISERR(error)) {
  /* Something went wrong.
   * >> Now we must unlink the newly created dirent, before closing its node. */
  kdirent_unlinknode(used_resent);
 }
 return error;
}


__ref struct kinode *kdirent_getnode(struct kdirent *self) {
 struct kinode *result;
 kassert_kdirent(self);
 kdirent_lock(self,KDIRENT_LOCK_NODE);
 if __likely((result = self->d_inode) != NULL) {
  kassert_kinode(result);
  if __unlikely(KE_ISERR(kinode_incref(result))) result = NULL;
 }
 kdirent_unlock(self,KDIRENT_LOCK_NODE);
 return result;
}

__ref struct kdirent *__kdirent_newinherited(__ref struct kdirent *parent,
                                             struct kdirentname const *name,
                                             __ref struct kinode *__restrict inode) {
 struct kdirent *result;
 kassert_kdirent(parent);
 kassert_kinode(inode);
 kassertobj(name);
 result = omalloc(struct kdirent);
 if __likely(result) {
  kobject_init(result,KOBJECT_MAGIC_DIRENT);
  result->d_refcnt = 1;
  result->d_locks = 0;
  result->d_flags = KDIRENT_FLAG_NONE;
  result->d_padding = 0;
  result->d_name = *name; // Inherit data
  result->d_parent = parent; // Inherit reference
  kdirentcache_init(&result->d_cache);
  result->d_inode = inode; // Inherit reference
 }
 return result;
}

__ref struct kdirent *
kdirent_new(struct kdirent *parent,
            struct kdirentname const *name,
            struct kinode *__restrict inode) {
 struct kdirentname namecopy;
 struct kdirent *result;
 kassert_kdirent(parent);
 kassertobj(name);
 kassert_kinode(inode);
 if __unlikely(KE_ISERR(kdirent_incref(parent))) return NULL;
 if __unlikely(KE_ISERR(kinode_incref(inode))) goto err_1;
 if __unlikely(KE_ISERR(kdirentname_initcopy(&namecopy,name))) goto err_2;
 result = __kdirent_newinherited(parent,&namecopy,inode);
 if __unlikely(!result) goto err_3;
 return result;
err_3: kdirentname_quit(&namecopy);
err_2: kinode_decref(inode);
err_1: kdirent_decref(parent);
 return NULL;
}


void kdirent_unlinknode(struct kdirent *self) {
 struct kinode *oldnode; struct kdirent *parent;
 kassert_kdirent(self);
 kassert_kdirent(self->d_parent);
 kdirent_lock(self,KDIRENT_LOCK_NODE);
 oldnode = self->d_inode,self->d_inode = NULL;
 kdirent_unlock(self,KDIRENT_LOCK_NODE);
 parent = self->d_parent;
 kassert_kdirent(parent); // This function may not be used for the ROOT
 kdirent_lock(parent,KDIRENT_LOCK_CACHE);
 __evalexpr(kdirentcache_remove(&parent->d_cache,self));
 kdirent_unlock(parent,KDIRENT_LOCK_CACHE);
 if (oldnode) kinode_decref(oldnode);
}

#ifndef __INTELLISENSE__
#define WALKENV
#include "fs-dirent-walk.inl"
#define WITHRESNODE
#include "fs-dirent-walk.inl"
#include "fs-dirent-walk.inl"

#define DO_RMDIR
#include "fs-dirent-rmobj.inl"
#define DO_UNLINK
#include "fs-dirent-rmobj.inl"
#define DO_REMOVE
#include "fs-dirent-rmobj.inl"

#define MKLNK
#include "fs-dirent-mkobj.inl"
#define MKDIR
#include "fs-dirent-mkobj.inl"
#define MKREG
#include "fs-dirent-mkobj.inl"
#endif



kerrno_t kdirent_getfilename(struct kdirent const *self, char *__restrict buf,
                             size_t bufsize, size_t *__restrict reqsize) {
 size_t copysize;
 if (reqsize) *reqsize = (self->d_name.dn_size+1)*sizeof(char);
 copysize = self->d_name.dn_size;
 if (bufsize < copysize) copysize = bufsize;
 memcpy(buf,self->d_name.dn_name,copysize);
 if (bufsize > copysize) buf[copysize] = '\0';
 return KE_OK;
}
kerrno_t kdirent_getpathname(struct kdirent const *self, struct kdirent *__restrict root,
                             char *__restrict buf, size_t bufsize, size_t *__restrict reqsize) {
 char *iter = buf,*end = buf+bufsize; size_t temp; kerrno_t error;
 kassert_kdirent(root);
 if (self->d_parent && self->d_parent != root) {
  error = kdirent_getpathname(self->d_parent,root,iter,bufsize,&temp);
  if __unlikely(KE_ISERR(error)) return error;
  iter += (temp-1);
 }
 if (iter < end) { *iter = KFS_SEP; } ++iter;
 if (iter < end) {
  memcpy(iter,self->d_name.dn_name,
         min((size_t)(end-iter),self->d_name.dn_size));
 }
 iter += self->d_name.dn_size;
 if (iter < end) { *iter = '\0'; } ++iter;
 if (reqsize) *reqsize = (size_t)(iter-buf);
 return KE_OK;
}

kerrno_t kdirent_walklast(struct kfspathenv const *env, __ref struct kdirent **newroot,
                          struct kdirentname *lastpart, char const *path, size_t pathmax) {
 struct kdirentname part; kerrno_t error;
 char const *iter,*end; struct kfspathenv newenv;
 kassertobj(env);
 kassert_kdirent(env->env_cwd);
 kassert_kdirent(env->env_root);
 kassertobj(newroot);
 kassertobj(path);
 assert(__evalexpr(strnlen(path,pathmax)) || 1);
 if __unlikely(!pathmax) {
  lastpart->dn_hash = 0;
  lastpart->dn_size = 0;
  lastpart->dn_name = NULL;
  return kdirent_incref(*newroot = env->env_cwd);
 }
 
 if (KFS_ISSEP(*path)) {
  do ++path,--pathmax; // Re-start at root
  while (pathmax && KFS_ISSEP(*path));
  kfspathenv_initfrom(&newenv,env,env->env_root,env->env_root);
  return kdirent_walklast(&newenv,newroot,lastpart,path,pathmax);
 }
 end = (iter = path)+pathmax;
 while (iter != end && isspace(*iter)) ++iter;
 part.dn_name = (char *)iter;
 while (iter != end && *iter && !KFS_ISSEP(*iter)) ++iter;
 part.dn_size = (size_t)(iter-part.dn_name);
 while (part.dn_size && isspace(part.dn_name[part.dn_size-1])) --part.dn_size;
 switch (part.dn_size) {
  case 2: if (part.dn_name[1] != '.') break;
  case 1: if (part.dn_name[0] != '.') break;
   // Special directory '.' or '..'
   lastpart->dn_hash = 0;
   lastpart->dn_size = 0;
   lastpart->dn_name = NULL;
   if (part.dn_size == 1 || env->env_root == env->env_cwd) {
    // Reference to own directory / attempt to surpass
    // the selected filesystem root is denied.
    return kdirent_incref(*newroot = env->env_cwd);
   }
   // If this fails, 'env_root' can't be reached from 'env_cwd'
   kassert_kdirent(env->env_cwd->d_parent);
   // Permission to visible parent directory is granted
   return kdirent_incref(*newroot = env->env_cwd->d_parent);
  default: break;
 }

 kdirentname_refreshhash(&part);
 if (!*iter || iter == end) {
  // Single-part pathname
  error = kdirent_incref(*newroot = env->env_cwd);
 } else {
  struct kdirent *used_root,*newusedroot;
  error = kdirent_walkenv(env,&part,&used_root);
  if __unlikely(KE_ISERR(error)) return error;
  kassert_kdirent(used_root);
  for (;;) {
   while (iter != end && KFS_ISSEP(*iter)) ++iter;
   while (iter != end && isspace(*iter)) ++iter;
   part.dn_name = (char *)iter;
   while (iter != end && *iter && !KFS_ISSEP(*iter)) ++iter;
   part.dn_size = (size_t)(iter-part.dn_name);
   while (part.dn_size && isspace(part.dn_name[part.dn_size-1])) --part.dn_size;
   kdirentname_refreshhash(&part);
   if (!*iter || iter == end) break;
   kfspathenv_initfrom(&newenv,env,used_root,env->env_root);
   error = kdirent_walkenv(&newenv,&part,&newusedroot);
   if __unlikely(KE_ISERR(error)) return error;
   kassert_kdirent(newusedroot);
   kdirent_decref(used_root);
   used_root = newusedroot;
  }
  *newroot = used_root; // Inherit reference
 }
 *lastpart = part;
 return error;
}
kerrno_t kdirent_walkall(struct kfspathenv const *env, __ref struct kdirent **finish,
                         char const *path, size_t pathmax) {
 struct kdirentname last; kerrno_t error;
 struct kdirent *finish_root;
 kassertobj(env);
 kassert_kdirent(env->env_cwd);
 kassert_kdirent(env->env_root);
 error = kdirent_walklast(env,&finish_root,&last,path,pathmax);
 if __unlikely(KE_ISERR(error)) return error;
 if (!last.dn_size) {
  *finish = finish_root; // Inherit reference
  return error;
 }
 error = kdirent_walk(finish_root,&last,finish);
 kdirent_decref(finish_root);
 return error;
}



kerrno_t kdirent_mkdirat(struct kfspathenv const *env, char const *path,
                         size_t pathmax, size_t ac, union kinodeattr const *av,
                         __ref /*opt*/struct kdirent **resent,
                         __ref /*opt*/struct kinode **resnode) {
 struct kdirentname last; kerrno_t error;
 struct kdirent *objparent;
 kassertobj(env);
 kassert_kdirent(env->env_cwd);
 kassert_kdirent(env->env_root);
 assert(__evalexpr(strnlen(path,pathmax)) || 1);
 error = kdirent_walklast(env,&objparent,&last,path,pathmax);
 if __unlikely(KE_ISERR(error)) return error;
 if __unlikely(!last.dn_size) error = KE_EXISTS;
 else error = kdirent_mkdir(objparent,&last,ac,av,resent,resnode);
 kdirent_decref(objparent);
 return error;
}
kerrno_t kdirent_mkregat(struct kfspathenv const *env, char const *path,
                         size_t pathmax, size_t ac, union kinodeattr const *av,
                         __ref /*opt*/struct kdirent **resent,
                         __ref /*opt*/struct kinode **resnode) {
 struct kdirentname last; kerrno_t error;
 struct kdirent *objparent;
 kassertobj(env);
 kassertmem(av,ac*sizeof(union kinodeattr));
 kassert_kdirent(env->env_cwd);
 kassert_kdirent(env->env_root);
 assert(__evalexpr(strnlen(path,pathmax)) || 1);
 error = kdirent_walklast(env,&objparent,&last,path,pathmax);
 if __unlikely(KE_ISERR(error)) return error;
 if __unlikely(!last.dn_size) error = KE_EXISTS;
 else error = kdirent_mkreg(objparent,&last,ac,av,resent,resnode);
 kdirent_decref(objparent);
 return error;
}
kerrno_t kdirent_mklnkat(struct kfspathenv const *env, char const *path, size_t pathmax,
                         size_t ac, union kinodeattr const *av, struct kdirentname const *target,
                         __ref /*opt*/struct kdirent **resent, __ref /*opt*/struct kinode **resnode) {
 struct kdirentname last; kerrno_t error;
 struct kdirent *objparent;
 kassertobj(env);
 kassert_kdirent(env->env_cwd);
 kassert_kdirent(env->env_root);
 kassertobj(target);
 assert(__evalexpr(strnlen(path,pathmax)) || 1);
 error = kdirent_walklast(env,&objparent,&last,path,pathmax);
 if __unlikely(KE_ISERR(error)) return error;
 if __unlikely(!last.dn_size) error = KE_EXISTS;
 else error = kdirent_mklnk(objparent,&last,ac,av,target,resent,resnode);
 kdirent_decref(objparent);
 return error;
}
kerrno_t kdirent_rmdirat(struct kfspathenv const *env, char const *path, size_t pathmax) {
 kerrno_t error; struct kdirent *objent;
 kassertobj(env);
 kassert_kdirent(env->env_cwd);
 kassert_kdirent(env->env_root);
 assert(__evalexpr(strnlen(path,pathmax)) || 1);
 error = kdirent_walkall(env,&objent,path,pathmax);
 if __unlikely(KE_ISERR(error)) return error;
 error = kdirent_rmdir(objent);
 kdirent_decref(objent);
 return error;
}
kerrno_t kdirent_unlinkat(struct kfspathenv const *env, char const *path, size_t pathmax) {
 kerrno_t error; struct kdirent *objent;
 kassertobj(env);
 kassert_kdirent(env->env_cwd);
 kassert_kdirent(env->env_root);
 assert(__evalexpr(strnlen(path,pathmax)) || 1);
 error = kdirent_walkall(env,&objent,path,pathmax);
 if __unlikely(KE_ISERR(error)) return error;
 error = kdirent_unlink(objent);
 kdirent_decref(objent);
 return error;
}
kerrno_t kdirent_removeat(struct kfspathenv const *env, char const *path, size_t pathmax) {
 kerrno_t error; struct kdirent *objent;
 kassertobj(env);
 kassert_kdirent(env->env_cwd);
 kassert_kdirent(env->env_root);
 assert(__evalexpr(strnlen(path,pathmax)) || 1);
 error = kdirent_walkall(env,&objent,path,pathmax);
 if __unlikely(KE_ISERR(error)) return error;
 error = kdirent_remove(objent);
 kdirent_decref(objent);
 return error;
}
kerrno_t kdirent_insnodat(struct kfspathenv const *env, char const *path, size_t pathmax,
                          struct kinode *__restrict node, __ref struct kdirent **resent) {
 struct kdirentname last; kerrno_t error;
 struct kdirent *objparent;
 kassertobj(env);
 kassert_kdirent(env->env_cwd);
 kassert_kdirent(env->env_root);
 kassertobj(resent);
 assert(__evalexpr(strnlen(path,pathmax)) || 1);
 error = kdirent_walklast(env,&objparent,&last,path,pathmax);
 if __unlikely(KE_ISERR(error)) return error;
 if __unlikely(!last.dn_size) error = KE_EXISTS;
 else error = kdirent_insnod(objparent,&last,node,resent);
 kdirent_decref(objparent);
 return error;
}
kerrno_t kdirent_mountat(struct kfspathenv const *env, char const *path, size_t pathmax,
                         struct ksuperblock *sblock, __ref /*opt*/struct kdirent **resent) {
 struct kdirentname last; kerrno_t error;
 struct kdirent *objparent;
 kassertobj(env);
 kassert_kdirent(env->env_cwd);
 kassert_kdirent(env->env_root);
 kassertobjnull(resent);
 assert(__evalexpr(strnlen(path,pathmax)) || 1);
 error = kdirent_walklast(env,&objparent,&last,path,pathmax);
 if __unlikely(KE_ISERR(error)) return error;
 if __unlikely(!last.dn_size) error = KE_EXISTS;
 else error = kdirent_mount(objparent,&last,sblock,resent);
 kdirent_decref(objparent);
 return error;
}

kerrno_t kdirent_openat(struct kfspathenv const *env, char const *path, size_t pathmax,
                        __openmode_t mode, size_t create_ac, union kinodeattr const *create_av,
                        __ref struct kfile **__restrict result) {
 struct kdirentname last; kerrno_t error;
 struct kdirent *pathroot;
 kassertobj(env);
 kassert_kdirent(env->env_cwd);
 kassert_kdirent(env->env_root);
 assert(__evalexpr(strnlen(path,pathmax)) || 1);
 kassertobj(result);
 error = kdirent_walklast(env,&pathroot,&last,path,pathmax);
 if __unlikely(KE_ISERR(error)) return error;
 if (last.dn_size) {
  error = kdirent_openlast(env,pathroot,&last,mode,create_ac,create_av,result);
 } else if __unlikely(mode&(O_CREAT|O_EXCL)) {
  error = KE_EXISTS;
 } else {
  struct kinode *pathnode;
  pathnode = kdirent_getnode(pathroot);
  if __unlikely(!pathnode) error = KE_DESTROYED;
  else {
   error = kfile_opennode(pathroot,pathnode,result,mode);
   kinode_decref(pathnode);
  }
 }
 kdirent_decref(pathroot);
 return error;
}
kerrno_t
kdirent_openlast(struct kfspathenv const *env, struct kdirent *dir, struct kdirentname const *name,
                 __openmode_t mode, size_t create_ac, union kinodeattr const *create_av,
                 __ref struct kfile **__restrict result) {
 struct kinode *filenode;
 struct kdirent *fileent;
 kerrno_t error;
 kassert_kdirent(dir);
 kassertobj(name);
 kassertobj(result);
 kassertobj(env);
 if (mode&O_CREAT) {
  // Attempt to create missing files
  error = kdirent_mkreg(dir,name,create_ac,create_av,&fileent,&filenode);
  if (mode&O_EXCL && error == KE_EXISTS) {
   // O_CREAT|O_EXCL --> Force creation of new files; fail if already exists
   // NOTE: But since we've already retrieved both the node
   //       and its dirent, we need to clean those up.
   kdirent_decref(fileent);
   kinode_decref(filenode);
   return KE_EXISTS;
  } else if (!(mode&O_EXCL) && error == KE_NOSYS) {
   // This can happen if the filesystem is mounted as read-only.
   // >> Try again by opening existing nodes
   goto open_existing;
  }
 } else {
open_existing:
  error = kdirent_walknode(dir,name,&fileent,&filenode);
 }
 if __unlikely(KE_ISERR(error)) return error;
 if (!(mode&_O_SYMLINK) && S_ISLNK(filenode->i_kind)) {
  struct kfspathenv target_env;
  struct kdirentname link_name;
  struct kdirent *target_ent;
  /* Dereference last symbolic link. */
  assert(env->env_lnk <= LINK_MAX);
  if (env->env_lnk == LINK_MAX) { error = KE_LOOP; goto end; }
  error = kinode_readlink(filenode,&link_name);
  if __unlikely(KE_ISERR(error)) goto end;
  kfspathenv_initfrom(&target_env,env,fileent,env->env_root);
  ++target_env.env_lnk;
  error = kdirent_walkall(&target_env,&target_ent,link_name.dn_name,link_name.dn_size);
  kdirentname_quit(&link_name);
  if __unlikely(KE_ISERR(error)) goto end;
  target_env.env_cwd = target_ent;
  error = kdirent_openlast(&target_env,target_ent,name,mode,create_ac,create_av,result);
  kdirent_decref(target_ent);
  return error;
 }
 kassert_kinode(filenode);
 error = kfile_opennode(fileent,filenode,result,mode);
end:
 kinode_decref(filenode);
 kdirent_decref(fileent);
 return error;
}






//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// --- KSUPERBLOCK

__ref struct ksdev *ksuperblock_getdev(struct ksuperblock const *self) {
#if 1
 kassert_ksuperblock(self);
 if __unlikely(!ksuperblock_type(self)->st_getdev) return NULL;
 return (*ksuperblock_type(self)->st_getdev)(self);
#else
 struct ksdev *result;
 struct kinode *root_node;
 kassert_ksuperblock(self);
 if __unlikely(!ksuperblock_type(self)->st_getdev) return NULL;
 root_node = (struct kinode *)ksuperblock_root(self);
 if (KE_ISOK(kcloselock_beginop(&root_node->i_closelock))) {
  result = (*ksuperblock_type(self)->st_getdev)(self);
  kcloselock_endop(&root_node->i_closelock);
 } else return NULL;
 return result;
#endif
}

kerrno_t ksuperblock_flush(struct ksuperblock *self) {
 kerrno_t error;
 kerrno_t(*callback)(struct ksuperblock *);
 kassert_ksuperblock(self);
 callback = ksuperblock_type(self)->st_flush;
 if (!callback) return KE_OK;
 if __unlikely(KE_ISERR(error = kcloselock_beginop(
  &self->s_root.i_closelock))) return error;
 error = (*callback)(self);
 kcloselock_endop(&self->s_root.i_closelock);
 return error;
}



void ksuperblock_generic_init(struct ksuperblock *self,
                              struct ksuperblocktype *stype) {
 kassertobj(self);
 kassertobj(stype);
 kobject_init(self,KOBJECT_MAGIC_SUPERBLOCK);
 kobject_init(&self->s_root,KOBJECT_MAGIC_INODE);
 ksupermount_init(&self->s_mount);
 __kinode_initcommon(&self->s_root);
 self->s_root.i_type = &stype->st_node;
 self->s_root.i_filetype = &kdirfile_type;
 self->s_root.i_sblock = NULL;
 self->s_root.i_kind = S_IFDIR;
 kcloselock_init(&self->s_root.i_closelock);
}


struct kmountman kfs_mountman = KMOUNTMAN_INIT;
void kmountman_unmount_unsafe(struct ksuperblock *fs) {
 kerrno_t error;
 kassert_ksuperblock(fs);
 if __unlikely(KE_ISERR(error = ksuperblock_umount(fs))) {
  if (k_sysloglevel >= KLOG_ERROR) {
   ksuperblock_syslogprefix(KLOG_ERROR,fs);
   k_dosyslogf(KLOG_ERROR,NULL,NULL,"Failed to safely unmount filesystem (%d)\n",error);
  }
  _ksuperblock_clearmnt(fs);
 }
}
void kmountman_unmountall(void) {
 struct ksuperblock *iter;
 kmountman_lock(KMOUNTMAN_LOCK_CHAIN);
 while ((iter = kfs_mountman.mm_first) != NULL) {
  if ((kfs_mountman.mm_first = iter->s_mount.sm_next) != NULL) {
   kfs_mountman.mm_first->s_mount.sm_prev = NULL;
   iter->s_mount.sm_next = NULL;
  }
  kmountman_unlock(KMOUNTMAN_LOCK_CHAIN);
  kmountman_unmount_unsafe(iter);
  kmountman_lock(KMOUNTMAN_LOCK_CHAIN);
  assertf(kfs_mountman.mm_first != iter,
          "But we just unmounted that one...");
 }
 kmountman_unlock(KMOUNTMAN_LOCK_CHAIN);
}

kerrno_t _ksuperblock_delmnt(struct ksuperblock *self,
                             struct kdirent *ent) {
 struct kdirent **iter,**end; kerrno_t error;
 kassert_ksuperblock(self);
 kassert_kdirent(ent);
 kmountman_lock(KMOUNTMAN_LOCK_CHAIN);
 end = (iter = self->s_mount.sm_mntv)+self->s_mount.sm_mntc;
 while (*iter != ent) { if (iter == end) { error = KE_NOENT; goto end; } ++iter; }
 assertf(katomic_load(ent->d_refcnt) >= 2,
         "The caller must be holding a reference, and we're also holding one.\n"
         ">> So the dirent should be holding at least 2 references.");
 memmove(iter,iter+1,(((size_t)(end-iter))-1)*sizeof(struct kdirent *));
 // Try to conserve some memory
 iter = (struct kdirent **)realloc(self->s_mount.sm_mntv,
                                   (self->s_mount.sm_mntc-1)*
                                   sizeof(struct kdirent *));
 if __likely(iter) self->s_mount.sm_mntv = iter;
 // Drop the reference to the mounting point we just removed.
 kdirent_decref(ent);
 if (!--self->s_mount.sm_mntc) {
  // The last mounting point was removed.
  // >> Now we must remove this superblock from
  //    the global list of active mounting points.
  kmountman_remove_unlocked_inheritref(self);
  // We now inherited a reference to ourselves.
  // >> Destroy it
  ksuperblock_decref(self);
 }
 error = KE_OK;
end:
 kmountman_unlock(KMOUNTMAN_LOCK_CHAIN);
 return error;
}
kerrno_t _ksuperblock_addmnt(struct ksuperblock *self,
                             struct kdirent *ent) {
 kerrno_t error;
 if __likely(KE_ISOK(error = kdirent_incref(ent))) {
  error = _ksuperblock_addmnt_inherited(self,ent);
  if __unlikely(KE_ISERR(error)) kdirent_decref(ent);
 }
 return error;
}
kerrno_t _ksuperblock_addmnt_inherited(struct ksuperblock *self,
                                       __ref struct kdirent *ent) {
 kerrno_t error; struct kdirent **mntpoints;
 kassert_ksuperblock(self);
 kassert_kdirent(ent);
 if __unlikely(KE_ISERR(error = kcloselock_beginop(&self->s_root.i_closelock))) return error;
 kmountman_lock(KMOUNTMAN_LOCK_CHAIN);
 if (!self->s_mount.sm_mntc) {
  // First mounting point got added
  // >> Must add this superblock to the list of mounted filesystems
  // NOTE: For this we also need a new reference to ourselves.
  error = ksuperblock_incref(self);
  if __unlikely(KE_ISERR(error)) goto end;
  kmountman_insert_unlocked_inheritref(self);
 }
 // Re-allocate the list of mounting points to make space for the new one
 mntpoints = (struct kdirent **)realloc(self->s_mount.sm_mntv,
                                       (self->s_mount.sm_mntc+1)*
                                        sizeof(struct kdirent *));
 if __unlikely(!mntpoints) {
  // Out-of-memory (Must also potentially remove
  //                the superblock from the mount manager)
  if (!self->s_mount.sm_mntc) {
   assertf(katomic_load(self->s_root.i_refcnt) >= 2,
           "We were the ones that added this reference...");
   ksuperblock_decref(self);
   kmountman_remove_unlocked_inheritref(self);
  }
  error = KE_NOMEM;
  goto end;
 }
 self->s_mount.sm_mntv = mntpoints;
 mntpoints[self->s_mount.sm_mntc++] = ent; // Inherit reference
 error = KE_OK;
end:
 kmountman_unlock(KMOUNTMAN_LOCK_CHAIN);
 kcloselock_endop(&self->s_root.i_closelock);
 return error;
}
void _ksuperblock_clearmnt(struct ksuperblock *self) {
 struct kdirent **mntv,**iter,**end; size_t mntc;
 kassert_object(self,KOBJECT_MAGIC_SUPERBLOCK);
 kmountman_lock(KMOUNTMAN_LOCK_CHAIN);
 if (!self->s_mount.sm_mntc) { kmountman_unlock(KMOUNTMAN_LOCK_CHAIN); return; }
 mntc = self->s_mount.sm_mntc,self->s_mount.sm_mntc = 0;
 mntv = self->s_mount.sm_mntv,self->s_mount.sm_mntv = NULL;
 kmountman_remove_unlocked_inheritref(self);
 kmountman_unlock(KMOUNTMAN_LOCK_CHAIN);
 end = (iter = mntv)+mntc;
 while (iter != end) kdirent_decref(*iter++);
 free(mntv);
 ksuperblock_decref(self);
}


/* Initialize a given path environment from user-space. */
__crit kerrno_t kfspathenv_inituser(struct kfspathenv *__restrict self) {
 kerrno_t error;
 struct kproc *caller = kproc_self();
 struct kfdentry entry;
 KTASK_CRIT_MARK
 kassertobj(self);
 error = kproc_getfd(caller,KFD_CWD,&entry);
 if __unlikely(KE_ISERR(error)) return error;
 if (entry.fd_type == KFDTYPE_FILE) {
  self->env_cwd = kfile_getdirent(entry.fd_file);
  kfile_decref(entry.fd_file);
  if __unlikely(!self->env_cwd) goto err_nofile;
 } else if (entry.fd_type == KFDTYPE_DIRENT) {
  self->env_cwd = entry.fd_dirent;
 } else goto err_entry;
 error = kproc_getfd(caller,KFD_ROOT,&entry);
 if __unlikely(KE_ISERR(error)) return error;
 if (entry.fd_type == KFDTYPE_FILE) {
  self->env_root = kfile_getdirent(entry.fd_file);
  kfile_decref(entry.fd_file);
  if __unlikely(!self->env_root) goto err_nofile2;
 } else if (entry.fd_type == KFDTYPE_DIRENT) {
  self->env_root = entry.fd_dirent;
 } else goto err_entry2;
 self->env_flags = 0;
 self->env_uid = kproc_uid(caller);
 self->env_gid = kproc_gid(caller);
 self->env_lnk = 0;
end: return error;
err_nofile2: kdirent_decref(self->env_cwd); goto err_nofile;
err_entry2:  kdirent_decref(self->env_cwd);
err_entry:  kfdentry_quit(&entry);
err_nofile: error = KE_NOFILE;
 goto end;
}






kerrno_t ksuperblock_newfat(struct ksdev *sdev, __ref struct ksuperblock **result) {
 struct kfatsuperblock *resblock; kerrno_t error;
 resblock = ocalloc(struct kfatsuperblock);
 if __unlikely(!resblock) return KE_NOMEM;
 ksuperblock_generic_init((struct ksuperblock *)resblock,&kfatsuperblock_type);
 error = kfatsuperblock_init(resblock,sdev);
 if (KE_ISOK(error)) *result = (struct ksuperblock *)resblock;
 else free(resblock);
 return error;
}


struct kdirent __kfs_root = KDIRENT_INIT_UNNAMED(NULL,NULL);

void kernel_initialize_filesystem(void) {
 struct ksuperblock *root_superblock;
 struct ksdev *storage_device; kerrno_t error;
 // Look for a usable storage device for the root filesystem
 error = ksdev_new_findfirstata(&storage_device);
 if (error == KE_NOSYS) {
  // Without a usable storage device, create a new ram disk
  error = ksdev_new_ramdisk(&storage_device,512,
                            /*128MB*/(1024*1024*128)/512);
 }
 if __unlikely(KE_ISERR(error)) {
  k_syslogf(KLOG_ERROR,"Failed to initialize the filesystem: %d\n",error);
  return;
 }
 // Initialize a FAT superblock
 error = ksuperblock_newfat(storage_device,&root_superblock);
 ksdev_decref(storage_device);
 if __unlikely(KE_ISERR(error)) {
  k_syslogf(KLOG_ERROR,"Failed to initialize the filesystem: %d\n",error);
  return;
 }
 // Add an additional reference for '__kproc_kernel_root'
 ++root_superblock->s_root.i_refcnt;
 __kfs_root.d_inode = ksuperblock_root(root_superblock); // Inherit reference
 extern struct kdirfile __kproc_kernel_root;
 __kproc_kernel_root.df_inode = __kfs_root.d_inode;
}

void kernel_finalize_filesystem(void) {
 struct kinode *root_node;
 extern struct kdirfile __kproc_kernel_root;
 (*kdirfile_type.ft_quit)((struct kfile *)&__kproc_kernel_root);

 // Close the kernel ZERO-task context, thus closing
 // all file descriptors opened within the kernel.
 // >> This makes shutting down the filesystem much easier.
 kproc_close(kproc_kernel());

 // Unmount all mounted filesystems
 kmountman_unmountall();
 // Close the filesystem root node
 kdirent_lock(&__kfs_root,KDIRENT_LOCK_NODE);
 root_node = __kfs_root.d_inode;
 __kfs_root.d_inode = NULL;
 kdirent_unlock(&__kfs_root,KDIRENT_LOCK_NODE);
 if __unlikely(!root_node) return; // Already closed?
 __evalexpr(kinode_close(root_node));
 if (root_node->i_refcnt != 1) {
  k_syslogf(KLOG_ERROR
           ,"[LEAK] Invalid reference count in filesystem root_node: %I32u\n"
           ,root_node->i_refcnt);
 }
 kinode_decref(root_node);
}


struct kinodetype kinode_generic_emptytype = {
 .it_size = sizeof(struct kinode)
};


__DECL_END

#ifndef __INTELLISENSE__
#include "file.c.inl"
#include "fs-blockfile.c.inl"
#include "fs-dirfile.c.inl"
#include "fs-generic.c.inl"
#endif

#endif /* !__KOS_KERNEL_FS_FS_C__ */

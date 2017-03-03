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
#ifndef __KOS_KERNEL_FS_FILE_C_INL__
#define __KOS_KERNEL_FS_FILE_C_INL__ 1

#include <fcntl.h>
#include <kos/attr.h>
#include <kos/compiler.h>
#include <kos/errno.h>
#include <kos/kernel/closelock.h>
#include <kos/kernel/fs/file.h>
#include <kos/kernel/fs/fs.h>
#include <kos/kernel/object.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

__DECL_BEGIN

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// --- KFILE

__crit kerrno_t
kfile_opennode(struct kdirent *__restrict dent,
               struct kinode *__restrict node,
               __ref struct kfile **__restrict result,
               openmode_t mode) {
 struct kfiletype *resulttype; struct kfile *resultfile; kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kdirent(dent);
 kassert_kinode(node);
 kassertobj(result);
 if __unlikely(mode&O_DIRECTORY) {
  /* Make sure that we're opening a directory. */
  if (!S_ISDIR(node->i_kind)) return KE_NODIR;
 }
 /* Lookup the used file type. */
 resulttype = node->i_filetype; kassertobj(resulttype);
 if __unlikely(resulttype->ft_size < sizeof(struct kfile)) return KE_NOSYS;
 resultfile = (struct kfile *)malloc(resulttype->ft_size);
 if __unlikely(!resultfile) return KE_NOMEM;
 kobject_init(resultfile,KOBJECT_MAGIC_FILE);
 resultfile->f_refcnt = 1;
 resultfile->f_type   = resulttype;
 /* Call the file-specific initializer. */
 if __likely(resulttype->ft_open) {
  error = (*resulttype->ft_open)(resultfile,dent,node,mode);
  if __unlikely(KE_ISERR(error)) { free(resultfile); return error; }
 }
 *result = resultfile;
 return KE_OK;
}


__crit kerrno_t
kfile_open(struct kdirent *__restrict dent,
           __ref struct kfile **__restrict result,
           openmode_t mode) {
 struct kinode *filenode; kerrno_t error;
 KTASK_CRIT_MARK
 if __unlikely((filenode = kdirent_getnode(dent)) == NULL) return KE_DESTROYED;
 error = kfile_opennode(dent,filenode,result,mode);
 kinode_decref(filenode);
 return error;
}


extern void kfile_destroy(struct kfile *__restrict self);
void kfile_destroy(struct kfile *__restrict self) {
 void(*quitcall)(struct kfile *);
 kassert_object(self,KOBJECT_MAGIC_FILE);
 kassertobj(self->f_type);
 /* Call the optional quit-callback. */
 if __likely((quitcall = self->f_type->ft_quit) != NULL) {
  kassertbyte(quitcall);
  (*quitcall)(self);
 }
 /* Finally, free the memory allocated for the file. */
 free(self);
}

kerrno_t
__kfile_user_getfilename_fromdirent(struct kfile const *__restrict self,
                                    __user char *buf, size_t bufsize,
                                    __kernel size_t *__restrict reqsize) {
 __ref struct kdirent *__restrict dent; kerrno_t error;
 if __unlikely((dent = kfile_getdirent((struct kfile *)self)) == NULL) return KE_NOSYS;
 error = kdirent_user_getfilename(dent,buf,bufsize,reqsize);
 kdirent_decref(dent);
 return error;
}
kerrno_t
__kfile_user_getpathname_fromdirent(struct kfile const *__restrict self,
                                    struct kdirent *__restrict root,
                                    __user char *buf, size_t bufsize,
                                    __kernel size_t *__restrict reqsize) {
 __ref struct kdirent *__restrict dent; kerrno_t error;
 if (root) kassert_kdirent(root);
 if __unlikely((dent = kfile_getdirent((struct kfile *)self)) == NULL) return KE_NOSYS;
 error = kdirent_user_getpathname(dent,root,buf,bufsize,reqsize);
 kdirent_decref(dent);
 return error;
}

kerrno_t
kfile_user_pread(struct kfile *__restrict self, pos_t pos,
                 __user void *buf, size_t bufsize,
                 __kernel size_t *__restrict rsize) {
 kerrno_t(*callback)(struct kfile *,pos_t,void *,size_t,size_t *);
 kerrno_t error; kassert_kfile(self);
 if ((callback = self->f_type->ft_pread) != NULL) {
  kassertbyte(callback);
  error = (*callback)(self,pos,buf,bufsize,rsize);
 } else {
  pos_t oldpos;
  error = kfile_seek(self,0,SEEK_CUR,&oldpos);
  if __likely(KE_ISOK(error)) {
   error = kfile_seek(self,*(off_t *)&pos,SEEK_SET,NULL);
   if __likely(KE_ISOK(error)) error = kfile_user_read(self,buf,bufsize,rsize);
   __evalexpr(kfile_seek(self,*(off_t *)&oldpos,SEEK_SET,NULL));
  }
 }
 return error;
}
kerrno_t
kfile_user_pwrite(struct kfile *__restrict self, pos_t pos,
                  __user void const *buf, size_t bufsize,
                  __kernel size_t *__restrict wsize) {
 kerrno_t(*callback)(struct kfile *,pos_t,void const *,size_t,size_t *);
 kerrno_t error; kassert_kfile(self);
 if ((callback = self->f_type->ft_pwrite) != NULL) {
  kassertbyte(callback);
  error = (*callback)(self,pos,buf,bufsize,wsize);
 } else {
  pos_t oldpos;
  error = kfile_seek(self,0,SEEK_CUR,&oldpos);
  if __likely(KE_ISOK(error)) {
   error = kfile_seek(self,*(off_t *)&pos,SEEK_SET,NULL);
   if __likely(KE_ISOK(error)) error = kfile_user_write(self,buf,bufsize,wsize);
   __evalexpr(kfile_seek(self,*(off_t *)&oldpos,SEEK_SET,NULL));
  }
 }
 return error;
}
kerrno_t
kfile_user_fast_pread(struct kfile *__restrict self, pos_t pos,
                      __user void *buf, size_t bufsize,
                      __kernel size_t *__restrict rsize) {
 kerrno_t(*callback)(struct kfile *,pos_t,void *,size_t,size_t *);
 kerrno_t error; kassert_kfile(self);
 if ((callback = self->f_type->ft_pread) != NULL) {
  kassertbyte(callback);
  error = (*callback)(self,pos,buf,bufsize,rsize);
 } else {
  error = kfile_seek(self,*(off_t *)&pos,SEEK_SET,NULL);
  if __likely(KE_ISOK(error)) error = kfile_user_read(self,buf,bufsize,rsize);
 }
 return error;
}
kerrno_t
kfile_user_fast_pwrite(struct kfile *__restrict self, pos_t pos,
                       __user void const *buf, size_t bufsize,
                       __kernel size_t *__restrict wsize) {
 kerrno_t(*callback)(struct kfile *,pos_t,void const *,size_t,size_t *);
 kerrno_t error; kassert_kfile(self);
 if ((callback = self->f_type->ft_pwrite) != NULL) {
  kassertbyte(callback);
  error = (*callback)(self,pos,buf,bufsize,wsize);
 } else {
  error = kfile_seek(self,*(off_t *)&pos,SEEK_SET,NULL);
  if __likely(KE_ISOK(error)) error = kfile_user_write(self,buf,bufsize,wsize);
 }
 return error;
}

kerrno_t
kfile_user_readall(struct kfile *__restrict self,
                   __user void *buf, size_t bufsize) {
 kerrno_t error; size_t rsize;
 kassert_kfile(self);
 kassertmem(buf,bufsize);
 while __likely(KE_ISOK(error = kfile_user_read(self,buf,bufsize,&rsize))) {
  assert(rsize <= bufsize);
  if __likely(rsize == bufsize) break;
  if __unlikely(!rsize) return KE_NOSPC;
  *(uintptr_t *)&buf += rsize;
  bufsize -= rsize;
 }
 return error;
}
kerrno_t
kfile_user_writeall(struct kfile *__restrict self,
                    __user void const *buf, size_t bufsize) {
 kerrno_t error; size_t wsize;
 kassert_kfile(self);
 kassertmem(buf,bufsize);
 while __likely(KE_ISOK(error = kfile_user_write(self,buf,bufsize,&wsize))) {
  assert(wsize <= bufsize);
  if __likely(wsize == bufsize) break;
  if __unlikely(!wsize) return KE_NOSPC;
  *(uintptr_t *)&buf += wsize;
  bufsize -= wsize;
 }
 return error;
}

kerrno_t
kfile_user_preadall(struct kfile *__restrict self, pos_t pos,
                    __user void *buf, size_t bufsize) {
 kerrno_t error; size_t rsize;
 kassert_kfile(self);
 kassertmem(buf,bufsize);
 while __likely(KE_ISOK(error = kfile_user_pread(self,pos,buf,bufsize,&rsize))) {
  assertf(rsize <= bufsize,"Invalid read size: %Iu > %Iu",rsize,bufsize);
  if __likely(rsize == bufsize) break;
  if __unlikely(!rsize) return KE_NOSPC;
  *(uintptr_t *)&buf += rsize;
  bufsize -= rsize;
  pos += rsize;
 }
 return error;
}

kerrno_t
kfile_user_pwriteall(struct kfile *__restrict self, pos_t pos,
                     __user void const *buf, size_t bufsize) {
 kerrno_t error; size_t wsize;
 kassert_kfile(self);
 kassertmem(buf,bufsize);
 while __likely(KE_ISOK(error = kfile_user_pwrite(self,pos,buf,bufsize,&wsize))) {
  assertf(wsize <= bufsize,"Invalid write size: %Iu > %Iu",wsize,bufsize);
  if __likely(wsize == bufsize) break;
  if __unlikely(!wsize) return KE_NOSPC;
  *(uintptr_t *)&buf += wsize;
  bufsize -= wsize;
  pos += wsize;
 }
 return error;
}

kerrno_t
kfile_user_fast_preadall(struct kfile *__restrict self, pos_t pos,
                         __user void *buf, size_t bufsize) {
 kerrno_t error; size_t rsize;
 kassert_kfile(self);
 kassertmem(buf,bufsize);
 while __likely(KE_ISOK(error = kfile_user_fast_pread(self,pos,buf,bufsize,&rsize))) {
  assert(rsize <= bufsize);
  if __likely(rsize == bufsize) break;
  if __unlikely(!rsize) return KE_NOSPC;
  *(uintptr_t *)&buf += rsize;
  bufsize -= rsize;
  pos += rsize;
 }
 return error;
}

kerrno_t
kfile_user_fast_pwriteall(struct kfile *__restrict self, pos_t pos,
                          __user void const *buf, size_t bufsize) {
 kerrno_t error; size_t wsize;
 kassert_kfile(self);
 kassertmem(buf,bufsize);
 while __likely(KE_ISOK(error = kfile_user_fast_pwrite(self,pos,buf,bufsize,&wsize))) {
  assert(wsize <= bufsize);
  if __likely(wsize == bufsize) break;
  if __unlikely(!wsize) return KE_NOSPC;
  *(uintptr_t *)&buf += wsize;
  bufsize -= wsize;
  pos += wsize;
 }
 return error;
}


kerrno_t
kfile_generic_open(struct kfile *__restrict __unused(self),
                   struct kdirent *__restrict __unused(dirent),
                   struct kinode *__restrict __unused(inode),
                   openmode_t __unused(mode)) {
 return KE_OK;
}
kerrno_t
kfile_generic_read_isdir(struct kfile *__restrict __unused(self),
                         __user void *__unused(buf), size_t __unused(bufsize),
                         __kernel size_t *__restrict __unused(rsize)) {
 return KE_ISDIR;
}
kerrno_t
kfile_generic_write_isdir(struct kfile *__restrict __unused(self),
                          __user void const *__unused(buf), size_t __unused(bufsize),
                          __kernel size_t *__restrict __unused(wsize)) {
 return KE_ISDIR;
}
kerrno_t
kfile_generic_readat_isdir(struct kfile *__restrict __unused(self), pos_t __unused(pos),
                           __user void *__unused(buf), size_t __unused(bufsize),
                           __kernel size_t *__restrict __unused(rsize)) {
 return KE_ISDIR;
}
kerrno_t
kfile_generic_writeat_isdir(struct kfile *__restrict __unused(self), pos_t __unused(pos),
                            __user void const *__unused(buf), size_t __unused(bufsize),
                            __kernel size_t *__restrict __unused(wsize)) {
 return KE_ISDIR;
}


__crit __kernel char *kfile_getmallname(struct kfile *__restrict fp) {
 char *result,*newresult; size_t bufsize,reqsize; kerrno_t error;
 KTASK_CRIT_MARK
 result = (char *)malloc(bufsize = (PATH_MAX+1)*sizeof(char));
 if __unlikely(!result) result = (char *)malloc((bufsize = 2*sizeof(char)));
 if __unlikely(!result) return NULL;
again:
 error = kfile_kernel_getattr(fp,KATTR_FS_PATHNAME,result,bufsize,&reqsize);
 if __unlikely(KE_ISERR(error)) goto err_res;
 if (reqsize != bufsize) {
  newresult = (char *)realloc(result,bufsize = reqsize);
  if __unlikely(!newresult) goto err_res;
  result = newresult;
  goto again;
 }
 return result;
err_res:
 free(result);
 return NULL; 
}

__DECL_END

#endif /* !__KOS_KERNEL_FS_FILE_C_INL__ */

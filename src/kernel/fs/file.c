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
#ifndef __KOS_KERNEL_FS_FILE_C__
#define __KOS_KERNEL_FS_FILE_C__ 1

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


#include <kos/kernel/syscall.h>
__DECL_BEGIN

KSYSCALL_DEFINE_EX5(c,kerrno_t,kfd_open,int,cwd,__user char const *,path,
                    size_t,maxpath,openmode_t,mode,mode_t,perms) {
 struct kproc *ctx = kproc_self();
 struct kfspathenv env; int result;
 struct kfdentry entry; kerrno_t error;
 KTASK_CRIT_MARK
 path = (char const *)kpagedir_translate(ktask_self()->t_epd,path); /* TODO: Unsafe. */
 env.env_cwd = kproc_getfddirent(ctx,cwd);
 if __unlikely(!env.env_cwd) return KE_NOCWD;
 env.env_root = kproc_getfddirent(ctx,KFD_ROOT);
 if __unlikely(!env.env_root) {
  kdirent_decref(env.env_cwd);
  return KE_NOROOT;
 }
 env.env_flags = mode;
 env.env_uid   = kproc_getuid(ctx);
 env.env_gid   = kproc_getgid(ctx);
 kfspathenv_initcommon(&env);
 if (env.env_flags&O_CREAT) {
  union kinodeattr attr[3];
  attr[0].ia_owner.a_id    = KATTR_FS_OWNER;
  attr[0].ia_owner.o_owner = env.env_uid;
  attr[1].ia_group.a_id    = KATTR_FS_GROUP;
  attr[1].ia_group.g_group = env.env_gid;
  attr[2].ia_perm.a_id     = KATTR_FS_PERM;
  attr[2].ia_perm.p_perm   = perms;
  error = kdirent_openat(&env,path,maxpath,env.env_flags,
                         COMPILER_ARRAYSIZE(attr),attr,&entry.fd_file);
 } else {
  error = kdirent_openat(&env,path,maxpath,env.env_flags,0,NULL,&entry.fd_file);
 }
 kdirent_decref(env.env_root);
 kdirent_decref(env.env_cwd);
 if __unlikely(KE_ISERR(error)) return error;
 kassert_kfile(entry.fd_file);
 entry.fd_attr = KFD_ATTR(KFDTYPE_FILE,(mode&O_CLOEXEC) ? KFD_FLAG_CLOEXEC : KFD_FLAG_NONE);
 assert(entry.fd_type == KFDTYPE_FILE);
 assert(entry.fd_flag == KFD_FLAG_NONE);
 error = kproc_insfd_inherited(ctx,&result,&entry);
 if __likely(KE_ISOK(error)) error = result;
 else kfile_decref(entry.fd_file);
 return error;
}

KSYSCALL_DEFINE_EX6(c,kerrno_t,kfd_open2,int,dfd,int,cwd,__user char const *,path,
                    size_t,maxpath,openmode_t,mode,mode_t,perms) {
 struct kproc *caller = kproc_self();
 struct kfspathenv env;
 struct kfdentry entry; kerrno_t error;
 KTASK_CRIT_MARK
 path = (char const *)kpagedir_translate(ktask_self()->t_epd,path); /* TODO: Unsafe. */
 if __unlikely(dfd == KFD_ROOT || dfd == KFD_CWD) {
  if __unlikely(!kproc_hasflag(caller,dfd == KFD_ROOT
                ? KPERM_FLAG_CHROOT : KPERM_FLAG_CHDIR)
                ) return KE_ACCES;
 }
 env.env_cwd = kproc_getfddirent(caller,cwd);
 if __unlikely(!env.env_cwd) return KE_NOCWD;
 env.env_root = kproc_getfddirent(caller,KFD_ROOT);
 if __unlikely(!env.env_root) { kdirent_decref(env.env_cwd); return KE_NOROOT; ; }
 env.env_flags = mode;
 env.env_uid   = kproc_getuid(caller);
 env.env_gid   = kproc_getgid(caller);
 kfspathenv_initcommon(&env);
 if (env.env_flags&O_CREAT) {
  union kinodeattr attr[3];
  attr[0].ia_owner.a_id    = KATTR_FS_OWNER;
  attr[0].ia_owner.o_owner = env.env_uid;
  attr[1].ia_group.a_id    = KATTR_FS_GROUP;
  attr[1].ia_group.g_group = env.env_gid;
  attr[2].ia_perm.a_id     = KATTR_FS_PERM;
  attr[2].ia_perm.p_perm   = perms;
  error = kdirent_openat(&env,path,maxpath,env.env_flags,
                         COMPILER_ARRAYSIZE(attr),attr,&entry.fd_file);
 } else {
  error = kdirent_openat(&env,path,maxpath,env.env_flags,0,NULL,&entry.fd_file);
 }
 kdirent_decref(env.env_root);
 kdirent_decref(env.env_cwd);
 if __unlikely(KE_ISERR(error)) return error;
 kassert_kfile(entry.fd_file);
 entry.fd_attr = KFD_ATTR(KFDTYPE_FILE,(mode&O_CLOEXEC) ? KFD_FLAG_CLOEXEC : KFD_FLAG_NONE);
 error = kproc_insfdat_inherited(caller,dfd,&entry);
 if __likely(KE_ISOK(error)) error = dfd;
 else kfile_decref(entry.fd_file);
 return error;
}

KSYSCALL_DEFINE_EX1(c,kerrno_t,kfd_close,int,fd) {
 kerrno_t error;
 struct kproc *caller = kproc_self();
 KTASK_CRIT_MARK
 if __unlikely(fd == KFD_ROOT || fd == KFD_CWD) {
  if __unlikely(!kproc_hasflag(caller,fd == KFD_ROOT
                ? KPERM_FLAG_CHROOT : KPERM_FLAG_CHDIR)
                ) return KE_ACCES;
 }
 error = kproc_closefd(caller,fd);
 return error;
}

KSYSCALL_DEFINE_EX2(c,unsigned int,kfd_closeall,int,low,int,high) {
 struct kproc *caller = kproc_self();
 KTASK_CRIT_MARK
 if __unlikely(low <= KFD_CWD) {
  __STATIC_ASSERT_M(KFD_ROOT < KFD_CWD,"FIXME: Must swap order");
  if (low <= KFD_ROOT && !kproc_hasflag(caller,KPERM_FLAG_CHROOT)) low = KFD_ROOT+1;
  if (low <= KFD_CWD  && !kproc_hasflag(caller,KPERM_FLAG_CHDIR )) low = KFD_CWD+1;
 }
 return kproc_closeall(kproc_self(),low,high);
}

#ifdef KFD_HAVE_BIARG_POSITIONS
#if (__BYTE_ORDER == __LITTLE_ENDIAN) == defined(KOS_ARCH_STACK_GROWS_DOWNWARDS)
KSYSCALL_DEFINE_EX5(c,kerrno_t,kfd_seek,int,fd,__s32,offhi,
                    __s32,offlo,int,whence,__u64 *,newpos)
#else /* linear: down */
KSYSCALL_DEFINE_EX5(c,kerrno_t,kfd_seek,int,fd,__s32,offlo,
                    __s32,offhi,int,whence,__u64 *,newpos)
#endif /* linear: up */
#else /* KFD_HAVE_BIARG_POSITIONS */
KSYSCALL_DEFINE_EX4(c,kerrno_t,kfd_seek,int,fd,__s64,off,
                    int,whence,__u64 *,newpos)
#endif /* !KFD_HAVE_BIARG_POSITIONS */
{
 __u64 kernel_newpos; kerrno_t error;
 KTASK_CRIT_MARK
#ifdef KFD_HAVE_BIARG_POSITIONS
 error = kproc_seekfd(kproc_self(),fd,
                     (__s64)offhi << 32 | offlo,whence,
                      newpos ? &kernel_newpos : NULL);
#else
 error = kproc_seekfd(kproc_self(),fd,off,whence,
                      newpos ? &kernel_newpos : NULL);
#endif
 if (__likely(KE_ISOK(error)) && newpos &&
     __unlikely(copy_to_user(newpos,&kernel_newpos,sizeof(kernel_newpos)))
     ) error = KE_FAULT;
 return error;
}

KSYSCALL_DEFINE_EX4(c,kerrno_t,kfd_read,int,fd,__user void *,buf,
                    size_t,bufsize,__user size_t *,rsize) {
 kerrno_t error; size_t kernel_rsize;
 KTASK_CRIT_MARK
 error = kproc_user_readfd(kproc_self(),fd,buf,bufsize,&kernel_rsize);
 if (__likely(KE_ISOK(error)) &&
     __unlikely(copy_to_user(rsize,&kernel_rsize,sizeof(kernel_rsize)))
     ) error = KE_FAULT;
 return error;
}

/*< _syscall4(kerrno_t,kfd_write,int,fd,void const *,buf,size_t,bufsize,size_t *,wsize); */
KSYSCALL_DEFINE_EX4(c,kerrno_t,kfd_write,int,fd,__user void const *,buf,
                    size_t,bufsize,__user size_t *,wsize) {
 kerrno_t error; size_t kernel_wsize;
 KTASK_CRIT_MARK
 error = kproc_user_writefd(kproc_self(),fd,buf,bufsize,&kernel_wsize);
 if (__likely(KE_ISOK(error)) &&
     __unlikely(copy_to_user(wsize,&kernel_wsize,sizeof(kernel_wsize)))
     ) error = KE_FAULT;
 return error;
}

#ifdef KFD_HAVE_BIARG_POSITIONS
#if (__BYTE_ORDER == __LITTLE_ENDIAN) == defined(KOS_ARCH_STACK_GROWS_DOWNWARDS)
KSYSCALL_DEFINE_EX6(c,kerrno_t,kfd_pread,int,fd,__u32,poshi,__u32,poslo,
                    __user void *,buf,size_t,bufsize,__user size_t *,rsize)
#else /* linear: down */
KSYSCALL_DEFINE_EX6(c,kerrno_t,kfd_pread,int,fd,__u32,poslo,__u32,poshi,
                    __user void *,buf,size_t,bufsize,__user size_t *,rsize)
#endif /* linear: up */
#else
KSYSCALL_DEFINE_EX5(c,kerrno_t,kfd_pread,int,fd,__u64,pos,
                    __user void *,buf,size_t,bufsize,__user size_t *,rsize)
#endif
{
 kerrno_t error; size_t kernel_rsize;
 KTASK_CRIT_MARK
#ifdef KFD_HAVE_BIARG_POSITIONS
 error = kproc_user_preadfd(kproc_self(),fd,
                           (__u64)poshi << 32 | poslo,
                            buf,bufsize,&kernel_rsize);
#else
 error = kproc_user_preadfd(kproc_self(),fd,pos,
                            buf,bufsize,&kernel_rsize);
#endif
 if (__likely(KE_ISOK(error)) &&
     __unlikely(copy_to_user(rsize,&kernel_rsize,sizeof(kernel_rsize)))
     ) error = KE_FAULT;
 return error;
}

#ifdef KFD_HAVE_BIARG_POSITIONS
#if (__BYTE_ORDER == __LITTLE_ENDIAN) == defined(KOS_ARCH_STACK_GROWS_DOWNWARDS)
KSYSCALL_DEFINE_EX6(c,kerrno_t,kfd_pwrite,int,fd,__u32,poshi,__u32,poslo,
                    __user void const *,buf,size_t,bufsize,__user size_t *,wsize)
#else /* linear: down */
KSYSCALL_DEFINE_EX6(c,kerrno_t,kfd_pwrite,int,fd,__u32,poslo,__u32,poshi,
                    __user void const *,buf,size_t,bufsize,__user size_t *,wsize)
#endif /* linear: up */
#else
KSYSCALL_DEFINE_EX5(c,kerrno_t,kfd_pwrite,int,fd,__u64,pos,
                    __user void const *,buf,size_t,bufsize,__user size_t *,wsize)
#endif
{
 kerrno_t error; size_t kernel_wsize;
 KTASK_CRIT_MARK
#ifdef KFD_HAVE_BIARG_POSITIONS
 error = kproc_user_pwritefd(kproc_self(),fd,
                            (__u64)poshi << 32 | poslo,
                             buf,bufsize,&kernel_wsize);
#else
 error = kproc_user_pwritefd(kproc_self(),fd,pos,
                             buf,bufsize,&kernel_wsize);
#endif
 if (__likely(KE_ISOK(error)) &&
     __unlikely(copy_to_user(wsize,&kernel_wsize,sizeof(kernel_wsize)))
     ) error = KE_FAULT;
 return error;
}

KSYSCALL_DEFINE_EX1(c,kerrno_t,kfd_flush,int,fd) {
 KTASK_CRIT_MARK
 return kproc_flushfd(kproc_self(),fd);
}

/*< _syscall{2|3}(kerrno_t,kfd_trunc,int,fd,{__u64,size|__u32,sizehi,__u32,sizelo}); */
#ifdef KFD_HAVE_BIARG_POSITIONS
#if (__BYTE_ORDER == __LITTLE_ENDIAN) == defined(KOS_ARCH_STACK_GROWS_DOWNWARDS)
KSYSCALL_DEFINE_EX3(c,kerrno_t,kfd_trunc,int,fd,__u32,sizehi,__u32,sizelo)
#else /* linear: down */
KSYSCALL_DEFINE_EX3(c,kerrno_t,kfd_trunc,int,fd,__u32,sizelo,__u32,sizehi)
#endif /* linear: up */
#else
KSYSCALL_DEFINE_EX2(c,kerrno_t,kfd_trunc,int,fd,__u64,size)
#endif
{
 KTASK_CRIT_MARK
#ifdef KFD_HAVE_BIARG_POSITIONS
 return kproc_truncfd(kproc_self(),fd,
                     (__u64)sizehi << 32 | sizelo);
#else
 return kproc_truncfd(kproc_self(),fd,size);
#endif
}

KSYSCALL_DEFINE_EX3(c,kerrno_t,kfd_fcntl,int,fd,
                    int,cmd,__user void *,arg) {
 KTASK_CRIT_MARK
 return kproc_user_fcntlfd(kproc_self(),fd,cmd,arg);
}

KSYSCALL_DEFINE_EX3(c,kerrno_t,kfd_ioctl,int,fd,
                    kattr_t,cmd,__user void *,arg) {
 KTASK_CRIT_MARK
 return kproc_user_ioctlfd(kproc_self(),fd,cmd,arg);
}

KSYSCALL_DEFINE_EX5(c,kerrno_t,kfd_getattr,int,fd,kattr_t,attr,
                    __user void *,buf,size_t,bufsize,
                    __user size_t *,reqsize) {
 kerrno_t error; size_t kernel_reqsize;
 KTASK_CRIT_MARK
 error = kproc_user_getattrfd(kproc_self(),fd,attr,buf,bufsize,
                              reqsize ? &kernel_reqsize : NULL);
 if (__likely(KE_ISOK(error)) && reqsize &&
     __unlikely(copy_to_user(reqsize,&kernel_reqsize,sizeof(kernel_reqsize)))
     ) error = KE_FAULT;
 return error;
}

KSYSCALL_DEFINE_EX4(c,kerrno_t,kfd_setattr,int,fd,kattr_t,attr,
                    __user void const *,buf,size_t,bufsize) {
 KTASK_CRIT_MARK
 return kproc_user_setattrfd(kproc_self(),fd,attr,buf,bufsize);
}

KSYSCALL_DEFINE_EX3(c,kerrno_t,kfd_readdir,int,fd,
                    __user struct kfddirent *,dent,__u32,flags) {
 struct kinode *inode; struct kdirentname *name;
 kerrno_t error; struct kfdentry entry;
 struct kfddirent kernel_dent;
 KTASK_CRIT_MARK
 if __unlikely(copy_from_user(&kernel_dent,dent,sizeof(struct kfdentry))) return KE_FAULT;
 error = kproc_getfd(kproc_self(),fd,&entry);
 if __unlikely(KE_ISERR(error)) return error;
 if __unlikely(entry.fd_type != KFDTYPE_FILE) error = KE_NOFILE;
 else if __likely((error = kfile_readdir(entry.fd_file,&inode,&name,
                                         flags&KFD_READDIR_MASKINTERNAL)
                   ) == KE_OK) {
  union kinodeattr  attr[2];
  union kinodeattr *iter = attr;
  size_t reqsize;
  kassert_kinode(inode);
  kassertobj(name);
  reqsize = (name->dn_size+1)*sizeof(char);
  if __unlikely(copy_to_user(kernel_dent.kd_namev,name->dn_name,
                             min(reqsize,kernel_dent.kd_namec))
                ) { error = KE_FAULT; goto end_inode; }
  if (kernel_dent.kd_namec >= reqsize && (flags&KFD_READDIR_FLAG_PERFECT)) {
   /* Advance the directory stream by one if the buffer was sufficient */
   error = kfile_seek(entry.fd_file,1,SEEK_CUR,NULL);
   if __unlikely(KE_ISERR(error)) goto end_inode;
  }
  kernel_dent.kd_namec = reqsize;
  /* Fill in additional information if requested to */
  if (flags&KFD_READDIR_FLAG_INO)   (*iter++).ia_common.a_id = KATTR_FS_INO;
  if (flags&KFD_READDIR_FLAG_PERM)  (*iter++).ia_common.a_id = KATTR_FS_PERM;
  if (iter != attr) {
   error = kinode_kernel_getattr(inode,(size_t)(iter-attr),attr);
   if __likely(KE_ISOK(error)) {
    iter = attr;
    if (flags&KFD_READDIR_FLAG_INO) kernel_dent.kd_ino = (*iter++).ia_ino.i_ino;
    if (flags&KFD_READDIR_FLAG_KIND) {
     kernel_dent.kd_mode = inode->i_kind;
     if (flags&KFD_READDIR_FLAG_PERM) kernel_dent.kd_mode |= (*iter++).ia_perm.p_perm;
    } else if (flags&KFD_READDIR_FLAG_PERM) {
     kernel_dent.kd_mode = (*iter++).ia_perm.p_perm;
    }
   }
  }
  if (  __likely(KE_ISOK(error)) &&
      __unlikely(copy_to_user(dent,&kernel_dent,sizeof(struct kfdentry)))
      ) error = KE_FAULT;
end_inode:
  kinode_decref(inode);
 }
 kfdentry_quit(&entry);
 return error;
}


KSYSCALL_DEFINE_EX4(c,kerrno_t,kfd_readlink,int,fd,
                    __user char *,buf,size_t,bufsize,
                    __user size_t *,reqsize) {
 struct kproc *caller = kproc_self();
 struct kfdentry entry; kerrno_t error;
 __ref struct kinode *filenode;
 struct kdirentname link_name;
 size_t link_req;
 KTASK_CRIT_MARK
 if __unlikely(KE_ISERR(error = kproc_getfd(caller,fd,&entry))) return error;
 if __unlikely(entry.fd_type != KFDTYPE_FILE) { kfdentry_quit(&entry); return KE_NOFILE; }
 filenode = kfile_getinode(entry.fd_file);
 kfile_decref(entry.fd_file);
 if __unlikely(!filenode) return KE_NOLNK;
 error = kinode_readlink(filenode,&link_name);
 kinode_decref(filenode);
 if __unlikely(KE_ISERR(error)) return error;
 link_req = (link_name.dn_size+1)*sizeof(char);
 if (reqsize && copy_to_user(reqsize,&link_req,sizeof(link_req))) goto err_fault;
 if (link_name.dn_name[link_name.dn_size] == '\0') {
  if (copy_to_user(buf,link_name.dn_name,min(bufsize,link_req))) goto err_fault;
 } else {
  size_t link_size = link_name.dn_size*sizeof(char);
  if (copy_to_user(buf,link_name.dn_name,min(bufsize,link_size))) goto err_fault;
  if (link_size < bufsize &&
      copy_to_user((void *)((uintptr_t)buf+link_size),
                   "\0",sizeof(char))) goto err_fault;
 }
end_name: kdirentname_quit(&link_name);
 return error;
err_fault: error = KE_FAULT; goto end_name;
}

KSYSCALL_DEFINE_EX2(c,int,kfd_dup,int,fd,int,flags) {
 kerrno_t error; int result;
 KTASK_CRIT_MARK
 error = kproc_dupfd(kproc_self(),fd,&result,(kfdflag_t)flags);
 if __likely(KE_ISOK(error)) error = result;
 return error;
}

KSYSCALL_DEFINE_EX3(c,int,kfd_dup2,int,fd,int,resfd,int,flags) {
 kerrno_t error;
 KTASK_CRIT_MARK
 error = kproc_dupfd2(kproc_self(),fd,resfd,(kfdflag_t)flags);
 if __likely(KE_ISOK(error)) error = resfd;
 return error;
}

KSYSCALL_DEFINE2(kerrno_t,kfd_equals,int,fda,int,fdb) {
 return kproc_equalfd(kproc_self(),fda,fdb);
}

__DECL_END

#endif /* !__KOS_KERNEL_FS_FILE_C__ */

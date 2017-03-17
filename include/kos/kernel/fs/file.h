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
#ifndef __KOS_KERNEL_FS_FILE_H__
#define __KOS_KERNEL_FS_FILE_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/attr.h>
#include <kos/compiler.h>
#include <kos/errno.h>
#include <kos/fd.h>
#include <kos/kernel/closelock.h>
#include <kos/kernel/debug.h>
#include <kos/kernel/object.h>
#include <kos/kernel/task.h>
#include <kos/kernel/proc.h>
#include <kos/types.h>

__DECL_BEGIN

#ifndef __ASSEMBLY__
struct kfiletype;
struct kfile;
struct kinode;
struct kdirent;
struct kdirentname;
#endif /* __ASSEMBLY__ */

#define KOBJECT_MAGIC_FILE       0xF17E     /*< FILE. */
#define kassert_kfile(ob)       kassert_refobject(ob,f_refcnt,KOBJECT_MAGIC_FILE)

#ifndef SEEK_SET
#   define SEEK_SET 0
#endif
#ifndef SEEK_CUR
#   define SEEK_CUR 1
#endif
#ifndef SEEK_END
#   define SEEK_END 2
#endif

#ifndef __ASSEMBLY__
struct kfiletype {
 __size_t   ft_size; /*< Size of the implemented file-structure (if not at least 'sizeof(struct kfile)', the file cannot be opened) */
 void     (*ft_quit)(struct kfile *__restrict self);
 kerrno_t (*ft_open)(struct kfile *__restrict self, struct kdirent *__restrict dirent,
                     struct kinode *__restrict inode, __openmode_t mode);
 kerrno_t (*ft_read)(struct kfile *__restrict self, __user void *buf, __size_t bufsize, __kernel __size_t *__restrict rsize);
 kerrno_t (*ft_write)(struct kfile *__restrict self, __user void const *buf, __size_t bufsize, __kernel __size_t *__restrict wsize);
 kerrno_t (*ft_ioctl)(struct kfile *__restrict self, kattr_t cmd, __user void *arg);
 kerrno_t (*ft_getattr)(struct kfile const *__restrict self, kattr_t attr, __user void *buf, __size_t bufsize, __kernel __size_t *__restrict reqsize); /* NOTE: 'reqsize' may be NULL. */
 kerrno_t (*ft_setattr)(struct kfile *__restrict self, kattr_t attr, __user void const *buf, __size_t bufsize);
 kerrno_t (*ft_pread)(struct kfile *__restrict self, __pos_t pos, __user void *buf, __size_t bufsize, __kernel __size_t *__restrict rsize);
 kerrno_t (*ft_pwrite)(struct kfile *__restrict self, __pos_t pos, __user void const *buf, __size_t bufsize, __kernel __size_t *__restrict wsize);
 kerrno_t (*ft_seek)(struct kfile *__restrict self, __off_t off, int whence, __kernel __pos_t *__restrict newpos); /* NOTE: 'newpos' may be NULL. */
 kerrno_t (*ft_trunc)(struct kfile *__restrict self, __pos_t size); /*< Set the current end-of-file marker to 'size' */
 kerrno_t (*ft_flush)(struct kfile *__restrict self);
 kerrno_t (*ft_readdir)(struct kfile *__restrict self, __ref struct kinode **__restrict inode, struct kdirentname **__restrict name, __u32 flags); /*< Returns KS_EMPTY when the end is reached. */
 __ref struct kinode *(*ft_getinode)(struct kfile *__restrict self);
 __ref struct kdirent *(*ft_getdirent)(struct kfile *__restrict self);
 void      *ft_padding[16]; /*< Padding operator members. */
};

struct kfile {
 KOBJECT_HEAD
 __atomic __u32    f_refcnt; /*< Reference counter. */
 struct kfiletype *f_type;   /*< [1..1][const] File type. */
};
#define KFILE_INIT(type) {KOBJECT_INIT(KOBJECT_MAGIC_FILE) 0xffff,type}
__local void kfile_init(struct kfile *__restrict self,
                        struct kfiletype *__restrict type) {
 kobject_init(self,KOBJECT_MAGIC_FILE);
 self->f_refcnt = 1;
 self->f_type = type;
}


__local KOBJECT_DEFINE_INCREF(kfile_incref,struct kfile,f_refcnt,kassert_kfile);
__local KOBJECT_DEFINE_DECREF(kfile_decref,struct kfile,f_refcnt,kassert_kfile,kfile_destroy);

//////////////////////////////////////////////////////////////////////////
// Opens the file associated with a given directory entry
// @return: * :           File-specific error
// @return: KE_NOMEM:     Not enough memory available
// @return: KE_NOSYS:     The given directory entry cannot be opened
// @return: KE_DESTROYED: The associated dirent was destroyed.
extern __crit __wunused __nonnull((1,2)) kerrno_t
kfile_open(struct kdirent *__restrict dent,
           __ref struct kfile **__restrict result,
           __openmode_t mode);
extern __crit __wunused __nonnull((1,2,3)) kerrno_t
kfile_opennode(struct kdirent *__restrict dent, struct kinode *__restrict node,
               __ref struct kfile **__restrict result, __openmode_t mode);

#define KFILE_READDIR_FLAG_NONE  KFD_READDIR_FLAG_NONE
#define KFILE_READDIR_FLAG_PEEK  KFD_READDIR_FLAG_PEEK /*< Only retrieve the current entry. - don't advance the directory file. */

#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Perform various operations on a given file
// @return: * : File-specific error
// @return: KE_DESTROYED: File was already closed
extern        __wunused __nonnull((1,4))   kerrno_t kfile_user_read(struct kfile *__restrict self, __user void *buf, __size_t bufsize, __kernel __size_t *__restrict rsize);
extern        __wunused __nonnull((1,4))   kerrno_t kfile_user_write(struct kfile *__restrict self, __user void const *buf, __size_t bufsize, __kernel __size_t *__restrict wsize);
extern        __wunused __nonnull((1))     kerrno_t kfile_user_ioctl(struct kfile *__restrict self, kattr_t cmd, __user void *arg);
extern        __wunused __nonnull((1))     kerrno_t kfile_user_getattr(struct kfile const *__restrict self, kattr_t attr, __user void *buf, __size_t bufsize, __kernel __size_t *__restrict reqsize);
extern        __wunused __nonnull((1))     kerrno_t kfile_user_setattr(struct kfile *__restrict self, kattr_t attr, __user void const *buf, __size_t bufsize);
extern        __wunused __nonnull((1,4))   kerrno_t kfile_kernel_read(struct kfile *__restrict self, __kernel void *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict rsize);
extern        __wunused __nonnull((1,4))   kerrno_t kfile_kernel_write(struct kfile *__restrict self, __kernel void const *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict wsize);
extern        __wunused __nonnull((1))     kerrno_t kfile_kernel_ioctl(struct kfile *__restrict self, kattr_t cmd, __kernel void *arg);
extern        __wunused __nonnull((1))     kerrno_t kfile_kernel_getattr(struct kfile const *__restrict self, kattr_t attr, __kernel void *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict reqsize);
extern        __wunused __nonnull((1))     kerrno_t kfile_kernel_setattr(struct kfile *__restrict self, kattr_t attr, __kernel void const *__restrict buf, __size_t bufsize);
extern        __wunused __nonnull((1))     kerrno_t kfile_seek(struct kfile *__restrict self, __off_t off, int whence, __kernel __pos_t *newpos);
extern        __wunused __nonnull((1))     kerrno_t kfile_trunc(struct kfile *__restrict self, __pos_t size);
extern        __wunused __nonnull((1))     kerrno_t kfile_flush(struct kfile *__restrict self);
extern __crit __wunused __nonnull((1,2,3)) kerrno_t kfile_readdir(struct kfile *__restrict self, __ref struct kinode **__restrict inode, struct kdirentname **__restrict name, __u32 flags);
#else
#define kfile_user_read(self,buf,bufsize,rsize) \
 __xblock({ struct kfile *const __kfrself = (self);\
            kassert_kfile(__kfrself);\
            __xreturn       __kfrself->f_type->ft_read\
             ? (kassertbyte(__kfrself->f_type->ft_read),\
                           *__kfrself->f_type->ft_read)(__kfrself,buf,bufsize,rsize)\
             : KE_NOSYS;\
 })
#define kfile_user_write(self,buf,bufsize,wsize) \
 __xblock({ struct kfile *const __kfwself = (self);\
            kassert_kfile(__kfwself);\
            __xreturn       __kfwself->f_type->ft_write\
             ? (kassertbyte(__kfwself->f_type->ft_write),\
                           *__kfwself->f_type->ft_write)(__kfwself,buf,bufsize,wsize)\
             : KE_NOSYS;\
 })
#define kfile_user_ioctl(self,cmd,arg) \
 __xblock({ struct kfile *const __kficself = (self);\
            kassert_kfile(__kficself);\
            __xreturn       __kficself->f_type->ft_ioctl\
             ? (kassertbyte(__kficself->f_type->ft_ioctl),\
                           *__kficself->f_type->ft_ioctl)(__kficself,cmd,arg)\
             : KE_NOSYS;\
 })
#define kfile_user_getattr(self,attr,buf,bufsize,reqsize) \
 __xblock({ struct kfile *const __kfgaself = (self);\
            kassert_kfile(__kfgaself);\
            __xreturn       __kfgaself->f_type->ft_getattr\
             ? (kassertbyte(__kfgaself->f_type->ft_getattr),\
                           *__kfgaself->f_type->ft_getattr)(__kfgaself,attr,buf,bufsize,reqsize)\
                                   : kfile_generic_getattr (__kfgaself,attr,buf,bufsize,reqsize);\
 })
#define kfile_user_setattr(self,attr,buf,bufsize) \
 __xblock({ struct kfile *const __kfsaself = (self);\
            kassert_kfile(__kfsaself);\
            __xreturn       __kfsaself->f_type->ft_setattr\
             ? (kassertbyte(__kfsaself->f_type->ft_setattr),\
                           *__kfsaself->f_type->ft_setattr)(__kfsaself,attr,buf,bufsize)\
                                   : kfile_generic_setattr (__kfsaself,attr,buf,bufsize);\
 })
#define kfile_seek(self,off,whence,newpos) \
 __xblock({ struct kfile *const __kfsself = (self);\
            kassert_kfile(__kfsself);\
            __xreturn       __kfsself->f_type->ft_seek\
             ? (kassertbyte(__kfsself->f_type->ft_seek),\
                           *__kfsself->f_type->ft_seek)(__kfsself,off,whence,newpos)\
             : KE_NOSYS;\
 })
#define kfile_trunc(self,size) \
 __xblock({ struct kfile *const __kftself = (self);\
            kassert_kfile(__kftself);\
            __xreturn       __kftself->f_type->ft_trunc\
             ? (kassertbyte(__kftself->f_type->ft_trunc),\
                           *__kftself->f_type->ft_trunc)(__kftself,size)\
             : KE_NOSYS;\
 })
#define kfile_flush(self) \
 __xblock({ struct kfile *const __kffself = (self);\
            kassert_kfile(__kffself);\
            __xreturn       __kffself->f_type->ft_flush\
             ? (kassertbyte(__kffself->f_type->ft_flush),\
                           *__kffself->f_type->ft_flush)(__kffself)\
             : KE_NOSYS;\
 })
#define kfile_readdir(self,inode,name,flags) \
 __xblock({ struct kfile *const __kfrdself = (self);\
            kassert_kfile(__kfrdself);\
            __xreturn       __kfrdself->f_type->ft_readdir\
             ? (kassertbyte(__kfrdself->f_type->ft_readdir),\
                           *__kfrdself->f_type->ft_readdir)(__kfrdself,inode,name,flags)\
             : KE_NOSYS;\
 })
#define kfile_kernel_read(self,buf,bufsize,rsize)           KTASK_KEPD(kfile_user_read(self,buf,bufsize,rsize))
#define kfile_kernel_write(self,buf,bufsize,wsize)          KTASK_KEPD(kfile_user_write(self,buf,bufsize,wsize))
#define kfile_kernel_ioctl(self,cmd,arg)                    KTASK_KEPD(kfile_user_ioctl(self,cmd,arg))
#define kfile_kernel_getattr(self,attr,buf,bufsize,reqsize) KTASK_KEPD(kfile_user_getattr(self,attr,buf,bufsize,reqsize))
#define kfile_kernel_setattr(self,attr,buf,bufsize)         KTASK_KEPD(kfile_user_setattr(self,attr,buf,bufsize))
#endif

extern __wunused __nonnull((1,5)) kerrno_t kfile_user_pread(struct kfile *__restrict self, __pos_t pos, __user void *buf, __size_t bufsize, __kernel __size_t *__restrict rsize);
extern __wunused __nonnull((1,5)) kerrno_t kfile_user_pwrite(struct kfile *__restrict self, __pos_t pos, __user void const *buf, __size_t bufsize, __kernel __size_t *__restrict wsize);
extern __wunused __nonnull((1,5)) kerrno_t kfile_user_fast_pread(struct kfile *__restrict self, __pos_t pos, __user void *buf, __size_t bufsize, __kernel __size_t *__restrict rsize);
extern __wunused __nonnull((1,5)) kerrno_t kfile_user_fast_pwrite(struct kfile *__restrict self, __pos_t pos, __user void const *buf, __size_t bufsize, __kernel __size_t *__restrict wsize);
#ifdef __INTELLISENSE__
extern __wunused __nonnull((1,5)) kerrno_t kfile_kernel_pread(struct kfile *__restrict self, __pos_t pos, __kernel void *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict rsize);
extern __wunused __nonnull((1,5)) kerrno_t kfile_kernel_pwrite(struct kfile *__restrict self, __pos_t pos, __kernel void const *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict wsize);
extern __wunused __nonnull((1,5)) kerrno_t kfile_kernel_fast_pread(struct kfile *__restrict self, __pos_t pos, __kernel void *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict rsize);
extern __wunused __nonnull((1,5)) kerrno_t kfile_kernel_fast_pwrite(struct kfile *__restrict self, __pos_t pos, __kernel void const *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict wsize);
#else
#define kfile_kernel_pread(self,pos,buf,bufsize,rsize)       KTASK_KEPD(kfile_user_pread(self,pos,buf,bufsize,rsize))
#define kfile_kernel_pwrite(self,pos,buf,bufsize,wsize)      KTASK_KEPD(kfile_user_pwrite(self,pos,buf,bufsize,wsize))
#define kfile_kernel_fast_pread(self,pos,buf,bufsize,rsize)  KTASK_KEPD(kfile_user_fast_pread(self,pos,buf,bufsize,rsize))
#define kfile_kernel_fast_pwrite(self,pos,buf,bufsize,wsize) KTASK_KEPD(kfile_user_fast_pwrite(self,pos,buf,bufsize,wsize))
#endif

#define kfile_rewind(self)     kfile_seek(self,0,SEEK_SET,NULL)
#define kfile_tell(self,pos)   kfile_seek(self,0,SEEK_CUR,pos)
#define kfile_setpos(self,pos) kfile_seek(self,(__off_t)(pos),SEEK_SET,NULL)

//////////////////////////////////////////////////////////////////////////
// Read/Write all data, returning 'KE_NOSPC' is not all could be read/written
extern __wunused __nonnull((1,2)) kerrno_t kfile_user_readall(struct kfile *__restrict self, __user void *buf, __size_t bufsize);
extern __wunused __nonnull((1,2)) kerrno_t kfile_user_writeall(struct kfile *__restrict self, __user void const *buf, __size_t bufsize);
extern __wunused __nonnull((1,3)) kerrno_t kfile_user_preadall(struct kfile *__restrict self, __pos_t pos, __user void *buf, __size_t bufsize);
extern __wunused __nonnull((1,3)) kerrno_t kfile_user_pwriteall(struct kfile *__restrict self, __pos_t pos, __user void const *buf, __size_t bufsize);
extern __wunused __nonnull((1,3)) kerrno_t kfile_user_fast_preadall(struct kfile *__restrict self, __pos_t pos, __user void *buf, __size_t bufsize);
extern __wunused __nonnull((1,3)) kerrno_t kfile_user_fast_pwriteall(struct kfile *__restrict self, __pos_t pos, __user void const *buf, __size_t bufsize);

#ifdef __INTELLISENSE__
extern __wunused __nonnull((1,2)) kerrno_t kfile_kernel_readall(struct kfile *__restrict self, __kernel void *__restrict buf, __size_t bufsize);
extern __wunused __nonnull((1,2)) kerrno_t kfile_kernel_writeall(struct kfile *__restrict self, __kernel void const *__restrict buf, __size_t bufsize);
extern __wunused __nonnull((1,3)) kerrno_t kfile_kernel_preadall(struct kfile *__restrict self, __pos_t pos, __kernel void *__restrict buf, __size_t bufsize);
extern __wunused __nonnull((1,3)) kerrno_t kfile_kernel_pwriteall(struct kfile *__restrict self, __pos_t pos, __kernel void const *__restrict buf, __size_t bufsize);
extern __wunused __nonnull((1,3)) kerrno_t kfile_kernel_fast_preadall(struct kfile *__restrict self, __pos_t pos, __kernel void *__restrict buf, __size_t bufsize);
extern __wunused __nonnull((1,3)) kerrno_t kfile_kernel_fast_pwriteall(struct kfile *__restrict self, __pos_t pos, __kernel void const *__restrict buf, __size_t bufsize);
#else
#define kfile_kernel_readall(self,buf,bufsize)            KTASK_KEPD(kfile_user_readall(self,buf,bufsize))
#define kfile_kernel_writeall(self,buf,bufsize)           KTASK_KEPD(kfile_user_writeall(self,buf,bufsize))
#define kfile_kernel_preadall(self,pos,buf,bufsize)       KTASK_KEPD(kfile_user_preadall(self,pos,buf,bufsize))
#define kfile_kernel_pwriteall(self,pos,buf,bufsize)      KTASK_KEPD(kfile_user_pwriteall(self,pos,buf,bufsize))
#define kfile_kernel_fast_preadall(self,pos,buf,bufsize)  KTASK_KEPD(kfile_user_fast_preadall(self,pos,buf,bufsize))
#define kfile_kernel_fast_pwriteall(self,pos,buf,bufsize) KTASK_KEPD(kfile_user_fast_pwriteall(self,pos,buf,bufsize))
#endif

//////////////////////////////////////////////////////////////////////////
// Returns the filename/absolute path of a given directory entry
// @return: KE_NOSYS: No dirent is associated with the given file.
extern __wunused __nonnull((1))   kerrno_t __kfile_user_getfilename_fromdirent(struct kfile const *__restrict self, __user char *buf, __size_t bufsize, __kernel __size_t *__restrict reqsize);
extern __wunused __nonnull((1,2)) kerrno_t __kfile_user_getpathname_fromdirent(struct kfile const *__restrict self, struct kdirent *__restrict root, __user char *buf, __size_t bufsize, __kernel __size_t *__restrict reqsize);
#ifdef __INTELLISENSE__
extern __wunused __nonnull((1))   kerrno_t __kfile_kernel_getfilename_fromdirent(struct kfile const *__restrict self, __kernel char *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict reqsize);
extern __wunused __nonnull((1,2)) kerrno_t __kfile_kernel_getpathname_fromdirent(struct kfile const *__restrict self, struct kdirent *__restrict root, __kernel char *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict reqsize);
#else
#define __kfile_kernel_getfilename_fromdirent(self,buf,bufsize,reqsize)      KTASK_KEPD(__kfile_user_getfilename_fromdirent(self,buf,bufsize,reqsize))
#define __kfile_kernel_getpathname_fromdirent(self,root,buf,bufsize,reqsize) KTASK_KEPD(__kfile_user_getpathname_fromdirent(self,root,buf,bufsize,reqsize))
#endif


extern __crit __malloccall __wunused __nonnull((1)) __kernel char *
kfile_getmallname(struct kfile *__restrict fp);


#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Returns the inode/dirent used to open a given file
// WARNING: Availability of this is file-type dependent, and
//          kfile types such as sockets don't implement them.
// @return: NULL: No inode/dirent is associated with the given file.
extern __crit __nonnull((1)) __ref struct kinode *kfile_getinode(struct kfile *__restrict self);
extern __crit __nonnull((1)) __ref struct kdirent *kfile_getdirent(struct kfile *__restrict self);
#else
#define kfile_getinode(self) \
 __xblock({ struct kfile *const __kfgiself = (self);\
            KTASK_CRIT_MARK\
            kassert_kfile(__kfgiself);\
            __xreturn       __kfgiself->f_type->ft_getinode\
             ? (kassertbyte(__kfgiself->f_type->ft_getinode),\
                           *__kfgiself->f_type->ft_getinode)(__kfgiself)\
             : NULL;\
 })
#define kfile_getdirent(self) \
 __xblock({ struct kfile *const __kfgdself = (self);\
            KTASK_CRIT_MARK\
            kassert_kfile(__kfgdself);\
            __xreturn       __kfgdself->f_type->ft_getdirent\
             ? (kassertbyte(__kfgdself->f_type->ft_getdirent),\
                           *__kfgdself->f_type->ft_getdirent)(__kfgdself)\
             : NULL;\
 })
#endif


//////////////////////////////////////////////////////////////////////////
// Generic ioctl / attributes.
// >> When overwriting ioctl, getattr or setattr, you should
//    call these functions for attributes you don't know.
extern __wunused __nonnull((1)) kerrno_t kfile_generic_ioctl(struct kfile *__restrict self, kattr_t cmd, __user void *arg);
extern __wunused __nonnull((1)) kerrno_t kfile_generic_getattr(struct kfile const *__restrict self, kattr_t attr, __user void *buf, __size_t bufsize, __kernel __size_t *__restrict reqsize);
extern __wunused __nonnull((1)) kerrno_t kfile_generic_setattr(struct kfile *__restrict self, kattr_t attr, __user void const *buf, __size_t bufsize);

extern kerrno_t kfile_generic_open(struct kfile *__restrict self, struct kdirent *__restrict dirent, struct kinode *__restrict inode, __openmode_t mode);
extern kerrno_t kfile_generic_read_isdir(struct kfile *__restrict self, __user void *buf, __size_t bufsize, __kernel __size_t *__restrict rsize);
extern kerrno_t kfile_generic_write_isdir(struct kfile *__restrict self, __user void const *buf, __size_t bufsize, __kernel __size_t *__restrict wsize);
extern kerrno_t kfile_generic_readat_isdir(struct kfile *__restrict self, __pos_t pos, __user void *buf, __size_t bufsize, __kernel __size_t *__restrict rsize);
extern kerrno_t kfile_generic_writeat_isdir(struct kfile *__restrict self, __pos_t pos, __user void const *buf, __size_t bufsize, __kernel __size_t *__restrict wsize);
#endif /* !__ASSEMBLY__ */

__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_FS_FILE_H__ */

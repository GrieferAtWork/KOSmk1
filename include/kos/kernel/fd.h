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
#ifndef __KOS_KERNEL_FD_H__
#define __KOS_KERNEL_FD_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <fcntl.h>
#include <kos/types.h>
#include <kos/endian.h>
#include <kos/fd.h>
#include <kos/task.h>

__DECL_BEGIN

#ifndef __ASSEMBLY__
struct kfile;
struct ktask;
struct kproc;
struct kinode;
struct kdirent;
struct kdev;
#endif /* __ASSEMBLY__ */

#define kassert_kfdentry kassertobj

#define KFD_FLAG_NONE    0x0000
#define KFD_FLAG_CLOEXEC FD_CLOEXEC

#ifndef __ASSEMBLY__
typedef __u16 kfdflag_t;
#endif /* !__ASSEMBLY__ */


#if __BYTE_ORDER == __LITTLE_ENDIAN
#define KFD_ATTR(type,flags) ((__u32)(type)|((__u32)(flags) << 16))
#else
#define KFD_ATTR(type,flags) ((__u32)(flags)|((__u32)(type) << 16))
#endif

/* Process file descriptor management */
#ifndef __ASSEMBLY__
struct __packed kfdentry {
 /* NOTE: Members of this structure are protected
  *       by the associated fd manager.
  *       >> Though you can take a reference to the linked resource
  *          and unlock the fd manager before operation on that resource. */
 union __packed {
  struct __packed {
   kfdtype_t            fd_type; /*< File descriptor type. */
   kfdflag_t            fd_flag; /*< File descriptor flags. */
  };
  __u32                 fd_attr; /*< Type and flag attributes. */
 };
 union __packed { /* [0..1] Resource pointer. */
  void                 *fd_data;   /*< [0..1] Resource data. */
  __ref struct kfile   *fd_file;   /*< [case:KFD_TYPE_FILE]. */
  __ref struct ktask   *fd_task;   /*< [case:KFD_TYPE_TASK]. */
  __ref struct kproc   *fd_proc;   /*< [case:KFDTYPE_PROC]. */
  __ref struct kinode  *fd_inode;  /*< [case:KFD_TYPE_INODE]. */
  __ref struct kdirent *fd_dirent; /*< [case:KFD_TYPE_DIRENT]. */
  __ref struct kdev    *fd_dev;    /*< [case:KFD_TYPE_DEVICE]. */
 };
};
#define kfdentry_init(self) \
 ((self)->fd_attr = KFD_ATTR(KFDTYPE_NONE,KFD_FLAG_NONE)\
 ,(self)->fd_data = NULL)
#define KFDENTRY_INIT         {{{KFDTYPE_NONE,KFD_FLAG_NONE}},{NULL}}
#define KFDENTRY_INIT_FILE(p) {{{KFDTYPE_FILE,KFD_FLAG_NONE}},{p}}
#define KFDENTRY_INIT_TASK(p) {{{KFDTYPE_TASK,KFD_FLAG_NONE}},{p}}

//////////////////////////////////////////////////////////////////////////
// Copies a given file descriptor slot (by reference)
// @return: KE_OK:       The descriptor slot was successfully copied.
// @return: KE_OVERFLOW: Too many references.
extern __crit kerrno_t kfdentry_initcopyself(struct kfdentry *__restrict self);
__local __crit kerrno_t kfdentry_initcopy(struct kfdentry *__restrict self,
                                          struct kfdentry const *__restrict right);

extern __crit void kfdentry_quitex(kfdtype_t type, void *__restrict data);
__local __crit void kfdentry_quit(struct kfdentry *__restrict self);

extern kerrno_t kfdentry_user_read(struct kfdentry *__restrict self, __user void *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict rsize);
extern kerrno_t kfdentry_user_write(struct kfdentry *__restrict self, __user void const *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict wsize);
extern kerrno_t kfdentry_user_pread(struct kfdentry *__restrict self, __pos_t pos, __user void *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict rsize);
extern kerrno_t kfdentry_user_pwrite(struct kfdentry *__restrict self, __pos_t pos, __user void const *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict wsize);
extern kerrno_t kfdentry_user_ioctl(struct kfdentry *__restrict self, kattr_t cmd, __user void *arg);
extern kerrno_t kfdentry_user_getattr(struct kfdentry *__restrict self, kattr_t attr, __user void *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict reqsize);
extern kerrno_t kfdentry_user_setattr(struct kfdentry *__restrict self, kattr_t attr, __user void const *__restrict buf, __size_t bufsize);
extern kerrno_t kfdentry_user_enumtasks(struct kfdentry *__restrict self, __user __size_t *__restrict idv, __size_t idc, __kernel __size_t *__restrict reqidc);

#ifdef __INTELLISENSE__
extern kerrno_t kfdentry_kernel_read(struct kfdentry *__restrict self, __kernel void *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict rsize);
extern kerrno_t kfdentry_kernel_write(struct kfdentry *__restrict self, __kernel void const *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict wsize);
extern kerrno_t kfdentry_kernel_pread(struct kfdentry *__restrict self, __pos_t pos, __kernel void *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict rsize);
extern kerrno_t kfdentry_kernel_pwrite(struct kfdentry *__restrict self, __pos_t pos, __kernel void const *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict wsize);
extern kerrno_t kfdentry_kernel_ioctl(struct kfdentry *__restrict self, kattr_t cmd, __kernel void *arg);
extern kerrno_t kfdentry_kernel_getattr(struct kfdentry *__restrict self, kattr_t attr, __kernel void *__restrict buf, __size_t bufsize, __kernel __size_t *__restrict reqsize);
extern kerrno_t kfdentry_kernel_setattr(struct kfdentry *__restrict self, kattr_t attr, __kernel void const *__restrict buf, __size_t bufsize);
extern kerrno_t kfdentry_kernel_enumtasks(struct kfdentry *__restrict self, __kernel __size_t *__restrict idv, __size_t idc, __kernel __size_t *__restrict reqidc);
#else
#define kfdentry_kernel_read(self,buf,bufsize,rsize)           KTASK_KEPD(kfdentry_user_read(self,buf,bufsize,rsize))
#define kfdentry_kernel_write(self,buf,bufsize,wsize)          KTASK_KEPD(kfdentry_user_write(self,buf,bufsize,wsize))
#define kfdentry_kernel_pread(self,pos,buf,bufsize,rsize)      KTASK_KEPD(kfdentry_user_pread(self,pos,buf,bufsize,rsize))
#define kfdentry_kernel_pwrite(self,pos,buf,bufsize,wsize)     KTASK_KEPD(kfdentry_user_pwrite(self,pos,buf,bufsize,wsize))
#define kfdentry_kernel_ioctl(self,cmd,arg)                    KTASK_KEPD(kfdentry_user_ioctl(self,cmd,arg))
#define kfdentry_kernel_getattr(self,attr,buf,bufsize,reqsize) KTASK_KEPD(kfdentry_user_getattr(self,attr,buf,bufsize,reqsize))
#define kfdentry_kernel_setattr(self,attr,buf,bufsize)         KTASK_KEPD(kfdentry_user_setattr(self,attr,buf,bufsize))
#define kfdentry_kernel_enumtasks(self,idv,idc,reqidc)         KTASK_KEPD(kfdentry_user_enumtasks(self,idv,idc,reqidc))
#endif

extern        kerrno_t kfdentry_seek(struct kfdentry *__restrict self, __off_t off, int whence, __kernel __pos_t *__restrict newpos);
extern        kerrno_t kfdentry_trunc(struct kfdentry *__restrict self, __pos_t size);
extern        kerrno_t kfdentry_flush(struct kfdentry *__restrict self);
extern        kerrno_t kfdentry_terminate(struct kfdentry *__restrict self, void *exitcode, ktaskopflag_t flags);
extern        kerrno_t kfdentry_suspend(struct kfdentry *__restrict self, ktaskopflag_t flags);
extern        kerrno_t kfdentry_resume(struct kfdentry *__restrict self, ktaskopflag_t flags);
extern __crit kerrno_t kfdentry_openprocof(struct kfdentry *__restrict self, __ref struct ktask **__restrict result);
extern __crit kerrno_t kfdentry_openparent(struct kfdentry *__restrict self, __ref struct ktask **__restrict result);
extern __crit kerrno_t kfdentry_openctx(struct kfdentry *__restrict self, __ref struct kproc**__restrict result);
extern        __size_t kfdentry_getparid(struct kfdentry *__restrict self);
extern        __size_t kfdentry_gettid(struct kfdentry *__restrict self);
extern __crit kerrno_t kfdentry_opentask(struct kfdentry *__restrict self, __size_t id, __ref struct ktask **__restrict result);
extern        kerrno_t kfdentry_getpriority(struct kfdentry *__restrict self, __kernel ktaskprio_t *__restrict result);
extern        kerrno_t kfdentry_setpriority(struct kfdentry *__restrict self, ktaskprio_t value);



#ifdef __INTELLISENSE__
extern __nonnull((1,2)) kerrno_t kfd_getflags(struct kfdentry const *__restrict self, kfdflag_t *pflags);
extern __nonnull((1))   kerrno_t kfd_addflags(struct kfdentry *__restrict self, kfdflag_t flags);
extern __nonnull((1))   kerrno_t kfd_delflags(struct kfdentry *__restrict self, kfdflag_t flags);
extern __nonnull((1))   kerrno_t kfd_setflags(struct kfdentry *__restrict self, kfdflag_t flags);
#else
#define kfd_getflags(self,pflags) (*(pflags) = (self)->fd_flag,KE_OK)
#define kfd_addflags(self,flags)  ((self)->fd_flag |= (flags),KE_OK)
#define kfd_delflags(self,flags)  ((self)->fd_flag &= ~(flags),KE_OK)
#define kfd_setflags(self,flags)  ((self)->fd_flag = (flags),KE_OK)
#endif


#ifndef __INTELLISENSE__
__local __crit kerrno_t
kfdentry_initcopy(struct kfdentry *__restrict self,
                  struct kfdentry const *__restrict right) {
 kassertobj(self); kassert_kfdentry(right);
 self->fd_attr = right->fd_attr;
 self->fd_data = right->fd_data;
 return kfdentry_initcopyself(self);
}

__local __crit void kfdentry_quit(struct kfdentry *__restrict self) {
 kassert_kfdentry(self);
 kfdentry_quitex(self->fd_type,self->fd_data);
#if 0
#ifdef __DEBUG__
 self->fd_type = KFDTYPE_NONE;
 self->fd_data = NULL;
#endif /* __DEBUG__ */
#endif
}
#endif
#endif /* !__ASSEMBLY__ */

__DECL_END

#ifndef __INTELLISENSE__
#include <kos/kernel/task.h>
#include <kos/kernel/proc.h>
#endif
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_FD_H__ */

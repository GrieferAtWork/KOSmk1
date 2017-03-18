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
#ifndef __KOS_KERNEL_PIPE_C__
#define __KOS_KERNEL_PIPE_C__ 1

#include <kos/config.h>
#include <kos/kernel/pipe.h>
#include <kos/kernel/proc.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <kos/kernel/fs/vfs-dev.h>
#include <kos/kernel/util/string.h>

__DECL_BEGIN

//struct ksuperblock kpipe_fs = {
// KSUPERBLOCK_INIT_HEAD
// KINODE_INIT()
//};

#define FIX_MAXSIZE(size) \
 min(size,kprocperm_getpipemax(kproc_getperm(kproc_self())))


#define SELF ((struct kpipe *)self)
static void pipe_quit(struct kinode *__restrict self) { kiobuf_quit(&SELF->p_iobuf); }
static kerrno_t
pipe_getattr(struct kinode const *__restrict self,
             size_t ac, __user union kinodeattr av[]) {
 kerrno_t error;
 union kinodeattr attr;
 for (; ac; --ac,++av) {
  if __unlikely(copy_from_user(&attr,av,sizeof(union kinodeattr))) return KE_FAULT;
  switch (attr.ia_common.a_id) {
   {
    size_t cursize;
    if (0) { case KATTR_FS_SIZE: error = kiobuf_getrsize(&SELF->p_iobuf,&cursize); }
    if (0) { case KATTR_FS_BUFSIZE: error = kiobuf_getwsize(&SELF->p_iobuf,&cursize); }
    if (0) { case KATTR_FS_MAXSIZE: error = kiobuf_getmaxsize(&SELF->p_iobuf,&cursize); }
    if __unlikely(KE_ISERR(error)) return error;
    attr.ia_size.sz_size = (pos_t)cursize;
    break;
   }
   default:
    error = kinode_user_generic_getattr(self,1,av);
    if __unlikely(KE_ISERR(error)) return error;
    goto next_attr;
  }
  if __unlikely(copy_to_user(av,&attr,sizeof(union kinodeattr))) return KE_FAULT;
next_attr:;
 }
 return KE_OK;
}
static kerrno_t
pipe_setattr(struct kinode *__restrict self, size_t ac,
             union kinodeattr const av[]) {
 kerrno_t error;
 size_t new_max_size;
 union kinodeattr attr;
 for (; ac; --ac,++av) {
  if __unlikely(copy_from_user(&attr,av,sizeof(attr))) return KE_FAULT;
  switch (attr.ia_common.a_id) {
   case KATTR_FS_MAXSIZE:
    new_max_size = FIX_MAXSIZE(attr.ia_size.sz_size);
    error = kiobuf_setmaxsize(&SELF->p_iobuf,new_max_size,NULL);
    if __unlikely(KE_ISERR(error)) return error;
    break;
   default:
    error = kinode_user_generic_setattr(self,1,av);
    if __unlikely(KE_ISERR(error)) return error;
    break;
  }
 }
 return KE_OK;
}
struct kinodetype kpipe_type = {
 .it_size    = sizeof(struct kpipe),
 .it_quit    = &pipe_quit,
 .it_getattr = &pipe_getattr,
 .it_setattr = &pipe_setattr,
};

__crit __ref struct kpipe *kpipe_new(size_t max_size) {
 __ref struct kpipe *result;
 KTASK_CRIT_MARK
 /* Create the pipe INode. */
#if 1
 result = (__ref struct kpipe *)__kinode_alloc((struct ksuperblock *)&kvfs_dev,&kpipe_type,
                                               &kpipesuper_type,
                                               S_IFIFO);
#else
 result = (__ref struct kpipe *)__kinode_alloc(&kpipe_fs,&kpipe_type,
                                               &kpipesuper_type,
                                               S_IFIFO);
#endif
 if __unlikely(!result) return NULL;
 kiobuf_init_ex(&result->p_iobuf,FIX_MAXSIZE(max_size));
 return result;
}



__local __crit __ref struct kpipefile *
kpipefile_new(struct kfiletype *__restrict type,
              struct kpipe *__restrict pipe,
              struct kdirent *dent) {
 __ref struct kpipefile *result;
 KTASK_CRIT_MARK
 if __unlikely(KE_ISERR(kinode_incref((struct kinode *)pipe))) return NULL;
 if __unlikely(dent && KE_ISERR(kdirent_incref(dent))) goto err_pipe;
 if __unlikely((result = omalloc(struct kpipefile)) == NULL) goto err_dent;
 kfile_init(&result->pr_file,type);
 result->pr_pipe = pipe; /* Inherit reference. */
 result->pr_dirent = dent; /* Inherit reference. */
 return result;
err_dent: if (dent) kdirent_decref(dent);
err_pipe: kinode_decref((struct kinode *)pipe);
 return NULL;
}
__crit __ref struct kpipefile *kpipefile_newreader(struct kpipe *__restrict pipe, struct kdirent *dent) { KTASK_CRIT_MARK return kpipefile_new(&kpipereader_type,pipe,dent); }
__crit __ref struct kpipefile *kpipefile_newwriter(struct kpipe *__restrict pipe, struct kdirent *dent) { KTASK_CRIT_MARK return kpipefile_new(&kpipewriter_type,pipe,dent); }


static void
kpipefile_quit(struct kpipefile *__restrict self) {
 kinode_decref((struct kinode *)self->pr_pipe);
 if (self->pr_dirent) kdirent_decref(self->pr_dirent);
}
static kerrno_t
kpipefile_read(struct kpipefile *__restrict self,
               __user void *buf, size_t bufsize,
               __kernel size_t *__restrict rsize) {
 /* TODO: Blocking behavior can be configured through fcntl. */
 return kiobuf_user_read(&self->pr_pipe->p_iobuf,buf,bufsize,rsize,
                         KIO_BLOCKFIRST|KIO_NONE);
}
static kerrno_t
kpipefile_write(struct kpipefile *__restrict self,
                __user void const *buf, size_t bufsize,
                __kernel size_t *__restrict wsize) {
 /* TODO: Blocking behavior can be configured through fcntl. */
 return kiobuf_user_write(&self->pr_pipe->p_iobuf,buf,bufsize,wsize,
                          KIO_BLOCKFIRST|KIO_NONE);
}
static kerrno_t
kpipefile_flush(struct kpipefile *__restrict self) {
 return kiobuf_flush(&self->pr_pipe->p_iobuf);
}
static kerrno_t
kpipefile_rseek(struct kpipefile *__restrict self, off_t off,
                int whence, __kernel pos_t *__restrict newpos) {
 kerrno_t error; size_t did_skip;
 if __unlikely(whence != SEEK_CUR) return KE_NOSYS;
 error = kiobuf_rseek(&self->pr_pipe->p_iobuf,(ssize_t)off,&did_skip);
 if (__likely(KE_ISOK(error)) && newpos) *newpos = (pos_t)did_skip;
 return error;
}
static kerrno_t
kpipefile_wseek(struct kpipefile *__restrict self, off_t off,
                int whence, __kernel pos_t *__restrict newpos) {
 kerrno_t error; size_t did_skip;
 if __unlikely(whence != SEEK_CUR) return KE_NOSYS;
 error = kiobuf_wseek(&self->pr_pipe->p_iobuf,(ssize_t)off,&did_skip);
 if (__likely(KE_ISOK(error)) && newpos) *newpos = (pos_t)did_skip;
 return error;
}


struct kfiletype kpipesuper_type = {
 .ft_size  = sizeof(struct kpipefile),
 .ft_quit  = (void(*)(struct kfile *__restrict))&kpipefile_quit,
 .ft_read  = (kerrno_t(*)(struct kfile *__restrict,void *__restrict,size_t,size_t *__restrict))&kpipefile_read,
 .ft_write = (kerrno_t(*)(struct kfile *__restrict,void const *__restrict,size_t,size_t *__restrict))&kpipefile_write,
};
struct kfiletype kpipereader_type = {
 .ft_size = sizeof(struct kpipefile),
 .ft_quit = (void(*)(struct kfile *__restrict))&kpipefile_quit,
 .ft_read = (kerrno_t(*)(struct kfile *__restrict,void *__restrict,size_t,size_t *__restrict))&kpipefile_read,
 .ft_seek = (kerrno_t(*)(struct kfile *__restrict,off_t,int,pos_t *__restrict))&kpipefile_rseek,
};
struct kfiletype kpipewriter_type = {
 .ft_size  = sizeof(struct kpipefile),
 .ft_quit  = (void(*)(struct kfile *__restrict))&kpipefile_quit,
 .ft_write = (kerrno_t(*)(struct kfile *__restrict,void const *__restrict,size_t,size_t *__restrict))&kpipefile_write,
 .ft_flush = (kerrno_t(*)(struct kfile *__restrict))&kpipefile_flush,
 .ft_seek  = (kerrno_t(*)(struct kfile *__restrict,off_t,int,pos_t *__restrict))&kpipefile_wseek,
};

__DECL_END


#include <kos/kernel/proc.h>
#include <kos/kernel/syscall.h>
#include <kos/kernel/fd.h>

__DECL_BEGIN

KSYSCALL_DEFINE_EX3(c,kerrno_t,kfd_pipe,__user int *,pipefd,
                    int,flags,size_t,max_size) {
 kerrno_t error; struct kpipe *pipe;
 struct kproc *proc_self = kproc_self();
 struct kfdentry entry; int kernel_pipefd[2];
 KTASK_CRIT_MARK
 pipe = kpipe_new(max_size);
 if __unlikely(!pipe) return KE_NOMEM;
 entry.fd_file = (struct kfile *)kpipefile_newreader(pipe,NULL);
 if __unlikely(!entry.fd_file) { error = KE_NOMEM; goto end_pipe; }
 entry.fd_type = KFDTYPE_FILE;
 entry.fd_flag = flags;
 error = kproc_insfd_inherited(proc_self,&kernel_pipefd[0],&entry);
 if __unlikely(KE_ISERR(error)) { kfdentry_quit(&entry); goto end_pipe; }
 entry.fd_file = (struct kfile *)kpipefile_newwriter(pipe,NULL);
 if __unlikely(!entry.fd_file) { error = KE_NOMEM; goto err_pipe0; }
 entry.fd_type = KFDTYPE_FILE;
 entry.fd_flag = flags;
 error = kproc_insfd_inherited(proc_self,&kernel_pipefd[1],&entry);
 if __unlikely(KE_ISERR(error)) { kfdentry_quit(&entry); goto err_pipe0; }
 /* File descriptors were created! */
 if __unlikely(copy_to_user(pipefd,kernel_pipefd,sizeof(kernel_pipefd))) {
  error = KE_FAULT;
/*err_pipe1:*/
  kproc_closefd(proc_self,kernel_pipefd[1]);
  goto err_pipe0;
 }
end_pipe:
 kinode_decref((struct kinode *)pipe);
 return error;
err_pipe0:
 kproc_closefd(proc_self,kernel_pipefd[0]);
 goto end_pipe;
}

__DECL_END

#endif /* !__KOS_KERNEL_PIPE_C__ */

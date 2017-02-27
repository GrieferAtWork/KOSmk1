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
#ifndef __KOS_KERNEL_FD_C_INL__
#define __KOS_KERNEL_FD_C_INL__ 1

#include <kos/compiler.h>
#include <kos/config.h>
#include <kos/kernel/dev/device.h>
#include <kos/kernel/fd.h>
#include <kos/kernel/fdman.h>
#include <kos/kernel/fs/file.h>
#include <kos/kernel/fs/fs.h>
#include <kos/kernel/task.h>
#include <kos/kernel/proc.h>
#include <kos/kernel/util/string.h>

__DECL_BEGIN

__crit kerrno_t kfdentry_initcopyself(struct kfdentry *__restrict self) {
 KTASK_CRIT_MARK
 kassertobj(self);
 switch (self->fd_type) {
  case KFDTYPE_FILE:   return kfile_incref(self->fd_file);
  case KFDTYPE_TASK:   return ktask_incref(self->fd_task);
  case KFDTYPE_PROC:   return kproc_incref(self->fd_proc);
  case KFDTYPE_INODE:  return kinode_incref(self->fd_inode);
  case KFDTYPE_DIRENT: return kdirent_incref(self->fd_dirent);
  case KFDTYPE_DEVICE: return kdev_incref(self->fd_dev);
  default:
#ifdef __DEBUG__
   self->fd_data = NULL;
#endif
   break;
 }
 return KE_OK;
}

__crit void kfdentry_quitex(kfdtype_t type, void *__restrict data) {
 KTASK_CRIT_MARK
 switch (type) {
  case KFDTYPE_FILE:   kfile_decref((struct kfile *)data); break;
  case KFDTYPE_TASK:   ktask_decref((struct ktask *)data); break;
  case KFDTYPE_PROC:   kproc_decref((struct kproc *)data); break;
  case KFDTYPE_INODE:  kinode_decref((struct kinode *)data); break;
  case KFDTYPE_DIRENT: kdirent_decref((struct kdirent *)data); break;
  case KFDTYPE_DEVICE: kdev_decref((struct kdev *)data); break;
  default: break;
 }
}

kerrno_t kfdentry_user_read(struct kfdentry *__restrict self,
                            __user void *__restrict buf, size_t bufsize,
                            __kernel size_t *__restrict rsize) {
 if (self->fd_type == KFDTYPE_FILE) return kfile_user_read(self->fd_file,buf,bufsize,rsize);
 return KE_NOSYS;
}
kerrno_t kfdentry_user_write(struct kfdentry *__restrict self,
                             __user void const *__restrict buf, size_t bufsize,
                             __kernel size_t *__restrict wsize) {
 if (self->fd_type == KFDTYPE_FILE) return kfile_user_write(self->fd_file,buf,bufsize,wsize);
 return KE_NOSYS;
}
kerrno_t kfdentry_user_pread(struct kfdentry *__restrict self, __pos_t pos,
                             __user void *__restrict buf, size_t bufsize,
                             __kernel size_t *__restrict rsize) {
 if (self->fd_type == KFDTYPE_FILE) return kfile_user_pread(self->fd_file,pos,buf,bufsize,rsize);
 return KE_NOSYS;
}
kerrno_t kfdentry_user_pwrite(struct kfdentry *__restrict self, __pos_t pos,
                              __user void const *__restrict buf, size_t bufsize,
                              __kernel size_t *__restrict wsize) {
 if (self->fd_type == KFDTYPE_FILE) return kfile_user_pwrite(self->fd_file,pos,buf,bufsize,wsize);
 return KE_NOSYS;
}
kerrno_t kfdentry_user_ioctl(struct kfdentry *__restrict self, kattr_t cmd, __user void *arg) {
 if (self->fd_type == KFDTYPE_FILE) return kfile_user_ioctl(self->fd_file,cmd,arg);
 return KE_NOSYS;
}
kerrno_t kfdentry_user_getattr(struct kfdentry *__restrict self, kattr_t attr,
                               __user void *__restrict buf, size_t bufsize,
                               __kernel size_t *__restrict reqsize) {
 switch (self->fd_type) {
  case KFDTYPE_FILE:  return kfile_user_getattr(self->fd_file,attr,buf,bufsize,reqsize);
  case KFDTYPE_TASK:  return ktask_user_getattr(self->fd_task,attr,buf,bufsize,reqsize);
  case KFDTYPE_PROC:  return kproc_user_getattr(self->fd_proc,attr,buf,bufsize,reqsize);
  case KFDTYPE_INODE: return kinode_user_getattr_legacy(self->fd_inode,attr,buf,bufsize,reqsize);
  {
   struct kinode *dirnode; kerrno_t error;
  case KFDTYPE_DIRENT:
   KTASK_CRIT_BEGIN
   if __unlikely((dirnode = kdirent_getnode(self->fd_dirent)) == NULL) error = KE_DESTROYED;
   else {
    error = kinode_user_getattr_legacy(dirnode,attr,buf,bufsize,reqsize);
    kinode_decref(dirnode);
   }
   KTASK_CRIT_END
   return error;
  }
  case KFDTYPE_DEVICE: return kdev_user_getattr(self->fd_dev,attr,buf,bufsize,reqsize);
  default: break;
 }
 return KE_NOSYS;
}
kerrno_t kfdentry_user_setattr(struct kfdentry *__restrict self, kattr_t attr,
                               __user void const *__restrict buf, size_t bufsize) {
 switch (self->fd_type) {
  case KFDTYPE_FILE:  return kfile_user_setattr(self->fd_file,attr,buf,bufsize);
  case KFDTYPE_TASK:  return ktask_user_setattr(self->fd_task,attr,buf,bufsize);
  case KFDTYPE_PROC:  return kproc_user_setattr(self->fd_proc,attr,buf,bufsize);
  case KFDTYPE_INODE: return kinode_user_setattr_legacy(self->fd_inode,attr,buf,bufsize);
  {
   struct kinode *dirnode; kerrno_t error;
  case KFDTYPE_DIRENT:
   KTASK_CRIT_BEGIN
   if __unlikely((dirnode = kdirent_getnode(self->fd_dirent)) == NULL) error = KE_DESTROYED;
   else {
    error = kinode_user_setattr_legacy(dirnode,attr,buf,bufsize);
    kinode_decref(dirnode);
   }
   KTASK_CRIT_END
   return error;
  }
  case KFDTYPE_DEVICE: return kdev_user_setattr(self->fd_dev,attr,buf,bufsize);
  default: break;
 }
 return KE_NOSYS;
}



kerrno_t kfdentry_seek(struct kfdentry *__restrict self, __off_t off,
                       int whence, __pos_t *__restrict newpos) {
 if (self->fd_type == KFDTYPE_FILE) return kfile_seek(self->fd_file,off,whence,newpos);
 return KE_NOSYS;
}
kerrno_t kfdentry_trunc(struct kfdentry *__restrict self, __pos_t size) {
 if (self->fd_type == KFDTYPE_FILE) return kfile_trunc(self->fd_file,size);
 return KE_NOSYS;
}
kerrno_t kfdentry_flush(struct kfdentry *__restrict self) {
 if (self->fd_type == KFDTYPE_FILE) return kfile_flush(self->fd_file);
 return KE_NOSYS;
}

kerrno_t kfdentry_terminate(struct kfdentry *__restrict self,
                            void *exitcode, ktaskopflag_t flags) {
 kerrno_t error; struct ktask *task;
 // NOTE: We have to use '_ktask_terminate' because otherwise a
 //       critical task could attempt to join itself in kernel space.
 // NOTE: Since syscalls are executed in critical blocks,
 //       if 'task' is actually the current user-task it will
 //       not be terminated until the critical block is left.
 // >> The idea of joining critical tasks can then be re-implemented in usercode.
 switch (self->fd_type) {
  case KFDTYPE_TASK:
   return ktask_terminateex(self->fd_task,exitcode,flags);
  case KFDTYPE_PROC:
   task = kproc_getroottask(self->fd_proc);
   if __unlikely(!task) return KE_DESTROYED;
   error = ktask_terminateex(task,exitcode,flags);
   ktask_decref(task);
   return error;
  default: break;
 }
 return KE_NOSYS;
}
kerrno_t kfdentry_suspend(struct kfdentry *__restrict self,
                          ktaskopflag_t flags) {
 kerrno_t error; struct ktask *task;
 switch (self->fd_type) {
  case KFDTYPE_TASK:
   return (flags&KTASKOPFLAG_RECURSIVE)
    ? ktask_suspend_r(self->fd_task)
    : ktask_suspend  (self->fd_task);
  case KFDTYPE_PROC:
   task = kproc_getroottask(self->fd_proc);
   if __unlikely(!task) return KE_DESTROYED;
   error = (flags&KTASKOPFLAG_RECURSIVE)
    ? ktask_suspend_r(task)
    : ktask_suspend  (task);
   ktask_decref(task);
   return error;
  default: break;
 }
 return KE_NOSYS;
}
kerrno_t kfdentry_resume(struct kfdentry *__restrict self,
                         ktaskopflag_t flags) {
 kerrno_t error; struct ktask *task;
 switch (self->fd_type) {
  case KFDTYPE_TASK:
   return (flags&KTASKOPFLAG_RECURSIVE)
    ? ktask_resume_r(self->fd_task)
    : ktask_resume  (self->fd_task);
  case KFDTYPE_PROC:
   task = kproc_getroottask(self->fd_proc);
   if __unlikely(!task) return KE_DESTROYED;
   error = (flags&KTASKOPFLAG_RECURSIVE)
    ? ktask_resume_r(task)
    : ktask_resume  (task);
   ktask_decref(task);
   return error;
  default: break;
 }
 return KE_NOSYS;
}

__crit kerrno_t kfdentry_openprocof(struct kfdentry *__restrict self,
                              __ref struct ktask **__restrict result) {
 KTASK_CRIT_MARK
 switch (self->fd_type) {
  case KFDTYPE_TASK:
   *result = ktask_procof(self->fd_task);
   return ktask_incref(*result);
  case KFDTYPE_PROC:
   *result = kproc_getroottask(self->fd_proc);
   if __unlikely(!*result) return KE_DESTROYED;
   return KE_OK;
  default: break;
 }
 return KE_NOSYS;
}
__crit kerrno_t kfdentry_openparent(struct kfdentry *__restrict self,
                              __ref struct ktask **__restrict result) {
 struct ktask *usedtask; kerrno_t error;
 KTASK_CRIT_MARK
 switch (self->fd_type) {
  case KFDTYPE_TASK:
   *result = ktask_parent(self->fd_task);
   return ktask_incref(*result);
  case KFDTYPE_PROC:
   usedtask = kproc_getroottask(self->fd_proc);
   if __unlikely(!usedtask) return KE_DESTROYED;
   *result = ktask_parent(usedtask);
   error = ktask_incref(*result);
   ktask_decref(usedtask);
   return error;
  default: break;
 }
 return KE_NOSYS;
}

__crit kerrno_t kfdentry_openctx(struct kfdentry *__restrict self,
                           __ref struct kproc**__restrict result) {
 KTASK_CRIT_MARK
 switch (self->fd_type) {
   if (0) {case KFDTYPE_TASK:    *result = ktask_getproc(self->fd_task); }
   if (0) {case KFDTYPE_PROC: *result = self->fd_proc; }
   return kproc_incref(*result);
  default: break;
 }
 return KE_NOSYS;
}

size_t kfdentry_getparid(struct kfdentry *__restrict self) {
 struct ktask *task; size_t result;
 switch (self->fd_type) {
  case KFDTYPE_TASK: return ktask_getparid(self->fd_task);
  case KFDTYPE_PROC:
   task = kproc_getroottask(self->fd_proc);
   if __likely(task) {
    result = ktask_getparid(task);
    ktask_decref(task);
    return result;
   }
   break;
  default: break;
 }
 return (size_t)-1;
}
size_t kfdentry_gettid(struct kfdentry *__restrict self) {
 struct ktask *task; size_t result;
 switch (self->fd_type) {
  case KFDTYPE_TASK: return ktask_gettid(self->fd_task);
  case KFDTYPE_PROC:
   task = kproc_getroottask(self->fd_proc);
   if __likely(task) {
    result = ktask_gettid(task);
    ktask_decref(task);
    return result;
   }
   break;
 }
 return (size_t)-1;
}

__crit kerrno_t kfdentry_opentask(struct kfdentry *__restrict self, size_t id,
                                  __ref struct ktask **__restrict result) {
 KTASK_CRIT_MARK
 // Open a child (task) / thread (taskctx)
 switch (self->fd_type) {
  case KFDTYPE_TASK:
   *result = ktask_getchild(self->fd_task,id);
   return *result ? KE_OK : KE_RANGE;
  case KFDTYPE_PROC:
   *result = kproc_getthread(self->fd_proc,id);
   return *result ? KE_OK : KE_RANGE;
  default: break;
 }
 return KE_NOSYS;
}

kerrno_t kfdentry_user_enumtasks(struct kfdentry *__restrict self,
                                 __user size_t *__restrict idv, size_t idc,
                                 __kernel size_t *__restrict reqidc) {
 kerrno_t error; __user size_t *iditer,*idend;
 struct ktask **iter,**end,*elem;
 idend = (iditer = idv)+idc;
 switch (self->fd_type) {
  {
   struct ktask *task;
  case KFDTYPE_TASK:
   task = self->fd_task;
   ktask_lock(task,KTASK_LOCK_CHILDREN);
   end = (iter = task->t_children.t_taskv)+task->t_children.t_taska;
   for (; iter != end; ++iter) {
    if ((elem = *iter) != NULL && katomic_load(elem->t_refcnt)) {
     if (iditer < idend) {
      if __unlikely(copy_to_user(iditer,&elem->t_parid,sizeof(size_t))) {
       error = KE_FAULT;
       goto end_task;
      }
     }
     ++iditer;
    }
   }
   error = KE_OK;
end_task: ktask_unlock(task,KTASK_LOCK_CHILDREN);
done:
   if (reqidc) *reqidc = (size_t)(iditer-idv);
   return error;
  }
  {
   struct kproc *ctx;
  case KFDTYPE_PROC:
   ctx = self->fd_proc;
   error = kproc_lock(ctx,KPROC_LOCK_THREADS);
   if __unlikely(KE_ISERR(error)) return error;
   end = (iter = ctx->p_threads.t_taskv)+ctx->p_threads.t_taska;
   for (; iter != end; ++iter) {
    if ((elem = *iter) != NULL && katomic_load(elem->t_refcnt)) {
     if (iditer < idend) {
      if __unlikely(copy_to_user(iditer,&elem->t_tid,sizeof(size_t))) {
       error = KE_FAULT;
       goto end_proc;
      }
     }
     ++iditer;
    }
   }
   error = KE_OK;
end_proc:
   kproc_unlock(ctx,KPROC_LOCK_THREADS);
   goto done;
  }
  default: break;
 }
 return KE_NOSYS;
}


kerrno_t kfdentry_getpriority(struct kfdentry *__restrict self,
                              ktaskprio_t *__restrict result) {
 struct ktask *task;
 switch (self->fd_type) {
  case KFDTYPE_TASK:
   *result = ktask_getpriority(self->fd_task);
   return KE_OK;
  case KFDTYPE_PROC:
   task = kproc_getroottask(self->fd_proc);
   if __likely(task) {
    *result = ktask_getpriority(task);
    ktask_decref(task);
    return KE_OK;
   }
   break;
 }
 return KE_NOSYS;
}
kerrno_t kfdentry_setpriority(struct kfdentry *__restrict self,
                              ktaskprio_t value) {
 struct ktask *task; kerrno_t error;
 switch (self->fd_type) {
  case KFDTYPE_TASK:
   return ktask_setpriority(self->fd_task,value);
  case KFDTYPE_PROC:
   task = kproc_getroottask(self->fd_proc);
   if __likely(task) {
    error = ktask_setpriority(task,value);
    ktask_decref(task);
    return error;
   }
   break;
 }
 return KE_NOSYS;
}

__DECL_END

#endif /* !__KOS_KERNEL_FDMAN_C_INL__ */

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
#ifndef __KOS_KERNEL_SCHED_PROC_FDMAN_C_INL__
#define __KOS_KERNEL_SCHED_PROC_FDMAN_C_INL__ 1

#include <kos/kernel/proc.h>
#include <kos/kernel/fd.h>
#include <kos/kernel/fs/file.h>
#include <sys/types.h>
#include <fcntl.h>

__DECL_BEGIN

static __crit void
kproc_free_unused_fd_memory(struct kproc *__restrict self) {
 struct kfdentry *iter,*begin;
 unsigned int new_alloc;
 KTASK_CRIT_MARK
 kassert_kproc(self);
 assert(kproc_islocked(self,KPROC_LOCK_FDMAN));
 iter = (begin = self->p_fdman.fdm_fdv)+self->p_fdman.fdm_fda;
 while (iter != begin && iter[-1].fd_type == KFDTYPE_NONE) --iter;
 new_alloc = (unsigned int)(iter-begin);
 assert((self->p_fdman.fdm_cnt != 0) == (new_alloc != 0));
 if (new_alloc != self->p_fdman.fdm_fda) {
  if (new_alloc) {
   begin = (struct kfdentry *)realloc(self->p_fdman.fdm_fdv,
                                      new_alloc*sizeof(struct kfdentry));
   if (begin) {
    self->p_fdman.fdm_fdv = begin;
    self->p_fdman.fdm_fda = new_alloc;
   }
  } else {
   free(self->p_fdman.fdm_fdv);
   self->p_fdman.fdm_fdv = NULL;
   self->p_fdman.fdm_fda = 0;
  }
 }
}

kerrno_t kproc_closefd(struct kproc *__restrict self, int fd) {
 kerrno_t error; struct kfdentry *slot,slotcopy;
 kassert_kproc(self);
 if __likely(KE_ISOK(error = kproc_lock(self,KPROC_LOCK_FDMAN))) {
  if __unlikely((slot = kfdman_slot(&self->p_fdman,fd)) == NULL ||
                (slot->fd_type == KFDTYPE_NONE)) {
   kproc_unlock(self,KPROC_LOCK_FDMAN);
   return KE_BADF;
  }
  slotcopy = *slot;
  slot->fd_type = KFDTYPE_NONE;
#ifdef __DEBUG__
  slot->fd_data = NULL;
#endif
  assert(self->p_fdman.fdm_cnt);
  --self->p_fdman.fdm_cnt;
  kproc_free_unused_fd_memory(self);
  kproc_unlock(self,KPROC_LOCK_FDMAN);
  // Drop the reference for the close file
  kfdentry_quit(&slotcopy);
 }
 return error;
}

unsigned int kproc_closeall(struct kproc *__restrict self, int low, int high) {
#define CLOSE(slot) \
do{ if ((slot)->fd_type != KFDTYPE_NONE) {\
    assert(self->p_fdman.fdm_cnt);\
    --self->p_fdman.fdm_cnt;\
    ++result;\
    slotcopy = *(slot);\
    (slot)->fd_type = KFDTYPE_NONE;\
    kproc_unlock(self,KPROC_LOCK_FDMAN);\
    kfdentry_quit(&slotcopy);\
    if __unlikely(KE_ISERR(kproc_lock(self,KPROC_LOCK_FDMAN))) goto end;\
} } while(0)
 unsigned int i,result = 0;
 struct kfdentry slotcopy;
 kassert_kproc(self);
 if __unlikely(high < low) return 0;
 if __unlikely(KE_ISERR(kproc_lock(self,KPROC_LOCK_FDMAN))) return 0;
 if (high >= 0) {
  unsigned int end_fd = (unsigned int)min(high,self->p_fdman.fdm_fda-1)+1u;
  assert(end_fd >= max(0,low));
  /* Close all regular descriptors in the given range. */
  for (i = (unsigned int)max(0,low); i != end_fd; ++i) {
   CLOSE(&self->p_fdman.fdm_fdv[i]);
  }
  if (result) kproc_free_unused_fd_memory(self);
 }
#define INRANGE(x) ((x) >= low && (x) <= high)
 /* Check for special descriptors. */
 if (INRANGE(KFD_ROOT)) CLOSE(&self->p_fdman.fdm_root);
 if (INRANGE(KFD_CWD)) CLOSE(&self->p_fdman.fdm_cwd);
#undef INRANGE
 kproc_unlock(self,KPROC_LOCK_FDMAN);
end:
#undef CLOSE
 return result;
}



void *kproc_getresourceaddr_nc(struct kproc const *__restrict self, int fd) {
 void *result; struct kfdentry *slot;
 kassert_kproc(self);
 switch (fd) {
  // Symbolic descriptors
  case KFD_TASKSELF: return (void *)ktask_self();
  case KFD_TASKROOT: return (void *)ktask_proc();
  case KFD_PROCSELF: return (void *)kproc_self();
  default: break;
 }
 KTASK_CRIT_BEGIN
 if __likely(KE_ISOK(kproc_lock((struct kproc *)self,KPROC_LOCK_FDMAN))) {
  slot = kfdman_slot((struct kfdman *)&self->p_fdman,fd);
  result = slot ? slot->fd_data : NULL;
  kproc_unlock((struct kproc *)self,KPROC_LOCK_FDMAN);
 } else {
  result = NULL;
 }
 KTASK_CRIT_END
 return result;
}
__crit void *kproc_getresourceaddr_c(struct kproc const *__restrict self, int fd) {
 void *result; struct kfdentry *slot;
 kassert_kproc(self);
 switch (fd) {
  // Symbolic descriptors
  case KFD_TASKSELF: return (void *)ktask_self();
  case KFD_TASKROOT: return (void *)ktask_proc();
  case KFD_PROCSELF: return (void *)kproc_self();
  default: break;
 }
 if __unlikely(KE_ISERR(kproc_lock((struct kproc *)self,KPROC_LOCK_FDMAN))) return NULL;
 slot = kfdman_slot((struct kfdman *)&self->p_fdman,fd);
 result = slot ? slot->fd_data : NULL;
 kproc_unlock((struct kproc *)self,KPROC_LOCK_FDMAN);
 return result;
}

__crit kerrno_t kproc_getfd(struct kproc const *__restrict self,
                            int fd, struct kfdentry *result) {
 struct kfdentry *slot; kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kproc(self);
 kassertobj(result);
 switch (fd) {
   if (0) { case KFD_TASKSELF: result->fd_task = ktask_self(); }
   if (0) { case KFD_TASKROOT: result->fd_task = ktask_proc(); }
   result->fd_attr = KFD_ATTR(KFDTYPE_TASK,KFD_FLAG_NONE);
   return ktask_incref(result->fd_task);
  case KFD_PROCSELF:
   result->fd_proc = kproc_self();
   result->fd_attr = KFD_ATTR(KFDTYPE_PROC,KFD_FLAG_NONE);
   return kproc_incref(result->fd_proc);
  default: break;
 }
 if __likely(KE_ISOK(error = kproc_lock((struct kproc *)self,KPROC_LOCK_FDMAN))) {
  assert(self->p_fdman.fdm_cnt <= self->p_fdman.fdm_fda);
  slot = kfdman_slot((struct kfdman *)&self->p_fdman,fd);
  if __unlikely(!slot) error = KE_BADF;
  else error = kfdentry_initcopy(result,slot);
  kproc_unlock((struct kproc *)self,KPROC_LOCK_FDMAN);
 }
 return error;
}

kerrno_t kproc_insfd_inherited(struct kproc *__restrict self, int *fd, struct kfdentry const *entry) {
 struct kfdentry *resfd,*newvec;
 kerrno_t error; size_t newalloc;
 kassert_kproc(self);
 kassertobj(fd);
 kassertobj(entry);
 assert(entry->fd_type != KFDTYPE_NONE);
 if __unlikely(KE_ISERR(error = kproc_lock(self,KPROC_LOCK_FDMAN))) return error;
 assert(self->p_fdman.fdm_cnt <= self->p_fdman.fdm_fda);
 // Check for free slots
 if (self->p_fdman.fdm_cnt != self->p_fdman.fdm_fda) for (;;) {
  assertf(self->p_fdman.fdm_fre < self->p_fdman.fdm_fda,
          "self->p_fdman.fdm_fre = %u\n"
          "self->p_fdman.fdm_fda = %u\n"
          "self->p_fdman.fdm_cnt = %u\n"
          ,self->p_fdman.fdm_fre
          ,self->p_fdman.fdm_fda
          ,self->p_fdman.fdm_cnt);
  // Check for the next free slot
  resfd = &self->p_fdman.fdm_fdv[self->p_fdman.fdm_fre];
  if (!resfd->fd_file) { *fd = self->p_fdman.fdm_fre; goto found; }
  if (++self->p_fdman.fdm_fre == self->p_fdman.fdm_fda) self->p_fdman.fdm_fre = 0;
 }
 // Make sure that the caller has
 // permissions to allocate a new file.
 if (self->p_fdman.fdm_cnt == self->p_fdman.fdm_max) {
  // Too many open file descriptors
  error = KE_MFILE;
  goto end;
 }
 assert(self->p_fdman.fdm_cnt < self->p_fdman.fdm_max);
 // Must allocate a new slot
 newalloc = self->p_fdman.fdm_fda ? self->p_fdman.fdm_fda*2 : 2;
 if (newalloc > self->p_fdman.fdm_max) newalloc = self->p_fdman.fdm_max;
 assert(newalloc != self->p_fdman.fdm_fda);
 newvec = (struct kfdentry *)realloc(self->p_fdman.fdm_fdv,
                                     newalloc*sizeof(struct kfdentry));
 if __unlikely(!newvec) { error = KE_NOMEM; goto end; }
 // Initialize the newly allocated area to zero
 memset(newvec+self->p_fdman.fdm_fda,0,
        (newalloc-self->p_fdman.fdm_fda)*sizeof(struct kfdentry));
 self->p_fdman.fdm_fdv = newvec;
 // NOTE: 'fdm_fre' may point out-of-bounds if the vector if full
 self->p_fdman.fdm_fre = (*fd = self->p_fdman.fdm_fda)+1;
 self->p_fdman.fdm_fda = newalloc;
 resfd = newvec+*fd;
found:
 ++self->p_fdman.fdm_cnt;
 *resfd = *entry; // Inherit data
end:
 kproc_unlock(self,KPROC_LOCK_FDMAN);
 return error;
}

kerrno_t kproc_insfdat_inherited(struct kproc *__restrict self, int fd, struct kfdentry const *entry) {
 kerrno_t error;
 struct kfdentry *resfd,*newvec,oldfd;
 kassert_kproc(self);
 kassertobj(entry);
 assert(entry->fd_type != KFDTYPE_NONE);
 if __unlikely(KE_ISERR(error = kproc_lock(self,KPROC_LOCK_FDMAN))) return error;
 assert(self->p_fdman.fdm_cnt <= self->p_fdman.fdm_fda);
 if __unlikely(fd < 0) {
  // Handle special slots
  resfd = kfdman_specialslot(&self->p_fdman,fd);
  if __unlikely(!resfd) error = KE_BADF;
  else {
   oldfd = *resfd,*resfd = *entry;
   if (oldfd.fd_type != KFDTYPE_NONE) goto unlock_and_drop_old;
   error = KE_OK;
  }
  goto end;
 }
 if __unlikely((unsigned int)fd >= self->p_fdman.fdm_max) {
  // Don't allow descriptor numbers above the maximum
  error = KE_MFILE;
  goto end;
 }
 if ((unsigned int)fd < self->p_fdman.fdm_fda) {
  resfd = &self->p_fdman.fdm_fdv[fd];
  if (resfd->fd_file) {
   oldfd = *resfd;
   *resfd = *entry; // Inherit data
unlock_and_drop_old:
   // Slot already in use (override)
   kproc_unlock(self,KPROC_LOCK_FDMAN);
   // Drop the reference previously held by the fd entry
   kfdentry_quit(&oldfd);
   return KS_FOUND;
  } else {
   ++self->p_fdman.fdm_cnt;
  }
 } else {
  // Allocate a new slot
  newvec = (struct kfdentry *)realloc(self->p_fdman.fdm_fdv,
                                     (fd+1)*sizeof(struct kfdentry));
  if __unlikely(!newvec) { error = KE_NOMEM; goto end; }
  memset(newvec+self->p_fdman.fdm_fda,0,
        (fd-self->p_fdman.fdm_fda)*sizeof(struct kfdentry));
  self->p_fdman.fdm_fdv = newvec;
  self->p_fdman.fdm_fda = fd+1;
  resfd = &newvec[fd];
  ++self->p_fdman.fdm_cnt;
 }
 *resfd = *entry; // Inherit data
 error = KE_OK;
end:
 kproc_unlock(self,KPROC_LOCK_FDMAN);
 return error;
}
kerrno_t kproc_insfdh_inherited(struct kproc *__restrict self, int hint,
                                int *fd, struct kfdentry const *entry) {
 struct kfdentry *resfd,*newvec,*end; kerrno_t error;
 kassert_kproc(self);
 kassertobj(fd);
 kassertobj(entry);
 assert(entry->fd_type != KFDTYPE_NONE);
 if __unlikely(hint < 0) hint = 0;
 if __unlikely(KE_ISERR(error = kproc_lock(self,KPROC_LOCK_FDMAN))) return error;
 assert(self->p_fdman.fdm_cnt <= self->p_fdman.fdm_fda);
 if ((unsigned int)hint < self->p_fdman.fdm_fda) {
  // Hinted slot is already allocated (search for nearest free slot)
  resfd = &self->p_fdman.fdm_fdv[hint];
  end = self->p_fdman.fdm_fdv+self->p_fdman.fdm_fda;
  assert(resfd < end);
  for (; resfd != end; ++resfd) {
   if (resfd->fd_type == KFDTYPE_NONE) goto found; // Closest free slot!
  }
 }
 // Must allocate hinted slot at the end
 hint = self->p_fdman.fdm_fda;
 // Make sure the hinted index is even allowed
 if ((unsigned int)hint >= self->p_fdman.fdm_max) { error = KE_MFILE; goto end; }
 newvec = (struct kfdentry *)realloc(self->p_fdman.fdm_fdv,
                                    (hint+1)*sizeof(struct kfdentry));
 if __unlikely(!newvec) { error = KE_NOMEM; goto end; }
 memset(newvec+self->p_fdman.fdm_fda,0,
       (hint-self->p_fdman.fdm_fda)*sizeof(struct kfdentry));
 self->p_fdman.fdm_fdv = newvec;
 self->p_fdman.fdm_fda = hint+1;
 resfd = &newvec[hint];
found:
 ++self->p_fdman.fdm_cnt;
 *resfd = *entry; // Inherit data
 *fd = (int)(resfd-self->p_fdman.fdm_fdv);
end:
 kproc_unlock(self,KPROC_LOCK_FDMAN);
 return error;
}


__crit kerrno_t kproc_dupfd_c(struct kproc *__restrict self, int oldfd, int *newfd, kfdflag_t flags) {
 struct kfdentry oldfdentry; kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kproc(self); kassertobj(newfd);
 if __unlikely(KE_ISERR(error = kproc_getfd(self,oldfd,&oldfdentry))) return error;
 oldfdentry.fd_flag = flags;
 error = kproc_insfd_inherited(self,newfd,&oldfdentry);
 if __unlikely(KE_ISERR(error)) kfdentry_quit(&oldfdentry);
 return error;
}
__crit kerrno_t kproc_dupfd2_c(struct kproc *__restrict self, int oldfd, int newfd, kfdflag_t flags) {
 struct kfdentry oldfdentry; kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kproc(self);
 if __unlikely(KE_ISERR(error = kproc_getfd(self,oldfd,&oldfdentry))) return error;
 oldfdentry.fd_flag = flags;
 error = kproc_insfdat_inherited(self,newfd,&oldfdentry);
 if __unlikely(KE_ISERR(error)) kfdentry_quit(&oldfdentry);
 return error;
}
__crit kerrno_t kproc_dupfdh_c(struct kproc *__restrict self, int oldfd, int hintfd, int *newfd, kfdflag_t flags) {
 struct kfdentry oldfdentry; kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kproc(self);
 if __unlikely(KE_ISERR(error = kproc_getfd(self,oldfd,&oldfdentry))) return error;
 oldfdentry.fd_flag = flags;
 error = kproc_insfdh_inherited(self,hintfd,newfd,&oldfdentry);
 if __unlikely(KE_ISERR(error)) kfdentry_quit(&oldfdentry);
 return error;
}



__crit kerrno_t
kproc_user_fcntlfd_c(struct kproc *__restrict self, int fd,
                     int cmd, __user void *arg) {
 kerrno_t error;
 kassert_kproc(self);
 switch (cmd) {
  {
   int resfd;
   if (0) {case F_DUPFD:         error = kproc_dupfdh_c(self,fd,(int)(uintptr_t)arg,&resfd,0); }
   if (0) {case F_DUPFD_CLOEXEC: error = kproc_dupfdh_c(self,fd,(int)(uintptr_t)arg,&resfd,FD_CLOEXEC); }
   return KE_ISERR(error) ? error : (kerrno_t)resfd;
   break;
  }
  {
   kfdflag_t result;
  case F_GETFD:
   if __unlikely(KE_ISERR(error = kproc_lock(self,KPROC_LOCK_FDMAN))) return error;
   error = kfdman_getflags(&self->p_fdman,fd,&result);
   kproc_unlock(self,KPROC_LOCK_FDMAN);
   return KE_ISERR(error) ? error : (kerrno_t)(int)result;
  }
  case F_SETFD:
   if __unlikely(KE_ISERR(error = kproc_lock(self,KPROC_LOCK_FDMAN))) return error;
   error = kfdman_setflags(&self->p_fdman,fd,(kfdflag_t)(int)(uintptr_t)arg);
   kproc_unlock(self,KPROC_LOCK_FDMAN);
   return error;
  case _F_GETYPE:
   if __unlikely(KE_ISERR(error = kproc_lock(self,KPROC_LOCK_FDMAN))) return error;
   if (fd < 0) switch (fd) {
    case KFD_ROOT    : error = KFDTYPE_FILE; break;
    case KFD_CWD     : error = KFDTYPE_FILE; break;
    case KFD_TASKSELF: error = KFDTYPE_TASK; break;
    case KFD_TASKROOT: error = KFDTYPE_TASK; break;
    case KFD_PROCSELF: error = KFDTYPE_PROC; break;
    default          : error = KE_BADF; break;
   } else if ((unsigned int)fd >= self->p_fdman.fdm_fda) error = KE_BADF;
   else if ((error = self->p_fdman.fdm_fdv[fd].fd_type) == 0) error = KE_BADF;
   kproc_unlock(self,KPROC_LOCK_FDMAN);
   return error;
  default: break;
 }
 return KE_NOSYS;
}





__crit __ref struct kfile *kproc_getfdfile(struct kproc *__restrict self, int fd) {
 struct kfdentry fdentry; KTASK_CRIT_MARK
 if __unlikely(KE_ISERR(kproc_getfd(self,fd,&fdentry))) return NULL;
 if (fdentry.fd_type == KFDTYPE_FILE) return fdentry.fd_file; /* Inherit reference. */
 kfdentry_quit(&fdentry);
 return NULL;
}
__crit __ref struct ktask *kproc_getfdtask(struct kproc *__restrict self, int fd) {
 struct ktask *result; struct kfdentry fdentry; KTASK_CRIT_MARK
 if __unlikely(KE_ISERR(kproc_getfd(self,fd,&fdentry))) return NULL;
 if (fdentry.fd_type == KFDTYPE_TASK) return fdentry.fd_task; /* Inherit reference. */
 if (fdentry.fd_type == KFDTYPE_PROC) {
  result = kproc_getroottask(fdentry.fd_proc);
  kproc_decref(fdentry.fd_proc);
  return result;
 }
 kfdentry_quit(&fdentry);
 return NULL;
}
__crit __ref struct kinode *kproc_getfdinode(struct kproc *__restrict self, int fd) {
 struct kfdentry fdentry; struct kinode *result; KTASK_CRIT_MARK
 if __unlikely(KE_ISERR(kproc_getfd(self,fd,&fdentry))) return NULL;
 if (fdentry.fd_type == KFDTYPE_INODE) return fdentry.fd_inode; /* Inherit reference. */
      if (fdentry.fd_type == KFDTYPE_FILE) result = kfile_getinode(fdentry.fd_file);
 else if (fdentry.fd_type == KFDTYPE_DIRENT) result = kdirent_getnode(fdentry.fd_dirent);
 else result = NULL;
 kfdentry_quit(&fdentry);
 return result;
}
__crit __ref struct kdirent *kproc_getfddirent(struct kproc *__restrict self, int fd) {
 struct kfdentry fdentry; struct kdirent *result; KTASK_CRIT_MARK
 if __unlikely(KE_ISERR(kproc_getfd(self,fd,&fdentry))) return NULL;
 if (fdentry.fd_type == KFDTYPE_DIRENT) return fdentry.fd_dirent; /* Inherit reference. */
 if (fdentry.fd_type == KFDTYPE_FILE) result = kfile_getdirent(fdentry.fd_file);
 else result = NULL;
 kfdentry_quit(&fdentry);
 return result;
}
__crit __ref struct kproc *kproc_getfdproc(struct kproc *__restrict self, int fd) {
 struct kfdentry fdentry; struct kproc *result; KTASK_CRIT_MARK
 if __unlikely(KE_ISERR(kproc_getfd(self,fd,&fdentry))) return NULL;
 if (fdentry.fd_type == KFDTYPE_PROC) return fdentry.fd_proc; /* Inherit reference. */
 if (fdentry.fd_type == KFDTYPE_TASK) {
  result = ktask_getproc(fdentry.fd_task);
  if __unlikely(KE_ISERR(kproc_incref(result))) result = NULL;
 } else result = NULL;
 kfdentry_quit(&fdentry);
 return result;
}

__crit __ref struct ktask *kproc_getroottask(struct kproc const *__restrict self) {
 struct ktask *result; struct ktask **siter,**send; KTASK_CRIT_MARK
 if __unlikely(KE_ISERR(kproc_lock((struct kproc *)self,KPROC_LOCK_THREADS))) return NULL;
 send = (siter = self->p_threads.t_taskv)+self->p_threads.t_taska;
 if (siter == send) result = NULL;
 else do if ((result = *siter) != NULL) {
  if __likely(KE_ISOK(ktask_incref(result))) break;
 } while (++siter != send);
 kproc_unlock((struct kproc *)self,KPROC_LOCK_THREADS);
 return result;
}
__crit int kproc_isrootvisible_c(struct kproc const *__restrict self) {
 int result; struct ktask *task; struct ktask **siter,**send; KTASK_CRIT_MARK
 if __unlikely(KE_ISERR(kproc_lock((struct kproc *)self,KPROC_LOCK_THREADS))) return 0;
 send = (siter = self->p_threads.t_taskv)+self->p_threads.t_taska;
 if (siter == send);
 else do if ((task = *siter) != NULL && katomic_load(task->t_refcnt)) {
  result = ktask_accessgp(task);
  goto end;
 } while (++siter != send);
 result = 0;
end:
 kproc_unlock((struct kproc *)self,KPROC_LOCK_THREADS);
 return result;
}


__crit kerrno_t
kproc_user_readfd_c(struct kproc *__restrict self, int fd,
                    __user void *__restrict buf, size_t bufsize,
                    __kernel size_t *__restrict rsize) {
 kerrno_t error; struct kfdentry fdentry;
 KTASK_CRIT_MARK
 if __likely(KE_ISOK(error = kproc_getfd(self,fd,&fdentry))) {
  error = kfdentry_user_read(&fdentry,buf,bufsize,rsize);
  kfdentry_quit(&fdentry);
 }
 return error;
}
__crit kerrno_t
kproc_user_writefd_c(struct kproc *__restrict self, int fd,
                     __user void const *__restrict buf, size_t bufsize,
                     __kernel size_t *__restrict wsize) {
 kerrno_t error; struct kfdentry fdentry;
 KTASK_CRIT_MARK
 if __likely(KE_ISOK(error = kproc_getfd(self,fd,&fdentry))) {
  error = kfdentry_user_write(&fdentry,buf,bufsize,wsize);
  kfdentry_quit(&fdentry);
 }
 return error;
}
__crit kerrno_t
kproc_user_preadfd_c(struct kproc *__restrict self, int fd, pos_t pos,
                     __user void *__restrict buf, size_t bufsize,
                     __kernel size_t *__restrict rsize) {
 kerrno_t error; struct kfdentry fdentry;
 KTASK_CRIT_MARK
 if __likely(KE_ISOK(error = kproc_getfd(self,fd,&fdentry))) {
  error = kfdentry_user_pread(&fdentry,pos,buf,bufsize,rsize);
  kfdentry_quit(&fdentry);
 }
 return error;
}
__crit kerrno_t
kproc_user_pwritefd_c(struct kproc *__restrict self, int fd, pos_t pos,
                      __user void const *__restrict buf, size_t bufsize,
                      __kernel size_t *__restrict wsize) {
 kerrno_t error; struct kfdentry fdentry;
 KTASK_CRIT_MARK
 if __likely(KE_ISOK(error = kproc_getfd(self,fd,&fdentry))) {
  error = kfdentry_user_pwrite(&fdentry,pos,buf,bufsize,wsize);
  kfdentry_quit(&fdentry);
 }
 return error;
}
__crit kerrno_t
kproc_user_ioctlfd_c(struct kproc *__restrict self, int fd,
                     kattr_t cmd, __user void *arg) {
 kerrno_t error; struct kfdentry fdentry;
 KTASK_CRIT_MARK
 if __likely(KE_ISOK(error = kproc_getfd(self,fd,&fdentry))) {
  error = kfdentry_user_ioctl(&fdentry,cmd,arg);
  kfdentry_quit(&fdentry);
 }
 return error;
}
__crit kerrno_t
kproc_user_getattrfd_c(struct kproc *__restrict self, int fd, kattr_t attr,
                       __user void *__restrict buf, size_t bufsize,
                       __kernel size_t *__restrict reqsize) {
 kerrno_t error; struct kfdentry fdentry;
 KTASK_CRIT_MARK
 if __likely(KE_ISOK(error = kproc_getfd(self,fd,&fdentry))) {
  error = kfdentry_user_getattr(&fdentry,attr,buf,bufsize,reqsize);
  kfdentry_quit(&fdentry);
 }
 return error;
}
__crit kerrno_t
kproc_user_setattrfd_c(struct kproc *__restrict self, int fd, kattr_t attr,
                       __user void const *__restrict buf, size_t bufsize) {
 kerrno_t error; struct kfdentry fdentry;
 KTASK_CRIT_MARK
 if __likely(KE_ISOK(error = kproc_getfd(self,fd,&fdentry))) {
  error = kfdentry_user_setattr(&fdentry,attr,buf,bufsize);
  kfdentry_quit(&fdentry);
 }
 return error;
}


__crit kerrno_t
kproc_seekfd_c(struct kproc *__restrict self, int fd, __off_t off,
               int whence, __kernel pos_t *__restrict newpos) {
 kerrno_t error; struct kfdentry fdentry;
 KTASK_CRIT_MARK
 if __likely(KE_ISOK(error = kproc_getfd(self,fd,&fdentry))) {
  error = kfdentry_seek(&fdentry,off,whence,newpos);
  kfdentry_quit(&fdentry);
 }
 return error;
}
__crit kerrno_t
kproc_truncfd_c(struct kproc *__restrict self, int fd, pos_t size) {
 kerrno_t error; struct kfdentry fdentry;
 KTASK_CRIT_MARK
 if __likely(KE_ISOK(error = kproc_getfd(self,fd,&fdentry))) {
  error = kfdentry_trunc(&fdentry,size);
  kfdentry_quit(&fdentry);
 }
 return error;
}
__crit kerrno_t
kproc_flushfd_c(struct kproc *__restrict self, int fd) {
 kerrno_t error; struct kfdentry fdentry;
 KTASK_CRIT_MARK
 if __likely(KE_ISOK(error = kproc_getfd(self,fd,&fdentry))) {
  error = kfdentry_flush(&fdentry);
  kfdentry_quit(&fdentry);
 }
 return error;
}
__crit kerrno_t
kproc_readdirfd(struct kproc *__restrict self, int fd,
                __ref struct kinode **__restrict inode,
                struct kdirentname **__restrict name,
                __ref struct kfile **__restrict fp,
                __u32 flags) {
 kerrno_t error; struct kfdentry fdentry;
 KTASK_CRIT_MARK
 kassert_kproc(self);
 kassertobj(inode);
 kassertobj(name);
 kassertobj(fp);
 if __likely(KE_ISOK(error = kproc_getfd(self,fd,&fdentry))) {
  if (fdentry.fd_type == KFDTYPE_FILE) {
   error = kfile_readdir(fdentry.fd_file,inode,name,flags);
   if __likely(error == KE_OK) {
    *fp = fdentry.fd_file; /* Inherit reference. */
    return error;
   }
  } else {
   error = KE_NOSYS;
  }
  kfdentry_quit(&fdentry);
 }
 return error;
}

__DECL_END

#endif /* !__KOS_KERNEL_SCHED_PROC_FDMAN_C_INL__ */

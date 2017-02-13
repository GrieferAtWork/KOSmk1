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
#ifndef __KOS_KERNEL_FS_VFS_PROC_C__
#define __KOS_KERNEL_FS_VFS_PROC_C__ 1

#include <ctype.h>
#include <itos.h>
#include <kos/kernel/fs/fs-dirfile.h>
#include <kos/kernel/fs/vfs-proc.h>
#include <kos/kernel/fs/vfs.h>
#include <kos/kernel/proc.h>
#include <kos/kernel/task.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <kos/fd.h>
#include <kos/kernel/linker.h>

__DECL_BEGIN

//////////////////////////////////////////////////////////////////////////
// Generic /proc/[PID] folder.
__crit __ref struct kvinodepiddir *
kvinodepiddir_new_inherited(struct kvprocdirent *__restrict files,
                            __ref struct kproc *__restrict proc) {
 __ref struct kvinodepiddir *result;
 assert(files);
 kassert_kproc(proc);
 result = (__ref struct kvinodepiddir *)__kinode_alloc((struct ksuperblock *)&kvfs_proc,
                                                       &kvinodedirtype_proc_pid,
                                                       &kdirfile_type,
                                                       S_IFDIR);
 if __unlikely(!result) return NULL;
 result->pd_file.pf_proc = proc; /* Inherit reference */
 result->pd_elem         = files;
 return result;
}

__crit __ref struct kvinodepidfile *
kvinodepidfile_new_inherited(struct kinodetype *__restrict type,
                             __ref struct kproc *__restrict proc,
                             struct kfiletype *__restrict file_type,
                             __mode_t mode) {
 __ref struct kvinodepidfile *result;
 kassertobj(type);
 kassertobj(file_type);
 kassert_kproc(proc);
 result = (__ref struct kvinodepidfile *)__kinode_alloc((struct ksuperblock *)&kvfs_proc,
                                                        type,file_type,mode);
 if __unlikely(!result) return NULL;
 result->pf_proc = proc; /* Inherit reference */
 return result;
}

__crit __ref struct kvinodepidfile *
kvinodepid_new_inherited(struct kvprocdirent const *__restrict decl,
                         __ref struct kproc *__restrict proc) {
 __ref struct kvinodepidfile *result;
 kassertobj(decl);
 kassert_kproc(proc);
 if (S_ISDIR(decl->pd_mode)) {
  /* Create a new directory node. */
  return (__ref struct kvinodepidfile *)kvinodepiddir_new_inherited(decl->pd_subd,proc);
 }
 assertf(decl->pd_type != &kvinodedirtype_proc_pid,
         "Non-directory-mode node declared with directory node type.");
 result = (__ref struct kvinodepidfile *)__kinode_alloc((struct ksuperblock *)&kvfs_proc,
                                                        decl->pd_type,decl->pd_file,
                                                        decl->pd_mode);
 if __unlikely(!result) return NULL;
 result->pf_proc = proc; /* Inherit reference */
 return result;
}



/* General purpose accessor macros. */
#define SELF      (self)
#define SELF_FILE ((struct kvinodepidfile *)SELF)
#define SELF_DIR  ((struct kvinodepiddir *)SELF)
#define PROC      (SELF_FILE->pf_proc)


//////////////////////////////////////////////////////////////////////////
// Generic functions for all "/proc/[PID]" files/folders.
void kvinodepid_generic_quit(struct kinode *self) { kproc_decref(PROC); }
kerrno_t
kvinodepid_generic_getattr(struct kinode const *self,
                                 __size_t ac, union kinodeattr *av) {
 kerrno_t error;
 for (; ac; ++av,--ac) switch (av->ia_common.a_id) {
#if KTASK_HAVE_STATS_FEATURE(KTASK_HAVE_STATS_START)
  {
   struct ktask *root_task;
  case KATTR_FS_ATIME:
  case KATTR_FS_MTIME:
#if 0
   /* TODO: access/modification time should be when any
    *       thread of the process was last scheduled. */
   break;
#endif
  case KATTR_FS_CTIME:
   /* creation time should be when the process root was spawned. */
   if __unlikely((root_task = kproc_getroottask(PROC)) == NULL) goto default_attrib;
   memcpy(&av->ia_time.tm_time,
          &root_task->t_stats.ts_abstime_start,
          sizeof(struct timespec));
   ktask_decref(root_task);
  } break;
#endif /* KTASK_HAVE_STATS_START */
  default:default_attrib:
   error = kinode_generic_getattr(self,1,av);
   if __unlikely(KE_ISERR(error)) return error;
   break;
 }
 return KE_OK;
}

//////////////////////////////////////////////////////////////////////////
// Generic functions for all "/proc/[PID]" folders.
kerrno_t
kvinodepiddir_generic_walk(struct kinode *self,
                           struct kdirentname const *name,
                           __ref struct kinode **resnode) {
 __ref struct kvinodepidfile *node; kerrno_t error;
 struct kvprocdirent *iter = SELF_DIR->pd_elem;
 while (!kdirentname_isempty(&iter->pd_name)) {
  if (kdirentname_lazyequal(&iter->pd_name,name)) {
   struct kproc *proc = PROC; /* Found it! */
   if __unlikely(KE_ISERR(error = kproc_incref(proc))) return error;
   node = kvinodepid_new_inherited(iter,proc);
   if __unlikely(!node) { kproc_decref(proc); return KE_NOMEM; }
   *resnode = (__ref struct kinode *)node;
   return KE_OK;
  }
  ++iter;
 }
 return KE_NOENT;
}
kerrno_t
kvinodepiddir_generic_enumdir(struct kinode *self,
                              pkenumdir callback,
                              void *closure) {
 __ref struct kvinodepidfile *node;
 kerrno_t error;
 struct kvprocdirent *iter = SELF_DIR->pd_elem;
 while (!kdirentname_isempty(&iter->pd_name)) {
  struct kproc *proc = PROC;
  if __unlikely(KE_ISERR(error = kproc_incref(proc))) return error;
  node = kvinodepid_new_inherited(iter,proc);
  if __unlikely(!node) { kproc_decref(proc); return KE_NOMEM; }
  error = (*callback)((struct kinode *)node,&iter->pd_name,closure);
  kinode_decref((struct kinode *)node);
  if __unlikely(error != KE_OK) return error;
  ++iter;
 }
 return KE_OK;
}

struct kinodetype kvinodedirtype_proc_pid = {
 .it_size    = sizeof(struct kvinodepiddir),
 .it_quit    = &kvinodepid_generic_quit,
 .it_getattr = &kvinodepid_generic_getattr,
 .it_walk    = &kvinodepiddir_generic_walk,
 .it_enumdir = &kvinodepiddir_generic_enumdir,
};






//////////////////////////////////////////////////////////////////////////
// "/proc/[PID]/cwd|root" special symbolic link files
static kerrno_t
procpid_fdentry_readlink(struct kinode *self,
                         struct kfdentry *fdentry,
                         struct kdirentname *target) {
 kerrno_t error; char buffer[PATH_MAX];
 char *newdynbuf,*dynbuf = buffer;
 size_t reqbufsize,bufsize = sizeof(buffer);
getattr_again:
 error = kfdentry_getattr(fdentry,KATTR_FS_PATHNAME,dynbuf,bufsize,&reqbufsize);
 if __unlikely(KE_ISERR(error)) goto end_buf;
 if (reqbufsize > bufsize) {
  newdynbuf = (char *)((dynbuf == buffer) ? realloc(dynbuf,reqbufsize) : malloc(reqbufsize));
  if __unlikely(!newdynbuf) { error = KE_NOMEM; goto end_buf; }
  dynbuf = newdynbuf;
  bufsize = reqbufsize;
  goto getattr_again;
 }
 target->dn_size = (reqbufsize/sizeof(char))-1;
 if (dynbuf == buffer) {
  target->dn_name = (char *)memdup(buffer,reqbufsize);
  if __unlikely(!target->dn_name) return KE_NOMEM;
 } else {
  target->dn_name = dynbuf;
  dynbuf = NULL;
 }
 kdirentname_refreshhash(target);
end_buf:
 if (dynbuf != buffer) free(dynbuf);
 return error;
}
static kerrno_t
procpid_fd_readlink(struct kinode *self, int fd,
                    struct kdirentname *target) {
 kerrno_t error; struct kfdentry fdentry;
 if __unlikely(KE_ISERR(error = kproc_getfd(PROC,fd,&fdentry))) return error;
 error = procpid_fdentry_readlink(self,&fdentry,target);
 kfdentry_quit(&fdentry);
 return error;
}
static kerrno_t vfsfile_proc_pid_cwd_readlink(struct kinode *self, struct kdirentname *target) { return procpid_fd_readlink(self,KFD_CWD,target); }
static kerrno_t vfsfile_proc_pid_root_readlink(struct kinode *self, struct kdirentname *target) { return procpid_fd_readlink(self,KFD_ROOT,target); }
static kerrno_t vfsfile_proc_pid_exe_readlink(struct kinode *self, struct kdirentname *target) {
 __ref struct kshlib *root_exe; struct kfdentry entry; kerrno_t error;
 if __unlikely((root_exe = kproc_getrootexe(PROC)) == NULL) return KE_DESTROYED;
 entry.fd_attr = KFD_ATTR(KFDTYPE_FILE,KFD_FLAG_NONE);
 entry.fd_file = root_exe->sh_file;
 error = procpid_fdentry_readlink(self,&entry,target);
 kshlib_decref(root_exe);
 return error;
}
struct kinodetype kvinodetype_proc_pid_cwd  = { KVINODEPIDFILETYPE_INIT .it_readlink = &vfsfile_proc_pid_cwd_readlink };
struct kinodetype kvinodetype_proc_pid_exe  = { KVINODEPIDFILETYPE_INIT .it_readlink = &vfsfile_proc_pid_exe_readlink };
struct kinodetype kvinodetype_proc_pid_root = { KVINODEPIDFILETYPE_INIT .it_readlink = &vfsfile_proc_pid_root_readlink };






#undef PROC
#undef SELF_DIR
#undef SELF_FILE
#undef SELF



//////////////////////////////////////////////////////////////////////////
// "/proc/[PID]/" Virtual directory contents.
struct kvprocdirent vfsent_proc_pid[] = {
 KVPROCDIRENT_INIT_FILE("cwd", &kvinodetype_proc_pid_cwd,&kvfile_empty_type,S_IFLNK),
 KVPROCDIRENT_INIT_FILE("exe", &kvinodetype_proc_pid_exe,&kvfile_empty_type,S_IFLNK),
 KVPROCDIRENT_INIT_FILE("root",&kvinodetype_proc_pid_root,&kvfile_empty_type,S_IFLNK),
 KVPROCDIRENT_INIT_SENTINAL
};





//////////////////////////////////////////////////////////////////////////
// "/proc/self" Symbolic link
extern struct kinodetype kvinodetype_proc_self;
static kerrno_t
proc_self_readlink(struct kinode *__unused(self),
                   struct kdirentname *target) {
 char buf[16];
 pid_t pid = kproc_getpid(kproc_self());
 target->dn_size = itos(buf,sizeof(buf),pid);
 assert(target->dn_size != 0);
 target->dn_name = (char *)malloc((target->dn_size+1)*sizeof(char));
 if __unlikely(!target->dn_name) return KE_NOMEM;
 memcpy(target->dn_name,buf,(target->dn_size+1)*sizeof(char));
 kdirentname_refreshhash(target);
 return KE_OK;
}
struct kinodetype kvinodetype_proc_self = {
 .it_size     = sizeof(struct kinode),
 .it_readlink = &proc_self_readlink,
};




//////////////////////////////////////////////////////////////////////////
// "/proc" Filesystem superblock
extern struct kvsdirsuperblock kvfs_proc;
extern struct ksuperblocktype kvfsproc_type;
static struct kvsdirent vfsent_proc[] = {
 KVSDIRENT_INIT("self",&kvinode_proc_self),
 KVSDIRENT_INIT_SENTINAL
};

static kerrno_t
kfsproc_walk(struct kinode *self,
             struct kdirentname const *name,
             __ref struct kinode **resnode) {
 char const *iter,*end; pid_t pid;
 __ref struct kproc *proc;
 /* Check if 'name' is an integer matching a visible & valid PID. */
 end = (iter = name->dn_name)+name->dn_size;
 if __unlikely(!name->dn_size) goto not_a_number;
 for (; iter != end; ++iter) {
  if (!isdigit(*iter)) goto not_a_number;
 }
 pid = strntos32(name->dn_name,name->dn_size,NULL,0);
 if __unlikely((proc = kproclist_getproc(pid)) != NULL) {
  struct kinode *result = (struct kinode *)kvinodepiddir_new_inherited(vfsent_proc_pid,proc);
  if __unlikely(!result) { kproc_decref(proc); return KE_NOMEM; }
  *resnode = result;
  return KE_OK;
 }
not_a_number:
 return kvsdirnode_walk(self,name,resnode);
}

#define yield(node,name) \
do if __unlikely((error = (*callback)(name,name,closure)) != KE_OK) return error;\
while(0)

static kerrno_t
kfsproc_enumdir(struct kinode *self,
                pkenumdir callback,
                void *closure) {
 kerrno_t error;
 char pidnamebuf[16];
 struct kdirentname entname;
 struct kproc *buffer[64];
 struct kproc **iter,**end,**newdynbuf,**dynbuf = buffer;
 __ref struct kvinodepiddir *iternode;
 size_t reqbufsize,bufsize = __compiler_ARRAYSIZE(buffer);
again_enumproc:
 reqbufsize = kproclist_enumproc(dynbuf,bufsize);
 if (reqbufsize > bufsize) {
  newdynbuf = (dynbuf == buffer)
   ? (struct kproc **)malloc(reqbufsize*sizeof(struct kproc *))
   : (struct kproc **)realloc(dynbuf,reqbufsize*sizeof(struct kproc *));
  if __unlikely(!newdynbuf) { error = KE_NOMEM; goto err_procsmem; }
  dynbuf = newdynbuf;
  bufsize = reqbufsize;
  goto again_enumproc;
 }
 end = (iter = dynbuf)+reqbufsize;
 entname.dn_name = pidnamebuf;
 while (iter != end) {
  iternode = kvinodepiddir_new_inherited(vfsent_proc_pid,*iter++);
  if __unlikely(!iternode) { error = KE_NOMEM; goto err_procs_iter; }
  entname.dn_size = itos(pidnamebuf,sizeof(pidnamebuf),
                         kproc_getpid(iternode->pd_file.pf_proc));
  assert(entname.dn_size != 0);
  kdirentname_refreshhash(&entname);
  /* Enumerate all processes as directory entires. */
  error = (*callback)((struct kinode *)iternode,&entname,closure);
  kinode_decref((struct kinode *)iternode);
  if __unlikely(error != KE_OK) goto err_procs_iter;
 }
 if (dynbuf != buffer) free(dynbuf);
 return kvsdirnode_enumdir(self,callback,closure);
err_procs_iter: while (iter != end) kproc_decref(*iter);
err_procsmem:   if (dynbuf != buffer) free(dynbuf);
 return error;
}
struct kvsdirsuperblock kvfs_proc = {
 KSUPERBLOCK_INIT_HEAD
 KVSDIRNODE_INIT_EX(NULL,vfsent_proc,&kvfsproc_type.st_node)
};
struct ksuperblocktype kvfsproc_type = {
 .st_node = {
  .it_size    = sizeof(struct kvsdirnode),
  .it_walk    = &kfsproc_walk,
  .it_enumdir = &kfsproc_enumdir,
 },
};

//////////////////////////////////////////////////////////////////////////
// Declare all the inodes
struct kinode kvinode_proc_self = KINODE_INIT(&kvinodetype_proc_self,&kvfile_empty_type,(struct ksuperblock *)&kvfs_proc,S_IFLNK);


__DECL_END

#endif /* !__KOS_KERNEL_FS_VFS_PROC_C__ */

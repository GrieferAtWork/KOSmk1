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
#ifndef __KOS_KERNEL_FS_VFS_PROC_H__
#define __KOS_KERNEL_FS_VFS_PROC_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/kernel/fs/fs.h>
#include <kos/kernel/fs/vfs.h>
#include <kos/kernel/fs/file.h>
#include <sys/stat.h>

__DECL_BEGIN

struct kvprocdirent {
 struct kdirentname   pd_name; /*< [eof(vd_name.dn_name == NULL)] Entry name, or NULL for directory end. */
 __mode_t             pd_mode; /*< INode kind. */
 struct kinodetype   *pd_type; /*< [1..1] Inode type. */
union{
 struct kfiletype    *pd_file; /*< [1..1] File type (only if 'pd_type != kvinodedirtype_proc_pid'). */
 struct kvprocdirent *pd_subd; /*< [1..1] Sub-directory elements (only if 'pd_type == kvinodedirtype_proc_pid'). */
};
};
#define KVPROCDIRENT_INIT(name,type,file,mode) {KDIRENTNAME_INIT(name),mode,type,{file}}
#define KVPROCDIRENT_INIT_DIR(name,elem)       {KDIRENTNAME_INIT(name),S_IFDIR,&kvinodedirtype_proc_pid,{(struct kfiletype *)(elem)}}
#define KVPROCDIRENT_INIT_SENTINAL             {KDIRENTNAME_INIT_EMPTY,0,NULL,{NULL}}

struct kvinodepidfile { /*< Structure for all non-directory nodes on '/proc/[PID]' */
 struct kinode       pf_node; /*< Underlying INode. */
 __ref struct kproc *pf_proc; /*< [1..1][const] Reference to the associated process. */
};
struct kvinodepiddir { /*< Structure for all directory nodes on '/proc/[PID]' */
 struct kvinodepidfile pd_file; /*< Underlying static directory INode. */
 struct kvprocdirent  *pd_elem; /*< [1..1][const] List of node elements. */
};


//////////////////////////////////////////////////////////////////////////
// "/proc/[PID]": A directory containing various informations about a process.
// @return: A new /proc/[PID]-inode for a given process.
extern __crit __nonnull((1,2)) __ref struct kvinodepiddir *
kvinodepiddir_new_inherited(struct kvprocdirent *__restrict files,
                            __ref struct kproc *__restrict proc);
extern __crit __nonnull((1,2,3)) __ref struct kvinodepidfile *
kvinodepidfile_new_inherited(struct kinodetype *__restrict type,
                             __ref struct kproc *__restrict proc,
                             struct kfiletype *__restrict file_type,
                             __mode_t mode);

//////////////////////////////////////////////////////////////////////////
// Returns a new proc-pid file/directory inode as created from the given decl.
extern __crit __nonnull((1,2)) __ref struct kvinodepidfile *
kvinodepid_new_inherited(struct kvprocdirent const *__restrict decl,
                         __ref struct kproc *__restrict proc);

/* Generic operators for "/proc/[PID]" INodes. */
extern void kvinodepid_generic_quit(struct kinode *self);
extern kerrno_t kvinodepid_generic_getattr(struct kinode const *self, __size_t ac, union kinodeattr *av);
extern kerrno_t kvinodepiddir_generic_walk(struct kinode *self, struct kdirentname const *name, __ref struct kinode **resnode);
extern kerrno_t kvinodepiddir_generic_enumdir(struct kinode *self, pkenumdir callback, void *closure);
#ifdef __INTELLISENSE__
#define KVINODEPIDTYPE_INIT(T)   /* nothing */
#define KVINODEPIDFILETYPE_INIT  KVINODEPIDTYPE_INIT(struct kvinodepidfile)
#define KVINODEPIDDIRTYPE_INIT   KVINODEPIDTYPE_INIT(struct kvinodepiddir)
#else
#define KVINODEPIDTYPE_INIT(T) \
 .it_size    = sizeof(T),\
 .it_quit    = &kvinodepid_generic_quit,\
 .it_getattr = &kvinodepid_generic_getattr,
#define KVINODEPIDFILETYPE_INIT \
  KVINODEPIDTYPE_INIT(struct kvinodepidfile)
#define KVINODEPIDDIRTYPE_INIT \
  KVINODEPIDTYPE_INIT(struct kvinodepiddir)\
 .it_walk    = &kvinodepiddir_generic_walk,\
 .it_enumdir = &kvinodepiddir_generic_enumdir,
#endif





/* Generic "/proc/[PID]" directory type. */
extern struct kinodetype kvinodedirtype_proc_pid;

/* "/proc/self": A symbolic link expanding to the pid of the calling process. */
extern struct kinodetype kvinodetype_proc_self;
extern struct kinode     kvinode_proc_self;

/* "/proc/kcore": A file usable for R/W access to physical memory. */
extern struct kinodetype kvinodetype_proc_kcore;
extern struct kfiletype  kvfiletype_proc_kcore;
extern struct kinode     kvinode_proc_kcore;
struct kvkcorefile { struct kvfile c_file; __kernel __byte_t *c_pos; };

/* Special files in "/proc/[PID]/..." */
extern struct kinodetype kvinodetype_proc_pid_cwd;
extern struct kinodetype kvinodetype_proc_pid_exe;
extern struct kinodetype kvinodetype_proc_pid_root;


/* Virtual folder elements of "/proc/[PID]" directories. */
extern struct kvprocdirent vfsent_proc_pid[]; /* "/proc/[PID]/" */


/* "/proc": The /proc filesystem superblock. */
extern struct kvsdirsuperblock kvfs_proc;
extern struct ksuperblocktype kvfsproc_type;


__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_FS_VFS_PROC_H__ */

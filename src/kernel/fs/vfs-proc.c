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

#include <kos/kernel/fs/vfs-proc.h>
#include <kos/kernel/fs/vfs.h>
#include <kos/kernel/fs/fs-dirfile.h>
#include <sys/stat.h>
#include <kos/kernel/task.h>
#include <kos/kernel/proc.h>
#include <sys/types.h>
#include <itos.h>

__DECL_BEGIN


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
 target->dn_name[target->dn_size] = '\0';
 if __likely(target->dn_size <= sizeof(buf)/sizeof(char)) {
  memcpy(target->dn_name,buf,target->dn_size*sizeof(char));
 } else itos(target->dn_name,(target->dn_size+1)*sizeof(char),pid);
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
 KVDIRENT_INIT("self",&kvinode_proc_self),
 KVDIRENT_INIT_SENTINAL
};

static kerrno_t
kfsproc_walk(struct kinode *self,
             struct kdirentname const *name,
             __ref struct kinode **resnode) {
 /* TODO: Check if 'name' is an integer matching a visible & valid PID. */

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
 return kvsdirnode_enumdir(self,callback,closure);
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

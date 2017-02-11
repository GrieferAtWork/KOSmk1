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
#ifndef __KOS_KERNEL_FS_FS_H__
#define __KOS_KERNEL_FS_FS_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/compiler.h>
#include <sched.h>
#include <string.h>
#include <kos/attr.h>
#include <kos/errno.h>
#include <kos/atomic.h>
#include <kos/kernel/closelock.h>
#include <kos/kernel/mutex.h>
#include <kos/kernel/object.h>
#include <kos/kernel/task.h>
#include <kos/kernel/types.h>
#include <kos/kernel/sched_yield.h>
#include <kos/timespec.h>

__DECL_BEGIN

#ifndef SEEK_SET
#   define SEEK_SET 0
#endif
#ifndef SEEK_CUR
#   define SEEK_CUR 1
#endif
#ifndef SEEK_END
#   define SEEK_END 2
#endif

#define KFS_SEP      '/'
#define KFS_ALTSEP   '\\'
#define KFS_ISSEP(c) ((c)=='/'||(c)=='\\')

#define LINK_MAX  64 /*< Max amount of times a link is allowed to be dereferenced. */

struct kinodetype;
struct kinode;

struct kdirent;
struct kdirentname;

struct kfiletype;
struct kfile;

struct ksuperblock;
struct ksuperblocktype;

struct kfspathenv;

#define KOBJECT_MAGIC_INODE      0x1A0DE    /*< INODE. */
#define KOBJECT_MAGIC_DIRENT     0xD15EA7   /*< DIRENT. */
#define KOBJECT_MAGIC_SUPERBLOCK 0x50935B70 /*< SUPERBLO[CK]. */
#define kassert_kinode(ob)      kassert_refobject(ob,i_refcnt,KOBJECT_MAGIC_INODE)
#define kassert_kdirent(ob)     kassert_refobject(ob,d_refcnt,KOBJECT_MAGIC_DIRENT)
#define kassert_ksuperblock(ob) kassert_refobject(ob,s_root.i_refcnt,KOBJECT_MAGIC_SUPERBLOCK)

typedef kerrno_t (*__wunused __nonnull((1,2)) pkenumdir)(struct kinode *__restrict inode,
                                                         struct kdirentname *name,
                                                         void *closure);

struct kinodetype {
 __size_t   it_size; /*< Size of the implemented dirent-structure (must be at least 'sizeof(struct kdirent)') */
 void     (*it_quit)(struct kinode *self);
 kerrno_t (*it_getattr)(struct kinode const *self, __size_t ac, union kinodeattr *av);
 kerrno_t (*it_setattr)(struct kinode *self, __size_t ac, union kinodeattr const *av);
 kerrno_t (*it_delnode)(struct kinode *self); /*< Called to delete a node once its 'i_lnkcnt' drops to 0. */
 kerrno_t (*it_hrdlnk)(struct kinode *self, struct kdirentname const *name, struct kinode *__restrict inode); /*< Add a new hard link in directory 'self', maned 'named' and pointing to 'inode'. */
 kerrno_t (*it_unlink)(struct kinode *self, struct kdirentname const *name, struct kinode *__restrict inode); /*< Remove 'name' from 'self'. (Also called to decref hard links). */
 kerrno_t (*it_rmdir)(struct kinode *self, struct kdirentname const *name, struct kinode *__restrict inode); /*< Remove 'name' from 'self'. */
 // NOTE: 'it_mkdir' & 'it_mkreg': If the specified file already exists,
 //       it's inode must be stored in '*resnode' and 'KS_FOUND' must be
 //       returned without any given attributes being re-applied.
 // NOTE: If the given file already exists, but isn't of the requested type,
 //       KE_ISDIR/KE_NODIR must be returned without '*resnode' being filled.
 kerrno_t (*it_mkdir)(struct kinode *self, struct kdirentname const *name, __mode_t mode, __ref struct kinode **resnode); /*< Create a directory in 'self'. */
 kerrno_t (*it_mkreg)(struct kinode *self, struct kdirentname const *name, __size_t ac, union kinodeattr const *av, __ref struct kinode **resnode); /*< Create a regular file in 'self'. */
 kerrno_t (*it_mklnk)(struct kinode *self, struct kdirentname const *name, struct kdirentname const *target, __ref struct kinode **resnode); /*< Create a symbolic link in 'self'. */
 kerrno_t (*it_walk)(struct kinode *self, struct kdirentname const *name, __ref struct kinode **resnode); /*< Walks to the given name. */
 kerrno_t (*it_enumdir)(struct kinode *self, pkenumdir callback, void *closure); /*< Enumerate all files/folders. NOTE: If 'callback' returns non-KE_OK, return with that error/signal. */
 kerrno_t (*it_readlink)(struct kinode *self, struct kdirentname *target); /*< Upon successful return, the caller must destroy the 'target' name. */
};

struct kinode {
 KOBJECT_HEAD
 __atomic __u32            i_refcnt;    /*< Reference counter. */
 struct kinodetype        *i_type;      /*< [1..1][const] Inode type. */
 struct kfiletype         *i_filetype;  /*< [1..1][const] File type. */
 // NOTE: The reference loop created by superblock -> root -> inode -> superblock
 //       is intentional. It is (ab-)used to keep the roots of mounted filesystem
 //       in memory (and thereby mounted), as otherwise they would be unmounted
 //       the moment no one has any more files opened within.
 //       >> When unmounting a filesystem, (among other things) we simply break
 //          that loop, thus allowing any remaining references to die on their own.
 __ref struct ksuperblock *i_sblock;    /*< [0..1][const] Associated superblock.
                                             NOTE: Only NULL if this node itself is a superblock. */
 __mode_t __const          i_kind;      /*< [const] File kind. */
 struct kcloselock         i_closelock; /*< Lock held while executing any node operation. */
};
#define KINODE_INIT(node_type,file_type,superblock,kind) \
 {KOBJECT_INIT(KOBJECT_MAGIC_INODE) 0xffff,node_type,\
  file_type,superblock,kind,KCLOSELOCK_INIT}

#define kinode_issuperblock(self) (!(self)->i_sblock)
#define __kinode_superblock(self) \
 ((struct ksuperblock *)((__uintptr_t)(self)-offsetof(struct ksuperblock,s_root)))
#if defined(__GNUC__) && !defined(__INTELLISENSE__)
#define kinode_superblock(self) ((self)->i_sblock ?: __kinode_superblock(self))
#else
#define kinode_superblock(self) ((self)->i_sblock ? (self)->i_sblock : __kinode_superblock(self))
#endif

//////////////////////////////////////////////////////////////////////////
// Creates a new inode with the given parameters
extern __wunused __nonnull((1,2,3)) __ref struct kinode *
__kinode_alloc(struct ksuperblock *superblock,
               struct kinodetype *nodetype,
               struct kfiletype *filetype,
               __mode_t filekind);

__local KOBJECT_DEFINE_INCREF(kinode_incref,struct kinode,i_refcnt,kassert_kinode);
__local KOBJECT_DEFINE_DECREF(kinode_decref,struct kinode,i_refcnt,kassert_kinode,kinode_destroy);


struct stat64;
//////////////////////////////////////////////////////////////////////////
// Collect stat information about a given INode
extern __wunused __nonnull((1,2)) kerrno_t kinode_stat(struct kinode *self, struct stat64 *result);

//////////////////////////////////////////////////////////////////////////
// Closes the given Information node
// @return: KE_DESTROYED: The given node was already closed.
extern __wunused kerrno_t kinode_close(struct kinode *self);

//////////////////////////////////////////////////////////////////////////
// Perform various operations on an INode.
// @return: KE_DESTROYED: The Associated node was closed.
extern __wunused __nonnull((1,3)) kerrno_t kinode_getattr(struct kinode const *self, __size_t ac, union kinodeattr *av);
extern __wunused __nonnull((1,3)) kerrno_t kinode_setattr(struct kinode *self, __size_t ac, union kinodeattr const *av);
extern __wunused __nonnull((1,3)) kerrno_t kinode_getattr_legacy(struct kinode const *self, kattr_t attr, void *__restrict buf, __size_t bufsize, __size_t *__restrict reqsize);
extern __wunused __nonnull((1,3)) kerrno_t kinode_setattr_legacy(struct kinode *self, kattr_t attr, void const *__restrict buf, __size_t bufsize);
extern __wunused __nonnull((1,3)) kerrno_t __kinode_getattr_legacy(struct kinode const *self, kattr_t attr, void *__restrict buf, __size_t bufsize, __size_t *__restrict reqsize);
extern __wunused __nonnull((1,3)) kerrno_t __kinode_setattr_legacy(struct kinode *self, kattr_t attr, void const *__restrict buf, __size_t bufsize);

//////////////////////////////////////////////////////////////////////////
// Returns the directory entry associated with the given name
// @return: KE_NOSYS:     The operation is not supported by the node
// @return: KE_NODIR:     'self' is not a directory
// @return: KE_DESTROYED: 'self' was deleted
extern __wunused __nonnull((1,2)) kerrno_t kinode_walk(struct kinode *self, struct kdirentname const *name,
                                                       __ref struct kinode **result);

//////////////////////////////////////////////////////////////////////////
// Perform various operations on a given INode
// @return: KE_NOSYS:     The operation is not supported by the node
// @return: KE_DESTROYED: 'self' was deleted
// @return: KE_ISDIR:     [kinode_unlink] 'dent' is a directory
// @return: KE_NODIR:     'self' is not a directory
//                        [kinode_rmdir] 'dent' is not a directory
// @return: KS_FOUND:     [kinode_mkdir|kinode_mkreg] The specified name already existed.
//                                                 >> In this case, attributes are not applied,
//                                                    but '*resent' and '*resnode' are filled in.
// @return: KE_NOLNK:     [kinode_readlink] 'self' is not a symbolic link
extern __wunused __nonnull((1,2)) kerrno_t kinode_unlink(struct kinode *self, struct kdirentname const *name, struct kinode *__restrict node);
extern __wunused __nonnull((1,2)) kerrno_t kinode_rmdir(struct kinode *self, struct kdirentname const *name, struct kinode *__restrict node);
extern __wunused __nonnull((1,2)) kerrno_t kinode_remove(struct kinode *self, struct kdirentname const *name, struct kinode *__restrict node);
extern __wunused __nonnull((1,2,4)) kerrno_t kinode_mkdir(struct kinode *self, struct kdirentname const *name,
                                                          __mode_t mode, __ref struct kinode **resnode);
extern __wunused __nonnull((1,2,5)) kerrno_t kinode_mkreg(struct kinode *self, struct kdirentname const *name,
                                                          __size_t ac, union kinodeattr const *av, __ref struct kinode **resnode);
extern __wunused __nonnull((1,2,3,4)) kerrno_t kinode_mklnk(struct kinode *self, struct kdirentname const *name,
                                                            struct kdirentname const *target, __ref struct kinode **resnode);
extern __wunused __nonnull((1,2)) kerrno_t kinode_readlink(struct kinode *self, struct kdirentname *target);

//////////////////////////////////////////////////////////////////////////
// Creates a new hardlink in 'self', named 'name' and pointing to 'target'
// >> Upon success, the node's 'KATTR_FS_NLINK'
//    attribute should have been incremented by one.
// @return: KE_NOSYS:     The associated superblock/filesystem does not support hardlinks.
// @return: KE_NODIR:     'self' is not a directory, and doesn't implement the 'it_hrdlnk' interface.
// @return: KE_DESTROYED: 'self' was closed.
extern __wunused __nonnull((1,2,3)) kerrno_t
kinode_mkhardlink(struct kinode *self,
                  struct kdirentname const *name,
                  struct kinode *target);



//////////////////////////////////////////////////////////////////////////
// Generic attributes.
// >> When overwriting getattr or setattr, you should
//    call these functions for attributes you don't know.
extern __wunused __nonnull((1,3)) kerrno_t kinode_generic_getattr(struct kinode const *self, __size_t ac, union kinodeattr *av);
extern __wunused __nonnull((1,3)) kerrno_t kinode_generic_setattr(struct kinode *self, __size_t ac, union kinodeattr const *av);


struct kdirentname {
 __size_t dn_hash; /*< Filename hash. */
 __size_t dn_size; /*< Filename size. */
 char    *dn_name; /*< [1..dn_size][owned] Filename. */
};
#define kdirentname_init(self,name) \
 __xblock({ (self)->dn_name = (char *)(name); \
            (self)->dn_size = strlen((self)->dn_name); \
            kdirentname_refreshhash(self); \
 })

#define KDIRENTNAME_INIT(name) {(size_t)-1,__compiler_STRINGSIZE(name),name}
#define KDIRENTNAME_INIT_EMPTY {0,0,NULL}

#define kdirentname_isempty(self) (!(self)->dn_name)
#define kdirentname_equal(a,b) \
 ((a)->dn_hash == (b)->dn_hash && \
  (a)->dn_size == (b)->dn_size && \
  memcmp((a)->dn_name,(b)->dn_name,(a)->dn_size) == 0)
#define kdirentname_lazyequal(a,b) \
 ((a)->dn_size == (b)->dn_size && \
 (kdirentname_lazyhash(a),(a)->dn_hash == (b)->dn_hash) && \
  memcmp((a)->dn_name,(b)->dn_name,(a)->dn_size) == 0)

#define kdirentname_lazyhash(self) \
 __xblock({ struct kdirentname *const __self = (self); \
            if (__self->dn_hash == (__size_t)-1) kdirentname_refreshhash(__self);\
            (void)0;\
 })

#define kdirentname_refreshhash(self) \
 (void)((self)->dn_hash = kdirentname_genhash((self)->dn_name,(self)->dn_size))

#define kdirentname_quit(self) free((self)->dn_name)
extern __wunused __nonnull((1,2)) kerrno_t kdirentname_initcopy(struct kdirentname *self, struct kdirentname const *right);
extern __wunused __nonnull((1)) __size_t kdirentname_genhash(char const *data, __size_t bytes);


struct kdirentcachevec {
 __size_t         d_vecc;  /*< Size of the vector. */
 struct kdirent **d_vecv;  /*< [0..1][0..d_childc][owned] Vector of dirents. */
};
#define KDIRENTCACHE_SIZE 8
struct kdirentcache { struct kdirentcachevec dc_cache[KDIRENTCACHE_SIZE]; /*< Index == (dn_hash%KDIRENTCACHE_SIZE) */ };
#define KDIRENTCACHE_INIT  {{{0,NULL},}}
#define kdirentcache_init(self) memset(self,0,sizeof(struct kdirentcache))

//////////////////////////////////////////////////////////////////////////
// @return: KE_EXISTS: A child with the given name already exists (it is now stored in '*existing_child')
// @return: KE_NOMEM:  Not enough memory available
extern __wunused __nonnull((1,2)) kerrno_t
kdirentcache_insert(struct kdirentcache *self, struct kdirent *child,
                    struct kdirent **existing_child);

//////////////////////////////////////////////////////////////////////////
// @return: KE_NOENT: The given child was not found
extern __wunused __nonnull((1,2)) kerrno_t
kdirentcache_remove(struct kdirentcache *self, struct kdirent *child);

//////////////////////////////////////////////////////////////////////////
// @return: NULL: The given child was not found
extern __wunused __nonnull((1,2)) struct kdirent *
kdirentcache_lookup(struct kdirentcache *self, struct kdirentname const *name);


typedef __u8 kdirent_flag_t;
struct kdirent {
 KOBJECT_HEAD
 __atomic __u32        d_refcnt;
#define KDIRENT_LOCK_NODE  0x01
#define KDIRENT_LOCK_CACHE 0x02
 __atomic __u8         d_locks;
#define KDIRENT_FLAG_NONE  0x00
#define KDIRENT_FLAG_VIRT  0x01   /*< [const] This dirent is virtual, meaning that removing it should not be mirrored on disk. */
 kdirent_flag_t        d_flags;
 __u16                 d_padding;
 struct kdirentname    d_name;    /*< [const] Dirent name. */
 __ref struct kdirent *d_parent;  /*< [0..1][const] Parent directory (NULL if fs_root/root_of_unmounted). */
 struct kdirentcache   d_cache;   /*< Cache of child nodes. */
 __ref struct kinode  *d_inode;   /*< [0..1][lock(d_parent->)] Inode associated with this directory entry (NULL if deleted or not instantiated).
                                       NOTE: This field also owns a reference to 'i_refcnt'.
                                       NOTE: If this field becomes. */
};
#define KDIRENT_INIT(parent,inode,name) \
 {KOBJECT_INIT(KOBJECT_MAGIC_DIRENT) 0xffff,0,KDIRENT_FLAG_NONE,0,\
 {0,(sizeof(name)/sizeof(char))-1,name},parent,KDIRENTCACHE_INIT,inode}
#define KDIRENT_INIT_UNNAMED(parent,inode) \
 {KOBJECT_INIT(KOBJECT_MAGIC_DIRENT) 0xffff,0,KDIRENT_FLAG_NONE,0,\
 {0,0,NULL},parent,KDIRENTCACHE_INIT,inode}

#define kdirent_trylock(self,lock) ((katomic_fetchor((self)->d_locks,lock)&(lock))==0)
#define kdirent_lock(self,lock)    KTASK_SPIN(kdirent_trylock(self,lock))
#define kdirent_unlock(self,lock)  _assertef((katomic_fetchand((self)->d_locks,~(lock))&(lock))!=0,"Lock not held")


__local KOBJECT_DEFINE_INCREF(kdirent_incref,struct kdirent,d_refcnt,kassert_kdirent);
__local KOBJECT_DEFINE_DECREF(kdirent_decref,struct kdirent,d_refcnt,kassert_kdirent,kdirent_destroy);

//////////////////////////////////////////////////////////////////////////
// Allocates an uninstantiated dirent referring to 'parent'.
// @return: NULL: Not enough available heap memory.
extern __wunused __nonnull((1,2)) __ref struct kdirent *
__kdirent_alloc(struct kdirent *parent,
                struct kdirentname const *name);

//////////////////////////////////////////////////////////////////////////
// Create a new dirent from inherited data (the returned inode will own all arguments)
// @return: NULL: Not enough available heap memory.
extern __wunused __nonnull((1,2,3)) __ref struct kdirent *
__kdirent_newinherited(__ref struct kdirent *parent,
                       struct kdirentname const *name,
                       __ref struct kinode *__restrict inode);
extern __wunused __nonnull((1,2,3)) __ref struct kdirent *
kdirent_new(struct kdirent *parent,
            struct kdirentname const *name,
            struct kinode *__restrict inode);

//////////////////////////////////////////////////////////////////////////
// Returns the Inode associated with a given directory entry
// @return: NULL: No node is associated with the entry (can happen if the entry was removed)
extern __wunused __nonnull((1)) __ref struct kinode *kdirent_getnode(struct kdirent *self);

//////////////////////////////////////////////////////////////////////////
// Returns the directory entry associated with the given name
// NOTE: This function will search the cache associated with the
//       dirent before asking the associated node for help ('kinode_walk').
// @return: KE_NOSYS:     The operation is not supported by the node
// @return: KE_NODIR:     'self' is not a directory
// @return: KE_DESTROYED: 'self' was deleted
extern __wunused __nonnull((1,2,3)) kerrno_t
kdirent_walk(struct kdirent *self, struct kdirentname const *name,
             __ref struct kdirent **result);
extern __wunused __nonnull((1,2,3)) kerrno_t
kdirent_walkenv(struct kfspathenv const *env, struct kdirentname const *name,
                __ref struct kdirent **result);
extern __wunused __nonnull((1,2,3,4)) kerrno_t
kdirent_walknode(struct kdirent *self, struct kdirentname const *name,
                 __ref struct kdirent **result, __ref struct kinode **resnode);

//////////////////////////////////////////////////////////////////////////
// Perform various operations on a given INode
// @return: KE_NOSYS:     The operation is not supported by the dirent's node
// @return: KE_DESTROYED: 'self' was deleted
// @return: KE_ISDIR:     [kdirent_unlink] 'self' is a directory
//                        [kdirent_unlink] 'self' is the root of a superblock
// @return: KE_NODIR:     [kdirent_rmdir] 'self' is not a directory
//                        [kdirent_rmdir|kdirent_remove] 'self' is the root of a superblock
// @return: KS_FOUND:     [kdirent_mkdir|kdirent_mkreg] The specified name already existed.
//                        >> In this case, attributes are not applied,
//                           but '*resent' and '*resnode' are filled in.
// @return: KE_ACCES:     [kdirent_unlink|kdirent_rmdir|kdirent_remove] The given dirent must be kept.
//                        >> It is possible that the given dirent is protected, 
//                           the global root directory, or simply a mounting point.
extern __wunused __nonnull((1)) kerrno_t kdirent_unlink(struct kdirent *self);
extern __wunused __nonnull((1)) kerrno_t kdirent_rmdir(struct kdirent *self);
extern __wunused __nonnull((1)) kerrno_t kdirent_remove(struct kdirent *self);
extern __wunused __nonnull((1,2)) kerrno_t
kdirent_mkdir(struct kdirent *self, struct kdirentname const *name, __mode_t mode, 
              __ref /*opt*/struct kdirent **resent,
              __ref /*opt*/struct kinode **resnode);
extern __wunused __nonnull((1,2)) kerrno_t
kdirent_mkreg(struct kdirent *self, struct kdirentname const *name,
              __size_t ac, union kinodeattr const *av,
              __ref /*opt*/struct kdirent **resent,
              __ref /*opt*/struct kinode **resnode);
extern __wunused __nonnull((1,2)) kerrno_t
kdirent_mklnk(struct kdirent *self, struct kdirentname const *name,
              struct kdirentname const *target,
              __ref /*opt*/struct kdirent **resent,
              __ref /*opt*/struct kinode **resnode);

//////////////////////////////////////////////////////////////////////////
// Insert a virtual INode into the given directory dirent
// >> The caller is responsible for keeping the associated
//    directory entry alive, as the nod will be deleted once
//    its reference counter drops to zero.
// @return: KE_DESTROYED: 'self' was destroyed.
// @return: KE_NODIR:     'self' is not a directory.
// @return: KE_NOMEM:     Not enough available memory.
// @return: KE_EXISTS:    A node with the given name already exists.
extern __wunused __nonnull((1,2,3,4)) kerrno_t
kdirent_insnod(struct kdirent *self, struct kdirentname const *name,
               struct kinode *__restrict node, __ref struct kdirent **resent,
               kdirent_flag_t flags);


//////////////////////////////////////////////////////////////////////////
// Mounts a given superblock under 'name' within the current directory 'self'
// @return: KE_DESTROYED: 'self' was destroyed.
// @return: KE_NODIR:     'self' is not a directory.
// @return: KE_NOMEM:     Not enough available memory.
// @return: KE_EXISTS:    A virtual dirent with the given name already exists
//                        NOTE: This only refers to virtual dirents, as
//                              physical entries can simply be overwritten with this.
extern __wunused __nonnull((1,2,3)) kerrno_t
kdirent_mount(struct kdirent *self, struct kdirentname const *name,
              struct ksuperblock *sblock, __ref /*opt*/struct kdirent **resent);



//////////////////////////////////////////////////////////////////////////
// Unlink the node from a given dirent and remove the dirent from it's parents cache.
extern __nonnull((1)) void kdirent_unlinknode(struct kdirent *self);

//////////////////////////////////////////////////////////////////////////
// Returns the filename/absolute path of a given directory entry
extern __wunused __nonnull((1,2)) kerrno_t
kdirent_getfilename(struct kdirent const *self, char *__restrict buf,
                    __size_t bufsize, __size_t *__restrict reqsize);
extern __wunused __nonnull((1,2,3)) kerrno_t
kdirent_getpathname(struct kdirent const *self, struct kdirent *__restrict root,
                    char *__restrict buf, __size_t bufsize, __size_t *__restrict reqsize);


//////////////////////////////////////////////////////////////////////////
// WARNING: After a successful return, 'lastpart' may not be destroyed.
extern __wunused __nonnull((1,2,3,4)) kerrno_t
kdirent_walklast(struct kfspathenv const *env, __ref struct kdirent **newroot,
                 struct kdirentname *lastpart, char const *path, __size_t pathmax);
extern __wunused __nonnull((1,2,3)) kerrno_t
kdirent_walkall(struct kfspathenv const *env, __ref struct kdirent **finish,
                char const *path, __size_t pathmax);

//////////////////////////////////////////////////////////////////////////
// High-level, pathname-based dirent file operations (NOTE: The given path is a strn-style string)
extern __wunused __nonnull((1,2)) kerrno_t
kdirent_mkdirat(struct kfspathenv const *env, char const *path,
                __size_t pathmax, __mode_t mode,
                __ref /*opt*/struct kdirent **resent,
                __ref /*opt*/struct kinode **resnode);
extern __wunused __nonnull((1,2)) kerrno_t
kdirent_mkregat(struct kfspathenv const *env, char const *path,
                __size_t pathmax, __size_t ac, union kinodeattr const *av,
                __ref /*opt*/struct kdirent **resent,
                __ref /*opt*/struct kinode **resnode);
extern __wunused __nonnull((1,2)) kerrno_t
kdirent_mklnkat(struct kfspathenv const *env, char const *path,
                __size_t pathmax, struct kdirentname const *target,
                __ref /*opt*/struct kdirent **resent,
                __ref /*opt*/struct kinode **resnode);
extern __wunused __nonnull((1,2)) kerrno_t kdirent_rmdirat(struct kfspathenv const *env, char const *path, __size_t pathmax);
extern __wunused __nonnull((1,2)) kerrno_t kdirent_unlinkat(struct kfspathenv const *env, char const *path, __size_t pathmax);
extern __wunused __nonnull((1,2)) kerrno_t kdirent_removeat(struct kfspathenv const *env, char const *path, __size_t pathmax);

//////////////////////////////////////////////////////////////////////////
// Insert a virtual node with a given name
// >> The caller is responsible for keeping the associated
//    directory entry alive, as the nod will be deleted once
//    its reference counter drops to zero.
// @return: KE_NODIR:  'self' is not a directory.
// @return: KE_NOMEM:  Not enough available memory.
// @return: KE_EXISTS: A node with the given name already exists.
extern __wunused __nonnull((1,2,4,5)) kerrno_t
kdirent_insnodat(struct kfspathenv const *env, char const *path, __size_t pathmax,
                 struct kinode *__restrict node, __ref struct kdirent **resent, kdirent_flag_t flags);


//////////////////////////////////////////////////////////////////////////
// Mounts a given superblock under 'name' within the current directory 'self'
// @return: KE_DESTROYED: 'self' was destroyed.
// @return: KE_NODIR:     'self' is not a directory.
// @return: KE_NOMEM:     Not enough available memory.
// @return: KE_EXISTS:    A virtual dirent with the given name already exists
//                        NOTE: This only refers to virtual dirents, as
//                              physical entries can simply be overwritten with this.
extern __wunused __nonnull((1,2,4)) kerrno_t
kdirent_mountat(struct kfspathenv const *env, char const *path, __size_t pathmax,
                struct ksuperblock *sblock, __ref /*opt*/struct kdirent **resent);

//////////////////////////////////////////////////////////////////////////
// Open a given file with the given attributes.
// @return: KS_FOUND: 'O_CREAT' was included in 'mode', but the file already existed.
//                    >> It was opened successfully, but the given attributes were not applied.
// @return: KE_DESTROYED: The specified 'path' describes a directory that was removed
//                        at one point, leaving the associated descriptor to weakly
//                        reference a dead filesystem branch.
extern __wunused __nonnull((1,2,7)) kerrno_t
kdirent_openat(struct kfspathenv const *env, char const *path, __size_t pathmax,
               __openmode_t mode, __size_t create_ac, union kinodeattr const *create_av,
               __ref struct kfile **__restrict result);
extern __wunused __nonnull((1,2,3,7)) kerrno_t
kdirent_openlast(struct kfspathenv const *env, struct kdirent *dir, struct kdirentname const *name,
                 __openmode_t mode, __size_t create_ac, union kinodeattr const *create_av,
                 __ref struct kfile **__restrict result);



struct ksdev;
struct ksuperblocktype {
 struct kinodetype     st_node; /*< Underlying node type routines. */
 // NOTE: For synchronizing operations with closing a superblock,
 //       the i_closelock lock from the inode of the root node is used.
 kerrno_t            (*st_close)(struct ksuperblock *self); /*< Close the superblock (Called during unmounting). */
 kerrno_t            (*st_flush)(struct ksuperblock *self); /*< Flush metadata to disk. */
 __ref struct ksdev *(*st_getdev)(struct ksuperblock const *self); /*< Return the associated storage device (NOTE: May be called after the superblock was closed). */
 kerrno_t            (*st_getattr)(struct ksuperblock const *self, kattr_t attr, void *__restrict buf, __size_t bufsize, __size_t *__restrict reqsize); /* NOTE: 'reqsize' may be NULL. */
 kerrno_t            (*st_setattr)(struct ksuperblock *self, kattr_t attr, void const *__restrict buf, __size_t bufsize);
};

struct ksupermount {
 // [lock(KMOUNTMAN_LOCK_CHAIN)]
 // The members of this structure are protected by
 // the lock of the global mount manager. (see below...)
 // >> When attempting to mount a superblock, one must also lock
 //    the closelock 'i_closelock' of the superblock's root node,
 //    though holding that lock isn't required for unmounting
 //    (obviously, as otherwise you'd never be able to unmount anything...)
 __ref struct ksuperblock *sm_prev; /*< [0..1] Previous mounted superblock. */
 __ref struct ksuperblock *sm_next; /*< [0..1] Next mounted superblock. */
 __size_t                  sm_mntc; /*< Amount of paths this superblock is mounted under. */
 __ref struct kdirent    **sm_mntv; /*< [1..1][0..sm_mntc][owned] Vector of active mounting points.
                                        >> This is the list of root dirents referencing this superblock's root node. */
};

#define ksupermount_init(self) memset(self,0,sizeof(struct ksupermount))
#define KSUPERMOUNT_INIT {NULL,NULL,0,NULL}

#define KSUPERBLOCK_TYPE(TNODE) \
 KOBJECT_HEAD \
 struct ksupermount s_mount; /*< Filesystem mounting information. */\
 TNODE              s_root;  /*< Filesystem root node. */


struct ksuperblock { KSUPERBLOCK_TYPE(struct kinode) };

#define KSUPERBLOCK_INIT_HEAD \
 KOBJECT_INIT(KOBJECT_MAGIC_SUPERBLOCK) \
 KSUPERMOUNT_INIT,

#define ksuperblock_type(self) ((struct ksuperblocktype *)(self)->s_root.i_type)
#define ksuperblock_root(self) (&(self)->s_root)
#define ksuperblock_incref(self)  kinode_incref(ksuperblock_root(self))
#define ksuperblock_decref(self)  kinode_decref(ksuperblock_root(self))

//////////////////////////////////////////////////////////////////////////
// Remove a given mounting point of the given superblock.
// NOTE: This method is only called internally, and should not
//       be called directly. To remove a mount point, simply
//       rmdir/remove the directory it is mounted under.
// @return: KE_NOENT: The given 'ent' isn't a valid mounting point for 'self'
extern kerrno_t _ksuperblock_delmnt(struct ksuperblock *self, struct kdirent *ent);

//////////////////////////////////////////////////////////////////////////
// Adds a given mounting point to the given superblock
// NOTE: This method is only called internally, and should not
//       be called directly. To add a mount point, use the higher-level
//       'kdirent_mount' function, and those derived from it.
// @return: KE_DESTROYED: 'self' has already been closed
// @return: KE_NOMEM:     Not enough memory to complete the operation
// @return: KE_OVERFLOW:  Failed to generate a reference for the initial mounting of 'self'
//                        Failed to create a new reference to 'ent'.
extern kerrno_t _ksuperblock_addmnt(struct ksuperblock *self, struct kdirent *ent);
extern kerrno_t _ksuperblock_addmnt_inherited(struct ksuperblock *self, __ref struct kdirent *ent);
extern void _ksuperblock_clearmnt(struct ksuperblock *self);

//////////////////////////////////////////////////////////////////////////
// Clears all mount points of a given superblock and unmounts it.
// NOTE: Once this function has been called, the superblock can no longer be mounted.
// @return: KE_DESTROYED: 'self' was already unmounted
#define ksuperblock_umount(self) kinode_close(ksuperblock_root(self))



//////////////////////////////////////////////////////////////////////////
// Flushes changed metadata (such as file headers, or link tables) to disk.
// >> Implementors of a superblock should note that this operation
//    isn't automatically performed when a superblock is closed.
//    Though the superblock's 'st_close' operator will be invoked.
extern __wunused __nonnull((1)) kerrno_t ksuperblock_flush(struct ksuperblock *self);


//////////////////////////////////////////////////////////////////////////
// Performs various operations on a given superblock
// @return: NULL:         The superblock was destroyed, or no device is associated
// @return: KE_DESTROYED: *ditto*
extern __wunused __nonnull((1)) __ref struct ksdev *ksuperblock_getdev(struct ksuperblock const *self);
#define ksuperblock_getattr(self,attr,buf,bufsize,reqsize) \
 kinode_getattr_legacy(ksuperblock_root(self),attr,buf,bufsize,reqsize)
#define ksuperblock_setattr(self,attr,buf,bufsize) \
 kinode_setattr_legacy(ksuperblock_root(self),attr,buf,bufsize)


// Create a new FAT filesystem superblock
extern __wunused __nonnull((1,2)) kerrno_t ksuperblock_newfat(struct ksdev *sdev,
                                                              __ref struct ksuperblock **result);

//////////////////////////////////////////////////////////////////////////
// Generic, internal initializer for a superblock
extern __nonnull((1,2)) void ksuperblock_generic_init(struct ksuperblock *self,
                                                      struct ksuperblocktype *stype);


//////////////////////////////////////////////////////////////////////////
// Generic attributes.
// >> When overwriting getattr or setattr, you should
//    call these functions for attributes you don't know.
extern __wunused __nonnull((1,3)) kerrno_t ksuperblock_generic_getattr(struct ksuperblock const *self, kattr_t attr, void *__restrict buf, __size_t bufsize, __size_t *__restrict reqsize);
extern __wunused __nonnull((1,3)) kerrno_t ksuperblock_generic_setattr(struct ksuperblock *self, kattr_t attr, void const *__restrict buf, __size_t bufsize);



struct kmountman {
#define KMOUNTMAN_LOCK_CHAIN 0x01 /*< Locks the chain of mounted superblock. */
 __u8                      mm_locks; /*< Mount manager locks. */
#define KMOUNTMAN_FLAG_NONE  0x00
 __u8                      mm_flags; /*< Mount manager flags. */
 __u8                      padding[2];
 __ref struct ksuperblock *mm_first; /*< [0..1] First mounted superblock. */
};
#define kmountman_trylock(lock) ((katomic_fetchor(kfs_mountman.mm_locks,lock)&(lock))==0)
#define kmountman_lock(lock)    KTASK_SPIN(kmountman_trylock(lock))
#define kmountman_unlock(lock)  _assertef((katomic_fetchand(kfs_mountman.mm_locks,~(lock))&(lock))!=0,"Lock not held")
extern struct kmountman kfs_mountman;

//////////////////////////////////////////////////////////////////////////
// Unmounts all mounted file systems and prints and error to
// k_syslogf(KLOG_ERROR) if a given filesystem cannot be unmounted safely.
extern void kmountman_unmountall(void);
extern void kmountman_unmount_unsafe(struct ksuperblock *fs);

#define KMOUNTMAN_INIT {0,KMOUNTMAN_FLAG_NONE,{0,0},NULL}
#define kmountman_insert_unlocked_inheritref(sblock) \
 __xblock({ struct ksuperblock *const __sblock = (sblock);\
            __sblock->s_mount.sm_prev = NULL;\
            __sblock->s_mount.sm_next = kfs_mountman.mm_first;\
            if (kfs_mountman.mm_first) kfs_mountman.mm_first->s_mount.sm_prev = __sblock;\
            kfs_mountman.mm_first = __sblock;\
            (void)0;\
 })
#define kmountman_remove_unlocked_inheritref(sblock) \
 __xblock({ struct ksuperblock *const __sblock = (sblock);\
            assert((__sblock->s_mount.sm_prev != NULL) ==\
                   (__sblock == kfs_mountman.mm_first));\
            if (__sblock->s_mount.sm_prev){ __sblock->s_mount.sm_prev\
                        ->s_mount.sm_next = __sblock->s_mount.sm_next;\
             __sblock->s_mount.sm_prev = NULL;\
            } else kfs_mountman.mm_first = __sblock->s_mount.sm_next;\
            if (__sblock->s_mount.sm_next){ __sblock->s_mount.sm_next\
                        ->s_mount.sm_prev = NULL;\
             __sblock->s_mount.sm_next = NULL;\
            }\
            (void)0;\
 })



struct kfspathenv {
 struct kdirent *env_cwd;   /*< [1..1] Current working directory. */
 struct kdirent *env_root;  /*< [1..1] Root directory. */
 __u32           env_flags; /*< O_* (from <fcntl.h>) style flags (Interesting part here is 'O_NOFOLLOW'). */
 __uid_t         env_uid;   /*< UID of the user performing the access. */
 __gid_t         env_gid;   /*< GID of the user performing the access. */
 __u32           env_lnk;   /*< Current link counter. */
};
#define kfspathenv_initcommon(self) ((self)->env_lnk = 0)
#define KFSPATHENV_INITROOT {kfs_getroot(),kfs_getroot(),0,0,0,0}
#define kfspathenv_initfrom(self,base,cwd,root)\
 ((self)->env_cwd   = (cwd),\
  (self)->env_root  = (root),\
  (self)->env_flags = (base)->env_flags,\
  (self)->env_uid   = (base)->env_uid,\
  (self)->env_gid   = (base)->env_gid,\
  (self)->env_lnk   = (base)->env_lnk)

extern __crit __nonnull((1)) kerrno_t kfspathenv_inituser(struct kfspathenv *__restrict self);
#define /*__crit*/ kfspathenv_quituser(self) \
 (kdirent_decref((self)->env_cwd),\
  kdirent_decref((self)->env_root))

extern struct kdirent __kfs_root; /*< ~Real~ filesystem root directory. */
#define kfs_getroot() (&__kfs_root)





//////////////////////////////////////////////////////////////////////////
// High-level, pathname-based dirent file operations
// (HINT: The given path is a strn-style string, meaning
//        you can pass (size_t)-1 for a c-style string,
//        or its actual length for a size-style one)
__local __wunused __nonnull((1)) kerrno_t
krootfs_mkdir(char const *__restrict path, __size_t pathmax, __mode_t mode,
              __ref /*opt*/struct kdirent **resent,
              __ref /*opt*/struct kinode **resnode);
__local __wunused __nonnull((1)) kerrno_t
krootfs_mkreg(char const *__restrict path, __size_t pathmax,
              __size_t ac, union kinodeattr const *av,
              __ref /*opt*/struct kdirent **resent,
              __ref /*opt*/struct kinode **resnode);
__local __wunused __nonnull((1,3)) kerrno_t
krootfs_mklnk(char const *__restrict path, __size_t pathmax,
              struct kdirentname const *__restrict target,
              __ref /*opt*/struct kdirent **resent,
              __ref /*opt*/struct kinode **resnode);
__local __wunused __nonnull((1)) kerrno_t
krootfs_rmdir(char const *__restrict path, __size_t pathmax);
__local __wunused __nonnull((1)) kerrno_t
krootfs_unlink(char const *__restrict path, __size_t pathmax);
__local __wunused __nonnull((1)) kerrno_t
krootfs_remove(char const *__restrict path, __size_t pathmax);
__local __crit __wunused __nonnull((1,3,4)) kerrno_t
krootfs_insnod(char const *__restrict path, __size_t pathmax,
               struct kinode *__restrict node,
               __ref struct kdirent **__restrict resent,
               kdirent_flag_t flags);
__local __wunused __nonnull((1,3)) kerrno_t
krootfs_mount(char const *path, __size_t pathmax,
              struct ksuperblock *__restrict sblock,
              __ref /*opt*/struct kdirent **resent);
__local __crit __wunused __nonnull((1,6)) kerrno_t
krootfs_open(char const *__restrict path, __size_t pathmax, __openmode_t mode,
             __size_t create_ac, union kinodeattr const *create_av,
             __ref struct kfile **__restrict result);

__local __wunused __nonnull((1)) kerrno_t
kuserfs_mkdir(char const *__restrict path, __size_t pathmax, __mode_t mode,
              __ref /*opt*/struct kdirent **resent,
              __ref /*opt*/struct kinode **resnode);
__local __wunused __nonnull((1)) kerrno_t
kuserfs_mkreg(char const *__restrict path, __size_t pathmax,
              __size_t ac, union kinodeattr const *av,
              __ref /*opt*/struct kdirent **resent,
              __ref /*opt*/struct kinode **resnode);
__local __wunused __nonnull((1,3)) kerrno_t
kuserfs_mklnk(char const *__restrict path, __size_t pathmax,
              struct kdirentname const *__restrict target,
              __ref /*opt*/struct kdirent **resent,
              __ref /*opt*/struct kinode **resnode);
__local __wunused __nonnull((1)) kerrno_t
kuserfs_rmdir(char const *__restrict path, __size_t pathmax);
__local __wunused __nonnull((1)) kerrno_t
kuserfs_unlink(char const *__restrict path, __size_t pathmax);
__local __wunused __nonnull((1)) kerrno_t
kuserfs_remove(char const *__restrict path, __size_t pathmax);
__local __crit __wunused __nonnull((1,3,4)) kerrno_t
kuserfs_insnod(char const *__restrict path, __size_t pathmax,
               struct kinode *__restrict node,
               __ref struct kdirent **__restrict resent,
               kdirent_flag_t flags);
__local __wunused __nonnull((1,3)) kerrno_t
kuserfs_mount(char const *path, __size_t pathmax,
              struct ksuperblock *__restrict sblock,
              __ref /*opt*/struct kdirent **resent);
__local __crit __wunused __nonnull((1,6)) kerrno_t
kuserfs_open(char const *__restrict path, __size_t pathmax, __openmode_t mode,
             __size_t create_ac, union kinodeattr const *create_av,
             __ref struct kfile **__restrict result);


#ifndef __INTELLISENSE__
__local kerrno_t
krootfs_mkdir(char const *__restrict path, __size_t pathmax, __mode_t mode,
              __ref /*opt*/struct kdirent **resent,
              __ref /*opt*/struct kinode **resnode) {
 struct kfspathenv env = KFSPATHENV_INITROOT;
 return kdirent_mkdirat(&env,path,pathmax,mode,resent,resnode);
}
__local kerrno_t
krootfs_mkreg(char const *__restrict path, __size_t pathmax,
              __size_t ac, union kinodeattr const *av,
              __ref /*opt*/struct kdirent **resent,
              __ref /*opt*/struct kinode **resnode) {
 struct kfspathenv env = KFSPATHENV_INITROOT;
 return kdirent_mkregat(&env,path,pathmax,ac,av,resent,resnode);
}
__local kerrno_t
krootfs_mklnk(char const *__restrict path, __size_t pathmax,
              struct kdirentname const *__restrict target,
              __ref /*opt*/struct kdirent **resent,
              __ref /*opt*/struct kinode **resnode) {
 struct kfspathenv env = KFSPATHENV_INITROOT;
 return kdirent_mklnkat(&env,path,pathmax,target,resent,resnode);
}
__local __wunused __nonnull((1)) kerrno_t
krootfs_rmdir(char const *__restrict path, __size_t pathmax) {
 struct kfspathenv env = KFSPATHENV_INITROOT;
 return kdirent_rmdirat(&env,path,pathmax);
}
__local __wunused __nonnull((1)) kerrno_t
krootfs_unlink(char const *__restrict path, __size_t pathmax) {
 struct kfspathenv env = KFSPATHENV_INITROOT;
 return kdirent_unlinkat(&env,path,pathmax);
}
__local __wunused __nonnull((1)) kerrno_t
krootfs_remove(char const *__restrict path, __size_t pathmax) {
 struct kfspathenv env = KFSPATHENV_INITROOT;
 return kdirent_removeat(&env,path,pathmax);
}
__local __crit kerrno_t
krootfs_insnod(char const *__restrict path, __size_t pathmax,
               struct kinode *__restrict node,
               __ref struct kdirent **__restrict resent,
               kdirent_flag_t flags) {
 KTASK_CRIT_MARK
 struct kfspathenv env = KFSPATHENV_INITROOT;
 return kdirent_insnodat(&env,path,pathmax,node,resent,flags);
}
__local kerrno_t
krootfs_mount(char const *__restrict path, __size_t pathmax,
              struct ksuperblock *__restrict sblock,
              __ref /*opt*/struct kdirent **resent) {
 struct kfspathenv env = KFSPATHENV_INITROOT;
 return kdirent_mountat(&env,path,pathmax,sblock,resent);
}
__local __crit kerrno_t
krootfs_open(char const *__restrict path, __size_t pathmax, __openmode_t mode,
             __size_t create_ac, union kinodeattr const *create_av,
             __ref struct kfile **__restrict result) {
 KTASK_CRIT_MARK
 struct kfspathenv env = KFSPATHENV_INITROOT;
 return kdirent_openat(&env,path,pathmax,mode,create_ac,create_av,result);
}

__local kerrno_t
kuserfs_mkdir(char const *__restrict path, __size_t pathmax, __mode_t mode,
              __ref /*opt*/struct kdirent **resent,
              __ref /*opt*/struct kinode **resnode) {
 kerrno_t error;
 struct kfspathenv env;
 KTASK_CRIT_BEGIN
 if __likely(KE_ISOK(error = kfspathenv_inituser(&env))) {
  error = kdirent_mkdirat(&env,path,pathmax,mode,resent,resnode);
  kfspathenv_quituser(&env);
 }
 KTASK_CRIT_END
 return error;
}
__local kerrno_t
kuserfs_mkreg(char const *__restrict path, __size_t pathmax,
              __size_t ac, union kinodeattr const *av,
              __ref /*opt*/struct kdirent **resent,
              __ref /*opt*/struct kinode **resnode) {
 kerrno_t error;
 struct kfspathenv env;
 KTASK_CRIT_BEGIN
 if __likely(KE_ISOK(error = kfspathenv_inituser(&env))) {
  error = kdirent_mkregat(&env,path,pathmax,ac,av,resent,resnode);
  kfspathenv_quituser(&env);
 }
 KTASK_CRIT_END
 return error;
}
__local kerrno_t
kuserfs_mklnk(char const *__restrict path, __size_t pathmax,
              struct kdirentname const *__restrict target,
              __ref /*opt*/struct kdirent **resent,
              __ref /*opt*/struct kinode **resnode) {
 kerrno_t error;
 struct kfspathenv env;
 KTASK_CRIT_BEGIN
 if __likely(KE_ISOK(error = kfspathenv_inituser(&env))) {
  error = kdirent_mklnkat(&env,path,pathmax,target,resent,resnode);
  kfspathenv_quituser(&env);
 }
 KTASK_CRIT_END
 return error;
}
__local kerrno_t
kuserfs_rmdir(char const *__restrict path, __size_t pathmax) {
 kerrno_t error;
 struct kfspathenv env;
 KTASK_CRIT_BEGIN
 if __likely(KE_ISOK(error = kfspathenv_inituser(&env))) {
  error = kdirent_rmdirat(&env,path,pathmax);
  kfspathenv_quituser(&env);
 }
 KTASK_CRIT_END
 return error;
}
__local kerrno_t
kuserfs_unlink(char const *__restrict path, __size_t pathmax) {
 kerrno_t error;
 struct kfspathenv env;
 KTASK_CRIT_BEGIN
 if __likely(KE_ISOK(error = kfspathenv_inituser(&env))) {
  error = kdirent_unlinkat(&env,path,pathmax);
  kfspathenv_quituser(&env);
 }
 KTASK_CRIT_END
 return error;
}
__local kerrno_t
kuserfs_remove(char const *__restrict path, __size_t pathmax) {
 kerrno_t error;
 struct kfspathenv env;
 KTASK_CRIT_BEGIN
 if __likely(KE_ISOK(error = kfspathenv_inituser(&env))) {
  error = kdirent_removeat(&env,path,pathmax);
  kfspathenv_quituser(&env);
 }
 KTASK_CRIT_END
 return error;
}
__local __crit kerrno_t
kuserfs_insnod(char const *__restrict path, __size_t pathmax,
               struct kinode *__restrict node,
               __ref struct kdirent **__restrict resent,
               kdirent_flag_t flags) {
 kerrno_t error;
 struct kfspathenv env;
 KTASK_CRIT_MARK
 if __likely(KE_ISOK(error = kfspathenv_inituser(&env))) {
  error = kdirent_insnodat(&env,path,pathmax,node,resent,flags);
  kfspathenv_quituser(&env);
 }
 return error;
}
__local kerrno_t
kuserfs_mount(char const *path, __size_t pathmax,
              struct ksuperblock *__restrict sblock,
              __ref /*opt*/struct kdirent **resent) {
 kerrno_t error;
 struct kfspathenv env;
 KTASK_CRIT_BEGIN
 if __likely(KE_ISOK(error = kfspathenv_inituser(&env))) {
  error = kdirent_mountat(&env,path,pathmax,sblock,resent);
  kfspathenv_quituser(&env);
 }
 KTASK_CRIT_END
 return error;
}
__local __crit kerrno_t
kuserfs_open(char const *__restrict path, __size_t pathmax, __openmode_t mode,
             __size_t create_ac, union kinodeattr const *create_av,
             __ref struct kfile **__restrict result) {
 kerrno_t error;
 struct kfspathenv env;
 KTASK_CRIT_MARK
 if __likely(KE_ISOK(error = kfspathenv_inituser(&env))) {
  error = kdirent_openat(&env,path,pathmax,mode,create_ac,create_av,result);
  kfspathenv_quituser(&env);
 }
 return error;
}
#endif /* !__INTELLISENSE__ */


//////////////////////////////////////////////////////////////////////////
// Some building inode/file types
extern struct kinodetype kinode_generic_emptytype; // No special features

#ifdef __MAIN_C__
extern void kernel_initialize_filesystem(void);
extern void kernel_finalize_filesystem(void);
#endif

__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_FS_FS_H__ */

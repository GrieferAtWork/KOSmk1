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
#ifdef __INTELLISENSE__
#include "fs.c"
#define DO_REMOVE
#endif

#ifdef DO_RMDIR
kerrno_t kdirent_rmdir(struct kdirent *self)
#elif defined(DO_UNLINK)
kerrno_t kdirent_unlink(struct kdirent *self)
#elif defined(DO_REMOVE)
kerrno_t kdirent_remove(struct kdirent *self)
#else
#error "Must #define something before #including this file"
#endif
{
 struct kinode *parnode,*mynode; kerrno_t error;
 kassert_kdirent(self);
 if __unlikely(!self->d_parent) return KE_ISDIR;
 if (self->d_flags&KDIRENT_FLAG_VIRT) {
  // Virtual dirent
  kdirent_lock(self,KDIRENT_LOCK_NODE);
  mynode = self->d_inode;
#ifdef DO_UNLINK
  if (mynode && S_ISDIR(mynode->i_kind)) {
   kdirent_unlock(self,KDIRENT_LOCK_NODE);
   return KE_ISDIR;
  }
#endif
  self->d_inode = NULL;
  kdirent_unlock(self,KDIRENT_LOCK_NODE);
  if (kinode_issuperblock(mynode)) {
   // Unmount a given mount point
   error = _ksuperblock_delmnt(__kinode_superblock(mynode),self);
   if __unlikely(KE_ISERR(error)) {
    // Undo the node's removal
    kdirent_lock(self,KDIRENT_LOCK_NODE);
    self->d_inode = mynode;
    kdirent_unlock(self,KDIRENT_LOCK_NODE);
    return error;
   }
  }
  kdirent_lock(self->d_parent,KDIRENT_LOCK_CACHE);
  __evalexpr(kdirentcache_remove(&self->d_parent->d_cache,self));
  kdirent_unlock(self->d_parent,KDIRENT_LOCK_CACHE);
  kinode_decref(mynode);
  return KE_OK;
 }
 if __unlikely((parnode = kdirent_getnode(self->d_parent)) == NULL) return KE_DESTROYED;
 if __likely((mynode = kdirent_getnode(self)) != NULL) {
#ifdef DO_RMDIR
  error = kinode_rmdir(parnode,&self->d_name,mynode);
#elif defined(DO_UNLINK)
  error = kinode_unlink(parnode,&self->d_name,mynode);
#elif defined(DO_REMOVE)
  error = kinode_remove(parnode,&self->d_name,mynode);
#endif
  kinode_decref(mynode);
 } else error = KE_DESTROYED;
 kinode_decref(parnode);
 return error;
}

#ifdef DO_RMDIR
#undef DO_RMDIR
#endif
#ifdef DO_UNLINK
#undef DO_UNLINK
#endif
#ifdef DO_REMOVE
#undef DO_REMOVE
#endif

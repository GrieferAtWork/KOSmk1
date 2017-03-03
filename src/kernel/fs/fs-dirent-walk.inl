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
#define WALKENV
#endif

#ifdef WALKENV
kerrno_t
kdirent_walkenv(struct kfspathenv const *__restrict env,
                struct kdirentname const *__restrict name,
                __ref struct kdirent **__restrict result)
#elif defined(WITHRESNODE)
kerrno_t
kdirent_walknode(struct kdirent *__restrict self,
                 struct kdirentname const *__restrict name,
                 __ref struct kdirent **__restrict result,
                 __ref struct kinode **__restrict resnode)
#else
kerrno_t
kdirent_walk(struct kdirent *__restrict self,
             struct kdirentname const *__restrict name,
             __ref struct kdirent **__restrict result)
#endif
{
 struct kdirent *resent,*newresult; struct kinode *inode;
 kerrno_t error; struct kinode *used_resnode;
 struct kdirentname used_name;
#ifdef WALKENV
 struct kdirent *__restrict self;
 kassertobj(env);
 kassertobj(result);
 kassert_kdirent(env->env_cwd);
 kassert_kdirent(env->env_root);
#ifdef WITHRESNODE
 kassertobj(resnode);
#endif
 self = env->env_cwd;
 switch (name->dn_size) {
  case 2: if (name->dn_name[1] != '.') break;
  case 1: if (name->dn_name[0] != '.') break;
   // Special directory '.' or '..'
   if (name->dn_size == 1 || env->env_root == self) {
    // Reference to own directory / attempt to surpass
    // the selected filesystem root is denied.
    error = kdirent_incref(*result = self);
   } else {
    // If this fails, 'env_root' can't be reached from 'env_cwd'
    kassert_kdirent(self->d_parent);
    // Permission to visible parent directory is granted
    error = kdirent_incref(*result = self->d_parent);
   }
#ifdef WITHRESNODE
   if (KE_ISOK(error) && __unlikely(
      (*resnode = kdirent_getnode(*result)) == NULL)) {
    kdirent_decref(*result);
    error = KE_DESTROYED;
   }
#endif
   return error;
  default: break;
 }
#endif
 kassert_kdirent(self);
 kassertobj(result);
#ifdef WITHRESNODE
 kassertobj(resnode);
#endif
 kdirent_lock(self,KDIRENT_LOCK_CACHE);
 resent = kdirentcache_lookup(&self->d_cache,name);
 if (resent) {
  __u32 refcnt;
  // NOTE: We must be careful here, as the entry may already be dead
  do {
   refcnt = katomic_load(resent->d_refcnt);
   if __unlikely(!refcnt) goto deadent;
   if __unlikely(refcnt == (__u32)-1) {
    kdirent_unlock(self,KDIRENT_LOCK_CACHE);
    return KE_OVERFLOW;
   }
  } while (!katomic_cmpxch(resent->d_refcnt,refcnt,refcnt+1));
  kdirent_unlock(self,KDIRENT_LOCK_CACHE);
  *result = resent;
#ifdef WITHRESNODE
  if __unlikely((*resnode = kdirent_getnode(resent)) == NULL) {
   kdirent_decref(resent);
   return KE_DESTROYED;
  }
#endif
  return KE_OK;
 }
deadent:
 kdirent_unlock(self,KDIRENT_LOCK_CACHE);
 /* Use the INode to walk the path (slow...) */
 if __unlikely((inode = kdirent_getnode(self)) == NULL) return KE_DESTROYED;
#ifdef WALKENV
 if (S_ISLNK(inode->i_kind) && !(env->env_flags&O_NOFOLLOW)) {
  /* Dereference symbolic link */
  struct kdirentname link_name;
  struct kfspathenv target_env;
  __ref struct kdirent *target_ent;
  assert(env->env_lnk <= LINK_MAX);
  if (env->env_lnk == LINK_MAX) { error = KE_LOOP; goto err_inode; }
  error = kinode_readlink(inode,&link_name);
  kinode_decref(inode);
  if __unlikely(KE_ISERR(error)) return error;
  kfspathenv_initfrom(&target_env,env,self,env->env_root);
  ++target_env.env_lnk;
  error = kdirent_walkall(&target_env,link_name.dn_name,link_name.dn_size,&target_ent);
  kdirentname_quit(&link_name);
  if __unlikely(KE_ISERR(error)) return error;
  target_env.env_cwd = target_ent;
  error = kdirent_walkenv(&target_env,name,result);
  kdirent_decref(target_ent);
  return error;
 }
#endif /* WALKENV */
 error = kinode_walk(inode,name,&used_resnode);
 kinode_decref(inode);
 if __unlikely(KE_ISERR(error)) return error;
#ifdef WITHRESNODE
 error = kinode_incref(*resnode = used_resnode);
 if __unlikely(KE_ISERR(error)) goto err_resnode;
#endif /* WITHRESNODE */
 // We've got a new node. Now we need to turn it into a dirent
 if __unlikely(KE_ISERR(error = kdirentname_initcopy(&used_name,name))) goto err_resnode2;
 if __unlikely(KE_ISERR(error = kdirent_incref(self))) goto err_name;
 resent = __kdirent_newinherited(self,&used_name,used_resnode);
 if __unlikely(!resent) {
  error = KE_NOMEM;
  kdirent_decref(self);
err_name:
  kdirentname_quit(&used_name);
err_resnode2:
#ifdef WITHRESNODE
#ifdef __DEBUG__
  assert(*resnode == used_resnode);
  *resnode = NULL;
#endif
  kinode_decref(used_resnode);
err_resnode:
#endif
  kinode_decref(used_resnode);
  return error;
 }
 // In order to speed up, try to insert the new node into the parent.
 // NOTE: If this fails, we can simply ignore the error,
 //       as the parent-child cache is completely optional.
 // EXCEPTION: If another task cached the same node while we were
 //            looking it up, we prefer using its dirent in order
 //            to somewhat reduce redundancy and memory consumption.
 kdirent_lock(self,KDIRENT_LOCK_CACHE);
 if (kdirentcache_insert(&self->d_cache,resent,&newresult) == KE_EXISTS &&
     KE_ISOK(kdirent_incref(newresult))) {
  *result = newresult; /* Inherit reference. */
  kdirent_unlock(self,KDIRENT_LOCK_CACHE);
  kdirent_decref(resent); // Drop our old reference
  return KE_OK;
 }
 kdirent_unlock(self,KDIRENT_LOCK_CACHE);
 *result = resent; /* Inherit reference. */
 return KE_OK;
#ifdef WALKENV
err_inode:
 kinode_decref(inode);
 return error;
#endif /* WALKENV */
}


#ifdef WALKENV
#undef WALKENV
#endif
#ifdef WITHRESNODE
#undef WITHRESNODE
#endif

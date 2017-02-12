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
#define MKLNK
#endif

#if !defined(MKLNK) && !defined(MKDIR) && !defined(MKREG)
#error "Must #define something before #including this file"
#endif

#ifdef MKLNK
kerrno_t kdirent_mklnk(struct kdirent *self, struct kdirentname const *name,
                       size_t ac, union kinodeattr const *av,
                       struct kdirentname const *target,
                       __ref /*opt*/struct kdirent **resent,
                       __ref /*opt*/struct kinode **resnode)
#elif defined(MKDIR)
kerrno_t kdirent_mkdir(struct kdirent *self,
                       struct kdirentname const *name,
                       size_t ac, union kinodeattr const *av, 
                       __ref /*opt*/struct kdirent **resent,
                       __ref /*opt*/struct kinode **resnode)
#elif defined(MKREG)
kerrno_t kdirent_mkreg(struct kdirent *self, struct kdirentname const *name,
                       __size_t ac, union kinodeattr const *av,
                       __ref /*opt*/struct kdirent **resent,
                       __ref /*opt*/struct kinode **resnode)
#endif
{
 struct kinode *node; kerrno_t error;
 struct kinode *used_resnode;
 struct kdirentname used_name;
 kassert_kdirent(self);
 kassertobj(name);
 kassertobjnull(resent);
 kassertobjnull(resnode);
#ifdef MKREG
 kassertmem(av,ac*sizeof(union kinodeattr));
#endif
 if __unlikely((node = kdirent_getnode(self)) == NULL) return KE_DESTROYED;
#ifdef MKLNK
 error = kinode_mklnk(node,name,ac,av,target,&used_resnode);
#elif defined(MKDIR)
 error = kinode_mkdir(node,name,ac,av,&used_resnode);
#elif defined(MKREG)
 error = kinode_mkreg(node,name,ac,av,&used_resnode);
#endif
 kinode_decref(node);
 if __unlikely(KE_ISERR(error)) return error;
 if (resent) {
  if (resnode && __unlikely(KE_ISERR(error = kinode_incref(*resnode = used_resnode)))) return error;
  if __unlikely(KE_ISERR(error)) {
err: if (resnode) kinode_decref(*resnode); return error;
  }
  if __unlikely(KE_ISERR(error = kdirent_incref(self))) {
err_usedresnode: kinode_decref(used_resnode); goto err;
  }
  if __unlikely(KE_ISERR(error = kdirentname_initcopy(&used_name,name))) {
err_self: kdirent_decref(self); goto err_usedresnode;
  }
  *resent = __kdirent_newinherited(self,&used_name,used_resnode);
  if __unlikely(!*resent) {
   kdirentname_quit(&used_name);
   goto err_self;
  }
 } else if (resnode) {
  *resnode = used_resnode; // Inherit reference
 } else {
  kinode_decref(used_resnode);
 }
 return error;
}


#ifdef MKLNK
#undef MKLNK
#endif
#ifdef MKDIR
#undef MKDIR
#endif
#ifdef MKREG
#undef MKREG
#endif

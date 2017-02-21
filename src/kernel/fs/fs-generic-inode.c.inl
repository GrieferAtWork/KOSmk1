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
#include "fs-generic.c.inl"
__DECL_BEGIN
#define GETATTR
#endif

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// --- KINODE
#ifdef GETATTR
kerrno_t
kinode_user_generic_getattr(struct kinode const *self, size_t ac,
                            __user union kinodeattr *av)
#else
kerrno_t
kinode_user_generic_setattr(struct kinode *self, size_t ac,
                            __user union kinodeattr const *av)
#endif
{
#ifdef GETATTR
 __user union kinodeattr *iter,*end;
        union kinodeattr attr;
#endif
 kassert_kinode(self);
 kassertmem(av,ac*sizeof(union kinodeattr));
#ifdef GETATTR
 end = (iter = av)+ac;
 while (iter != end) {
  if __unlikely(copy_from_user(&attr,iter,sizeof(attr))) return KE_FAULT;
  switch (attr.ia_common.a_id) {
   case KATTR_FS_ATIME:
   case KATTR_FS_CTIME:
   case KATTR_FS_MTIME:
    /* boot time? */
    memcpy(&attr.ia_time.tm_time,
           &KERNEL_BOOT_TIME,
           sizeof(struct timespec));
    break;
   case KATTR_FS_PERM:  attr.ia_perm.p_perm = 0777; break;
   case KATTR_FS_OWNER: attr.ia_owner.o_owner = 0; break;
   case KATTR_FS_GROUP: attr.ia_group.g_group = 0; break;
   case KATTR_FS_SIZE:  attr.ia_size.sz_size = 4096; break; /*< Eh... */
   case KATTR_FS_INO:   attr.ia_ino.i_ino = (__ino_t)self; break;
   case KATTR_FS_NLINK: attr.ia_nlink.n_lnk = 1; break;
   case KATTR_FS_KIND:  attr.ia_kind.k_kind = self->i_kind; break;
   default: return KE_NOSYS;
  }
  if __unlikely(copy_to_user(iter,&attr,sizeof(attr))) return KE_FAULT;
  ++iter;
 }
 return KE_OK;
#else
 (void)self,(void)av,(void)ac;
 return KE_NOSYS;
#endif
}

#ifdef GETATTR
#undef GETATTR
#endif

#ifdef __INTELLISENSE__
__DECL_END
#endif

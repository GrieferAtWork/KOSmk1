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
// --- KFILE
#ifdef GETATTR
kerrno_t kfile_generic_getattr(struct kfile const *__restrict self, kattr_t attr,
                               __user void *__restrict buf, size_t bufsize,
                               __kernel size_t *__restrict reqsize)
#else
kerrno_t kfile_generic_setattr(struct kfile *__restrict self, kattr_t attr,
                               __user void const *__restrict buf, size_t bufsize)
#endif
{
 kerrno_t error;
 kassert_kfile(self);
 kassertmem(buf,bufsize);
#ifdef GETATTR
 kassertobjnull(reqsize);
#endif
 switch (KATTR_GETTOKEN(attr)) {
   // Forward fs attributes to the file's inode
#ifdef GETATTR
  case KATTR_TOKEN_NONE:
   if (attr == KATTR_GENERIC_NAME) goto getfilename;
#endif
  {
   __ref struct kinode *filenode;
  case KATTR_TOKEN_FS:
#ifdef GETATTR
   if (attr == KATTR_FS_FILENAME) {
getfilename:
    return __kfile_user_getfilename_fromdirent(self,(char *)buf,bufsize,reqsize);
   }
   if (attr == KATTR_FS_PATHNAME) {
    kerrno_t error; struct kdirent *rootdirent;
    rootdirent = kproc_getfddirent(kproc_self(),KFD_ROOT);
    if __unlikely(!rootdirent) {
     // Return at least ~some~ information...
     return __kfile_user_getfilename_fromdirent(self,(char *)buf,bufsize,reqsize);
    }
    error = __kfile_user_getpathname_fromdirent(self,rootdirent,(char *)buf,bufsize,reqsize);
    kdirent_decref(rootdirent);
    return error;
   }
#endif
   if __unlikely((filenode = kfile_getinode((struct kfile *)self)) == NULL) return KE_NOSYS;
#ifdef GETATTR
   error = kinode_user_getattr_legacy(filenode,attr,buf,bufsize,reqsize);
#else
   error = kinode_user_setattr_legacy(filenode,attr,buf,bufsize);
#endif
   kinode_decref(filenode);
   return error;
  }

  default: break;
 }
 return KE_NOSYS;
}

#ifdef GETATTR
#undef GETATTR
#endif

#ifdef __INTELLISENSE__
__DECL_END
#endif

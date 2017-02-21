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
// --- KSUPERBLOCK
#ifdef GETATTR
kerrno_t
ksuperblock_user_generic_getattr(struct ksuperblock const *self, kattr_t attr,
                                 __user void *__restrict buf, size_t bufsize,
                                 __kernel size_t *__restrict reqsize)
#else
kerrno_t
ksuperblock_user_generic_setattr(struct ksuperblock *self, kattr_t attr,
                                 __user void const *__restrict buf, __size_t bufsize)
#endif
{
 kerrno_t error;
 kassert_ksuperblock(self);
#ifdef GETATTR
 kassertobjnull(reqsize);
#endif
 switch (KATTR_GETTOKEN(attr)) {
  {
   struct ksdev *mydev;
   /* Forward dev attributes to an associated device. */
  case KATTR_TOKEN_DEV:
   if ((mydev = ksuperblock_getdev(self)) != NULL) {
#ifdef GETATTR
    error = kdev_user_getattr((struct kdev *)mydev,attr,buf,bufsize,reqsize);
#else
    error = kdev_user_setattr((struct kdev *)mydev,attr,buf,bufsize);
#endif
    ksdev_decref(mydev);
    return error;
   }
   break;
  }

  default: break;
 }
 /* Fallback: Use the legacy INode attribute interface. */
#ifdef GETATTR
 return __kinode_user_getattr_legacy(ksuperblock_root(self),attr,buf,bufsize,reqsize);
#else
 return __kinode_user_setattr_legacy(ksuperblock_root(self),attr,buf,bufsize);
#endif
}

#ifdef GETATTR
#undef GETATTR
#endif

#ifdef __INTELLISENSE__
__DECL_END
#endif

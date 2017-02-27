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
#ifndef __MOD_C__
#define __MOD_C__ 1

#include <kos/config.h>
#ifndef __KERNEL__
#include <mod.h>
#include <kos/mod.h>
#include <kos/task.h>
#include <malloc.h>
#include <errno.h>
#include <stddef.h>
#include <limits.h>
#include <kos/syslog.h>

__DECL_BEGIN

__public mod_t mod_open(char const *name, __u32 flags) {
 mod_t result; kerrno_t error;
 error = kmod_open(name,(size_t)-1,&result,flags);
 if __likely(KE_ISOK(error)) return result;
 __set_errno(-error);
 return MOD_ERR;
}
__public mod_t mod_fopen(int fd, __u32 flags) {
 mod_t result; kerrno_t error;
 error = kmod_fopen(fd,&result,flags);
 if __likely(KE_ISOK(error)) return result;
 __set_errno(-error);
 return MOD_ERR;
}
__public int mod_close __D1(mod_t,mod) {
 kerrno_t error;
 /* NOTE: Don't allow mod_close(MOD_SELF), because
  *       the function would affect libc instead of
  *       what the caller probably meant as a joke... */
 if __unlikely(mod == MOD_SELF) error = KE_INVAL;
 else error = kmod_close(mod);
 if __likely(KE_ISOK(error)) return 0;
 __set_errno(-error);
 return -1;
}


__public modinfo_t *
__mod_info(mod_t mod, modinfo_t *buf,
           size_t bufsize, __u32 flags) {
 kerrno_t error; size_t reqsize;
 if (!bufsize) {
  modinfo_t *result,*newresult;
  size_t sizehint;
       if (flags&MOD_INFO_NAME) sizehint = sizeof(modinfo_t)+(PATH_MAX+1)*sizeof(char);
  else if ((flags&MOD_INFO_INFO) == MOD_INFO_INFO) sizehint = offsetof(modinfo_t,mi_name);
  else if ((flags&MOD_INFO_ID) == MOD_INFO_ID)     sizehint = offsetafter(modinfo_t,mi_id);
  else sizehint = 64;
  result = (modinfo_t *)malloc(sizehint);
  if __unlikely(!result) return NULL;
  for (;;) {
   result->mi_name = NULL;
   error = kmod_info(kproc_self(),mod,result,sizehint,&reqsize,flags);
   if __unlikely(KE_ISERR(error)) { free(result); goto err; }
   if __likely(reqsize <= sizehint) break;
   /* Must allocate more memory. */
   newresult = (modinfo_t *)realloc(result,reqsize);
   if __unlikely(!newresult) { free(result); return NULL; }
   /* Try again with a larger buffer. */
   result   = newresult;
   sizehint = reqsize;
  }
  if (reqsize != sizehint) {
   newresult = (modinfo_t *)realloc(result,reqsize);
   if (newresult) result = newresult;
  }
  return result;
 }
 error = kmod_info(kproc_self(),mod,buf,bufsize,&reqsize,flags);
 if __likely(KE_ISOK(error)) {
  if (reqsize > bufsize) { __set_errno(EBUFSIZ); return NULL; }
  return buf;
 }
err:
 __set_errno(-error);
 return NULL;
}

__public char *
__mod_name(mod_t mod, char *buf,
           size_t bufsize, __u32 flags) {
 modinfo_t info;
 kerrno_t error;
 if (!bufsize) {
  char *newbuf;
  /* Use a dynamic buffer. */
  bufsize = (PATH_MAX+1)*sizeof(char);
  buf = (char *)malloc(bufsize);
  if __unlikely(!buf) return NULL;
  for (;;) {
   info.mi_name = buf;
   info.mi_nmsz = bufsize;
   error = kmod_info(kproc_self(),mod,&info,sizeof(info),NULL,flags);
   if __unlikely(KE_ISERR(error)) {err_buf: free(buf); goto err; }
   if __unlikely(info.mi_name != buf) { error = KE_INVAL; goto err_buf; }
   if __likely(info.mi_nmsz <= bufsize) break;
   /* Must allocate more memory. */
   newbuf = (char *)realloc(buf,info.mi_nmsz);
   if __unlikely(!newbuf) { free(buf); return NULL; }
   /* Try again with a larger buffer. */
   buf     = newbuf;
   bufsize = info.mi_nmsz;
  }
  if (info.mi_nmsz != bufsize) {
   newbuf = (char *)realloc(buf,info.mi_nmsz);
   if (newbuf) buf = newbuf;
  }
  return buf;
 }
 info.mi_name = buf;
 info.mi_nmsz = bufsize;
 error = kmod_info(kproc_self(),mod,&info,sizeof(info),NULL,flags);
 if __unlikely(KE_ISERR(error)) goto err;
 if __unlikely(info.mi_name != buf) { __set_errno(EINVAL); return NULL; }
 if __unlikely(info.mi_nmsz > bufsize) { __set_errno(EBUFSIZ); return NULL; }
 return buf;
err: __set_errno(-error);
 return NULL;
}

__public syminfo_t *
__mod_syminfo(mod_t mod, struct ksymname const *addr_or_name,
              syminfo_t *buf, size_t bufsize, __u32 flags) {
 kerrno_t error; size_t reqsize;
 if (!bufsize) {
  syminfo_t *result,*newresult;
  size_t sizehint = sizeof(struct ksyminfo);
  if (flags&KMOD_SYMINFO_FLAG_WANTNAME) sizehint += (256+1)*sizeof(char);
  if (flags&KMOD_SYMINFO_FLAG_WANTFILE) sizehint += (256+1)*sizeof(char);
  result = (syminfo_t *)malloc(sizehint);
  if __unlikely(!result) return NULL;
  for (;;) {
   result->si_name = NULL;
   result->si_file = NULL;
   error = kmod_syminfo(kproc_self(),mod,addr_or_name,
                        result,sizehint,&reqsize,flags);
   if __unlikely(KE_ISERR(error)) { free(result); goto err; }
   if __likely(reqsize <= sizehint) break;
   /* Must allocate more memory. */
   newresult = (syminfo_t *)realloc(result,reqsize);
   if __unlikely(!newresult) { free(result); return NULL; }
   /* Try again with a larger buffer. */
   result   = newresult;
   sizehint = reqsize;
  }
  if (reqsize != sizehint) {
   newresult = (syminfo_t *)realloc(result,reqsize);
   if (newresult) result = newresult;
  }
  return result;
 }
 error = kmod_syminfo(kproc_self(),mod,addr_or_name,
                      buf,bufsize,&reqsize,flags);
 if __likely(KE_ISOK(error)) {
  if (reqsize > bufsize) { __set_errno(EBUFSIZ); return NULL; }
  return buf;
 }
err:
 __set_errno(-error);
 return NULL;
}



__DECL_END
#endif /* !__KERNEL__ */

#endif /* !__MOD_C__ */

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
#ifndef __KOS_KERNEL_STRING_C__
#define __KOS_KERNEL_STRING_C__ 1

#include <kos/compiler.h>
#include <kos/kernel/proc.h>
#include <kos/kernel/paging.h>
#include <kos/kernel/util/string.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <format-printer.h>

__DECL_BEGIN

__crit size_t
__user_memset_c(__user void *p, int byte, size_t bytes) {
 size_t maxbytes; __kernel void *kptr;
 KTASK_CRIT_MARK
 USER_FOREACH_BEGIN(p,bytes,kptr,maxbytes,1) {
  memset(kptr,byte,maxbytes);
 } USER_FOREACH_END({
  return USER_FOREACH_PENDING;
 });
 return 0;
}

__crit size_t
__user_strncpy_c(__user char *p, __kernel char const *s, size_t maxchars) {
 size_t maxbytes; __kernel void *kptr;
 KTASK_CRIT_MARK
 maxchars = strnlen(s,maxchars)*sizeof(char);
 USER_FOREACH_BEGIN(p,maxchars+1,kptr,maxbytes,1) {
  if (maxbytes > maxchars) {
   memcpy(kptr,s,maxchars);
   memset((char *)kptr+maxchars/sizeof(char),0,
          maxbytes-maxchars);
   maxchars = 0;
  } else {
   memcpy(kptr,s,maxbytes);
   s        += maxbytes;
   maxchars -= maxbytes;
  }
 } USER_FOREACH_END({
  return USER_FOREACH_PENDING;
 });
 return 0;
}

__crit ssize_t
__user_snprintf_c(__user char *buf, size_t bufsize,
                  /*opt*/__kernel size_t *reqsize,
                  __kernel char const *__restrict fmt, ...) {
 va_list args;
 ssize_t result;
 va_start(args,fmt);
 result = __user_vsnprintf_c(buf,bufsize,reqsize,fmt,args);
 va_end(args);
 return result;
}

struct user_vsnprintf_data {
 struct ktranslator trans;    /*< Used address translator. */
 __user   char     *u_bufpos; /*< [?..1] Start of the next user-buffer part. */
 __user   char     *u_bufend; /*< [?..1] End of the user-buffer. */
 __kernel char     *k_bufpos; /*< [?..1] Start of the current kernel-buffer part. */
 size_t             k_bufsiz; /*< Amount of available bytes starting at 'k_bufpos'. */
};
static int
user_vsnprintf_callback(__kernel char const *s, size_t maxlen,
                        struct user_vsnprintf_data *data) {
 size_t copysize;
 maxlen = strnlen(s,maxlen)*sizeof(char);
 for (;;) {
  copysize = min(maxlen,data->k_bufsiz);
  memcpy(data->k_bufpos,s,copysize);
  data->k_bufpos += copysize;
  data->k_bufsiz -= copysize;
  maxlen         -= copysize;
  if (!maxlen) break;
  assert(!data->k_bufsiz);
  /* Check for out-of-bounds / faulty pointer. */
  if (data->u_bufend <= data->k_bufpos) {
   data->k_bufpos += maxlen;
   return 0;
  }
  /* Must translate another part. */
  data->k_bufpos = (__kernel char *)ktranslator_exec(&data->trans,data->u_bufpos,
                                                    (size_t)(data->u_bufend-data->u_bufpos),
                                                     &data->k_bufsiz,1);
  if (!data->k_bufpos) {
   /* Faulty pointer (mark as out-of-bounds for now). */
   data->u_bufend = data->k_bufpos;
   assert(!data->k_bufsiz);
  } else {
   /* Advance the user-pointer to point into the next part. */
   data->u_bufpos += data->k_bufsiz;
   assert(data->u_bufpos <= data->u_bufend);
  }
 }
 return 0;
}


__crit ssize_t
__user_vsnprintf_c(__user char *buf, size_t bufsize,
                   /*opt*/__kernel size_t *reqsize,
                   __kernel char const *__restrict fmt, va_list args) {
 struct user_vsnprintf_data data;
 ssize_t result; __user char *real_bufend;
 KTASK_CRIT_MARK
 data.u_bufend = real_bufend = (data.u_bufpos = buf)+bufsize;
 data.k_bufpos = NULL,data.k_bufsiz = 0;
 if __unlikely(KE_ISERR(ktranslator_init(&data.trans,ktask_self()))) {
  /* Still need to evaluate the format string. */
  result = (ssize_t)vsnprintf(NULL,0,fmt,args);
  if (reqsize) *reqsize = result;
  return -result;
 }
 format_vprintf((pformatprinter)&user_vsnprintf_callback,
                &data,fmt,args);
 /* Append a terminating \0-character. */
 if (data.k_bufsiz) {
  *data.k_bufpos = '\0';
  --data.k_bufsiz;
 } else if (data.u_bufpos < data.u_bufend) {
  size_t avail_bytes;
  data.k_bufpos = (__kernel char *)ktranslator_exec(&data.trans,data.u_bufpos,
                                                    sizeof(char),&avail_bytes,1);
  if (data.k_bufpos) *data.k_bufpos = '\0';
  else data.u_bufend = data.u_bufpos;
  ++data.u_bufpos;
 }
 ktranslator_quit(&data.trans);
 /* Subtract memory not used by the translated kernel buffer. */
 data.u_bufpos -= data.k_bufsiz;
 /* When requested to, store the required size. */
 if (reqsize) *reqsize = (size_t)(data.u_bufpos-buf);
 /* If the user-buffer pointer is out-of-bounds, it
  * was either of insufficient size, or faulty.
  * If not, everything worked. */
 if (data.u_bufpos <= data.u_bufend) return 0;
 result = (ssize_t)(data.u_bufpos-data.u_bufend);
 /* The buffer was faulty if the end-pointer was moved. */
 assert(data.u_bufend <= real_bufend);
 if (data.u_bufend != real_bufend) result = -result;
 return result;
}




/* TODO: Everything in this file should use 'USER_FOREACH'! */

__crit __user void *
__user_memchr_c(__user void const *p, int needle, size_t bytes) {
 size_t partsize; __kernel void *partp;
 __user void *result = NULL;
 KTASK_CRIT_MARK
 USER_FOREACH_BEGIN(p,bytes,partp,partsize,0) {
  if ((result = memchr(partp,needle,partsize)) != NULL) {
   /* Convert the kernel-pointer into a user-pointer. */
   result = (__user void *)((uintptr_t)p+((uintptr_t)result-(uintptr_t)partp));
   USER_FOREACH_BREAK;
  }
 } USER_FOREACH_END({
  result = NULL;
 });
 return result;
}

__crit __kernel char *
__user_strndup_c(__user char const *s, size_t maxchars) {
 size_t bufsize; char *result;
 KTASK_CRIT_MARK
 bufsize = user_strnlen(s,maxchars);
 result = (char *)malloc((bufsize+1)*sizeof(char));
 if __unlikely(!result) return NULL;
 bufsize -= copy_from_user(result,s,bufsize);
 result[bufsize] = '\0';
#if 0
 {
  /* It is possible that the user screwed with their memory to truncate the string
   * >> We can do the same to possibly save memory.
   * NOTE: This isn't really required, and due to the
   *       additional call to strlen disabled by default... */
  size_t real_size = strlen(result);
  assert(real_size <= bufsize);
  if (real_size != bufsize) {
   char *newresult = (char *)realloc(result,(real_size+1)*sizeof(char));
   if (newresult) result = newresult;
  }
 }
#endif
 return result;
}

#define OPERATION_BUFFERSIZE  64


__crit size_t __user_strlen_c(__user char const *s) {
 size_t result = 0,partmaxsize,partsize; char *addr;
 struct ktranslator trans;
 KTASK_CRIT_MARK
 if __unlikely(KE_ISERR(ktranslator_init(&trans,ktask_self()))) return 0;
 for (;;) {
  addr = (char *)ktranslator_exec(&trans,s,PAGESIZE,&partmaxsize,0);
  if __unlikely(!addr) break; /* FAULT */
  partmaxsize /= sizeof(char);
  partsize = strnlen(addr,partmaxsize);
  result += partsize;
  if (partmaxsize != partsize) break;
  *(uintptr_t *)&s += partmaxsize;
 }
 ktranslator_quit(&trans);
 return result;
}
__crit size_t __user_strnlen_c(__user char const *s, size_t maxchars) {
 size_t result = 0,partmaxsize,partsize; char *addr;
 struct ktranslator trans;
 KTASK_CRIT_MARK
 if __unlikely(KE_ISERR(ktranslator_init(&trans,ktask_self()))) return 0;
 for (;;) {
  addr = (char *)ktranslator_exec(&trans,s,maxchars,&partmaxsize,0);
  if __unlikely(!addr) break; /* FAULT */
  partmaxsize /= sizeof(char);
  partsize = strnlen(addr,min(maxchars,partmaxsize))*sizeof(char);
  result += partsize;
  if (partmaxsize != (partsize/sizeof(char)) ||
      maxchars == partsize) break;
  *(uintptr_t *)&s += partmaxsize;
  maxchars         -= partsize;
 }
 ktranslator_quit(&trans);
 return result/sizeof(char);
}



__DECL_END

#endif /* !__KOS_KERNEL_STRING_C__ */

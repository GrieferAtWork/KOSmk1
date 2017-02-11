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
#ifndef __ENVIRON_C__
#define __ENVIRON_C__ 1

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <kos/atomic.h>
#include <kos/compiler.h>
#ifdef __KERNEL__
#include <kos/kernel/procenv.h>
#include <kos/kernel/proc.h>
#else
#include <kos/task.h>
#include <envlock.h>
#include <malloc.h>
#include <assert.h>
#include <kos/config.h>
#endif

__DECL_BEGIN

#ifdef __KERNEL__
__public int setenv(char const *__restrict name,
                    char const *value, int overwrite) {
 return kprocenv_setenv(&kproc_kernel()->p_environ,
                        name,(size_t)-1,value,(size_t)-1,overwrite);
}
__public int unsetenv(char const *__restrict name) {
 return kprocenv_delenv(&kproc_kernel()->p_environ,name,(size_t)-1);
}
__public int putenv(char *string) {
 return kprocenv_putenv(&kproc_kernel()->p_environ,string,(size_t)-1);
}
__public int clearenv(void) {
 kprocenv_clear(&kproc_kernel()->p_environ);
 return 0;
}
__public char *getenv(char const *__restrict name) {
 // WARNING: This isn't ~technically~ thread-safe, but it is
 //          still fine as long as the entire kernel is aware
 //          of this, and doesn't go modifying its own environment,
 //          which it shouldn't do to being with, considering that
 //          the kernel env is meant to house the grub commandline.
 return (char *)kprocenv_getenv(&kproc_kernel()->p_environ,name,(size_t)-1);
}
#else

//////////////////////////////////////////////////////////////////////////
// Allocates (and returns) a fresh copy of the
// process's environment, as known to the kernel.
// The format is: "foo=bar\0bar=42\0\0" (Terminated with two '\0'-es)
__local char *kos_allocenvtext(void) {
 char *result,*newresult; size_t bufsize,reqsize;
 kerrno_t error;
 bufsize = 512*sizeof(char);
 result = (char *)malloc(bufsize);
 if __unlikely(!result) return NULL;
again:
 error = kproc_enumenv(kproc_self(),result,bufsize,&reqsize);
 if __unlikely(KE_ISERR(error)) goto err;
 if (reqsize == bufsize) return result;
 newresult = (char *)realloc(result,reqsize);
 if __unlikely(!newresult) goto err_r;
 if (reqsize > bufsize) {
  bufsize = reqsize;
  result = newresult;
  goto again;
 }
 return newresult;
err:   __set_errno(-error);
err_r: free(result);
 return NULL;
}




static char *__env_default[] = {
 (char *)NULL
};

#undef environ
#undef __environ
extern char **environ;
__public char **environ = __env_default;
__public __atomic int __env_spinlock = 0;

// [0..1][owned_if(!in __envtext_begin:__envtext_end)]
// [0..1][owned_if(!= __env_empty)]
static char **__environ_v = __env_default;
static size_t __environ_c = 0;
static size_t __environ_a = 0;
#define SET_ENVIRON(v) (__environ_v = environ = (v))

// Our copy of the kernel environment text.
// NOTE: Entires within 'environ' that point within this region of
//       memory must not be free'd through use of free(), but instead
//       should be left alone.
static char *__envtext_begin = NULL; /*< [0..1][owned]. */
static char *__envtext_end   = NULL; /*< [0..1] End of '__envtext_begin'. */

#ifdef __LIBC_HAVE_DEBUG_MALLOC
static int __clearenv(int update_kos);
static void atexit_freeenv(void) { __clearenv(0); }
#endif

__private void user_initialize_environ(void) {
 size_t envc = 1; /*< Always have one for padding! */
 char *iter,**proc_environ,**dest;
 // The following check might seem redundant, but this
 // assertion is done to ensure that the linker has properly
 // initialized global (as in exported) variables.
 // >> If the following assertion fails, libc was either
 //    initialized more than once, or the linker is broken (again?)
 // TODO: This should be removed at some point, as this
 //       doesn't apply if a user overwrites 'environ'.
 assertf(environ == __env_default
        ,"environ = %p (expected: %p)"
        ,environ,&__env_default[0]);
 __envtext_begin = kos_allocenvtext();
 if __unlikely(!__envtext_begin) return;
 for (iter = __envtext_begin; *iter; iter = strend(iter)+1) ++envc;
 __envtext_end = iter;
 proc_environ = (char **)malloc(envc*sizeof(char *));
 if __unlikely(!proc_environ) goto err;
 iter = __envtext_begin; dest = proc_environ;
 for (iter = __envtext_begin; *iter; iter = strend(iter)+1) *dest++ = iter;
 assert(dest == proc_environ+(envc-1));
 *dest = NULL; // Terminate with NULL-character
 SET_ENVIRON(proc_environ);
 __environ_a = __environ_c = envc-1;
#ifdef __LIBC_HAVE_DEBUG_MALLOC
 // Prevent environ memory from producing leaks during exit
 atexit(&atexit_freeenv);
#endif
 return;
err:
 free(__envtext_begin);
 __envtext_end = __envtext_begin = NULL;
 SET_ENVIRON(__env_default);
 __environ_a = __environ_c = 0;
}

static int __clearenv(int update_kos) {
 char **iter,**end,**env,*line,*equals;
 char *old_text_begin,*old_text_end;
 size_t old_environ_c;
 ENV_LOCK;
 env = __environ_v,old_environ_c = __environ_c;
 SET_ENVIRON(__env_default),__environ_a = __environ_c = 0;
 old_text_begin = __envtext_begin,__envtext_begin = NULL;
 old_text_end = __envtext_end,__envtext_end   = NULL;
 ENV_UNLOCK;
 free(old_text_begin);
 assert((env != __env_default) == (old_environ_c != 0));
 if (old_environ_c) {
  // Free all environment table entries.
  end = (iter = env)+old_environ_c;
  for (; iter != end; ++iter) if ((line = *iter) != NULL) {
   if (update_kos && (equals = strchr(line,'=')) != NULL) {
    // Hint the kernel to delete this variable as well...
    kproc_delenv(kproc_self(),line,(size_t)(equals-line));
   }
   if (line < old_text_begin || line >= old_text_end) free(line);
  }
  free(env);
 }
 return 0;
}
__public int clearenv(void) {
 return __clearenv(1);
}

static int __setenv(char const *__restrict name, size_t name_size,
                    char const *value, size_t value_size, int overwrite) {
 char *existing_line,*line,**newenv; size_t newenva;
 // Inform the kernel of the change environment variable.
 // NOTE: We ignore errors in this call, because this kind-of is just a hint.
 kproc_setenv(kproc_self(),name,name_size,value,value_size,overwrite);
 line = (char *)malloc((name_size+value_size+2)*sizeof(char));
 if __unlikely(!line) goto no_mem;
 memcpy(line,name,name_size*sizeof(char));
 line[name_size] = '=';
 memcpy(line+name_size+1,value,value_size*sizeof(char));
 line[name_size+value_size+1] = '\0';
 ENV_LOCK;
 // Search for an existing line
 newenv = __environ_v+__environ_c;
 while (newenv-- != __environ_v) {
  existing_line = *newenv;
  if (strlen(existing_line) > name_size &&
      memcmp(existing_line,name,name_size) == 0 &&
      existing_line[name_size] == '=') {
   if (!overwrite) { free(line); goto end; }
   // Entry already exists
   if (existing_line < __envtext_begin ||
       existing_line >= __envtext_end
       ) free(existing_line);
   *newenv = line;
   goto end;
  }
 }
 if (__environ_a == __environ_c) {
  newenva = __environ_a ? __environ_a*2 : 2;
  newenv = (char **)realloc(__environ_v,(newenva+1)*sizeof(char *));
  if __unlikely(!newenv) { ENV_UNLOCK; goto no_mem; }
  SET_ENVIRON(newenv);
  __environ_a = newenva;
 } else {
  newenv = __environ_v;
 }
 newenv += __environ_c++;
 *newenv++ = line;
 *newenv = NULL;
end:
 ENV_UNLOCK;
 return 0;
no_mem:
 __set_errno(-ENOMEM);
 return -1;
}

__public int unsetenv(char const *__restrict name) {
 char **iter,**end,*line; size_t name_size;
 if __unlikely(!name) { __set_errno(EINVAL); return -1; }
 name_size = strlen(name);
 ENV_LOCK;
 end = (iter = __environ_v)+__environ_c;
 for (; iter != end; ++iter) if ((line = *iter) != NULL) {
  if (strlen(line) > name_size &&
      memcmp(line,name,name_size) == 0 &&
      line[name_size] == '=') {
   // Found it!
   if (line < __envtext_begin || line >= __envtext_end) free(line);
   --__environ_c;
   memmove(iter,iter+1,(__environ_c-(iter-__environ_v))*sizeof(char *));
   if (!__environ_c) {
    __environ_a = 0;
    if (__environ_v != __env_default) free(__environ_v);
    SET_ENVIRON(__env_default);
   }
   break;
  }
 }
 ENV_UNLOCK;
 return 0;
}


__public int setenv(char const *__restrict name,
                    char const *value, int overwrite) {
 if __unlikely(!name || !value || strchr(name,'=')
               ) { __set_errno(EINVAL); return -1; }
 return __setenv(name,strlen(name),value,strlen(value),overwrite);
}

__public char *getenv(char const *__restrict name) {
 char *e,**iter,*result; size_t namelen;
 if __unlikely(!name) return NULL;
 namelen = strlen(name);
 result = NULL;
 ENV_LOCK;
 assert((__environ_v != __env_default) == (__environ_c != 0));
 iter = __environ_v;
 while ((e = *iter++) != NULL) {
  if (strlen(e) > namelen &&
      memcmp(e,name,namelen) == 0 &&
      e[namelen] == '=') {
   result = e+(namelen+1);
   break;
  }
 }
 ENV_UNLOCK;
 return result;
}

__public int putenv(char *string) {
 char *equals = strchr(string,'=');
 if (!equals) return unsetenv(string);
 return __setenv(string,(size_t)(equals-string),
                 equals+1,strlen(equals+1),1);
}

#endif



__DECL_END

#endif /* !__ENVIRON_C__ */

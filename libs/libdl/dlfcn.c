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
#ifndef __DLFCN_C__
#define __DLFCN_C__ 1

#include <dlfcn.h>
#include <errno.h>
#include <kos/compiler.h>
#include <kos/atomic.h>
#include <kos/mod.h>
#include <malloc.h>
#include <string.h>

__DECL_BEGIN

struct dl {
 kmodid_t modid;
};

#undef strdupf /* Ignore that one, potential leak. */
static char *dl_lasterror; /*< [0..1] Last occurred error. */
static char *dl_currerror; /*< [0..1] Last reported error. */

#define dl_seterror(...) \
do{ char *new_error = strdupf(__VA_ARGS__);\
    free(katomic_xch(dl_lasterror,new_error));\
    katomic_store(dl_currerror,new_error);\
}while(0)


__public void *dlfopen(int fd, int mode) {
 struct dl *result;
 kerrno_t error;
 result = omalloc(struct dl);
 if __unlikely(!result) {
  dl_seterror("Not memory");
  return NULL;
 }
 error = kmod_fopen(fd,&result->modid,mode);
 if __unlikely(KE_ISERR(error)) {
  free(result);
  result = NULL;
  dl_seterror("dlfopen(%d) : %s",fd,strerror(error));
 }
 return result;
}

__public void *dlopen(char const *__restrict file, int mode) {
 struct dl *result;
 kerrno_t error;
 result = omalloc(struct dl);
 if __unlikely(!result) {
  dl_seterror("Not memory");
  return NULL;
 }
 error = kmod_open(file,(size_t)-1,&result->modid,mode);
 if __unlikely(KE_ISERR(error)) {
  free(result);
  result = NULL;
  dl_seterror("dlopen(%s) : %s",file,strerror(error));
 }
 return result;
}
__public int dlclose(void *handle) {
 kerrno_t error;
 kmodid_t modid;
 if (!handle) {
  dl_seterror("dlclose(NULL) : Invalid handle");
  return EINVAL;
 }
 modid = ((struct dl *)handle)->modid;
 error = kmod_close(modid);
 if (KE_ISOK(error)) { free(handle); return 0; }
 dl_seterror("dlclose(%p:%Iu) : %s",handle,modid,strerror(error));
 return -error;
}
__public void *dlsym(void *__restrict handle,
                     char const *__restrict name) {
 void *result;
 kmodid_t modid;
 if (!handle) {
  // TODO: Lookup global symbol
  modid = (kmodid_t)-1; // TODO: global id?
 } else {
  modid = ((struct dl *)handle)->modid;
 }
 result = kmod_sym(modid,name,(size_t)-1);
 if (!result) {
  // TODO: Include the module's name in this error message
  dl_seterror("dlsym(%p:%Iu) : Symbol '%s' not found",handle,modid,name);
 }
 return result;
}
__public char *dlerror(void) {
 return katomic_xch(dl_currerror,NULL);
}

#ifndef __STDC_PURE__
__public __compiler_ALIAS(_dlfopen,dlfopen);
#endif

__DECL_END

#endif /* !__DLFCN_C__ */

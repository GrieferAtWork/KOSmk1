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
#include <stdint.h>
#include <mod.h>

__DECL_BEGIN

// These offsets are defined to have a dl-pointer of...
//  ... 'NULL'/'RTLD_DEFAULT' == 'KMODID_ALL'
//  ... 'RTLD_NEXT'           == 'KMODID_NEXT'
#define ID_2_PTR(id)  ((void *)((kmodid_t)(id)+1))
#define PTR_2_ID(ptr) ((kmodid_t)((uintptr_t)(void *)(ptr))-1)
                          

static char *dl_lasterror = NULL; /*< [0..1] Last occurred error. */
static char *dl_currerror = NULL; /*< [0..1] Last reported error. */

#define dl_seterror(...) \
do{ char *new_error = (strdupf)(__VA_ARGS__);\
    free(katomic_xch(dl_lasterror,new_error));\
    katomic_store(dl_currerror,new_error);\
}while(0)


__public void *_dlfopen(int fd, int mode) {
 kerrno_t error; kmodid_t result;
 error = kmod_fopen(fd,&result,(__u32)mode);
 if __unlikely(KE_ISERR(error)) {
  dl_seterror("kmod_fopen(%d) : %s",fd,strerror(error));
  return NULL;
 }
 return ID_2_PTR(result);
}

__public void *dlopen(char const *__restrict file, int mode) {
 kmodid_t result; kerrno_t error;
 error = kmod_open(file,(size_t)-1,&result,(__u32)mode);
 if __unlikely(KE_ISERR(error)) {
  dl_seterror("kmod_open(%q) : %s",file,strerror(error));
  return NULL;
 }
 return ID_2_PTR(result);
}
__public int dlclose(void *handle) {
 kmodid_t modid = PTR_2_ID(handle);
 kerrno_t error = kmod_close(modid);
 if (KE_ISOK(error)) return 0;
 dl_seterror("kmod_close(%Iu) : %s",
             modid,strerror(error));
 return -error;
}
__public void *dlsym(void *__restrict handle,
                     char const *__restrict name) {
 kmodid_t modid = PTR_2_ID(handle);
 void *result = kmod_sym(modid,name,(size_t)-1);
 if (!result) {
  // TODO: Include the module's name in this error message
  dl_seterror("kmod_sym(%Iu,%q) : Symbol not found",modid,name);
 }
 return result;
}



__public int dladdr(void *addr, Dl_info *info) {
 /* We must use a static buffer for this, because
  * all the information is managed by the kernel. */
 static syminfo_t *last_syminfo = NULL;
 static modinfo_t *last_modinfo = NULL;
 syminfo_t *syminfo;
 modinfo_t *modinfo;
 /* Generate symbol and module */
 syminfo = __mod_syminfo(MOD_ALL,(struct ksymname *)addr,
                         NULL,0,KMOD_SYMINFO_FLAG_WANTNAME|
                         KMOD_SYMINFO_FLAG_LOOKUPADDR);
 if __unlikely(!syminfo) return -1;
 modinfo = __mod_info(syminfo->si_modid,NULL,0,
                      MOD_INFO_NAME|MOD_INFO_INFO);
 if __unlikely(!modinfo) { free(syminfo); return -1; }
 /* Exchange the latest symbol information, freeing the old stuff.
  * NOTE: We untrack the new information because it might not get freed... */
 free(katomic_xch(last_syminfo,_malloc_untrack(syminfo)));
 free(katomic_xch(last_modinfo,_malloc_untrack(modinfo)));

 info->dli_fname = modinfo->mi_name;
 info->dli_fbase = modinfo->mi_base;
 info->dli_saddr = syminfo->si_base;
 info->dli_sname = syminfo->si_name;
 return 0;
}

__public char *dlerror(void) {
 return katomic_xch(dl_currerror,NULL);
}

__DECL_END

#endif /* !__DLFCN_C__ */

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
#ifndef __MOD_H__
#define __MOD_H__ 1

#include <kos/config.h>
#ifndef __KERNEL__
#include <kos/compiler.h>
#include <kos/mod.h>
#ifndef __INTELLISENSE__
#include <kos/task.h>
#include <errno.h>
#endif

#ifdef __STDC_PURE__
#warning "This header is only available on KOS platforms."
#endif

//////////////////////////////////////////////////////////////////////////
// GLibc - style wrapper functions for KOS module/shared_lib/dynlib system calls.
// >> Functions behave the same as their <kos/mod.h> counterparts,
//    though they are integrated into the errno system and return
//    -1 / NULL / MOD_ERR on error, and 0 (or some other, meaningful value) on success.
// >> Detailed documentations can be found in <kos/mod.h>

__DECL_BEGIN

#define MOD_ALL  KMODID_ALL  /*< Search for symbols in all loaded modules. */
#define MOD_NEXT KMODID_NEXT /*< Search for symbols in all loaded modules, checking the calling module last. */
#define MOD_SELF KMODID_SELF /*< Search for symbols in the calling module. */
#define MOD_KERN KMODID_KERN /*< Reserved for future use: Special kernel-based module containing statically loaded user-space helper functions. */

#define MOD_ERR  ((kmodid_t)-5)

#ifndef __mod_t_defined
#define __mod_t_defined 1
typedef kmodid_t mod_t;
#endif

#ifndef __modinfo_t_defined
#define __modinfo_t_defined 1
typedef struct kmodinfo modinfo_t;
#endif


//////////////////////////////////////////////////////////////////////////
// Open/Close a module, given ids ID.
// @param: flags: A set of 'MOD_OPEN_*'
extern __wunused __nonnull((1))  mod_t mod_open  __P((char const *__restrict __name, __u32 __flags));
extern __wunused                 mod_t mod_fopen __P((int __fd, __u32 __flags));
extern                           int   mod_close __P((mod_t __mod));
#define MOD_OPEN_NONE  KMOD_OPEN_FLAG_NONE

//////////////////////////////////////////////////////////////////////////
// Lookup a symbol within a module, given its name.
// @param: MOD:   The module ID, as returned by mod_(f)open, or one of 'MOD_*'.
// @return: NULL: The given symbol could not be found.
__local __wunused __nonnull((2)) void *mod_sym __P((mod_t __mod, char const *__restrict __symname));

//////////////////////////////////////////////////////////////////////////
// Returns the real id of the given module and validate it to be correct.
// NOTE: This function can be used to figure out your own module id.
__local __wunused mod_t mod_id __P((mod_t __mod));


//////////////////////////////////////////////////////////////////////////
// NOTE: The following functions follow getcwd-style semantics, allowing you
//       to pass ZERO(0) for BUFSIZE to have the function return a buffer
//       newly allocated with malloc() (meaning you'll have to free() it).
// >> char *name = mod_file(MOD_SELF,NULL,0);
// >> printf("My module file is called %q\n",name);
// >> free(name);
// WARNING: When 'mod_path' is used to retrieve the full path of a module,
//          and the module lies outside the active chroot-prison, the
//          equivalent of 'mod_file' is returned instead.
__local __wunused modinfo_t *mod_info __P((mod_t __mod, modinfo_t *__buf, __size_t __bufsize, __u32 __flags));
__local __wunused char      *mod_file __P((mod_t __mod, char *__buf, __size_t __bufsize));
__local __wunused char      *mod_path __P((mod_t __mod, char *__buf, __size_t __bufsize));
#define MOD_INFO_NONE   KMOD_INFO_FLAG_NONE
#define MOD_INFO_ID     KMOD_INFO_FLAG_ID
#define MOD_INFO_INFO   KMOD_INFO_FLAG_INFO
#define MOD_INFO_NAME   KMOD_INFO_FLAG_NAME
#define MOD_INFO_PATH   KMOD_INFO_FLAG_PATH


extern __wunused modinfo_t *__mod_info __P((mod_t __mod, modinfo_t *__buf, __size_t __bufsize, __u32 __flags));
extern __wunused char      *__mod_name __P((mod_t __mod, char *__buf, __size_t __bufsize, __u32 __flags));




#ifndef __INTELLISENSE__
// In order to support the 'MOD_SELF' symbolic module id,
// we must generate these wrapper functions statically.
__local __wunused mod_t mod_id __D1(mod_t,__mod) {
 mod_t __result;
 kerrno_t __error = kmod_info(kproc_self(),__mod,(modinfo_t *)&__result,
                              sizeof(__result),NULL,MOD_INFO_ID);
 if __unlikely(KE_ISERR(__error)) { __set_errno(-__error); return MOD_ERR; }
 return __result;
}
__local void *mod_sym __D2(mod_t,__mod,char const *,__symname) {
 void *__result = kmod_sym(__mod,__symname,(size_t)-1);
 if __unlikely(!__result) __set_errno(ENOENT);
 return __result;
}
__local __wunused modinfo_t *mod_info __D4(mod_t,__mod,modinfo_t *,__buf,
                                           __size_t,__bufsize,__u32,__flags) {
 if (__mod == MOD_SELF && (__mod = mod_id(MOD_SELF)) == MOD_ERR) return NULL;
 return __mod_info(__mod,__buf,__bufsize,__flags);
}
__local __wunused char *mod_file __D3(mod_t,__mod,char *,__buf,__size_t,__bufsize) {
 if (__mod == MOD_SELF && (__mod = mod_id(MOD_SELF)) == MOD_ERR) return NULL;
 return __mod_name(__mod,__buf,__bufsize,MOD_INFO_NAME);
}
__local __wunused char *mod_path __D3(mod_t,__mod,char *,__buf,__size_t,__bufsize) {
 if (__mod == MOD_SELF && (__mod = mod_id(MOD_SELF)) == MOD_ERR) return NULL;
 return __mod_name(__mod,__buf,__bufsize,MOD_INFO_PATH);
}
#endif


__DECL_END
#endif /* !__KERNEL__ */

#endif /* !__MOD_H__ */

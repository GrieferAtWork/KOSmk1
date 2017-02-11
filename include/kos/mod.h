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
#ifndef __KOS_DL_H__
#define __KOS_DL_H__ 1

#include <kos/config.h>
#include <kos/compiler.h>
#ifndef __NO_PROTOTYPES
#include <kos/errno.h>
#include <kos/types.h>
#include <kos/syscall.h>
#endif /* !__NO_PROTOTYPES */

//////////////////////////////////////////////////////////////////////////
// KOS Module (aka. dynamic/shared library) User-level Interface.

__DECL_BEGIN

#ifndef __kmodid_t_defined
#define __kmodid_t_defined 1
typedef __kmodid_t kmodid_t;
#endif

#ifndef __NO_PROTOTYPES

//////////////////////////////////////////////////////////////////////////
// Opens a module within the calling process,
// as designated through the given 'name'.
// - Unlike many other APIs, 'kmod_open' is not restricted by
//   chroot prisons, always resolving the given name in origin
//   to the true filesystem root.
// - This does however not pose a security risk, as loading
//   a module is a read-only operation from start to finish.
// - I should also mention, that this only applies to resolution
//   of previously set module paths. - doing calling something like:
//   >> open2(KFD_ROOT,"/home");
//   >> kmod_open("/usr/lib/libc.so",...);
//   will fail, but something like this won't, assuming "/usr/lib"
//   was part of the search path set when the process was started:
//   >> open2(KFD_ROOT,"/home");
//   >> kmod_open("libc.so",...);
// Plus: Wouldn't make much sense to trap a program if doing
//       so prevents it from running all-together...
// - Though 'name' usually isn't absolute in any case, and
//   in that likely case of it not being, the given name
//   is resolved as required by a standard-conforming 'dlopen'
//  (Which will probably be what you'll call instead of this...)
// NOTE: You should note, that this function can actually do
//       more than just load position-independent so-files,
//       and is capable of loading any kind of executable
//       into the address space of an existing process.
//       Though modules cannot be loaded more than once (with
//       KOS telling them apart based on their absolute
//       and sanitized filename), it is possible to load
//       two completely different applications into the same
//       process (as long none of the new process's addresses
//       clash with those of the existing process)
// NOTE: Some executable formats are not supported for such
//       loading at a later point (most notably shebang).
// @param: modid: Filled with an identifier that can be used
//                to unload the module, after successful return
//                of this function, at a later point, as well
//                as load symbols from that module.
// @return: KE_OK:        The module was successfully loaded.
// @return: KE_NOENT:     [kmod_open] No module matching the given name was found.
// @return: KE_ACCES:     [kmod_open] The caller doesn't have access to the only matching executable/dependency.
// @return: KE_BADF:      [kmod_fopen] Bad file-descriptor.
// @return: KE_NOMEM:     Insufficient memory.
// @return: KE_RANGE:     The specified module is position-dependent,
//                        but the region of memory required to load
//                        the module to is already in use.
// @return: KE_NOSPC:     The specified module is position-independent,
//                        but no region of memory big enough to hold it
//                        could be found (Damn, yo address space cluttered...).
// @return: KE_NOEXEC:    A file matching the given name was found, but it, or one
//                        of its dependencies isn't an executable recognized by KOS.
// @return: KE_NODEP:     At least one dependencies required by the module could not be found.
// @return: KE_LOOP:      The library and its dependencies are creating an unresolvable loop
// @return: KS_UNCHANGED: The given module was already loaded. (*modid was filled with
//                        its ID, and 'kmod_close' must be invoked one additional time
//                        to ~actually~ close it)
// @return: KE_OVERFLOW:  A reference counter would have overflown (the library is loaded too often)
// @return: KE_ISERR(*):  Some other, possibly file-specific error has occurred
_syscall4(kerrno_t,kmod_open,char const *,name,
          __size_t,namemax,kmodid_t *,modid,__u32,flags);
_syscall3(kerrno_t,kmod_fopen,int,fd,kmodid_t *,modid,__u32,flags);
#define KMOD_OPEN_FLAG_NONE 0x00000000

//////////////////////////////////////////////////////////////////////////
// Closes a given module, unmapping all memory tabs associated.
// WARNING: It is possible for some portion of an executable
//          to close it's own module. (The simplest way of
//          doing so being 'kmod_close(0)', assuming that
//          the executable was started by regular means, with the
//          initial process being loaded at module slot ZERO).
//          Doing so does ~theoretically~ work, though you
//          should not that as soon as the kernel returns control
//          to you, your application will SEGFAULT.
// @return: KE_OK:    The module associated with the given id was successfully closed.
// @return: KE_INVAL: Invalid/already closed module id.
_syscall1(kerrno_t,kmod_close,kmodid_t,modid);

//////////////////////////////////////////////////////////////////////////
// Load a symbol from a given module, given its name.
// @return: NULL: Something went wrong (probably just doesn't export the given symbol)
_syscall3(void *,kmod_sym,kmodid_t,modid,
          char const *,name,__size_t,namemax);

#endif /* !__NO_PROTOTYPES */

struct kmodinfo {
 void *mi_base;       /*< Base address to which the given module was loaded. */
 void *mi_padding[6]; /*< Padding (unused) data. */
 char  mi_name[1];    /*< ZERO-terminated module name. (Filename only; no associated path) */
};

#ifndef __NO_PROTOTYPES
//////////////////////////////////////////////////////////////////////////
// Query information on a loaded module by its id.
// @return: KE_OK:    The given buffer was filled with information.
//                    NOTE: Depending on the value of '*reqsize',
//                          not all information may have been loaded.
// @return: KE_INVAL: Invalid/already closed module id.
// @return: KE_NOSYS: An unsupported bit was set in 'flags'
_syscall5(kerrno_t,kmod_info,kmodid_t,modid,struct kmodinfo *,buf,
          __size_t,bufsize,__size_t *,reqsize,__u32,flags);
#define KMOD_INFO_FLAG_NONE 0x00000000
#define KMOD_INFO_FLAG_BASE 0x00000001 /*< Fill 'mi_base' with the module's base address. */
#define KMOD_INFO_FLAG_NAME 0x00000002 /*< Fill 'mi_name' with the module's name. */
#endif /* !__NO_PROTOTYPES */


__DECL_END

#endif /* !__KOS_DL_H__ */

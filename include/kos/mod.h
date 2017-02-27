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

/* Special/symbolic module ids. */
#define KMODID_ALL   ((kmodid_t)-1) /*< Search for symbols in all loaded modules. */
#define KMODID_NEXT  ((kmodid_t)-2) /*< Search for symbols in all loaded modules, checking the calling module last. */
#define KMODID_SELF  ((kmodid_t)-3) /*< Search for symbols in the calling module. */
#define KMODID_KERN  ((kmodid_t)-4) /*< Reserved for future use: Special kernel-based module containing statically loaded user-space helper functions. */
#define KMODID_ISNORMAL(id)  ((id) < KMODID_KERN)
#define KMODID_ISSPECIAL(id) ((id) >= KMODID_KERN)

#ifndef __NO_PROTOTYPES

//////////////////////////////////////////////////////////////////////////
// Opens a module within the calling process,
// as designated through the given 'name'.
// - Unlike many other APIs, 'kmod_open' is not restricted by chroot prisons,
//   always resolving the given name in origin to the true filesystem root
//  (if it doesn't contain any slashes; i.e. is just the filename).
// - This does however not pose a security risk, as loading
//   a module is a read-only operation from start to finish,
//   with library that have the SETUID flag set being responsible
//   to prevent abuse by never returning to non-privileged code
//   after having done a rootfork().
// - doing calling something like:
//   >> open2(KFD_ROOT,"/home");
//   >> kmod_open("/usr/lib/libc.so",...);
//   will fail, but something like this won't:
//   >> open2(KFD_ROOT,"/home");
//   >> kmod_open("libc.so",...);
//   NOTE: The list of special prison-privileged library
//         paths is currently hard-coded as "/lib:/usr/lib",
//         though a way of setting this will be added
//        (and also require root privileges).
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
//       process (as long as none of the new binaries addresses
//       clash with those of already the already existing binary)
// NOTE: Some executable formats are not supported for such
//       loading at a later point (most notably shebang).
// @param: modid: Filled with an identifier that can be used
//                to unload the module, after successful return
//                of this function, at a later point, as well
//                as load symbols from that module.
// @param: flags: A set of 'KMOD_OPEN_FLAG_*'.
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
// @return: KE_FAULT:     A faulty pointer was given.
// @return: KE_ISERR(*):  Some other, possibly file-specific error has occurred
__local _syscall4(kerrno_t,kmod_open,char const *,name,
                  __size_t,namemax,kmodid_t *,modid,__u32,flags);
__local _syscall3(kerrno_t,kmod_fopen,int,fd,kmodid_t *,modid,__u32,flags);
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
// @param: modid: A valid module id obtained from 'kmod_open' or 'kmod_fopen'.
//                Because there is ~some~ sense to it, you can specify 'KMODID_*',
//                though I'd really recommend not doing a call like
//               'kmod_close(KMODID_ALL)', because that just makes too much sense...
//               (I guess it would be useful if you need to cover your tracks,
//                but then again, why isn't whoever you're running from just
//                suspending you?)
// @return: KE_OK:        The module associated with the given id was successfully closed.
// @return: KE_INVAL:     Invalid/already closed module id.
// @return: KS_UNCHANGED: The module load counter was decreased, but didn't hit zero.
//                        -> The module remains loaded because of multiple calls to 'kmod_fopen'
__local _syscall1(kerrno_t,kmod_close,kmodid_t,modid);

//////////////////////////////////////////////////////////////////////////
// Load a symbol from a given module, given its name.
// NOTE: This function will also search all dependencies of
//       the given module for the same symbol, only failing
//       if neither the module itself, nor any of its
//       dependencies carry it.
// @param: modid: A valid module id obtained from 'kmod_open' or 'kmod_fopen',
//                or one of the special 'KMODID_*' module ids.
//                In case of 'KMODID_NEXT', the calling module is loaded last.
// @param: name:  The name of a given symbol. Together with
//                'namemax', it describes an strnlen-style string.
// @return: NULL: Something went wrong (probably just doesn't export the given symbol)
__local _syscall3(void *,kmod_sym,kmodid_t,modid,
                  char const *,name,__size_t,namemax);
#endif /* !__NO_PROTOTYPES */


#ifndef __kmodkind_t_defined
#define __kmodkind_t_defined 1
typedef __u32 kmodkind_t;
#endif

/* Module kind flags & utilities. */
#define KMODFLAG_NONE       0x00000000
#define KMODFLAG_BINARY     0x00000100 /*< FLAG: Module is a binary. */
#define KMODFLAG_FIXED      0x00000200 /*< FLAG: The module has no relocation information and must be loaded to a fixed address. */
#define KMODFLAG_PREFFIXED  0x00000400 /*< FLAG: The module prefers being loaded to a fixed address (base = NULL), but is capable of being relocated elsewhere. */
#define KMODFLAG_CANEXEC    0x00000800 /*< FLAG: The module can be executed (NOTE: ). */
#define KMODFLAG_LOADED     0x80000000 /*< FLAG: Always set; Used internally to prevent dependecy loops (the library and all dependencies were loaded successfully). */
#define KMODKIND_FMASK      0xffffff00 /*< Mask for flags (KMODFLAG_*). */
#define KMODKIND_KMASK      0x000000ff /*< Mask for kind (KMODKIND_*). */
#define KMODKIND_KIND(x)  ((x)&KMODKIND_KMASK)
#define KMODKIND_FLAGS(x) ((x)&KMODKIND_FMASK)

/* Module technologies. */
#define KMODKIND_UNKNOWN    0 /*< Unknown binary type (May safely be used for error-cases). */
#define KMODKIND_ELF32      1 /*< ELF-32 (.../.so) binary/shared library. */
#define KMODKIND_PE32       2 /*< PE-32 (.exe/.dll) binary/shared library. */
#define KMODKIND_KERNEL  0xff /*< Exclusively used by the kernel itself. */

#ifndef __kmodinfo_defined
#define __kmodinfo_defined 1
struct kmodinfo {
    /* The kind of module/binary.
     * - One of 'KMODKIND_*'.
     * - Potentially or-ed with addition flags.
     * - Use 'KMODKIND_KIND' to extract the raw switch-capable kind.
     * - Use 'KMODKIND_FLAGS' to extract flags.
     */
    kmodid_t   mi_id;         /*< The real module id. */
    kmodkind_t mi_kind;       /*< The kind of module/binary. */
    void      *mi_base;       /*< Base address to which the given module was loaded. */
    /* == Section-based address range ==
     * Given an instruction-pointer, one can determine what module
     * the given pointer belongs to through use of its section range:
     * >> if (eip >= module->mi_begin && eip < module->mi_end) found_module();
     * - 'mi_begin' refers to the lowest address belonging to a given
     *    module, while 'mi_end' refers to that after the greatest.
     * - Note, that depending on the module technology, there may
     *   be portions of memory between begin and end not actually
     *   mapped to that module (section holes).
     *   While the kernel is aware of those holes, even capable
     *   of using them when unmapped memory starts getting scarce,
     *   due to their rarity there is no reason for detecting them
     *   in user-land (but prove me wrong on this and I'll add something).
     */
    void      *mi_begin;      /*< Begin of section-based memory. */
    void      *mi_end;        /*< End of section-based memory. */
    void      *mi_start;      /*< Module entry point (undefined/technology-specific when 'KMODFLAG_CANEXEC' isn't set). */
    char      *mi_name;       /*< [0..1] ZERO-terminated module file/path name. */
    __size_t   mi_nmsz;       /*< Size of 'mi_name' (WARNING: is bytes & including the terminating \0-character) */
    /* Future members & dynamic buffer space are located here. */
};
#endif

#ifndef __NO_PROTOTYPES
//////////////////////////////////////////////////////////////////////////
// Query information on a loaded module, given its id.
// @param: procfd:  A process/task descriptor for the process
// @param: modid:   A module id to query information for, or 'KMODID_SELF'
//                  for the calling module, 'KMODID_NEXT' for the next module after self,
//                  or 'KMODID_ALL' for the root executable, as also listed in k_syslog.
// @param: bufsize: The supplied buffer size.
// @param: reqsize: When non-NULL, store the required buffer size.
// @return: KE_OK:    The given buffer was filled with information.
//                    NOTE: Depending on the value of '*reqsize',
//                          not all information may have been loaded.
// @return: KE_BADF:  The given descriptor for 'procfd' is invalid, or not a task/process.
// @return: KE_INVAL: Invalid/already closed module id.
// @return: KE_NOSYS: An unsupported bit was set in 'flags'.
// @return: KE_FAULT: A faulty pointer was given.
__local _syscall6(kerrno_t,kmod_info,int,procfd,kmodid_t,modid,
                  struct kmodinfo *,buf,__size_t,bufsize,
                  __size_t *,reqsize,__u32,flags);
#endif /* !__NO_PROTOTYPES */

#define KMOD_INFO_FLAG_NONE 0x0000

/* Fill in 'mi_id' */
#define KMOD_INFO_FLAG_ID   0x0001

/* Fill Generic module information.
 * NOTE: Implies 'KMOD_INFO_FLAG_ID'. */
#define KMOD_INFO_FLAG_INFO 0x0003

/* Fill 'mi_name' with the module's name (filename w/o path)
 * NOTE: On input, use 'mi_nmsz' as bufsize; On output, store reqsize in 'mi_nmsz'.
 * NOTE: If 'mi_nmsz' is ZERO(0) or 'mi_name' is NULL, try to use memory provided
 *       by 'buf', including the size required for the name in '*reqsize'. */
#define KMOD_INFO_FLAG_NAME 0x0004

/* Fill 'mi_name' with the module's path (filename w/ path)
 * NOTE: Implies 'KMOD_INFO_FLAG_NAME'. */
#define KMOD_INFO_FLAG_PATH 0x000C

__DECL_END

#endif /* !__KOS_DL_H__ */

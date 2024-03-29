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
//  (Though only if it doesn't contain any slashes; i.e. is just the filename).
// - This does however not pose a security risk, as loading
//   a module is a read-only operation from start to finish,
//   with libraries that have the SETUID flag set being responsible
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
//       and sanitized filenames), it is possible to load
//       two completely different, non-relocatable applications
//       into the same process (so long as none of their section
//       addresses clash with those of already existing binary)
//    >> With this in mind, you could write a program that
//       relocates itself to some high memory location before
//       loading another binary, which it can then proceed
//       to modify in some way (such as changing data of its
//       BSS section), before finally passing over control,
//       thus allowing for yet another way of secret handshakes
//       between applications, as well as custom loaders.
// WARNING: Some executable formats are not supported for such
//          loading at a later point (most notably shebang).
// @param: modid:   Filled with an identifier that can be used
//                  to unload the module at a later point and
//                  after a successful return from this function, as
//                  well as explicitly load symbols from that module.
// @param: flags:   A set of 'KMOD_OPEN_FLAG_*'.
// @param: namemax: strnlen-style max length of the given 'name'.
// @return: KE_OK:        The module was successfully loaded.
// @return: KE_NOENT:     [kmod_open] No module matching the given name was found.
// @return: KE_ACCES:     [kmod_open] The caller doesn't have access to the only matching executable/dependency.
// @return: KE_BADF:      [kmod_fopen] Bad file-descriptor.
// @return: KE_NOSYS:     Some file interface required by the linker is not supported
//                        by the specified file (stop trying to kmod_fopen() a pipe!)
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
//                Because there is ~some~ sense to it, you can also specify one of the
//                'KMODID_*' value, though I'd really recommend not doing a call like
//                'kmod_close(KMODID_ALL)', because that'd just makes too much sense...
//               (I guess it would be useful if you need to cover your tracks,
//                but then again, why isn't whoever you're running from just
//                ktask_suspend()-ing you?)
// @return: KE_OK:        The module associated with the given id was successfully closed.
// @return: KE_INVAL:     Invalid/already closed module id.
// @return: KS_UNCHANGED: The module load counter was decreased, but didn't hit zero.
//                        -> The module remains loaded because of multiple calls to 'kmod_{f}open'
__local _syscall1(kerrno_t,kmod_close,kmodid_t,modid);

//////////////////////////////////////////////////////////////////////////
// Load a symbol from a given module, given its name.
// NOTE: This function will also search all dependencies of
//       the given module for the same symbol, only failing
//       if neither the module itself, nor any of its
//       dependencies carry it.
// HINT: Weak symbols will also be resolved during
//       the return trip from the dependency tree.
// @param: modid: A valid module id obtained from 'kmod_open' or 'kmod_fopen',
//                or one of the special 'KMODID_*' module ids.
//                'KMODID_NEXT' behaves similar to KMODID_ALL, but the
//                calling module is check last for the symbol, making this
//                id very useful when implementing some kind of modloader,
//                without wanting to do some actual error/dependency checks.
//             >> #include <mod.h>
//             >> 
//             >> __public int game_main(void) {
//             >>   return 42;
//             >> }
//             >> 
//             >> static int (*pgame_main)(void);
//             >> int main(void) {
//             >>   // Load some cool A$$ Mods!
//             >>   mod_open("/usr/lib/game/mods/killa_tweex.so",0);
//             >>   mod_open("/usr/lib/game/mods/epic_render.so",0);
//             >>   mod_open("/usr/lib/game/mods/gnarly_patches.so",0);
//             >>   *(void **)&pgame_main = kmod_sym(MOD_NEXT,"game_main",(size_t)-1);
//             >>   return (*pgame_main)();
//             >> }
//                The exact check order here depends on what module the caller is from:
//                A B C D E F  (Caller is module 'D')
//                3 4 5 6 1 2  (Most likely module lookup order)
//      REMINDER: The calling module is the one invoking the system call 'int $0x69'
// @param: name:  The name of a given symbol. Together with
//                'namemax', it describes an strnlen-style string.
// @return: NULL: Something went wrong (probably just doesn't export the given symbol)
__local _syscall3(void *,kmod_sym,kmodid_t,modid,
                  char const *,name,__size_t,namemax);
#endif /* !__NO_PROTOTYPES */

#ifndef _NO_PROTOTYPES

#ifndef __ksymflag_t_defined
#define __ksymflag_t_defined 1
typedef __u32 ksymflag_t;
#endif
#define KSYMINFO_FLAG_NONE    0x00000000
#define KSYMINFO_FLAG_NAME    0x00000100 /*< The symbol has a known name ('si_name'). */
#define KSYMINFO_FLAG_FILE    0x00000200 /*< The symbol has a known file name ('si_file'). */
#define KSYMINFO_FLAG_LINE    0x00000400 /*< The symbol has a known line number ('si_line'). */
#define KSYMINFO_FLAG_COL     0x00000800 /*< The symbol has a known column number ('si_col'). */
#define KSYMINFO_FLAG_BASE    0x00001000 /*< The symbol has a known base ('si_base'). */
#define KSYMINFO_FLAG_SIZE    0x00002000 /*< The symbol has a known size ('si_size'). */
#define KSYMINFO_FLAG_PUBLIC  0x00010000 /*< The symbol is exported as public. */
#define KSYMINFO_FLAG_PRIVATE 0x00020000 /*< The symbol is exported as private. */
#define KSYMINFO_FLAG_WEAK    0x00030000 /*< The symbol is exported as weak. */

#define KSYMINFO_TYPE_NONE    0x00000000
#define KSYMINFO_TYPE_FUNC    0x00000001 /*< Function symbol. */
#define KSYMINFO_TYPE_OBJ     0x00000002 /*< Object symbol. */

#define KSYMINFO_TYPE_MASK    0x000000ff
#define KSYMINFO_FLAG_MASK    0xffffff00
#define KSYMINFO_TYPE(flags) ((flags)&KSYMINFO_TYPE_MASK)
#define KSYMINFO_FLAG(flags) ((flags)&KSYMINFO_FLAG_MASK)

struct ksymname {
 char const *sn_name; /*< [1..sn_size] Symbol name. */
 __size_t    sn_size; /*< Strnlen-style max symbol name length. */
};

struct ksyminfo {
 ksymflag_t   si_flags; /*< Symbol flags & available information (One of 'KSYMINFO_TYPE_*' + set of 'KSYMINFO_FLAG_*'). */
 kmodid_t     si_modid; /*< Module id that this symbol belongs to. */
 void        *si_base;  /*< Base address of the symbol. */
 __size_t     si_size;  /*< Symbol size (ZERO(0) if the 'KSYMINFO_FLAG_SIZE' flag isn't set). */
 char        *si_name;  /*< [0..1] Symbol name (NULL if 'KSYMINFO_FLAG_NAME' isn't set). */
 __size_t     si_nmsz;  /*< Size of 'si_name' (WARNING: is bytes & including the terminating \0-character) */
 /* WARNING: File and line information usually isn't available for ELF files. */
 char        *si_file;  /*< [0..1] Symbol file name (NULL if 'KSYMINFO_FLAG_FILE' isn't set). */
 __size_t     si_flsz;  /*< Size of 'si_file' (WARNING: is bytes & including the terminating \0-character) */
 unsigned int si_line;  /*< Line number of the first address associated with the symbol, or when information
                            was requested given an address instead of a name, the line number of the given address.
                            HINT: Symbol line numbers originate at ZERO(0), whilst text editors usually start at ONE(1).
                            NOTE: When generating tracebacks, you will often see this point to the next line after a call,
                                  simply because you're looking up the return address, which _does_ point to the next line. */
 unsigned int si_col;   /*< Column number (May not be available even if a line number is). */
};


//////////////////////////////////////////////////////////////////////////
// Lookup a symbol by name or address within a given module,
// storing symbol information in the given buffer.
// HINT: This function can also be used to get extended
//       error information when a call to 'kmod_sym' with
//       the same arguments returned NULL.
// NOTE: When an address is provided, the nearest symbol with a lower
//       base address is searched for (as available through 'si_base').
// @param: addr_or_name: When 'KMOD_SYMINFO_FLAG_LOOKUPADDR' is set,
//                       re-interpret this argument as the 'void *'
//                       pointing to the symbol's address, otherwise
//                       a 'struct ksymname' structure is located
//                       here that contains the name of the symbol,
//                       as well as an strnlen-style max name length.
// @return: KE_OK: The supplied information buffer was successfully
//                 filled with the requested set of information.
// NOTE: The name and file strings behave the same way as described
//       in the documentation of 'KMOD_INFO_FLAG_NAME', allowing
//       you to either specify your own buffer, or left 'kmod_syminfo'
//       re-use the buffer you're providing through 'buf'.
__local _syscall7(kerrno_t,kmod_syminfo,int,procfd,kmodid_t,modid,
                  struct ksymname const *,addr_or_name,struct ksyminfo *,buf,
                  __size_t,bufsize,__size_t *,reqsize,__u32,flags);
#define KMOD_SYMINFO_FLAG_NONE       0x00000000
#define KMOD_SYMINFO_FLAG_LOOKUPADDR 0x00000001 /*< 'addr_or_name' is the address of the symbol instead of its name. */
#define KMOD_SYMINFO_FLAG_WANTNAME   KSYMINFO_FLAG_NAME /*< Fill 'si_name' and 'si_nmsz'. */
#define KMOD_SYMINFO_FLAG_WANTFILE   KSYMINFO_FLAG_FILE /*< Fill 'si_file' and 'si_flsz'. */
#define KMOD_SYMINFO_FLAG_WANTLINE   KSYMINFO_FLAG_LINE /*< Fill 'si_line' and 'si_col'. */

#endif /* !__NO_PROTOTYPES */


#ifndef __kmodkind_t_defined
#define __kmodkind_t_defined 1
typedef __u32 kmodkind_t;
#endif

/* Module kind flags & utilities. */
#define KMODFLAG_NONE       0x00000000
#define KMODFLAG_BINARY     0x00000100 /*< FLAG: Module is a binary (NOTE: Not set for scripts, such as shebang). */
#define KMODFLAG_FIXED      0x00000200 /*< FLAG: The module has no relocation information and must be loaded to a fixed address. */
#define KMODFLAG_PREFFIXED  0x00000400 /*< FLAG: The module prefers being loaded to a fixed address (base = NULL), but is capable of being relocated elsewhere. */
#define KMODFLAG_CANEXEC    0x00000800 /*< FLAG: The module can be executed (Aka. has an entry point). */
#define KMODFLAG_LOADED     0x80000000 /*< FLAG: Always set; Used internally to prevent dependecy loops (indicates that the library and all dependencies were loaded successfully). */
#define KMODKIND_KMASK      0x000000ff /*< Mask for kind (KMODKIND_*). */
#define KMODKIND_FMASK      0xffffff00 /*< Mask for flags (KMODFLAG_*). */
#define KMODKIND_KIND(x)  ((x)&KMODKIND_KMASK)
#define KMODKIND_FLAGS(x) ((x)&KMODKIND_FMASK)

/* Module technologies. (NOTE: All of these are implemented and working!) */
#define KMODKIND_UNKNOWN    0 /*< Unknown binary type (May safely be used for error-cases). */
#define KMODKIND_ELF32      1 /*< ELF-32 (.../.so) binary/shared library. */
#define KMODKIND_PE32       2 /*< PE-32 (.exe/.dll) binary/shared library. */
#define KMODKIND_SHEBANG 0x80 /*< Shebang ('#!'-style script file). (Used internally) */
#define KMODKIND_KERNEL  0xff /*< Exclusively used by the kernel itself. */
#define KMODKIND_ISLIB(x) (!((x)&0x80)) /*< The library can be loaded by 'mod_open'. - Does not affect 'exec()' */

#ifndef __kmodinfo_defined
#define __kmodinfo_defined 1
struct kmodinfo {
    /* The kind of module/binary.
     * - One of 'KMODKIND_*'.
     * - Potentially or-ed with addition flags.
     * - Use 'KMODKIND_KIND' to extract the raw switch-capable kind (aka. technology).
     * - Use 'KMODKIND_FLAGS' to extract flags.
     */
    kmodid_t   mi_id;         /*< The real module id. */
    kmodkind_t mi_kind;       /*< The kind of module/binary. */
    void      *mi_base;       /*< Base address to which the given module was loaded. */
    /* == Section-based address range ==
     * Given an instruction-pointer, one can determine what module
     * the given pointer belongs to through use of its section range:
     * >> if (eip >= module->mi_begin && eip < module->mi_end) found_module(module);
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

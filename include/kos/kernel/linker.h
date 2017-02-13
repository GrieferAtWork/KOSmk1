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
#ifndef __KOS_KERNEL_LINKER_H__
#define __KOS_KERNEL_LINKER_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <dlfcn.h>
#include <elf.h>
#include <kos/compiler.h>
#include <kos/types.h>
#include <kos/errno.h>
#include <kos/kernel/features.h>
#include <kos/kernel/object.h>
#include <kos/kernel/paging.h>
#include <kos/kernel/types.h>

__DECL_BEGIN

struct kfile;

#define KOBJECT_MAGIC_SYMTABLE   0x5F47AB7E /*< SYMTABLE. */
#define KOBJECT_MAGIC_SHLIB      0x5471B    /*< SHLIB. */
#define KOBJECT_MAGIC_EXECUTABLE 0xE7CE     /*< EXE. */

#define kassert_ksymtable(self)   kassert_object(self,KOBJECT_MAGIC_SYMTABLE)
#define kassert_kshlib(self)      kassert_refobject(self,sh_refcnt,KOBJECT_MAGIC_SHLIB)
#define kassert_kreloc(self)      kassertobj(self)
#define kassert_kexecutable(self) kassert_object(self,KOBJECT_MAGIC_EXECUTABLE)

#define KSYM_INVALID ((ksymaddr_t)-1)
#ifndef __ksymaddr_t_defined
#define __ksymaddr_t_defined 1
typedef __uintptr_t    ksymaddr_t;
#endif
#ifndef __ksymhash_t_defined
#define __ksymhash_t_defined 1
typedef __size_t       ksymhash_t;
#endif

#ifndef __ksymhash_of_defined
#define __ksymhash_of_defined 1
//////////////////////////////////////////////////////////////////////////
// @return: The hash used to represent the given text in a symbol table.
extern __wunused __nonnull((1)) ksymhash_t
ksymhash_of(char const *__restrict text, __size_t size);
#endif

struct ksymbol {
 struct ksymbol *s_nextname; /*< [0..1][owned] Next symbol who's name has the same hash. */
 struct ksymbol *s_nextaddr; /*< [0..1][owned] Next symbol who's address has the same hash. */
 ksymaddr_t      s_addr;     /*< Address associated with this symbol. */
 __size_t        s_size;     /*< Size of this symbol (Or ZERO(0) if not known). */
 ksymhash_t      s_hash;     /*< Unmodulated hash of this symbol. */
 __size_t        s_shndx;    /*< Section index of this symbol (== Elf32_Sym::st_shndx). */
 __size_t        s_nmsz;     /*< Length of this symbol's name. */
 char            s_name[1];  /*< [s_nmsz] Name of the symbol (inlined to improve speed). */
 //char          s_zero;     /*< Ensure zero-termination of 's_name'. */
};

struct ksymlist {
 __size_t         sl_symc; /*< Amount of symbols. */
 struct ksymbol **sl_symv; /*< [0..1][0..sl_symc][owned] Vector of symbols. */
};
#define KSYMLIST_INIT      {0,NULL}
#define ksymlist_init(self) memset(self,0,sizeof(struct ksymlist))
#define ksymlist_quit(self) free((self)->sl_symv)




struct ksymtablebucket {
 struct ksymbol *b_lname; /*< [0..1][owned] Linked list of symbols who's name shares this hash. */
 struct ksymbol *b_laddr; /*< [0..1] Linked list of addresses who share this hash. */
};

struct ksymtable {
 // Symbol table (resolve name -> address)
 KOBJECT_HEAD
 __size_t                st_size;  /*< Amount of existing symbol. */
 __size_t                st_mapa;  /*< Allocated symbol table size (Also used as modulator for hashes). */
 struct ksymtablebucket *st_mapv;  /*< [0..st_mapa][owned] Hash-map of symbol table entries. */
};
#define KSYMTABLE_INIT  {KOBJECT_INIT(KOBJECT_MAGIC_SYMTABLE) 0,0,NULL}

#define ksymtable_modnamehash2(hash,bucketcount) ((hash) % (bucketcount))
#define ksymtable_modnamehash(self,hash) ksymtable_modnamehash2(hash,(self)->st_mapa)

#define ksymtable_modaddrhash2(addr,bucketcount) ((__uintptr_t)(addr) % (bucketcount))
#define ksymtable_modaddrhash(self,addr) ksymtable_modaddrhash2(addr,(self)->st_mapa)

//////////////////////////////////////////////////////////////////////////
// Initialize/Finalize a given symbol table.
extern __nonnull((1)) void ksymtable_quit(struct ksymtable *self);
#ifdef __INTELLISENSE__
extern __nonnull((1)) void ksymtable_init(struct ksymtable *self);

//////////////////////////////////////////////////////////////////////////
// Clear all symbols from a given symbol table
extern __nonnull((1)) void ksymtable_clear(struct ksymtable *self);
#else
#define ksymtable_init(self) kobject_initzero(self,KOBJECT_MAGIC_SYMTABLE)
#define ksymtable_clear(self) \
 __xblock({ struct ksymtable *const __kstcself = (self);\
            ksymtable_quit(__kstcself);\
            ksymtable_init(__kstcself);\
            (void)0;\
 })

#endif


//////////////////////////////////////////////////////////////////////////
// Lookup a given symbol by its name, length and hash
// @return: NULL: No symbol matching the given criteria was found
extern __wunused __nonnull((1,2)) struct ksymbol const *
ksymtable_lookupname_h(struct ksymtable const *self, char const *symname,
                       __size_t symnamelength, ksymhash_t hash);
__local __wunused __nonnull((1,2)) struct ksymbol const *
ksymtable_lookupname(struct ksymtable const *self, char const *symname,
                     __size_t symnamelength);

extern __wunused __nonnull((1)) struct ksymbol const *
ksymtable_lookupaddr_single(struct ksymtable const *self, ksymaddr_t addr);

//////////////////////////////////////////////////////////////////////////
// Lookup the entry closest to the given address, decrementing the
// given address until ZERO(0) has been reached, or a entry was found.
extern __wunused __nonnull((1)) struct ksymbol const *
ksymtable_lookupaddr(struct ksymtable const *self, ksymaddr_t addr);

//////////////////////////////////////////////////////////////////////////
// Rehash the given symbol table to use 'bucketcount' buckets.
// NOTE: 'bucketcount' must never be zero when passed to this function.
// @return: KE_OK:    The table was rehashed to use 'bucketcount' buckets.
// @return: KE_NOMEM: Rehashing failed due to not enough memory being available.
extern __nonnull((1)) kerrno_t
ksymtable_rehash(struct ksymtable *self, __size_t bucketcount);




//////////////////////////////////////////////////////////////////////////
// Shared library relocations
struct krelocvec {
#define KRELOCVEC_TYPE_ELF32_REL  0
#define KRELOCVEC_TYPE_ELF32_RELA 1
#define KRELOCVEC_TYPE_ELF32_PLT  2
 __u32        rv_type; /*< Type of relocation vector (one of 'KRELOCVEC_TYPE_*') */
 __size_t     rv_relc; /*< Amount of relocation entries. */
 union{
  void       *rv_data; /*< [0..rv_relc*sizeof(?)][owned] Vector of relocation commands. */
  Elf32_Rel  *rv_elf32_relv;  /*< [0..rv_relc] ELF-32 relocations. */
  Elf32_Rela *rv_elf32_relav; /*< [0..rv_relc] ELF-32 relocations (w/ addend). */
 };
};
struct kreloc {
 __size_t          r_vecc; /*< Amount of relocation vectors. */
 struct krelocvec *r_vecv; /*< [0..r_vecc][owned] Vector of known relocations. */
 struct ksymlist   r_syms; /*< Vector of dynamic symbols. */
};
#ifdef __INTELLISENSE__
extern __nonnull((1)) void kreloc_init(struct kreloc *__restrict self);
#else
#define kreloc_init(self) \
 memset(self,0,sizeof(struct kreloc))
#endif
extern __nonnull((1)) void kreloc_quit(struct kreloc *__restrict self);

struct kshm;
struct kprocmodule;

//////////////////////////////////////////////////////////////////////////
// Execute relocation within the given 'memspace',
// using 'base' as the new base address.
extern __nonnull((1)) void
kreloc_exec(struct kreloc *__restrict self,
            struct kshm *__restrict memspace,
            struct kproc *__restrict proc,
            struct kprocmodule *__restrict reloc_module,
            struct kprocmodule *__restrict start_module);


struct kshmtab;
struct kshlibsection {
 // Loaded data of a shared library
 __ref struct kshmtab    *sls_tab;       /*< [1..1] Shared memory tab. */
 __pagealigned ksymaddr_t sls_albase;    /*< Starting symbol address (page-aligned). */
 ksymaddr_t               sls_base;      /*< Starting symbol address. */
 __size_t                 sls_size;      /*< Exact size of the memory tab in bytes. */
 ksymaddr_t               sls_filebase;  /*< Starting symbol address (in the file). */
 __size_t                 sls_filesize;  /*< Size of the section (in the file). */
};

struct ksecdata {
 // Loaded data of a shared library
 __size_t              ed_secc; /*< Amount of sections. */
 struct kshlibsection *ed_secv; /*< [0..ed_secc][owned] Vector of sections. */
};

//////////////////////////////////////////////////////////////////////////
// Load shared library data from a given file.
// @param: pheaderv: Vector of program headers. (Elf32_Ehdr::e_phoff)
// @param: pheaderc: Amount of program headers. (Elf32_Ehdr::e_phnum)
// @param: pheadersize: Size of a single header. (Elf32_Ehdr::e_phentsize)
extern __wunused __nonnull((1,2,5)) kerrno_t
ksecdata_init(struct ksecdata *__restrict self,
              Elf32_Phdr const *__restrict pheaderv,
              __size_t pheaderc, __size_t pheadersize,
              struct kfile *__restrict elf_file);
extern __nonnull((1)) void ksecdata_quit(struct ksecdata *__restrict self);

//////////////////////////////////////////////////////////////////////////
// Translate a given address into its physical counterpart.
// @param: maxsize: Filled with the max amount of consecutively valid bytes when
//                  translating the address into its physical counterpart.
//                  NOTE: Once '*maxsize' bytes were read/written, 'addr' must
//                        be adjusted accordingly ('addr += *maxsize') before
//                        performing another translation.
// @return: NULL: The given address is invalid
//               [ksecdata_translate_rw] The associated tab is marked as read-only.
extern __kernel __wunused __nonnull((1,3)) void const *
ksecdata_translate_ro(struct ksecdata const *__restrict self,
                      ksymaddr_t addr, __size_t *__restrict maxsize);
extern __kernel __wunused __nonnull((1,3)) void *
ksecdata_translate_rw(struct ksecdata *__restrict self,
                      ksymaddr_t addr, __size_t *__restrict maxsize);

//////////////////////////////////////////////////////////////////////////
// Reads/Writes up to 'bufsize' bytes from/to 'addr' into/from 'buf'
// @return: * : Amount of actually read/written bytes.
extern __wunused __nonnull((1,3)) __size_t
ksecdata_getmem(struct ksecdata const *__restrict self, ksymaddr_t addr,
                void *__restrict buf, __size_t bufsize);
extern __nonnull((1,3)) __size_t
ksecdata_setmem(struct ksecdata *__restrict self, ksymaddr_t addr,
                void const *__restrict buf, __size_t bufsize);


struct kshlib_callbacks {
 // NOTE: Callbacks not specified are marked with 'KSYM_INVALID'
 ksymaddr_t slc_start;           /*< void(*)(void):  Address of the '_start'-function (NOTE: Shared library usually don't have this...). */
 ksymaddr_t slc_preinit_array_v; /*< void(**)(void): Address of an array of pre-initialization functions. */
 __size_t   slc_preinit_array_c; /*< Max amount of functions in 'slc_init_array_v' (terminate on first NULL entry). */
 ksymaddr_t slc_init;            /*< void(*)(void):  Address of an initialization function. */
 ksymaddr_t slc_init_array_v;    /*< void(**)(void): Address of an array of initialization functions. */
 __size_t   slc_init_array_c;    /*< Max amount of functions in 'slc_init_array_v' (terminate on first NULL entry). */
 ksymaddr_t slc_fini;            /*< void(*)(void):  Address of a finalization function. */
 ksymaddr_t slc_fini_array_v;    /*< void(**)(void): Address of an array of finalization functions. */
 __size_t   slc_fini_array_c;    /*< Max amount of functions in 'slc_fini_array_v' (terminate on first NULL entry). */
};

struct kshliblist {
 __size_t              sl_libc; /*< Amount of shared libraries. */
 __ref struct kshlib **sl_libv; /*< [1..1][0..sl_libc][owned] Vector of shared libraries. */
};
#define KSHLIBLIST_INIT  {0,NULL}

#ifdef __INTELLISENSE__
extern __nonnull((1)) void kshliblist_init(struct kshliblist *__restrict self);
#else
#define kshliblist_init(self) \
 memset(self,0,sizeof(struct kshliblist))
#endif
extern __nonnull((1)) void kshliblist_quit(struct kshliblist *__restrict self);


struct kshlib {
 // Shared library/cached executable controller object
 // NOTE: At this point, shared libs and exe-s are the same thing
 // NOTE: Shared libraries are implicitly synchronized, as all members
 //       must be considered constant post initialization.
 KOBJECT_HEAD
 __atomic __u32           sh_refcnt;    /*< Reference counter. */
 __ref struct kfile      *sh_file;      /*< [1..1] Open file to this shared library (Used to retrieve library name/path). */
#define KSHLIB_FLAG_NONE   0x00000000
#define KSHLIB_FLAG_FIXED  0x00000001   /*< The shared library must be loaded at a fixed address (base = NULL). */
#define KSHLIB_FLAG_LOADED 0x00000002   /*< Set when the library is fully loaded (used to detect cyclic dependencies). */
 __u32                   sh_flags;      /*< Shared library flags. */
 __size_t                sh_cidx;       /*< [lock(kslcache_lock)] Cache index (set to (size_t)-1 when not cached). */
 struct ksymtable        sh_publicsym;  /*< Hash-table of public symbols (exported by this shared library). */
 struct ksymtable        sh_weaksym;    /*< Hash-table of weak symbols (exported by this shared library). */
 struct ksymtable        sh_privatesym; /*< Hash-table of private symbols (for addr2line, etc.). */
 struct kshliblist       sh_deps;       /*< Library dependencies. */
 struct ksecdata         sh_data;       /*< Section data. */
 struct kreloc           sh_reloc;      /*< Relocation information. */
 struct kshlib_callbacks sh_callbacks;  /*< Special callbacks recognized by the linker. */
};

__local KOBJECT_DEFINE_INCREF(kshlib_incref,struct kshlib,sh_refcnt,kassert_kshlib);
__local KOBJECT_DEFINE_DECREF(kshlib_decref,struct kshlib,sh_refcnt,kassert_kshlib,kshlib_destroy);

//#define KSHLIB_INIT  {KOBJECT_INIT(KOBJECT_MAGIC_SHLIB) KSYMTABLE_INIT}

//////////////////////////////////////////////////////////////////////////
// Initialize a shared library/executable from a given file.
// @return: KE_NOMEM:    Insufficient memory.
// @return: KE_NOEXEC:   Not a valid ELF executable.
// @return: KE_LOOP:     The library and its dependencies are creating an unresolvable loop
// @return: KE_NODEP:    At least one dependencies required by the module could not be found.
// @return: KE_OVERFLOW: A reference counter would have overflown (the library is loaded too often)
// @return: KE_ISERR(*): Some other, possibly file-specific error has occurred
extern __crit __wunused __nonnull((1,2)) kerrno_t
kshlib_new(struct kshlib **__restrict result,
           struct kfile *__restrict elf_file);
extern __crit __wunused __nonnull((1,2)) kerrno_t
kshlib_new_elf32(struct kshlib **__restrict result,
                 struct kfile *__restrict elf_file);

//////////////////////////////////////////////////////////////////////////
// Returns the symbol address of a given file address.
// NOTE: Returns 'KSYM_INVALID' for invalid addresses, including
//       those not pointing into sections loaded with DT_LOAD.
extern __wunused __nonnull((1)) ksymaddr_t
kshlib_fileaddr2symaddr(struct kshlib const *__restrict self,
                        ksymaddr_t fileaddr);

//////////////////////////////////////////////////////////////////////////
// Returns the file address of a given symbol address.
// NOTE: Returns ZERO(0) for invalid addresses, including
//       those not pointing into sections loaded with DT_LOAD.
// HINT: Useful for translating pointers from ELF's 'd_ptr' field.
extern __wunused __nonnull((1)) ksymaddr_t
kshlib_symaddr2fileaddr(struct kshlib const *__restrict self,
                        ksymaddr_t symaddr);

#ifndef __INTELLISENSE__
__local struct ksymbol const *
ksymtable_lookupname(struct ksymtable const *__restrict self,
                     char const *__restrict symname,
                     __size_t symnamelength) {
 return ksymtable_lookupname_h(self,symname,symnamelength,
                               ksymhash_of(symname,symnamelength));
}
#endif


#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Returns a shared library object that describes the kernel.
// NOTE: This library is kind-of special, as its instance is a singleton,
//       meaning that loading it as a dependency doesn't spawn a new instance.
extern __wunused __constcall __retnonnull struct kshlib *kshlib_kernel(void);
#else
extern struct kshlib __kshlib_kernel;
#define kshlib_kernel() (&__kshlib_kernel)
#endif


struct kproc;
struct kshm;

//////////////////////////////////////////////////////////////////////////
// Spawn the root task of a given shared library (in a suspended state)
// @return: NULL: Not enough memory to spawn a new task.
extern __crit __wunused __nonnull((1,2)) __ref struct ktask *
kshlib_spawn(struct kshlib const *__restrict self,
             struct kproc *__restrict proc);


struct kfspathenv;
//////////////////////////////////////////////////////////////////////////
// Opens a shared library, given its (file)name
//  - 'kshlib_openlib' will walk the $LD_LIBRARY_PATH
//    environment variable of the calling process.
//  - If the calling process is running in windows compatibility mode,
//    additional paths, such as $PATH, getcwd() and the directory of
//    its executable are searched as well.
// @return: KE_NOMEM:  Not enough available memory.
// @return: KE_NOENT:  The given library wasn't found.
// @return: KE_NOEXEC: The found file isn't an executable.
// @return: KE_ACCES:  The caller does not have access to the only matching file/dependency.
extern __crit __wunused __nonnull((1,3)) kerrno_t
kshlib_openlib(char const *__restrict name, __size_t namemax,
               __ref struct kshlib **__restrict result);

/* Open a user-level executable as searched for by exec().
 * Looks at the $PATH environment variable of the calling process,
 * but also accepts absolute paths. */
extern __crit __wunused __nonnull((1,3)) kerrno_t
kshlib_openexe(char const *__restrict name, __size_t namemax,
               __ref struct kshlib **__restrict result, int allow_path_search);


extern __crit __wunused __nonnull((1,2,4,6)) kerrno_t
kshlib_opensearch(struct kfspathenv const *pathenv,
                  char const *__restrict name, __size_t namemax,
                  char const *__restrict search_paths, __size_t search_paths_max,
                  __ref struct kshlib **__restrict result,
                  int require_exec_permissions);
extern __crit __wunused __nonnull((1,2,4)) kerrno_t
kshlib_openfileenv(struct kfspathenv const *pathenv,
                   char const *__restrict filename, __size_t filename_max,
                   __ref struct kshlib **__restrict result,
                   int require_exec_permissions);
extern __crit __wunused __nonnull((1,2)) kerrno_t
kshlib_fopenfile(struct kfile *fp, __ref struct kshlib **__restrict result);

// TODO: These compiler-settings should be turned
//       into process-local debug flags.
#define KSHLIB_IGNORE_MISSING_DEPENDENCIES 1


//////////////////////////////////////////////////////////////////////////
// Add/Delete a given shared library to/from the global shlib cache.
// NOTE: The global shlib cache is required to correctly detect
//       same-dependency-style dependency trees. e.g.:
//       /bin/init -+--> libproc --> libc
//                  |                 |
//                  +--> libc <-------+
//       >> 'libc' is only loaded once
// NOTE: It is also used to speed up loading of shared libraries
//       in situations where a process attempts to load a shared
//       library already loaded by another process.
// NOTE: 'kshlibcache_addlib' does not expect the library to be fully
//       initialized. The fields that must be initialized are:
//       'sh_refcnt', 'sh_file' and 'sh_flags' (as well as '__magic' in debug mode)
// NOTE: The caller must ensure that any library is only added once
//      (That is an individual instance of a shared library; aka. the lib-pointer)
// @return: KE_OK:    The given library has successfully been cached
// @return: KE_NOMEM: Not enough memory to cache the given library
extern __crit __wunused kerrno_t kshlibcache_addlib(struct kshlib *__restrict lib);
extern __crit void kshlibcache_dellib(struct kshlib *__restrict lib);

//////////////////////////////////////////////////////////////////////////
// Returns a cached shared library, given its absolute path.
// WARNING: If this function returns NULL, that doesn't mean the
//          given path does not contain what you are looking for.
//          It merely means that the library isn't already cached!
// WARNING: The given path must be absolute in relation to the
//          true filesystem root to function properly. If it doesn't
//          start with a slash, the function always returns NULL.
// WARNING: If a cached library was found, the caller is responsible
//          for ensuring that it has been fully loaded (aka.
//          the 'KSHLIB_FLAG_LOADED' flag has been set).
//          This must be done to detect and handle cyclic dependencies.
// @return: NULL: No library with the given path was cached.
// @return: * :   A new reference to a cached shared library.
extern __crit __ref struct kshlib *kshlibcache_getlib(char const *__restrict absolute_path);
extern __crit __ref struct kshlib *kshlibcache_fgetlib(struct kfile *fp);



#if KSHLIB_RECENT_CACHE_SIZE
//////////////////////////////////////////////////////////////////////////
// Special shlib cache for recently loaded libraries
// >> This cache is used to keep libraries loaded for a
//    while even when seemingly no one is using them.
extern __crit void kshlibrecent_add(struct kshlib *lib);
extern __crit void kshlibrecent_clear(void);
#else
#define kshlibrecent_add(lib) (void)0
#define kshlibrecent_clear()  (void)0
#endif


__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_LINKER_H__ */

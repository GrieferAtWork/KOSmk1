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
#include <kos/mod.h>
#include <kos/errno.h>
#include <kos/kernel/features.h>
#include <kos/kernel/object.h>
#include <kos/kernel/paging.h>
#include <kos/kernel/types.h>

__DECL_BEGIN

struct kfile;
struct kshlib;

#define KOBJECT_MAGIC_SYMTABLE   0x5F47AB7E /*< SYMTABLE. */
#define KOBJECT_MAGIC_SHLIB      0x5471B    /*< SHLIB. */

#define kassert_ksymtable(self)   kassert_object(self,KOBJECT_MAGIC_SYMTABLE)
#define kassert_kshlib(self)      kassert_refobject(self,sh_refcnt,KOBJECT_MAGIC_SHLIB)
#define kassert_kreloc(self)      kassertobj(self)

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
 struct ksymbol    *s_nextname; /*< [0..1][owned] Next symbol who's name has the same hash. */
 struct ksymbol    *s_nextaddr; /*< [0..1][owned] Next symbol who's address has the same hash. */
 ksymaddr_t         s_addr;     /*< Address associated with this symbol. */
 __size_t           s_size;     /*< Size of this symbol (Or ZERO(0) if not known). */
 ksymhash_t         s_hash;     /*< Unmodulated hash of this symbol. */
 __size_t           s_shndx;    /*< Section index of this symbol (== Elf32_Sym::st_shndx).
                                    NOTE: In PE-mode, this field is set to 'SHN_UNDEF' for imported symbols, and
                                          some other value for those either exported, or simply not imported. */
 __size_t           s_nmsz;     /*< Length of this symbol's name. */
 char               s_name[1];  /*< [s_nmsz] Name of the symbol (inlined to improve speed). */
 //char             s_zero;     /*< Ensure zero-termination of 's_name'. */
};

extern __crit __wunused __malloccall __nonnull((1))
struct ksymbol *ksymbol_new(char const *__restrict name);
#define ksymbol_delete   free


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
 /* Symbol table (resolve name -> address) */
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
ksymtable_lookupaddr(struct ksymtable const *self,
                     ksymaddr_t addr, ksymaddr_t addr_min);

//////////////////////////////////////////////////////////////////////////
// Rehash the given symbol table to use 'bucketcount' buckets.
// NOTE: 'bucketcount' must never be zero when passed to this function.
// @return: KE_OK:    The table was rehashed to use 'bucketcount' buckets.
// @return: KE_NOMEM: Rehashing failed due to not enough memory being available.
extern __nonnull((1)) kerrno_t
ksymtable_rehash(struct ksymtable *__restrict self,
                 __size_t bucketcount);

//////////////////////////////////////////////////////////////////////////
// Insert a given symbol and its address into the symbol table.
// NOTE: The Symbol table will always try to keep at least as many
//       buckets as it has symbols. aka:
//       >> if (st_mapa < st_size+1) ksymtable_rehash(st_size+1);
//       So to speed up this function, call 'ksymtable_rehash' yourself
//       beforehand to reduce processing time otherwise required to
//       constantly reallocate the symbol table.
//      (With try I mean that a failure to rehash is silently ignored)
// NOTE: The caller is responsible to prevent KSYM_INVALID addresses from being inserted.
// @return: KE_OK:        The given symbol was successfully inserted.
// @return: KE_NOMEM:     Not enough available memory (can still
//                        happen if the entry cannot be allocated)
extern __wunused __nonnull((1,2)) kerrno_t
ksymtable_insert_inherited(struct ksymtable *__restrict self,
                           struct ksymbol *__restrict sym);



//////////////////////////////////////////////////////////////////////////
// Shared library relocations
struct kreloc_peimport {
 /* PE-style import table relocation. */
 struct ksymbol **rpi_symv;  /*< [1..1][0..:rv_relc][owned] Vector of symbols affected (Same order as that in 'rpi_thunk') */
 struct kshlib   *rpi_hint;  /*< [0..1] Hint pointer describing the first library to scan for imports. */
 ksymaddr_t       rpi_thunk; /*< Symbol address of 'FirstThunk' (Array of pointers to imported functions). */
};
struct kreloc_pereloc32 {
 __u16           *rpr_offv;  /*< [0..:rv_relc][owned] Vector of symbol type & offset (this is IMAGE_BASE_RELOCATION::TypeOffset) */
 ksymaddr_t       rpr_base;  /*< Section base (Added to offsets to find ksymaddr_t for '__u32 *' to which a delta is added) */
};


struct krelocvec {
#define KRELOCVEC_TYPE_ELF32_REL  0
#define KRELOCVEC_TYPE_ELF32_RELA 1
#define KRELOCVEC_TYPE_PE_IMPORT  2
#define KRELOCVEC_TYPE_PE32_RELOC 3
 __u32        rv_type; /*< Type of relocation vector (one of 'KRELOCVEC_TYPE_*') */
 __size_t     rv_relc; /*< Amount of relocation entries. */
 union{
  void       *rv_data; /*< [0..rv_relc*sizeof(?)][owned] Vector of relocation commands. */
  Elf32_Rel  *rv_elf32_relv;  /*< [0..rv_relc] ELF-32 relocations. */
  Elf32_Rela *rv_elf32_relav; /*< [0..rv_relc] ELF-32 relocations (w/ addend). */
  struct kreloc_peimport rv_pe; /*< PE import relocations. */
  struct kreloc_pereloc32 rv_pe32_reloc; /*< 32-bit PE relocations. */
 };
};
struct kreloc {
 __size_t          r_vecc;     /*< Amount of relocation vectors. */
 struct krelocvec *r_vecv;     /*< [0..r_vecc][owned] Vector of known relocations. */
 struct ksymlist   r_elf_syms; /*< Vector of dynamic symbols (Used by ELF). */
};
#ifdef __INTELLISENSE__
extern __nonnull((1)) void kreloc_init(struct kreloc *__restrict self);
#else
#define kreloc_init(self) \
 memset(self,0,sizeof(struct kreloc))
#endif
extern __crit __nonnull((1)) void kreloc_quit(struct kreloc *__restrict self);

//////////////////////////////////////////////////////////////////////////
// Allocate and return a new entry within the relocation vector.
// @return: * :   A pointer to an uninitialized relocation entry, to-be filled by the caller.
// @return: NULL: Not enough memory.
extern __crit __wunused __malloccall __nonnull((1))
struct krelocvec *kreloc_alloc(struct kreloc *__restrict self);


struct kshm;
struct kproc;
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


struct kshmregion;
struct kshlibsection {
 /* Loaded data of a shared library (one section) */
 __ref struct kshmregion *sls_region;    /*< [1..1] Shared memory region. */
 __pagealigned ksymaddr_t sls_albase;    /*< Starting symbol address (page-aligned). */
 ksymaddr_t               sls_base;      /*< Starting symbol address. */
 __size_t                 sls_size;      /*< Exact size of the memory tab in bytes. */
 ksymaddr_t               sls_filebase;  /*< Starting symbol address (in the file). */
 __size_t                 sls_filesize;  /*< Size of the section (in the file). */
};

struct ksecdata {
 /* Loaded data of a shared library (many sections) */
 __size_t              ed_secc;  /*< Amount of sections. */
 struct kshlibsection *ed_secv;  /*< [0..ed_secc][owned] Vector of sections. */
 ksymaddr_t            ed_begin; /*< Lowest address of any valid section (page-aligned), or 'KSYM_INVALID' if 'ed_secc == 0'. */
 ksymaddr_t            ed_end;   /*< Address after the greatest of any valid section, or '0' if 'ed_secc == 0'. */
};

extern __nonnull((1)) void ksecdata_quit(struct ksecdata *__restrict self);

//////////////////////////////////////////////////////////////////////////
// Calculate the 'ed_begin'/'ed_end' cache.
extern void ksecdata_cacheminmax(struct ksecdata *__restrict self);

//////////////////////////////////////////////////////////////////////////
// Translate a given address into its physical counterpart.
// @param: maxsize: Filled with the max amount of consecutively valid bytes when
//                  translating the address into its physical counterpart.
//                  NOTE: Once '*maxsize' bytes were read/written, 'addr' must
//                        be adjusted accordingly ('addr += *maxsize') before
//                        performing another translation.
// @return: NULL: The given address is invalid
//               [ksecdata_translate_rw] The associated tab is marked as read-only.
extern __wunused __nonnull((1,3)) __kernel void const *
ksecdata_translate_ro(struct ksecdata const *__restrict self,
                      ksymaddr_t addr, __size_t *__restrict maxsize);
extern __wunused __nonnull((1,3)) __kernel void *
ksecdata_translate_rw(struct ksecdata *__restrict self,
                      ksymaddr_t addr, __size_t *__restrict maxsize);

//////////////////////////////////////////////////////////////////////////
// Returns TRUE(1) if the given addr is mapped. FALSE(0) otherwise.
extern __wunused __nonnull((1)) int 
ksecdata_ismapped(struct ksecdata const *__restrict self, ksymaddr_t addr);

//////////////////////////////////////////////////////////////////////////
// Reads/Writes up to 'bufsize' bytes from/to 'addr' into/from 'buf'
// @return: 0 : Successfully transferred all bytes.
// @return: * : Amount of bytes not transferred.
extern __wunused __nonnull((1,3)) __size_t
ksecdata_getmem(struct ksecdata const *__restrict self, ksymaddr_t addr,
                void *__restrict buf, __size_t bufsize);
extern __nonnull((1,3)) __size_t
ksecdata_setmem(struct ksecdata *__restrict self, ksymaddr_t addr,
                void const *__restrict buf, __size_t bufsize);



//////////////////////////////////////////////////////////////////////////
// Addr2line support
struct kfileandline {
 char const  *fal_path;   /*< [0..1] Path name. */
 char const  *fal_file;   /*< [0..1] File name. */
 unsigned int fal_line;   /*< Line number. */
 unsigned int fal_column; /*< Column number. */
 __u32        fal_flags;  /*< flags (Set of 'KFILEANDLINE_FLAG_*'). */
#define KFILEANDLINE_FLAG_NONE     0x00000000
#define KFILEANDLINE_FLAG_FREEPATH 0x00000001 /*< Caller must free() 'fal_path'. */
#define KFILEANDLINE_FLAG_FREEFILE 0x00000002 /*< Caller must free() 'fal_file'. */
#define KFILEANDLINE_FLAG_HASLINE  0x00000010 /*< 'fal_line' is filled with a sensible value. */
#define KFILEANDLINE_FLAG_HASCOL   0x00000020 /*< 'fal_column' is filled with a sensible value. */
};
#define kfileandline_quit(self) \
 (((self)->fal_flags&KFILEANDLINE_FLAG_FREEPATH) ? free((char *)(self)->fal_path) : (void)0,\
  ((self)->fal_flags&KFILEANDLINE_FLAG_FREEFILE) ? free((char *)(self)->fal_file) : (void)0)

struct ka2ldwarf;

#define KA2L_KIND_NONE   0
#define KA2L_KIND_DWARF  1
struct kaddr2line {
 __u32 a2l_kind; /*< The kind of addr2line technology used. */
 union{
  struct ka2ldwarf *a2l_dwarf; /*< [1..1][owned] Dwarf address2line engine. */
 };
};
extern __crit __nonnull((1)) void kaddr2line_quit(struct kaddr2line *__restrict self);

//////////////////////////////////////////////////////////////////////////
// Invoke an addr2line technology, as described by 'lib'.
// Upon success, '*result' is filled with a
// valid instance of an 'kfileandline' structure.
// @return: KE_NOENT: No information available for the given address.
extern __crit __wunused __nonnull((1,2,4)) kerrno_t
kaddr2line_exec(struct kaddr2line const *__restrict self,
                struct kshlib *__restrict lib, ksymaddr_t addr,
                struct kfileandline *__restrict result);


struct kaddr2linelist {
 __size_t           a2ll_techc; /*< Amount of available technologies. */
 struct kaddr2line *a2ll_techv; /*< [0..a2ll_techc][owned] Vector of technologies. */
};
#define kaddr2linelist_init(self) \
 (void)((self)->a2ll_techc = 0,(self)->a2ll_techv = NULL)
extern __crit void kaddr2linelist_quit(struct kaddr2linelist *__restrict self);


//////////////////////////////////////////////////////////////////////////
// Allocate an return a new addr2line technology entry.
// @return: NULL: Not enough available memory.
extern __crit __wunused __nonnull((1)) struct kaddr2line *
kaddr2linelist_alloc(struct kaddr2linelist *__restrict self);


//////////////////////////////////////////////////////////////////////////
// Invoke all available addr2line technologies, returning the
// error of the first one that didn't return KE_NOENT.
// @return: KE_NOENT: No information available for the given address.
extern __crit __wunused __nonnull((1,2,4)) kerrno_t
kaddr2linelist_exec(struct kaddr2linelist const *__restrict self,
                    struct kshlib *__restrict lib, ksymaddr_t addr,
                    struct kfileandline *__restrict result);




struct kshlib_callbacks {
 /* NOTE: Callbacks not specified are marked with 'KSYM_INVALID' */
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
extern __crit __nonnull((1)) void kshliblist_quit(struct kshliblist *__restrict self);

//////////////////////////////////////////////////////////////////////////
// Append a given library to this list.
// @return: KE_OK:    Successfully inherited a reference and appended the given lib.
// @return: KE_NOMEM: Not enough available memory (No reference is inherited)
extern __crit __nonnull((1,2)) kerrno_t
kshliblist_append_inherited(struct kshliblist *__restrict self,
                            __ref struct kshlib *__restrict lib);



#define K_SHEBANG_MAXCMDLINE 4096 /*< Max amount of bytes allowed for use as a commandline. */

struct ksbargs {
 /* Shebang additional arguments.
  * KOS supports multiple arguments as opposed to the standardized single-argument,
  * supporting the use of either "" or '' to group arguments together.
  * Following standard conventions, the absolute path of the script file
  * (if visible to the caller, following 'KATTR_FS_PATHNAME' rules) is appended
  * as its own argument, followed by any remaining arguments.
  * Note, that Shebang scripts may _NOT_ be loaded as shared libraries. */
 __size_t sb_argc; /*< Amount of optional arguments (including interpreter name). */
 char   **sb_argv; /*< [1..1][owned][0..sb_argc][owned] Vector of optional arguments. */
};

//////////////////////////////////////////////////////////////////////////
// Initialize the given shebang optional arguments.
// NOTES:
//  - This function is capable of parsing '' and "".
//    With the ability of using both, you are able to
//    include either quotes in the resulting commandline:
//    >> #!/bin/echo "You've got to be kidding me"
//    >> #!/bin/echo 'And then he said "This is SPARTA!"'
//  - The passed text should describe everything after
//    '#!' and before the associated linefeed/EOF.
//  - Unresolved (missing) quotation marks are silently
//    ignored and implicitly terminated at the end.
// @return: KE_OK:    Successfully initialized Shebang arguments.
// @return: KE_NOMEM: Not enough available memory.
// @return: KE_INVAL: An invalid (non-printable ctype.h:isprint)
//                    character was encountered within the given text.
//              NOTE: This does not affect space (ctype.h:isspace) characters.
extern __wunused kerrno_t
ksbargs_init(struct ksbargs *__restrict self,
             char const *__restrict text, size_t textsize);
extern void ksbargs_quit(struct ksbargs *__restrict self);

//////////////////////////////////////////////////////////////////////////
// Prepend all arguments from the given argc|argv vector to self,
// creating strdup()-copies of all strings in the given vector.
// @return: KE_OK:    Successfully initialized Shebang arguments.
// @return: KE_NOMEM: Not enough available memory.
extern __wunused kerrno_t
ksbargs_prepend(struct ksbargs *__restrict self,
                __size_t argc, char const *const *__restrict argv);



struct kshlib {
 /* Shared library/cached executable controller object
  * NOTE: At this point, shared libs and exe-s (aka. binaries) are the same thing.
  * NOTE: Shared libraries are implicitly synchronized, as all members
  *       must be considered constant post initialization. */
 KOBJECT_HEAD
 __atomic __u32          sh_refcnt;     /*< Reference counter. */
 __ref struct kfile     *sh_file;       /*< [1..1] Open file to this shared library (Used to retrieve library name/path). */
 kmodkind_t              sh_flags;      /*< Shared library kind & flags. */
 __size_t                sh_cidx;       /*< [lock(kslcache_lock)] Cache index (set to (size_t)-1 when not cached). */
union{struct{
 struct ksymtable        sh_publicsym;  /*< Hash-table of public symbols (exported by this shared library). */
 struct ksymtable        sh_weaksym;    /*< Hash-table of weak symbols (exported by this shared library). */
 struct ksymtable        sh_privatesym; /*< Hash-table of private symbols (for addr2line, etc.). */
 struct kshliblist       sh_deps;       /*< Library dependencies. */
 struct ksecdata         sh_data;       /*< Section data. */
 struct kreloc           sh_reloc;      /*< Relocation information. */
 struct kshlib_callbacks sh_callbacks;  /*< Special callbacks recognized by the linker. */
 struct kaddr2linelist   sh_addr2line;  /*< Address->line technologies. */
};struct{ /* KMODKIND_SHEBANG */
 __ref struct kshlib    *sh_sb_ref;     /*< [1..1][const] Reference to the binary referenced by the given shebang script. */
 struct ksbargs          sh_sb_args;    /*<  */
};};
};

__local KOBJECT_DEFINE_INCREF(kshlib_incref,struct kshlib,sh_refcnt,kassert_kshlib);
__local KOBJECT_DEFINE_DECREF(kshlib_decref,struct kshlib,sh_refcnt,kassert_kshlib,kshlib_destroy);

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
kshlib_elf32_new(struct kshlib **__restrict result,
                 struct kfile *__restrict elf_file);
extern __crit __wunused __nonnull((1,2)) kerrno_t
kshlib_pe32_new(struct kshlib **__restrict result,
                struct kfile *__restrict pe_file);
extern __crit __wunused __nonnull((1,2)) kerrno_t
kshlib_shebang_new(struct kshlib **__restrict result,
                   struct kfile *__restrict sb_file);

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

//////////////////////////////////////////////////////////////////////////
// Open a user-level executable as searched for by exec().
// Looks at the $PATH environment variable of the calling process,
// but also accepts absolute paths.
// @param: flags: A set of 'KTASK_EXEC_FLAG_*'
extern __crit __wunused __nonnull((1,3)) kerrno_t
kshlib_openexe(char const *__restrict name, __size_t namemax,
               __ref struct kshlib **__restrict result, __u32 flags);


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
kshlib_fopenfile(struct kfile *__restrict fp,
                 __ref struct kshlib **__restrict result);

/* TODO: These compiler-settings should be turned
 *       into process-local debug flags. */
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
//          the 'KMODFLAG_LOADED' flag has been set).
//          This must be done to detect and handle cyclic dependencies.
// @return: NULL: No library with the given path was cached.
// @return: * :   A new reference to a cached shared library.
extern __crit __ref struct kshlib *kshlibcache_getlib(char const *__restrict absolute_path);
extern __crit __ref struct kshlib *kshlibcache_fgetlib(struct kfile *__restrict fp);

//////////////////////////////////////////////////////////////////////////
// Query information on a given module
extern __crit __wunused __nonnull((1)) kerrno_t
kshlib_user_getinfo(struct kshlib *__restrict self,
                    __user void *module_base, kmodid_t id,
                    __user struct kmodinfo *buf, __size_t bufsize,
                    __kernel __size_t *reqsize, __u32 flags);

//////////////////////////////////////////////////////////////////////////
// Returns the symbol closest to the given address,
// or NULL if not symbol is located at that address.
extern __wunused __nonnull((1)) struct ksymbol const *
kshlib_get_closest_symbol(struct kshlib *__restrict self,
                          ksymaddr_t sym_address, __u32 *flags);

extern __wunused __nonnull((1,2)) struct ksymbol const *
kshlib_get_any_symbol(struct kshlib *__restrict self,
                      char const *name, __size_t name_size,
                      __size_t name_hash, __u32 *flags);


#if KCONFIG_SHLIB_RECENT_CACHE_SIZE
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

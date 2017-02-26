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
#ifndef __KOS_KERNEL_LINKER_PE32_C_INL__
#define __KOS_KERNEL_LINKER_PE32_C_INL__ 1

#include <kos/compiler.h>
#include <kos/config.h>
#include <kos/errno.h>
#include <kos/kernel/debug.h>
#include <kos/kernel/fs/file.h>
#include <kos/kernel/linker.h>
#include <kos/kernel/syslog.h>
#include <kos/kernel/pageframe.h>
#include <kos/kernel/shm.h>
#include <sys/types.h>
#include <windows/pe.h>
#include <elf.h>
#include <math.h>
#include <stddef.h>

#include "linker-pe32.h"

__DECL_BEGIN


static __crit __wunused char *
ksecdata_getzerostring(struct ksecdata const *__restrict self,
                       ksymaddr_t symbol_address, void **free_buffer) {
 size_t max_size = (size_t)-1;
 size_t result_size,nsize;
 char *addr,*result,*resiter;
 ksymaddr_t sym_iter;
 addr = (char *)ksecdata_translate_ro(self,symbol_address,&max_size);
 if __unlikely(!addr) return NULL;
 if (strnlen(addr,max_size/sizeof(char)) != max_size/sizeof(char)) {
  *free_buffer = NULL;
  return addr;
 }
 /* Must create a new buffer of linear memory. */
 result_size = 0;
 sym_iter    = symbol_address;
 for (;;) {
  result_size += max_size/sizeof(char);
  sym_iter    += max_size;
  max_size = (size_t)-1;
  addr = (char *)ksecdata_translate_ro(self,sym_iter,&max_size);
  if __unlikely(!addr) return NULL;
  nsize = strnlen(addr,max_size/sizeof(char));
  if (nsize != max_size/sizeof(char)) break;
 }
 result_size += nsize;
 result = (char *)malloc((result_size+1)*sizeof(char));
 if __unlikely(!result) return NULL; /* TODO: Better error handling. */
 max_size = result_size;
 resiter = result;
 result_size *= sizeof(char);
 while (result_size) {
  max_size = result_size;
  addr = (char *)ksecdata_translate_ro(self,symbol_address,&max_size);
  if __unlikely(!addr) break;
  memcpy(resiter,addr,max_size);
  resiter     += max_size;
  result_size -= max_size;
 }
 *resiter = '\0';
 *free_buffer = (void *)result;
 return result;
}

__crit kerrno_t
kshlib_pe32_parsethunks(struct kshlib *__restrict self,
                        ksymaddr_t first_thunk, ksymaddr_t rt_thunks,
                        struct kfile *__restrict pe_file, __uintptr_t image_base,
                        struct kshlib *__restrict import_lib) {
 struct ksymbol *sym;
 struct ksymbol **symv,**new_symv;
 size_t symc,syma,new_syma;
 IMAGE_THUNK_DATA32 t;
 kerrno_t error = KE_OK;
 syma = symc = 0; symv = NULL;
 for (; !ksecdata_getmem(&self->sh_data,first_thunk,
                         &t,sizeof(t));
        first_thunk += sizeof(t)) {
  char *import_name; void *zero_buffer;
  if (!t.AddressOfData) break; /* ZERO-Terminated. */
  /* Extract a pointer to the symbol name.
   * NOTE: Since the KOS linker uses a hash-table, we don't have any
   *       use for the symbol hint, which is why we simply skip it. */
  import_name = ksecdata_getzerostring(&self->sh_data,
                                       t.AddressOfData+image_base+
                                       offsetof(IMAGE_IMPORT_BY_NAME,Name),
                                       &zero_buffer);
  if (import_name) {
   /* We've got an imported symbol! */
   k_syslogf_prefixfile(KLOG_TRACE,pe_file,"[PE] Importing symbol: %q\n",import_name);
   sym = ksymbol_new(import_name);
   free(zero_buffer);
   if __unlikely(!sym) { error = KE_NOMEM; goto err; }
   if __unlikely(syma == symc) {
    /* Append the symbol to the symbol vector later used during relocations. */
    new_syma = syma ? syma*2 : 2;
    new_symv = (struct ksymbol **)realloc(symv,new_syma*sizeof(struct ksymbol *));
    if __unlikely(!new_symv) { error = KE_NOMEM; goto err_sym; }
    symv = new_symv,syma = new_syma;
   }
   symv[symc++] = sym;
   sym->s_size  = 0;
   sym->s_addr  = 0;
   sym->s_shndx = SHN_UNDEF;
   /* Following ELF-style linkage convention, we declare imported symbols as public. */
   error = ksymtable_insert_inherited(&self->sh_publicsym,sym);
   if __unlikely(KE_ISERR(error)) goto err_sym;
  }
 }
 assert((symc != 0) == (symv != NULL));
 if (symc) {
  struct krelocvec *relentry;
  /* Try to safe some memory in the symbol vector. */
  if __likely(syma != symc) {
   new_symv = (struct ksymbol **)realloc(symv,symc*sizeof(struct ksymbol *));
   if (new_symv) symv = new_symv;
  }
  /* Register a relocation entry for our symbol import vector.
   * >> That way, imported symbols from the import
   *    table will be relocated during loading. */
  relentry = kreloc_alloc(&self->sh_reloc);
  if __unlikely(!relentry) goto err;
  relentry->rv_type         = KRELOCVEC_TYPE_PE_IMPORT;
  relentry->rv_relc         = symc;
  relentry->rv_pe.rpi_symv  = symv;
  relentry->rv_pe.rpi_thunk = rt_thunks;
  relentry->rv_pe.rpi_hint  = import_lib;
 }
 return error;
err_sym: ksymbol_delete(sym);
err:     free(symv);
 return error;
}


__crit kerrno_t
kshlib_pe32_parseimports(struct kshlib *__restrict self,
                         IMAGE_IMPORT_DESCRIPTOR const *__restrict descrv,
                         size_t max_descrc, struct kfile *__restrict pe_file,
                         uintptr_t image_base) {
 IMAGE_IMPORT_DESCRIPTOR const *iter,*end;
 kerrno_t error; char *name; void *zero_buffer;
 __ref struct kshlib *dep;
 KTASK_CRIT_MARK
 kassert_kshlib(self);
 assert(max_descrc);
 kassertmem(descrv,max_descrc*sizeof(IMAGE_IMPORT_DESCRIPTOR));
 k_syslogf_prefixfile(KLOG_TRACE,pe_file,"Parse up to %Iu import descriptors...\n",max_descrc);
 end = (iter = descrv)+max_descrc;
 do {
  /* The import is terminated by a ZERO-entry.
   * https://msdn.microsoft.com/en-us/library/ms809762.aspx */
  if (!iter->Characteristics && !iter->TimeDateStamp &&
      !iter->ForwarderChain && !iter->Name &&
      !iter->FirstThunk) break;
  name = ksecdata_getzerostring(&self->sh_data,iter->Name+image_base,&zero_buffer);
  if __likely(name) {
   k_syslogf_prefixfile(KLOG_INFO,pe_file,"[PE] Searching for dependency: %q\n",name);
   /* Recursively open dependencies */
   if __unlikely(KE_ISERR(error = kshlib_openlib(name,(size_t)-1,&dep))) {
    k_syslogf_prefixfile(KLOG_ERROR,pe_file,"[PE] Missing dependency: %q\n",name);
    if (KSHLIB_IGNORE_MISSING_DEPENDENCIES) { error = KE_OK; goto after_thunks; }
   } else if (!(dep->sh_flags&KSHLIB_FLAG_LOADED)) {
    k_syslogf_prefixfile(KLOG_ERROR,pe_file,"[PE] Dependency loop: %q\n",name);
    /* Dependency loop */
    kshlib_decref(dep);
    error = KE_LOOP;
   } else {
    error = kshliblist_append_inherited(&self->sh_deps,dep);
    if __unlikely(KE_ISERR(error)) kshlib_decref(dep);
   }
   if __likely(KE_ISOK(error)) {
    DWORD first_thunk,rt_thunk;
    if ((first_thunk = iter->OriginalFirstThunk) == 0) first_thunk = iter->FirstThunk;
    if ((rt_thunk    = iter->FirstThunk) == 0) rt_thunk    = iter->OriginalFirstThunk;
    error = kshlib_pe32_parsethunks(self,image_base+first_thunk,
                                         image_base+rt_thunk,
                                    pe_file,image_base,dep);
   }
after_thunks:
   free(zero_buffer);
   if __unlikely(KE_ISERR(error)) return error;
  }
 } while (++iter != end);
 return KE_OK;
}

__crit __wunused kerrno_t
kshlib_pe32_parseexports(struct kshlib *__restrict self,
                         IMAGE_EXPORT_DIRECTORY const *__restrict descrv,
                         size_t max_descrc, struct kfile *__restrict pe_file,
                         uintptr_t image_base) {
 IMAGE_EXPORT_DIRECTORY const *iter,*end; kerrno_t error;
 DWORD num_exports; char *name; void *zero_buffer;
 ksymaddr_t funp_iter,namp_iter; struct ksymbol *sym;
 ksymaddr_t addr,nameaddr;
 KTASK_CRIT_MARK
 kassert_kshlib(self);
 assert(max_descrc);
 kassertmem(descrv,max_descrc*sizeof(IMAGE_IMPORT_DESCRIPTOR));
 end = (iter = descrv)+max_descrc;
 do {
  num_exports = min(iter->NumberOfFunctions,iter->NumberOfNames);
  if (num_exports) {
   funp_iter = image_base+iter->AddressOfFunctions;
   namp_iter = image_base+iter->AddressOfNames;
   while (num_exports--) {
    /* Read array fields and dereference the name address (OMG! So much indirection :( ). */
    if (ksecdata_getmem(&self->sh_data,funp_iter,&addr,sizeof(addr)) ||
        ksecdata_getmem(&self->sh_data,namp_iter,&nameaddr,sizeof(nameaddr)) ||
       (name = ksecdata_getzerostring(&self->sh_data,image_base+nameaddr,&zero_buffer)) == NULL) break;
    addr += image_base;
    k_syslogf_prefixfile(KLOG_DEBUG,pe_file,"Exporting symbol %q at %p...\n",name,addr);
    sym = ksymbol_new(name);
    free(zero_buffer);
    if __unlikely(!sym) return KE_NOMEM;
    sym->s_size  = 0;
    sym->s_addr  = (ksymaddr_t)addr;
    sym->s_shndx = SHN_UNDEF+1; /* Something different than 'SHN_UNDEF'... */
    /* Declare exported symbols public, as well. */
    error = ksymtable_insert_inherited(&self->sh_publicsym,sym);
    if __unlikely(KE_ISERR(error)) { ksymbol_delete(sym); return error; }
    /* Advance to the next symbol. */
    funp_iter += sizeof(ksymaddr_t);
    namp_iter += sizeof(ksymaddr_t);
   }
  }
 } while (++iter != end);
 return KE_OK;
}



__crit kerrno_t
ksecdata_pe32_init(struct ksecdata *__restrict self,
                   IMAGE_SECTION_HEADER const *__restrict headerv,
                   size_t headerc, struct kfile *__restrict pe_file,
                   uintptr_t image_base) {
 struct kshlibsection *iter; struct kshmregion *region;
 IMAGE_SECTION_HEADER const *head_iter,*head_end;
 size_t usable_headers = 0; kerrno_t error = KE_OK;
 size_t pages,filesize; kshm_flag_t flags;
 kassertobj(self);
 head_end = (head_iter = headerv)+headerc;
 for (; head_iter != head_end; ++head_iter) {
  if (head_iter->Misc.VirtualSize) ++usable_headers;
 }
 self->ed_secc = usable_headers;
 if (!usable_headers) { self->ed_secv = NULL; goto done; }
 self->ed_secv = (struct kshlibsection *)malloc(usable_headers*sizeof(struct kshlibsection));
 if __unlikely(!self->ed_secv) return KE_NOMEM;
 iter = self->ed_secv;
 head_end = (head_iter = headerv)+headerc;
 for (; head_iter != head_end; ++head_iter) {
  if (head_iter->Misc.VirtualSize) {
   assert(iter != self->ed_secv+self->ed_secc);
   pages = ceildiv(head_iter->Misc.VirtualSize,PAGESIZE);
   flags = KSHMREGION_FLAG_NONE;
#define CHARACTERISTIC(f) ((head_iter->Characteristics&(f))==(f))
   if (CHARACTERISTIC(IMAGE_SCN_MEM_SHARED))  flags |= KSHMREGION_FLAG_SHARED;
   if (CHARACTERISTIC(IMAGE_SCN_MEM_EXECUTE)) flags |= KSHMREGION_FLAG_EXEC;
   if (CHARACTERISTIC(IMAGE_SCN_MEM_READ))    flags |= KSHMREGION_FLAG_READ;
   if (CHARACTERISTIC(IMAGE_SCN_MEM_WRITE))   flags |= KSHMREGION_FLAG_WRITE;
#undef CHARACTERISTIC
   /* Allocate a region big enough to support this section. */
   region = kshmregion_newram(pages,flags);
   if __unlikely(!region) { error = KE_NOMEM; goto err_iter; }
   iter->sls_region = region;
   iter->sls_size   = head_iter->Misc.VirtualSize;
   iter->sls_base   = image_base+(ksymaddr_t)head_iter->VirtualAddress;
   iter->sls_albase = alignd(iter->sls_base,PAGEALIGN);
   filesize         = head_iter->SizeOfRawData;
   if __unlikely(filesize > iter->sls_size) filesize = iter->sls_size;
   iter->sls_filebase = (ksymaddr_t)head_iter->PointerToRawData;
   if (!iter->sls_filebase) filesize = 0;
   iter->sls_filesize = filesize;
   /* Actually load the section from disk. */
   error = kshmregion_ploaddata(region,iter->sls_base-iter->sls_albase,
                                pe_file,filesize,iter->sls_filebase);
   if __unlikely(KE_ISERR(error)) goto err_region;
   k_syslogf_prefixfile(KLOG_INFO,pe_file
                       ,"[PE] Loading section %.*q from %.8I64u to %.8p+%.8Iu...%.8p (%c%c%c%c)\n"
                       ,(unsigned)IMAGE_SIZEOF_SHORT_NAME,(char *)head_iter->Name
                       ,(__u64)iter->sls_filebase
                       ,(uintptr_t)iter->sls_base,(size_t)iter->sls_size
                       ,(uintptr_t)iter->sls_base+(size_t)iter->sls_size
                       ,(kshmregion_getflags(region)&KSHMREGION_FLAG_SHARED) ? 'S' : '-'
                       ,(kshmregion_getflags(region)&KSHMREGION_FLAG_READ) ? 'R' : '-'
                       ,(kshmregion_getflags(region)&KSHMREGION_FLAG_WRITE) ? 'W' : '-'
                       ,(kshmregion_getflags(region)&KSHMREGION_FLAG_EXEC) ? 'X' : '-');
   ++iter;
   /* TODO: Relocations. */
   // head_iter->PointerToRelocations;
  }
 }
 assert(iter == self->ed_secv+self->ed_secc);
done:
 ksecdata_cacheminmax(self);
 return error;
err_region: kshmregion_decref_full(region);
err_iter:
 while (iter-- != self->ed_secv) {
  kshmregion_decref_full(iter->sls_region);
 }
 free(self->ed_secv);
 return error;
}




__crit kerrno_t
kshlib_pe32_new(struct kshlib **__restrict result,
                struct kfile *__restrict pe_file) {
 struct kshlib *lib; kerrno_t error;
 IMAGE_DOS_HEADER dos_header;
 IMAGE_NT_HEADERS32 file_header;
 IMAGE_SECTION_HEADER *section_headers;
 pos_t section_headers_start;
 uintptr_t image_base;
 kassertobj(result);
 kassert_kfile(pe_file);
 /* Win32 PE executables. */
 if __unlikely((lib = omalloc(struct kshlib)) == NULL) return KE_NOMEM;
 kobject_init(lib,KOBJECT_MAGIC_SHLIB);
 lib->sh_refcnt = 1;
 lib->sh_flags = KSHLIB_FLAG_PREFER_FIXED;
 error = kfile_kernel_readall(pe_file,&dos_header,sizeof(dos_header));
 if __unlikely(KE_ISERR(error)) goto err_lib;
 /* Validate that this is really a PE executable. */
 if __unlikely(dos_header.e_magic != PE_DOSHEADER_MAGIC) { error = KE_NOEXEC; goto err_lib; }
 error = kfile_incref(lib->sh_file = pe_file);
 if __unlikely(KE_ISERR(error)) goto err_lib;
 error = kshlibcache_addlib(lib);
 if __unlikely(KE_ISERR(error)) {
  lib->sh_cidx = (__size_t)-1;
  k_syslogf_prefixfile(KLOG_WARN,pe_file,
                       "[PE] Failed to cache library file\n");
 }

 /* Load the file header post the 16-bit realmode code. */
 error = kfile_seek(pe_file,dos_header.e_lfanew,SEEK_SET,NULL);
 if __unlikely(KE_ISERR(error)) goto err_cache;
 error = kfile_kernel_readall(pe_file,&file_header,offsetof(IMAGE_NT_HEADERS32,OptionalHeader));
 if __unlikely(KE_ISERR(error)) goto err_cache;
 if (file_header.Signature != PE_FILE_SIGNATURE
#ifdef __i386__
  || file_header.FileHeader.Machine != IMAGE_FILE_MACHINE_I386
#endif
     ) { error = KE_NOEXEC; goto err_cache; }

#define HAS_OPTION(x) \
 (file_header.FileHeader.SizeOfOptionalHeader >= offsetafter(IMAGE_OPTIONAL_HEADER32,x))
#define HAS_DATADIR(i) HAS_OPTION(DataDirectory[i])
#define DATADIR(i) file_header.OptionalHeader.DataDirectory[i]

 if (file_header.FileHeader.SizeOfOptionalHeader) {
  if (file_header.FileHeader.SizeOfOptionalHeader > sizeof(file_header.OptionalHeader))
      file_header.FileHeader.SizeOfOptionalHeader = sizeof(file_header.OptionalHeader);
  error = kfile_kernel_readall(pe_file,&file_header.OptionalHeader,
                               file_header.FileHeader.SizeOfOptionalHeader);
  if __unlikely(KE_ISERR(error)) goto err_cache;
 }

 image_base = HAS_OPTION(ImageBase) ? file_header.OptionalHeader.ImageBase : 0;
 if (HAS_OPTION(AddressOfEntryPoint)) {
  lib->sh_callbacks.slc_start = (ksymaddr_t)file_header.OptionalHeader.AddressOfEntryPoint;
 } else {
  k_syslogf_prefixfile(KLOG_WARN,pe_file,"[PE] Binary file has no entry point (Optional header size: %Ix)\n",
                      (size_t)file_header.FileHeader.SizeOfOptionalHeader);
  lib->sh_callbacks.slc_start = (ksymaddr_t)0; /* ??? */
 }
 lib->sh_callbacks.slc_start += image_base;
 k_syslogf_prefixfile(KLOG_DEBUG,pe_file,"[PE] Determined entry pointer %p (base %p)\n",
                      lib->sh_callbacks.slc_start,image_base);
 lib->sh_callbacks.slc_init            = KSYM_INVALID;
 lib->sh_callbacks.slc_fini            = KSYM_INVALID;
 lib->sh_callbacks.slc_preinit_array_v = KSYM_INVALID;
 lib->sh_callbacks.slc_preinit_array_c = 0;
 lib->sh_callbacks.slc_init            = KSYM_INVALID;
 lib->sh_callbacks.slc_init_array_v    = KSYM_INVALID;
 lib->sh_callbacks.slc_init_array_c    = 0;
 lib->sh_callbacks.slc_fini            = KSYM_INVALID;
 lib->sh_callbacks.slc_fini_array_v    = KSYM_INVALID;
 lib->sh_callbacks.slc_fini_array_c    = 0;

 ksymtable_init(&lib->sh_publicsym);
 ksymtable_init(&lib->sh_weaksym);
 ksymtable_init(&lib->sh_privatesym);

 kshliblist_init(&lib->sh_deps);
 kreloc_init(&lib->sh_reloc);

 /* Allocate temporary memory for all section headers. */
 section_headers_start = dos_header.e_lfanew+
                         offsetof(IMAGE_NT_HEADERS32,OptionalHeader)+
                         file_header.FileHeader.SizeOfOptionalHeader;
 section_headers = (IMAGE_SECTION_HEADER *)malloc(file_header.FileHeader.NumberOfSections*
                                                sizeof(IMAGE_SECTION_HEADER));
 if __unlikely(!section_headers) { error = KE_NOMEM; goto err_cache; }
 error = kfile_kernel_preadall(pe_file,section_headers_start,section_headers,
                               file_header.FileHeader.NumberOfSections*
                               sizeof(IMAGE_SECTION_HEADER));
 if __unlikely(KE_ISERR(error)) goto err_secheaders;
 error = ksecdata_pe32_init(&lib->sh_data,section_headers,
                            file_header.FileHeader.NumberOfSections,
                            pe_file,image_base);
 free(section_headers); /* Set to NULL to trick cleanup code. */
 if __unlikely(KE_ISERR(error)) goto err_reloc;

 /* Parse Imports. */
 if (HAS_DATADIR(IMAGE_DIRECTORY_ENTRY_IMPORT)) {
  PIMAGE_IMPORT_DESCRIPTOR import_descrv;
  PIMAGE_DATA_DIRECTORY dir = &DATADIR(IMAGE_DIRECTORY_ENTRY_IMPORT);
  size_t good_data,import_descrc = dir->Size/sizeof(IMAGE_IMPORT_DESCRIPTOR);
  ksymaddr_t dir_address;
  if (import_descrc) {
   dir->Size = good_data = import_descrc*sizeof(IMAGE_IMPORT_DESCRIPTOR);
   dir_address   = dir->VirtualAddress+image_base;
   import_descrv = (PIMAGE_IMPORT_DESCRIPTOR)ksecdata_translate_ro(&lib->sh_data,dir_address,&good_data);
   if __likely(import_descrv && good_data == dir->Size) {
    /* No need to allocate a temporary buffer (Data wasn't partitioned). */
    error = kshlib_pe32_parseimports(lib,import_descrv,good_data/
                                     sizeof(IMAGE_IMPORT_DESCRIPTOR),
                                     pe_file,image_base);
   } else {
    good_data = dir->Size;
    import_descrv = (PIMAGE_IMPORT_DESCRIPTOR)malloc(good_data);
    if __unlikely(!import_descrv) { error = KE_NOMEM; goto err_data; }
    /* Handle invalid pointers by clamping their address range.
     * >> What we're doing here is just the perfect example of
     *    what I always call "weak undefined behavior":
     *    It'll continue running stable, but it's
     *    probably not gonna work correctly. */
    good_data -= ksecdata_getmem(&lib->sh_data,dir_address,
                                 import_descrv,good_data);
    if __likely(good_data) {
     error = kshlib_pe32_parseimports(lib,import_descrv,good_data/
                                      sizeof(IMAGE_IMPORT_DESCRIPTOR),
                                      pe_file,image_base);
    } else {
     error = KE_OK;
    }
    free(import_descrv);
   }
   if __unlikely(KE_ISERR(error)) goto err_data;
  }
 }

 /* Parse Exports. */
 if (HAS_DATADIR(IMAGE_DIRECTORY_ENTRY_EXPORT)) {
  PIMAGE_EXPORT_DIRECTORY export_descrv;
  PIMAGE_DATA_DIRECTORY dir = &DATADIR(IMAGE_DIRECTORY_ENTRY_EXPORT);
  size_t good_data,export_descrc = dir->Size/sizeof(IMAGE_EXPORT_DIRECTORY);
  ksymaddr_t dir_address;
  if (export_descrc) {
   dir->Size = good_data = export_descrc*sizeof(IMAGE_EXPORT_DIRECTORY);
   dir_address   = dir->VirtualAddress+image_base;
   export_descrv = (PIMAGE_EXPORT_DIRECTORY)ksecdata_translate_ro(&lib->sh_data,dir_address,&good_data);
   if __likely(export_descrv && good_data == dir->Size) {
    /* No need to allocate a temporary buffer (Data wasn't partitioned). */
    error = kshlib_pe32_parseexports(lib,export_descrv,good_data/
                                     sizeof(IMAGE_EXPORT_DIRECTORY),
                                     pe_file,image_base);
   } else {
    good_data = dir->Size;
    export_descrv = (PIMAGE_EXPORT_DIRECTORY)malloc(good_data);
    if __unlikely(!export_descrv) { error = KE_NOMEM; goto err_data; }
    good_data -= ksecdata_getmem(&lib->sh_data,dir_address,
                                 export_descrv,good_data);
    if __likely(good_data) {
     error = kshlib_pe32_parseexports(lib,export_descrv,good_data/
                                      sizeof(IMAGE_EXPORT_DIRECTORY),
                                      pe_file,image_base);
    } else {
     error = KE_OK;
    }
    free(export_descrv);
   }
   if __unlikely(KE_ISERR(error)) goto err_data;
  }
 }

#undef DATADIR
#undef HAS_DATADIR
#undef HAS_OPTION
 lib->sh_flags |= KSHLIB_FLAG_LOADED;
 *result = lib;
 kshlibrecent_add(lib);

 return error;
err_secheaders: free(section_headers);
 if (0) {err_data: ksecdata_quit(&lib->sh_data); }
err_reloc: kreloc_quit(&lib->sh_reloc);
 kshliblist_quit(&lib->sh_deps);
 ksymtable_quit(&lib->sh_privatesym);
 ksymtable_quit(&lib->sh_weaksym);
 ksymtable_quit(&lib->sh_publicsym);
err_cache: kshlibcache_dellib(lib);
/*err_file:*/ kfile_decref(lib->sh_file);
err_lib: free(lib);
 return error;
}


__DECL_END

#endif /* !__KOS_KERNEL_LINKER_PE32_C_INL__ */

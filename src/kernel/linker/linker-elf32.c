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
#ifndef __KOS_KERNEL_LINKER_ELF32_C_INL__
#define __KOS_KERNEL_LINKER_ELF32_C_INL__ 1

#include "linker-elf32.h"
#include <kos/errno.h>
#include <kos/syslog.h>
#include <kos/compiler.h>
#include <kos/kernel/linker.h>
#include <kos/kernel/fs/file.h>
#include <elf.h>
#include <sys/types.h>
#include <malloc.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>

#include "linker-DWARF.h"

__DECL_BEGIN


__crit kerrno_t
ksecdata_elf32_init(struct ksecdata *__restrict self,
                    Elf32_Phdr const *__restrict pheaderv,
                    size_t pheaderc, size_t pheadersize,
                    struct kfile *__restrict elf_file) {
 Elf32_Phdr const *iter,*end; size_t req_sections;
 struct kshlibsection *sections,*seciter; kerrno_t error;
 kassertobj(self);
 kassertmem(pheaderv,pheaderc*sizeof(Elf32_Phdr));
 kassert_kfile(elf_file); req_sections = 0;
 if __unlikely(pheadersize < offsetafter(Elf32_Phdr,p_memsz)) goto set_section_count;
 // Figure out how many sections are needed (Only counting non-empty PT_LOAD headers)
 end = (Elf32_Phdr *)((uintptr_t)(iter = pheaderv)+(pheaderc*pheadersize));
 for (; iter != end; *(uintptr_t *)&iter += pheadersize) {
  if (iter->p_type == PT_LOAD && iter->p_memsz) ++req_sections;
 }
set_section_count:
 self->ed_secc = req_sections;
 if __unlikely(!req_sections) {
  self->ed_secv = NULL;
  return KE_OK;
 }
 sections = (struct kshlibsection *)malloc(req_sections*sizeof(struct kshlibsection));
 if __unlikely(!sections) return KE_NOMEM;
 self->ed_secv = seciter = sections;
 for (iter = pheaderv; iter != end; *(uintptr_t *)&iter += pheadersize) {
  if (iter->p_type == PT_LOAD && iter->p_memsz) {
   struct kshmregion *section_region; size_t memsize,filesize;
   Elf32_Phdr header_copy; Elf32_Phdr const *header;
   if __likely(pheadersize >= sizeof(Elf32_Phdr)) {
    header = iter;
   } else {
    // Super unlikely case, but must be checked
    memset(&header_copy,0,sizeof(header_copy));
    memcpy(&header_copy,iter,pheadersize);
    header = &header_copy;
   }
   memsize = (size_t)header->p_memsz;
   k_syslogf_prefixfile(KLOG_INFO,elf_file
                       ,"[ELF] Loading segment from %.8I64u to %.8p+%.8Iu...%.8p (%c%c%c) (align: %Iu)\n"
                       ,(__u64)header->p_offset
                       ,(uintptr_t)header->p_vaddr,(size_t)header->p_memsz
                       ,(uintptr_t)header->p_vaddr+(size_t)header->p_memsz
                       ,(header->p_flags&PF_R) ? 'R' : '-'
                       ,(header->p_flags&PF_W) ? 'W' : '-'
                       ,(header->p_flags&PF_X) ? 'X' : '-'
                       ,(uintptr_t)header->p_align);
   section_region = kshmregion_newram(ceildiv(memsize,PAGESIZE),
#if PF_X == KSHMREGION_FLAG_EXEC && \
    PF_W == KSHMREGION_FLAG_WRITE && \
    PF_R == KSHMREGION_FLAG_READ
                                  (header->p_flags&(PF_X|PF_W|PF_R))
#else
                                  ((header->p_flags&PF_X) ? KSHMREGION_FLAG_EXEC : 0)|
                                  ((header->p_flags&PF_W) ? KSHMREGION_FLAG_WRITE : 0)|
                                  ((header->p_flags&PF_R) ? KSHMREGION_FLAG_READ : 0)
#endif
                                  );
   if __unlikely(!section_region) {
    error = KE_NOMEM;
err_seciter:
    while (seciter != sections) {
     --seciter;
     kshmregion_decref_full(seciter->sls_region);
     free(sections);
    }
    return error;
   }
   seciter->sls_region = section_region;
   seciter->sls_size   = memsize;
   seciter->sls_base   = (ksymaddr_t)header->p_vaddr;
   seciter->sls_albase = alignd(seciter->sls_base,PAGEALIGN);
   filesize = header->p_filesz;
   if __unlikely(filesize > memsize) filesize = memsize;
   seciter->sls_filebase = (ksymaddr_t)header->p_offset;
   seciter->sls_filesize = filesize;
   error = kshmregion_ploaddata(section_region,seciter->sls_base-seciter->sls_albase,
                                elf_file,filesize,header->p_offset);
   if __unlikely(KE_ISERR(error)) {
    kshmregion_decref_full(section_region);
    goto err_seciter;
   }
   ++seciter;
  }
 }
 assert(seciter == self->ed_secv+self->ed_secc);
 return KE_OK;
}

static kerrno_t
kshlib_elf32_load_symtable(struct kshlib *__restrict self, Elf32_Shdr const *__restrict strtab,
                           Elf32_Shdr const *__restrict symtab, struct kfile *__restrict elf_file,
                           int is_dynamic) {
 char *strtable,*name; kerrno_t error;
 Elf32_Sym *symtable,*sym_iter,*sym_end;
 size_t strtable_size,symtable_size,symcount;
 size_t private_count,public_count,weak_count;
 struct ksymbol **dynsym;
 if (symtab->sh_entsize < offsetafter(Elf32_Sym,st_value)) return KE_OK;
 if (!strtab->sh_offset) {
  strtable_size = 0;
  strtable      = NULL;
 } else {
  strtable_size = strtab->sh_size;
  strtable = (char *)malloc(strtable_size+sizeof(char));
  if __unlikely(!strtable) return KE_NOMEM;
  error = kfile_kernel_preadall(elf_file,strtab->sh_offset,
                                strtable,strtable_size);
  if __unlikely(KE_ISERR(error)) goto end_strtable;
  strtable[strtable_size/sizeof(char)] = '\0';
 }
#define STRAT(addr) ((addr) >= strtable_size ? NULL : strtable+(addr))
 symtable_size = (size_t)(strtab->sh_offset-symtab->sh_offset);
 if (symtab->sh_size < symtable_size) symtable_size = symtab->sh_size;
 if (!symtable_size) return KE_OK;
 symtable = (Elf32_Sym *)malloc(symtable_size);
 if __unlikely(!symtable) { error = KE_NOMEM; goto end_strtable; }
 error = kfile_kernel_preadall(elf_file,symtab->sh_offset,symtable,symtable_size);
 if __unlikely(KE_ISERR(error)) goto end_symtable;
 sym_end = (Elf32_Sym *)((uintptr_t)symtable+symtable_size);
 private_count = public_count = weak_count = 0;
 for (sym_iter = symtable; sym_iter < sym_end; *(uintptr_t *)&sym_iter += symtab->sh_entsize) {
  if ((name = STRAT(sym_iter->st_name)) != NULL && *name != '\0') {
   unsigned char bind = ELF32_ST_BIND(sym_iter->st_info);
   switch (bind) {
    case STB_GLOBAL: ++public_count; break;
    case STB_LOCAL:  ++private_count; break;
    case STB_WEAK:   ++weak_count; break;
    default: break;
   }
  }
 }
 if __unlikely((private_count+public_count+weak_count) == 0) { error = KE_OK; goto end_symtable; }
 if (private_count) ksymtable_rehash(&self->sh_privatesym,self->sh_privatesym.st_size+private_count);
 if (public_count) ksymtable_rehash(&self->sh_publicsym,self->sh_publicsym.st_size+public_count);
 if (weak_count) ksymtable_rehash(&self->sh_weaksym,self->sh_weaksym.st_size+weak_count);
 if (is_dynamic) {
  // Vector of dynamic symbols.
  symcount = symtable_size/symtab->sh_entsize;
  dynsym = (struct ksymbol **)malloc(symcount*sizeof(struct ksymbol *));
  if __unlikely(!dynsym) { error = KE_NOMEM; goto end_symtable; }
  self->sh_reloc.r_elf_syms.sl_symc = symcount;
  self->sh_reloc.r_elf_syms.sl_symv = dynsym;
 }
 for (sym_iter = symtable; sym_iter < sym_end; *(uintptr_t *)&sym_iter += symtab->sh_entsize) {
  if ((name = STRAT(sym_iter->st_name)) != NULL && *name != '\0') {
   unsigned char bind = ELF32_ST_BIND(sym_iter->st_info);
   struct ksymbol *sym; struct ksymtable *symtbl;
        if (bind == STB_LOCAL) symtbl = &self->sh_privatesym;
   else if (bind == STB_GLOBAL) symtbl = &self->sh_publicsym;
   else if (bind == STB_WEAK) symtbl = &self->sh_weaksym;
   else goto sym_next;
   k_syslogf_prefixfile(KLOG_INSANE,elf_file,"[ELF] SYMBOL: %s %s: %p\n"
                       ,bind == STB_GLOBAL ? "__global" :
                        bind == STB_LOCAL ? "__local" :
                        bind == STB_WEAK ? "__weak" : ""
                       ,name,(uintptr_t)sym_iter->st_value);
   sym = ksymbol_new(name);
   if __unlikely(!sym) { error = KE_NOMEM; goto end_symtable; }
   sym->s_size = (size_t)sym_iter->st_size;
   sym->s_addr = (ksymaddr_t)sym_iter->st_value;
   sym->s_shndx = (size_t)sym_iter->st_shndx;
   error = ksymtable_insert_inherited(symtbl,sym);
   if __unlikely(KE_ISERR(error)) { ksymbol_delete(sym); goto end_symtable; }
   if (is_dynamic) *dynsym++ = sym;
  } else {
sym_next:;
   if (is_dynamic) *dynsym++ = NULL;
  }
 }
 assert(!is_dynamic ||
       (dynsym == self->sh_reloc.r_elf_syms.sl_symv+
                  self->sh_reloc.r_elf_syms.sl_symc));
end_symtable:
 free(symtable);
#undef STRAT
end_strtable:
 free(strtable);
 return error;
}




__local kerrno_t
kshlib_elf32_parse_needed(struct kshlib *__restrict self, struct kfile *__restrict elf_file,
                          Elf32_Dyn const *__restrict dynheader, size_t dynheader_c,
                          Elf32_Shdr const *__restrict strtable_header, size_t needed_count) {
 Elf32_Dyn const *iter,*end; char *strtable; kerrno_t error;
 size_t strtable_size = strtable_header->sh_size;
 __ref struct kshlib **dependencies,**dep_iter,*dep;
 strtable = (char *)malloc(strtable_size+sizeof(char));
 if __unlikely(!strtable) return KE_NOMEM;
 error = kfile_kernel_preadall(elf_file,strtable_header->sh_offset,
                        strtable,strtable_size);
 if __unlikely(KE_ISERR(error)) {
  k_syslogf_prefixfile(KLOG_ERROR,elf_file
                      ,"[ELF] Failed to load DT_NEEDED string table from %I64u|%Iu\n",
                      (__u64)strtable_header->sh_offset,strtable_size);
  goto end_strtable;
 }
 strtable_size /= sizeof(char);
 strtable[strtable_size] = '\0';
 end = (iter = dynheader)+dynheader_c;
 dependencies = (__ref struct kshlib **)malloc(needed_count*
                                               sizeof(struct kshlib *));
 if __unlikely(!dependencies) { error = KE_NOMEM; goto end_strtable; }
 self->sh_deps.sl_libc = needed_count;
 self->sh_deps.sl_libv = dep_iter = dependencies;
 for (; iter != end; ++iter) {
  char const *name;
  if (iter->d_tag == DT_NULL) break;
  if (iter->d_tag == DT_NEEDED && iter->d_un.d_val < strtable_size) {
   if ((name = strtable+iter->d_un.d_val)[0] != '\0') {
    k_syslogf_prefixfile(KLOG_INFO,elf_file,"[ELF] Searching for dependency: %q\n",name);
    // Recursively open dependencies
    if __unlikely(KE_ISERR(error = kshlib_openlib(name,(size_t)-1,&dep))) {
     k_syslogf_prefixfile(KLOG_ERROR,elf_file,"[ELF] Missing dependency: %q\n",name);
     if (!KSHLIB_IGNORE_MISSING_DEPENDENCIES) {
      if (error == KE_NOENT) error = KE_NODEP;
err_depiter:
      while (dep_iter != dependencies) kshlib_decref(*--dep_iter);
      free(dependencies);
      goto end_strtable;
     } else {
      --self->sh_deps.sl_libc;
     }
    } else if (!(dep->sh_flags&KMODFLAG_LOADED)) {
     k_syslogf_prefixfile(KLOG_ERROR,elf_file,"[ELF] Dependency loop: %q\n",name);
     // Dependency loop
     error = KE_LOOP;
     kshlib_decref(dep);
     goto err_depiter;
    } else {
     *dep_iter++ = dep; // Inherit reference
    }
   } else {
    --self->sh_deps.sl_libc;
   }
  }
 }
 assert(dep_iter == dependencies+self->sh_deps.sl_libc);
 if __unlikely(self->sh_deps.sl_libc != needed_count) {
  // This can happen if NULL-named dependencies were found.
  // >> Since KOS's linker isn't too strict, we simply ignore those.
  if __unlikely(!self->sh_deps.sl_libc) {
   free(dependencies);
   self->sh_deps.sl_libv = NULL;
  } else {
   dependencies = (__ref struct kshlib **)realloc(self->sh_deps.sl_libv,
                                                  self->sh_deps.sl_libc*
                                                  sizeof(__ref struct kshlib *));
   if __likely(dependencies) self->sh_deps.sl_libv = dependencies;
  }
 }
end_strtable:
 free(strtable);
 return error;
}

// Initialize 'sh_data' + 'sh_deps' and fill 'dyninfo'
__local kerrno_t
kshlib_elf32_load_pheaders(struct kshlib *__restrict self,
                           struct elf32_dynsyminfo *__restrict dyninfo,
                           struct kfile *__restrict elf_file,
                           Elf32_Ehdr const *__restrict ehdr) {
 kerrno_t error;
 kassertobj(self);
 kassertobj(dyninfo);
 kassertobj(ehdr);
 kassert_kfile(elf_file);
 kshliblist_init(&self->sh_deps);
 if __likely(ehdr->e_phnum && ehdr->e_phentsize >= offsetafter(Elf32_Phdr,p_memsz)) {
  Elf32_Dyn *dhdr,*diter,*dend; size_t has_needed;
  Elf32_Phdr *phdr,*phdr_iter,*phdr_end;
  size_t phsize = (size_t)ehdr->e_phnum*ehdr->e_phentsize;
  phdr = (Elf32_Phdr *)malloc(phsize);
  if __unlikely(!phdr) { error = KE_NOMEM; goto err_deps; }
  error = kfile_kernel_preadall(elf_file,ehdr->e_phoff,phdr,phsize);
  if __unlikely(KE_ISERR(error)) {__err_pheaders: free(phdr); goto err_deps; }
  error = ksecdata_elf32_init(&self->sh_data,phdr,ehdr->e_phnum,ehdr->e_phentsize,elf_file);
  if __unlikely(KE_ISERR(error)) goto __err_pheaders;
  phdr_end = (Elf32_Phdr *)((uintptr_t)(phdr_iter = phdr)+phsize);
  // Iterate all segments
  for (; phdr_iter != phdr_end;
       *(uintptr_t *)&phdr_iter += ehdr->e_phentsize
       ) switch (phdr_iter->p_type) {
   case PT_LOAD: break; // Already handled inside of 'ksecdata_elf32_init'
   case PT_DYNAMIC: // Dynamic header
    dhdr = (Elf32_Dyn *)malloc(phdr_iter->p_filesz);
    if __unlikely(!dhdr) { error = KE_NOMEM; err_pheaders: free(phdr); goto err_data; }
    error = kfile_kernel_preadall(elf_file,phdr_iter->p_offset,dhdr,
                           phdr_iter->p_filesz);
    if __unlikely(KE_ISERR(error)) {err_dhdr: free(dhdr); goto err_pheaders; }
    dend = dhdr+(phdr_iter->p_filesz/sizeof(Elf32_Dyn));
    has_needed = 0;
    for (diter = dhdr; diter != dend; ++diter) switch (diter->d_tag) {
     case DT_NULL           : goto done_dynamic_1;
     case DT_STRTAB         : dyninfo->dyn_strtable_header.sh_offset  = kshlib_symaddr2fileaddr(self,diter->d_un.d_ptr); break;
     case DT_STRSZ          : dyninfo->dyn_strtable_header.sh_size    = diter->d_un.d_val; break;
     case DT_SYMTAB         : dyninfo->dyn_symtable_header.sh_offset  = kshlib_symaddr2fileaddr(self,diter->d_un.d_ptr); break;
     case DT_SYMENT         : dyninfo->dyn_symtable_header.sh_entsize = diter->d_un.d_val; break;
     case DT_HASH:
      // This can be used to figure out the size of the symbol table
      // >> The size can be read from index 1 (aka. the sector index)
      if (ksecdata_getmem(&self->sh_data,
                          kshlib_fileaddr2symaddr(self,diter->d_un.d_ptr)+sizeof(Elf32_Word), // read the second index
                          &dyninfo->dyn_symtable_header.sh_size,sizeof(Elf32_Word))
          ) dyninfo->dyn_symtable_header.sh_size = (Elf32_Word)-1; // Was worth a try...
      break;
     case DT_INIT           : self->sh_callbacks.slc_init            = diter->d_un.d_ptr; break;
     case DT_FINI           : self->sh_callbacks.slc_fini            = diter->d_un.d_ptr; break;
     case DT_INIT_ARRAY     : self->sh_callbacks.slc_init_array_v    = diter->d_un.d_ptr; break;
     case DT_FINI_ARRAY     : self->sh_callbacks.slc_fini_array_v    = diter->d_un.d_ptr; break;
     case DT_INIT_ARRAYSZ   : self->sh_callbacks.slc_init_array_c    = diter->d_un.d_val; break;
     case DT_FINI_ARRAYSZ   : self->sh_callbacks.slc_fini_array_c    = diter->d_un.d_val; break;
     case DT_PREINIT_ARRAY  : self->sh_callbacks.slc_preinit_array_v = diter->d_un.d_ptr; break;
     case DT_PREINIT_ARRAYSZ: self->sh_callbacks.slc_preinit_array_c = diter->d_un.d_val; break;
     case DT_NEEDED         : ++has_needed; break; /* Handled in step #2 */
     case DT_RELACOUNT:
     case DT_RELCOUNT:
     case DT_RELA:
     case DT_RELASZ:
     case DT_RELAENT:
     case DT_REL:
     case DT_RELSZ:
     case DT_RELENT:
     case DT_PLTREL:
     case DT_TEXTREL:
     case DT_JMPREL:
     case DT_PLTRELSZ:
     case DT_PLTGOT:
      // We take our relocation information from section headers
      break;
     case DT_DEBUG:
      // Don't warn about this one...
      break;
     default:
      // Don't warn about these...
      if (diter->d_tag >= DT_LOOS && diter->d_tag <= DT_HIOS) break;
      if (diter->d_tag >= DT_LOPROC && diter->d_tag <= DT_HIPROC) break;
      k_syslogf_prefixfile(KLOG_WARN,elf_file,"[ELF] Unrecognized dynamic header: %d\n",(int)diter->d_tag);
      break;
    }
done_dynamic_1:
    if (dyninfo->dyn_strtable_header.sh_offset != 0 &&
        dyninfo->dyn_strtable_header.sh_size == (Elf32_Word)-1 &&
        dyninfo->dyn_strtable_header.sh_offset < dyninfo->dyn_symtable_header.sh_offset) {
      dyninfo->dyn_strtable_header.sh_size = dyninfo->dyn_symtable_header.sh_offset-
                                             dyninfo->dyn_strtable_header.sh_offset;
    }

    if (has_needed &&
        dyninfo->dyn_strtable_header.sh_offset != 0 &&
        dyninfo->dyn_strtable_header.sh_size != (Elf32_Word)-1) {
     error = kshlib_elf32_parse_needed(self,elf_file,dhdr,
                                 phdr_iter->p_filesz/sizeof(Elf32_Dyn),
                                 &dyninfo->dyn_strtable_header,has_needed);
     if __unlikely(KE_ISERR(error)) goto err_dhdr;
    }
    free(dhdr);
    break;
   case PT_NOTE:
   case PT_PHDR:
    break;
   default:
    if (phdr_iter->p_type == 3) break; // Something about interpreters, that wouldn't even work right now...
    k_syslogf_prefixfile(KLOG_WARN,elf_file
                    ,"[ELF] Unrecognized program header: %d\n"
                    ,(int)phdr_iter->p_type);
    break;
  }
  free(phdr);
 } else {
  self->sh_data.ed_secc = 0;
  self->sh_data.ed_secv = NULL;
 }
 /* Calculate the min/max section address pair. */
 ksecdata_cacheminmax(&self->sh_data);
 return KE_OK;
err_data: ksecdata_quit(&self->sh_data);
err_deps: kshliblist_quit(&self->sh_deps);
 return error;
}


// Returns 'KS_FOUND' if the given 'data' is inherited
__local kerrno_t krelocvec_elf32_init(struct krelocvec *__restrict self,
                                      Elf32_Rel const *data, size_t entsize,
                                      size_t entcount, int is_rela) {
 size_t vecsize,reqentsize,copysize;
 uintptr_t iter,end;
 kassertobj(self);
 assert(entsize != 0 && entcount != 0);
 reqentsize = is_rela ? sizeof(Elf32_Rela) : sizeof(Elf32_Rel);
 self->rv_type = is_rela ? KRELOCVEC_TYPE_ELF32_RELA : KRELOCVEC_TYPE_ELF32_REL;
 self->rv_relc = entcount;
 if (reqentsize == entsize) {
  // Inherit data directly
  self->rv_data = (void *)data;
  return KS_FOUND;
 }
 vecsize = entcount*reqentsize;
 self->rv_data = malloc(vecsize);
 if __unlikely(!self->rv_data) return KE_NOMEM;
 self->rv_relc = entcount;
 copysize = min(entsize,reqentsize);
 end = (iter = (uintptr_t)self->rv_data)+vecsize;
 for (; iter != end; iter += reqentsize,*(uintptr_t *)&data += entsize) {
  memset((void *)iter,0,reqentsize);
  memcpy((void *)iter,data,copysize);
 }
 return KE_OK;
}

__local kerrno_t
kshlib_elf32_load_rel(struct kshlib *__restrict self,
                      struct kfile *__restrict elf_file,
                      Elf32_Shdr const *__restrict shdr,
                      int is_rela
#ifdef __DEBUG__
                      , char const *__restrict name
#endif
                      ) {
 Elf32_Rel *rel_data;
 struct krelocvec *newreloc;
 size_t relsize;
 kerrno_t error;
 kassert_kshlib(self);
 kassert_kfile(elf_file);
 relsize = (size_t)shdr->sh_size;
 if __unlikely(!relsize || !shdr->sh_entsize) {
  /* Empty section */
#ifdef __DEBUG__
  k_syslogf_prefixfile(KLOG_WARN,elf_file
                      ,"[ELF] Empty ELF REL%s section: %s\n"
                      ,is_rela ? "A" : "",name);
#endif
  return KE_OK;
 }
 rel_data = (Elf32_Rel *)malloc(relsize);
 if __unlikely(!rel_data) return KE_NOMEM;
 /* Extract relocation information from the binary. */
 error = kfile_kernel_preadall(elf_file,shdr->sh_offset,
                               rel_data,relsize);
 if __unlikely(KE_ISERR(error)) goto end_reldata;
 newreloc = (struct krelocvec *)realloc(self->sh_reloc.r_vecv,
                                       (self->sh_reloc.r_vecc+1)*
                                        sizeof(struct krelocvec));
 if __unlikely(!newreloc) { error = KE_NOMEM; goto end_reldata; }
 self->sh_reloc.r_vecv = newreloc;
 newreloc += self->sh_reloc.r_vecc++;
 error = krelocvec_elf32_init(newreloc,rel_data,shdr->sh_entsize,
                              relsize/shdr->sh_entsize,is_rela);
 if (error == KS_FOUND) error = KE_OK;
 else {
  if __unlikely(KE_ISERR(error)) {
   newreloc = (struct krelocvec *)realloc(self->sh_reloc.r_vecv,
                                         (--self->sh_reloc.r_vecc)*
                                          sizeof(struct krelocvec));
   if (newreloc) self->sh_reloc.r_vecv = newreloc;
  }
end_reldata:
  free(rel_data);
 }
 return error;
}


__local kerrno_t
kshlib_elf32_load_sheaders(struct kshlib *__restrict self,
                           struct elf32_dynsyminfo const *__restrict dyninfo,
                           struct kfile *__restrict elf_file,
                           Elf32_Ehdr const *__restrict ehdr) {
 kerrno_t error;
 Elf32_Shdr *shdr,*shdr_iter,*shdr_end; size_t shsize;
 Elf32_Shdr *sh_dynstr,*sh_dynsym;
 Elf32_Shdr *sh_strtab,*sh_symtab;
 Elf32_Addr  sh_strtable_addr;
 Elf32_Word  sh_strtable_size;
 char *strtable;
 if (!ehdr->e_shnum) return KE_OK; // No sections headers found
 sh_dynstr = sh_dynsym = sh_strtab = sh_symtab = NULL;

 assert(ehdr->e_shnum);
 shsize = (size_t)ehdr->e_shnum*ehdr->e_shentsize;
 shdr = (Elf32_Shdr *)malloc(shsize);
 if __unlikely(!shdr) return KE_NOMEM;
 error = kfile_kernel_preadall(elf_file,ehdr->e_shoff,shdr,shsize);
 if __unlikely(KE_ISERR(error)) goto end_sheaders;
 // String table for section names
 if (ehdr->e_shstrndx < ehdr->e_shnum &&
     ehdr->e_shentsize >= offsetafter(Elf32_Shdr,sh_size)) {
  Elf32_Shdr *strt = (Elf32_Shdr *)((uintptr_t)shdr+
                     ((size_t)ehdr->e_shstrndx*ehdr->e_shentsize));
  sh_strtable_addr = strt->sh_offset;
  sh_strtable_size = strt->sh_size;
 } else {
  sh_strtable_addr = dyninfo->dyn_strtable_header.sh_offset;
  sh_strtable_size = dyninfo->dyn_strtable_header.sh_size;
 }
 if __unlikely(!sh_strtable_addr) {
  sh_strtable_size = 0;
  strtable         = NULL;
 } else {
  strtable = (char *)malloc(sh_strtable_size+sizeof(char));
  if __unlikely(!strtable) { error = KE_NOMEM; goto end_sheaders; }
  error = kfile_kernel_preadall(elf_file,sh_strtable_addr,strtable,sh_strtable_size);
  if __unlikely(KE_ISERR(error)) {err_strtab_data: free(strtable); goto end_sheaders; }
  strtable[sh_strtable_size/sizeof(char)] = '\0';
 }

#define STRAT(addr) ((addr) >= sh_strtable_size ? NULL : strtable+(addr))
 shdr_end = (Elf32_Shdr *)((uintptr_t)(shdr_iter = shdr)+shsize);
 for (; shdr_iter != shdr_end; *(uintptr_t *)&shdr_iter += ehdr->e_shentsize) {
  int hrdtype = shdr_iter->sh_type;
  char const *__restrict name = STRAT(shdr_iter->sh_name);
  k_syslogf_prefixfile(KLOG_TRACE,elf_file,"[ELF] %.8p %8I64x %6I64d %3d %s\n",
                      (uintptr_t)shdr_iter->sh_addr,
                      (__u64)shdr_iter->sh_offset,
                      (__u64)shdr_iter->sh_size,
                       hrdtype,name);
  switch (hrdtype) {
   case SHT_NULL: break; // Just ignore this one...
   case SHT_PROGBITS:
    /* Parse DWARF .debug_line debug sections. */
    if (!strcmp(name,".debug_line")) {
     /* Ignore errors if this fails. */
     kshlib_a2l_add_dwarf_debug_line(self,
                                    (__u64)shdr_iter->sh_offset,
                                    (__u64)shdr_iter->sh_size);
    }
    break; /* Used for debug information and such... */
   case SHT_NOBITS: break; /* For bss sections (Already parsed by program headers, right?) */
   case SHT_HASH: break; /* We do our own hashing */
   case SHT_STRTAB:
    if (name) {
          if (!strcmp(name,".dynstr")) sh_dynstr = shdr_iter;
     else if (!strcmp(name,".strtab")) sh_strtab = shdr_iter;
    }
    break;
   case SHT_SYMTAB: sh_symtab = shdr_iter; break;
   case SHT_DYNSYM: sh_dynsym = shdr_iter; break;
   case SHT_DYNAMIC: break;
   case SHT_RELA:
   case SHT_REL:
    error = kshlib_elf32_load_rel(self,elf_file,shdr_iter,hrdtype == SHT_RELA
#ifdef __DEBUG__
                                  ,name
#endif
                                  );
    if __unlikely(KE_ISERR(error)) goto err_strtab_data;
    break;
   default:
    /* Don't warn about these... */
    if (hrdtype >= SHT_LOPROC && hrdtype <= SHT_HIPROC) break;
    if (hrdtype >= SHT_LOUSER && hrdtype <= SHT_HIUSER) break;
    k_syslogf_prefixfile(KLOG_WARN,elf_file
                        ,"[ELF] Unrecognized section header: %d (%q)\n"
                        ,hrdtype,name);
    break;
  }
 }
#undef STRAT
 free(strtable);

 // Default to use Program-header-based tables
 error = kshlib_elf32_load_symtable(self,sh_dynstr ? sh_dynstr : &dyninfo->dyn_strtable_header,
                                         sh_dynsym ? sh_dynsym : &dyninfo->dyn_symtable_header,
                                    elf_file,1);
 if __unlikely(KE_ISERR(error)) goto end_sheaders;
 // Load an optional symbol table
 if (sh_strtab && sh_symtab) {
  error = kshlib_elf32_load_symtable(self,sh_strtab,sh_symtab,elf_file,0);
  if __unlikely(KE_ISERR(error)) goto end_sheaders;
 }

end_sheaders: 
 free(shdr);
 return error;
}




__crit kerrno_t kshlib_elf32_new(__ref struct kshlib **result, struct kfile *__restrict elf_file) {
 Elf32_Ehdr ehdr; struct kshlib *lib; kerrno_t error;
 struct elf32_dynsyminfo dyninfo;
 KTASK_CRIT_MARK
 kassert_kfile(elf_file);
 if __unlikely((lib = omalloc(struct kshlib)) == NULL) return KE_NOMEM;
 kobject_init(lib,KOBJECT_MAGIC_SHLIB);
 lib->sh_refcnt = 1;
 lib->sh_flags = KMODFLAG_NONE|KMODKIND_ELF32;
 error = kfile_kernel_readall(elf_file,&ehdr,sizeof(ehdr));
 if __unlikely(KE_ISERR(error)) goto err_lib;
 if __unlikely(ehdr.ehrd_magic[0] != ELFMAG0 || ehdr.ehrd_magic[1] != ELFMAG1
            || ehdr.ehrd_magic[2] != ELFMAG2 || ehdr.ehrd_magic[3] != ELFMAG3
            ) { error = KE_NOEXEC; goto err_lib; }
 /* TODO: Is there some other (better conforming) way of detecting fixed-address images? */
 if (ehdr.e_type == ET_EXEC) {
  lib->sh_flags |= KMODFLAG_FIXED|KMODFLAG_BINARY|KMODFLAG_CANEXEC;
 }
 error = kfile_incref(lib->sh_file = elf_file);
 if __unlikely(KE_ISERR(error)) goto err_lib;
 error = kshlibcache_addlib(lib);
 /* If we can't cache the library, still allow it to be loaded,
  * but make sure that we mark the library as not cached through
  * use of an always out-of-bounds cache index. */
 if __unlikely(KE_ISERR(error)) {
  lib->sh_cidx = (__size_t)-1;
  k_syslogf_prefixfile(KLOG_WARN,elf_file
                      ,"[PE] Failed to cache library file\n");
 }

 dyninfo.dyn_strtable_header.sh_offset  = 0;
 dyninfo.dyn_strtable_header.sh_size    = (Elf32_Word)-1;
 dyninfo.dyn_symtable_header.sh_offset  = 0;
 dyninfo.dyn_symtable_header.sh_size    = (Elf32_Word)-1;
 dyninfo.dyn_symtable_header.sh_entsize = (Elf32_Word)-1;
 lib->sh_callbacks.slc_start            = (ksymaddr_t)ehdr.e_entry;
 lib->sh_callbacks.slc_init             = KSYM_INVALID;
 lib->sh_callbacks.slc_fini             = KSYM_INVALID;
 lib->sh_callbacks.slc_preinit_array_v  = KSYM_INVALID;
 lib->sh_callbacks.slc_preinit_array_c  = 0;
 lib->sh_callbacks.slc_init             = KSYM_INVALID;
 lib->sh_callbacks.slc_init_array_v     = KSYM_INVALID;
 lib->sh_callbacks.slc_init_array_c     = 0;
 lib->sh_callbacks.slc_fini             = KSYM_INVALID;
 lib->sh_callbacks.slc_fini_array_v     = KSYM_INVALID;
 lib->sh_callbacks.slc_fini_array_c     = 0;
 // Load program headers and collect dynamic information
 error = kshlib_elf32_load_pheaders(lib,&dyninfo,elf_file,&ehdr);
 if __unlikely(KE_ISERR(error)) goto err_cache;
 // Initialize symbol tables
 ksymtable_init(&lib->sh_publicsym);
 ksymtable_init(&lib->sh_weaksym);
 ksymtable_init(&lib->sh_privatesym);
 kaddr2linelist_init(&lib->sh_addr2line);
 kreloc_init(&lib->sh_reloc);
 error = kshlib_elf32_load_sheaders(lib,&dyninfo,elf_file,&ehdr);
 if __unlikely(KE_ISERR(error)) goto err_post_pheaders;
 else {
  /* Without any relocations, the module must be loaded as fixed. */
  //if (!lib->sh_reloc.r_vecc) lib->sh_flags |= KMODFLAG_FIXED;
  kshlibrecent_add(lib);
 }
 lib->sh_flags |= KMODFLAG_LOADED;
 *result = lib;
 return error;
err_post_pheaders:
 kreloc_quit(&lib->sh_reloc);
 kaddr2linelist_quit(&lib->sh_addr2line);
 ksymtable_quit(&lib->sh_privatesym);
 ksymtable_quit(&lib->sh_weaksym);
 ksymtable_quit(&lib->sh_publicsym);
 ksecdata_quit(&lib->sh_data);
 kshliblist_quit(&lib->sh_deps);
err_cache: kshlibcache_dellib(lib);
/*err_file:*/ kfile_decref(lib->sh_file);
err_lib: free(lib);
 return error;
}

__DECL_END

#endif /* !__KOS_KERNEL_LINKER_ELF32_C_INL__ */

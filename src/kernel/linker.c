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
#ifndef __KOS_KERNEL_LINKER_C__
#define __KOS_KERNEL_LINKER_C__ 1

#include <assert.h>
#include <elf.h>
#include <kos/compiler.h>
#include <kos/kernel/debug.h>
#include <kos/kernel/fs/file.h>
#include <kos/kernel/linker.h>
#include <kos/syslog.h>
#include <kos/kernel/shm.h>
#include <malloc.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <kos/kernel/proc.h>
#include <kos/kernel/paging.h>
#include <stdlib.h>
#include <kos/kernel/fs/fs.h>

__DECL_BEGIN

ksymhash_t
ksymhash_of(char const *__restrict text,
            size_t size) {
 // Taken from my other project TPP: 'tpp.c', '_TPPKeywordList_HashOfL'
 char const *end = text+size;
 ksymhash_t result = 1;
 kassertmem(text,size);
 for (; text != end; ++text) 
  result = result*263+*text;
 return result;
}

__local struct ksymbol *
ksymbol_new(char const *__restrict name) {
 struct ksymbol *result;
 size_t name_size = strlen(name);
 result = (struct ksymbol *)malloc(offsetof(struct ksymbol,s_name)+
                                  (name_size+1)*sizeof(char));
 if __likely(result) {
  result->s_hash = ksymhash_of(name,name_size);
  result->s_nmsz = name_size;
  memcpy(result->s_name,name,name_size*sizeof(char));
  result->s_name[name_size] = '\0';
 }
 return result;
}
#define ksymbol_delete   free



void ksymtable_quit(struct ksymtable *self) {
 struct ksymtablebucket *biter,*bend;
 struct ksymbol *iter,*next;
 kassert_ksymtable(self);
 bend = (biter = self->st_mapv)+self->st_mapa;
 for (; biter != bend; ++biter) {
  if ((iter = biter->b_lname) != NULL) do {
   next = iter->s_nextname;
   free(iter);
  } while ((iter = next) != NULL);
 }
 free(self->st_mapv);
}


struct ksymbol const *
ksymtable_lookupname_h(struct ksymtable const *self, char const *symname,
                       size_t symnamelength, ksymhash_t hash) {
 struct ksymbol const *iter;
 kassert_ksymtable(self);
 if __unlikely(!self->st_mapa) return NULL;
 iter = self->st_mapv[ksymtable_modnamehash(self,hash)].b_lname;
 for (; iter; iter = iter->s_nextname) {
  if (iter->s_hash == hash &&
      iter->s_nmsz == symnamelength &&
      memcmp(iter->s_name,symname,symnamelength*sizeof(char)) == 0
      ) return iter; // Found it!
  
 }
 return NULL;
}

struct ksymbol const *
ksymtable_lookupaddr_single(struct ksymtable const *self,
                            ksymaddr_t addr) {
 struct ksymbol const *iter;
 kassert_ksymtable(self);
 if __unlikely(!self->st_mapa) return NULL;
 iter = self->st_mapv[ksymtable_modaddrhash(self,addr)].b_laddr;
 for (; iter; iter = iter->s_nextaddr) {
  if (iter->s_addr == addr) return iter; // Found it!
 }
 return NULL;
}


struct ksymbol const *
ksymtable_lookupaddr(struct ksymtable const *self, ksymaddr_t addr) {
 struct ksymbol const *result;
 while ((result = ksymtable_lookupaddr_single(self,addr)) == NULL && addr--);
 return result;
}


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
kerrno_t ksymtable_insert_inherited(struct ksymtable *self,
                                    struct ksymbol *sym) {
 size_t newsize;
 struct ksymtablebucket *bucket;
 kassert_ksymtable(self);
 kassertobj(sym);
 assert(ksymhash_of(sym->s_name,sym->s_nmsz) == sym->s_hash);
 assert((self->st_mapa != 0) == (self->st_mapv != NULL));
#if 0
 if (self->st_mapv) {
  newslot = self->st_mapv[ksymtable_modnamehash(self,sym->s_hash)].b_lname;
  while (newslot) {
   if (newslot->s_hash == sym->s_hash &&
       newslot->s_size == sym->s_size &&
       newslot->s_addr == sym->s_addr && /*< Should this really be in here? */
       memcmp(newslot->s_name,sym->s_name,sym->s_size) == 0
       ) return KS_UNCHANGED;
   newslot = newslot->s_nextname;
  }
 }
#endif
 newsize = self->st_size+1;
 if (self->st_mapa < newsize && KE_ISERR(ksymtable_rehash(self,newsize)) &&
    !self->st_mapa) return KE_NOMEM; // Failed to allocate the initial bucket
 assert(self->st_mapv);
 bucket = self->st_mapv+ksymtable_modnamehash(self,sym->s_hash);
 sym->s_nextname = bucket->b_lname;
 bucket->b_lname = sym;
 bucket = self->st_mapv+ksymtable_modaddrhash(self,sym->s_addr);
 sym->s_nextaddr = bucket->b_laddr;
 bucket->b_laddr = sym;
 assert(self->st_size == newsize-1);
 self->st_size = newsize;
 return KE_OK;
}


kerrno_t ksymtable_rehash(struct ksymtable *self,
                          size_t bucketcount) {
 struct ksymtablebucket *newvec;
 kassert_ksymtable(self);
 assert((self->st_mapa != 0) == (self->st_mapv != NULL));
 assertf(bucketcount != 0,"Invalid bucket count");
 // Allocate the new bucket vector (Use calloc to already initialize it to ZERO; aka. NULL)
 newvec = (struct ksymtablebucket *)calloc(bucketcount,
                                           sizeof(struct ksymtablebucket));
 if __unlikely(!newvec) return KE_NOMEM;
 if (self->st_mapa) {
  struct ksymtablebucket *biter,*bend,*dest;
  struct ksymbol *niter,*nnext;
  struct ksymbol *aiter,*anext;
  // The symbol table already has some entires (Must re-hash those)
  bend = (biter = self->st_mapv)+self->st_mapa;
  for (; biter != bend; ++biter) {
   if ((niter = biter->b_lname) != NULL) do {
    nnext = niter->s_nextname;
    // Insert the entry into its newly modulated bucket within the new bucket vector
    dest = newvec+ksymtable_modnamehash2(niter->s_hash,
                                         bucketcount);
    niter->s_nextname = dest->b_lname;
    dest->b_lname = niter;
   } while ((niter = nnext) != NULL);
   if ((aiter = biter->b_laddr) != NULL) do {
    anext = aiter->s_nextaddr;
    // Insert the entry into its newly modulated bucket within the new bucket vector
    dest = newvec+ksymtable_modaddrhash2(aiter->s_addr,
                                         bucketcount);
    aiter->s_nextaddr = dest->b_laddr;
    dest->b_laddr = aiter;
   } while ((aiter = anext) != NULL);
  }
  // Free the old bucket vector
  free(self->st_mapv);
 }
 self->st_mapv = newvec;
 self->st_mapa = bucketcount;
 return KE_OK;
}


__local kerrno_t kshmtab_loaddata(struct kshmtab *self, size_t filesz,
                                  uintptr_t offset, struct kfile *fp);
__local kerrno_t kshmtab_ploaddata(struct kshmtab *self, uintptr_t offset, pos_t pos,
                                   size_t filesz, struct kfile *fp) {
 kerrno_t error;
 if __unlikely(KE_ISERR(error = kfile_seek(fp,pos,SEEK_SET,NULL))) return error;
 return kshmtab_loaddata(self,filesz,offset,fp);
}
__local kerrno_t kshmtab_loaddata(struct kshmtab *self, size_t filesz,
                                  uintptr_t offset, struct kfile *fp) {
 size_t maxbytes,copysize;
 __kernel void *addr; kerrno_t error;
 while (filesz) {
  addr = kshmtab_translate_offset(self,offset,&maxbytes);
  copysize = min(maxbytes,filesz);
  offset += copysize,filesz -= copysize;
  error = kfile_readall(fp,addr,copysize);
  if __unlikely(KE_ISERR(error)) return error;
  if __unlikely(!filesz) break;
 }
 // Fill the rest with ZEROs
 while ((addr = kshmtab_translate_offset(self,offset,&maxbytes)) != NULL) {
  memset(addr,0,maxbytes);
  offset += maxbytes;
 }
 return KE_OK;
}


kerrno_t ksecdata_init(struct ksecdata *__restrict self,
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
   struct kshmtab *section_tab; size_t memsize,filesize;
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
                       ,"Loading segment from %.8I64u to %.8p+%.8Iu...%.8p (%c%c%c) (align: %Iu)\n"
                       ,(__u64)header->p_offset
                       ,(uintptr_t)header->p_vaddr,(size_t)header->p_memsz
                       ,(uintptr_t)header->p_vaddr+(size_t)header->p_memsz
                       ,(header->p_flags&PF_R) ? 'R' : '-'
                       ,(header->p_flags&PF_W) ? 'W' : '-'
                       ,(header->p_flags&PF_X) ? 'X' : '-'
                       ,(uintptr_t)header->p_align);
   section_tab = kshmtab_newram(ceildiv(memsize,PAGESIZE),
                               ((header->p_flags&PF_X) ? PROT_EXEC  : 0)|
#if 1 // TODO: Without this we get #PFs (Why?)
                               PROT_WRITE|
#else
                               ((header->p_flags&PF_W) ? PROT_WRITE : KSHMTAB_FLAG_S)|
#endif
                               ((header->p_flags&PF_R) ? PROT_READ  : 0));
   if __unlikely(!section_tab) {
    error = KE_NOMEM;
err_seciter:
    while (seciter != sections) {
     --seciter;
     kshmtab_decref(seciter->sls_tab);
     free(sections);
    }
    return error;
   }
   seciter->sls_tab = section_tab;
   seciter->sls_size = memsize;
   seciter->sls_base = (ksymaddr_t)header->p_vaddr;
   seciter->sls_albase = alignd(seciter->sls_base,PAGEALIGN);
   filesize = header->p_filesz;
   if __unlikely(filesize > memsize) filesize = memsize;
   seciter->sls_filebase = (ksymaddr_t)header->p_offset;
   seciter->sls_filesize = filesize;
   error = kshmtab_ploaddata(section_tab,
                             seciter->sls_base-seciter->sls_albase,
                             header->p_offset,filesize,elf_file);
   if __unlikely(KE_ISERR(error)) {
    kshmtab_decref(section_tab);
    goto err_seciter;
   }
   ++seciter;
  }
 }
 assert(seciter == self->ed_secv+self->ed_secc);
 return KE_OK;
}

void ksecdata_quit(struct ksecdata *self) {
 struct kshlibsection *iter,*end;
 end = (iter = self->ed_secv)+self->ed_secc;
 for (; iter != end; ++iter) kshmtab_decref(iter->sls_tab);
 free(self->ed_secv);
}



__kernel void const *
ksecdata_translate_ro(struct ksecdata const *__restrict self,
                      ksymaddr_t addr, size_t *__restrict maxsize) {
 struct kshlibsection const *iter,*end;
 end = (iter = self->ed_secv)+self->ed_secc;
 for (; iter != end; ++iter) {
  if (addr >= iter->sls_base
  &&  addr < iter->sls_base+iter->sls_size
#if 0
  && (iter->sls_tab->mt_flags&KSHMTAB_FLAG_R)
#endif
      ) {
   return kshmtab_translate_offset(iter->sls_tab,addr-iter->sls_albase,maxsize);
  }
 }
 return NULL;
}
__kernel void *
ksecdata_translate_rw(struct ksecdata *__restrict self,
                      ksymaddr_t addr, size_t *__restrict maxsize) {
 struct kshlibsection const *iter,*end;
 end = (iter = self->ed_secv)+self->ed_secc;
 for (; iter != end; ++iter) {
  if (addr >= iter->sls_base
  &&  addr < iter->sls_base+iter->sls_size
  && (iter->sls_tab->mt_flags&KSHMTAB_FLAG_W)) {
   return kshmtab_translate_offset(iter->sls_tab,addr-iter->sls_albase,maxsize);
  }
 }
 return NULL;
}


size_t ksecdata_getmem(struct ksecdata const *__restrict self, ksymaddr_t addr,
                       void *__restrict buf, size_t bufsize) {
 void const *shptr; size_t result = 0,maxbytes,copysize;
 kassertobj(self);
 while ((shptr = ksecdata_translate_ro(self,addr,&maxbytes)) != NULL) {
  copysize = min(maxbytes,bufsize);
  memcpy(buf,shptr,copysize);
  result             += copysize;
  if (copysize == bufsize) break;
  bufsize            -= copysize;
  *(uintptr_t *)&buf += copysize;
 }
 return result;
}
size_t ksecdata_setmem(struct ksecdata *self, ksymaddr_t addr,
                       void const *__restrict buf, size_t bufsize) {
 void *shptr; size_t result = 0,maxbytes,copysize;
 kassertobj(self);
 while ((shptr = ksecdata_translate_rw(self,addr,&maxbytes)) != NULL) {
  copysize = min(maxbytes,bufsize);
  memcpy(shptr,buf,copysize);
  result             += copysize;
  if (copysize == bufsize) break;
  bufsize            -= copysize;
  *(uintptr_t *)&buf += copysize;
 }
 return result;
}


kerrno_t kshlib_load_elf32_symtable(struct kshlib *self, Elf32_Shdr const *strtab,
                                    Elf32_Shdr const *symtab, struct kfile *elf_file,
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
  error = kfile_preadall(elf_file,strtab->sh_offset,
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
 error = kfile_preadall(elf_file,symtab->sh_offset,symtable,symtable_size);
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
  self->sh_reloc.r_syms.sl_symc = symcount;
  self->sh_reloc.r_syms.sl_symv = dynsym;
 }
 for (sym_iter = symtable; sym_iter < sym_end; *(uintptr_t *)&sym_iter += symtab->sh_entsize) {
  if ((name = STRAT(sym_iter->st_name)) != NULL && *name != '\0') {
   unsigned char bind = ELF32_ST_BIND(sym_iter->st_info);
   struct ksymbol *sym; struct ksymtable *symtbl;
        if (bind == STB_LOCAL) symtbl = &self->sh_privatesym;
   else if (bind == STB_GLOBAL) symtbl = &self->sh_publicsym;
   else if (bind == STB_WEAK) symtbl = &self->sh_weaksym;
   else goto sym_next;
   k_syslogf_prefixfile(KLOG_INSANE,elf_file,"SYMBOL: %s %s: %p\n",
                    bind == STB_GLOBAL ? "__global" :
                    bind == STB_LOCAL ? "__local" :
                    bind == STB_WEAK ? "__weak" : "",
                    name,(uintptr_t)sym_iter->st_value);
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
       (dynsym == self->sh_reloc.r_syms.sl_symv+
                  self->sh_reloc.r_syms.sl_symc));
end_symtable:
 free(symtable);
#undef STRAT
end_strtable:
 free(strtable);
 return error;
}


ksymaddr_t kshlib_fileaddr2symaddr(struct kshlib const *__restrict self,
                                   ksymaddr_t fileaddr) {
 struct kshlibsection const *iter,*end;
 end = (iter = self->sh_data.ed_secv)+self->sh_data.ed_secc;
 for (; iter != end; ++iter) {
  if (fileaddr >= iter->sls_filebase &&
      fileaddr < iter->sls_filebase+iter->sls_filesize
      ) return iter->sls_base+(fileaddr-iter->sls_filebase);
 }
 return KSYM_INVALID;
}
ksymaddr_t kshlib_symaddr2fileaddr(struct kshlib const *__restrict self,
                                   ksymaddr_t symaddr) {
 struct kshlibsection const *iter,*end;
 end = (iter = self->sh_data.ed_secv)+self->sh_data.ed_secc;
 for (; iter != end; ++iter) {
  if (symaddr >= iter->sls_base &&
      symaddr < iter->sls_base+iter->sls_filesize
      ) return iter->sls_filebase+(symaddr-iter->sls_base);
 }
 return KSYM_INVALID;
}


void kreloc_quit(struct kreloc *__restrict self) {
 struct krelocvec *iter,*end;
 ksymlist_quit(&self->r_syms);
 end = (iter = self->r_vecv)+self->r_vecc;
 for (; iter != end; ++iter) free(iter->rv_data);
 free(self->r_vecv);
}



void kshliblist_quit(struct kshliblist *__restrict self) {
 struct kshlib **iter,**end;
 end = (iter = self->sl_libv)+self->sl_libc;
 for (; iter != end; ++iter) kshlib_decref(*iter);
 free(self->sl_libv);
}







__local kerrno_t
kshlib_parse_needed(struct kshlib *self, struct kfile *elf_file,
                    Elf32_Dyn const *dynheader, size_t dynheader_c,
                    Elf32_Shdr const *strtable_header, size_t needed_count) {
 Elf32_Dyn const *iter,*end; char *strtable; kerrno_t error;
 size_t strtable_size = strtable_header->sh_size;
 __ref struct kshlib **dependencies,**dep_iter,*dep;
 strtable = (char *)malloc(strtable_size+sizeof(char));
 if __unlikely(!strtable) return KE_NOMEM;
 error = kfile_preadall(elf_file,strtable_header->sh_offset,
                        strtable,strtable_size);
 if __unlikely(KE_ISERR(error)) {
  k_syslogf_prefixfile(KLOG_ERROR,elf_file,"Failed to load DT_NEEDED string table from %I64u|%Iu\n",
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
    k_syslogf_prefixfile(KLOG_INFO,elf_file,"Searching for dependency: %s\n",name);
    // Recursively open dependencies
    if __unlikely(KE_ISERR(error = kshlib_open(name,(size_t)-1,&dep))) {
     k_syslogf_prefixfile(KLOG_ERROR,elf_file,"Missing dependency: %s\n",name);
     if (!KSHLIB_IGNORE_MISSING_DEPENDENCIES) {
      if (error == KE_NOENT) error = KE_NODEP;
err_depiter:
      while (dep_iter != dependencies) kshlib_decref(*--dep_iter);
      free(dependencies);
      goto end_strtable;
     } else {
      --self->sh_deps.sl_libc;
     }
    } else if (!(dep->sh_flags&KSHLIB_FLAG_LOADED)) {
     k_syslogf_prefixfile(KLOG_ERROR,elf_file,"Dependency loop: %s\n",name);
     // Dependency loop
     error = KE_LOOP;
     kshlib_decref(*dep_iter);
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

struct dynsyminfo {
 Elf32_Shdr dyn_strtable_header;
 Elf32_Shdr dyn_symtable_header;
};

// Initialize 'sh_data' + 'sh_deps' and fill 'dyninfo'
__local kerrno_t
kshlib_load_elf32_pheaders(struct kshlib *self,
                           struct dynsyminfo *dyninfo,
                           struct kfile *elf_file,
                           Elf32_Ehdr const *ehdr) {
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
  error = kfile_preadall(elf_file,ehdr->e_phoff,phdr,phsize);
  if __unlikely(KE_ISERR(error)) {__err_pheaders: free(phdr); goto err_deps; }
  error = ksecdata_init(&self->sh_data,phdr,ehdr->e_phnum,ehdr->e_phentsize,elf_file);
  if __unlikely(KE_ISERR(error)) goto __err_pheaders;
  phdr_end = (Elf32_Phdr *)((uintptr_t)(phdr_iter = phdr)+phsize);
  // Iterate all segments
  for (; phdr_iter != phdr_end;
       *(uintptr_t *)&phdr_iter += ehdr->e_phentsize
       ) switch (phdr_iter->p_type) {
   case PT_LOAD: break; // Already handled inside of 'ksecdata_init'
   case PT_DYNAMIC: // Dynamic header
    dhdr = (Elf32_Dyn *)malloc(phdr_iter->p_filesz);
    if __unlikely(!dhdr) { error = KE_NOMEM; err_pheaders: free(phdr); goto err_data; }
    error = kfile_preadall(elf_file,phdr_iter->p_offset,dhdr,
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
       &dyninfo->dyn_symtable_header.sh_size,sizeof(Elf32_Word)) != sizeof(Elf32_Word)
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
      k_syslogf_prefixfile(KLOG_WARN,elf_file,"Unrecognized dynamic header: %d\n",(int)diter->d_tag);
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
     error = kshlib_parse_needed(self,elf_file,dhdr,
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
                    ,"Unrecognized program header: %d\n"
                    ,(int)phdr_iter->p_type);
    break;
  }
  free(phdr);
 } else {
  self->sh_data.ed_secc = 0;
  self->sh_data.ed_secv = NULL;
 }
 return KE_OK;
err_data: ksecdata_quit(&self->sh_data);
err_deps: kshliblist_quit(&self->sh_deps);
 return error;
}


// Returns 'KS_FOUND' if the given 'data' is inherited
__local kerrno_t krelocvec_init_elf32(struct krelocvec *self,
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
kshlib_load_elf32_rel(struct kshlib *self,
                      struct kfile *elf_file,
                      Elf32_Shdr const *shdr,
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
  // Empty section
  k_syslogf_prefixfile(KLOG_WARN,elf_file
                  ,"Empty ELF REL%s section: %s\n"
                  ,is_rela ? "A" : "",name);
  return KE_OK;
 }
 rel_data = (Elf32_Rel *)malloc(relsize);
 if __unlikely(!rel_data) return KE_NOMEM;
 error = kfile_preadall(elf_file,shdr->sh_offset,
                        rel_data,relsize);
 if __unlikely(KE_ISERR(error)) goto end_reldata;
 newreloc = (struct krelocvec *)realloc(self->sh_reloc.r_vecv,
                                       (self->sh_reloc.r_vecc+1)*
                                        sizeof(struct krelocvec));
 if __unlikely(!newreloc) { error = KE_NOMEM; goto end_reldata; }
 self->sh_reloc.r_vecv = newreloc;
 newreloc += self->sh_reloc.r_vecc++;
 error = krelocvec_init_elf32(newreloc,rel_data,shdr->sh_entsize,
                              relsize/shdr->sh_entsize,is_rela);
 if (error == KS_FOUND) error = KE_OK;
 else {
  newreloc = (struct krelocvec *)realloc(self->sh_reloc.r_vecv,
                                        (--self->sh_reloc.r_vecc)*
                                         sizeof(struct krelocvec));
  if (newreloc) self->sh_reloc.r_vecv = newreloc;
end_reldata:
  free(rel_data);
 }
 return error;
}


__local kerrno_t
kshlib_load_elf32_sheaders(struct kshlib *self,
                           struct dynsyminfo const *dyninfo,
                           struct kfile *elf_file,
                           Elf32_Ehdr const *ehdr) {
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
 error = kfile_preadall(elf_file,ehdr->e_shoff,shdr,shsize);
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
  error = kfile_preadall(elf_file,sh_strtable_addr,strtable,sh_strtable_size);
  if __unlikely(KE_ISERR(error)) {err_strtab_data: free(strtable); goto end_sheaders; }
  strtable[sh_strtable_size/sizeof(char)] = '\0';
 }

#define STRAT(addr) ((addr) >= sh_strtable_size ? NULL : strtable+(addr))
 shdr_end = (Elf32_Shdr *)((uintptr_t)(shdr_iter = shdr)+shsize);
 for (; shdr_iter != shdr_end; *(uintptr_t *)&shdr_iter += ehdr->e_shentsize) {
  int hrdtype = shdr_iter->sh_type;
  char const *__restrict name = STRAT(shdr_iter->sh_name);
  k_syslogf_prefixfile(KLOG_TRACE,elf_file," %.8p %8I64x %6I64d %3d %s\n"
                 ,(uintptr_t)shdr_iter->sh_addr
                 ,(__u64)shdr_iter->sh_offset
                 ,(__u64)shdr_iter->sh_size
                 ,hrdtype,name);
  switch (hrdtype) {
   case SHT_NULL: break; // Just ignore this one...
   case SHT_PROGBITS: break; // Used for debug information and such...
   case SHT_NOBITS: break; // For bss sections (Already parsed by program headers, right?)
   case SHT_HASH: break; // We do our own hashing
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
    error = kshlib_load_elf32_rel(self,elf_file,shdr_iter,hrdtype == SHT_RELA
#ifdef __DEBUG__
                                  ,name
#endif
                                  );
    if __unlikely(KE_ISERR(error)) goto err_strtab_data;
    break;
   default:
    // Don't warn about these...
    if (hrdtype >= SHT_LOPROC && hrdtype <= SHT_HIPROC) break;
    if (hrdtype >= SHT_LOUSER && hrdtype <= SHT_HIUSER) break;
    k_syslogf_prefixfile(KLOG_WARN,elf_file
              ,"Unrecognized section header: %d (%q)\n"
              ,hrdtype,name);
    break;
  }
 }
#undef STRAT
 free(strtable);

 // Default to use Program-header-based tables
 error = kshlib_load_elf32_symtable(self,sh_dynstr ? sh_dynstr : &dyninfo->dyn_strtable_header,
                                         sh_dynsym ? sh_dynsym : &dyninfo->dyn_symtable_header,
                                    elf_file,1);
 if __unlikely(KE_ISERR(error)) goto end_sheaders;
 // Load an optional symbol table
 if (sh_strtab && sh_symtab) {
  error = kshlib_load_elf32_symtable(self,sh_strtab,sh_symtab,elf_file,0);
  if __unlikely(KE_ISERR(error)) goto end_sheaders;
 }

end_sheaders: 
 free(shdr);
 return error;
}




__crit kerrno_t kshlib_new_elf32(__ref struct kshlib **result, struct kfile *elf_file) {
 Elf32_Ehdr ehdr; struct kshlib *lib; kerrno_t error;
 struct dynsyminfo dyninfo;
 KTASK_CRIT_MARK
 kassert_kfile(elf_file);
 if __unlikely((lib = omalloc(struct kshlib)) == NULL) return KE_NOMEM;
 kobject_init(lib,KOBJECT_MAGIC_SHLIB);
 lib->sh_refcnt = 1;
 lib->sh_flags = KSHLIB_FLAG_NONE;
 error = kfile_readall(elf_file,&ehdr,sizeof(ehdr));
 if __unlikely(KE_ISERR(error)) goto err_lib;
 if __unlikely(ehdr.ehrd_magic[0] != ELFMAG0 || ehdr.ehrd_magic[1] != ELFMAG1
            || ehdr.ehrd_magic[2] != ELFMAG2 || ehdr.ehrd_magic[3] != ELFMAG3
            ) { error = KE_NOEXEC; goto err_lib; }
 // TODO: Is there some other (better conforming) way of detecting fixed-address images?
 if (ehdr.e_type == ET_EXEC) lib->sh_flags |= KSHLIB_FLAG_FIXED;
 error = kfile_incref(lib->sh_file = elf_file);
 if __unlikely(KE_ISERR(error)) goto err_lib;
 error = kshlibcache_addlib(lib);
 if __unlikely(KE_ISERR(error)) goto err_file;

 dyninfo.dyn_strtable_header.sh_offset  = 0;
 dyninfo.dyn_strtable_header.sh_size    = (Elf32_Word)-1;
 dyninfo.dyn_symtable_header.sh_offset  = 0;
 dyninfo.dyn_symtable_header.sh_size    = (Elf32_Word)-1;
 dyninfo.dyn_symtable_header.sh_entsize = (Elf32_Word)-1;
 if (ehdr.e_type == ET_EXEC) {
  lib->sh_callbacks.slc_start = (ksymaddr_t)ehdr.e_entry;
 } else {
  lib->sh_callbacks.slc_start = KSYM_INVALID;
 }
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
 // Load program headers and collect dynamic information
 error = kshlib_load_elf32_pheaders(lib,&dyninfo,elf_file,&ehdr);
 if __unlikely(KE_ISERR(error)) goto err_cache;
 // Initialize symbol tables
 ksymtable_init(&lib->sh_publicsym);
 ksymtable_init(&lib->sh_weaksym);
 ksymtable_init(&lib->sh_privatesym);
 kreloc_init(&lib->sh_reloc);
 error = kshlib_load_elf32_sheaders(lib,&dyninfo,elf_file,&ehdr);
 if __unlikely(KE_ISERR(error)) goto err_post_pheaders;

 lib->sh_flags |= KSHLIB_FLAG_LOADED;
 *result = lib;
 return error;
err_post_pheaders:
 kreloc_quit(&lib->sh_reloc);
 ksymtable_quit(&lib->sh_privatesym);
 ksymtable_quit(&lib->sh_weaksym);
 ksymtable_quit(&lib->sh_publicsym);
 ksecdata_quit(&lib->sh_data);
 kshliblist_quit(&lib->sh_deps);
err_cache: kshlibcache_dellib(lib);
err_file: kfile_decref(lib->sh_file);
err_lib: free(lib);
 return error;
}


void kshlib_destroy(struct kshlib *__restrict self) {
 kassert_object(self,KOBJECT_MAGIC_SHLIB);
 kshlibcache_dellib(self);
 kreloc_quit(&self->sh_reloc);
 ksecdata_quit(&self->sh_data);
 ksymtable_quit(&self->sh_privatesym);
 ksymtable_quit(&self->sh_weaksym);
 ksymtable_quit(&self->sh_publicsym);
 kshliblist_quit(&self->sh_deps);
 kfile_decref(self->sh_file);
 free(self);
}

__crit __ref struct ktask *
kshlib_spawn(struct kshlib const *__restrict self,
             struct kproc *__restrict proc) {
 struct ktask *result; void *useresp;
 KTASK_CRIT_MARK
 // Can't spawn main task in shared libraries
 if __unlikely(self->sh_callbacks.slc_start == KSYM_INVALID) return NULL;
 result = ktask_newuser(ktask_self(),proc,&useresp,
                        KTASK_STACK_SIZE_DEF,KTASK_STACK_SIZE_DEF);
 if __unlikely(!result) return NULL;
 ktask_setupuser(result,useresp,(void *)self->sh_callbacks.slc_start);
 return result;
}


struct kshmtab __omni_tab = {
 KOBJECT_INIT(KOBJECT_MAGIC_SHMTAB)
 /* mt_refcnt        */0xffff,
 /* mt_flags         */KSHMTAB_FLAG_R|KSHMTAB_FLAG_W|KSHMTAB_FLAG_X|KSHMTAB_FLAG_S|KSHMTAB_FLAG_K,
 /* mt_pages         */((size_t)-1)/PAGESIZE,
 /* mt_scat          */{
 /* mt_scat.ts_next  */NULL,
 /* mt_scat.ts_addr  */NULL,
 /* mt_scat.ts_pages */((size_t)-1)/PAGESIZE}
};
struct kshlibsection __omni_sections[] = {{
 /* sls_tab      */&__omni_tab,
 /* sls_albase   */0,
 /* sls_base     */0,
 /* sls_size     */(size_t)-1,
 /* sls_filebase */0,
 /* sls_filesize */(size_t)-1,
}};

static struct kinode *
get_root_inode(struct kfile *__restrict __unused(self)) {
 return kdirent_getnode(kfs_getroot());
}
static struct kdirent *
get_root_dirent(struct kfile *__restrict __unused(self)) {
 struct kdirent *result;
 if __unlikely(KE_ISERR(kdirent_incref(result = kfs_getroot()))) result = NULL;
 return result;
}

struct kfiletype kshlibkernel_file_type = {
 .ft_size      = sizeof(struct kfile),
 .ft_getinode  = &get_root_inode,
 .ft_getdirent = &get_root_dirent,
};
struct kfile kshlibkernel_file = {
 KOBJECT_INIT(KOBJECT_MAGIC_FILE)
 0xffff,
 &kshlibkernel_file_type,
};

extern void _start(void);
struct kshlib __kshlib_kernel = {
 KOBJECT_INIT(KOBJECT_MAGIC_SHLIB)
 /* sh_refcnt                        */0xffff,
 /* sh_file                          */&kshlibkernel_file,
 /* sh_flags                         */KSHLIB_FLAG_FIXED|KSHLIB_FLAG_LOADED,
 /* sh_cidx                          */(size_t)-1,
 /* sh_publicsym                     */KSYMTABLE_INIT,
 /* sh_weaksym                       */KSYMTABLE_INIT,
 /* sh_privatesym                    */KSYMTABLE_INIT, /*< TODO: Add all kernel symbols to this for debugging information. */
 /* sh_deps                          */KSHLIBLIST_INIT,
 /* sh_data                          */{
 /* sh_data.ed_secc                  */__compiler_ARRAYSIZE(__omni_sections),
 /* sh_data.ed_secv                  */__omni_sections},
 /* sh_reloc                         */{
 /* sh_reloc.r_vecc                  */0,
 /* sh_reloc.r_vecv                  */NULL,
 /* sh_reloc.r_syms                  */KSYMLIST_INIT},
 /* sh_callbacks                     */{
 /* sh_callbacks.slc_start           */(ksymaddr_t)(uintptr_t)&_start,
 /* sh_callbacks.slc_preinit_array_v */KSYM_INVALID,
 /* sh_callbacks.slc_preinit_array_c */0,
 /* sh_callbacks.slc_init            */KSYM_INVALID,
 /* sh_callbacks.slc_init_array_v    */KSYM_INVALID,
 /* sh_callbacks.slc_init_array_c    */0,
 /* sh_callbacks.slc_fini            */KSYM_INVALID,
 /* sh_callbacks.slc_fini_array_v    */KSYM_INVALID,
 /* sh_callbacks.slc_fini_array_c    */0}
};

__crit kerrno_t
kshlib_fopenfile(struct kfile *fp, __ref struct kshlib **__restrict result) {
 KTASK_CRIT_MARK
 // Check for a cached version of the library
 if ((*result = kshlibcache_fgetlib(fp)) != NULL) return KE_OK;
 return kshlib_new(result,fp);
}

__crit kerrno_t
kshlib_openfile(char const *__restrict filename, size_t filename_max,
                __ref struct kshlib **__restrict result) {
 struct kfile *fp; kerrno_t error;
 KTASK_CRIT_MARK
 // Check for a cached version of the library
 // TODO: We must ensure that the given path is absolute,
 //       as otherwise there would be a race condition allowing
 //       user applications to change their CWD while we are
 //       scanning for a library, potentially causing the
 //       kernel to load a library more than once into the
 //       same executable. (While technically not really a problem,
 //       I can't imagine the resulting confusion when some library
 //       uses a different 'errno' than your main application...)
 if ((*result = kshlibcache_getlib(filename)) != NULL) return KE_OK;
 if (KE_ISERR(error = krootfs_open(filename,filename_max,O_RDONLY,0,NULL,&fp))) return error;
 error = kshlib_new(result,fp);
 kfile_decref(fp);
 if (KE_ISOK(error)) {
  k_syslogf(KLOG_TRACE,"Loaded shlib: %.*q\n",
           (unsigned)filename_max,filename);
 }
 return error;
}

__crit kerrno_t
kshlib_open(char const *__restrict name,
            __size_t namemax,
            __ref struct kshlib **__restrict result) {
 char const *search_path;
 char const *iter,*next;
 kerrno_t error,last_error = KE_NOENT;
 size_t namesize = strnlen(name,namemax);
 KTASK_CRIT_MARK
 search_path = getenv("LD_LIBRARY_PATH");
 if (!search_path) search_path = "/usr/lib:/lib";
 iter = search_path;
 for (;;) {
  size_t partsize;
  next = strchr(iter,':');
  partsize = next ? next-iter : strlen(iter);
  if (partsize) {
   char *path;
   int hassep = KFS_ISSEP(iter[partsize-1]);
   if (!hassep) ++partsize;
   path = (char *)malloc((partsize+namesize+1)*sizeof(char));
   if __unlikely(!path) return KE_NOMEM;
   memcpy(path,iter,partsize*sizeof(char));
   if (!hassep) path[partsize-1] = KFS_SEP;
   memcpy(path+partsize,name,namesize*sizeof(char));
   path[partsize+namesize] = '\0';
   error = kshlib_openfile(path,(size_t)-1,result);
   free(path);
   // If we did manage to load the library, we've succeeded
   if (KE_ISOK(error)) return error;
   // Handle a hand full of errors specially
   switch (error) {
    case KE_NOENT: break;
    case KE_ACCES: last_error = KE_ACCES; break;
    case KE_NOEXEC:
    default:
     if (last_error == KE_OK) last_error = error;
     break;
   }
  }
  if (!next) break;
  iter = next+1;
 }
 return last_error;
}

__DECL_END

#ifndef __INTELLISENSE__
#include "procmodules.c.inl"
#include "linker-slloaders.c.inl"
#include "linker-relocate.c.inl"
#include "linker-cache.c.inl"
#endif

#endif /* !__KOS_KERNEL_LINKER_C__ */

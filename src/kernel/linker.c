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
#include <ctype.h>
#include <sys/stat.h>

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
  // The symbol table already has some entries (Must re-hash those)
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


__local kerrno_t kshmtab_loaddata(struct kshmregion *self, size_t filesz,
                                  uintptr_t offset, struct kfile *fp);
__local kerrno_t kshmtab_ploaddata(struct kshmregion *self, uintptr_t offset, pos_t pos,
                                   size_t filesz, struct kfile *fp) {
 kerrno_t error;
 if __unlikely(KE_ISERR(error = kfile_seek(fp,pos,SEEK_SET,NULL))) return error;
 return kshmtab_loaddata(self,filesz,offset,fp);
}
__local kerrno_t kshmtab_loaddata(struct kshmregion *self, size_t filesz,
                                  uintptr_t offset, struct kfile *fp) {
 size_t maxbytes,copysize;
 __kernel void *addr; kerrno_t error;
 while (filesz) {
  addr = kshmregion_getphysaddr_s(self,offset,&maxbytes);
  copysize = min(maxbytes,filesz);
  offset += copysize,filesz -= copysize;
  error = kfile_readall(fp,addr,copysize);
  if __unlikely(KE_ISERR(error)) return error;
  if __unlikely(!filesz) break;
 }
 /* Fill the rest with ZEROs */
 while ((addr = kshmregion_getphysaddr_s(self,offset,&maxbytes)) != NULL) {
  memset(addr,0x00,maxbytes);
  offset += maxbytes;
 }
 return KE_OK;
}

static void
ksecdata_cacheminmax(struct ksecdata *__restrict self) {
 struct kshlibsection *iter,*end;
 ksymaddr_t sec_min,sec_max,temp;
 kassertobj(self);
 end = (iter = self->ed_secv)+self->ed_secc;
 sec_min = (ksymaddr_t)-1,sec_max = 0;
 for (; iter != end; ++iter) {
  temp = iter->sls_base+iter->sls_size;
  if (iter->sls_albase < sec_min) sec_min = iter->sls_albase;
  if (temp > sec_max) sec_max = temp;
 }
 self->ed_begin = sec_min;
 self->ed_end = sec_max;
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
                       ,"Loading segment from %.8I64u to %.8p+%.8Iu...%.8p (%c%c%c) (align: %Iu)\n"
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
   error = kshmtab_ploaddata(section_region,
                             seciter->sls_base-seciter->sls_albase,
                             header->p_offset,filesize,elf_file);
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

void ksecdata_quit(struct ksecdata *self) {
 struct kshlibsection *iter,*end;
 end = (iter = self->ed_secv)+self->ed_secc;
 for (; iter != end; ++iter) kshmregion_decref_full(iter->sls_region);
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
  && (kshmregion_getflags(iter->sls_region)&KSHMREGION_FLAG_READ)
#endif
      ) {
   return kshmregion_getphysaddr_s(iter->sls_region,addr-iter->sls_albase,maxsize);
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
  && (kshmregion_getflags(iter->sls_region)&KSHMREGION_FLAG_WRITE)) {
   return kshmregion_getphysaddr_s(iter->sls_region,addr-iter->sls_albase,maxsize);
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
    if __unlikely(KE_ISERR(error = kshlib_openlib(name,(size_t)-1,&dep))) {
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
 /* Calculate the min/max section address pair. */
 ksecdata_cacheminmax(&self->sh_data);
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
 /* If we can't cache the library, still allow it to be loaded,
  * but make sure that we mark the library as not cached through
  * use of an always out-of-bounds cache index. */
 if __unlikely(KE_ISERR(error)) {
  lib->sh_cidx = (__size_t)-1;
  k_syslogf_prefixfile(KLOG_WARN,elf_file,"[LINKER] Failed to cache library file\n");
 }

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
 else kshlibrecent_add(lib);
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
/*err_file:*/ kfile_decref(lib->sh_file);
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
 /* sh_data.ed_secc                  */0,
 /* sh_data.ed_secv                  */NULL},
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


// Generate a sanitized true-root path with normalized
// slashes as well as prepending the true root path if
// the given pathenv isn't connected.
// Returns the required buffer size (Including the terminating ZERO-character).
// WARNING: This function is not perfect!
static size_t
get_true_path(char *buf, size_t bufsize,
              struct kfspathenv const *pathenv,
              char const *path, size_t pathmax) {
 size_t reqsize;
 char *end = buf+bufsize,*iter = buf;
 if (pathenv->env_root != kfs_getroot()) {
  /* Must prepend the path's root path. */
  kdirent_getpathname(pathenv->env_root,kfs_getroot(),iter,
                      iter < end ? (size_t)(end-iter) : 0,&reqsize);
  iter += (reqsize/sizeof(char))-1;
  /* 'kdirent_getpathname' doesn't include a trailing slash. - append it now! */
  if (iter < end) *iter = KFS_SEP;
  ++iter;
 }
 if (!pathmax || !KFS_ISSEP(path[0])) {
  /* The given path points into the CWD directory.
   * >> Now we must also prepend the given cwd path. */
  kdirent_getpathname(pathenv->env_cwd,pathenv->env_root,iter,
                      iter < end ? (size_t)(end-iter) : 0,&reqsize);
  iter += (reqsize/sizeof(char))-1;
  if (pathenv->env_cwd != kfs_getroot()) {
   /* 'kdirent_getpathname' doesn't include a trailing slash. - append it now! */
   if (iter < end) *iter = KFS_SEP;
   ++iter;
  }
 }
 /* Now must append the given path, and we're done. */
 /* WARNING: This isn't perfect because we don't
  *          handle '.' and '..' directory references. */
 while (pathmax && *path) {
  if (iter < end) {
   if ((*iter = *path) == KFS_ALTSEP) *iter = KFS_SEP;
  }
  ++iter;
  ++path;
  --pathmax;
 }
 if (iter < end) *iter = '\0';
 ++iter;
 return (size_t)(iter-buf);
}

static kerrno_t check_exec_permissions(struct kfile *fp) {
 mode_t mode; kerrno_t error;
 /* TODO: Check different bits based on filesystem UID/GID. */
 error = kfile_getattr(fp,KATTR_FS_PERM,&mode,sizeof(mode),NULL);
 if __unlikely(KE_ISERR(error)) return error;
 if (!(mode&(S_IXUSR|S_IXGRP|S_IXOTH)))
  return KE_NOEXEC; /* Don't allow execution without these bits. */
 return KE_OK;
}

__crit kerrno_t
kshlib_openfileenv(struct kfspathenv const *pathenv,
                   char const *__restrict filename, __size_t filename_max,
                   __ref struct kshlib **__restrict result,
                   int require_exec_permissions) {
 struct kfile *fp; kerrno_t error;
 char trueroot_buf[PATH_MAX];
 char *trueroot,*newtrueroot;
 size_t trueroot_size,trueroot_reqsize;
 KTASK_CRIT_MARK
 /* Generate the ~true~ absolute path for the given filename. */
 trueroot = trueroot_buf,trueroot_size = sizeof(trueroot_buf);
again_true_root:
 trueroot_reqsize = get_true_path(trueroot,trueroot_size,
                                  pathenv,filename,filename_max);
 if (trueroot_reqsize > trueroot_size) {
  newtrueroot = (trueroot == trueroot_buf)
   ? (char *)malloc(trueroot_reqsize*sizeof(char))
   : (char *)realloc(trueroot,trueroot_reqsize*sizeof(char));
  if __unlikely(!newtrueroot) { error = KE_NOMEM; goto err_trueroot; }
  trueroot = newtrueroot;
  trueroot_size = trueroot_reqsize;
  goto again_true_root;
 }
 /* Check the SHLIB cache for the trueroot path. */
 if ((*result = kshlibcache_getlib(trueroot)) != NULL) {
  if (require_exec_permissions) {
   /* Make sure that the caller has execute permissions on the binary. */
   error = check_exec_permissions((*result)->sh_file);
   if __unlikely(KE_ISERR(error)) kshlib_decref(*result);
  } else {
   error = KE_OK;
  }
  goto err_trueroot;
 }
 /* Must opening a new shared library. */
 //printf("filename = %.*q\n",(unsigned)filename_max,filename);
 //printf("trueroot = %.*q\n",(unsigned)trueroot_size,trueroot);
 error = kdirent_openat(pathenv,trueroot,trueroot_size,O_RDONLY,0,NULL,&fp);
 if (KE_ISERR(error)) goto err_trueroot;
 if (require_exec_permissions) {
  /* If needed, make sure that the caller has execute permissions on the binary. */
  error = check_exec_permissions(fp);
  if __unlikely(KE_ISERR(error)) goto err_fptrueroot;
 }
 /* Since get_true_path isn't perfect, generate another path based
  * on the file we just opened, then use that to lookup the cache again!
  * NOTE: It wasn't perfect because it can't handle '.' and '..' references. */
again_true_root2:
 error = __kfile_getpathname_fromdirent(fp,kfs_getroot(),trueroot,
                                        trueroot_size,&trueroot_reqsize);
 /* NOTE: Not all files may be able to actually have a path associated
  *       with them. We still allow those files to be loaded if they
  *       are binaries, but we simply don't cache them. */
 if __likely(KE_ISOK(error)) {
  if (trueroot_reqsize > trueroot_size) {
   newtrueroot = (trueroot == trueroot_buf)
    ? (char *)malloc(trueroot_reqsize*sizeof(char))
    : (char *)realloc(trueroot,trueroot_reqsize*sizeof(char));
   if __unlikely(!newtrueroot) { error = KE_NOMEM; goto err_fptrueroot; }
   trueroot = newtrueroot;
   trueroot_size = trueroot_reqsize;
   goto again_true_root2;
  }
  /* Check the cache with the known true-root again. */
  if ((*result = kshlibcache_getlib(trueroot)) != NULL) {
   error = KE_OK;
   goto err_fptrueroot;
  }
  /* NOPE! This library definitely isn't cached, or the given path
   *       quite simply (while existing) isn't actually a valid binary. */
 }
 if (trueroot != trueroot_buf) free(trueroot);
 error = kshlib_new(result,fp);
 kfile_decref(fp);
 return error;
err_fptrueroot:
 kfile_decref(fp);
err_trueroot:
 if (trueroot != trueroot_buf) free(trueroot);
 return error;
}

__crit kerrno_t
kshlib_opensearch(struct kfspathenv const *pathenv,
                  char const *__restrict name, __size_t namemax,
                  char const *__restrict search_paths, __size_t search_paths_max,
                  __ref struct kshlib **__restrict result,
                  int require_exec_permissions) {
 char search_cat[PATH_MAX];
 char const *iter,*end,*next;
 kerrno_t error,last_error = KE_NOENT;
 KTASK_CRIT_MARK
 namemax = strnlen(name,namemax);
 end = (iter = search_paths)+search_paths_max;
 while (iter != end) {
  size_t partsize;
  assert(iter < end);
  next = strchr(iter,':');
  partsize = next ? next-iter : strlen(iter);
  if (partsize) {
   char *path; size_t reqsize;
   int hassep = KFS_ISSEP(iter[partsize-1]);
   if (!hassep) ++partsize;
   reqsize = (partsize+namemax+1)*sizeof(char);
   if (reqsize > sizeof(search_cat)) {
    path = (char *)malloc(reqsize);
    if __unlikely(!path) return KE_NOMEM;
   } else {
    path = search_cat;
   }
   memcpy(path,iter,partsize*sizeof(char));
   if (!hassep) path[partsize-1] = KFS_SEP;
   memcpy(path+partsize,name,namemax*sizeof(char));
   path[partsize+namemax] = '\0';
#if 0
   printf("Searchcat: %.*q + %.*q -> %.*q\n",
          (unsigned)(hassep ? partsize : partsize-1),iter,(unsigned)namemax,name,
          (unsigned)(partsize+namemax),path);
#endif
   error = kshlib_openfileenv(pathenv,path,(reqsize/sizeof(char))-1,
                              result,require_exec_permissions);
   if (path != search_cat) free(path);
   /* If we did manage to load the library, we've succeeded */
   if (KE_ISOK(error)) return error;
   /* Handle a hand full of errors specially */
   switch (error) {
    case KE_NOENT: break; /* File doesn't exist. */
    case KE_ACCES: last_error = error; break;
    case KE_DEVICE: return error; /*< A device error occurred. */
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

static kerrno_t copyenvstring(char const *name, char **result, size_t *result_size) {
 char *resstring,*newresstring; kerrno_t error;
 size_t reqsize,ressize = 256*sizeof(char);
 struct kproc *caller = kproc_self();
 if __unlikely((resstring = (char *)malloc(ressize)) == NULL) return KE_NOMEM;
again_lookup:
 error = kproc_getenv(caller,name,(size_t)-1,resstring,ressize,&reqsize);
 if __unlikely(KE_ISERR(error)) {err_resstring: free(resstring); return error; }
 if (reqsize != ressize) {
  newresstring = (char *)realloc(resstring,reqsize*sizeof(char));
  if __unlikely(!newresstring) { error = KE_NOMEM; goto err_resstring; }
  resstring = newresstring;
  if (reqsize > ressize) { ressize = reqsize; goto again_lookup; }
 }
 *result = newresstring;
 *result_size = reqsize;
 return error;
}


__crit kerrno_t
kshlib_openlib(char const *__restrict name, __size_t namemax,
               __ref struct kshlib **__restrict result) {
 static char const default_libsearch_paths[] = "/lib:/usr/lib";
 kerrno_t error; struct kfspathenv env;
 env.env_root = NULL;
 if __likely(KE_ISOK(error = kfspathenv_inituser(&env))) {
  if (namemax && KFS_ISSEP(name[0])) {
   /* Absolute library path (open directly in respect to chroot-prisons). */
   error = kshlib_openfileenv(&env,name,namemax,result,0);
  } else {
   char *envval; size_t env_size;
   /* Check for process-local environment variable 'LD_LIBRARY_PATH'. */
   error = copyenvstring("LD_LIBRARY_PATH",&envval,&env_size);
   if (error == KE_NOENT) {
    error = kshlib_opensearch(&env,name,namemax,default_libsearch_paths,
                              __compiler_STRINGSIZE(default_libsearch_paths),
                              result,0);
   } else if (KE_ISERR(error)) {
    goto end_err;
   } else {
    error = kshlib_opensearch(&env,name,namemax,envval,env_size,result,0);
    free(envval);
   }
  }
 end_err:
  kfspathenv_quituser(&env);
  if __likely(KE_ISOK(error)) return error;
 }
 /* When searching for shared libraries, we somewhat break the rules
  * by allowing applications to search for libraries outside their
  * normal chroot-prison.
  * Hard restrictions are in-place though, and they are not allowed
  * to search in sub-folders or anywhere else for that matter because
  * we don't do so if the library name contains slashes.
  * >> By doing this, we make it much easier to place some executable
  *    in a chroot prison because you don't have to create a copy of
  *    all your libraries just for that one test. */
 {
  /* While questionable, this should still be completely save with respect
   * to how KOS granst rootfork() permissions, making it impossible to
   * use this mechanism to get the root-privileges of another library. */
  static char const fs_seps[2] = {KFS_SEP,KFS_ALTSEP};
  if (env.env_root != kfs_getroot() &&
     !strnpbrk(name,namemax,fs_seps,2)) {
   struct kfspathenv rootenv = KFSPATHENV_INITROOT;
   error = kshlib_opensearch(&rootenv,name,namemax,default_libsearch_paths,
                             __compiler_STRINGSIZE(default_libsearch_paths),
                             result,0);
  }
 }
 return error;
}



__crit kerrno_t
kshlib_openexe(char const *__restrict name, __size_t namemax,
               __ref struct kshlib **__restrict result,
               int allow_path_search) {
 kerrno_t error; struct kfspathenv env;
 if __unlikely(KE_ISERR(error = kfspathenv_inituser(&env))) return error;
 if (!allow_path_search || (namemax && KFS_ISSEP(name[0]))) {
  /* Absolute executable path (open directly in respect to chroot-prisons). */
  error = kshlib_openfileenv(&env,name,namemax,result,1);
 } else {
  char *envval; size_t env_size;
  /* Check for process-local environment variable 'PATH'. */
  error = copyenvstring("PATH",&envval,&env_size);
  if __unlikely(KE_ISERR(error)) goto end_err;
  error = kshlib_opensearch(&env,name,namemax,envval,env_size,result,1);
  free(envval);
 }
end_err:
 kfspathenv_quituser(&env);
 return error;
}


__DECL_END

#ifndef __INTELLISENSE__
#include "procmodules.c.inl"
#include "linker-slloaders.c.inl"
#include "linker-relocate.c.inl"
#include "linker-cache.c.inl"
#include "linker-recent.c.inl"
#endif

#endif /* !__KOS_KERNEL_LINKER_C__ */

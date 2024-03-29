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
#ifndef __KOS_KERNEL_LINKER_RELOCATE_C__
#define __KOS_KERNEL_LINKER_RELOCATE_C__ 1

#include <kos/kernel/linker.h>
#include <kos/kernel/shm.h>
#include <kos/syslog.h>
#include <sys/types.h>
#include <elf.h>
#include <stdint.h>
#include <kos/kernel/procmodules.h>
#include <kos/kernel/proc.h>
#include <windows/pe.h>

__DECL_BEGIN

static __user void *
__reloc_dlsym(struct kproc *__restrict proc, struct kprocmodule *__restrict start_module,
              struct ksymbol *__restrict sym, struct kprocmodule *__restrict exclude_module) {
 kmodid_t *iter,*end; __user void *result;
 struct ksymbol const *symbol;
 struct kshlib *lib = (start_module->pm_flags&KPROCMODULE_FLAG_RELOC) ? start_module->pm_lib : NULL;
 if (start_module != exclude_module && lib &&
    (symbol = ksymtable_lookupname_h(&lib->sh_publicsym,sym->s_name,sym->s_nmsz,sym->s_hash)) != NULL &&
     symbol->s_shndx != SHN_UNDEF) goto found_symbol;
 end = (iter = start_module->pm_depids)+start_module->pm_lib->sh_deps.sl_libc;
 for (; iter != end; ++iter) {
  assert(*iter < proc->p_modules.pms_moda);
  result = __reloc_dlsym(proc,&proc->p_modules.pms_modv[*iter],sym,exclude_module);
  if (result != NULL) return result;
 }
 if (lib && (symbol = ksymtable_lookupname_h(&lib->sh_weaksym,sym->s_name,sym->s_nmsz,sym->s_hash)) != NULL &&
     symbol->s_shndx != SHN_UNDEF) goto found_symbol;
 return NULL;
found_symbol:
 return (__user void *)((uintptr_t)start_module->pm_base+symbol->s_addr);
}

static __user void *
__new_dlsym(struct kproc *__restrict proc, struct kprocmodule *__restrict start_module,
            struct ksymbol *__restrict sym, struct kprocmodule *__restrict exclude_module) {
 kmodid_t *iter,*end; __user void *result;
 struct ksymbol const *symbol; struct kshlib *lib = start_module->pm_lib;
 if (start_module != exclude_module && 
    (symbol = ksymtable_lookupname_h(&lib->sh_publicsym,sym->s_name,sym->s_nmsz,sym->s_hash)) != NULL &&
     symbol->s_shndx != SHN_UNDEF) goto found_symbol;
 end = (iter = start_module->pm_depids)+start_module->pm_lib->sh_deps.sl_libc;
 for (; iter != end; ++iter) {
  assert(*iter < proc->p_modules.pms_moda);
  result = __new_dlsym(proc,&proc->p_modules.pms_modv[*iter],sym,exclude_module);
  if (result != NULL) return result;
 }
 if ((symbol = ksymtable_lookupname_h(&lib->sh_weaksym,sym->s_name,sym->s_nmsz,sym->s_hash)) != NULL &&
      symbol->s_shndx != SHN_UNDEF) goto found_symbol;
 return NULL;
found_symbol:
 return (__user void *)((uintptr_t)start_module->pm_base+symbol->s_addr);
}

__local __user void *
__dlsym(struct kproc *__restrict proc, struct kprocmodule *__restrict start_module,
        struct ksymbol *__restrict sym, struct kprocmodule *__restrict exclude_module) {
 /* Must prefer symbols from modules that are already relocated
  * in order to prevent symbols from being defined multiple
  * times when originating from different modules.
  * e.g.:
  * $ gcc -o main main.c -ldl -lc
  * 
  * main.c:
  * >> int main(int argc, char *argv[]) {
  * >>   // libc is already loaded, meaning that an existing
  * >>   // definition of 'errno' must be preferred over
  * >>   // a definition that would otherwise be loaded
  * >>   // as a result of the insertion of 'libcurses.so'
  * >>   // WARNING: 'errno' isn't really the name of an exported symbol,
  * >>   //          but my point should be clear none-the-less.
  * >>   void *d = dlopen("libcurses.so",RTLD_NOW);
  * >>   int *peno = (int *)dlsym(d,"errno");
  * >>   assert(peno == &errno); // This would otherwise fail
  * >> }
  */
#if 0
 return __new_dlsym(proc,start_module,sym,exclude_module);
#else
 void        *result = __reloc_dlsym(proc,start_module,sym,exclude_module);
 if (!result) result =   __new_dlsym(proc,start_module,sym,exclude_module);
 return result;
#endif
}

#define LOG(level,...) k_syslogf_prefixfile(level,reloc_module->pm_lib->sh_file,__VA_ARGS__)


static void
kreloc_elf32_rel_exec(Elf32_Rel const *relv, size_t relc,
                      struct ksymlist *__restrict symbols,
                      struct kshm *__restrict memspace,
                      struct kproc *__restrict proc,
                      struct kprocmodule *__restrict reloc_module,
                      struct kprocmodule *__restrict start_module) {
 Elf32_Rel const *iter,*end;
 uintptr_t new_address,base,last_symaddr;
 struct ksymbol *sym,*last_sym = NULL;
 base = (uintptr_t)reloc_module->pm_base;
 end = (iter = relv)+relc;
 for (; iter != end; ++iter) {
  __u8  type   = ELF32_R_TYPE(iter->r_info);
  __u32 symbol = ELF32_R_SYM(iter->r_info);
  if (symbol >= symbols->sl_symc) {
   LOG(KLOG_ERROR,"[CMD %Iu] Out-of-bounds symbol index %Iu >= %Iu while relocating\n",
       iter-relv,(size_t)symbol,symbols->sl_symc);
   goto rel_next;
  }
  sym = symbols->sl_symv[symbol];
  kassertobjnull(sym);
#define PGETMEM(dst,addr,size) \
 if (!kshm_copyfromuser(memspace,dst,addr,size));else\
     LOG(KLOG_ERROR,"[CMD %Iu] failed: memcpy(k:%p,u:%p,%Iu)\n",\
         iter-relv,dst,addr,size)
#define GETMEM(T,addr) \
 __xblock({ T __inst; \
            PGETMEM(&__inst,addr,sizeof(__inst)),__inst = 0;\
            __xreturn __inst;\
 })
#define SETMEM(dst,src,s) \
 do{\
  if (kshm_copytouser_w(memspace,dst,src,s)) {\
   LOG(KLOG_ERROR,"[CMD %Iu] failed: memcpy(u:%p,k:%p,%Iu)\n",iter-relv,dst,src,s);\
  }\
 }while(0)
#define ADDR(offset)      ((__user void *)((uintptr_t)(offset)+base))
  if (!sym) new_address = 0;
  else switch (type) {
   case R_386_32:
   case R_386_PC32:
   case R_386_COPY:
   case R_386_GLOB_DAT:
   case R_386_JMP_SLOT:
    /* http://www.skyfree.org/linux/references/ELF_Format.pdf (page #29):
     * During execution, the dynamic linker copies data associated with the
     * shared object's symbol to the location specified by the offset.
     * >> Meaning we must not search for the symbol within the current module for R_386_COPY */
    if __likely(sym == last_sym) {
     /* No need to look up the same symbol twice in a row. */
     new_address = last_symaddr;
    } else {
     new_address = (uintptr_t)__dlsym(proc,start_module,sym,type == R_386_COPY ? reloc_module : NULL);
     LOG(KLOG_TRACE,"[CMD %Iu] Lookup symbol: %I32u:%s %p -> %p\n",
         iter-relv,symbol,sym->s_name,iter->r_offset,new_address);
     last_sym     = sym;
     last_symaddr = new_address;
    }
    /* TODO: Weak symbols may be initialized to NULL. */
    if __likely(new_address != 0) break;
    LOG(KLOG_ERROR,"[CMD %Iu] Failed to find symbol: %s\n",iter-relv,sym->s_name);
   default: new_address = sym->s_addr+base;
    break;
  }
  switch (type) {
    if (0) { case R_386_PC32: new_address -= (iter->r_offset+base); }
    if (0) { case R_386_RELATIVE: new_address = base; }
   case R_386_32:
    new_address += GETMEM(ssize_t,ADDR(iter->r_offset));
/*
   if (0) { case R_386_GLOB_DAT: }
    // TODO: Lookup global symbol 'sym->s_name' and overwrite 'new_address':
    //   >> new_address = get_global_symbol(sym->s_name) ?: new_address;
*/
   case R_386_JMP_SLOT:
    SETMEM(ADDR(iter->r_offset),&new_address,sizeof(uintptr_t));
    break;
   case R_386_COPY:
    if (sym &&
        kshm_copyinuser_w(memspace,ADDR(iter->r_offset),
                         (void *)new_address,sym->s_size)) {
     LOG(KLOG_ERROR,"[CMD %Iu] failed: memcpy(u:%p,u:%p,%Iu)\n",
         iter-relv,ADDR(iter->r_offset),(void *)new_address,sym->s_size);
    }
    break;
   default:
    LOG(KLOG_ERROR,"[CMD %Iu] Unrecognized relocation type: %d\n",
        iter-relv,type);
    break;
  }
rel_next:;
 }
}



static void
kreloc_pe_import_exec(struct krelocvec const *__restrict self,
                      struct kshm *__restrict memspace,
                      struct kproc *__restrict proc,
                      struct kprocmodule *__restrict reloc_module,
                      struct kprocmodule *__restrict start_module) {
#define relv  self->rv_pe.rpi_symv
 struct ksymbol **iter,**end,*sym;
 struct kprocmodule *hint_module,*module_end;
 uintptr_t new_address,base; ksymaddr_t thunk_iter;
 base = (uintptr_t)reloc_module->pm_base;
 end = (iter = relv)+self->rv_relc;
 if (self->rv_pe.rpi_hint) {
  /* Try to locate the loaded instance of the hinted module. */
  module_end = (hint_module = proc->p_modules.pms_modv)+proc->p_modules.pms_moda;
  for (;; ++hint_module) {
   if (hint_module == module_end) { hint_module = NULL; break; }
   if (hint_module->pm_lib == self->rv_pe.rpi_hint) break;
  }
 } else {
  hint_module = NULL;
 }
 thunk_iter = self->rv_pe.rpi_thunk;
 for (; iter != end; ++iter) {
  sym = *iter;
  kassertobj(sym);
  LOG(KLOG_TRACE,"[PE][CMD %Iu] Importing symbol %s\n",
      iter-relv,sym->s_name);
  /* Prefer loading the symbol from the hinted module.
   * NOTE: I am fully aware that we're already breaking windows rules
   *       by not only scanning the hinted module, but also all of its
   *       dependencies. - But the important part is that the hinted
   *       module is scanned first, as is required by PE, and if you
   *       look further down below, you'll see that we're even performing
   *       regular ELF-style dependency scanning as well! */
  new_address = __likely(hint_module)
   ? (uintptr_t)__dlsym(proc,hint_module,sym,reloc_module) 
   : 0;
  /* If we didn't already find the symbol, do a regular ELF-style symbol scan. */
  if __unlikely(!new_address) {
   new_address = (uintptr_t)__dlsym(proc,start_module,sym,reloc_module);
  }
  if (!new_address) {
   LOG(KLOG_ERROR,"[PE][CMD %Iu] Failed to find symbol: %s\n",
       iter-relv,sym->s_name);
  }
  /* Store the address we've just figured out in the thunk. */
  SETMEM(ADDR(thunk_iter),&new_address,sizeof(new_address));
  thunk_iter += sizeof(uintptr_t);
 }
#undef relv
}

static void
kreloc_pe32_reloc_exec(struct krelocvec const *__restrict self,
                       struct kshm *__restrict memspace,
                       struct kproc *__restrict proc,
                       struct kprocmodule *__restrict reloc_module,
                       struct kprocmodule *__restrict start_module) {
#define relv  self->rv_pe32_reloc.rpr_offv
 __u16 *iter,*end,mode,offset,temp16;
 __u32 temp32,base,delta; __u64 temp64; __user void *absaddr;
 /* NOTE: Since we always added module_base to everything,
  *       we have essentially aligned everything to NULL.
  *       With that in mind, a shift to any address other
  *       than NULL implies that address as delta. */
 delta = reloc_module->pm_base32/*-0*/;
 if __unlikely(!delta) return; /* No relocation required. */
 end = (iter = self->rv_pe32_reloc.rpr_offv)+self->rv_relc;
 /* New base address of this relocation 'block'.
  * >> This is the module base address plus the relocation base. */
 base = reloc_module->pm_base32+self->rv_pe32_reloc.rpr_base;
 for (; iter != end; ++iter) {
  mode = *iter;
  /* Offset within the block. */
  offset = mode & 0xfff;
  mode >>= 12;
  /* NOTE: This relocation interpreter is based on stuff found here:
   * https://katjahahn.github.io/PortEx/javadocs/com/github/katjahahn/parser/sections/reloc/RelocType.html
   * https://opensource.apple.com/source/emacs/emacs-56/emacs/nt/preprep.c.auto.html
   */
  absaddr = (__user void *)(base+offset);
  switch (mode) {
   case IMAGE_REL_BASED_ABSOLUTE:
    /* 'The base relocation is skipped' */
    break;

   case IMAGE_REL_BASED_HIGHLOW:
   case IMAGE_REL_BASED_HIGHADJ:
    temp32  = GETMEM(__u32,absaddr);
    temp32 += delta;
    SETMEM(absaddr,&temp32,sizeof(__u32));
    break;

   case IMAGE_REL_BASED_LOW:
   case IMAGE_REL_BASED_HIGH:
    temp16  = GETMEM(__u16,absaddr);
    temp16 += (__u16)((mode == IMAGE_REL_BASED_LOW) ? (delta & 0xffff) : ((delta & 0xffff0000) >> 16));
    SETMEM(absaddr,&temp16,sizeof(__u16));
    break;

   case IMAGE_REL_BASED_DIR64:
    temp64  = GETMEM(__u64,absaddr);
    temp64 += delta;
    SETMEM(absaddr,&temp64,sizeof(__u64));
    break;

   case IMAGE_REL_BASED_SECTION: /* TODO */
   case IMAGE_REL_BASED_REL32: /* TODO */
   default:
    LOG(KLOG_ERROR,"[PE][CMD %Iu] Unsupported relocation command %I16u\n",
        iter-relv,mode);
    break;
  }
 }
#undef relv
}

__local void
krelocvec_exec(struct krelocvec *__restrict self,
               struct ksymlist *__restrict symbols,
               struct kshm *__restrict memspace,
               struct kproc *__restrict proc,
               struct kprocmodule *__restrict reloc_module,
               struct kprocmodule *__restrict start_module) {
 switch (self->rv_type) {
  case KRELOCVEC_TYPE_ELF32_REL:
   kreloc_elf32_rel_exec(self->rv_elf32_relv,self->rv_relc,
                         symbols,memspace,proc,reloc_module,start_module);
   break;
  case KRELOCVEC_TYPE_ELF32_RELA:
   LOG(KLOG_ERROR,"ELF-32 Rela-relocations not implemented\n");
   break;
  case KRELOCVEC_TYPE_PE_IMPORT:
   kreloc_pe_import_exec(self,memspace,proc,reloc_module,start_module);
   break;
  case KRELOCVEC_TYPE_PE32_RELOC:
   kreloc_pe32_reloc_exec(self,memspace,proc,reloc_module,start_module);
   break;
  default: __compiler_unreachable(); break;
 }
}
void kreloc_exec(struct kreloc *__restrict self,
                 struct kshm *__restrict memspace,
                 struct kproc *__restrict proc,
                 struct kprocmodule *__restrict reloc_module,
                 struct kprocmodule *__restrict start_module) {
 struct krelocvec *iter,*end;
 kassert_kreloc(self);
 kassert_kshm(memspace);
 kassertmem(self->r_vecv,self->r_vecc*sizeof(struct krelocvec));
 end = (iter = self->r_vecv)+self->r_vecc;
 for (; iter != end; ++iter) {
  LOG(KLOG_TRACE,"Relocating... (%Iu commands)\n",iter->rv_relc);
  krelocvec_exec(iter,&self->r_elf_syms,memspace,proc,reloc_module,start_module);
 }
}


__DECL_END

#endif /* !__KOS_KERNEL_LINKER_RELOCATE_C__ */

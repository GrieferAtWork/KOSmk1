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
#ifndef __KOS_KERNEL_LINKER_RELOCATE_C_INL__
#define __KOS_KERNEL_LINKER_RELOCATE_C_INL__ 1

#include <kos/kernel/linker.h>
#include <kos/kernel/shm.h>
#include <kos/syslog.h>
#include <sys/types.h>
#include <elf.h>
#include <stdint.h>
#include <kos/kernel/procmodules.h>
#include <kos/kernel/proc.h>

__DECL_BEGIN

static __user void *
__reloc_dlsym(struct kproc *__restrict proc,
              struct kprocmodule *__restrict start_module,
              char const *name, size_t name_size, ksymhash_t name_hash,
              struct kprocmodule *__restrict exclude_module) {
 kmodid_t *iter,*end; __user void *result;
 struct ksymbol const *symbol;
 struct kshlib *lib = (start_module->pm_flags&KPROCMODULE_FLAG_RELOC) ? start_module->pm_lib : NULL;
 if (start_module != exclude_module && lib &&
    (symbol = ksymtable_lookupname_h(&lib->sh_publicsym,name,name_size,name_hash)) != NULL &&
     symbol->s_shndx != SHN_UNDEF) goto found_symbol;
 end = (iter = start_module->pm_depids)+start_module->pm_lib->sh_deps.sl_libc;
 for (; iter != end; ++iter) {
  assert(*iter < proc->p_modules.pms_moda);
  result = __reloc_dlsym(proc,&proc->p_modules.pms_modv[*iter],
                         name,name_size,name_hash,exclude_module);
  if (result != NULL) return result;
 }
 if (lib && (symbol = ksymtable_lookupname_h(&lib->sh_weaksym,name,name_size,name_hash)) != NULL &&
     symbol->s_shndx != SHN_UNDEF) goto found_symbol;
 return NULL;
found_symbol:
 return (__user void *)((uintptr_t)start_module->pm_base+symbol->s_addr);
}

static __user void *
__new_dlsym(struct kproc *__restrict proc,
            struct kprocmodule *__restrict start_module,
            char const *name, size_t name_size, ksymhash_t name_hash,
            struct kprocmodule *__restrict exclude_module) {
 kmodid_t *iter,*end; __user void *result;
 struct ksymbol const *symbol; struct kshlib *lib = start_module->pm_lib;
 if (start_module != exclude_module && 
    (symbol = ksymtable_lookupname_h(&lib->sh_publicsym,name,name_size,name_hash)) != NULL &&
      symbol->s_shndx != SHN_UNDEF) goto found_symbol;
 end = (iter = start_module->pm_depids)+start_module->pm_lib->sh_deps.sl_libc;
 for (; iter != end; ++iter) {
  assert(*iter < proc->p_modules.pms_moda);
  result = __new_dlsym(proc,&proc->p_modules.pms_modv[*iter],
                       name,name_size,name_hash,exclude_module);
  if (result != NULL) return result;
 }
 if ((symbol = ksymtable_lookupname_h(&lib->sh_weaksym,name,name_size,name_hash)) != NULL &&
     symbol->s_shndx != SHN_UNDEF) goto found_symbol;
 return NULL;
found_symbol:
 return (__user void *)((uintptr_t)start_module->pm_base+symbol->s_addr);
}

__local __user void *
__dlsym(struct kproc *__restrict proc,
        struct kprocmodule *__restrict start_module,
        char const *name, size_t name_size, ksymhash_t name_hash,
        struct kprocmodule *__restrict exclude_module) {
 // Must prefer symbols from modules that are already relocated
 // in order to prevent symbols from being defined multiple
 // times, while originating from different modules.
 // e.g.:
 // $ gcc -o main main.c -ldl -lc
 // 
 // main.c:
 // >> int main(int argc, char **argv) {
 // >>   // libc is already loaded, meaning that an existing
 // >>   // definition of 'errno' must be preferred over
 // >>   // a definition that would otherwise be loaded
 // >>   // as a result of the insertion of 'libproc.so'
 // >>   // WARNING: 'errno' isn't really the name of an exported symbol,
 // >>   //          but my point should be clear none-the-less.
 // >>   void *d = dlopen("libproc.so",RTLD_NOW);
 // >>   int *peno = (int *)dlsym(d,"errno");
 // >>   assert(peno == &errno); // This would otherwise fail
 // >> }
#if 0
 return __new_dlsym(proc,start_module,name,name_size,name_hash,exclude_module);
#else
 void        *result = __reloc_dlsym(proc,start_module,name,name_size,name_hash,exclude_module);
 if (!result) result =   __new_dlsym(proc,start_module,name,name_size,name_hash,exclude_module);
 return result;
#endif
}

#define LOG(level,...) k_syslogf_prefixfile(level,reloc_module->pm_lib->sh_file,__VA_ARGS__)


__local void
kreloc_elf32_rel_exec(Elf32_Rel const *relv, size_t relc,
                      struct ksymlist *__restrict symbols,
                      struct kshm *__restrict memspace,
                      struct kproc *__restrict proc,
                      struct kprocmodule *__restrict reloc_module,
                      struct kprocmodule *__restrict start_module) {
 Elf32_Rel const *iter,*end;
 uintptr_t new_address,base;
 base = (uintptr_t)reloc_module->pm_base;
 end = (iter = relv)+relc;
 for (; iter != end; ++iter) {
  struct ksymbol *sym;
  __u8  type   = ELF32_R_TYPE(iter->r_info);
  __u32 symbol = ELF32_R_SYM(iter->r_info);
  if (symbol >= symbols->sl_symc) {
   LOG(KLOG_ERROR,"[CMD %Iu] Out-of-bounds symbol index %Iu >= %Iu while relocating\n",
       iter-relv,(size_t)symbol,symbols->sl_symc);
   goto rel_next;
  }
  sym = symbols->sl_symv[symbol];
  kassertobjnull(sym);
#define GETMEM(T,addr) \
 __xblock({ T __inst; \
            if (kshm_memcpy_u2k(&proc->p_shm,&__inst,addr,sizeof(T)) != sizeof(T)) {\
             LOG(KLOG_ERROR,"[CMD %Iu] failed: memcpy(k:%p,u:%p,%Iu)\n",iter-relv,&__inst,addr,sizeof(T));\
             __inst = 0;\
            }\
            __xreturn __inst;\
 })
#define SETMEM(dst,src,s) \
 do{\
  if (kshm_memcpy_k2u(&proc->p_shm,dst,src,s) != (s)) {\
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
    // http://www.skyfree.org/linux/references/ELF_Format.pdf (page #29):
    // During execution, the dynamic linker copies data associated with the
    // shared object's symbol to the location specified by the offset.
    // >> Meaning we must not search for the symbol within the current module for R_386_COPY
    new_address = (uintptr_t)__dlsym(proc,start_module,sym->s_name,
                                     sym->s_nmsz,sym->s_hash,
                                     type == R_386_COPY ? reloc_module : NULL);
    LOG(KLOG_INSANE,"[CMD %Iu] Lookup symbol: %I32u:%s %p -> %p\n",
        iter-relv,symbol,sym->s_name,iter->r_offset,new_address);
    // TODO: Weak symbols may be initialized to NULL
    if (new_address != 0) break;
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
        kshm_memcpy_u2u(&proc->p_shm,ADDR(iter->r_offset),
                       (void *)new_address,sym->s_size) != sym->s_size) {
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
  default: __builtin_unreachable(); break;
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
  krelocvec_exec(iter,&self->r_syms,memspace,proc,reloc_module,start_module);
 }
}


__DECL_END

#endif /* !__KOS_KERNEL_LINKER_RELOCATE_C_INL__ */

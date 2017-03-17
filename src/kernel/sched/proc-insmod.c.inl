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
#ifndef __KOS_KERNEL_SCHED_PROC_INSMOD_C_INL__
#define __KOS_KERNEL_SCHED_PROC_INSMOD_C_INL__ 1

#include <kos/kernel/proc.h>
#include <kos/kernel/linker.h>
#include <kos/kernel/procmodules.h>
#include <kos/kernel/paging.h>
#include <kos/syslog.h>
#include <math.h>
#include <malloc.h>
#include <kos/mod.h>
#include <kos/kernel/shm.h>

__DECL_BEGIN


__local __crit kerrno_t
kproc_loadmodsections(struct kproc *__restrict self,
                      struct kshlib *__restrict module,
                      __pagealigned __user void *base) {
 struct kshlibsection *iter,*end; kerrno_t error;
 KTASK_CRIT_MARK
 end = (iter = module->sh_data.ed_secv)+module->sh_data.ed_secc;
 for (; iter != end; ++iter) {
  error = kshm_mapfullregion_unlocked(kproc_getshm(self),
                                     (void *)((uintptr_t)base+iter->sls_albase),
                                      iter->sls_region);
  if __unlikely(KE_ISERR(error)) goto err_seciter;
 }
 return KE_OK;
err_seciter:
 // Unmap all already mapped sections
 while (iter-- != module->sh_data.ed_secv) {
  kshm_unmap_unlocked(kproc_getshm(self),
                     (void *)((uintptr_t)base+iter->sls_albase),
                      kshmregion_getpages(iter->sls_region),
                      KSHMUNMAP_FLAG_NONE);
 }
 return error;
}




// Unloads all sections originally loaded from a given module.
// NOTE: Sections already unmapped (such as through use of munmap) are silently ignored.
// WARNING: Read-write sections are forceably unmapped.
__local __crit void
kproc_unloadmodsections(struct kproc *__restrict self,
                        struct kshlib *__restrict module,
                        __pagealigned __user void *base) {
 struct kshlibsection *seciter,*secend;
 KTASK_CRIT_MARK
 assert(isaligned((uintptr_t)base,PAGEALIGN));
 secend = (seciter = module->sh_data.ed_secv)+module->sh_data.ed_secc;
 for (; seciter != secend; ++seciter) {
  if (kshmregion_getflags(seciter->sls_region)&KSHMREGION_FLAG_WRITE) {
   /* Force unmap writable sections (Due to copy-on-write,
    * they may have been re-mapped. - An operation we can't track) */
   kshm_unmap_unlocked(kproc_getshm(self),
                      (void *)((uintptr_t)base+seciter->sls_albase),
                      (seciter->sls_base-seciter->sls_albase)+
                       kshmregion_getpages(seciter->sls_region),
                       KSHMUNMAP_FLAG_NONE);
  } else {
   KTASK_NOINTR_BEGIN
   __evalexpr(kshm_unmapregion_unlocked(kproc_getshm(self),
                                       (void *)((uintptr_t)base+seciter->sls_albase),
                                        seciter->sls_region));
   KTASK_NOINTR_END
  }
 }
}




__local __crit void
kproc_call_user(struct kproc *__restrict self,
                struct kprocmodule *module,
                void (__user *callback)(void)) {
 KTASK_CRIT_MARK
 k_syslogf_prefixfile(KLOG_DEBUG,module->pm_lib->sh_file,
                      "TODO: Call user function at %p\n",callback);
 // TODO: Execute the given user-callback
}

__local __crit void
kproc_call_symaddr(struct kproc *__restrict self,
                   struct kprocmodule *module,
                   ksymaddr_t symaddr) {
 void *useraddr;
 KTASK_CRIT_MARK
 useraddr = kprocmodule_translate(module,symaddr);
 if (useraddr) kproc_call_user(self,module,(void(*)(void))useraddr);
}
__local __crit void
kproc_call_symarray(struct kproc *__restrict self,
                    struct kprocmodule *module, ksymaddr_t array_start,
                    size_t array_size) {
 KTASK_CRIT_MARK
 // TODO: Iterate the array in reverse order and execute entries
 (void)self,(void)module,(void)array_start,(void)array_size;
}

__local __crit void
kproc_call_module_fini(struct kproc *__restrict self,
                       struct kprocmodule *module) {
 struct kshlib *shlib;
 KTASK_CRIT_MARK
 kassert_kproc(self);
 kassert_kprocmodule(module);
 shlib = module->pm_lib;
 kassert_kshlib(shlib);
 kproc_call_symarray(self,module,
                     shlib->sh_callbacks.slc_fini_array_v,
                     shlib->sh_callbacks.slc_fini_array_c);
 kproc_call_symaddr(self,module,shlib->sh_callbacks.slc_fini);
}

__crit __user void *
kproc_find_base_for_module(struct kproc *__restrict self,
                           struct kshlib *__restrict module) {
 uintptr_t addr_lo,addr_hi;
 __user void *hint,*result;
 KTASK_CRIT_MARK
 addr_lo = module->sh_data.ed_begin;
 addr_hi = module->sh_data.ed_end;
 // TODO: We don't need to ensure free memory between sections:
 //              vvv don't need to check for these
 //       ..LLLLL...LLLL...AAA
 // .: unused
 // L: lib
 // A: application
 if (addr_hi <= addr_lo) return (void *)(uintptr_t)-1;
 if (module->sh_flags&KMODFLAG_PREFFIXED) {
  /* Prefer loading the module to its lowest-address,
   * thus potentially speeding up relocations later on.
   * >> This is used for Windows PE binaries. */
  hint = (__user void *)align(addr_lo,PAGEALIGN);
 } else {
  hint = KPAGEDIR_MAPANY_HINT_LIBS;
 }
 result = kpagedir_findfreerange(kproc_getpagedir(self),
                                 ceildiv(addr_hi-addr_lo,PAGESIZE),hint);
 if (result == KPAGEDIR_FINDFREERANGE_ERR) return (void *)(uintptr_t)-1;
 /* Re-align the result free range to fix the module's symbolic start address. */
 if ((uintptr_t)result < addr_lo) return (void *)(uintptr_t)-1;
 *(uintptr_t *)&result -= addr_lo;
 return result;
}

__crit kerrno_t
kproc_insmod_single_unlocked(struct kproc *__restrict self,
                             struct kshlib *__restrict module,
                             kmodid_t *__restrict module_id,
                             kmodid_t *__restrict depids) {
 struct kprocmodule *modtab,*modend;
 struct kprocmodule *free_slot = NULL;
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kproc(self);
 kassert_kshlib(module);
 kassertmem(depids,module->sh_deps.sl_libc*sizeof(kmodid_t));
 assert(krwlock_iswritelocked(&self->p_shm.s_lock));
 assert(kproc_islocked(self,KPROC_LOCK_MODS));
 // Check if this exact module is already loaded.
 // (We can simply compare pointers here...)
 modend = (modtab = self->p_modules.pms_modv)+self->p_modules.pms_moda;
 for (; modtab != modend; ++modtab) {
  if (modtab->pm_lib == module) {
   // Increment the load counter.
   if __unlikely(modtab->pm_loadc == (__u32)-1) return KE_OVERFLOW;
   ++modtab->pm_loadc;
   *module_id = (kmodid_t)(modtab-self->p_modules.pms_modv);
   return KS_UNCHANGED;
  } else if (!modtab->pm_lib) {
   free_slot = modtab;
  }
 }
 if (free_slot) {
  modtab = free_slot;
 } else {
  // Must allocate a new entry for this module (the first time it's loaded).
  modtab = (struct kprocmodule *)realloc(self->p_modules.pms_modv,
                                        (self->p_modules.pms_moda+1)*
                                         sizeof(struct kprocmodule));
  if __unlikely(!modtab) return KE_NOMEM;
  self->p_modules.pms_modv = modtab;
  modtab += self->p_modules.pms_moda++;
 }
 kobject_init(modtab,KOBJECT_MAGIC_PROCMODULE);
 modtab->pm_loadc  = 1;
 modtab->pm_flags  = KPROCMODULE_FLAG_NONE;
 modtab->pm_lib    = module;
 modtab->pm_depids = depids;
 *module_id = (kmodid_t)(modtab-self->p_modules.pms_modv);
 if __unlikely(KE_ISERR(error = kshlib_incref(module))) goto err_modtab;
 // Use a fixed base of NULL for fixed-address modules (such as executables)
 if ((module->sh_flags&KMODFLAG_FIXED) ||
     !module->sh_data.ed_secc) modtab->pm_base = NULL;
 else {
  modtab->pm_base = kproc_find_base_for_module(self,module);
  if (modtab->pm_base == (void *)(uintptr_t)-1) { error = KE_NOSPC; goto err_module; }
 }
 assert(modtab->pm_lib == module);
 k_syslogf_prefixfile(KLOG_TRACE,module->sh_file,"Loading module in process (pid = %I32d) to %.8p\n",
                      self->p_pid,modtab->pm_base);
 error = kproc_loadmodsections(self,module,modtab->pm_base);
 if __unlikely(KE_ISERR(error)) goto err_module;
 assertf(modtab->pm_loadc != 0,"Invalid module load counter");
 return error;
err_module:
 kshlib_decref(module);
err_modtab:
 modtab->pm_lib = NULL;
 assert(KE_ISERR(error));
 // TODO: Free unused memory from 'self->p_modules.pms_modv'
 return error;
}


__crit kerrno_t
kproc_delmod_single_unlocked(struct kproc *__restrict self,
                             kmodid_t module_id,
                             kmodid_t **dep_idv, size_t *dep_idc) {
 struct kprocmodule *module; size_t newvecsize;
 __ref struct kshlib *shlib;
 KTASK_CRIT_MARK
 kassert_kproc(self);
 kassertobj(dep_idv);
 kassertobj(dep_idc);
 assert(krwlock_iswritelocked(&self->p_shm.s_lock));
 assert(kproc_islocked(self,KPROC_LOCK_MODS));
 if __unlikely(module_id >= self->p_modules.pms_moda) return KE_INVAL;
 module = self->p_modules.pms_modv+module_id;
 if __unlikely(!module->pm_lib) return KE_INVAL;
 kassert_kshlib(module->pm_lib);
 assertf(module->pm_loadc != 0,"Invalid module load counter");
 if (--module->pm_loadc != 0) return KS_UNCHANGED; // Module must still remain loaded
 // Call finalization functions
 shlib = module->pm_lib;
 kproc_call_module_fini(self,module);
 // Unload all sections loaded by this module
 kproc_unloadmodsections(self,shlib,module->pm_base);
 // Drop the reference previously held by the the slot
 kshlib_decref(shlib);
 // Inherit the dependency vector into the caller
 *dep_idv = module->pm_depids;
 *dep_idc = shlib->sh_deps.sl_libc;

 newvecsize = self->p_modules.pms_moda-1;
 if (module_id == newvecsize) {
  // Release memory if the highest-index module was unloaded
  while (newvecsize && !self->p_modules.pms_modv[newvecsize-1].pm_lib) --newvecsize;
  self->p_modules.pms_moda = newvecsize;
  if (!newvecsize) {
   free(self->p_modules.pms_modv);
   self->p_modules.pms_modv = NULL;
  } else {
   module = (struct kprocmodule *)realloc(self->p_modules.pms_modv,
                                          newvecsize*sizeof(struct kprocmodule));
   if (module) self->p_modules.pms_modv = module;
  }
 } else {
  // Mark the module entry as unused
  module->pm_lib = NULL;
  kobject_badmagic(module);
#ifdef __DEBUG__
  module->pm_depids = (kmodid_t *)(uintptr_t)-1;
  module->pm_loadc  = 0;
  module->pm_flags  = KPROCMODULE_FLAG_NONE;
  module->pm_base   = (__user void *)(uintptr_t)-1;
#endif
 }
 return KE_OK;
}


static __crit kerrno_t
kproc_insmod_unlocked_impl(struct kproc *__restrict self,
                           struct kshlib *__restrict module,
                           kmodid_t *module_id) {
 struct kshlib **iter,**end; kerrno_t error;
 kmodid_t *depids,*depids_iter;
 KTASK_CRIT_MARK
 kassertobj(module_id);
 depids = (kmodid_t *)malloc(module->sh_deps.sl_libc*sizeof(kmodid_t));
 if __unlikely(!depids) return KE_NOMEM;
 end = (iter = module->sh_deps.sl_libv)+module->sh_deps.sl_libc;
 depids_iter = depids;
 for (; iter != end; ++depids_iter,++iter) {
  error = kproc_insmod_unlocked_impl(self,*iter,depids_iter);
  if __unlikely(KE_ISERR(error)) goto err_iter;
 }
 assert(depids_iter == depids+module->sh_deps.sl_libc);
 error = kproc_insmod_single_unlocked(self,module,module_id,depids);
 if __unlikely(error != KE_OK) free(depids);
 return error;
err_iter:
 while (depids_iter-- != depids) {
  kproc_delmod_unlocked(self,*depids_iter);
 }
 free(depids);
 return error;
}

static void
kproc_modrelocate(struct kproc *self,
                  kmodid_t modid,
                  kmodid_t start_modid) {
 struct kprocmodule *module;
 kmodid_t *iter,*end;
 assert(start_modid < self->p_modules.pms_moda);
 assert(modid < self->p_modules.pms_moda);
 module = &self->p_modules.pms_modv[modid];
 assert(module);
 assert(module->pm_lib);
 kassert_kprocmodule(module);
 kassert_kshlib(module->pm_lib);
 // Recursively relocate all dependencies (do this first!)
 end = (iter = module->pm_depids)+module->pm_lib->sh_deps.sl_libc;
 for (; iter != end; ++iter) kproc_modrelocate(self,*iter,start_modid);
 if (!(module->pm_flags&KPROCMODULE_FLAG_RELOC)) {
  // Relocate the module (Make sure every module is only relocated _ONCE_)
  kreloc_exec(&module->pm_lib->sh_reloc,
             kproc_getshm(self),self,module,
             &self->p_modules.pms_modv[start_modid]);
  // Must set the reloc flag _AFTER_ we did it, as otherwise
  // the module might try to load its own non-relocated symbols...
  module->pm_flags |= KPROCMODULE_FLAG_RELOC;
 }
}


__crit kerrno_t
kproc_insmod_unlocked(struct kproc *__restrict self,
                      struct kshlib *__restrict module,
                      kmodid_t *module_id) {
 kerrno_t error;
 KTASK_CRIT_MARK
 error = kproc_insmod_unlocked_impl(self,module,module_id);
 if __likely(error == KE_OK) {
  // NOTE: Newly loaded modules must be relocated
  //       after they've all been inserted, in order
  //       to properly link global (__public) variables.
  kproc_modrelocate(self,*module_id,*module_id);
 }
 return error;
}


__crit kerrno_t
kproc_delmod_unlocked(struct kproc *__restrict self,
                      kmodid_t module_id) {
 kerrno_t error;
 kmodid_t *dep_idv,*iter; size_t dep_idc;
 KTASK_CRIT_MARK
 kassert_kproc(self);
 assert(krwlock_iswritelocked(&self->p_shm.s_lock));
 assert(kproc_islocked(self,KPROC_LOCK_MODS));
 error = kproc_delmod_single_unlocked(self,module_id,&dep_idv,&dep_idc);
 if (error != KE_OK) return error;
 iter = dep_idv+dep_idc;
 // Recursively unload all modules
 while (iter-- != dep_idv) {
  kproc_delmod_unlocked(self,*iter);
 }
 free(dep_idv);
 return KE_OK;
}





__crit kerrno_t
kproc_insmod_c(struct kproc *__restrict self,
               struct kshlib *__restrict module,
               kmodid_t *module_id) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kproc(self);
 error = kmmutex_locks(&self->p_lock,KPROC_LOCK_MODS);
 if __unlikely(KE_ISERR(error)) return error;
 error = krwlock_beginwrite(&self->p_shm.s_lock);
 if __unlikely(KE_ISERR(error)) goto end_mutex;
 error = kproc_insmod_unlocked(self,module,module_id);
 krwlock_endwrite(&self->p_shm.s_lock);
end_mutex:
 kmmutex_unlocks(&self->p_lock,KPROC_LOCK_MODS);
 return error;
}
__crit kerrno_t
kproc_delmod_c(struct kproc *__restrict self,
               kmodid_t module_id) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kproc(self);
 error = kmmutex_locks(&self->p_lock,KPROC_LOCK_MODS);
 if __unlikely(KE_ISERR(error)) return error;
 error = krwlock_beginwrite(&self->p_shm.s_lock);
 if __unlikely(KE_ISERR(error)) goto end_mutex;
 error = kproc_delmod_unlocked(self,module_id);
 krwlock_endwrite(&self->p_shm.s_lock);
end_mutex:
 kmmutex_unlocks(&self->p_lock,KPROC_LOCK_MODS);
 return error;
}

static __crit kerrno_t
kproc_delmodat_c(struct kproc *__restrict self, __user void *addr) {
 kerrno_t error; kmodid_t id;
 KTASK_CRIT_MARK
 kassert_kproc(self);
 error = kmmutex_locks(&self->p_lock,KPROC_LOCK_MODS);
 if __unlikely(KE_ISERR(error)) return error;
 error = krwlock_beginwrite(&self->p_shm.s_lock);
 if __unlikely(KE_ISERR(error)) goto end_mutex;
 if __likely(KE_ISOK(error = kproc_getmodat_unlocked(self,&id,addr))) {
  error = kproc_delmod_unlocked(self,id);
 }
 krwlock_endwrite(&self->p_shm.s_lock);
end_mutex:
 kmmutex_unlocks(&self->p_lock,KPROC_LOCK_MODS);
 return error;
}

__local kmodid_t
kproc_getnextmodid_unlocked(struct kproc const *__restrict self,
                            kmodid_t id) {
 for (;;) {
  if (++id >= self->p_modules.pms_moda) id = 0;
  if (self->p_modules.pms_modv[id].pm_lib) break;
 }
 return id;
}

static __crit kerrno_t
kproc_delmodafter_c(struct kproc *__restrict self, __user void *addr) {
 kerrno_t error; kmodid_t id;
 KTASK_CRIT_MARK
 kassert_kproc(self);
 error = kmmutex_locks(&self->p_lock,KPROC_LOCK_MODS);
 if __unlikely(KE_ISERR(error)) return error;
 error = krwlock_beginwrite(&self->p_shm.s_lock);
 if __unlikely(KE_ISERR(error)) goto end_mutex;
 if __likely(KE_ISOK(error = kproc_getmodat_unlocked(self,&id,addr))) {
  id = kproc_getnextmodid_unlocked(self,id);
  error = kproc_delmod_unlocked(self,id);
 }
 krwlock_endwrite(&self->p_shm.s_lock);
end_mutex:
 kmmutex_unlocks(&self->p_lock,KPROC_LOCK_MODS);
 return error;
}

static __crit kerrno_t
kproc_delmod_all(struct kproc *__restrict self) {
 kerrno_t error; kmodid_t i;
 int error_occurred;
 error = kmmutex_locks(&self->p_lock,KPROC_LOCK_MODS);
 if __unlikely(KE_ISERR(error)) return error;
 error = krwlock_beginwrite(&self->p_shm.s_lock);
 if __unlikely(KE_ISERR(error)) goto end_mutex;
 for (;;) {
  error_occurred = 0;
  if (!self->p_modules.pms_moda) break;
  for (i = self->p_modules.pms_moda; i; ++i) {
   error = kproc_delmod_unlocked(self,i);
   if (KE_ISERR(error) && error != KE_INVAL) error_occurred = 1;
  }
  if (error_occurred) break;
 }
 krwlock_endwrite(&self->p_shm.s_lock);
end_mutex:
 kmmutex_unlocks(&self->p_lock,KPROC_LOCK_MODS);
 return error;
}

__crit kerrno_t
kproc_delmod2_c(struct kproc *__restrict self,
                 kmodid_t module_id, __user void *caller_eip) {
 switch (module_id) {
  case KMODID_ALL:  return kproc_delmod_all(self);
  case KMODID_NEXT: return kproc_delmodafter_c(self,caller_eip);
  case KMODID_SELF: return kproc_delmodat_c(self,caller_eip);
  default:          return kproc_delmod_c(self,module_id);
 }
}


__crit kerrno_t
kproc_getmodat_unlocked(struct kproc *__restrict self,
                        kmodid_t *__restrict module_id,
                        __user void *addr) {
 struct kprocmodule *iter,*end;
 kassert_kproc(self);
 assert(kproc_islockeds(self,KPROC_LOCK_MODS));
 end = (iter = self->p_modules.pms_modv)+self->p_modules.pms_moda;
 for (; iter != end; ++iter) if (iter->pm_lib && (uintptr_t)addr >= (uintptr_t)iter->pm_base) {
  uintptr_t offset_addr = (uintptr_t)addr-(uintptr_t)iter->pm_base;
  struct kshlib *lib = iter->pm_lib;
  if (offset_addr >= lib->sh_data.ed_begin &&
      offset_addr < lib->sh_data.ed_end) {
   *module_id = (kmodid_t)(iter-self->p_modules.pms_modv);
   return KE_OK;
  }
 }
 return KE_FAULT;
}



__crit __user void *
kproc_dlsymex_unlocked(struct kproc *__restrict self,
                       kmodid_t module_id, char const *__restrict name,
                       size_t name_size, ksymhash_t name_hash) {
 struct kprocmodule *module; kmodid_t *iter,*end; __user void *result;
 struct ksymbol const *symbol; struct kshlib *lib;
 KTASK_CRIT_MARK
 kassert_kproc(self);
 if __unlikely(module_id >= self->p_modules.pms_moda) return NULL;
 module = &self->p_modules.pms_modv[module_id];
 if __unlikely((lib = module->pm_lib) == NULL) return NULL;
 if ((symbol = ksymtable_lookupname_h(&lib->sh_publicsym,name,name_size,name_hash)) != NULL &&
     symbol->s_shndx != SHN_UNDEF) goto found_symbol;
 end = (iter = module->pm_depids)+lib->sh_deps.sl_libc;
 for (; iter != end; ++iter) {
  result = kproc_dlsymex_unlocked(self,*iter,name,
                                  name_size,name_hash);
  if (result != NULL) return result;
 }
 if ((symbol = ksymtable_lookupname_h(&lib->sh_weaksym,name,name_size,name_hash)) != NULL &&
     symbol->s_shndx != SHN_UNDEF) goto found_symbol;
 return NULL;
found_symbol:
#if 0
 printf("Found symbol: %.?q @ %Iu\n",
        name_size,name,symbol->s_addr);
#endif
 return (__user void *)((uintptr_t)module->pm_base+symbol->s_addr);
}

__crit __user void *
kproc_dlsymex_c(struct kproc *__restrict self,
                kmodid_t module_id, char const *__restrict name,
                size_t name_size, ksymhash_t name_hash) {
 void *result;
 KTASK_CRIT_MARK
 kassert_kproc(self);
 if __unlikely(KE_ISERR(kproc_lock(self,KPROC_LOCK_MODS))) return NULL;
 result = kproc_dlsymex_unlocked(self,module_id,name,name_size,name_hash);
 kproc_unlock(self,KPROC_LOCK_MODS);
 return result;
}

__crit __user void *
kproc_dlsymall_c(struct kproc *__restrict self, char const *__restrict name,
                 size_t name_size, ksymhash_t name_hash) {
 __user void *result;
 kmodid_t check_id = self->p_modules.pms_moda;
 while (check_id--) {
  result = kproc_dlsymex_unlocked(self,check_id,name,name_size,name_hash);
  if (result != NULL) return result;
 }
 return NULL;
}

__crit __user void *
kproc_dlsymself_c(struct kproc *__restrict self, char const *__restrict name,
                  size_t name_size, ksymhash_t name_hash,
                  __user void *caller_eip) {
 kmodid_t modid;
 if __unlikely(KE_ISERR(kproc_getmodat_unlocked(self,&modid,caller_eip))) return NULL;
 return kproc_dlsymex_unlocked(self,modid,name,name_size,name_hash);
}

__crit __user void *
kproc_dlsymnext_c(struct kproc *__restrict self, char const *__restrict name,
                  size_t name_size, ksymhash_t name_hash,
                  __user void *caller_eip) {
 kmodid_t first_id,modid,maxid; __user void *result;
 if __unlikely(KE_ISERR(kproc_getmodat_unlocked(self,&first_id,caller_eip))) return NULL;
 maxid = self->p_modules.pms_moda;
 /* Search above. */
 for (modid = first_id+1; modid < maxid; ++modid) {
  result = kproc_dlsymex_unlocked(self,modid,name,name_size,name_hash);
  if (result != NULL) return result;
 }
 /* Search below. */
 if (first_id != 0) {
  modid = first_id-1;
  for (;;) {
   result = kproc_dlsymex_unlocked(self,modid,name,name_size,name_hash);
   if (result != NULL) return result;
   if (!modid) break;
   --modid;
  }
 }
 /* Search self. */
 return kproc_dlsymex_unlocked(self,first_id,name,name_size,name_hash);
}

__crit __user void *
kproc_dlsymex2_c(struct kproc *__restrict self,
                 kmodid_t module_id, char const *__restrict name,
                 size_t name_size, ksymhash_t name_hash,
                 __user void *caller_eip) {
 void *result;
 KTASK_CRIT_MARK
 kassert_kproc(self);
 if __unlikely(KE_ISERR(kproc_lock(self,KPROC_LOCK_MODS))) return NULL;
 switch (module_id) {
  case KMODID_ALL:  result = kproc_dlsymall_c(self,name,name_size,name_hash); break;
  case KMODID_NEXT: result = kproc_dlsymnext_c(self,name,name_size,name_hash,caller_eip); break;
  case KMODID_SELF: result = kproc_dlsymself_c(self,name,name_size,name_hash,caller_eip); break;
  default:          result = kproc_dlsymex_unlocked(self,module_id,name,name_size,name_hash); break;
 }
 kproc_unlock(self,KPROC_LOCK_MODS);
 return result;
}


__DECL_END

#endif /* !__KOS_KERNEL_SCHED_PROC_INSMOD_C_INL__ */

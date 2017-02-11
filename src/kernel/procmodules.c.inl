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
#ifndef __KOS_KERNEL_PROCMODULES_C_INL__
#define __KOS_KERNEL_PROCMODULES_C_INL__ 1

#include <assert.h>
#include <kos/compiler.h>
#include <kos/config.h>
#include <kos/kernel/linker.h>
#include <kos/kernel/procmodules.h>
#include <malloc.h>

__DECL_BEGIN

kerrno_t
kprocmodules_initcopy(struct kprocmodules *__restrict self,
                      struct kprocmodules const *__restrict right) {
 kerrno_t error;
 kassertobj(self);
 kassert_kprocmodules(right);
 kobject_init(self,KOBJECT_MAGIC_PROCMODULES);
 assert((right->pms_moda != 0) == (right->pms_modv != NULL));
 if ((self->pms_moda = right->pms_moda) != 0) {
  struct kprocmodule *iter,*end,*src;
  self->pms_modv = (struct kprocmodule *)malloc(right->pms_moda*
                                                sizeof(struct kprocmodule));
  if __unlikely(!self->pms_modv) return KE_NOMEM;
  end = (iter = self->pms_modv)+self->pms_moda;
  src = right->pms_modv;
  for (; iter != end; ++iter,++src) {
   kobject_init(iter,KOBJECT_MAGIC_PROCMODULE);
   kassert_kshlib(src->pm_lib);
   if ((iter->pm_lib = src->pm_lib) != NULL) {
    if __unlikely(KE_ISERR(error = kshlib_incref(iter->pm_lib))) {
err_iter:
     while (iter-- != self->pms_modv) if (iter->pm_lib) {
      kshlib_decref(iter->pm_lib);
      free(iter->pm_depids);
     }
     free(self->pms_modv);
     return error;
    }
    if (src->pm_depids) {
     iter->pm_depids = (kmodid_t *)memdup(src->pm_depids,
                                          src->pm_lib->sh_deps.sl_libc*
                                          sizeof(kmodid_t));
     if __unlikely(!iter->pm_depids) {
      error = KE_NOMEM;
      kshlib_decref(iter->pm_lib);
      goto err_iter;
     }
    } else iter->pm_depids = NULL;
    iter->pm_loadc = src->pm_loadc;
    iter->pm_flags = src->pm_flags;
    iter->pm_base = src->pm_base;
   } else {
    kobject_badmagic(iter);
#ifdef __DEBUG__
    iter->pm_depids = (kmodid_t *)(uintptr_t)-1;
    iter->pm_loadc = 0;
    iter->pm_flags = KPROCMODULE_FLAG_NONE;
    iter->pm_base = (__user void *)(uintptr_t)-1;
#endif
   }
  }
 } else {
  self->pms_modv = NULL;
 }
 return KE_OK;
}

void kprocmodules_quit(struct kprocmodules *__restrict self) {
 struct kprocmodule *iter,*end;
 kassert_kprocmodules(self);
 end = (iter = self->pms_modv)+self->pms_moda;
 for (; iter != end; ++iter) if (iter->pm_lib) {
  // TODO: Execute finalization callbacks
  kshlib_decref(iter->pm_lib);
  free(iter->pm_depids);
 }
 free(self->pms_modv);
}


__DECL_END

#endif /* !__KOS_KERNEL_PROCMODULES_C_INL__ */

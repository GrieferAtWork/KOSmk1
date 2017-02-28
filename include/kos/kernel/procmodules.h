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
#ifndef __KOS_KERNEL_PROCMODULES_H__
#define __KOS_KERNEL_PROCMODULES_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/compiler.h>
#include <kos/types.h>
#include <kos/kernel/object.h>
#include <kos/kernel/paging.h>

__DECL_BEGIN

/* Process modules (aka.: How KOS tracks shared libraries loaded into processes)
 * NOTE: These are mostly data structures. - The real magic happens in "/src/kernel/linker/*" */

#define KOBJECT_MAGIC_PROCMODULE  0x960C40D  /*< PROCMOD */
#define KOBJECT_MAGIC_PROCMODULES 0x960C40D5 /*< PROCMODS */

#define kassert_kprocmodule(self)  kassert_object(self,KOBJECT_MAGIC_PROCMODULE)
#define kassert_kprocmodules(self) kassert_object(self,KOBJECT_MAGIC_PROCMODULES)

#ifndef __ASSEMBLY__
struct kshlib;
struct kprocmodule {
 KOBJECT_HEAD
 __u32                      pm_loadc;   /*< Amount of times this module was loaded (When this reaches ZERO(0), the module unloaded). */
#define KPROCMODULE_FLAG_NONE  0x00000000
#define KPROCMODULE_FLAG_RELOC 0x00000001 /*< [set_once] Module was relocated. */
 __u32                      pm_flags;   /*< Module flags. */
 __ref struct kshlib       *pm_lib;     /*< [0..1] Shared library associated with this module (NULL for unloaded modules). */
union{
 __pagealigned __user void *pm_base;    /*< The base address at which this module is mapped. */
 __pagealigned __user __u32 pm_base32;  /*< The base address (32-bit mode only). */
};
 kmodid_t                  *pm_depids;  /*< [0..pm_lib->sh_deps.sl_libc][owned] Vector of module ids for dependencies. */
};

//////////////////////////////////////////////////////////////////////////
// Translate a shared library symbol address into a virtual user-space address.
#define kprocmodule_translate(self,symaddr) \
 ((__user void *)((__uintptr_t)(self)->pm_base+(__uintptr_t)(symaddr)))


struct kprocmodules {
 KOBJECT_HEAD
 __size_t            pms_moda; /*< Allocated module vector size. */
 struct kprocmodule *pms_modv; /*< [0..pms_moda][owned] Vector of loaded modules. */
};

//////////////////////////////////////////////////////////////////////////
// Initializes a given process module list as a copy of another.
// NOTE: This function does not re-setup any modules and should
//       only be called when the caller has already set up the correct
//       module environment, such as during a fork() operation.
// @return: KE_OK:       The operation completed successfully.
// @return: KE_OVERFLOW: The reference counter of a module would have overflown.
// @return: KE_NOMEM:    Not enough available kernel memory.
extern __wunused __nonnull((1,2)) kerrno_t
kprocmodules_initcopy(struct kprocmodules *__restrict self,
                      struct kprocmodules const *__restrict right);

#ifdef __INTELLISENSE__
extern __nonnull((1)) void kprocmodules_init(struct kprocmodules *__restrict self);
#else
#define kprocmodules_init(self) \
 kobject_initzero(self,KOBJECT_MAGIC_PROCMODULES)
#endif
extern __nonnull((1)) void kprocmodules_quit(struct kprocmodules *__restrict self);
#endif /* !__ASSEMBLY__ */

__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_PROCMODULES_H__ */

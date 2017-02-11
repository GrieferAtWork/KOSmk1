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
#ifndef __KOS_KERNEL_SHM_H__
#define __KOS_KERNEL_SHM_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <sys/mman.h>
#include <kos/compiler.h>
#include <kos/types.h>
#include <kos/errno.h>
#include <kos/kernel/features.h>
#include <kos/kernel/object.h>
#include <kos/kernel/paging.h>

// Shared memory layout
__DECL_BEGIN

#define MAP_FAIL   ((void *)(uintptr_t)-1)

#define KOBJECT_MAGIC_SHMTAB  0x5177AB  /*< SHMTAB */
#define KOBJECT_MAGIC_SHM     0x517     /*< SHM */
#define kassert_kshmtab(self)  kassert_refobject(self,mt_refcnt,KOBJECT_MAGIC_SHMTAB)
#define kassert_kshm(self)     kassert_object(self,KOBJECT_MAGIC_SHM)


// NOTE: Dependent on the arch, some flags may not be enforced
#define KSHMTAB_FLAG_X PROT_EXEC  /*< Data inside of this tab can be executed. */
#define KSHMTAB_FLAG_W PROT_WRITE /*< The tab is writable. */
#define KSHMTAB_FLAG_R PROT_READ  /*< The tab is readable. */
#define KSHMTAB_FLAG_S 0x08       /*< The tab can be shared between processes. */
#define KSHMTAB_FLAG_K 0x10       /*< The tab can only be accessed by the kernel (Usually means this is a kernel stack). */
#define KSHMTAB_FLAG_D 0x20       /*< The tab references device memory that should not be freed. */

struct kfile;

struct kshmtabscatter {
 struct kshmtabscatter *ts_next;  /*< [0..1][owned] Next scatter entry. */
 __kernel void         *ts_addr;  /*< [1..1][owned] Physical (kernel) address of this scatter portion. */
               __size_t ts_pages; /*< [!0] Amount of pages in this scatter. */
};

//////////////////////////////////////////////////////////////////////////
// Initialize a shared-memory-tab scatter entry with a given amount of pages.
// NOTE: Linear memory refers to continuous physical memory, meaning
//       that the physical address of 32 linear allocated pages can quickly
//       be calculated from the virtual address by adding a constant offset
//       to the address. (aka. 'function phys_of(virt) -> get_base()+virt')
// @return: KE_OK:    The given scatter tab was successfully initialized, and is now holding exactly 'pages' pages.
// @return: KE_NOMEM: Not enough memory (NOTE: non-linear allocation may succeed where linear fails)
extern __wunused __nonnull((1)) kerrno_t kshmtabscatter_init(struct kshmtabscatter *__restrict self, __size_t pages);
extern __wunused __nonnull((1)) kerrno_t kshmtabscatter_init_linear(struct kshmtabscatter *__restrict self, __size_t pages);
extern           __nonnull((1))     void kshmtabscatter_quit(struct kshmtabscatter *__restrict self);

//////////////////////////////////////////////////////////////////////////
// Returns TRUE(1) if the given 'addr' is part of the
// physical memory described by 'self'. FALSE(0) otherwise.
extern __wunused __nonnull((1)) int
kshmtabscatter_contains(struct kshmtabscatter *__restrict self,
                        __kernel void *addr);

//////////////////////////////////////////////////////////////////////////
// Copy the data between two given SHM memory tabs.
// NOTE: The caller must ensure that both tabs describe the same amount of pages
extern __nonnull((1,2)) void
kshmtabscatter_copybytes(struct kshmtabscatter const *__restrict dst,
                         struct kshmtabscatter const *__restrict src);


struct kshmtab {
 KOBJECT_HEAD
 __atomic __u32        mt_refcnt; /*< Amount of processes using this tab NOTE: Owns a reference to 'mt_refcnt' (When >1, KSHMTAB_FLAG_W and !KSHMTAB_FLAG_S, write-access must cause a pagefault for copy-on-write). */
          __u32        mt_flags;  /*< Tab flags (Set of 'KSHMTAB_FLAG_*'). */
          __size_t     mt_pages;  /*< Total amount of pages (sum of all scatter entries). */
 struct kshmtabscatter mt_scat;   /*< First scatter entry. */
};

__local KOBJECT_DEFINE_INCREF(kshmtab_incref,struct kshmtab,mt_refcnt,kassert_kshmtab);
__local KOBJECT_DEFINE_DECREF(kshmtab_decref,struct kshmtab,mt_refcnt,kassert_kshmtab,kshmtab_destroy);


//////////////////////////////////////////////////////////////////////////
// Allocates new shared memory tab.
// NOTE: 'kshmtab_copy' will also copy all stored data into the new & returned tab upon success.
// @return: NULL: Failed to allocate the new tab due to too little memory being available.
// @return: * :   A new SHM tab with its reference/user counter set to ONE(1).
extern __malloccall __wunused __ref struct kshmtab *kshmtab_newram(__size_t pages, __u32 flags);
extern __malloccall __wunused __ref struct kshmtab *kshmtab_newram_linear(__size_t pages, __u32 flags);
extern __malloccall __wunused __nonnull((1)) __ref struct kshmtab *kshmtab_newfp(struct kfile *__restrict fp, __u64 offset, __size_t bytes, __u32 flags);
extern __malloccall __wunused __ref struct kshmtab *kshmtab_newdev(__pagealigned __kernel void *start, __size_t pages, __u32 flags);
extern __malloccall __wunused __nonnull((1)) __ref struct kshmtab *kshmtab_copy(struct kshmtab const *__restrict self);

//////////////////////////////////////////////////////////////////////////
// Ensure of uniqueness of '*self' by creating a copy
// if the reference counter is greater than ONE(1).
// @return: KE_OK:        A copy was successfully created and stored in '*self'.
// @return: KS_UNCHANGED: No copy had to be created (the caller already was the owner).
// @return: KE_NOMEM:     Not enough memory to create a copy.
extern __wunused __nonnull((1)) kerrno_t
kshmtab_unique(__ref struct kshmtab **__restrict self);

//////////////////////////////////////////////////////////////////////////
// Returns a set of flags to be used when mapping this tab into a page directory
extern __wunused __nonnull((1)) __kpageflag_t
kshmtab_getpageflags(struct kshmtab const *__restrict self);

#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Translate a given offset (starting at ZERO(0)) within a
// shared memory tab into its physical address counterpart.
// @param: maxbytes: Filled with the max amount of consecutive bytes available
//                   before the end of the tab is reached, or another scatter
//                   entry starts. (aka. When you have to re-translate a new offset)
// @return: NULL: The given offset lies outside of valid TAB boundaries.
extern __kernel void *kshmtab_translate_offset(struct kshmtab const *__restrict self,
                                               __uintptr_t offset, size_t *maxbytes);
#else
extern __kernel void *__kshmtab_translate_offset(struct kshmtab const *__restrict self,
                                                 __uintptr_t offset, size_t *maxbytes);
// Special optimization for offset ZERO(0)
#define kshmtab_translate_offset(self,offset,maxbytes) \
 ((__builtin_constant_p(offset) && (offset) == 0)\
  ? (*(maxbytes) = (self)->mt_scat.ts_pages*PAGESIZE,(self)->mt_scat.ts_addr)\
  : __kshmtab_translate_offset(self,offset,maxbytes))

#endif

#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Returns TRUE(1) if the given 'addr' is part of the
// physical memory described by 'self'. FALSE(0) otherwise.
extern int kshmtab_contains(struct kshmtab const *__restrict self,
                            __kernel void const *addr);
#else
#define kshmtab_contains(self,addr) kshmtabscatter_contains(&(self)->mt_scat,addr)
#endif

struct kshmtabentry {
 __pagealigned __user void *te_map;   /*< [1..1] User-space address this tab is mapped to. */
 __u32                      te_flags; /*< Set of (MAP_PRIVATE|MAP_SHARED). */
 __ref struct kshmtab      *te_tab;   /*< [1..1] Referenced tab. */
};

#define KSHM_GROUPCOUNT                 16
#define KSHM_GROUPOF(uaddr)     (__u8)((__uintptr_t)(uaddr) >> ((__SIZEOF_POINTER__*8)-4))

struct kpagedir;
struct kshm {
 KOBJECT_HEAD
 // Per-process shared memory manager
 // NOTE: To allow for fast forking, all userland memory can be shared
 struct kpagedir     *sm_pd;   /*< [1..1][owned] Page directory associated with this process. */
 // NOTE: The vector is sorted ascending by virtual address mappings.
 __size_t             sm_tabc; /*< Amount of used shared memory tabs. */
 struct kshmtabentry *sm_tabv; /*< [1..1][0..tv_tabc][owned] Vector of shm-tabs (Sorted by virtual user address). */
 struct kshmtabentry *sm_groups[KSHM_GROUPCOUNT];
};
#define KSHM_INITROOT(pagedir) {KOBJECT_INIT(KOBJECT_MAGIC_SHM) pagedir,0,NULL,{NULL,}}

//////////////////////////////////////////////////////////////////////////
// Initialize/Finalize a given SHM manager.
// NOTE: 'kshm_init' can fail because it must allocate a new page directory.
// @return: KE_NOMEM: Not enough memory to allocate a new page directory.
extern __wunused __nonnull((1)) kerrno_t kshm_init(struct kshm *__restrict self);
extern           __nonnull((1)) void kshm_quit(struct kshm *__restrict self);

//////////////////////////////////////////////////////////////////////////
// Initializes a given SHM as a copy of another.
// >> This function re-configures paging to do all the copy-on-write magic.
// NOTE: When calling 'kshm_initfork', SHM tabs will usually be re-used
//       and marked as read-only to allow a PAGE-FAULT handler to
//       perform copy-on-write, thus saving a lot of memory is the process.
//    >> 'kshm_initcopy' bypasses this functionality,
//       always creating hard copies of all stored SHM tabs.
// @return: KE_NOMEM: Not enough memory (can still happen for control structures)
extern __wunused __nonnull((1,2)) kerrno_t
kshm_initcopy(struct kshm *__restrict self,
              struct kshm *__restrict right);
#ifdef KCONFIG_HAVE_SHM_COPY_ON_WRITE
extern __wunused __nonnull((1,2)) kerrno_t
kshm_initfork(struct kshm *__restrict self,
              struct kshm *__restrict right);
#else
#define kshm_initfork   kshm_initcopy
#endif

//////////////////////////////////////////////////////////////////////////
// Insert a given shared memory tab.
// @return: KE_OK:    The given tab was successfully inserted and mapped
//                   (The caller no longer owns a reference).
// @return: KE_RANGE: flags&MAP_FIXED: The given range is already mapped.
// @return: KE_NOSPC: !(flags&MAP_FIXED): Failed to find an unused region of sufficient size.
// @return: KE_NOMEM: Not enough memory
extern __crit __wunused __nonnull((1,2)) kerrno_t
kshm_instab_inherited(struct kshm *__restrict self,
                      struct kshmtab *__restrict tab,
                      int flags, __user void *hint,
                      __user void **mapped_address);

//////////////////////////////////////////////////////////////////////////
// Insert a given shared memory tab.
// @return: KE_NOENT: The given tab isn't mapped
// NOTE: After calling 'kshm_deltab', the caller must still decref 'tab'
extern __crit __wunused __nonnull((1,2)) kerrno_t
kshm_deltab(struct kshm *__restrict self,
            struct kshmtab *__restrict tab,
            __user void *uaddr);
extern __crit __nonnull((1,2)) void
kshm_delslot(struct kshm *__restrict self,
             struct kshmtabentry *__restrict slot);

//////////////////////////////////////////////////////////////////////////
// Returns the tab associated with a given virtual address.
// @return: NULL: The given address is not associated with any tab.
extern __wunused __nonnull((1)) struct kshmtab *
kshm_gettab(struct kshm *__restrict self, __user void *uaddr);
extern __wunused __nonnull((1)) struct kshmtabentry *
kshm_getentry(struct kshm *__restrict self, __user void *uaddr);

//////////////////////////////////////////////////////////////////////////
// Maps memory to the given SHM manager, allocating new tabs along the way.
// @return: MAP_FAIL: Not enough memory
extern __crit __wunused __nonnull((1)) __user void *
kshm_mmap(struct kshm *__restrict self, __user void *hint, __size_t len,
          int prot, int flags, struct kfile *fp, __u64 off);
extern __crit __wunused __nonnull((1)) __user void *
kshm_mmapram(struct kshm *__restrict self, __user void *hint,
             __size_t len, int prot, int flags);
extern __crit __wunused __nonnull((1,2)) kerrno_t
kshm_mmapdev(struct kshm *__restrict self,
             void __user * __kernel *hint_and_result,
             __size_t len, int prot, int flags,
             __kernel void *phys_ptr);
extern __crit __wunused __nonnull((1,6)) __user void *
kshm_mmap_linear(struct kshm *__restrict self, __user void *hint,
                 __size_t len, int prot, int flags,
                 __kernel void **__restrict kaddr);


//////////////////////////////////////////////////////////////////////////
// Returns KE_OK If the calling task is allowed
// to mmap the given physical address range.
// Returns some error otherwise
extern kerrno_t kshm_allow_devmap(uintptr_t addr, __size_t pages);


//////////////////////////////////////////////////////////////////////////
// Unmaps a given area of memory, decref-ing any tabs that
// used to be located within the given area (addr..addr+length).
// NOTE: Tabs that only intersect partially with the given range
//       are fully deallocated (No splicing of tabs is performed).
// @return: ZERO(0): No tabs were found in the given range (potentially indicates an invalid range)
// @return: * :      The amount of unmapped tabs
extern __crit __nonnull((1)) __size_t
kshm_munmap(struct kshm *__restrict self, __user void *addr,
            __size_t length, int allow_kernel);

// TODO: mremap(...)

#ifdef __INTELLISENSE__
extern struct kshm *kshm_kernel(void); /*< Macro in <kos/kernel/proc.h> */
#endif

extern           __nonnull((1,3)) __size_t __kshm_memcpy_k2u(struct kshm const *__restrict self, __user void *dst, __kernel void const *__restrict src, __size_t bytes);
extern __wunused __nonnull((1,2)) __size_t __kshm_memcpy_u2k(struct kshm const *__restrict self, __kernel void *__restrict dst, __user void const *src, __size_t bytes);
extern           __nonnull((1))   __size_t __kshm_memcpy_u2u(struct kshm const *__restrict self, __user void *dst, __user void const *src, __size_t bytes);
extern __wunused __nonnull((1,3)) __kernel void *__kshm_translate_1(struct kshm const *__restrict self, __user void const *addr, /*out*/__size_t *__restrict max_bytes, int read_write);
extern __wunused __nonnull((1,3)) __kernel void *__kshm_translate_u(struct kshm const *__restrict self, __user void const *addr, /*inout*/__size_t *__restrict max_bytes, int read_write);

#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Copy memory within user-space.
// NOTE: The caller is responsible to ensure that no pages
//       possibly affected can be unmapped from the SHM.
// @return: Amount of transferred bytes (If != bytes, assume KE_FAULT)
extern           __nonnull((1,3)) __size_t kshm_memcpy_k2u(struct kshm const *__restrict self, __user void *dst, __kernel void const *__restrict src, __size_t bytes);
extern __wunused __nonnull((1,2)) __size_t kshm_memcpy_u2k(struct kshm const *__restrict self, __kernel void *__restrict dst, __user void const *src, __size_t bytes);
extern           __nonnull((1))   __size_t kshm_memcpy_u2u(struct kshm const *__restrict self, __user void *dst, __user void const *src, __size_t bytes);
extern __wunused __nonnull((1,3)) __kernel void *kshm_translate_1(struct kshm const *__restrict self, __user void const *addr, /*out*/__size_t *__restrict max_bytes, int read_write);
extern __wunused __nonnull((1,3)) __kernel void *kshm_translate_u(struct kshm const *__restrict self, __user void const *addr, /*inout*/__size_t *__restrict max_bytes, int read_write);
#endif

__DECL_END

#ifndef __INTELLISENSE__
#ifndef __KOS_KERNEL_PROC_H__
#include <kos/kernel/proc.h>
#endif
#endif

#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_SHM_H__ */

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
#ifndef __KOS_KERNEL_PAGING_H__
#define __KOS_KERNEL_PAGING_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/arch.h>
#include <kos/compiler.h>
#include <kos/errno.h>
#include <kos/types.h>
#include <math.h>
#include <string.h>
#ifndef __INTELLISENSE__
#include <malloc.h>
#endif

#ifdef __x86__
#include <kos/arch/x86/mmu.h>
#endif

__DECL_BEGIN

#define PAGESIZE  4096 /*< Size of smallest amount of memory individually mappable through paging. */
#define PAGEALIGN PAGESIZE

#define __pagealigned    __aligned(PAGEALIGN)

#if (PAGESIZE%PAGEALIGN) != 0
#error "Pages must be aligned when packed together"
#endif


#define PAGENIL   ((void *)(uintptr_t)-1)



//////////////////////////////////////////////////////////////////////////
// Page directory
//////////////////////////////////////////////////////////////////////////

#ifdef __x86__
#if ((~(X86_PTE_MASK_PPTR))&0xffffffff)+1 != PAGEALIGN
#error "i386/x86-64 requires 4096 as page size"
#endif
#ifndef __ASSEMBLY__
#ifndef __kpageflag_t_defined
#define __kpageflag_t_defined 1
typedef __kpageflag_t kpageflag_t;
#endif
typedef x86_pte kpage_t;
struct __packed kpagedir { x86_pde d_entries[X86_PDE4DIC]; };
#endif /* !__ASSEMBLY__ */
#define PAGEDIR_FLAG_USER       X86_PTE_FLAG_USER
#define PAGEDIR_FLAG_READ_WRITE X86_PTE_FLAG_READ_WRITE
#define PAGEDIR_FLAG_MASK    (~(X86_PTE_MASK_PPTR))
#endif

#ifdef __arch_enablepaging
#define kpaging_enable             __arch_enablepaging
#endif
#ifdef __arch_disablepaging
#define kpaging_disable            __arch_disablepaging
#endif
#ifdef __arch_pagingenabled
#define kpaging_enabled            __arch_pagingenabled
#endif
#ifdef __arch_getpagedirectory
#define kpagedir_current()        (struct kpagedir *)__arch_getpagedirectory()
#endif
#ifdef __arch_setpagedirectory
#define kpagedir_makecurrent(self) __arch_setpagedirectory(&(self)->d_entries)
#endif

#ifndef __ASSEMBLY__
//////////////////////////////////////////////////////////////////////////
// Allocate a new page directory
// @return: * :      A pointer to the newly allocated page directory.
// @return: PAGENIL: Not enough available physical memory.
extern __crit struct kpagedir *kpagedir_new(void);
extern __crit void kpagedir_delete(struct kpagedir *__restrict self);

//////////////////////////////////////////////////////////////////////////
// Map a given physical address range to its virtual counterpart
// @param: phys:  The physical address to map (must be page-aligned; PAGEALIGN)
// @param: virt:  The virtual address to map (must be page-aligned; PAGEALIGN)
// @param: pages: The amount of consecutive pages to map
//                NOTE: A single page has a size of 'KPAGEDIR_PAGESIZE'
// @param: flags: set of 'PAGEDIR_FLAG_*'
// @return: KE_OK:      Successfully (re-)mapped the given area of virtual memory.
// @return: KE_NOMEM:   No enough available memory.
// @return: KE_EXISTS: [kpagedir_map] The given area (or a part of it) was already mapped.
extern __crit __nonnull((1)) kerrno_t
kpagedir_map(struct kpagedir *__restrict self,
             __kernel void const *phys,
             __user void const *virt,
             __size_t pages, kpageflag_t flags);
extern __crit __nonnull((1)) kerrno_t
kpagedir_remap(struct kpagedir *__restrict self,
               __kernel void const *phys,
               __user void const *virt,
               __size_t pages, kpageflag_t flags);

//////////////////////////////////////////////////////////////////////////
// Modifies the flags of all pages within a given address range.
// @return: * : Amount of successfully modified pages.
extern __nonnull((1)) __size_t
kpagedir_setflags(struct kpagedir *__restrict self,
                  __user void const *virt,
                  __size_t pages, kpageflag_t mask,
                  kpageflag_t flags);

//////////////////////////////////////////////////////////////////////////
// Find an unmapped range bit enough to map at least 'pages' pages.
// NOTE: Will never map the NULL page
// @return: * :                         The virtual address the given physical range was mapped to.
// @return: KPAGEDIR_FINDFREERANGE_ERR: Failed to find an unused are big enough to map the given range.
extern __nonnull((1)) __user void *
kpagedir_findfreerange(struct kpagedir const *__restrict self,
                       __size_t pages, __user void const *hint);
#define KPAGEDIR_FINDFREERANGE_ERR ((__user void *)(__uintptr_t)-1)

//////////////////////////////////////////////////////////////////////////
// Map the given physical address range to the first available free virtual range
// NOTE: Will never map the NULL page
// @param: phys:  The physical address to map (must be page-aligned; PAGEALIGN)
// @param: flags: Set of flags masked by 'PAGEDIR_FLAG_MASK'
// @return: * :   The virtual address the given physical range was mapped to.
// @return: NULL: Failed to find an unused are big enough to
//                map the given range, or not enough memory.
extern __nonnull((1,2)) __user void *
kpagedir_mapanyex(struct kpagedir *__restrict self, __kernel void const *phys,
                  __size_t pages, __user void const *hint, kpageflag_t flags);

#define KPAGEDIR_MAPANY_HINT_UDEV     ((void *)0x20000000) /*< mmap_dev-mapped device memory. */
#define KPAGEDIR_MAPANY_HINT_UHEAP    ((void *)0x40000000) /*< User-space heap. */
#define KPAGEDIR_MAPANY_HINT_USTACK   ((void *)0x80000000) /*< User-space stack. */
#define KPAGEDIR_MAPANY_HINT_LIBS     ((void *)0xa0000000) /*< Shared libraries. */
#define KPAGEDIR_MAPANY_HINT_TLS      ((void *)0xd0000000) /*< User-space TLS memory. */
#define KPAGEDIR_MAPANY_HINT_KINTERN  ((void *)0xe0000000) /*< Various internal kernel-specific mappings (e.g.: LDT vectors). */
#define KPAGEDIR_MAPANY_HINT_KSTACK   ((void *)0xf0000000) /*< Kernel-space stack. */

//////////////////////////////////////////////////////////////////////////
// Get attributes amount of a given range of virtually mapped memory.
// @return: * : A set of 'KPAGEDIR_RANGEATTR_*'
extern __nonnull((1)) int
kpagedir_rangeattr(struct kpagedir const *__restrict self,
                   __user void const *virt,
                   __size_t pages);
#define KPAGEDIR_RANGEATTR_NONE         0
#define KPAGEDIR_RANGEATTR_HASMAPPED    1 /*< There are mapped pages within the given range. */
#define KPAGEDIR_RANGEATTR_HASUNMAPPED  2 /*< There are unmapped pages within the given range. */
#define KPAGEDIR_RANGEATTR_ANY(x)     ((x)&KPAGEDIR_RANGEATTR_HASMAPPED)
#define KPAGEDIR_RANGEATTR_ALL(x)   (!((x)&KPAGEDIR_RANGEATTR_HASUNMAPPED))
#define KPAGEDIR_RANGEATTR_EMPTY(x) (((x)&(KPAGEDIR_RANGEATTR_HASUNMAPPED|KPAGEDIR_RANGEATTR_HASMAPPED))==(KPAGEDIR_RANGEATTR_HASUNMAPPED))

//////////////////////////////////////////////////////////////////////////
// Returns non-zero if all pages of the given virtual address range are mapped (all)
extern __wunused __nonnull((1)) int
kpagedir_ismapped(struct kpagedir const *__restrict self,
                  __pagealigned __user void const *virt,
                  __size_t pages);
extern __wunused __nonnull((1)) int
kpagedir_ismappedex(struct kpagedir const *__restrict self,
                    __pagealigned __user void const *virt,
                    __size_t pages, kpageflag_t mask, kpageflag_t flags);
extern __wunused __nonnull((1)) int
kpagedir_ismappedex_b(struct kpagedir const *__restrict self,
                      __user void const *virt,
                      __size_t bytes, kpageflag_t mask, kpageflag_t flags);

//////////////////////////////////////////////////////////////////////////
// Returns non-zero if no page in the given virtual address range is mapped (!any)
extern __wunused __nonnull((1)) int
kpagedir_isunmapped(struct kpagedir const *__restrict self,
                    __user void const *virt,
                    __size_t pages);

//////////////////////////////////////////////////////////////////////////
// Returns TRUE(!0) or FALSE(0) if the given pointer 'p' is/isn't mapped.
#define kpagedir_ismappedp(self,p)   kpagedir_ismapped(self,(void *)alignd((__uintptr_t)(p),PAGESIZE),1)
#define kpagedir_isunmappedp(self,p) kpagedir_isunmapped(self,(void *)alignd((__uintptr_t)(p),PAGESIZE),1)


//////////////////////////////////////////////////////////////////////////
// Maps the entire kernel within the given page directory
// @return: KE_NOMEM:  No enough available memory.
// @return: KE_EXISTS: The kernel memory range was already mapped differently.
extern __crit __wunused __nonnull((1)) kerrno_t
kpagedir_mapkernel(struct kpagedir *__restrict self, kpageflag_t flags);

//////////////////////////////////////////////////////////////////////////
// Unmaps a given area of memory from the given page directory
extern __crit __nonnull((1)) void
kpagedir_unmap(struct kpagedir *__restrict self,
               __user void const *virt,
               __size_t pages);


//////////////////////////////////////////////////////////////////////////
// Enumerate mapped memory regions
// (usual rules: non-zero callback return aborts with that exitcode)
// @param: vbegin:    Virtual region address (PAGEALIGN aligned)
// @param: pbegin:    Physical region address (PAGEALIGN aligned)
// @param: pagecount: The amount of mapped pages (multiply by 'PAGESIZE' to bytes)
extern __nonnull((1,2)) int
kpagedir_enum(struct kpagedir const *__restrict self,
              int (*callback)(__user void *vbegin,
                              __kernel void *pbegin,
                              __size_t pagecount,
                              kpageflag_t flags,
                              void *closure),
              void *closure);
#define KPAGEDIR_ENUM_FLAG_MASK  (PAGEDIR_FLAG_USER|PAGEDIR_FLAG_READ_WRITE)

#ifdef __DEBUG__
//////////////////////////////////////////////////////////////////////////
// Print a human-readable representation of mapped memory regions
extern __nonnull((1)) void kpagedir_print(struct kpagedir const *__restrict self);
#endif

#ifdef X86_PTE_FLAG_PRESENT
#define KPAGEDIR_TRANSLATE_FLAGS(flags) ((flags)|X86_PTE_FLAG_PRESENT)
#else
#define KPAGEDIR_TRANSLATE_FLAGS  /* nothing */
#endif

#if defined(__i386__)
#   define KPAGEDIR_TRANSLATE_CALL __fastcall
#else
#   define KPAGEDIR_TRANSLATE_CALL /* nothing */
#endif

//////////////////////////////////////////////////////////////////////////
// Translates a given virtual address into its physical counterpart
// >> This function does the same conversion as performed
//    by the CPU when the page directory is active.
// @return: NULL: The given address is not mapped.
extern __constcall __wunused __nonnull((1)) __kernel void *KPAGEDIR_TRANSLATE_CALL
kpagedir_translate(struct kpagedir const *__restrict self, __user void const *virt);
extern __constcall __wunused __nonnull((1)) __kernel void *KPAGEDIR_TRANSLATE_CALL
kpagedir_translate_flags(struct kpagedir const *__restrict self, __user void const *virt, kpageflag_t flags);

extern __constcall __wunused __nonnull((1)) kpage_t *KPAGEDIR_TRANSLATE_CALL
kpagedir_getpage(struct kpagedir *__restrict self,
                 __user void const *virt);


//////////////////////////////////////////////////////////////////////////
// Returns non-ZERO(0) if the given up to 'bytes' can be accesses
// quickly (using one translation), starting at 'user_address'.
#define KPAGEDIR_CAN_QUICKACCESS(user_address,bytes) \
 (((__uintptr_t)(user_address)&(PAGESIZE-1))+(bytes) <= PAGESIZE)
#define KPAGEDIR_MAYBE_QUICKACCESS(bytes)   ((bytes) <= PAGESIZE)

//////////////////////////////////////////////////////////////////////////
// Returns the page directory used by the kernel
// WARNING: This page directory is constant and addresses
//          should neither be mapped, nor re-mapped.
//       >> Active whenever kernel-level code is being
//          executed, this page directory simply maps
//          any virtual address to the same physical.
#define kpagedir_kernel() (&__kpagedir_kernel)
extern struct kpagedir      __kpagedir_kernel;

extern void
kpagedir_kernel_installmemory(__pagealigned __kernel void *addr,
                              __size_t pages, kpageflag_t flags);
#define KERNEL_INSTALLMEMORY(start,bytes,writable) \
 kpagedir_kernel_installmemory((void *)alignd((__uintptr_t)(start),PAGEALIGN)\
                              ,ceildiv((bytes)+((__uintptr_t)(start)-alignd((__uintptr_t)(start),PAGEALIGN)),PAGESIZE)\
                              ,(writable) ? PAGEDIR_FLAG_READ_WRITE : 0)


#ifdef __MAIN_C__
extern void kernel_initialize_paging(void);
#endif /* __MAIN_C__ */

/* ========================== *
 * Low-level paging utilities *
 * ========================== */

#ifdef __INTELLISENSE__
extern __kernel void *
kpagedir_translate_u(struct kpagedir const *__restrict self,
                     __user void const *addr,
                     __kernel __size_t *__restrict max_bytes,
                     int read_write);
#else
extern __kernel void *
__kpagedir_translate_u_impl(struct kpagedir const *__restrict self,
                            __user void const *addr,
                            __kernel __size_t *__restrict max_bytes,
                            __u32 flags);
#define kpagedir_translate_u(self,addr,max_bytes,read_write) \
 __kpagedir_translate_u_impl(self,addr,max_bytes,\
   KPAGEDIR_TRANSLATE_FLAGS((read_write)?PAGEDIR_FLAG_READ_WRITE:0))
#endif

#endif /* !__ASSEMBLY__ */

__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_PAGING_H__ */

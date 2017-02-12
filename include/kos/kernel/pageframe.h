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
#ifndef __KOS_KERNEL_PAGEFRAME_H__
#define __KOS_KERNEL_PAGEFRAME_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/compiler.h>
#include <kos/errno.h>
#include <kos/kernel/types.h>
#include <kos/kernel/multiboot.h>
#include <kos/kernel/paging.h>

//////////////////////////////////////////////////////////////////////////
// ============== Page Frame Allocator ==============

__DECL_BEGIN

//////////////////////////////////////////////////////////////////////////
// Layout of pages
//////////////////////////////////////////////////////////////////////////
// 1. During initialization, the kernel will gather
//    information about all available memory regions.
// 2. Using that information, consecutive regions
//    are then chained together by filling them
//    with as many free pageframes as possible.
// -> Information about the actually available
//    ram may be lost during this process, but no
//    additional information about regions must be stored.

struct kpageframe;
struct __packed kpageframe {
union __packed {
 __s8 pf_sbytes[PAGESIZE];
 __u8 pf_ubytes[PAGESIZE];
 struct __packed {
  /* Data used to track free pages. */
  struct kpageframe *pff_prev; /*< [0..1] Previous free page (with a lower memory address). */
  struct kpageframe *pff_next; /*< [0..1] Next free page (with a greater memory address). */
  __size_t           pff_size; /*< Amount of consecutively available pages (including this one). */
  // HINT: The total amount of free memory is 'pff_size*PAGESIZE'
 };
};
};

// Define as something to return for ZERO-allocation, or leave
// undefined to have zero-allocations cause undefined behavior.
//#define KPAGEFRAME_ALLOC_ZERO_RETURN_VALUE NULL

#if 1
#   define KPAGEFRAME_INVPTR NULL
#else /* TODO: Rewrite existing code to accept the following! */
#   define KPAGEFRAME_INVPTR ((void *)(__uintptr_t)-1) /*< Impossible memory address. */
#endif

//////////////////////////////////////////////////////////////////////////
// Allocate/Free 'n' consecutive page frames
// >> The returned pointers are physical and 'KPAGEFRAME_INVPTR' is
//    returned if no more memory was available.
extern __crit __wunused __malloccall __pagealigned struct kpageframe *
kpageframe_alloc(__size_t n_pages);
extern __crit __wunused __malloccall __pagealigned struct kpageframe *
kpageframe_tryalloc(__size_t n_pages, __size_t *__restrict did_alloc_pages);
extern __crit __nonnull((1)) void
kpageframe_free(__pagealigned struct kpageframe *__restrict start, __size_t n_pages);

/* TODO: kpageframe_realloc */


//////////////////////////////////////////////////////////////////////////
// Allocate consecutive page frames at a pre-defined address
// @return: KPAGEFRAME_INVPTR: The given start+n area is already allocated, or not mapped
// @return: start: Successfully allocated 'n_pages' of memory starting at 'start'
extern __crit __wunused __malloccall __pagealigned struct kpageframe *
kpageframe_allocat(__pagealigned struct kpageframe *__restrict start, __size_t n_pages);

struct kpageframeinfo {
 __size_t pfi_minregion;   /*< The smallest free frame-region. */
 __size_t pfi_maxregion;   /*< The biggest free frame-region. (Maximum value for which 'kpageframe_alloc' can succeed) */
 __size_t pfi_freeregions; /*< The amount of existing free frame-regions (scatter). */
 __size_t pfi_freepages;   /*< The total amount of free pages. */
 __size_t pfi_freebytes;   /*< The total amount of free bytes. */
};

//////////////////////////////////////////////////////////////////////////
// Collect information about the current state of available pages
// NOTES:
//   - Purely meant for profiling. When multi-threading, the
//     returned data may already not be up-to-date upon return.
//   - Neither 'pfi_minregion', nor 'pfi_maxregion' are set if 'pfi_freeregions' is 0 upon return
//   - 'malloc' from <malloc.h> may still have left-over buffers, even
//     when this function indicates that no more free pages are available.
extern __nonnull((1)) void kpageframe_getinfo(struct kpageframeinfo *__restrict info);

//////////////////////////////////////////////////////////////////////////
// >> Returns true(1) if any bytes described by
//    'p'...'p+s' is part of a free frame region.
// >> Returns false(0) otherwise
// >> Always returns false(0) if 's' is zero
// WARNING: This function only does that. - no further checks are performed!
//          e.g.: True is returned for NULL
extern __wunused __nonnull((1)) int kpageframe_isfreepage(void const *__restrict p, __size_t s);

//////////////////////////////////////////////////////////////////////////
// Print the layout of the physical memory allocator.
// >> Usefuly to debug scattering in physical memory.
extern void kpageframe_printphysmem(void);


#ifdef __MAIN_C__
//////////////////////////////////////////////////////////////////////////
// Initialize the list of available memory regions from info given by GRUB
// NOTE: 'size' is the total size of 'mbt' in bytes
// WARNING: These functions must not be called post initialization
// NOTE: Also initialized dynamic memory and dlmalloc, as well as the GRUB command line.
extern void kernel_initialize_raminfo(void);
#endif

extern void raminfo_addregion(__u64 start, __u64 size);


#ifndef __KOS_KERNEL_KMEM_VALIDATE_DECLARED
#define __KOS_KERNEL_KMEM_VALIDATE_DECLARED
//////////////////////////////////////////////////////////////////////////
// Validates a given memory range to describe valid kernel memory
// @return: KE_OK:    Everything's fine!
// @return: KE_INVAL: The given memory describes a portion of memory that is not allocated
// @return: KE_RANGE: The given memory doesn't map to any valid RAM/Device memory
extern __wunused kerrno_t kmem_validatebyte(__u8 const *p);
extern __wunused kerrno_t kmem_validate(void const *p, __size_t s);
#endif

__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_PAGEFRAME_H__ */

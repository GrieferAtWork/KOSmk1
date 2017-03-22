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
#include <kos/compiler.h>
#include <kos/errno.h>
#include <kos/kernel/features.h>
#include <kos/kernel/gdt.h>
#include <kos/kernel/mutex.h>
#include <kos/kernel/object.h>
#include <kos/kernel/paging.h>
#include <kos/kernel/rwlock.h>
#include <kos/types.h>
#include <math.h>
#include <strings.h>

/* TODO: Make SHM branches use <kos/addrtree.h>. */

/* Shared memory management. */

__DECL_BEGIN

//////////////////////////////////////////////////////////////////////////
// copy-on-write
//
// #1 (before fork()) MEMORY: ro_memory_range_A.refcnt = 1
// procA [rw_memory_range_A] 
// #2 (after fork()) MEMORY: ro_memory_range_A.refcnt = 2
// procA [ro_memory_range_A] 
// procB [ro_memory_range_A] 
// #3 (procA writes) MEMORY: ro_memory_range_A.refcnt = 2
// procA [ro_memory_range_A[PF]ro_memory_range_A] 
// procB [ro_memory_range_A....ro_memory_range_A]
//
// #4 (MAGIC) ...
//
// #5 (wanted result)
// owners: procA [ro_memory_range_A0][rw_memory_range_B][ro_memory_range_A2] 
// owners: procB [ro_memory_range_A0..ro_memory_range_A1.ro_memory_range_A2]
// ro_memory_range_A0.refcnt = 2;
// ro_memory_range_A1.refcnt = 1;
// ro_memory_range_A2.refcnt = 2;
// rw_memory_range_B.refcnt = 1; (Copy of 'ro_memory_range_A1')
//
// In order to dynamically sub-dividing chunks, we
// differentiate between SHM parts, chunks and regions:
//
// >> shm-part: (Global/Shared)
//   - Not reference counted descriptor for a single,
//     page-aligned region of physical memory, as allocated
//     directly using the page frame allocator.
// 
// >> shm-chunk: (Global/Shared)
//   - Vector of SHM-parts
//   - Tracking of SHM-part order
//
// >> shm-cluster: (Global/Shared)
//   - A reference counted descriptor for up to
//     'KSHM_CLUSTERSIZE' pages described by shm-parts.
//   - Knows lowest associated part for pointer to allow for O(1) part-lookup.
//
// >> shm-region: (Global/Shared)
//   - Owns an SHM-chunk.
//   - Owns a vector of shm-clusters for
//     reference accounting of all shm-parts.
//   - Stores meta-data to track access permissions,
//     such as read, write, exec, shared, etc.
//
// >> shm-branch: (Per-process)
//   - Reference-counted descriptor for a single SHM region.
//   - During copy-on-write is split into multiple branches.
//   - Can be located very fast given a virtual address contained.
//   - Also used to store information about partially mapped
//     regions, as resulting from calls to mmap(), followed
//     by a fork(), then followed by munamp(), where the
//     latter only unmaps apart of what was previously mapped
//     by 'mmap'.
//
// >> shm-map: (Per-process)
//   - Per-process mapping between regions and virtual addresses.
//
// >> shm: (Per-process)
//   - Per-process structure containing a shm-map among
//     other memory-related things such as the LDT vector
//     and the page directory.
//

#ifndef __ASSEMBLY__
struct kpageframe;
#endif /* __ASSEMBLY__ */

#define KOBJECT_MAGIC_SHMCHUNK  0x573C7C25 /*< SHMCHUNK. */
#define KOBJECT_MAGIC_SHMREGION 0x5736E610 /*< SHMREGIO. */
#define KOBJECT_MAGIC_SHM       0x573      /*< SHM. */
#define kassert_kshmchunk(self)  kassert_object(self,KOBJECT_MAGIC_SHMCHUNK)
#define kassert_kshmregion(self) kassert_refobject(self,sre_clustera,KOBJECT_MAGIC_SHMREGION)
#define kassert_kshm(self)       kassert_object(self,KOBJECT_MAGIC_SHM)

/* In pages: amount of memory managed by a single reference counter.
 *        >> Higher values --> more redundency during copy-on-write.
 *        >> Lower values --> copy-on-write must be performed more often. */
#define KSHM_CLUSTERSIZE  16


#define KSHMREFCNT_SIZEOF  4
#ifndef __ASSEMBLY__
typedef __u32 kshmrefcnt_t;
#endif /* !__ASSEMBLY__ */

#define KSHMREGION_FLAG_NONE       0x0000
#define KSHMREGION_FLAG_EXEC       0x0001 /*< Currently unused; later this will work alongside the LDT to control execute permissions. */
#define KSHMREGION_FLAG_WRITE      0x0002 /*< Memory is mapped as writable from ring-#3 (Unless used with 'KSHMRANGE_FLAG_SHARED' or 'KSHMREGION_FLAG_NOCOPY', this also enables copy-on-write semantics). */
#define KSHMREGION_FLAG_READ       0x0004 /*< Memory is mapped as readable from ring-#3. */
#define KSHMREGION_FLAG_SHARED     0x0008 /*< Memory is shared between all processes it is mapped within. */
#define KSHMREGION_FLAG_LOSEONFORK 0x0010 /*< This memory region is not copied during a fork() operation. - Used to not copy stuff like a kernel-thread-stack or user-space thread-local storage. */
#define KSHMREGION_FLAG_RESTRICTED 0x0020 /*< This region of memory is restricted, meaning that the user cannot use 'munmap()' to delete it. (NOTE: Superceided by 'KSHMREGION_FLAG_LOSEONFORK'). */
#define KSHMREGION_FLAG_NOFREE     0x0040 /*< Don't free memory from this chunk when the reference counter hits ZERO(0). - Used for 1-1 physical/device memory mappings. */
#define KSHMREGION_FLAG_NOCOPY     0x0080 /*< Don't hard-copy memory from this chunk, but alias it when set in 'right->sre_chunk.sc_flags' in 'kshmchunk_hardcopy'. */
//#define KSHMREGION_FLAG_ZERO     0x0400 /*< TODO: Symbolic region of potentially infinite size; fill-on-read; filled only with zeros (useful for bss sections). */

/* Returns the set of sub-flags that carry effects that must be considered before merging two regions.
 * Basically: Only merge two regions if their effective flags are equal. */
#define KSHMREGION_EFFECTIVEFLAGS(flags) (flags)

#define KSHM_FLAG_SIZEOF  2
#ifndef __ASSEMBLY__
typedef __u16 kshm_flag_t; /*< Set of 'KSHMREGION_FLAG_*' */
#endif /* !__ASSEMBLY__ */

/* Returns TRUE(1) if the given set of flags either requires the
 * use of COW semantics, or to perform hard copies during fork().
 * >> Basically just: Is is writable and not shared memory.
 * NOTE: As a special addition: nocopy-pages implicitly behave as shared. */
#define KSHMREGION_FLAG_USECOW(flags) \
 (((flags)&(KSHMREGION_FLAG_WRITE)) && \
 !((flags)&(KSHMREGION_FLAG_SHARED|KSHMREGION_FLAG_NOCOPY)))



#define KSHMCHUNK_PAGE_SIZEOF     __SIZEOF_POINTER__
#define KSHMREGION_PAGE_SIZEOF    __SIZEOF_POINTER__
#define KSHMREGION_ADDR_SIZEOF    __SIZEOF_POINTER__
#define KSHMREGION_CLUSTER_SIZEOF __SIZEOF_POINTER__
#ifndef __ASSEMBLY__
typedef __uintptr_t      kshmchunk_page_t;
typedef kshmchunk_page_t kshmregion_page_t;
typedef __uintptr_t      kshmregion_addr_t;
typedef __uintptr_t      kshmregion_cluster_t;
#endif /* !__ASSEMBLY__ */


#define KSHMPART_SIZEOF          (__SIZEOF_POINTER__+KSHMREGION_PAGE_SIZEOF+__SIZEOF_SIZE_T__)
#define KSHMPART_OFFSETOF_FRAME  (0)
#define KSHMPART_OFFSETOF_START  (__SIZEOF_POINTER__)
#define KSHMPART_OFFSETOF_PAGES  (__SIZEOF_POINTER__+KSHMREGION_PAGE_SIZEOF)
#ifndef __ASSEMBLY__
struct kshmpart {
 /* WARNING: Individual pages are only allocated when the reference counter of the associated cluster is non-ZERO. */
 /* TODO: When swap/file mappings get added, we'll have to add some kind of token system that can live here (union-style w/ sp_frame). */
 __pagealigned struct kpageframe *sp_frame; /*< [1..sp_pages][const] Physical starting address of this part. */
 kshmregion_page_t                sp_start; /*< [const] Page index of the start of this part (aka. 'sum(sp_pages)' of all preceding parts). */
 __size_t                         sp_pages; /*< [!0][const] Amount of linear pages associated with this part. */
};
#endif /* !__ASSEMBLY__ */


#define KSHMCHUNK_SIZEOF          (KOBJECT_SIZEOFHEAD+KSHM_FLAG_SIZEOF+2+__SIZEOF_POINTER__+__SIZEOF_SIZE_T__)
#define KSHMCHUNK_OFFSETOF_FLAGS  (KOBJECT_SIZEOFHEAD)
#define KSHMCHUNK_OFFSETOF_PARTC  (KOBJECT_SIZEOFHEAD+KSHM_FLAG_SIZEOF)
#define KSHMCHUNK_OFFSETOF_PARTV  (KOBJECT_SIZEOFHEAD+KSHM_FLAG_SIZEOF+2)
#define KSHMCHUNK_OFFSETOF_PAGES  (KOBJECT_SIZEOFHEAD+KSHM_FLAG_SIZEOF+2+__SIZEOF_POINTER__)
#ifndef __ASSEMBLY__
struct kshmchunk {
 KOBJECT_HEAD
 kshm_flag_t      sc_flags; /*< [const] A set of 'KSHMREGION_FLAG_*' flags describing permissions
                                  NOTE: Yes, these flags _ARE_ immutable; you can't get memory previously mapped as read-only!
                                        And it would be impossible to implement such a switch because we can't inform other processes of
                                        such a change and get them to create their own copies once they start writing to this memory
                                        because we neither know, nor would be allowed to modify their page directories. */
 __u16            sc_partc; /*< [!0][const] Amount of parts used to represent this region. */
 struct kshmpart *sc_partv; /*< [1..sre_partc][const][owned] Vector of scattered memory descriptors. */
 __size_t         sc_pages; /*< [const] Total amount of pages originally allocated (NOTE: Must not be a cluster-aligned value!). */
};

//////////////////////////////////////////////////////////////////////////
// Initialize a given SHM chunk of memory.
// @return: KE_OK:    Successfully initialized the given chunk.
// @return: KE_NOMEM: Not enough available physical memory.
//                    May either refer to physical (pageframe),
//                    or kernel memory (malloc) memory.
extern __crit __wunused __nonnull((1)) kerrno_t kshmchunk_initlinear(struct kshmchunk *__restrict self, __size_t pages, kshm_flag_t flags);
extern __crit __wunused __nonnull((1)) kerrno_t kshmchunk_initphys(struct kshmchunk *__restrict self, __pagealigned __kernel void *addr, __size_t pages, kshm_flag_t flags);
extern __crit __wunused __nonnull((1)) kerrno_t kshmchunk_initram(struct kshmchunk *__restrict self, __size_t pages, kshm_flag_t flags);
extern __crit __wunused __nonnull((1,2)) kerrno_t kshmchunk_inithardcopy(struct kshmchunk *__restrict self, struct kshmchunk const *__restrict right);
extern __crit __nonnull((1)) void kshmchunk_quit(struct kshmchunk *__restrict self);


//////////////////////////////////////////////////////////////////////////
// Copy 'page_count*PAGESIZE' bytes from the given SHM chunk
// into the provided buffer, starting at 'first_page'.
// NOTE: The caller is responsible to ensure that 'first_page+page_count' is
//       not out-of-bounds, as well as the given buffer being of sufficient size.
extern void kshmchunk_copypages(struct kshmchunk const *self,
                                kshmchunk_page_t first_page,
                                __kernel void *buf, __size_t page_count);
#endif /* !__ASSEMBLY__ */



#define KSHMCLUSTER_SIZEOF          (KSHMREFCNT_SIZEOF+__SIZEOF_POINTER__)
#define KSHMCLUSTER_OFFSETOF_REFCNT (0)
#define KSHMCLUSTER_OFFSETOF_PART   (KSHMREFCNT_SIZEOF)
#ifndef __ASSEMBLY__
struct kshmcluster {
 __atomic kshmrefcnt_t sc_refcnt; /*< Reference counters for cluster memory (always controls 1..KSHM_CLUSTERSIZE (inclusive) pages of memory). */
 struct kshmpart      *sc_part;   /*< [1..1] The first part that includes pages from this cluster. */
};
#endif /* !__ASSEMBLY__ */


#define KSHMREGION_OFFSETOF_CHUNK       (0)
#define KSHMREGION_OFFSETOF_BRANCHES    (KSHMCHUNK_SIZEOF)
#define KSHMREGION_OFFSETOF_CLUSTERA    (KSHMCHUNK_SIZEOF+__SIZEOF_SIZE_T__)
#define KSHMREGION_OFFSETOF_CLUSTERC    (KSHMCHUNK_SIZEOF+__SIZEOF_SIZE_T__*2)
#define KSHMREGION_OFFSETOF_CLUSTERV(i) (KSHMCHUNK_SIZEOF+__SIZEOF_SIZE_T__*3+(i)*KSHMCLUSTER_SIZEOF)
#define KSHMREGION_SIZEOF(clusterc)      KSHMREGION_OFFSETOF_CLUSTERV(clusterc)
#ifndef __ASSEMBLY__
struct kshmregion {
 KOBJECT_HEAD /* NOTE: This structure is synchronized because it is fully atomic/singleton. */
 struct kshmchunk   sre_chunk;       /*< Associated chunks of data. */
 struct kshmregion *sre_origin;      /*< [0..1][const] Base region that this one is originating from.
                                          Used to track the origin of a mapping throughout touches, allowing you to safely
                                          unmap a specific region at a specific address even after it has been touched.
                                          WARNING: Not a reference! The origin region may long be dead. - So don't deref! */
 __atomic __size_t  sre_branches;    /*< Amount of different reference holders that this region is associated with (Used to ensure uniqueness of a region). */
 __atomic __size_t  sre_clustera;    /*< [!0] Allocated amount of clusters (When this hits ZERO(0), free the region controller structure). */
 __size_t           sre_clusterc;    /*< [!0][const][== ceildiv(sre_chunk.sc_pages,KSHM_CLUSTERSIZE)] Amount of tracked clusters. */
 struct kshmcluster sre_clusterv[1]; /*< [sre_clusterc] Inlined vector of available clusters. */
};

#define kshmregion_getflags(self) ((self)->sre_chunk.sc_flags)
#define kshmregion_getpages(self) ((self)->sre_chunk.sc_pages)

#define kshmregion_getcluster(self,page_index) \
 ((self)->sre_clusterv+((page_index)/KSHM_CLUSTERSIZE))

//////////////////////////////////////////////////////////////////////////
// Increment/Decrement the reference counter of all of the specified clusters.
// When a cluster reference counter reaches ZERO(0), all associated pages
// will be freed using 'kpageframe_free()', unless the 'KSHMREGION_FLAG_NOFREE'
// flag has been set.
// NOTE: Using these functions to acquire/release references to a given region
//       will also generate 'sre_branches' references, meaning that every call
//       to 'kshmregion_incref' must eventually be followed by a call to
//       'kshmregion_decref', passing the exact same arguments to ensure
//       binary integrity.
// WARNING: Unlike many other reference counters in KOS, the sre_branches
//          counter is not checked for overflow during increment, meaning
//          that the caller must ensure an environment in which no more
//          than SIZE_MAX references are possible.
//    HINT: This can easily be implemented by assigning/reserving at least
//          ONE(1) byte of physical memory for every reference (to-be) taken.
// @return: KE_OK:       Successfully incremented the reference counters of all associated clusters.
// @return: KE_OVERFLOW: One of the cluster reference counters has overflown.
//                 NOTE: Undefined behavior is invoked if the 'sre_branches' counter overflows.
extern __crit __wunused __nonnull((1,2,3)) kerrno_t kshmregion_incref(struct kshmregion *__restrict self, struct kshmcluster *cls_min, struct kshmcluster *cls_max);
extern __crit           __nonnull((1,2,3))     void kshmregion_decref(struct kshmregion *__restrict self, struct kshmcluster *cls_min, struct kshmcluster *cls_max);
#define kshmregion_incref_full(self) kshmregion_incref(self,(self)->sre_clusterv,(self)->sre_clusterv+((self)->sre_clusterc-1))
#define kshmregion_decref_full(self) kshmregion_decref(self,(self)->sre_clusterv,(self)->sre_clusterv+((self)->sre_clusterc-1))

//////////////////////////////////////////////////////////////////////////
// Returns the physical address of a given page-index, as well
// the max amount of consecutively valid pages at that address.
extern __wunused __retnonnull __nonnull((1,3)) __kernel struct kpageframe *
kshmregion_getphyspage(struct kshmregion *__restrict self,
                       kshmregion_page_t page_index,
                       __size_t *__restrict max_pages);

//////////////////////////////////////////////////////////////////////////
// Convenience wrapper around 'kshmregion_getphyspage'.
// Automatically handles page alignment.
extern __wunused __retnonnull __nonnull((1,3)) __kernel void *
kshmregion_translate_fast(struct kshmregion *__restrict self,
                          kshmregion_addr_t address_offset,
                          __size_t *__restrict max_bytes);

//////////////////////////////////////////////////////////////////////////
// Similar to 'kshmregion_translate_fast', but simply returns NULL
// and sets '*max_bytes' to ZERO(0) for out-of-bounds indices.
extern __wunused __nonnull((1,3)) __kernel void *
kshmregion_translate(struct kshmregion *__restrict self,
                     kshmregion_addr_t address_offset,
                     __size_t *__restrict max_bytes);


struct kfile;
//////////////////////////////////////////////////////////////////////////
// Fill an SHM region with data read from the given file.
// NOTE: Data not read from the file will be filled with ZERO(0),
//       making these functions perfect for use during the linker
//       loading process.
// @return: KE_ISOK(*):  Successfully read data from the given file.
// @return: KE_ISERR(*): Some file-specific error occurred.
extern __wunused __nonnull((1,3)) kerrno_t
kshmregion_ploaddata(struct kshmregion *__restrict self,
                     __uintptr_t offset, struct kfile *__restrict fp,
                     __size_t filesz, __pos_t pos);
extern __wunused __nonnull((1,3)) kerrno_t
kshmregion_loaddata(struct kshmregion *__restrict self,
                    __uintptr_t offset, struct kfile *__restrict fp,
                    __size_t filesz);


//////////////////////////////////////////////////////////////////////////
// Create various kinds of SHM memory chunks.
// NOTE: Initially, the caller inherits ONE(1) reference to every cluster.
// kshmregion_newram(...):
//    Allocate new, possibly parted (aka. scattered) memory,
//    returning a region describing that newly allocated ram.
// kshmregion_newlinear(...):
//    Similar to 'kshmregion_newram', but ensure that
//    allocated memory is liner in a physical sense.
//    That is: it is one continuous string in the physical address space, with the
//             region only consisting of a single part ('return->sre_chunk.sc_partc == 1').
// kshmregion_newphys(...):
//    Create a new region that can be used for mapping a physical
//    address, allowing you to map special areas of memory, such
//    as the VGA screen buffer outside of ring-#0.
//    NOTE: 'KSHMREGION_FLAG_NOFREE' is automatically added to the given flags.
// @return: * :   A new reference to a newly allocated SHM region, as described above.
// @return: NULL: Not enough available kernel, or (more likely) physical memory.
extern __crit __wunused __malloccall __ref struct kshmregion *kshmregion_newram(__size_t pages, kshm_flag_t flags);
extern __crit __wunused __malloccall __ref struct kshmregion *kshmregion_newlinear(__size_t pages, kshm_flag_t flags);
extern __crit __wunused __malloccall __ref struct kshmregion *kshmregion_newphys(__pagealigned __kernel void *addr, __size_t pages, kshm_flag_t flags);

//////////////////////////////////////////////////////////////////////////
// Merge two regions into one, inheriting all parts from both.
// NOTES:
//  - Upon success (non-NULL return), both of the given
//    regions have been deallocated (free()'d), meaning that
//    any further dereferencing of any of their members
//    causes undefined behavior.
//  - For optimization, this algorithm will attempt to
//    merge the last part of 'min_region' with the first
//    of 'max_region', given the possibility that their
//    physical addresses align in a way that allows for this.
//    >> Other than that, the returned region describes the
//       addressable memory space of 'min_region', directly
//      (that is page-aligned) followed by that of 'max_region'.
//    >> Like all other region-allocators the returned region
//       will be initialized with 'sre_branches' set to ONE(1),
//       as well as all associated clusters initialized to ONE(1).
//  - Both regions must be fully allocated, as easily deducible
//    by asserting 'sre_clustera == sre_clusterc' on both of them.
// WARNING: The caller is responsible to ensure that both regions
//          share the same chunk-flags, as well as be unique
//          (that is: the caller is the only owner of references,
//           as easily deducible by checking 'sre_branches' to
//           equal ONE(1))
// WARNING: Due to cluster alignment, the actual cluster count
//          of the returned region may not necessarily be the
//          sum of clusters from the given regions.
// @return: * :   A new region controller for both of the
//                given regions, initialized as described above.
// @return: NULL: Not enough available memory to allocate the new region controller.
extern __crit __wunused __malloccall __nonnull((1,2)) __ref struct kshmregion *
kshmregion_merge(__ref struct kshmregion *__restrict min_region,
                 __ref struct kshmregion *__restrict max_region);

//////////////////////////////////////////////////////////////////////////
// Create a hard copy of the given region, containing a new area
// of memory the same size of the old, unless the given region
// has the 'KSHMCHUNK_FLAG_NOCOPY' flag set, in which case it
// will alias the memory of the existing region.
// - This function differs from 'kshmregion_extractpart' in the fact
//   that it allows for precise page-based sub-creation of regions
//   instead of being restricted to cluster borders.
// NOTE:    All flags of the given region are inherited.
// WARNING: Even if the given region only contains linear memory,
//          the returned hard copy may contain non-linear parts
//          when the 'KSHMCHUNK_FLAG_NOCOPY' flag wasn't set.
extern __crit __wunused __malloccall __nonnull((1)) __ref struct kshmregion *
kshmregion_hardcopy(struct kshmregion const *__restrict self,
                    kshmregion_page_t page_offset,
                    __size_t pages);
#define kshmregion_fullcopy(self) kshmregion_hardcopy(self,0,(self)->sre_chunk.sc_pages)


//////////////////////////////////////////////////////////////////////////
// Copy a part (specified in pages) of a given region, creating a new region in the process.
// NOTES:
//   - The caller is responsible to prevent out-of-bounds indices from being passed.
//   - The caller is responsible to ensure that all clusters within the
//     specified sub-range as alive and allocated. (ASSERTED IN DEBUG MODE)
//   - The returned region inherits the flags of the specified region.
//   - If the 'KSHMREGION_FLAG_NOCOPY' flag is set in the given region,
//     memory will not be copied, but the instead new region will instead
//     reference the same parts as the given region.
//     - If the given range covers the entirety of the old region,
//       the given region is returned without modification.
//   - Initially, the caller inherits ONE(1) reference to every cluster.
//   - A reference is returned from every cluster covered by the given area of memory.
//     - For optimization and to allow for page re-use, clusters who's reference
//       counter is ONE(1) when being extracted, will not be copied, but instead
//       removed from the original region, setting the reference counter in 'self'
//       to ZERO(0) and updating 'sre_clustera', before being moved into the
//       new, returned region.
//     - Upon failure, 'self' will still remain unchanged because such
//       optimizations are detected and performed in a two-step process
//       that allocates all necessary memory beforehand.
//   - In the unlikely case of during all of this, the 'sre_clustera' counter
//     of 'self' dropping to ZERO(0), the given region will still be freed.
// @return: * :   A new reference to a newly allocated SHM region, as described above.
// @return: NULL: Not enough available memory.
extern __crit __wunused __malloccall __nonnull((1,2,3)) __ref struct kshmregion *
kshmregion_extractpart(__ref struct kshmregion *__restrict self,
                       struct kshmcluster *cls_min,
                       struct kshmcluster *cls_max);

//////////////////////////////////////////////////////////////////////////
// Fill all memory within the given region with the given byte.
extern __crit __nonnull((1)) void
kshmregion_memset(__ref struct kshmregion *__restrict self, int byte);

#endif /* !__ASSEMBLY__ */










//////////////////////////////////////////////////////////////////////////
// SHM Memory map.
// - Similar to a binary tree, every layer splits pointer
//   resolution, meaning that when taking PAGESIZE into
//   account, lookup has a worst case time of O(20).
//   Lookup time itself decreases the closer an address
//   is to '0x80000000', where '0x80000000' itself is O(1).
// - WARNING: This is something completely custom... And it gets complicated quick!
// - Splitting is done as follows:
//   
// PTR_MIN   = 0
// PTR_MAX   = 63
// GIVEN_PTR = 50
//            <------------------------------64------------------------------>
// [==============================================================================]
// STEP #1:   <--------------32-------------->|<--------------31------------->
// >> GIVEN_PTR(50) lies above semi(32) (increment semi by 16 and try again)
// [==============================================================================]
// STEP #2:   <--------------48------------------------------>|<------15----->
// >> GIVEN_PTR(50) lies above semi(48) (increment semi by 8 and try again)
// [==============================================================================]
// STEP #3:   <--------------56-------------------------------------->|<--7-->
// >> GIVEN_PTR(50) lies below semi(56) (decrement semi by 4 and try again)
// [==============================================================================]
// STEP #4:   <--------------52---------------------------------->|<----11--->
// >> GIVEN_PTR(50) lies below semi(52) (decrement semi by 2 and try again)
// [==============================================================================]
// STEP #5:   <--------------50-------------------------------->|<-----13---->
// >> GIVEN_PTR(50) equals semi(50)
// >> If we still havn't found the correct SHM tab, we managed
//    to determine that GIVEN_PTR isn't mapped in at most 5 steps.
// 
// >> If the branch of any of the prior steps covered GIVEN_PTR,
//    or didn't lead to any other branches, we were able to
//    stop searching before even getting to step #5.
//
#define KSHMBRANCH_SIZEOF                (__SIZEOF_POINTER__*8+__SIZEOF_SIZE_T__)
#define KSHMBRANCH_OFFSETOF_MIN          (0)
#define KSHMBRANCH_OFFSETOF_MAX          (__SIZEOF_POINTER__)
#define KSHMBRANCH_OFFSETOF_MAP_MIN      (__SIZEOF_POINTER__*2)
#define KSHMBRANCH_OFFSETOF_MAP_MAX      (__SIZEOF_POINTER__*3)
#define KSHMBRANCH_OFFSETOF_RSTART       (__SIZEOF_POINTER__*4)
#define KSHMBRANCH_OFFSETOF_RPAGES       (__SIZEOF_POINTER__*5)
#define KSHMBRANCH_OFFSETOF_REGION       (__SIZEOF_POINTER__*5+__SIZEOF_SIZE_T__)
#define KSHMBRANCH_OFFSETOF_CLUSTER_MIN  (__SIZEOF_POINTER__*6+__SIZEOF_SIZE_T__)
#define KSHMBRANCH_OFFSETOF_CLUSTER_MAX  (__SIZEOF_POINTER__*7+__SIZEOF_SIZE_T__)
#ifndef __ASSEMBLY__
struct kshmbranch {
 /* NOTE: This structure is synchronized because it is only accessible per-process. */
 struct kshmbranch         *sb_min; /*< [0..1][owned] Branch for addresses in ('addr < addr_semi'); When selected, continue with 'addr_semi -= (__uintptr_t)1 << addr_level--;' */
 struct kshmbranch         *sb_max; /*< [0..1][owned] Branch for addresses in ('addr >= addr_semi'); When selected, continue with 'addr_semi += (__uintptr_t)1 << --addr_level;' */
 /* NOTE: The memory described within the following two members _MUST_ be located
  *       within the valid range of memory of the associated address level.
  *       These valid value ranges can be calculated using
  *       'KSHMBRANCH_MAPMIN' and 'KSHMBRANCH_MAPMAX'. */
union{
 __uintptr_t                sb_map_min;     /*< [1..1][const] Lowest mapped address. */
 __pagealigned __user void *sb_map;         /*< [1..1][const] User-space address this tab is mapped to. */
};
 __uintptr_t                sb_map_max;     /*< [1..1][const][==sb_map_min+(sb_rpages*PAGESIZE)-1] Highest mapped address. */
 kshmregion_page_t          sb_rstart;      /*< Index of the first page within the associated region that is mapped by this branch. */
 __size_t                   sb_rpages;      /*< Amount of pages used from the associated region, starting at 'sb_rstart'. */
 __ref struct kshmregion   *sb_region;      /*< [1..1][const] Reference (sre_branches) to the associated region of memory. */
 /* NOTE: The following two pointers describe a range of clusters to which this branch owns references. */
 __ref struct kshmcluster  *sb_cluster_min; /*< [1..1][in(sb_region->sre_clusterv)][<= sb_cluster_max] Lowest cluster to which this branch owns references. */
 __ref struct kshmcluster  *sb_cluster_max; /*< [1..1][in(sb_region->sre_clusterv)][>= sb_cluster_min] Highest cluster to which this branch owns references. */
};

/* Returns TRUE(1) if the given branch can be expanded up-, or downwards. */
#define kshmbranch_canexpand_min(self) \
 ((self)->sb_rstart == 0 && katomic_load((self)->sb_region->sre_branches) == 1)
#define kshmbranch_canexpand_max(self) \
 ((self)->sb_rstart+(self)->sb_rpages == \
  (self)->sb_region->sre_chunk.sc_pages && \
   katomic_load((self)->sb_region->sre_branches) == 1)

#define KSHMBRANCH_ADDRSEMI_INIT       ((((__uintptr_t)-1)/2)+1)
#define KSHMBRANCH_ADDRLEVEL_INIT      ((__SIZEOF_POINTER__*8)-1)
#define KSHMBRANCH_MAPMIN(semi,level)  ((semi)&~(((__uintptr_t)1 << (level))))
#define KSHMBRANCH_MAPMAX(semi,level)  ((semi)| (((__uintptr_t)1 << (level))-1))
#define KSHMBRANCH_WALKMIN(semi,level) ((semi)  = (((semi)&~((__uintptr_t)1 << (level)))|((__uintptr_t)1 << ((level)-1))),--(level)) /*< unset level'th bit; set level'th-1 bit. */
#define KSHMBRANCH_WALKMAX(semi,level) (--(level),(semi) |= ((__uintptr_t)1 << (level)))                                             /*< set level'th-1 bit. */
#define KSHMBRANCH_NEXTMIN(semi,level) (((semi)&~((__uintptr_t)1 << (level)))|((__uintptr_t)1 << ((level)-1))) /*< unset level'th bit; set level'th-1 bit. */
#define KSHMBRANCH_NEXTMAX(semi,level) ((semi)|((__uintptr_t)1 << ((level)-1)))                                /*< set level'th-1 bit. */
#define KSHMBRANCH_SEMILEVEL(semi)     (ffs(semi)-1) /*< Get the current level associated with a given semi-address. */

//////////////////////////////////////////////////////////////////////////
// Print a representation of the given SHM
// branch and all those reachable from it.
// >> Only used for debugging and panic-info.
extern void
kshmbranch_print(struct kshmbranch *__restrict branch,
                 __uintptr_t addr_semi, unsigned int level);

//////////////////////////////////////////////////////////////////////////
// Recursively delete an entire SHM-tree, starting at the given 'start'
// NOTE: This function is not to be confused with 'kshmbranch_remove',
//       which merely pops a single SHM leaf from its branch without
//       even performing cleanup on that leaf, while this function
//       will not remove any branches, but instead walk along
//       the tree and recursively drop all references, as well
//       as free all leaves.
// WARNING: The caller is responsible to ensure no further use
//          of the given 'start', or any branch that used to
//          be reachable from it.
extern void kshmbranch_deltree(struct kshmbranch *__restrict start);

/* Low-level branch-version of generic mapping functions.
 * For documentation, see the 'kshmmap_*' equivalents below. */
extern __crit __nomp __nonnull((1,2)) kerrno_t
kshmbranch_insert(struct kshmbranch **__restrict proot,
                  struct kshmbranch *__restrict newleaf,
                  __uintptr_t addr_semi, unsigned int addr_level);
extern __crit __nomp __nonnull((1)) struct kshmbranch *
kshmbranch_remove(struct kshmbranch **__restrict proot,
                  __uintptr_t addr);
__local __wunused struct kshmbranch *
kshmbranch_locate(struct kshmbranch *__restrict root,
                  __uintptr_t addr);


//////////////////////////////////////////////////////////////////////////
// Returns the old '*proot' and replaces it with either its min, or max branch.
// The min/max branches of *proot will be reinserted.
extern __crit __nomp __wunused __retnonnull __nonnull((1)) struct kshmbranch *
kshmbranch_popone(struct kshmbranch **__restrict proot,
                  __uintptr_t addr_semi, unsigned int addr_level);

//////////////////////////////////////////////////////////////////////////
// Recursively re-insert all branches reachable
// from 'insert_root' in to the given '*proot'.
// NOTE: Undefined behavior is invoked if any of
//       the leaves overlap with existing branches.
extern __crit __nomp __nonnull((1,2)) void
kshmbranch_reinsertall(struct kshmbranch **__restrict proot,
                       struct kshmbranch *__restrict insert_root,
                       __uintptr_t addr_semi, unsigned int addr_level);


//////////////////////////////////////////////////////////////////////////
// Locate the pointer to the branch associated with the given address.
// If non-NULL, store the associated semi-address in '*result_semi' upon success.
// @return: NULL: Failed to find a branch containing the given address.
__local __wunused struct kshmbranch **
kshmbranch_plocate(struct kshmbranch **__restrict proot, __uintptr_t addr,
                   __uintptr_t *result_semi);
__local __wunused struct kshmbranch **
kshmbranch_plocateat(struct kshmbranch **__restrict proot, __uintptr_t addr,
                     __uintptr_t *addr_semi, unsigned int *addr_level);

//////////////////////////////////////////////////////////////////////////
// Creates a split at the given page offset within the branch at '*pself'.
// UNDEFINED-BEHAVIOR:
//   - 'addr_semi' isn't the real semi-address of the given branch '*pself' (CANNOT BE CHECKED)
//   - 'page_offset' is out-of-bounds of '(*pself)->sb_rstart ... (*pself)->sb_rpages'. (ASSERTED)
// NOTES:
//   - Depending on the value of 'addr_semi', the newly created branch
//     may be what '*pself' points to or not, as the function will also
//     automatically insert the new branch into the given branch pointer,
//     or into one of the sub-branches of '*pself', depending on whether
//     or not the addr-semi lies before, or after the true address
//     associated with the given 'page_offset'.
// @param: pmin:      [out] When non-NULL, upon success, pointer to the branch < the given 'page_offset'.
// @param: pmax:      [out] When non-NULL, upon success, pointer to the branch >= the given 'page_offset'.
// @param: pmin_semi: [out] When non-NULL, upon success, addr_semi value of the '**pmin'-branch.
// @param: pmax_semi: [out] When non-NULL, upon success, addr_semi value of the '**pmax'-branch.
// @return: KE_OK:    Successfully created the new split.
// @return: KE_NOMEM: Failed to allocate the new branch.
// WARNING: The given 'page_offset' is an offset within the SHM region of '*pself'.
//          It is _NOT_ an offset within the area of that region mapped by '*pself'!
extern __crit __nomp __wunused __nonnull((1)) kerrno_t
kshmbrach_putsplit(struct kshmbranch **__restrict pself, __uintptr_t addr_semi,
            /*opt*/struct kshmbranch ***pmin, /*opt*/__uintptr_t *pmin_semi,
            /*opt*/struct kshmbranch ***pmax, /*opt*/__uintptr_t *pmax_semi,
                   kshmregion_page_t page_offset);

//////////////////////////////////////////////////////////////////////////
// Merge two given branches 'pmin' and 'pmax' that are located directly next to each other.
// Upon success and if non-NULL, store the branch-pointer and address-semi
// of the merged branch in the given 'presult' and 'presult_semi' locations.
// @param: pmin:    [in] Pointer to the min-branch that should be merged.
// @param: pmax:    [in] Pointer to the max-branch that should be merged.
// @param: presult: [out+opt] The final, merged branch.
// @return: KE_OK:    Successfully merged the given branches.
// @return: KE_NOMEM: Not enough available memory to merge the branches.
extern __crit __nomp __wunused __nonnull((1,3)) kerrno_t
kshmbrach_merge(struct kshmbranch **__restrict pmin, __uintptr_t min_semi,
                struct kshmbranch **__restrict pmax, __uintptr_t max_semi,
         /*opt*/struct kshmbranch ***presult, /*opt*/__uintptr_t *presult_semi);

//////////////////////////////////////////////////////////////////////////
// (Re-)maps the region associated with a given
// branch within the given page directory.
// NOTE: This function automatically looks at region flags,
//       as well as reference counters to determine whether
//       some cluster of pages can be mapped as read-write,
//       of must be mapped to allow for copy-on-write later
//       down the line.
// @return: KE_OK:    Successfully mapped the given branch.
// @return: KE_NOMEM: Not enough memory to create the necessary mappings.
extern __crit __nomp __wunused __nonnull((1,2)) kerrno_t
kshmbranch_remap(struct kshmbranch const *__restrict self,
                 struct kpagedir *__restrict pd);


typedef __u32 kshmunmap_flag_t; /*< A set of 'KSHMUNMAP_FLAG_*' */
#define KSHMUNMAP_FLAG_NONE       0x00000000
#define KSHMUNMAP_FLAG_RESTRICTED KSHMREGION_FLAG_RESTRICTED /*< Allow unmapping of restricted regions. */


//////////////////////////////////////////////////////////////////////////
// Unmaps all mapped pages within the specified user-address-range.
// NOTE: The SHM tree is searched recursively for potential branches.
// @return: * : The total sum of all pages successfully unmapped.
extern __crit __nomp __wunused __nonnull((1,2)) __size_t
kshmbranch_unmap(struct kshmbranch **__restrict proot,
                 struct kpagedir *__restrict pd,
                 __pagealigned __uintptr_t addr_min,
                               __uintptr_t addr_max,
                 __uintptr_t addr_semi, unsigned int addr_level,
                 kshmunmap_flag_t flags, struct kshmregion *origin);


//////////////////////////////////////////////////////////////////////////
// Unmaps a portion of the given branch '*pself'.
// This function is used when a call to 'kshmbranch_unmap'
// specifies an address range only covering a part of some
// branch, instead of its entirety.
// In such a situation, this function is called to perform
// the necessary transformations that allow use to split
// the branch (if necessary), or to simply trim it at the
// front or its back.
// In the event that the specified range truly doesn't border
// up against the end of the branch, there sadly is a situation in
// which this function can (and of all reasons, it'll be of-of-memory).
// WARNING: The given 'first_page' is an offset within the SHM region of '*pself'.
//          It is _NOT_ an offset within the area of that region mapped by '*pself'!
// @return: KE_OK:    Successfully truncated/split the given branch and unmapped
//                    the associated pages from the given page directory.
// @return: KE_NOMEM: Not enough memory to create a necessary split.
extern __crit __nomp __wunused __nonnull((1,2)) kerrno_t
kshmbranch_unmap_portion(struct kshmbranch **__restrict pself,
                         struct kpagedir *__restrict pd,
                         kshmregion_page_t first_page,
                         __uintptr_t page_count,
                         __uintptr_t addr_semi);


//////////////////////////////////////////////////////////////////////////
// Recursively initializes an SHM-branch tree from the given source.
// Initialization is performed according to copy-on-write
// rules, with a fallback of using hard copies.
// NOTE: COW-style regions are either re-mapped as read-only
//       in the given 'right_pd', or are hard-copied.
// @return: KE_NOMEM: Not enough available memory.
extern __crit __nomp __wunused __nonnull((1,2,3,4)) kerrno_t
kshmbranch_fillfork(struct kshmbranch **__restrict proot, struct kpagedir *__restrict self_pd,
                    struct kshmbranch *__restrict source, struct kpagedir *__restrict source_pd);
#endif /* !__ASSEMBLY__ */






#define KSHMMAP_SIZEOF         (__SIZEOF_POINTER__)
#define KSHMMAP_OFFSETOF_ROOT  (0)
#ifndef __ASSEMBLY__
struct kshmmap {
 struct kshmbranch *m_root; /*< [0..1][owned] Root branch. */
};
#define kshmmap_init(self) (void)((self)->m_root = NULL)
#define kshmmap_quit(self) ((self)->m_root ? kshmbranch_deltree((self)->m_root) : (void)0)



#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Insert a given node into the given SHM region map.
// @return: KE_OK:     Successfully inserted the given node.
// @return: KE_EXISTS: Some other branch was already using some
//                     part of the memory specified within 'newnode'.
// NOTE: The caller is responsible to ensure sufficient
//       references can actually be created to the given entry.
extern __crit __nomp __nonnull((1,2)) kerrno_t
kshmmap_insert(struct kshmmap *__restrict self,
               struct kshmbranch *__restrict newleaf);

//////////////////////////////////////////////////////////////////////////
// The reverse of insert, remove the leaf at a given address.
// @return: * :   The inherited pointer of what was previously the entry
//                possible to locate by calling 'kshmbranch_locate(...)'.
// @return: NULL: No branch was associated with the given address.
extern __crit __nomp __nonnull((1)) struct kshmbranch *
kshmmap_remove(struct kshmmap *__restrict self,
               __uintptr_t addr);

//////////////////////////////////////////////////////////////////////////
// Locate the SHM leaf associated with the given address.
// @return: * :   The branch associated with the given address.
// @return: NULL: No branch was associated with the given address.
__local __nonnull((1)) struct kshmbranch *
kshmmap_locate(struct kshmmap const *__restrict self,
               __uintptr_t addr);
#else
#define kshmmap_insert(self,newleaf) kshmbranch_insert(&(self)->m_root,newleaf,KSHMBRANCH_ADDRSEMI_INIT,KSHMBRANCH_ADDRLEVEL_INIT)
#define kshmmap_remove(self,addr)    kshmbranch_remove(&(self)->m_root,addr)
#define kshmmap_locate(self,addr)    kshmbranch_locate((self)->m_root,addr)
#endif
#endif /* !__ASSEMBLY__ */







#define KLDT_SIZEOF           (4+__SIZEOF_POINTER__*2)
#define KLDT_OFFSETOF_GDTID   (0)
#define KLDT_OFFSETOF_LIMIT   (2)
#define KLDT_OFFSETOF_BASE    (4)
#define KLDT_OFFSETOF_VECTOR  (4+__SIZEOF_POINTER__)
#ifndef __ASSEMBLY__
COMPILER_PACK_PUSH(1)
struct __packed kldt {
 /* Local descriptor table (One for each process). */
 ksegid_t                                ldt_gdtid;  /*< [const] Associated GDT offset/index. */
 kseglimit_t                             ldt_limit;  /*< LDT vector limit. */
 __user struct ksegment                 *ldt_base;   /*< [0..1] User-mapped base address of the LDT vector. */
 __pagealigned __kernel struct ksegment *ldt_vector; /*< [0..1][owned] Physical address of the LDT vector. */
};
COMPILER_PACK_POP
#endif /* !__ASSEMBLY__ */





#define KSHM_OFFSETOF_LOCK (KOBJECT_SIZEOFHEAD)
#define KSHM_OFFSETOF_PD   (KOBJECT_SIZEOFHEAD+KRWLOCK_SIZEOF)
#define KSHM_OFFSETOF_MAP  (KOBJECT_SIZEOFHEAD+KRWLOCK_SIZEOF+__SIZEOF_POINTER__)
#define KSHM_OFFSETOF_LDT  (KOBJECT_SIZEOFHEAD+KRWLOCK_SIZEOF+__SIZEOF_POINTER__+KSHMMAP_SIZEOF)
#define KSHM_SIZEOF        (KOBJECT_SIZEOFHEAD+KRWLOCK_SIZEOF+__SIZEOF_POINTER__+KSHMMAP_SIZEOF+KLDT_SIZEOF)
#ifndef __ASSEMBLY__
struct kshm {
 /* Main, per-process controller structure for everything memory-related. */
 KOBJECT_HEAD
 struct krwlock   s_lock; /*< R/W lock used to synchronize access to this structure. */
 struct kpagedir *s_pd;   /*< [lock(s_lock)][1..1][owned] Per-process page directory (NOTE: May contain more mappings than those of 's_map', though not less!). */
 struct kshmmap   s_map;  /*< [lock(s_lock)] Map used for mapping a given user-space pointer to a mapped region of memory. */
 struct kldt      s_ldt;  /*< [lock(s_lock)] The per-process local descriptor table. */
};


//////////////////////////////////////////////////////////////////////////
// Initialize/Finalize a given SHM per-process controller structure.
// @return: KE_OK:    Successfully initialized the given structure.
// @return: KE_NOMEM: Not enough available memory (Such as when allocating the page directory).
extern __crit __wunused __nonnull((1)) kerrno_t
kshm_init(struct kshm *__restrict self,
          kseglimit_t ldt_size_hint);
#ifdef __INTELLISENSE__
extern __crit __nonnull((1)) void
kshm_quit(struct kshm *__restrict self);
#else
#define kshm_quit  (void)kshm_close
#endif

//////////////////////////////////////////////////////////////////////////
// Close a given SHM manager.
// @return: KE_OK:        Successfully closed the given manager.
// @return: KS_UNCHANGED: The given manager was already closed.
extern __crit __nonnull((1)) kerrno_t
kshm_close(struct kshm *__restrict self);

//////////////////////////////////////////////////////////////////////////
// Performs all the operations necessary to initialized 'self' as
// the counterpart of 'right', as seen during a fork() operation.
// This means that a new page directory is created, important
// areas of kernel memory are re-mapped in that directory, all
// mapped regions of memory are re-mapped as read-only in both
// the new, as well as page directory of 'right', as well as the
// entire mapping tree being copied, incrementing the user-counters
// of all regions before sharing all of them by reference.
// NOTE: When calling 'kshm_initfork_unlocked', the caller
//       must hold a write-lock on the given 'right' SHM.
// @return: KE_OK:        Successfully initialized 'self' as a fork()-copy of 'right'.
// @return: KE_NOMEM:     Not enough available memory.
// @return: KE_DESTROYED: [kshm_initfork] The given SHM was closed.
// @return: KE_INTR:      [kshm_initfork] The calling task was interrupted.
extern __crit __nomp __wunused __nonnull((1,2)) kerrno_t
kshm_initfork_unlocked(struct kshm *__restrict self,
                       struct kshm *__restrict right);
extern __crit __wunused __nonnull((1,2)) kerrno_t
kshm_initfork(struct kshm *__restrict self,
              struct kshm *__restrict right);



//////////////////////////////////////////////////////////////////////////
// Map a given amount of pages, starting at a given page-address from
// the given region to (yet another) given virtual user-space pointer.
// NOTE: The caller is responsible that no pages between 'address' and
//       'address+in_region_page_count' have been manually mapped, such
//       as as part of a special kernel-mapping with the page directory.
//       The fact that the specified memory isn't used is _NOT_ checked
//       by this function, though it will make sure that no memory in
//       the specified area is already in-use by other SHM-style mappings.
// NOTE: The given 'region' may already be in use by another process,
//       though in this case the caller is responsible to ensure that
//       the given region is either allowed to be shared between processes,
//       or to keep a write-lock on all other process's SHM locks,
//       as well as remap the region as read-only in all of them upon
//       success of this function.
// HINT: If the caller doesn't desire to map the given region at
//       the fixed address 'address', 'kpagedir_findfreerange' should
//       be called to locate a free region of sufficient size, and
//       the found region should be passed instead.
// HINT: To simply map dynamic memory, allocate a region
//       and then pass that region to this function.
// WARNING: If the given region is unique and the function returns success,
//          the given region may no longer point to a valid block of memory.
//          This can happen due to automatic region merging that is used
//          to merge the given 'region' with additional regions that may
//          or may not exist immediately below and above the given one.
//    HINT: You can easily suppress this behavior by passing
//          a region who's reference (aka. 'sre_branches')
//          counter is greater than ONE(1), indicating that the
//          region isn't unique and cannot be modified directly,
//          as would be required during the process of merging.
// NOTE: When calling '*_unlocked', the caller must
//       hold a write-lock on the given 'self' SHM.
// @return: KE_OK:         Successfully mapped the given region of memory.
// @return: KE_EXISTS:     Some part of the given area of memory was
//                         already mapped by an SHM-style mapping.
// @return: KE_NOMEM:      Not enough kernel memory to allocate the
//                         necessary control entries and structures.
// @return: KE_DESTROYED: [!*_unlocked] The given SHM was closed.
// @return: KE_INTR:      [!*_unlocked] The calling task was interrupted.
extern __crit __nomp __wunused __nonnull((1,3)) kerrno_t
kshm_mapregion_inherited_unlocked(struct kshm *__restrict self,
                                  __pagealigned __user void *address,
                                  __ref struct kshmregion *__restrict region,
                                  kshmregion_page_t in_region_page_start,
                                  __size_t in_region_page_count);
extern __crit __nomp __wunused __nonnull((1,3)) kerrno_t
kshm_mapregion_unlocked(struct kshm *__restrict self,
                        __pagealigned __user void *address,
                        struct kshmregion *__restrict region,
                        kshmregion_page_t in_region_page_start,
                        __size_t in_region_page_count);
extern __crit __wunused __nonnull((1,3)) kerrno_t
kshm_mapregion_inherited(struct kshm *__restrict self,
                         __pagealigned __user void *address,
                         __ref struct kshmregion *__restrict region,
                         kshmregion_page_t in_region_page_start,
                         __size_t in_region_page_count);
extern __crit __wunused __nonnull((1,3)) kerrno_t
kshm_mapregion(struct kshm *__restrict self,
               __pagealigned __user void *address,
               __ref struct kshmregion *__restrict region,
               kshmregion_page_t in_region_page_start,
               __size_t in_region_page_count);
__local __crit __nomp __wunused __nonnull((1,3)) kerrno_t
kshm_mapfullregion_inherited_unlocked(struct kshm *__restrict self,
                                      __pagealigned __user void *address,
                                      struct kshmregion *__restrict region);
__local __crit __nomp __wunused __nonnull((1,3)) kerrno_t
kshm_mapfullregion_unlocked(struct kshm *__restrict self,
                            __pagealigned __user void *address,
                            struct kshmregion *__restrict region);
__local __crit __wunused __nonnull((1,3)) kerrno_t
kshm_mapfullregion_inherited(struct kshm *__restrict self,
                             __pagealigned __user void *address,
                             struct kshmregion *__restrict region);
__local __crit __wunused __nonnull((1,3)) kerrno_t
kshm_mapfullregion(struct kshm *__restrict self,
                   __pagealigned __user void *address,
                   struct kshmregion *__restrict region);


//////////////////////////////////////////////////////////////////////////
// Fast-pass for automatically mapping a given region.
// Behaves the same as call 'kpagedir_findfreerange' to
// find a suitable user-space address before passing
// that value in a call to 'kshm_mapregion'.
// NOTE: When calling '*_unlocked', the caller must
//       hold a write-lock on the given 'self' SHM.
// @param: hint: Used as a hint when searching for usable space in the page directory.
// @return: KE_OK:         Successfully mapped the given region of memory.
// @return: KE_NOMEM:      Not enough kernel memory to allocate the
//                         necessary control entries and structures.
// @return: KE_NOSPC:      Failed to find an unused region of sufficient size.
// @return: KE_DESTROYED: [!*_unlocked] The given SHM was closed.
// @return: KE_INTR:      [!*_unlocked] The calling task was interrupted.
extern __crit __nomp __wunused __nonnull((1,2,3)) kerrno_t
kshm_mapautomatic_inherited_unlocked(struct kshm *__restrict self,
                              /*out*/__pagealigned __user void **__restrict user_address,
                                     __ref struct kshmregion *__restrict region, __pagealigned __user void *hint,
                                     kshmregion_page_t in_region_page_start, __size_t in_region_page_count);
extern __crit __wunused __nonnull((1,2,3)) kerrno_t
kshm_mapautomatic_inherited(struct kshm *__restrict self,
                     /*out*/__pagealigned __user void **__restrict user_address,
                            __ref struct kshmregion *__restrict region, __pagealigned __user void *hint,
                            kshmregion_page_t in_region_page_start, __size_t in_region_page_count);
extern __crit __nomp __wunused __nonnull((1,2,3)) kerrno_t
kshm_mapautomatic_unlocked(struct kshm *__restrict self,
                    /*out*/__pagealigned __user void **__restrict user_address,
                           struct kshmregion *__restrict region, __pagealigned __user void *hint,
                           kshmregion_page_t in_region_page_start, __size_t in_region_page_count);
extern __crit __nomp __wunused __nonnull((1,2,3)) kerrno_t
kshm_mapautomatic(struct kshm *__restrict self,
           /*out*/__pagealigned __user void **__restrict user_address,
                  struct kshmregion *__restrict region, __pagealigned __user void *hint,
                  kshmregion_page_t in_region_page_start, __size_t in_region_page_count);
__local __crit __nomp __wunused __nonnull((1,2,3)) kerrno_t
kshm_mapfullautomatic_unlocked(struct kshm *__restrict self,
                        /*out*/__pagealigned __user void **__restrict user_address,
                               struct kshmregion *__restrict region, __pagealigned __user void *hint);
__local __crit __nomp __wunused __nonnull((1,2,3)) kerrno_t
kshm_mapfullautomatic(struct kshm *__restrict self,
               /*out*/__pagealigned __user void **__restrict user_address,
                      struct kshmregion *__restrict region, __pagealigned __user void *hint);

//////////////////////////////////////////////////////////////////////////
// Fast-pass for quickly mapping (linear) RAM.
// Behaves the same as first allocating a new region, then
// calling 'kshm_mapregion_inherited' to map that region after
// finding a suitable location using 'kpagedir_findfreerange'.
// @param: hint:  Used as a hint when searching for usable space in the page directory.
// @param: flags: A set of 'KSHMREGION_FLAG_*' describing permissions to use for the region.
// @return: KE_OK:    Successfully mapped the given region of memory.
// @return: KE_NOMEM: Not enough kernel memory to allocate the
//                    necessary control entries and structures.
// @return: KE_NOSPC: Failed to find an unused region of sufficient size.
extern __crit __nomp __wunused __nonnull((1,2)) kerrno_t
kshm_mapram_unlocked(struct kshm *__restrict self,
              /*out*/__pagealigned __user void **__restrict user_address,
                     __size_t pages, __pagealigned __user void *hint, kshm_flag_t flags);
extern __crit __nomp __wunused __nonnull((1,2,3)) kerrno_t
kshm_mapram_linear_unlocked(struct kshm *__restrict self,
                     /*out*/__pagealigned __kernel void **__restrict kernel_address,
                     /*out*/__pagealigned __user   void **__restrict user_address,
                            __size_t pages, __pagealigned __user void *hint, kshm_flag_t flags);
extern __crit __nomp __wunused __nonnull((1,3)) kerrno_t
kshm_mapphys_unlocked(struct kshm *__restrict self,
                      __pagealigned __kernel void *__restrict phys_address,
               /*out*/__pagealigned __user void **__restrict user_address,
                      __size_t pages, __pagealigned __user void *hint, kshm_flag_t flags);
extern __crit __wunused __nonnull((1,2)) kerrno_t
kshm_mapram(struct kshm *__restrict self,
     /*out*/__pagealigned __user void **__restrict user_address,
            __size_t pages, __pagealigned __user void *hint, kshm_flag_t flags);
extern __crit __wunused __nonnull((1,2,3)) kerrno_t
kshm_mapram_linear(struct kshm *__restrict self,
            /*out*/__pagealigned __kernel void **__restrict kernel_address,
            /*out*/__pagealigned __user   void **__restrict user_address,
                   __size_t pages, __pagealigned __user void *hint, kshm_flag_t flags);
extern __crit __wunused __nonnull((1,3)) kerrno_t
kshm_mapphys(struct kshm *__restrict self,
             __pagealigned __kernel void *__restrict phys_address,
      /*out*/__pagealigned __user void **__restrict user_address,
             __size_t pages, __pagealigned __user void *hint, kshm_flag_t flags);


//////////////////////////////////////////////////////////////////////////
// Touch the cluster potentially mapped at the given address.
// If the cluster belongs to a writable region of memory,
// but has been mapped as read-only in order to allow for
// copy-on-write semantics, that cluster (and that cluster only)
// will be copied while creating up to THREE(3) new branches,
// as well as up to ONE(1) new region covering the area of
// mapped memory immediately before, referencing the existing
// region of memory, a new branch covering the specified address,
// now containing a hard copy of the affected cluster, and
// similar to the first branch, a third one also referencing
// the original region afterwards:
//     
//  R1 [..........X.........]
//      |        ||\        \
//      |        |\ \        \
//      |        | | \        \
//      |        | \  \        \
//      |        | dup \        \
//      |        |  \   \        |
//      |        |   |   |       |
//      |        |   |   |       |
//      |   R1   |   R2  |  R1   |
//     [..........] [.] [.........]
//      |            |   |
//      +------------|---|--- #1 branch (sub-range of R1)
//                   +---|--- #2 branch (contains a copy of the cluster at 'address')
//                       +--- #3 branch (sub-range of R1)
//
// Special optimizations are performed to merge small consecutive
// regions of memory located directly next to each other in the case
// of consecutive write operations covering multiple clusters.
// NOTE: When calling 'kshm_touch_unlocked', the caller
//       must hold a write-lock on the given 'self' SHM.
// @param: req_flags: A set of flags that the found branch must have
//                    set in order to allow touching to commence.
//                    For request originating from user-space, this argument
//                    should be set to 'KSHMREGION_FLAG_READ|KSHMREGION_FLAG_WRITE',
//                    enforcing both read and write access before allowing a region
//                    of SHM-mapped memory to be touched, yet since other things, such as
//                    the ELF linker require write-access to otherwise read-only regions,
//                    they will pass ZERO(0) for this parameter to not
//                    indicate any special privilege requirements.
// @return: 0 : Failed to touch anything.
//        NOTE: Touching may fail for multiple reasons, including
//              not enough memory, as well as a faulty user pointer.
//        NOTE: The given 'address' must point into a valid region of mapped memory,
//              and this function is not capable of touching regions
//              of memory spanning multiple branches.
//              At most 'touch_pages' are touched from the mapping at 'address'.
// @return: * : The amount of successfully touched pages.
// WARNING: Upon success, more pages than specified may get touched, as
//          the algorithm attempts to align touches to cluster boundaries.
extern __crit __nomp __nonnull((1)) __size_t
kshm_touch_unlocked(struct kshm *__restrict self,
                    __pagealigned __user void *address,
                    __size_t touch_pages, kshm_flag_t req_flags);
extern __crit __nonnull((1)) __size_t
kshm_touch(struct kshm *__restrict self,
           __pagealigned __user void *address,
           __size_t touch_pages, kshm_flag_t req_flags);

//////////////////////////////////////////////////////////////////////////
// Similar to 'kshm_touch', but capable of touching all pages
// from all branches within the given area of memory.
// NOTE: When calling 'kshm_touchall_unlocked', the caller
//       must hold a write-lock on the given 'self' SHM.
// @return: * : The amount of successfully touched pages (sum).
extern __crit __nomp __nonnull((1)) __size_t
kshm_touchall_unlocked(struct kshm *__restrict self,
                       __pagealigned __user void *address,
                       __size_t touch_pages, kshm_flag_t req_flags);
extern __crit __nonnull((1)) __size_t
kshm_touchall(struct kshm *__restrict self,
              __pagealigned __user void *address,
              __size_t touch_pages, kshm_flag_t req_flags);

//////////////////////////////////////////////////////////////////////////
// Similar to 'kshm_touch', but allows for finer control
// over what pages in which branch should be touched.
// NOTE: When calling 'kshm_touchex_unlocked', the caller
//       must hold a write-lock on the given 'self' SHM.
// @return: * : The amount of successfully touched pages.
extern __crit __nomp __nonnull((1,2)) __size_t
kshm_touchex_unlocked(struct kshm *__restrict self,
                      struct kshmbranch **__restrict pbranch,
                      __uintptr_t addr_semi,
                      kshmregion_page_t min_page,
                      kshmregion_page_t max_page);
extern __crit __nonnull((1,2)) __size_t
kshm_touchex(struct kshm *__restrict self,
             struct kshmbranch **__restrict pbranch,
             __uintptr_t addr_semi,
             kshmregion_page_t min_page,
             kshmregion_page_t max_page);




//////////////////////////////////////////////////////////////////////////
// Unmap all SHM memory mappings matching the description
// provided by 'flags' within the given region of page-based memory.
// NOTE: This function will perform all the necessary adjustments to
//       use counters, as well as updating the page directory before
//       returning. - There is no ~real~ fail-case for this function.
// WARNING: If the given address range only scrapes an SHM-region,
//          that region will be sub-ranged, with only apart of it
//          remaining mapped. (Meaning that partial munmap() is allowed)
// NOTE: When calling 'kshm_unmap_unlocked', the caller
//       must hold a write-lock on the given 'self' SHM.
// @param: origin: When non-NULL, only allow regions that originated from this to be unmapped.
// @return: 0 : Nothing was unmapped, [kshm_unmap] or the given SHM was closed.
// @return: * : The amount of actually unmapped pages (as removed from the page directory).
extern __crit __nomp __nonnull((1)) __size_t
kshm_unmap_unlocked(struct kshm *__restrict self,
                    __pagealigned __user void *address,
                    __size_t pages, kshmunmap_flag_t flags,
                    struct kshmregion *origin);
extern __crit __nonnull((1)) __size_t
kshm_unmap(struct kshm *__restrict self,
           __pagealigned __user void *address,
           __size_t pages, kshmunmap_flag_t flags,
           struct kshmregion *origin);

//////////////////////////////////////////////////////////////////////////
// Unmaps a specific region mapped to a given 'base_address'.
// NOTE: Although much less powerful than 'kshm_unmap', this function
//       does allow you to safely unmap a given region from a given
//       address, allowing you to ensure that you aren't accidentally
//       unmapping something other than what you're expecting to find
//       at a given address, given the fact that user-code can freely
//       unmap anything that wasn't mapped with 'KSHMREGION_FLAG_RESTRICTED'.
// NOTE: SHM internally tracks the origin of regions, meaning that
//       as a fallback, this function will automatically unmap all
//       regions with the given 'region' as origin in the area covering
//       'base_address' up to the region's size in the event that the
//       region was not found.
// @return: KE_FAULT:      Nothing was mapped to the given 'base_address'.
// @return: KE_RANGE:      The given 'base_address' doesn't point to the base of a mapped region.
// @return: KE_PERM:       The given 'region' didn't match the one mapped at 'base_address'.
// @return: KE_OK:         Successfully unmapped the entirety of the given region.
// @return: KE_DESTROYED: [kshm_unmapregion] The given SHM was closed.
// @return: KE_INTR:      [kshm_unmapregion] The calling task was interrupted.
extern __crit __nomp __nonnull((1,3)) kerrno_t
kshm_unmapregion_unlocked(struct kshm *__restrict self,
                          __pagealigned __user void *base_address,
                          struct kshmregion *__restrict region);
extern __crit __wunused __nonnull((1,3)) kerrno_t
kshm_unmapregion(struct kshm *__restrict self,
                 __pagealigned __user void *base_address,
                 struct kshmregion *__restrict region);


//////////////////////////////////////////////////////////////////////////
// Checks if the given thread 'caller' has access to
// the specified area of physical (aka. device) memory.
// @return: KE_OK:    Access is granted.
// @return: KE_ACCES: They don't...
extern __crit __nonnull((1)) kerrno_t
kshm_devaccess(struct ktask *__restrict caller,
               __pagealigned __kernel void const *base_address,
               __size_t pages);



/* === Local descriptor table === */

//////////////////////////////////////////////////////////////////////////
// Allocate/Free segments within a local descriptor table.
// Upon successful allocation, the new segment will have already
// been flushed and capable of being used for whatever means necessary.
// NOTE: On success, the returned index always compares true for 'KSEG_ISLDT()'
// NOTE: When calling '*_unlocked', the caller
//       must hold a write-lock on the given 'self' SHM.
// @param: seg:   The initial contents of the segment to-be allocated.
//                HINT: Contents can later be changed through calls to 'kldt_setseg'.
// @param: reqid: The requested segment id to allocate the entry at.
//                NOTE: This value must be compare true for 'KSEG_ISLDT()'
// @return: * :        Having successfully registered the given segment,
//                     this function returns a value capable of being
//                     written into a segment register without causing
//                     what is the origin on the term 'SEGFAULT'.
// @return: reqid:     Successfully reserved the given index.
// @return: KSEG_NULL: Failed to allocate a new segment (no-memory/too-many-segments)
//                    [kldt_allocsegat] The given ID is already in use.
extern __crit __nomp __wunused __nonnull((1,2)) ksegid_t kshm_ldtalloc_unlocked(struct kshm *__restrict self, struct ksegment const *__restrict seg);
extern __crit __nomp __wunused __nonnull((1,3)) ksegid_t kshm_ldtallocat_unlocked(struct kshm *__restrict self, ksegid_t reqid, struct ksegment const *__restrict seg);
extern __crit __nomp           __nonnull((1)) void kshm_ldtfree_unlocked(struct kshm *__restrict self, ksegid_t id);
extern __crit __wunused __nonnull((1,2)) ksegid_t kshm_ldtalloc(struct kshm *__restrict self, struct ksegment const *__restrict seg);
extern __crit __wunused __nonnull((1,3)) ksegid_t kshm_ldtallocat(struct kshm *__restrict self, ksegid_t reqid, struct ksegment const *__restrict seg);
extern __crit           __nonnull((1)) void kshm_ldtfree(struct kshm *__restrict self, ksegid_t id);

//////////////////////////////////////////////////////////////////////////
// Get/Set the segment data associated with a given segment ID.
// NOTE: 'kldt_setseg' will flush the changed segment upon success.
// NOTE: When calling 'kshm_ldtget_unlocked', the caller must hold a read-lock on the given 'self' SHM.
// NOTE: When calling 'kshm_ldtset_unlocked', the caller must hold a write-lock on the given 'self' SHM.
// @return: KE_OK:        [!*_unlocked] Successfully read/wrote the given segment.
// @return: KE_DESTROYED: [!*_unlocked] The given SHM was closed.
// @return: KE_INTR:      [!*_unlocked] The calling task was interrupted.
extern __nomp __nonnull((1,3)) void kshm_ldtget_unlocked(struct kshm const *__restrict self, ksegid_t id, struct ksegment *__restrict seg);
extern __nomp __nonnull((1,3)) void kshm_ldtset_unlocked(struct kshm *__restrict self, ksegid_t id, struct ksegment const *__restrict seg);
#ifdef __INTELLISENSE__
extern __wunused __nonnull((1,3)) kerrno_t kshm_ldtget(struct kshm const *__restrict self, ksegid_t id, struct ksegment *__restrict seg);
extern __wunused __nonnull((1,3)) kerrno_t kshm_ldtset(struct kshm *__restrict self, ksegid_t id, struct ksegment const *__restrict seg);
#else
extern __crit __wunused __nonnull((1,3)) kerrno_t __kshm_ldtget_c(struct kshm const *__restrict self, ksegid_t id, struct ksegment *__restrict seg);
extern __crit __wunused __nonnull((1,3)) kerrno_t __kshm_ldtset_c(struct kshm *__restrict self, ksegid_t id, struct ksegment const *__restrict seg);
#define kshm_ldtget(self,id,seg) KTASK_CRIT(__kshm_ldtget_c(self,id,seg))
#define kshm_ldtset(self,id,seg) KTASK_CRIT(__kshm_ldtset_c(self,id,seg))
#endif



struct ktranslator {
 struct kshm           *t_shm;  /*< [1..1][const] Associated SHM. */
 struct kpagedir const *t_epd;  /*< [1..1][const] Effective page directory. */
#define KTRANSLATOR_FLAG_NONE 0x00000000
#define KTRANSLATOR_FLAG_LOCK 0x00000001 /*< When set, the translator holds a read-lock on 't_shm'. */
#define KTRANSLATOR_FLAG_SEPD 0x00000002 /*< When set, 't_epd == t_shm->s_pd'. */
#define KTRANSLATOR_FLAG_KEPD 0x00000004 /*< When set, 't_epd' is 'kpagedir_kernel()'. */
#define KTRANSLATOR_FLAG_RQLK KTRANSLATOR_FLAG_SEPD /*< The translator requires a read-lock to function. */
 __u32                  t_flags; /*< A set of 'KTRANSLATOR_FLAG_*'. */
};

//////////////////////////////////////////////////////////////////////////
// Initialize a translator describing the ring-#3
// address space and EPD associated with the given task.
// @return: KE_OK:        Successfully initialized the translator.
// @return: KE_DESTROYED: The associated SHM was closed.
#ifdef __INTELLISENSE__
extern __crit kerrno_t ktranslator_init(struct ktranslator *__restrict self, struct ktask *__restrict caller);
extern __crit kerrno_t ktranslator_init_shm(struct ktranslator *__restrict self, struct kshm *__restrict shm);
extern __crit void     ktranslator_quit(struct ktranslator *__restrict self);
#else
#define ktranslator_init(self,caller) \
 (/*KTASK_ISKEPD_P   ? (__ktranslator_init_kepd(self),KE_OK) :*/\
  KTASK_ISNOINTR_P ?  __ktranslator_init_nointr(self,caller)\
                   :  __ktranslator_init_intr(self,caller))
__local __crit kerrno_t
ktranslator_init_shm(struct ktranslator *__restrict self,
                     struct kshm *__restrict shm) {
 kerrno_t error = KE_OK;
 KTASK_CRIT_MARK
 kassertobj(self);
 kassert_kshm(shm);
 self->t_shm = shm;
 if ((self->t_epd = shm->s_pd) == kpagedir_kernel()) {
  self->t_flags = KTRANSLATOR_FLAG_KEPD;
 } else {
  self->t_flags = KTRANSLATOR_FLAG_LOCK|KTRANSLATOR_FLAG_SEPD;
  KTASK_NOINTR_BEGIN
  error = krwlock_beginread(&self->t_shm->s_lock);
  KTASK_NOINTR_END
 }
 return error;
}
__local __crit kerrno_t
__ktranslator_init_intr(struct ktranslator *__restrict self,
                        struct ktask *__restrict caller);
__local __crit kerrno_t
__ktranslator_init_nointr(struct ktranslator *__restrict self,
                          struct ktask *__restrict caller);
__local __crit void
__ktranslator_init_kepd(struct ktranslator *__restrict self) {
#ifdef kpagedir_kernel
 self->t_epd = kpagedir_kernel();
#else
 extern struct kpagedir __kpagedir_kernel;
 self->t_epd = &__kpagedir_kernel;
#endif
 self->t_flags = KTRANSLATOR_FLAG_KEPD;
}
__local __crit void
ktranslator_quit(struct ktranslator *__restrict self) {
 KTASK_CRIT_MARK
 kassertobj(self);
 if (self->t_flags&KTRANSLATOR_FLAG_LOCK) {
  krwlock_endread(&self->t_shm->s_lock);
 }
}
#endif

#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Translate a given user-space pointer to its physical counterpart.
// @param: addr:     A virtual user-space pointer.
// @param: maxbytes: The max amount of bytes of confirm (*rwbytes
//                   will not be set to something higher than this).
// @param: rwbytes:  Mandatory output parameter that stores the amount
//                   of linear bytes available for operations.
//                   NOTE: In all situations, it can be assumed that
//                         upon return this value is '<= maxbytes'.
// @param: writable: When non-ZERO, perform translation with the intent
//                   of writing to the returned pointer upon success.
//                   >> If the user-space address is part of an uncopied copy-on-write page,
//                      that page is copied the same way a page-fault would when ring-#3
//                      user-code had tried to write to the same address.
//                   >> If the region of memory associated with the address is not writable
//                      to the user, NULL is returned and '*rwbytes' is set to ZERO(0).
// @return: * :       The physical counterpart to the given virtual address.
// @return: NULL :    '*rwbytes' was set to ZERO(0) because the given pointer isn't mapped.
extern __crit __wunused __nonnull((1,4)) __kernel void *
ktranslator_exec(struct ktranslator *__restrict self, __user void const *addr,
                 __size_t maxbytes, __kernel __size_t *__restrict rwbytes, int writable);

//////////////////////////////////////////////////////////////////////////
// Same 'ktranslator_exec', but allows write-access to otherwise
// read-only regions, using the same semantics as though they were
// mapped as writable.
// WARNING: This should only be used by the kernel, as it bypasses
//          the intended restrictions of read-only memory, such as
//          '.text' sections, yet is required during processes,
//          such as dynamic relocations.
extern __crit __wunused __nonnull((1,4)) __kernel void *
ktranslator_wexec(struct ktranslator *__restrict self, __user void *addr,
                  __size_t maxbytes, __kernel __size_t *__restrict rwbytes);

extern __crit __wunused __nonnull((1)) __kernel void *
ktranslator_qexec(struct ktranslator *__restrict self, __user void *addr, int writable);
extern __crit __wunused __nonnull((1)) __kernel void *
ktranslator_qwexec(struct ktranslator *__restrict self, __user void *addr);
#else
extern __crit __wunused __nonnull((1,3)) __kernel void *
__ktranslator_exec_impl(struct ktranslator *__restrict self,
                        __user void const *addr,
                        __kernel __size_t *__restrict rwbytes,
                        int writable);
extern __crit __wunused __nonnull((1,3)) __kernel void *
__ktranslator_wexec_impl(struct ktranslator *__restrict self,
                         __user void *addr,
                         __kernel __size_t *__restrict rwbytes);
extern __crit __wunused __nonnull((1)) __kernel void *
__ktranslator_w1qexec_impl(struct ktranslator *__restrict self,
                           __user void const *addr);
extern __crit __wunused __nonnull((1)) __kernel void *
__ktranslator_qwexec_impl(struct ktranslator *__restrict self,
                          __user void *addr);
#define __ktranslator_w0qexec_impl(self,addr) \
 kpagedir_translate_flags((self)->t_epd,addr,X86_PTE_FLAG_PRESENT)
#define __ktranslator_qexec_impl(self,addr,writable) \
 ((writable) ? __ktranslator_w1qexec_impl(self,addr) \
             : __ktranslator_w0qexec_impl(self,addr))
#define __ktranslator_exec(self,addr,maxbytes,rwbytes,writable) \
 (*(rwbytes) = (maxbytes),__ktranslator_exec_impl(self,addr,rwbytes,writable))
#define __ktranslator_wexec(self,addr,maxbytes,rwbytes) \
 (*(rwbytes) = (maxbytes),__ktranslator_wexec_impl(self,addr,rwbytes))
#define __ktranslator_qexec   __ktranslator_qexec_impl
#define __ktranslator_qwexec  __ktranslator_qwexec_impl
#define ktranslator_exec(self,addr,maxbytes,rwbytes,writable) \
 ((__builtin_constant_p(maxbytes) && !KPAGEDIR_MAYBE_QUICKACCESS(maxbytes))\
  ? __ktranslator_exec(self,addr,maxbytes,rwbytes,writable)\
  : __xblock({ __user void const *__addr = (addr); __size_t const __maxbytes = (maxbytes);\
               __xreturn KPAGEDIR_CAN_QUICKACCESS(__addr,__maxbytes)\
                ? (*(rwbytes) = __maxbytes,__ktranslator_qexec(self,__addr,writable))\
                : __ktranslator_exec(self,__addr,__maxbytes,rwbytes,writable);\
    }))
#define ktranslator_wexec(self,addr,maxbytes,rwbytes) \
 ((__builtin_constant_p(maxbytes) && !KPAGEDIR_MAYBE_QUICKACCESS(maxbytes))\
  ? __ktranslator_wexec(self,addr,maxbytes,rwbytes)\
  : __xblock({ __user void *__addr = (addr); __size_t const __maxbytes = (maxbytes);\
               __xreturn KPAGEDIR_CAN_QUICKACCESS(__addr,__maxbytes)\
                ? (*(rwbytes) = __maxbytes,__ktranslator_qwexec(self,addr))\
                : __ktranslator_wexec(self,addr,maxbytes,rwbytes);\
    }))
#define ktranslator_qexec   __ktranslator_qexec
#define ktranslator_qwexec  __ktranslator_qwexec
#endif


#ifdef __INTELLISENSE__
extern __size_t ktranslator_copyinuser(struct ktranslator *__restrict self, __user void *dst, __user void const *src, __size_t bytes);
extern __size_t ktranslator_copytouser(struct ktranslator *__restrict self, __user void *dst, __kernel void const *src, __size_t bytes);
extern __size_t ktranslator_copyfromuser(struct ktranslator *__restrict self, __kernel void *dst, __user void const *src, __size_t bytes);
extern __size_t ktranslator_copyinuser_w(struct ktranslator *__restrict self, __user void *dst, __user void const *src, __size_t bytes);
extern __size_t ktranslator_copytouser_w(struct ktranslator *__restrict self, __user void *dst, __kernel void const *src, __size_t bytes);
#else
extern __size_t __ktranslator_copyinuser_impl(struct ktranslator *__restrict self, __user void *dst, __user void const *src, __size_t bytes);
extern __size_t __ktranslator_copytouser_impl(struct ktranslator *__restrict self, __user void *dst, __kernel void const *src, __size_t bytes);
extern __size_t __ktranslator_copyfromuser_impl(struct ktranslator *__restrict self, __kernel void *dst, __user void const *src, __size_t bytes);
extern __size_t __ktranslator_copyinuser_w_impl(struct ktranslator *__restrict self, __user void *dst, __user void const *src, __size_t bytes);
extern __size_t __ktranslator_copytouser_w_impl(struct ktranslator *__restrict self, __user void *dst, __kernel void const *src, __size_t bytes);
__local __size_t
__ktranslator_q_copyinuser_impl(struct ktranslator *__restrict self,
                                __user void *dst, __user void const *src,
                                __size_t bytes) {
 __kernel void *kdst,*ksrc;
 if __unlikely((kdst = ktranslator_qexec(self,dst,1)) == NULL ||
               (ksrc = ktranslator_qexec(self,src,0)) == NULL) return bytes;
 memcpy(kdst,ksrc,bytes);
 return 0;
}
__local __size_t
__ktranslator_q_copytouser_impl(struct ktranslator *__restrict self,
                                __user void *dst, __kernel void const *src,
                                __size_t bytes) {
 __kernel void *kdst = ktranslator_qexec(self,dst,1);
 if __unlikely(!kdst) return bytes;
 memcpy(kdst,src,bytes);
 return 0;
}
__local __size_t
__ktranslator_q_copyfromuser_impl(struct ktranslator *__restrict self,
                                  __kernel void *dst, __user void const *src,
                                  __size_t bytes) {
 __kernel void *ksrc = ktranslator_qexec(self,src,0);
 if __unlikely(!ksrc) return bytes;
 memcpy(dst,ksrc,bytes);
 return 0;
}
__local __size_t
__ktranslator_q_copyinuser_w_impl(struct ktranslator *__restrict self,
                                  __user void *dst, __user void const *src,
                                  __size_t bytes) {
 __kernel void *kdst,*ksrc;
 if __unlikely((kdst = ktranslator_qwexec(self,dst)) == NULL ||
               (ksrc = ktranslator_qexec(self,src,0)) == NULL) return bytes;
 memcpy(kdst,ksrc,bytes);
 return 0;
}
__local __size_t
__ktranslator_q_copytouser_w_impl(struct ktranslator *__restrict self,
                                  __user void *dst, __kernel void const *src,
                                  __size_t bytes) {
 __kernel void *kdst = ktranslator_qwexec(self,dst);
 if __unlikely(!kdst) return bytes;
 memcpy(kdst,src,bytes);
 return 0;
}
__forcelocal __size_t
ktranslator_copyinuser(struct ktranslator *__restrict self,
                       __user void *dst, __user void const *src,
                       __size_t bytes) {
 if (__builtin_constant_p(bytes) && !KPAGEDIR_MAYBE_QUICKACCESS(bytes)) {
  return bytes ? __ktranslator_copyinuser_impl(self,dst,src,bytes) : 0;
 } else if (KPAGEDIR_CAN_QUICKACCESS(dst,bytes) &&
            KPAGEDIR_CAN_QUICKACCESS(src,bytes)) {
  return __ktranslator_q_copyinuser_impl(self,dst,src,bytes);
 } else {
  return __ktranslator_copyinuser_impl(self,dst,src,bytes);
 }
}
__forcelocal __size_t
ktranslator_copytouser(struct ktranslator *__restrict self,
                       __user void *dst, __kernel void const *src,
                       __size_t bytes) {
 if (__builtin_constant_p(bytes) && !KPAGEDIR_MAYBE_QUICKACCESS(bytes)) {
  return bytes ? __ktranslator_copytouser_impl(self,dst,src,bytes) : 0;
 } else if (KPAGEDIR_CAN_QUICKACCESS(dst,bytes)) {
  return __ktranslator_q_copytouser_impl(self,dst,src,bytes);
 } else {
  return __ktranslator_copytouser_impl(self,dst,src,bytes);
 }
}
__forcelocal __size_t
ktranslator_copyfromuser(struct ktranslator *__restrict self,
                         __kernel void *dst, __user void const *src,
                         __size_t bytes) {
 if (__builtin_constant_p(bytes) && !KPAGEDIR_MAYBE_QUICKACCESS(bytes)) {
  return bytes ? __ktranslator_copyfromuser_impl(self,dst,src,bytes) : 0;
 } else if (KPAGEDIR_CAN_QUICKACCESS(src,bytes)) {
  return __ktranslator_q_copyfromuser_impl(self,dst,src,bytes);
 } else {
  return __ktranslator_copyfromuser_impl(self,dst,src,bytes);
 }
}
__forcelocal __size_t
ktranslator_copyinuser_w(struct ktranslator *__restrict self,
                         __user void *dst, __user void const *src,
                         __size_t bytes) {
 if (__builtin_constant_p(bytes) && !KPAGEDIR_MAYBE_QUICKACCESS(bytes)) {
  return bytes ? __ktranslator_copyinuser_w_impl(self,dst,src,bytes) : 0;
 } else if (KPAGEDIR_CAN_QUICKACCESS(dst,bytes) &&
            KPAGEDIR_CAN_QUICKACCESS(src,bytes)) {
  return __ktranslator_q_copyinuser_w_impl(self,dst,src,bytes);
 } else {
  return __ktranslator_copyinuser_w_impl(self,dst,src,bytes);
 }
}
__forcelocal __size_t
ktranslator_copytouser_w(struct ktranslator *__restrict self,
                         __user void *dst, __kernel void const *src,
                         __size_t bytes) {
 if (__builtin_constant_p(bytes) && !KPAGEDIR_MAYBE_QUICKACCESS(bytes)) {
  return bytes ? __ktranslator_copytouser_w_impl(self,dst,src,bytes) : 0;
 } else if (KPAGEDIR_CAN_QUICKACCESS(dst,bytes)) {
  return __ktranslator_q_copytouser_w_impl(self,dst,src,bytes);
 } else {
  return __ktranslator_copytouser_w_impl(self,dst,src,bytes);
 }
}
#endif



#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Translate a given user-space pointer to its physical counterpart.
// @param: addr:     A virtual user-space pointer.
// @param: maxbytes: The max amount of bytes of confirm (*rwbytes
//                   will not be set to something higher than this).
// @param: rwbytes:  Mandatory output parameter that stores the amount
//                   of linear bytes available for operations.
//                   NOTE: In all situations, it can be assumed that
//                         upon return this value is '<= maxbytes'.
// @param: writable: When non-ZERO, perform translation with the intent
//                   of writing to the returned pointer upon success.
//                   >> If the user-space address is part of an uncopied copy-on-write page,
//                      that page is copied the same way a page-fault would when ring-#3
//                      user-code had tried to write to the same address.
//                   >> If the region of memory associated with the address is not writable
//                      to the user, NULL is returned and '*rwbytes' is set to ZERO(0).
// @return: * :       The physical counterpart to the given virtual address.
// @return: NULL :    '*rwbytes' was set to ZERO(0) because the given pointer isn't mapped.
extern __crit __nomp __wunused __nonnull((1,2,5)) __kernel void *
kshm_translateuser(struct kshm const *__restrict self, struct kpagedir const *__restrict epd,
                   __user void const *addr, __size_t maxbytes,
                   __size_t *__restrict rwbytes, int writable);

//////////////////////////////////////////////////////////////////////////
// Same 'kshm_translateuser', but allows write-access to otherwise
// read-only regions, using the same semantics as though they were
// mapped as writable.
// WARNING: This should only be used by the kernel, as it bypasses
//          the intended restrictions of read-only memory, such as
//          '.text' sections, yet is required during processes,
//          such as dynamic relocations.
extern __crit __nomp __wunused __nonnull((1,2,5)) __kernel void *
kshm_wtranslateuser(struct kshm const *__restrict self, struct kpagedir const *__restrict epd,
                    __user void *addr, __size_t maxbytes,
                    __size_t *__restrict rwbytes);

//////////////////////////////////////////////////////////////////////////
// Quick-translate a given address.
// WARNING: Only use this if 'KPAGEDIR_CAN_QUICKACCESS' returned true.
extern __crit __nomp __wunused __nonnull((1,2)) __kernel void *
kshm_qtranslateuser(struct kshm const *__restrict self,
                    struct kpagedir const *__restrict epd,
                    __user void *addr, int writable);
extern __crit __nomp __wunused __nonnull((1,2)) __kernel void *
kshm_qwtranslateuser(struct kshm const *__restrict self,
                     struct kpagedir const *__restrict epd,
                     __user void *addr);
#else
extern __crit __nomp __wunused __nonnull((1,2,4)) __kernel void *
__kshm_translateuser_impl(struct kshm const *__restrict self, struct kpagedir const *__restrict epd,
                          __user void const *addr, __size_t *__restrict rwbytes, int writable);
extern __crit __nomp __wunused __nonnull((1,2,4)) __kernel void *
__kshm_wtranslateuser_impl(struct kshm const *__restrict self, struct kpagedir const *__restrict epd,
                           __user void *addr, __size_t *__restrict rwbytes);
extern __crit __nomp __wunused __nonnull((1,2)) __kernel void *
__kshm_qwtranslateuser_impl(struct kshm const *__restrict self,
                            struct kpagedir const *__restrict epd, __user void const *addr);
extern __crit __nomp __wunused __nonnull((1,2)) __kernel void *
__kshm_w1qtranslateuser_impl(struct kshm const *__restrict self,
                             struct kpagedir const *__restrict epd, __user void const *addr);
#define __kshm_w0qtranslateuser_impl(self,epd,addr)\
 kpagedir_translate_flags(epd,addr,X86_PTE_FLAG_PRESENT)
#define __kshm_qtranslateuser_impl(self,epd,addr,writable) \
 ((writable) ? __kshm_w1qtranslateuser_impl(self,epd,addr)\
             : __kshm_w0qtranslateuser_impl(self,epd,addr))
#define __kshm_qtranslateuser  __kshm_qtranslateuser_impl
#define __kshm_qwtranslateuser __kshm_qwtranslateuser_impl
#define __kshm_translateuser(self,epd,addr,maxbytes,rwbytes,writable) \
 (*(rwbytes) = (maxbytes),__kshm_translateuser_impl(self,epd,addr,rwbytes,writable))
#define __kshm_wtranslateuser(self,epd,addr,maxbytes,rwbytes) \
 (*(rwbytes) = (maxbytes),__kshm_wtranslateuser_impl(self,epd,addr,rwbytes))
#define kshm_translateuser(self,epd,addr,maxbytes,rwbytes,writable) \
 ((__builtin_constant_p(maxbytes) && !KPAGEDIR_MAYBE_QUICKACCESS(maxbytes))\
  ? __kshm_translateuser(self,epd,addr,maxbytes,rwbytes,writable)\
  : __xblock({ __user void const *__addr = (addr); __size_t const __maxbytes = (maxbytes);\
               __xreturn KPAGEDIR_CAN_QUICKACCESS(__addr,__maxbytes)\
                ? (*(rwbytes) = __maxbytes,__kshm_qtranslateuser(self,epd,addr,writable))\
                : __kshm_translateuser(self,epd,__addr,__maxbytes,rwbytes,writable);\
    }))
#define kshm_wtranslateuser(self,epd,addr,maxbytes,rwbytes) \
 ((__builtin_constant_p(maxbytes) && !KPAGEDIR_MAYBE_QUICKACCESS(maxbytes))\
  ? __kshm_wtranslateuser(self,epd,addr,maxbytes,rwbytes)\
  : __xblock({ __user void *__addr = (addr); __size_t const __maxbytes = (maxbytes);\
               __xreturn KPAGEDIR_CAN_QUICKACCESS(__addr,__maxbytes)\
                ? (*(rwbytes) = __maxbytes,__kshm_qwtranslateuser(self,epd,addr))\
                : __kshm_wtranslateuser(self,epd,__addr,__maxbytes,rwbytes);\
    }))
#define kshm_qtranslateuser   __kshm_qtranslateuser
#define kshm_qwtranslateuser  __kshm_qwtranslateuser
#endif


//////////////////////////////////////////////////////////////////////////
// Copy memory to/from/in user space.
// NOTE: These functions are simple wrappers around 'kshm_translateuser'.
// @param: dst:   The (potentially virtual) destination address.
// @param: src:   The (potentially virtual) source address.
// @param: bytes: The (max) amount of bytes to transfer.
// @return: 0 :   Success transferred all specified memory.
// @return: * :   Amount of bytes _NOT_ transferred.
#ifdef __INTELLISENSE__
extern __crit __nomp __wunused __nonnull((1))   __size_t kshm_copyinuser(struct kshm const *self, __user void *dst, __user void const *src, __size_t bytes);
extern __crit __nomp __wunused __nonnull((1,3)) __size_t kshm_copytouser(struct kshm const *self, __user void *dst, __kernel void const *src, __size_t bytes);
extern __crit __nomp __wunused __nonnull((1,2)) __size_t kshm_copyfromuser(struct kshm const *self, __kernel void *dst, __user void const *src, __size_t bytes);
#else
extern __crit __nomp __wunused __nonnull((1))   __size_t __kshm_copyinuser_impl(struct kshm const *self, __user void *dst, __user void const *src, __size_t bytes);
extern __crit __nomp __wunused __nonnull((1,3)) __size_t __kshm_copytouser_impl(struct kshm const *self, __user void *dst, __kernel void const *src, __size_t bytes);
extern __crit __nomp __wunused __nonnull((1,2)) __size_t __kshm_copyfromuser_impl(struct kshm const *self, __kernel void *dst, __user void const *src, __size_t bytes);
__local __crit __nomp __wunused __nonnull((1)) __size_t
__kshm_q_copyinuser_impl(struct kshm const *self, __user void *dst,
                         __user void const *src, __size_t bytes) {
 __kernel void *kdst,*ksrc;
 if __unlikely((kdst = kshm_qtranslateuser(self,self->s_pd,dst,1)) == NULL ||
               (ksrc = kshm_qtranslateuser(self,self->s_pd,src,0)) == NULL) return bytes;
 memcpy(kdst,ksrc,bytes);
 return 0;
}
__local __crit __nomp __wunused __nonnull((1,3)) __size_t
__kshm_q_copytouser_impl(struct kshm const *self, __user void *dst,
                         __kernel void const *src, __size_t bytes) {
 __kernel void *kdst;
 if __unlikely((kdst = kshm_qtranslateuser(self,self->s_pd,dst,1)) == NULL) return bytes;
 memcpy(kdst,src,bytes);
 return 0;
}
__local __crit __nomp __wunused __nonnull((1,2)) __size_t
__kshm_q_copyfromuser_impl(struct kshm const *self, __kernel void *dst,
                         __user void const *src, __size_t bytes) {
 __kernel void *ksrc;
 if __unlikely((ksrc = kshm_qtranslateuser(self,self->s_pd,src,0)) == NULL) return bytes;
 memcpy(dst,ksrc,bytes);
 return 0;
}

__forcelocal __size_t
kshm_copyinuser(struct kshm *__restrict self,
                __user void *dst, __user void const *src,
                __size_t bytes) {
 if (__builtin_constant_p(bytes) && !KPAGEDIR_MAYBE_QUICKACCESS(bytes)) {
  return bytes ? __kshm_copyinuser_impl(self,dst,src,bytes) : 0;
 } else if (KPAGEDIR_CAN_QUICKACCESS(dst,bytes) &&
            KPAGEDIR_CAN_QUICKACCESS(src,bytes)) {
  return __kshm_q_copyinuser_impl(self,dst,src,bytes);
 } else {
  return __kshm_copyinuser_impl(self,dst,src,bytes);
 }
}
__forcelocal __size_t
kshm_copytouser(struct kshm *__restrict self,
                __user void *dst, __kernel void const *src,
                __size_t bytes) {
 if (__builtin_constant_p(bytes) && !KPAGEDIR_MAYBE_QUICKACCESS(bytes)) {
  return bytes ? __kshm_copytouser_impl(self,dst,src,bytes) : 0;
 } else if (KPAGEDIR_CAN_QUICKACCESS(dst,bytes)) {
  return __kshm_q_copytouser_impl(self,dst,src,bytes);
 } else {
  return __kshm_copytouser_impl(self,dst,src,bytes);
 }
}
__forcelocal __size_t
kshm_copyfromuser(struct kshm *__restrict self,
                  __kernel void *dst, __user void const *src,
                  __size_t bytes) {
 if (__builtin_constant_p(bytes) && !KPAGEDIR_MAYBE_QUICKACCESS(bytes)) {
  return bytes ? __kshm_copyfromuser_impl(self,dst,src,bytes) : 0;
 } else if (KPAGEDIR_CAN_QUICKACCESS(src,bytes)) {
  return __kshm_q_copyfromuser_impl(self,dst,src,bytes);
 } else {
  return __kshm_copyfromuser_impl(self,dst,src,bytes);
 }
}
#endif

//////////////////////////////////////////////////////////////////////////
// Similar to the !*_w-versions, but allows for
// write-access to otherwise read-only regions of memory.
// Copy-on-write style semantics are still active for
// such areas of memory, meaning that writing to such
// a read-only region will follow the same chain of
// commands, as well as share the same set of semantics
// implied when if the region was writable.
#ifdef __INTELLISENSE__
extern __crit __nomp __wunused __nonnull((1))   __size_t kshm_copyinuser_w(struct kshm const *self, __user void *dst, __user void const *src, __size_t bytes);
extern __crit __nomp __wunused __nonnull((1,3)) __size_t kshm_copytouser_w(struct kshm const *self, __user void *dst, __kernel void const *src, __size_t bytes);
#else
extern __crit __nomp __wunused __nonnull((1))   __size_t __kshm_copyinuser_w_impl(struct kshm const *self, __user void *dst, __user void const *src, __size_t bytes);
extern __crit __nomp __wunused __nonnull((1,3)) __size_t __kshm_copytouser_w_impl(struct kshm const *self, __user void *dst, __kernel void const *src, __size_t bytes);
__local __size_t
__kshm_q_copyinuser_w_impl(struct kshm *__restrict self,
                           __user void *dst, __user void const *src,
                           __size_t bytes) {
 __kernel void *kdst,*ksrc;
 if __unlikely((kdst = kshm_qwtranslateuser(self,self->s_pd,dst)) == NULL ||
               (ksrc = kshm_qtranslateuser(self,self->s_pd,src,0)) == NULL) return bytes;
 memcpy(kdst,ksrc,bytes);
 return 0;
}
__local __size_t
__kshm_q_copytouser_w_impl(struct kshm *__restrict self,
                           __user void *dst, __kernel void const *src,
                           __size_t bytes) {
 __kernel void *kdst = kshm_qwtranslateuser(self,self->s_pd,dst);
 if __unlikely(!kdst) return bytes;
 memcpy(kdst,src,bytes);
 return 0;
}
__forcelocal __size_t
kshm_copyinuser_w(struct kshm *__restrict self,
                  __user void *dst, __user void const *src,
                  __size_t bytes) {
 if (__builtin_constant_p(bytes) && !KPAGEDIR_MAYBE_QUICKACCESS(bytes)) {
  return bytes ? __kshm_copyinuser_w_impl(self,dst,src,bytes) : 0;
 } else if (KPAGEDIR_CAN_QUICKACCESS(dst,bytes) &&
            KPAGEDIR_CAN_QUICKACCESS(src,bytes)) {
  return __kshm_q_copyinuser_w_impl(self,dst,src,bytes);
 } else {
  return __kshm_copyinuser_w_impl(self,dst,src,bytes);
 }
}
__forcelocal __size_t
kshm_copytouser_w(struct kshm *__restrict self,
                  __user void *dst, __kernel void const *src,
                  __size_t bytes) {
 if (__builtin_constant_p(bytes) && !KPAGEDIR_MAYBE_QUICKACCESS(bytes)) {
  return bytes ? __kshm_copytouser_w_impl(self,dst,src,bytes) : 0;
 } else if (KPAGEDIR_CAN_QUICKACCESS(dst,bytes)) {
  return __kshm_q_copytouser_w_impl(self,dst,src,bytes);
 } else {
  return __kshm_copytouser_w_impl(self,dst,src,bytes);
 }
}
#endif



//////////////////////////////////////////////////////////////////////////
// Page-fault IRQ handler to implement copy-on-write semantics.
extern void kshm_pf_handler(struct kirq_registers *__restrict regs);

#ifdef __MAIN_C__
extern void kernel_initialize_copyonwrite(void);
#endif
#endif /* !__ASSEMBLY__ */



#ifndef __INTELLISENSE__
#ifndef __ASSEMBLY__
__local __crit __nomp kerrno_t
kshm_mapfullregion_inherited_unlocked(struct kshm *__restrict self,
                                      __pagealigned __user void *address,
                                      struct kshmregion *__restrict region) {
 kassert_kshmregion(region);
 return kshm_mapregion_inherited_unlocked(self,address,region,
                                          0,region->sre_chunk.sc_pages);
}
__local __crit __nomp kerrno_t
kshm_mapfullregion_unlocked(struct kshm *__restrict self,
                            __pagealigned __user void *address,
                            struct kshmregion *__restrict region) {
 kassert_kshmregion(region);
 return kshm_mapregion_unlocked(self,address,region,
                                0,region->sre_chunk.sc_pages);
}
__local __crit kerrno_t
kshm_mapfullregion_inherited(struct kshm *__restrict self,
                             __pagealigned __user void *address,
                             struct kshmregion *__restrict region) {
 kassert_kshmregion(region);
 return kshm_mapregion_inherited(self,address,region,
                                 0,region->sre_chunk.sc_pages);
}
__local __crit kerrno_t
kshm_mapfullregion(struct kshm *__restrict self,
                   __pagealigned __user void *address,
                   struct kshmregion *__restrict region) {
 kassert_kshmregion(region);
 return kshm_mapregion(self,address,region,
                       0,region->sre_chunk.sc_pages);
}
__local __crit __nomp kerrno_t
kshm_mapfullautomatic_unlocked(struct kshm *__restrict self,
                        /*out*/__pagealigned __user void **__restrict user_address,
                               struct kshmregion *__restrict region,
                               __pagealigned __user void *hint) {
 kassert_kshmregion(region);
 return kshm_mapautomatic_unlocked(self,user_address,region,hint,
                                   0,region->sre_chunk.sc_pages);
}
__local __crit kerrno_t
kshm_mapfullautomatic(struct kshm *__restrict self,
               /*out*/__pagealigned __user void **__restrict user_address,
                      struct kshmregion *__restrict region,
                      __pagealigned __user void *hint) {
 kassert_kshmregion(region);
 return kshm_mapautomatic(self,user_address,region,hint,
                          0,region->sre_chunk.sc_pages);
}
__local __wunused struct kshmbranch *
kshmbranch_locate(struct kshmbranch *__restrict root, __uintptr_t addr) {
 /* addr_semi is the center point splitting the max
  * ranges of the underlying sb_min/sb_max branches. */
 __uintptr_t addr_semi = KSHMBRANCH_ADDRSEMI_INIT;
 unsigned int addr_level = KSHMBRANCH_ADDRLEVEL_INIT;
 while (root) {
  kassertobj(root);
#ifdef __DEBUG__
  { /* Assert that the current branch has a valid min/max address range. */
   __uintptr_t addr_min = KSHMBRANCH_MAPMIN(addr_semi,addr_level);
   __uintptr_t addr_max = KSHMBRANCH_MAPMAX(addr_semi,addr_level);
   assertf(root->sb_map_min <= root->sb_map_max,"Branch has invalid min/max configuration (min(%p) > max(%p))",root->sb_map_min,root->sb_map_max);
   assertf(root->sb_map_min >= addr_min,
           "Unexpected branch min address (%p < %p; max: %p; looking for %p; semi %p; level %u)",
           root->sb_map_min,addr_min,root->sb_map_max,addr,addr_semi,addr_level);
   assertf(root->sb_map_max <= addr_max,
           "Unexpected branch max address (%p > %p; min: %p; looking for %p; semi %p; level %u)",
           root->sb_map_max,addr_max,root->sb_map_min,addr,addr_semi,addr_level);
  }
#endif
  /* Check if the given address lies within this branch. */
  if (addr >= root->sb_map_min &&
      addr <= root->sb_map_max) break;
  assert(addr_level);
  if (addr < addr_semi) {
   /* Continue with min-branch */
   KSHMBRANCH_WALKMIN(addr_semi,addr_level);
   root = root->sb_min;
  } else {
   /* Continue with max-branch */
   KSHMBRANCH_WALKMAX(addr_semi,addr_level);
   root = root->sb_max;
  }
 }
 return root;
}
__local __wunused struct kshmbranch **
kshmbranch_plocate(struct kshmbranch **__restrict proot,
                   __uintptr_t addr, __uintptr_t *result_semi) {
 struct kshmbranch *root;
 /* addr_semi is the center point splitting the max
  * ranges of the underlying sb_min/sb_max branches. */
 __uintptr_t addr_semi = KSHMBRANCH_ADDRSEMI_INIT;
 unsigned int addr_level = KSHMBRANCH_ADDRLEVEL_INIT;
 while ((kassertobj(proot),(root = *proot) != NULL)) {
  kassertobj(root);
  kassert_kshmregion(root->sb_region);
#ifdef __DEBUG__
  { /* Assert that the current branch has a valid min/max address range. */
   __uintptr_t addr_min = KSHMBRANCH_MAPMIN(addr_semi,addr_level);
   __uintptr_t addr_max = KSHMBRANCH_MAPMAX(addr_semi,addr_level);
   assertf(root->sb_map_min <= root->sb_map_max,"Branch has invalid min/max configuration (min(%p) > max(%p))",root->sb_map_min,root->sb_map_max);
   assertf(root->sb_map_min >= addr_min,
           "Unexpected branch min address (%p < %p; max: %p; looking for %p; semi %p; level %u)",
           root->sb_map_min,addr_min,root->sb_map_max,addr,addr_semi,addr_level);
   assertf(root->sb_map_max <= addr_max,
           "Unexpected branch max address (%p > %p; min: %p; looking for %p; semi %p; level %u)",
           root->sb_map_max,addr_max,root->sb_map_min,addr,addr_semi,addr_level);
  }
#endif
  /* Check if the given address lies within this branch. */
  if (addr >= root->sb_map_min &&
      addr <= root->sb_map_max) {
   if (result_semi) *result_semi = addr_semi;
   return proot;
  }
  assert(addr_level);
  if (addr < addr_semi) {
   /* Continue with min-branch */
   KSHMBRANCH_WALKMIN(addr_semi,addr_level);
   proot = &root->sb_min;
  } else {
   /* Continue with max-branch */
   KSHMBRANCH_WALKMAX(addr_semi,addr_level);
   proot = &root->sb_max;
  }
 }
 /*printf("Nothing found for %p at %p %u\n",addr,addr_semi,addr_level);*/
 return NULL;
}
__local __wunused struct kshmbranch **
kshmbranch_plocateat(struct kshmbranch **__restrict proot, __uintptr_t addr,
                     __uintptr_t *_addr_semi, unsigned int *_addr_level) {
 struct kshmbranch *root;
 /* addr_semi is the center point splitting the max
  * ranges of the underlying sb_min/sb_max branches. */
 __uintptr_t addr_semi = *_addr_semi;
 unsigned int addr_level = *_addr_level;
 while ((kassertobj(proot),(root = *proot) != NULL)) {
  kassertobj(root);
  kassert_kshmregion(root->sb_region);
#ifdef __DEBUG__
  { /* Assert that the current branch has a valid min/max address range. */
   __uintptr_t addr_min = KSHMBRANCH_MAPMIN(addr_semi,addr_level);
   __uintptr_t addr_max = KSHMBRANCH_MAPMAX(addr_semi,addr_level);
   assertf(root->sb_map_min <= root->sb_map_max,"Branch has invalid min/max configuration (min(%p) > max(%p))",root->sb_map_min,root->sb_map_max);
   assertf(root->sb_map_min >= addr_min,
           "Unexpected branch min address (%p < %p; max: %p; looking for %p; semi %p; level %u)",
           root->sb_map_min,addr_min,root->sb_map_max,addr,addr_semi,addr_level);
   assertf(root->sb_map_max <= addr_max,
           "Unexpected branch max address (%p > %p; min: %p; looking for %p; semi %p; level %u)",
           root->sb_map_max,addr_max,root->sb_map_min,addr,addr_semi,addr_level);
  }
#endif
  /* Check if the given address lies within this branch. */
  if (addr >= root->sb_map_min &&
      addr <= root->sb_map_max) {
   *_addr_semi = addr_semi;
   *_addr_level = addr_level;
   return proot;
  }
  assert(addr_level);
  if (addr < addr_semi) {
   /* Continue with min-branch */
   KSHMBRANCH_WALKMIN(addr_semi,addr_level);
   proot = &root->sb_min;
  } else {
   /* Continue with max-branch */
   KSHMBRANCH_WALKMAX(addr_semi,addr_level);
   proot = &root->sb_max;
  }
 }
 /*printf("Nothing found for %p at %p %u\n",addr,addr_semi,addr_level);*/
 return NULL;
}
#endif /* !__ASSEMBLY__ */
#endif

__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_SHM_H__ */

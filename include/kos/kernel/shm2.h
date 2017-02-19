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
#ifndef __KOS_KERNEL_SHM2_H__
#define __KOS_KERNEL_SHM2_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <sys/mman.h>
#include <kos/kernel/features.h>
#include <kos/compiler.h>
#include <kos/types.h>
#include <kos/kernel/gdt.h>
#include <kos/errno.h>
#include <kos/kernel/mutex.h>
#include <kos/kernel/features.h>
#include <kos/kernel/object.h>
#include <kos/kernel/paging.h>
#include <strings.h>
#include <math.h>
#include <stdio.h>
#if KCONFIG_USE_SHM2

// Shared memory layout
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
// >> shm-cluster: (Global/Shared)
//   - A reference counted descriptor for
//     'KSHM_CLUSTERSIZE' pages of shm-parts.
//
// >> shm-region: (Global/Shared)
//   - Owns a vector of shm-parts.
//   - Owns a vector of shm-clusters to ref-track its shm-parts.
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

__struct_fwd(kpageframe);

#define KOBJECT_MAGIC_SHMCHUNK  0x573C7C25 /*< SHMCHUNK. */
#define KOBJECT_MAGIC_SHMREGION 0x5736E610 /*< SHMREGIO. */
#define KOBJECT_MAGIC_SHM      0x5732     /*< SHM2. */
#define kassert_kshmchunk(self)  kassert_object(self,KOBJECT_MAGIC_SHMCHUNK)
#define kassert_kshmregion(self) kassert_refobject(self,sre_clustera,KOBJECT_MAGIC_SHMREGION)
#define kassert_kshm(self)      kassert_object(self,KOBJECT_MAGIC_SHM)

/* In pages: amount of memory managed by a single reference counter.
 *        >> Higher values --> more redundency during copy-on-write.
 *        >> Lower values --> copy-on-write must be performed more often. */
#define KSHM_CLUSTERSIZE  16


#define KSHMREFCNT_SIZEOF  4
#ifndef __ASSEMBLY__
typedef __u32 kshmrefcnt_t;
#endif /* !__ASSEMBLY__ */

#define KSHMREGION_FLAG_EXEC       0x0001 /*< Currently unused; later this will work alongside the LDT to control execute permissions. */
#define KSHMREGION_FLAG_WRITE      0x0002 /*< Memory is mapped as writable from ring-#3 (Unless used with 'KSHMRANGE_FLAG_SHARED', this also enables copy-on-write semantics). */
#define KSHMREGION_FLAG_READ       0x0004 /*< Memory is mapped as readable from ring-#3. */
#define KSHMREGION_FLAG_SHARED     0x0008 /*< Memory is mapped as writable from ring-#3 (Unless used). */
#define KSHMREGION_FLAG_LOSEONFORK 0x0010 /*< This memory region is not copied during a fork() operation. - Used to not copy stuff like a kernel-thread-stack or user-space thread-local storage. */
#define KSHMREGION_FLAG_RESTRICTED 0x0020 /*< This region of memory is restricted, meaning that the user cannot use 'munmap()' to delete it. (NOTE: Superceided by 'KSHMREGION_FLAG_LOSEONFORK'). */
#define KSHMREGION_FLAG_NOFREE     0x0040 /*< Don't free memory from this chunk when the reference counter hits ZERO(0). - Used for 1-1 physical/device memory mappings. */
#define KSHMREGION_FLAG_NOCOPY     0x0080 /*< Don't hard-copy memory from this chunk, but alias it when set in 'right->mt_flags' in 'kshmchunk_hardcopy'. */

#define KSHM_FLAG_SIZEOF  2
#ifndef __ASSEMBLY__
typedef __u16 kshm_flag_t; /*< Set of 'KSHMREGION_FLAG_*' */
#endif /* !__ASSEMBLY__ */

/* Returns TRUE(1) if the given set of flags either requires the
 * use of COW semantics, or to perform hard copies during fork().
 * >> Basically just: Is is writeable and not shared memory.
 * NOTE: As a special addition: nocopy-pages implicitly behave as shared. */
#define KSHMREGION_FLAG_USECOW(flags) \
 (((flags)&(KSHMREGION_FLAG_WRITE)) && \
 !((flags)&(KSHMREGION_FLAG_SHARED|KSHMREGION_FLAG_NOCOPY)))



#define KSHMCHUNK_PAGE_SIZEOF     __SIZEOF_POINTER__
#define KSHMREGION_PAGE_SIZEOF    __SIZEOF_POINTER__
#define KSHMREGION_ADDR_SIZEOF    __SIZEOF_POINTER__
#define KSHMREGION_CLUSTER_SIZEOF __SIZEOF_POINTER__
#ifndef __ASSEMBLY__
typedef __uintptr_t kshmchunk_page_t;
typedef __uintptr_t kshmregion_page_t;
typedef __uintptr_t kshmregion_addr_t;
typedef __uintptr_t kshmregion_cluster_t;
#endif /* !__ASSEMBLY__ */


#define KSHMPART_SIZEOF          (__SIZEOF_POINTER__+KSHMREGION_PAGE_SIZEOF+__SIZEOF_SIZE_T__)
#define KSHMPART_OFFSETOF_FRAME  (0)
#define KSHMPART_OFFSETOF_START  (__SIZEOF_POINTER__)
#define KSHMPART_OFFSETOF_PAGES  (__SIZEOF_POINTER__+KSHMREGION_PAGE_SIZEOF)
#ifndef __ASSEMBLY__
struct kshmpart {
 /* WARNING: Individual pages are only allocated when the reference counter of the associated chunk is non-ZERO. */
 __pagealigned struct kpageframe *sp_frame; /*< [1..sp_pages][const] Physical starting address of this part. */
 kshmregion_page_t                sp_start; /*< [const] Page index of the start of this part (aka. 'sum(sp_pages)' of all preceiding parts). */
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
 __atomic kshmrefcnt_t sc_refcnt; /*< Reference counters for cluster memory. */
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
 /* NOTE: This structure is synchronized because it is fully atomic/singleton. */
 KOBJECT_HEAD
 struct kshmchunk   sre_chunk;       /*< Associated chunks of data. */
 __atomic __size_t  sre_branches;    /*< Amount of branches that this region is associated with (Used to ensure uniqueness of a region). */
 __atomic __size_t  sre_clustera;    /*< [!0] Allocated amount of clusters (When this hits ZERO(0), free the region controller structure). */
 __size_t           sre_clusterc;    /*< [!0][const][== ceildiv(sre_chunk.sc_pages,KSHM_CLUSTERSIZE)] Amount of tracked clusters. */
 struct kshmcluster sre_clusterv[1]; /*< [sre_clusterc] Inlined vector of available clusters. */
};

#define kshmregion_getcluster(self,page_index) \
 ((self)->sre_clusterv+((page_index)/KSHM_CLUSTERSIZE))

//////////////////////////////////////////////////////////////////////////
// Increment/Decrement the reference counter of all of the specified clusters.
// When a cluster reference counter reaches ZERO(0), 
// @return: KE_OVERFLOW: One of the cluster reference counters has overflown,
extern __crit __wunused kerrno_t kshmregion_incref(struct kshmregion *__restrict self, struct kshmcluster *cls_min, struct kshmcluster *cls_max);
extern __crit void               kshmregion_decref(struct kshmregion *__restrict self, struct kshmcluster *cls_min, struct kshmcluster *cls_max);
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
// Automatically handles page alignemnt.
extern __wunused __retnonnull __nonnull((1,3)) __kernel void *
kshmregion_getphysaddr(struct kshmregion *__restrict self,
                       kshmregion_addr_t address_offset,
                       __size_t *__restrict max_bytes);

//////////////////////////////////////////////////////////////////////////
// Similar to 'kshmregion_getphysaddr', but simply returns NULL
// and sets '*max_bytes' to ZERO(0) for out-of-bounds indices.
extern __wunused __nonnull((1,3)) __kernel void *
kshmregion_getphysaddr_s(struct kshmregion *__restrict self,
                         kshmregion_addr_t address_offset,
                         __size_t *__restrict max_bytes);


//////////////////////////////////////////////////////////////////////////
// Create various kinds of SHM memory chunks.
// NOTE: Initially, the caller inherits ONE(1) reference to every cluster.
// kshmregion_newram:
//    Allocate new, possibly parted (aka. scattered) memory,
//    returning a region describing that newly allocated ram.
// kshmregion_newlinear:
//    Similar to 'kshmregion_newram', but ensure that
//    allocated memory is liner in a physical sense.
//    That is: it is one continuous string in the physical address space,
//             with the region only consisting of a single part.
// kshmregion_newphys:
//    Create a new region that can be used for mapping a physical
//    address, allowing you to map special areas of memory, such
//    as the VGA screen buffer outside of ring-#0.
// kshmregion_hardcopy:
//    Creates a hard copy of the given region, containing a new area
//    of memory the same size of the old, unless the given region
//    has the 'KSHMCHUNK_FLAG_NOCOPY' flag set, in which case it
//    will alias the memory of the existing region.
//    NOTE: All flags of the given region are inherited.
//    WARNING: Even if the given region only contains linear memory, the
//             returned hard copy may contain non-linear memory instead
//             when the 'KSHMCHUNK_FLAG_NOCOPY' flag isn't set.
// @return: * :   A new reference to a newly allocated SHM region, as described above.
// @return: NULL: Not enough available kernel, or (more likely) physical memory.
extern __crit __wunused __malloccall __ref struct kshmregion *kshmregion_newram(__size_t pages, kshm_flag_t flags);
extern __crit __wunused __malloccall __ref struct kshmregion *kshmregion_newlinear(__size_t pages, kshm_flag_t flags);
extern __crit __wunused __malloccall __ref struct kshmregion *kshmregion_newphys(__pagealigned __kernel void *addr, __size_t pages, kshm_flag_t flags);

extern __crit __wunused __malloccall __ref struct kshmregion *
kshmregion_hardcopy(struct kshmregion const *self,
                    kshmregion_page_t page_offset,
                    __size_t pages);

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
extern __crit __wunused __malloccall __ref struct kshmregion *
kshmregion_extractpart(__ref struct kshmregion *self,
                       struct kshmcluster *cls_min,
                       struct kshmcluster *cls_max);
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
 struct kshmregion         *sb_region;      /*< [1..1][const] Reference to the associated region of memory. */
 /* NOTE: The following two pointers describe a range of clusters to which this branch owns references. */
 __ref struct kshmcluster  *sb_cluster_min; /*< [1..1][in(sb_region->sre_clusterv)][<= sb_cluster_max] Lowest cluster to which this branch owns references. */
 __ref struct kshmcluster  *sb_cluster_max; /*< [1..1][in(sb_region->sre_clusterv)][>= sb_cluster_min] Highest cluster to which this branch owns references. */
};

/* Returns TRUE(1) if the given branch can be expanded up-, or downwards. */
#define kshmbranch_canexpand_min(self) \
 ((self)->sb_region->sre_branches == 1 && (self)->sb_rstart == 0)
#define kshmbranch_canexpand_max(self) \
 ((self)->sb_region->sre_branches == 1 && \
  (self)->sb_rstart+(self)->sb_rpages == \
  (self)->sb_region->sre_chunk.sc_pages)

#define KSHMBRANCH_ADDRSEMI_INIT       ((((__uintptr_t)-1)/2)+1)
#define KSHMBRANCH_ADDRLEVEL_INIT      ((__SIZEOF_POINTER__*8)-1)
#define KSHMBRANCH_MAPMIN(semi,level)  ((semi)&~(((__uintptr_t)1 << (level))))
#define KSHMBRANCH_MAPMAX(semi,level)  ((semi)| (((__uintptr_t)1 << (level))-1))
#define KSHMBRANCH_WALKMIN(semi,level) ((semi)  = (((semi)&~((__uintptr_t)1 << (level)))|((__uintptr_t)1 << ((level)-1))),--(level)) /*< unset level'th bit; set level'th-1 bit. */
#define KSHMBRANCH_WALKMAX(semi,level) (--(level),(semi) |= ((__uintptr_t)1 << (level)))                                             /*< set level'th-1 bit. */
#define KSHMBRANCH_SEMILEVEL(semi)     (ffs(semi)-1) /*< Get the current level associated with a given semi-address. */


extern __crit __nomp __nonnull((1,2)) kerrno_t kshmbranch_insert(struct kshmbranch **__restrict proot, struct kshmbranch *__restrict newleaf, __uintptr_t addr_semi, unsigned int addr_level);
extern __crit __nomp __nonnull((1)) struct kshmbranch *kshmbranch_remove(struct kshmbranch **__restrict proot, __uintptr_t addr);
__local __wunused struct kshmbranch *kshmbranch_locate(struct kshmbranch *root, __uintptr_t addr);


//////////////////////////////////////////////////////////////////////////
// Returns the old '*proot' and replaces it with either its min, or max branch.
// Special handling is performed to determine the perfect match when
// '*proot' has both a min and a max branch.
extern __crit __nomp __wunused __retnonnull __nonnull((1)) struct kshmbranch *
kshmbranch_popone(struct kshmbranch **__restrict proot);

//////////////////////////////////////////////////////////////////////////
// Returns the root of two min/max branches after combining them.
extern __crit __nomp __wunused __retnonnull __nonnull((1,2)) struct kshmbranch *
kshmbranch_combine(struct kshmbranch *__restrict min_branch,
                   struct kshmbranch *__restrict max_branch);


//////////////////////////////////////////////////////////////////////////
// Locate the pointer to the branch associated with the given address.
// If non-NULL, store the associated semi-address in '*result_semi' upon success.
// @return: NULL: Failed to find a branch containing the given address.
__local __wunused struct kshmbranch **
kshmbranch_plocate(struct kshmbranch **proot, __uintptr_t addr,
                   __uintptr_t *result_semi);

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
// @param: pmin: When non-NULL, upon success, pointer to the branch < the given 'page_offset'.
// @param: pmax: When non-NULL, upon success, pointer to the branch >= the given 'page_offset'.
// @return: KE_OK:    Successfully created the new split.
// @return: KE_NOMEM: Failed to allocate the new branch.
// WARNING: The given 'page_offset' is an offset within the SHM region of '*pself'.
//          It is _NOT_ an offset within the area of that region mapped by '*pself'!
extern __crit __nomp __wunused __nonnull((1)) kerrno_t
kshmbrach_putsplit(struct kshmbranch **__restrict pself,
                   struct kshmbranch ***pmin, __uintptr_t *pmin_semi,
                   struct kshmbranch ***pmax, __uintptr_t *pmax_semi,
                   kshmregion_page_t page_offset,
                   __uintptr_t addr_semi);

//////////////////////////////////////////////////////////////////////////
// Merge two given branches 'pmin' and 'pmax' that are located directly next to each other.
// Upon success and if non-NULL, store the branch-pointer and address-semi
// of the merged branch in the given 'presult' and 'presult_semi' locations.
// @return: KE_NOMEM: Not enough available memory to merge the branches.
extern __crit __nomp __wunused __nonnull((1,3)) kerrno_t
kshmbrach_merge(struct kshmbranch **__restrict pmin, __uintptr_t min_semi,
                struct kshmbranch **__restrict pmax, __uintptr_t max_semi,
                struct kshmbranch ***__restrict presult, __uintptr_t *presult_semi);

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
extern __crit __nomp __wunused kerrno_t
kshmbranch_remap(struct kshmbranch const *self,
                 struct kpagedir *pd);


typedef __u32 kshmunmap_flag_t; /*< A set of 'KSHMUNMAP_FLAG_*' */
#define KSHMUNMAP_FLAG_NONE       0x00000000
#define KSHMUNMAP_FLAG_RESTRICTED KSHMREGION_FLAG_RESTRICTED /*< Allow unmapping of restricted regions. */


//////////////////////////////////////////////////////////////////////////
// Unmaps all mapped pages within the specified user-address-range.
// NOTE: The SHM tree is searched recursively for potential branches.
// @return: * : The total sum of all pages successfully unmapped.
extern __crit __nomp __wunused __size_t
kshmbranch_unmap(struct kshmbranch **__restrict proot,
                 struct kpagedir *__restrict pd,
                 __pagealigned __uintptr_t addr_min,
                               __uintptr_t addr_max,
                 __uintptr_t addr_semi, unsigned int addr_level,
                 kshmunmap_flag_t flags);


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
extern __crit __nomp __wunused kerrno_t
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
kshmbranch_fillfork(struct kshmbranch **__restrict pself, struct kpagedir *self_pd,
                    struct kshmbranch *__restrict source, struct kpagedir *source_pd);
#endif /* !__ASSEMBLY__ */






#define KSHMMAP_SIZEOF         (__SIZEOF_POINTER__)
#define KSHMMAP_OFFSETOF_ROOT  (0)
#ifndef __ASSEMBLY__
struct kshmmap {
 struct kshmbranch *m_root; /*< [0..1][owned] Root branch. */
};
#define KSHMMAP_INIT       {NULL}
#define kshmmap_init(self) (void)((self)->m_root = NULL)
extern __crit void kshmmap_quit(struct kshmmap *self);


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
__local struct kshmbranch *
kshmmap_locate(struct kshmmap const *self, __uintptr_t addr);
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
__COMPILER_PACK_PUSH(1)
struct __packed kldt {
 /* Local descriptor table (One for each process). */
 __u16                                   ldt_gdtid;  /*< [const] Associated GDT offset/index. */
 __u16                                   ldt_limit;  /*< LDT vector limit. */
 __user struct ksegment                 *ldt_base;   /*< [0..1] User-mapped base address of the LDT vector. */
 __pagealigned __kernel struct ksegment *ldt_vector; /*< [0..1][owned] Physical address of the LDT vector. */
};
__COMPILER_PACK_POP
#define KLDT_INIT(gdtid)  {gdtid,0,NULL,NULL}
#endif /* !__ASSEMBLY__ */





#define KSHM_OFFSETOF_PD   (KOBJECT_SIZEOFHEAD)
#define KSHM_OFFSETOF_MAP  (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__)
#define KSHM_OFFSETOF_LDT  (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__+KSHMMAP_SIZEOF)
#define KSHM_SIZEOF        (KOBJECT_SIZEOFHEAD+__SIZEOF_POINTER__+KSHMMAP_SIZEOF+KLDT_SIZEOF)
#ifndef __ASSEMBLY__
struct kshm {
 /* Main, per-process controller structure for everything memory-related.
  * NOTE: Access to this structure is not thread-safe and therefor
  *       protected by the 'KPROC_LOCK_SHM' lock in kproc. */
 KOBJECT_HEAD
 struct kpagedir *s_pd;  /*< [1..1][owned] Per-process page directory (NOTE: May contain more mappings than those of 's_map', though not less!). */
 struct kshmmap   s_map; /*< Map used for mapping a given user-space pointer to a mapped region of memory. */
 struct kldt      s_ldt; /*< The per-process local descriptor table. */
};
#define KSHM_INIT(pagedir,gdtid) \
 {KOBJECT_INIT(KOBJECT_MAGIC_SHM) pagedir,KSHMMAP_INIT,KLDT_INIT(gdtid)}


//////////////////////////////////////////////////////////////////////////
// Initialize/Finalize a given SHM per-process controller structure.
// @return: KE_OK:    Successfully initialized the given structure.
// @return: KE_NOMEM: Not enough available memory (Such as when allocating the page directory).
extern __crit __nomp __wunused kerrno_t kshm_init(struct kshm *self, __u16 ldt_size_hint);
extern __crit __nomp void kshm_quit(struct kshm *self);

//////////////////////////////////////////////////////////////////////////
// Performs all the operations necessary to initialized 'self' as
// the counterpart of 'right', as seen during a fork() operation.
// This means that a new page directory is created, important
// areas of kernel memory are re-mapped in that directory, all
// mapped regions of memory are re-mapped as read-only in both
// the new, as well as page directory of 'right', as well as the
// entire mapping tree being copied, incrementing the user-counters
// of all regions before sharing all of them by reference.
// @return: KE_OK:    Successfully initialized 'self' as a fork()-copy of 'right'.
// @return: KE_NOMEM: Not enough available memory.
extern __crit __nomp __wunused kerrno_t kshm_initfork(struct kshm *self, struct kshm *right);



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
//       or to keep a lock on all other process's KPROC_LOCK_SHM locks,
//       as well as remap the region as read-only in all of them upon
//       success of this function.
// HINT: If the caller doesn't desire to map the given region at
//       the fixed address 'address', 'kpagedir_findfreerange' should
//       be called to locate a free region of sufficient size, and
//       the found region should be passed instead.
// HINT: To simply map dynamic memory, allocate a region
//       and then pass that region to this function.
// @return: KE_OK:     Successfully mapped the given region of memory.
// @return: KE_EXISTS: Some part of the given area of memory was
//                     already mapped by an SHM-style mapping.
// @return: KE_NOMEM:  Not enough kernel memory to allocate the
//                     necessary control entries and structures.
extern __crit __nomp __wunused kerrno_t
kshm_mapregion_inherited(struct kshm *__restrict self,
                         __pagealigned __user void *address,
                         __ref struct kshmregion *__restrict region,
                         kshmregion_page_t in_region_page_start,
                         __size_t in_region_page_count);
extern __crit __nomp __wunused kerrno_t
kshm_mapregion(struct kshm *__restrict self,
               __pagealigned __user void *address,
               struct kshmregion *__restrict region,
               kshmregion_page_t in_region_page_start,
               __size_t in_region_page_count);
__local __crit __nomp __wunused kerrno_t
kshm_mapfullregion(struct kshm *__restrict self,
                   __pagealigned __user void *address,
                   struct kshmregion *__restrict region);


//////////////////////////////////////////////////////////////////////////
// Fast-pass for automatically mapping a given region.
// Behaves the same as call 'kpagedir_findfreerange' to
// find a suitable user-space address before passing
// that value in a call to 'kshm_mapregion'.
// @param: hint: Used as a hint when searching for usable space in the page directory.
// @return: KE_OK:    Successfully mapped the given region of memory.
// @return: KE_NOMEM: Not enough kernel memory to allocate the
//                    necessary control entries and structures.
// @return: KE_NOSPC: Failed to find an unused region of sufficient size.
extern __crit __nomp __wunused __nonnull((1,2,3)) kerrno_t
kshm_mapautomatic(struct kshm *__restrict self,
                  /*out*/__pagealigned __user void **__restrict user_address,
                  struct kshmregion *__restrict region, __pagealigned __user void *hint,
                  kshmregion_page_t in_region_page_start, __size_t in_region_page_count);

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
kshm_mapram(struct kshm *__restrict self,
            /*out*/__pagealigned __user void **__restrict user_address,
            __size_t pages, __pagealigned __user void *hint, kshm_flag_t flags);
extern __crit __nomp __wunused __nonnull((1,2,3)) kerrno_t
kshm_mapram_linear(struct kshm *__restrict self,
                   /*out*/__pagealigned __kernel void **__restrict kernel_address,
                   /*out*/__pagealigned __user   void **__restrict user_address,
                   __size_t pages, __pagealigned __user void *hint, kshm_flag_t flags);
extern __crit __nomp __wunused __nonnull((1,3)) kerrno_t
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
kshm_touch(struct kshm *__restrict self,
           __pagealigned __user void *address,
           __size_t touch_pages);




//////////////////////////////////////////////////////////////////////////
// Unmap all SHM memory mappings matching the description
// provided by 'flags' within the given region of page-based memory.
// NOTE: This function will perform all the necessary adjustments to
//       use counters, as well as updating the page directory before
//       returning. - There is no ~real~ fail-case for this function.
// WARNING: If the given address range only scrapes an SHM-region,
//          that region will be sub-ranged, with only apart of it
//          remaining mapped. (Meaning that partial munmap() is allowed)
// @return: * : The amount of actually unmapped pages (as removed from the page directory).
extern __crit __nomp __nonnull((1)) __size_t
kshm_unmap(struct kshm *__restrict self,
           __pagealigned __user void *address,
           __size_t pages, kshmunmap_flag_t flags);

//////////////////////////////////////////////////////////////////////////
// Unmaps a specific region mapped to a given 'base_address'.
// NOTE: Although much less powerful than 'kshm_unmap', this function
//       does allow you to safely unmap a given region from a given
//       address, allowing you to ensure that you aren't accidentally
//       unmapping something other than what you're expecting to find
//       at a given address, given the fact that user-code can freely
//       unmap anything that wasn't mapped with 'KSHMREGION_FLAG_RESTRICTED'.
// @return: KE_FAULT: Nothing was mapped to the given 'base_address'.
// @return: KE_RANGE: The given 'base_address' doesn't point to the base of a mapped region.
// @return: KE_PERM:  The given 'region' didn't match the one mapped at 'base_address'.
// @return: KE_OK:    Successfully unmapped the entirety of the given region.
extern __crit __nomp __nonnull((1,3)) kerrno_t
kshm_unmapregion(struct kshm *__restrict self,
                 __pagealigned __user void *base_address,
                 struct kshmregion *__restrict region);


//////////////////////////////////////////////////////////////////////////
// Checks if the given process 'caller' has access to
// the specified area of physical (aka. device) memory.
// @return: KE_OK:    Access is granted.
// @return: KE_ACCES: They don't...
extern __crit kerrno_t
kshm_devaccess(struct ktask *__restrict caller,
               __pagealigned __kernel void const *base_address,
               __size_t pages);



// === Local descriptor table ===

//////////////////////////////////////////////////////////////////////////
// Allocate/Free segments within a local descriptor table.
// Upon successful allocation, the new segment will have already
// been flushed and capable of being used for whatever means necessary.
// NOTE: On success, the returned index always compares true for 'KSEG_ISLDT()'
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
extern __nomp __crit __wunused __nonnull((1,2)) ksegid_t kshm_ldtalloc(struct kshm *__restrict self, struct ksegment const *__restrict seg);
extern __nomp __crit __wunused __nonnull((1,3)) ksegid_t kshm_ldtallocat(struct kshm *__restrict self, ksegid_t reqid, struct ksegment const *__restrict seg);
extern __nomp __crit           __nonnull((1)) void kshm_ldtfree(struct kshm *__restrict self, ksegid_t id);

//////////////////////////////////////////////////////////////////////////
// Get/Set the segment data associated with a given segment ID.
// NOTE: 'kldt_setseg' will flush the changed segment upon success.
extern __nomp __nonnull((1,3)) void kshm_ldtget(struct kshm const *__restrict self, ksegid_t id, struct ksegment *__restrict seg);
extern __nomp __nonnull((1,3)) void kshm_ldtset(struct kshm *__restrict self, ksegid_t id, struct ksegment const *__restrict seg);


//////////////////////////////////////////////////////////////////////////
// Copy memory to/from/in userspace.
// @param: dst:   The (potentially virtual) destination address.
// @param: src:   The (potentially virtual) source address.
// @param: bytes: The (max) amount of bytes to transfer.
// @return: 0 :   Success transferred all specified memory.
// @return: * :   Amount of bytes _NOT_ transferred.
extern __nomp __crit __wunused __nonnull((1)) __size_t kshm_copyinuser(struct kshm const *self, __user void *dst, __user void const *src, __size_t bytes);
extern __nomp __crit __wunused __nonnull((1,3)) __size_t kshm_copytouser(struct kshm const *self, __user void *dst, __kernel void const *src, __size_t bytes);
extern __nomp __crit __wunused __nonnull((1,2)) __size_t kshm_copyfromuser(struct kshm const *self, __kernel void *dst, __user void const *src, __size_t bytes);

#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Translate a given user-space pointer to its physical counterpart.
// @param: addr:      A virtual user-space pointer.
// @param: maxbytes:  The max amount of bytes of confirm (*rwbytes
//                    will not be set to something higher than this).
// @param: rwbytes:   Mandatory output parameter that stores the amount
//                    of linear bytes available for operations.
//                    NOTE: In all situations, it can be assumed that
//                          upon return this value is '<= maxbytes'.
// @param: writeable: When non-ZERO, perform translation with the intent
//                    of writing to the returned pointer upon success.
//                    >> If the user-space address is part of an uncopied copy-on-write page,
//                       that page is copied the same way a page-fault would when ring-#3
//                       user-code had tried to write to the same address.
//                    >> If the region of memory associated with the address is not writable
//                       to the user, NULL is returned and '*rwbytes' is set to ZERO(0).
// @return: * :       The physical counterpart to the given virtual address.
// @return: NULL :    '*rwbytes' was set to ZERO(0) because the given pointer isn't mapped.
extern __nomp __crit __wunused __nonnull((1,4)) __kernel void *
kshm_translateuser(struct kshm const *self, __user void const *addr,
                   __size_t maxbytes, __size_t *rwbytes, int writeable);
#else
#define kshm_translateuser(self,addr,maxbytes,rwbytes,writeable) \
 (*(rwbytes) = (maxbytes),__kshm_translateuser_impl(self,addr,rwbytes,writeable))
#endif
extern __nomp __crit __wunused __nonnull((1,3)) __kernel void *
__kshm_translateuser_impl(struct kshm const *self, __user void const *addr,
                          __size_t *rwbytes, int writeable);


//////////////////////////////////////////////////////////////////////////
// Page-fault IRQ handler to implement copy-on-write semantics.
extern void kshm_pf_handler(struct kirq_registers *__restrict regs);

#ifdef __MAIN_C__
extern void kernel_initialize_copyonwrite(void);
#endif
#endif /* !__ASSEMBLY__ */


/* Old function/macro/member names (placeholder before FULL refactoring). */
#define kshm_memcpy_k2u(self,dst,src,bytes) ((bytes)-kshm_copytouser(self,dst,src,bytes))
#define kshm_memcpy_u2k(self,dst,src,bytes) ((bytes)-kshm_copyfromuser(self,dst,src,bytes))
#define kshm_memcpy_u2u(self,dst,src,bytes) ((bytes)-kshm_copyinuser(self,dst,src,bytes))
#define kshm_translate_1(self,addr,rwbytes,writeable) \
 (*(rwbytes) = PAGESIZE,__kshm_translateuser_impl(self,addr,rwbytes,writeable))
#define kshm_translate_u __kshm_translateuser_impl

#define sm_pd            s_pd
#define sm_ldt           s_ldt
#define kshmtab                  kshmregion
#define kshmtab_translate_offset kshmregion_getphysaddr_s
#define kshmtab_newram           kshmregion_newram
#define kshmtab_incref           kshmregion_incref_full
#define kshmtab_decref           kshmregion_decref_full
#define mt_flags                 sre_chunk.sc_flags
#define mt_pages                 sre_chunk.sc_pages
#define KSHMTAB_FLAG_X           KSHMREGION_FLAG_EXEC
#define KSHMTAB_FLAG_W           KSHMREGION_FLAG_WRITE
#define KSHMTAB_FLAG_R           KSHMREGION_FLAG_READ
#define KSHMTAB_FLAG_S           KSHMREGION_FLAG_SHARED
#define KSHMTAB_FLAG_K          (KSHMREGION_FLAG_LOSEONFORK|KSHMREGION_FLAG_RESTRICTED)
#define KSHMTAB_FLAG_D          (KSHMREGION_FLAG_NOFREE|KSHMREGION_FLAG_NOCOPY)
#define kshm_initcopy(self,right,copy) kshm_initfork(self,right)
#define KSHM_INITROOT            KSHM_INIT

#ifndef __ASSEMBLY__
__local __size_t kshm_munmap(struct kshm *self, void *start,
                             __size_t bytes, int restricted) {
 __uintptr_t aligned_start = alignd((uintptr_t)start,PAGEALIGN);
 return kshm_unmap(self,(__user void *)aligned_start,
                   ceildiv(bytes+((uintptr_t)start-aligned_start),PAGESIZE),
                   restricted ? KSHMUNMAP_FLAG_RESTRICTED : KSHMUNMAP_FLAG_NONE);
}
#endif


#ifndef __INTELLISENSE__
#ifndef __ASSEMBLY__
__local __crit __nomp kerrno_t
kshm_mapfullregion(struct kshm *__restrict self,
                   __pagealigned __user void *address,
                   struct kshmregion *__restrict region) {
 kassert_kshmregion(region);
 return kshm_mapregion(self,address,region,0,region->sre_chunk.sc_pages);
}
__local __wunused struct kshmbranch *
kshmbranch_locate(struct kshmbranch *root, __uintptr_t addr) {
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
#endif /* !__ASSEMBLY__ */
#endif

__DECL_END
#endif /* KCONFIG_USE_SHM2 */
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_SHM2_H__ */

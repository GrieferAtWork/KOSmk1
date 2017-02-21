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
#ifndef __KOS_KERNEL_SHM_MAP_C_INL__
#define __KOS_KERNEL_SHM_MAP_C_INL__ 1

#include <malloc.h>
#include <kos/config.h>
#include <kos/kernel/features.h>
#include <kos/kernel/interrupts.h>
#include <kos/kernel/shm.h>

__DECL_BEGIN

void
kshmbranch_print(struct kshmbranch *__restrict branch,
                 uintptr_t addr_semi, unsigned int level) {
 unsigned int new_level,i; uintptr_t new_semi;
 if (!branch) return;
again:
 kassertobj(branch);
 kassert_kshmregion(branch->sb_region);
 for (i = 31; i > level; --i) printf("%s",(addr_semi&(1 << i)) ? "+" : "-");
 for (i = 0; i < level; ++i) printf(" ");
 printf("leaf (%c%c%c%c%c%c) %Iu pages %p .. %p at %p .. %p semi %p, level %u\n",
       (branch->sb_region->sre_chunk.sc_flags&KSHMREGION_FLAG_EXEC) ? 'X' : '-',
       (branch->sb_region->sre_chunk.sc_flags&KSHMREGION_FLAG_WRITE) ? 'W' : '-',
       (branch->sb_region->sre_chunk.sc_flags&KSHMREGION_FLAG_READ) ? 'R' : '-',
       (branch->sb_region->sre_chunk.sc_flags&KSHMREGION_FLAG_SHARED) ? 'S' : '-',
       (branch->sb_region->sre_chunk.sc_flags&KSHMREGION_FLAG_LOSEONFORK) ? 'F' : '-',
       (branch->sb_region->sre_chunk.sc_flags&KSHMREGION_FLAG_RESTRICTED) ? 'R' : '-',
        branch->sb_rpages,
        branch->sb_map_min,branch->sb_map_max,
        KSHMBRANCH_MAPMIN(addr_semi,level),
        KSHMBRANCH_MAPMAX(addr_semi,level),
        addr_semi,level);
 assert(branch->sb_rpages != 0);
 assert(branch->sb_rstart+branch->sb_rpages <=
        branch->sb_region->sre_chunk.sc_pages);
 assert(branch->sb_cluster_min == kshmregion_getcluster(branch->sb_region,branch->sb_rstart));
 assert(branch->sb_cluster_max == kshmregion_getcluster(branch->sb_region,branch->sb_rstart+(branch->sb_rpages-1)));
 assert(branch->sb_map_min <= branch->sb_map_max);
 assert(branch->sb_map_min >= KSHMBRANCH_MAPMIN(addr_semi,level));
 assert(branch->sb_map_max <= KSHMBRANCH_MAPMAX(addr_semi,level));
 if (branch->sb_min) {
  if (branch->sb_max) {
   new_semi = addr_semi,new_level = level;
   KSHMBRANCH_WALKMAX(new_semi,new_level);
   kshmbranch_print(branch->sb_max,new_semi,new_level);
  }
  KSHMBRANCH_WALKMIN(addr_semi,level);
  branch = branch->sb_min;
  goto again;
 }
 if (branch->sb_max) {
  KSHMBRANCH_WALKMAX(addr_semi,level);
  branch = branch->sb_max;
  goto again;
 }
}

__crit __nomp kerrno_t
kshmbranch_insert(struct kshmbranch **__restrict pcurr,
                  struct kshmbranch *__restrict newleaf,
                  uintptr_t addr_semi, unsigned int addr_level) {
 struct kshmbranch *iter;
 uintptr_t newleaf_min,newleaf_max;
 kassertobj(pcurr);
 kassertobj(newleaf);
 kassert_kshmregion(newleaf->sb_region);
 newleaf_min = newleaf->sb_map_min;
 newleaf_max = newleaf->sb_map_max;
again:
 /* Make sure that the given entry can truly be inserted somewhere within this branch. */
 assertf(newleaf_min >= KSHMBRANCH_MAPMIN(addr_semi,addr_level),
         "The given leaf cannot be inserted within this branch (%p < %p) (addr_semi %p; level %u)",
         newleaf_min,KSHMBRANCH_MAPMIN(addr_semi,addr_level),addr_semi,addr_level);
 assertf(newleaf_max <= KSHMBRANCH_MAPMAX(addr_semi,addr_level),
         "The given leaf cannot be inserted within this branch (%p > %p) (addr_semi %p; level %u)",
         newleaf_max,KSHMBRANCH_MAPMAX(addr_semi,addr_level),addr_semi,addr_level);
 if ((iter = *pcurr) == NULL) {
  /* Simple case: First leaf. */
  newleaf->sb_min = NULL;
  newleaf->sb_max = NULL;
  *pcurr = newleaf;
got_it:
  assert(newleaf_min >= KSHMBRANCH_MAPMIN(addr_semi,addr_level));
  assert(newleaf_max <= KSHMBRANCH_MAPMAX(addr_semi,addr_level));
  return KE_OK;
 }
 /* Special case: Check if the given branch overlaps with our current. */
 if __unlikely(newleaf_min <= iter->sb_map_max &&
               newleaf_max >= iter->sb_map_min) {
  /* ERROR: Requested address range is already covered. */
  return KE_EXISTS;
 }
 /* Special case: Our new leaf covers this exact branch.
  * --> Must move the existing leaf and replace '*proot' */
 if (newleaf_min <= addr_semi &&
     newleaf_max >= addr_semi) {
  assertf(iter->sb_map_max <= addr_semi ||
          iter->sb_map_min >= addr_semi,
          "But that would mean we are overlapping...");
  /* Override a given branch with a new node.
   * This is a pretty complicated process, because we
   * can't simply shift the entire tree down one level.
   * >> Some of the underlying branches may have been
   *    perfect fits before (aka. addr_semi-fits), yet
   *    if we were to shift them directly, they would
   *    reside in invalid and unexpected locations,
   *    causing the entire tree to break.
   * >> Instead we must recursively re-insert all
   *    underlying branches, even though that might
   *    seem extremely inefficient. */
  newleaf->sb_min = NULL;
  newleaf->sb_max = NULL;
  *pcurr = newleaf;
  kshmbranch_reinsertall(pcurr,iter,addr_semi,addr_level);
  goto got_it;
 }
 /* We are not a perfect fit for this leaf because
  * we're not covering its addr_semi.
  * -> Instead, we are either located entirely below,
  *    or entirely above the semi-point. */
 if (newleaf_max < addr_semi) {
  /* We are located below. */
  KSHMBRANCH_WALKMIN(addr_semi,addr_level);
  pcurr = &iter->sb_min;
 } else {
  /* We are located above. */
  assertf(newleaf_min > addr_semi
         ,"We checked above if we're covering the semi!");
  KSHMBRANCH_WALKMAX(addr_semi,addr_level);
  pcurr = &iter->sb_max;
 }
 goto again;
}

__crit __nomp void
kshmbranch_reinsertall(struct kshmbranch **__restrict proot,
                       struct kshmbranch *__restrict insert_root,
                       uintptr_t addr_semi, unsigned int addr_level) {
#if 0
 printf("recursive_insert leaf %p .. %p at %p .. %p semi %p, level %u\n",
        newleaf->sb_map_min,newleaf->sb_map_max,
        KSHMBRANCH_MAPMIN(addr_semi,level),
        KSHMBRANCH_MAPMAX(addr_semi,level),
        addr_semi,level);
#endif
 kassertobj(insert_root);
 kassert_kshmregion(insert_root->sb_region);
 if (insert_root->sb_min) kshmbranch_reinsertall(proot,insert_root->sb_min,addr_semi,addr_level);
 if (insert_root->sb_max) kshmbranch_reinsertall(proot,insert_root->sb_max,addr_semi,addr_level);
 asserte(kshmbranch_insert(proot,insert_root,addr_semi,addr_level) == KE_OK);
}



__crit __nomp struct kshmbranch *
kshmbranch_popone(struct kshmbranch **proot,
                  __uintptr_t addr_semi,
                  unsigned int addr_level) {
 struct kshmbranch *root;
 kassertobj(proot);
 root = *proot;
 kassertobj(root);
 kassert_kshmregion(root->sb_region);
 *proot = NULL;
 if (root->sb_min) kshmbranch_reinsertall(proot,root->sb_min,addr_semi,addr_level);
 if (root->sb_max) kshmbranch_reinsertall(proot,root->sb_max,addr_semi,addr_level);
 return root;
}


__crit __nomp struct kshmbranch *
kshmbranch_remove(struct kshmbranch **__restrict proot,
                  uintptr_t addr) {
 struct kshmbranch *iter;
 uintptr_t    addr_semi  = KSHMBRANCH_ADDRSEMI_INIT;
 unsigned int addr_level = KSHMBRANCH_ADDRLEVEL_INIT;
 kassertobj(proot);
 while ((iter = *proot) != NULL) {
  /* Make sure that the given entry can truly be inserted somewhere within this branch. */
  assertf(addr >= KSHMBRANCH_MAPMIN(addr_semi,addr_level),
          "The given address isn't located within this branch (%p < %p)",
          addr,KSHMBRANCH_MAPMIN(addr_semi,addr_level));
  assertf(addr <= KSHMBRANCH_MAPMAX(addr_semi,addr_level),
          "The given address isn't located within this branch (%p > %p)",
          addr,KSHMBRANCH_MAPMAX(addr_semi,addr_level));
  /* Check if the given address lies within this branch. */
  if (addr >= iter->sb_map_min &&
      addr <= iter->sb_map_max) goto found_it;
  assert(addr_level);
  if (addr < addr_semi) {
   /* Continue with min-branch */
   proot = &iter->sb_min;
   KSHMBRANCH_WALKMIN(addr_semi,addr_level);
  } else {
   /* Continue with max-branch */
   KSHMBRANCH_WALKMAX(addr_semi,addr_level);
   proot = &iter->sb_max;
  }
 }
 return NULL;
found_it:
 /* Reminders/Sanity checks. */
 assert(iter == *proot);
 assert(addr >= iter->sb_map_min &&
        addr <= iter->sb_map_max);
 return kshmbranch_popone(proot,addr_semi,addr_level);
}

__DECL_END

#endif /* !__KOS_KERNEL_SHM_MAP_C_INL__ */

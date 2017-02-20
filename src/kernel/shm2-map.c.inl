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
#ifndef __KOS_KERNEL_SHM2_MAP_C_INL__
#define __KOS_KERNEL_SHM2_MAP_C_INL__ 1

#include <malloc.h>
#include <kos/config.h>
#include <kos/kernel/features.h>
#include <kos/kernel/interrupts.h>
#include <kos/kernel/shm2.h>

#if KCONFIG_USE_SHM2
__DECL_BEGIN

static __attribute_unused void
print_branch(struct kshmbranch *__restrict branch,
             uintptr_t addr_semi, unsigned int level) {
 unsigned int new_level,i; uintptr_t new_semi;
 for (i = 31; i > level; --i) printf("%s",(addr_semi&(1 << i)) ? "+" : "-");
 for (i = 0; i < level; ++i) printf(" ");
 printf("leaf (%c%c%c) %p .. %p at %p .. %p semi %p, level %u\n",
        (branch->sb_region->sre_chunk.sc_flags&KSHMREGION_FLAG_EXEC ) ? 'X' : '-',
        (branch->sb_region->sre_chunk.sc_flags&KSHMREGION_FLAG_WRITE) ? 'W' : '-',
        (branch->sb_region->sre_chunk.sc_flags&KSHMREGION_FLAG_READ ) ? 'R' : '-',
        branch->sb_map_min,branch->sb_map_max,
        KSHMBRANCH_MAPMIN(addr_semi,level),
        KSHMBRANCH_MAPMAX(addr_semi,level),
        addr_semi,level);
 assert(branch->sb_map_min >= KSHMBRANCH_MAPMIN(addr_semi,level));
 assert(branch->sb_map_max <= KSHMBRANCH_MAPMAX(addr_semi,level));
 if (branch->sb_min) {
  new_semi = addr_semi,new_level = level;
  KSHMBRANCH_WALKMIN(new_semi,new_level);
  print_branch(branch->sb_min,new_semi,new_level);
 }
 if (branch->sb_max) {
  new_semi = addr_semi,new_level = level;
  KSHMBRANCH_WALKMAX(new_semi,new_level);
  print_branch(branch->sb_max,new_semi,new_level);
 }
}

static void
reinsert_recursive(struct kshmbranch **__restrict pbranch,
                   struct kshmbranch *__restrict newleaf,
                   uintptr_t addr_semi, unsigned int level) {
#if 0
 printf("recursive_insert leaf %p .. %p at %p .. %p semi %p, level %u\n",
        newleaf->sb_map_min,newleaf->sb_map_max,
        KSHMBRANCH_MAPMIN(addr_semi,level),
        KSHMBRANCH_MAPMAX(addr_semi,level),
        addr_semi,level);
#endif
 if (newleaf->sb_min) reinsert_recursive(pbranch,newleaf->sb_min,addr_semi,level);
 if (newleaf->sb_max) reinsert_recursive(pbranch,newleaf->sb_max,addr_semi,level);
 asserte(kshmbranch_insert(pbranch,newleaf,addr_semi,level) == KE_OK);
}


/* Override a given branch with a new node.
 * This is a pretty complicated process, because any branch of the old
 * branch's min->[max...] and max->[min...] chain must be rebuild. */
static void
override_branch(struct kshmbranch **__restrict pbranch,
                struct kshmbranch *__restrict newleaf,
                uintptr_t addr_semi, unsigned int level) {
#if 0
 struct kshmbranch *iter,**app_min,**app_max;
 uintptr_t newleaf_min;
 app_min     = &newleaf->sb_min;
 app_max     = &newleaf->sb_max;
 newleaf_min = newleaf->sb_map_min;
 iter        = *pbranch,*pbranch = newleaf;
 while (iter) {
  assertf(iter->sb_map_max < newleaf_min ||
          iter->sb_map_min > newleaf->sb_map_max,
          "newleaf is overlapping with an existing branch");
  if (iter->sb_map_max < addr_semi) {
   /* Branch is located below the new leaf.
    * (append to min-chain & continue searching towards max) */
   *app_min = iter;
   app_min  = &iter->sb_max;
   iter     = iter->sb_max;
   KSHMBRANCH_WALKMAX(addr_semi,level);
  } else {
   assert(iter->sb_map_min > addr_semi);
   /* Branch is located above the new leaf.
    * (append to max-chain & continue searching towards min) */
   *app_max = iter;
   app_max  = &iter->sb_min;
   iter     = iter->sb_min;
   KSHMBRANCH_WALKMIN(addr_semi,level);
  }
 }
 /* Terminate the append-chains. */
 *app_min = NULL;
 *app_max = NULL;
#else
 struct kshmbranch *iter;
 iter = *pbranch,*pbranch = newleaf;
 newleaf->sb_min = NULL;
 newleaf->sb_max = NULL;
 reinsert_recursive(pbranch,iter,addr_semi,level);
#endif
}

__crit __nomp kerrno_t
kshmbranch_insert(struct kshmbranch **__restrict pcurr,
                  struct kshmbranch *__restrict newleaf,
                  uintptr_t addr_semi, unsigned int addr_level) {
 struct kshmbranch *iter;
 uintptr_t newleaf_min,newleaf_max;
 kassertobj(pcurr);
 kassertobj(newleaf);
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
  override_branch(pcurr,newleaf,addr_semi,addr_level);
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


__crit __nomp struct kshmbranch *
kshmbranch_combine(struct kshmbranch *__restrict min_branch,
                   struct kshmbranch *__restrict max_branch) {
 /* NOTE: Technically we'd need to initialize the sizes to 1, but since
  *       we're just comparing them later, we can just offset both. */
 unsigned int min_size = 0,max_size = 0;
 struct kshmbranch **min_pend,**max_pend;
 kassertobj(min_branch);
 kassertobj(max_branch);
 min_pend = &min_branch->sb_max,
 max_pend = &max_branch->sb_min;
 /* Complicated case: the leaf we're supposed to remove
  *                   has two branches coming off of it.
  * >> We solve this situation by looking at how deep
  *    the right-most branch of the left branch reaches,
  *    as well as how deep the left-most of the right one does.
  * >> Then we append the longer of the two at the end of the shorter:
  *
  *   .   e        >>|>>    .   e
  *    \ /         >>|>>     \ /
  * .   c   d   .  >>|>>  .   c
  *  \ /     \ /   >>|>>   \ /
  *   a       b    >>|>>    a   .
  *    \     /     >>|>>     \ /
  *     \   /      >>|>>      d
  *      \ /       >>|>>       \
  *      [x]       >>|>>        b
  *      /         >>|>>       /
  *     .          >>|>>      .
  */
 while (*min_pend) ++min_size,min_pend = &(*min_pend)->sb_max;
 while (*max_pend) ++max_size,max_pend = &(*max_pend)->sb_min;
 assert(!*max_pend);
 assert(!*min_pend);
 if (max_size <= min_size) {
  /* Append min onto max & append max on *proot. */
  *max_pend = min_branch;
  return max_branch;
 } else {
  /* Append max onto min & append min on *proot. */
  *min_pend = max_branch;
  return min_branch;
 }
}


__crit __nomp struct kshmbranch *
kshmbranch_popone(struct kshmbranch **proot) {
 struct kshmbranch *root;
 kassertobj(proot);
 root = *proot;
 kassertobj(root);
 kassert_kshmregion(root->sb_region);
 *proot = root->sb_min ? (root->sb_max
  ? kshmbranch_combine(root->sb_min,root->sb_max) /*< Combine min+max. */
  : root->sb_min) /*< Only min branch. */
  : root->sb_max; /*< Only max branch. */
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
 return kshmbranch_popone(proot);
}

__DECL_END
#endif /* KCONFIG_USE_SHM2 */

#endif /* !__KOS_KERNEL_SHM2_MAP_C_INL__ */

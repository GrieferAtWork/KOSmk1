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

__crit __nomp kerrno_t
kshmbranch_insert(struct kshmbranch **__restrict pcurr,
                  struct kshmbranch *__restrict newleaf) {
 struct kshmbranch *iter;
 uintptr_t    addr_semi  = KSHMBRANCH_ADDRSEMI_INIT;
 unsigned int addr_level = KSHMBRANCH_ADDRLEVEL_INIT;
 uintptr_t newleaf_min,newleaf_max;
 kassertobj(pcurr);
 kassertobj(newleaf);
 newleaf_min = newleaf->sb_map_min;
 newleaf_max = newleaf->sb_map_max;
again:
 /* Make sure that the given entry can truly be inserted somewhere within this branch. */
 assertf(newleaf_min >= KSHMBRANCH_MAPMIN(addr_semi,addr_level),
         "The given leaf cannot be inserted within this branch (%p < %p)",
         newleaf_min,KSHMBRANCH_MAPMIN(addr_semi,addr_level));
 assertf(newleaf_max <= KSHMBRANCH_MAPMAX(addr_semi,addr_level),
         "The given leaf cannot be inserted within this branch (%p > %p)",
         newleaf_max,KSHMBRANCH_MAPMAX(addr_semi,addr_level));
 if ((iter = *pcurr) == NULL) {
  /* Simple case: First leaf. */
  *pcurr = newleaf;
  return KE_OK;
 }
 /* Special case: Check if the given branch overlaps with our current. */
 if __unlikely(newleaf_min <= iter->sb_map_max &&
               newleaf_max >= iter->sb_map_min) {
  /* ERROR: Requested address range is already covered. */
  return KE_EXISTS;
 }
 /* Special case: Our new covers this exact branch.
  * --> Must move the existing leaf and replace '*proot' */
 if (newleaf_min <= addr_semi &&
     newleaf_max >= addr_semi) {
  assertf(iter->sb_map_max <= addr_semi ||
          iter->sb_map_min >= addr_semi,
          "But that would mean we are overlapping...");
  if (iter->sb_map_max <= addr_semi) {
   /* Old leave must be stored within the min-branch. */
   newleaf->sb_min = iter;
   newleaf->sb_max = NULL;
  } else {
   /* Old leave must be stored within the max-branch. */
   newleaf->sb_max = iter;
   newleaf->sb_min = NULL;
  }
  *pcurr = newleaf;
  return KE_OK;
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
  assert(newleaf_min > addr_semi);
  KSHMBRANCH_WALKMAX(addr_semi,addr_level);
  pcurr = &iter->sb_max;
 }
 goto again;
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
 if (iter->sb_min) {
  if (!iter->sb_max) *proot = iter->sb_min;
  else {
   /* NOTE: Technically we'd need to initialize the sizes to 1, but since
    *       we're just comparing them later, we can just offset both. */
   unsigned int min_size = 0,max_size = 0;
   struct kshmbranch **min_pend = &iter->sb_min->sb_max,
                     **max_pend = &iter->sb_max->sb_min;
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
    *max_pend = iter->sb_min;
    *proot    = iter->sb_max;
   } else {
    /* Append max onto min & append min on *proot. */
    *min_pend = iter->sb_max;
    *proot    = iter->sb_min;
   }
  }
 } else {
  *proot = iter->sb_max;
 }
 return iter;
}


__DECL_END

#endif /* !__KOS_KERNEL_SHM_MAP_C_INL__ */

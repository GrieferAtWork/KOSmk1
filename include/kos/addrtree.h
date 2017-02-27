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
#ifndef __KOS_KERNEL_UTIL_ADDRTREE_H__
#define __KOS_KERNEL_UTIL_ADDRTREE_H__ 1

#include <kos/config.h>
#include <kos/compiler.h>

//////////////////////////////////////////////////////////////////////////
// An address tree is a binary-tree based container designed
// for extremely fast lookup to a mapping for any address.
// Worst case lookup speed is O(31) on 32-bit and O(63) on 64-bit,
// though available ram will severely reduce this number again,
// with lookup speed also being affected by the amount of mapped
// leafs/branches, as well as how they are mapped, as well as the
// index of the least significant bit set in a given address.
//
// Also noteworthy is the fact that instead of mapping individual
// addresses to unique leafs, an address tree maps regions of
// addresses (so called address ranges/regions).
// With that in mind, mapping an address range more than once
// isn't allowed, causing either an error, or undefined behavior.
//
// An address tree follows strict ordering rules that enforce
// a min-max mapping for every leaf that can be determined by
// its associated addrsemi and addrlevel value.
// 
// addrsemi:
//   - The center point used as hint to plot the
//     correct path for any mapped address.
//   - Each time a branch is reached, the given pointer
//     is checked to be located inside of its mapping.
//     Only when it wasn't found, continue search as described below.
//   Assuming 32-bit addresses, a path might be plotted like this:
//     ADDR: 0x73400000
//     LEVEL 31; SEMI 0x80000000 >= 0x73400000 -> MIN (unset bit 31; set bit 30)
//     LEVEL 30; SEMI 0x40000000 <  0x73400000 -> MAX (              set bit 29)
//     LEVEL 29; SEMI 0x60000000 <  0x73400000 -> MAX (              set bit 28)
//     LEVEL 28; SEMI 0x70000000 <  0x73400000 -> MAX (              set bit 27)
//     LEVEL 27; SEMI 0x78000000 >= 0x73400000 -> MIN (unset bit 27; set bit 26)
//     LEVEL 26; SEMI 0x74000000 >= 0x73400000 -> MIN (unset bit 26; set bit 25)
//     LEVEL 25; SEMI 0x72000000 <  0x73400000 -> MAX (              set bit 24)
//     LEVEL 24; SEMI 0x73000000 <  0x73400000 -> MAX (              set bit 23)
//     LEVEL 23; SEMI 0x73800000 >= 0x73400000 -> MIN (unset bit 23; set bit 22)
//     LEVEL 22; SEMI 0x73400000 == 0x73400000 -> STOP SEARCH
//   >> The address '0x73400000' can be mapped to 10 different leafes,
//      with the worst case (aka. failure) lookup time for this address
//      always being O(10).
//      Note though that for the lookup time to actually be 10, other mapping
//      must be available that cover _all_ of the SEMI-values during all levels.
//     
// addrlevel:
//   - The index of the bit that was set for the current iteration.
//   - Technically not required, as it can be calculated from an addrsemi,
//     carrying this around still allows the addrtree to run faster.
//
//
// The 'addrtree_head' is designed as an inline
// structure to be included in other, larger objects:
//
// >> struct my_mapping {
// >>   struct addrtree_head head;
// >>   int                  value;
// >> };
// >> 
// >> static struct my_mapping *tree = NULL;
// >> 
// >> static int lookup(uintptr_t addr) {
// >>   struct my_mapping *m = (struct my_mapping *)ADDRTREE_LOCATE(tree,addr);
// >>   return m ? m->value : -1;
// >> }
// >> static void map(uintptr_t min, uintptr_t max, int val) {
// >>   struct my_mapping *m = omalloc(struct my_mapping);
// >>   addrtree_init_for_insert(&m->head,min,max);
// >>   m->value = val;
// >>   ADDRTREE_INSERT(&tree,m);
// >> }
// >> static void unmapat(uintptr_t addr) {
// >>   // NOTE: Fast ranged unmapping is also possible, but not covered here.
// >>   free(ADDRTREE_REMOVE(&tree,addr));
// >> }
//
//

#ifndef __addrtree_interface_defined
#define __addrtree_interface_defined 1
#include <kos/types.h>
#include <kos/errno.h>
#include <stddef.h>
#include <strings.h>

__DECL_BEGIN

typedef __uintptr_t  addrtree_addr_t;
typedef __uintptr_t  addrtree_semi_t;
typedef unsigned int addrtree_level_t;

struct addrtree_head {
 struct addrtree_head *at_min;     /*< [0..1] Min-address mapping. */
 struct addrtree_head *at_max;     /*< [0..1] Max-address mapping. */
 addrtree_addr_t       at_map_min; /*< Lowest address associated with this mapping. */
 addrtree_addr_t       at_map_max; /*< Greatest address associated with this mapping. */
};
#define ADDRTREE_INIT(min,max) {NULL,NULL,min,max}
#define addrtree_init(self,min,max) \
 (void)((self)->at_min = (self)->at_max = NULL,\
        (self)->at_map_min = (min),\
        (self)->at_map_max = (max))
#define addrtree_init_for_insert(self,min,max) \
 (void)((self)->at_map_min = (min),\
        (self)->at_map_max = (max))

#define ADDRTREE_ADDRSEMI_INIT   ((((addrtree_semi_t)-1)/2)+1)
#define ADDRTREE_ADDRLEVEL_INIT  ((__SIZEOF_POINTER__*8)-1)

#define ADDRTREE_MAPMIN(semi,level)  (addrtree_addr_t)((semi)&~(((addrtree_semi_t)1 << (level))))
#define ADDRTREE_MAPMAX(semi,level)  (addrtree_addr_t)((semi)| (((addrtree_semi_t)1 << (level))-1))
#define ADDRTREE_WALKMIN(semi,level) ((semi)  = (((semi)&~((addrtree_semi_t)1 << (level)))|((addrtree_semi_t)1 << ((level)-1))),--(level)) /*< unset level'th bit; set level'th-1 bit. */
#define ADDRTREE_WALKMAX(semi,level) (--(level),(semi) |= ((addrtree_semi_t)1 << (level)))                                                  /*< set level'th-1 bit. */
#define ADDRTREE_NEXTMIN(semi,level) (((semi)&~((addrtree_semi_t)1 << (level)))|((addrtree_semi_t)1 << ((level)-1))) /*< unset level'th bit; set level'th-1 bit. */
#define ADDRTREE_NEXTMAX(semi,level) ((semi)|((addrtree_semi_t)1 << ((level)-1)))                                     /*< set level'th-1 bit. */
#define ADDRTREE_SEMILEVEL(semi)     (addrtree_level_t)(ffs(semi)-1) /*< Get the current level associated with a given semi-address. */

/* Convenience functions. */
#define ADDRTREE_LOCATE(root,addr)        addrtree_locate(root,addr,ADDRTREE_ADDRSEMI_INIT,ADDRTREE_ADDRLEVEL_INIT)
#define ADDRTREE_INSERT(proot,newleaf)    addrtree_insert(proot,newleaf,ADDRTREE_ADDRSEMI_INIT,ADDRTREE_ADDRLEVEL_INIT)
#define ADDRTREE_REMOVE(proot,address)    addrtree_remove(proot,address,ADDRTREE_ADDRSEMI_INIT,ADDRTREE_ADDRLEVEL_INIT)
#define ADDRTREE_TRYINSERT(proot,newleaf) addrtree_tryinsert(proot,newleaf,ADDRTREE_ADDRSEMI_INIT,ADDRTREE_ADDRLEVEL_INIT)

#if defined(__KERNEL__) || defined(WANT_ADDRTREE_EXTERN)
#define ADDRTREE_DECL extern
#else
#define ADDRTREE_DECL __local
#endif


//////////////////////////////////////////////////////////////////////////
// Tries to insert the given leaf.
// @return: KE_OK:     Successfully inserted the given leaf.
// @return: KE_EXISTS: The address range described by 'newleaf' is already mapped.
ADDRTREE_DECL __crit __nomp __nonnull((1,2)) kerrno_t
addrtree_tryinsert(struct addrtree_head **__restrict proot,
                   struct addrtree_head *__restrict newleaf,
                   addrtree_semi_t addr_semi, addrtree_level_t addr_level);

//////////////////////////////////////////////////////////////////////////
// Similar to 'addrtree_tryinsert', but causes undefined behavior
// if the address range covered by the given leaf already existed.
ADDRTREE_DECL __crit __nomp __nonnull((1,2)) void
addrtree_insert(struct addrtree_head **__restrict proot,
                struct addrtree_head *__restrict newleaf,
                addrtree_semi_t addr_semi, addrtree_level_t addr_level);

//////////////////////////////////////////////////////////////////////////
// Returns the old '*proot' and replaces it with either its min, or max branch.
// Special handling is performed to determine the perfect match when
// '*proot' has both a min and a max branch.
ADDRTREE_DECL __crit __nomp __nonnull((1)) struct addrtree_head *
addrtree_pop(struct addrtree_head **__restrict proot,
             addrtree_semi_t addr_semi, addrtree_level_t addr_level);

//////////////////////////////////////////////////////////////////////////
// Remove and return the leaf associated with the given address.
// >> This is a combination of 'addrtree_plocate' and 'addrtree_pop'
ADDRTREE_DECL __crit __nomp __nonnull((1)) struct addrtree_head *
addrtree_remove(struct addrtree_head **__restrict proot,
                addrtree_addr_t address,
                addrtree_semi_t addr_semi,
                addrtree_level_t addr_level);


//////////////////////////////////////////////////////////////////////////
// Locate the leaf associated with a given address.
// @return: NULL: No leaf associated with the given address.
// NOTE: 'addrtree_plocate' will update 'addr_semi' and 'addr_level'.
ADDRTREE_DECL __nomp struct addrtree_head *
addrtree_locate(struct addrtree_head *root,
                addrtree_addr_t address,
                addrtree_semi_t addr_semi,
                addrtree_level_t addr_level);
ADDRTREE_DECL __nomp __nonnull((1,3,4)) struct addrtree_head **
addrtree_plocate(struct addrtree_head **__restrict proot,
                 addrtree_addr_t address,
                 addrtree_semi_t *__restrict paddr_semi,
                 addrtree_level_t *__restrict paddr_level);

__DECL_END
#endif /* !__addrtree_interface_defined */


#if !(defined(__KERNEL__) || defined(WANT_ADDRTREE_EXTERN)) || \
      defined(IMPLEMENT_ADDRTREE_EXTERN) || defined(__INTELLISENSE__)
#ifndef __addrtree_implementation_defined
#define __addrtree_implementation_defined 1
#include <kos/types.h>
#include <kos/errno.h>
#include <kos/kernel/debug.h>
#include <stddef.h>
#include <assert.h>

__DECL_BEGIN

static __crit __nomp void
__addrtree_reinsertall(struct addrtree_head **__restrict proot,
                       struct addrtree_head *__restrict insert_root,
                       addrtree_semi_t addr_semi,
                       addrtree_level_t addr_level) {
 kassertobj(insert_root);
 if (insert_root->at_min) __addrtree_reinsertall(proot,insert_root->at_min,addr_semi,addr_level);
 if (insert_root->at_max) __addrtree_reinsertall(proot,insert_root->at_max,addr_semi,addr_level);
 addrtree_insert(proot,insert_root,addr_semi,addr_level);
}


//////////////////////////////////////////////////////////////////////////
// Tries to insert the given leaf.
// @return: KE_OK:     Successfully inserted the given leaf.
// @return: KE_EXISTS: The address range described by 'newleaf' is already mapped.
ADDRTREE_DECL __crit __nomp kerrno_t
addrtree_tryinsert(struct addrtree_head **__restrict proot,
                   struct addrtree_head *__restrict newleaf,
                   addrtree_semi_t addr_semi,
                   addrtree_level_t addr_level) {
 struct addrtree_head *iter;
 addrtree_semi_t newleaf_min,newleaf_max;
 kassertobj(proot);
 kassertobj(newleaf);
 newleaf_min = newleaf->at_map_min;
 newleaf_max = newleaf->at_map_max;
again:
 /* Make sure that the given entry can truly be inserted somewhere within this branch. */
 assertf(newleaf_min >= ADDRTREE_MAPMIN(addr_semi,addr_level),
         "The given leaf cannot be inserted within this branch (%p < %p) (addr_semi %p; level %u)",
         newleaf_min,ADDRTREE_MAPMIN(addr_semi,addr_level),addr_semi,addr_level);
 assertf(newleaf_max <= ADDRTREE_MAPMAX(addr_semi,addr_level),
         "The given leaf cannot be inserted within this branch (%p > %p) (addr_semi %p; level %u)",
         newleaf_max,ADDRTREE_MAPMAX(addr_semi,addr_level),addr_semi,addr_level);
 if ((iter = *proot) == NULL) {
  /* Simple case: First leaf. */
  newleaf->at_min = NULL;
  newleaf->at_max = NULL;
  *proot = newleaf;
got_it:
  assert(newleaf_min >= ADDRTREE_MAPMIN(addr_semi,addr_level));
  assert(newleaf_max <= ADDRTREE_MAPMAX(addr_semi,addr_level));
  return KE_OK;
 }
 /* Special case: Check if the given branch overlaps with our current. */
 if __unlikely(newleaf_min <= iter->at_map_max &&
               newleaf_max >= iter->at_map_min) {
  /* ERROR: Requested address range is already covered. */
  return KE_EXISTS;
 }
 /* Special case: Our new leaf covers this exact branch.
  * --> Must move the existing leaf and replace '*proot' */
 if (newleaf_min <= addr_semi &&
     newleaf_max >= addr_semi) {
  assertf(iter->at_map_max <= addr_semi ||
          iter->at_map_min >= addr_semi,
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
  newleaf->at_min = NULL;
  newleaf->at_max = NULL;
  *proot = newleaf;
  __addrtree_reinsertall(proot,iter,addr_semi,addr_level);
  goto got_it;
 }
 /* We are not a perfect fit for this leaf because
  * we're not covering its addr_semi.
  * -> Instead, we are either located entirely below,
  *    or entirely above the semi-point. */
 if (newleaf_max < addr_semi) {
  /* We are located below. */
  ADDRTREE_WALKMIN(addr_semi,addr_level);
  proot = &iter->at_min;
 } else {
  /* We are located above. */
  assertf(newleaf_min > addr_semi
         ,"We checked above if we're covering the semi!");
  ADDRTREE_WALKMAX(addr_semi,addr_level);
  proot = &iter->at_max;
 }
 goto again;
}

//////////////////////////////////////////////////////////////////////////
// Similar to 'addrtree_tryinsert', but causes undefined behavior
// if the address range covered by the given leaf already existed.
ADDRTREE_DECL __crit __nomp void
addrtree_insert(struct addrtree_head **__restrict proot,
                struct addrtree_head *__restrict newleaf,
                addrtree_semi_t addr_semi,
                addrtree_level_t addr_level) {
 struct addrtree_head *iter;
 addrtree_semi_t newleaf_min,newleaf_max;
 kassertobj(proot);
 kassertobj(newleaf);
 newleaf_min = newleaf->at_map_min;
 newleaf_max = newleaf->at_map_max;
again:
 /* Make sure that the given entry can truly be inserted somewhere within this branch. */
 assertf(newleaf_min >= ADDRTREE_MAPMIN(addr_semi,addr_level),
         "The given leaf cannot be inserted within this branch (%p < %p) (addr_semi %p; level %u)",
         newleaf_min,ADDRTREE_MAPMIN(addr_semi,addr_level),addr_semi,addr_level);
 assertf(newleaf_max <= ADDRTREE_MAPMAX(addr_semi,addr_level),
         "The given leaf cannot be inserted within this branch (%p > %p) (addr_semi %p; level %u)",
         newleaf_max,ADDRTREE_MAPMAX(addr_semi,addr_level),addr_semi,addr_level);
 if ((iter = *proot) == NULL) {
  /* Simple case: First leaf. */
  newleaf->at_min = NULL;
  newleaf->at_max = NULL;
  *proot = newleaf;
got_it:
  assert(newleaf_min >= ADDRTREE_MAPMIN(addr_semi,addr_level));
  assert(newleaf_max <= ADDRTREE_MAPMAX(addr_semi,addr_level));
  return;
 }
 /* Special case: Check if the given branch overlaps with our current. */
 assertf(!(newleaf_min <= iter->at_map_max &&
           newleaf_max >= iter->at_map_min),
         "ERROR: Requested address range is already covered");
 /* Special case: Our new leaf covers this exact branch.
  * --> Must move the existing leaf and replace '*proot' */
 if (newleaf_min <= addr_semi &&
     newleaf_max >= addr_semi) {
  assertf(iter->at_map_max <= addr_semi ||
          iter->at_map_min >= addr_semi,
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
  newleaf->at_min = NULL;
  newleaf->at_max = NULL;
  *proot = newleaf;
  __addrtree_reinsertall(proot,iter,addr_semi,addr_level);
  goto got_it;
 }
 /* We are not a perfect fit for this leaf because
  * we're not covering its addr_semi.
  * -> Instead, we are either located entirely below,
  *    or entirely above the semi-point. */
 if (newleaf_max < addr_semi) {
  /* We are located below. */
  ADDRTREE_WALKMIN(addr_semi,addr_level);
  proot = &iter->at_min;
 } else {
  /* We are located above. */
  assertf(newleaf_min > addr_semi
         ,"We checked above if we're covering the semi!");
  ADDRTREE_WALKMAX(addr_semi,addr_level);
  proot = &iter->at_max;
 }
 goto again;
}

//////////////////////////////////////////////////////////////////////////
// Returns the old '*proot' and replaces it with either its min, or max branch.
// Special handling is performed to determine the perfect match when
// '*proot' has both a min and a max branch.
ADDRTREE_DECL __crit __nomp __nonnull((1)) struct addrtree_head *
addrtree_pop(struct addrtree_head **__restrict proot,
             addrtree_semi_t addr_semi,
             addrtree_level_t addr_level) {
 struct addrtree_head *root;
 kassertobj(proot); root = *proot;
 kassertobj(root); *proot = NULL;
 if (root->at_min) __addrtree_reinsertall(proot,root->at_min,addr_semi,addr_level);
 if (root->at_max) __addrtree_reinsertall(proot,root->at_max,addr_semi,addr_level);
 return root;
}

//////////////////////////////////////////////////////////////////////////
// Remove and return the leaf associated with the given address.
// >> This is a combination of 'addrtree_plocate' and 'addrtree_pop'
ADDRTREE_DECL __crit __nomp __nonnull((1)) struct addrtree_head *
addrtree_remove(struct addrtree_head **__restrict proot,
                addrtree_addr_t address,
                addrtree_semi_t addr_semi,
                addrtree_level_t addr_level) {
 struct addrtree_head **remove_head;
 remove_head = addrtree_plocate(proot,address,&addr_semi,&addr_level);
 return remove_head ? addrtree_pop(remove_head,addr_semi,addr_level) : NULL;
}


//////////////////////////////////////////////////////////////////////////
// Locate the leaf associated with a given address.
// @return: NULL: No leaf associated with the given address.
// NOTE: 'addrtree_plocate' will update 'addr_semi' and 'addr_level'.
ADDRTREE_DECL __nomp struct addrtree_head *
addrtree_locate(struct addrtree_head *root,
                addrtree_addr_t address,
                addrtree_semi_t addr_semi,
                addrtree_level_t addr_level) {
 /* addr_semi is the center point splitting the max
  * ranges of the underlying sb_min/sb_max branches. */
 while (root) {
  kassertobj(root);
#ifdef __DEBUG__
  { /* Assert that the current branch has a valid min/max address range. */
   addrtree_addr_t addr_min = ADDRTREE_MAPMIN(addr_semi,addr_level);
   addrtree_addr_t addr_max = ADDRTREE_MAPMAX(addr_semi,addr_level);
   assertf(root->at_map_min <= root->at_map_max,
           "Branch has invalid min/max configuration (min(%p) > max(%p))",
           root->at_map_min,root->at_map_max);
   assertf(root->at_map_min >= addr_min,
           "Unexpected branch min address (%p < %p; max: %p; looking for %p; semi %p; level %u)",
           root->at_map_min,addr_min,root->at_map_max,address,addr_semi,addr_level);
   assertf(root->at_map_max <= addr_max,
           "Unexpected branch max address (%p > %p; min: %p; looking for %p; semi %p; level %u)",
           root->at_map_max,addr_max,root->at_map_min,address,addr_semi,addr_level);
  }
#endif
  /* Check if the given address lies within this branch. */
  if (address >= root->at_map_min &&
      address <= root->at_map_max) break;
  assert(addr_level);
  if (address < addr_semi) {
   /* Continue with min-branch */
   ADDRTREE_WALKMIN(addr_semi,addr_level);
   root = root->at_min;
  } else {
   /* Continue with max-branch */
   ADDRTREE_WALKMAX(addr_semi,addr_level);
   root = root->at_max;
  }
 }
 return root;
}
ADDRTREE_DECL __nomp struct addrtree_head **
addrtree_plocate(struct addrtree_head **__restrict proot,
                 addrtree_addr_t address,
                 addrtree_semi_t *__restrict paddr_semi,
                 addrtree_level_t *__restrict paddr_level) {
 struct addrtree_head *root;
 addrtree_semi_t addr_semi = (kassertobj(paddr_semi),*paddr_semi);
 addrtree_level_t addr_level = (kassertobj(paddr_level),*paddr_level);
 /* addr_semi is the center point splitting the max
  * ranges of the underlying sb_min/sb_max branches. */
 while ((kassertobj(proot),(root = *proot) != NULL)) {
  kassertobj(root);
#ifdef __DEBUG__
  { /* Assert that the current branch has a valid min/max address range. */
   addrtree_addr_t addr_min = ADDRTREE_MAPMIN(addr_semi,addr_level);
   addrtree_addr_t addr_max = ADDRTREE_MAPMAX(addr_semi,addr_level);
   assertf(root->at_map_min <= root->at_map_max,"Branch has invalid min/max configuration (min(%p) > max(%p))",root->at_map_min,root->at_map_max);
   assertf(root->at_map_min >= addr_min,
           "Unexpected branch min address (%p < %p; max: %p; looking for %p; semi %p; level %u)",
           root->at_map_min,addr_min,root->at_map_max,address,addr_semi,addr_level);
   assertf(root->at_map_max <= addr_max,
           "Unexpected branch max address (%p > %p; min: %p; looking for %p; semi %p; level %u)",
           root->at_map_max,addr_max,root->at_map_min,address,addr_semi,addr_level);
  }
#endif
  /* Check if the given address lies within this branch. */
  if (address >= root->at_map_min &&
      address <= root->at_map_max) {
   *paddr_semi = addr_semi;
   *paddr_level = addr_level;
   return proot;
  }
  assert(addr_level);
  if (address < addr_semi) {
   /* Continue with min-branch */
   ADDRTREE_WALKMIN(addr_semi,addr_level);
   proot = &root->at_min;
  } else {
   /* Continue with max-branch */
   ADDRTREE_WALKMAX(addr_semi,addr_level);
   proot = &root->at_max;
  }
 }
 /*printf("Nothing found for %p at %p %u\n",addr,addr_semi,addr_level);*/
 return NULL;
}

__DECL_END
#endif /* !__addrtree_implementation_defined */
#endif




#endif /* !__KOS_KERNEL_UTIL_ADDRTREE_H__ */

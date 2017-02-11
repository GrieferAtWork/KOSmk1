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
#ifndef __KOS_KERNEL_DEV_SOCKET_ALLOCFRAME_C_INL__
#define __KOS_KERNEL_DEV_SOCKET_ALLOCFRAME_C_INL__ 1

#include <kos/compiler.h>
#include <kos/config.h>
#include <kos/atomic.h>
#include <kos/kernel/task.h>
#include <kos/kernel/sched_yield.h>
#include <kos/kernel/dev/socket.h>
#include <malloc.h>
#include <stdint.h>

__DECL_BEGIN

// The maximum size of the ethernet cache
// SPECIAL VALUES:
//  - 0:        Disable ethernet caching
//  - SIZE_MAX: Unlimited cache size
#define ETHERCACHE_MAXSIZE 1024


//
// --- Implementation ---
//

#if ETHERCACHE_MAXSIZE
union ethercache {
 union  ethercache *ec_next;  /*< [0..1][lock(ethercache_lock)] Next cache entry. */
 struct ketherframe ec_frame; /*< Frame of this cache slot. */
};

static union  ethercache *ethercache_list = NULL;
#if ETHERCACHE_MAXSIZE != SIZE_MAX
static             size_t ethercache_size = 0;
#endif
static __atomic int ethercache_lock = 0;
#define ETHERCACHE_ACQUIRE  KTASK_SPIN(katomic_cmpxch(ethercache_lock,0,1));
#define ETHERCACHE_RELEASE  ethercache_lock = 0;


__crit struct ketherframe *ketherframe_alloc(void) {
 struct ketherframe *result;
 KTASK_CRIT_MARK
 ETHERCACHE_ACQUIRE
#if ETHERCACHE_MAXSIZE != SIZE_MAX
 assert((ethercache_size != 0) == (ethercache_list != NULL));
#endif
 if (ethercache_list) {
  union ethercache *next = ethercache_list->ec_next;
  result = &ethercache_list->ec_frame;
  ethercache_list = next;
#if ETHERCACHE_MAXSIZE != SIZE_MAX
  --ethercache_size;
#endif
  ETHERCACHE_RELEASE
  return result;
 }
 ETHERCACHE_RELEASE
 return omalloc(struct ketherframe);
}
__crit void ketherframe_free(struct ketherframe *ob) {
 union ethercache *cache;
 KTASK_CRIT_MARK
 cache = (union ethercache *)ob;
 ETHERCACHE_ACQUIRE
#if ETHERCACHE_MAXSIZE != SIZE_MAX
 if (ethercache_size == SIZE_MAX) {
  ETHERCACHE_RELEASE
  free(ob);
  return;
 }
 ++ethercache_size;
#endif
 cache->ec_next = ethercache_list;
 ethercache_list = cache;
 ETHERCACHE_RELEASE
}
__crit void ketherframe_clearcache(void) {
 union ethercache *iter,*next;
 KTASK_CRIT_MARK
 ETHERCACHE_ACQUIRE
 iter = ethercache_list;
 ethercache_list = NULL;
#if ETHERCACHE_MAXSIZE != SIZE_MAX
 ethercache_size = 0;
#endif
 ETHERCACHE_RELEASE
 while (iter) {
  next = iter->ec_next;
  free(iter);
  iter = next;
 }
}
#else /* ETHERCACHE_MAXSIZE */
__crit struct ketherframe *ketherframe_alloc(void) { return omalloc(struct ketherframe); }
__crit void ketherframe_free(struct ketherframe *ob) { free(ob); }
__crit void ketherframe_clearcache(void) { }
#endif /* !ETHERCACHE_MAXSIZE */

__DECL_END

#endif /* !__KOS_KERNEL_DEV_SOCKET_ALLOCFRAME_C_INL__ */

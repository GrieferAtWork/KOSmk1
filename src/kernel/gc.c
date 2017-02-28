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
#ifndef __KOS_KERNEL_GC_C__
#define __KOS_KERNEL_GC_C__ 1

#include <kos/config.h>
#include <kos/errno.h>
#include <kos/kernel/gc.h>
#include <kos/kernel/mutex.h>
#include <kos/kernel/task.h>

__DECL_BEGIN

/* TODO: We should probably use a recursive mutex for this... */
static struct kmutex   gc_lock  = KMUTEX_INIT;
static struct kgchead *gc_chain = NULL;
static struct kgchead *gc_current = NULL; /* Used when running the GC (current gc object) */

__crit size_t gc_run(void) {
 struct kgchead *iter;
 size_t result = 0;
 KTASK_CRIT_MARK
 if __unlikely(KE_ISERR(kmutex_lock(&gc_lock))) return 0;
 /* Re-use the global GC iterator to prevent recursive gc-runs. */
 if (!gc_current) {
  gc_current = gc_chain;
  while ((iter = gc_current) != NULL) {
   kassertbyte(iter->gc_call);
   /* Execute */
   (*iter->gc_call)(iter);
   ++result;
   if (gc_current == iter) {
    /* If this gc head wasn't untracked, manually go to the next one. */
    gc_current = iter->gc_next;
   }
  }
 }
 kmutex_unlock(&gc_lock);
 return result;
}

__crit void gc_track(struct kgchead *__restrict head) {
 KTASK_CRIT_MARK
 kassert_kgchead(head);
 if __unlikely(head->gc_next) return;
 if __unlikely(KE_ISERR(kmutex_lock(&gc_lock))) return;
 __compiler_barrier();
 if __likely(!head->gc_next && head != gc_chain) {
  /* Begin tracking the head... */
  assert(!head->gc_prev);
  if ((head->gc_next = gc_chain) != NULL) gc_chain->gc_prev = head;
  gc_chain = head;
 }
 kmutex_unlock(&gc_lock);
}

__crit void gc_untrack(struct kgchead *__restrict head) {
 KTASK_CRIT_MARK
 kassert_kgchead(head);
 if __unlikely(KE_ISERR(kmutex_lock(&gc_lock))) return;
 /* If this is the GC object that's currently being collected,
  * we must advance the GC iterator to allow the caller safe
  * deallocation of this header's container. */
 if __unlikely(head == gc_current) gc_current = head->gc_next;
 if __likely(head->gc_prev) {
  assert(head != gc_chain);
  head->gc_prev->gc_next = head->gc_next;
 } else if (head == gc_chain) {
  gc_chain = head->gc_next;
 }
 if (head->gc_next) head->gc_next->gc_prev = head->gc_prev;
 head->gc_prev = NULL;
 head->gc_next = NULL;
 kmutex_unlock(&gc_lock);
}


__DECL_END

#endif /* !__KOS_KERNEL_GC_C__ */

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
#ifndef __KOS_KERNEL_GC_H__
#define __KOS_KERNEL_GC_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <stddef.h>
#include <kos/compiler.h>
#include <kos/errno.h>
#include <kos/kernel/types.h>
#include <kos/kernel/object.h>

__DECL_BEGIN

struct kgchead;

//////////////////////////////////////////////////////////////////////////
// WARNING: This is not ~really~ a garbage collector as you might think of
//          one, but instead a system for easily declaring callbacks for
//          clearing caches when available memory becomes spare.

typedef void (*pgc_callback)(struct kgchead *__restrict head);

#define KOBJECT_MAGIC_GCHEAD  0x6C3EAD /*< GCHEAD. */
#define kassert_kgchead(self) kassert_object(self,KOBJECT_MAGIC_GCHEAD)

struct kgchead {
 KOBJECT_HEAD
 struct kgchead *gc_prev; /*< [lock(INTERNAL)] Previous GC entry. */
 struct kgchead *gc_next; /*< [lock(INTERNAL)] Next GC entry. */
 pgc_callback    gc_call; /*< [1..1][const] Callback to execute during GC. */
};
#define KGCHEAD_INIT(callback) \
 {KOBJECT_INIT(KOBJECT_MAGIC_GCHEAD) NULL,NULL,callback}
#define kgchead_init(self,callback) \
 (void)(kobject_init(self,KOBJECT_MAGIC_GCHEAD)\
       ,(self)->gc_prev = NULL\
       ,(self)->gc_next = NULL\
       ,(self)->gc_call = (callback))
#define kgchead_quit   gc_untrack

//////////////////////////////////////////////////////////////////////////
// Run the garbage collector
// @return: * : Amount of executed callbacks.
//     WARNING: A non-zero return does not necessarily indicate
//              that we were actually able to allocate more memory.
extern __crit __size_t gc_run(void);

//////////////////////////////////////////////////////////////////////////
// Track/Untack a given GC head.
// NOTE: The head must have previously been initialized,
//       but these behave as no-op if the head was already
//       tracked/untracked.
extern __crit __nonnull((1)) void gc_track(struct kgchead *__restrict head);
extern __crit __nonnull((1)) void gc_untrack(struct kgchead *__restrict head);

#define GC_TRACK(self)     gc_track(&(self)->__gc_head)
#define GC_UNTRACK(self) gc_untrack(&(self)->__gc_head)

#define GC_HEAD         struct kgchead __gc_head;
#define GC_SELF(T,head) ((T *)((__uintptr_t)(head)-offsetof(T,__gc_head)))


/* TODO */
// extern void *gmalloc(size_t s);
// extern void *gcalloc(size_t c, size_t s);
// extern void *grealloc(void *p, size_t s);
// extern void  gfree(void *p);


__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_GC_H__ */

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
#ifndef __GOS_KERNEL_KTASK_CHILD_C_INL__
#define __GOS_KERNEL_KTASK_CHILD_C_INL__ 1

#include <gos/compiler.h>
#include <gos/kernel/ktask.h>
#include <sys/types.h>

__DECL_BEGIN


ktask_t *__ktask_child_alloc(ktask_t *self) {
 struct __ktasklistcache *newcache;
 struct __ktasklistcache **newlist;
 ktask_t *result,*iter,*end; pid_t next_pid;
 size_t newlistsize;
 result = self->t_children.tl_free;
 if (result) {
  self->t_children.tl_free = result->t_free.tf_next;
  return result;
 }
 // Must allocate a new cache entry
 // NOTE: We allocate the cache entires as ZERO-initialized memory!
 newcache = ocalloc(struct __ktasklistcache);
 if __unlikely(!newcache) return NULL;
 newlistsize = self->t_children.tl_cachec+1;
 newlist = (struct __ktasklistcache **)realloc(self->t_children.tl_cachev,
                                               newlistsize*sizeof(struct __ktasklistcache *));
 if __unlikely(!newlist) { free(newcache); return NULL; }
 next_pid = self->t_children.tl_cachec*__KTASKLIST_CACHESIZE;
 self->t_children.tl_cachec = newlistsize;
 self->t_children.tl_cachev = newlist;
 end = (iter = newcache->tle_tasks)+__KTASKLIST_CACHESIZE;
 // Initialize the newly allocated cache, marking all overflowing entries as free
 result = iter++;
 result->t_pid = next_pid++;
 result->t_parent = self;
#if __KTASKLIST_CACHESIZE != 1
 while (iter != end) {
  iter->t_pid = next_pid++;
  iter->t_parent = self;
  iter->t_free.tf_next = self->t_children.tl_free;
  self->t_children.tl_free = iter;
  ++iter;
 }
#endif
 return result;
}


void __ktask_child_free(ktask_t *self, ktask_t *child) {
 memset(child,0,sizeof(ktask_t));
 child->t_free.tf_next = self->t_children.tl_free;
 self->t_children.tl_free = child;
 // v already done by memset
 //child->t_pd = NULL; // Sign of a free task

 // TODO: We can try to free up some memory here
}

__DECL_END

#endif /* !__GOS_KERNEL_KTASK_CHILD_C_INL__ */

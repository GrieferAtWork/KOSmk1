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
#ifndef __GOS_KERNEL_KTASK_TID_C_INL__
#define __GOS_KERNEL_KTASK_TID_C_INL__ 1

#include <assert.h>
#include <sys/types.h>
#include <gos/compiler.h>
#include <gos/kernel/katomic.h>
#include <gos/kernel/ktask.h>

__DECL_BEGIN


kerrno_t ktask_getchild(ktask_t const *self, __pid_t pid, ktask_t **child) {
 struct __ktasklistcache *cache_entry; ktask_flag_t taskflags;
 size_t cache_id; ktask_t *result; kerrno_t error;
 assert(!__ktask_isdetached(self));
 cache_id = pid/__KTASKLIST_CACHESIZE;
restart:
 _ktask_lock((ktask_t *)self);
 if __unlikely(cache_id >= self->t_children.tl_cachec) {
err_inval:
  error = KE_INVAL;
err:
  _ktask_unlock((ktask_t *)self);
  return error;
 }
 cache_entry = self->t_children.tl_cachev[cache_id];
 assert(cache_entry != NULL);
 result = &cache_entry->tle_tasks[pid % __KTASKLIST_CACHESIZE];
 if __unlikely(__ktask_isfree(result)) goto err_inval; // Freed task
 taskflags = katomic_load(result->t_flags);
 if __unlikely((taskflags&KTASK_FLAG_TERMINATED)!=0) { error = KE_EXITED; goto err; }
 if __unlikely((taskflags&__KTASK_FLAG_DETACHED)!=0) { error = KE_RUNNING; goto err; }
 error = _ktask_trylock(result);
 _ktask_unlock((ktask_t *)self);
 if __unlikely(!error) { ktask_yield(); goto restart; } // Failed to lock the child --> Start over
 *child = result;
 return KE_OK;
}

__DECL_END

#endif /* !__GOS_KERNEL_KTASK_TID_C_INL__ */

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
#ifndef __KOS_KERNEL_LINKER_RECENT_C_INL__
#define __KOS_KERNEL_LINKER_RECENT_C_INL__ 1

#include <assert.h>
#include <kos/kernel/linker.h>
#include <kos/kernel/spinlock.h>
#include <kos/kernel/task.h>

__DECL_BEGIN

#if KSHLIB_RECENT_CACHE_SIZE
static __ref struct kshlib *recent_libs[KSHLIB_RECENT_CACHE_SIZE] = {NULL,};

/* Position of the where the next recent library is placed. */
static struct kshlib **recent_libs_rotate = recent_libs;

static struct kspinlock recent_libs_lock = KSPINLOCK_INIT;
#define RECENT_ACQUIRE  kspinlock_lock(&recent_libs_lock);
#define RECENT_RELEASE  kspinlock_unlock(&recent_libs_lock);

__crit void kshlibrecent_add(struct kshlib *lib) {
 struct kshlib *old_recent;
 KTASK_CRIT_MARK
 kassert_kshlib(lib);
 /* Create the reference that will live in the recent cache. */
 if __unlikely(KE_ISERR(kshlib_incref(lib))) return;
 RECENT_ACQUIRE
 assert(recent_libs_rotate >= recent_libs &&
        recent_libs_rotate < recent_libs+KSHLIB_RECENT_CACHE_SIZE);
 /* Overwrite an existing recent library and advance the cache R/W pointer. */
 old_recent = *recent_libs_rotate;
 *recent_libs_rotate++ = lib;
 if __unlikely(recent_libs_rotate == recent_libs+KSHLIB_RECENT_CACHE_SIZE) {
  recent_libs_rotate = recent_libs;
 }
 RECENT_RELEASE
 /* Drop a reference to an existing recent lib. */
 if (old_recent) {
  kshlib_decref(old_recent);
 }
}

__crit void kshlibrecent_clear(void) {
 __ref struct kshlib *recent_copy[KSHLIB_RECENT_CACHE_SIZE];
 struct kshlib **iter,**end;
 KTASK_CRIT_MARK
 RECENT_ACQUIRE
 /* Extract the recent list of libraries and reset it. */
 memcpy(recent_copy,recent_libs,sizeof(recent_libs));
 memset(recent_libs,0,sizeof(recent_libs));
 recent_libs_rotate = recent_libs;
 RECENT_RELEASE
 /* Drop stored references from all recent libraries. */
 end = (iter = recent_copy)+KSHLIB_RECENT_CACHE_SIZE;
 for (; iter != end; ++iter) if (*iter) {
  kshlib_decref(*iter);
 }
}

#endif /* KSHLIB_RECENT_CACHE_SIZE */

__DECL_END

#endif /* !__KOS_KERNEL_LINKER_RECENT_C_INL__ */

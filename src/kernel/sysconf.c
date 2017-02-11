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
#ifndef __KOS_KERNEL_SYSCONF_C__
#define __KOS_KERNEL_SYSCONF_C__ 1

#include <kos/sysconf.h>
#include <kos/errno.h>
#include <kos/kernel/paging.h>
#include <kos/kernel/proc.h>
#include <kos/kernel/task.h>
#include <kos/kernel/pageframe.h>

__DECL_BEGIN

static __size_t __get_argmax(void) {
 __size_t result; struct kproc *proc = kproc_self();
 NOIRQ_BEGINLOCK(kproc_trylock(proc,KPROC_LOCK_ENVIRON));
 result = proc->p_environ.pe_memmax;
 NOIRQ_ENDUNLOCK(kproc_unlock(proc,KPROC_LOCK_ENVIRON));
 return result;
}

static unsigned int __get_openmax(void) {
 unsigned int result; struct kproc *proc = kproc_self();
 NOIRQ_BEGINLOCK(kproc_trylock(proc,KPROC_LOCK_FDMAN));
 result = proc->p_fdman.fdm_max;
 NOIRQ_ENDUNLOCK(kproc_unlock(proc,KPROC_LOCK_FDMAN));
 return result;
}
static size_t __get_avphyspages(void) {
 struct kpageframeinfo info;
 kpageframe_getinfo(&info);
 return info.pfi_freepages;
}


extern long k_sysconf(int name);
long k_sysconf(int name) {
 switch (name) {
  case _SC_ARG_MAX         : return __get_argmax();
  case _SC_OPEN_MAX        : return __get_openmax();
  case _SC_NPROCESSORS_CONF: return 1; // Not implemented (yet)
  case _SC_NPROCESSORS_ONLN: return 1; // Not implemented (yet)
  case _SC_PAGE_SIZE       : return PAGESIZE;
  case _SC_AVPHYS_PAGES    : return __get_avphyspages();
  default: break;
 }
 return KE_INVAL;
}

__DECL_END

#endif /* !__KOS_KERNEL_SYSCONF_C__ */

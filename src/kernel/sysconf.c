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
#include <limits.h>

__DECL_BEGIN


static size_t __get_argmax(void) {
 size_t result; struct kproc *proc = kproc_self();
 if (!kproc_hasflag(proc,KPERM_FLAG_GETPERM)) return 0;
 NOIRQ_BEGINLOCK(kproc_trylock(proc,KPROC_LOCK_ENVIRON));
 result = proc->p_environ.pe_memmax;
 NOIRQ_ENDUNLOCK(kproc_unlock(proc,KPROC_LOCK_ENVIRON));
 return result;
}

static unsigned int __get_openmax(void) {
 unsigned int result; struct kproc *proc = kproc_self();
 if (!kproc_hasflag(proc,KPERM_FLAG_GETPERM)) return 0;
 NOIRQ_BEGINLOCK(kproc_trylock(proc,KPROC_LOCK_FDMAN));
 result = proc->p_fdman.fdm_max;
 NOIRQ_ENDUNLOCK(kproc_unlock(proc,KPROC_LOCK_FDMAN));
 return result;
}
static size_t __get_thrdmax(void) {
 struct kproc *proc = kproc_self();
 if (!kproc_hasflag(proc,KPERM_FLAG_GETPERM)) return 0;
 return kprocperm_getthrdmax(kproc_getperm(proc));
}
#if KCONFIG_HAVE_PAGEFRAME_COUNT_ALLOCATED
static size_t __get_totalphyspages(void) {
 struct kpageframeinfo info;
 kpageframe_getinfo(&info);
 /* xxx: Should we include pages reserved for the kernel binary? */
 return info.pfi_freepages+info.pfi_usedpages;
}
#else /* KCONFIG_HAVE_PAGEFRAME_COUNT_ALLOCATED */
#define __get_totalphyspages()  (((size_t)-1)/PAGESIZE)
#endif /* KCONFIG_HAVE_PAGEFRAME_COUNT_ALLOCATED */
static size_t __get_availphyspages(void) {
 struct kpageframeinfo info;
 /* xxx: What about memory restrictions in the calling process? */
 kpageframe_getinfo(&info);
 return info.pfi_freepages;
}

extern long k_sysconf(int name);
long k_sysconf(int name) {
 switch (name) {
  case _SC_ARG_MAX           : return __get_argmax();
  case _SC_STREAM_MAX        : /* xxx: what exactly is this? */
  case _SC_OPEN_MAX          : return __get_openmax();
  case _SC_THREAD_THREADS_MAX: return __get_thrdmax();
  case _SC_TTY_NAME_MAX      : return 11; /*< Suitable for up to 999 PTYs: "/dev/tty123" */
  case _SC_NPROCESSORS_CONF  : return 1; /*< Not implemented (yet) */
  case _SC_NPROCESSORS_ONLN  : return 1; /*< Not implemented (yet) */
  case _SC_PAGE_SIZE         : return PAGESIZE;
  case _SC_PHYS_PAGES        : return __get_totalphyspages();
  case _SC_AVPHYS_PAGES      : return __get_availphyspages();
  /* ~sigh~ (look away from this nonsense...) */
  case _SC_CHAR_BIT          : return CHAR_BIT;
  case _SC_CHAR_MAX          : return CHAR_MAX;
  case _SC_CHAR_MIN          : return CHAR_MIN;
  case _SC_INT_MAX           : return INT_MAX;
  case _SC_INT_MIN           : return INT_MIN;
  case _SC_LONG_BIT          : return __SIZEOF_LONG__*8;
  case _SC_WORD_BIT          : return __SIZEOF_SHORT__*8;
  case _SC_NZERO             : return KTASK_PRIORITY_DEF;
  case _SC_SSIZE_MAX         : return SSIZE_MAX;
  case _SC_SCHAR_MAX         : return SCHAR_MAX;
  case _SC_SCHAR_MIN         : return SCHAR_MIN;
  case _SC_SHRT_MAX          : return SHRT_MAX;
  case _SC_SHRT_MIN          : return SHRT_MIN;
  case _SC_UCHAR_MAX         : return UCHAR_MAX;
  case _SC_UINT_MAX          : return UINT_MAX;
  case _SC_ULONG_MAX         : return ULONG_MAX;
  case _SC_USHRT_MAX         : return USHRT_MAX;
  default: break;
 }
 return KE_INVAL;
}

__DECL_END


#include <kos/kernel/syscall.h>
__DECL_BEGIN

KSYSCALL_DEFINE1(long,k_sysconf,int,name) {
 return k_sysconf(name);
}

__DECL_END

#endif /* !__KOS_KERNEL_SYSCONF_C__ */

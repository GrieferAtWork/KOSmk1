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
#ifndef __KOS_KERNEL_SYSCALL_SYSCALL_KMEM_C_INL__
#define __KOS_KERNEL_SYSCALL_SYSCALL_KMEM_C_INL__ 1

#include "syscall-common.h"
#include <kos/errno.h>
#include <kos/kernel/pageframe.h>
#include <kos/kernel/paging.h>
#include <kos/kernel/shm.h>
#include <kos/kernel/util/string.h>
#include <kos/mem.h>
#include <kos/syscallno.h>
#include <math.h>
#include <sys/mman.h>
#include <kos/syslog.h>

__DECL_BEGIN

/*< _syscall6(void *,kmem_map,void *,hint,size_t,length,int,prot,int,flags,int,fd,__u64,offset); */
SYSCALL(sys_kmem_map) {
#ifdef KFD_HAVE_BIARG_POSITIONS
 LOAD7(void *,K(hint),
       size_t,K(length),
       int   ,K(prot),
       int   ,K(flags),
       int   ,K(fd),
       __u64 ,D(offset),
       void  ,N);
#else
 LOAD6(void *,K(hint),
       size_t,K(length),
       int   ,K(prot),
       int   ,K(flags),
       int   ,K(fd),
       __u64 ,K(offset));
#endif
 struct kproc *procself = kproc_self();
 __ref struct kshmregion *region; size_t pages; kerrno_t error;
 (void)fd,(void)offset; /* TODO? */
 /* Fix the given hint. */
 if (!hint) hint = KPAGEDIR_MAPANY_HINT_UHEAP;
 else hint = (void *)alignd((uintptr_t)hint,PAGEALIGN);
 prot &= (KSHMREGION_FLAG_EXEC|KSHMREGION_FLAG_READ|KSHMREGION_FLAG_WRITE); /*< Don't reveal the hidden flags. */
 if (flags&MAP_SHARED) prot |= KSHMREGION_FLAG_SHARED;
 if (flags&_MAP_LOOSE) prot |= KSHMREGION_FLAG_LOSEONFORK;
 /* Calculate the min amount of pages. */
 pages = ceildiv(length,PAGESIZE);
 /* Make sure we're always allocating something. */
 if __unlikely(!pages) pages = 1;
 KTASK_CRIT_BEGIN_FIRST
 error = kproc_lock(procself,KPROC_LOCK_SHM);
 if __unlikely(KE_ISERR(error)) { hint = (void *)(uintptr_t)-1; goto end; }
 if (!(flags&MAP_FIXED)) {
  /* For non-fixed mappings, find a suitable free range. */
  hint = kpagedir_findfreerange(kproc_getpagedir(procself),pages,hint);
  if __unlikely(!hint) goto err_unlock;
 }
 region = kshmregion_newram(pages,prot);
 if __unlikely(!region) goto err_unlock;
 error = kshm_mapfullregion_inherited(kproc_getshm(procself),hint,region);
 if __unlikely(KE_ISERR(error)) { kshmregion_decref_full(region); goto err_unlock; }
end_unlock:
 kproc_unlock(procself,KPROC_LOCK_SHM);
end: KTASK_CRIT_END
 RETURN(hint);
err_unlock:
 hint = (void *)(uintptr_t)-1;
 goto end_unlock;
}

/*< _syscall5(kerrno_t,kmem_mapdev,void **,hint_and_result,__size_t,length,int,prot,int,flags,void *,physptr); */
SYSCALL(sys_kmem_mapdev) {
 LOAD5(void  **,K(hint_and_result),
       __size_t,K(length),
       int     ,K(prot),
       int     ,K(flags),
       void   *,K(physptr));
 void *hint,*aligned_physptr,*result;
 size_t alignment_offset;
 struct ktask *caller = ktask_self();
 struct kproc *procself = ktask_getproc(caller);
 __ref struct kshmregion *region; size_t pages; kerrno_t error;
 /* Fix the given hint. */
 aligned_physptr  = (void *)alignd((uintptr_t)physptr,PAGEALIGN);
 alignment_offset = ((uintptr_t)physptr-(uintptr_t)aligned_physptr);
 length          += alignment_offset;
 prot            &= (KSHMREGION_FLAG_EXEC|KSHMREGION_FLAG_READ|KSHMREGION_FLAG_WRITE); /*< Don't reveal the hidden flags. */
 if (flags&MAP_SHARED) prot |= KSHMREGION_FLAG_SHARED;
 if (flags&_MAP_LOOSE) prot |= KSHMREGION_FLAG_LOSEONFORK;
 if __unlikely(copy_from_user(&hint,hint_and_result,sizeof(hint))) RETURN(KE_FAULT);
 if (flags&MAP_FIXED) {
  void *aligned_hint;
  aligned_hint    = (void *)alignd((uintptr_t)hint,PAGEALIGN);
  if (((uintptr_t)aligned_hint-(uintptr_t)hint) != alignment_offset
      ) RETURN(KE_INVAL); /*< Impossible alignment requirements. */
  hint = aligned_hint;
 } else if (!hint) {
  hint = KPAGEDIR_MAPANY_HINT_UDEV;
 }
 /* Calculate the min amount of pages. */
 pages = ceildiv(length,PAGESIZE);
 /* Make sure we're always allocating something. */
 if __unlikely(!pages) pages = 1;
 /* Make sure that the specified area of physical memory doesn't overflow. */
 if __unlikely(((uintptr_t)aligned_physptr+pages*PAGESIZE) <=
                (uintptr_t)aligned_physptr) RETURN(KE_OVERFLOW);
 KTASK_CRIT_BEGIN_FIRST
 /* Check if the calling process has access to this physical  */
 error = kshm_devaccess(caller,aligned_physptr,pages);
 if __unlikely(KE_ISERR(error)) goto end;
 error = kproc_lock(procself,KPROC_LOCK_SHM);
 if __unlikely(KE_ISERR(error)) goto end;
 if (!(flags&MAP_FIXED)) {
  /* For non-fixed mappings, find a suitable free range. */
  hint = kpagedir_findfreerange(kproc_getpagedir(procself),pages,hint);
  if __unlikely(!hint) { error = KE_NOSPC; goto end_unlock; }
 }
 result = (void *)((uintptr_t)hint+alignment_offset);
 if __unlikely(kshm_copytouser(kproc_getshm(procself),
                               hint_and_result,
                               &result,sizeof(void *))) {
  error = KE_FAULT;
  goto end_unlock;
 }
 k_syslogf(KLOG_DEBUG,"Creating user-requested physical mapping %p -> %p (%Iu pages)\n",
           aligned_physptr,hint,pages);
 region = kshmregion_newphys(aligned_physptr,pages,
                             prot|KSHMREGION_FLAG_NOCOPY);
 if __unlikely(!region) { error = KE_NOMEM; goto end_unlock; }
 /* Map the new region of memory within the calling process. */
 error = kshm_mapfullregion_inherited(kproc_getshm(procself),hint,region);
 if __unlikely(KE_ISERR(error)) kshmregion_decref_full(region);

end_unlock:
 kproc_unlock(procself,KPROC_LOCK_SHM);
end: KTASK_CRIT_END
 RETURN(error);
}


/*< _syscall2(kerrno_t,kmem_unmap,void *,addr,size_t,length); */
SYSCALL(sys_kmem_unmap) {
 LOAD2(void *,K(addr),
       size_t,K(length));
 kerrno_t error;
 uintptr_t aligned_start,aligned_length;
 struct kproc *procself = kproc_self();
 aligned_start = alignd((uintptr_t)addr,PAGEALIGN);
 aligned_length = length+((uintptr_t)addr-aligned_start);
 KTASK_CRIT_BEGIN_FIRST
 error = kproc_lock(procself,KPROC_LOCK_SHM);
 if __unlikely(KE_ISERR(error)) goto end;
 /* REMINDER: Don't allow unmapping of kernel SHM branches. */
 error = kshm_unmap(kproc_getshm(procself),
                   (__user void *)aligned_start,
                    ceildiv(aligned_length,PAGESIZE),
                    KSHMUNMAP_FLAG_NONE);
 kproc_unlock(procself,KPROC_LOCK_SHM);
end:
 KTASK_CRIT_END
 RETURN(error);
}


/*< _syscall2(kerrno_t,kmem_validate,void *__restrict,addr,__size_t,bytes); */
SYSCALL(sys_kmem_validate) {
 LOAD2(void *,K(addr),
       size_t,K(bytes));
 kerrno_t error;
 struct kproc *procself = kproc_self();
 KTASK_CRIT_BEGIN_FIRST
 error = kproc_lock(procself,KPROC_LOCK_SHM);
 if __likely(KE_ISOK(error)) {
  void *pageaddr = (void *)alignd((uintptr_t)addr,PAGEALIGN);
  size_t alignedsize = ((uintptr_t)addr-(uintptr_t)pageaddr)+bytes;
  //printf("CHECKING: %p + %Iu (%p + %Iu) (%Iu pages)\n",
  //       pageaddr,alignedsize,addr,bytes,
  //       ceildiv(alignedsize,PAGESIZE));
  if __unlikely(!kpagedir_ismapped(kproc_getpagedir(procself),pageaddr,ceildiv(alignedsize,PAGESIZE))) {
   error = KE_FAULT;
  } else {
   assert(ceildiv(alignedsize,PAGESIZE) < 2 ||
          kpagedir_ismapped(kproc_getpagedir(procself),(void *)((uintptr_t)pageaddr+PAGESIZE),1));
   addr = kpagedir_translate(kproc_getpagedir(procself),addr);
   error = kmem_validate(addr,bytes);
  }
  kproc_unlock(procself,KPROC_LOCK_SHM);
 }
 KTASK_CRIT_END
 RETURN(error);
}


__DECL_END

#endif /* !__KOS_KERNEL_SYSCALL_SYSCALL_KMEM_C_INL__ */

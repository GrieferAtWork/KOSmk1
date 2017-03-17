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
#ifndef __KOS_KERNEL_SHM_SYSCALL_C_INL__
#define __KOS_KERNEL_SHM_SYSCALL_C_INL__ 1

#include <kos/compiler.h>
#include <sys/mman.h>
#include <kos/config.h>
#include <kos/syslog.h>
#include <kos/kernel/syscall.h>
#include <kos/kernel/shm.h>
#include <kos/kernel/proc.h>

__DECL_BEGIN

/*< _syscall6(void *,kmem_map,void *,hint,size_t,length,int,prot,int,flags,int,fd,__u64,offset); */
#ifdef KFD_HAVE_BIARG_POSITIONS
#if (__BYTE_ORDER == __LITTLE_ENDIAN) == defined(KOS_ARCH_STACK_GROWS_DOWNWARDS)
KSYSCALL_DEFINE7(__user void *,kmem_map,__user void *,hint,size_t,length,
                 int,prot,int,flags,int,fd,__u32,offhi,__u32,offlo)
#else /* linear: down */
KSYSCALL_DEFINE7(__user void *,kmem_map,__user void *,hint,size_t,length,
                 int,prot,int,flags,int,fd,__u32,offlo,__u32,offhi)
#endif /* linear: up */
#else /* KFD_HAVE_BIARG_POSITIONS */
KSYSCALL_DEFINE6(__user void *,kmem_map,__user void *,hint,size_t,length,
                 int,prot,int,flags,int,fd,__u64,offset)
#endif /* !KFD_HAVE_BIARG_POSITIONS */
{
 struct kproc *procself = kproc_self();
 __ref struct kshmregion *region; size_t pages; kerrno_t error;
 (void)fd;
#ifdef KFD_HAVE_BIARG_POSITIONS
 (void)offlo,(void)offhi; /* TODO? */
#else
 (void)offset; /* TODO? */
#endif
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
 error = krwlock_beginwrite(&procself->p_shm.s_lock);
 if __unlikely(KE_ISERR(error)) { hint = (void *)(uintptr_t)-1; goto end; }
 if (!(flags&MAP_FIXED)) {
  /* For non-fixed mappings, find a suitable free range. */
  hint = kpagedir_findfreerange(kproc_getpagedir(procself),pages,hint);
  if __unlikely(hint == KPAGEDIR_FINDFREERANGE_ERR) goto err_unlock;
 }
 region = kshmregion_newram(pages,prot);
 if __unlikely(!region) goto err_unlock;
 error = kshm_mapfullregion_inherited_unlocked(kproc_getshm(procself),hint,region);
 if __unlikely(KE_ISERR(error)) { kshmregion_decref_full(region); goto err_unlock; }
end_unlock:
 krwlock_endwrite(&procself->p_shm.s_lock);
end: KTASK_CRIT_END
 return hint;
err_unlock:
 hint = (void *)(uintptr_t)-1;
 goto end_unlock;
}

KSYSCALL_DEFINE5(kerrno_t,kmem_mapdev,__user void **,hint_and_result,
                 __size_t,length,int,prot,int,flags,__kernel void *,physptr) {
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
 if __unlikely(copy_from_user(&hint,hint_and_result,sizeof(hint))) return KE_FAULT;
 if (flags&MAP_FIXED) {
  void *aligned_hint;
  aligned_hint    = (void *)alignd((uintptr_t)hint,PAGEALIGN);
  if (((uintptr_t)aligned_hint-(uintptr_t)hint) != alignment_offset
      ) return KE_INVAL; /*< Impossible alignment requirements. */
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
                (uintptr_t)aligned_physptr) return KE_OVERFLOW;
 KTASK_CRIT_BEGIN_FIRST
 /* Check if the calling process has access to this physical  */
 error = kshm_devaccess(caller,aligned_physptr,pages);
 if __unlikely(KE_ISERR(error)) goto end;
 error = krwlock_beginwrite(&procself->p_shm.s_lock);
 if __unlikely(KE_ISERR(error)) goto end;
 if (!(flags&MAP_FIXED)) {
  /* For non-fixed mappings, find a suitable free range. */
  hint = kpagedir_findfreerange(kproc_getpagedir(procself),pages,hint);
  if __unlikely(hint == KPAGEDIR_FINDFREERANGE_ERR) { error = KE_NOSPC; goto end_unlock; }
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
 error = kshm_mapfullregion_inherited_unlocked(kproc_getshm(procself),hint,region);
 if __unlikely(KE_ISERR(error)) kshmregion_decref_full(region);

end_unlock:
 krwlock_endwrite(&procself->p_shm.s_lock);
end: KTASK_CRIT_END
 return error;
}


KSYSCALL_DEFINE2(kerrno_t,kmem_unmap,void *,addr,size_t,length) {
 kerrno_t error;
 uintptr_t aligned_start,aligned_length;
 struct kproc *procself = kproc_self();
 aligned_start = alignd((uintptr_t)addr,PAGEALIGN);
 aligned_length = length+((uintptr_t)addr-aligned_start);
 KTASK_CRIT_BEGIN_FIRST
 error = krwlock_beginwrite(&procself->p_shm.s_lock);
 if __unlikely(KE_ISERR(error)) goto end;
 /* REMINDER: Don't allow unmapping of kernel SHM branches. */
 error = kshm_unmap_unlocked(kproc_getshm(procself),
                            (__user void *)aligned_start,
                             ceildiv(aligned_length,PAGESIZE),
                             KSHMUNMAP_FLAG_NONE);
 krwlock_endwrite(&procself->p_shm.s_lock);
end:
 KTASK_CRIT_END
 return error;
}


KSYSCALL_DEFINE2(kerrno_t,kmem_validate,
                 __user void *__restrict,addr,
                 __size_t,bytes) {
 kerrno_t error;
 struct kproc *procself = kproc_self();
 KTASK_CRIT_BEGIN_FIRST
 error = krwlock_beginread(&procself->p_shm.s_lock);
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
  krwlock_endread(&procself->p_shm.s_lock);
 }
 KTASK_CRIT_END
 return error;
}

__DECL_END

#endif /* !__KOS_KERNEL_SHM_SYSCALL_C_INL__ */

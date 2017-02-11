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
#include <kos/mem.h>
#include <kos/syscallno.h>
#include <math.h>
#include <kos/kernel/util/string.h>

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
 void *result; struct kfile *fp;
 struct kproc *procself = kproc_self();
 if (!hint) hint = KPAGEDIR_MAPANY_HINT_UHEAP;
 else hint = (void *)alignd((uintptr_t)hint,PAGEALIGN);
 prot &= (PROT_READ|PROT_WRITE|PROT_EXEC); /*< Don't reveil the hidden flags. */
 KTASK_CRIT_BEGIN
 fp = (flags&MAP_ANONYMOUS) ? NULL : kproc_getfdfile(procself,fd);
 result = kshm_mmap(&procself->p_shm,hint,length,prot,flags,fp,offset);
 if (fp) kfile_decref(fp);
 KTASK_CRIT_END
 RETURN(result);
}

/*< _syscall5(kerrno_t,kmem_mapdev,void **,hint_and_result,__size_t,length,int,prot,int,flags,void *,physptr); */
SYSCALL(sys_kmem_mapdev) {
 LOAD5(void  **,K(hint_and_result),
       __size_t,K(length),
       int     ,K(prot),
       int     ,K(flags),
       void   *,K(physptr));
 void *result; kerrno_t error;
 struct kproc *procself = kproc_self();
 if __unlikely(u_get(hint_and_result,result)) RETURN(KE_FAULT);
 if (!result) result = KPAGEDIR_MAPANY_HINT_UDEV;
 else result = (void *)alignd((uintptr_t)result,PAGEALIGN);
 prot &= (PROT_READ|PROT_WRITE|PROT_EXEC); /*< Don't reveil the hidden flags. */
 KTASK_CRIT_BEGIN
 error = kshm_mmapdev(&procself->p_shm,&result,length,prot,flags,physptr);
 if (__likely(KE_ISOK(error)) && __unlikely(u_set(hint_and_result,result))) {
  kshm_munmap(&procself->p_shm,result,length,0);
  error = KE_FAULT;
 }
 KTASK_CRIT_END
 RETURN(error);
}


/*< _syscall2(kerrno_t,kmem_unmap,void *,addr,size_t,length); */
SYSCALL(sys_kmem_unmap) {
 LOAD2(void *,K(addr),
       size_t,K(length));
 kerrno_t error;
 struct kproc *procself = kproc_self();
 KTASK_CRIT_BEGIN
 /* Don't allow unmapping the kernel pages. */
 error = kshm_munmap(&procself->p_shm,addr,length,0);
 KTASK_CRIT_END
 RETURN(error);
}


/*< _syscall2(kerrno_t,kmem_validate,void *__restrict,addr,__size_t,bytes); */
SYSCALL(sys_kmem_validate) {
 LOAD2(void *,K(addr),
       size_t,K(bytes));
 kerrno_t error;
 struct kproc *procself = kproc_self();
 KTASK_CRIT_BEGIN
 error = kproc_lock(procself,KPROC_LOCK_SHM);
 if __likely(KE_ISOK(error)) {
  void *pageaddr = (void *)alignd((uintptr_t)addr,PAGEALIGN);
  size_t alignedsize = ((uintptr_t)addr-(uintptr_t)pageaddr)+bytes;
  //printf("CHECKING: %p + %Iu (%p + %Iu) (%Iu pages)\n",
  //       pageaddr,alignedsize,addr,bytes,
  //       ceildiv(alignedsize,PAGESIZE));
  if __unlikely(!kpagedir_ismapped(procself->p_shm.sm_pd,pageaddr,ceildiv(alignedsize,PAGESIZE))) {
   error = KE_FAULT;
  } else {
   assert(ceildiv(alignedsize,PAGESIZE) < 2 ||
          kpagedir_ismapped(procself->p_shm.sm_pd,(void *)((uintptr_t)pageaddr+PAGESIZE),1));
   addr = kpagedir_translate(procself->p_shm.sm_pd,addr);
   error = kmem_validate(addr,bytes);
  }
  kproc_unlock(procself,KPROC_LOCK_SHM);
 }
 KTASK_CRIT_END
 RETURN(error);
}


__DECL_END

#endif /* !__KOS_KERNEL_SYSCALL_SYSCALL_KMEM_C_INL__ */

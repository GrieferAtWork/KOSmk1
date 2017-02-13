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
#ifndef __KOS_KERNEL_SCHED_PROC_PID_C_INL__
#define __KOS_KERNEL_SCHED_PROC_PID_C_INL__ 1

#include <kos/kernel/proc.h>
#include <kos/kernel/mutex.h>
#include <stdint.h>
#include <sys/types.h>

__DECL_BEGIN

#define pl   (*kproclist_global())

#define CHECK_PROCLIST() \
do{\
 struct kproc **iter,**end;\
 end = (iter = pl.pl_procv)+pl.pl_proca;\
 for (; iter != end; ++iter) if (*iter) \
  assertf(kobject_verify(*iter,KOBJECT_MAGIC_PROC)\
         ,"Invalid proclist entry: %Iu/%Iu (%p at %p)"\
         ,iter-pl.pl_procv\
         ,pl.pl_proca,*iter,iter);\
}while(0)

__crit kerrno_t kproclist_addproc(struct kproc *__restrict proc) {
 struct kproc **newvec; __u32 newsize,result;
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kproc(proc);
 error = kmutex_lock(&pl.pl_lock);
 if __unlikely(KE_ISERR(error)) return error;
 CHECK_PROCLIST();
 // Search for a free slot
 newvec = pl.pl_procv;
 for (result = pl.pl_free; result < pl.pl_proca; ++result) {
  if (!newvec[result]) goto found;
 }
 for (result = 0; result < pl.pl_free; ++result) {
  if (!newvec[result]) goto found;
 }
 newsize = pl.pl_proca ? pl.pl_proca*2 : 2;
 // Must allocate a new slot
 newvec = (struct kproc **)realloc(newvec,newsize*sizeof(struct kproc *));
 if __unlikely(!newvec) { error = KE_NOMEM; goto end; }
 memset(newvec+pl.pl_proca,0,(newsize-pl.pl_proca)*
        sizeof(struct kproc *));
 result = pl.pl_proca;
 pl.pl_procv = newvec;
 pl.pl_proca = newsize;
found:
#if PID_MAX != UINT32_MAX
 if __unlikely(result > PID_MAX) {
  pl.pl_free     = result;
  newvec[result] = NULL;
  error               = KE_OVERFLOW;
 } else
#endif
 {
  pl.pl_free     = result+1;
  newvec[result] = proc;
  proc->p_pid    = result;
 }
end:
 CHECK_PROCLIST();
 kmutex_unlock(&pl.pl_lock);
 return error;
}
__crit void kproclist_delproc(struct kproc *__restrict proc) {
 __u32 procpid; kerrno_t lockerror;
 KTASK_CRIT_MARK
 kassert_object(proc,KOBJECT_MAGIC_PROC);
 lockerror = kmutex_lock(&pl.pl_lock);
 CHECK_PROCLIST();
 // Sanity check
 procpid = proc->p_pid;
 if __likely(procpid < pl.pl_proca && pl.pl_procv[procpid] == proc) {
  pl.pl_procv[procpid] = NULL;
  if (procpid == pl.pl_proca-1) {
   // Free up some memory
   while (procpid && !pl.pl_procv[procpid-1]) --procpid;
   if (procpid) {
    struct kproc **newvec;
    newvec = (struct kproc **)realloc(pl.pl_procv,procpid*
                                      sizeof(struct kproc *));
    if __likely(newvec) {
     pl.pl_proca = procpid;
     pl.pl_procv = newvec;
    }
   } else {
    free(pl.pl_procv);
    pl.pl_proca = 0;
    pl.pl_procv = NULL;
   }
  } else {
   // If the current free slot isn't free,
   // set it to what we just freed.
   if (pl.pl_free > pl.pl_proca || pl.pl_procv[pl.pl_free]
       ) pl.pl_free = procpid;
  }
 }
 if __likely(KE_ISOK(lockerror)) kmutex_unlock(&pl.pl_lock);
}


__crit kerrno_t kproclist_close(void) {
 kerrno_t error;
 KTASK_CRIT_MARK
 error = kmutex_close(&pl.pl_lock);
 if (error != KS_UNCHANGED) {
  // Free the vector.
  // NOTE: Any process's attempt at removing itself
  //       after this point will be handled as no-op.
  free(pl.pl_procv);
  pl.pl_proca = 0;
  pl.pl_procv = NULL;
 }
 return error;
}

__crit __ref struct kproc *kproclist_getproc(__u32 pid) {
 __ref struct kproc *result;
 KTASK_CRIT_MARK
 if __likely((result = kproclist_getproc_k(pid)) != NULL) {
  // Make sure the process's root task is visible to the caller.
  if __unlikely(!kproc_isrootvisible_c(result)) {
   kproc_decref(result);
   result = NULL;
  }
 }
 return result;
}

__crit __ref struct kproc *kproclist_getproc_k(__u32 pid) {
 __ref struct kproc *result;
 KTASK_CRIT_MARK
 if __unlikely(KE_ISERR(kmutex_lock(&pl.pl_lock))) return NULL;
 if (pid >= pl.pl_proca) result = NULL;
 else if __likely((result = pl.pl_procv[pid]) != NULL) {
  if __unlikely(KE_ISERR(kproc_tryincref(result))) result = NULL;
 }
 kmutex_unlock(&pl.pl_lock);
 return result;
}

kerrno_t kproclist_enumpid(pid_t *__restrict pidv,
                           size_t pidc, size_t *__restrict reqpidc) {
 __pid_t *iter,*end; kerrno_t error;
 struct kproc **piter,**pend,*proc;
 kassertmem(pidv,pidc*sizeof(pid_t));
 kassertobjnull(reqpidc);
 end = (iter = pidv)+pidc;
 if __likely(KE_ISOK(error = kmutex_lock(&pl.pl_lock))) {
  pend = (piter = pl.pl_procv)+pl.pl_proca;
  while (piter != pend) {
   proc = *piter++;
   if (proc && katomic_load(proc->p_refcnt) &&
       kproc_isrootvisible_c(proc)) {
    if (iter < end) *iter = proc->p_pid;
    ++iter;
   }
  }
  kmutex_unlock(&pl.pl_lock);
  if (reqpidc) *reqpidc = (size_t)(iter-pidv);
 }
 return error;
}

__crit size_t
kproclist_enumproc(__ref struct kproc **__restrict procv, size_t procc) {
 KTASK_CRIT_MARK
 __ref struct kproc **iter,**end;
 struct kproc **piter,**pend,*proc;
 size_t result;
 kassertmem(procv,procc*sizeof(struct kproc *));
 kassertobjnull(reqprocc);
 end = (iter = procv)+procc;
 if __unlikely(KE_ISERR(kmutex_lock(&pl.pl_lock))) return 0;
 pend = (piter = pl.pl_procv)+pl.pl_proca;
 for (; piter != pend; ++piter) {
  if ((proc = *piter) != NULL &&
      katomic_load(proc->p_refcnt) &&
      kproc_isrootvisible_c(proc)) {
   if (iter < end) {
    asserte(katomic_fetchinc(proc->p_refcnt) != 0);
    *iter = proc;
   }
   ++iter;
  }
 }
 kmutex_unlock(&pl.pl_lock);
 result = (size_t)(iter-procv);
 if (result > procc) {
  /* Drop all references already acquired. */
  end = (iter = procv)+procc;
  for (; iter != end; ++iter) kproc_decref(*iter);
 }
 return result;
}



#undef pl

__DECL_END

#endif /* !__KOS_KERNEL_SCHED_PROC_PID_C_INL__ */

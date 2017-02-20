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
#ifndef __KOS_KERNEL_SCHED_TASK_UTIL_C_INL__
#define __KOS_KERNEL_SCHED_TASK_UTIL_C_INL__ 1

#include <assert.h>
#include <kos/compiler.h>
#include <kos/atomic.h>
#include <kos/kernel/debug.h>
#include <kos/kernel/features.h>
#include <kos/kernel/gdt.h>
#include <kos/kernel/interrupts.h>
#include <kos/kernel/task.h>
#include <kos/kernel/proc.h>
#include <kos/kernel/time.h>
#include <kos/kernel/paging.h>
#include <kos/kernel/sched_yield.h>
#include <kos/errno.h>
#include <kos/timespec.h>
#include <kos/types.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <kos/arch.h>
#ifdef __x86__
#include <kos/arch/x86/eflags.h>
#endif

__DECL_BEGIN

__crit __ref struct ktask *
ktask_newctxex_suspended(struct ktask *parent, struct kproc *__restrict ctx,
                         void *(*main)(void *), void *closure,
                         size_t stacksize, __u32 flags) {
__COMPILER_PACK_PUSH(1)
 struct __packed {
  // Mockup of how the task's stack will look like
  struct ktaskregisters regs;
  void (*return_address)(void);
  void  *closure_value;
 } stack;
__COMPILER_PACK_POP
 extern void ktask_exitnormal(void);
 __ref struct ktask *result;
 KTASK_CRIT_MARK
 kassert_ktask(parent);
 kassert_kproc(ctx);
 kassertbyte(main);
 result = ktask_newkernel(parent,ctx,stacksize,flags);
 if __unlikely(!result) return NULL;
 memset(&stack,0,sizeof(stack));
 stack.regs.cs       = KSEG_KERNEL_CODE;
 stack.regs.ds       = KSEG_KERNEL_DATA;
#if KCONFIG_HAVE_I386_SAVE_SEGMENT_REGISTERS
 stack.regs.es       = KSEG_KERNEL_DATA;
 stack.regs.fs       = KSEG_KERNEL_DATA;
 stack.regs.gs       = KSEG_KERNEL_DATA;
#endif
 stack.regs.eflags   = KARCH_X86_EFLAGS_IF;
 stack.regs.callback = main;
#ifdef __DEBUG__
 stack.return_address = (void(*)(void))((uintptr_t)&ktask_exitnormal+1);
#else
 stack.return_address = &ktask_exitnormal;
#endif
 stack.closure_value = closure;
 ktask_stackpush_sp_unlocked(result,&stack,sizeof(stack));
 return result;
}
__crit __ref struct ktask *
ktask_newctxex(struct ktask *parent, struct kproc *__restrict ctx,
               void *(*main)(void *), void *closure, size_t stacksize, __u32 flags) {
 __ref struct ktask *result;
 KTASK_CRIT_MARK
 result = ktask_newctxex_suspended(parent,ctx,main,closure,stacksize,flags);
 if __unlikely(!result) return NULL;
 if __unlikely(KE_ISERR(ktask_resume_k(result))) { ktask_decref(result); result = NULL; }
 return result;
}

__crit __ref struct ktask *
ktask_newex_suspended(void *(*main)(void *), void *closure,
                      size_t stacksize, __u32 flags) {
 struct ktask *result,*parent = ktask_self();
 struct kproc *proc;
 KTASK_CRIT_MARK
 proc = ktask_getproc(parent);
 if __unlikely(KE_ISERR(kproc_lock(proc,KPROC_LOCK_SHM))) return NULL;
 result = ktask_newctxex_suspended(parent,proc,main,closure,stacksize,flags);
 kproc_unlock(proc,KPROC_LOCK_SHM);
 return result;
}
__crit __ref struct ktask *
ktask_newex(void *(*main)(void *), void *closure,
            size_t stacksize, __u32 flags) {
 __ref struct ktask *result;
 KTASK_CRIT_MARK
 result = ktask_newex_suspended(main,closure,stacksize,flags);
 if __unlikely(!result) return NULL;
 if __unlikely(KE_ISERR(ktask_resume_k(result))) { ktask_decref(result); result = NULL; }
 return result;
}


kerrno_t _ktask_terminate_k(struct ktask *__restrict self, void *exitcode) {
 kerrno_t error; kassert_ktask(self);
 error = ktask_unschedule(self,KTASK_STATE_TERMINATED,exitcode);
 // We don't care if the task wasn't unscheduled because it was waiting
 if (error == KS_UNCHANGED) error = KE_OK;
 return error;
}
kerrno_t ktask_terminate_k(struct ktask *__restrict self, void *exitcode) {
 kerrno_t error;
 error = _ktask_terminate_k(self,exitcode);
 if __unlikely(error == KS_BLOCKING) {
  // TODO: We should have special handling here for joining
  //       a freshly terminated, critical, but suspended task.
  //    >> If we don't there might be a deadlock when there's
  //       not another task to resume the one we just terminated.
  assertf(self != ktask_self(),
          "If you want to terminate yourself while being "
          "inside a critical block, use _ktask_terminate_k!\n"
          "That way, you'll be terminated once you leave the block.");
  // Wait for a critical task to terminate
  ktask_join(self,NULL);
  error = KE_OK;
 }
 return error;
}


__local __crit kerrno_t
__ktask_terminate_s(struct ktask *self, void *exitcode,
                    struct ktask *__restrict caller, __u32 flags) {
 kerrno_t error;
 KTASK_CRIT_MARK
 if (flags&KTASKOPFLAG_NOCALLER && self == caller) return KE_OK;
 error = _ktask_terminate_k(self,exitcode);
 if (!(flags&KTASKOPFLAG_ASYNC) &&
      (KE_ISOK(error) || error == KE_DESTROYED) &&
      (self != caller)) ktask_join(self,NULL);
 return error;
}

static __crit kerrno_t
__ktask_terminate_r(struct ktask *__restrict self,
                    struct ktask *__restrict caller,
                    void *exitcode, struct kproc *proc, __u32 flags) {
 kerrno_t error; struct ktask *child;
 size_t i; int found_running;
 KTASK_CRIT_MARK
 // This is really hacky, and might appear like a bad solution,
 // but in actuality it does the job quite well!
 // >> Basically: Recursively terminate all tasks until all
 //    that's left are zombies, or nothing is left all-together.
 ktask_lock(self,KTASK_LOCK_CHILDREN);
 while (self->t_children.t_taska) {
  found_running = 0;
  for (i = 0; i < self->t_children.t_taska; ++i) {
   if ((child = self->t_children.t_taskv[i]) != NULL &&
       (!proc || child->t_proc == proc) &&
       __likely(KE_ISOK(ktask_incref(child)))) {
    // TODO: Error handling when can't incref child
    ktask_unlock(self,KTASK_LOCK_CHILDREN);
    error = __ktask_terminate_r(child,caller,exitcode,proc,flags);
    if (error == KS_BLOCKING) {
     // Join a child task currently executing a critical operation
     if (child != caller) {
      found_running = 1;
      ktask_join(child,NULL);
     }
    } else if (error != KE_DESTROYED) {
     assert(KE_ISOK(error));
     found_running = 1;
    }
    ktask_decref(child);
    ktask_lock(self,KTASK_LOCK_CHILDREN);
   }
  }
  if (!found_running) break;
 }
 // NOTE: Normally, we'd have to be concerned about the following
 //       line of code creating a deadlock, as we are still locking
 //       'KTASK_LOCK_CHILDREN'.
 //       But because 'self' quite simply has to remain alive during
 //       this process (the implicit reference owned by the caller),
 //       we are safe because the only point where a terminating
 //       task attempts to lock 'KTASK_LOCK_CHILDREN', is within
 //       the deallocation destruction, which is only invoked once
 //       the task's reference counter hits ZERO.
 //       Technically speaking, the only situation in which a task
 //       terminate can offer a task that will also be destructed
 //       by the terminator is the ktask_terminate(ktask_self())
 //       scenario, which for us is covered by the caller running
 //       all of this from within a critical block.
 error = __ktask_terminate_s(self,exitcode,caller,flags);
 ktask_unlock(self,KTASK_LOCK_CHILDREN);
 return error;
}

kerrno_t ktask_terminateex_k(struct ktask *__restrict self,
                             void *exitcode, __ktaskopflag_t mode) {
 kerrno_t error;
 struct ktask *caller = ktask_self();
 KTASK_CRIT_BEGIN
 KTASK_NOINTR_BEGIN
 if (mode&KTASKOPFLAG_RECURSIVE) {
  error = __ktask_terminate_r(self,caller,exitcode,
                             ((mode&(KTASKOPFLAG_TREE&~(KTASKOPFLAG_RECURSIVE))) ? NULL : caller->t_proc),
                              mode);
 } else {
  error = __ktask_terminate_s(self,exitcode,caller,mode);
 }
 // If we have terminated ktask_self(), the following line will kill us
 KTASK_NOINTR_END
 KTASK_CRIT_END
 return error;
}


void ktask_exit(void *exitcode) {
 __evalexpr(_ktask_terminate_k(ktask_self(),exitcode));
 __builtin_unreachable();
}


kerrno_t ktask_pause(struct ktask *__restrict self) {
 kassert_ktask(self);
 return ktask_unschedule(self,KTASK_STATE_WAITING,NULL);
}
kerrno_t ktask_abssleep(struct ktask *__restrict self,
                        struct timespec const *__restrict abstime) {
 kassert_ktask(self); kassertobj(abstime);
 return ktask_unschedule(self,KTASK_STATE_WAITINGTMO,(void *)abstime);
}
kerrno_t ktask_sleep(struct ktask *__restrict self,
                     struct timespec const *__restrict timeout) {
 struct timespec abstime;
 kassert_ktask(self);
 kassertobj(timeout);
 ktime_getnoworcpu(&abstime);
 __timespec_add(&abstime,timeout);
 return ktask_abssleep(self,&abstime);
}


kerrno_t ktask_suspend_k(struct ktask *__restrict self) {
 __s32 oldcounter;
 kassert_ktask(self);
 do {
  oldcounter = katomic_load(self->t_suspended);
  if (oldcounter == KTASK_SUSPEND_MAX) return KE_OVERFLOW;
 } while (!katomic_cmpxch(self->t_suspended,oldcounter,oldcounter+1));
 if (!oldcounter) {
  kerrno_t error;
  // NOTE: 'KS_UNCHANGED' might be returned if another task was still in the scheduler
  //       of resuming the task after it had been suspended previously.
  //       In that case, try again.
  //       If, on the other hand, the task was terminated, 'KE_DESTROYED' would be returned
  while ((error = ktask_unschedule(self,KTASK_STATE_SUSPENDED,NULL)) == KS_UNCHANGED) ktask_yield();
  return error;
 }
 return KS_UNCHANGED;
}
kerrno_t ktask_resume_k(struct ktask *__restrict self) {
 __s32 oldcounter;
 kassert_ktask(self);
 do {
  oldcounter = katomic_load(self->t_suspended);
  if (oldcounter == KTASK_SUSPEND_MIN) return KE_OVERFLOW;
 } while (!katomic_cmpxch(self->t_suspended,oldcounter,oldcounter-1));
 if (oldcounter == 1) {
  kerrno_t error;
  // NOTE: 'KS_UNCHANGED' might be returned if another task was still in the scheduler
  //       of suspending the task after it had been running previously.
  //       In that case, try again.
  //       If, on the other hand, the task was terminated, 'KE_DESTROYED' would be returned
  // NOTE: We must only allow waking the task if 
  while ((error = ktask_reschedule_ex(self,kcpu_leastload(),
   KTASK_RESCHEDULE_HINTFLAG_UNDOSUSPEND|
   KTASK_RESCHEDULE_HINT_DEFAULT)) == KS_UNCHANGED) ktask_yield();
  return error;
 }
 return KS_UNCHANGED;
}


kerrno_t ktask_tryjoin(struct ktask *__restrict self,
                       void **__restrict exitcode) {
 union __ktask_attrib;
 kassert_ktask(self);
 kassertobjnull(exitcode);
 if (!ktask_isterminated(self)) return KE_WOULDBLOCK;
 if (exitcode) *exitcode = self->t_exitcode;
 return KE_OK;
}
kerrno_t ktask_join(struct ktask *__restrict self,
                    void **__restrict exitcode) {
 kerrno_t error;
 kassert_ktask(self);
 kassertobjnull(exitcode);
 if ((error = ktask_tryjoin(self,exitcode)) == KE_WOULDBLOCK) {
  // The given task is still running (wait for it)
  error = ksignal_recv(&self->t_joinsig);
  if (error == KE_OK || error == KE_DESTROYED) {
   assertf(ktask_isterminated(self)
          ,"Join signal received, but task hasn't been terminated.");
   if (exitcode) *exitcode = self->t_exitcode;
   error = KE_OK;
  }
 }
 return error;
}
kerrno_t ktask_timedjoin(struct ktask *__restrict self,
                         struct timespec const *__restrict abstime,
                         void **__restrict exitcode) {
 kerrno_t error;
 kassert_ktask(self);
 kassertobj(abstime);
 kassertobjnull(exitcode);
 if ((error = ktask_tryjoin(self,exitcode)) == KE_WOULDBLOCK) {
  error = ksignal_timedrecv(&self->t_joinsig,abstime);
  if (error == KE_OK || error == KE_DESTROYED) {
   assertf(ktask_isterminated(self)
          ,"Join signal received, but task hasn't been terminated.");
   if (exitcode) *exitcode = self->t_exitcode;
   error = KE_OK;
  }
 }
 return error;
}
kerrno_t ktask_timeoutjoin(struct ktask *__restrict self,
                           struct timespec const *__restrict timeout,
                           void **__restrict exitcode) {
 struct timespec abstime;
 kassert_ktask(self);
 kassertobj(timeout);
 kassertobjnull(exitcode);
 ktime_getnoworcpu(&abstime);
 __timespec_add(&abstime,timeout);
 return ktask_timedjoin(self,&abstime,exitcode);
}

kerrno_t
ktask_getexitcode(struct ktask *__restrict self,
                  void **__restrict exitcode) {
 kassert_ktask(self);
 kassertobj(exitcode);
 if __unlikely(!ktask_isterminated(self)) return KE_BUSY;
 *exitcode = self->t_exitcode;
 return KE_OK;
}


void ktask_stackpush_sp_unlocked(struct ktask *__restrict self,
                                 void const *__restrict p, size_t s) {
 void *dest; kassert_ktask(self); kassertmem(p,s);
 assert(ktask_issuspended(self));
 // NOTE: The actual value of 't_esp' may not be translatable if
 //       it is located just out-of-bounds of a whole page (aka. fully aligned)
 //    >> So we must only assert that the new ESP if in bounds!
 assertf((uintptr_t)self->t_esp > s,"Overflow");
 dest = kpagedir_translate(self->t_userpd,(void *)((uintptr_t)self->t_esp-s));
 assertf(dest,"ESP Pointer is not mapped to any page");
 assertf(dest >= self->t_kstack,"ESP is out-of-bounds: %p (below)",dest);
 assertf(dest < self->t_kstackend,"ESP is out-of-bounds: %p (above)",dest);
 memcpy(dest,p,s);
 *(uintptr_t *)&self->t_esp -= s;
}
void ktask_stackpop_sp_unlocked(struct ktask *__restrict self,
                                void *__restrict p, size_t s) {
 void *src;
 kassert_ktask(self); kassertmem(p,s);
 assert(ktask_issuspended(self));
 src = kpagedir_translate(self->t_userpd,self->t_esp);
 assertf(src,"ESP Pointer is not mapped to any page");
 assertf(src >= self->t_kstack,"ESP is out-of-bounds (below)");
 assertf(src < self->t_kstackend,"ESP is out-of-bounds (above)");
 assertf(!((uintptr_t)src+s < (uintptr_t)src ||
           (uintptr_t)src+s > (uintptr_t)self->t_kstackend),
         "The new ESP is out-of-bounds (below)");
 memcpy(p,(void *)src,s);
 *(uintptr_t *)&self->t_esp += s;
}

#if KCONFIG_HAVE_TASK_NAMES
__crit kerrno_t
ktask_setnameex_ck(struct ktask *__restrict self,
                   char const *__restrict name, size_t maxlen) {
 char *dupname;
 KTASK_CRIT_MARK
 kassert_ktask(self);
 if __unlikely((dupname = strndup(name,maxlen)) == NULL) return KE_NOMEM;
 if __likely(katomic_cmpxch(self->t_name,NULL,dupname)) return KE_OK;
 // A name had already been set
 free(dupname);
 return KE_EXISTS;
}
#endif



__kernel void *ktask_getkernelesp_n(struct ktask const *__restrict self) {
#ifdef __DEBUG__
 void *result;
 kassert_ktask(self);
 result = ktask_getkernelesp(self);
 assertf(result != NULL,"useresp %p of task %p(%s) is not mapped",
         self->t_esp,self,ktask_getname(self));
 return result;
#else
 kassert_ktask(self);
 return ktask_getkernelesp(self);
#endif
}

struct ktask *ktask_procof(struct ktask *__restrict self) {
 struct ktask *result;
 struct kproc *firstctx;
 kassert_ktask(self);
 firstctx = (result = self)->t_proc;
 while (result->t_parent != result &&
        result->t_parent->t_proc == firstctx
        ) result = result->t_parent;
 return result;
}



void ktask_setupuser(struct ktask *self, __user void *useresp, __user void *eip) {
 struct ktaskregisters3 regs;
 kassert_ktask(self);
 assert(ktask_isusertask(self));
 /* The 'self->t_proc->p_shm.sm_ldt.ldt_vector' vector (aka. the LDT vector)
  * must be mapped in the page directory active when in ring-#3
  *       
  * My god! Why do I have to figure this stuff out myself?
  *         Why is there no one on the entire Internet that already had to deal with this?
  * >> Like seriously: Am I writing the first hobby OS planned to include TRUE thread-local storage?
  */
#if KCONFIG_HAVE_I386_SAVE_SEGMENT_REGISTERS
 regs.base.es     =
 regs.base.fs     =
 regs.base.gs     =
#endif
 regs.base.ds     =
#ifdef KSEG_USER_DATA
 regs.ss          = KSEG_USER_DATA|3;
#else
 regs.ss          = self->t_proc->p_regs.pr_ds|3;
#endif
#ifdef KSEG_USER_CODE
 regs.base.cs     = KSEG_USER_CODE|3;
#else
 regs.base.cs     = self->t_proc->p_regs.pr_cs|3;
#endif
 regs.base.eip    = (uintptr_t)eip;
 regs.useresp     = (uintptr_t)useresp;
 regs.base.eflags = KARCH_X86_EFLAGS_IF;
 ktask_stackpush_sp_unlocked(self,&regs,sizeof(regs));
}

__crit kerrno_t
ktask_settls_c(struct ktask *__restrict self, __ktls_t tlsid,
               void *value, void **oldvalue) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_ktask(self);
 kassertobjnull(oldvalue);
 error = kproc_lock(self->t_proc,KPROC_LOCK_TLSMAN);
 if __unlikely(KE_ISERR(error)) return error;
 if __unlikely(!ktlsman_validtls(&self->t_proc->p_tlsman,tlsid)) {
  error = KE_INVAL;
  goto end_proc;
 }
 ktask_lock((struct ktask *)self,KTASK_LOCK_TLS);
 if (oldvalue) *oldvalue = ktlspt_get(&self->t_tls,tlsid);
 error = ktlspt_set(&self->t_tls,tlsid,value);
 ktask_unlock((struct ktask *)self,KTASK_LOCK_TLS);
end_proc:
 kproc_unlock(self->t_proc,KPROC_LOCK_TLSMAN);
 return error;
}




size_t ktasklist_insert(struct ktasklist *self, struct ktask *task) {
 size_t result,newsize; struct ktask **newvec;
 kassert_ktasklist(self);
 kassert_ktask(task);
 for (result = self->t_freep; result < self->t_taska; ++result)
  if (!self->t_taskv[result]) goto foundfree;
 if (self->t_freep < self->t_taska) {
  for (result = 0; result < self->t_freep; ++result)
   if (!self->t_taskv[result]) goto foundfree;
 }
 // Must allocate a new slot
 result = self->t_taska;
 newsize = result ? (result*3)/2 : 2;
 if __unlikely(newsize <= result) newsize = result+1;
 newvec = (struct ktask **)realloc(self->t_taskv,newsize*
                                   sizeof(struct ktask *));
 if __unlikely(!newvec) return (size_t)-1;
 memset(newvec+result,0,(newsize-result)*sizeof(struct ktask *));
 self->t_taska = newsize;
 self->t_taskv = newvec;
foundfree:
 self->t_freep = result+1;
//found:
 assert(result < self->t_taska);
 assert(!self->t_taskv[result]);
 self->t_taskv[result] = task;
 return result;
}
struct ktask *ktasklist_erase(struct ktasklist *self, size_t index) {
 struct ktask *result,**newvec; size_t newsize;
 kassert_ktasklist(self);
 if __unlikely(index >= self->t_taska) return NULL;
 assert(self->t_taska && self->t_taskv);
 result = self->t_taskv[index];
 self->t_taskv[index] = NULL;
 for (newsize = index+1; newsize != self->t_taska; ++newsize) {
  if (self->t_taskv[newsize]) { self->t_freep = index; return result; }
 }
 newsize = index; // Clean up some unused memory
 while (newsize && !self->t_taskv[newsize-1]) --newsize;
 if (!newsize) {
  free(self->t_taskv);
  self->t_taskv = NULL;
  self->t_taska = 0;
 } else {
  newvec = (struct ktask **)realloc(self->t_taskv,newsize*
                                    sizeof(struct ktask *));
  if __likely(newvec) {
   self->t_taskv = newvec;
   self->t_taska = newsize;
  }
 }
 return result;
}


__crit __ref struct ktask *
ktask_getchild(struct ktask const *self, size_t parid) {
 __ref struct ktask *result;
 kassert_ktask(self);
 KTASK_CRIT_MARK
 ktask_lock((struct ktask *)self,KTASK_LOCK_CHILDREN);
 result = ktasklist_get(&self->t_children,parid);
 if (__likely(result) && __unlikely(KE_ISERR(ktask_tryincref(result)))) result = NULL;
 ktask_unlock((struct ktask *)self,KTASK_LOCK_CHILDREN);
 return result;
}


__DECL_END

#endif /* !__KOS_KERNEL_SCHED_TASK_UTIL_C_INL__ */

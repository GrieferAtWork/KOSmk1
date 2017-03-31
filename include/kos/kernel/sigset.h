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
#ifndef __KOS_KERNEL_SIGSET_H__
#define __KOS_KERNEL_SIGSET_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/types.h>
#include <kos/kernel/signal.h>
#include <kos/kernel/debug.h>
#ifndef __INTELLISENSE__
#include <string.h>
#include <malloc.h>
#endif

__DECL_BEGIN

#ifndef __ASSEMBLY__

/* Signal set used for easily specifying sets
 * of signals during unschedule scheduler-calls. */
struct ksigset {
 struct ksigset  *ss_next;    /*< [0..1] Pointer to another signal set, allowing the user to chain sets into larger ones. */
 __size_t         ss_elemcnt; /*< Amount of signals in this set. */
 __size_t         ss_elemsiz; /*< Offset between elements of an inline signal vector (ss_eleminl). (When ZERO(0), a deref vector (ss_elemvec) is used instead)
                                  WARNING: If this value is non-ZERO, but lower than sizeof(struct ksignal), undefined behavior is invoked. */
 __size_t         ss_elemoff; /*< Offset (in bytes) to be added to elements of 'ss_elemvec' after dereferencing (holds the signal offset if the signal objects are not located at offset ZERO(0)). */
union{
 struct ksignal  *ss_eleminl; /*< [if(ss_elemsiz != 0)][elemsize(ss_elemsiz)][0..ss_elemcnt] Pointer to the first signal, with the next located when 'ss_elemsiz' is added to this. */
 void           **ss_elemvec; /*< [if(ss_elemsiz == 0)][1..1][0..ss_elemcnt] Vector of signal pointers. */
};
};

#define KSIGSET_INITONE(sig)        {NULL,1,sizeof(struct ksignal),0,{sig}}
#define KSIGSET_INITVEC(sigc,sigv)  {NULL,sigc,0,0,{(struct ksignal *)(sigv)}}                      /*< struct ksignal **sigv; */
#define KSIGSET_INITIVEC(sigc,sigv) {NULL,sigc,sizeof(struct ksignal),0,{(struct ksignal *)(sigv)}} /*< struct ksignal *sigv; */
#define KSIGSET_INITEMPTY           {NULL,0,0,0,{NULL}}

//////////////////////////////////////////////////////////////////////////
// Returns a pointer to the i'th signal in the given set.
#define KSIGSET_ELEMVEC_AT(self,i) ((struct ksignal *)((__uintptr_t)(self)->ss_elemvec[i]+(self)->ss_elemoff))
#define KSIGSET_ELEMINL_AT(self,i) ((struct ksignal *)((__uintptr_t)(self)->ss_eleminl+(i)*(self)->ss_elemsiz))
#define KSIGSET_GET(self,i) ((self)->ss_elemsiz ? KSIGSET_ELEMINL_AT(self,i) : KSIGSET_ELEMVEC_AT(self,i))

#define KSIGSET_FOREACH(signal,self) \
 for (struct ksigset const *__set = (self); __set; __set = __set->ss_next)\
 for (__size_t __i = 0; __i != __set->ss_elemcnt; ++__i)\
 if (((signal) = KSIGSET_GET(__set,__i),kassert_ksignal(signal),0));else
#define KSIGSET_FOREACH_NN(signal,self) \
 for (struct ksigset const *__set = (kassertobj(self),self); __set; __set = __set->ss_next)\
 for (__size_t __i = 0; __i != __set->ss_elemcnt; ++__i)\
 if (((signal) = KSIGSET_GET(__set,__i),kassert_ksignal(signal),0));else
#define KSIGSET_FOREACH_NN_ONE(signal,self) \
 for (__size_t __i = 0; __i != (self)->ss_elemcnt; ++__i)\
 if (((signal) = KSIGSET_GET(self,__i),kassert_ksignal(signal),0));else
#define KSIGSET_BREAK   { __set = NULL; break; }

#ifdef __INTELLISENSE__
extern void kassert_ksigset(struct ksigset const *self);
#elif defined(__DEBUG__)
#define kassert_ksigset(self) \
 __xblock({ struct ksignal const *__ss_sig;\
            KSIGSET_FOREACH_NN(__ss_sig,self) kassert_ksignal(__ss_sig);\
            (void)0;\
 })
#else
#define kassert_ksigset(self) (void)0
#endif



#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Initialize/Finalize a dynamic signal set.
#define KSIGSET_DYN_INIT            KSIGSET_INITEMPTY
extern __crit __nomp __nonnull((1)) void ksigset_dyn_init(struct ksigset *__restrict self);
extern __crit __nomp __nonnull((1)) void ksigset_dyn_quit(struct ksigset *__restrict self);
#else
#define KSIGSET_DYN_INIT            KSIGSET_INITEMPTY
#define ksigset_dyn_init(self) (void)memset(self,0,sizeof(struct ksigset))
#define ksigset_dyn_quit(self)       free((self)->ss_elemvec)
#endif

//////////////////////////////////////////////////////////////////////////
// Insert/Remove a given signal from the specified signal set.
// WARNING: These functions are not thread-safe and must _NOT_ be called on global signal sets.
// WARNING: The given signal set must be initialized/finalized using 'ksigset_dyn_(init|quit)'
// @return: KE_OK:        Successfully inserted/removed the given set.
// @return: KE_EXISTS:    [ksigset_dyn_insert] The given signal is already part of the set.
// @return: KE_NOMEM:     [ksigset_dyn_insert] Not enough available memory.
// @return: KS_UNCHANGED: [ksigset_dyn_remove] The given signal wasn't part of the set.
extern __crit __nomp __nonnull((1)) kerrno_t ksigset_dyn_insert(struct ksigset *__restrict self, struct ksignal *__restrict sig);
extern __crit __nomp __nonnull((1)) kerrno_t ksigset_dyn_remove(struct ksigset *__restrict self, struct ksignal *__restrict sig);


//////////////////////////////////////////////////////////////////////////
// Attempts to acquire a lock on all signals in the given set.
// @return: ZERO(0): Failed to acquire locks on all signals
//                  (All those we managed to acquire have been released again)
// @return: ONE(1):  Successfully acquired the given lock in all signals.
extern __crit __wunused __nonnull((1)) int
ksigset_trylocks_c(struct ksigset *__restrict self, ksiglock_t lock);

#ifdef __INTELLISENSE__
/* Returns true if the given signal set contains dead (aka. closed) signals. */
extern        __wunused __nonnull((1)) bool ksigset_isdeads(struct ksigset const *__restrict self);
extern __crit __wunused __nonnull((1)) bool ksigset_isdeads_c(struct ksigset const *__restrict self);
extern        __wunused __nonnull((1)) bool ksigset_isdeads_unlocked(struct ksigset const *__restrict self);

/* Returns true if at least one signal contained within the given set has at least one receiver. */
extern        __wunused __nonnull((1)) bool ksigset_hasrecvs(struct ksigset const *__restrict self);
extern __crit __wunused __nonnull((1)) bool ksigset_hasrecvs_c(struct ksigset const *__restrict self);
extern        __wunused __nonnull((1)) bool ksigset_hasrecvs_unlocked(struct ksigset const *__restrict self);

/* Returns the total amount of receivers of all signals contained within the given set. */
extern        __wunused __nonnull((1)) __size_t ksigset_cntrecvs(struct ksigset const *__restrict self);
extern __crit __wunused __nonnull((1)) __size_t ksigset_cntrecvs_c(struct ksigset const *__restrict self);
extern        __wunused __nonnull((1)) __size_t ksigset_cntrecvs_unlocked(struct ksigset const *__restrict self);

/* Returns true if all signals in the given set are locked using the specified lock. */
extern        __wunused __nonnull((1)) bool ksigset_islockeds(struct ksigset const *__restrict self, ksiglock_t lock);

//////////////////////////////////////////////////////////////////////////
// Lock a given signal while simultaneously ensuring a critical block in the calling task.
#define ksigset_locks                        do{ksigset_locks_c
#define ksigset_unlocks(sigset,lock)         do{ksigset_unlocks_c(sigset,lock);}while(0);}while(0)
#define ksigset_breaklocks()                 break
#define ksigset_breakunlocks                 ksigset_unlocks_c
#define ksigset_endlocks()                   do{}while(0);}while(0)
extern __crit __nonnull((1)) void ksigset_locks_c(struct ksigset const *__restrict self, ksiglock_t lock);
extern __crit __nonnull((1)) void ksigset_unlocks_c(struct ksigset const *__restrict self, ksiglock_t lock);

#else
#define ksigset_breaklocks()              NOIRQ_BREAK
#define ksigset_breakunlocks(sigset,lock) NOIRQ_BREAKUNLOCK(ksigset_unlocks_c(sigset,lock))
#define ksigset_endlocks()                NOIRQ_END
#define /*__crit*/ ksigset_locks_c(sigset,lock) \
 __xblock({ struct ksigset *const __kset = (sigset);\
            ksiglock_t const __kslslock = (lock);\
            KTASK_SPIN(ksigset_trylocks_c(__kset,__kslslock));\
            (void)0;\
 })
#define /*__crit*/ ksigset_unlocks_c(sigset,lock) \
 __xblock({ ksiglock_t const __ksul_lock = (lock);\
            struct ksignal *__ksul_sig;\
            KSIGSET_FOREACH_NN(__ksul_sig,sigset) {\
             ksignal_unlock_c(__ksul_sig,__ksul_lock);\
            }\
            (void)0;\
 })
#define ksigset_locks(sigset,lock)   NOIRQ_BEGINLOCK(ksignal_trylocks_c(sigset,lock))
#define ksigset_unlocks(sigset,lock) NOIRQ_ENDUNLOCK(ksignal_unlocks_c(sigset,lock))
#define __ksigset_getprops_boolany(sigset,single_getter) \
 __xblock({ struct ksignal *__ksgpsysig; int __ksgpsyres = 0;\
            KSIGSET_FOREACH_NN(__ksgpsysig,sigset) {\
             if (single_getter(__ksgpsysig)) { __ksgpsyres = 1; KSIGSET_BREAK; }\
            }\
            __xreturn __ksgpsyres;\
 })
#define __ksigset_getprops_sizesum(sigset,single_getter) \
 __xblock({ struct ksignal *__ksgpsysig; __size_t __ksgpsyres = 0;\
            KSIGSET_FOREACH_NN(__ksgpsysig,sigset)\
             __ksgpsyres += single_getter(__ksgpsysig);\
            __xreturn __ksgpsyres;\
 })
#define __ksigset_getprops_boolany_(sigset,single_getter,...) \
 __xblock({ struct ksignal *__ksgpsysig; int __ksgpsyres = 0;\
            KSIGSET_FOREACH_NN(__ksgpsysig,sigset) {\
             if (single_getter(__ksgpsysig,__VA_ARGS__)) { __ksgpsyres = 1; KSIGSET_BREAK; }\
            }\
            __xreturn __ksgpsyres;\
 })
#define __ksigset_getprops_boolall(sigset,single_getter) \
 __xblock({ struct ksignal *__ksgpsasig; int __ksgpsares = 1;\
            KSIGSET_FOREACH_NN(__ksgpsasig,sigset) {\
             if (!single_getter(__ksgpsasig)) { __ksgpsares = 0; KSIGSET_BREAK; }\
            }\
            __xreturn __ksgpsares;\
 })
#define __ksigset_getprops_boolall_(sigset,single_getter,...) \
 __xblock({ struct ksignal *__ksgpsasig; int __ksgpsares = 1;\
            KSIGSET_FOREACH_NN(__ksgpsasig,sigset) {\
             if (!single_getter(__ksgpsasig,__VA_ARGS__)) { __ksgpsares = 0; KSIGSET_BREAK; }\
            }\
            __xreturn __ksgpsares;\
 })
#define            ksigset_isdeads(sigset)           __ksigset_getprops_boolany(sigset,ksignal_isdead)
#define /*__crit*/ ksigset_isdeads_c(sigset)         __ksigset_getprops_boolany(sigset,ksignal_isdead_c)
#define            ksigset_isdeads_unlocked(sigset)  __ksigset_getprops_boolany(sigset,ksignal_isdead_unlocked)
#define            ksigset_hasrecvs(sigset)          __ksigset_getprops_boolany(sigset,ksignal_hasrecv)
#define /*__crit*/ ksigset_hasrecvs_c(sigset)        __ksigset_getprops_boolany(sigset,ksignal_hasrecv_c)
#define            ksigset_hasrecvs_unlocked(sigset) __ksigset_getprops_boolany(sigset,ksignal_hasrecv_unlocked)
#define            ksigset_cntrecvs(sigset)          __ksigset_getprops_sizesum(sigset,ksignal_cntrecv)
#define /*__crit*/ ksigset_cntrecvs_c(sigset)        __ksigset_getprops_sizesum(sigset,ksignal_cntrecv_c)
#define            ksigset_cntrecvs_unlocked(sigset) __ksigset_getprops_sizesum(sigset,ksignal_cntrecv_unlocked)
#define            ksigset_islockeds(sigset,lock)    __ksigset_getprops_boolall_(sigset,ksignal_islocked,lock)
#endif

#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Waits for one of a given set of signals to be emitted.
// NOTE: If you are wondering why there is no 'ksigset_trywaits', think again,
//       because you obviously don't understand what signals are supposed to be.
//       >> They don't have a state. - You can't check if a signal was send.
//          The only thing you can do, is wait until the signal is send by
//          someone, or something, in which case you will be woken up.
//         (Or you can wait for that signal with an optional timeout)
// @return: KE_OK:         A signal was successfully received.
// @return: KE_DESTROYED:  A signal was/has-been killed.
// @return: KE_TIMEDOUT:   [ksigset_(timed|timeout)recvs] The given timeout has passed.
// @return: KE_INTR:       The calling task was interrupted.
extern           __nonnull((1))   kerrno_t ksigset_recvs(struct ksigset const *__restrict self);
extern __wunused __nonnull((1,2)) kerrno_t ksigset_timedrecvs(struct ksigset const *__restrict self, struct timespec const *__restrict abstime);
extern __wunused __nonnull((1,2)) kerrno_t ksigset_timeoutrecvs(struct ksigset const *__restrict self, struct timespec const *__restrict timeout);
#else
#define ksigset_recvs(sigset) \
 __xblock({ kerrno_t __ksrcvserr;\
            struct ksigset const *const __ksrcvssigset = (sigset);\
            kassert_ksigset(__ksrcvssigset);\
            ksigset_locks(__ksrcvssigset,KSIGNAL_LOCK_WAIT);\
            __ksrcvserr = _ksigset_recvs_andunlock_c(__ksrcvssigset);\
            ksigset_endlocks();\
            __xreturn __ksrcvserr;\
 })
#define ksigset_timedrecvs(sigset,abstime) \
 __xblock({ kerrno_t __kstrcvserr;\
            struct ksigset const *const __kstrcvssigset = (sigset);\
            kassert_ksigset(__kstrcvssigset);\
            ksigset_locks(__kstrcvssigset,KSIGNAL_LOCK_WAIT);\
            __kstrcvserr = _ksignal_timedrecvs_andunlock_c(__kstrcvssigset,abstime);\
            ksigset_endlocks();\
            __xreturn __kstrcvserr;\
 })
#define ksigset_timeoutrecvs(sigset,timeout) \
 __xblock({ kerrno_t __kstorcvserr;\
            struct ksigset const *const __kstorcvssigset = (sigset);\
            kassert_ksigset(__kstorcvssigset);\
            ksigset_locks(__kstorcvssigset,KSIGNAL_LOCK_WAIT);\
            __kstorcvserr = _ksignal_timeoutrecvs_andunlock_c(__kstorcvssigset,timeout);\
            ksigset_endlocks();\
            __xreturn __kstorcvserr;\
 })
#endif
extern __crit __nonnull((1)) kerrno_t
_ksigset_recvs_andunlock_c(struct ksigset const *__restrict self);
extern __crit __wunused __nonnull((1,2)) kerrno_t
_ksigset_timedrecvs_andunlock_c(struct ksigset const *__restrict self,
                                struct timespec const *__restrict abstime);
extern __crit __wunused __nonnull((1,2)) kerrno_t
_ksigset_timeoutrecvs_andunlock_c(struct ksigset const *__restrict self,
                                  struct timespec const *__restrict timeout);

#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Condition variables:
// >> Begin receiving signals and run the given unlock code before
//    the calling task will be added to the queue of waiting tasks.
// WARNING: Any lock released will not be re-acquired after
//          the calling task is re-awakened (if ever).
// NOTE: The given code is executed in all situations (even when an error is returned)
// @return: KE_OK:         A signal was successfully received.
// @return: KE_DESTROYED:  A signal was/has-been killed.
// @return: KE_TIMEDOUT:  [ksigset_(timed|timeout)recvs] The given timeout has passed.
// @return: KE_INTR:       The calling task was interrupted.
#define ksigset_recvcs(sigset,...)                __xblock({ struct ksigset const *__ksrcvcssigset = (sigset); (__VA_ARGS__); __xreturn KE_OK; })
#define ksigset_timedrecvcs(sigset,abstime,...)   __xblock({ struct ksigset const *__kstrcvcssigset = (sigset); struct timespec const *__kstrcvcsabstime = (abstime); (__VA_ARGS__); __xreturn KE_OK; })
#define ksigset_timeoutrecvcs(sigset,timeout,...) __xblock({ struct ksigset const *__kstorcvcssigset = (sigset); struct timespec const *__kstorcvcstimeout = (timeout); (__VA_ARGS__); __xreturn KE_OK; })
#else
#define ksigset_recvcs(sigset,...) \
 __xblock({ kerrno_t __ksrcvcserr;\
            struct ksigset const *const __ksrcvcssigset = (sigset);\
            kassert_ksigset(__ksrcvcssigset);\
            ksigset_locks(__ksrcvcssigset,KSIGNAL_LOCK_WAIT);\
            (__VA_ARGS__);\
            __ksrcvcserr = _ksigset_recvs_andunlock_c(__ksrcvcssigset);\
            ksigset_endlocks();\
            __xreturn __ksrcvcserr;\
 })
#define ksignal_timedrecvcs(sigset,abstime,...) \
 __xblock({ kerrno_t __kstrcvcserr;\
            struct ksigset const *const __kstrcvcssigset = (sigset);\
            kassert_ksigset(__kstrcvcssigset);\
            ksigset_locks(__kstrcvcssigset,KSIGNAL_LOCK_WAIT);\
            (__VA_ARGS__);\
            __kstrcvcserr = _ksigset_timedrecvs_andunlock_c(__kstrcvcssigset,abstime);\
            ksigset_endlocks();\
            __xreturn __kstrcvcserr;\
 })
#define ksignal_timeoutrecvcs(sigset,timeout,...) \
 __xblock({ kerrno_t __kstorcvcserr;\
            struct ksigset const *const __kstorcvcssigset = (sigset);\
            kassert_ksigset(__kstorcvcssigset);\
            ksigset_locks(__kstorcvcssigset,KSIGNAL_LOCK_WAIT);\
            (__VA_ARGS__);\
            __kstorcvcserr = _ksigset_timeoutrecvs_andunlock_c(__kstorcvcssigset,timeout);\
            ksigset_endlocks();\
            __xreturn __kstorcvcserr;\
 })
#endif

#endif /* !__ASSEMBLY__ */

__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_SIGSET_H__ */

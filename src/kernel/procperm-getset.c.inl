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

#ifdef __INTELLISENSE__
#include "procperm.c"
#define GET
__DECL_BEGIN
#endif

#define CASE(name) case KPERM_GETID(name)

#ifdef GET
__crit kerrno_t
kproc_perm_get(struct kproc *__restrict self,
               struct kperm *perm)
#elif defined(SET)
__crit kerrno_t
kproc_perm_set(struct kproc *__restrict self,
               struct kperm const *perm)
#elif defined(XCH)
__crit kerrno_t
kproc_perm_xch(struct kproc *__restrict self,
               struct kperm *perm)
#endif
{
 size_t *psizefield;
 kerrno_t error;
 kassert_kproc(self);
 switch (perm->p_id) {

  CASE(KPERM_NAME_PRIO_MIN):
#ifdef GET
   perm->p_data.d_prio = kprocperm_getpriomin(kproc_getperm(self));
   perm->p_size        = sizeof(ktaskprio_t);
#else
   if __unlikely(perm->p_size < sizeof(ktaskprio_t)) return KE_BUFSIZ;
   {
    ktaskprio_t old_minval,old_maxval,new_val;
    do {
     old_minval = kprocperm_getpriomin(kproc_getperm(self));
     if __unlikely(perm->p_data.d_prio < old_minval) return KE_ACCES;
     old_maxval = kprocperm_getpriomax(kproc_getperm(self));
     new_val    = min(perm->p_data.d_prio,old_maxval);
    } while (!katomic_cmpxch(kproc_getperm(self)->pp_priomax,old_maxval,old_maxval)
          || !katomic_cmpxch(kproc_getperm(self)->pp_priomin,old_minval,new_val));
#ifdef XCH
    perm->p_data.d_prio = old_minval;
#endif
   }
#endif
   break;

  CASE(KPERM_NAME_PRIO_MAX):
#ifdef GET
   perm->p_data.d_prio = kprocperm_getpriomax(kproc_getperm(self));
   perm->p_size        = sizeof(ktaskprio_t);
#else
   if __unlikely(perm->p_size < sizeof(ktaskprio_t)) return KE_BUFSIZ;
   {
    ktaskprio_t old_minval,old_maxval,new_val;
    do {
     old_maxval = kprocperm_getpriomax(kproc_getperm(self));
     if __unlikely(perm->p_data.d_prio > old_maxval) return KE_ACCES;
     old_minval = kprocperm_getpriomin(kproc_getperm(self));
     new_val    = max(perm->p_data.d_prio,old_minval);
    } while (!katomic_cmpxch(kproc_getperm(self)->pp_priomin,old_minval,old_minval)
          || !katomic_cmpxch(kproc_getperm(self)->pp_priomax,old_maxval,new_val));
#ifdef XCH
    perm->p_data.d_prio = old_maxval;
#endif
   }
#endif
   break;

  {
   __u16 group;
  CASE(KPERM_NAME_FLAG):
   group = perm->p_data.d_flag_group;
#ifdef GET
   perm->p_size = sizeof(kperm_flag_t);
   if (group >= KPERM_FLAG_GROUPCOUNT) {
    perm->p_data.d_flag_mask = 0xffff; /* Forward compatibility (set by default). */
    return KE_INVAL; /*< Still signal an unknown permission. */
   }
   /* Load the permissions mask for the given group. */
   perm->p_data.d_flag_mask = katomic_load(kproc_getperm(self)->pp_flags[group]);
#else
   if (perm->p_size < sizeof(kperm_flag_t)) return KE_BUFSIZ;
   if (group >= KPERM_FLAG_GROUPCOUNT) {
#ifdef XCH
    perm->p_data.d_flag_mask = 0xffff; /* Forward compatibility (set by default). */
#endif
    return KE_INVAL; /*< Still signal an unknown permission. */
   }
#ifdef XCH
   perm->p_data.d_flag_mask =
#endif /* Delete all flags specified by the mask */
   katomic_fetchand(kproc_getperm(self)->pp_flags[group],
                   ~perm->p_data.d_flag_mask);
#endif
  } break;

  CASE(KPERM_NAME_ENV_MEMMAX):
   error = kproc_lock(self,KPROC_LOCK_ENVIRON);
   if __unlikely(KE_ISERR(error)) return error;
#ifdef GET
   perm->p_data.d_size = self->p_environ.pe_memmax;
   perm->p_size        = sizeof(size_t);
#else
   if __unlikely(perm->p_size < sizeof(size_t)) return KE_BUFSIZ;
   {
    size_t const oldval = self->p_environ.pe_memmax;
    if __unlikely(perm->p_data.d_size > oldval) return KE_ACCES;
    self->p_environ.pe_memmax = perm->p_data.d_size;
#ifdef XCH
    perm->p_data.d_size = oldval;
#endif
   }
#endif
   kproc_unlock(self,KPROC_LOCK_ENVIRON);
   break;

  CASE(KPERM_NAME_TASKNAME_MAX):
   psizefield = &kproc_getperm(self)->pp_namemax;
   goto atomic_size_field;
  CASE(KPERM_NAME_PIPE_MAX):
   psizefield = &kproc_getperm(self)->pp_pipemax;
   goto atomic_size_field;
  CASE(KPERM_NAME_MAXTHREADS):
   psizefield = &kproc_getperm(self)->pp_thrdmax;
   goto atomic_size_field;

  CASE(KPERM_NAME_MAXMEM):     /* TODO */
  CASE(KPERM_NAME_MAXMEM_RW):  /* TODO */
#ifdef GET
   perm->p_data.d_size = (size_t)-1;
   perm->p_size        = sizeof(size_t);
#else
   if __unlikely(perm->p_size < sizeof(size_t)) return KE_BUFSIZ;
#ifdef XCH
   perm->p_data.d_size = (size_t)-1;
#endif
#endif
   break;

  CASE(KPERM_NAME_SYSLOG):
#ifdef GET
   perm->p_data.d_int = kprocperm_getlogpriv(kproc_getperm(self));
   perm->p_size = sizeof(int);
#else
   if __unlikely(perm->p_size < sizeof(int)) return KE_BUFSIZ;
   {
    __u32 oldstate,newstate;
    do {
     oldstate = katomic_load(kproc_getperm(self)->pp_state);
     if (perm->p_data.d_int < (int)(oldstate >> KPROCSTATE_SHIFT_LOGPRIV)) return KE_ACCES;
     newstate = (oldstate&~(KPROCSTATE_MASK_LOGPRIV))|
                (perm->p_data.d_int << KPROCSTATE_SHIFT_LOGPRIV);
    } while (!katomic_cmpxch(kproc_getperm(self)->pp_state,oldstate,newstate));
#ifdef XCH
    perm->p_data.d_int = (int)(oldstate >> KPROCSTATE_SHIFT_LOGPRIV);
#endif
   }
#endif
   break;

  CASE(KPERM_NAME_FDMAX):
   error = kproc_lock(self,KPROC_LOCK_FDMAN);
   if __unlikely(KE_ISERR(error)) return error;
#ifdef GET
   perm->p_data.d_uint = self->p_fdman.fdm_max;
   perm->p_size = sizeof(unsigned int);
#else
   if __unlikely(perm->p_size < sizeof(unsigned int)) return KE_BUFSIZ;
   {
    unsigned int const oldval = self->p_fdman.fdm_max;
    if __unlikely(perm->p_data.d_uint > oldval) return KE_ACCES;
    self->p_fdman.fdm_max = perm->p_data.d_uint;
#ifdef XCH
    perm->p_data.d_uint = oldval;
#endif
   }
#endif
   kproc_unlock(self,KPROC_LOCK_FDMAN);
   break;

  default:
   return KE_INVAL;
 }
 return KE_OK;
atomic_size_field:
 {
#ifdef GET
  perm->p_data.d_size = katomic_load(*psizefield);
  perm->p_size        = sizeof(size_t);
#else
  size_t oldval;
  if __unlikely(perm->p_size < sizeof(size_t)) return KE_BUFSIZ;
  do {
   oldval = katomic_load(*psizefield);
   /* Make sure that the new value isn't greater than the old. */
   if __unlikely(perm->p_data.d_size > oldval) return KE_ACCES;
  } while (!katomic_cmpxch(*psizefield,oldval,perm->p_data.d_size));
#ifdef XCH
  perm->p_data.d_size = oldval;
#endif
#endif
  return KE_OK;
 }
}

#undef CASE
#ifdef GET
#undef GET
#endif
#ifdef SET
#undef SET
#endif
#ifdef XCH
#undef XCH
#endif

#ifdef __INTELLISENSE__
__DECL_END
#endif

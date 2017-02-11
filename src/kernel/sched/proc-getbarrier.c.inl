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
#include "proc.c"
#define GETREF
#endif

__DECL_BEGIN

#ifdef GETREF
__ref struct ktask *kproc_getbarrier_r_impl
#else
      struct ktask *kproc_getbarrier_impl
#endif
(struct kproc const *__restrict self, ksandbarrier_t mode) {
 struct ktask *result; kassert_kproc(self);
#ifdef GETREF
 if __unlikely(KE_ISERR(kproc_lock((struct kproc *)self,KPROC_LOCK_SAND))) return NULL;
#define LOAD    /* nothing */
#else
#define LOAD    katomic_load
#endif
 // Simple case: Single-barrier/No-barrier
 switch (mode&0xf) {
  case KSANDBOX_BARRIER_NOSETMEM:  result = LOAD(self->p_sand.ts_smbarrier); goto done;
  case KSANDBOX_BARRIER_NOGETMEM:  result = LOAD(self->p_sand.ts_gmbarrier); goto done;
  case KSANDBOX_BARRIER_NOSETPROP: result = LOAD(self->p_sand.ts_spbarrier); goto done;
  case KSANDBOX_BARRIER_NOGETPROP: result = LOAD(self->p_sand.ts_gpbarrier); goto done;
  case KSANDBOX_BARRIER_NONE:      result = ktask_self(); goto done; // Revert to ktask_self
  default: break;
 }
 {
  // Complicated case: multiple barriers requested
  struct ktask *reqbarrs[KSANDBOX_BARRIER_COUNT];
  struct ktask **iter,**end = reqbarrs;
#ifndef GETREF
  if __unlikely(KE_ISERR(kproc_lock((struct kproc *)self,KPROC_LOCK_SAND))) return NULL;
#endif
  // Collect all requested barriers
  if (mode&KSANDBOX_BARRIER_NOSETMEM)  *end++ = self->p_sand.ts_smbarrier;
  if (mode&KSANDBOX_BARRIER_NOGETMEM)  *end++ = self->p_sand.ts_gmbarrier;
  if (mode&KSANDBOX_BARRIER_NOSETPROP) *end++ = self->p_sand.ts_spbarrier;
  if (mode&KSANDBOX_BARRIER_NOGETPROP) *end++ = self->p_sand.ts_gpbarrier;
  assert(end != reqbarrs); iter = reqbarrs;
  assert(iter != end);     result = *iter++;
  assert(iter != end);
  // Select the barrier with the lowest prevalence
  do if (*iter != result && ktask_ischildof(*iter,result)) result = *iter;
  while (iter++ != end);
#ifndef GETREF
  kproc_unlock((struct kproc *)self,KPROC_LOCK_SAND);
#endif
 }
done:
#ifdef GETREF
 if __unlikely(KE_ISERR(ktask_incref(result))) result = NULL;
 kproc_unlock((struct kproc *)self,KPROC_LOCK_SAND);
#endif
 return result;
}

#undef LOAD

#ifdef GETREF
#undef GETREF
#endif

__DECL_END


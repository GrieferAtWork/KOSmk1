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
#ifndef __KOS_KERNEL_SCHED_PROC_ENVIRON_C_INL__
#define __KOS_KERNEL_SCHED_PROC_ENVIRON_C_INL__ 1

#include <kos/compiler.h>
#include <kos/config.h>
#include <kos/kernel/proc.h>

__DECL_BEGIN

__crit kerrno_t
kproc_getenv_ck(struct kproc *__restrict self,
                char const *__restrict name, size_t namemax,
                char *buf, size_t bufsize, size_t *reqsize) {
 char const *envvalue;
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kproc(self);
 if __unlikely(KE_ISERR(error = kproc_lock(self,KPROC_LOCK_ENVIRON))) return error;
 if __unlikely((envvalue = kprocenv_getenv(&self->p_environ,name,namemax)) == NULL)
  error = KE_NOENT;
 else {
  size_t valsize = (strlen(envvalue)+1)*sizeof(char);
  if (reqsize) *reqsize = valsize;
  memcpy(buf,envvalue,min(valsize,bufsize));
 }
 kproc_unlock(self,KPROC_LOCK_ENVIRON);
 return error;
}
__crit kerrno_t
kproc_setenv_ck(struct kproc *__restrict self,
                char const *__restrict name, size_t namemax,
                char const *__restrict value, size_t valuemax,
                int override) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kproc(self);
 if __unlikely(KE_ISERR(error = kproc_lock(self,KPROC_LOCK_ENVIRON))) return error;
 error = kprocenv_setenv(&self->p_environ,name,namemax,value,valuemax,override);
 kproc_unlock(self,KPROC_LOCK_ENVIRON);
 return error;
}
__crit kerrno_t
kproc_delenv_ck(struct kproc *__restrict self,
                char const *__restrict name, size_t namemax) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kproc(self);
 if __unlikely(KE_ISERR(error = kproc_lock(self,KPROC_LOCK_ENVIRON))) return error;
 error = kprocenv_delenv(&self->p_environ,name,namemax);
 kproc_unlock(self,KPROC_LOCK_ENVIRON);
 return error;
}
__crit kerrno_t
kproc_enumenv_ck(struct kproc *__restrict self,
                 penumenv callback, void *closure) {
 struct kenventry **iter,**end,*bucket;
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kproc(self);
 kassertbyte(callback);
 if __unlikely(KE_ISERR(error = kproc_lock(self,KPROC_LOCK_ENVIRON))) return error;
 end = (iter = self->p_environ.pe_map)+KPROCENV_HASH_SIZE;
 for (; iter != end; ++iter) {
  bucket = *iter;
  while (bucket) {
   char const *value = kenventry_value(bucket);
   error = (*callback)(bucket->ee_name,bucket->ee_namsize,
                       value,strlen(value),closure);
   if __unlikely(error != 0) goto done;
   bucket = bucket->ee_next;
  }
 }
done:
 kproc_unlock(self,KPROC_LOCK_ENVIRON);
 return error;
}

__crit kerrno_t
kproc_getenv_c(struct kproc *__restrict self,
               char const *__restrict name, __size_t namemax,
               char *buf, __size_t bufsize, __size_t *reqsize) {
 struct ktask *root_task; int can_access;
 KTASK_CRIT_MARK
 if __unlikely((root_task = kproc_getroottask(self)) == NULL) return KE_DESTROYED;
 can_access = ktask_accessgm(root_task);
 ktask_decref(root_task);
 if __unlikely(!can_access) return KE_ACCES;
 return kproc_getenv_ck(self,name,namemax,buf,bufsize,reqsize);
}
__crit kerrno_t
kproc_setenv_c(struct kproc *__restrict self,
               char const *__restrict name, __size_t namemax,
               char const *__restrict value, __size_t valuemax,
               int override) {
 struct ktask *root_task; int can_access;
 KTASK_CRIT_MARK
 if __unlikely((root_task = kproc_getroottask(self)) == NULL) return KE_DESTROYED;
 can_access = ktask_accesssm(root_task);
 ktask_decref(root_task);
 if __unlikely(!can_access) return KE_ACCES;
 return kproc_setenv_ck(self,name,namemax,value,valuemax,override);
}
__crit kerrno_t
kproc_delenv_c(struct kproc *__restrict self,
               char const *__restrict name, __size_t namemax) {
 struct ktask *root_task; int can_access;
 KTASK_CRIT_MARK
 if __unlikely((root_task = kproc_getroottask(self)) == NULL) return KE_DESTROYED;
 can_access = ktask_accesssm(root_task);
 ktask_decref(root_task);
 if __unlikely(!can_access) return KE_ACCES;
 return kproc_delenv_ck(self,name,namemax);
}
__crit kerrno_t
kproc_enumenv_c(struct kproc *__restrict self,
                penumenv callback, void *closure) {
 struct ktask *root_task; int can_access;
 KTASK_CRIT_MARK
 if __unlikely((root_task = kproc_getroottask(self)) == NULL) return KE_DESTROYED;
 can_access = ktask_accessgm(root_task);
 ktask_decref(root_task);
 if __unlikely(!can_access) return KE_ACCES;
 return kproc_enumenv_ck(self,callback,closure);
}


__crit kerrno_t
kproc_enumcmd_ck(struct kproc *__restrict self,
                 penumcmd callback, void *closure) {
 kerrno_t error = 0; char **iter,**end;
 KTASK_CRIT_MARK
 kassertbyte(callback); kassert_kproc(self);
 if __unlikely(KE_ISERR(error = kproc_lock(self,KPROC_LOCK_ENVIRON))) return error;
 end = (iter = self->p_environ.pe_argv)+self->p_environ.pe_argc;
 for (; iter != end; ++iter) if __unlikely((error = (*callback)(*iter,closure)) != 0) break;
 kproc_unlock(self,KPROC_LOCK_ENVIRON);
 return error;
}


__DECL_END

#endif /* !__KOS_KERNEL_SCHED_PROC_ENVIRON_C_INL__ */

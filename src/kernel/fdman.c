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
#ifndef __KOS_KERNEL_FDMAN_C__
#define __KOS_KERNEL_FDMAN_C__ 1

#include <kos/compiler.h>
#include <kos/config.h>
#include <kos/kernel/fd.h>
#include <kos/kernel/fdman.h>
#include <malloc.h>
#include <assert.h>

__DECL_BEGIN

__crit void kfdman_quit(struct kfdman *self) {
 struct kfdentry *iter,*end;
 KTASK_CRIT_MARK
 kassert_kfdman(self);
 // Close all file handles that were still opened
 // NOTE: While doing so, we do not run the close operator callback,
 //       but simply jump to directly calling the destructor, thus
 //       preventing possible errors due to already, or partially closed
 //       files, even allowing the individual files to decide how
 //       the even should be handled by themselves.
 end = (iter = self->fdm_fdv)+self->fdm_fda;
 for (; iter != end; ++iter) kfdentry_quit(iter);
 free(self->fdm_fdv);
 KFDMAN_FOREACH_SPECIAL(self,kfdentry_quit);
}

__crit kerrno_t kfdman_initcopy(struct kfdman *self,
                                struct kfdman const *right) {
 kerrno_t error; struct kfdentry *iter,*end;
 KTASK_CRIT_MARK
 kassertobj(self);
 kassert_kfdman(right);
 kobject_init(self,KOBJECT_MAGIC_FDMAN);
 self->fdm_max = right->fdm_max;
 assert((right->fdm_cnt != 0) == (right->fdm_fda != 0));
 assert(right->fdm_cnt <= right->fdm_fda);
 if ((self->fdm_cnt = right->fdm_cnt) != 0) {
  assert(right->fdm_fda);
  self->fdm_fre = right->fdm_fre;
  self->fdm_fda = right->fdm_fda;
  self->fdm_fdv = (struct kfdentry *)memdup(right->fdm_fdv,
                                            right->fdm_fda*
                                            sizeof(struct kfdentry));
  if __unlikely(!self->fdm_fdv) return KE_NOMEM;
  end = (iter = self->fdm_fdv)+self->fdm_fda;
  for (; iter != end; ++iter) if (iter->fd_type != KFDTYPE_NONE) {
   if __unlikely(KE_ISERR(error = kfdentry_initcopyself(iter))) goto err_iter;
  }
 } else {
  self->fdm_fre = 0;
  self->fdm_fda = 0;
  self->fdm_fdv = NULL;
 }
 assert(self->fdm_cnt <= self->fdm_fda);
 error = kfdentry_initcopy(&self->fdm_root,&right->fdm_root);
 if __unlikely(KE_ISERR(error)) goto err_fdv;
 error = kfdentry_initcopy(&self->fdm_cwd,&right->fdm_cwd);
 if __unlikely(KE_ISERR(error)) {
  kfdentry_quit(&self->fdm_root);
  goto err_fdv;
 }
 return KE_OK;
err_fdv:
 if (!self->fdm_cnt) return error;
 assert(self->fdm_fdv != NULL && self->fdm_fda != 0);
 iter = self->fdm_fdv+self->fdm_fda;
err_iter:
 while (iter-- != self->fdm_fdv) {
  if (iter->fd_type != KFDTYPE_NONE) kfdentry_quit(iter);
 }
 free(self->fdm_fdv);
 return error;
}


#ifndef __INTELLISENSE__
#include "fd.c.inl"
#endif

__DECL_END

#endif /* !__KOS_KERNEL_FDMAN_C__ */

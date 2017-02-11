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
#ifndef __KOS_KERNEL_PAGING_TRANSFER_C_INL__
#define __KOS_KERNEL_PAGING_TRANSFER_C_INL__ 1

#include <assert.h>
#include <kos/compiler.h>
#include <kos/kernel/debug.h>
#include <kos/kernel/paging.h>
#include <malloc.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

__DECL_BEGIN

__kernel void *
__kpagedir_translate_u_impl(struct kpagedir const *self,
                            __user void const *addr,
                            __size_t *__restrict max_bytes,
                            __u32 flags) {
 size_t req_max_bytes,avail_max_bytes;
 uintptr_t result,aligned_addr,found_aligned_addr;
 kassertobj(max_bytes);
 req_max_bytes = *max_bytes;
 result = (uintptr_t)kpagedir_translate_flags(self,addr,KPAGEDIR_TRANSLATE_FLAGS(flags));
 if __unlikely(!result) { avail_max_bytes = 0; goto end; }
 aligned_addr = alignd(result,PAGEALIGN);
 avail_max_bytes = PAGESIZE-(result-aligned_addr);
 if (avail_max_bytes >= req_max_bytes) { avail_max_bytes = req_max_bytes; goto end; }
 req_max_bytes -= avail_max_bytes;
 for (;;) {
  *(uintptr_t *)&addr += PAGESIZE,aligned_addr += PAGESIZE;
  found_aligned_addr = (uintptr_t)kpagedir_translate_flags(self,addr,KPAGEDIR_TRANSLATE_FLAGS(flags));
  if (found_aligned_addr != aligned_addr) break;
  if (req_max_bytes <= PAGESIZE) { avail_max_bytes += req_max_bytes; break; }
  avail_max_bytes += PAGESIZE;
  req_max_bytes   -= PAGESIZE;
 }
end:
 *max_bytes = avail_max_bytes;
 return (__kernel void *)result;
}


size_t __kpagedir_memcpy_k2u(struct kpagedir const *self, __user void *dst,
                             __kernel void const *__restrict src, size_t bytes) {
 uintptr_t dstaddr,pageend;
 size_t copymax,result;
 kassertobj(self);
 result = 0;
 while (bytes) {
  dstaddr = (uintptr_t)kpagedir_translate_flags(self,dst,KPAGEDIR_TRANSLATE_FLAGS
                                               (PAGEDIR_FLAG_USER|PAGEDIR_FLAG_READ_WRITE));
  if __unlikely(!dstaddr) break;
  pageend = alignd(dstaddr+PAGESIZE,PAGEALIGN);
  assert(pageend != dstaddr);
  copymax = pageend-dstaddr;
  if (bytes < copymax) copymax = bytes;
  memcpy((void *)dstaddr,src,copymax);
  result += copymax;
  if (bytes == copymax) break;
  bytes -= copymax;
  *(uintptr_t *)&dst += copymax;
  *(uintptr_t *)&src += copymax;
 }
 return result;
}
size_t __kpagedir_memcpy_u2k(struct kpagedir const *self, __kernel void *__restrict dst,
                             __user void const *src, size_t bytes) {
 uintptr_t srcaddr,pageend;
 size_t copymax,result;
 kassertobj(self);
 result = 0;
 while (bytes) {
  srcaddr = (uintptr_t)kpagedir_translate_flags(self,src,KPAGEDIR_TRANSLATE_FLAGS
                                               (PAGEDIR_FLAG_USER));
  if __unlikely(!srcaddr) break;
  pageend = alignd(srcaddr+PAGESIZE,PAGEALIGN);
  assert(pageend != srcaddr);
  copymax = pageend-srcaddr;
  if (bytes < copymax) copymax = bytes;
  memcpy(dst,(void *)srcaddr,copymax);
  result += copymax;
  if (bytes == copymax) break;
  bytes -= copymax;
  *(uintptr_t *)&dst += copymax;
  *(uintptr_t *)&src += copymax;
 }
 return result;
}
size_t __kpagedir_memcpy_u2u(struct kpagedir const *self, __user void *dst,
                             __user void const *src, size_t bytes) {
 void *__restrict buf; size_t result;
 kassertobj(self);
 // TODO: Real Implementation
 buf = malloc(bytes);
 if __unlikely(!buf) return 0;
 result = __kpagedir_memcpy_u2k(self,buf,src,bytes);
 result = __kpagedir_memcpy_k2u(self,dst,buf,result);
 free(buf);
 return result;
}


__DECL_END

#endif /* !__KOS_KERNEL_PAGING_TRANSFER_C_INL__ */

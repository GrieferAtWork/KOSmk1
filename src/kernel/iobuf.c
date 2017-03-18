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
#ifndef __KOS_KERNEL_IOBUF_C__
#define __KOS_KERNEL_IOBUF_C__ 1

#include <kos/compiler.h>
#include <kos/kernel/iobuf.h>
#include <kos/kernel/signal.h>
#include <kos/kernel/task.h>
#include <kos/kernel/util/string.h>
#include <sys/types.h>

__DECL_BEGIN

__crit kerrno_t
__kiobuf_flush_c(struct kiobuf *__restrict self) {
 kerrno_t error; byte_t *newbuf; size_t newsize;
 KTASK_CRIT_MARK
 kassert_kiobuf(self);
 if __unlikely(KE_ISERR(error = krwlock_beginwrite(&self->ib_rwlock))) goto end_always;
 if ((newsize = __kiobuf_maxread(self,self->ib_rpos)) != self->ib_size) {
  if (!newsize) {
   free(self->ib_buffer);
   self->ib_buffer = NULL;
   self->ib_rpos   = NULL;
   self->ib_wpos   = NULL;
  } else {
   size_t rindex = self->ib_rpos-self->ib_buffer;
   newbuf = (byte_t *)realloc(self->ib_buffer,newsize);
   if __unlikely(!newbuf) { error = KE_NOMEM; goto end; }
   /* Shift unread memory towards the start */
   memmove(newbuf,newbuf+rindex,newsize-rindex);
   self->ib_rpos   = newbuf;
   self->ib_wpos   = newbuf+newsize;
   self->ib_buffer = newbuf;
  }
  self->ib_size = newsize;
 } else {
  error = KS_UNCHANGED;
 }
end: krwlock_endwrite(&self->ib_rwlock);
end_always:
 return error;
}

__crit size_t
__kiobuf_reserve_c(struct kiobuf *__restrict self,
                   size_t write_size) {
 size_t result,new_total_size;
 byte_t *new_buffer;
 KTASK_CRIT_MARK
 kassert_kiobuf(self);
 if __unlikely(KE_ISERR(krwlock_beginwrite(&self->ib_rwlock))) { result = 0; goto end_always; }
 assert((self->ib_size != 0) == (self->ib_buffer != NULL));
 assert((self->ib_rpos != NULL) == (self->ib_buffer != NULL));
 assert((self->ib_wpos != NULL) == (self->ib_buffer != NULL));
 new_total_size = self->ib_size+write_size;
 /* Clamp the new total size to what is actually allowed. */
 new_total_size = min(new_total_size,self->ib_maxsize);
 assert(new_total_size >= self->ib_size);
 if (new_total_size != self->ib_size) {
  size_t rpos,wpos;
  assert(new_total_size);
  /* Try to allocate a new buffer of sufficient size. */
  new_buffer = (byte_t *)realloc(self->ib_buffer,new_total_size);
  if __unlikely(!new_buffer) goto res_zero;
  result = new_total_size-self->ib_size;
#if KIOBUF_ZERO_UNUSED_BUFFER
  memset(new_buffer+self->ib_size,0,result);
#endif
  /* Install the new buffer */
  rpos = self->ib_rpos-self->ib_buffer;
  wpos = self->ib_wpos-self->ib_buffer;
  self->ib_buffer = new_buffer;
  self->ib_rpos = new_buffer+rpos;
  self->ib_wpos = new_buffer+wpos;
  self->ib_size = new_total_size;
 } else {
res_zero:
  /* The buffer can't grow this large, or 'write_size' was 0 */
  result = 0;
 }
 krwlock_endwrite(&self->ib_rwlock);
end_always:
 return result;
}


__crit kerrno_t
__kiobuf_interrupt_c(struct kiobuf *__restrict self) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kiobuf(self);
 if __unlikely(KE_ISERR(error = krwlock_beginwrite(&self->ib_rwlock))) goto end_always;
 /* Set the interrupt flag to stop the next empty read call. */
 self->ib_flags |= KIOBUF_FLAG_INTR_BLOCKFIRST;
 error = ksignal_sendall(&self->ib_avail) ? KE_OK : KS_EMPTY;
 krwlock_endwrite(&self->ib_rwlock);
end_always:
 return error;
}


__crit kerrno_t
__kiobuf_unread_c(struct kiobuf *__restrict self, __size_t max_unread,
                  __size_t *__restrict unread_bytes) {
 /* Since this function could be used to access random, freed kernel memory,
  * I/O buffers will ZERO out new buffer memory after allocation. */
 kerrno_t error;
 size_t max_writesize;
 KTASK_CRIT_MARK
 kassert_kiobuf(self);
 kassertobj(unread_bytes);
 if __unlikely(KE_ISERR(error = krwlock_beginwrite(&self->ib_rwlock))) goto end_always;
 max_writesize = __kiobuf_maxwrite(self,self->ib_rpos);
 /* Clamp the unread size to what is actually possible. */
 max_unread = min(max_unread,max_writesize);
 if ((*unread_bytes = max_unread) != 0) {
  assert(self->ib_rpos != self->ib_wpos);
  if (self->ib_rpos > self->ib_wpos) {
   /* Unread upper memory. */
   self->ib_rpos -= max_unread;
   assert(self->ib_rpos >= self->ib_wpos);
  } else {
   /* Figure out how much buffer is available between begin & wpos */
   size_t partsize = (size_t)(self->ib_rpos-self->ib_buffer);
   if (partsize <= max_unread) {
    /* The entire shift takes place in lower memory. */
    self->ib_rpos -= max_unread;
    assert(self->ib_rpos <= self->ib_wpos);
   } else {
    /* The shift is destined to put the r-pointer
     * above the write-position in upper memory. */
    max_unread -= partsize;
    self->ib_rpos = __kiobuf_bufend(self)-max_unread;
    assert(self->ib_rpos >= self->ib_wpos);
   }
  }
  assert(self->ib_rpos >= self->ib_buffer);
  /* Sanity: If all was unread, the read pointer
   *         must match the write pointer. */
  assert((max_unread == max_writesize) ==
         (self->ib_rpos == self->ib_wpos));
 }
end_always:
 return error;
}


__crit kerrno_t
__kiobuf_unwrite_c(struct kiobuf *__restrict self, size_t max_unwrite,
                   size_t *__restrict unwritten_bytes) {
 kerrno_t error;
 size_t max_readsize;
 KTASK_CRIT_MARK
 kassert_kiobuf(self);
 kassertobj(unwritten_bytes);
 if __unlikely(KE_ISERR(error = krwlock_beginwrite(&self->ib_rwlock))) goto end_always;
 max_readsize = __kiobuf_maxread(self,self->ib_rpos);
 /* Clamp the unwrite size to what is actually possible. */
 max_unwrite = min(max_unwrite,max_readsize);
 if ((*unwritten_bytes = max_unwrite) != 0) {
  if (max_unwrite == max_readsize) {
   /* Special case: Discard everything. */
   self->ib_wpos = self->ib_buffer;
   self->ib_rpos = self->ib_buffer+self->ib_size;
  } else {
   /* Normal case: Discard part of the written buffer. */
   if (self->ib_rpos < self->ib_wpos) {
    self->ib_wpos -= max_unwrite;
    assert(self->ib_wpos > self->ib_rpos);
   } else {
    /* Figure out how much buffer is available between begin & wpos */
    size_t partsize = (size_t)(self->ib_wpos-self->ib_buffer);
    if (partsize <= max_unwrite) {
     /* The entire shift takes place in lower memory. */
     self->ib_wpos -= max_unwrite;
     assert(self->ib_wpos < self->ib_rpos);
    } else {
     /* The shift is destined to put the w-pointer
      * above the read-position in upper memory. */
     max_unwrite -= partsize;
     assert(max_unwrite);
     self->ib_wpos = __kiobuf_bufend(self)-max_unwrite;
     assert(self->ib_wpos > self->ib_rpos);
    }
    assert(self->ib_wpos >= self->ib_buffer);
   }
  }
 }
 krwlock_endwrite(&self->ib_rwlock);
end_always:
 return error;
}


__crit kerrno_t
__kiobuf_rseek_c(struct kiobuf *__restrict self,
                 ssize_t off, size_t *diff) {
 KTASK_CRIT_MARK
 kassert_kiobuf(self);
 kassertobj(diff);
 if (off < 0) {
  return kiobuf_unread(self,(size_t)-off,diff);
 } else {
  return kiobuf_skip(self,(size_t)off,diff,KIO_BLOCKNONE);
 }
}
__crit kerrno_t
__kiobuf_wseek_c(struct kiobuf *__restrict self,
                 ssize_t off, size_t *diff) {
 KTASK_CRIT_MARK
 kassert_kiobuf(self);
 kassertobj(diff);
 if (off < 0) {
  return kiobuf_unwrite(self,(size_t)-off,diff);
 } else {
  *diff = 0;
  return KE_OK;
 }
}


__DECL_END

#ifndef __INTELLISENSE__
#define SKIP_MEMORY
#include "iobuf-readimpl.c.inl"
#define USER_MEMORY
#include "iobuf-readimpl.c.inl"
#include "iobuf-readimpl.c.inl"
#define USER_MEMORY
#include "iobuf-writeimpl.c.inl"
#include "iobuf-writeimpl.c.inl"
#endif

#endif /* !__KOS_KERNEL_IOBUF_C__ */

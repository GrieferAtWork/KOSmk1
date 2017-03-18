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
#include "iobuf.c"
#define USER_MEMORY
__DECL_BEGIN
#endif


#ifdef USER_MEMORY
__crit kerrno_t
__kiobuf_user_write_c_impl(struct kiobuf *__restrict self, __user void const *buf,
                           size_t bufsize, __kernel size_t *__restrict wsize, kioflag_t mode)
#else
__crit kerrno_t
__kiobuf_write_c(struct kiobuf *__restrict self, void const *buf,
                 size_t bufsize, size_t *__restrict wsize, kioflag_t mode)
#endif
{
 size_t max_write;
 kerrno_t error; byte_t *bufend;
 KTASK_CRIT_MARK
 kassert_kiobuf(self);
 kassertobj(wsize);
#ifndef USER_MEMORY
 kassertmem(buf,bufsize);
#define MEMCPY(dst,src,bytes) memcpy(dst,src,bytes)
#else
#define MEMCPY(dst,src,bytes) if __unlikely(copy_from_user(dst,src,bytes)) goto err_fault
#endif
 *wsize = 0;
again:
 if __unlikely(KE_ISERR(error = krwlock_beginwrite(&self->ib_rwlock))) goto end_always;
 assert(self->ib_rpos >= self->ib_buffer && self->ib_rpos <= __kiobuf_bufend(self));
 assert(self->ib_wpos >= self->ib_buffer && self->ib_wpos <= __kiobuf_bufend(self));
 assert(self->ib_size <= self->ib_maxsize);
 max_write = __kiobuf_maxwrite(self,self->ib_rpos);
 if (!max_write) {
buffer_is_full:
  if ((mode&KIO_BLOCKFIRST) && self->ib_size == self->ib_maxsize) {
   /* Don't wait if we've already read something and are
    * only supposed to block for the first chunk of data. */
   if (*wsize && (mode&(KIO_BLOCKFIRST|KIO_BLOCKALL)) == KIO_BLOCKFIRST) goto end;
   //printf("WAIT ON FULL BUFFER (r:%p w:%p b:%p e:%p)\n",
   //       self->ib_rpos,self->ib_wpos,self->ib_buffer,
   //       self->ib_buffer+self->ib_size);
   /* Wait until at there is at least something to read
    * NOTE: The following code essentially performs what a
    *       condition variable calls its wait-operation. */
   error = ksignal_recvc(&self->ib_nfull,krwlock_endwrite(&self->ib_rwlock));
   if __unlikely(KE_ISERR(error)) goto end_always;
   goto again;
  } /*else goto end;*/
 }
 bufend = __kiobuf_bufend(self);
 if (max_write < bufsize) {
  byte_t *new_buffer;
  size_t read_pos,write_pos,new_buffer_size;
  /* Difficult case: Check if we can reallocate the buffer to be large
   *                 enough, while respecting the buffer's 'ib_maxsize'. */
  read_pos = (size_t)(self->ib_rpos-self->ib_buffer);
  write_pos = (size_t)(self->ib_wpos-self->ib_buffer);
  new_buffer_size = self->ib_size+(bufsize-max_write);
  new_buffer_size = min(new_buffer_size,self->ib_maxsize);
  //printf("Realloc for %Iu bytes (%Iu)\n",bufsize,new_buffer_size);
  assert(new_buffer_size >= self->ib_size);
  assert(new_buffer_size <= self->ib_maxsize);
  if (new_buffer_size != self->ib_size) {
   new_buffer = (byte_t *)realloc(self->ib_buffer,new_buffer_size);
   if __unlikely(!new_buffer) { error = KE_NOMEM; goto end; }
#if KIOBUF_ZERO_UNUSED_BUFFER
   /* ZERO-initialize newly allocated memory. */
   memset(new_buffer+self->ib_size,0,
          new_buffer_size-self->ib_size);
#endif
   self->ib_buffer = new_buffer;
   self->ib_size = new_buffer_size;
   /* Be careful to preserve the an empty-buffer state. */
   self->ib_rpos = new_buffer+((self->ib_rpos == bufend) ? new_buffer_size : read_pos);
   self->ib_wpos = new_buffer+write_pos;
   bufend = new_buffer+new_buffer_size;
  } else if (!max_write) {
   goto end;
  }
 }
 /* Copy into the upper portion */
 max_write = (size_t)(bufend-self->ib_wpos);
 if (self->ib_rpos > self->ib_wpos) {
  /* Make sure not to overwrite unread buffer space.
   * This check is especially necessary if the reader is slow. */
  max_write = min(max_write,(size_t)(self->ib_rpos-self->ib_wpos));
 }
 max_write = min(max_write,bufsize);
 //printf("Upper copy: %Iu\n",max_write);
 MEMCPY(self->ib_wpos,buf,max_write);
 self->ib_wpos += max_write;
 *wsize += max_write;
 assert(max_write <= bufsize);
 if (bufsize == max_write) {
  //if (self->ib_wpos == bufend)
  // self->ib_wpos = self->ib_buffer;
  goto end_ok;
 }
 *(uintptr_t *)&buf += max_write;
 bufsize -= max_write;
 assert(bufsize);
 if (self->ib_wpos != bufend) {
  goto buffer_is_full;
 }
 /* Copy into the lower portion */
 max_write = (size_t)(self->ib_rpos-self->ib_buffer);
 if (!max_write) {
  /* Special case: We can't wrap the buffer because
   *               that would indicate an empty ring. */
  goto buffer_is_full;
 }
 max_write = min(max_write,bufsize);
 MEMCPY(self->ib_buffer,buf,max_write);
 self->ib_wpos = self->ib_buffer+max_write;
 *wsize += max_write;
 assert(max_write <= bufsize);
 assert(self->ib_wpos <= self->ib_rpos);
 assert(self->ib_wpos < bufend);
 if (bufsize == max_write) goto end_ok;
 assert(self->ib_wpos == self->ib_rpos);
 *(uintptr_t *)&buf += max_write;
 bufsize -= max_write;
 /* Unwritten buffer data must still be available */
 assert(bufsize);
 goto buffer_is_full;
end_ok:
 error = KE_OK;
end:
 if (*wsize) {
  assert(bufend == self->ib_buffer+self->ib_size);
  assert(self->ib_rpos <= bufend);
  if (self->ib_rpos == bufend
#if 0
      /* Technically, we'd only need to set the r-pointer if
       * the following was true, but since there is no harm in
       * always wrapping it when it is out-of-bounds, we do so anyways.
       * NOTE: The condition would assert the w-ptr at start condition,
       *       as defined as part of the empty-buffer state, in the
       *       description in 'kiobuf_read'. */
   && self->ib_wpos == self->ib_buffer
#endif
   ) {
   /* Prevent the buffer from appearing as though it was empty */
   self->ib_rpos = self->ib_buffer;
  }
  /* Signal that data has become available */
  ksignal_sendall(&self->ib_avail);
 }
#ifdef USER_MEMORY
end_write:
#endif
 krwlock_endwrite(&self->ib_rwlock);
end_always:
 return error;
#ifdef USER_MEMORY
err_fault: error = KE_FAULT; goto end_write;
#endif
}


#undef MEMCPY
#ifdef USER_MEMORY
#undef USER_MEMORY
#endif

#ifdef __INTELLISENSE__
__DECL_END
#endif

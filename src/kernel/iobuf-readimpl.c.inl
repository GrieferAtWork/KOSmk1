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
//#define SKIP_MEMORY
#define USER_MEMORY
__DECL_BEGIN
#endif


#if defined(__DEBUG__) && 0 /* Used during creation... */
#include <stdio.h>
#define DEBUG_TRACE(x) printf x
#else
#define DEBUG_TRACE(x) (void)0
#endif

#ifdef SKIP_MEMORY
__crit kerrno_t
__kiobuf_skip_c(struct kiobuf *__restrict self,
                size_t bufsize, size_t *__restrict rsize, kioflag_t mode)
#elif defined(USER_MEMORY)
__crit kerrno_t
__kiobuf_user_read_c_impl(struct kiobuf *__restrict self, __user void *buf,
                          size_t bufsize, __kernel size_t *__restrict rsize,
                          kioflag_t mode)
#else
__crit kerrno_t
__kiobuf_read_c(struct kiobuf *__restrict self, void *__restrict buf,
                size_t bufsize, size_t *__restrict rsize, kioflag_t mode)
#endif
{
 size_t max_read_linear,max_read,destsize;
 kerrno_t error; byte_t *bufend,*rpos,*start_rpos;
 KTASK_CRIT_MARK
#ifndef SKIP_MEMORY
 byte_t *destbuf;
#endif
 kassert_kiobuf(self);
 kassertobj(rsize);
#ifndef SKIP_MEMORY
#ifndef USER_MEMORY
 kassertmem(buf,bufsize);
#endif
#endif
again_full:
#ifndef SKIP_MEMORY
#ifdef USER_MEMORY
#define MEMCPY(dst,src,bytes) if __unlikely(copy_to_user(dst,src,bytes)) goto err_fault
#else
#define MEMCPY(dst,src,bytes) memcpy(dst,src,bytes)
#endif
 destbuf = (byte_t *)buf;
#else
#define MEMCPY(dst,src,bytes) /* nothing */
#endif
 destsize = bufsize;
 *rsize = 0;
again:
 error = krwlock_beginread(&self->ib_rwlock);
again_locked:
 if __unlikely(KE_ISERR(error)) goto end_always;
 kassertmem(self->ib_buffer,self->ib_size);
 start_rpos = rpos = katomic_load(self->ib_rpos);
 assert(rpos >= self->ib_buffer && rpos <= __kiobuf_bufend(self));
 assert(self->ib_wpos >= self->ib_buffer && self->ib_wpos <= __kiobuf_bufend(self));
 max_read = __kiobuf_maxread(self,rpos);
 if (!max_read) {
buffer_is_empty:
  if (mode&KIO_BLOCKFIRST) {
   /* Don't wait if we've already read something and are
    * only supposed to block for the first chunk of data. */
   if ((mode&(KIO_BLOCKFIRST|KIO_BLOCKALL)) == KIO_BLOCKFIRST) {
    if (*rsize) goto end_rpos;
    if (self->ib_flags&KIOBUF_FLAG_INTR_BLOCKFIRST) {
handle_intr:
     error = krwlock_upgrade(&self->ib_rwlock);
     if __unlikely(KE_ISERR(error)) goto end_always;
     /* Must check for the block-first flag again
      * if we had to release the read-lock. */
     if __unlikely(error == KS_UNLOCKED &&
                   self->ib_flags&KIOBUF_FLAG_INTR_BLOCKFIRST) {
      self->ib_flags &= ~(KIOBUF_FLAG_INTR_BLOCKFIRST);
      krwlock_endwrite(&self->ib_rwlock);
      error = KS_EMPTY;
      goto end_always;
     }
     krwlock_downgrade(&self->ib_rwlock);
     goto again_locked;
    }
   }
   /* Wait until at there is at least something to read
    * NOTE: The following like essentially performs what a
    *       condition variable calls its wait-operation. */
   //DEBUG_TRACE(("WAIT FOR DATA\n"));
   error = ksignal_recvc(&self->ib_avail,krwlock_endread(&self->ib_rwlock));
   DEBUG_TRACE(("DATA AVAILABLE %Iu %p %p %p %p\n",
                __kiobuf_maxread(self,self->ib_rpos),
                self->ib_rpos,self->ib_wpos,
                self->ib_buffer,self->ib_buffer+self->ib_size));
   if __unlikely(KE_ISERR(error)) goto end_always;
   goto again;
  } else goto end_rpos;
 }
 /* Check for an I/O interrupt in block-first mode. */
 if ((mode&(KIO_BLOCKFIRST|KIO_BLOCKALL)) == KIO_BLOCKFIRST &&
     self->ib_flags&KIOBUF_FLAG_INTR_BLOCKFIRST) goto handle_intr;
 bufend = __kiobuf_bufend(self);
 /* Read upper-half memory */
 max_read_linear = min(max_read,(size_t)(bufend-rpos));
 max_read_linear = min(max_read_linear,destsize);
 MEMCPY(destbuf,rpos,max_read_linear);
 rpos += max_read_linear;
 *rsize += max_read_linear;
 if (destsize == max_read_linear ||
     rpos == self->ib_wpos) goto end_rpos;
 assertf(rpos == bufend
        ,"rpos   = %p\n"
         "wpos   = %p\n"
         "buffer = %p\n"
         "bufend = %p\n"
        ,rpos
        ,self->ib_wpos
        ,self->ib_buffer
        ,bufend);
 rpos = self->ib_buffer;
 destsize -= max_read_linear;
#ifndef SKIP_MEMORY
 destbuf += max_read_linear;
#endif
 /* Read lower-half memory */
 max_read_linear = min((size_t)(self->ib_wpos-rpos),destsize);
 MEMCPY(destbuf,rpos,max_read_linear);
 rpos += max_read_linear;
 *rsize += max_read_linear;
 if (destsize == max_read_linear) goto end_rpos;
 /* All available memory was read */
 assertf(rpos == self->ib_wpos
        ,"rpos   = %p\n"
         "wpos   = %p\n"
         "buffer = %p\n"
         "bufend = %p\n"
        ,rpos
        ,self->ib_wpos
        ,self->ib_buffer
        ,bufend);
 destsize -= max_read_linear;
#ifndef SKIP_MEMORY
 destbuf += max_read_linear;
#endif
 goto buffer_is_empty;
end_rpos:
 (void)(bufend = bufend); /* It is always initialized! */
 assert(bufend == self->ib_buffer+self->ib_size);
 assert(start_rpos >= self->ib_buffer && start_rpos <= bufend);
 assert(rpos >= self->ib_buffer && rpos <= bufend);
 if (!(mode&KIO_PEEK) && *rsize) {
  int buffer_was_full;
  buffer_was_full = (start_rpos == self->ib_wpos ||
                    (start_rpos == self->ib_buffer &&
                     self->ib_wpos == bufend));
  if (rpos == self->ib_wpos) {
   /* Special case: The buffer is now empty, but if we
    * would simply set the r-pointer like usual, the buffer
    * would look like it was full.
    * >> Instead, we must upgrade our lock to get write-access,
    *    and then continue to setup the buffer as follow:
    * BEFORE:               v r/w-pos
    *   ====================|======
    *
    * AFTER: r-pos (out-of-bounds) v
    *   |==========================|
    *   ^ w-pos
    */
   error = krwlock_upgrade(&self->ib_rwlock);
   if __unlikely(KE_ISERR(error)) goto end_always;
   __compiler_barrier();
   /* Make sure our original start r-pos is still valid.
    * Also make sure no other was performed a write, thus making this just a regular case. */
   DEBUG_TRACE(("BUFFER BECAME EMPTY\n"));
   /* NOTE: the second check might fail if some other task quickly performed a read/write. */
   if __unlikely(error == KS_UNLOCKED &&
                (self->ib_rpos != start_rpos || rpos != self->ib_wpos)) {
    /* Some other task already performed a read. */
    krwlock_endwrite(&self->ib_rwlock);
    //printf("Some other task already performed a read.\n");
    goto again_full;
   }
   assert(bufend == self->ib_buffer+self->ib_size);
   /* If the buffer was full, we'll have to wake writers later! */

   /* Finally... */
   self->ib_wpos = self->ib_buffer;
   self->ib_rpos = bufend;

   if (buffer_was_full) {
    DEBUG_TRACE(("FULL BUFFER BECAME EMPTY\n"));
    /* Wake writers if the buffer used to be full */
    krwlock_downgrade(&self->ib_rwlock);
    goto wake_writers;
   }
   krwlock_endwrite(&self->ib_rwlock);
   error = KE_OK;
   goto end_always;
  }
  /* Overwrite the read-position, if we're the fastest in doing so.
   * NOTE: Due to the fact that were only holding a read-lock,
   *       some other task may have been faster than us and
   *       already read the same data. */
  if (!katomic_cmpxch(self->ib_rpos,start_rpos,rpos) &&
      !(mode&KIO_QUICKMOVE)) goto again_full;
  /* Wake writers if the buffer used to be full */
  if (buffer_was_full) {
wake_writers:
   if (ksignal_sendall(&self->ib_nfull)) {
    DEBUG_TRACE(("Waking writers...\n"));
   }
  }
 }
 error = KE_OK;
#ifdef USER_MEMORY
end_read:
#endif
 krwlock_endread(&self->ib_rwlock);
end_always:
 return error;
#ifdef USER_MEMORY
err_fault: error = KE_FAULT; goto end_read;
#endif
}

#undef DEBUG_TRACE
#undef MEMCPY
#ifdef USER_MEMORY
#undef USER_MEMORY
#endif
#ifdef SKIP_MEMORY
#undef SKIP_MEMORY
#endif

#ifdef __INTELLISENSE__
__DECL_END
#endif

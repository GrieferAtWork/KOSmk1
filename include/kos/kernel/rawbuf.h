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
#ifndef __KOS_KERNEL_RAWBUF_H__
#define __KOS_KERNEL_RAWBUF_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/errno.h>
#include <kos/kernel/ioflag.h>
#include <kos/kernel/keyboard.h>
#include <kos/kernel/object.h>
#include <kos/kernel/sched_yield.h>
#include <kos/types.h>
#ifndef __INTELLISENSE__
#include <assert.h>
#include <kos/kernel/debug.h>
#include <kos/kernel/interrupts.h>
#include <kos/kernel/util/string.h>
#include <malloc.h>
#include <math.h>
#endif

__DECL_BEGIN

/* A simple, asynchronous buffer, not capable of
 * performing any blocking, and meant for intermediate
 * storage of data only arriving in small chunks,
 * but required to be flushed as a whole when a
 * specific event occurs, such as a control input
 * arriving.
 * 
 * (This is what implements the canon, aka. line-buffer in terminals) */

#define KOBJECT_MAGIC_RAWBUF 0x6A7BCF /*< RAWBUF. */
#define kassert_krawbuf(self) kassert_object(self,KOBJECT_MAGIC_RAWBUF)

struct krawbuf {
 KOBJECT_HEAD
#define KRAWBUF_LOCK_DATA 0x01
 __atomic __u8 rb_locks;   /*< Locks. */
#define KRAWBUF_FLAG_NONE 0x00
#define KRAWBUF_FLAG_DEAD 0x01
 __u8          rb_flags;   /*< [lock(KRAWBUF_LOCK_DATA)] Flags. */
 __u16         rb_padding; /*< Padding data. */
 __size_t      rb_maxsize; /*< [lock(KRAWBUF_LOCK_DATA)] Maximal allowed buffer size. */
 __byte_t     *rb_buffer;  /*< [lock(KRAWBUF_LOCK_DATA)][0..1] Start address for the buffer. */
 __byte_t     *rb_bufend;  /*< [lock(KRAWBUF_LOCK_DATA)][0..1] Start address for the buffer. */
 __byte_t     *rb_bufpos;  /*< [lock(KRAWBUF_LOCK_DATA)][in(rb_buffer..rb_bufend)] Current write position. */
};
#define KRAWBUF_INIT  KRAWBUF_INIT_EX((__size_t)-1)
#define KRAWBUF_INIT_EX(max_size) \
 {KOBJECT_INIT(KOBJECT_MAGIC_RAWBUF) 0,KRAWBUF_FLAG_NONE,0,max_size,NULL,NULL,NULL}

#define krawbuf_islocked(self,lock)  ((katomic_load((self)->rb_locks)&(lock))!=0)
#define krawbuf_trylock(self,lock)   ((katomic_fetchor((self)->rb_locks,lock)&(lock))==0)
#define krawbuf_lock(self,lock)      __xblock({ KTASK_SPIN(krawbuf_trylock(self,lock)); (void)0; })
#define krawbuf_unlock(self,lock)    assertef((katomic_fetchand((self)->rb_locks,~(lock))&(lock))!=0,"Lock not held")

#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Initializes a given I/O buffer
// NOTE: If no maxsize is specified, use the equivalent of 'SIZE_MAX' instead
extern void krawbuf_init(struct krawbuf *__restrict self);
extern void krawbuf_init_ex(struct krawbuf *__restrict self, __size_t maxsize);

//////////////////////////////////////////////////////////////////////////
// Destructs a given I/O buffer
extern void krawbuf_quit(struct krawbuf *__restrict self);

//////////////////////////////////////////////////////////////////////////
// Closes a given I/O buffer
// @return: KE_OK:        The I/O buffer was successfully closed.
// @return: KS_UNCHANGED: The I/O buffer was already closed.
extern kerrno_t krawbuf_close(struct krawbuf *__restrict self);

//////////////////////////////////////////////////////////////////////////
// Resets a given I/O buffer
// @return: KE_OK:        The I/O buffer was successfully reset.
// @return: KS_UNCHANGED: The I/O buffer was not closed.
extern kerrno_t krawbuf_reset(struct krawbuf *__restrict self);
#else
#define krawbuf_init_ex(self,maxsize) \
 __xblock({ struct krawbuf *const __rbieself = (self);\
            kobject_init(__rbieself,KOBJECT_MAGIC_RAWBUF);\
            __rbieself->rb_locks   = 0;\
            __rbieself->rb_flags   = KRAWBUF_FLAG_NONE;\
            __rbieself->rb_maxsize = (maxsize);\
            __rbieself->rb_buffer  = NULL;\
            __rbieself->rb_bufend  = NULL;\
            __rbieself->rb_bufpos  = NULL;\
            (void)0;\
 })
#define krawbuf_init(self) krawbuf_init_ex(self,(__size_t)-1)
#define krawbuf_quit(self) free((self)->rb_buffer)
#define krawbuf_close(self) \
 __xblock({ struct krawbuf *const __rbcself = (self); kerrno_t __rbcerror;\
            kassert_krawbuf(__rbcself);\
            NOIRQ_BEGINLOCK(krawbuf_trylock(__rbcself,KRAWBUF_LOCK_DATA));\
            if (!(__rbcself->rb_flags&KRAWBUF_FLAG_DEAD)) {\
             free(__rbcself->rb_buffer);\
             __rbcself->rb_flags |= KRAWBUF_FLAG_DEAD;\
             __rbcerror = KE_OK;\
            } else {\
             __rbcerror = KS_UNCHANGED;\
            }\
            NOIRQ_ENDUNLOCK(krawbuf_unlock(__rbrself,KRAWBUF_LOCK_DATA));\
            __xreturn __rbcerror;\
 })
#define krawbuf_reset(self) \
 __xblock({ struct krawbuf *const __rbrself = (self); kerrno_t __rbcerror;\
            kassert_krawbuf(__rbrself);\
            NOIRQ_BEGINLOCK(krawbuf_trylock(__rbrself,KRAWBUF_LOCK_DATA));\
            if (__rbrself->rb_flags&KRAWBUF_FLAG_DEAD) {\
             __rbcself->rb_flags &= ~(KRAWBUF_FLAG_DEAD);\
            __rbieself->rb_buffer = NULL;\
            __rbieself->rb_bufend = NULL;\
            __rbieself->rb_bufpos = NULL;\
             __rbcerror           = KE_OK;\
            } else {\
             assert((__rbieself->rb_bufend != NULL) == (__rbieself->rb_buffer != NULL));\
             assert((__rbieself->rb_bufpos != NULL) == (__rbieself->rb_buffer != NULL));\
             __rbcerror = KS_UNCHANGED;\
            }\
            NOIRQ_ENDUNLOCK(krawbuf_unlock(__rbrself,KRAWBUF_LOCK_DATA));\
            __xreturn __rbcerror;\
 })
#endif


#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Write data into a given raw buffer.
// @return: KE_OK:        Up to 'bufsize' bytes were written (Exact count is stored in '*wsize')
// @return: KE_NOMEM:     Not enough memory was available to reallocate the buffer ('*wsize' is set to '0').
// @return: KE_PERM:      The buffer is full and not allowed to grow anymore.
// @return: KE_DESTROYED: The buffer was destroyed (closed).
// @return: KE_FAULT:     [krawbuf_user_write] A faulty pointer was given.
__local __wunused __nonnull((1)) kerrno_t
krawbuf_write(struct krawbuf *__restrict self, __kernel void const *buf,
              __size_t bufsize, __kernel __size_t *wsize);
__local __wunused __nonnull((1)) kerrno_t
krawbuf_user_write(struct krawbuf *__restrict self, __user void const *buf,
                   __size_t bufsize, __kernel __size_t *wsize);


//////////////////////////////////////////////////////////////////////////
// Capture raw buffer data:
// >> krawbuf_capture_begin(buf);
// >> error = flush_data(krawbuf_capture_data(buf),
// >>                    krawbuf_capture_size(buf));
// >> krawbuf_capture_end(buf);
// >> handle_error(error);
extern __crit void krawbuf_capture_begin(struct krawbuf *self);
extern __crit void *krawbuf_capture_data(struct krawbuf *self);
extern __crit __size_t krawbuf_capture_size(struct krawbuf *self);
extern __crit void krawbuf_capture_end(struct krawbuf *self);
extern __crit void krawbuf_capture_end_inherit(struct krawbuf *self);
#else
#define krawbuf_capture_begin(self) krawbuf_lock(self,KRAWBUF_LOCK_DATA)
#define krawbuf_capture_data(self) (assert(krawbuf_islocked(self,KRAWBUF_LOCK_DATA)),(void *)(self)->rb_buffer)
#define krawbuf_capture_size(self) (assert(krawbuf_islocked(self,KRAWBUF_LOCK_DATA)),(size_t)((self)->rb_bufpos-(self)->rb_buffer))
#define krawbuf_capture_end_inherit(self) \
 __xblock({ struct krawbuf *const __rbceself = (self);\
            __rbceself->rb_buffer = NULL;\
            __rbceself->rb_bufpos = NULL;\
            __rbceself->rb_bufend = NULL;\
            krawbuf_unlock(__rbceself,KRAWBUF_LOCK_DATA);\
 })
#if 1
#define krawbuf_capture_end(self) \
 __xblock({ struct krawbuf *const __rbceself = (self);\
            __rbceself->rb_bufpos = __rbceself->rb_buffer;\
            krawbuf_unlock(__rbceself,KRAWBUF_LOCK_DATA);\
 })
#else
#define krawbuf_capture_end(self) \
 __xblock({ struct krawbuf *const __rbceself = (self);\
            free(__rbceself->rb_buffer);\
            __rbceself->rb_buffer = NULL;\
            __rbceself->rb_bufpos = NULL;\
            __rbceself->rb_bufend = NULL;\
            krawbuf_unlock(__rbceself,KRAWBUF_LOCK_DATA);\
 })
#endif
#endif


#ifndef __INTELLISENSE__
#define krawbuf_write(self,buf,bufsize,wsize) \
 (KTASK_ISCRIT_P \
  ? __xblock({ struct krawbuf *const __rbwself = (self); kerrno_t __rbwerror;\
               krawbuf_lock(__rbwself,KRAWBUF_LOCK_DATA);\
               __rbwerror = __krawbuf_write_unlocked(__rbwself,buf,bufsize,wsize);\
               krawbuf_unlock(__rbwself,KRAWBUF_LOCK_DATA);\
               __xreturn __rbwerror;\
    })\
  : __xblock({ struct krawbuf *const __rbwself = (self); kerrno_t __rbwerror;\
               NOIRQ_BEGINLOCK(krawbuf_trylock(__rbwself,KRAWBUF_LOCK_DATA));\
               __rbwerror = __krawbuf_write_unlocked(__rbwself,buf,bufsize,wsize);\
               NOIRQ_ENDUNLOCK(krawbuf_unlock(__rbwself,KRAWBUF_LOCK_DATA));\
               __xreturn __rbwerror;\
    })\
 )
#define krawbuf_user_write(self,buf,bufsize,wsize) \
 (KTASK_ISCRIT_P \
  ? __xblock({ struct krawbuf *const __rbwself = (self); kerrno_t __rbwerror;\
               krawbuf_lock(__rbwself,KRAWBUF_LOCK_DATA);\
               __rbwerror = __krawbuf_user_write_unlocked(__rbwself,buf,bufsize,wsize);\
               krawbuf_unlock(__rbwself,KRAWBUF_LOCK_DATA);\
               __xreturn __rbwerror;\
    })\
  : __xblock({ struct krawbuf *const __rbwself = (self); kerrno_t __rbwerror;\
               NOIRQ_BEGINLOCK(krawbuf_trylock(__rbwself,KRAWBUF_LOCK_DATA));\
               __rbwerror = __krawbuf_user_write_unlocked(__rbwself,buf,bufsize,wsize);\
               NOIRQ_ENDUNLOCK(krawbuf_unlock(__rbwself,KRAWBUF_LOCK_DATA));\
               __xreturn __rbwerror;\
    })\
 )

__local kerrno_t
__krawbuf_write_unlocked(struct krawbuf *__restrict self,
                         __kernel void const *buf, __size_t bufsize,
                         __kernel __size_t *wsize) {
 __size_t newsize,bufavail,copysize = 0;
 __byte_t *newbuf; kerrno_t error = KE_OK;
 kassert_krawbuf(self); kassertobj(wsize);
 assert((self->rb_bufend != NULL) == (self->rb_buffer != NULL));
 assert((self->rb_bufpos != NULL) == (self->rb_buffer != NULL));
 bufavail = (__size_t)(self->rb_bufend-self->rb_bufpos);
 if (bufsize > bufavail) {
  __size_t bufpos;
  if __unlikely(self->rb_flags&KRAWBUF_FLAG_DEAD) { error = KE_DESTROYED; goto end; }
  newsize = (__size_t)(self->rb_bufpos-self->rb_buffer)+bufsize;
  newsize = align(newsize,64);
  assert(newsize != (__size_t)(self->rb_bufend-self->rb_buffer));
  if (newsize > self->rb_maxsize) {
   newsize = self->rb_maxsize;
   if (newsize == (__size_t)(self->rb_bufend-self->rb_buffer)) { error = KE_PERM; goto end; }
  }
  newbuf = (__byte_t *)realloc(self->rb_buffer,newsize);
  if __unlikely(!newbuf) { error = KE_NOMEM; goto end; }
  bufpos = (__size_t)(self->rb_bufpos-self->rb_buffer);
  assert(newsize > bufpos);
  self->rb_buffer = newbuf;
  self->rb_bufend = newbuf+newsize;
  self->rb_bufpos = newbuf+bufpos;
  bufavail = (__size_t)(newsize-bufpos);
 }
 copysize = min(bufavail,bufsize);
 assert(!(self->rb_flags&KRAWBUF_FLAG_DEAD));
 memcpy(self->rb_bufpos,buf,copysize);
 self->rb_bufpos += copysize;
end:
 *wsize = copysize;
 return error;
}
__local kerrno_t
__krawbuf_user_write_unlocked(struct krawbuf *__restrict self,
                              __user void const *buf, __size_t bufsize,
                              __kernel __size_t *wsize) {
 __size_t newsize,bufavail,copyfail,copysize = 0;
 __byte_t *newbuf; kerrno_t error = KE_OK;
 kassert_krawbuf(self); kassertobj(wsize);
 assert((self->rb_bufend != NULL) == (self->rb_buffer != NULL));
 assert((self->rb_bufpos != NULL) == (self->rb_buffer != NULL));
 bufavail = (__size_t)(self->rb_bufend-self->rb_bufpos);
 if (bufsize > bufavail) {
  __size_t bufpos;
  if __unlikely(self->rb_flags&KRAWBUF_FLAG_DEAD) { error = KE_DESTROYED; goto end; }
  newsize = (__size_t)(self->rb_bufpos-self->rb_buffer)+bufsize;
  newsize = align(newsize,64);
  assert(newsize != (__size_t)(self->rb_bufend-self->rb_buffer));
  if (newsize > self->rb_maxsize) {
   newsize = self->rb_maxsize;
   if (newsize == (__size_t)(self->rb_bufend-self->rb_buffer)) { error = KE_PERM; goto end; }
  }
  newbuf = (__byte_t *)realloc(self->rb_buffer,newsize);
  if __unlikely(!newbuf) { error = KE_NOMEM; goto end; }
  bufpos = (__size_t)(self->rb_bufpos-self->rb_buffer);
  assert(newsize > bufpos);
  self->rb_buffer = newbuf;
  self->rb_bufend = newbuf+newsize;
  self->rb_bufpos = newbuf+bufpos;
  bufavail = (__size_t)(newsize-bufpos);
 }
 copysize = min(bufavail,bufsize);
 assert(!(self->rb_flags&KRAWBUF_FLAG_DEAD));
 copyfail = copy_from_user(self->rb_bufpos,buf,copysize);
 if __unlikely(copyfail) { error = KE_FAULT; copysize -= copyfail; }
 self->rb_bufpos += copysize;
end:
 *wsize = copysize;
 return error;
}
#endif

__DECL_END

#endif /* !__KERNEL__ */

#endif /* !__KOS_KERNEL_RAWBUF_H__ */

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
#ifndef __KOS_KERNEL_IOBUF_H__
#define __KOS_KERNEL_IOBUF_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/types.h>
#include <kos/errno.h>
#include <kos/kernel/object.h>
#include <kos/kernel/signal.h>
#include <kos/kernel/rwlock.h>
#include <kos/kernel/ioflag.h>
#ifndef __INTELLISENSE__
#include <kos/kernel/task.h>
#include <kos/kernel/debug.h>
#include <malloc.h>
#endif

__DECL_BEGIN

//
// An I/O Buffer for one-directional read/write.
// >> Specially designed to be written to from
//    something like an interrupt handler, while
//    being read from normal code.
// >> Features a runtime-configurable max-size
//

#define KOBJECT_MAGIC_IOBUF  0x10BCF /*< IOBUF */
#define kassert_kiobuf(self) kassert_object(self,KOBJECT_MAGIC_IOBUF)

struct kiobuf {
 KOBJECT_HEAD
#define KIOBUF_FLAG_INTR_BLOCKFIRST 0x01
#define ib_flags ib_avail.s_useru
 struct ksignal     ib_avail;   /*< Signal send when data becomes available (Used as a condition variable). */
 struct ksignal     ib_nfull;   /*< Signal send when memory becomes available in a previously full buffer. */
 struct krwlock     ib_rwlock;  /*< Read-write lock for all members below (Main lock; aka. used for close/reset). */
 __size_t           ib_maxsize; /*< [lock(ib_rwlock)] Max size, the buffer is allowed to grow to, before data is dropped. */
 __size_t           ib_size;    /*< [lock(ib_rwlock)] Allocated buffer size. */
 __byte_t          *ib_buffer;  /*< [lock(ib_rwlock)][0..ib_size][owned] Base address of the R/W buffer. */
 __atomic __byte_t *ib_rpos;    /*< [lock(ib_rwlock)][0..1][in(ib_buffer+=ib_size)][cyclic:<=ib_wpos] Read pointer within the cyclic R/W buffer (Note atomic-write is allowed when holding a read-lock). */
 __byte_t          *ib_wpos;    /*< [lock(ib_rwlock)][0..1][in(ib_buffer+=ib_size)][cyclic:>=ib_rpos] Write pointer within the cyclic R/W buffer. */
};

#define __kiobuf_bufend(self)        ((self)->ib_buffer+(self)->ib_size)
#define __kiobuf_maxread(self,rpos) \
 ((rpos) < (self)->ib_wpos ? (__size_t)((self)->ib_wpos-(rpos)) :\
 ((self)->ib_size-(__size_t)((rpos)-(self)->ib_wpos)))
#define __kiobuf_maxwrite(self,rpos) \
 ((rpos) >= (self)->ib_wpos ? (__size_t)((rpos)-(self)->ib_wpos) :\
 ((self)->ib_size-(__size_t)((self)->ib_wpos-(rpos))))

#define KIOBUF_INIT_EX(max_size) \
 {KOBJECT_INIT(KOBJECT_MAGIC_IOBUF) KSIGNAL_INIT\
 ,KSIGNAL_INIT,KRWLOCK_INIT,max_size,0,NULL,NULL,NULL}
#define KIOBUF_INIT    KIOBUF_INIT_EX((__size_t)-1)


#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Initializes a given I/O buffer
// NOTE: If no maxsize is specified, use the equivalent of 'SIZE_MAX' instead
extern void kiobuf_init(struct kiobuf *__restrict self);
extern void kiobuf_init_ex(struct kiobuf *__restrict self, __size_t maxsize);

//////////////////////////////////////////////////////////////////////////
// Destructs a given I/O buffer
extern __crit void kiobuf_quit(struct kiobuf *__restrict self);

//////////////////////////////////////////////////////////////////////////
// Closes a given I/O buffer
// @return: KE_OK:        The I/O buffer was successfully closed.
// @return: KS_UNCHANGED: The I/O buffer was already closed.
extern __crit kerrno_t kiobuf_close(struct kiobuf *__restrict self);

//////////////////////////////////////////////////////////////////////////
// Resets a given I/O buffer
// @return: KE_OK:        The I/O buffer was successfully reset.
// @return: KS_UNCHANGED: The I/O buffer was not closed.
extern __crit kerrno_t kiobuf_reset(struct kiobuf *__restrict self);
#else
#define kiobuf_init_ex(self,maxsize) \
 __xblock({ struct kiobuf *const __iobieself = (self);\
            kobject_init(__iobieself,KOBJECT_MAGIC_IOBUF);\
            ksignal_init(&__iobieself->ib_avail);\
            ksignal_init(&__iobieself->ib_nfull);\
            krwlock_init(&__iobieself->ib_rwlock);\
            __iobieself->ib_maxsize = (maxsize);\
            __iobieself->ib_size    = 0;\
            __iobieself->ib_buffer  = NULL;\
            __iobieself->ib_rpos    = NULL;\
            __iobieself->ib_wpos    = NULL;\
            (void)0;\
 })
#define kiobuf_init(self) kiobuf_init_ex(self,(__size_t)-1)
#define /*__crit*/ kiobuf_quit(self) (void)kiobuf_close(self)
#define /*__crit*/ kiobuf_close(self) \
 __xblock({ struct kiobuf *const __iobcself = (self); kerrno_t __iobcerror; KTASK_CRIT_MARK \
            if __likely((__iobcerror = krwlock_close(&__iobcself->ib_rwlock)) != KS_UNCHANGED) {\
             ksignal_close(&__iobcself->ib_avail);\
             ksignal_close(&__iobcself->ib_nfull);\
             free(__iobcself->ib_buffer);\
             __iobcself->ib_size   = 0;\
             __iobcself->ib_buffer = NULL;\
             __iobcself->ib_rpos   = NULL;\
             __iobcself->ib_wpos   = NULL;\
            }\
            __xreturn __iobcerror;\
 })
#define /*__crit*/ kiobuf_reset(self) \
 __xblock({ struct kiobuf *const __iobrself = (self); kerrno_t __iobrerror; KTASK_CRIT_MARK \
            if __likely((__iobrerror = krwlock_reset(&__iobrself->ib_rwlock)) != KS_UNCHANGED) {\
             ksignal_reset(&__iobrself->ib_avail);\
             ksignal_reset(&__iobrself->ib_nfull);\
            }\
            __xreturn __iobrerror;\
 })
#endif

//////////////////////////////////////////////////////////////////////////
// Interrupts the next, or current blocking read operation
// specifying 'KIO_BLOCKFIRST' as its blocking argument.
// Contrary to the documentation of 'KIO_BLOCKFIRST', the
// read operation will then return with '*rsize' set to ZERO(0).
// @return: KE_OK:        The I/O interrupt was performed.
// @return: KS_EMPTY:     The I/O interrupt was scheduled.
// @return: KE_DESTROYED: The given U/O buffer was closed.
extern __nonnull((1)) kerrno_t kiobuf_interrupt_c(struct kiobuf *__restrict self);
#ifdef __INTELLISENSE__
extern __nonnull((1)) kerrno_t kiobuf_interrupt(struct kiobuf *__restrict self);
#else
#define kiobuf_interrupt(self) KTASK_CRIT(kiobuf_interrupt_c(self))
#endif

//////////////////////////////////////////////////////////////////////////
// Get/Set the maximum allowed buffer size for a given I/O buffer.
// NOTE: Setting the maximum size does not retroactively shrink an already allocated buffer.
// @return: KE_OK:        The size was successfully read/written
// @return: KE_DESTROYED: The given I/O buffer was closed.
__local __wunused __nonnull((1,2)) kerrno_t
kiobuf_getmaxsize_c(struct kiobuf *__restrict self,
                    __size_t *__restrict result);
__local __wunused __nonnull((1)) kerrno_t
kiobuf_setmaxsize_c(struct kiobuf *__restrict self,
                    __size_t value,
                    __size_t *__restrict old_value);
#ifdef __INTELLISENSE__
extern __wunused __nonnull((1,2)) kerrno_t
kiobuf_getmaxsize(struct kiobuf *__restrict self,
                  __size_t *__restrict result);
extern __wunused __nonnull((1)) kerrno_t
kiobuf_setmaxsize(struct kiobuf *__restrict self,
                  __size_t value,
                  __size_t *__restrict old_value);
#else
#define kiobuf_getmaxsize(self,result) \
 KTASK_CRIT(kiobuf_getmaxsize_c(self,result))
#define kiobuf_setmaxsize(self,value,old_value) \
 KTASK_CRIT(kiobuf_setmaxsize_c(self,value,old_value))
#endif


//////////////////////////////////////////////////////////////////////////
// Get the current maximum read/write buffer sizes.
// WARNING: When used, these values must only be considered as
//          meaningless hints, as the actual read/write size
//          could already be something completely different at
//          the moment this function returns.
// @return: KE_OK:        The size was successfully stored in '*result'.
// @return: KE_DESTROYED: The given I/O buffer was closed.
__local __crit __wunused __nonnull((1,2)) kerrno_t kiobuf_getrsize_c(struct kiobuf const *__restrict self, __size_t *__restrict result);
__local __crit __wunused __nonnull((1,2)) kerrno_t kiobuf_getwsize_c(struct kiobuf const *__restrict self, __size_t *__restrict result);
#ifdef __INTELLISENSE__
extern __wunused __nonnull((1,2)) kerrno_t kiobuf_getrsize(struct kiobuf const *__restrict self, __size_t *__restrict result);
extern __wunused __nonnull((1,2)) kerrno_t kiobuf_getwsize(struct kiobuf const *__restrict self, __size_t *__restrict result);
#else
#define kiobuf_getrsize(self,result) KTASK_CRIT(kiobuf_getrsize_c(self,result))
#define kiobuf_getwsize(self,result) KTASK_CRIT(kiobuf_getwsize_c(self,result))
#endif


//////////////////////////////////////////////////////////////////////////
// Try to reserve (preallocate) memory for write
// operations with a total of 'write_size' bytes.
// This function respects a set max-size, and returns
// ZERO(0) if the buffer is at its limit, or when
// it failed to allocate a bigger buffer.
// @return: 0 : - Failed to reallocate memory.
//              - The I/O buffer was closed.
//              - The buffer is already at its limit
//                and not allowed to grow anymore.
//              - The given 'write_size' was ZERO(0)
// @return: * : Amount of bytes that were reserved.
extern __nonnull((1)) __size_t
kiobuf_reserve_c(struct kiobuf *__restrict self,
                 __size_t write_size);
#ifdef __INTELLISENSE__
extern __nonnull((1)) __size_t
kiobuf_reserve(struct kiobuf *__restrict self,
               __size_t write_size);
#else
#define kiobuf_reserve(self,write_size) \
 KTASK_CRIT(kiobuf_reserve_c(self,write_size))
#endif


//////////////////////////////////////////////////////////////////////////
// Try to release memory by flushing unused buffer memory.
// @return: KE_OK:        The size was successfully read/written.
// @return: KE_DESTROYED: The given I/O buffer was closed.
// @return: KE_NOMEM:     Failed to re-allocate the buffer.
// @return: KS_UNCHANGED: No memory was released.
extern __nonnull((1)) kerrno_t kiobuf_flush_c(struct kiobuf *__restrict self);
#ifdef __INTELLISENSE__
extern __nonnull((1)) kerrno_t kiobuf_flush(struct kiobuf *__restrict self);
#else
#define kiobuf_flush(self) KTASK_CRIT(kiobuf_flush_c(self))
#endif

//////////////////////////////////////////////////////////////////////////
// Read/Write generic memory to/from a given I/O buffer.
// @param: mode:        Block configuration to use.
// @param: rsize|wsize: Amount of bytes transferred after a success call.
// @return: KE_OK:        Up to 'bufsize' bytes were transferred (exact size is stored in '*rsize' / '*wsize')
// @return: KE_DESTROYED: The given I/O buffer was closed.
// @return: KE_TIMEDOUT:  [!KIO_BLOCKNONE] A timeout previously set using alarm() has expired.
// @return: KE_INTR:      [!KIO_BLOCKNONE] The calling thread was interrupted.
// @return: KS_EMPTY:     [kiobuf_read&KIO_BLOCKFIRST] The I/O buffer was interrupted using 'kiobuf_interrupt' ('*rsize' is set to ZERO(0)).
// NOTE: [kiobuf_write] Neither the 'KIO_PEEK', nor the 'KIO_QUICKMOVE' flag is supported.
extern __wunused __nonnull((1,4)) kerrno_t
kiobuf_read_c(struct kiobuf *__restrict self, void *buf,
              __size_t bufsize, __size_t *__restrict rsize,
              kioflag_t mode);
extern __wunused __nonnull((1,4)) kerrno_t
kiobuf_write_c(struct kiobuf *__restrict self, void const *buf,
               __size_t bufsize, __size_t *__restrict wsize,
               kioflag_t mode);
#ifdef __INTELLISENSE__
extern __wunused __nonnull((1,4)) kerrno_t
kiobuf_read(struct kiobuf *__restrict self, void *buf,
            __size_t bufsize, __size_t *__restrict rsize,
            kioflag_t mode);
extern __wunused __nonnull((1,4)) kerrno_t
kiobuf_write(struct kiobuf *__restrict self, void const *buf,
             __size_t bufsize, __size_t *__restrict wsize,
             kioflag_t mode);
#else
#define kiobuf_read(self,buf,bufsize,rsize,mode)  \
 KTASK_CRIT(kiobuf_read_c(self,buf,bufsize,rsize,mode))
#define kiobuf_write(self,buf,bufsize,wsize,mode) \
 KTASK_CRIT(kiobuf_write_c(self,buf,bufsize,wsize,mode))
#endif


//////////////////////////////////////////////////////////////////////////
// Skips up to a given amount of bytes of input memory
// @param: skipped_bytes: Amount of bytes skipped after a success call.
// @param: mode:  One of 'KIOBUF_MODE_*'+set of 'KIOBUF_FLAG_*'
// @return: KE_OK:        Up to 'max_skip' bytes were successfully skipped.
// @return: KE_DESTROYED: The given I/O buffer was closed.
extern __wunused __nonnull((1,3)) kerrno_t
kiobuf_skip_c(struct kiobuf *__restrict self, __size_t max_skip,
              __size_t *__restrict skipped_bytes, kioflag_t mode);
#ifdef __INTELLISENSE__
extern __wunused __nonnull((1,3)) kerrno_t
kiobuf_skip(struct kiobuf *__restrict self, __size_t max_skip,
            __size_t *__restrict skipped_bytes, kioflag_t mode);
#else
#define kiobuf_skip(self,max_skip,skipped_bytes,mode) \
 KTASK_CRIT(kiobuf_skip_c(self,max_skip,skipped_bytes,mode))
#endif


#ifndef __INTELLISENSE__
__local __crit kerrno_t
kiobuf_getmaxsize_c(struct kiobuf *__restrict self,
                    __size_t *__restrict result) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kiobuf(self); kassertobj(result);
 if __likely(KE_ISOK(error = krwlock_beginread(&self->ib_rwlock))) {
  *result = self->ib_maxsize;
  krwlock_endread(&self->ib_rwlock);
 }
 return error;
}
__local __crit kerrno_t
kiobuf_setmaxsize_c(struct kiobuf *__restrict self,
                    __size_t value,
                    __size_t *__restrict old_value) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kiobuf(self); kassertobj(result);
 if __likely(KE_ISOK(error = krwlock_beginwrite(&self->ib_rwlock))) {
  if (old_value) *old_value = self->ib_maxsize;
  self->ib_maxsize = value;
  krwlock_endwrite(&self->ib_rwlock);
 }
 return error;
}
__local __crit kerrno_t
kiobuf_getrsize_c(struct kiobuf const *__restrict self,
                  __size_t *__restrict result) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kiobuf(self); kassertobj(result);
 if __likely(KE_ISOK(error = krwlock_beginread(&((struct kiobuf *)self)->ib_rwlock))) {
  __byte_t *rpos = katomic_load(self->ib_rpos);
  *result = __kiobuf_maxread(self,rpos);
  krwlock_endread(&((struct kiobuf *)self)->ib_rwlock);
 }
 return error;
}
__local __crit kerrno_t
kiobuf_getwsize_c(struct kiobuf const *__restrict self,
                  __size_t *__restrict result) {
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kiobuf(self); kassertobj(result);
 if __likely(KE_ISOK(error = krwlock_beginread(&((struct kiobuf *)self)->ib_rwlock))) {
  __byte_t *rpos = katomic_load(self->ib_rpos);
  *result = __kiobuf_maxwrite(self,rpos);
  krwlock_endread(&((struct kiobuf *)self)->ib_rwlock);
 }
 return error;
}
#endif


__DECL_END

#endif /* !__KERNEL__ */

#endif /* !__KOS_KERNEL_IOBUF_H__ */

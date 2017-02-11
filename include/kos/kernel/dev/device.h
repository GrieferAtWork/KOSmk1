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
#ifndef __KOS_KERNEL_DEV_DEVICE_H__
#define __KOS_KERNEL_DEV_DEVICE_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/compiler.h>
#include <kos/kernel/types.h>
#include <kos/errno.h>
#include <kos/kernel/rwlock.h>
#include <kos/kernel/object.h>
#include <kos/attr.h>

__DECL_BEGIN

struct kdev;

#define KOBJECT_MAGIC_DEV 0xDE1
#define kassert_kdev(self) kassert_refobject(self,d_refcnt,KOBJECT_MAGIC_DEV)

#define KDEV_HEAD \
 KOBJECT_HEAD\
 __u32          d_refcnt; /*< Reference counter. */\
 struct krwlock d_rwlock; /*< Read/Write lock held when using the device. */\
 void         (*d_quit)(struct kdev *self);\
 /* NOTE: The following two are optional. */\
 /* @return: KE_NOSYS: Querying/Setting the given attribute isn't supported by the device */\
 kerrno_t     (*d_getattr)(struct kdev const *self, kattr_t attr, void *__restrict buf, __size_t bufsize, __size_t *__restrict reqsize);\
 kerrno_t     (*d_setattr)(struct kdev *self, kattr_t attr, void const *__restrict buf, __size_t bufsize);

#define KDEV_INIT(quit,getattr,setattr) \
 KOBJECT_INIT(KOBJECT_MAGIC_DEV),0xffff,\
 KRWLOCK_INIT,quit,getattr,setattr

#define kdev_init(self) \
 (kobject_init(self,KOBJECT_MAGIC_DEV),\
 (self)->d_refcnt = 1,krwlock_init(&(self)->d_rwlock))

struct kdev { KDEV_HEAD };

__local KOBJECT_DEFINE_INCREF(kdev_incref,struct kdev,d_refcnt,kassert_kdev);
__local KOBJECT_DEFINE_DECREF(kdev_decref,struct kdev,d_refcnt,kassert_kdev,kdev_destroy);

//////////////////////////////////////////////////////////////////////////
// Lock a given device.
// @return: KE_OK:         The operation was sucessful.
// @return: KE_DESTROYED:  The given device was destroyed.
// @return: KE_TIMEDOUT:   The given timeout has passed.
// @return: KE_WOULDBLOCK: 'kdev_trylock' failed to immediately acquire a lock.
#define kdev_lock_r(self)                krwlock_beginread((struct krwlock *)&(self)->d_rwlock)
#define kdev_trylock_r(self)             krwlock_trybeginread((struct krwlock *)&(self)->d_rwlock)
#define kdev_timedlock_r(self,abstime)   krwlock_timedbeginread((struct krwlock *)&(self)->d_rwlock,abstime)
#define kdev_timeoutlock_r(self,timeout) krwlock_timeoutbeginread((struct krwlock *)&(self)->d_rwlock,timeout)
#define kdev_unlock_r(self)              krwlock_endread((struct krwlock *)&(self)->d_rwlock)
#define kdev_lock_w(self)                krwlock_beginwrite((struct krwlock *)&(self)->d_rwlock)
#define kdev_trylock_w(self)             krwlock_trybeginwrite((struct krwlock *)&(self)->d_rwlock)
#define kdev_timedlock_w(self,abstime)   krwlock_timedbeginwrite((struct krwlock *)&(self)->d_rwlock,abstime)
#define kdev_timeoutlock_w(self,timeout) krwlock_timeoutbeginwrite((struct krwlock *)&(self)->d_rwlock,timeout)
#define kdev_unlock_w(self)              krwlock_endwrite((struct krwlock *)&(self)->d_rwlock)

//////////////////////////////////////////////////////////////////////////
// Closes a given device. (noexcept)
// WARNING: This operation cannot be reverted.
// @return: KE_OK:        The device was successfully closed.
// @return: KS_UNCHANGED: The device was already closed. (NOT AN ERROR)
#define kdev_close(self) \
 __xblock({ struct kdev *const __kcself = (self); kerrno_t __kcerr; \
            if __likely(krwlock_close(&__kcself->d_rwlock) != KS_UNCHANGED) { \
             __kcerr = KE_OK; if (__kcself->d_quit) (*__kcself->d_quit)(self);\
            } else __kcerr = KS_UNCHANGED;\
            __xreturn __kcerr;\
 })



#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Get/Set a given (possibly) device-specific attribute.
//  - The most widely supported attributes are 'KATTR_DEV_NAME' and 'KATTR_DEV_TYPE', read-only
//    attributes that can be used to retrieve human-readable names for a given device.
//  - In general, read/write attributes expect the safe data format for input, as well as output.
//  - When reading, if the caller doesn't provide enough memory through 'buf'..'bufsize', 'reqsize'
//    can be provided to query the buffersize required for the given attribute of the device.
//    - This can be especially useful for variable-length attributes, such as 'KATTR_DEV_NAME'
// @param: attr:    The attribute to get/set
// @param: buf:     Caller-provided input/output buffer, providing a value to
//                  apply to the attribute, or a location to store the attribute.
// @param: bufsize: The amount of bytes available for reading/writing
// @param: reqsize: [=NULL] Optional output parameter to store the used/required buffersize
// @return: KE_DESTROYED: The device was closed/destroyed. (Not returned by *_unlocked)
// @return: KE_NOSYS:     The device does not support the given attribute.
// @return: * :           Any attribute-/device specific error
extern __wunused __nonnull((1,3)) kerrno_t kdev_getattr(struct kdev const *self, kattr_t attr, void *__restrict buf, __size_t bufsize, __size_t *__restrict reqsize);
extern __wunused __nonnull((1,3)) kerrno_t kdev_setattr(struct kdev *self, kattr_t attr, void const *__restrict buf, __size_t bufsize);
extern __wunused __nonnull((1,3)) kerrno_t kdev_getattr_unlocked(struct kdev const *self, kattr_t attr, void *__restrict buf, __size_t bufsize, __size_t *__restrict reqsize);
extern __wunused __nonnull((1,3)) kerrno_t kdev_setattr_unlocked(struct kdev *self, kattr_t attr, void const *__restrict buf, __size_t bufsize);
#else
#define kdev_getattr_unlocked(self,attr,buf,bufsize,reqsize) \
 __xblock({ struct kdev const *const __kgau_self = (self);\
            __xreturn __kgau_self->d_getattr ? (*__kgau_self->d_getattr)\
                     (__kgau_self,attr,buf,bufsize,reqsize) : KE_NOSYS;\
 })
#define kdev_setattr_unlocked(self,attr,buf,bufsize) \
 __xblock({ struct kdev *const __ksau_self = (self);\
            __xreturn __ksau_self->d_setattr ? (*__ksau_self->d_setattr)\
                     (__ksau_self,attr,buf,bufsize) : KE_NOSYS;\
 })
#define kdev_getattr(self,attr,buf,bufsize,reqsize) \
 __xblock({ struct kdev const *const __kga_self = (self); kerrno_t __kga_err;\
            if __likely(KE_ISOK(__kga_err = kdev_lock_r(__kga_self))) {\
             __kga_err = __kga_self->d_getattr ? (*__kga_self->d_getattr)\
                        (__kga_self,attr,buf,bufsize,reqsize) : KE_NOSYS;\
             kdev_unlock_r(__kga_self);\
            }\
            __xreturn __kga_err;\
 })
#define kdev_setattr(self,attr,buf,bufsize) \
 __xblock({ struct kdev *const __ksa_self = (self); kerrno_t __ksa_err;\
            if __likely(KE_ISOK(__ksa_err = kdev_lock_w(__ksa_self))) {\
             __ksa_err = __ksa_self->d_setattr ? (*__ksa_self->d_setattr)\
                        (__ksa_self,attr,buf,bufsize) : KE_NOSYS;\
             kdev_unlock_w(__ksa_self);\
            }\
            __xreturn __ksa_err;\
 })
#endif

__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_DEV_DEVICE_H__ */

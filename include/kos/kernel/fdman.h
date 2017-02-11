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
#ifndef __KOS_KERNEL_FDMAN_H__
#define __KOS_KERNEL_FDMAN_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/compiler.h>
#include <kos/kernel/fd.h>
#include <kos/kernel/object.h>

__DECL_BEGIN

#define KOBJECT_MAGIC_FDMAN  0xFD77AA /*< FDMAN */
#define kassert_kfdman(self) kassert_object(self,KOBJECT_MAGIC_FDMAN)

struct kfdman {
 KOBJECT_HEAD
 // NOTE: The following are int, because the FD number (an int) is
 //       used as index, meaning that INT_MAX is the absolute
 //       maximum amount of usable file descriptors by any fdman.
 //    >> The unsigned was simply added to prevent illegal states.
 unsigned int     fdm_cnt;  /*< Amount of valid file descriptors. */
 unsigned int     fdm_max;  /*< Max amount of allowed file descriptors. */
 unsigned int     fdm_fre;  /*< Index of the last free FD slot (Ring-index; used to speed up locating free entries). */
 unsigned int     fdm_fda;  /*< Allocated amount of file descriptors (aka. highest fd index +1). */
 struct kfdentry *fdm_fdv;  /*< [0..fdm_fda][owned] List of file descriptors. */
 // Special file descriptors
 struct kfdentry  fdm_root; /*< Descriptor for the fs root (KFD_ROOT). */
 struct kfdentry  fdm_cwd;  /*< Descriptor for the fs cwd (KFD_CWD). */
};
#define KFDMAN_FDMAX_TECHNICAL_MAXIMUM   INT_MAX

#define KFDMAN_INITROOT(rootfp) \
 {KOBJECT_INIT(KOBJECT_MAGIC_FDMAN) 0,KFDMAN_FDMAX_TECHNICAL_MAXIMUM\
 ,0,0,NULL,KFDENTRY_INIT_FILE(rootfp),KFDENTRY_INIT_FILE(rootfp)}

#ifdef __INTELLISENSE__
extern __nonnull((1)) void kfdman_init(struct kfdman *__restrict self,
                                       unsigned int fd_max);
#else
#define kfdman_init(self,fd_max) \
 __xblock({ struct kfdman *const __fdself = (self);\
            kobject_init(__fdself,KOBJECT_MAGIC_FDMAN);\
            __fdself->fdm_cnt  = 0;\
            __fdself->fdm_max  = (fd_max);\
            __fdself->fdm_fre  = 0;\
            __fdself->fdm_fda  = 0;\
            __fdself->fdm_fdv  = NULL;\
            kfdentry_init(&__fdself->fdm_root);\
            kfdentry_init(&__fdself->fdm_cwd);\
            (void)0;\
 })
#endif

//////////////////////////////////////////////////////////////////////////
// Destroys a given fd manager
extern __crit __nonnull((1)) void kfdman_quit(struct kfdman *__restrict self);

//////////////////////////////////////////////////////////////////////////
// Initialize a copy/fork of a given FS Manager.
// @return: KE_OVERFLOW:  A reference counter would have overflown.
// @return: KE_NOMEM:     Not enough available memory
extern __crit __wunused __nonnull((1,2)) kerrno_t
kfdman_initcopy(struct kfdman *__restrict self,
                struct kfdman const *__restrict right);

#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Interact with the file descriptor flags of a given fd manager.
// @return: KE_OK:   The flag operation was successful.
// @return: KE_BADF: The given file descriptor was invalid.
extern __wunused __nonnull((1,2)) kerrno_t kfdman_getflags(struct kfdman *__restrict self, int fd, kfdflag_t *__restrict flags);
extern __wunused __nonnull((1))   kerrno_t kfdman_setflags(struct kfdman *__restrict self, int fd, kfdflag_t flags);
extern __wunused __nonnull((1))   kerrno_t kfdman_addflags(struct kfdman *__restrict self, int fd, kfdflag_t flags);
extern __wunused __nonnull((1))   kerrno_t kfdman_delflags(struct kfdman *__restrict self, int fd, kfdflag_t flags);
#else
#define __kfdman_withfd(self,fd,callback,arg)\
 __xblock({ struct kfdman *const __fdmself = (self);\
            kerrno_t __fdmerr; int const __fdmfd = (fd);\
            if __unlikely(__fdmfd < 0) __fdmerr = KE_BADF;\
            else if __likely((unsigned int)__fdmfd < __fdmself->fdm_fda) {\
             struct kfdentry *const __fdmfp = &__fdmself->fdm_fdv[__fdmfd];\
             if __likely(__fdmfp->fd_type != KFDTYPE_NONE) {\
              __fdmerr = callback(__fdmfp,arg);\
             } else __fdmerr = KE_BADF;\
            } else __fdmerr = KE_BADF;\
            __xreturn __fdmerr;\
 })
#define kfdman_getflags(self,fd,flags) __kfdman_withfd(self,fd,kfd_getflags,flags)
#define kfdman_setflags(self,fd,flags) __kfdman_withfd(self,fd,kfd_setflags,flags)
#define kfdman_addflags(self,fd,flags) __kfdman_withfd(self,fd,kfd_addflags,flags)
#define kfdman_delflags(self,fd,flags) __kfdman_withfd(self,fd,kfd_delflags,flags)
#endif

__local __wunused __nonnull((1)) struct kfdentry *kfdman_specialslot(struct kfdman *__restrict self, int fd);
__local __wunused __nonnull((1)) struct kfdentry *kfdman_slot(struct kfdman *__restrict self, int fd);


#ifndef __INTELLISENSE__
__local struct kfdentry *kfdman_specialslot(struct kfdman *__restrict self, int fd) {
 if (fd == KFD_ROOT) return &self->fdm_root;
 if (fd == KFD_CWD) return &self->fdm_cwd;
 return NULL;
}
__local struct kfdentry *kfdman_slot(struct kfdman *__restrict self, int fd) {
 if (fd < 0) return kfdman_specialslot(self,fd);
 if ((unsigned int)fd >= self->fdm_fda) return NULL;
 return &self->fdm_fdv[fd];
}
#endif


__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_FDMAN_H__ */

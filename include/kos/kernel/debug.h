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
#ifndef __KOS_KERNEL_DEBUG_H__
#define __KOS_KERNEL_DEBUG_H__ 1

#include <kos/config.h>

#ifndef __ASSEMBLY__
#ifdef __KERNEL__
#ifdef __DEBUG__
#include <kos/compiler.h>
#include <kos/errno.h>
#include <kos/kernel/types.h>
#include <kos/kernel/features.h>
#include <traceback.h>

__DECL_BEGIN

#define KDEBUG_SOURCEPATH_PREFIX "../"

#define debug_hexdumpo(o) debug_hexdump(&(o),sizeof(o))
extern __nonnull((1)) void debug_hexdump(void const *p, __size_t s);

//////////////////////////////////////////////////////////////////////////
// Validates a given memory range to describe valid kernel memory
// @return: KE_OK:    Everything's fine!
// @return: KS_EMPTY: The NULL pointer is part of the given range
// @return: KE_INVAL: The given memory describes a portion of memory that is not allocated
// @return: KE_RANGE: The given memory doesn't map to any valid RAM/Device memory
extern __nonnull((1)) kerrno_t kmem_validatebyte(__kernel __byte_t const *__restrict p);
extern __nonnull((1)) kerrno_t kmem_validate(__kernel void const *__restrict p, __size_t s);

__DECL_END

#if KCONFIG_HAVE_DEBUG_MEMCHECKS
#include <assert.h>
#include <kos/errno.h>
#define kassertobj(o)        kassertmem(o,sizeof(*(o)))
#define kassertobjnull(o)    kassertmemnull(o,sizeof(*(o)))
#define kassertobjmsg(o,msg) kassertmemmsg(o,sizeof(*(o)),msg)
#define kassertmem_msg __kassertmem_msg
__forcelocal char const *__kassertmem_msg(kerrno_t err) {
 return err == KE_INVAL ? "Dynamic memory range not allocated" :
        err == KE_RANGE ? "Doesn't map to RAM/Device memory" :
        err == KS_EMPTY ? "NULL Pointer" :
                          "Unknown reason";
}
#define kassertbyte(p) \
 __xblock({ __u8 const *const __p = (__u8 const *)(p);\
            kerrno_t __errid = kmem_validatebyte(__p);\
            __assert_heref("kassertbyte(" #p ")",0,__errid == KE_OK,\
                           "Invalid memory address: %p (%s)",__p,kassertmem_msg(__errid));\
 })
#define kassertmem(p,s) \
 __xblock({ void const *const __p = (p);\
            __size_t const __s = (s);\
            kerrno_t __errid = kmem_validate(__p,__s);\
            __assert_heref("kassertmem(" #p "," #s ")",0,__errid == KE_OK,\
                           "Invalid memory address: %p+%Iu ... %p (%s)",\
                           __p,__s,(void *)((__uintptr_t)__p+__s),kassertmem_msg(__errid));\
 })
#define kassertmemnull(p,s) \
 __xblock({ void const *const __p = (p);\
            __size_t const __s = (s);\
            kerrno_t __errid = __p ? kmem_validate(__p,__s) : KE_OK;\
            __assert_heref("kassertmemnull(" #p "," #s ")",0,__errid == KE_OK,\
                           "Invalid memory address: %p+%Iu ... %p (%s)",\
                           __p,__s,(void *)((__uintptr_t)__p+__s),kassertmem_msg(__errid));\
 })
#define kassertmemmsg(p,s,msg) \
 __xblock({ void const *const __p = (p);\
            __size_t const __s = (s);\
            kerrno_t __errid = kmem_validate(__p,__s);\
            __assert_heref("kassertmemmsg(" #p "," #s ",...)",0,__errid == KE_OK,\
                           "Invalid memory: %p+%Iu ... %p (%s)\n>> %s",\
                           __p,__s,(void *)((__uintptr_t)__p+__s),kassertmem_msg(__errid),msg);\
 })
#endif /* KCONFIG_HAVE_DEBUG_MEMCHECKS */
#endif /* !__DEBUG__ */
#else /* __KERNEL__ */
#define kmem_validatebyte(p) ((p) ? KE_OK : KS_EMPTY)
#define kmem_validate(p,s)   ((p) ? KE_OK : KS_EMPTY)
#endif /* __KERNEL__ */

#ifndef kmem_validateob
#define kmem_validateob(ob) kmem_validate(ob,sizeof(*(ob)))
#endif /* !kmem_validateob */

#ifndef kassertmem_msg
#define kassertmem_msg(eno)          ((char const *)0)
#endif
#ifndef kassertobj
#define kassertobj(o)                (void)0
#endif
#ifndef kassertobjnull
#define kassertobjnull(o)            (void)0
#endif
#ifndef kassertobjmsg
#define kassertobjmsg(o,msg)         (void)0
#endif
#ifndef kassertbyte
#define kassertbyte(p)               (void)0
#endif
#ifndef kassertmem
#define kassertmem(p,s)              (void)0
#endif
#ifndef kassertmemnull
#define kassertmemnull(p,s)          (void)0
#endif
#ifndef kassertmemmsg
#define kassertmemmsg(p,s,msg)       (void)0
#endif
#endif /* !__ASSEMBLY__ */

#endif /* !__KOS_KERNEL_DEBUG_H__ */

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
#ifndef __KOS_KERNEL_OBJECT_H__
#define __KOS_KERNEL_OBJECT_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/compiler.h>
#include <kos/kernel/debug.h>
#include <kos/atomic.h>
#include <assert.h>
#include <string.h>

__DECL_BEGIN

#ifdef __DEBUG__
#define KOBJECT_SIZEOFHEAD           4
#define KOBJECT_GETMAGIC(magic)     (0x70000000^(magic))
#ifndef __ASSEMBLY__
#define KOBJECT_HEAD                 __u32 __magic;
#define KOBJECT_INIT(magic)          KOBJECT_GETMAGIC(magic),
#define kobject_badmagic(ob)         (void)((ob)->__magic = 0x0BAD0BAD)
#define kobject_init(ob,magic)       (void)((ob)->__magic = KOBJECT_GETMAGIC(magic))
#define kobject_initzero(ob,magic)   (void)(((__typeof__(ob))memset(ob,0,sizeof(*(ob))))->__magic = KOBJECT_GETMAGIC(magic))
#define kobject_getmagic(ob)         ((ob)->__magic)
#define kobject_verify(ob,magic)     (kobject_getmagic(ob)==KOBJECT_GETMAGIC(magic))
#define kassert_object(ob,magic) \
 __xblock({ __u32 __magic; __typeof__(ob) const __ob = (ob);\
            kassertobj(__ob); __magic = kobject_getmagic(__ob);\
            assertf(__magic==KOBJECT_GETMAGIC(magic),\
                    "Invalid magic in %p (%I32x != %I32x) : " #ob,\
                    __ob,__magic,KOBJECT_GETMAGIC(magic));\
            (void)0;\
 })
#define kassert_refobject(ob,refcnt_member,magic) \
 __xblock({ __u32 __magic; __typeof__(ob) const __ob = (ob);\
            kassertobj(__ob); __magic = kobject_getmagic(__ob);\
            assertf(__magic==KOBJECT_GETMAGIC(magic),\
                    "Invalid magic in %p (%I32x != %I32x) : " #ob,\
                    __ob,__magic,KOBJECT_GETMAGIC(magic));\
            assertf(katomic_load(__ob->refcnt_member)!=0,\
                    "Invalid refcnt in %p (0) : " #ob,__ob);\
            (void)0;\
 })
#endif /* !__ASSEMBLY__ */
#else
#define KOBJECT_SIZEOFHEAD                        0
#define KOBJECT_GETMAGIC(magic)                   0
#ifndef __ASSEMBLY__
#define KOBJECT_HEAD                              /* nothing */
#define KOBJECT_INIT_IS_MEMSET_ZERO               1
#define KOBJECT_INIT(magic)                       /* nothing */
#define kobject_badmagic(ob)                      (void)0
#define kobject_init(ob,magic)                    (void)0
#define kobject_initzero(ob,magic)                (void)memset(ob,0,sizeof(*(ob)))
#define kobject_getmagic(ob)                      0
#define kobject_verify(ob,magic)                  1
#define kassert_object(ob,magic)                  (void)0
#define kassert_refobject(ob,refcnt_member,magic) (void)0
#endif /* !__ASSEMBLY__ */
#endif


#ifndef __ASSEMBLY__
//////////////////////////////////////////////////////////////////////////
// Generally useful helper macros
#define KOBJECT_DECLARE_INCREF(funname,T) \
 __crit __wunused __nonnull((1)) kerrno_t funname(T *__restrict __self)
#define KOBJECT_DECLARE_DECREF(funname,T) \
 __crit __nonnull((1)) void funname(T *__restrict __self)
#ifdef __INTELLISENSE__
#define KOBJECT_DEFINE_INCREF(funname,T,refcnt_member,kassert) \
 KOBJECT_DECLARE_INCREF(funname,T) { /*kassert(__self); __self->refcnt_member;*/ return KE_OK; }
#define KOBJECT_DEFINE_TRYINCREF(funname,T,refcnt_member,kassert) \
 KOBJECT_DECLARE_INCREF(funname,T) { /*kassert(__self); __self->refcnt_member;*/ return KE_OK; }
#define KOBJECT_DEFINE_DECREF(funname,T,refcnt_member,kassert,destroy_funname) \
 KOBJECT_DECLARE_DECREF(funname,T) { /*kassert(__self); __self->refcnt_member;*/ }
#define KOBJECT_DEFINE_INCREF_D KOBJECT_DEFINE_INCREF
#define KOBJECT_DEFINE_DECREF_D KOBJECT_DEFINE_DECREF
#else
#ifndef ktask_iscrit
extern int ktask_iscrit(void);
#endif
#define KOBJECT_DEFINE_INCREF(funname,T,refcnt_member,kassert) \
 KOBJECT_DECLARE_INCREF(funname,T) {\
  typedef __typeof__(((T *)0)->refcnt_member) __Tref;\
  __Tref __refcnt; assert(ktask_iscrit()); kassert(__self); do {\
   __refcnt = katomic_load(__self->refcnt_member);\
   assertf(__refcnt != 0,#funname "(%p) on dead " #T " object",__self);\
   if __unlikely(__refcnt == (__Tref)-1) return KE_OVERFLOW;\
  } while (!katomic_cmpxch(__self->refcnt_member,__refcnt,__refcnt+1));\
  return KE_OK;\
 }
#define KOBJECT_DEFINE_TRYINCREF(funname,T,refcnt_member,kassert) \
 KOBJECT_DECLARE_INCREF(funname,T) {\
  typedef __typeof__(((T *)0)->refcnt_member) __Tref;\
  __Tref __refcnt; assert(ktask_iscrit()); kassert(__self); do {\
   __refcnt = katomic_load(__self->refcnt_member);\
   if __unlikely(!__refcnt) return KE_DESTROYED;\
   if __unlikely(__refcnt == (__Tref)-1) return KE_OVERFLOW;\
  } while (!katomic_cmpxch(__self->refcnt_member,__refcnt,__refcnt+1));\
  return KE_OK;\
 }
#define KOBJECT_DEFINE_DECREF(funname,T,refcnt_member,kassert,destroy_funname) \
 KOBJECT_DECLARE_DECREF(funname,T) {\
  typedef __typeof__(((T *)0)->refcnt_member) __Tref;\
  extern void destroy_funname(T *);\
  __Tref __refcnt; assert(ktask_iscrit()); kassert(__self);\
  __refcnt = katomic_decfetch(__self->refcnt_member);\
  assertf(__refcnt != (__Tref)-1,#funname "(%p) on dead " #T " object",__self);\
  if __unlikely(!__refcnt) destroy_funname(__self);\
 }
#define KOBJECT_DEFINE_INCREF_D(funname,T,refcnt_member,kassert) \
 KOBJECT_DECLARE_INCREF(funname,T) {\
  typedef __typeof__(((T *)0)->refcnt_member) __Tref;\
  __Tref __refcnt; assert(ktask_iscrit()); kassert(__self); do {\
   __refcnt = katomic_load(__self->refcnt_member);\
   assertf(__refcnt != 0,#funname "(%p) on dead " #T " object",__self);\
   if __unlikely(__refcnt == (__Tref)-1) return KE_OVERFLOW;\
  } while (!katomic_cmpxch(__self->refcnt_member,__refcnt,__refcnt+1));\
  tb_printex(1);\
  return KE_OK;\
 }
#define KOBJECT_DEFINE_DECREF_D(funname,T,refcnt_member,kassert,destroy_funname) \
 KOBJECT_DECLARE_DECREF(funname,T) {\
  typedef __typeof__(((T *)0)->refcnt_member) __Tref;\
  extern void destroy_funname(T *);\
  __Tref __refcnt; assert(ktask_iscrit()); kassert(__self);\
  __refcnt = katomic_decfetch(__self->refcnt_member);\
  assertf(__refcnt != (__Tref)-1,#funname "(%p) on dead " #T " object",__self);\
  if __unlikely(!__refcnt) destroy_funname(__self);\
  tb_printex(1);\
 }
#endif
#else /* !__ASSEMBLY__ */
#define KOBJECT_DECLARE_INCREF(funname,T)                                        /* nothing */
#define KOBJECT_DECLARE_DECREF(funname,T)                                        /* nothing */
#define KOBJECT_DEFINE_INCREF(funname,T,refcnt_member,kassert)                   /* nothing */
#define KOBJECT_DEFINE_TRYINCREF(funname,T,refcnt_member,kassert)                /* nothing */
#define KOBJECT_DEFINE_DECREF(funname,T,refcnt_member,kassert,destroy_funname)   /* nothing */
#define KOBJECT_DEFINE_INCREF_D(funname,T,refcnt_member,kassert)                 /* nothing */
#define KOBJECT_DEFINE_DECREF_D(funname,T,refcnt_member,kassert,destroy_funname) /* nothing */
#endif /* __ASSEMBLY__ */

__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_OBJECT_H__ */

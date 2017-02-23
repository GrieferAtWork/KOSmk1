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
#ifndef __KOS_ATOMIC_H__
#define __KOS_ATOMIC_H__ 1

#include <kos/compiler.h>

__DECL_BEGIN

#ifndef __ASSEMBLY__
#ifndef __memory_order_defined
#define __memory_order_defined 1
typedef enum {
 memory_order_relaxed = __ATOMIC_RELAXED,
 memory_order_consume = __ATOMIC_CONSUME,
 memory_order_acquire = __ATOMIC_ACQUIRE,
 memory_order_release = __ATOMIC_RELEASE,
 memory_order_acq_rel = __ATOMIC_ACQ_REL,
 memory_order_seq_cst = __ATOMIC_SEQ_CST,
} memory_order;
#endif




#define katomic_x_load(x,order)                          __atomic_load_n(&(x),order)
#define katomic_x_store(x,v,order)                       __atomic_store_n(&(x),v,order)
#define katomic_x_xch(x,v,order)                         __atomic_exchange_n(&(x),v,order)
#define katomic_x_cmpxch(x,oldv,newv,succ,fail)          __xblock({ __typeof__(x) __oldv=(oldv); __xreturn __atomic_compare_exchange_n(&(x),&__oldv,newv,0,fail,succ); })
#define katomic_x_cmpxch_weak(x,oldv,newv,succ,fail)     __xblock({ __typeof__(x) __oldv=(oldv); __xreturn __atomic_compare_exchange_n(&(x),&__oldv,newv,1,fail,succ); })
#define katomic_x_cmpxch_val(x,oldv,newv,succ,fail)      __xblock({ __typeof__(x) __oldv=(oldv); __atomic_compare_exchange_n(&(x),&__oldv,newv,0,fail,succ); __xreturn __oldv; })
#define katomic_x_cmpxch_val_weak(x,oldv,newv,succ,fail) __xblock({ __typeof__(x) __oldv=(oldv); __atomic_compare_exchange_n(&(x),&__oldv,newv,1,fail,succ); __xreturn __oldv; })
#define katomic_x_addfetch(x,v,order)                    __atomic_add_fetch(&(x),v,order)
#define katomic_x_subfetch(x,v,order)                    __atomic_sub_fetch(&(x),v,order)
#define katomic_x_andfetch(x,v,order)                    __atomic_and_fetch(&(x),v,order)
#define katomic_x_orfetch(x,v,order)                     __atomic_or_fetch(&(x),v,order)
#define katomic_x_xorfetch(x,v,order)                    __atomic_xor_fetch(&(x),v,order)
#define katomic_x_nandfetch(x,v,order)                   __atomic_nand_fetch(&(x),v,order)
#define katomic_x_fetchadd(x,v,order)                    __atomic_fetch_add(&(x),v,order)
#define katomic_x_fetchsub(x,v,order)                    __atomic_fetch_sub(&(x),v,order)
#define katomic_x_fetchand(x,v,order)                    __atomic_fetch_and(&(x),v,order)
#define katomic_x_fetchor(x,v,order)                     __atomic_fetch_or(&(x),v,order)
#define katomic_x_fetchxor(x,v,order)                    __atomic_fetch_xor(&(x),v,order)
#define katomic_x_fetchnand(x,v,order)                   __atomic_fetch_nand(&(x),v,order)
#define katomic_x_incfetch(x,order)                      katomic_x_addfetch(x,1,order)
#define katomic_x_decfetch(x,order)                      katomic_x_subfetch(x,1,order)
#define katomic_x_fetchinc(x,order)                      katomic_x_fetchadd(x,1,order)
#define katomic_x_fetchdec(x,order)                      katomic_x_fetchsub(x,1,order)

#define katomic_load(x)                      katomic_x_load(x,__ATOMIC_SEQ_CST)
#define katomic_store(x,v)                   katomic_x_store(x,v,__ATOMIC_SEQ_CST)
#define katomic_xch(x,v)                     katomic_x_xch(x,v,__ATOMIC_SEQ_CST)
#define katomic_cmpxch(x,oldv,newv)          katomic_x_cmpxch(x,oldv,newv,__ATOMIC_SEQ_CST,__ATOMIC_SEQ_CST)
#define katomic_cmpxch_weak(x,oldv,newv)     katomic_x_cmpxch_weak(x,oldv,newv,__ATOMIC_SEQ_CST,__ATOMIC_SEQ_CST)
#define katomic_cmpxch_val(x,oldv,newv)      katomic_x_cmpxch_val(x,oldv,newv,__ATOMIC_SEQ_CST,__ATOMIC_SEQ_CST)
#define katomic_cmpxch_val_weak(x,oldv,newv) katomic_x_cmpxch_val_weak(x,oldv,newv,__ATOMIC_SEQ_CST,__ATOMIC_SEQ_CST)
#define katomic_addfetch(x,v)                katomic_x_addfetch(x,v,__ATOMIC_SEQ_CST)
#define katomic_subfetch(x,v)                katomic_x_subfetch(x,v,__ATOMIC_SEQ_CST)
#define katomic_andfetch(x,v)                katomic_x_andfetch(x,v,__ATOMIC_SEQ_CST)
#define katomic_orfetch(x,v)                 katomic_x_orfetch(x,v,__ATOMIC_SEQ_CST)
#define katomic_xorfetch(x,v)                katomic_x_xorfetch(x,v,__ATOMIC_SEQ_CST)
#define katomic_nandfetch(x,v)               katomic_x_nandfetch(x,v,__ATOMIC_SEQ_CST)
#define katomic_fetchadd(x,v)                katomic_x_fetchadd(x,v,__ATOMIC_SEQ_CST)
#define katomic_fetchsub(x,v)                katomic_x_fetchsub(x,v,__ATOMIC_SEQ_CST)
#define katomic_fetchand(x,v)                katomic_x_fetchand(x,v,__ATOMIC_SEQ_CST)
#define katomic_fetchor(x,v)                 katomic_x_fetchor(x,v,__ATOMIC_SEQ_CST)
#define katomic_fetchxor(x,v)                katomic_x_fetchxor(x,v,__ATOMIC_SEQ_CST)
#define katomic_fetchnand(x,v)               katomic_x_fetchnand(x,v,__ATOMIC_SEQ_CST)
#define katomic_incfetch(x)                  katomic_x_incfetch(x,__ATOMIC_SEQ_CST)
#define katomic_decfetch(x)                  katomic_x_decfetch(x,__ATOMIC_SEQ_CST)
#define katomic_fetchinc(x)                  katomic_x_fetchinc(x,__ATOMIC_SEQ_CST)
#define katomic_fetchdec(x)                  katomic_x_fetchdec(x,__ATOMIC_SEQ_CST)
#endif /* !__ASSEMBLY__ */

__DECL_END

#endif /* !__KOS_ATOMIC_H__ */

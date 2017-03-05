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
#include "shm.c"
#define UNLOCKED
__DECL_BEGIN
#endif

#ifdef UNLOCKED
#define F(x)                        x##_unlocked
#define M(x) __crit __nomp kerrno_t x##_unlocked
#else
#define F(x)                        x
#define M(x) __crit        kerrno_t x
#endif

M(kshm_mapram)(struct kshm *__restrict self,
        /*out*/__pagealigned __user void **__restrict user_address,
               size_t pages, __pagealigned __user void *hint, kshm_flag_t flags)
{
 struct kshmregion *region;
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kshm(self);
 assert(pages != 0);
 region = kshmregion_newram(pages,flags);
 if __unlikely(!region) return KE_NOMEM;
 error = F(kshm_mapautomatic_inherited)(self,user_address,region,hint,
                                        0,region->sre_chunk.sc_pages);
 if __unlikely(KE_ISERR(error)) kshmregion_decref_full(region);
 return error;
}
M(kshm_mapram_linear)(struct kshm *__restrict self,
               /*out*/__pagealigned __kernel void **__restrict kernel_address,
               /*out*/__pagealigned __user   void **__restrict user_address,
                      size_t pages, __pagealigned __user void *hint, kshm_flag_t flags) {
 struct kshmregion *region;
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kshm(self);
 kassertobj(kernel_address);
 assert(pages != 0);
 region = kshmregion_newlinear(pages,flags);
 if __unlikely(!region) return KE_NOMEM;
 assert(region->sre_chunk.sc_partc == 1);
 *kernel_address = region->sre_chunk.sc_partv[0].sp_frame;
 error = F(kshm_mapautomatic_inherited)(self,user_address,region,hint,
                                        0,region->sre_chunk.sc_pages);
 if __unlikely(KE_ISERR(error)) kshmregion_decref_full(region);
 return error;
}
M(kshm_mapphys)(struct kshm *__restrict self,
                __pagealigned __kernel void *__restrict phys_address,
         /*out*/__pagealigned __user void **__restrict user_address,
                size_t pages, __pagealigned __user void *hint, kshm_flag_t flags) {
 struct kshmregion *region;
 kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kshm(self);
 kassertobj(user_address);
 assert(pages != 0);
 region = kshmregion_newphys(phys_address,pages,flags);
 if __unlikely(!region) return KE_NOMEM;
 assert(region->sre_chunk.sc_partc             == 1);
 assert(region->sre_chunk.sc_partv[0].sp_frame == phys_address);
 error = F(kshm_mapautomatic_inherited)(self,user_address,region,hint,
                                        0,region->sre_chunk.sc_pages);
 if __unlikely(KE_ISERR(error)) kshmregion_decref_full(region);
 return error;
}

#undef F
#undef M

#ifdef UNLOCKED
#undef UNLOCKED
#endif

#ifdef __INTELLISENSE__
__DECL_END
#endif


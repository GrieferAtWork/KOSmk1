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
#ifndef __KOS_KERNEL_TASK2_C__
#define __KOS_KERNEL_TASK2_C__ 1

#include <kos/config.h>
#include <kos/compiler.h>
#include <kos/kernel/task2.h>
#include <kos/kernel/task2-cpu.h>

__DECL_BEGIN

         __size_t      kcpu2_total;
__atomic __size_t      kcpu2_enabled;
__atomic __size_t      kcpu2_online;
__atomic __size_t      kcpu2_offline;
         struct kcpu2  kcpu2_vector[KCPU2_MAXCOUNT];
         struct kcpu2 *kcpu2_bootcpu;
__atomic struct kcpu2 *kcpu2_hwirq;

#define ID(x) ((x)-kcpu2_vector)

void kernel_initialize_cpu(void);
void kernel_initialize_scheduler(void);
void kernel_finalize_scheduler(void);


__crit void
ktask2_destroy(struct ktask2 *__restrict self) {
 kassert_object(self,KOBJECT_MAGIC_TASK2);
 assert(self->t_state != KTASK2_STATE_RUNNING);
 if (self->t_flags&KTASK2_FLAG_CRITICAL) {
  /* TODO: Special case: Revive critical task after being
  *                      detached while waiting/suspended. */
 }
 /* TODO: Destroy the task. */
 free(self);
}


struct kcpu2 *kcpu2_getoffline(void) {
 struct kcpu2 *result;
 KCPU2_FOREACH_ALL(result) {
  if ((katomic_load(result->c_flags)&(KCPU2_FLAG_PRESENT|KCPU2_FLAG_ENABLED|
                                      KCPU2_FLAG_INUSE|KCPU2_FLAG_TURNOFF|
                                      KCPU2_FLAG_STARTED|KCPU2_FLAG_RUNNING)) ==
                                     (KCPU2_FLAG_PRESENT|KCPU2_FLAG_ENABLED)
      ) return result;
 }
 return NULL;
}
struct kcpu2 *kcpu2_getoffline_maybe_disabled(void) {
 struct kcpu2 *result;
 KCPU2_FOREACH_ALL(result) {
  if ((katomic_load(result->c_flags)&(KCPU2_FLAG_PRESENT|
                                      KCPU2_FLAG_INUSE|KCPU2_FLAG_TURNOFF|
                                      KCPU2_FLAG_STARTED|KCPU2_FLAG_RUNNING)) ==
                                     (KCPU2_FLAG_PRESENT)
      ) return result;
 }
 return NULL;
}


__DECL_END

#ifndef __INTELLISENSE__
#include "task2-apic.c.inl"
#include "task2-cpupower.c.inl"
#include "task2-cpusched.c.inl"
#include "task2-cpustack.c.inl"
#include "task2-cputoggle.c.inl"
#include "task2-idle.c.inl"
#include "task2-preempt.c.inl"
#include "task2-state.c.inl"
#include "task2-unsched.c.inl"
#include "task2-yield.c.inl"
#endif

#endif /* !__KOS_KERNEL_TASK2_C__ */

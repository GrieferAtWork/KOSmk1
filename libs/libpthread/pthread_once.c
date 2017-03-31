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
#ifndef __PTHREAD_ONCE_C__
#define __PTHREAD_ONCE_C__ 1

#include <errno.h>
#include <kos/atomic.h>
#include <pthread.h>

__DECL_BEGIN

__public int
pthread_once(pthread_once_t *__restrict once_control,
             void (*init_routine)(void)) {
 int oldval;
 if (!once_control || !init_routine) return EINVAL;
 oldval = katomic_cmpxch_val(once_control->__po_done,0,-1);
 if (!oldval) {
  (*init_routine)();
  katomic_store(once_control->__po_done,1);
 } else while (oldval < 0) {
  pthread_yield();
  oldval = katomic_load(once_control->__po_done);
 }
 return 0;
}

__DECL_END

#endif /* !__PTHREAD_ONCE_C__ */

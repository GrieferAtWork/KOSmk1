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
#ifndef __PTHREAD_KEY_C__
#define __PTHREAD_KEY_C__ 1

#include <errno.h>
#include <kos/atomic.h>
#include <proc.h>
#include <pthread.h>
#include <stdint.h>
#include <stddef.h>

__DECL_BEGIN

static ptrdiff_t     pthread_tlsoffset = 0;
static pthread_key_t pthread_tlsnextkey = 0;

__public int
pthread_key_create(pthread_key_t *key, void (*destr_function)(void *)) {
 if __unlikely(!pthread_tlsoffset) {
  ptrdiff_t newp,oldp;
  newp = tls_alloc(NULL,__SIZEOF_POINTER__); /* TODO: Size */
  if __unlikely(!newp) return -errno;
  oldp = katomic_cmpxch_val(pthread_tlsoffset,0,newp);
  if (oldp) tls_free(newp);
 }
 /* TODO: 'destr_function'. */
 *key = katomic_fetchadd(pthread_tlsnextkey,__SIZEOF_POINTER__);
 return 0;
}
__public int pthread_key_delete(pthread_key_t key) { return 0; /* TODO */ }
__public void *pthread_getspecific(pthread_key_t key) { return tls_getp(pthread_tlsoffset+key); }
__public int pthread_setspecific(pthread_key_t key, void const *pointer) { tls_putp(pthread_tlsoffset+key,(void *)pointer); return 0; }

__DECL_END

#endif /* !__PTHREAD_KEY_C__ */

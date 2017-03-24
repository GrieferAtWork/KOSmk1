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

#include "helper.h"

/* Reminder: This file gets compiler with GCC. - Not VC/VC++! */
#include <assert.h>
#include <exception.h>
#include <malloc.h>
#include <mod.h>
#include <proc.h>
#include <traceback.h>


TEST(exceptions) {
 /* Here to ensure all registers are either dead, or preserved. */
 register int register_variable = 42;
 //tb_print();
 char *p = (char *)0xdeadbeef;
 for (;;) {
  assert(register_variable == 42);
  *p++ = '\xAA';
 }
 __try {
  __try {
   /* Time to mess things up! */
   char *p = (char *)0xdeadbeef;
   for (;;) { assert(register_variable == 42); *p++ = '\xAA'; }
  } __finally {
   outf("In finally\n");
  }
 } __except (exc_code() == KEXCEPTION_SEGFAULT) {
#if 1
  exc_tbprint(0);
#else
  syminfo_t *syminfo; void *eip;
  assert(register_variable == 42);
  assert(exc_current->ex_info&KEXCEPTIONINFO_SEGFAULT_WRITE);
  assert(!(exc_current->ex_info&KEXCEPTIONINFO_SEGFAULT_INSTR_FETCH));
  assertf(exc_current->ex_ptr[0] == (void *)0xdeadbeef,"%p",exc_current->ex_ptr[0]);
  tb_printeip((void *)tls_self->u_exstate.eip);
  tb_printebp((void *)tls_self->u_exstate.ebp);
  eip = (void *)tls_self->u_exstate.eip;
  /* Figure out some symbol information about the EIP. */
  syminfo = mod_addrinfo(MOD_ALL,eip,NULL,0,MOD_SYMINFO_NONE);
  assert(syminfo);
  assert(syminfo->si_base == (void *)&NAME(exceptions));
  free(syminfo);
#endif
  assert(register_variable == 42);
 }
 assert(register_variable == 42);
}

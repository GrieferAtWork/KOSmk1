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
#include <malloc.h>
#include <mod.h>
#include <string.h>

typedef void (*test_t)(void);
static void run_test(test_t test) {
 syminfo_t *sym = mod_addrinfo(MOD_SELF,test,NULL,0,MOD_SYMINFO_ALL);
 char *name = sym ? sym->si_name : NULL;
 if (name && !memcmp(name,"_test__",7)) name += 7;
 printf("\033[32m");
 outf("%s(%d) : %s : %p : Running test\n",
      sym ? sym->si_file : NULL,
      sym ? sym->si_line+1 : 0,name,test);
 free(sym);
 printf("\033[m");
 (*test)();
}


int main(void) {
#define RUN(name) { extern TEST(name); run_test(&NAME(name)); }
 RUN(exceptions);
 RUN(quote);
 RUN(tls);
 RUN(unaligned);
#undef RUN
 return 0;
}

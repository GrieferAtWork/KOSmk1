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

#include <kos/compiler.h>
#include <lib/libc.h>
#include <stdlib.h>

__DECL_BEGIN

extern int main(int argc, char *argv[], char *envp[]);

extern char **environ;

#ifdef __PE__
/* Windows strangeness causes main() to emit a call to a function named '__main()'.
 * While I'm not sure _why_ someone chose to do this at some point, we still have
 * to supply the linker with ~something~ to call...
 * NOTE: I am aware that it's to do with initialization, but
 *       why do it this way? - And even worse: hard-code it?
 * Reference: http://wiki.osdev.org/User:Combuster/Alloca */
void __main(void) {}
#endif

void _start(void) {
 struct __libc_args av;

 /* Initialize lib-c. */
 __libc_init();

 /* Load commandline arguments. */
 __libc_get_argv(&av);

 /* Execute the hosted C-main() function, and exit. */
 exit(main((int)av.__c_argc,
                av.__c_argv,
           environ));

 /* We should never get here! */
 __compiler_unreachable();
}

__DECL_END


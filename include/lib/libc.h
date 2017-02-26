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
#ifndef __LIB_LIBC_H__
#define __LIB_LIBC_H__ 1

#include <kos/compiler.h>
#include <kos/types.h>

__DECL_BEGIN

struct __libc_args {
 __size_t __c_argc;
 char   **__c_argv;
};

/* Free dynamic memory allocated by '__libc_get_argv', theoretically
 * allowing code outside of lib_start to re-load the original argument vector.
 * NOTE: Internally, the argv text is allocated as a single continuous
 *       chunk of text residing in another allocated vector.
 *       With that in mind, we only need to free the first argument and the vector itself. */
#define __libc_free_argv(a) \
 ((a)->__c_argv ? (free((a)->__c_argv[0]),free((a)->__c_argv)) : (void)0)

/* Called after '__libc_init', this function performs the necessary
 * system calls to retrieve the arguments the calling process was executed
 * with, storing the information in a dynamic buffer that is not tracked
 * by mall and must therefor not be freed to prevent memory leaks during exit(). */
extern void __libc_get_argv __P((struct __libc_args *__a));

/* Called before main(): Initializes global variables such as environ
 *                       and generates a random seed for mall & dlmalloc.
 * NOTE: Failing to call this function before main(), or more
 *       than once causes undefined behavior at any point.
 * NOTE: When libc was compiled in debug-mode, this function will also
 *       register hooks used to dump memory leaks during exit(). */
extern void __libc_init __P((void));

__DECL_END

#endif /* !__LIB_LIBC_H__ */

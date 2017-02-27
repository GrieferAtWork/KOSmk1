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
#ifndef __DLFCN_H__
#ifndef _DLFCN_H
#define __DLFCN_H__ 1
#define _DLFCN_H 1

#include <kos/compiler.h>

#ifndef __ASSEMBLY__
__DECL_BEGIN

#define RTLD_NEXT	    ((void *)-1l)
#define RTLD_DEFAULT	 ((void *)0)

#define RTLD_NOW    0x00002
#define RTLD_LAZY   0x00001
#define RTLD_GLOBAL 0x00100
#define RTLD_LOCAL  0

extern void *_dlfopen __P((int __fd, int __mode));
#ifndef __STDC_PURE__
#ifndef __NO_asmname
extern void *dlfopen __P((int __fd, int __mode)) __asmname("_dlfopen");
#else
#   define dlfopen _dlfopen
#endif
#endif

extern void *dlopen __P((char const *__restrict __file, int __mode));
extern int   dlclose __P((void *__handle));
extern void *dlsym __P((void *__restrict __handle, char const *__restrict __name));
extern char *dlerror __P((void));

typedef struct {
    /* Taken from: https://linux.die.net/man/3/dladdr */
    char const *dli_fname; /*< Pathname of shared object that contains address. */
    void       *dli_fbase; /*< Address at which shared object is loaded. */
    char const *dli_sname; /*< Name of nearest symbol with address lower than addr. */
    void       *dli_saddr; /*< Exact address of symbol named in dli_sname. */
} Dl_info;

extern int dladdr __P((void *__addr, Dl_info *__info));

#if 0 /* TODO */
//extern void *dlvsym(void *handle, char *symbol, char *version); // ???
#endif

__DECL_END
#endif /* !__ASSEMBLY__ */

#endif /* !_DLFCN_H */
#endif /* !__DLFCN_H__ */

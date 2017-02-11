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
#ifndef __GETOPT_H__
#ifndef _GETOPT_H
#define __GETOPT_H__ 1
#define _GETOPT_H 1

#include <kos/compiler.h>
#ifndef __ASSEMBLY__
__DECL_BEGIN

struct option {
    char const *name;
    int         has_arg;
    int        *flag;
    int         val;
};

#define no_argument       0
#define required_argument 1
#define optional_argument 2

// newlib names
#define NO_ARG       no_argument
#define REQUIRED_ARG required_argument
#define OPTIONAL_ARG optional_argument

extern char *optarg;
extern int   optind;
extern int   opterr;
extern int   optopt;

extern int getopt __P((int __argc,char *__argv[], const char *__shortopts));
extern int getopt_long __P((int __argc, char *__argv[], const char *__shortopts, const struct option *__longopts, int *__longind));
extern int getopt_long_only __P((int __argc, char *__argv[], const char *__shortopts, const struct option *__longopts, int *__longind));

__DECL_END
#endif /* !__ASSEMBLY__ */

#endif /* !_GETOPT_H */
#endif /* !__GETOPT_H__ */

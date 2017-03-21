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
#ifndef __BUILTIN_H__
#define __BUILTIN_H__ 1

#include <stddef.h>
#include <kos/compiler.h>

#define CONFIG_HAVE_IO 1

struct shcmd {
 char const *name; /*< [1..1][oef(NULL)] CMD name, or NULL for end of list. */
 size_t      size; /*< strlen of 'name'. */
 int       (*main)(int argc, char *argv[]); /*< Command main() function. */
};
#define SHCMD(name,main) {name,__COMPILER_STRINGSIZE(name),main}

/* List of builtin commands. */
extern struct shcmd const shbuiltin[];

extern int builtin_cd(int argc, char *argv[]);
extern int builtin_exit(int argc, char *argv[]);
extern int builtin_color(int argc, char *argv[]);
extern int builtin_sand(int argc, char *argv[]);
extern int builtin_rootfork(int argc, char *argv[]);

#if CONFIG_HAVE_IO
extern int builtin_sys_open(int argc, char *argv[]);
extern int builtin_sys_open2(int argc, char *argv[]);
extern int builtin_sys_close(int argc, char *argv[]);
extern int builtin_sys_read(int argc, char *argv[]);
extern int builtin_sys_seek(int argc, char *argv[]);
extern int builtin_sys_write(int argc, char *argv[]);
extern int builtin_sys_dup(int argc, char *argv[]);
extern int builtin_sys_dup2(int argc, char *argv[]);
#endif /* CONFIG_HAVE_IO */


#endif /* !__BUILTIN_H__ */

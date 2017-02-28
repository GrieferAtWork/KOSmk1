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
#ifndef __SH_H__
#define __SH_H__ 1

extern int   verbose;          /* Log every system()-style call. */
extern int   interactive;
extern int   restricted_shell;
extern int   sani_descriptors; /*< Do descriptor sanitization, as described in unistd.h:_isafile */
extern int   fork_sandlevel;   /*< (0..4) Sandbox-level to put spawned tasks into. */
extern char *prompt;

extern void term_setunbuffered(void);
extern void term_setbuffered(void);

extern int exec_fork(char *exe, char *argv[]);
extern char **split_argv(char *cmd);
extern int exec_unistd(char *exe, char *argv[]);
extern int exec_system(char *cmd);
extern int joinproc(int p);
extern int do_system(char *cmd);

extern void update_prompt(void);
extern void usage(int fd, char *name);

#endif /* !__SH_H__ */

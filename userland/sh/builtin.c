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
#ifndef __BUILTIN_C__
#define __BUILTIN_C__ 1

#include "sh.h"
#include "builtin.h"
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

struct shcmd const shbuiltin[] = {
 SHCMD("cd",&builtin_cd),
 SHCMD("exit",&builtin_exit),
 SHCMD("color",&builtin_color),
 SHCMD("sand",&builtin_sand),
 SHCMD("rootfork",&builtin_rootfork),
#if CONFIG_HAVE_IO
 SHCMD("sys.open",&builtin_sys_open),
 SHCMD("sys.open2",&builtin_sys_open2),
 SHCMD("sys.close",&builtin_sys_close),
 SHCMD("sys.read",&builtin_sys_read),
 SHCMD("sys.seek",&builtin_sys_seek),
 SHCMD("sys.write",&builtin_sys_write),
 SHCMD("sys.dup",&builtin_sys_dup),
 SHCMD("sys.dup2",&builtin_sys_dup2),
#endif /* CONFIG_HAVE_IO */
 {NULL,0,NULL}
};
#include <stdint.h>
#include <proc.h>


//////////////////////////////////////////////////////////////////////////
// Builtin shell commands
int builtin_cd(int argc, char *argv[]) {
 char buf[256],*newpwd,*oldpwd,*cwd;
 if (restricted_shell) {
  dprintf(STDERR_FILENO,"cd: restricted\n");
  return EXIT_FAILURE;
 }
 newpwd = argc >= 2 ? argv[1] : getenv("HOME");
 if (!newpwd) {
  dprintf(STDERR_FILENO,"cd: Failed to find HOME\n");
  return EXIT_FAILURE;
 }
 if (!strcmp(newpwd,"-")) {
  if ((oldpwd = getenv("OLDPWD")) == NULL) {
   dprintf(STDERR_FILENO,"cd: OLDPWD not set\n");
   return EXIT_FAILURE;
  }
  newpwd = oldpwd;
 }
 cwd = getcwd(buf,sizeof(buf)) ? buf : getcwd(NULL,0);
 setenv("OLDPWD",cwd,1);
 if (cwd != buf) free(cwd);

 if (chdir(newpwd) == -1) {
  char *msg = strdupf("cd : %s",newpwd);
  perror(msg ? msg : newpwd);
  free(msg);
  return EXIT_FAILURE;
 }
 update_prompt();
 return EXIT_SUCCESS;
}

int builtin_sand(int argc, char *argv[]) {
 int newlevel;
 /* Set the sandbox level. */
 if (argc <= 1) {
  dprintf(STDOUT_FILENO,"%d\n",fork_sandlevel);
  return fork_sandlevel;
 }
 newlevel = atoi(argv[1]);
 if (newlevel < 0 || newlevel > 4) {
  dprintf(STDERR_FILENO
         ,"Invalid level %d (Must be one of 0,1,2,3 or 4)\n"
         ,newlevel);
  return EXIT_FAILURE;
 } else {
  fork_sandlevel = newlevel;
 }
 return EXIT_SUCCESS;
}

int builtin_rootfork(int argc, char *argv[]) {
 uintptr_t exitcode;
 /* Do a return fork. When successful, this will
  * drop the user into a copy of the shell that has
  * root privileges. The original shell will continue
  * running, but will be join the root shell once it terminates.
  * Other than this join, (which cannot be traced) there is no
  * way of accessing the root shell from the original one.
  * NOTE: Normally, this should always fail with EACCES
  *       because /bin/sh shouldn't have the SETUID bit set.
  *       But this command's current main purpose is to test
  *       the entire root-fork system in an easy-to-test way.
  */
 if (task_rootfork(&exitcode) == -1) {
  perror("task_rootfork");
  return EXIT_FAILURE;
 }
 return (int)exitcode;
}


int builtin_exit(int argc, char *argv[]) {
 _exit(argc >= 2 ? atoi(argv[1]) : 0);
}


#include <proc.h>
#include <kos/syslog.h>

static ptrdiff_t offset;

static void *thread_test(void *closure) {
 k_syslogf(KLOG_ERROR,"thread #2: tls_addr = %p\n",tls_addr);
 k_syslogf(KLOG_ERROR,"thread #2: %q\n",(char *)(tls_addr+offset));
 memset(tls_addr+offset,0,strlen((char *)(tls_addr+offset)));
 return closure;
}

int builtin_color(int argc, char *argv[]) {
#if 1
 static char const template_[] = "\xAA\xAA\xAA\xAATLS TEMPLATE STRING";
 offset = tls_alloc(template_,sizeof(template_));
 if (!offset) perror("tls_alloc");
 else {
  int t;
  k_syslogf(KLOG_ERROR,"offset = %Id\n",offset);
  k_syslogf(KLOG_ERROR,"thread #1: tls_addr = %p\n",tls_addr);
  k_syslogf(KLOG_ERROR,"thread #1: %q\n",(char *)(tls_addr+offset));
  t = task_newthread(&thread_test,NULL,TASK_NEWTHREAD_DEFAULT);
  if (t == -1) perror("task_newthread");
  else task_join(t,NULL),close(t);
  if (tls_free(offset) == -1) perror("tls_free");
 }

#else
 dprintf(STDOUT_FILENO,
         "\033[48;5;0;37m00\033[48;5;1;30m01\033[48;5;2;30m02\033[48;5;3;30m03\033[48;5;4;37m04\033[48;5;5;30m05\033[48;5;6;30m06\033[48;5;7;30m07\n"
         "\033[48;5;8;30m08\033[48;5;9;30m09\033[48;5;10;30m0a\033[48;5;11;30m0b\033[48;5;12;30m0c\033[48;5;13;30m0d\033[48;5;14;30m0e\033[48;5;15;30m0f\033[0m\n");
#endif
 return EXIT_SUCCESS;
}

#endif /* !__BUILTIN_C__ */

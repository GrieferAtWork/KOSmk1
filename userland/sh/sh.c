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
#ifndef __SH_C__
#define __SH_C__ 1

#include "sh.h"
#include "cmd.h"
#include "builtin.h"
#include <errno.h>
#include <proc.h>
#include <lib/cmd.h>
#include <lib/rline.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <string.h>
#include <kos/mod.h>
#include <kos/syslog.h>

int verbose = 0; /* Log every system()-style call. */
int errorlevel = 0; /* $? */
int interactive = 0;
int restricted_shell = 0;
int sani_descriptors = 0;
int fork_sandlevel = 0;
struct termios oldios;
char *prompt = NULL;

void term_setunbuffered(void) {
 struct termios newios;
 if (tcgetattr(STDIN_FILENO,&oldios) == -1) perror("tcgetattr");
 memcpy(&newios,&oldios,sizeof(struct termios));
 newios.c_lflag &= ~(ICANON|ECHO);
 if (tcsetattr(STDIN_FILENO,TCSAFLUSH,&newios) == -1) perror("tcsetattr");
}
void term_setbuffered(void) {
 if (tcsetattr(STDIN_FILENO,TCSAFLUSH,&oldios) == -1) perror("tcsetattr");
}

int exec_fork(char *exe, char **argv) {
 int childfd;
 if ((childfd = task_fork()) == 0) {
  /* Set the child process as CTRL+C-termination target.
   * For now, this is KOS's shortcut to killing processes, until
   * I've gotten around to properly implement posix signals.
   */
  tcsetpgrp(STDIN_FILENO,getpid());
  /* Force-sanitize descriptors, if wanted to. */
  if (sani_descriptors) {
   if (!isafile(0)) close(0);
   if (!isafile(1)) close(1);
   if (!isafile(2)) close(2);
   closeall(3,INT_MAX);
  }
  /* Enter sandbox-mode, if wanted to. */
  if (fork_sandlevel) {
   proc_barrier(KSANDBOX_BARRIERLEVEL(fork_sandlevel));
  }
  execvp(exe,argv);
  perror(exe);
  _exit(errno);
 }
 if (childfd == -1) {
  perror("task_fork");
  return -1;
 }
 return joinproc(childfd);
}

#define ARGV_BUFSIZE   6
char **split_argv(char *cmd) {
 int in_quote = 0;
 char *iter = cmd,*arg_start = cmd;
 size_t result_size = ARGV_BUFSIZE;
 char **result = (char **)malloc((ARGV_BUFSIZE+1)*sizeof(char *));
 char **res_iter,**res_end,**newresult;
 if (!result) return NULL;
 res_end = (res_iter = result)+ARGV_BUFSIZE;
 for (;;) {
  if (!*iter || (!in_quote && *iter == ' ' && iter[-1] != '\\')) {
   if (arg_start != iter) {
    if (res_iter == res_end) {
     size_t newsize = result_size*2;
     newresult = (char **)realloc(result,(newsize+1)*sizeof(char *));
     if __unlikely(!newresult) { free(result); return NULL; }
     result = newresult;
     result_size = newsize;
    }
    *res_iter++ = arg_start;
    if (!*iter) break;
   }
   arg_start = iter+1;
   *iter = '\0';
  } else if (in_quote <= 1 && *iter == '"') {
   in_quote = !in_quote;
  } else if (*iter == '\'') {
   in_quote = in_quote == 2 ? 0 : 2;
  }
  ++iter;
 }
 *res_iter = NULL;
 return result;
}

int exec_unistd(char *exe, char **argv) {
 struct shcmd const *iter = shbuiltin;
 while (iter->name) {
  if (!strcmp(iter->name,exe)) {
   char **argend = argv;
   while (*argend) ++argend;
   return (*iter->main)(argend-argv,argv);
  }
  ++iter;
 }
 return exec_fork(exe,argv);
}
int exec_system(char *cmd) {
 int result; char **argv;
 if (verbose) dprintf(STDERR_FILENO,"exec: %q\n",cmd);
 argv = split_argv(cmd);
 if (!argv) return (errorlevel = errno);
 result = exec_unistd(cmd,argv);
 free(argv);
 return result;
}
int joinproc(int p) {
 void *exitcode; kerrno_t error;
 if (p == -1) return (errorlevel = errno);
 error = ktask_join(p,&exitcode,KTASKOPFLAG_RECURSIVE);
 close(p);
 if (KE_ISERR(error)) {
  errorlevel = errno = error;
  perror("ktask_join");
  return errorlevel;
 }
 errorlevel = (int)(uintptr_t)exitcode;
 return errorlevel;
}


int do_system(char *cmd) {
#if 1
 return (int)shcmd_system(cmd,strlen(cmd));
#else
 // TODO: Logic operator: cmda && cmdb, cmda || cmdb, !cmda
 // TODO: Expand environment variables: $FOO, ${FOO}
 // TODO: Run and insert stdout of process: `echo foo`, $(echo foo)
 // TODO: Expand '*' paths: "/dir/f*" --> "/dir/file1 /dir/file2 /dir/file42"
 // TODO: Redirect/duplicate descriptors: cmd > foo.txt, cmd < bar.txt, cmd 2>&1
 return exec_system(cmd);
#endif
}




void update_prompt(void) {
 if (interactive) {
  char buf[256];
  char *cwd = getcwd(buf,sizeof(buf)) ? buf : getcwd(NULL,0);
  free(prompt);
  prompt = strdupf("kos:%s# ",cwd);
  if (cwd != buf) free(cwd);
 }
}


void usage(int fd, char *name) {
 dprintf(fd
        ,"USAGE: %s [-irvS] [-s LV] [-c CMD]\n"
         "\t-c|--command CMD   Parse, execute and exit with CMD.\n"
         "\t-i|--interactive   Create an interactive shell. (default)\n"
         "\t-r|--restricted    Create a restricted shell.\n"
         "\t-v|--verbose       Verbose output every executed command.\n"
         "\t-S|--safe          Enable safe-descriptor mode, as\n"
         "\t                   described in unistd.h:_isafile.\n"
         "\t-s|--sandbox LV    Exec new tasks in a LV-level sandbox. (default: 0)\n"
         "\t-h|--help          Show this help and exit\n"
        ,name);
}

static struct option const longopts[] = {
 {"command",required_argument,NULL,'c'},
 {"interactive",no_argument,NULL,'i'},
 {"restricted",no_argument,NULL,'r'},
 {"verbose",no_argument,NULL,'v'},
 {"safe",no_argument,NULL,'S'},
 {"sandbox",required_argument,NULL,'s'},
 {"help",no_argument,NULL,'h'},
 {NULL,0,NULL,0}
};

__attribute_thread int foo = 42;
__attribute_thread int bar = 84;

#include <mod.h>
void addrinfo(void *addr) {
 syminfo_t *info; char *modname;
 info = mod_addrinfo(MOD_ALL,addr,NULL,0,MOD_SYMINFO_ALL);
 if (!info) perror("mod_addrinfo");
 else {
  modname = mod_path(info->si_modid,NULL,0);
  printf("%s(%u) : %q : %q+%Ix : Here\n",
         info->si_file,info->si_line,modname,info->si_name,
        (uintptr_t)addr-(uintptr_t)info->si_base);
  free(modname);
  free(info);
 }
}

int main(int argc, char *argv[]) {
 int error; struct rline *r; int optc;
 addrinfo(&addrinfo);
 // movl	%gs:0, %eax
 // movl	foo@ntpoff(%eax), %eax
 //int x = foo;
 //int y = bar;
 //printf("thread_local %d %d\n",x,y);

 while ((optc = getopt_long(argc,argv,"c:irvSs:h",longopts,NULL)) != -1) {
  switch (optc) {
   case 'c': _exit(do_system(optarg)); break;
   case 'i': /* ... Why dou? */ break;
   case 'r': restricted_shell = 1; break;
   case 'v': verbose = 1; break;
   case 'S': sani_descriptors = 1; break;
   case 's': fork_sandlevel = atoi(optarg); break;
   case 'h': usage(STDERR_FILENO,argv[0]); _exit(EXIT_SUCCESS); break;
   case '?': usage(STDERR_FILENO,argv[0]); _exit(EXIT_FAILURE); break;
   default: break;
  }
 }

 // TODO: Tab-auto-complete
 r = rline_new(NULL,NULL,NULL,STDIN_FILENO,STDOUT_FILENO);
 if (!r) {
  perror("Failed to create rline");
  _exit(EXIT_FAILURE);
 }

 interactive = 1;
 update_prompt();

 /* Do the interactive loop thingy! */
 for (;;) {
  char *text;
  term_setunbuffered();
  error = rline_read(r,prompt);
  term_setbuffered();
  if (error != RLINE_OK) break;
  text = rline_text(r);
  /* Remove a trailing linefeed */
  if (rline_size(r) && text[rline_size(r)-1] == '\n'
      ) text[rline_size(r)-1] = '\0';
  if (!*text) continue; /* Ignore empty lines */
  if ((text = strdup(text)) != NULL) {
   /* Do a system-style exec */
   do_system(text);
   free(text);
  } else {
   do_system(rline_text(r));
  }
 }
 cmd_engine_quit(&shcmd_engine);
 rline_delete(r);
 free(prompt);
 return error == RLINE_EOF ? EXIT_SUCCESS : EXIT_FAILURE;
}


#endif /* !__SH_C__ */

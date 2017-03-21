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
#ifndef __INIT_C__
#define __INIT_C__ 1

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <stdlib.h>
#include <proc.h>
#include <errno.h>
#include <fcntl.h>
#include <kos/keyboard.h>
#include <kos/syslog.h>
#include <kos/perm.h>

extern char **environ;

static perm_t const default_permissions[] = {
  /* Set up some reasonable limits. */
  {{KPERM_NAME_PIPE_MAX},  { .d_size = 0x00001000 }},
  {{KPERM_NAME_MAXMEM_RW}, { .d_size = 0x01000000 }},
  {{KPERM_NAME_ENV_MEMMAX},{ .d_size = 0x00001000 }},
  {{KPERM_NAME_MAXTHREADS},{ .d_size = 128 }},
  {{KPERM_NAME_FDMAX},     { .d_uint = 512 }},
  {{KPERM_NAME_SYSLOG},    { .d_int  = KLOG_ERROR }},
  {{KPERM_NAME_FLAG},      { .d_flag = KPERM_FLAG_CHTIME }},
//{{KPERM_NAME_FLAG},      { .d_flag = KPERM_FLAG_CHDIR }}, // TEST ONLY (make chdir illegal)
};


int main(int argc, char *argv[]) {

 /* Set-up some default environment variables */
 setenv("LD_LIBRARY_PATH","/lib:/usr/lib",0);
 setenv("PATH","/bin:/usr/bin:/usr/local/bin",0);
 setenv("USER","root",0);
 setenv("HOME","/",0);

 /* Set initial permission restrictions.
  * After this, many resources will no longer be unlimited
  * for this process and everything it will ever spawn.
  * >> To escape from this, a rootfork() must be performed. */
 proc_setpermex(default_permissions,__COMPILER_ARRAYSIZE(default_permissions));

 /* Launch a proper terminal */
 if (open2(STDIN_FILENO,"/dev/kbevent",O_RDONLY) == -1) perror("open2('/dev/kbevent')");
 execl("/bin/terminal-vga","terminal-vga","/bin/sh",(char *)NULL);
 k_syslogf(KLOG_ERROR,"Failed to exec terminal: %d: %s\n",errno,strerror(errno));

 /* Fallback: Try to start a shell directly (this is bad...) */
 if (open2(STDIN_FILENO,"/dev/kbtext",O_RDONLY) == -1) perror("open2('/dev/kbtext')");
 if (open2(STDOUT_FILENO,"/dev/tty",O_WRONLY) == -1) perror("open2('/dev/tty')");
 dup2(STDOUT_FILENO,STDERR_FILENO);
 execl("/bin/sh","sh",(char *)NULL);

 perror("FAILED TO LAUNCH STARTUP BINARY");
 return EXIT_FAILURE;
}

#endif /* !__INIT_C__ */

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
#ifndef __BUILTIN_SYSIO_C__
#define __BUILTIN_SYSIO_C__ 1

#include "builtin.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#if CONFIG_HAVE_IO
int builtin_sys_open(int argc, char *argv[]) {
 int result,perms,fmode = 0;
 char *mode = argc >= 3 ? argv[2] : "r";
 if (argc < 2) { dprintf(STDERR_FILENO,"usage: open FILENAME [mode=r] [perms=0644]\n"); return -1; }
      if (*mode == 'r') fmode = O_RDONLY,++mode;
 else if (*mode == 'w') fmode = O_WRONLY|O_CREAT|O_TRUNC,++mode;
 if (*mode == '+') fmode = fmode == O_RDONLY ? O_RDWR : O_RDWR|O_CREAT;
 perms = argc >= 4 ? atoi(argv[3]) : 0644;
 result = open(argv[1],fmode,perms);
 if (result == -1) perror(argv[1]);
 return result;
}

int builtin_sys_open2(int argc, char *argv[]) {
 int result,perms,fmode = 0;
 char *mode = argc >= 4 ? argv[3] : "r";
 if (argc < 3) { dprintf(STDERR_FILENO,"usage: open2 FILENO FILENAME [mode=r] [perms=0644]\n"); return -1; }
      if (*mode == 'r') fmode = O_RDONLY,++mode;
 else if (*mode == 'w') fmode = O_WRONLY|O_CREAT|O_TRUNC,++mode;
 if (*mode == '+') fmode = fmode == O_RDONLY ? O_RDWR : O_RDWR|O_CREAT;
 perms = argc >= 5 ? atoi(argv[4]) : 0644;
 result = open2(atoi(argv[1]),argv[2],fmode,perms);
 if (result == -1) perror(argv[1]);
 return result;
}

int builtin_sys_close(int argc, char *argv[]) {
 int result;
 if (argc < 2) { dprintf(STDERR_FILENO,"usage: close FILENO\n"); return -1; }
 result = close(atoi(argv[1]));
 if (result == -1) perror(argv[1]);
 return result;
}
int builtin_sys_read(int argc, char *argv[]) {
 return -1; // TODO
}
int builtin_sys_seek(int argc, char *argv[]) {
 return -1; // TODO
}
int builtin_sys_write(int argc, char *argv[]) {
 return -1; // TODO
}
int builtin_sys_dup(int argc, char *argv[]) {
 return -1; // TODO
}
int builtin_sys_dup2(int argc, char *argv[]) {
 return -1; // TODO
}
#endif /* CONFIG_HAVE_IO */


#endif /* !__BUILTIN_SYSIO_C__ */

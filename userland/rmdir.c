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
#ifndef __RMDIR_C__
#define __RMDIR_C__ 1

#include <assert.h>
#include <errno.h>
#include <libgen.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>


static void mkdir_error(char const *path) {
 int olderr = errno;
 char *msg = strdupf("rmdir '%s'",path);
 errno = olderr;
 perror(msg ? msg : path);
 free(msg);
 errno = olderr;
}

static int verbose = 0;
static int recursive = 0;

static void user_rmdir(char const *path) {
 if (verbose) printf("rmdir: removing directory '%s'\n",path);
 if (rmdir(path) == -1) mkdir_error(path);
 else if (recursive && strchr(path,'/')) {
  // Remove parent directory
  char *temp = strdup(path);
  user_rmdir(dirname(temp));
  free(temp);
 }
}

int main(int argc, char *argv[]) {

 if (argc) --argc,++argv;
 for (; argc; --argc,++argv) {
  user_rmdir(*argv);
 }
 return errno;
}

#endif /* !__RMDIR_C__ */

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
#ifndef __MKDIR_C__
#define __MKDIR_C__ 1

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <libgen.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <getopt.h>


static mode_t mode = 0644;
static int verbose = 0;
static int recursive = 0;


static void mkdir_error(char const *path) {
 int olderr = errno;
 char *msg = strdupf("mkdir '%s'",path);
 errno = olderr;
 perror(msg ? msg : path);
 free(msg);
 errno = olderr;
}

static void user_mkdir(char const *path) {
 if (mkdir(path,mode) == -1) {
  int error = errno;
  if (recursive && error == ENODIR) {
   // Create parent directory
   char *temp = strdup(path);
   user_mkdir(dirname(temp));
   free(temp);
   if (mkdir(path,mode) == 0) goto done;
   errno = error;
  }
  mkdir_error(path);
 } else done: if (verbose) {
  printf("mkdir: created directory '%s'\n",path);
 }
}


static struct option const longopts[] = {
 {"mode",required_argument,NULL,'m'},
 {"help",no_argument,NULL,'h'},
 {"parents",no_argument,NULL,'p'},
 {"verbose",no_argument,NULL,'v'},
 {NULL,0,NULL,0}
};

static void usage(char const *name, int error) {
 fprintf(stderr,"Usage: %s [OPTION]... DIRECTORY...\n",name);
 fputs("\t-m, --mode=MODE Directory creation mode\n"
       "\t-p, --parents   Create missing parent directories\n"
       "\t-v, --verbose   Print message for every created directory\n",
       stderr);
 exit(error);
}

int main(int argc, char *argv[]) {
 int optc;
 while ((optc = getopt_long(argc,argv,"pm:vh",longopts,NULL)) != -1) {
  switch (optc) {
   case 'p': recursive = 1; break;
   case 'm': mode = _strtos32(optarg,NULL,8); break;
   case 'v': verbose = 1; break;
   case 'h': case '?': usage(argv[0],EXIT_SUCCESS); break;
   default: usage(argv[0],EXIT_FAILURE);
  }
 }
 argc -= optind,argv += optind;
 for (; argc; --argc,++argv) user_mkdir(*argv);
 return errno;
}

#endif /* !__MKDIR_C__ */

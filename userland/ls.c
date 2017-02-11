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
#ifndef __LS_C__
#define __LS_C__ 1

#include <kos/compiler.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>

static void mkpermstr(mode_t m, char buf[11]) {
 char *iter = buf;
 *iter++ = S_ISDIR(m) ? 'd' :
           S_ISLNK(m) ? 'l' :
           S_ISCHR(m) ? 'c' :
           S_ISBLK(m) ? 'b' : '-';
 *iter++ = m & S_IRUSR ? 'r' : '-';
 *iter++ = m & S_IWUSR ? 'w' : '-';
 *iter++ = m & S_IXUSR ? 'x' : '-';
 *iter++ = m & S_IRGRP ? 'r' : '-';
 *iter++ = m & S_IWGRP ? 'w' : '-';
 *iter++ = m & S_IXGRP ? 'x' : '-';
 *iter++ = m & S_IROTH ? 'r' : '-';
 *iter++ = m & S_IWOTH ? 'w' : '-';
 *iter++ = m & S_IXOTH ? 'x' : '-';
 *iter = '\0';
}

static int long_mode = 0;
static int recursive = 0;
static int show_hidden = 0;

static int ls_path(char const *path) {
 struct dirent *ent;
 DIR *d = opendir(path);
 int path_endswith_slash;
 if (!strcmp(path,".")) path = "";
 path_endswith_slash = !*path || strend(path)[-1] == '/';
 if (!d) {
  printf("Failed to open '%s': %s\n",path,strerror(errno));
  return EXIT_FAILURE;
 }
 errno = 0;
 while ((ent = readdir(d)) != NULL) {
  struct stat st; char perm[11];
  int stat_failed = 0;
  struct tm mtime; char *abspath;
  if (!show_hidden && ent->d_name[0] == '.') continue;
  abspath = strdupf("%s%s%s",path,path_endswith_slash ? "" : "/",ent->d_name);
  if (long_mode) {
   if (stat(abspath,&st) == -1) stat_failed = errno,memset(&st,0,sizeof(st));
   mkpermstr(st.st_mode,perm);
   localtime_r(&st.st_mtime,&mtime);
   mkpermstr(st.st_mode,perm);
   printf("%s % 10I64u %.2d:%.2d:%.2d %.2d.%.2d.%.4d ",
          perm,st.st_size,
          mtime.tm_hour,mtime.tm_min,mtime.tm_sec,
          mtime.tm_mday,mtime.tm_mon+1,1900+mtime.tm_year);
   if (stat_failed) {
    printf("%s (%s)\n",abspath,strerror(stat_failed));
   } else {
    printf("%s\n",abspath);
   }
  } else {
   printf("%s\n",abspath);
  }
  if (recursive && S_ISDIR(st.st_mode)) ls_path(abspath);
  free(abspath);
 }
 closedir(d);
 return errno ? EXIT_SUCCESS : EXIT_FAILURE;
}


static void usage(char *name, int ret) {
 dprintf(STDERR_FILENO,
         "USAGE: %s [-lar] [PATH...]\n"
         "\t-l Long mode\n"
         "\t-a Show hidden files\n"
         "\t-r Recursively enumerate folders\n"
        ,name);
 _exit(ret);
}

int main(int argc, char *argv[]) {
 int optc;
 while ((optc = getopt(argc,argv,"lar?")) != -1) {
  switch (optc) {
   case 'l': long_mode = 1; break;
   case 'a': show_hidden = 1; break;
   case 'r': recursive = 1; break;
   case '?': usage(argv[0],EXIT_SUCCESS); break;
   default: break;
  }
 }
      if (optind < argc) return ls_path(argv[optind]);
 else if (optind == argc) return ls_path(".");
 else while (optind < argc) {
  ls_path(argv[optind++]);
 }
 return EXIT_SUCCESS;
}

#endif /* !__LS_C__ */

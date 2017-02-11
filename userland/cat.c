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
#ifndef __CAT_C__
#define __CAT_C__ 1

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <kos/fd.h>
#include <kos/syscall.h>
#include <unistd.h>


#define CAT_BUFSIZE 4096

static int catfd(int fd) {
 ssize_t size;
 char buffer[CAT_BUFSIZE];
 while ((size = read(fd,buffer,CAT_BUFSIZE)) > 0) {
  write(STDOUT_FILENO,buffer,size);
 }
 return (int)size;
}
static void catfile(char const *filename) {
 struct stat st;
 int fd = open(filename,O_RDONLY);
 if (fd == -1) {
error:
  printf("cat: %s: %s\n",filename,strerror(errno));
  goto end;
 }
 if (fstat(fd,&st) == -1) goto error;
 if (S_ISDIR(st.st_mode)) { errno = EISDIR; goto error; }
 if (catfd(fd) == -1) goto error;
end:
 close(fd);
}

int main(int argc, char *argv[]) {
 char **iter,**end;
 if (argc) --argc,++argv;
 if (!argc) catfd(STDIN_FILENO);
 else {
  end = (iter = argv)+argc;
  for (; iter != end; ++iter) catfile(*iter);
 }
 return errno;
}

#endif /* !__CAT_C__ */

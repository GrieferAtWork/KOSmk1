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
#ifndef __MAIN_C__
#define __MAIN_C__ 1

#include <kos/compiler.h>
#include <stdio.h>
#include <proc.h>
#include <unistd.h>


#include <kos/arch/string.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <kos/fd.h>
#include <malloc.h>
#include <stdlib.h>

static char const *fdtypename(int type) {
 switch (type) {
  case KFDTYPE_FILE   : return "file";
  case KFDTYPE_TASK   : return "task";
  case KFDTYPE_PROC   : return "proc";
  case KFDTYPE_INODE  : return "inode";
  case KFDTYPE_DIRENT : return "dirent";
  case KFDTYPE_DEVICE : return "device";
  default             : return "??" "?";
 }
}

void psprintfd(int orgid, int fd, int indent) {
 int i; char namebuf[64]; kerrno_t error;
 for (i = 0; i < indent; ++i) printf("| ");
 if (KE_ISERR(error = kfd_getattr(fd,KATTR_FS_PATHNAME,namebuf,sizeof(namebuf),NULL)) &&
     KE_ISERR(error = kfd_getattr(fd,KATTR_GENERIC_NAME,namebuf,sizeof(namebuf),NULL))
     ) strcpy(namebuf,"??" "?");
 else namebuf[63] = '\0';
 printf("\\-FD[%d] -- %s : %s\n",orgid,fdtypename(fcntl(fd,_F_GETYPE)),namebuf);
}

void psprint_thread(task_t fd, int indent) {
 int i; char *name;
 for (i = 0; i < indent; ++i) printf("| ");
 name = task_getname(fd,NULL,0);
 printf("%s (parid = %Iu | tid = %Iu)\n",name,task_getparid(fd),task_gettid(fd));
 free(name);
}

void psprintctx(proc_t context, int indent) {
 int fdv[128]; ssize_t fdi,fdc;
 fdc = proc_enumfd(context,fdv,128);
 if (fdc != -1) {
  int dupfd;
  for (fdi = 0; fdi < fdc; ++fdi) {
   dupfd = proc_openfd(context,fdv[fdi],0);
   if (dupfd != -1) {
    psprintfd(fdv[fdi],dupfd,indent);
    close(dupfd);
   }
  }
 }
}


void pslistthread(task_t thread, proc_t lastcontext, int indent) {
 size_t children[128]; ssize_t childc,i;
 proc_t threadcontext;
 psprint_thread(thread,indent);
 if ((threadcontext = task_openproc(thread)) != -1) {
  if (!kfd_equals(threadcontext,lastcontext) &&
      !kfd_equals(threadcontext,kproc_self())) {
   psprintctx(threadcontext,indent+1);
  }
 }
 if ((childc = task_enumchildren(thread,children,128)) != -1) {
  for (i = 0; i < childc; ++i) {
   int child = task_openchild(thread,children[i]);
   if (child != -1) {
    pslistthread(child,threadcontext != -1
                 ? threadcontext : lastcontext,
                 indent+1);
    close(child);
   }
  }
 }
 close(threadcontext);
}

void ps(void) {
 int fd = proc_openbarrier(proc_self(),
                           KSANDBOX_BARRIER_NOVISIBLE);
 if (fd < 0) perror("proc_openbarrier()");
 else {
  pslistthread(fd,-1,0);
  printf("----------DONE\n");
  close(fd);
 }
}

int main(int argc, char *argv[]) {
 ps();

 return 42;
}

#endif /* !__MAIN_C__ */

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
#ifndef __PTY_C__
#define __PTY_C__ 1

#include <kos/compiler.h>
#include <kos/config.h>
#include <kos/fd.h>
#include <pty.h>
#include <errno.h>
#include <termios.h>
#include <sys/ioctl.h>

__DECL_BEGIN

__public int
openpty(int *__restrict amaster, int *__restrict aslave, char *name,
        struct termios const *termp, struct winsize const *winp) {
#ifdef __KERNEL__
 return kfd_openpty(amaster,aslave,name,termp,winp);
#else
 kerrno_t error;
 error = kfd_openpty(amaster,aslave,name,termp,winp);
 if __likely(KE_ISOK(error)) return 0;
 __set_errno(-error);
 return -1;
#endif
}

__public int
tcgetattr(int fd, struct termios *termios_p) {
 return ioctl(fd,TCGETA,termios_p);
}

__public int
tcsetattr(int fd, int optional_actions,
          struct termios const *termios_p) {
 unsigned long int cmd;
 switch (optional_actions) {
  case TCSANOW:   cmd = TCSETS; break;
  case TCSADRAIN: cmd = TCSETSW; break;
  case TCSAFLUSH: cmd = TCSETSF; break;
  default: __set_errno(EINVAL); return -1;
 }
 return ioctl(fd,cmd,termios_p);
}


__DECL_END

#endif /* !__PTY_C__ */

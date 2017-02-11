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
#ifndef __KOS_KERNEL_SYSCALL_SYSCALL_GENERIC_C_INL__
#define __KOS_KERNEL_SYSCALL_SYSCALL_GENERIC_C_INL__ 1

#include "syscall-common.h"
#include <kos/errno.h>
#include <kos/kernel/linker.h>
#include <kos/kernel/serial.h>
#include <kos/kernel/tty.h>
#include <kos/syscallno.h>
#include <kos/sysconf.h>
#include <kos/syslog.h>
#include <unistd.h>
#include <kos/timespec.h>
#include <kos/kernel/time.h>
#include <kos/kernel/syslog.h>

__DECL_BEGIN

/* _syscall2(kerrno_t,k_syslog,char const *,s,size_t,maxlen); */
SYSCALL(sys_k_syslog) {
 LOAD3(int         ,K(level),
       char const *,K(s),
       size_t      ,K(maxlen));
 struct kproc *caller = kproc_self();
 int allowed_level = (katomic_load(caller->p_sand.ts_state) >> KPROCSTATE_SHIFT_LOGPRIV);
 if (level < allowed_level) RETURN(KE_ACCES); /* You're not allowed to log like this! */
 if (k_sysloglevel < level) RETURN(KS_UNCHANGED); /* Your log level is currently disabled. */
 RETURN(k_dosyslog_u(kproc_self(),level,s,maxlen));
}

/*< _syscall1(long,k_sysconf,int,name); */
SYSCALL(sys_k_sysconf) {
 LOAD1(int,K(name));
 RETURN(k_sysconf(name));
}

__DECL_END

#endif /* !__KOS_KERNEL_SYSCALL_SYSCALL_GENERIC_C_INL__ */

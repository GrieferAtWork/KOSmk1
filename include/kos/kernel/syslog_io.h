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
#ifndef __KOS_KERNEL_SYSLOG_IO_H__
#define __KOS_KERNEL_SYSLOG_IO_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/kernel/syslog.h>
#include <kos/types.h>
#include <kos/errno.h>
#ifndef __INTELLISENSE__
#include <kos/kernel/task.h>
#endif

__DECL_BEGIN

typedef void (*psyslogprinter)(int level, char const *msg, __size_t msg_max, void *closure);

#ifdef __INTELLISENSE__
//////////////////////////////////////////////////////////////////////////
// Add/Delete a syslog printer.
// When a syslog even occurs, all printers for the associated level are invoked.
// @return: KE_OK:        Successfully registered/deleted the given printer.
// @return: KE_NOMEM:     [ksyslog_addprinter] Not enough available memory to register the printer.
// @return: KS_UNCHANGED: [ksyslog_delprinter] The given printer was not previously registered.
extern kerrno_t ksyslog_addprinter(int level, psyslogprinter printer, void *closure);
extern kerrno_t ksyslog_delprinter(int level, psyslogprinter printer);
#else
extern __crit kerrno_t __ksyslog_addprinter_c(int level, psyslogprinter printer, void *closure);
extern __crit kerrno_t __ksyslog_delprinter_c(int level, psyslogprinter printer);
#define ksyslog_addprinter(level,printer,closure) KTASK_CRIT(__ksyslog_addprinter_c(level,printer,closure))
#define ksyslog_delprinter(level,printer)         KTASK_CRIT(__ksyslog_delprinter_c(level,printer))
#endif

/* Add/Delete the TTY as syslog output. */
extern void ksyslog_addtty(void);
extern void ksyslog_deltty(void);

__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_SYSLOG_IO_H__ */

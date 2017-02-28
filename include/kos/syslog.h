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
#ifndef __KOS_SYSLOG_H__
#define __KOS_SYSLOG_H__ 1

#include <kos/compiler.h>
#include <stdarg.h>
#include <kos/types.h>
#ifndef __KERNEL__
#ifndef __INTELLISENSE__
#include <format-printer.h>
#include <kos/errno.h>
#include <kos/syscall.h>
#include <kos/syscallno.h>
#endif
#endif /* !__KERNEL__ */

__DECL_BEGIN

#define KLOG_RAW  (-1)
#define KLOG_CRIT   0
#define KLOG_ERROR  1
#define KLOG_WARN   2
#define KLOG_MSG    3
#define KLOG_INFO   4
#define KLOG_DEBUG  5
#define KLOG_TRACE  6
#define KLOG_INSANE 7
#define KLOG_COUNT  8

#ifndef __KERNEL__
#ifdef __INTELLISENSE__
extern __nonnull((2)) void k_syslog(int level, char const *msg, size_t msg_max);
extern __attribute_vaformat(__printf__,2,3) void k_syslogf(int level, char const *__restrict fmt, ...);
extern __attribute_vaformat(__printf__,2,0) void k_vsyslogf(int level, char const *__restrict fmt, va_list args);
#else
__local _syscall3(kerrno_t,k_syslog,int,level,char const *,s,__size_t,maxlen);
static __attribute_unused int __ksyslog_callback(char const *s, __size_t maxlen, void *level) { return k_syslog((int)(__uintptr_t)level,s,maxlen); }
#define k_syslogf(level,...)           format_printf(&__ksyslog_callback,(void *)(__uintptr_t)(level),__VA_ARGS__)
#define k_vsyslogf(level,format,args) format_vprintf(&__ksyslog_callback,(void *)(__uintptr_t)(level),format,args)
#endif
#endif /* !__KERNEL__ */

__DECL_END

#ifdef __KERNEL__
#ifndef __KOS_KERNEL_SYSLOG_H__
#include <kos/kernel/syslog.h>
#endif /* !__KOS_KERNEL_SYSLOG_H__ */
#endif /* __KERNEL__ */


#endif /* !__KOS_SYSLOG_H__ */

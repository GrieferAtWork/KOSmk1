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
#ifndef __KOS_KERNEL_SYSLOG_H__
#define __KOS_KERNEL_SYSLOG_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#ifndef __KOS_SYSLOG_H__
#include <kos/syslog.h>
#endif /* !__KOS_SYSLOG_H__ */
#include <kos/errno.h>
#include <kos/types.h>

__DECL_BEGIN

struct kfile;
struct kproc;

/* Do syslog output, as written by 'caller'.
 * Don't perform checks if the caller is allowed to, or even should log with the given level. */
extern kerrno_t k_dosyslog_u(struct kproc *caller, int level, __user char const *s, __size_t maxlen);

extern void k_dosyslog(int level, void (*print_prefix)(int,void *), void *closure, __kernel char const *s, __size_t maxlen);
extern void k_dosyslogf(int level, void (*print_prefix)(int,void *), void *closure, char const *fmt, ...);
extern void k_dovsyslogf(int level, void (*print_prefix)(int,void *), void *closure, char const *fmt, va_list args);

extern void k_dosyslog_prefixfile(int level, struct kfile *file, char const *s, __size_t maxlen);
extern void k_dosyslogf_prefixfile(int level, struct kfile *file, char const *format, ...);
extern void k_dovsyslogf_prefixfile(int level, struct kfile *file, char const *format, va_list args);


/* Currently set syslog level (One of 'KLOG_*'). */
extern int k_sysloglevel;


#ifdef __INTELLISENSE__
extern void k_syslog(int level, char const *s, __size_t maxlen);
extern void k_syslogf(int level, char const *fmt, ...);
extern void k_vsyslogf(int level, char const *fmt, va_list args);
extern void k_syslog_prefixfile(int level, struct kfile *file, char const *s, __size_t maxlen);
extern void k_syslogf_prefixfile(int level, struct kfile *file, char const *format, ...);
extern void k_vsyslogf_prefixfile(int level, struct kfile *file, char const *format, va_list args);
#else
#define k_syslog(level,s,maxlen)   (k_sysloglevel>=(level)?k_dosyslog(level,s,maxlen,NULL,NULL):(void)0)
#define k_syslogf(level,...)       (k_sysloglevel>=(level)?k_dosyslogf(level,NULL,NULL,__VA_ARGS__):(void)0)
#define k_vsyslogf(level,fmt,args) (k_sysloglevel>=(level)?k_dovsyslogf(level,NULL,NULL,fmt,args):(void)0)
#define k_syslog_prefixfile(level,file,s,maxlen)      (k_sysloglevel>=(level)?k_dosyslog_prefixfile(level,file,s,maxlen):(void)0)
#define k_syslogf_prefixfile(level,file,...)          (k_sysloglevel>=(level)?k_dosyslogf_prefixfile(level,file,__VA_ARGS__):(void)0)
#define k_vsyslogf_prefixfile(level,file,format,args) (k_sysloglevel>=(level)?k_dovsyslogf_prefixfile(level,file,format,args):(void)0)
#endif

extern __wunused __retnonnull char const *k_sysloglevel_mnemonic(int level);
extern __nonnull((2)) void k_writesyslog(int level, char const *msg, __size_t msg_max);

__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_SYSLOG_H__ */

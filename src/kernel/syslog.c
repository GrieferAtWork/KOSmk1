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
#ifndef __KOS_KERNEL_SYSLOG_C__
#define __KOS_KERNEL_SYSLOG_C__ 1

#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <format-printer.h>
#include <kos/syslog.h>
#include <kos/kernel/syslog.h>
#include <kos/kernel/serial.h>
#include <kos/kernel/fs/file.h>
#include <kos/kernel/proc.h>
#include <kos/kernel/shm.h>
#include <kos/kernel/time.h>
#include <kos/kernel/linker.h>
#include <kos/timespec.h>
#include <kos/attr.h>
#include <stdio.h>
#include <alloca.h>

__DECL_BEGIN

#define KLOGFORMAT_PREFIXTIME 1
#define KLOGFORMAT_KERNEL     "[KERNEL]"
#define KLOGFORMAT_APP_1      "[APP:'"
#define KLOGFORMAT_APP_2      "']"
#define KLOGFORMAT_TAGCH_1    '['
#define KLOGFORMAT_TAGCH_2    ']'
#define KLOGFORMAT_TAGSTR_1   "["
#define KLOGFORMAT_TAGSTR_2   "]"



void k_writesyslog(int level, char const *msg, __size_t msg_max) {
 /* TODO: Do something more with this... */
 (void)level;
 serial_printn(SERIAL_01,msg,msg_max);
}

struct syslog_callback_data {
 int    level;
 void (*print_prefix)(int,void *);
 void  *closure;
};
static int syslog_callback(char const *msg, size_t msg_max, void *data) {
 k_dosyslog(((struct syslog_callback_data *)data)->level,
            ((struct syslog_callback_data *)data)->print_prefix,
            ((struct syslog_callback_data *)data)->closure,msg,msg_max);
 return 0;
}

void k_dovsyslogf(int level, void (*print_prefix)(int,void *), void *closure,
                  char const *fmt, va_list args) {
 struct syslog_callback_data data = {level,print_prefix,closure};
 format_vprintf(&syslog_callback,&data,fmt,args);
}
void k_dosyslogf(int level, void (*print_prefix)(int,void *), void *closure,
                 char const *fmt, ...) {
 va_list args;
 va_start(args,fmt);
 k_dovsyslogf(level,print_prefix,closure,fmt,args);
 va_end(args);
}

char const *k_sysloglevel_mnemonic(int level) {
 switch (level) {
  default:
#ifdef KLOG_RAW
   if (level <= KLOG_RAW) return "";
#endif
   return "#E";
  case KLOG_WARN  : return "#W";
  case KLOG_MSG   : return "#M";
  case KLOG_INFO  : return "#I";
  case KLOG_DEBUG : return "#D";
  case KLOG_TRACE : return "#T";
  case KLOG_INSANE: return "#S";
 }
}


static void print_file_name(int level, struct kfile *file) {
 char filename[PATH_MAX];
 if (KE_ISERR(kfile_kernel_getattr(file,KATTR_FS_PATHNAME,filename,sizeof(filename),NULL)) &&
     KE_ISERR(kfile_kernel_getattr(file,KATTR_FS_FILENAME,filename,sizeof(filename),NULL))
     ) strcpy(filename,"??" "?");
 else filename[PATH_MAX-1] = '\0';
 k_writesyslog(level,"['",2);
 k_writesyslog(level,filename,PATH_MAX);
 k_writesyslog(level,"']",2);
}


void k_dosyslog_prefixfile(int level, struct kfile *file, char const *s, __size_t maxlen) {
 k_dosyslog(level,(void(*)(int,void *))&print_file_name,file,s,maxlen);
}
struct printf_prefixfile_data {
 int level;
 struct kfile *file;
};
static int syslog_callback_prefixfile(char const *msg, size_t msg_max, void *buf) {
 k_dosyslog_prefixfile(((struct printf_prefixfile_data *)buf)->level,
                       ((struct printf_prefixfile_data *)buf)->file,
                       msg,msg_max);
 return 0;
}
void k_dovsyslogf_prefixfile(int level, struct kfile *file, char const *format, va_list args) {
 struct printf_prefixfile_data data = {level,file};
 format_vprintf(&syslog_callback_prefixfile,&data,format,args);
}
void k_dosyslogf_prefixfile(int level, struct kfile *file, char const *format, ...) {
 struct printf_prefixfile_data data = {level,file};
 va_list args;
 va_start(args,format);
 format_vprintf(&syslog_callback_prefixfile,&data,format,args);
 va_end(args);
}

__DECL_END

#ifndef __INTELLISENSE__
#define USER
#include "syslog-write.c.inl"
#include "syslog-write.c.inl"
#endif

#endif /* !__KOS_KERNEL_SYSLOG_C__ */

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

#include <alloca.h>
#include <format-printer.h>
#include <kos/attr.h>
#include <kos/kernel/fs/file.h>
#include <kos/kernel/linker.h>
#include <kos/kernel/proc.h>
#include <kos/kernel/serial.h>
#include <kos/kernel/shm.h>
#include <kos/kernel/syslog.h>
#include <kos/kernel/syslog_io.h>
#include <kos/kernel/time.h>
#include <kos/kernel/tty.h>
#include <kos/errno.h>
#include <kos/syslog.h>
#include <kos/timespec.h>
#include <malloc.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

__DECL_BEGIN

#define KLOGFORMAT_PREFIXTIME 1
#define KLOGFORMAT_KERNEL     "[KERNEL]"
#define KLOGFORMAT_APP_1      "[APP:'"
#define KLOGFORMAT_APP_2      "']"
#define KLOGFORMAT_TAGCH_1    '['
#define KLOGFORMAT_TAGCH_2    ']'
#define KLOGFORMAT_TAGSTR_1   "["
#define KLOGFORMAT_TAGSTR_2   "]"


struct syslog_printer {
 struct syslog_printer *sp_next;    /*< [0..1] Next printer. */
 psyslogprinter         sp_printer; /*< [1..1] Printer callback. */
 void                  *sp_closure; /*< [?..?] Callback closure. */
};

static __atomic int printers_lock = 0;
static struct syslog_printer *printers[KLOG_COUNT] = {NULL,};
#define PRINTERS_ACQUIRE   KTASK_SPIN(katomic_cmpxch(printers_lock,0,1))
#define PRINTERS_RELEASE   katomic_store(printers_lock,0)

__crit kerrno_t
__ksyslog_addprinter_c(int level, psyslogprinter printer, void *closure) {
 struct syslog_printer **chain,*entry;
 KTASK_CRIT_MARK
 assert(level >= 0 && level < KLOG_COUNT);
 kassertbyte(printer);
 entry = omalloc(struct syslog_printer);
 if __unlikely(!entry) return KE_NOMEM;
 chain = &printers[level];
 entry->sp_printer = printer;
 entry->sp_closure = closure;
 PRINTERS_ACQUIRE;
 entry->sp_next = *chain;
 (*chain) = entry;
 PRINTERS_RELEASE;
 return KE_OK;
}
__crit kerrno_t
__ksyslog_delprinter_c(int level, psyslogprinter printer) {
 struct syslog_printer **chain,*entry;
 KTASK_CRIT_MARK
 assert(level >= 0 && level < KLOG_COUNT);
 kassertbyte(printer);
 chain = &printers[level];
 PRINTERS_ACQUIRE;
 while ((entry = *chain) != NULL) {
  if (entry->sp_printer == printer) {
   /* Unlink the entry */
   *chain = entry->sp_next;
   PRINTERS_RELEASE;
   free(entry);
   return KE_OK;
  }
  chain = &entry->sp_next;
 }
 PRINTERS_RELEASE;
 return KS_UNCHANGED;
}

__local void
ksyslog_invoke_printers(int level, char const *msg, size_t msg_max) {
 struct syslog_printer **chain,*start,*iter;
 assert(level >= 0 && level < KLOG_COUNT);
 KTASK_CRIT_BEGIN
 chain = &printers[level];
 PRINTERS_ACQUIRE;
 start = *chain;
 *chain = NULL;
 PRINTERS_RELEASE;
 if (start) {
  iter = start;
  do (*iter->sp_printer)(level,msg,msg_max,iter->sp_closure);
  while ((iter = iter->sp_next) != NULL);
  PRINTERS_ACQUIRE;
  if __unlikely(*chain) {
   iter = *chain;
   while (iter->sp_next) iter = iter->sp_next;
   iter->sp_next = start;
  } else {
   *chain = start;
  }
  PRINTERS_RELEASE;
 }
 KTASK_CRIT_END
}



__nomp void k_writesyslog(int level, char const *msg, size_t msg_max) {
 /* TODO: Do something more with this... */
 (void)level;
 serial_printn(SERIAL_01,msg,msg_max);
 ksyslog_invoke_printers(level,msg,msg_max);
}

static void
tty_syslog(int __unused(level),
           char const *msg, size_t msg_max,
           void *__unused(closure)) {
 tty_printn(msg,msg_max);
}

void ksyslog_addtty(void) {
 int level;
 for (level = 0; level != KLOG_COUNT; ++level)
  ksyslog_addprinter(level,&tty_syslog,NULL);
}
void ksyslog_deltty(void) {
 int level;
 for (level = 0; level != KLOG_COUNT; ++level)
  ksyslog_delprinter(level,&tty_syslog);
}


struct syslog_callback_data {
 int           level;
 psyslogprefix print_prefix;
 void         *closure;
};
static int syslog_callback(char const *msg, size_t msg_max, void *data) {
 k_dosyslog(((struct syslog_callback_data *)data)->level,
            ((struct syslog_callback_data *)data)->print_prefix,
            ((struct syslog_callback_data *)data)->closure,msg,msg_max);
 return 0;
}

void k_dovsyslogf(int level, psyslogprefix print_prefix, void *closure,
                  char const *__restrict fmt, va_list args) {
 struct syslog_callback_data data = {level,print_prefix,closure};
 format_vprintf(&syslog_callback,&data,fmt,args);
}
void k_dosyslogf(int level, psyslogprefix print_prefix, void *closure,
                 char const *__restrict fmt, ...) {
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
int k_sysloglevel_fromstring(char const *name) {
 if (!name) return -1;
 if (*name == '#') ++name;
 switch (*name) {
  case 'e': case 'E': return KLOG_ERROR;
  case 'w': case 'W': return KLOG_WARN;
  case 'm': case 'M': return KLOG_MSG;
  case 'i': case 'I': return KLOG_INFO;
  case 'd': case 'D': return KLOG_DEBUG;
  case 't': case 'T': return KLOG_TRACE;
  case 's': case 'S': return KLOG_INSANE;
  case '\0': return -1;
  default: break;
 }
 if (*name >= '0' && *name <= '9')
  return atoi(name);
 return -1;
}

void kernel_initialize_syslog(void) {
 int new_level;
 char *env_level           = getenv("LOGLEVEL");
 if (!env_level) env_level = getenv("LOGLV");
 if (!env_level) return;
 new_level = k_sysloglevel_fromstring(env_level);
 if (new_level != -1) k_sysloglevel = new_level;
 else k_syslogf(KLOG_ERROR,"Invalid $LOGLEVEL content: %q\n",env_level);
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


void k_dosyslog_prefixfile(int level, struct kfile *file, char const *s, size_t maxlen) {
 k_dosyslog(level,(psyslogprefix)&print_file_name,file,s,maxlen);
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
void k_dovsyslogf_prefixfile(int level, struct kfile *file, char const *__restrict format, va_list args) {
 struct printf_prefixfile_data data = {level,file};
 format_vprintf(&syslog_callback_prefixfile,&data,format,args);
}
void k_dosyslogf_prefixfile(int level, struct kfile *file, char const *__restrict format, ...) {
 struct printf_prefixfile_data data = {level,file};
 va_list args;
 va_start(args,format);
 format_vprintf(&syslog_callback_prefixfile,&data,format,args);
 va_end(args);
}

__DECL_END


#include <kos/kernel/syscall.h>
__DECL_BEGIN

KSYSCALL_DEFINE3(kerrno_t,k_syslog,int,level,__user char const *,s,size_t,maxlen) {
 struct kproc *caller = kproc_self();
 int allowed_level = kprocperm_getlogpriv(kproc_getperm(caller));
 //if __unlikely(level < allowed_level) return KE_ACCES;     /* You're not allowed to log like this! */
 if __unlikely(k_sysloglevel < level) return KS_UNCHANGED; /* Your log level is currently disabled. */
 if __unlikely(level >= KLOG_COUNT)   return KE_NOSYS;     /* That's not a valid level. */
 return k_dosyslog_u(caller,level,s,maxlen);
}

__DECL_END

#ifndef __INTELLISENSE__
#define USER
#include "syslog-write.c.inl"
#include "syslog-write.c.inl"
#endif

#endif /* !__KOS_KERNEL_SYSLOG_C__ */

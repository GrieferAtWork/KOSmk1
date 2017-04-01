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
#ifndef __STDIO_C__
#define __STDIO_C__ 1
#undef __STDC_PURE__

#ifndef __INTELLISENSE__
#include "stdio-file.c.inl"
#endif

#include <format-printer.h>
#include <kos/arch.h>
#include <kos/compiler.h>
#include <kos/config.h>
#include <kos/kernel/debug.h>
#include <itos.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifdef __KERNEL__
#include <kos/kernel/serial.h>
#include <kos/kernel/tty.h>
#else
#include <kos/syscall.h>
#include <errno.h>
#include <stdint.h>
#endif
#ifndef __CONFIG_MIN_BSS__
#include <kos/syslog.h>
#endif /* !__CONFIG_MIN_BSS__ */


__DECL_BEGIN

#ifdef __KERNEL__
#define IO_WRITEN(data,size) (serial_printn(SERIAL_01,data,size),tty_printn(data,size))
#define IO_WRITES(data,size) (serial_prints(SERIAL_01,data,size),tty_prints(data,size))
#define IO_WRITE(data)       IO_WRITEN(data,(size_t)-1)
#else
#define IO_WRITEN(data,size) write(STDOUT_FILENO,data,strnlen(data,size))
#define IO_WRITES(data,size) write(STDOUT_FILENO,data,size)
#define IO_WRITE(data)       write(STDOUT_FILENO,data,strlen(data))
#endif


__public int putchar(int c) {
 char buf[1] = {(char)c};
 IO_WRITES(buf,1);
 return 0;
}

__public int puts(char const *__restrict s) {
 static char const lf[1] = {'\n'};
 IO_WRITE(s);
 IO_WRITES(lf,1);
 return 0;
}

__public int printf(char const *__restrict format, ...) {
 int result;
 va_list args;
 va_start(args,format);
 result = vprintf(format,args);
 va_end(args);
 return result;
}

static int printf_callback(char const *data, size_t maxchars, void *__unused(closure)) {
 IO_WRITEN(data,maxchars);
 return 0;
}
__public int vprintf(char const *__restrict format, va_list args) {
 return format_vprintf(&printf_callback,NULL,format,args);
}


__public size_t
_sprintf(char *__restrict buf,
         char const *__restrict format, ...) {
 va_list args; size_t result;
 va_start(args,format);
 result = _vsprintf(buf,format,args);
 va_end(args);
 return result;
}
__public size_t
_snprintf(char *__restrict buf, size_t bufsize,
          char const *__restrict format, ...) {
 va_list args; size_t result;
 va_start(args,format);
 result = _vsnprintf(buf,bufsize,format,args);
 va_end(args);
 return result;
}


struct sprintf_data { char *__restrict buf; };
static int
sprintf_callback(char const *__restrict data, size_t maxchars,
                 struct sprintf_data *__restrict buffer) {
 buffer->buf = strncpy(buffer->buf,data,maxchars);
 return 0;
}
__public size_t
_vsprintf(char *__restrict buf,
          char const *__restrict format,
          va_list args) {
 struct sprintf_data data;
 size_t result; data.buf = buf;
 format_vprintf((pformatprinter)&sprintf_callback,
                &data,format,args);
 result = (size_t)(data.buf-buf);
 kassertbyte(data.buf);
 *data.buf = '\0';
 return result;
}

struct snprintf_data {
 char *bufpos;
 char *bufend;
};
static int
snprintf_callback(char const *__restrict data, size_t maxchars,
                  struct snprintf_data *__restrict buffer) {
 size_t datasize = strnlen(data,maxchars);
 /* Don't exceed the buffer end */
 if (buffer->bufpos < buffer->bufend) {
  size_t maxwrite = (size_t)(buffer->bufend-buffer->bufpos);
  memcpy(buffer->bufpos,data,maxwrite < datasize ? maxwrite : datasize);
 }
 /* Still seek past the end, as to
  * calculate the required buffersize. */
 buffer->bufpos += datasize;
 return 0;
}

__public size_t
_vsnprintf(char *__restrict buf, size_t bufsize,
           char const *__restrict format, va_list args) {
 struct snprintf_data data;
 data.bufend = (data.bufpos = buf)+bufsize;
 format_vprintf((pformatprinter)&snprintf_callback,&data,format,args);
 if __likely(data.bufpos < data.bufend) *data.bufpos = '\0';
 return (size_t)(data.bufpos-buf);
}


#if __SIZEOF_INT__ == __SIZEOF_SIZE_T__
__public __COMPILER_ALIAS(sprintf,_sprintf);
__public __COMPILER_ALIAS(vsprintf,_vsprintf);
__public __COMPILER_ALIAS(snprintf,_snprintf);
__public __COMPILER_ALIAS(vsnprintf,_vsnprintf);
#else
#if 1
#   define SPRINTF_RETURN_TYPE unsigned int
#else
#   define SPRINTF_RETURN_TYPE          int
#endif
__public SPRINTF_RETURN_TYPE
sprintf(char *__restrict buf,
        char const *__restrict format, ...) {
 SPRINTF_RETURN_TYPE result;
 va_list args;
 va_start(args,format);
 result = (SPRINTF_RETURN_TYPE)_vsprintf(buf,format,args);
 va_end(args);
 return result;
}
__public SPRINTF_RETURN_TYPE
vsprintf(char *__restrict buf,
         char const *__restrict format, va_list args) {
 return (SPRINTF_RETURN_TYPE)_vsprintf(buf,format,args);
}
__public SPRINTF_RETURN_TYPE
snprintf(char *__restrict buf, size_t bufsize,
         char const *__restrict format, ...) {
 SPRINTF_RETURN_TYPE result;
 va_list args;
 va_start(args,format);
 result = (SPRINTF_RETURN_TYPE)_vsnprintf(buf,bufsize,format,args);
 va_end(args);
 return result;
}
__public SPRINTF_RETURN_TYPE
vsnprintf(char *__restrict buf, size_t bufsize,
          char const *__restrict format, va_list args) {
 return (SPRINTF_RETURN_TYPE)_vsnprintf(buf,bufsize,format,args);
}
#undef SPRINTF_RETURN_TYPE
#endif




__public int
sscanf(char const *__restrict data,
       char const *__restrict format, ...) {
 va_list args; int result;
 va_start(args,format);
 result = vsscanf(data,format,args);
 va_end(args);
 return result;
}
__public int
_snscanf(char const *__restrict data,
         size_t maxdata, char const *__restrict format, ...) {
 va_list args; int result;
 va_start(args,format);
 result = vsnscanf(data,maxdata,format,args);
 va_end(args);
 return result;
}


struct sscanf_data { char const *datapos; };
static int
sscanf_scanner(int *__restrict ch,
               struct sscanf_data *__restrict data) {
 if ((*ch = *data->datapos) == 0) *ch = -1;
 return 0;
}

__public int
vsscanf(char const *__restrict data,
        char const *__restrict format, va_list args) {
 struct sscanf_data cdata = {data};
 return format_vscanf((pformatscanner)&sscanf_scanner,
                      NULL,&cdata,format,args);
}

struct snscanf_data { char const *datapos,*dataend; };
static int snscanf_scanner(int *__restrict ch,
                           struct snscanf_data *__restrict data) {
      if (data->datapos == data->dataend) *ch = -1;
 else if ((*ch = *data->datapos) == 0) *ch = -1;
 return 0;
}

__public int
_vsnscanf(char const *__restrict data, size_t maxdata,
          char const *__restrict format, va_list args) {
 struct snscanf_data cdata = {data,data+maxdata};
 return format_vscanf((pformatscanner)&snscanf_scanner,
                      NULL,&cdata,format,args);
}

#ifndef __CONFIG_MIN_BSS__
__public void perror(char const *s) {
 int eno = __get_errno();
 printf("%s: %s\n",s,eno,strerror(eno));
 k_syslogf(KLOG_ERROR,"%s: %d: %s\n",s,eno,strerror(eno));
}
#endif /* !__CONFIG_MIN_BSS__ */


#ifndef __CONFIG_MIN_LIBC__
static int stdio_fd_formatprinter(char const *data,
                                  size_t maxchars,
                                  intptr_t fd) {
 return write((int)fd,data,strnlen(data,maxchars)) < 0 ? -1 : 0;
}
__public int vdprintf(int fd, char const *__restrict format, va_list args) {
 return format_vprintf((pformatprinter)&stdio_fd_formatprinter,
                       (void *)(intptr_t)fd,format,args);
}
__public int dprintf(int fd, char const *__restrict format, ...) {
 va_list args; int error;
 va_start(args,format);
 error = vdprintf(fd,format,args);
 va_end(args);
 return error;
}
#endif /* !__CONFIG_MIN_LIBC__ */

__DECL_END

#endif /* !__STDIO_H__ */

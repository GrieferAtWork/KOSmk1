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
#ifndef __STDIO_FILE_C_INL__
#define __STDIO_FILE_C_INL__ 1
#undef __STDC_PURE__

#ifndef __KERNEL__
#define __f_flags  f_flags
#define __f_fd     f_fd
#define __f_kind   f_kind

#define FILE_FLAG_NONE   0x00
#define FILE_FLAG_STATIC 0x01 /*< File is statically allocated; don't free(); allow freopen after fclose. */
#define FILE_TYPE_NONE   0
#define FILE_TYPE_FD     1
#endif


#include <stdio.h>
#include <kos/compiler.h>
#include <sys/types.h>
#include <fcntl.h>
#include <format-printer.h>
#include <kos/errno.h>
#include <string.h>

#ifdef __KERNEL__
#include <kos/kernel/keyboard.h>
#include <kos/kernel/fs/fs.h>
#include <kos/kernel/fs/file.h>
#else
#include <malloc.h>
#include <unistd.h>
#endif
#include <errno.h>

__DECL_BEGIN

__public int fprintf(FILE *fp, char const *__restrict fmt, ...) {
 va_list args; int result;
 va_start(args,fmt);
 result = vfprintf(fp,fmt,args);
 va_end(args);
 return result;
}
__public int fscanf(FILE *__restrict fp,
                    char const *__restrict fmt, ...) {
 va_list args; int result;
 va_start(args,fmt);
 result = vfscanf(fp,fmt,args);
 va_end(args);
 return result;
}

__local openmode_t f_getopenmode(char const *mode) {
 openmode_t openmode = 0;
      if (*mode == 'r') openmode = O_RDONLY,++mode;
 else if (*mode == 'w') openmode = O_WRONLY|O_CREAT|O_TRUNC,++mode;
 if (*mode == '+') openmode = openmode == O_RDONLY ? O_RDWR : O_RDWR|O_CREAT;
 return openmode;
}


#ifdef __KERNEL__
__public FILE *fopen(char const *filename,
                     char const *mode) {
 FILE *result;
 if __unlikely(KE_ISERR(krootfs_open(filename,(size_t)-1,
                                     f_getopenmode(mode),
                                     0,NULL,&result)
              )) return NULL;
 return result;
}
__public FILE *freopen(char const *__restrict filename,
                       char const *__restrict mode,
                       FILE *__restrict fp) {
 kassertobj(fp);
 /* Not ~really~ possible in kernel mode
  * >> Though the only reason why this function even
  *    exists (which is to reopen the std streams),
  *    doesn't hold up as the kernel itself, as
  *    the kernel doesn't have STD streams. */
 kfile_decref(fp);
 return fopen(filename,mode);
}
__public int fclose(FILE *fp) {
 kfile_decref(fp);
 return 0;
}
__public int fflush(FILE *fp) { return kfile_flush(fp); }
__public int fgetpos(FILE *fp, fpos_t *pos) { return kfile_tell(fp,pos); }
__public int fsetpos(FILE *fp, fpos_t const *pos) { return kfile_setpos(fp,*pos); }
__public int fseek(FILE *fp, long off, int whence) {  return kfile_seek(fp,(__off_t)off,whence,NULL); }
__public int fseeko(FILE *fp, off_t off, int whence) { return kfile_seek(fp,(__off_t)off,whence,NULL); }
__public long ftell(FILE *fp) {  pos_t filepos; kerrno_t error; if __unlikely(KE_ISERR(error = kfile_tell(fp,&filepos))) return error; return (long)filepos; }
__public off_t ftello(FILE *fp) { pos_t filepos; kerrno_t error; if __unlikely(KE_ISERR(error = kfile_tell(fp,&filepos))) return error; return (off_t)filepos; }
__public void rewind(FILE *fp) { __evalexpr(kfile_rewind(fp)); }
__public int getchar(void) {
 struct kbevent evt;
 struct kaddistticket ticket;
 kerrno_t error = kaddist_genticket(&keyboard_input,&ticket);
 if __unlikely(KE_ISERR(error)) return error;
 do {
  error = KTASK_DEADLOCK_INTENDED(kaddist_vrecv(&keyboard_input,&ticket,&evt));
 } while (KE_ISOK(error) && (!KEY_ISUTF8(evt.e_key) ||
                             !KEY_ISDOWN(evt.e_key)
          ));
 kaddist_delticket(&keyboard_input,&ticket);
 if __unlikely(KE_ISERR(error)) return error;
 return (int)KEY_TOUTF8(evt.e_key);
}
__public size_t fread(void *__restrict buf, size_t size,
                      size_t count, FILE *fp) {
 size_t result;
 if __unlikely(KE_ISERR(kfile_kernel_read(fp,buf,size*count,&result))) return 0;
 return result;
}
__public size_t fwrite(const void *__restrict buf, size_t size,
                       size_t count, FILE *fp) {
 size_t result;
 if __unlikely(KE_ISERR(kfile_kernel_write(fp,buf,size*count,&result))) return 0;
 return result;
}
__public int fgetc(FILE *fp) {
 char ch; size_t rc;
 if __unlikely(KE_ISERR(kfile_kernel_read(fp,&ch,sizeof(ch),&rc)) ||
               !rc) return EOF;
 return (int)ch;
}
__public int fputc(int c, FILE *fp) {
 kerrno_t error; size_t wc; char ch = (char)c;
 if __unlikely(KE_ISERR(error = kfile_kernel_write(fp,&ch,sizeof(ch),&wc))) return EOF;
 return wc ? c : EOF;
}
__public char *fgets(char *__restrict buf, int bufsize, FILE *fp) {
 char temp; char *iter,*end;
 size_t rc;
 kerrno_t error;
 if __unlikely(!bufsize) return NULL;
 end = (iter = buf)+(bufsize-1);
 while (iter != end) {
  if __unlikely(KE_ISERR(error = kfile_kernel_read(fp,&temp,1,&rc))) return NULL;
  if __unlikely(!rc) break;
  *iter++ = temp;
  if (temp == '\n') break;
 }
 *iter = '\0';
 return buf;
}
__public int fputs(char const *s, FILE *fp) {
 size_t wc; kerrno_t error;
 if __unlikely(KE_ISERR(error = kfile_kernel_write(fp,s,strlen(s),&wc))) return error;
 return (int)wc;
}
__public int getw(FILE *fp) {
 int w; size_t rc;
 if __unlikely(KE_ISERR(kfile_kernel_read(fp,&w,sizeof(w),&rc)) ||
               rc < sizeof(int)) return EOF;
 return w;
}
__public int putw(int w, FILE *fp) {
 kerrno_t error; size_t wc;
 if __unlikely(KE_ISERR(error = kfile_kernel_write(fp,&w,sizeof(w),&wc))) return EOF;
 return wc == sizeof(w) ? w : EOF;
}
__public int fseek64(FILE *fp, __s64 off, int whence) {
 return kfile_seek(fp,(__off_t)off,whence,NULL);
}
__public __u64 ftell64(FILE *fp) {
 __pos_t result;
 if __unlikely(KE_ISERR(kfile_seek(fp,0,SEEK_CUR,&result))) return (__u64)-1;
 return (__u64)result;
}
static int stdio_file_formatprinter(char const *data,
                                    size_t maxchars,
                                    FILE *fp) {
 size_t wsize; kerrno_t error;
 maxchars = strnlen(data,maxchars);
 error = kfile_kernel_write(fp,data,maxchars,&wsize);
 if __unlikely(KE_ISERR(error)) return error;
 return wsize == maxchars ? KE_OK : KE_NOSPC;
}
__public int vfprintf(FILE *fp, char const *__restrict fmt, va_list args) {
 return format_vprintf((pformatprinter)&stdio_file_formatprinter,fp,fmt,args);
}
static int fscanf_scanner(int *__restrict ch,
                          FILE *__restrict fp) {
 char rch;
 size_t rsize; kerrno_t error;
 if __unlikely(KE_ISERR(error = kfile_kernel_read(fp,&rch,sizeof(char),&rsize))) return error;
 *ch = rsize ? (int)rch : -1;
 return 0;
}
static int fscanf_returnch(int __unused(ch), FILE *__restrict fp) {
 return kfile_seek(fp,-1,SEEK_CUR,NULL);
}
__public int vfscanf(FILE *__restrict fp,
                     char const *__restrict fmt,
                     va_list args) {
 return format_vscanf((pformatscanner)&fscanf_scanner,
                      (pformatreturn)&fscanf_returnch,fp,fmt,args);
}
#else /* __KERNEL__ */

#undef stdin
#undef stderr
#undef stdout
#define __FILE_INIT_STDFILE(fd) {FILE_FLAG_STATIC,FILE_TYPE_FD,0,{fd}}
static FILE __std_files[] = {
 __FILE_INIT_STDFILE(STDIN_FILENO),
 __FILE_INIT_STDFILE(STDOUT_FILENO),
 __FILE_INIT_STDFILE(STDERR_FILENO)
};
#undef __FILE_INIT_STDFILE
__public FILE *stdin  = &__std_files[0];
__public FILE *stdout = &__std_files[1];
__public FILE *stderr = &__std_files[2];


__local FILE *fnew_fromfd(int fd) {
 FILE *result = omalloc(FILE);
 if (result) result->f_fd = fd;
 return result;
}
__public FILE *fdopen(int fd, char const *__restrict mode) {
 (void)mode;
 return fnew_fromfd(fd);
}
__public FILE *fopen(char const *filename,
                     char const *mode) {
 FILE *result; int fd;
 if __unlikely(!mode) return NULL;
 fd = open(filename,f_getopenmode(mode),0644);
 if __unlikely(fd == -1) return NULL;
 result = fnew_fromfd(fd);
 if __unlikely(!result) close(fd);
 return result;
}
__public FILE *freopen(char const *__restrict filename,
                       char const *__restrict mode,
                       FILE *__restrict fp) {
 if __unlikely(!fp) return fopen(filename,mode); /* yes, no, maybe ??? */
 /* Easy: Just re-open the file descriptor. */
 if __unlikely(open2(fp->f_fd,filename,
  f_getopenmode(mode),0644) == -1) return NULL;
 return fp;
}

__public int fclose(FILE *fp) {
 int error;
 if __unlikely(!fp) return -1;
 if (fp->f_kind == FILE_TYPE_FD) {
  error = close(fp->f_fd);
 } else {
  error = 0; /*< no-op? */
 }
 if __likely(error != -1) {
  if (fp->f_flags&FILE_FLAG_STATIC) {
   fp->f_kind = FILE_TYPE_NONE;
  } else {
   free(fp);
  }
 }
 return error;
}
__public int fflush(FILE *fp) {
 if __unlikely(!fp) return -1;
 return fsync(fp->f_fd);
}
__public int fgetpos(FILE *fp, fpos_t *pos) {
 off64_t off;
 if __unlikely(!fp) return -1;
 off = lseek64(fp->f_fd,0,SEEK_CUR);
 if (off >= 0) { *pos = off; return 0; }
 return -1;
}
__public int fsetpos(FILE *fp, fpos_t const *pos) {
 if __unlikely(!fp) return -1;
 if (lseek64(fp->f_fd,(off64_t)*pos,SEEK_SET) < 0) return -1;
 return 0;
}
__public int fseek(FILE *fp, long off, int whence) {
 if __unlikely(!fp) return -1;
 if (lseek64(fp->f_fd,(off64_t)off,whence) < 0) return -1;
 return 0;
}
__public int fseeko(FILE *fp, off_t off, int whence) {
 if __unlikely(!fp) return -1;
 if (lseek64(fp->f_fd,(off64_t)off,whence) < 0) return -1;
 return 0;
}
__public long ftell(FILE *fp) {
 if __unlikely(!fp) return -1;
 return (long)lseek64(fp->f_fd,0,SEEK_CUR);
}
__public off_t ftello(FILE *fp) {
 if __unlikely(!fp) return -1;
 return (off_t)lseek64(fp->f_fd,0,SEEK_CUR);
}
__public void rewind(FILE *fp) {
 if __likely(fp) {
  __evalexpr(lseek64(fp->f_fd,0,SEEK_SET));
 }
}
__public int fpurge(FILE *__restrict fp) {
 if __unlikely(!fp) { __set_errno(EBADF); return -1; }
 /* TODO: Implement when stdio is buffered. */
 return 0;
}
__public int getchar(void) { return getc(stdin); }
__public size_t fread(void *__restrict buf, size_t size,
                      size_t count, FILE *fp) {
 ssize_t error;
 if __unlikely(!fp) return 0;
 error = read(fp->f_fd,buf,size*count);
 return error < 0 ? 0 : (size_t)error; /* TODO: ferror/feof. */
}
__public size_t fwrite(const void *__restrict buf, size_t size,
                       size_t count, FILE *fp) {
 ssize_t error;
 if __unlikely(!fp) return 0;
 error = write(fp->f_fd,buf,size*count);
 return error < 0 ? 0 : (size_t)error; /* TODO: ferror/feof. */
}
__public int fgetc(FILE *fp) {
 char ch; ssize_t error;
 if __unlikely(!fp) return EOF;
 error = read(fp->f_fd,&ch,sizeof(char));
 /* TODO: Error. */
 return error < 0 ? EOF : (int)ch;
}
__public int fputc(int c, FILE *fp) {
 ssize_t error; char ch = (char)c;
 if __unlikely(!fp) return EOF;
 error = write(fp->f_fd,&ch,sizeof(char));
 /* TODO: Error. */
 return error < 0 ? EOF : (int)ch;
}
__public char *fgets(char *__restrict buf, int bufsize, FILE *fp) {
 char temp; char *iter,*end;
 ssize_t rc;
 if __unlikely(!fp) return NULL;
 if __unlikely(!bufsize) return NULL;
 end = (iter = buf)+(bufsize-1);
 while (iter != end) {
  rc = read(fp->f_fd,&temp,1);
  if __unlikely(rc < 0) return NULL;
  if __unlikely(!rc) break;
  *iter++ = temp;
  if (temp == '\n') break;
 }
 *iter = '\0';
 return buf;
}
__public int fputs(char const *s, FILE *fp) {
 if __unlikely(!s || !fp) return -1;
 return (int)write(fp->f_fd,s,strlen(s));
}
__public int getw(FILE *fp) {
 int w; ssize_t error;
 if __unlikely(!fp) return EOF;
 error = read(fp->f_fd,&w,sizeof(w));
 /* TODO: Error. */
 return error < 0 ? EOF : w;
}
__public int putw(int w, FILE *fp) {
 ssize_t error;
 if __unlikely(!fp) return -1;
 error = write(fp->f_fd,&w,sizeof(w));
 /* TODO: Error. */
 return error < 0 ? EOF : (int)w;
}

__public int fseek64(FILE *fp, __s64 off, int whence) {
 if __unlikely(!fp) return -1;
 return lseek64(fp->f_fd,(off64_t)off,whence) < 0 ? -1 : 0;
}
__public __u64 ftell64(FILE *fp) {
 if __unlikely(!fp) return -1;
 return (__u64)lseek64(fp->f_fd,0,SEEK_CUR);
}

static int stdio_file_formatprinter(char const *data,
                                    size_t maxchars,
                                    FILE *fp) {
 return write(fp->f_fd,data,strnlen(data,maxchars)) < 0 ? -1 : 0;
}

__public int vfprintf(FILE *fp, char const *__restrict fmt, va_list args) {
 if __unlikely(!fp || !fmt) return -1;
 return format_vprintf((pformatprinter)&stdio_file_formatprinter,fp,fmt,args);
}

static int fscanf_scanner(int *__restrict ch,
                          FILE *__restrict fp) {
 char rch;
 ssize_t error = read(fp->f_fd,&rch,sizeof(char));
 if (error < 0) return -1;
 *ch = error ? (int)rch : -1;
 return 0;
}
static int fscanf_returnch(int __unused(ch), FILE *__restrict fp) {
 return lseek64(fp->f_fd,-1,SEEK_CUR) < 0 ? -1 : 0;
}
__public int vfscanf(FILE *__restrict fp,
                     char const *__restrict fmt,
                     va_list args) {
 if __unlikely(!fp || !fmt) return -1;
 return format_vscanf((pformatscanner)&fscanf_scanner,
                      (pformatreturn)&fscanf_returnch,fp,fmt,args);
}
#endif /* !__KERNEL__ */




#ifndef __KERNEL__
__public int fileno(FILE *fp) {
 if __unlikely(!fp) return -1;
 return fp->f_fd;
}

#undef getc
#undef putc
__public __compiler_ALIAS(getc,fgetc)
__public __compiler_ALIAS(putc,fputc)
#undef _fseek64
#undef _ftell64
__public __compiler_ALIAS(_fseek64,fseek64)
__public __compiler_ALIAS(_ftell64,ftell64)
#endif

__DECL_END

#endif /* !__STDIO_FILE_C_INL__ */

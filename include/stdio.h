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
#ifndef __STDIO_H__
#ifndef _STDIO_H
#ifndef _INC_STDIO
#define __STDIO_H__ 1
#define _STDIO_H 1
#define _INC_STDIO 1

#include <__null.h>
#include <stdarg.h>
#include <kos/compiler.h>
#include <kos/types.h>
#ifdef __KERNEL__
#include <kos/kernel/fs/fs.h>
#endif

__DECL_BEGIN

#ifndef SEEK_SET
#   define SEEK_SET 0
#endif
#ifndef SEEK_CUR
#   define SEEK_CUR 1
#endif
#ifndef SEEK_END
#   define SEEK_END 2
#endif

#ifndef __ASSEMBLY__
#ifndef __off_t_defined
#define __off_t_defined 1
typedef __off_t off_t;
#endif

#ifndef __fpos_t_defined
#define __fpos_t_defined 1
typedef __pos64_t fpos_t;
#endif

#ifdef __KERNEL__
typedef struct kfile FILE;
#else
typedef struct __filestruct FILE;
struct __filestruct {
 __u8  __f_flags;   /*< File flags. */
 __u8  __f_kind;    /*< File kind. */
 __u16 __f_padding; /*< Padding data. */
 union{
  int  __f_fd; /*< File-descriptor-style file. */
 };
};
#endif

#ifndef __KERNEL__
#undef stdin
#undef stdout
#undef stderr

// Standard files (NOTE: Not available in kernel mode)
extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;
#endif
#endif /* !__ASSEMBLY__ */

#define EOF    (-1)
#define BUFSIZ  256

#define P_tmpdir  "/tmp"
#define L_tmpnam  20

#ifndef __ASSEMBLY__
extern __wunused __nonnull((1,2)) FILE *
fopen __P((char const *__restrict __filename,
           char const *__restrict __mode));
extern __wunused __nonnull((1,2)) FILE *
freopen __P((char const *__restrict __filename,
             char const *__restrict __mode,
             FILE *__restrict __fp));
#ifndef __KERNEL__
extern __wunused __nonnull((2)) FILE *
fdopen __P((int __fd, char const *__restrict __mode));
#endif
extern __nonnull((1)) int fclose __P((FILE *__restrict __fp));

extern __nonnull((1)) int fflush __P((FILE *__restrict __fp));

extern __nonnull((1,2)) int fgetpos(FILE *__restrict __fp, fpos_t *__restrict __pos);
extern __nonnull((1,2)) int fsetpos(FILE *__restrict __fp, fpos_t const *__restrict __pos);
extern __nonnull((1)) int fseek(FILE *__restrict __fp, long __off, int __whence);
extern __nonnull((1)) int fseeko(FILE *__restrict __fp, off_t __off, int __whence);
extern __nonnull((1)) long ftell __P((FILE *__restrict __fp));
extern __nonnull((1)) off_t ftello __P((FILE *__restrict __fp));
extern __nonnull((1)) void rewind __P((FILE *__restrict __fp));
extern __nonnull((1)) int fpurge __P((FILE *__restrict __fp));
extern int getchar(void);

extern __wunused __nonnull((1,4)) __size_t fread __P((void *__restrict __buf, __size_t __size,
                                                      __size_t __count, FILE *__restrict __fp));
extern __wunused __nonnull((1,4)) __size_t fwrite __P((const void *__restrict __buf, __size_t __size,
                                                       __size_t __count, FILE *__restrict __fp));
extern __wunused __nonnull((1)) int fgetc __P((FILE *__restrict __fp));
extern __nonnull((2)) int fputc __P((int c, FILE *__restrict __fp));
extern __nonnull((1,3)) char *fgets __P((char *__restrict __buf, int bufsize, FILE *__restrict __fp));
extern __nonnull((1,2)) int fputs __P((char const *__restrict __s, FILE *__restrict __fp));
#ifndef __KERNEL__
extern __nonnull((1)) int getc __P((FILE *__restrict __fp));
extern __nonnull((2)) int putc __P((int c, FILE *__restrict __fp));
#endif
#define getc  fgetc
#define putc  fputc
extern __nonnull((1)) int getw __P((FILE *__restrict __fp));
extern __nonnull((2)) int putw __P((int w, FILE *__restrict __fp));

extern __nonnull((1,2)) __attribute_vaformat(__printf__,2,3) int fprintf __P((FILE *__restrict __fp, char const *__restrict __fmt, ...));
extern __nonnull((1,2)) __attribute_vaformat(__printf__,2,0) int vfprintf __P((FILE *__restrict __fp, char const *__restrict __fmt, va_list __args));

#ifndef __STDC_PURE__
#ifdef __KERNEL__
#define _fseek64 fseek64
#define _ftell64 ftell64
#else
extern __nonnull((1)) int fseek64 __P((FILE *__restrict __fp, __s64 __off, int __whence));
extern __nonnull((1)) __u64 ftell64 __P((FILE *__restrict __fp));
#endif
#endif

extern __nonnull((1)) int _fseek64 __P((FILE *__restrict __fp, __s64 __off, int __whence));
extern __nonnull((1)) __u64 _ftell64 __P((FILE *__restrict __fp));

extern __nonnull((1,2)) __attribute_vaformat(__scanf__,2,3) int fscanf __P((FILE *__restrict __fp, char const *__restrict __fmt, ...));
extern __nonnull((1,2)) __attribute_vaformat(__scanf__,2,0) int vfscanf __P((FILE *__restrict __fp, char const *__restrict __fmt, va_list __args));

#ifndef __KERNEL__
extern __nonnull((1)) int fileno __P((FILE *__restrict __fp));
// extern void     clearerr(FILE *);
// extern int      ferror(FILE *);
// extern void     flockfile(FILE *);
// extern int      ftrylockfile(FILE *);
// extern void     funlockfile(FILE *);
// extern int      getc_unlocked(FILE *);
// extern int      pclose(FILE *);
// extern FILE    *popen(char const *, char const *);
// extern int      putc_unlocked(int, FILE *);
// extern void     setbuf(FILE *, char *);
// extern int      setvbuf(FILE *, char *, int, __size_t);
// extern FILE    *tmpfile(void);
// extern int      ungetc(int, FILE *);
#endif


extern int putchar __P((int c));
extern __nonnull((1)) int puts __P((char const *__restrict __s));
extern __nonnull((1)) __attribute_vaformat(__printf__,1,2) int printf __P((char const *__restrict __format, ...));
extern __nonnull((1)) __attribute_vaformat(__printf__,1,0) int vprintf __P((char const *__restrict __format, va_list __args));

extern __nonnull((2)) __attribute_vaformat(__printf__,2,3) __size_t _sprintf __P((char *__restrict __buf, char const *__restrict __format, ...));
extern __nonnull((2)) __attribute_vaformat(__printf__,2,0) __size_t _vsprintf __P((char *__restrict __buf, char const *__restrict __format, va_list __args));
extern __nonnull((3)) __attribute_vaformat(__printf__,3,4) __size_t _snprintf __P((char *__restrict __buf, __size_t bufsize, char const *__restrict __format, ...));
extern __nonnull((3)) __attribute_vaformat(__printf__,3,0) __size_t _vsnprintf __P((char *__restrict __buf, __size_t bufsize, char const *__restrict __format, va_list __args));

#ifndef __STDIO_C__
#ifdef __STDC_PURE__
/* Standard-compliant sprintf */
extern __nonnull((2)) __attribute_vaformat(__printf__,2,3) int sprintf __P((char *__restrict __buf, char const *__restrict __format, ...));
extern __nonnull((2)) __attribute_vaformat(__printf__,2,0) int vsprintf __P((char *__restrict __buf, char const *__restrict __format, va_list __args));
extern __nonnull((3)) __attribute_vaformat(__printf__,3,4) int snprintf __P((char *__restrict __buf, __size_t bufsize, char const *__restrict __format, ...));
extern __nonnull((3)) __attribute_vaformat(__printf__,3,0) int vsnprintf __P((char *__restrict __buf, __size_t bufsize, char const *__restrict __format, va_list __args));
#else
/* sprintf with a return value that makes sense.
 * NOTE: Unsigned because KOS doesn't have undefined behavior for sprintf. */
#ifndef __NO_asmname
extern __nonnull((2)) __attribute_vaformat(__printf__,2,3) __size_t sprintf __P((char *__restrict __buf, char const *__restrict __format, ...)) __asmname("_sprintf");
extern __nonnull((2)) __attribute_vaformat(__printf__,2,0) __size_t vsprintf __P((char *__restrict __buf, char const *__restrict __format, va_list __args)) __asmname("_vsprintf");
extern __nonnull((3)) __attribute_vaformat(__printf__,3,4) __size_t snprintf __P((char *__restrict __buf, __size_t bufsize, char const *__restrict __format, ...)) __asmname("_snprintf");
extern __nonnull((3)) __attribute_vaformat(__printf__,3,0) __size_t vsnprintf __P((char *__restrict __buf, __size_t bufsize, char const *__restrict __format, va_list __args)) __asmname("_vsnprintf");
#else
#   define sprintf   _sprintf
#   define vsprintf  _vsprintf
#   define snprintf  _snprintf
#   define vsnprintf _vsnprintf
#endif
#endif
#endif /* !__STDIO_C__ */

extern __nonnull((1,2)) __attribute_vaformat(__scanf__,2,3) int sscanf __P((char const *__restrict __data, char const *__restrict __format, ...));
extern __nonnull((1,2)) __attribute_vaformat(__scanf__,2,0) int vsscanf __P((char const *__restrict __data, char const *__restrict __format, va_list __args));
extern __nonnull((1,3)) __attribute_vaformat(__scanf__,3,4) int _snscanf __P((char const *__restrict __data, __size_t __maxdata, char const *__restrict __format, ...));
extern __nonnull((1,3)) __attribute_vaformat(__scanf__,3,0) int _vsnscanf __P((char const *__restrict __data, __size_t __maxdata, char const *__restrict __format, va_list __args));

#ifndef __STDC_PURE__
#ifndef __NO_asmname
extern __nonnull((1,3)) __attribute_vaformat(__scanf__,3,4) int snscanf __P((char const *__restrict __data, __size_t __maxdata, char const *__restrict __format, ...)) __asmname("_snscanf");
extern __nonnull((1,3)) __attribute_vaformat(__scanf__,3,0) int vsnscanf __P((char const *__restrict __data, __size_t __maxdata, char const *__restrict __format, va_list __args)) __asmname("_vsnscanf");
#else
#define snscanf  _snscanf
#define vsnscanf _vsnscanf
#endif
#endif


#ifndef __CONFIG_MIN_LIBC__
//////////////////////////////////////////////////////////////////////////
// Removes a given file/directory.
// NOTE: In KOS, this is a level #2 (system) call, instead of a level #3,
//       and are implemented in /src/libc/unistd.c
extern __nonnull((1)) int remove __P((char const *__pathname));
extern __nonnull((2)) int removeat __P((int __dirfd, char const *__pathname));
#endif /* !__CONFIG_MIN_LIBC__ */


#ifndef __CONFIG_MIN_BSS__
extern __coldcall __nonnull((1)) void perror __P((char const *__s));
#endif

#ifndef __CONFIG_MIN_LIBC__
extern __nonnull((2)) __attribute_vaformat(__printf__,2,3) int dprintf __P((int __fd, char const *__restrict __format, ...));
extern __nonnull((2)) __attribute_vaformat(__printf__,2,0) int vdprintf __P((int __fd, char const *__restrict __format, va_list __args));
#endif /* !__CONFIG_MIN_LIBC__ */
#endif /* !__ASSEMBLY__ */

__DECL_END

#endif /* !_INC_STDIO */
#endif /* !_STDIO_H */
#endif /* !__STDIO_H__ */

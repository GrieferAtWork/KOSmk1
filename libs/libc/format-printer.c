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
#ifndef __FORMAT_PRINTER_C__
#define __FORMAT_PRINTER_C__ 1
#undef __STDC_PURE__

#include <assert.h>
#include <ctype.h>
#include <format-printer.h>
#include <malloc.h>
#include <itos.h>
#include <kos/compiler.h>
#include <kos/config.h>
#include <kos/kernel/debug.h>
#include <kos/syscall.h>
#include <ctype.h>
#include <math.h>
#include <stddef.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <alloca.h>
#include <sys/types.h>
#ifdef __KERNEL__
#include <kos/atomic.h>
#include <kos/kernel/paging.h>
#include <kos/kernel/util/string.h>
#endif

__DECL_BEGIN

__public int format_printf(pformatprinter printer, void *closure,
                           char const *__restrict format, ...) {
 int result;
 va_list args;
 va_start(args,format);
 result = format_vprintf(printer,closure,format,args);
 va_end(args);
 return result;
}

#define PRINTF_EXTENSION_POINTERSIZE 1        /*< Allow pointer-size length modifiers 'I' */
#define PRINTF_EXTENSION_FIXLENGTH   1        /*< Allow fixed length modifiers 'I8|I16|I32|I64' */
#define PRINTF_EXTENSION_DOTQUESTION 1        /*< Allow '.?' in place of '.*' to read size_t from the argument list instead of 'unsigned int' */
#define PRINTF_EXTENSION_QUOTESTRING 1        /*< Allow '%q' in place of '%s' for quoting a string (escaping all special characters). */
#define PRINTF_EXTENSION_NULLSTRING  "(null)" /*< Replace NULL-arguments to %s and %q with this string (don't define to cause undefined behavior) */
#define STRFTIME_EXTENSION_LONGNAMES 1        /*< Allow %[...] for long attribute names. */
#define PRINTF_EXTENSION_VIRTUALPTR  1        /*< Allow '%~' to describe virtual pointers. */

#ifndef __KERNEL__
#undef PRINTF_EXTENSION_VIRTUALPTR
#define PRINTF_EXTENSION_VIRTUALPTR 0
#endif

enum printf_length {
 len_I8,len_I16,len_I32,len_I64,
 len_L    = 'L',len_z = 'z',len_t = 't',
 len_none = __PP_CAT_2(len_I,__PP_MUL8(__SIZEOF_INT__)),
 len_I    = __PP_CAT_2(len_I,__PP_MUL8(__SIZEOF_POINTER__)),
#ifdef __SIZEOF_CHAR__
 len_hh   = __PP_CAT_2(len_I,__PP_MUL8(__SIZEOF_CHAR__)),
#else
 len_hh   = len_I8,
#endif
 len_h    = __PP_CAT_2(len_I,__PP_MUL8(__SIZEOF_SHORT__)),
 len_l    = __PP_CAT_2(len_I,__PP_MUL8(__SIZEOF_LONG__)),
 len_ll   = __PP_CAT_2(len_I,__PP_MUL8(__SIZEOF_LONG_LONG__)),
 len_j    = len_I64, /* intmax_t */
};

#define PRINTF_FLAG_NONE     0x0000
#define PRINTF_FLAG_LJUST    0x0001 /*< '%-'. */
#define PRINTF_FLAG_SIGN     0x0002 /*< '%+'. */
#define PRINTF_FLAG_SPACE    0x0004 /*< '% '. */
#define PRINTF_FLAG_PREFIX   0x0008 /*< '%#'. */
#define PRINTF_FLAG_PADZERO  0x0010 /*< '%0'. */
#define PRINTF_FLAG_HASWIDTH 0x0020 /*< '%123'. */
#define PRINTF_FLAG_HASPREC  0x0040 /*< '%.123'. */
#define PRINTF_FLAG_SIGNED   0x0080
#ifdef __KERNEL__
#define PRINTF_FLAG_VIRTUAL  0x0100 /*< '%~' (Modifies %s and %q: the pointer referrs to a user-space address). */
#endif

#ifdef __KERNEL__
/* Prevent infinite-recursion, as assert() uses this function as well
 * >> While this should _never_ happen, it can potentially prevent
 *    the worst-case-scenario during absolute fatality. */
static __atomic int __in_formatprintf_check = 0;
#define __printf_assert(x) \
 (katomic_cmpxch(__in_formatprintf_check,0,1)\
  ? ((x),katomic_store(__in_formatprintf_check,0))\
  : (void)0)
#else
#define __printf_assert
#endif

#define print(p,s) \
do if __unlikely((error = (*printer)(p,s,closure)) != 0) return error;\
while(0)
#define quote(p,s,flags) \
do if __unlikely((error = format_quote(printer,closure,p,s,flags)) != 0) return error;\
while(0)
#define printf(...) \
do if __unlikely((error = format_printf(printer,closure,__VA_ARGS__)) != 0) return error;\
while(0)

#ifdef __KERNEL__
int format_userprint(pformatprinter printer, void *closure,
                     __user char const *userstring, size_t maxchars) {
 struct ktranslator trans; char *addr;
 int error; size_t partmaxsize,partsize;
 KTASK_CRIT_MARK
 error = ktranslator_init(&trans,ktask_self());
 if __unlikely(KE_ISERR(error)) return error;
 for (;;) {
  addr = (char *)ktranslator_exec(&trans,userstring,maxchars,
                                  &partmaxsize,0);
  if __unlikely(!addr) { error = KE_FAULT; break; }
  partmaxsize /= sizeof(char);
  partsize = strnlen(addr,min(maxchars,partmaxsize))*sizeof(char);
  error = (*printer)(addr,partsize,closure);
  if (error != 0 ||
      partmaxsize != (partsize/sizeof(char)) ||
      maxchars == partsize) break;
  *(uintptr_t *)&userstring += partmaxsize;
  maxchars                  -= partsize;
 }
 ktranslator_quit(&trans);
 return error;
}

struct format_userquote_args {
 pformatprinter real_printer;
 void          *real_closure;
 __u32          quote_flags;
};
static int
format_userquote_callback(char const *text, size_t maxlen,
                          struct format_userquote_args *args) {
 return format_quote(args->real_printer,args->real_closure,
                     text,maxlen,args->quote_flags);
}

int format_userquote(pformatprinter printer, void *closure,
                     __user char const *userstring, size_t maxchars,
                     __u32 flags) {
 int error;
 struct format_userquote_args args = {printer,closure,flags|FORMAT_QUOTE_FLAG_PRINTRAW};
 if (!(flags&FORMAT_QUOTE_FLAG_PRINTRAW)) {
  error = (*printer)("\"",1,closure);
  if __unlikely(error != 0) goto end;
  error = format_userprint((pformatprinter)&format_userquote_callback,
                            &args,userstring,maxchars);
  if __unlikely(error != 0) goto end;
  error = (*printer)("\"",1,closure);
 } else {
  error = format_userprint((pformatprinter)&format_userquote_callback,
                            &args,userstring,maxchars);
 }
end:
 return error;
}
#endif /* __KERNEL__ */


__public int format_vprintf(pformatprinter printer, void *closure,
                            char const *__restrict format, va_list args) {
 char const *format_begin,*iter; char ch; int error;
#if defined(__KERNEL__) && defined(__DEBUG__)
#define readch() (__printf_assert(kassertbyte(iter)),*iter++)
#define peekch() (__printf_assert(kassertbyte(iter)),*iter)
#else
#define readch() (*iter++)
#define peekch() (*iter)
#endif
 iter = format_begin = format;
 __printf_assert(kassertbyte(printer));
 for (;;) {
next:
  ch = readch();
  switch (ch) {
   case '\0': goto end;
   case '%': {
    enum printf_length length;
    unsigned int flags;
    size_t width,precision;
    if (format_begin != iter) {
     print(format_begin,((size_t)(iter-format_begin))-1);
    }
    flags = PRINTF_FLAG_NONE;
flag_next: /* Flags */
    ch = readch();
    switch (ch) {
     case '-': flags |= PRINTF_FLAG_LJUST;   goto flag_next;
     case '+': flags |= PRINTF_FLAG_SIGN;    goto flag_next;
     case ' ': flags |= PRINTF_FLAG_SPACE;   goto flag_next;
     case '#': flags |= PRINTF_FLAG_PREFIX;  goto flag_next;
     case '0': flags |= PRINTF_FLAG_PADZERO; goto flag_next;
     default: break;
    }
    /* Width */
#if PRINTF_EXTENSION_DOTQUESTION
#if __SIZEOF_INT__ != __SIZEOF_SIZE_T__
    if (ch == '?') {
     width = va_arg(args,size_t);
     goto have_width;
    } else
    if (ch == '*')
#else
    if (ch == '*' || ch == '?')
#endif
#else
    if (ch == '*')
#endif
    {
     width = (size_t)va_arg(args,unsigned int); /* Technically int, but come on... */
#if PRINTF_EXTENSION_DOTQUESTION && (__SIZEOF_INT__ != __SIZEOF_SIZE_T__)
have_width:
#endif
     flags |= PRINTF_FLAG_HASWIDTH;
     ch = readch();
    } else if (ch >= '0' && ch <= '9') {
     width = (size_t)(ch-'0');
     for (;;) {
      ch = readch();
      if (ch < '0' || ch > '9') break;
      width = (size_t)(width*10+(ch-'0'));
     }
     flags |= PRINTF_FLAG_HASWIDTH;
    } else {
     width = 0;
    }
    /* Precision */
    if (ch == '.') {
     ch = readch();
     flags |= PRINTF_FLAG_HASPREC;
#if PRINTF_EXTENSION_DOTQUESTION
#if __SIZEOF_INT__ != __SIZEOF_SIZE_T__
     if (ch == '?') {
      precision = va_arg(args,size_t);
      goto have_precision;
     } else
     if (ch == '*')
#else
     if (ch == '*' || ch == '?')
#endif
#else
     if (ch == '*')
#endif
     {
      precision = (size_t)va_arg(args,unsigned int); /* Technically int, but come on... */
#if PRINTF_EXTENSION_DOTQUESTION && (__SIZEOF_INT__ != __SIZEOF_SIZE_T__)
have_precision:
#endif
      ch = readch();
     } else if __likely(ch >= '0' && ch <= '9') {
      precision = ch-'0';
      for (;;) {
       ch = readch();
       if (ch < '0' || ch > '9') break;
       precision = (size_t)(precision*10+(ch-'0'));
      }
     } else {
      /* Undefined behavior (ignore) */
      ch = *--iter;
      flags &= ~(PRINTF_FLAG_HASPREC);
      precision = 0;
     }
    } else {
     precision = 0;
    }
    /* Length */
    switch (ch) {
     case 'h': ch = readch(); if (ch == 'h') length = len_hh,ch = readch(); else length = len_h; break;
     case 'l': ch = readch(); if (ch == 'l') length = len_ll,ch = readch(); else length = len_l; break;
     case 'j': ch = readch(); length = len_j; break;
     case 'z': case 't': case 'L':
      length = (enum printf_length)ch;
      ch = readch();
      break;
#if PRINTF_EXTENSION_POINTERSIZE || PRINTF_EXTENSION_FIXLENGTH
     case 'I': {
#if PRINTF_EXTENSION_FIXLENGTH
      int off; ch = readch();
      if (ch == '8') length = len_I8,off = 1;
      else if (ch == '1' && peekch() == '6') length = len_I16,off = 2;
      else if (ch == '3' && peekch() == '2') length = len_I32,off = 2;
      else if (ch == '6' && peekch() == '4') length = len_I64,off = 2;
#if PRINTF_EXTENSION_POINTERSIZE
      else length = len_I,off = 0;
#else
      else { --iter; goto def_length; }
#endif
      iter += off,ch = iter[-1];
#else /* PRINTF_EXTENSION_FIXLENGTH */
      length = len_I;
      ch = readch();
#endif /* !PRINTF_EXTENSION_FIXLENGTH */
     } break;
#endif /* PRINTF_EXTENSION_POINTERSIZE || PRINTF_EXTENSION_FIXLENGTH */
#if !PRINTF_EXTENSION_POINTERSIZE && PRINTF_EXTENSION_FIXLENGTH
def_length:
#endif
     default: length = len_none; break;
    }

    switch (ch) {
     case '\0': goto end;
     case '%': format_begin = iter; goto next;

     {
      int numsys;
      char buf[32];
      char *bufbegin;
      size_t bufused;
      union {
       __s32 s_32; __s64 s_64;
       __u32 u_32; __u64 u_64;
      } arg;
      if (0) { case 'o': numsys = 8; }
      if (0) { case 'u': numsys = 10; if __unlikely(length == len_t) flags |= PRINTF_FLAG_SIGNED; }
      if (0) { case 'd': case 'i': numsys = 10; if __likely(length != len_z) flags |= PRINTF_FLAG_SIGNED; }
      if (0) { case 'x': case 'X': case 'p': numsys = 16; if __unlikely(length == len_t) flags |= PRINTF_FLAG_SIGNED; }
      if (length == len_I64) arg.u_64 = va_arg(args,__u64);
      else arg.u_32 = va_arg(args,__u32);
#if __SIZEOF_INT__ < 4
#error This printf implementation required sizeof(int) >= 4
#endif
      bufbegin = buf;
      if ((flags&(PRINTF_FLAG_SPACE|PRINTF_FLAG_SIGNED)) ==
                 (PRINTF_FLAG_SPACE|PRINTF_FLAG_SIGNED) &&
          (length == len_I64 ? arg.s_64 >= 0 : arg.s_32 >= 0)
           ) *bufbegin++ = ' ';
      if (flags&PRINTF_FLAG_PREFIX && numsys != 10) {
       *bufbegin++ = '0';
       if (numsys == 16) *bufbegin++ = 'x';
      }
      if (length == len_I64) {
       if (flags&PRINTF_FLAG_SIGNED) {
        bufused = _itos64_ns(bufbegin,sizeof(buf)-(bufbegin-buf),arg.s_64,numsys);
       } else {
        bufused = _utos64_ns(bufbegin,sizeof(buf)-(bufbegin-buf),arg.u_64,numsys);
       }
      } else {
       if (flags&PRINTF_FLAG_SIGNED) {
        bufused = _itos32_ns(bufbegin,sizeof(buf)-(bufbegin-buf),arg.s_32,numsys);
       } else {
        bufused = _utos32_ns(bufbegin,sizeof(buf)-(bufbegin-buf),arg.u_32,numsys);
       }
      }
      if (precision > bufused) {
       size_t offset = precision-bufused;
       memmove(bufbegin+offset,bufbegin,bufused);
       memset(bufbegin,'0',offset);
       bufused = precision;
      }
      bufused += (size_t)(bufbegin-buf);
      if (flags&PRINTF_FLAG_LJUST) print(buf,bufused);
      if (width > bufused) {
       size_t overflow = width-bufused;
       char spacebuf[16];
       memset(spacebuf,(flags&PRINTF_FLAG_PADZERO)
              ? '0' : ' ',min(sizeof(spacebuf),overflow));
       while (overflow > sizeof(spacebuf)) {
        print(spacebuf,sizeof(spacebuf));
        overflow -= sizeof(spacebuf);
       }
       __printf_assert(assert(overflow));
       print(spacebuf,overflow);
      }
      if (!(flags&PRINTF_FLAG_LJUST)) print(buf,bufused);
      break;
     case 'c':
      buf[0] = va_arg(args,int);
      print(buf,1);
      break;
     }

     {
      char const *arg;
     case 's':
#if PRINTF_EXTENSION_QUOTESTRING
     case 'q':
#endif /* PRINTF_EXTENSION_QUOTESTRING */
      arg = va_arg(args,char const *);
#if PRINTF_EXTENSION_VIRTUALPTR
      if (flags&PRINTF_FLAG_VIRTUAL && arg) {
       if __likely(ch == 's') {
        error = format_userprint(printer,closure,arg,
                                (flags&PRINTF_FLAG_HASPREC) ? precision : (size_t)-1);
       } else {
        error = format_userquote(printer,closure,arg,
                                (flags&PRINTF_FLAG_HASPREC) ? precision : (size_t)-1,
                                (flags&PRINTF_FLAG_PREFIX) ? FORMAT_QUOTE_FLAG_PRINTRAW : FORMAT_QUOTE_FLAG_NONE);
       }
       if __unlikely(error != 0) return error;
      } else
#endif
      {
#ifdef PRINTF_EXTENSION_NULLSTRING
       if __unlikely(!arg) arg = PRINTF_EXTENSION_NULLSTRING;
#endif /* PRINTF_EXTENSION_NULLSTRING */
#if PRINTF_EXTENSION_QUOTESTRING
       if __likely(ch == 's') {
        print(arg,(flags&PRINTF_FLAG_HASPREC) ? precision : (size_t)-1);
       } else {
        quote(arg,(flags&PRINTF_FLAG_HASPREC) ? precision : (size_t)-1,
             (flags&PRINTF_FLAG_PREFIX) ? FORMAT_QUOTE_FLAG_PRINTRAW : FORMAT_QUOTE_FLAG_NONE);
       }
#else /* PRINTF_EXTENSION_QUOTESTRING */
       print(arg,(flags&PRINTF_FLAG_HASPREC) ? precision : (size_t)-1);
#endif /* !PRINTF_EXTENSION_QUOTESTRING */
      }
      break;
     }

     /* print a floating point value */
     {
      int g;
      double d;
      char buf[128];
      size_t sz;
     case 'f':
     case 'g':
      g = (ch == 'g');
      d = va_arg(args,double);
      if (!(flags&PRINTF_FLAG_HASPREC)) precision = 6;
      sz = _dtos(buf,sizeof(buf)-1,d,
                 width ? width : 1,
                 precision,g);
      print(buf,sz);
      break;
     }

     default: goto next;
    }
    format_begin = iter;
   } break;
   default: break;
  }
 }
end:
 if (format_begin != iter) {
  print(format_begin,(size_t)(iter-format_begin));
 }
#undef peekch
#undef readch
 return 0;
}



__public int format_scanf(pformatscanner scanner, pformatreturn returnch,
                          void *closure, char const *__restrict format, ...) {
 va_list args; int result;
 va_start(args,format);
 result = format_vscanf(scanner,returnch,closure,format,args);
 va_end(args);
 return result;
}
__public int format_vscanf(pformatscanner scanner, pformatreturn returnch,
                           void *closure, char const *__restrict format, va_list args) {
#define doload()  do{ if __unlikely((error = (*scanner)(&rch,closure)) != 0) goto end; }while(0)
#define load()    do{ if (!stored) { if __unlikely((error = (*scanner)(&rch,closure)) != 0) goto end; stored = 1; } }while(0)
#define take()   (stored = 0)
#define retch(ch) do{ if (returnch) { if __unlikely((error = (*returnch)(ch,closure)) != 0) goto end; } }while(0)
#if defined(__KERNEL__) && defined(__DEBUG__)
#define peekch()  (kassertbyte(iter),*iter)
#define readch()  (kassertbyte(iter),*iter++)
#define D(x)       kassertbyte(x),*(x)
#else
#define peekch()  (*iter)
#define readch()  (*iter++)
#define D          *
#endif
 char const *iter; char fch; int rch,error,nread = 0,stored = 0;
 kassertbyte(scanner);
 iter = format;
 for (;;) {
  kassertbyte(iter);
  fch = readch();
parsefch:
  switch (fch) {
   case '\0': goto done;
   case '\n': /* Skip any kind of linefeed */
    load();
    if (rch == '\n') take();
    else if (rch == '\r') {
     doload();
     if (rch == '\n') take(); /* '\r\n' */
    } else goto done;
    break;
   case ' ': /* Skip whitespace */
    for (;;) { load(); if (rch < 0 || !isspace(rch)) break; take(); }
    break;

   {
    enum printf_length length;
    size_t width,bufsize;
    int ignore_data;
   case '%':
    kassertbyte(iter);
    fch = readch();
    if (fch == '%') goto def;
    ignore_data = (fch == '*');
    if (ignore_data) fch = readch();
    /* Parse the buffersize modifier */
    if (fch == '$') fch = readch(),bufsize = va_arg(args,size_t);
    else bufsize = (size_t)-1;
    /* Parse the width modifier */
    if (fch >= '0' && fch <= '9') {
     width = (size_t)(fch-'0');
     for (;;) {
      fch = readch();
      if (fch < '0' || fch > '9') break;
      width = width*10+(size_t)(fch-'0');
     }
    } else width = (size_t)-1;
    /* Parse the length modifier */
    switch (fch) {
     case 'h': fch = readch(); if (fch == 'h') length = len_hh,fch = readch(); else length = len_h; break;
     case 'l': fch = readch(); if (fch == 'l') length = len_ll,fch = readch(); else length = len_l; break;
     case 'j': fch = readch(); length = len_j; break;
     case 'z': case 't': case 'L':
      length = (enum printf_length)fch;
      fch = readch();
      break;
     case 'I': {
      int off; fch = readch();
      if (fch == '8') length = len_I8,off = 1;
      else if (fch == '1' && peekch() == '6') length = len_I16,off = 2;
      else if (fch == '3' && peekch() == '2') length = len_I32,off = 2;
      else if (fch == '6' && peekch() == '4') length = len_I64,off = 2;
      else length = len_I,off = 0;
      iter += off,fch = iter[-1];
     } break;
     default: length = len_none; break;
    }
    switch (fch) {

     case 'd':
      (void)length; // TODO
      break;

     {
      int inverse,found;
      char const *sel_begin,*sel_iter,*sel_end;
      char *bufdst,*bufend;
     case 's':
      bufdst = va_arg(args,char *);
      bufend = bufdst+bufsize;
      /* Read until a whitespace character is found */
      while (bufdst != bufend && width--) {
       load();
       if (rch < 0 || isspace(rch)) break;
       D(bufdst)++ = rch;
       take();
      }
      if (bufdst != bufend) D(bufdst) = '\0';
      ++nread;
      break;

     case '[':
      fch = readch();
      inverse = (fch == '^');
      if (inverse) fch = readch();
      sel_begin = iter;
      while (1) {
       if (fch == '\\' && peekch()) ++iter; /* escape the following character */
       else if (fch == ']') { sel_end = iter,fch = readch(); break; }
       else if (!fch) { sel_end = iter; break; }
       fch = readch();
      }
      bufdst = va_arg(args,char *);
      bufend = bufdst+bufsize;
      while (bufdst != bufend && width--) {
       load();
       sel_iter = sel_begin;
       found = 0;
       if (rch < 0) break;
       while (sel_iter != sel_end) {
        if (sel_iter[1] == '-') { /* range */
         if (rch >= sel_iter[0] && rch <= sel_iter[2]) { found = 1; break; }
         sel_iter += 3;
        } else if (*sel_iter == '\\') {
         if (rch == sel_iter[1]) { found = 1; break; }
         sel_iter += 2;
        } else {
         if (rch == *sel_iter) { found = 1; break; }
         ++sel_iter;
        }
       }
       if ((found ^ inverse) == 0) break;
       D(bufdst)++ = rch;
       take();
      }
      if (bufdst != bufend) D(bufdst) = '\0';
      ++nread;
      if (!fch) goto done;
      goto parsefch;
     }
     default: break;
    }
    break;
   }

   default:def:
    load();
    if (rch != fch) goto done;
    take();
    break;
  }
 }
done:
 error = nread;
 /* Return the last character if it's still stored */
 if (stored && rch >= 0) retch(rch);
end:
 return error;
#undef D
#undef readch
#undef peekch
#undef retch
#undef take
#undef load
}


__private char const abbr_month_names[12][4] = {
 "Jan","Feb","Mar","Apr","May","Jun",
 "Jul","Aug","Sep","Oct","Nov","Dec"};
__private char const abbr_wday_names[7][4] = {
 "Sun","Mon","Tue","Wed","Thu","Fri","Sat"};

static char const *full_month_names[12] = {
 "January","February","March","April","May","June",
 "July","August","September","October","November","December"};
static char const *full_wday_names[7] = {
 "Sunday","Monday","Tuesday","Wednesday",
 "Thursday","Friday","Saturday"};
static char const am_pm[2][3] = {"AM","PM"};
static char const am_pm_lower[2][3] = {"am","pm"};

#if STRFTIME_EXTENSION_LONGNAMES
enum{
 ID_NONE,
 ID_YEAR,
 ID_MONTH,
 ID_WDAY,
 ID_MDAY,
 ID_YDAY,
 ID_HOUR,
 ID_MINUTE,
 ID_SECOND,
 ID_COUNT,
};

static ptrdiff_t id_fields[ID_COUNT] = {
#ifndef __INTELLISENSE__
 [ID_NONE]   = 0,
 [ID_YEAR]   = offsetof(struct tm,tm_year),
 [ID_MONTH]  = offsetof(struct tm,tm_mon),
 [ID_WDAY]   = offsetof(struct tm,tm_wday),
 [ID_MDAY]   = offsetof(struct tm,tm_mday),
 [ID_YDAY]   = offsetof(struct tm,tm_yday),
 [ID_HOUR]   = offsetof(struct tm,tm_hour),
 [ID_MINUTE] = offsetof(struct tm,tm_min),
 [ID_SECOND] = offsetof(struct tm,tm_sec),
#endif
};

__local int strftime_attrid(char const *name_begin, size_t name_len) {
 switch (name_len) {
  case 1:
   switch (*name_begin) {
    case 'Y': return ID_YEAR;
    case 'M': return ID_MONTH;
    case 'D': return ID_MDAY;
    case 'H': return ID_HOUR;
    case 'I': return ID_MINUTE;
    case 'S': return ID_SECOND;
    default: break;
   }
   break;
  case 2:
   if (*name_begin == 'M') {
    if (name_begin[1] == 'D') return ID_MDAY;
    if (name_begin[1] == 'I') return ID_MINUTE;
   }
   else if (*name_begin == 'Y' && name_begin[1] == 'D') return ID_YDAY;
   else if (*name_begin == 'W' && name_begin[1] == 'D') return ID_WDAY;
   break;

#define CHECK(n,name,mode) if (memcmp(name_begin,name,(n)*sizeof(char))==0) return mode
  case 4:
   CHECK(4,"year",ID_YEAR);
   CHECK(4,"wday",ID_WDAY);
   CHECK(4,"mday",ID_MDAY);
   CHECK(4,"yday",ID_YDAY);
   CHECK(4,"hour",ID_HOUR);
   break;
  case 5:
   CHECK(5,"month",ID_MONTH);
   break;
  case 6:
   CHECK(6,"minute",ID_MINUTE);
   CHECK(6,"second",ID_SECOND);
   break;
#undef CHECK
  default: break;
 }
 return ID_NONE;
}
#endif /* STRFTIME_EXTENSION_LONGNAMES */


int format_strftime(pformatprinter printer, void *closure,
                    char const *__restrict format, struct tm const *tm) {
 char const *format_begin,*iter; char ch; int error;
#if defined(__KERNEL__) && defined(__DEBUG__)
#define readch() (__printf_assert(kassertbyte(iter)),*iter++)
#define peekch() (__printf_assert(kassertbyte(iter)),*iter)
#else
#define readch() (*iter++)
#define peekch() (*iter)
#endif

 iter = format_begin = format;
 __printf_assert(kassertbyte(printer));
 for (;;) {
next:
  ch = readch();
  switch (ch) {
   case '\0': goto end;
   case '%': {
    if (format_begin != iter) {
     print(format_begin,((size_t)(iter-format_begin))-1);
    }
#define safe_elem(arr,i) ((arr)[min(i,__compiler_ARRAYSIZE(arr)-1)])


    switch ((ch = *iter++)) {
//TODO: @begin locale_dependent
     case 'a': print(safe_elem(abbr_wday_names,tm->tm_wday),(size_t)-1); break;
     case 'A': print(safe_elem(full_wday_names,tm->tm_wday),(size_t)-1); break;
     case 'h':
     case 'b': print(safe_elem(abbr_month_names,tm->tm_mon),(size_t)-1); break;
     case 'B': print(safe_elem(full_month_names,tm->tm_mon),(size_t)-1); break;
     case 'c': printf("%s %s %0.2u %0.2u:%0.2u:%0.2u %u"
                     ,abbr_wday_names[tm->tm_wday]
                     ,abbr_month_names[tm->tm_mon]
                     ,tm->tm_mday,tm->tm_hour
                     ,tm->tm_min,tm->tm_sec
                     ,tm->tm_year+1900);
               break;
     case 'x': printf("%0.2u/%0.2u/%0.2u"
                     ,tm->tm_mon+1,tm->tm_mday
                     ,tm->tm_year+1900);
               break;
     case 'X': printf("%0.2u:%0.2u:%0.2u"
                     ,tm->tm_hour,tm->tm_min
                     ,tm->tm_sec);
               break;
     case 'z': break; // TODO: ISO 8601 offset from UTC in timezone (1 minute=1, 1 hour=100) | If timezone cannot be determined, no characters	+100
     case 'Z': break; // TODO: Timezone name or abbreviation * | If timezone cannot be determined, no characters	CDT
//TODO: @end locale_dependent
     case 'C': printf("%0.2u",((tm->tm_year+1900)/100)%100); break;
     case 'd': printf("%0.2u",tm->tm_mday); break;
     case 'D': printf("%0.2u/%0.2u/%0.2u",tm->tm_mon+1,tm->tm_mday,(tm->tm_year+1900)%100); break;
     case 'e': printf("%0.2u",tm->tm_mday); break;
     case 'F': printf("%0.4u-%0.2u-%0.2u",tm->tm_year+1900,tm->tm_mon+1,tm->tm_mday); break;
     case 'H': printf("%0.2u",tm->tm_hour); break;
     case 'I': printf("%0.2u",tm->tm_hour); break;
     case 'j': printf("%0.3u",tm->tm_yday+1); break;
     case 'm': printf("%0.2u",tm->tm_mon+1); break;
     case 'M': printf("%0.2u",tm->tm_min); break;
     case 'p': print(safe_elem(am_pm,tm->tm_hour/12),(size_t)-1); break;
     case 'r': printf("%0.2u:%0.2u:%0.2u %s"
                     ,tm->tm_hour%12,tm->tm_min,tm->tm_sec
                     ,am_pm_lower[tm->tm_hour/12]);
               break;
     case 'R': printf("%0.2u:%0.2u",tm->tm_hour,tm->tm_min); break;
     case 'S': printf("%0.2u",tm->tm_sec); break;
     case 'T': printf("%0.2u:%0.2u:%0.2u",tm->tm_hour,tm->tm_min,tm->tm_sec); break;
     case 'u': printf("%0.2u",1+((tm->tm_wday+6)%7)); break;
     case 'w': printf("%0.2u",tm->tm_wday); break;
     case 'y': printf("%0.2u",(tm->tm_year+1900)%100); break;
     case 'Y': printf("%u",tm->tm_year+1900); break;

     // I don't understand this week-based stuff.
     // I read the wikipedia article, but I still don't really get it.
     // >> So this might be supported in the future when I understand it...
     // %g	Week-based year, last two digits (00-99)	01
     // %G	Week-based year	2001
     // %U	Week number with the first Sunday as the first day of week one (00-53)	33
     // %V	ISO 8601 week number (00-53)	34
     // %W	Week number with the first Monday as the first day of week one (00-53)	34

     case 'n': print("\n",1); break;
     case 't': print("\t",1); break;
     case '%': case '\0': print("%",1);
      if (!ch) goto end;
      break;

#if STRFTIME_EXTENSION_LONGNAMES
     case '[': {
      char const *tag_begin,*tag_end,*mode_begin,*mode_end;
      unsigned int bracket_recursion = 1; unsigned int attribval;
      int repr_mode,attribute_id,width = 0;
      /* Extended formatting */
      mode_end = mode_begin = tag_begin = iter;
      while (1) {
       ch = *iter++;
       if (ch == ']') { if (!--bracket_recursion) { tag_end = iter-1; break; } }
       else if (ch == '[') ++bracket_recursion;
       else if (ch == ':' && bracket_recursion == 1)
        mode_end = iter-1,tag_begin = iter;
       else if (!ch) { tag_end = iter; break; }
      }
      if (mode_begin != mode_end) {
       if (*mode_begin == 'n' || *mode_begin == 's' ||
           *mode_begin == 'S' || *mode_begin == ' ')
        repr_mode = *mode_begin++;
       else repr_mode = 0;
       /* Parse the width modifier */
       while (mode_begin != mode_end) {
        if (*mode_begin >= '0' && *mode_begin <= '9') {
         width = width*10+(*mode_begin-'0');
        } else goto format_end;
        ++mode_begin;
       }
      } else repr_mode = 0;
      attribute_id = strftime_attrid(tag_begin,(size_t)(tag_end-tag_begin));
      if __unlikely(attribute_id == ID_NONE) goto format_end;
      attribval = *(unsigned int *)((__u8 *)tm+id_fields[attribute_id]);
      if (repr_mode == 's' || repr_mode == 'S') {
       char const *repr_value;
       if (attribute_id == ID_MONTH) {
        repr_value = repr_mode == 'S'
         ? safe_elem(full_month_names,attribval)
         : safe_elem(abbr_month_names,attribval);
       } else if (attribute_id == ID_WDAY) {
        repr_value = repr_mode == 'S'
         ? safe_elem(full_wday_names,attribval)
         : safe_elem(abbr_wday_names,attribval);
       } else {
        goto format_end;
       }
       print(repr_value,(size_t)-1);
      } else {
       static char const suffix_values[] = "st" "nd" "rd" "th";
       if (width) {
        if (repr_mode != ' ') printf("%0.*u",(unsigned int)width,attribval);
        else                  printf( "%.*u",(unsigned int)width,attribval);
       } else printf("%u",attribval);
       if (repr_mode == 'n') {
        unsigned int suffix_offset = (attribval >= 3 ? 3 : attribval)*2;
        print(suffix_values+suffix_offset,2);
       }
      }
     } break;
#endif /* STRFTIME_EXTENSION_LONGNAMES */

     default:
      format_begin = iter-2;
      goto next;
    }
format_end:
    format_begin = iter;
   } break;
   default: break;
  }
 }
end:
 if (format_begin != iter) {
  print(format_begin,(size_t)(iter-format_begin));
 }
#undef peekch
#undef readch
 return 0;
}


#define tooct(x) ('0'+(x))

int
format_quote(pformatprinter printer, void *closure,
             char const *__restrict text, size_t maxtext,
             __u32 flags) {
 char encoded_text[4];
 size_t encoded_text_size;
 int error; char ch;
 char const *iter,*end,*flush_start;
 register int force_special = !!(flags&FORMAT_QUOTE_FLAG_NOASCII);
 __printf_assert(kassertbyte(printer));
 end = (iter = flush_start = text)+maxtext;
 encoded_text[0] = '\\';
 if (!(flags&FORMAT_QUOTE_FLAG_PRINTRAW)) print("\"",1);
 while (iter != end) {
  ch = *iter;
  if (ch < 32    || ch >= 127  || ch == '\'' ||
      ch == '\"' || ch == '\\' || force_special) {
   /* Character requires special encoding. */
   if (!ch && !(flags&FORMAT_QUOTE_FLAG_QUOTEALL)) goto end;
   /* Flush unprinted text. */
   if (iter != flush_start) {
    print(flush_start,(size_t)(iter-flush_start));
    flush_start = iter+1;
   }
   if (ch < 32) {
    /* Control character. */
    if (flags&FORMAT_QUOTE_FLAG_NOCTRL) {
default_ctrl:
     if (flags&FORMAT_QUOTE_FLAG_FORCEHEX) goto encode_hex;
encode_oct:
     if ((__u8)ch <= 0x07) {
      encoded_text[1] = tooct(((__u8)ch & 0x07));
      encoded_text_size = 2;
     } else if ((__u8)ch <= 0x38) {
      encoded_text[1] = tooct(((__u8)ch & 0x38) >> 3);
      encoded_text[2] = tooct(((__u8)ch & 0x07));
      encoded_text_size = 3;
     } else {
      encoded_text[1] = tooct(((__u8)ch & 0xC0) >> 6);
      encoded_text[2] = tooct(((__u8)ch & 0x38) >> 3);
      encoded_text[3] = tooct(((__u8)ch & 0x07));
      encoded_text_size = 4;
     }
     goto print_encoded;
    }
special_control:
    switch (ch) {
     case '\a':   ch = 'a'; break;
     case '\b':   ch = 'b'; break;
     case '\f':   ch = 'f'; break;
     case '\n':   ch = 'n'; break;
     case '\r':   ch = 'r'; break;
     case '\t':   ch = 't'; break;
     case '\v':   ch = 'v'; break;
     case '\033': ch = 'e'; break;
     case '\\': case '\'': case '\"': break;
     default: goto default_ctrl;
    }
    encoded_text[1] = ch;
    encoded_text_size = 2;
    goto print_encoded;
   } else if ((ch == '\\' || ch == '\'' || ch == '\"') &&
             !(flags&FORMAT_QUOTE_FLAG_NOCTRL)) {
    goto special_control;
   } else {
    /* Non-ascii character. */
/*default_nonascii:*/
    if (flags&FORMAT_QUOTE_FLAG_FORCEOCT) goto encode_oct;
encode_hex:
    encoded_text[1] = flags&FORMAT_QUOTE_FLAG_UPPERPRE ? 'X' : 'x';
    if ((__u8)ch <= 0xf) {
     encoded_text[2] = tohex(flags&FORMAT_QUOTE_FLAG_UPPERSUF,(__u8)ch);
     encoded_text_size = 3;
    } else {
     encoded_text[2] = tohex(flags&FORMAT_QUOTE_FLAG_UPPERSUF,((__u8)ch & 0xf0) >> 4);
     encoded_text[3] = tohex(flags&FORMAT_QUOTE_FLAG_UPPERSUF, (__u8)ch & 0x0f);
     encoded_text_size = 4;
    }
print_encoded:
    print(encoded_text,encoded_text_size);
    goto next;
   }
  }
next:
  ++iter;
 }
end:
 if (iter != flush_start) print(flush_start,(size_t)(iter-flush_start));
 if (!(flags&FORMAT_QUOTE_FLAG_PRINTRAW)) print("\"",1);
 return 0;
}

#define MAX_SPACE_SIZE  64
#define MAX_ASCII_SIZE  64

static int
print_space(pformatprinter printer,
            void *closure, size_t count) {
 size_t used_size,bufsize;
 char *spacebuf; int error;
 bufsize = min(count,MAX_SPACE_SIZE);
 spacebuf = (char *)alloca(bufsize);
 memset(spacebuf,' ',bufsize);
 for (;;) {
  used_size = min(count,bufsize);
  print(spacebuf,used_size);
  if (used_size == bufsize) break;
  bufsize -= used_size;
 }
 return 0;
}

__public int
format_hexdump(pformatprinter printer, void *closure,
               void const *__restrict data, size_t size,
               size_t linesize, __u32 flags) {
 char hex_buf[3],*ascii_line;
 char const *hex_translate;
 byte_t const *line,*iter,*end; byte_t b; int error;
 size_t lineuse,overflow,ascii_size; unsigned int offset_size;
 if __unlikely(!size) return 0;
 if __unlikely(!linesize) linesize = 16;
 if (!(flags&FORMAT_HEXDUMP_FLAG_NOASCII)) {
  /* Allocate a small buffer we can overwrite for ascii text. */
  ascii_size = min(MAX_ASCII_SIZE,linesize);
  ascii_line = (char *)alloca(ascii_size);
 }
 if (flags&FORMAT_HEXDUMP_FLAG_OFFSETS) {
  /* Figure out how wide we should pad the address offset field. */
  size_t i = (__size_t)1 << (offset_size = __SIZEOF_POINTER__*8-1);
  while (!(linesize&i)) --offset_size,i >>= 1;
  offset_size = ceildiv(offset_size,4);
 }
 /* The last character of the hex buffer is always a space. */
 hex_buf[2] = ' ';
 /* Figure out the hex translation vector that should be used. */
 hex_translate = __ctype_tohex[flags&FORMAT_HEXDUMP_FLAG_HEXLOWER];
 for (line = (byte_t const *)data;;) {
  if (linesize <= size) {
   lineuse  = linesize;
   overflow = 0;
  } else {
   lineuse  = size;
   overflow = linesize-size;
  }
  if (flags&(FORMAT_HEXDUMP_FLAG_ADDRESS|FORMAT_HEXDUMP_FLAG_OFFSETS)) {
   /* Must print some sort of prefix. */
   if (flags&FORMAT_HEXDUMP_FLAG_ADDRESS) printf("%p ",line);
   if (flags&FORMAT_HEXDUMP_FLAG_OFFSETS) printf("+%.*Ix ",offset_size,(uintptr_t)line-(uintptr_t)data);
  }
  end = line+lineuse;
  if (!(flags&FORMAT_HEXDUMP_FLAG_NOHEX)) {
   for (iter = line; iter != end; ++iter) {
    b = *iter;
    hex_buf[0] = hex_translate[(b&0xf0) >> 4];
    hex_buf[1] = hex_translate[b&0xf];
    print(hex_buf,__compiler_ARRAYSIZE(hex_buf));
   }
   if (overflow) {
    error = print_space(printer,closure,overflow*
                        __compiler_ARRAYSIZE(hex_buf));
    if __unlikely(error != 0) return error;
   }
  }
  if (!(flags&FORMAT_HEXDUMP_FLAG_NOASCII)) {
   for (iter = line; iter != end;) {
    char *aiter,*aend;
    size_t textcount = min(ascii_size,(size_t)(end-iter));
    memcpy(ascii_line,iter,textcount);
    /* Filter out non-printable characters, replacing them with '.' */
    aend = (aiter = ascii_line)+textcount;
    for (; aiter != aend; ++aiter) {
     if (!isprint(*aiter)) *aiter = '.';
    }
    /* Print our ascii text portion. */
    print(ascii_line,textcount);
    iter += textcount;
   }
   if (overflow) {
    /* Fill any overflow with space. */
    error = print_space(printer,closure,overflow);
    if __unlikely(error != 0) return error;
   }
  }
  if (size == lineuse) break;
  line  = end;
  size -= lineuse;
 }
 return 0;
}



__public int
stringprinter_init(struct stringprinter *__restrict self,
                   size_t hint) {
 kassertobj(self);
 if (!hint) hint = 4*sizeof(void *);
 self->sp_buffer = (char *)malloc((hint+1)*sizeof(char));
 if __unlikely(!self->sp_buffer) return -1;
 self->sp_bufpos = self->sp_buffer;
 self->sp_bufend = self->sp_buffer+hint;
 self->sp_bufend[0] = '\0';
 return 0;
}
__public char *
stringprinter_pack(struct stringprinter *__restrict self,
                   size_t *length) {
 char *result; size_t result_size;
 kassertobj(self);
 assert(self->sp_bufpos >= self->sp_buffer);
 assert(self->sp_bufpos <= self->sp_bufend);
 result_size = (size_t)(self->sp_bufpos-self->sp_buffer);
 if (self->sp_bufpos != self->sp_bufend) {
  result = (char *)realloc(self->sp_buffer,(result_size+1)*sizeof(char));
  if __unlikely(!result) result = self->sp_buffer;
 } else {
  result = self->sp_buffer;
 }
 result[result_size] = '\0';
 self->sp_buffer = NULL;
 if (length) *length = result_size;
 return result;
}
__public void
stringprinter_quit(struct stringprinter *__restrict self) {
 kassertobj(self);
 free(self->sp_buffer);
}
__public int
stringprinter_print(char const *__restrict data,
                    size_t maxchars, void *closure) {
 struct stringprinter *self = (struct stringprinter *)closure;
 size_t size_avail,newsize,reqsize;
 char *new_buffer;
 kassertobj(self);
 assert(self->sp_bufpos >= self->sp_buffer);
 assert(self->sp_bufpos <= self->sp_bufend);
 maxchars = strnlen(data,maxchars);
 size_avail = (size_t)(self->sp_bufend-self->sp_bufpos);
 if __unlikely(size_avail < maxchars) {
  /* Must allocate more memory. */
  newsize = (size_t)(self->sp_bufend-self->sp_buffer);
  assert(newsize);
  reqsize = newsize+(maxchars-size_avail);
  /* Double the buffer size until it is of sufficient length. */
  do newsize *= 2; while (newsize < reqsize);
  /* Reallocate the buffer (But include 1 character for the terminating '\0') */
  new_buffer = (char *)realloc(self->sp_buffer,(newsize+1)*sizeof(char));
  if __unlikely(!new_buffer) return -1;
  self->sp_bufpos = new_buffer+(self->sp_bufpos-self->sp_buffer);
  self->sp_bufend = new_buffer+newsize;
  self->sp_buffer = new_buffer;
 }
 memcpy(self->sp_bufpos,data,maxchars);
 self->sp_bufpos += maxchars;
 assert(self->sp_bufpos <= self->sp_bufend);
 return 0;
}



#undef printf
#undef quote
#undef print


__DECL_END

#endif /* !__FORMAT_PRINTER_C__ */

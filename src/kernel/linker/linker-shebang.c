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
#ifndef __KOS_KERNEL_LINKER_SHEBANG_C__
#define __KOS_KERNEL_LINKER_SHEBANG_C__ 1

#include <kos/compiler.h>
#include <kos/config.h>
#include <kos/errno.h>
#include <kos/syslog.h>
#include <kos/kernel/linker.h>
#include <kos/kernel/fs/file.h>
#include <stddef.h>
#include <malloc.h>
#include <ctype.h>

__DECL_BEGIN


//////////////////////////////////////////////////////////////////////////
// === ksbargs ===
kerrno_t
ksbargs_init(struct ksbargs *__restrict self,
             char const *__restrict text, size_t textsize) {
 char **argv,**new_argv,ch,*newarg,inquote;
 size_t arga,argc,new_arga; kerrno_t error;
 char const *iter,*end,*currarg_start;
 kassertobj(self);
 end = (iter = text)+textsize;
 argc = arga = 0;
 argv = NULL;
 goto set_currarg;
 for (;;) {
  if (iter == end || (ch = *iter,
      /* Check for quotation end. */
      ch == inquote || (!inquote &&
      /* Check for non-escaped whitespace character. */
    ((iter == text || iter[-1] != '\\') && isspace(ch))))) {
   size_t argsize = (size_t)(iter-currarg_start);
   /* Strip the final quotation mark from the argument string. */
   if (argsize) { /* Append the new argument. */
    k_syslogf(KLOG_INFO,"[Shebang] Found argument %.?q\n",
              argsize,currarg_start);
    if (argc == arga) {
     new_arga = arga ? arga*2 : 2;
     new_argv = (char **)realloc(argv,new_arga*sizeof(char *));
     if __unlikely(!new_argv) goto err_nomem;
     argv = new_argv,arga = new_arga;
    }
    newarg = (char *)malloc((argsize+1)*sizeof(char));
    if __unlikely(!newarg) goto err_nomem;
    memcpy(newarg,currarg_start,argsize*sizeof(char));
    newarg[argsize] = '\0';
    argv[argc++] = newarg;
   }
   /* Skip the quotation end. */
   if (iter != end && ch == inquote) ++iter;
set_currarg:
   while (iter != end && isspace(*iter)) ++iter;
   if (iter == end) break;
   ch = *iter;
   if (ch == '\"' || ch == '\'') {
    /* Quotation mark. */
    inquote       = ch;
    currarg_start = iter+1;
   } else {
    inquote       = 0;
    currarg_start = iter;
   }
  }
  /* Make sure the given character is printable. */
  if (!isprint(ch) && !isspace(ch)) { error = KE_INVAL; goto err; }
  ++iter;
 }
 /* Flush unused argument memory. */
 if (argc != arga) {
  assert(argc);
  new_argv = (char **)realloc(argv,argc*sizeof(char *));
  if (new_argv) argv = new_argv;
 }
 self->sb_argc = argc;
 self->sb_argv = argv;
 return KE_OK;
err_nomem: error = KE_NOMEM;
err: assert(KE_ISERR(error));
 new_argv = argv+argc;
 while (new_argv != argv) free(*--new_argv);
 free(argv);
 return error;
}


void ksbargs_quit(struct ksbargs *__restrict self) {
 char **iter,**end;
 kassertobj(self);
 end = (iter = self->sb_argv)+self->sb_argc;
 for (; iter != end; ++iter) free(*iter);
 free(self->sb_argv);
}


kerrno_t
ksbargs_prepend(struct ksbargs *__restrict self,
                size_t argc, char const *const *__restrict argv) {
 char **new_argv,**iter,**end;
 size_t new_argc;
 kassertobj(self);
 if (!argc) return KE_OK;
 new_argc = self->sb_argc+argc;
 new_argv = (char **)realloc(self->sb_argv,new_argc*sizeof(char *));
 if __unlikely(!new_argv) return KE_NOMEM;
 memmove(new_argv+argc,new_argv,self->sb_argc*sizeof(char *));
 end = (iter = new_argv)+argc;
 for (; iter != end; ++iter,++argv) {
  if __unlikely((*iter = strdup(*argv)) == NULL) goto err;
 }
 self->sb_argc = new_argc;
 self->sb_argv = new_argv;
 return KE_OK;
err:
 while (iter != new_argv) free(*--iter);
 memmove(new_argv,new_argv+argc,self->sb_argc*sizeof(char *));
 self->sb_argv = new_argv;
 new_argv = (char **)realloc(new_argv,self->sb_argc*sizeof(char *));
 if (new_argv) self->sb_argv = new_argv;
 return KE_NOMEM;
}


__crit kerrno_t
kshlib_shebang_new(struct kshlib **__restrict result,
                   struct kfile *__restrict sb_file) {
 struct kshlib *reslib;
 kerrno_t error; size_t bufsize;
 /* v TODO: Stack overflow when the interpreter is
  *         a shebang script over and over again. */
 char buffer[K_SHEBANG_MAXCMDLINE];
 char *bufpos,*bufend;
 error = kfile_kernel_read(sb_file,buffer,sizeof(buffer),&bufsize);
 if __unlikely(KE_ISERR(error)) return error;
 /* Search for a universal linefeed. ('\n', '\r', '\r\n') */
              bufend = (char *)memchr(buffer,'\n',bufsize);
 if (!bufend) bufend = (char *)memchr(buffer,'\r',bufsize);
 else if (bufend != buffer && bufend[-1] == '\r') --bufend;
 if (!bufend) {
  if (bufsize == sizeof(buffer)) return KE_NOEXEC; /*< Missing linefeed in first 4096 bytes of data (assume not-a-shebang) */
  bufend = buffer+bufsize; /*< Without a linefeed, but a short file all-together, assume one-line file and use its end as symbolic linefeed. */
 }
 /* Sanity checks. */
 if ((bufend-buffer) <= 2 || buffer[0] != '#' || buffer[1] != '!') return KE_NOEXEC;
 bufpos = buffer+2;
 assert(bufpos <= bufend);
 /* Allocate the result library. */
 reslib = (struct kshlib *)malloc(offsetafter(struct kshlib,sh_sb_args));
 if __unlikely(!reslib) return KE_NOMEM;
 kobject_init(reslib,KOBJECT_MAGIC_SHLIB);
 reslib->sh_refcnt = 1;
 reslib->sh_flags  = KMODFLAG_CANEXEC|KMODKIND_SHEBANG;
 error = kfile_incref(reslib->sh_file = sb_file);
 if __unlikely(KE_ISERR(error)) goto err_reslib;
 /* Cache the Shebang binary to prevent self-recursion. */
 error = kshlibcache_addlib(reslib);
 if __unlikely(KE_ISERR(error)) {
  reslib->sh_cidx = (__size_t)-1;
  k_syslogf_prefixfile(KLOG_WARN,sb_file,
                       "[Shebang] Failed to cache library file\n");
 }

 /* Initialize the argument vector. */
 error = ksbargs_init(&reslib->sh_sb_args,bufpos,(size_t)(bufend-bufpos));
 if __unlikely(KE_ISERR(error)) goto err_cache;

 /* Make sure there is at least one argument (the interpreter). */
 if __unlikely(!reslib->sh_sb_args.sb_argc) { error = KE_NOEXEC; goto err_args; }

 /* Load the interpreter binary described by the script file.
  * NOTE: Infinite recursion of a self-referencing library is
  *       prevented because we're caching the library, and don't
  *       have the 'KMODFLAG_LOADED' flag set. */
 error = kshlib_openexe(reslib->sh_sb_args.sb_argv[0],(size_t)-1,
                       &reslib->sh_sb_ref,KTASK_EXEC_FLAG_RESOLVEEXT);
 if __unlikely(KE_ISERR(error)) goto err_args;

 /* While the referenced library is a shebang script, try to recursively
  * merge optional arguments to speed up later invokation during exec(). */
 while (KMODKIND_KIND(reslib->sh_sb_ref->sh_flags) == KMODKIND_SHEBANG) {
  struct kshlib *myref = reslib->sh_sb_ref;
  /* Prepend all arguments. - If this fails,
   * simply ignore the error and stop collapsing. */
  if __unlikely(KE_ISERR(ksbargs_prepend(&reslib->sh_sb_args,
                         myref->sh_sb_args.sb_argc,
                        (char const *const *)myref->sh_sb_args.sb_argv))) break;
  /* Replace the current shebang reference with the underlying one. */
  reslib->sh_sb_ref = myref->sh_sb_ref;
  kshlib_decref(myref);
 }

 /* OK! Everything's checking out. - We've loaded the Shebang script! */
 reslib->sh_flags |= KMODFLAG_LOADED;
 *result = reslib;
 kshlibrecent_add(reslib);
 return error;
err_args:    ksbargs_quit(&reslib->sh_sb_args);
err_cache:   kshlibcache_dellib(reslib);
/*err_file:*/kfile_decref(sb_file);
err_reslib:  free(reslib);
 return error;
}


__DECL_END

#endif /* !__KOS_KERNEL_LINKER_SHEBANG_C__ */

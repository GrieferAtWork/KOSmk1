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
#ifndef __CMD_C__
#define __CMD_C__ 1

#include <lib/cmd.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <proc.h>
#include <sys/types.h>
#include <kos/fd.h>
#include <errno.h>

#include <stdio.h>

__DECL_BEGIN

static struct kwd {
 char const *name;
 size_t      size;
 int         type;
} const keywords[] = {
#define KEYWORD(name,id) {name,__compiler_STRINGSIZE(name),id}
 KEYWORD("if",CMD_TOKEN_KWD_IF),
 KEYWORD("then",CMD_TOKEN_KWD_THEN),
 KEYWORD("else",CMD_TOKEN_KWD_ELSE),
 KEYWORD("elif",CMD_TOKEN_KWD_ELIF),
 KEYWORD("fi",CMD_TOKEN_KWD_FI),
 KEYWORD("case",CMD_TOKEN_KWD_CASE),
 KEYWORD("esac",CMD_TOKEN_KWD_ESAC),
 KEYWORD("function",CMD_TOKEN_KWD_FUNCTION),
#undef KEYWORD
 {NULL,0,0}
};


__local void
check_keyword(struct cmd_token *__restrict token) {
 size_t kwd_size; struct kwd const *iter;
 assert(token->ct_kind == CMD_TOKEN_TEXT);
 kwd_size = (size_t)(token->ct_end-token->ct_begin);
 iter = keywords;
 while (iter->name) {
  if (iter->size == kwd_size &&
     !memcmp(iter->name,token->ct_begin,kwd_size*sizeof(char))) {
   token->ct_kind = iter->type;
   return;
  }
  ++iter;
 }
}



__public int
cmd_parser_yield(struct cmd_parser *__restrict self,
                 struct cmd_token *__restrict token) {
 char const *iter,*end; char ch;
 assert(self);
 assert(token);
 assert(isspace('\r'));
 assert(isspace('\n'));
 iter = self->cp_begin;
 end  = self->cp_end;
again:
 assert(iter <= end);
 /* Skip whitespace. */
 while (iter != end && (ch = *iter,isspace(ch))) {
  ++iter;
  if (ch == '\r') {
   if (iter != end && *iter == '\n') ++iter;
eol:
   token->ct_kind  = CMD_TOKEN_EOL;
   token->ct_begin = iter-1;
   token->ct_end   = iter;
   goto done;
  }
  if (ch == '\n') goto eol;
 }
 /* Check for EOF-tokens. */
 if __unlikely(iter == end) {
eof:
  token->ct_kind  = CMD_TOKEN_EOF;
  token->ct_begin = iter;
  token->ct_end   = iter;
  goto done;
 }
 token->ct_begin = iter;
 ch = *iter++;
 switch (ch) {
  case ';': goto eol;


  case '#':
   /* Line-comment; Skip until true eol */
   while (iter != end && (ch = *iter++,ch != '\n' && ch != '\r'));
   if (iter != end && ch == '\r' && *iter == '\n') ++iter;
   goto again;

  case '$':
   if (iter == end) goto eof;
   ch = *iter++;
   if (ch == '(') {
    /* Recursive inline command. */
    token->ct_kind = CMD_TOKEN_INLINE;
    ch = ')';
    goto scan_until_ch;
   } else if (ch == '{') {
    /* Explicit environment variable. */
    ch = '}';
    token->ct_kind = CMD_TOKEN_ENVVAR;
scan_until_ch:
    token->ct_begin = iter;
    while (iter != end && *iter != ch) ++iter;
    token->ct_end = iter;
    if (iter != end) ++iter;
   } else if (isalpha(ch)) {
    /* Implicit environment variable. */
    token->ct_kind = CMD_TOKEN_ENVVAR;
    token->ct_begin = iter-1;
    while (iter != end && isalnum(*iter)) ++iter;
    token->ct_end = iter;
   } else if (isdigit(ch)) {
    /* Argument reference. */
    token->ct_kind = CMD_TOKEN_ARG;
    token->ct_begin = iter-1;
    while (iter != end && (ch = *iter++,isdigit(ch)));
    token->ct_end = iter;
   } else if (ch == '?') {
    /* Expand errno/last error. */
    token->ct_kind = CMD_TOKEN_ERRNO;
   } else if (ch == '*') {
    /* Expand all arguments. */
    token->ct_kind = CMD_TOKEN_ARGS;
   } else {
    /* Undo (This is just a regular '$'). */
    --iter;
    goto def;
   }
   break;

  case '.':
  case '!':
   /* 1-char logical operator. */
   token->ct_kind = ch;
end_normal:
   token->ct_end = iter;
   break;

  case '>':
  case '<':
   token->ct_kind = CMD_TOKEN_REDIRECT;
   goto end_normal;

  case '&':
  case '|':
   /* 2-char logical operator. */
   if (iter == end || *iter != ch) goto def;
   token->ct_kind = ch,++iter;
   goto end_normal;

  case '\'':
  case '\"':
   /* Quotation marks. */
   token->ct_kind = (int)ch;
   ++token->ct_begin;
   while (iter != end && *iter != ch) ++iter;
   token->ct_end = iter;
   /* Skip final quote. */
   if (iter != end) ++iter;
   break;

  {
   int has_assign;
  default:def:
   has_assign = 0;
   token->ct_kind = CMD_TOKEN_TEXT;
   while (iter != end && (ch = *iter++,!isspace(ch))) {
    if (ch == '=') has_assign = 1;
    if (ch == '>') {
     if (iter != end && *iter == '>') ++iter;
     if (iter != end && *iter == '&') ++iter;
     token->ct_end  = iter;
     token->ct_kind = CMD_TOKEN_REDIRECT;
     goto done;
    }
   }
   token->ct_end = iter;
   if (has_assign) {
    token->ct_kind = CMD_TOKEN_SETENV;
   } else {
    check_keyword(token);
    if (token->ct_kind == CMD_TOKEN_KWD_FUNCTION) {
     self->cp_begin = iter;
     /* Parse the actual function name. */
     cmd_parser_yield(self,token);
     token->ct_kind = CMD_TOKEN_KWD_FUNCTION;
    } else if (token->ct_kind == CMD_TOKEN_TEXT) {
     if (token->ct_end >= token->ct_begin+2 &&
         token->ct_end[-2] == '(' &&
         token->ct_end[-1] == ')') {
      /* bash-style function definition. */
      token->ct_end -= 2;
      token->ct_kind = CMD_TOKEN_KWD_FUNCTION;
     }
    }
   }
  } break;
 }
done:
 self->cp_begin = iter;
 return token->ct_kind;
}


#define printl(s,l) \
do if __unlikely((error = (*printer)(s,l,closure))) return error;\
while(0)

__public int
cmd_expand(struct cmd_operations const *ops,
           char const *text, size_t maxlen,
           pformatprinter printer, void *closure,
           void *ops_closure) {
 char const *flush_start,*iter,*end; int error;
 char const *text_begin,*text_end;
 end = (iter = flush_start = text)+maxlen;
 for (; iter != end && *iter; ++iter) {
  if (*iter == '$' || *iter == '`') {
   if (iter != flush_start) {
    printl(flush_start,(size_t)(iter-flush_start));
   }
   ++iter;
   if (iter == end) break;
   if (iter[-1] == '$') {
    char mode = *iter++;
    if (mode == '{' || mode == '(') {
     int recursion = 1;
     char mode_end = mode == '{' ? '}' : ')';
     text_begin = iter;
     while (iter != end && *iter) {
           if (*iter == mode) ++recursion;
      else if (*iter == mode_end) {
       if (!--recursion) break;
      }
     }
     text_end = iter;
     if (iter != end) ++iter;
    } else {
     text_begin = iter-1;
     while (iter != end && isalnum(*iter)) ++iter;
     text_end = iter;
    }
    if (mode == '(') {
capture_stdout:
     /* Capture stdout */
     error = cmd_capturefd(ops,text_begin,
                          (text_end-text_begin),
                           printer,closure,
                           STDOUT_FILENO,
                           ops_closure);
     if __unlikely(error != 0) return error;
    } else {
     char const *envvar = (*ops->getenv)(text_begin,(size_t)(text_end-text_begin));
     if (envvar) printl(envvar,(size_t)-1);
     else {
      /* Restore the original text if the variable doesn't exist. */
      if (mode == '{') --text_begin;
      flush_start = text_begin;
      goto next;
     }
    }
   } else {
    assert(iter[-1] == '`');
    text_begin = iter;
    while (iter != end && *iter && *iter != '`') ++iter;
    text_end = iter;
    if (iter != end) ++iter;
    goto capture_stdout;
   }
   flush_start = iter;
  }
next:;
 }
 if (iter != flush_start) {
  printl(flush_start,(size_t)(iter-flush_start));
 }
 return 0;
}


__public int
cmd_fd_doall(size_t fdc, struct cmd_fd const *__restrict fdv) {
 struct cmd_fd const *iter,*end; int error;
 end = (iter = fdv)+fdc;
 for (; iter != end; ++iter) {
  if (iter->cf_filename) {
   error = kfd_open2(iter->cf_newfd,KFD_CWD,iter->cf_filename,iter->cf_filesize,iter->cf_filemode,0644);
   if __unlikely(KE_ISERR(error)) { __set_errno(-error); return -1; }
  } else {
   error = dup2(iter->cf_oldfd,iter->cf_newfd);
   if (error == -1) return error;
  }
 }
 return 0;
}


struct capture_exec_closure {
 int (*base_exec)(struct cmd_engine *engine,
                  struct cmd const *cmd,
                  uintptr_t *exitcode);
 int   capture_fd;
 int   redirected_fd;
};

static void cmdrun_addfd_alias(struct cmd_engine *__restrict self,
                               int old_fd, int new_fd);

static int capture_exec(struct cmd_engine *engine,
                        struct cmd const *cmd,
                        uintptr_t *exitcode) {
 struct capture_exec_closure *closure;
 closure = (struct capture_exec_closure *)engine->ce_internal;
 cmdrun_addfd_alias(engine,closure->capture_fd,
                    closure->redirected_fd);
 return (*closure->base_exec)(engine,cmd,exitcode);
}


__public int
cmd_capturefdf(struct cmd_operations const *__restrict ops,
               char const *__restrict text, size_t textsize,
               int capfd, int outputfd, void *ops_closure) {
 int error;
 struct capture_exec_closure exec_closure;
 struct cmd_operations custom_ops = *ops;
 struct cmd_engine engine = CMD_ENGINE_INIT(&custom_ops,ops_closure);
 custom_ops.exec = &capture_exec;
 engine.ce_internal = &exec_closure;
 error = cmd_engine_push(&engine,text,textsize,CMD_ENGINE_PUSH_MODE_ALIAS);
 if __likely(error == 0) error = cmd_engine_exec(&engine);
 cmd_engine_quit(&engine);
 return error;
}



__public int
cmd_capturefd(struct cmd_operations const *__restrict ops,
              char const *__restrict text, size_t textsize,
              pformatprinter printer, void *closure, int capfd,
              void *ops_closure) {
 int pipes[2],error;
 char buffer[256]; ssize_t rsize;
 if (pipe(pipes) == -1) return -1;
 error = cmd_capturefdf(ops,text,textsize,capfd,
                        pipes[1],ops_closure);
 kfd_close(pipes[1]);
 if (error == 0) {
  while ((rsize = read(pipes[0],buffer,sizeof(buffer))) > 0) {
   error = (*printer)(buffer,(size_t)rsize,closure);
   if __unlikely(error != 0) return error;
  }
  if __unlikely(rsize < 0) error = -1;
 }
 kfd_close(pipes[0]);
 return error;
}




static int
default_exec(struct cmd_engine *__unused(engine),
             struct cmd const *cmd,
             uintptr_t *exitcode) {
 task_t child_task; int error;
 if ((child_task = task_fork()) == 0) {
  struct kexecargs args;
  kerrno_t kerr;
  /* Execute fd-redirections. */
  cmd_fd_doall(cmd->c_fdc,cmd->c_fdv);
  /* Setup the arguments to ktask_exec. */
  args.ea_argc    = cmd->c_args.ca_argc;
  args.ea_argv    = cmd->c_args.ca_argv;
  args.ea_arglenv = cmd->c_args.ca_arglenv;
  args.ea_envc    = 0;
  args.ea_environ = NULL;
  args.ea_envlenv = NULL;
  kerr = ktask_exec(cmd->c_exe,cmd->c_esz,
                    &args,KTASK_EXEC_FLAG_SEARCHPATH);
  _exit(kerr == KE_NOENT ? 127 : 126);
 }
 if (child_task == -1) return -1;
 if (cmd->c_detached) {
  *exitcode = 0;
 } else {
  error = task_join(child_task,(void **)exitcode);
 }
 kfd_close(child_task);
 return error;
}
static int default_syntax_error(struct cmd_engine *__unused(engine),
                                int __unused(code)) {
 __set_errno(ESYNTAX);
 return -1;
}

__public struct cmd_operations const cmd_operations_default = {
 &_getnenv,
 &_setnenv,
 &default_exec,
 &default_syntax_error,
};


__public void
cmd_engine_quit(struct cmd_engine *__restrict self) {
 struct cmd_stack *iter,*end;
 char **siter,**send;
 assert(self);
 assert(!self->ce_top || (
         self->ce_top >= self->ce_stack_v &&
         self->ce_top <  self->ce_stack_v+
                         self->ce_stack_a));
 if (self->ce_top) {
  iter = self->ce_stack_v,end = self->ce_top+1;
  for (; iter != end; ++iter) free(iter->cs_text);
 }
 send = (siter = self->ca_ownv)+self->ca_ownc;
 for (; siter != send; ++siter) free(*siter);
 free(self->ca_ownv);
 free(self->ce_cmd.c_fdv);
 free(self->ce_cmd.c_args.ca_argv);
 free(self->ce_cmd.c_args.ca_arglenv);
 free(self->ce_stack_v);
}
__public void
cmd_engine_clear(struct cmd_engine *__restrict self) {
 self->ce_cmd.c_args.ca_argc = 0;
 self->ce_cmd.c_exe          = NULL;
 self->ce_cmd.c_fdc          = 0;
 self->ce_cmd.c_detached     = 0;
}


__public int
cmd_engine_yield(struct cmd_engine *__restrict self) {
 int result;
 assert(self);
again:
 if (!self->ce_top || self->ce_top < self->ce_stack_v) return CMD_TOKEN_EOF;
 result = cmd_parser_yield(&self->ce_top->cs_parser,
                           &self->ce_token);
 if (result == CMD_TOKEN_EOF) {
  /* Pop the top stack frame. */
  if (cmd_engine_pop(self)) return CMD_TOKEN_EOF;
  goto again;
 }
 return result;
}


__public int
cmd_engine_push(struct cmd_engine *__restrict self,
                char const *__restrict text,
                size_t text_size, int mode) {
 struct cmd_stack *newstack; size_t stacksize,stacktop;
 assert(self);
 if (self->ce_top == self->ce_stack_v+ self->ce_stack_a ||
     self->ce_top == self->ce_stack_v+(self->ce_stack_a-1)) {
  /* Must allocate more space on the stack vector. */
  stacksize = self->ce_stack_a ? self->ce_stack_a*2 : 2;
  stacktop = (size_t)(self->ce_top-self->ce_stack_v);
  newstack = (struct cmd_stack *)realloc(self->ce_stack_v,stacksize*
                                         sizeof(struct cmd_stack));
  if __unlikely(!newstack) return -1;
  self->ce_stack_v = newstack;
  self->ce_top     = newstack+stacktop;
  self->ce_stack_a = stacksize;
 }
 newstack = ++self->ce_top;
 switch (mode) {
  default:
  case CMD_ENGINE_PUSH_MODE_INHERIT:
   newstack->cs_text            = mode == CMD_ENGINE_PUSH_MODE_INHERIT ? (char *)text : NULL;
   newstack->cs_parser.cp_begin = text;
   newstack->cs_parser.cp_end   = text+text_size;
   break;
  case CMD_ENGINE_PUSH_MODE_COPY:
   newstack->cs_text            = (char *)memdup(text,text_size*sizeof(char));
   if __unlikely(!newstack->cs_text) { --self->ce_top; return -1; }
   newstack->cs_parser.cp_begin = newstack->cs_text;
   newstack->cs_parser.cp_end   = newstack->cs_text+text_size;
   break;
 }
 return 0;
}

__public int
cmd_engine_pushfd(struct cmd_engine *__restrict self, int fd) {
 pos_t fdsize; ssize_t rsize; int error;
 size_t text_off,textsize; char *text,*newtext;
 if (getattr(fd,KATTR_FS_SIZE,&fdsize,sizeof(fdsize)) == sizeof(fdsize)) {
  textsize = (size_t)fdsize;
 } else {
  textsize = 4096;
 }
 text = (char *)malloc((textsize+1)*sizeof(char));
 if __unlikely(!text) return -1;
 text_off = 0;
 while ((rsize = read(fd,text+text_off,(textsize+1)*sizeof(char))) > 0) {
  text_off += rsize;
  textsize -= rsize;
  if (!textsize) {
   /* Allocate more memory. */
   textsize = 4096;
   newtext  = (char *)realloc(text,(textsize+text_off+1)*sizeof(char));
   if __unlikely(!newtext) goto err_text;
   text = newtext;
  }
 }
 if (rsize < 0) {err_text: free(text); return -1; }
 error = cmd_engine_push(self,text,textsize,CMD_ENGINE_PUSH_MODE_INHERIT);
 if (error == -1) free(text);
 return error;
}

__public int
cmd_engine_pushfile(struct cmd_engine *__restrict self,
                    char const *__restrict filename, size_t filesize) {
 int error,fd = kfd_open(KFD_CWD,filename,filesize,O_RDONLY,0644);
 if __unlikely(KE_ISERR(fd)) { __set_errno(-fd); return -1; }
 error = cmd_engine_pushfd(self,fd);
 kfd_close(fd);
 return error;
}



__public int
cmd_engine_pop(struct cmd_engine *__restrict self) {
 assert(self);
 if (!self->ce_top) return 1;
 assert(self->ce_top >= self->ce_stack_v &&
        self->ce_top <  self->ce_stack_v+
                        self->ce_stack_a);
 if (self->ce_top->cs_text) free(self->ce_top->cs_text);
 if (self->ce_top == self->ce_stack_v) self->ce_top = NULL;
 else --self->ce_top;
 return 0;
}


#define TOK               (self->ce_token)
#define KIND              (self->ce_token.ct_kind)
#define TEXT              (self->ce_token.ct_begin)
#define SIZE     ((size_t)(self->ce_token.ct_end-self->ce_token.ct_begin))
#define IS_EMPTY         (!self->ce_cmd.c_exe)
#define YIELD()            cmd_engine_yield(self)
#define THROW()            longjmp(self->ce_jmp,1)
#define SYNTAX_ERROR(x) \
do{ if ((*self->ce_ops->syntax_error)(self,x)) THROW();\
}while(0)


static void cmdrun_exec(struct cmd_engine *__restrict self) {
 int error; uintptr_t errorcode;
 char **iter,**end;
 if (!self->ce_dontexec && self->ce_cmd.c_exe) {
  error = (*self->ce_ops->exec)(self,&self->ce_cmd,&errorcode);
  if (error == -1) errorcode = errno == ENOENT ? 127 : 126;
  self->ce_lasterr = errorcode;
 }
 self->ce_cmd.c_exe = 0;
 self->ce_cmd.c_fdc = 0;
 self->ce_cmd.c_args.ca_argc = 0;
 /* Free all owned strings. */
 end = (iter = self->ca_ownv)+self->ca_ownc;
 for (; iter != end; ++iter) free(*iter);
 self->ca_ownc = 0;
}

static int cmdrun_addownedstring(struct cmd_engine *__restrict self,
                                 char *string) {
 char **newstringv; size_t newa;
 if (self->ca_ownc == self->ca_owna) {
  newa = self->ca_owna ? self->ca_owna*2 : 2;
  newstringv = (char **)realloc(self->ca_ownv,newa*sizeof(char *));
  if __unlikely(!newstringv) return -1;
  self->ca_ownv = newstringv;
  self->ca_owna = newa;
 }
 self->ca_ownv[self->ca_ownc++] = string;
 return 0;
}

static void cmdrun_addarg(struct cmd_engine *__restrict self,
                          char const *arg, size_t arglen) {
 size_t new_arga;
 char const **newargv;
 size_t *newarglenv;
 assertf(self->ca_arga >= self->ce_cmd.c_args.ca_argc,
         "%Iu < %Iu",self->ca_arga,self->ce_cmd.c_args.ca_argc);
 if (self->ca_arga == self->ce_cmd.c_args.ca_argc) {
  /* Must allocate more arguments. */
  new_arga = self->ca_arga ? self->ca_arga*2 : 2;
  newargv = (char const **)realloc(self->ce_cmd.c_args.ca_argv,
                                   new_arga*sizeof(char const *));
  if __unlikely(!newargv) THROW();
  self->ce_cmd.c_args.ca_argv = newargv;
  newarglenv = (size_t *)realloc(self->ce_cmd.c_args.ca_arglenv,
                                 new_arga*sizeof(size_t));
  if __unlikely(!newarglenv) THROW();
  self->ce_cmd.c_args.ca_arglenv = newarglenv;
  self->ca_arga = new_arga;
 }
 new_arga = self->ce_cmd.c_args.ca_argc++;
 self->ce_cmd.c_args.ca_argv[new_arga]    = arg;
 self->ce_cmd.c_args.ca_arglenv[new_arga] = arglen;
 if (IS_EMPTY) {
  /* Re-use the first argument as executable. */
  self->ce_cmd.c_exe = arg;
  self->ce_cmd.c_esz = arglen;
 }
}
static void
cmdrun_addarg_expand(struct cmd_engine *__restrict self,
                     char const *arg, size_t arglen) {
 struct stringprinter printer; int error;
 char *text; size_t text_length;
 if (stringprinter_init(&printer,arglen)) THROW();
 error = cmd_expand(self->ce_ops,arg,arglen,
                    &stringprinter_print,&printer,
                    self->ce_user);
 if __unlikely(error) {
  stringprinter_quit(&printer);
  THROW();
 }
 text = stringprinter_pack(&printer,&text_length);
 error = cmdrun_addownedstring(self,text);
 if __unlikely(error) { free(text); THROW(); }
 cmdrun_addarg(self,text,text_length);
}

static __malloccall __retnonnull
struct cmd_fd *cmdrun_allocfd(struct cmd_engine *__restrict self) {
 size_t new_fda; struct cmd_fd *newfd;
 if (self->ca_fda == self->ce_cmd.c_fdc) {
  /* Must allocate more arguments. */
  new_fda = self->ca_fda ? self->ca_fda*2 : 2;
  newfd = (struct cmd_fd *)realloc(self->ce_cmd.c_fdv,
                                   new_fda*sizeof(struct cmd_fd));
  if __unlikely(!newfd) THROW();
  self->ce_cmd.c_fdv = newfd;
 } else {
  newfd = self->ce_cmd.c_fdv;
 }
 newfd += self->ce_cmd.c_fdc++;
 return newfd;
}
static void cmdrun_addfd_alias(struct cmd_engine *__restrict self,
                               int old_fd, int new_fd) {
 struct cmd_fd *slot = cmdrun_allocfd(self);
 slot->cf_newfd      = old_fd;
 slot->cf_oldfd      = new_fd;
 slot->cf_filename   = NULL;
 slot->cf_filesize   = 0;
 slot->cf_filemode   = 0;
}
static void cmdrun_addfd_redirect(struct cmd_engine *__restrict self,
                                  int fdno, char const *filename,
                                  size_t filesize, int openmode) {
 struct cmd_fd *slot = cmdrun_allocfd(self);
 slot->cf_newfd      = fdno;
 slot->cf_oldfd      = -1;
 slot->cf_filename   = filename;
 slot->cf_filesize   = filesize;
 slot->cf_filemode   = openmode;
}



static void cmdrun_unary(struct cmd_engine *__restrict self) {
 int error;
 if (IS_EMPTY) {
  switch (KIND) {
   /* Ignore empty commands. */
   case CMD_TOKEN_EOF:
   case CMD_TOKEN_EOL: break;

   case '!':
    YIELD();
    cmdrun_unary(self);
    break;

   {
    char const *filename;
    size_t      filesize;
   case CMD_TOKEN_DOT:
    YIELD();
    if (!SIZE) SYNTAX_ERROR(CMD_SYNTAXERROR_EXPECTED_FILENAME_AFTER_DOT);
    filename = TEXT;
    filesize = SIZE;
    YIELD();
    if (!CMD_TOKEN_ISEOL(KIND)) {
     SYNTAX_ERROR(CMD_SYNTAXERROR_EXPECTED_EOL_AFTER_DOT_FILENAME);
    }
    error = cmd_engine_pushfile(self,filename,filesize);
    if __unlikely(error) THROW();
    YIELD();
    break;
   }

   {
    char const *equal_ptr;
   case CMD_TOKEN_SETENV:
    equal_ptr = strnchr(TEXT,SIZE,'=');
    assert(equal_ptr);
    error = (*self->ce_ops->setenv)(TEXT,(size_t)(equal_ptr-TEXT),equal_ptr+1,
                                   (size_t)(TOK.ct_end-(equal_ptr+1)),1);
    if __unlikely(error) THROW();
    YIELD();
    if (!CMD_TOKEN_ISEOL(KIND)) {
     SYNTAX_ERROR(CMD_SYNTAXERROR_EXPECTED_EOL_AFTER_SETENV);
    }
    YIELD();
    break;
   }
   default: break;
  }
 }

 for (;;) switch (KIND) {
  case CMD_TOKEN_EOF:
  case CMD_TOKEN_EOL:
   goto finish_exec;
  case CMD_TOKEN_REDIRECT: {
   int fdno,fdno2,openmode,redirect_fileno;
   assert(SIZE);
   /* File descriptor redirection. */
   if (TEXT[0] == '<') {
    YIELD();
    if (!SIZE) SYNTAX_ERROR(CMD_SYNTAXERROR_EXPECTED_FILENAME_AFTER_LANGLE);
    fdno = STDIN_FILENO;
    openmode = O_RDONLY;
    redirect_fileno = 0;
    goto redirect_file;
   } else {
    char *iter = (char *)TEXT;
    char const *end = TOK.ct_end;
    if (isdigit(*iter)) fdno = strntos32(iter,SIZE,&iter,10);
    else fdno                = STDOUT_FILENO;
    openmode = O_WRONLY;
    if (iter != end && *iter == '>') ++iter;
    if (iter != end && *iter == '>') ++iter,openmode |= O_APPEND;
    redirect_fileno = (iter != end && *iter == '&');
redirect_file:
    YIELD();
    if (redirect_fileno) {
     if (!SIZE) SYNTAX_ERROR(CMD_SYNTAXERROR_EXPECTED_FILENO_AFTER_RANGLE);
     fdno2 = strntos32(TEXT,SIZE,NULL,10);
     cmdrun_addfd_alias(self,fdno,fdno2);
    } else {
     if (!SIZE) SYNTAX_ERROR(CMD_SYNTAXERROR_EXPECTED_FILENAME_AFTER_RANGLE);
     cmdrun_addfd_redirect(self,fdno,TEXT,SIZE,openmode);
    }
   }
  } break;

  case CMD_TOKEN_TEXT_QUOTE:
  case CMD_TOKEN_ENVVAR:
  case CMD_TOKEN_INLINE:
   cmdrun_addarg_expand(self,TEXT,SIZE);
   YIELD();
   break;

  default:
   cmdrun_addarg(self,TEXT,SIZE);
   YIELD();
   break;
 }
finish_exec:
 /* Execute the current command */
 cmdrun_exec(self);
}

static void cmdrun_land(struct cmd_engine *__restrict self) {
 cmdrun_unary(self);
 if (KIND == CMD_TOKEN_LAND) {
  if (self->ce_lasterr != EXIT_SUCCESS) ++self->ce_dontexec;
  YIELD();
  cmdrun_unary(self);
  if (self->ce_lasterr != EXIT_SUCCESS) --self->ce_dontexec;
 }
}
static void cmdrun_lor(struct cmd_engine *__restrict self) {
 cmdrun_land(self);
 if (KIND == CMD_TOKEN_LOR) {
  if (self->ce_lasterr == EXIT_SUCCESS) ++self->ce_dontexec;
  YIELD();
  cmdrun_land(self);
  if (self->ce_lasterr == EXIT_SUCCESS) --self->ce_dontexec;
 }
}


__public int
cmd_engine_exec(struct cmd_engine *__restrict self) {
 int error;
 assert(self);
 self->ca_arga = 0;
 self->ca_fda  = 0;
 self->ca_owna = 0;
 self->ca_ownc = 0;
 self->ca_ownv = NULL;
 /* Yield the initial token. */
 if (!YIELD()) return 0;
 if ((error = setjmp(self->ce_jmp)) == 0) {
  /* Execute full expressions until EOF is reached. */
  while (KIND) cmdrun_lor(self);
 }
 return error == 1 ? -1 : 0;
}
#undef YIELD
#undef KIND
#undef TOK



__public int cmd_system(char const *text, size_t size,
                        uintptr_t *errorcode) {
 int error;
 struct cmd_engine engine = CMD_ENGINE_INIT(&cmd_operations_default,NULL);
 error = cmd_engine_push(&engine,text,size,CMD_ENGINE_PUSH_MODE_ALIAS);
 if __likely(error == 0) error = cmd_engine_exec(&engine);
 if __likely(error == 0 && errorcode) *errorcode = engine.ce_lasterr;
 cmd_engine_quit(&engine);
 return error;
}


__DECL_END

#endif /* !__CMD_C__ */

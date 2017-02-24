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
#ifndef GUARD_LIB_CMD_H
#define GUARD_LIB_CMD_H 1

#include <kos/compiler.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <format-printer.h>

__DECL_BEGIN

struct cmd_token;
struct cmd_engine;
struct cmd_fd;
struct cmd_args;
struct cmd;
struct cmd_operations;

#define CMD_TOKEN_ISEOL(x) ((x) <= CMD_TOKEN_EOL)
#define CMD_TOKEN_EOF         0      /*< End of input. */
#define CMD_TOKEN_EOL         1      /*< End of line or ';'. */
#define CMD_TOKEN_TEXT        '\''   /*< Regular token text (e.g.: 'ls') variables not expanded; single-argument. */
#define CMD_TOKEN_TEXT_QUOTE  '\"'   /*< "foo" - style quoted text (token begin/end is quoted text; This text should be expanded again, but the output should only be one argument). */
#define CMD_TOKEN_REDIRECT    '>'    /*< '>', '<', '2>', '2>&', ...: Redirect input/output. */
#define CMD_TOKEN_DOT         '.'    /*< '.' */
#define CMD_TOKEN_LAND        '&'    /*< '&&': Logical and. */
#define CMD_TOKEN_LOR         '|'    /*< '||': Logical or */
#define CMD_TOKEN_NOT         '!'    /*< '!': Logical inversion */
#define CMD_TOKEN_ENVTEXT     '$'    /*< '$FOO' / '$(FOO)': Text containing '$'. */
#define CMD_TOKEN_FUNDEF      'F'    /*< 'function foo', 'foo()': Function definition (token begin/end is function name) */
#define CMD_TOKEN_ARG         'A'    /*< '$1': Argument reference. */
#define CMD_TOKEN_ERRNO       '?'    /*< '$?': Errno reference. */
#define CMD_TOKEN_ARGS        '*'    /*< '$*': All arguments expansion (Excluding arg0). */
#define CMD_TOKEN_SETENV      '='    /*< 'foo=bar': Set environment string (NOTE: The argument is the entire string). */

/* Keyword tokens. */
#define CMD_TOKEN_KWD_IF       0x100
#define CMD_TOKEN_KWD_THEN     0x101
#define CMD_TOKEN_KWD_ELSE     0x102
#define CMD_TOKEN_KWD_ELIF     0x103
#define CMD_TOKEN_KWD_FI       0x104
#define CMD_TOKEN_KWD_CASE     0x105
#define CMD_TOKEN_KWD_ESAC     0x106
#define CMD_TOKEN_KWD_FUNCTION 0x107 /*< Used internally. */

struct cmd_token {
 int         ct_kind;  /*< One of 'CMD_TOKEN_*'. */
 char const *ct_begin; /*< [1..1][maybe(null_if(ct_kind == CMD_TOKEN_EOF))] Pointer to the start of the token. */
 char const *ct_end;   /*< [1..1][maybe(null_if(ct_kind == CMD_TOKEN_EOF))] Pointer to the end of the token. */
};
#define CMD_TOKEN_INIT       {CMD_TOKEN_EOF,NULL,NULL}
#define cmd_token_init(self)  memset(self,0,sizeof(struct cmd_token))

struct cmd_parser {
 /* NOTE: The following two pointers must only be valid when non-equal. */
 char const *cp_begin; /*< [?..1] Input text begin (aka. current parser position). */
 char const *cp_end;   /*< [?..1] Input text end. */
};
#define CMD_PARSER_INIT      {NULL,NULL}
#define cmd_parser_init(self) memset(self,0,sizeof(struct cmd_parser))

//////////////////////////////////////////////////////////////////////////
// Yield one token from the given CMD parser.
// @return: *: The kind of the token ( == token->ct_kind).
extern __nonnull((1,2)) int
cmd_parser_yield(struct cmd_parser *__restrict self,
                 struct cmd_token *__restrict token);



//////////////////////////////////////////////////////////////////////////
// Expand environment variables $... and ${...}, as well as $(...) and `...`-style
// text, consecutively writing all output text to the given printer.
// @param: maxlen: strnlen-style max text length.
// @return: 0: Successfully printed everything.
// @return: *: First non-ZERO return value of the given printer.
extern __nonnull((1,2,4)) int
cmd_expand(struct cmd_operations const *__restrict ops,
           char const *__restrict text, size_t maxlen,
           pformatprinter printer, void *closure,
           void *ops_closure);



struct cmd_fd {
 int         cf_newfd;    /*< New file descriptor number in the child process. */
 int         cf_oldfd;    /*< Old file descriptor number in the child process (dup2-style). */
 char const *cf_filename; /*< [0..cf_filesize] Filename to use as output. (When non-null, ignore 'cf_oldfd'). */
 size_t      cf_filesize; /*< Amount of characters to use for the filename. */
 int         cf_filemode; /*< 'O_*'-style mode to open the given filename with. */
};

//////////////////////////////////////////////////////////////////////////
// Execute all redirections, as indicated by
// the given fd-vector within the calling process.
extern int cmd_fd_doall(size_t fdc, struct cmd_fd const *__restrict fdv);

struct cmd_args {
 size_t       ca_argc;    /*< Amount of arguments. */
 char const **ca_argv;    /*< [1..ca_arglenv[*]][0..ca_argc] Vector of arguments. */
 size_t      *ca_arglenv; /*< [0..ca_argc] Vector argument string lengths. */
};

struct cmd {
 size_t          c_esz;      /*< Amount of characters within the 'c_exe' string. */
 char const     *c_exe;      /*< [1..c_esz] Name of the executable to run. */
 size_t          c_fdc;      /*< Amount of file descriptor overrides. */
 struct cmd_fd  *c_fdv;      /*< [0..c_fdc] Vector of file descriptor overrides. */
 struct cmd_args c_args;     /*< Arguments. */
 int             c_detached; /*< When non-zero, don't join the child process. */
};
#define CMD_INIT  {0,NULL,0,NULL,{0,NULL,NULL},0}


#define CMD_SYNTAXERROR_NONE                            0
#define CMD_SYNTAXERROR_EXPECTED_FILENAME_AFTER_LANGLE  1 /*< 'cat < foo': Missing filename after '<' */
#define CMD_SYNTAXERROR_EXPECTED_FILENAME_AFTER_RANGLE  2 /*< 'cat > foo': Missing filename after '>' */
#define CMD_SYNTAXERROR_EXPECTED_FILENO_AFTER_RANGLE    3 /*< 'cat 2>&1': Missing fileno after '[int]>&' */
#define CMD_SYNTAXERROR_EXPECTED_FILENAME_AFTER_DOT     4 /*< '. script.sh': Missing filename. */
#define CMD_SYNTAXERROR_EXPECTED_EOL_AFTER_DOT_FILENAME 5 /*< '. script.sh': Missing end-of-line. */
#define CMD_SYNTAXERROR_EXPECTED_EOL_AFTER_SETENV       6 /*< 'FOO=BAR': Missing end-of-line. */

struct cmd_operations {
 char *(*getenv)(char const *envname, size_t envnamelen);
 int   (*setenv)(char const *envname, size_t envnamelen,
                 char const *envval, size_t envvallen,
                 int overwrite);
 /* Execute the given command. */
 int   (*exec)(struct cmd_engine *engine, struct cmd const *cmd, uintptr_t *exitcode);
 /* Handle a syntax error, as specified by 'code'. */
 int   (*syntax_error)(struct cmd_engine *engine, int code);
};

/* Default CMD operations (using the environment of the
 * calling process & fork()+exec() to execute commands). */
extern struct cmd_operations const cmd_operations_default;


struct cmd_stack {
 char             *cs_text;   /*< [0..1][owned] Pointer to the original text start, or NULL if the text of this entry isn't owned. */
 struct cmd_parser cs_parser; /*< Parser associated with this stack entry. */
};

struct cmd_engine {
 struct cmd_token             ce_token;    /*< Next token. */
 struct cmd_operations const *ce_ops;      /*< [1..1] Engine callbacks. */
 struct cmd_stack            *ce_top;      /*< [1..1][in(ce_parser_v)] Currently selected stack entry. */
 size_t                       ce_stack_a;  /*< Allocated amount of stack entries. */
 struct cmd_stack            *ce_stack_v;  /*< [0..ce_stack_a][owned] Vector of available parsers. */
 uintptr_t                    ce_lasterr;  /*< Errorcode returned by the last execution of a command (NOTE: Must not be changed when 'ce_dontexec' is non-zero). */
 int                          ce_dontexec; /*< When non-zero, don't actually execute commands (Set when parsing inactive code). */
 struct cmd                   ce_cmd;      /*< Command currently being executed. */
 void                        *ce_user;     /*< [?..?] User-defined pointer (aka. closure). */
 jmp_buf                      ce_jmp;      /*< Used internally. */
 size_t                       ca_arga;     /*< Used internally. (Allocated argument vector size). */
 size_t                       ca_fda;      /*< Used internally. (Allocated FD redirect vector size). */
 size_t                       ca_owna;     /*< Used internally. (Allocated amount of owned strings). */
 size_t                       ca_ownc;     /*< Used internally. (Amount of owned strings). */
 char                       **ca_ownv;     /*< Used internally. ([0..ca_owna] Vector of owned strings). */
 void                        *ce_internal; /*< [?..?] Internally used pointer. */
};

#define cmd_engine_token(self) (&(self)->ce_token)

#define CMD_ENGINE_INIT(ops,user) \
 {CMD_TOKEN_INIT,ops,NULL,0,NULL,EXIT_SUCCESS,0,\
  CMD_INIT,user,_JMP_BUF_STATIC_INIT,0,0,0,0,NULL,NULL}
#define cmd_engine_init(self,ops,user) \
 (memset(self,0,sizeof(struct cmd_engine)),\
 (self)->ce_ops = (ops),(self)->ce_user = (user))

extern __nonnull((1)) void cmd_engine_quit(struct cmd_engine *__restrict self);

//////////////////////////////////////////////////////////////////////////
// Same as 'cmd_parser_yield', but in the event of 'CMD_TOKEN_EOF',
// this function will attempt to pop a stack entry and unless it
// was the last, will try to yield another token from the previous.
// @return: *: The kind of the token ( == cmd_engine_token(self)->ct_kind).
extern __nonnull((1)) int cmd_engine_yield(struct cmd_engine *__restrict self);


//////////////////////////////////////////////////////////////////////////
// Push the given text as a new stack entry.
// @param: copy:      One of 'CMD_ENGINE_PUSH_MODE_*'
// @param: text_size: The exact length of the given text (_NOT_ an strnlen-style length!)
// @return: 0:  Successfully pushed the given text.
// @return: -1: An error occurred (see 'errno' for more information)
extern __nonnull((1,2)) int
cmd_engine_push(struct cmd_engine *__restrict self,
                char const *__restrict text,
                size_t text_size, int mode);
#define CMD_ENGINE_PUSH_MODE_ALIAS   0 /*< Use an alias of the given text internally. (Caller must keep text alive for the lifetime of the engine). */
#define CMD_ENGINE_PUSH_MODE_COPY    1 /*< Use a copy of the given text internally. */
#define CMD_ENGINE_PUSH_MODE_INHERIT 2 /*< Inherit the given text upon success, using 'free()' to deallocate it later on. */

//////////////////////////////////////////////////////////////////////////
// Read the entirety of the given file 'fd' and push its contents.
// NOTE: Same as the bash-command '. <filename>'
extern __nonnull((1)) int cmd_engine_pushfd(struct cmd_engine *__restrict self, int fd);
extern __nonnull((1,2)) int
cmd_engine_pushfile(struct cmd_engine *__restrict self,
                    char const *__restrict filename, size_t filesize);

//////////////////////////////////////////////////////////////////////////
// Pop one stack entry.
// @return: 0: Successfully poped one entry.
// @return: 1: No more entries to pop.
extern __nonnull((1)) int cmd_engine_pop(struct cmd_engine *__restrict self);


//////////////////////////////////////////////////////////////////////////
// Execute all frames within the given CMD-engine.
extern __nonnull((1)) int cmd_engine_exec(struct cmd_engine *__restrict self);


//////////////////////////////////////////////////////////////////////////
// Perform a system()-style command execution,
extern __nonnull((1)) int cmd_system(char const *text, size_t size,
                                     uintptr_t *errorcode);

//////////////////////////////////////////////////////////////////////////
// Execute the given text, capturing all output from the given
// file descriptor and outputting it to the provided printer.
// @param: ops_closure: Closure value as available to 'ops'
extern __nonnull((1,2,4)) int
cmd_capturefd(struct cmd_operations const *__restrict ops,
              char const *__restrict text, size_t textsize,
              pformatprinter printer, void *closure, int capfd,
              void *ops_closure);
extern __nonnull((1,2)) int
cmd_capturefdf(struct cmd_operations const *__restrict ops,
               char const *__restrict text, size_t textsize,
               int capfd, int outputfd, void *ops_closure);


__DECL_END

#endif /* !GUARD_LIB_CMD_H */

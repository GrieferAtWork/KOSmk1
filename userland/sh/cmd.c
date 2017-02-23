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
#ifndef GUARD_CMD_C
#define GUARD_CMD_C 1

#include "sh.h"
#include "cmd.h"
#include "builtin.h"
#include <errno.h>
#include <unistd.h>
#include <stdio.h>


static int run_builtin(struct shcmd const *builtin,
                       struct cmd const *cmd) {
 char **fixed_argv,**iter,**end; int result;
 char const **src; size_t *len;
 fixed_argv = (char **)malloc((cmd->c_args.ca_argc+1)*sizeof(char *));
 if __unlikely(!fixed_argv) return -1;
 end = (iter = fixed_argv)+cmd->c_args.ca_argc;
 src = cmd->c_args.ca_argv;
 len = cmd->c_args.ca_arglenv;
 for (; iter != end; ++iter,++src,++len) {
  if __unlikely((*iter = strndup(*src,*len)) == NULL) {
   result = -1;
   memset(iter,0,(end-iter)*sizeof(char *));
   goto end;
  }
 }
 result = (*builtin->main)((int)cmd->c_args.ca_argc,fixed_argv);
end:
 end = (iter = fixed_argv)+cmd->c_args.ca_argc;
 for (; iter != end; ++iter) free(*iter);
 free(fixed_argv);
 return result;
}

int shcmd_exec(struct cmd_engine *engine,
               struct cmd const *cmd,
               uintptr_t *exitcode) {
 struct shcmd const *iter; int error;
 if (verbose) dprintf(STDERR_FILENO,"exec: %.*q\n",
                     (unsigned)cmd->c_esz,cmd->c_exe);
 for (iter = shbuiltin; iter->name; ++iter) {
  if (iter->size == cmd->c_esz &&
     !memcmp(iter->name,cmd->c_exe,cmd->c_esz*sizeof(char))) {
   *exitcode = (uintptr_t)run_builtin(iter,cmd);
   return 0;
  }
 }
 error = (*cmd_operations_default.exec)(engine,cmd,exitcode);
 return error;
}

int shcmd_syntax_error(struct cmd_engine *__unused(engine), int code) {
 char const *message;
 switch (code) {
  case CMD_SYNTAXERROR_EXPECTED_FILENAME_AFTER_LANGLE : message = "Missing filename after '<'"; break;
  case CMD_SYNTAXERROR_EXPECTED_FILENAME_AFTER_RANGLE : message = "Missing filename after '>'"; break;
  case CMD_SYNTAXERROR_EXPECTED_FILENO_AFTER_RANGLE   : message = "Missing fileno after '...>&'"; break;
  case CMD_SYNTAXERROR_EXPECTED_FILENAME_AFTER_DOT    : message = "Missing filename after '.'"; break;
  case CMD_SYNTAXERROR_EXPECTED_EOL_AFTER_DOT_FILENAME: message = "Expected end-of-line after '. <filename>'"; break;
  case CMD_SYNTAXERROR_EXPECTED_EOL_AFTER_SETENV      : message = "Expected end-of-line after 'FOO=BAR'"; break;
  default: message = "Unknown error"; break;
 }
 dprintf(STDERR_FILENO,"Syntax error: %s\n",message);
 errno = ESYNTAX;
 return -1;
}

struct cmd_operations const shcmd_operations = {&_getnenv,&_setnenv,&shcmd_exec,&shcmd_syntax_error};
struct cmd_engine shcmd_engine = CMD_ENGINE_INIT(&shcmd_operations,NULL);


uintptr_t shcmd_system(char const *command, size_t command_size) {
 int error; uintptr_t result;
 cmd_engine_clear(&shcmd_engine);
 error = cmd_engine_push(&shcmd_engine,command,command_size,
                         CMD_ENGINE_PUSH_MODE_ALIAS);
 if (error == 0) error = cmd_engine_exec(&shcmd_engine);
 if (error != 0) error = errno;
 /* Translate errno to bash-errors:
  * http://www.tldp.org/LDP/abs/html/exitcodes.html
  */
      if (error == ESYNTAX) result = 2;
 else if (error == ENOEXEC ||
          error == ENODEP)  result = 126;
 else if (error == ENOENT)  result = 127;
 else if (error != EOK)     result = 1;
 else return SH_LASTERROR;
 SH_LASTERROR = result;
 return result;
}



#endif /* !GUARD_CMD_C */

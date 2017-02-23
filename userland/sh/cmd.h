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
#ifndef GUARD_CMD_H
#define GUARD_CMD_H 1

#include <lib/cmd.h>

extern int shcmd_exec(struct cmd_engine *engine, struct cmd const *cmd, uintptr_t *exitcode);
extern int shcmd_syntax_error(struct cmd_engine *engine, int code);
extern struct cmd_operations const shcmd_operations;
extern struct cmd_engine           shcmd_engine;

#define SH_LASTERROR   (shcmd_engine.ce_lasterr)

/* Execute a top-level (stdlib:system()-style) command.
 * NOTE: 'command_size' is the exact amount of characters to parse. */
extern uintptr_t shcmd_system(char const *command, size_t command_size);


#endif /* !GUARD_CMD_H */
